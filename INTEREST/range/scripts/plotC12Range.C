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

void plotC12Range()
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
	
	/* Experimental data: electrons */	
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
		4.241E-07, 8.319E-07, 1.224E-06, 1.980E-06, 2.718E-06, 3.448E-06, 4.175E-06,
		5.619E-06, 7.056E-06, 1.062E-05, 1.413E-05, 2.095E-05, 2.749E-05, 3.375E-05,
		3.974E-05, 5.101E-05, 6.144E-05, 8.459E-05, 1.046E-04, 1.382E-04, 1.664E-04,
		1.911E-04, 2.132E-04, 2.522E-04, 2.864E-04, 3.598E-04, 4.238E-04, 5.405E-04,
		6.540E-04, 7.708E-04, 8.926E-04, 1.152E-03, 1.430E-03, 2.209E-03, 3.100E-03,
		5.208E-03, 7.749E-03, 1.072E-02, 1.413E-02, 2.230E-02, 3.222E-02, 6.432E-02,
		1.063E-01, 2.180E-01, 3.646E-01, 5.438E-01, 7.542E-01, 1.263E+00, 1.881E+00
	};
	
	/* Parameters (Standard set: DNA Opt2, 4, 6, 8) */

	const std::string particleName = "C12";

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
	double yAxisMin = 2e-8;   
	double yAxisMax = 2e0;
	double xAxisMin = 0.001;
	double xAxisMax = 1e5;
	double nbBins = 1;
	std::vector<double> x_err = {0.001, 0.1, 1., 10., 100., 1000.};
	std::vector<double> y_err = {0., 0., 0., 0., 0.};
	std::vector<double> ex = {0., 0., 0., 0., 0.};
	std::vector<double> ey = {1., 0.3, 0.15, 0.05, 0.02, 0.02};

	// Фиксированные координаты легенды NDC
	double xMinLeg = 0.7; 
	double xMaxLeg = 0.9;
	double yAxisResMin = -0.65;
	double yAxisResMax = 0.13;
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
	
	TLegend *legend = new TLegend(0.1, 0.6, 0.1 + xMaxLeg - xMinLeg, 0.9);
	legend->SetTextSize(0.045);
	legend->SetBorderSize(1); 
	
	TLegend *legendRes = new TLegend(xMinLeg, 0.3, xMaxLeg, 1);
	legendRes->SetTextSize(0.05);

	int counter = 0;
	for (auto name : parameters.names){
		pad1->cd();
		/* Fetch the data from .txt files */
		std::ifstream inputFile(parameters.paths[name]);
		std::vector<double> vecEnergy, vecRange, vecSME;

		double rho = 0.998;
		double energyVar, rangeVar, rmseVar, emptyVar, nbVar;
		while (inputFile >> energyVar >> rangeVar >> rmseVar 
						 >> emptyVar >> emptyVar >> emptyVar >> emptyVar
						 >> nbVar){
			vecEnergy.push_back(energyVar*1e-6);
			vecRange.push_back(rangeVar*rho/cm);
			vecSME.push_back(rmseVar*rho/cm/TMath::Sqrt(nbVar));
			//vecNb.push_back(nbVar);
		}
		inputFile.close();
		long long unsigned nSim = vecEnergy.size();

		//TGraphErrors* graph = new TGraphErrors(nSim, vecEnergy.data(), vecSP.data(), nullptr, vecRMSE.data());
		TGraphAsymmErrors* graph = new TGraphAsymmErrors(nSim);
		for (int i=0; i<nSim; ++i){
			double x = vecEnergy[i];
			double y = vecRange[i];
			double sme = vecSME[i];

			graph->SetPoint(i, x, y);

			graph->SetPointError(i, 0, 0, sme, sme);
		}

		
		graph->SetLineWidth(lineWidth);
		graph->SetLineColor(parameters.colors[name]);
		
		if (counter == 0){
			pad1->cd();
			TH1F* hFrame1 = new TH1F("hFrame1", canvasName.c_str(), nbBins, xAxisMin, xAxisMax);
			hFrame1->SetStats(0);
			TAxis* xAxis = hFrame1->GetXaxis();
			TAxis* yAxis = hFrame1->GetYaxis();
			hFrame1->Draw("AXIS"); // Рисуем рамку первой
			xAxis->SetLimits(xAxisMin, xAxisMax);
			xAxis->SetRangeUser(xAxisMin, xAxisMax);
			xAxis->SetLabelSize(0); 
			xAxis->SetTitleSize(0);
			xAxis->SetTickLength(0.03);
			yAxis->SetRangeUser(yAxisMin, yAxisMax);
			yAxis->ChangeLabelByValue(1e1, -1, -1, -1, -1, -1, " ");
			yAxis->ChangeLabelByValue(1e2, -1, -1, -1, -1, -1, " ");
			yAxis->ChangeLabelByValue(1e3, -1, -1, -1, -1, -1, " ");
			yAxis->SetTitle("Range #rhor, [g cm^{-2}] ");
			yAxis->SetTitleSize(0.04);
			yAxis->SetTitleOffset(0.72);
			yAxis->CenterTitle(true);
			pad1->Update();

			
			graph->GetXaxis()->SetLimits(xAxisMin, xAxisMax);
			graph->GetXaxis()->SetRangeUser(xAxisMin, xAxisMax);
			graph->GetYaxis()->SetRangeUser(yAxisMin, yAxisMax);
			graph->GetXaxis()->SetTitle("Energy, [MeV]");
			graph->GetYaxis()->SetTitleSize(0.04);
			graph->GetYaxis()->SetTitleOffset(0.72);
			graph->GetXaxis()->CenterTitle(true);
			graph->GetYaxis()->CenterTitle(true);
			graph->Draw("L SAME");
			pad1->RedrawAxis();

			TText* authorsText = new TText(xAxisMax, yAxisMax * 1.1, authors);
			authorsText->SetNDC();
			authorsText->SetTextAlign(31); 
			authorsText->SetTextSize(authorsTextSize);
			authorsText->SetTextFont(42);
			authorsText->DrawText(0.895, 0.91, authors);

			TLatex *texTitle = new TLatex(0.5, 0.955, canvasName.c_str());
			texTitle->SetNDC();
			texTitle->SetTextAlign(22); // Центр-Центр
			texTitle->SetTextFont(42);
			texTitle->SetTextSize(0.045);
			texTitle->Draw();
			
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
		for (size_t i=0; i<n; i++){
			double simVal = graph->Eval(xData[i]);
			res[i] = (yData[i] - simVal) / yData[i];
		}

		/* Chi2 calculations */
		double chi2Sum = 0;
		int validPoints = 0;
		for (int i = 0; i < n; i++) {
			double simVal = vecRange[i]; 
			double energy = xData[i];
			double errSim = vecSME[i];

			// ICRU90 Error
			double relErrExp = ey.back();
			for (int j=0; j < x_err.size(); j++ ){
				if (energy >= x_err[j]) relErrExp = ey[j];
			}
			
			double errExp = relErrExp * yData[i];

			// Остатки
			res[i] = (yData[i] - simVal) / yData[i];

			// Chi2
			double totalVariance = TMath::Power(errSim, 2) + TMath::Power(errExp, 2);
			if (totalVariance > 0) {
				chi2Sum += TMath::Power(yData[i] - simVal, 2) / totalVariance;
				validPoints++;
			}
		}
		double rmse = 0;
		for(int i=0; i<n; i++) rmse += res[i]*res[i];
		rmse = TMath::Sqrt(rmse/n);

		double normChi2 = (validPoints > 1) ? chi2Sum / (validPoints - 1) : 0;
		std::cout << "Model: " << name << " | Reduced Chi2 = " << normChi2 << " | RMSE = " << rmse << std::endl;
		
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
			icruBand->Draw("E3 SAME"); 
			legendRes->AddEntry(icruBand, "#bf{ICRU90 Relative error}", "f");

			TLine *zeroLine = new TLine(0.0, 0.0, xAxisMax, 0.0);
			zeroLine->SetLineStyle(2);
			zeroLine->Draw("SAME");

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
		
		TGraph* splineRes = new TGraph(n, xData, res);
		splineRes->SetLineWidth(lineWidth);
		splineRes->SetLineColor(parameters.colors[name]);
		splineRes->Draw("L SAME");
		legendRes->AddEntry(splineRes, Form("#bf{%s}  |   #bf{#chi^{2}} = %5.4f", name.c_str(), normChi2), "l");
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

	TGraphAsymmErrors* graphExp = new TGraphAsymmErrors(n);
	for (int i=0; i < n; i++){
		double x = xData[i];
		double y = yData[i];
		double relErr = ey.back();
		for (int i=0; i < ey.size(); i++ ){
			if (x >= x_err[i]) relErr = ey[i];
		}
		graphExp->SetPoint(i, x, y);
		graphExp->SetPointError(i, 0, 0, y*relErr, y*relErr);
	}

	graphExp->SetMarkerStyle(25);
	//graphExp->SetMarkerStyle();
	graphExp->SetMarkerSize(3);
	graphExp->SetMarkerColor(kBlack);
	graphExp->Draw("P E SAME");
	legend->AddEntry(graphExp, parameters.legends["Exp"], "p");
	
	legend->Draw();
	mainCanvas->Update();

	std::string folderName = "plots";
	if (!fs::exists(folderName)) fs::create_directory(folderName);
	mainCanvas->SaveAs((folderName + "/" + canvasName + ".pdf").c_str());
}

const std::string FormCanvasName(std::string path, const std::string particleName){
	if (path.find(particleName) != std::string::npos) return "Range of " + particleName;
	return "Range";
}
