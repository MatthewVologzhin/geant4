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

void plotProton()
{
  /* Auxilary variables */
	double nm, um, mm, m, MeV;
	m =  1e9; 		//you can change this value
	mm = 1e-3*m;	// ... but not this!
	um = 1e-6*m;	// ... but not this!
	nm = 1e-9*m;	// ... but not this!
	MeV = 1e6*1.602e-19;
	
	TString authors = "Vologzhin & Batmunkh & Ivanchenko (2026)";
	TString dataAuthors = "Conte (2012)";
	
	/* Picture quality configuration */
	gStyle->SetCanvasPreferGL(kTRUE);
	gStyle->SetPadBorderSize(0);																										  
	
	/* Experimental data: Protons */	
	const int n = 12;
	double xData[n] = {
		0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0, 11.0
	};
	double yData[n] = {
		0.780000, 0.150000, 0.035000, 0.011000, 0.003800, 0.001600,
		0.000800, 0.000400, 0.000150, 0.000180, 0.000048, 0.000060
	};
	
	
	/* Parameters */
	Parameters parameters;
	//parameters.names = {"DNA2", "DNA4", "DNA6", "DNA8", "S4"};
	parameters.names = {"DNA2", "DNA4", "DNA6", "DNA8", "S4"};
	parameters.paths["DNA2"]  = "root/ICSD_proton_20-MeV_DNA2.root";
	parameters.paths["DNA4"]  = "root/ICSD_proton_20-MeV_DNA4.root";
	parameters.paths["DNA6"]  = "root/ICSD_proton_20-MeV_DNA6.root";
	parameters.paths["DNA8"]  = "root/ICSD_proton_20-MeV_DNA8.root";
	parameters.paths["S4"] 	  = "root/ICSD_proton_20-MeV_S4.root";
	parameters.colors["DNA2"] = kAzure-14;
	parameters.colors["DNA4"] = kAzure-7;
	parameters.colors["DNA6"] = kAzure;
	parameters.colors["DNA8"] = kAzure+7;
	parameters.colors["S4"]   = kRed;
	parameters.legends["DNA2"] = Form("#bf{DNA Opt2}");
	parameters.legends["DNA4"] = Form("#bf{DNA Opt4}");
	parameters.legends["DNA6"] = Form("#bf{DNA Opt6}");
	parameters.legends["DNA8"] = Form("#bf{DNA Opt8}");
	parameters.legends["S4"]   = Form("#bf{EM Standard Opt4}");
	parameters.legends["Exp"]  = Form("#bf{%s}", dataAuthors.Data());
	
	/* Other parameters */
	double lineWidth = 1.5;
	double yAxisMin, yAxisMax, xAxisMin, xAxisMax, xMinLegend;
	//double authorsTextSize = 0.0295;
	double authorsTextSize = 0.0252;
	double yAxisResMin, yAxisResMax;
	yAxisMin = 1e-5;
	yAxisMax = 1;
	//yAxisMin = 0.5*(*std::min_element(std::begin(yData), std::end(yData)));
	//yAxisMax = 4*(*std::max_element(std::begin(yData), std::end(yData)));
	xAxisMin = xData[0];
	xAxisMax = 1.2*xData[n-1];
	xMinLegend = 0.1+0.75*(xData[n-1]-xAxisMin)/(xAxisMax-xAxisMin);
	yAxisResMin = -2.2;
	yAxisResMax = 1.2;
	unsigned nbBins = 1.2*n;
	double binWidth = xAxisMax/nbBins;

	
	/* Canvas name from file name */
	std::string canvasName = FormCanvasName(parameters.paths["DNA2"]);
	TCanvas *mainCanvas = new TCanvas("mainCanvas", canvasName.c_str(),
									  1920, 1080);	
	
	/* Up and down pad initialization */
    TPad *pad1 = new TPad("pad1", "main plot", 0.0, 0.3, 1.0, 1.0); // Верхний Pad для основного графика
    TPad *pad2 = new TPad("pad2", "residuals", 0.0, 0.0, 1.0, 0.3); // Нижний Pad для графика остатков
    //pad1->SetLogx();
    pad1->SetLogy(); 
    pad1->SetBottomMargin(0);
    pad1->Draw();
    pad2->SetTopMargin(0);    
    pad2->SetBottomMargin(0.3); //pad2->SetFillColor(kBlack); //pad2->SetFrameFillColor(kBlack);
    //pad2->SetLogx();
    pad2->Draw();
    
    TH1F* hFrame = new TH1F("hFrame", "", 100, 0.8, 120);
	TAxis *XaxisRes = hFrame->GetXaxis();
	TAxis *YaxisRes = hFrame->GetYaxis();
	
	TLegend *legend = new TLegend(xMinLegend, 0.6, 0.9, 0.9);
	legend->SetTextSize(0.05);
	TLegend *legendRes = new TLegend(xMinLegend, 0.3, 0.9, 1);
	legendRes->SetTextSize(0.055);
	int counter = 0;
	for (auto name : parameters.names){
		/* Trees initialization */
		std::string path = parameters.paths[name];
		TFile *f =  new TFile(path.c_str());
		if (f->IsZombie()){
			continue;
		} 
		TTree *tree = (TTree*)f->Get("ntuple_1");
	
		/* Upper pad */
		pad1->cd();
		double ion;
		tree->SetBranchAddress("ionisations", &ion);
	
		/* Data initialization */
		TGraph* graph = new TGraph();
		TH1F* hist = new TH1F(name.c_str(), "", nbBins, 0, xAxisMax);
		int nbPts = tree->GetEntries(); 
		for (size_t i=0; i<nbPts; ++i){
			tree->GetEntry(i);
			hist->Fill(ion);
		}
		double normFactor = .0;
		for (size_t j=1; j<=nbBins; ++j){
			normFactor += hist->GetBinContent(j);
		
		}
		double center, content;
		for (size_t j=1; j<=nbBins; ++j){
			center = hist->GetBinCenter(j)-0.5*binWidth;
			if (j!=1) center = center + (gRandom->Uniform(0,1))*binWidth;
			content = hist->GetBinContent(j);
			graph->SetPoint(graph->GetN(), center, content/normFactor);
		}
		
		
		
		graph->SetLineWidth(lineWidth);
		Int_t color = parameters.colors[name];
		graph->SetLineColor(color);
		graph->SetTitle(canvasName.c_str());
		TAxis *Xaxis = graph->GetXaxis();
		TAxis *Yaxis = graph->GetYaxis();
		Xaxis->SetLimits(xAxisMin, xAxisMax);
		Xaxis->SetRangeUser(xAxisMin, xAxisMax);
		Yaxis->SetLimits(yAxisMin, yAxisMax);
		Yaxis->SetRangeUser(yAxisMin, yAxisMax);
		Xaxis->SetTitle("Ionisation number");
		Yaxis->SetTitle("Frequency");
		Yaxis->SetTitleSize(0.062);
		Yaxis->SetTitleOffset(0.45);
		Xaxis->CenterTitle(true); Xaxis->SetTitleOffset(1.4);
		Yaxis->CenterTitle(true); 
		Yaxis->ChangeLabelByValue(1e-5, -1, -1, -1, -1, -1, " "); // Delete label '1e-5'
		if (counter == 0){
			graph->Draw("AL");
			TText* authorsText = new TText(0.938*xData[n-1], yAxisMax*1.1, authors);
			authorsText->SetTextSize(authorsTextSize);
			authorsText->SetTextFont(42);
			authorsText->Draw();
		} else {
			graph->Draw("L SAME");
		};
		
		/*TText* authorsDataText = new TText(60, 8000, dataAuthors);
		authorsDataText->SetTextFont(42);
		authorsDataText->SetTextSize(0.04);
		authorsDataText->Draw();*/
		
			
		/* Top axis of PAD #1 */
		double xMin, xMax, yMin, yMax;
		xMin = Xaxis->GetXmin();
		xMax = Xaxis->GetXmax();
		yMin = Yaxis->GetXmin();
		yMax = Yaxis->GetXmax();
		TString chopt = "S-";
		int numDecadesForTGaxis = 0;
		if (xMin > 0 && xMax > xMin) {
			numDecadesForTGaxis = (int)ceil(log10(xMax) - log10(xMin));
			if (numDecadesForTGaxis < 1) numDecadesForTGaxis = 1;
		} else {
			std::cerr << "Warning: Invalid X axis range for calculating decades for TGaxis. Using default." << std::endl;
			numDecadesForTGaxis = 3;
		}
		TGaxis *topAxis = new TGaxis(xMin, yMax, xMax, yMax,
									 xMin, xMax,
									 510,
									 chopt.Data());
		topAxis->SetTickSize(0.07);
		topAxis->SetLineColor(kBlack);
		topAxis->SetLineWidth(1);
		topAxis->SetTickLength(0.037);
		topAxis->SetLabelOffset(999);
		topAxis->Draw("SAME");
		
		/* Right axis on the Pad #1 */
		numDecadesForTGaxis = 0;
		TString choptRightPad1 = "+GU";
		if (yMin > 0 && yMax > yMin) {
			numDecadesForTGaxis = (int)ceil(log10(yMax) - log10(yMin));
			if (numDecadesForTGaxis < 1) numDecadesForTGaxis = 1;
		} else {
			std::cerr << "Warning: Invalid Y axis range for calculating decades for TGaxis. Using default." << std::endl;
			numDecadesForTGaxis = 3;
		}
		TGaxis *rightAxis = new TGaxis(xMax, yMin, xMax, yMax,
									   yMin, yMax,
									   numDecadesForTGaxis,
									   choptRightPad1.Data());
		//rightAxis->SetTickSize(-0.1);
		//rightAxis->SetLineColor(kBlue);
		//rightAxis->SetLineWidth(1);
		//rightAxis->SetTickLength(-0.037);
		//rightAxis->SetLabelOffset(999);
		rightAxis->Draw();
		pad1->Update();
		pad2->Update();
		mainCanvas->Modified();
		mainCanvas->Update();	
	
		///* Legend: Draw & Initialization */
		TString legendText = parameters.legends[name];
		legend->AddEntry(graph, name.c_str(), "l");
	
		///* Lower pad: Residual pad */
		pad2->cd();
		double res[n];
		double rmse = 0;

		for (size_t i=0; i<n; i++){
			res[i] = (yData[i]-graph->Eval(xData[i]))/yData[i];
			rmse +=  TMath::Power((yData[i]-graph->Eval(xData[i]))/yData[i], 2);
		}
		rmse = TMath::Sqrt(rmse/n);
		
		if (counter == 0){
			hFrame->SetStats(0); // Не показывать статистическое окно
			hFrame->Draw("AXIS"); // Рисуем оси hFrame на активном паде
		}
		
		
		/* Set Limits and Ranges for Pad #2 */
		XaxisRes->SetLimits(xAxisMin, xAxisMax);
		XaxisRes->SetRangeUser(xAxisMin, xAxisMax);
		XaxisRes->SetTitleSize(.09);
		XaxisRes->SetTitle("Ionisation number");
		XaxisRes->CenterTitle(true);
		XaxisRes->SetAxisColor(kBlack);
		XaxisRes->SetTickSize(0.07);
		XaxisRes->SetLabelColor(kBlack);
		XaxisRes->SetLabelSize(0.09);
		XaxisRes->ChangeLabelByValue(12, -1, -1, -1, -1, -1, " "); // Delete label '100'
		YaxisRes->SetLimits(yAxisResMin, yAxisResMax);
		YaxisRes->SetRangeUser(yAxisResMin, yAxisResMax);
		YaxisRes->SetTitleSize(1.2);
		YaxisRes->SetLabelSize(0.08);
		/* attempt to remove bold fraction line */
		//YaxisRes->SetTitleFont(42);   
		YaxisRes->SetTitleSize(.07);
		/* attempt to remove bold fraction line */
		YaxisRes->SetTitle(Form("#eta = [1 - Geant4/Exp]"));
		YaxisRes->CenterTitle(true);
		YaxisRes->SetTitleOffset(0.38);
		YaxisRes->SetLabelColor(kBlack);
		YaxisRes->SetNdivisions(305);
		/* Draw axis between Pad #1 & Pad #2 */
		double xMinRes, xMaxRes, yMinRes, yMaxRes;
		xMinRes = XaxisRes->GetXmin();
		xMaxRes = XaxisRes->GetXmax();
		yMinRes = YaxisRes->GetXmin();
		yMaxRes = YaxisRes->GetXmax();
		if (yMinRes >= yMaxRes) {
			std::cerr << "Error: X-axis range is zero or invalid after drawing main object. Cannot draw top axis." << std::endl;
			std::cerr << "Debug: xMinRes = " << xMinRes << ", xMaxRes = " << xMaxRes << std::endl;
			return;
		}
		chopt = "S-"; 
		numDecadesForTGaxis = 0;
		if (xMinRes > 0 && xMaxRes > xMinRes) {
			numDecadesForTGaxis = (int)ceil(xMaxRes - xMinRes);
			if (numDecadesForTGaxis < 1) numDecadesForTGaxis = 1;
		} else {
			std::cerr << "Warning: Invalid X axis range for calculating decades for TGaxis. Using default." << std::endl;
			numDecadesForTGaxis = 3; // Значение по умолчанию на случай ошибки
		}
		TGaxis *topAxisRes = new TGaxis(xMinRes, yMaxRes, xMaxRes, yMaxRes,
										xMinRes, xMaxRes,
										510,
										chopt.Data());
		topAxisRes->SetTickSize(0.07);
		topAxisRes->SetLineColor(kBlack);
		topAxisRes->SetLineWidth(1);
		topAxisRes->Draw();
		chopt = "+"; 
		numDecadesForTGaxis = 305;
		TGaxis *rightAxisRes = new TGaxis(xData[n-1], yAxisResMin, xData[n-1], yAxisResMax,
										  yAxisResMin, yAxisResMax,
										  YaxisRes->GetNdivisions(),
										  chopt.Data());
		rightAxisRes->SetTickSize(0.03);
		rightAxisRes->SetLineColor(kBlack);
		rightAxisRes->SetLineWidth(1);
		rightAxisRes->SetLabelOffset(999);
		rightAxisRes->Draw();
		pad2->Update();
		pad2->Draw();
		/* Draw: Zero-axis on Pad #2 */
		if (counter == 0){
			TLine *zeroLine = new TLine();
			zeroLine->SetLineColor(kBlack);
			zeroLine->SetLineWidth(1);
			zeroLine->SetLineStyle(2);
			zeroLine->DrawLine(XaxisRes->GetXmin(), 0.0, XaxisRes->GetXmax(), 0.0);
		}
	
		/* Data on Pad #2: Residual data */
		//TGraph* graphRes = new TGraph(n, xData, res);
		//graphRes->SetTitle("");
		TGraph* splineRes = new TGraph(n, xData, res);
		splineRes->SetTitle("");
		//TSpline* splineRes = new TSpline3(name.c_str(), graphRes);
		splineRes->SetLineWidth(lineWidth);
		splineRes->SetLineColor(parameters.colors[name]);
		splineRes->Draw("L SAME");

		/* The legend: Pad #2 */
		legendRes->AddEntry(splineRes, Form("#bf{%s} | #bf{RMSE} = %.4f", name.c_str(), rmse), "l");
		legendRes->Draw();
		
		++counter;
	}
	
	
	/* Plot: Experimental data */
	pad1->cd();
	TGraph* graphExp = new TGraph(n, xData, yData);
	graphExp->SetMarkerStyle(25);
	graphExp->SetMarkerSize(3);
	graphExp->SetLineWidth(4);
	graphExp->SetMarkerColor(kBlack);
	graphExp->Draw("P SAME");
	legend->AddEntry(graphExp,  parameters.legends["Exp"], "p");
	
	/* Update full canvas */
	legend->Draw();
	mainCanvas->Update();

	/* Save the plot */
	std::string folderName = "plots";
	if (!fs::exists(folderName)){
		fs::create_directory(folderName);
	}

	mainCanvas->SaveAs((folderName + "/" + canvasName + ".pdf").c_str());
}


const std::string FormCanvasName(std::string path){
	std::string energyName, ionName;
	size_t nameIndex, energyIndex;
	ionName  = path.substr(10,-1);
	nameIndex = std::find(begin(ionName), end(ionName), '_')-begin(ionName);
	energyName = ionName.substr(nameIndex+1, -1);
	ionName = ionName.substr(0, nameIndex);
	energyIndex = std::find(begin(energyName), end(energyName), '_') -begin(energyName);
	energyName = energyName.substr(0, energyIndex);
	std::replace(begin(energyName), end(energyName), '-', ' ');
	const std::string canvasName = "Ionisation Cluster Size Distribution of " + ionName + " at energy E=" + energyName;
	return canvasName;
};
