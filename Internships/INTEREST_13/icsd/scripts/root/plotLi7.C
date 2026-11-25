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

void plotLi7()
{
	/* Auxilary variables */
	double nm, um, mm, m, MeV;
	m =  1e9; 
	mm = 1e-3*m;
	um = 1e-6*m;
	nm = 1e-9*m;
	MeV = 1e6*1.602e-19;
	
	TString authors = "Vologzhin & Batmunkh & Ivanchenko (2026)";
	TString dataAuthors = "Conte (2012)";
	
	/* Picture quality configuration */
	gStyle->SetCanvasPreferGL(kTRUE);
	gStyle->SetPadBorderSize(0);																										  
	
	/* Experimental data: Li7 (26.7 MeV) */	
	const int n = 25;
	double xData[n] = {
		0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
		11.0, 12.0, 13.0, 14.0, 15.0, 16.0, 17.0, 18.0, 19.0, 20.0, 
		21.0, 22.0, 23.0, 24.0
	};
	double yData[n] = {
		0.000159147462391498, 0.000997044855880535, 0.004314808733352142, 0.012522369372651936, 0.027843898815540148, 
		0.04958756760220695, 0.07616371035542542, 0.10089186583741058, 0.11698316847561942, 0.1241167564495287, 
		0.11872717639526417, 0.10392249143011521, 0.08701393415575853, 0.06666647668219298, 0.04958756760220695, 
		0.034253458934878384, 0.0233136044822903, 0.014955721790423539, 0.009042703550444323, 0.005230088690678805, 
		0.003115826972632146, 0.0016985459502511977, 0.0008989338803444511, 0.0003699332863897141, 0.00013725632585590486
	};
	
	/* Parameters */
	Parameters parameters;
	parameters.names = {"DNA2", "DNA4", "DNA6", "DNA8"};//, "DNA6", "DNA8"};
	parameters.paths["DNA2"]  = "root/ICSD_Li7_26.7-MeV_DNA2.root";
	parameters.paths["DNA4"]  = "root/ICSD_Li7_26.7-MeV_DNA4.root";
	parameters.paths["DNA6"]  = "root/ICSD_Li7_26.7-MeV_DNA6.root";
	parameters.paths["DNA8"]  = "root/ICSD_Li7_26.7-MeV_DNA8.root";

	parameters.colors["DNA2"] = kRed;      
	parameters.colors["DNA4"] = kBlue;     
	parameters.colors["DNA6"] = kGreen+2;  
	parameters.colors["DNA8"] = kMagenta+2;

	parameters.legends["DNA2"] = Form("#bf{DNA Opt2}");
	parameters.legends["DNA4"] = Form("#bf{DNA Opt4}");
	parameters.legends["DNA6"] = Form("#bf{DNA Opt6}");
	parameters.legends["DNA8"] = Form("#bf{DNA Opt8}");
	parameters.legends["Exp"]  = Form("#bf{%s}", dataAuthors.Data());
	
	/* Other parameters */
	double lineWidth = 1.5;
	double yAxisMin, yAxisMax, xAxisMin, xAxisMax;
	double authorsTextSize = 0.0252;
	double yAxisResMin, yAxisResMax;

	yAxisMin = 1e-4;
	yAxisMax = 1e0;

	// Параметры осей (фиксируем для Li6)
	xAxisMin = -0.5;
	xAxisMax = 31.5; // Можно оставить 29.5 как у Li7 для единообразия сеток
	unsigned nbBins = xAxisMax-xAxisMin; 

	double xMinLeg = 0.7; 
	double xMaxLeg = 0.9;
	yAxisResMin = -6;
	yAxisResMax = 1.2;
	double totalNExp = 1e4;
	double systemError = 0.05;
	double sumRelErrExp2 = 0;
	for (int i = 0; i < n; i++) {
		double relStat = TMath::Sqrt(yData[i] / totalNExp) / yData[i];
		sumRelErrExp2 += TMath::Power(relStat, 2) + TMath::Power(systemError, 2);
	}
	double avgRelErrExp = TMath::Sqrt(sumRelErrExp2 / n);

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
    pad2->SetTopMargin(0);    
    pad2->SetBottomMargin(0.3);
    pad2->Draw();
    
    TH1F* hFrame = new TH1F("hFrame", "", nbBins, xAxisMin, xAxisMax);
	TAxis *XaxisRes = hFrame->GetXaxis();
	TAxis *YaxisRes = hFrame->GetYaxis();
	
	/*TLegend *legend = new TLegend(xMinLegend, 0.6, 0.9, 0.9);
	legend->SetTextSize(0.05);
	TLegend *legendRes = new TLegend(xMinLegend, 0.3, 0.9, 1);
	legendRes->SetTextSize(0.055);*/

	//TLegend *legend = new TLegend(xMinLeg, 0.5, xMaxLeg, 0.9);
	TLegend *legend = new TLegend(0.3, 0.0, 0.5, 0.4);
	legend->SetTextSize(0.035);
	legend->SetBorderSize(1);
	legend->SetEntrySeparation(0.5);
	//TLegend *legendRes = new TLegend(xMinLeg, 0.3, xMaxLeg, 1);
	//legendRes->SetTextSize(0.05);

	int counter = 0;
	for (auto name : parameters.names){
		TFile *f =  new TFile(parameters.paths[name].c_str());
		if (!f || f->IsZombie()) continue;
		TTree *tree = (TTree*)f->Get("ntuple_1");
	
		pad1->cd();
		double ion;
		tree->SetBranchAddress("ionisations", &ion);
	
		/* Histogram and Graph with FIXED BINNING */
		TH1F* hist = new TH1F(Form("h_%s", name.c_str()), "", nbBins, xAxisMin, xAxisMax);
		hist->Sumw2();
		int nbEntries = tree->GetEntries(); 
		for (size_t i=0; i<nbEntries; ++i){
			tree->GetEntry(i);
			hist->Fill(ion);
		}

		double normFactor = hist->Integral();
		if(normFactor > 0) hist->Scale(1.0/normFactor);
	
		/* Transfer to TGraphAsymmErrors (Стиль Range) */
		TGraphAsymmErrors* graph = new TGraphAsymmErrors();
		for (int j=1; j<=nbBins; ++j){
			double center = hist->GetBinCenter(j);
			double content = hist->GetBinContent(j);
			double error = hist->GetBinError(j);
			if (content > 0) {
				int nPt = graph->GetN();
				graph->SetPoint(nPt, center, content);
				graph->SetPointError(nPt, 0, 0, error, error);
			}
		}
		
		graph->SetLineWidth(lineWidth);
		graph->SetLineColor(parameters.colors[name]);
		
		if (counter == 0){
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
			yAxis->ChangeLabelByValue(1e-4, -1, -1, -1, -1, -1, " ");
			yAxis->ChangeLabelByValue(1e1, -1, -1, -1, -1, -1, " ");
			yAxis->SetTitle("Frequency");
			yAxis->SetTitleSize(0.07);
			yAxis->SetTitleOffset(0.40);
			yAxis->CenterTitle(true);
			graph->Draw("L SAME");
			pad1->Update();
			pad1->RedrawAxis();

			// ПРИЖИМАЕМ АВТОРОВ ВПРАВО К ГРАНИЦЕ xAxisMax
			/*TText* authorsText = new TText(xAxisMax, yAxisMax * 1.1, authors);
			authorsText->SetNDC();
			authorsText->SetTextAlign(31); 
			authorsText->SetTextSize(authorsTextSize);
			authorsText->SetTextFont(42);
			authorsText->DrawText(0.895, 0.91, authors);*/

			TLatex *texTitle = new TLatex(0.5, 0.955, (canvasName+"/u").c_str());
			texTitle->SetNDC();
			texTitle->SetTextAlign(22); // Центр-Центр
			texTitle->SetTextFont(42);
			texTitle->SetTextSize(0.060);
			texTitle->Draw();
		} else {
			graph->Draw("L SAME");
		};
		
		double res[n];
		double rmse = 0;
		for (size_t i=0; i<n; i++){
			double simVal = graph->Eval(xData[i]);
			res[i] = (yData[i] != 0) ? (yData[i] - simVal) / yData[i] : 0;
			res[i] = (res[i] > 1.) ? 1. : res[i];
			rmse += TMath::Power(res[i], 2);
		}
		rmse = TMath::Sqrt(rmse/n);

		double chi2[n];	
		double normChi2 = 0;
		double totalNSim = (double)tree->GetEntries();
		for (size_t i=0; i<n; ++i){
			double simVal = graph->Eval(xData[i]);
			chi2[i] = TMath::Power(yData[i] - simVal, 2)/
						  (simVal/totalNSim +
						   yData[i]/totalNExp +
						   TMath::Power(systemError * yData[i], 2));
			normChi2 += chi2[i];
		}
		normChi2 /= n - 1;


		/* Additional axes for Pad 1 */
		TGaxis *topAxis = new TGaxis(0.0, yAxisMax, xData[n-1], yAxisMax, 0.0, xData[n-1], 510, "S-");
		topAxis->SetTickSize(0.07);
		topAxis->SetTickLength(0.037);
		topAxis->SetLabelOffset(999);
		topAxis->Draw("SAME");
		
		TGaxis *rightAxis = new TGaxis(xData[n-1], yAxisMin, xData[n-1], yAxisMax, yAxisMin, yAxisMax, 510, "+GU");
		rightAxis->Draw();
		legend->AddEntry(graph, Form("#scale[0.9]{%s  |   #bf{#chi^{2}} = %5.2f}", parameters.legends[name].Data(), normChi2), "l");

		pad2->cd();
		if (counter == 0){
			hFrame->SetStats(0);
			hFrame->Draw("AXIS");
			XaxisRes->SetLimits(xData[0], xData[n-1]);
			XaxisRes->SetRangeUser(xData[0], xData[n-1]);
			XaxisRes->SetTitle("Ionisation number");
			XaxisRes->SetTitleSize(0.135);
			XaxisRes->SetLabelSize(0.09);
			XaxisRes->CenterTitle(true);
			XaxisRes->SetTickSize(0.07);
			XaxisRes->SetTitleOffset(0.88);

			XaxisRes->ChangeLabelByValue(30, -1, -1, -1, -1, -1, " ");
			XaxisRes->ChangeLabelByValue(25, -1, -1, -1, -1, -1, " ");
			
			YaxisRes->SetRangeUser(yAxisResMin, yAxisResMax);
			YaxisRes->SetTitle("#eta = [1 - Geant4/Exp]");
			YaxisRes->SetTitleSize(0.082);
			YaxisRes->SetTitleOffset(0.30);
			YaxisRes->SetLabelSize(0.07);
			YaxisRes->CenterTitle(true);
			YaxisRes->SetNdivisions(305);
			
			TGraphErrors* errCorridor = new TGraphErrors(n);
			for (int i = 0; i < n; i++) {
				double x = xData[i];
				double y = yData[i];
				
				// Относительная погрешность (в долях единицы)
				double relStatErr = TMath::Sqrt(y / totalNExp) / y;
				double relSystErr = systemError;
				double totalRelErr = TMath::Sqrt(relStatErr*relStatErr + relSystErr*relSystErr);
				
				errCorridor->SetPoint(i, x, 0.0); // Центрируем на нуле
				errCorridor->SetPointError(i, 0.5, totalRelErr); 
			}
			errCorridor->SetFillColorAlpha(kGray, 0.4);
			errCorridor->SetFillStyle(1001);
			errCorridor->SetLineWidth(0);
			errCorridor->Draw("E3 SAME");

			//legendRes->AddEntry(errCorridor, Form("#bf{Exp.} | #bf{SystErr} = %1.0f%% | #bf{AvgErr} = %5.1f%%", systemError*100, avgRelErrExp * 100.0), "f");
			//legendRes->AddEntry(errCorridor, Form("#bf{Exp. tolerance}   |    #bf{AvgErr} = %4.1f%%", avgRelErrExp * 100.0), "f");

			TLine *zeroLine = new TLine(0.0, 0.0, xData[n-1], 0.0);
			zeroLine->SetLineStyle(2);
			zeroLine->Draw("SAME");

			// Верхняя ось для Pad 2
			TGaxis *topAxisRes = new TGaxis(0.0, yAxisResMax, xData[n-1], yAxisResMax, 0.0, xData[n-1], 510, "-S");
			topAxisRes->SetTickSize(0.07);
			topAxisRes->SetLabelOffset(999);
			topAxisRes->Draw("SAME");

			// Правая ось для Pad 2 (Floating стиль)
			//double xRightAxisRes = 0.0 + (xMinLeg + 0.05) * (xAxisMax - 0.0);
			TGaxis *rightAxisRes = new TGaxis(xData[n-1], yAxisResMin, xData[n-1], yAxisResMax, 
											yAxisResMin, yAxisResMax, 
											YaxisRes->GetNdivisions(), // Синхронизация меток
											"+");
			rightAxisRes->SetTickSize(0.03);
			rightAxisRes->SetLabelOffset(999);
			rightAxisRes->Draw("SAME");

			pad2->RedrawAxis();
		}
	
		TGraph* splineRes = new TGraph(n, xData, res);
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
		double statErr = TMath::Sqrt(y / totalNExp);
    	double systErr = systemError * y;
    	double totalErr = TMath::Sqrt(statErr*statErr + systErr*systErr);

		graphExp->SetPoint(i, x, y);
		graphExp->SetPointError(i, 0, 0, totalErr, totalErr);
	}
	graphExp->SetMarkerStyle(25);
	graphExp->SetMarkerSize(3);
	graphExp->SetMarkerColor(kBlack);
	graphExp->Draw("P E SAME");

	
	legend->AddEntry(graphExp, Form("#scale[0.9]{%s}", parameters.legends["Exp"].Data()), "p");
	legend->Draw();
	mainCanvas->Update();

	std::string folderName = "plots";
	if (!fs::exists(folderName)) fs::create_directory(folderName);
	mainCanvas->SaveAs((folderName + "/" + canvasName + ".pdf").c_str());
}

const std::string FormCanvasName(std::string path){
	if (path.find("Li7") != std::string::npos) return "Ionisation cluster size distribution of Li7 at energy E=3.81 MeV";
	return "Ionisation cluster size distribution";
}