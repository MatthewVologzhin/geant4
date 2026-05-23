#include <filesystem>
#include <algorithm>
#include <cmath>
#include <TFile.h>
#include <random>
#include <map>

namespace fs = std::filesystem;

double computeIntegral(TGraph*);

void protonRadialDose(){
	/* Auxilary variables */
	double nm, um, mm, m, MeV;
	m =  1e9; 		//you can change this value
	mm = 1e-3*m;	// ... but not this!
	um = 1e-6*m;	// ... but not this!
	nm = 1e-9*m;	// ... but not this!
	MeV = 1e6*1.602e-19;

	/* Picture quality configuration */
	gStyle->SetCanvasPreferGL(kTRUE);
	gStyle->SetPadBorderSize(0);

	/* Experimental data: Alpha-particles */
	double LETExp = 27;
	const int n = 20;
	double xData[n] = {0.888529886914378, 1.8578352180936977, 2.1001615508885276, 
					     3.715670436187398, 4.765751211631664, 5.1696284329563795, 
					     7.67366720516963, 6.865912762520194, 9.289176090468498, 
					     11.87399030694669, 10.50080775444265, 13.974151857835217, 
					     15.912762520193862, 18.497576736672055, 23.263327948303722, 
					     27.382875605815833, 32.7140549273021, 40.710823909531506, 
					     46.849757673667206, 49.11147011308562};
    double yData[n] = {149527.54039751206, 38339.206726007076, 15210.94282220963,
						 6687.731218888133, 2722.351744008087, 1949.677704183298, 
						 1759.3537047359928, 1052.7005084892514, 753.9167982614002, 
						 462.8364025196084, 340.093642057334, 203.49333335529653, 
						 118.67232263265367, 89.46905752145418, 59.32451559539817, 
						 38.33920672600708, 25.421692486106757, 10.348320919317063, 
						 8.21293037776651, 7.223306769529858};
	
	/* Data initialization */
	std::string pathDNA2, pathDNA4, pathDNA6, pathDNA8, pathS4;
	pathDNA2 = "root/simplified-ionrange-proton-1_MeV-DNA2.root";	
	pathDNA4 = "root/simplified-ionrange-proton-1_MeV-DNA4.root";
	pathDNA6 = "root/simplified-ionrange-proton-1_MeV-DNA6.root";
	pathDNA8 = "root/simplified-ionrange-proton-1_MeV-DNA8.root";
	pathS4 = "root/simplified-ionrange-proton-1_MeV-S4.root";

	TFile *fDNA2 =  new TFile(pathDNA2.c_str());
	TFile *fDNA4 =  new TFile(pathDNA4.c_str());
	TFile *fDNA6 =  new TFile(pathDNA6.c_str());
	TFile *fDNA8 =  new TFile(pathDNA8.c_str());
	TFile *fS4 	 =  new TFile(pathS4.c_str());
	
	TTree *treeDNA2 = (TTree*)fDNA2->Get("radialDoseData");
	TTree *treeDNA4 = (TTree*)fDNA4->Get("radialDoseData");
	TTree *treeDNA6 = (TTree*)fDNA6->Get("radialDoseData");
	TTree *treeDNA8 = (TTree*)fDNA8->Get("radialDoseData");
	TTree *treeS4 	= (TTree*)fS4->Get("radialDoseData");
	TTree *treeLETDNA2 = (TTree*)fDNA2->Get("LETdata");
	TTree *treeLETDNA4 = (TTree*)fDNA4->Get("LETdata");
	TTree *treeLETDNA6 = (TTree*)fDNA6->Get("LETdata");
	TTree *treeLETDNA8 = (TTree*)fDNA8->Get("LETdata");
	TTree *treeLETS4   = (TTree*)fS4->Get("LETdata");

	/* Colors */
	std::map<std::string, Int_t> colors;
	colors["DNA2"] = kAzure-14;
	colors["DNA4"] = kAzure-7;
	colors["DNA6"] = kAzure;
	colors["DNA8"] = kAzure+7;
	colors["S4"] = 	 kRed;
	
	/* Other parameters */
	double lineWidth = 1.5;
	double yAxisMin, yAxisMax, xAxisMin, xAxisMax, xMinLegend;
	double authorsTextSize = 0.0295;
	double yAxisResMin, yAxisResMax;
	yAxisMin = 0.5*yData[n-1];
	yAxisMax = 2*yData[0];
	xAxisMin = xData[0];
	xAxisMax = 4*xData[n-1];
	xMinLegend = 0.1+0.8*(log10(xData[n-1])-log10(xAxisMin))/(log10(xAxisMax)-log10(xAxisMin));
	yAxisResMin = -3;
	yAxisResMax = 2.2;
	TString authors = "Vologzhin & Batmunkh & Ivanchenko (2025)";
	TString dataAuthors = "Wingate & Baum (1976)";
	
	/* Canvas name from file name */
	std::string energyName, ionName;
	size_t nameIndex, energyIndex;
	ionName  = pathDNA2.substr(25,-1);
	nameIndex = std::find(begin(ionName), end(ionName), '-')-begin(ionName);
	energyName = ionName.substr(nameIndex+1, -1);
	ionName = ionName.substr(0, nameIndex);
	energyIndex = std::find(begin(energyName), end(energyName), '-') -begin(energyName);
	energyName = energyName.substr(0, energyIndex);
	std::replace(begin(energyName), end(energyName), '_', ' ');
	const std::string canvasName = "Radial dose distribution of " + ionName + " at E=" + energyName;
	TCanvas *mainCanvas = new TCanvas("mainCanvas", canvasName.c_str(),
									  1920, 1080);
	
	/* Up and down pad initialization */
    TPad *pad1 = new TPad("pad1", "main plot", 0.0, 0.3, 1.0, 1.0); // Верхний Pad для основного графика
    TPad *pad2 = new TPad("pad2", "residuals", 0.0, 0.0, 1.0, 0.3); // Нижний Pad для графика остатков
    pad1->SetLogx();
    pad1->SetLogy(); 
    pad1->SetBottomMargin(0);
    pad1->Draw();
    pad2->SetTopMargin(0);    
    pad2->SetBottomMargin(0.3); //pad2->SetFillColor(kBlack); //pad2->SetFrameFillColor(kBlack);
    pad2->Draw();
	
	/* Upper pad */
	pad1->cd();
	double axialRadiusDNA2, doseDNA2; //,LETDNA2;
	double axialRadiusDNA4, doseDNA4; //,LETDNA4;
	double axialRadiusDNA6, doseDNA6; //,LETDNA6;
	double axialRadiusDNA8, doseDNA8; //,LETDNA8;
	double axialRadiusS4, doseS4; 	  //,LETS4;
	treeDNA2->SetBranchAddress("radius", &axialRadiusDNA2);
	treeDNA2->SetBranchAddress("radialDose", &doseDNA2);
	treeDNA4->SetBranchAddress("radius", &axialRadiusDNA4);
	treeDNA4->SetBranchAddress("radialDose", &doseDNA4);
	treeDNA6->SetBranchAddress("radius", &axialRadiusDNA6);
	treeDNA6->SetBranchAddress("radialDose", &doseDNA6);
	treeDNA8->SetBranchAddress("radius", &axialRadiusDNA8);
	treeDNA8->SetBranchAddress("radialDose", &doseDNA8);
	treeS4->SetBranchAddress("radius", &axialRadiusS4);
	treeS4->SetBranchAddress("radialDose", &doseS4);
	/*treeLETDNA2->SetBranchAddress("LET", &LETDNA2);
	treeLETDNA4->SetBranchAddress("LET", &LETDNA4);
	treeLETDNA6->SetBranchAddress("LET", &LETDNA6);
	treeLETDNA8->SetBranchAddress("LET", &LETDNA8);
	treeLETS4->SetBranchAddress("LET", &LETS4);*/
	
	/* DNA Option 2 initialization */
	int nPointsDNA2 = treeDNA2->GetEntries(); 
	TGraph* graphDNA2 = new TGraph();
	for (Long64_t i=1; i < nPointsDNA2; ++i){
		treeDNA2->GetEntry(i);
		if (axialRadiusDNA2 >= 0 and doseDNA2 >= 0){
			graphDNA2->SetPoint(graphDNA2->GetN(), axialRadiusDNA2, doseDNA2);
		}
	}
	
	graphDNA2->SetLineWidth(lineWidth);
	graphDNA2->SetLineColor(colors["DNA2"]);
	graphDNA2->SetTitle(canvasName.c_str());
	TAxis *Xaxis = graphDNA2->GetXaxis();
    TAxis *Yaxis = graphDNA2->GetYaxis();
	Xaxis->SetLimits(xAxisMin, xAxisMax);
	Xaxis->SetRangeUser(xAxisMin, xAxisMax);
	Yaxis->SetLimits(yAxisMin, yAxisMax);
	Yaxis->SetRangeUser(yAxisMin, yAxisMax);
	Xaxis->SetTitle("Distance from the axis [nm]");
	Yaxis->SetTitle("Dose [Gy]");
	Yaxis->SetTitleSize(0.062);
	Yaxis->SetTitleOffset(0.45);
	Xaxis->CenterTitle(true); Xaxis->SetTitleOffset(1.4);
	Yaxis->CenterTitle(true); 
	graphDNA2->Draw("AL");
	
	/*TText* authorsDataText = new TText(60, 8000, dataAuthors);
	authorsDataText->SetTextFont(42);
	authorsDataText->SetTextSize(0.04);
	authorsDataText->Draw();*/
	TText* authorsText = new TText(xData[n-1], yAxisMax*1.1, authors);
	authorsText->SetTextSize(authorsTextSize);
	authorsText->SetTextFont(42);
	authorsText->Draw();
	
	/* Top axis of PAD #1 */
	double xMin, xMax, yMin, yMax;
	xMin = Xaxis->GetXmin();
	xMax = Xaxis->GetXmax();
	yMin = Yaxis->GetXmin();
	yMax = Yaxis->GetXmax();
	TString chopt = "SN-G";
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
								 numDecadesForTGaxis,
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
	pad2->Update();
	mainCanvas->Modified();
	mainCanvas->Update();
	
	/* Plot: DNA Option 4 */
	int nPointsDNA4 = treeDNA4->GetEntries(); 
	TGraph* graphDNA4 = new TGraph();
	for (Long64_t i=0; i < nPointsDNA4; ++i){
		treeDNA4->GetEntry(i);
		if (axialRadiusDNA4 >= 0 and doseDNA4 >= 0){
			graphDNA4->SetPoint(graphDNA4->GetN(), axialRadiusDNA4, doseDNA4);
		}
	}
	graphDNA4->SetLineWidth(lineWidth);
	graphDNA4->SetLineColor(colors["DNA4"]);
	graphDNA4->Draw("L SAME");
	
	/* Plot: DNA Option 6 */
	int nPointsDNA6 = treeDNA6->GetEntries(); 
	TGraph* graphDNA6 = new TGraph();
	for (Long64_t i=0; i < nPointsDNA6; ++i){
		treeDNA6->GetEntry(i);
		if (axialRadiusDNA6 >= 0 and doseDNA6 >= 0){
			graphDNA6->SetPoint(graphDNA6->GetN(), axialRadiusDNA6, doseDNA6);
		}
	}
	graphDNA6->SetLineColor(colors["DNA6"]);
	graphDNA6->SetLineWidth(lineWidth);
	graphDNA6->Draw("L SAME");
	
	/* Plot: DNA Option 8 */
	int nPointsDNA8 = treeDNA8->GetEntries(); 
	TGraph* graphDNA8 = new TGraph();
	for (Long64_t i=0; i < nPointsDNA8; ++i){
		treeDNA8->GetEntry(i);
		if (axialRadiusDNA8 >= 0 and doseDNA8 >= 0){
			graphDNA8->SetPoint(graphDNA8->GetN(), axialRadiusDNA8, doseDNA8);
		}
	}
	graphDNA8->SetLineColor(colors["DNA8"]);
	graphDNA8->SetLineWidth(lineWidth);
	graphDNA8->Draw("L SAME");
	
	/* Plot: EM Standard Option 4 */
	int nPointsS4 = treeS4->GetEntries(); 
	TGraph* graphS4 = new TGraph();
	for (Long64_t i=0; i < nPointsS4; ++i){
		treeS4->GetEntry(i);
		if (axialRadiusS4 >= 0 and doseS4 >= 0){
			graphS4->SetPoint(graphS4->GetN(), axialRadiusS4, doseS4);
		}
	}
	graphS4->SetLineColor(colors["S4"]);
	graphS4->SetLineWidth(lineWidth);
	graphS4->Draw("L SAME");
	
	/* Plot: Experimental data */
	TGraph* gpExp = new TGraph(n, xData, yData);
	gpExp->SetMarkerStyle(25);
	gpExp->SetMarkerSize(3);
	gpExp->SetLineWidth(4);
	gpExp->SetMarkerColor(kBlack);
	gpExp->Draw("P SAME");
	//gPad->SetFrameBorderMode(0);
	
	
	/* LET: Fetching from TTree */
	double LETDNA2, LETDNA4, LETDNA6, LETDNA8, LETS4;
	double LETDNA2error, LETDNA4error, LETDNA6error, LETDNA8error, LETS4error;
	std::vector<double*> vecLET = {&LETDNA2, &LETDNA4, &LETDNA6, &LETDNA8, &LETS4};
	std::vector<double*> vecLETerror = {&LETDNA2error, &LETDNA4error, &LETDNA6error, &LETDNA8error, &LETS4error};
	std::vector<TTree*> vecTreeLET = {treeLETDNA2, treeLETDNA4, treeLETDNA6, treeLETDNA8, treeLETS4};
	for (size_t i=0; i<vecLET.size(); ++i){
		vecTreeLET[i]->Draw("LET", "", "goff");
		TH1F* hist = (TH1F*)gROOT->FindObject("htemp");
		*(vecLET[i]) = (double)(hist->GetMean());
		double t = TMath::StudentQuantile(0.975, n);
		*(vecLETerror[i]) = t*(double)(hist->GetStdDev())/TMath::Sqrt(n);
	}
	
	/* LET: Old method | Calculation by trapezoid method */
	/*double LET1 = computeIntegral(graphDNA2);
	double LET2 = computeIntegral(graphDNA4);
	double LET3 = computeIntegral(graphDNA6);
	double LET4 = computeIntegral(graphDNA8);
	double LET5 = computeIntegral(graphS4);*/
	
	/* Legend: Draw & Initialization */
	TLegend *legend = new TLegend(xMinLegend, 0.6, 0.9, 0.9);
	TString strDNA2 = Form("#bf{DNA Opt2} | #bf{LET} = %.2f #pm %.2f keV/um", LETDNA2, LETDNA2error);
	TString strDNA4 = Form("#bf{DNA Opt4} | #bf{LET} = %.2f #pm %.2f keV/um", LETDNA4, LETDNA4error);
	TString strDNA6 = Form("#bf{DNA Opt6} | #bf{LET} = %.2f #pm %.2f keV/um", LETDNA6, LETDNA6error);
	TString strDNA8 = Form("#bf{DNA Opt8} | #bf{LET} = %.2f #pm %.2f keV/um", LETDNA8, LETDNA8error);
	TString strS4 	= Form("#bf{EM Standard Opt4} | #bf{LET} = %.2f #pm %.2f keV/um", LETS4, LETS4error);
	TString strExp 	= Form("#bf{%s} | #bf{LET} = %.0f keV/um", dataAuthors.Data(), LETExp);
	legend->AddEntry(graphDNA2, strDNA2, "l");
	legend->AddEntry(graphDNA4, strDNA4, "l");
	legend->AddEntry(graphDNA6, strDNA6, "l");
	legend->AddEntry(graphDNA8, strDNA8, "l");
	legend->AddEntry(graphS4, strS4, "l");
	legend->AddEntry(gpExp,  strExp, "p");
	legend->Draw();
	
	/* Lower pad: Residual pad */
	pad2->cd();
	pad2->SetLogx();
	double resDNA2[n];
	double resDNA4[n];
	double resDNA6[n];
	double resDNA8[n];
	double resS4[n];
	double rmseDNA2 = 0;
	double rmseDNA4 = 0;
	double rmseDNA6 = 0;
	double rmseDNA8 = 0;
	double rmseS4 = 0;

	for (size_t i=0; i<n; i++){
		resDNA2[i] = (yData[i]-graphDNA2->Eval(xData[i]))/yData[i];
		resDNA4[i] = (yData[i]-graphDNA4->Eval(xData[i]))/yData[i];
		resDNA6[i] = (yData[i]-graphDNA6->Eval(xData[i]))/yData[i];
		resDNA8[i] = (yData[i]-graphDNA8->Eval(xData[i]))/yData[i];
		resS4[i] =   (yData[i]-graphS4->Eval(xData[i]))/yData[i];
	
		rmseDNA2 +=  TMath::Power((yData[i]-graphDNA2->Eval(xData[i]))/yData[i], 2);
		rmseDNA4 +=  TMath::Power(abs(yData[i]-graphDNA4->Eval(xData[i]))/yData[i], 2);
		rmseDNA6 +=  TMath::Power(abs(yData[i]-graphDNA6->Eval(xData[i]))/yData[i], 2);
		rmseDNA8 +=  TMath::Power(abs(yData[i]-graphDNA8->Eval(xData[i]))/yData[i], 2);
		rmseS4	 +=  TMath::Power(abs(yData[i]-graphS4->Eval(xData[i]))/yData[i], 2);
	}
	rmseDNA2 = TMath::Sqrt(rmseDNA2/n);
	rmseDNA4 = TMath::Sqrt(rmseDNA4/n);
	rmseDNA6 = TMath::Sqrt(rmseDNA6/n);
	rmseDNA8 = TMath::Sqrt(rmseDNA8/n);
	rmseS4 =   TMath::Sqrt(rmseS4/n);
	

	/* Draw frame for Pad #2*/
	TH1F* hFrame = new TH1F("hFrame", "", 100, 0.8, 120);
	hFrame->SetStats(0); // Не показывать статистическое окно
	hFrame->Draw("AXIS"); // Рисуем оси hFrame на активном паде
	TAxis *XaxisRes = hFrame->GetXaxis();
	TAxis *YaxisRes = hFrame->GetYaxis();
	
	/* Set Limits and Ranges for Pad #2 */
	XaxisRes->SetLimits(xAxisMin, xAxisMax);
	XaxisRes->SetRangeUser(xAxisMin, xAxisMax);
	XaxisRes->SetTitleSize(.09);
	XaxisRes->SetTitle("Distance from the axis [nm]");
	XaxisRes->CenterTitle(true);
	XaxisRes->SetAxisColor(kBlack);
	XaxisRes->SetTickSize(0.07);
	XaxisRes->SetLabelColor(kBlack);
	XaxisRes->SetLabelSize(0.09);
	XaxisRes->ChangeLabelByValue(100, -1, -1, -1, -1, -1, " "); // Delete label '100'
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
	YaxisRes->SetTitleOffset(0.3);
	YaxisRes->SetLabelColor(kBlack);
	YaxisRes->SetNdivisions(305);
	pad2->Update();

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
	chopt = "SN-G"; 
	numDecadesForTGaxis = 0;
	if (xMinRes > 0 && xMaxRes > xMinRes) {
		numDecadesForTGaxis = (int)ceil(log10(xMaxRes) - log10(xMinRes));
		if (numDecadesForTGaxis < 1) numDecadesForTGaxis = 1;
	} else {
		std::cerr << "Warning: Invalid X axis range for calculating decades for TGaxis. Using default." << std::endl;
		numDecadesForTGaxis = 3; // Значение по умолчанию на случай ошибки
	}
	TGaxis *topAxisRes = new TGaxis(xMinRes, yMaxRes, xMaxRes, yMaxRes,
									xMinRes, xMaxRes,
									numDecadesForTGaxis,
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

	/* Draw: Zero-axis on Pad #2 */
	TLine *zeroLine = new TLine();
	zeroLine->SetLineColor(kBlack);
	zeroLine->SetLineWidth(1);
	zeroLine->SetLineStyle(2);
	zeroLine->DrawLine(XaxisRes->GetXmin(), 0.0, XaxisRes->GetXmax(), 0.0);
	
	/* Data on Pad #2: Residual data */
	TGraph* resGraphDNA2 = new TGraph(n, xData, resDNA2);
	TGraph* resGraphDNA4 = new TGraph(n, xData, resDNA4);
	TGraph* resGraphDNA6 = new TGraph(n, xData, resDNA6);
	TGraph* resGraphDNA8 = new TGraph(n, xData, resDNA8);
	TGraph* resGraphS4 = new TGraph(n, xData, resS4);
	resGraphDNA2->SetTitle("");
	TSpline* splineDNA2 = new TSpline3("resDNA2", resGraphDNA2);
	splineDNA2->SetLineWidth(lineWidth);
	splineDNA2->SetLineColor(colors["DNA2"]);
	TSpline* splineDNA4 = new TSpline3("resDNA4", resGraphDNA4);
	splineDNA4->SetLineWidth(lineWidth);
	splineDNA4->SetLineColor(colors["DNA4"]);
	TSpline* splineDNA6 = new TSpline3("resDNA6", resGraphDNA6);
	splineDNA6->SetLineWidth(lineWidth);
	splineDNA6->SetLineColor(colors["DNA6"]);
	TSpline* splineDNA8 = new TSpline3("resDNA8", resGraphDNA8);
	splineDNA8->SetLineWidth(lineWidth);
	splineDNA8->SetLineColor(colors["DNA8"]);
	TSpline* splineS4 = new TSpline3("resS4", resGraphS4);
	splineS4->SetLineWidth(lineWidth);
	splineS4->SetLineColor(colors["S4"]);
	splineDNA2->Draw("L SAME");
	splineDNA4->Draw("L SAME");
	splineDNA6->Draw("L SAME");
	splineDNA8->Draw("L SAME");
	splineS4->Draw("L SAME");

	/* The legend: Pad #2 */
	TLegend *legendRes = new TLegend(xMinLegend, 0.3, 0.9, 1);
	legendRes->SetTextSize(0.055);
	legendRes->AddEntry(splineDNA2, Form("#bf{DNA Opt2} | #bf{RMSE} = %.4f", rmseDNA2), "l");
	legendRes->AddEntry(splineDNA4, Form("#bf{DNA Opt4} | #bf{RMSE} = %.4f", rmseDNA4), "l");
	legendRes->AddEntry(splineDNA6, Form("#bf{DNA Opt6} | #bf{RMSE} = %.4f", rmseDNA6), "l");
	legendRes->AddEntry(splineDNA8, Form("#bf{DNA Opt8} | #bf{RMSE} = %.4f", rmseDNA8), "l");
	legendRes->AddEntry(splineS4, 	Form("#bf{EM Standard Opt4} | #bf{RMSE} = %.4f", rmseS4), "l");
	legendRes->Draw();
	
	/* Update full canvas */
	mainCanvas->Update();

	/* Save the plot */
	std::string folderName = "plots";
	if (!fs::exists(folderName)){
		fs::create_directory(folderName);
	}
	std::replace(begin(energyName), end(energyName), ' ', '_');
	const std::string fileName = ionName + "-" + energyName + "-" + "radialDose";
	mainCanvas->SaveAs(("plots/" + fileName + ".pdf").c_str());
};

double computeIntegral(TGraph* graph){
	/* Trapezoidal method */
	const int nPoints = graph->GetN();
	auto x = graph->GetX();
	auto y = graph->GetY();
	
	double integral = .0;
	double u1, u2;
	for (size_t i = 0; i < nPoints; i++){
		if (x[i] > 0 and y[i] > 0){
			u1 = 2*M_PI*x[i]*y[i];
			u2 = 2*M_PI*x[i+1]*y[i+1];
			u1 = 1/1.602*1e19/1e24*u1; 
			u2 = 1/1.602*1e19/1e24*u2;
			integral += (x[i+1]-x[i])*(u1 + u2)/2;
		}
	}
	
	return integral;
}
