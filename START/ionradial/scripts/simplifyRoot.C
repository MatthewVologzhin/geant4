#include <TFile.h>
#include <TTree.h>
#include <algorithm>
#include <cmath>
#include <filesystem>

namespace fs = std::filesystem;

void simplifyRoot(const std::string& path){
	double nm, um, mm, m, eV;
	m =  1e9; 		//you can change this value
	mm = 1e-3*m;	// ... but not this!
	um = 1e-6*m;	// ... but not this!
	nm = 1e-9*m;	// ... but not this!
	eV = 1.602e-19;
	
	TFile *f =  new TFile(path.c_str());
	
	TNtuple *tupleDose = (TNtuple*)f->Get("doseData");

	std::string ionName = path.substr(14,-1);
	size_t nameIndex = std::find(begin(ionName), end(ionName), '-')-begin(ionName);
	std::string energyName = ionName.substr(nameIndex+1, -1);
	ionName = ionName.substr(0, nameIndex);
	
	size_t energyIndex = std::find(begin(energyName), end(energyName), '-') -begin(energyName);
	energyName = energyName.substr(0, energyIndex);
	std::replace(begin(energyName), end(energyName), '_', ' ');
	
	TNtuple *tupleIonData  = (TNtuple*)f->Get("ionInitialData");						  
	double ionsNbEntry, ionsEnergyEntry;
	double ionsNb{}, ionsEnergy{};
	tupleIonData->SetBranchAddress("number", &ionsNbEntry);
	tupleIonData->SetBranchAddress("energy", &ionsEnergyEntry);
	//tupleIonData->SetBranchAddress("LET", &LETvalue);
	Long64_t nEntriesIon = tupleIonData->GetEntries();
	for (Long64_t i=0; i<nEntriesIon; ++i){
		tupleIonData->GetEntry(i);
		ionsNb += ionsNbEntry;
		ionsEnergy += ionsEnergyEntry;
	}
	
	TNtuple *tupleTotalLET  = (TNtuple*)f->Get("totalLET");						  
	double LETvalue;
	size_t nEntriesLET = tupleTotalLET->GetEntries();
	tupleTotalLET->SetBranchAddress("LET", &LETvalue);
	
	
	std::string new_path = "root/simplified-" + path.substr(5, -1);
	double energyDep, axialRadius, x, y, z, deltaZ, binWidth, minRadius, maxRadius;
	double radialDose, dose2D, LET, treeLETvalue;
	double minX, maxX, minY, maxY, minZ, maxZ;
	double summaryEDep;
	TFile* newFile = new TFile(new_path.c_str(), "RECREATE");
	TTree* treeRadialDose = new TTree("radialDoseData", "Data for plot creation");
	TTree* treeLETdata = new TTree("LETdata", "LET data");
	TTree* treeDose2D = new TTree("2DdoseData", "Data for 2D plot creation");
	treeRadialDose->Branch("radius", &axialRadius, "radius/D");
	treeRadialDose->Branch("radialDose", &radialDose, "dose/D");
	treeRadialDose->Branch("LET", &LET, "LET/D");
	treeLETdata->Branch("LET", &treeLETvalue, "LET/D");
	treeDose2D->Branch("x", &x, "x/D");
	treeDose2D->Branch("y", &y, "y/D");
	treeDose2D->Branch("dose2D", &dose2D, "dose2D/D");
	
	
	long nBins, nBinsZ;
	tupleDose->SetBranchAddress("energy", &energyDep);
	tupleDose->SetBranchAddress("radius", &axialRadius);
	tupleDose->SetBranchAddress("x", &x);
	tupleDose->SetBranchAddress("y", &y);
	tupleDose->SetBranchAddress("z", &z);
	minRadius = 0.01; // Else we have the problem with logarithmic scale at r=0
	maxRadius = tupleDose->GetMaximum("radius");
	minX = tupleDose->GetMinimum("x");
	maxX = tupleDose->GetMaximum("x");
	minY = tupleDose->GetMinimum("y");
	maxY = tupleDose->GetMaximum("y");
	minZ = tupleDose->GetMinimum("z");
	maxZ = tupleDose->GetMaximum("z");
	deltaZ = maxZ - minZ;
	std::cout << "Minimum radius: " << minRadius  << " nm" << std::endl;
	std::cout << "Maximum radius: " << maxRadius << " nm" << std::endl;
	std::cout << "Minumum X: " << minX << " nm" << std::endl;
	std::cout << "Maximum X: " << maxX << " nm" << std::endl;
	std::cout << "Minumum Y: " << minY << " nm" << std::endl;
	std::cout << "Maximum Y: " << maxY << " nm" << std::endl;
	std::cout << "Minumum Z: " << minZ << " nm" << std::endl;
	std::cout << "Maximum Z: " << maxZ << " nm" << std::endl;
	
	//Logarithmic scale
	nBins = 200;
	double* bins = new double[nBins + 1];
    double logMin = TMath::Log10(minRadius);
    double logMax = TMath::Log10(maxRadius);
    for (int i = 0; i <= nBins; ++i){
        bins[i] = TMath::Power(10, logMin + i * (logMax - logMin) / nBins);
    }
    
    
	TH1F* h1RadialDose = new TH1F("radialDose", "Electrons radial dose distribution", nBins, bins);
	TH2F* h2Dose = new TH2F("dose2D", "XY Dose distribution", nBins, minX, maxX, nBins, minY, maxY);
	
	
	double density = 1000/(m*m*m);
	double tubeVolume, cubeVolume;
	double innerRadius, outerRadius;
	Long64_t nEntries = tupleDose->GetEntries();
	for (Long64_t i=0; i<nEntries; i++){
		tupleDose->GetEntry(i);
		for (int i = 0; i < nBins; ++i){
			if ((bins[i] < axialRadius) and (bins[i+1] > axialRadius)){
					binWidth = bins[i+1] - bins[i];
					innerRadius = bins[i];
					outerRadius = bins[i+1];
					summaryEDep += energyDep;
					tubeVolume = M_PI*(outerRadius*outerRadius-innerRadius*innerRadius)*deltaZ;
					cubeVolume = (maxX-minX)*(maxY-minY)/(nBins*nBins)*deltaZ;
					radialDose = energyDep/(density*tubeVolume*ionsNb);
					dose2D = 	 energyDep/(density*cubeVolume*ionsNb);
					h1RadialDose->Fill(axialRadius, radialDose);
					h2Dose->Fill(x, y, dose2D);
					break;
			}
		}
	}
	
	nBins = h1RadialDose->GetNbinsX();
	for (Long64_t i=1; i <= nBins; ++i){
		axialRadius = h1RadialDose->GetBinCenter(i);
		radialDose = h1RadialDose->GetBinContent(i);
		LET = (summaryEDep/(eV))/(deltaZ*ionsNb)/nBins;
		treeRadialDose->Fill();
	}
	
	
	for (Long64_t i=1; i <= nEntriesLET; ++i){
		tupleTotalLET->GetEntry(i);
		treeLETvalue = LETvalue;
		treeLETdata->Fill();
	}
	
	int nBinsX = h2Dose->GetNbinsX();
	int nBinsY = h2Dose->GetNbinsY(); 
	for (Long64_t i=1; i <= nBinsX; ++i){
		for (Long64_t j=1; j <= nBinsY; ++j){
			x = h2Dose->GetXaxis()->GetBinCenter(i);
			y = h2Dose->GetYaxis()->GetBinCenter(j);
			dose2D = h2Dose->GetBinContent(i, j);
			treeDose2D->Fill();
		}
	}
	
	std::cout << "LET (only e-) = " << LET*nBins << " keV/um" << std::endl;
	
	treeRadialDose->Write();
	treeDose2D->Write();
	treeLETdata->Write();
	
	newFile->Close();
};
