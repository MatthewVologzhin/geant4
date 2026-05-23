#include <filesystem>
#include <algorithm>
#include <cmath>
#include <TFile.h>
#include <random>
#include <map>
#include <vector>

namespace fs = std::filesystem;

double computeIntegral(TGraph*);

const std::string FormCanvasName(std::string);

struct Parameters{
	std::map<std::string, Int_t> colors;
	std::map<std::string, std::string> paths;
	std::map<std::string, TString> legends;
	std::vector<std::string> names;
};

void protonRange(){
	/* Auxilary variables */
	double nm, um, mm, m, MeV;
	m =  1e9; 		//you can change this value
	mm = 1e-3*m;	// ... but not this!
	um = 1e-6*m;	// ... but not this!
	nm = 1e-9*m;	// ... but not this!
	MeV = 1e6*1.602e-19;
	
	TString authors = "Vologzhin & Ivanchenko & Batmunkh (2025)";
	TString dataAuthors = "ICRU90";
	
	/* Picture quality configuration */
	gStyle->SetCanvasPreferGL(kTRUE);
	gStyle->SetPadBorderSize(0);																										  
	
	/* Experimental data: Protons */	
	const int n = 49;
	double rho = 0.998*0.1; // 0.998*g/cm^(-3)*mm
	double xData[n] = {
		0.0010, 0.0015, 0.0020, 0.0030, 0.0040, 0.0050, 0.0060, 0.0080, 0.0100,
		0.0150, 0.0200, 0.0300, 0.0400, 0.0500, 0.0600, 0.0800, 0.1000, 0.1500,
		0.2000, 0.3000, 0.4000, 0.5000, 0.6000, 0.8000, 1.0000, 1.5000, 2.0000,
		3.0000, 4.0000, 5.0000, 6.0000, 8.0000, 10.0000, 15.0000, 20.0000, 30.0000,
		40.0000, 50.0000, 60.0000, 80.0000, 100.0000, 150.0000, 200.0000, 300.0000,
		400.0000, 500.0000, 600.0000, 800.0000, 1000.0000
	};

	double yData[n] = {
		6.319e-6, 8.969e-6, 1.137e-5, 1.560e-5, 1.930e-5, 2.262e-5, 2.567e-5,
		3.113e-5, 3.599e-5, 4.657e-5, 5.578e-5, 7.187e-5, 8.613e-5, 9.935e-5,
		1.120e-4, 1.364e-4, 1.607e-4, 2.249e-4, 2.967e-4, 4.645e-4, 6.640e-4,
		8.945e-4, 1.155e-3, 1.765e-3, 2.487e-3, 4.753e-3, 7.639e-3, 1.514e-2,
		2.482e-2, 3.656e-2, 5.028e-2, 8.346e-2, 1.240e-1, 2.558e-1, 4.289e-1,
		8.910e-1, 1.498e0, 2.240e0, 3.111e0, 5.212e0, 7.759e0, 1.586e1, 2.609e1,
		5.170e1, 8.263e1, 1.176e2, 1.556e2, 2.382e2, 3.268e2
	};

	/* Parameters */
	Parameters parameters;
	//parameters.names = {"DNA2", "DNA4", "DNA6", "DNA8", "S4"};
	parameters.names = {"DNA2"};
	parameters.paths["DNA2"]  = "root/ionrange-proton-DNA2.root";
	parameters.paths["DNA4"]  = "root/ionrange-proton-DNA4.root";
	parameters.paths["DNA6"]  = "root/ionrange-proton-DNA6.root";
	parameters.paths["DNA8"]  = "root/ionrange-proton-DNA8.root";
	parameters.paths["S4"] 	  = "root/ionrange-proton-S4.root";
	parameters.colors["DNA2"] = kAzure-14;
	parameters.colors["DNA4"] = kAzure-7;
	parameters.colors["DNA6"] = kAzure;
	parameters.colors["DNA8"] = kAzure+7;
	parameters.colors["S4"]   = kRed;
	parameters.legends["DNA2"] = Form("#bf{DNA Opt2}");
	parameters.legends["DNA4"] = Form("#bf{DNA Opt4}");
	parameters.legends["DNA6"] = Form("#bf{DNA Opt6}");
	parameters.legends["DNA8"] = Form("#bf{DNA Opt8}");
	parameters.legends["S4"]   = Form("#bf{EM Standard Opt4}");
	parameters.legends["Exp"]  = Form("#bf{%s}", dataAuthors.Data());
	
	/* Other parameters */
	double lineWidth = 1.5;
	double yAxisMin, yAxisMax, xAxisMin, xAxisMax, xMinLegend;
	double authorsTextSize = 0.0295;
	double yAxisResMin, yAxisResMax;
	yAxisMin = 0.5*yData[0];
	yAxisMax = 2*yData[n-1];
	xAxisMin = xData[0];
	xAxisMax = 40*xData[n-1];
	xMinLegend = 0.1+0.8*(log10(xData[n-1])-log10(xAxisMin))/(log10(xAxisMax)-log10(xAxisMin));
	yAxisResMin = -1.2e2;
	yAxisResMax = 1.2e2;

	
	/* Canvas name from file name */
	std::string canvasName = FormCanvasName(parameters.paths["DNA2"]);
	TCanvas *mainCanvas = new TCanvas("mainCanvas", canvasName.c_str(),
									  1920, 1080);	
	
	/* Up and down pad initialization */
    TPad *pad1 = new TPad("pad1", "main plot", 0.0, 0.3, 1.0, 1.0); // Верхний Pad для основного графика
    TPad *pad2 = new TPad("pad2", "residuals", 0.0, 0.0, 1.0, 0.3); // Нижний Pad для графика остатков
    pad1->SetLogx();
    pad1->SetLogy(); 
    pad1->SetBottomMargin(0);
    pad1->Draw();
    pad2->SetTopMargin(0);    
    pad2->SetBottomMargin(0.3); //pad2->SetFillColor(kBlack); //pad2->SetFrameFillColor(kBlack);
    pad2->SetLogx();
    pad2->Draw();
	
	TLegend *legend = new TLegend(xMinLegend, 0.6, 0.9, 0.9);
	legend->SetTextSize(0.05);
	TLegend *legendRes = new TLegend(xMinLegend, 0.3, 0.9, 1);
	legendRes->SetTextSize(0.055);
	int counter = 0;
	for (auto name : parameters.names){
		/* Trees initialization */
		std::string path = parameters.paths[name];
		TFile *f =  new TFile(path.c_str());
		if (f->IsZombie()){
			continue;
		} 
		++counter;
		TTree *treeRange = (TTree*)f->Get("ionRanges");
	
		/* Upper pad */
		pad1->cd();
		double energy, range; 
		treeRange->SetBranchAddress("energy", &energy);
		treeRange->SetBranchAddress("range", &range);
	
		/* Data initialization */
		int nbPtsRange = treeRange->GetEntries(); 
		double energy1, energy2;
		std::vector<long> nbParticlesVector;
		std::vector<double> energyVector;
		long particlesNb = 1;
		for (Long64_t i=0; i < nbPtsRange; ++i){
			treeRange->GetEntry(i);
			energy1 = energy;
			if (i == 0){
			}
			treeRange->GetEntry(i+1);
			energy2 = energy;
			if (energy1 == energy2){
				++particlesNb;
			} else {
				nbParticlesVector.push_back(particlesNb);
				energyVector.push_back(energy1);
				particlesNb = 1;
			}
		}
		
		TGraph* graphRange = new TGraph();
		long nbRuns = nbParticlesVector.size();
		double mean, std;
		long index = 0;
		for (size_t i=0; i<nbRuns; i++){
			mean = 0;
			std = 0;
			long oldIndex = index;
			long nb = nbParticlesVector[i];
			for (size_t j=0; j<nb; j++){
				treeRange->GetEntry(index);
				mean += range;	
				++index;
			}
			mean = mean/nb;
			
			index = oldIndex;
			for (size_t j=0; j<nb; j++){
				treeRange->GetEntry(index);
				std += TMath::Power(mean-range, 2);	
				++index;
			} 
			double t = TMath::StudentQuantile(0.975, nb);
			std = t*(double)(std)/TMath::Sqrt(nb)*TMath::Sqrt(rho);
			graphRange->SetPoint(graphRange->GetN(), energyVector[i], rho*mean);
		}
		
		
		graphRange->SetLineWidth(lineWidth);
		Int_t color = parameters.colors[name];
		graphRange->SetLineColor(color);
		graphRange->SetTitle(canvasName.c_str());
		TAxis *Xaxis = graphRange->GetXaxis();
		TAxis *Yaxis = graphRange->GetYaxis();
		Xaxis->SetLimits(xAxisMin, xAxisMax);
		Xaxis->SetRangeUser(xAxisMin, xAxisMax);
		Yaxis->SetLimits(yAxisMin, yAxisMax);
		Yaxis->SetRangeUser(yAxisMin, yAxisMax);
		Xaxis->SetTitle("Initial particle energy E, [MeV]");
		Yaxis->SetTitle("Range #rhoR, [g/cm^{-2}]");
		Yaxis->SetTitleSize(0.062);
		Yaxis->SetTitleOffset(0.45);
		Xaxis->CenterTitle(true); Xaxis->SetTitleOffset(1.4);
		Yaxis->CenterTitle(true); 
		if (counter == 1){
			graphRange->Draw("AL");
		} else {
			graphRange->Draw("L SAME");
		};
		
		/*TText* authorsDataText = new TText(60, 8000, dataAuthors);
		authorsDataText->SetTextFont(42);
		authorsDataText->SetTextSize(0.04);
		authorsDataText->Draw();*/
		TText* authorsText = new TText(xData[n-1], yAxisMax*1.1, authors);
		authorsText->SetTextSize(authorsTextSize);
		authorsText->SetTextFont(42);
		authorsText->Draw();
			
		/* Top axis of PAD #1 */
		double xMin, xMax, yMin, yMax;
		xMin = Xaxis->GetXmin();
		xMax = Xaxis->GetXmax();
		yMin = Yaxis->GetXmin();
		yMax = Yaxis->GetXmax();
		TString chopt = "SN-G";
		int numDecadesForTGaxis = 0;
		if (xMin > 0 && xMax > xMin) {
			numDecadesForTGaxis = (int)ceil(log10(xMax) - log10(xMin));
			if (numDecadesForTGaxis < 1) numDecadesForTGaxis = 1;
		} else {
			std::cerr << "Warning: Invalid X axis range for calculating decades for TGaxis. Using default." << std::endl;
			numDecadesForTGaxis = 3;
		}
		TGaxis *topAxis = new TGaxis(xMin, yMax, xMax, yMax,
									 xMin, xMax,
									 numDecadesForTGaxis,
									 chopt.Data());
		topAxis->SetTickSize(0.07);
		topAxis->SetLineColor(kBlack);
		topAxis->SetLineWidth(1);
		topAxis->SetTickLength(0.037);
		topAxis->SetLabelOffset(999);
		topAxis->Draw("SAME");
		
		/* Right axis on the Pad #1 */
		numDecadesForTGaxis = 0;
		TString choptRightPad1 = "+GU";
		if (yMin > 0 && yMax > yMin) {
			numDecadesForTGaxis = (int)ceil(log10(yMax) - log10(yMin));
			if (numDecadesForTGaxis < 1) numDecadesForTGaxis = 1;
		} else {
			std::cerr << "Warning: Invalid Y axis range for calculating decades for TGaxis. Using default." << std::endl;
			numDecadesForTGaxis = 3;
		}
		TGaxis *rightAxis = new TGaxis(xMax, yMin, xMax, yMax,
									   yMin, yMax,
									   numDecadesForTGaxis,
									   choptRightPad1.Data());
		//rightAxis->SetTickSize(-0.1);
		//rightAxis->SetLineColor(kBlue);
		//rightAxis->SetLineWidth(1);
		//rightAxis->SetTickLength(-0.037);
		//rightAxis->SetLabelOffset(999);
		rightAxis->Draw();
		pad1->Update();
		pad2->Update();
		mainCanvas->Modified();
		mainCanvas->Update();	
	
		///* Legend: Draw & Initialization */
		TString legendText = parameters.legends[name];
		legend->AddEntry(graphRange, name.c_str(), "l");
	
		///* Lower pad: Residual pad */
		pad2->cd();
		double res[n];
		double rmse = 0;

		for (size_t i=0; i<n; i++){
			res[i] = (yData[i]-graphRange->Eval(xData[i]))/yData[i];
			rmse +=  TMath::Power((yData[i]-graphRange->Eval(xData[i]))/yData[i], 2);
		}
		rmse = TMath::Sqrt(rmse/n);
		
		TH1F* hFrame = new TH1F("hFrame", "", 100, 0.8, 120);
		hFrame->SetStats(0); // Не показывать статистическое окно
		hFrame->Draw("AXIS"); // Рисуем оси hFrame на активном паде
		TAxis *XaxisRes = hFrame->GetXaxis();
		TAxis *YaxisRes = hFrame->GetYaxis();
		
		/* Set Limits and Ranges for Pad #2 */
		XaxisRes->SetLimits(xAxisMin, xAxisMax);
		XaxisRes->SetRangeUser(xAxisMin, xAxisMax);
		XaxisRes->SetTitleSize(.09);
		XaxisRes->SetTitle("Initial particle energy E, [MeV]");
		XaxisRes->CenterTitle(true);
		XaxisRes->SetAxisColor(kBlack);
		XaxisRes->SetTickSize(0.07);
		XaxisRes->SetLabelColor(kBlack);
		XaxisRes->SetLabelSize(0.09);
		XaxisRes->ChangeLabelByValue(100, -1, -1, -1, -1, -1, " "); // Delete label '100'
		YaxisRes->SetLimits(yAxisResMin, yAxisResMax);
		YaxisRes->SetRangeUser(yAxisResMin, yAxisResMax);
		YaxisRes->SetTitleSize(1.2);
		YaxisRes->SetLabelSize(0.08);
		/* attempt to remove bold fraction line */
		//YaxisRes->SetTitleFont(42);   
		YaxisRes->SetTitleSize(.07);
		/* attempt to remove bold fraction line */
		YaxisRes->SetTitle(Form("#eta = [1 - Geant4/Exp]"));
		YaxisRes->CenterTitle(true);
		YaxisRes->SetTitleOffset(0.38);
		YaxisRes->SetLabelColor(kBlack);
		YaxisRes->SetNdivisions(305);
		/* Draw axis between Pad #1 & Pad #2 */
		double xMinRes, xMaxRes, yMinRes, yMaxRes;
		xMinRes = XaxisRes->GetXmin();
		xMaxRes = XaxisRes->GetXmax();
		yMinRes = YaxisRes->GetXmin();
		yMaxRes = YaxisRes->GetXmax();
		if (yMinRes >= yMaxRes) {
			std::cerr << "Error: X-axis range is zero or invalid after drawing main object. Cannot draw top axis." << std::endl;
			std::cerr << "Debug: xMinRes = " << xMinRes << ", xMaxRes = " << xMaxRes << std::endl;
			return;
		}
		chopt = "SN-G"; 
		numDecadesForTGaxis = 0;
		if (xMinRes > 0 && xMaxRes > xMinRes) {
			numDecadesForTGaxis = (int)ceil(log10(xMaxRes) - log10(xMinRes));
			if (numDecadesForTGaxis < 1) numDecadesForTGaxis = 1;
		} else {
			std::cerr << "Warning: Invalid X axis range for calculating decades for TGaxis. Using default." << std::endl;
			numDecadesForTGaxis = 3; // Значение по умолчанию на случай ошибки
		}
		TGaxis *topAxisRes = new TGaxis(xMinRes, yMaxRes, xMaxRes, yMaxRes,
										xMinRes, xMaxRes,
										numDecadesForTGaxis,
										chopt.Data());
		topAxisRes->SetTickSize(0.07);
		topAxisRes->SetLineColor(kBlack);
		topAxisRes->SetLineWidth(1);
		topAxisRes->Draw();
		chopt = "+"; 
		numDecadesForTGaxis = 305;
		TGaxis *rightAxisRes = new TGaxis(xData[n-1], yAxisResMin, xData[n-1], yAxisResMax,
										  yAxisResMin, yAxisResMax,
										  YaxisRes->GetNdivisions(),
										  chopt.Data());
		rightAxisRes->SetTickSize(0.03);
		rightAxisRes->SetLineColor(kBlack);
		rightAxisRes->SetLineWidth(1);
		rightAxisRes->SetLabelOffset(999);
		rightAxisRes->Draw();
		/* Draw: Zero-axis on Pad #2 */
		TLine *zeroLine = new TLine();
		zeroLine->SetLineColor(kBlack);
		zeroLine->SetLineWidth(1);
		zeroLine->SetLineStyle(2);
		//zeroLine->DrawLine(XaxisRes->GetXmin(), 0.0, XaxisRes->GetXmax(), 0.0);
		pad2->Update();
		pad2->Draw();
		

	
		/* Data on Pad #2: Residual data */
		TGraph* graphRes = new TGraph(n, xData, res);
		graphRes->SetTitle("");
		TSpline* splineRes = new TSpline3(name.c_str(), graphRes);
		splineRes->SetLineWidth(lineWidth);
		splineRes->SetLineColor(parameters.colors[name]);
		splineRes->Draw("L SAME");

		/* The legend: Pad #2 */
		legendRes->AddEntry(splineRes, Form("#bf{%s} | #bf{RMSE} = %.4f", name.c_str(), rmse), "l");
		legendRes->Draw();
	}

	/* Plot: Experimental data */
	pad1->cd();
	TGraph* graphExp = new TGraph(n, xData, yData);
	graphExp->SetMarkerStyle(25);
	graphExp->SetMarkerSize(3);
	graphExp->SetLineWidth(4);
	graphExp->SetMarkerColor(kBlack);
	graphExp->Draw("P SAME");
	legend->AddEntry(graphExp,  parameters.legends["Exp"], "p");
	
	/* Update full canvas */
	legend->Draw();
	mainCanvas->Update();

	/* Save the plot */
	std::string folderName = "plots";
	if (!fs::exists(folderName)){
		fs::create_directory(folderName);
	}

	mainCanvas->SaveAs(("plots/" + canvasName + ".pdf").c_str());
};

const std::string FormCanvasName(std::string path){
	std::string energyName, ionName;
	size_t nameIndex, energyIndex;
	ionName  = path.substr(14,-1);
	nameIndex = std::find(begin(ionName), end(ionName), '-')-begin(ionName);
	ionName = ionName.substr(0, nameIndex);
	const std::string canvasName = "Range of " + ionName;
	return canvasName;
};
