// *********************************************************************
// To execute this macro under ROOT,
//   1 - launch ROOT (usually type 'root' at your machine's prompt)
//   2 - type '.X plot.C' at the ROOT session prompt
// This macro needs the output ROOT file
// *********************************************************************

#include <iomanip>
#include <vector>

void SetLeafAddress(TNtuple* ntuple, const char* name, void* address);

namespace fs = std::filesystem;

const std::string FormCanvasName(std::string, const std::string);

struct Parameters{
	std::map<std::string, Int_t> colors;
	std::map<std::string, std::string> paths;
	std::map<std::string, TString> legends;
	std::vector<std::string> names;
};

void plotPositronDetour()
{
	/* Auxilary variables */
	double nm, um, mm, cm, m, MeV;
	m =  1e9; 
	cm = 1e-2*m;
	mm = 1e-3*m;
	um = 1e-6*m;
	nm = 1e-9*m;
	MeV = 1e6*1.602e-19;
	
	TString authors = "Vologzhin & Batmunkh & Ivanchenko (2026)";
	TString dataAuthors = "ICRU90";
	
	/* Picture quality configuration */
	gStyle->SetCanvasPreferGL(kTRUE);
	gStyle->SetPadBorderSize(0);																										  
	
	/* Experimental data: positron */	
	const int n = 49;
    double xData[n] = {
        0.0010, 0.0015, 0.0020, 0.0030, 0.0040, 0.0050, 0.0060, 0.0080, 0.0100,
        0.0150, 0.0200, 0.0300, 0.0400, 0.0500, 0.0600, 0.0800, 0.1000, 0.1500,
        0.2000, 0.3000, 0.4000, 0.5000, 0.6000, 0.8000, 1.0000, 1.5000, 2.0000,
        3.0000, 4.0000, 5.0000, 6.0000, 8.0000, 10.0000, 15.0000, 20.0000, 30.0000,
        40.0000, 50.0000, 60.0000, 80.0000, 100.0000, 150.0000, 200.0000, 300.0000,
        400.0000, 500.0000, 600.0000, 800.0000, 1000.0000
    };
    double yData[n] = {
		0.5212, 0.5364, 0.5501, 0.5737, 0.5934, 0.6102, 0.6250,
		0.6497, 0.6700, 0.7080, 0.7352, 0.7724, 0.7973, 0.8155,
		0.8296, 0.8503, 0.8649, 0.8886, 0.9032, 0.9212, 0.9323,
		0.9402, 0.9462, 0.9551, 0.9615, 0.9722, 0.9789, 0.9867,
		0.9907, 0.9931, 0.9946, 0.9963, 0.9973, 0.9983, 0.9988,
		0.9992, 0.9993, 0.9991, 0.9994, 0.9995, 0.9995, 0.9996,
		0.9996, 0.9996, 0.9996, 0.9997, 0.9997, 0.9997, 0.9997
	};
	
	/* Parameters (Standard set: DNA Opt2, 4, 6, 8) */

	const std::string particleName = "e+";

	Parameters parameters;
	parameters.names = {"DNA2", "DNA4", "DNA6", "DNA8", "S4"};
	//parameters.names = {"DNA2"};
	parameters.paths["DNA2"]  = "root/" + particleName + "_DNA2.txt";
	parameters.paths["DNA4"]  = "root/" + particleName + "_DNA4.txt";
	parameters.paths["DNA6"]  = "root/" + particleName + "_DNA6.txt";
	parameters.paths["DNA8"]  = "root/" + particleName + "_DNA8.txt";
	parameters.paths["S4"]  = "root/" + particleName + "_S4.txt";

	parameters.colors["DNA2"] = kRed;      
	parameters.colors["DNA4"] = kBlue;     
	parameters.colors["DNA6"] = kGreen+2;  
	parameters.colors["DNA8"] = kMagenta+2;
	parameters.colors["S4"] = kOrange;

	parameters.legends["DNA2"] = Form("#bf{DNA Opt2}");
	parameters.legends["DNA4"] = Form("#bf{DNA Opt4}");
	parameters.legends["DNA6"] = Form("#bf{DNA Opt6}");
	parameters.legends["DNA8"] = Form("#bf{DNA Opt8}");
	parameters.legends["S4"] = Form("#bf{Standard Opt4}");
	parameters.legends["Exp"]  = Form("#bf{%s}", dataAuthors.Data());
	
	/* Global Axis & Legend Parameters */
	double lineWidth = 1.5;
	double yAxisMin = 3e-1;   
	double yAxisMax = 3;
	double xAxisMin = 0.001;
	double xAxisMax = 1e5;
	//double xAxisMax = 4641.5;
	//double xAxisMax = 1000.0;
	double nbBins = 1;

	// Фиксированные координаты легенды NDC
	double xMinLeg = 0.7; 
	double xMaxLeg = 0.9;
	double yAxisResMin = -0.32;
	double yAxisResMax = 0.07;
	double authorsTextSize = 0.0252;

	/* Canvas initialization */
	std::string canvasName = FormCanvasName(parameters.paths["DNA2"], particleName);
	TCanvas *mainCanvas = new TCanvas("mainCanvas", canvasName.c_str(), 1920, 1080);	
	
    TPad *pad1 = new TPad("pad1", "main plot", 0.0, 0.3, 1.0, 1.0);
    TPad *pad2 = new TPad("pad2", "residuals", 0.0, 0.0, 1.0, 0.3);
    pad1->SetLogx(); 
	pad1->SetLogy();
    pad1->SetBottomMargin(0);
    pad1->Draw();
	pad2->SetLogx(); 
    pad2->SetTopMargin(0);    
    pad2->SetBottomMargin(0.3);
    pad2->Draw();
    
    TH1F* hFrame = new TH1F("hFrame", "", nbBins, xAxisMin, xAxisMax);
	TAxis *XaxisRes = hFrame->GetXaxis();
	TAxis *YaxisRes = hFrame->GetYaxis();
	
	TLegend *legend = new TLegend(xMinLeg, 0.6, xMaxLeg, 0.9);
	legend->SetTextSize(0.045);
	legend->SetBorderSize(1); 
	
	TLegend *legendRes = new TLegend(xMinLeg, 0.3, xMaxLeg, 1);
	legendRes->SetTextSize(0.05);

	int counter = 0;
	for (auto name : parameters.names){
		pad1->cd();
		/* Fetch the data from .txt files */
		std::ifstream inputFile(parameters.paths[name]);
		std::vector<double> vecEnergy, vecDetour, vecSME;

		double rho = 0.998;
		double energyVar, rangeVar, rmseRangeVar, penVar, rmsePenVar, emptyVar, nbVar;
		while (inputFile >> energyVar >> rangeVar >> rmseRangeVar 
						 >> emptyVar >> emptyVar >> penVar >> rmsePenVar >> nbVar){
			vecEnergy.push_back(energyVar*1e-6);
			vecDetour.push_back(penVar/rangeVar);
			vecSME.push_back((TMath::Sqrt(
				TMath::Power(rmseRangeVar*penVar/(rangeVar*rangeVar), 2)+
				TMath::Power(rmsePenVar/(rangeVar*rangeVar), 2)/TMath::Sqrt(nbVar))
			));
		}
		inputFile.close();
		long long unsigned nSim = vecEnergy.size();

		//TGraphErrors* graph = new TGraphErrors(nSim, vecEnergy.data(), vecSP.data(), nullptr, vecRMSE.data());
		TGraphAsymmErrors* graph = new TGraphAsymmErrors(nSim);
		for (int i=0; i<nSim; ++i){
			double x = vecEnergy[i];
			double y = vecDetour[i];
			double sme = vecSME[i];

			graph->SetPoint(i, x, y);

			graph->SetPointError(i, 0, 0, sme, sme);
		}

		
		graph->SetLineWidth(lineWidth);
		graph->SetLineColor(parameters.colors[name]);
		
		if (counter == 0){
			graph->SetTitle(canvasName.c_str());
			graph->Draw("AL");
			graph->GetXaxis()->SetLimits(xAxisMin, xAxisMax);
			graph->GetXaxis()->SetRangeUser(xAxisMin, xAxisMax);
			graph->GetYaxis()->SetRangeUser(yAxisMin, yAxisMax);
			graph->GetXaxis()->SetTitle("Energy, [MeV]");
			graph->GetYaxis()->SetTitle("Detour factor D = #frac{Range}{Penetration}");
			//graph->GetYaxis()->SetTitleSize(0.062);
			//graph->GetYaxis()->SetTitleOffset(0.45);
			graph->GetYaxis()->SetTitleSize(0.04);
			graph->GetYaxis()->SetTitleOffset(0.72);
			graph->GetXaxis()->CenterTitle(true);
			graph->GetYaxis()->CenterTitle(true);
			//graph->GetYaxis()->ChangeLabelByValue(1, -1, -1, -1, -1, -1, " ");
			//graph->GetYaxis()->ChangeLabelByValue(1e-5, -1, -1, -1, -1, -1, " ");

			// ПРИЖИМАЕМ АВТОРОВ ВПРАВО К ГРАНИЦЕ xAxisMax
			TText* authorsText = new TText(xAxisMax, yAxisMax * 1.1, authors);
			authorsText->SetTextAlign(31); 
			authorsText->SetTextSize(authorsTextSize);
			authorsText->SetTextFont(42);
			authorsText->Draw();
			
		} else {
			graph->Draw("L SAME");
		};
		
		/* Additional axes for Pad 1 */
		TGaxis *topAxis = new TGaxis(0.0, yAxisMax, xAxisMax, yAxisMax, 0.0, xAxisMax, 510, "S-");
		topAxis->SetTickSize(0.07);
		topAxis->SetTickLength(0.037);
		topAxis->SetLabelOffset(999);
		topAxis->Draw("SAME");
		
		TGaxis *rightAxis = new TGaxis(xAxisMax, yAxisMin, xAxisMax, yAxisMax, yAxisMin, yAxisMax, 510, "+GU");
		rightAxis->Draw();
		legend->AddEntry(graph, parameters.legends[name], "l");
	
		/* Residuals Calculation */
		pad2->cd();
		double res[n];
		double rmse = 0;
		for (size_t i=0; i<n; i++){
			double simVal = graph->Eval(xData[i]);
			res[i] = (yData[i] - simVal) / yData[i];
			rmse += TMath::Power(res[i], 2);
		}
		rmse = TMath::Sqrt(rmse/n);
		
		if (counter == 0){
			hFrame->SetStats(0);
			hFrame->Draw("AXIS");

			XaxisRes->SetLimits(xAxisMin, xAxisMax);
			XaxisRes->SetRangeUser(xAxisMin, xAxisMax);
			XaxisRes->SetTitle("Energy, [MeV]");
			XaxisRes->SetTitleSize(0.09);
			XaxisRes->SetLabelSize(0.09);
			XaxisRes->CenterTitle(true);
			XaxisRes->ChangeLabelByValue(1e4, -1, -1, -1, -1, -1, " ");
			XaxisRes->ChangeLabelByValue(1e5, -1, -1, -1, -1, -1, " ");
			XaxisRes->SetTickSize(0.07);
			XaxisRes->SetTitleOffset(1.2);
		
			YaxisRes->SetLimits(yAxisResMin, yAxisResMax);
			YaxisRes->SetRangeUser(yAxisResMin, yAxisResMax);
			YaxisRes->SetTitle("#eta = [1 - Geant4/Exp]");
			YaxisRes->SetTitleSize(0.07);
			YaxisRes->SetTitleOffset(0.38);
			YaxisRes->SetLabelSize(0.06);
			YaxisRes->CenterTitle(true);
			
			/*std::vector<double> x_err = {0.001, 0.01, 0.1, 1.0, 1000.0};
			std::vector<double> y_err = {0., 0., 0., 0., 0.};
			std::vector<double> ex = {0., 0., 0., 0., 0.};
			std::vector<double> ey = {0.05, 0.015, 0.01, 0.01, 0.01};
			TGraphErrors* icruBand = new TGraphErrors(x_err.size(), &x_err[0], &y_err[0], nullptr, &ey[0]);
			icruBand->SetFillColorAlpha(kGray, 0.5);
			icruBand->SetFillStyle(1001);
			icruBand->SetLineColor(kGray);
			icruBand->SetLineWidth(0);
			TAxis* xAxisBand = icruBand->GetXaxis();
			TAxis* yAxisBand = icruBand->GetYaxis();
			xAxisBand->SetLimits(xAxisMin, xAxisMax);
			xAxisBand->SetRange(xAxisMin, xAxisMax);
			xAxisBand->SetTitle("Energy, [MeV]");
			xAxisBand->ChangeLabelByValue(1e3, -1, -1, -1, -1, -1, " ");
			xAxisBand->ChangeLabelByValue(1e4, -1, -1, -1, -1, -1, " ");
			xAxisBand->SetTitleSize(0.09);
			xAxisBand->SetLabelSize(0.09);
			xAxisBand->SetTitleOffset(1.2);
			xAxisBand->CenterTitle(true);
			yAxisBand->SetTitle("#eta = [1 - Geant4/Exp]");
			yAxisBand->SetTitleSize(0.07);
			yAxisBand->SetLabelSize(0.06);
			yAxisBand->SetTitleOffset(0.38);
			yAxisBand->CenterTitle(true);
			icruBand->SetTitle("");
			//icruBand->GetXaxis()->SetMoreLogLabels();
			icruBand->Draw("3"); */

			TLine *zeroLine = new TLine(0.0, 0.0, xAxisMax, 0.0);
			zeroLine->SetLineStyle(2);
			zeroLine->Draw("SAME");

			/*legendRes->AddEntry(icruBand, "#bf{ICRU90 Relative error}", "f");*/

			TString chopt = "SN-G";
			int numDecadesForTGaxis = 0;
			if (xAxisMin > 0 && xAxisMax > xAxisMin) {
				numDecadesForTGaxis = (int)ceil(log10(xAxisMax) - log10(xAxisMin));
				if (numDecadesForTGaxis < 1) numDecadesForTGaxis = 1;
			} else {
				std::cerr << "Warning: Invalid X axis range for calculating decades for TGaxis. Using default." << std::endl;
				numDecadesForTGaxis = 3;
			}
			TGaxis *topAxisRes = new TGaxis(xAxisMin, yAxisResMax, xAxisMax, yAxisResMax,
									xAxisMin, xAxisMax,
									numDecadesForTGaxis,
									chopt.Data());
			topAxisRes->SetTickSize(0.07);
			topAxisRes->SetLineColor(kBlack);
			topAxisRes->SetLineWidth(1);
			topAxisRes->Draw("SAME A");
			
			chopt = "+"; 
			numDecadesForTGaxis = 305;
			double xRightAxisRes = TMath::Power(10, log10(xAxisMin)+(xMinLeg+0.05)*(log10(xAxisMax)-log10(xAxisMin)));
			TGaxis *rightAxisRes = new TGaxis(xRightAxisRes, yAxisResMin, xRightAxisRes, yAxisResMax,
											yAxisResMin, yAxisResMax,
											YaxisRes->GetNdivisions(),
											chopt.Data());
			rightAxisRes->SetTickSize(0.03);
			rightAxisRes->SetLineColor(kBlack);
			rightAxisRes->SetLineWidth(1);
			rightAxisRes->SetLabelOffset(999);
			rightAxisRes->Draw("SAME");


			pad2->RedrawAxis();
		}
		
		/*TGraph* splineRes = new TGraph(n, xData, res);
		splineRes->SetLineWidth(lineWidth);
		splineRes->SetLineColor(parameters.colors[name]);
		splineRes->Draw("L SAME");

		TGraphAsymmErrors* splineRes = new TGraphAsymmErrors(n);
		for (int i=0; i<n; ++i){
			double x = xData[i];
			double y = res[i];
			double rmse = vecRMSE[i];

			splineRes->SetPoint(i, x, y);
			splineRes->SetPointError(i, 0, 0, rmse, rmse);
		}*/
		/*
		splineRes->SetLineWidth(lineWidth);
		splineRes->SetLineColor(parameters.colors[name]);
		splineRes->Draw("L SAME");

		legendRes->AddEntry(splineRes, Form("#bf{%s} | #bf{RMSE} = %.4f", name.c_str(), rmse), "l");*/
		legendRes->Draw();
		
		counter++;
	}
	
	/* Experimental data overlay */
	pad1->cd();
	/*TGraph* graphExp = new TGraph(n, xData, yData);
	graphExp->SetMarkerStyle(25);
	graphExp->SetMarkerSize(3);
	graphExp->SetMarkerColor(kBlack);
	graphExp->Draw("P SAME");
	legend->AddEntry(graphExp, parameters.legends["Exp"], "p");*/

	/*TGraphAsymmErrors* graphExp = new TGraphAsymmErrors(n);
	for (int i=0; i < n; i++){
		double x = xData[i];
		double y = yData[i];
		double relErr = 0.01;

		if (x <= 0.01) relErr = 0.05;
		else if (x <= 0.1) relErr = 0.015;
		else relErr = 0.01;

		graphExp->SetPoint(i, x, y);
		graphExp->SetPointError(i, 0, 0, y*relErr, y*relErr);
	}*/

	/*graphExp->SetMarkerStyle(25);
	//graphExp->SetMarkerStyle();
	graphExp->SetMarkerSize(3);
	graphExp->SetMarkerColor(kBlack);
	graphExp->Draw("P E SAME");
	legend->AddEntry(graphExp, parameters.legends["Exp"], "p");*/
	
	legend->Draw();
	mainCanvas->Update();

	std::string folderName = "plots";
	if (!fs::exists(folderName)) fs::create_directory(folderName);
	mainCanvas->SaveAs((folderName + "/" + canvasName + ".pdf").c_str());
}

const std::string FormCanvasName(std::string path, const std::string particleName){
	if (path.find(particleName) != std::string::npos) return "Detour factor of " + particleName;
	return "Detour factor";
}
