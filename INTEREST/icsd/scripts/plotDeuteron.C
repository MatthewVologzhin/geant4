// *********************************************************************
// To execute this macro under ROOT,
//   1 - launch ROOT (usually type 'root' at your machine's prompt)
//   2 - type '.X plot.C' at the ROOT session prompt
// This macro needs the output ROOT file
// *********************************************************************

#include <iomanip>
#include <vector>
#include <filesystem>

void SetLeafAddress(TNtuple* ntuple, const char* name, void* address);

namespace fs = std::filesystem;

const std::string FormCanvasName(std::string);

struct Parameters{
	std::map<std::string, Int_t> colors;
	std::map<std::string, std::string> paths;
	std::map<std::string, TString> legends;
	std::vector<std::string> names;
};

void plotDeuteron()
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
	
	/* Experimental data: Deuteron (16 MeV) */	
	const int n = 16;
	double xData[n] = {
		0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
		11.0, 12.0, 13.0, 15.0, 16.0
	};
	double yData[n] = {
		0.520000, 0.220000, 0.090000, 0.032000, 0.012000, 0.005500, 
		0.002500, 0.001200, 0.000550, 0.000300, 0.000160, 0.000055, 
		0.000070, 0.000011, 0.000022, 0.000016
	};
	
	/* Parameters */
	Parameters parameters;
	parameters.names = {"DNA2", "DNA4", "DNA6", "DNA8"};
	parameters.paths["DNA2"]  = "root/ICSD_deuteron_16-MeV_DNA2.root";
	parameters.paths["DNA4"]  = "root/ICSD_deuteron_16-MeV_DNA4.root";
	parameters.paths["DNA6"]  = "root/ICSD_deuteron_16-MeV_DNA6.root";
	parameters.paths["DNA8"]  = "root/ICSD_deuteron_16-MeV_DNA8.root";

	parameters.colors["DNA2"] = kRed;      
	parameters.colors["DNA4"] = kBlue;     
	parameters.colors["DNA6"] = kGreen+2;  
	parameters.colors["DNA8"] = kMagenta+2;

	parameters.legends["DNA2"] = Form("#bf{DNA Opt2}");
	parameters.legends["DNA4"] = Form("#bf{DNA Opt4}");
	parameters.legends["DNA6"] = Form("#bf{DNA Opt6}");
	parameters.legends["DNA8"] = Form("#bf{DNA Opt8}");
	parameters.legends["Exp"]  = Form("#bf{%s}", dataAuthors.Data());
	
	/* Global Axis & Legend Parameters (Стиль как в Range) */
	double lineWidth = 1.5;
	double yAxisMin = 3e-6;
	double yAxisMax = 1.0;
	double xAxisMin = -0.5;
	double xAxisMax = 21.5; 
	unsigned nbBins = xAxisMax - xAxisMin; 
	double totalNExp = 2.5e4;
	double systemError = 0.05;

	double xMinLeg = 0.7; 
	double xMaxLeg = 0.9;
	double yAxisResMin = -8.5;
	double yAxisResMax = 1.5;
	double authorsTextSize = 0.0252;

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
    pad1->SetLogy(); 
    pad1->SetBottomMargin(0);
    pad1->Draw();
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
		TFile *f = new TFile(parameters.paths[name].c_str());
		if (!f || f->IsZombie()) continue;
		TTree *tree = (TTree*)f->Get("ntuple_1");
	
		pad1->cd();
		double ion;
		tree->SetBranchAddress("ionisations", &ion);
	
		/* Histogram processing with Sumw2() */
		TH1F* hist = new TH1F(Form("h_%s", name.c_str()), "", nbBins, xAxisMin, xAxisMax);
		hist->Sumw2();
		int nbEntries = tree->GetEntries(); 
		for (int i=0; i<nbEntries; ++i){
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
			graph->Draw("AL");
			graph->GetXaxis()->SetLimits(0.0, xAxisMax);
			graph->GetXaxis()->SetRangeUser(0.0, xAxisMax);
			graph->GetYaxis()->SetRangeUser(yAxisMin, yAxisMax);
			graph->GetXaxis()->SetTitle("Ionisation number");
			graph->GetYaxis()->SetTitle("Frequency");
			graph->GetYaxis()->SetTitleSize(0.04);
			graph->GetYaxis()->SetTitleOffset(0.72);
			graph->GetXaxis()->CenterTitle(true);
			graph->GetYaxis()->CenterTitle(true);

			// ПРИЖИМАЕМ АВТОРОВ ВПРАВО (как в Range)
			TText* authorsText = new TText(xAxisMax, yAxisMax * 1.1, authors);
			authorsText->SetNDC();
			authorsText->SetTextAlign(31); 
			authorsText->SetTextSize(authorsTextSize);
			authorsText->SetTextFont(42);
			authorsText->DrawText(0.895, 0.91, authors);

			TLatex *texTitle = new TLatex(0.5, 0.955, (canvasName + "/u").c_str());
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
		double rmse = 0;
		for (size_t i=0; i<n; i++){
			double simVal = graph->Eval(xData[i]);
			res[i] = (yData[i] != 0) ? (yData[i] - simVal) / yData[i] : 0;
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
		
		if (counter == 0){
			hFrame->SetStats(0);
			hFrame->Draw("AXIS");
			XaxisRes->SetLimits(0.0, xAxisMax);
			XaxisRes->SetRangeUser(0.0, xAxisMax);
			XaxisRes->SetTitle("Ionisation number");
			XaxisRes->SetTitleSize(0.09);
			XaxisRes->SetLabelSize(0.09);
			XaxisRes->CenterTitle(true);
			XaxisRes->SetTickSize(0.07);
			XaxisRes->SetTitleOffset(1.2);
			
			// Скрываем метки под легендой
			XaxisRes->ChangeLabelByValue(18, -1, -1, -1, -1, -1, " ");
			XaxisRes->ChangeLabelByValue(20, -1, -1, -1, -1, -1, " ");
			XaxisRes->ChangeLabelByValue(22, -1, -1, -1, -1, -1, " ");
			XaxisRes->ChangeLabelByValue(24, -1, -1, -1, -1, -1, " ");
			
			YaxisRes->SetRangeUser(yAxisResMin, yAxisResMax);
			YaxisRes->SetTitle("#eta = [1 - Geant4/Exp]");
			YaxisRes->SetTitleSize(0.07);
			YaxisRes->SetTitleOffset(0.38);
			YaxisRes->SetLabelSize(0.06);
			YaxisRes->CenterTitle(true);
			YaxisRes->SetNdivisions(503);

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
			legendRes->AddEntry(errCorridor, Form("#bf{Exp. tolerance}   |    #bf{AvgErr} = %4.1f%%", avgRelErrExp * 100.0), "f");

			TLine *zeroLine = new TLine(0.0, 0.0, xAxisMax, 0.0);
			zeroLine->SetLineStyle(2);
			zeroLine->Draw("SAME");

			// Верхняя ось для Pad 2
			TGaxis *topAxisRes = new TGaxis(0.0, yAxisResMax, xAxisMax, yAxisResMax, 0.0, xAxisMax, 510, "-S");
			topAxisRes->SetTickSize(0.07);
			topAxisRes->SetLabelOffset(999);
			topAxisRes->Draw("SAME");

			// Правая ось для Pad 2 (Floating стиль)
			double xRightAxisRes = 0.0 + (xMinLeg + 0.05) * (xAxisMax - 0.0);
			TGaxis *rightAxisRes = new TGaxis(xRightAxisRes, yAxisResMin, xRightAxisRes, yAxisResMax, 
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
		legendRes->AddEntry(splineRes, Form("#bf{%s}  |   #bf{#chi^{2}} = %5.2f", name.c_str(), normChi2), "l");
		legendRes->Draw();
		
		counter++;
	}
	
	/* Experimental data overlay */
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

	
	legend->AddEntry(graphExp, parameters.legends["Exp"], "p");
	legend->Draw();
	mainCanvas->Update();

	std::string folderName = "plots";
	if (!fs::exists(folderName)) fs::create_directory(folderName);
	mainCanvas->SaveAs((folderName + "/" + canvasName + ".pdf").c_str());
}

const std::string FormCanvasName(std::string path){
	if (path.find("deuteron") != std::string::npos) return "Ionisation cluster size distribution of deuteron at energy E=8 MeV";
	return "Ionisation cluster size distribution";
}