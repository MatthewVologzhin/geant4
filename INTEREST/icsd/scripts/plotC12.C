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
	TString dataAuthors = "Hilgers (2017)";
	
	/* Picture quality configuration */
	gStyle->SetCanvasPreferGL(kTRUE);
	gStyle->SetPadBorderSize(0);																										  
	
	/* Experimental data: C12 (88 MeV) */	
	const int n = 38;
	double xData[n] = {
		1.031776335327308, 2.0109456535573376, 2.992321072504348, 3.975393492002885,
		5.0422977387467665, 5.945102032158159, 6.931398752704597, 7.918034873361334,
		8.987654320987657, 9.975648042085611, 10.963811463238725, 11.952314284502142,
		13.023630732679988, 14.012642654108864, 15.002163675703192, 15.909040770438247,
		16.981205718891864, 17.970896440541345, 18.960587162190826, 19.950447583895468,
		20.940477705655255, 22.01264265410886, 22.920537949174836, 24.07517712443257,
		24.982902719443388, 25.97276314114802, 26.962623562852663, 27.953162784777902,
		29.02549743328666, 30.099019982181495, 31.00657587713716, 32.07908022570107,
		32.98714522082219, 33.978023842857745, 34.967375164396934, 36.04123711340207,
		37.02804293411395, 38.019260956259814
	};
	double yData[n] = {
		0.002087313244716945, 0.006839928255402964, 0.016462464269211954, 0.031249664626297947,
		0.047908477682064736, 0.06839928255402965, 0.08270385876972713, 0.09536333778926674,
		0.10000000000000000, 0.09536333778926674, 0.08880831053642047, 0.07886916020333298,
		0.06522783886743432, 0.05394593973546805, 0.04154862931544807, 0.033556232581823706,
		0.024646348057119207, 0.018537076282360607, 0.013942154687651545, 0.0102402197940476,
		0.0073447899721821915, 0.005394593973546809, 0.003778501401404567, 0.0027101272981508054,
		0.0019438367733440394, 0.001427707284041629, 0.0010486210143040428, 0.0007004236871084298,
		0.0005023783646072937, 0.00030516595595401813, 0.0002241379853142465, 0.0001569915540948596,
		0.00010738109667130533, 0.00006839928255402965, 0.000053945939735468084, 0.00003124966462629795,
		0.000035187770646174344, 0.000021374546404928186
	};
	
	/* Parameters (Standard set: DNA Opt2, 4, 6, 8) */
	Parameters parameters;
	//parameters.names = {"DNA2", "DNA4", "DNA6", "DNA8", "DNA2MI", "DNA8MI"};
	parameters.names = {"DNA2", "DNA8", "DNA2MI", "DNA8MI"};
	parameters.paths["DNA2"]  = "root/ICSD_C12_88-MeV_DNA2.root";
	parameters.paths["DNA4"]  = "root/ICSD_C12_88-MeV_DNA4.root";
	parameters.paths["DNA6"]  = "root/ICSD_C12_88-MeV_DNA6.root";
	parameters.paths["DNA8"]  = "root/ICSD_C12_88-MeV_DNA8.root";
	parameters.paths["DNA2MI"]  = "root/ICSD_C12_88-MeV_DNA2_MultiIoni.root";
	parameters.paths["DNA8MI"]  = "root/ICSD_C12_88-MeV_DNA8_MultiIoni.root";

	parameters.colors["DNA2"] = kRed;      
	parameters.colors["DNA4"] = kBlue;     
	parameters.colors["DNA6"] = kGreen+2;  
	parameters.colors["DNA8"] = kMagenta+2;
	
	parameters.colors["DNA2MI"] = kRed+2;
	parameters.colors["DNA8MI"] = kMagenta+4;

	parameters.legends["DNA2"] = Form("#bf{DNA Opt2}");
	parameters.legends["DNA4"] = Form("#bf{DNA Opt4}");
	parameters.legends["DNA6"] = Form("#bf{DNA Opt6}");
	parameters.legends["DNA8"] = Form("#bf{DNA Opt8}");
	parameters.legends["Exp"]  = Form("#bf{%s}", dataAuthors.Data());
	
	parameters.legends["DNA2MI"] = Form("#bf{DNA Opt2 MI}");
	parameters.legends["DNA8MI"] = Form("#bf{DNA Opt8 MI}");
	
	/* Global Axis & Legend Parameters */
	double lineWidth = 1.5;
	double yAxisMin = 1e-5;
	double yAxisMax = 1;
	double xAxisMin = -0.5;
	double xAxisMax = 44.5; // Расширенный диапазон для C12
	unsigned nbBins = 45; 

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
	
	TLegend *legend = new TLegend(xMinLeg, 0.55, xMaxLeg, 0.9);
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
	
		/* Histogram and Graph with FIXED BINNING */
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
			// Убираем лейбл на краю шкалы, если он мешает
			XaxisRes->ChangeLabelByValue(45, -1, -1, -1, -1, -1, " ");
			
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
	if (path.find("C12") != std::string::npos) return "Ionisation Cluster Size Distribution of C12 at energy E=88 MeV";
	return "Ionisation Cluster Size Distribution";
}
