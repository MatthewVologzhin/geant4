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

void plotHe4()
{
	/* Auxilary variables */
	double nm, um, mm, m, MeV;
	m =  1e9; 
	mm = 1e-3*m;
	um = 1e-6*m;
	nm = 1e-9*m;
	MeV = 1e6*1.602e-19;
	
	TString authors = "Vologzhin & Batmunkh & Ivanchenko (2026)";
	TString dataAuthors = "Hilgers (2017)";
	
	/* Picture quality configuration */
	gStyle->SetCanvasPreferGL(kTRUE);
	gStyle->SetPadBorderSize(0);																										  
	
	/* Experimental data: Alpha (He4, 4 MeV) */	
	/*const int n = 40;
	double xData[n] = {
		0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 
		11.0, 12.0, 13.0, 14.0, 15.0, 16.0, 17.0, 18.0, 19.0, 20.0, 
		21.0, 22.0, 23.0, 24.0, 25.0, 26.0, 27.0, 29.0, 30.0, 32.0, 33.0, 
		34.0, 35.0, 36.0, 37.0, 38.0, 40.0, 41.0
	};
	double yData[n] = {
		0.016360147852916288, 0.034631364000086776, 0.060451233746279635, 0.08486676949364359, 0.10896786998943343, 
		0.12303481270458483, 0.11871875350137238, 0.10291640097202498, 0.08639567717542654, 0.06899017615052154, 
		0.05865405787873787, 0.04667454422568845, 0.035005703892480484, 0.02614329008659324, 0.01903518696266078, 
		0.014458666990041051, 0.010102293562625176, 0.007457549881666016, 0.005293134607018148, 
		0.004035030689202465, 0.002957476722020576, 0.0024214338407523516, 0.0018003232060964998, 0.0013675199062170372, 
		0.0009810764351832612, 0.0006278332166113309, 0.0005560512952220835, 0.0003876834717881385, 0.00021584126758335875, 
		0.00015595734500777988, 0.00012999045024286992, 0.00006206996366174329, 0.00007998218148685374, 0.00006571968616650389, 
		0.00004296771224343196, 0.00003358399859170438, 0.000009973259805297734, 0.000014355734898098783, 0.000010335840935061586
	};*/
	const int n = 43;
	double xData[n] = {
		0.0,  1.0,  2.0,  3.0,  4.0,  5.0,  6.0,  7.0,  8.0,  9.0,
		10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0, 17.0, 18.0, 19.0,
		20.0, 21.0, 22.0, 23.0, 24.0, 25.0, 26.0, 27.0, 28.0, 29.0,
		30.0, 31.0, 32.0, 33.0, 34.0, 35.0, 36.0, 37.0, 38.0, 39.0,
		40.0, 41.0, 42.0
	};

	double yData[n] = {
		0.0166, 0.0349, 0.0600, 0.0920, 0.1082, 0.1222, 0.1119, 0.0951, 0.0836, 0.0683, 
		0.0566, 0.0451, 0.0346, 0.0257, 0.0192, 0.0147, 0.0096, 0.0075, 0.0052, 0.0041, 
		0.0029, 0.0024, 0.0018, 0.0013, 0.0010, 0.00063, 0.00056, 0.00039, 0.00040, 0.00022, 
		0.00016, 0.00013, 0.000063, 0.000078, 0.000068, 0.000043, 0.000033, 0.000011, 0.000014, 0.000011, 
		0.000014, 0.000011, 0.000011 
	};
	
	/* Parameters */
	Parameters parameters;
	parameters.names = {"DNA2", "DNA4", "DNA6", "DNA8"};
	parameters.paths["DNA2"]  = "root/ICSD_alpha_4-MeV_DNA2.root";
	parameters.paths["DNA4"]  = "root/ICSD_alpha_4-MeV_DNA4.root";
	parameters.paths["DNA6"]  = "root/ICSD_alpha_4-MeV_DNA6.root";
	parameters.paths["DNA8"]  = "root/ICSD_alpha_4-MeV_DNA8.root";

	parameters.colors["DNA2"] = kRed;      
	parameters.colors["DNA4"] = kBlue;     
	parameters.colors["DNA6"] = kGreen+2;  
	parameters.colors["DNA8"] = kMagenta+2;

	parameters.legends["DNA2"] = Form("#bf{DNA Opt2}");
	parameters.legends["DNA4"] = Form("#bf{DNA Opt4}");
	parameters.legends["DNA6"] = Form("#bf{DNA Opt6}");
	parameters.legends["DNA8"] = Form("#bf{DNA Opt8}");
	parameters.legends["Exp"]  = Form("#bf{%s}", dataAuthors.Data());
	
	/* Global Axis & Legend Parameters (Стиль Range) */
	double lineWidth = 1.5;
	double yAxisMin = 1e-6;
	double yAxisMax = 1.0;
	double xAxisMin = -0.5;
	double xAxisMax = 44.5; 
	unsigned nbBins = xAxisMax - xAxisMin; 
	double totalNExp = 1e4;
	double systemError = 0.05;

	double xMinLeg = 0.7; 
	double xMaxLeg = 0.9;
	double yAxisResMin = -.75;
	double yAxisResMax = 1.2;
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
	
	TLegend *legend = new TLegend(xMinLeg, 0.5, xMaxLeg, 0.9);
	legend->SetTextSize(0.035);
	legend->SetBorderSize(1); 
	legend->SetEntrySeparation(0.5);
	
	//TLegend *legendRes = new TLegend(xMinLeg, 0.3, xMaxLeg, 1);
	//legendRes->SetTextSize(0.05);

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
			//yAxis->ChangeLabelByValue(1, -1, -1, -1, -1, -1, "10^{0}");
			yAxis->ChangeLabelByValue(1e-6, -1, -1, -1, -1, -1, " ");
			yAxis->SetTitle("Frequency");
			yAxis->SetTitleSize(0.07);
			yAxis->SetTitleOffset(0.40);
			yAxis->CenterTitle(true);
			graph->Draw("L SAME");
			pad1->Update();
			pad1->RedrawAxis();

			// ПРИЖИМАЕМ АВТОРОВ ВПРАВО (как в Range)
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
		
		/* Residuals Calculation */
		double res[n];
		double rmse = 0;
		for (size_t i=0; i<n; i++){
			double simVal = graph->Eval(xData[i]);
			res[i] = (yData[i] != 0) ? (yData[i] - simVal) / yData[i] : 0;
			res[i] = (res[i] > 1.) ? 1. : res[i];
			rmse += TMath::Power(res[i], 2);
		}
		rmse = TMath::Sqrt(rmse/n);

		/* Chi2 calculations */
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
	
		//legend->AddEntry(graph, parameters.legends[name], "l");
		legend->AddEntry(graph, Form("#scale[0.9]{%s  |   #bf{#chi^{2}} = %5.2f}", parameters.legends[name].Data(), normChi2), "l");
	
		/* Residuals Calculation */
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
			
			// Скрываем метки под легендой
			//XaxisRes->ChangeLabelByValue(40, -1, -1, -1, -1, -1, " ");
			// XaxisRes->ChangeLabelByValue(50, -1, -1, -1, -1, -1, " ");
			
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

	
	legend->AddEntry(graphExp, Form("#scale[0.9]{%s}", parameters.legends["Exp"].Data()), "p");
	legend->Draw();
	mainCanvas->Update();

	std::string folderName = "plots";
	if (!fs::exists(folderName)) fs::create_directory(folderName);
	mainCanvas->SaveAs((folderName + "/" + canvasName + ".pdf").c_str());
}

const std::string FormCanvasName(std::string path){
	if (path.find("alpha") != std::string::npos) return "Ionisation cluster size distribution of alpha at energy E=1 MeV";
	return "Ionisation cluster size distribution";
}