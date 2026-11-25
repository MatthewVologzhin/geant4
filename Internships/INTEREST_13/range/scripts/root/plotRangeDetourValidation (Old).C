// scripts/root/plotRangeDetourValidation.C
// Execute under ROOT: root -l -q 'scripts/root/plotRangeDetourValidation.C+'

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <filesystem>
#include <cmath>
#include <map>
#include <algorithm>
#include <iomanip>

// ROOT Headers
#include <TCanvas.h>
#include <TPad.h>
#include <TH1F.h>
#include <TGraphAsymmErrors.h>
#include <TGraphErrors.h>
#include <TGraph.h>
#include <TLine.h>
#include <TLegend.h>
#include <TLatex.h>
#include <TGaxis.h>
#include <TStyle.h>
#include <TMath.h>
#include <TString.h>

namespace fs = std::filesystem;

struct ValidationConfig {
    std::string mode;             // "range" or "detour"
    std::string configName;       // e.g. "Range_alpha"
    std::string particleKey;      // "alpha", "proton", "C12", "e-", "e+"
    std::string particleLabel;    // "alpha", "proton", "{}^{12}C", "e^{-}", "e^{+}"
    int nucleons = 1;             // A (4 for alpha, 12 for C12, 1 for proton/e-/e+)
    double maxEnergyU = 50.0;     // Energy cutoff in MeV/u
    double yAxisMin = 0.0;
    double yAxisMax = 0.0;
    double yAxisResMin = 0.0;
    double yAxisResMax = 0.0;
    std::vector<std::string> physModels;
    std::string dataFile;
    std::string dataAuthors;
};

struct RefPoint {
    double energy; // MeV/u or MeV
    double val;    // Range [g cm^-2] or Detour factor D
    double relErr; // Relative uncertainty
};

// Colors mapping matching the original scripts
std::map<std::string, Int_t> GetColorMap() {
    std::map<std::string, Int_t> colors;
    colors["DNA2"] = kRed;
    colors["DNA4"] = kBlue;
    colors["DNA6"] = kGreen + 2;
    colors["DNA8"] = kMagenta + 2;
    colors["S4"]   = kOrange;
    return colors;
}

// Legend names formatting matching the original scripts
std::map<std::string, TString> GetLegendMap() {
    std::map<std::string, TString> legends;
    legends["DNA2"] = Form("#bf{DNA Opt2}");
    legends["DNA4"] = Form("#bf{DNA Opt4}");
    legends["DNA6"] = Form("#bf{DNA Opt6}");
    legends["DNA8"] = Form("#bf{DNA Opt8}");
    legends["S4"]   = Form("#bf{STD Opt4}");
    return legends;
}

// Helper to parse comma-separated string of physics models
std::vector<std::string> ParseModels(const std::string& str) {
    std::vector<std::string> result;
    std::stringstream ss(str);
    std::string item;
    while (std::getline(ss, item, ',')) {
        size_t first = item.find_first_not_of(" \t\r\n");
        if (first != std::string::npos) {
            size_t last = item.find_last_not_of(" \t\r\n");
            result.push_back(item.substr(first, last - first + 1));
        }
    }
    return result;
}

// Load configurations from main config file
std::vector<ValidationConfig> LoadValidationConfigs(const std::string& filename) {
    std::vector<ValidationConfig> configs;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open config file: " << filename << std::endl;
        return configs;
    }
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        size_t first = line.find_first_not_of(" \t\r\n");
        if (first == std::string::npos || line[first] == '#') continue;

        std::stringstream ss(line);
        ValidationConfig cfg;
        std::string modelsStr;

        if (ss >> cfg.mode
               >> cfg.configName
               >> cfg.particleKey
               >> cfg.particleLabel
               >> cfg.nucleons
               >> cfg.maxEnergyU
               >> cfg.yAxisMin
               >> cfg.yAxisMax
               >> cfg.yAxisResMin
               >> cfg.yAxisResMax
               >> modelsStr
               >> cfg.dataFile
               >> cfg.dataAuthors) {

            // Remove quotes if present in particle label
            if (cfg.particleLabel.front() == '"' && cfg.particleLabel.back() == '"') {
                cfg.particleLabel = cfg.particleLabel.substr(1, cfg.particleLabel.size() - 2);
            }

            cfg.physModels = ParseModels(modelsStr);
            configs.push_back(cfg);
        }
    }
    return configs;
}

// Load reference ICRU90 data points
std::vector<RefPoint> LoadReferenceData(const std::string& filename) {
    std::vector<RefPoint> points;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open reference data file: " << filename << std::endl;
        return points;
    }
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        size_t first = line.find_first_not_of(" \t\r\n");
        if (first == std::string::npos || line[first] == '#') continue;

        std::stringstream ss(line);
        double e = 0.0, v = 0.0, err = 0.0;
        if (ss >> e >> v >> err) {
            points.push_back({e, v, err});
        }
    }
    return points;
}

void ProcessSingleValidation(const ValidationConfig& cfg) {
    auto colors = GetColorMap();
    auto legendNames = GetLegendMap();

    // Picture quality configuration
    gStyle->SetCanvasPreferGL(kTRUE);
    gStyle->SetPadBorderSize(0);

    // Load Reference Data (ICRU90)
    auto refPoints = LoadReferenceData(cfg.dataFile);
    if (refPoints.empty()) {
        std::cerr << "No reference data loaded for " << cfg.configName << std::endl;
        return;
    }

    int n = static_cast<int>(refPoints.size());
    std::vector<double> xData(n), yData(n), relErrData(n);
    for (int i = 0; i < n; ++i) {
        xData[i] = refPoints[i].energy;
        yData[i] = refPoints[i].val;
        relErrData[i] = refPoints[i].relErr;
    }

    /* Global Axis & Legend Parameters */
    double lineWidth = 1.5;
    double xAxisMin = 0.001;
    double xAxisMax = 1e5;
    double nbBins = 1;

    double xMinLeg = 0.75;
    if (cfg.particleKey == "e-" || cfg.particleKey == "e+" || (cfg.mode == "detour" && cfg.particleKey == "C12")) {
        xMinLeg = 0.70;
    }
    double xMaxLeg = 0.90;

    /* Pretty Title Formatting */
    TString prettyTitle;
    if (cfg.mode == "range") {
        if (cfg.particleKey == "C12") {
            prettyTitle = Form("Range of {}^{12}C");
        } else {
            prettyTitle = Form("Range of %s", cfg.particleLabel.c_str());
        }
    } else {
        prettyTitle = Form("Detour factor of %s", cfg.particleLabel.c_str());
    }

    TCanvas *mainCanvas = new TCanvas("mainCanvas", prettyTitle.Data(), 1920, 1080);
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

    TLegend *legend = new TLegend(xMinLeg, 0.0, xMaxLeg, 0.4);
    if (cfg.mode == "detour" && (cfg.particleKey == "C12" || cfg.particleKey == "e-" || cfg.particleKey == "e+")) {
        legend->SetY1NDC(0.6);
        legend->SetY2NDC(0.9);
        legend->SetTextSize(0.045);
    } else {
        legend->SetTextSize(0.035);
        legend->SetTextAlign(22);
        legend->SetMargin(0.5);
        legend->SetEntrySeparation(0.5);
    }
    legend->SetBorderSize(1);

    int counter = 0;
    TGraphAsymmErrors* expBand = nullptr;

    // Physical constants for conversion
    double rho = 0.998;
    double m = 1e9;
    double cm = 1e-2 * m;

    for (const auto& modelName : cfg.physModels) {
        pad1->cd();

        // Fetch simulation data from output/root/<particle>_<model>.txt
        std::string simFilePath = "output/root/" + cfg.particleKey + "_" + modelName + ".txt";
        std::ifstream inputFile(simFilePath);

        if (!inputFile.is_open()) {
            std::cout << "  [Warning] Missing file: " << simFilePath << std::endl;
            continue;
        }

        std::vector<double> vecEnergy, vecSimVal, vecSME;

        if (cfg.mode == "range") {
            double energyVar, rangeVar, rmseVar, emptyVar, nbVar;
            while (inputFile >> energyVar >> rangeVar >> rmseVar 
                             >> emptyVar >> emptyVar >> emptyVar >> emptyVar 
                             >> nbVar) {
                vecEnergy.push_back(energyVar * 1e-6 / cfg.nucleons);
                vecSimVal.push_back(rangeVar * rho / cm);
                vecSME.push_back(rmseVar * rho / cm / TMath::Sqrt(nbVar));
            }
        } else { // detour factor
            double energyVar, rangeVar, rmseRangeVar, penVar, rmsePenVar, emptyVar, nbVar;
            while (inputFile >> energyVar >> rangeVar >> rmseRangeVar 
                             >> emptyVar >> emptyVar >> penVar >> rmsePenVar >> nbVar) {
                vecEnergy.push_back(energyVar * 1e-6 / cfg.nucleons);
                vecSimVal.push_back(penVar / rangeVar);
                vecSME.push_back(TMath::Sqrt(
                    TMath::Power(rmseRangeVar * penVar / (rangeVar * rangeVar), 2) +
                    TMath::Power(rmsePenVar / (rangeVar * rangeVar), 2) / TMath::Sqrt(nbVar)
                ));
            }
        }
        inputFile.close();

        size_t nSim = vecEnergy.size();
        if (nSim == 0) continue;

        TGraphAsymmErrors* graph = new TGraphAsymmErrors(nSim);
        for (size_t i = 0; i < nSim; ++i) {
            double x = vecEnergy[i];
            double y = vecSimVal[i];
            double sme = vecSME[i];
            sme = 0; // Explicitly set 0 error for curve as in original macros
            graph->SetPoint(i, x, y);
            graph->SetPointError(i, 0, 0, sme, sme);
        }

        graph->SetLineWidth(lineWidth);
        graph->SetLineColor(colors[modelName]);

        if (counter == 0) {
            pad1->cd();
            TH1F* hFrame1 = new TH1F("hFrame1", prettyTitle.Data(), nbBins, xAxisMin, xAxisMax);
            hFrame1->SetStats(0);
            TAxis* xAxis = hFrame1->GetXaxis();
            TAxis* yAxis = hFrame1->GetYaxis();
            hFrame1->Draw("AXIS");

            xAxis->SetLimits(xData[0], xData[n - 1]);
            xAxis->SetRangeUser(xData[0], xData[n - 1]);
            xAxis->SetLabelSize(0);
            xAxis->SetTitleSize(0);
            xAxis->SetTickLength(0.03);

            yAxis->SetRangeUser(cfg.yAxisMin, cfg.yAxisMax);

            if (cfg.mode == "range") {
                yAxis->SetTitle("Range  #it{#rhor}, [g cm^{-2}] ");
            } else {
                yAxis->SetTitle("Detour factor   #it{D} =  #frac{#scale[0.8]{Range}}{#scale[0.8]{Penetration}}");
            }
            yAxis->SetTitleSize(0.065);
            yAxis->SetTitleOffset(0.55);
            yAxis->CenterTitle(true);
            pad1->Update();
            pad1->RedrawAxis();

            // Reference ICRU90 Band
            expBand = new TGraphAsymmErrors(n);
            for (int i = 0; i < n; i++) {
                double x = xData[i];
                double y = yData[i];
                double relErr = relErrData[i];
                expBand->SetPoint(i, x, y);
                expBand->SetPointError(i, 0, 0, y * relErr, y * relErr);
            }
            expBand->SetFillColorAlpha(kGray, 0.5);
            expBand->SetFillStyle(1001);
            expBand->SetLineColor(kBlack);
            expBand->SetLineStyle(2);
            expBand->Draw("E3 SAME");
            expBand->Draw("L X SAME");

            TLatex *texTitle = new TLatex(0.5, 0.955, prettyTitle.Data());
            texTitle->SetNDC();
            texTitle->SetTextAlign(22);
            texTitle->SetTextFont(42);
            texTitle->SetTextSize(0.060);
            texTitle->Draw();
        }

        graph->Draw("L SAME");

        /* Additional axes for Pad 1 */
        TGaxis *topAxis = new TGaxis(xData[0], cfg.yAxisMax, xData[n - 1], cfg.yAxisMax, xData[0], xData[n - 1], 510, "GS-");
        topAxis->SetTickSize(0.07);
        topAxis->SetTickLength(0.037);
        topAxis->SetLabelOffset(999);
        topAxis->Draw("SAME");

        TGaxis *rightAxis = new TGaxis(xData[n - 1], cfg.yAxisMin, xData[n - 1], cfg.yAxisMax, cfg.yAxisMin, cfg.yAxisMax, 510, "+GU");
        rightAxis->Draw();

        /* Residuals & Chi2 Calculation */
        pad2->cd();
        std::vector<double> res(n, 0.0);
        double chi2Sum = 0.0;
        int validPoints = 0;

        for (int i = 0; i < n; i++) {
            double simVal = graph->Eval(xData[i]);
            double energy = xData[i];
            double errSim = 0.0;
            double relErrExp = relErrData[i];
            double errExp = relErrExp * yData[i];

            res[i] = (yData[i] - simVal) / yData[i];

            double totalVariance = TMath::Power(errSim, 2) + TMath::Power(errExp, 2);
            if (totalVariance > 0) {
                chi2Sum += TMath::Power(yData[i] - simVal, 2) / totalVariance;
                validPoints++;
            }
        }

        double rmse = 0.0;
        for (int i = 0; i < n; i++) rmse += res[i] * res[i];
        rmse = TMath::Sqrt(rmse / n);

        double normChi2 = (validPoints > 1) ? chi2Sum / (validPoints - 1) : 0;
        std::cout << "Model: " << std::left << std::setw(6) << modelName 
                  << " | Particle: " << std::setw(6) << cfg.particleKey 
                  << " | Mode: " << std::setw(6) << cfg.mode
                  << " | Reduced Chi2 = " << std::setw(8) << normChi2 
                  << " | RMSE = " << rmse << std::endl;

        legend->AddEntry(graph, Form("#scale[0.9]{%s}", legendNames[modelName].Data()), "l");

        if (counter == 0) {
            hFrame->SetStats(0);
            hFrame->Draw("AXIS");

            XaxisRes->SetLimits(xData[0], xData[n - 1]);
            XaxisRes->SetRangeUser(xData[0], xData[n - 1]);
            if (cfg.nucleons > 1) {
                XaxisRes->SetTitle("Energy, [MeV/u]");
            } else {
                XaxisRes->SetTitle("Energy, [MeV]");
            }
            XaxisRes->SetTitleSize(0.155);
            XaxisRes->SetLabelSize(0.09);
            XaxisRes->CenterTitle(true);
            XaxisRes->SetTickSize(0.07);
            XaxisRes->SetTitleOffset(0.88);

            YaxisRes->SetRangeUser(cfg.yAxisResMin, cfg.yAxisResMax);
            YaxisRes->SetTitle("#it{#eta} = [1 - Geant4/Ref]");
            YaxisRes->SetTitleSize(0.082);
            YaxisRes->SetTitleOffset(0.30);
            YaxisRes->SetLabelSize(0.07);
            YaxisRes->CenterTitle(true);
            YaxisRes->SetNdivisions(305);

            // Grey band in residual plot
            TGraphErrors* icruBand = new TGraphErrors(n);
            for (int i = 0; i < n; ++i) {
                icruBand->SetPoint(i, xData[i], 0.0);
                icruBand->SetPointError(i, 0, relErrData[i]);
            }
            icruBand->SetFillColorAlpha(kGray, 0.5);
            icruBand->SetFillStyle(1001);
            icruBand->SetLineColor(kGray);
            icruBand->SetLineWidth(0);
            icruBand->Draw("E3 SAME");

            TLine *zeroLine = new TLine(xData[0], 0.0, xData[n - 1], 0.0);
            zeroLine->SetLineStyle(2);
            zeroLine->Draw("SAME");

            // Upper axis for Pad 2
            TString chopt = "SN-G";
            int numDecadesForTGaxis = 0;
            if (xAxisMin > 0 && xAxisMax > xAxisMin) {
                numDecadesForTGaxis = (int)ceil(log10(xAxisMax) - log10(xAxisMin));
                if (numDecadesForTGaxis < 1) numDecadesForTGaxis = 1;
            } else {
                numDecadesForTGaxis = 3;
            }
            TGaxis *topAxisRes = new TGaxis(xData[0], cfg.yAxisResMax, xData[n - 1], cfg.yAxisResMax,
                                            xData[0], xData[n - 1],
                                            numDecadesForTGaxis,
                                            chopt.Data());
            topAxisRes->SetTickSize(0.07);
            topAxisRes->SetLineColor(kBlack);
            topAxisRes->SetLineWidth(1);
            topAxisRes->Draw("SAME A");

            chopt = "+";
            TGaxis *rightAxisRes = new TGaxis(xData[n - 1], cfg.yAxisResMin, xData[n - 1], cfg.yAxisResMax,
                                              cfg.yAxisResMin, cfg.yAxisResMax,
                                              YaxisRes->GetNdivisions(),
                                              chopt.Data());
            rightAxisRes->SetTickSize(0.03);
            rightAxisRes->SetLineColor(kBlack);
            rightAxisRes->SetLineWidth(1);
            rightAxisRes->SetLabelOffset(999);
            rightAxisRes->Draw("SAME");
            pad2->RedrawAxis();
        }

        TGraph* splineRes = new TGraph(n, xData.data(), res.data());
        splineRes->SetLineWidth(lineWidth);
        splineRes->SetLineColor(colors[modelName]);
        splineRes->Draw("L SAME");

        counter++;
    }

    /* Experimental Data Overlay */
    pad1->cd();
    if (expBand) {
        legend->AddEntry(expBand, Form("#scale[0.9]{#bf{%s}}", cfg.dataAuthors.c_str()), "lf");
    }
    pad1->RedrawAxis();
    legend->Draw();
    mainCanvas->Update();

    std::string folderName = "output/plots";
    if (!fs::exists(folderName)) fs::create_directories(folderName);
    std::string outputPath = folderName + "/" + cfg.configName + ".pdf";
    mainCanvas->SaveAs(outputPath.c_str());

    delete mainCanvas;
    std::cout << "  [Saved] " << outputPath << std::endl;
}

void plotRangeDetourValidation() {
    std::string configFile = "input/experimental_data/range_detour_configs.txt";
    auto configs = LoadValidationConfigs(configFile);

    if (configs.empty()) {
        std::cerr << "No validation configs loaded from " << configFile << std::endl;
        return;
    }

    std::cout << "\n=======================================================\n";
    std::cout << "  Starting Range & Detour Factor Validation Pipeline   \n";
    std::cout << "=======================================================\n";

    for (const auto& cfg : configs) {
        std::cout << "\nProcessing: " << cfg.configName << " (" << cfg.particleKey << ")\n";
        ProcessSingleValidation(cfg);
    }

    std::cout << "\n=======================================================\n";
    std::cout << "  Validation Complete. PDF plots saved to 'output/plots/'\n";
    std::cout << "=======================================================\n";
}