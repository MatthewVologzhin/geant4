// *********************************************************************
// To execute this macro under ROOT,
//   1 - launch ROOT (usually type 'root' at your machine's prompt)
//   2 - type '.X plot.C' at the ROOT session prompt
// This macro needs the output ROOT file
// *********************************************************************

void SetLeafAddress(TNtuple* ntuple, const char* name, void* address);

namespace fs = std::filesystem;

const TString FormCanvasName(std::string, TString, TString);

struct Parameters{
	std::map<std::string, Int_t> colors;
	std::map<std::string, std::string> paths;
	std::map<std::string, TString> legends;
	std::map<std::string, TString> markers;
	std::vector<std::string> physnames;
	std::vector<std::string> structnames;
};

void plotProtonBio()
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
	TString structureName = "Cytoskeleton";
	TString particleName = "proton";
	
	/* Picture quality configuration */
	gStyle->SetCanvasPreferGL(kTRUE);
	gStyle->SetPadBorderSize(0);																										  
	
	/* Experimental data: Protons (20 MeV) */	
	const int n = 12;
	double xData[n] = {
		0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 10.99
	};
	double yData[n] = {
		0.780000, 0.150000, 0.035000, 0.011000, 0.003800, 0.001600,
		0.000800, 0.000400, 0.000150, 0.000180, 0.000048, 0.000060
	};
	
	/* Parameters (Standard set: DNA Opt2, 4, 6, 8) */
	Parameters parameters;
	parameters.structnames = {"Cytoskeleton", "Histon", "NMDA", "Ribosome"};
	parameters.markers["Cytoskeleton"] = 20; // Полный круг (стандарт)
	parameters.markers["Histone"]      = 21; // Полный квадрат
	parameters.markers["NMDA"]         = 22; // Полный треугольник (вершиной вверх)
	parameters.markers["Ribosome"]     = 33; // Полный ромб (Diamond)

	parameters.physnames = {"DNA2", "DNA4", "DNA6", "DNA8"};
	parameters.paths["DNA2"] = std::string("root/ICSD") + "_" + particleName + "_" + "20-MeV_DNA2" + "_" + structureName + ".root";
	parameters.paths["DNA4"] = std::string("root/ICSD") + "_" + particleName + "_" + "20-MeV_DNA4" + "_" + structureName + ".root";
	parameters.paths["DNA6"] = std::string("root/ICSD") + "_" + particleName + "_" + "20-MeV_DNA6" + "_" + structureName + ".root";
	parameters.paths["DNA8"] = std::string("root/ICSD") + "_" + particleName + "_" + "20-MeV_DNA8" + "_" + structureName + ".root";
	parameters.colors["DNA2"] = kRed;      
	parameters.colors["DNA4"] = kBlue;     
	parameters.colors["DNA6"] = kGreen+2;  
	parameters.colors["DNA8"] = kMagenta+2;
	parameters.legends["DNA2"] = Form("#bf{DNA Opt2}");
	parameters.legends["DNA4"] = Form("#bf{DNA Opt4}");
	parameters.legends["DNA6"] = Form("#bf{DNA Opt6}");
	parameters.legends["DNA8"] = Form("#bf{DNA Opt8}");
	parameters.legends["Exp"]  = Form("#bf{%s}", dataAuthors.Data());
	
	/* Global Axis & Legend Parameters */
	double lineWidth = 1.5;
	double yAxisMin = 1e-5;
	double yAxisMax = 1;

	/* 1. ПРЕДВАРИТЕЛЬНЫЙ ПОИСК МАКСИМАЛЬНОЙ ИОНИЗАЦИИ */
    double globalMaxX = 0;
    for (auto name : parameters.physnames) {
        TFile *fTemp = TFile::Open(parameters.paths[name].c_str());
        if (!fTemp || fTemp->IsZombie()) continue;
        TTree *treeTemp = (TTree*)fTemp->Get("ntuple_1");
        if (treeTemp) {
            double m = treeTemp->GetMaximum("ionisations");
            if (m > globalMaxX) globalMaxX = m;
        }
        fTemp->Close();
    }
    
    // Если данные не найдены, ставим дефолт
    if (globalMaxX <= 0) globalMaxX = 15; 

    /* Динамические параметры осей */
    double xAxisMin = -0.5;
    double xAxisMax = globalMaxX + 0.5; 
    unsigned int nbBins = (unsigned int)(globalMaxX + 1); 

	// Фиксированные координаты легенды NDC
	double xMinLeg = 0.7; 
	double xMaxLeg = 0.9;
	double authorsTextSize = 0.0252;


	/* Canvas initialization */
	TString canvasName = FormCanvasName(parameters.paths["DNA2"], particleName, structureName);
	TCanvas *mainCanvas = new TCanvas("mainCanvas", canvasName.Data(), 1920, 1080);	
	
    TPad* pad = new TPad("", "main plot", 0.0, 0.0, 1.0, 1.0);
	double leftMargin = 0.10;
	double rightMargin = 0.10; // Фиксируем отступ справа
	pad->SetLeftMargin(leftMargin);
	pad->SetRightMargin(rightMargin);
    pad->SetLogy(); 
    pad->Draw();
    
	TLegend *legend = new TLegend(xMinLeg, 0.5, xMaxLeg, 0.9);
	legend->SetTextSize(0.035);
	legend->SetBorderSize(1);
	legend->SetEntrySeparation(0.5);

	int counter = 0;
	TH1F* hFrame1 = new TH1F("hFrame1", canvasName.Data(), nbBins, xAxisMin, xAxisMax);
	TAxis* xAxis = hFrame1->GetXaxis();
	TAxis* yAxis = hFrame1->GetYaxis();
	for (auto name : parameters.physnames){
		TFile *f =  new TFile(parameters.paths[name].c_str());
		if (!f || f->IsZombie()) continue;
		TTree *tree = (TTree*)f->Get("ntuple_1");
	
		pad->cd();
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

			hFrame1->SetStats(0);
			hFrame1->Draw("AXIS"); // Рисуем рамку первой
			xAxis->SetTitle("Ionisation number");
			xAxis->SetTitleSize(0.07);
			xAxis->SetTitleOffset(0.5);
			xAxis->CenterTitle(true);
			xAxis->SetTickSize(0.07);
			xAxis->SetTickLength(0.037);
			xAxis->SetLimits(xAxisMin+0.5, xAxisMax-0.5);
			yAxis->SetRangeUser(yAxisMin, yAxisMax);
			yAxis->ChangeLabelByValue(1e-5, -1, -1, -1, -1, -1, " ");
			yAxis->SetTitle("Frequency");
			yAxis->SetTitleSize(0.07);
			yAxis->SetTitleOffset(0.5);
			yAxis->CenterTitle(true);
			pad->Update();
			pad->RedrawAxis();

			// ПРИЖИМАЕМ АВТОРОВ ВПРАВО К ГРАНИЦЕ xAxisMax
			TText* authorsText = new TText(xAxisMax, yAxisMax * 1.1, authors);
			authorsText->SetNDC();
			authorsText->SetTextAlign(31); 
			authorsText->SetTextSize(authorsTextSize);
			authorsText->SetTextFont(42);
			authorsText->DrawText(0.895, 0.91, authors);

			TLatex *texTitle = new TLatex(0.5, 0.955, (canvasName).Data());
			texTitle->SetNDC();
			texTitle->SetTextAlign(22); // Центр-Центр
			texTitle->SetTextFont(42);
			texTitle->SetTextSize(0.060);
			texTitle->Draw();
			counter++;
		}
		graph->Draw("L SAME");
		
		
		/* Additional axes for Pad */
		double xMin = hFrame1->GetXaxis()->GetXmin();
		double xMax = hFrame1->GetXaxis()->GetXmax();
		double yMax = hFrame1->GetYaxis()->GetXmax();
		int nDiv    = hFrame1->GetXaxis()->GetNdivisions();

		// Top Axis
		// Параметры: xStart, yStart, xEnd, yEnd, vMin, vMax, nDiv, chopt
		TGaxis *topAxis = new TGaxis(xMin, yMax, xMax, yMax, xMin, xMax, nDiv, "-");
		topAxis->SetTickSize(hFrame1->GetXaxis()->GetTickLength());
		topAxis->SetLabelOffset(999); // Прячем цифры, если нужны только черточки
		topAxis->Draw();
		
		// Right Axis
		TGaxis *rightAxis = new TGaxis(xAxisMax-0.5, yAxisMin, xAxisMax-0.5, yAxisMax, yAxisMin, yAxisMax, 510, "+GU");
		rightAxis->Draw();
		legend->AddEntry(graph, Form("#scale[0.9]{%s}", parameters.legends[name].Data()), "l");
	}
	

	//legend->Draw();
	mainCanvas->Update();

	std::string folderName = "plots";
	if (!fs::exists(folderName)) fs::create_directory(folderName);
	mainCanvas->SaveAs((folderName + "/" + canvasName + ".pdf").Data());
}

const TString FormCanvasName(std::string path, TString particleName, TString structureName){
	if (path.find("proton") != std::string::npos) return std::string("ICSD of ") + particleName + " at energy E=20 MeV for " + structureName;
	return "Ionisation cluster size distribution";
}