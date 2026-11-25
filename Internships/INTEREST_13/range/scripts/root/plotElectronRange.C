// *********************************************************************
// To execute this macro under ROOT,
//   1 - launch ROOT (usually type 'root' at your machine's prompt)
//   2 - type '.X plot.C' at the ROOT session prompt
// This macro needs the output ROOT file
// *********************************************************************
namespace fs = std::filesystem;

const std::string FormCanvasName(std::string, const std::string);

struct Parameters{
	std::map<std::string, Int_t> colors;
	std::map<std::string, std::string> paths;
	std::map<std::string, TString> legends;
	std::vector<std::string> names;
};

void plotElectronRange()
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
	int n = 49;
	const int nuclons = 1;
	const int maxEnergyU = 50;
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
        4.235E-06, 9.118E-06, 1.524E-05, 3.098E-05, 5.110E-05, 7.536E-05, 1.036E-04,
		1.714E-04, 2.537E-04, 5.189E-04, 8.632E-04, 1.769E-03, 2.939E-03, 4.348E-03, 
		5.978E-03, 9.833E-03, 1.439E-02, 2.933E-02, 4.512E-02, 8.464E-02, 1.294E-01, 
		1.774E-01, 2.275E-01, 3.315E-01, 4.384e-01, 7.096e-01, 9.811e-01, 1.517E+00, 
		2.041E+00, 2.554E+00, 3.057E+00, 4.035E+00, 4.980E+00, 7.226E+00, 9.327E+00, 
		1.318E+01, 1.667E+01, 1.985E+01, 2.277E+01, 2.801E+01, 3.259E+01, 4.204E+01, 
		4.955E+01, 6.110E+01, 6.986E+01, 7.692E+01, 8.284E+01, 9.239E+01, 9.994E+01
    };
	
	/* Parameters (Standard set: DNA Opt2, 4, 6, 8) */

	const std::string particleName = "e-";

	Parameters parameters;
	parameters.names = {"DNA2", "DNA4", "DNA6", "DNA8", "S4"};
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
	parameters.legends["S4"] = Form("#bf{STD Opt4}");
	parameters.legends["Exp"]  = Form("#bf{%s}", dataAuthors.Data());
	
	/* Global Axis & Legend Parameters */
	double lineWidth = 1.5;
	double yAxisMin = 2e-6;   
	double yAxisMax = 1e2;
	double xAxisMin = 0.001;
	double xAxisMax = 1e5;
	double nbBins = 1;
	std::vector<double> x_err = {0.001/nuclons, 0.01/nuclons, 0.1/nuclons, 1000.0/nuclons};
	std::vector<double> y_err = {0., 0., 0., 0.};
	std::vector<double> ex = {0., 0., 0., 0.};
	std::vector<double> ey = {0.05, 0.015, 0.01, 0.01};

	// Фиксированные координаты легенды NDC
	double xMinLeg = 0.75; 
	double xMaxLeg = 0.9;
	double yAxisResMin = -0.92;
	double yAxisResMax = 0.18;
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
	
	//TLegend *legend = new TLegend(0.1, 0.6, 0.1 + xMaxLeg - xMinLeg, 0.9);
	TLegend *legend = new TLegend(xMinLeg, 0.0, xMaxLeg, 0.4);
	legend->SetTextSize(0.035);
	legend->SetTextAlign(22);
	legend->SetMargin(0.5);
	legend->SetBorderSize(1); 
	legend->SetEntrySeparation(0.5);
	
	//TLegend *legendRes = new TLegend(xMinLeg, 0.3, xMaxLeg, 1);
	//legendRes->SetTextSize(0.05);

	int counter = 0;
	TGraphAsymmErrors* expBand = nullptr;
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
			vecEnergy.push_back(energyVar*1e-6/nuclons);
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
			xAxis->SetLimits(xData[0], xData[n-1]);
			xAxis->SetRangeUser(xData[0], xData[n-1]);
			xAxis->SetLabelSize(0); 
			xAxis->SetTitleSize(0);
			xAxis->SetTickLength(0.03);
			yAxis->SetRangeUser(yAxisMin, yAxisMax);
			yAxis->SetTitle("Range  #it{#rhor}, [g cm^{-2}] ");
			yAxis->SetTitleSize(0.05);
			yAxis->SetTitleOffset(0.55);
			yAxis->CenterTitle(true);
			pad1->Update();
			pad1->RedrawAxis();

			expBand = new TGraphAsymmErrors(n);
			for (int i=0; i < n; i++){
				double x = xData[i];
				double y = yData[i];
				double relErr = ey.back();
				for (int i=0; i < ey.size(); i++ ){
					if (x >= x_err[i]) relErr = ey[i];
				}

				expBand->SetPoint(i, x, y);
				expBand->SetPointError(i, 0, 0, y*relErr, y*relErr);
			}
			expBand->SetFillColorAlpha(kGray, 0.5);
			expBand->SetFillStyle(1001);
			expBand->SetLineColor(kBlack);
			//expBand->SetLineWidth(0);;
			expBand->SetLineStyle(2);;
			expBand->Draw("E3 SAME");
			expBand->Draw("L X SAME");

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
		
		legend->AddEntry(graph, Form("#scale[0.9]{%s}", parameters.legends[name].Data()), "l");
		
		if (counter == 0){
			hFrame->SetStats(0);
			hFrame->Draw("AXIS");

			hFrame->SetStats(0);
			hFrame->Draw("AXIS");
			XaxisRes->SetLimits(xData[0], xData[n-1]);
			XaxisRes->SetRangeUser(xData[0], xData[n-1]);
			XaxisRes->SetTitle("Energy, [MeV]");
			XaxisRes->SetTitleSize(0.135);
			XaxisRes->SetLabelSize(0.09);
			XaxisRes->CenterTitle(true);
			XaxisRes->SetTickSize(0.07);
			XaxisRes->SetTitleOffset(0.88);
		
			YaxisRes->SetRangeUser(yAxisResMin, yAxisResMax);
			YaxisRes->SetTitle("#eta = [1 - Geant4/Ref]");
			YaxisRes->SetTitleSize(0.082);
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
	legend->AddEntry(expBand, Form("#scale[0.9]{%s}",
								    parameters.legends["Exp"].Data()), "lf");
	pad1->RedrawAxis();
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
