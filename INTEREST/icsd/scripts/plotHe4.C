// *********************************************************************
// To execute this macro under ROOT,
//   1 - launch ROOT (usually type 'root' at your machine's prompt)
//   2 - type '.X plot.C' at the ROOT session prompt
// This macro needs the output ROOT file
// *********************************************************************

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
	// Data derived from user input, rounded/mapped to integer cluster sizes (nu) based on the shape of Hilgers Figure 23 (4MeV He, dp=0)
	const int n = 43;
	double xData[n] = {
		0.0,  1.0,  2.0,  3.0,  4.0,  5.0,  6.0,  7.0,  8.0,  9.0,
		10.0, 11.0, 12.0, 13.0, 14.0, 15.0, 16.0, 17.0, 18.0, 19.0,
		20.0, 21.0, 22.0, 23.0, 24.0, 25.0, 26.0, 27.0, 28.0, 29.0,
		30.0, 31.0, 32.0, 33.0, 34.0, 35.0, 36.0, 37.0, 38.0, 39.0,
		40.0, 41.0, 42.0
	};

	double yData[n] = {
		0.0166, 0.0349, 0.0600, 0.0920, 0.1082, 0.1222, 0.1119, 0.0951, 0.0836, 0.0683, // 0-9
		0.0566, 0.0451, 0.0346, 0.0257, 0.0192, 0.0147, 0.0096, 0.0075, 0.0052, 0.0041, // 10-19
		0.0029, 0.0024, 0.0018, 0.0013, 0.0010, 0.00063, 0.00056, 0.00039, 0.00040, 0.00022, // 20-29 (Небольшая аномалия на 28)
		0.00016, 0.00013, 0.000063, 0.000078, 0.000068, 0.000043, 0.000033, 0.000011, 0.000014, 0.000011, // 30-39
		0.000014, 0.000011, 0.000011 // 40-42
	};
	
	/* Parameters */
	Parameters parameters;
	parameters.names = {"DNA2", "DNA4", "DNA6", "DNA8", "DNA2MI", "DNA8MI"};
	parameters.paths["DNA2"]  = "root/ICSD_alpha_4-MeV_DNA2.root";
	parameters.paths["DNA2MI"]  = "root/ICSD_alpha_4-MeV_DNA2_MultiIoni.root";
	parameters.paths["DNA4"]  = "root/ICSD_alpha_4-MeV_DNA4.root";
	parameters.paths["DNA6"]  = "root/ICSD_alpha_4-MeV_DNA6.root";
	parameters.paths["DNA8"]  = "root/ICSD_alpha_4-MeV_DNA8.root";
	parameters.paths["DNA8MI"]  = "root/ICSD_alpha_4-MeV_DNA8_MultiIoni.root";

	parameters.colors["DNA2"] = kRed;      
	parameters.colors["DNA2MI"] = kRed+2;
	parameters.colors["DNA4"] = kBlue;     
	parameters.colors["DNA6"] = kGreen+2;  
	parameters.colors["DNA8"] = kMagenta+2;
	parameters.colors["DNA8MI"] = kMagenta+4;

	parameters.legends["DNA2"] = Form("#bf{DNA Opt2}");
	parameters.legends["DNA2MI"] = Form("#bf{DNA Opt2 MI}");
	parameters.legends["DNA4"] = Form("#bf{DNA Opt4}");
	parameters.legends["DNA6"] = Form("#bf{DNA Opt6}");
	parameters.legends["DNA8"] = Form("#bf{DNA Opt8}");
	parameters.legends["DNA8MI"] = Form("#bf{DNA Opt8 MI}");
	parameters.legends["Exp"]  = Form("#bf{%s}", dataAuthors.Data());
	
	/* Axis & Legend global settings */
	double lineWidth = 1.5;
	double yAxisMin = 1e-5;
	double yAxisMax = 1;

	// FIXED BINNING: от -0.5 до 44.5 (бины шириной 1.0)
	double xAxisMin = -0.5;
	double xAxisMax = 44.5; 
	unsigned nbBins = 45; 

	// Координаты легенды NDC
	double xMinLeg = 0.7; 
	double xMaxLeg = 0.9;
	double yAxisResMin = -1.2;
	double yAxisResMax = 1.2;
	double authorsTextSize = 0.0252;

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
	
		/* Histogram with FIXED BINNING */
		TH1F* hist = new TH1F(Form("h_%s", name.c_str()), "", nbBins, xAxisMin, xAxisMax);
		int nbPts = tree->GetEntries(); 
		for (size_t i=0; i<nbPts; ++i){
			tree->GetEntry(i);
			hist->Fill(ion);
		}

		double normFactor = hist->Integral();
		TGraph* graph = new TGraph();
		for (int j=1; j<=nbBins; ++j){
			double center = hist->GetBinCenter(j);
			double content = hist->GetBinContent(j);
			if (content > 0) graph->SetPoint(graph->GetN(), center, content/normFactor);
		}
		
		graph->SetLineWidth(lineWidth);
		graph->SetLineColor(parameters.colors[name]);
		
		if (counter == 0){
			graph->SetTitle(canvasName.c_str());
			graph->Draw("AL");
			graph->GetXaxis()->SetLimits(0.0, xAxisMax);
			graph->GetXaxis()->SetRangeUser(0.0, xAxisMax);
			graph->GetYaxis()->SetRangeUser(yAxisMin, yAxisMax);
			graph->GetXaxis()->SetTitle("Ionisation number");
			graph->GetYaxis()->SetTitle("Frequency");
			graph->GetYaxis()->SetTitleSize(0.062);
			graph->GetYaxis()->SetTitleOffset(0.45);
			graph->GetXaxis()->CenterTitle(true);
			graph->GetYaxis()->CenterTitle(true);
			graph->GetYaxis()->ChangeLabelByValue(1e-5, -1, -1, -1, -1, -1, " ");

			// ПРИЖИМАЕМ АВТОРОВ ВПРАВО
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
			XaxisRes->SetLimits(0.0, xAxisMax);
			XaxisRes->SetRangeUser(0.0, xAxisMax);
			XaxisRes->SetTitle("Ionisation number");
			XaxisRes->SetTitleSize(0.09);
			XaxisRes->SetLabelSize(0.09);
			XaxisRes->CenterTitle(true);
			XaxisRes->ChangeLabelByValue(xAxisMax, -1, -1, -1, -1, -1, " ");
			
			YaxisRes->SetRangeUser(yAxisResMin, yAxisResMax);
			YaxisRes->SetTitle("#eta = [1 - Geant4/Exp]");
			YaxisRes->SetTitleSize(0.07);
			YaxisRes->SetTitleOffset(0.38);
			YaxisRes->SetLabelSize(0.09);
			YaxisRes->SetNdivisions(305);
			YaxisRes->CenterTitle(true);

			TLine *zeroLine = new TLine(0.0, 0.0, xAxisMax, 0.0);
			zeroLine->SetLineStyle(2);
			zeroLine->Draw();
		}
		
		TGraph* splineRes = new TGraph(n, xData, res);
		splineRes->SetLineWidth(lineWidth);
		splineRes->SetLineColor(parameters.colors[name]);
		splineRes->Draw("L SAME");

		legendRes->AddEntry(splineRes, Form("#bf{%s} | #bf{RMSE} = %.4f", name.c_str(), rmse), "l");
		legendRes->Draw();
		
		counter++;
	}
	
	/* Experimental data overlay */
	pad1->cd();
	TGraph* graphExp = new TGraph(n, xData, yData);
	graphExp->SetMarkerStyle(25);
	graphExp->SetMarkerSize(3);
	graphExp->SetMarkerColor(kBlack);
	graphExp->Draw("P SAME");
	legend->AddEntry(graphExp, parameters.legends["Exp"], "p");
	
	legend->Draw();
	mainCanvas->Update();

	std::string folderName = "plots";
	if (!fs::exists(folderName)) fs::create_directory(folderName);
	mainCanvas->SaveAs((folderName + "/" + canvasName + ".pdf").c_str());
}

const std::string FormCanvasName(std::string path){
	if (path.find("alpha") != std::string::npos) return "Ionisation Cluster Size Distribution of alpha at energy E=4 MeV";
	return "Ionisation Cluster Size Distribution";
}
