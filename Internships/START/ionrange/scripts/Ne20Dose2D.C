#include <filesystem>
#include <algorithm>
#include <cmath>
#include <TFile.h>
#include <random>

namespace fs = std::filesystem;

double computeIntegral(TGraph*);

void Ne20Dose2D(){
	double nm, um, mm, m, eV;
	m =  1e9; 		//you can change this value
	mm = 1e-3*m;	// ... but not this!
	um = 1e-6*m;	// ... but not this!
	nm = 1e-9*m;	// ... but not this!
	eV = 1.602e-19;

	std::string path, path1, path2, path3, path4, path5;
	
	path = "root/simplified-ionrange-Ne20-163.46_MeV-DNA8.root";
	TFile *f =  new TFile(path.c_str());
	
	TTree *tree = (TTree*)f->Get("2DdoseData");


	std::string ionName = path.substr(25,-1);
	size_t nameIndex = std::find(begin(ionName), end(ionName), '-')-begin(ionName);
	std::string energyName = ionName.substr(nameIndex+1, -1);
	ionName = ionName.substr(0, nameIndex);
	
	size_t energyIndex = std::find(begin(energyName), end(energyName), '-') -begin(energyName);
	energyName = energyName.substr(0, energyIndex);
	std::replace(begin(energyName), end(energyName), '_', ' ');

	const std::string canvasName = "Dose distribution on XY surface of " + ionName + " at E=" + energyName;
	std::cout << canvasName << std::endl;
	TCanvas *c = new TCanvas("c", canvasName.c_str(),
							  1920, 1080);
    
					  
	double x, y, dose2D;
	
	tree->SetBranchAddress("x", &x);
	tree->SetBranchAddress("y", &y);
	tree->SetBranchAddress("dose2D", &dose2D);
	
	int nPoints = tree->GetEntries(); 
	TGraph2D* graph = new TGraph2D();
	for (Long64_t i=0; i <= nPoints; ++i){
		tree->GetEntry(i);
		graph->SetPoint(graph->GetN(), x, y, dose2D);
	}
    
	graph->SetTitle(canvasName.c_str());
	graph->SetLineWidth(1);
	//graph->SetLineColor(kBlue-5);
	graph->Draw("surf1");
	//graph->Draw("lego2");
	gPad->Update();
	gPad->Modified();
	TAxis *Xaxis = graph->GetXaxis();
    TAxis *Yaxis = graph->GetYaxis();
    TAxis *Zaxis = graph->GetZaxis();
    Xaxis->SetTitle("X [nm]"); 		Xaxis->CenterTitle(true); 	Xaxis->SetTitleOffset(1.5);
    Yaxis->SetTitle("Y [nm]"); 		Yaxis->CenterTitle(true);	Yaxis->SetTitleOffset(1.5);
    Zaxis->SetTitle("Dose [Gy]"); 	Zaxis->CenterTitle(true);	Zaxis->SetTitleOffset(1.);
	
	gPad->SetLogz();
	gStyle->SetPalette(1); 
	gPad->GetFrame()->SetBorderSize(0);
	gPad->GetFrame()->SetLineColor(kWhite);
	
	c->Update();
	std::string folderName = "plots";
	if (!fs::exists(folderName)){
		fs::create_directory(folderName);
	}
	
	std::replace(begin(energyName), end(energyName), ' ', '_');
	const std::string fileName = ionName + "-" + energyName + "-" + "Dose2D";
	c->SaveAs(("plots/" + fileName + ".png").c_str());
};

double computeIntegral(TGraph* graph){
	const int nPoints = graph->GetN();
	auto x = graph->GetX();
	auto y = graph->GetY();
	
	double integral = .0;
	double u1, u2;
	for (size_t i = 0; i < nPoints; i++){
		if (x[i] > 0 and y[i] > 0){
			u1 = 2*M_PI*x[i]*y[i];
			u2 = 2*M_PI*x[i+1]*y[i+1];
			u1 = 1/1.602*1e19/1e24*u1; 
			u2 = 1/1.602*1e19/1e24*u2;
			integral += (x[i+1]-x[i])*(u1 + u2)/2;
		}
	}
	
	return integral;
}
