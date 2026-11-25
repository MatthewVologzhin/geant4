// *********************************************************************
// To execute this macro under ROOT,
//   1 - launch ROOT (usually type 'root' at your machine's prompt)
//   2 - type '.X plot.C' at the ROOT session prompt
// This macro needs the output ROOT file
// *********************************************************************

namespace fs = std::filesystem;

const std::string FormCanvasName(std::string);

struct Parameters{
	std::map<std::string, Int_t> colors;
	std::map<std::string, std::string> paths;
	std::map<std::string, TString> legends;
	std::vector<std::string> names;
};

void plotNe20()
{
	/* Auxilary variables */
	double nm, um, mm, m, MeV;
	m =  1e9; 
	mm = 1e-3*m;
	um = 1e-6*m;
	nm = 1e-9*m;
	MeV = 1e6*1.602e-19;
	
	TString authors = "Vologzhin & Batmunkh & Ivanchenko (2026)";
	TString dataAuthors = "Fain, et al (1973)";
	
	/* Picture quality configuration */
	gStyle->SetCanvasPreferGL(kTRUE);
	gStyle->SetPadBorderSize(0);																										  
	
	/* Experimental data: Proton (1 MeV) */
	double LETExp = 450;
	//Ne20
	const int n = 9;
	double xData[n] = {
		0.25041279383207293, 0.8278138279516931, 2.4906975418397823,
		8.469602149853747, 26.213027367815044, 90.83140247674322,
		278.48458220386567, 974.1151041712695, 3014.841244400653
	};
	double yData[n] = {
		5917507.805933531, 719579.4261593443, 115959.82033840637, 
		10954.905150547167, 852.2605013226715, 68.2633307843561, 
		7.532828078322647, 0.6272486544902323, 0.015821484971240064
	};
	/* Parameters (Standard set: DNA Opt2, 4, 6, 8) */
	Parameters parameters;
	parameters.names = {"DNA2", "DNA4", "DNA6", "DNA8"};//, "S4"};
	parameters.paths["DNA2"]  = "root/RDD_Ne20_163.46-MeV_DNA2.root";
	parameters.paths["DNA4"]  = "root/RDD_Ne20_163.46-MeV_DNA4.root";
	parameters.paths["DNA6"]  = "root/RDD_Ne20_163.46-MeV_DNA6.root";
	parameters.paths["DNA8"]  = "root/RDD_Ne20_163.46-MeV_DNA8.root";
	parameters.paths["S4"]  = "root/RDD_proton_1-MeV_S4.root";

	parameters.colors["DNA2"] = kRed;      
	parameters.colors["DNA4"] = kBlue;     
	parameters.colors["DNA6"] = kGreen+2;  
	parameters.colors["DNA8"] = kMagenta+2;
	parameters.colors["S4"] = kBlack;

	parameters.legends["DNA2"] = Form("#bf{DNA Opt2}");
	parameters.legends["DNA4"] = Form("#bf{DNA Opt4}");
	parameters.legends["DNA6"] = Form("#bf{DNA Opt6}");
	parameters.legends["DNA8"] = Form("#bf{DNA Opt8}");
	parameters.legends["S4"] = Form("#bf{Standard Opt4}");
	parameters.legends["Exp"]  = Form("#bf{%s}", dataAuthors.Data());
	
	/* Global Axis & Legend Parameters */
	double lineWidth = 1.5;
	double yAxisMin = yData[n-1]*0.1;
	double yAxisMax = yData[0]*10;
	double xMinLeg = 0.725; 
	double xMaxLeg = 0.9;
	double yAxisResMin = -1.7;
	double yAxisResMax = 0.8;
	//double xAxisMax = xAxisMin + (xData[n-1] -xAxisMin)*((xMaxLeg-.1)/(xMinLeg-.1)); 
	double xAxisMin = 0.8*xData[0];
	double xAxisMax = xData[n-1]*5/4;
	
	// Фиксированные координаты легенды NDC
	double nbBins = 1;
	double authorsTextSize = 0.0252;
	double systemError = 0.05;

	/* Canvas initialization */
	std::string canvasName = FormCanvasName(parameters.paths["DNA2"]);
	TCanvas *mainCanvas = new TCanvas("mainCanvas", canvasName.c_str(), 1920, 1080);	
	
    TPad *pad1 = new TPad("pad1", "main plot", 0.0, 0.3, 1.0, 1.0);
    TPad *pad2 = new TPad("pad2", "residuals", 0.0, 0.0, 1.0, 0.3);

	double leftMargin = 0.10;
	double rightMargin = 0.10; // Фиксируем отступ справа

	pad1->SetLeftMargin(leftMargin);
	pad1->SetRightMargin(rightMargin);
	pad2->SetLeftMargin(leftMargin);
	pad2->SetRightMargin(rightMargin);
	
    pad1->SetLogy(); 
    pad1->SetBottomMargin(0);
    pad1->Draw();
	pad1->SetLogx();
    pad2->SetTopMargin(0);    
    pad2->SetBottomMargin(0.3);
    pad2->Draw();
	pad2->SetLogx();
    
    TH1F* hFrame = new TH1F("hFrame", "", nbBins, xAxisMin, xAxisMax);
	TAxis *XaxisRes = hFrame->GetXaxis();
	TAxis *YaxisRes = hFrame->GetYaxis();
	
	TLegend *legend = new TLegend(xMinLeg, 0.5, xMaxLeg, 0.9);
	//legend->SetTextSize(0.045);
	legend->SetTextSize(0.035);
	legend->SetBorderSize(1); 
	legend->SetEntrySeparation(0.5);
	
	//TLegend *legendRes = new TLegend(xMinLeg, 0.3, xMaxLeg, 1);
	//legendRes->SetTextSize(0.05);

	int counter = 0;
	TLegendEntry* errorEntry;
	for (auto name : parameters.names){
		TFile *f = new TFile(parameters.paths[name].c_str());
		if (!f || f->IsZombie()) continue;
		TTree *tree = (TTree*)f->Get("radial");
		TTree *treeLET = (TTree*)f->Get("LET");
		long long unsigned nbEntries = tree->GetEntries();
		long long unsigned nbEntriesLET = treeLET->GetEntries();
	
		pad1->cd();
		/* Transfer to TGraphAsymmErrors (Стиль Range) */
		double dose, radius, rmseDose, meanLET, rmseLET;
		Int_t nbEvents;
		tree->SetBranchAddress("dose", &dose);
		tree->SetBranchAddress("radius", &radius);
		tree->SetBranchAddress("rmse", &rmseDose);
		treeLET->SetBranchAddress("mean", &meanLET);
		treeLET->SetBranchAddress("rmse", &rmseLET);
		treeLET->SetBranchAddress("events", &nbEvents);
		treeLET->GetEntry(0);

		// Adjust RMSE for experimental points
		TGraphAsymmErrors* graph = new TGraphAsymmErrors();
		size_t xCounter = 0;
		tree->GetEntry(0); 
		double prevRadius = radius;
		double prevDose = dose;
		double prevRmseDose = rmseDose;
		graph->SetPoint(0, radius, dose);
		graph->SetPointError(0, 0, 0, 0, 0);

		for (size_t j = 1; j <= nbEntries; ++j) {
			tree->GetEntry(j);
			while (xCounter < n && xData[xCounter] <= radius) {
				if (xData[xCounter] >= prevRadius) {
					double t = (radius == prevRadius) ? 0 : (xData[xCounter] - prevRadius) / (radius - prevRadius);
					double interpDose = prevDose + t * (dose - prevDose);
					double interpRMSE = prevRmseDose + t * (rmseDose - prevRmseDose);
					int nextIdx = graph->GetN();
					graph->SetPoint(nextIdx, xData[xCounter], interpDose);
					graph->SetPointError(nextIdx, 0, 0, interpRMSE, interpRMSE);
				}
				xCounter++;
			}

			// Добавляем текущую точку симуляции (без усов)
			int nextIdx = graph->GetN();
			graph->SetPoint(nextIdx, radius, dose);
			graph->SetPointError(nextIdx, 0, 0, 0, 0);

			// Обновляем предыдущие значения
			prevRadius = radius;
			prevDose = dose;
			prevRmseDose = rmseDose;
		}
		
		graph->SetLineWidth(lineWidth);
		graph->SetLineColor(parameters.colors[name]);
		
		if (counter == 0){
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
			yAxis->SetMoreLogLabels(kFALSE); 
			pad1->Update(); 
			for (int i=-20; i<20; i++){
				if (abs(i%2) == 1) yAxis->ChangeLabelByValue(TMath::Power(10., i), -1, 0, -1, -1, -1,"");
			}
			yAxis->ChangeLabelByValue(1, -1, -1, -1, -1, -1, "10^{0}");
			pad1->Modified();
			pad1->Update();
			yAxis->SetTitle("Dose [Gy]");
			yAxis->SetTitleSize(0.07);
			yAxis->SetTitleOffset(0.40);
			yAxis->CenterTitle(true);
			pad1->Update();

			pad1->RedrawAxis();

			// ПРИЖИМАЕМ АВТОРОВ ВПРАВО К ГРАНИЦЕ xAxisMax
			/*TText* authorsText = new TText(xAxisMax, yAxisMax * 1.1, authors);
			authorsText->SetNDC();
			authorsText->SetTextAlign(31); 
			authorsText->SetTextSize(authorsTextSize);
			authorsText->SetTextFont(42);
			authorsText->DrawText(0.895, 0.91, authors);*/

			TLatex *texTitle = new TLatex(0.5, 0.955, (canvasName + "/u").c_str());
			texTitle->SetNDC();
			texTitle->SetTextAlign(22); // Центр-Центр
			texTitle->SetTextFont(42);
			texTitle->SetTextSize(0.060);
			texTitle->Draw();
		} else {
			graph->Draw("L SAME");
		};
		
		/* Additional axes for Pad 1 */
		TGaxis *topAxis = new TGaxis(xAxisMin, yAxisMax, xAxisMax, yAxisMax, xAxisMin, xAxisMax, 510, "GS-");
		topAxis->SetTickSize(0.07);
		topAxis->SetTickLength(0.037);
		topAxis->SetLabelOffset(999);
		topAxis->Draw("SAME");
		
		TGaxis *rightAxis = new TGaxis(xAxisMax, yAxisMin, xAxisMax, yAxisMax, yAxisMin, yAxisMax, 510, "+GU");
		rightAxis->Draw();
	
		/* Residuals Calculation */
		pad2->cd();
		double res[n]; // ВАЖНО: добавить эту строку здесь
		double sumChi2 = 0;
		for (size_t i = 0; i < n; i++) {
			double xExp = xData[i];
			double yExp = yData[i];
            
			double simVal = graph->Eval(xExp); 
            
			double simRMSE = 0; 
			int nPoints = graph->GetN();
			for(int k=0; k < nPoints; ++k) {
				if(TMath::Abs(graph->GetX()[k] - xExp) < 1e-7) {
					simRMSE = graph->GetErrorYhigh(k);
					break;
				}
			}

			double simErrorMean = (nbEvents > 0) ? (simRMSE / TMath::Sqrt(nbEvents)) : 0;
			double relExpErr = 0.15;
			double sigmaExp = relExpErr * yExp;

			double totalSigma2 = sigmaExp*sigmaExp + simErrorMean*simErrorMean;
            
			if (totalSigma2 > 0) {
				sumChi2 += TMath::Power(yExp - simVal, 2) / totalSigma2;
			}
			res[i] = (yExp != 0) ? (yExp - simVal) / yExp : 0;
		}
		double normChi2 = sumChi2 / n;

		//legend->AddEntry(graph, Form("#scale[0.9]{#splitline{%s  |  #bf{#chi^{2}} = %.2f}{#bf{LET} = %.2f keV/um}}", 
		//						parameters.legends[name].Data(), normChi2, meanLET), "l");
		legend->AddEntry(graph, Form("#scale[0.9]{%s  |  #bf{#chi^{2}} = %.2f}", 
								parameters.legends[name].Data(), normChi2), "l");
		
		
		if (counter == 0){
			hFrame->SetStats(0);
			hFrame->Draw("AXIS");
			XaxisRes->SetRangeUser(xAxisMin, xAxisMax);
			XaxisRes->SetLimits(xAxisMin, xAxisMax);
			XaxisRes->SetTitle("Distance from the axis [nm]");
			XaxisRes->SetTitleSize(0.135);
			XaxisRes->SetLabelSize(0.09);
			XaxisRes->CenterTitle(true);
			XaxisRes->SetTickSize(0.07);
			XaxisRes->SetTitleOffset(0.88);

			//XaxisRes->ChangeLabelByValue(50, -1, -1, -1, -1, -1, " ");
			//XaxisRes->ChangeLabelByValue(60, -1, -1, -1, -1, -1, " ");
			
			YaxisRes->SetRangeUser(yAxisResMin, yAxisResMax);
			YaxisRes->SetTitle("#eta = [1 - Geant4/Exp]");
			YaxisRes->SetTitleSize(0.082);
			YaxisRes->SetTitleOffset(0.30);
			YaxisRes->SetLabelSize(0.07);
			YaxisRes->SetLabelOffset(0.008);
			YaxisRes->SetNdivisions(305);
			YaxisRes->CenterTitle(true);
			
			TGraphErrors* errCorridor = new TGraphErrors(n+2); 
			errCorridor->SetPoint(0, xAxisMin, 0.0); 
			errCorridor->SetPointError(0, 0.5, 0.15); 
			for (int i = 1; i < n+1; i++) {
				double x = xData[i];
				double y = yData[i];
				double relErr = 0.15;
				errCorridor->SetPoint(i, x, 0.0); 
				errCorridor->SetPointError(i, 0.5, relErr); 
			}
			errCorridor->SetPoint(n+1, xAxisMax, 0.0); 
			errCorridor->SetPointError(n+1, 0.5, 0.2); 
			errCorridor->SetFillColorAlpha(kGray, 0.4);
			errCorridor->SetFillStyle(1001);
			errCorridor->SetLineWidth(0);
			errCorridor->Draw("E3 SAME");
			//errorEntry = legend->AddEntry(errCorridor, Form("#bf{Exp. tolerance}"), "f");

			TLine *zeroLine = new TLine(xAxisMin, 0.0, xAxisMax, 0.0);
			zeroLine->SetLineStyle(2);
			zeroLine->Draw("SAME");

			// Верхняя ось для Pad 2
			TGaxis *topAxisRes = new TGaxis(xAxisMin, yAxisResMax, xAxisMax, yAxisResMax, xAxisMin, xAxisMax, 510, "-GS");
			topAxisRes->SetTickSize(0.07);
			topAxisRes->SetLabelOffset(999);
			topAxisRes->Draw("SAME");

			// Правая ось для Pad 2 (Floating стиль)
			//double xRightAxisRes = xAxisMin + (xMinLeg + 0.05) * (xAxisMax - xAxisMin);
			TGaxis *rightAxisRes = new TGaxis(xAxisMax, yAxisResMin, xAxisMax, yAxisResMax, 
											yAxisResMin, yAxisResMax, 
											YaxisRes->GetNdivisions(), // Синхронизация меток
											"+");
			rightAxisRes->SetTickSize(0.03);
			rightAxisRes->SetLabelOffset(999);
			rightAxisRes->Draw("SAME");

			pad2->RedrawAxis();
		}
		
		TGraph* splineRes = new TGraph(n, xData, res);
		splineRes->SetPoint(splineRes->GetN(), xAxisMin, splineRes->Eval(xAxisMin));
		splineRes->SetPoint(splineRes->GetN(), xAxisMax, splineRes->Eval(xAxisMax));
		splineRes->Sort();
		splineRes->SetLineWidth(lineWidth);
		splineRes->SetLineColor(parameters.colors[name]);
		splineRes->Draw("L SAME");

		//legendRes->AddEntry(splineRes, Form("#bf{%s} | #bf{RMSE} = %.4f", name.c_str(), rmse), "l");
		//legendRes->SetTextFont(102); 
		//legendRes->SetTextSize(0.0475);
		//legendRes->AddEntry(splineRes, Form("#bf{%s} | #bf{#chi^{2}} = %5.2f | #bf{RMSRE} = %.1f%%", name.c_str(), normChi2, rmse*100), "l");
		//legendRes->AddEntry(splineRes, Form("#bf{%s}  |   #bf{#chi^{2}} = %5.2f", name.c_str(), normChi2), "l");
		//legendRes->Draw();
		
		counter++;
	}
	
	pad1->cd();
	//TGraph* graphExp = new TGraph(n, xData, yData);
	TGraphAsymmErrors* graphExp = new TGraphAsymmErrors(n);
	for (int i=0; i < n; i++){
		double x = xData[i];
		double y = yData[i];
		double totalRelErr = 0.15;
		double totalErr = totalRelErr * y;

		graphExp->SetPoint(i, x, y);
		graphExp->SetPointError(i, 0, 0, totalErr, totalErr);
	}
	graphExp->SetMarkerStyle(25);
	graphExp->SetMarkerSize(3);
	graphExp->SetMarkerColor(kBlack);
	graphExp->Draw("P E SAME");

	//legend->AddEntry(graphExp, Form("#scale[0.85]{#splitline{%s}{#bf{LET} = %.2f keV/um}}",
	//							    parameters.legends["Exp"].Data(), LETExp), "p");
	legend->AddEntry(graphExp, Form("#scale[0.9]{%s}",
								    parameters.legends["Exp"].Data()), "p");									
	legend->Draw();
	mainCanvas->Update();

	std::string folderName = "plots";
	if (!fs::exists(folderName)) fs::create_directory(folderName);
	mainCanvas->SaveAs((folderName + "/" + canvasName + ".pdf").c_str());
}

const std::string FormCanvasName(std::string path){
	if (path.find("Ne20") != std::string::npos) return "Radial dose distribution of Ne20 at energy E=8.1 MeV";
	return "Radial dose distribution";
}