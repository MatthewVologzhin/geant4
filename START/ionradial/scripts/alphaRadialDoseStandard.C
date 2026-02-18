#include <filesystem>
#include <algorithm>
#include <cmath>
#include <TFile.h>
#include <random>

namespace fs = std::filesystem;

double computeIntegral(TGraph*);

void alphaRadialDoseStandard(){
	double nm, um, mm, m, MeV;
	m =  1e9; 		//you can change this value
	mm = 1e-3*m;	// ... but not this!
	um = 1e-6*m;	// ... but not this!
	nm = 1e-9*m;	// ... but not this!
	MeV = 1e6*1.602e-19;

	std::string path1, path2, path3, path4, path5;
	
	path1 = "root/simplified-ionradial-alpha-3_MeV-S4-withIon.root";	
	path2 = "root/simplified-ionradial-alpha-3_MeV-S4-withoutMsc.root";
	path3 = "root/simplified-ionradial-alpha-3_MeV-S4-withoutIon.root";
	path4 = "root/simplified-ionradial-alpha-3_MeV-S4-withoutMscIon.root";
	
	TFile *f1 =  new TFile(path1.c_str());
	TFile *f2 =  new TFile(path2.c_str());
	TFile *f3 =  new TFile(path3.c_str());
	TFile *f4 =  new TFile(path4.c_str());
	
	TTree *tree1 = (TTree*)f1->Get("radialDoseData");
	TTree *tree2 = (TTree*)f2->Get("radialDoseData");
	TTree *tree3 = (TTree*)f3->Get("radialDoseData");
	TTree *tree4 = (TTree*)f4->Get("radialDoseData");
	

	std::string ionName = path1.substr(25,-1);
	size_t nameIndex = std::find(begin(ionName), end(ionName), '-')-begin(ionName);
	std::string energyName = ionName.substr(nameIndex+1, -1);
	ionName = ionName.substr(0, nameIndex);
	
	size_t energyIndex = std::find(begin(energyName), end(energyName), '-') -begin(energyName);
	energyName = energyName.substr(0, energyIndex);
	std::replace(begin(energyName), end(energyName), '_', ' ');

	const std::string canvasName = "Radial dose distribution of " + ionName + " at E=" + energyName + " in Standard Opt4";
	std::cout << canvasName << std::endl;
	TCanvas *c1 = new TCanvas("c", canvasName.c_str(),
							  1920, 1080);
    gPad->SetLogx();
    gPad->SetLogy(); 
	
					  
	double axialRadius1, dose1;
	double axialRadius2, dose2;
	double axialRadius3, dose3;
	double axialRadius4, dose4;
	
	tree1->SetBranchAddress("radius", &axialRadius1);
	tree1->SetBranchAddress("radialDose", &dose1);
	tree2->SetBranchAddress("radius", &axialRadius2);
	tree2->SetBranchAddress("radialDose", &dose2);
	tree3->SetBranchAddress("radius", &axialRadius3);
	tree3->SetBranchAddress("radialDose", &dose3);
	tree4->SetBranchAddress("radius", &axialRadius4);
	tree4->SetBranchAddress("radialDose", &dose4);
	
	int nPoints1 = tree1->GetEntries(); 
	TGraph* graph1 = new TGraph();
	for (Long64_t i=0; i <= nPoints1; ++i){
		tree1->GetEntry(i);
		if (axialRadius1 >= 0 and dose1 >= 0){
			graph1->SetPoint(graph1->GetN(), axialRadius1, dose1);
		}
	}
	graph1->SetLineWidth(2);
	graph1->SetLineColor(kOrange);
	graph1->GetXaxis()->SetLimits(0.9, 2*1e2);
	graph1->GetXaxis()->SetRangeUser(0.9, 2*1e2);
	graph1->GetYaxis()->SetLimits(1, 1e6);
	graph1->GetYaxis()->SetRangeUser(1, 1e6);
	graph1->SetTitle(canvasName.c_str());
	TAxis *Xaxis = graph1->GetXaxis();
    TAxis *Yaxis = graph1->GetYaxis();
	Xaxis->SetTitle("Distance from the axis [nm]");
	Yaxis->SetTitle("Dose [Gy]");
	Xaxis->CenterTitle(true); Xaxis->SetTitleOffset(1.2);
	Yaxis->CenterTitle(true); 
	graph1->Draw("AL");
	
	int nPoints2 = tree2->GetEntries(); 
	TGraph* graph2 = new TGraph();
	for (Long64_t i=0; i <= nPoints2; ++i){
		tree2->GetEntry(i);
		if (axialRadius2 >= 0 and dose2 >= 0){
			graph2->SetPoint(graph2->GetN(), axialRadius2, dose2);
		}
	}
	graph2->SetLineWidth(2);
	graph2->SetLineColor(kOrange+2);
	graph2->Draw("L SAME");
	
	
	int nPoints3 = tree3->GetEntries(); 
	TGraph* graph3 = new TGraph();
	for (Long64_t i=0; i <= nPoints3; ++i){
		tree3->GetEntry(i);
		if (axialRadius3 >= 0 and dose3 >= 0){
			graph3->SetPoint(graph3->GetN(), axialRadius3, dose3);
		}
	}
	graph3->SetLineColor(kRed);
	graph3->SetLineWidth(2);
	graph3->Draw("L SAME");
	
	int nPoints4 = tree4->GetEntries(); 
	TGraph* graph4 = new TGraph();
	for (Long64_t i=0; i <= nPoints4; ++i){
		tree4->GetEntry(i);
		if (axialRadius4 >= 0 and dose4 >= 0){
			graph4->SetPoint(graph4->GetN(), axialRadius4, dose4);
		}
	}
	graph4->SetLineColor(kBlue);
	graph4->SetLineWidth(2);
	graph4->Draw("L SAME");
							  
	//Alpha
	const size_t n1 = 13;
	double xData1[n1] = {
        0.9853947035711945, 1.9675678267752414, 2.970595032507586,
        3.928703025225193, 4.97147915739702, 6.108611311306012,
        7.076854320439322, 9.926704909340241, 19.82095614551727,
        29.92528799473532, 40.75901143926727, 50.08186707272078,
        55.51482123742945
    };

    double yData1[n1] = {
        334774.5810705731, 91284.28949429006, 46064.49735804079,
        24890.842912355743, 14401.769029678595, 9554.281212853728,
        6338.408101544674, 2272.142910168382, 312.64357272238095,
        104.66512108254256, 43.01930267113846, 18.93336131218539,
        10.23059729842507
    };
	
	
	TGraph* gpExp1 = new TGraph(n1, xData1, yData1);
	gpExp1->SetMarkerStyle(25);
	gpExp1->SetMarkerSize(3);
	gpExp1->SetLineWidth(4);
	gpExp1->SetMarkerColor(kBlack);
	gpExp1->GetXaxis()->SetRangeUser(1, 1e3);
	gpExp1->GetYaxis()->SetRangeUser(1e-8, 1e7);
	gpExp1->Draw("P SAME");
	
	gPad->SetFrameBorderMode(0);
	
	double LET1 = computeIntegral(graph1);
	double LET2 = computeIntegral(graph2);
	double LET3 = computeIntegral(graph3);
	double LET4 = computeIntegral(graph4);
	double LETExp = computeIntegral(gpExp1);
	
	TLegend *legend = new TLegend(0.7, 0.7, 0.9, 0.9);
	
	legend->AddEntry(graph1, Form("#bf{Just Ion} | #bf{LET} = %.2f keV/um", LET1), "l");
	legend->AddEntry(graph2, Form("#bf{Ion, but without msc} | #bf{LET} = %.2f keV/um", LET2), "l");
	legend->AddEntry(graph3, Form("#bf{only e-} | #bf{LET} = %.2f keV/um", LET3), "l");
	legend->AddEntry(graph4, Form("#bf{only e- without msc} | #bf{LET} = %.2f keV/um", LET4), "l");

	legend->AddEntry(gpExp1, Form("#bf{Experimental data} | #bf{LET} = %.2f kev/um", LETExp), "p");
	
	legend->Draw();
	
	c1->Update();
	
	gPad->GetFrame()->SetBorderSize(0); // убирает рамку вокруг холста
	gPad->GetFrame()->SetLineColor(kWhite);
	
	std::string folderName = "plots";
	if (!fs::exists(folderName)){
		fs::create_directory(folderName);
	}
	
	std::replace(begin(energyName), end(energyName), ' ', '_');
	const std::string fileName = ionName + "-" + energyName + "-" + "radialDose";
	
	c1->SaveAs(("plots/" + fileName + "-S4" +".png").c_str());
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
