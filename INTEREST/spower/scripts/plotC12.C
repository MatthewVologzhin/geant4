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

void plotC12()
{
	/* Auxilary variables */
	double nm, um, mm, m, MeV;
	m =  1e9; 
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
	int n = 49;
	const int nuclons = 12;
	const int maxEnergyU = 20;
    double xData[] = {
        0.0010, 0.0015, 0.0020, 0.0030, 0.0040, 0.0050, 0.0060, 0.0080, 0.0100,
        0.0150, 0.0200, 0.0300, 0.0400, 0.0500, 0.0600, 0.0800, 0.1000, 0.1500,
        0.2000, 0.3000, 0.4000, 0.5000, 0.6000, 0.8000, 1.0000, 1.5000, 2.0000,
        3.0000, 4.0000, 5.0000, 6.0000, 8.0000, 10.0000, 15.0000, 20.0000, 30.0000,
        40.0000, 50.0000, 60.0000, 80.0000, 100.0000, 150.0000, 200.0000, 300.0000,
        400.0000, 500.0000, 600.0000, 800.0000, 1000.0000
    };
	int maxIndex = 1;
	for (double &x : xData){
		x /= nuclons;
		if (x == maxEnergyU) {n = maxIndex;}
		++maxIndex;
	}
    double yData[] = {
    	1.421E+02, 1.773E+02, 2.075E+02, 2.589E+02, 3.047E+02, 3.416E+02, 3.776E+02,
    	4.412E+02, 4.975E+02, 6.183E+02, 7.207E+02, 8.930E+02, 1.038E+03, 1.166E+03,
    	1.281E+03, 1.488E+03, 1.676E+03, 2.099E+03, 2.476E+03, 3.131E+03, 3.696E+03,
    	4.198E+03, 4.652E+03, 5.446E+03, 6.116E+03, 7.364E+03, 8.139E+03, 8.791E+03,
    	8.708E+03, 8.372E+03, 8.029E+03, 7.422E+03, 6.926E+03, 5.980E+03, 5.284E+03,
    	4.297E+03, 3.627E+03, 3.134E+03, 2.750E+03, 2.207E+03, 1.855E+03, 1.346E+03,
    	1.069E+03, 7.717E+02, 6.124E+02, 5.123E+02, 4.434E+02, 3.540E+02, 2.984E+02
	};
	
	/* Parameters (Standard set: DNA Opt2, 4, 6, 8) */

	const std::string particleName = "C12";

	Parameters parameters;
	parameters.names = {"DNA2", "DNA4", "DNA6", "DNA8"};
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
	double yAxisMin = 8e1;   
	double yAxisMax = 3e4;
	double xAxisMin = 0.001;
	double xAxisMax = 1e5;
	double nbBins = 1;
	double systemError = 0.05;
	std::vector<double> x_err = {0.001/nuclons, 0.1/nuclons, 1./nuclons, 10./nuclons, 100./nuclons, 1000./nuclons};
	std::vector<double> y_err = {0., 0., 0., 0., 0.};
	std::vector<double> ex = {0., 0., 0., 0., 0.};
	std::vector<double> ey = {1., 0.3, 0.15, 0.05, 0.02, 0.02};

	// Фиксированные координаты легенды NDC
	double xMinLeg = 0.1; 
	double xMaxLeg = 0.3;
	double yAxisResMin = -0.15;
	double yAxisResMax = 1.8;
	double authorsTextSize = 0.0252;

	//std::cout << TMath::Power(10., TMath::Log10(xAxisMin)+(xMaxLeg-0.1)/(xMinLeg-0.1)*(TMath::Log10(1e3) - TMath::Log10(xAxisMin))) << std::endl;

	/* Canvas initialization */
	std::string canvasName = FormCanvasName(parameters.paths["DNA2"], particleName);
	TCanvas *mainCanvas = new TCanvas("mainCanvas", canvasName.c_str(), 1920, 1080);	
    TPad *pad1 = new TPad("pad1", "main plot", 0.0, 0.3, 1.0, 1.0);
    TPad *pad2 = new TPad("pad2", "residuals", -0.0, 0.0, 1.0, 0.3);
	double leftMargin = 0.10;
	double rightMargin = 0.10; // Фиксируем отступ справа
	pad1->SetLeftMargin(leftMargin);
	pad1->SetRightMargin(rightMargin);
	pad2->SetLeftMargin(leftMargin);
	pad2->SetRightMargin(rightMargin);
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
	
	TLegend *legend = new TLegend(xMinLeg, 0.5, xMaxLeg, 0.9);
	legend->SetTextSize(0.035);
	legend->SetBorderSize(1); 
	legend->SetEntrySeparation(0.5);
	//TLegend *legendRes = new TLegend(xMinLeg, 0.3, xMaxLeg, 1);
	//legendRes->SetTextSize(0.05);

	int counter = 0;
	for (auto name : parameters.names){
		pad1->cd();
		/* Fetch the data from .txt files */
		std::ifstream inputFile(parameters.paths[name]);
		std::vector<double> vecEnergy, vecSP, vecRMSE;

		double rho = 0.998;
		double energyVar, spVar, rmseVar, partNb;
		while (inputFile >> energyVar >> spVar >> rmseVar >> partNb){
			vecEnergy.push_back(energyVar*1e-6/nuclons);
			vecSP.push_back(spVar*10/rho);
			vecRMSE.push_back(rmseVar*10/rho/TMath::Sqrt(partNb));
		}
		inputFile.close();
		long long unsigned nSim = vecEnergy.size();

		//TGraphErrors* graph = new TGraphErrors(nSim, vecEnergy.data(), vecSP.data(), nullptr, vecRMSE.data());
		TGraphAsymmErrors* graph = new TGraphAsymmErrors(nSim);
		for (int i=0; i<nSim; ++i){
			double x = vecEnergy[i];
			double y = vecSP[i];
			double rmse = vecRMSE[i];
			graph->SetPoint(i, x, y);
			graph->SetPointError(i, 0, 0, rmse, rmse);
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
			xAxis->SetLimits(xData[0], xData[n-1]);
			xAxis->SetRangeUser(xData[0], xData[n-1]);
			xAxis->SetLabelSize(0); 
			xAxis->SetTitleSize(0);
			xAxis->SetTickLength(0.03);
			yAxis->SetRangeUser(yAxisMin, yAxisMax);
			yAxis->ChangeLabelByValue(1e4, -1, -1, -1, -1, -1, " ");
			yAxis->ChangeLabelByValue(10, -1, -1, -1, -1, -1, " ");
			yAxis->ChangeLabelByValue(1, -1, -1, -1, -1, -1, " ");
			yAxis->SetTitle("Stopping power #frac{S_{el}}{#rho}, #left[#frac{MeV cm^{2}}{g}#right]");
			yAxis->SetTitleSize(0.05);
			yAxis->SetTitleOffset(0.55);
			yAxis->CenterTitle(true);
			pad1->Update();
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
			texTitle->SetTextSize(0.060);
			texTitle->Draw();
			
		} else {
			graph->Draw("L SAME");
		};
		
		/* Additional axes for Pad 1 */
		TGaxis *topAxis = new TGaxis(xData[0], yAxisMax, xData[n-1], yAxisMax, xData[0], xData[n-1], 510, "GS-");
		topAxis->SetTickSize(0.07);
		topAxis->SetTickLength(0.037);
		topAxis->SetLabelOffset(999);
		topAxis->Draw("SAME");
		
		TGaxis *rightAxis = new TGaxis(xData[n-1], yAxisMin, xData[n-1], yAxisMax, yAxisMin, yAxisMax, 510, "+GU");
		rightAxis->Draw();

	
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
			double simVal = vecSP[i]; 
			double energy = xData[i];
			double errSim = vecRMSE[i];

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
		
		legend->AddEntry(graph, Form("#scale[0.9]{%s  |   #bf{#chi^{2}} = %5.2f}", parameters.legends[name].Data(), normChi2), "l");
		
		if (counter == 0){
			hFrame->SetStats(0);
			hFrame->Draw("AXIS");
			XaxisRes->SetLimits(xData[0], xData[n-1]);
			XaxisRes->SetRangeUser(xData[0], xData[n-1]);
			XaxisRes->SetTitle("Energy, [MeV/u]");
			XaxisRes->SetTitleSize(0.135);
			XaxisRes->SetLabelSize(0.09);
			XaxisRes->CenterTitle(true);
			XaxisRes->SetTickSize(0.07);
			XaxisRes->SetTitleOffset(0.88);
			XaxisRes->ChangeLabelByValue(1e4, -1, -1, -1, -1, -1, " ");
			XaxisRes->ChangeLabelByValue(1e5, -1, -1, -1, -1, -1, " ");
		
			YaxisRes->SetRangeUser(yAxisResMin, yAxisResMax);
			YaxisRes->SetTitle("#eta = [1 - Geant4/Exp]");
			YaxisRes->SetTitleSize(0.085);
			YaxisRes->SetTitleOffset(0.30);
			YaxisRes->SetLabelSize(0.07);
			YaxisRes->CenterTitle(true);
			YaxisRes->SetNdivisions(305);
			
			TGraphErrors* icruBand = new TGraphErrors(x_err.size(), &x_err[0], &y_err[0], nullptr, &ey[0]);
			icruBand->SetFillColorAlpha(kGray, 0.5);
			icruBand->SetFillStyle(1001);
			icruBand->SetLineColor(kGray);
			icruBand->SetLineWidth(0);
			icruBand->Draw("E3 SAME"); 
			//legendRes->AddEntry(icruBand, "#bf{ICRU90 Relative error}", "f");

			TLine *zeroLine = new TLine(0.0, 0.0, xData[n-1], 0.0);
			zeroLine->SetLineStyle(2);
			zeroLine->Draw("SAME");

			// Верхняя ось для Pad 2
			TString chopt = "SN-G";
			int numDecadesForTGaxis = 0;
			if (xAxisMin > 0 && xAxisMax > xAxisMin) {
				numDecadesForTGaxis = (int)ceil(log10(xAxisMax) - log10(xAxisMin));
				if (numDecadesForTGaxis < 1) numDecadesForTGaxis = 1;
			} else {
				std::cerr << "Warning: Invalid X axis range for calculating decades for TGaxis. Using default." << std::endl;
				numDecadesForTGaxis = 3;
			}
			TGaxis *topAxisRes = new TGaxis(xData[0], yAxisResMax, xData[n-1], yAxisResMax,
									xData[0], xData[n-1],
									numDecadesForTGaxis,
									chopt.Data());
			topAxisRes->SetTickSize(0.07);
			topAxisRes->SetLineColor(kBlack);
			topAxisRes->SetLineWidth(1);
			topAxisRes->Draw("SAME A");

			chopt = "+"; 
			numDecadesForTGaxis = 305;
			double xRightAxisRes = TMath::Power(10, log10(xAxisMin)+(xMinLeg+0.05)*(log10(xAxisMax)-log10(xAxisMin)));
			TGaxis *rightAxisRes = new TGaxis(xData[n-1], yAxisResMin, xData[n-1], yAxisResMax,
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
		
		counter++;
	}
	
	/* Experimental data overlay */
	pad1->cd();

	TGraphAsymmErrors* graphExp = new TGraphAsymmErrors(n);
	for (int i=0; i < n; i++){
		double x = xData[i];
		double y = yData[i];
		double relErr = ey.back();
		for (int i=0; i < ey.size(); i++ ){
			if (x >= x_err[i]) relErr = ey[i]*(x_err[i+1]-x)/(x_err[i+1]-x_err[i]) + ey[i+1]*(x-x_err[i])/(x_err[i+1]-x_err[i]);
		}
		graphExp->SetPoint(i, x, y);
		graphExp->SetPointError(i, 0, 0, y*relErr, y*relErr);
	}

	graphExp->SetMarkerStyle(25);
	graphExp->SetMarkerSize(3);
	graphExp->SetMarkerColor(kBlack);
	graphExp->Draw("P E SAME");
	legend->AddEntry(graphExp, Form("#scale[0.9]{%s}",
								    parameters.legends["Exp"].Data()), "p");
	legend->Draw();
	mainCanvas->Update();

	std::string folderName = "plots";
	if (!fs::exists(folderName)) fs::create_directory(folderName);
	mainCanvas->SaveAs((folderName + "/" + canvasName + ".pdf").c_str());
}

const std::string FormCanvasName(std::string path, const std::string particleName){
	if (path.find(particleName) != std::string::npos) return "Electronic stopping power for " + particleName;
	return "Electronic stopping power";
}
