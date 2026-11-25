// scripts/root/plotRDDValidation.C
// Execute under ROOT: root -l -q 'scripts/root/plotRDDValidation.C+'

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
#include <TFile.h>
#include <TTree.h>
#include <TChain.h>
#include <TH1F.h>
#include <TGraphAsymmErrors.h>
#include <TGraphErrors.h>
#include <TGraph.h>
#include <TLine.h>
#include <TLegend.h>
#include <TLatex.h>
#include <TText.h>
#include <TGaxis.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TSystemDirectory.h>
#include <TSystemFile.h>
#include <TMath.h>
#include <TString.h>

namespace fs = std::filesystem;

struct AxisBound {
    bool isExp = true;
    double val = 0.0;
};

struct RDDConfig {
    std::string section;
    std::string name;
    std::string particleLabel;
    int Z = 0;
    int A = 0;
    double energy = 0.0;
    std::string energyUnit = "MeV";
    AxisBound xAxisMin;
    AxisBound xAxisMax;
    AxisBound yAxisMin;
    AxisBound yAxisMax;
    double yAxisResMin = -1.5;
    double yAxisResMax = 1.5;
    double xTickLength = 0.015;
    double yTickLength = 0.015;
    double yResTickLength = 0.0193;
    bool isLogX = false;
    std::string expRelErrType = "threshold";
    std::string expDataFile = "";
    std::string dataAuthors = "Experimental Data";
    std::string dataType = "Experiment";
    double legX1 = 0.68;
    double legY1 = 0.50;
    double legX2 = 0.88;
    double legY2 = 0.88;
};

struct ExpPoint {
    double x;
    double y;
};

struct PhysOpt {
    std::string name;
    std::string fileSuffix;
    std::string legend;
    Int_t color;
};

struct ValidationResult {
    std::string particle;
    double energy;
    std::string energyUnit;
    std::string model;
    double reducedChi2;
    double rmse;
    Int_t nEvents;
};

// Функция удаления лишних пробелов и табов по краям
std::string Trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

// Парсинг значения границы (число или "exp" / "auto")
AxisBound ParseBound(const std::string& token) {
    AxisBound b;
    std::string s = Trim(token);
    if (s == "exp" || s == "EXP" || s == "auto" || s.empty()) {
        b.isExp = true;
        b.val = 0.0;
    } else {
        try {
            b.val = std::stod(s);
            b.isExp = false;
        } catch (...) {
            b.isExp = true;
            b.val = 0.0;
        }
    }
    return b;
}

// Форматирование энергии (1.0 -> "1", 24.022 -> "24p022")
std::string FormatEnergy(double energy) {
    if (std::abs(energy - std::round(energy)) < 1e-4) {
        return std::to_string(static_cast<long long>(std::round(energy)));
    } else {
        std::ostringstream ss;
        ss << energy;
        std::string str = ss.str();
        size_t pos = str.find('.');
        if (pos != std::string::npos) {
            str.replace(pos, 1, "p");
        }
        return str;
    }
}

// Загрузка конфигурационного файла RDD в формате INI/CFG
std::vector<RDDConfig> LoadRDDConfigsFromCFG(const std::string& filename) {
    std::vector<RDDConfig> configs;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open configuration file: " << filename << std::endl;
        return configs;
    }

    std::string line;
    RDDConfig currentCfg;
    bool hasSection = false;

    auto pushCurrent = [&]() {
        if (hasSection) {
            configs.push_back(currentCfg);
            currentCfg = RDDConfig();
            hasSection = false;
        }
    };

    while (std::getline(file, line)) {
        line = Trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;

        if (line.front() == '[' && line.back() == ']') {
            pushCurrent();
            currentCfg.section = Trim(line.substr(1, line.size() - 2));
            hasSection = true;
            continue;
        }

        size_t eqPos = line.find('=');
        if (eqPos == std::string::npos || !hasSection) continue;

        std::string key = Trim(line.substr(0, eqPos));
        std::string val = Trim(line.substr(eqPos + 1));

        if (key == "name") currentCfg.name = val;
        else if (key == "label") currentCfg.particleLabel = val;
        else if (key == "Z") currentCfg.Z = std::stoi(val);
        else if (key == "A") currentCfg.A = std::stoi(val);
        else if (key == "energy") currentCfg.energy = std::stod(val);
        else if (key == "unit") currentCfg.energyUnit = val;
        else if (key == "x_min") currentCfg.xAxisMin = ParseBound(val);
        else if (key == "x_max") currentCfg.xAxisMax = ParseBound(val);
        else if (key == "y_min") currentCfg.yAxisMin = ParseBound(val);
        else if (key == "y_max") currentCfg.yAxisMax = ParseBound(val);
        else if (key == "y_res_min") currentCfg.yAxisResMin = std::stod(val);
        else if (key == "y_res_max") currentCfg.yAxisResMax = std::stod(val);
        else if (key == "is_log_x") currentCfg.isLogX = (std::stoi(val) != 0);
        else if (key == "err_type") currentCfg.expRelErrType = val;
        else if (key == "data_file") currentCfg.expDataFile = val;
        else if (key == "authors") currentCfg.dataAuthors = val;
        else if (key == "data_type") currentCfg.dataType = val;
        else if (key == "legend_pos") {
            std::replace(val.begin(), val.end(), ',', ' ');
            std::stringstream ss(val);
            ss >> currentCfg.legX1 >> currentCfg.legY1 >> currentCfg.legX2 >> currentCfg.legY2;
        }
    }
    pushCurrent();
    file.close();
    return configs;
}

// Загрузка экспериментальных данных
std::vector<ExpPoint> LoadExperimentalData(const RDDConfig& cfg) {
    std::vector<ExpPoint> points;
    if (cfg.expDataFile.empty() || cfg.expDataFile == "none") return points;

    std::ifstream file(cfg.expDataFile);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            line = Trim(line);
            if (line.empty() || line[0] == '#' || line[0] == ';') continue;
            std::replace(line.begin(), line.end(), ',', ' ');

            std::stringstream ss(line);
            double x = 0.0, y = 0.0;
            if (ss >> x >> y) {
                points.push_back({x, y});
            }
        }
        file.close();

        if (!points.empty()) {
            std::sort(points.begin(), points.end(), [](const ExpPoint& a, const ExpPoint& b) {
                return a.x < b.x;
            });
            std::cout << "  [Info] Loaded " << points.size() << " points from: " << cfg.expDataFile << std::endl;
            return points;
        }
    }

    return points;
}

// Определение относительной ошибки эксперимента
double GetExpRelErr(double x, const RDDConfig& cfg) {
    if (cfg.expRelErrType == "fixed0.10") return 0.10;
    if (cfg.expRelErrType == "fixed0.15") return 0.15;
    if (cfg.expRelErrType == "fixed0.20") return 0.20;

    // Исключение Wingate & Baum (1976) для Alpha 1 MeV
    if (cfg.name == "alpha" && std::abs(cfg.energy - 1.0) < 0.1) {
        return (x < 5.0) ? 0.50 : 0.35;
    }

    // Стандартное правило Wingate & Baum (1976)
    return (x < 5.0) ? 0.40 : 0.20;
}

// Фильтрация файлов воркеров Geant4 MT (*_t0.root, *_t1.root) и сборка TChain
int AddFilteredFilesToChain(TChain* chain, const std::string& dirPath, const std::string& searchPrefix) {
    TSystemDirectory dir("root_dir", dirPath.c_str());
    TList* files = dir.GetListOfFiles();
    if (!files) return 0;

    int addedCount = 0;
    TIter next(files);
    TSystemFile* file;
    while ((file = (TSystemFile*)next())) {
        TString fname = file->GetName();
        if (file->IsDirectory()) continue;
        if (!fname.EndsWith(".root")) continue;

        bool isWorker = false;
        for (int t = 0; t < 64; ++t) {
            if (fname.Contains(Form("_t%d.root", t)) || fname.Contains(Form("_t%d_", t))) {
                isWorker = true;
                break;
            }
        }
        if (isWorker) continue;

        if (fname.Contains(searchPrefix.c_str())) {
            TString fullPath = Form("%s/%s", dirPath.c_str(), fname.Data());
            chain->Add(fullPath);
            addedCount++;
        }
    }
    return addedCount;
}

void plotSingleRDD(const RDDConfig& cfg, std::vector<ValidationResult>& allResults) {
    TH1::AddDirectory(kFALSE);
    gStyle->SetCanvasPreferGL(kTRUE);
    gStyle->SetPadBorderSize(0);

    auto expPoints = LoadExperimentalData(cfg);
    bool hasExpData = !expPoints.empty();
    size_t n = expPoints.size();

    std::vector<double> xData(n), yData(n);
    for (size_t i = 0; i < n; ++i) {
        xData[i] = expPoints[i].x;
        yData[i] = expPoints[i].y;
    }

    // ДИАПАЗОН ОСИ X: СТРОГО ОТ MIN ДО MAX ЭКСПЕРИМЕНТА
    double xAxisMin = 0.0, xAxisMax = 0.0, yAxisMin = 0.0, yAxisMax = 0.0;

    if (hasExpData) {
        double expXMin = xData.front();
        double expXMax = xData.back();
        double expYMin = *std::min_element(yData.begin(), yData.end());
        double expYMax = *std::max_element(yData.begin(), yData.end());

        xAxisMin = cfg.xAxisMin.isExp ? expXMin : cfg.xAxisMin.val;
        xAxisMax = cfg.xAxisMax.isExp ? expXMax : cfg.xAxisMax.val;
        yAxisMin = cfg.yAxisMin.isExp ? (expYMin * 0.5) : cfg.yAxisMin.val;
        yAxisMax = cfg.yAxisMax.isExp ? (expYMax * 2.0) : cfg.yAxisMax.val;
    } else {
        xAxisMin = cfg.xAxisMin.isExp ? (cfg.isLogX ? 0.1 : 0.0) : cfg.xAxisMin.val;
        xAxisMax = cfg.xAxisMax.isExp ? 100.0 : cfg.xAxisMax.val;
        yAxisMin = cfg.yAxisMin.isExp ? 1.0 : cfg.yAxisMin.val;
        yAxisMax = cfg.yAxisMax.isExp ? 1e7 : cfg.yAxisMax.val;
    }

    double lineWidth = 1.5;
    double nbBins = 1;

    const std::vector<PhysOpt> physOptions = {
        {"G4EmDNAPhysics_option2", "DNA2", "DNA Opt2", kRed},
        {"G4EmDNAPhysics_option4", "DNA4", "DNA Opt4", kBlue},
        {"G4EmDNAPhysics_option6", "DNA6", "DNA Opt6", kGreen + 2},
        {"G4EmDNAPhysics_option8", "DNA8", "DNA Opt8", kMagenta + 2}
    };

    std::string canvasIdentifier = "RDD_" + cfg.name + "_" + FormatEnergy(cfg.energy) + cfg.energyUnit;
    TCanvas* mainCanvas = new TCanvas("mainCanvas", canvasIdentifier.c_str(), 1920, 1080);

    TPad* pad1 = new TPad("pad1", "main plot", 0.0, 0.3, 1.0, 1.0);
    TPad* pad2 = new TPad("pad2", "residuals", 0.0, 0.0, 1.0, 0.3);

    double leftMargin = 0.10;
    double rightMargin = 0.10;

    pad1->SetLeftMargin(leftMargin);
    pad1->SetRightMargin(rightMargin);
    pad2->SetLeftMargin(leftMargin);
    pad2->SetRightMargin(rightMargin);

    pad1->SetLogy();
    pad1->SetBottomMargin(0);
    pad1->SetTickx(1);
    pad1->SetTicky(1);
    if (cfg.isLogX) pad1->SetLogx();
    pad1->Draw();

    pad2->SetTopMargin(0);
    pad2->SetBottomMargin(0.3);
    pad2->SetTickx(1);
    pad2->SetTicky(1);
    if (cfg.isLogX) pad2->SetLogx();
    pad2->Draw();

    TH1F* hFrame = new TH1F("hFrame", "", nbBins, xAxisMin, xAxisMax);
    TAxis* XaxisRes = hFrame->GetXaxis();
    TAxis* YaxisRes = hFrame->GetYaxis();

    // 1. Отрисовка базовой рамки Pad 1
    pad1->cd();
    TH1F* hFrame1 = new TH1F("hFrame1", "", nbBins, xAxisMin, xAxisMax);
    hFrame1->SetStats(0);
    TAxis* xAxis = hFrame1->GetXaxis();
    TAxis* yAxis = hFrame1->GetYaxis();
    hFrame1->Draw("AXIS");

    xAxis->SetLimits(xAxisMin, xAxisMax);
    xAxis->SetRangeUser(xAxisMin, xAxisMax);
    xAxis->SetLabelSize(0);
    xAxis->SetTitleSize(0);
    xAxis->SetTickSize(2.95 * cfg.yTickLength);

    yAxis->SetRangeUser(yAxisMin, yAxisMax);
    if (cfg.isLogX) {
        yAxis->SetMoreLogLabels(kFALSE);
        pad1->Update();
        for (int i = -20; i < 20; i++) {
            if (std::abs(i % 2) == 1) {
                yAxis->ChangeLabelByValue(TMath::Power(10., i), -1, 0, -1, -1, -1, "");
            }
        }
        yAxis->ChangeLabelByValue(1.0, -1, -1, -1, -1, -1, "10^{0}");
        pad1->Modified();
    }
    yAxis->SetTitle("Dose (Gy)");
    yAxis->SetTickSize(cfg.yTickLength);
    yAxis->SetTitleSize(0.07);
    yAxis->SetTitleOffset(0.40);
    yAxis->CenterTitle(true);

    pad1->Update();
    pad1->RedrawAxis();

    // =========================================================================
    // 2. Серая область погрешности эксперимента на Pad 1 (ДО линий симуляции)
    // =========================================================================
    TGraphAsymmErrors* expBand = nullptr;
    if (hasExpData) {
        expBand = new TGraphAsymmErrors(n);
        for (size_t i = 0; i < n; ++i) {
            double relErr = GetExpRelErr(xData[i], cfg);
            double totalErr = relErr * yData[i];

            // Защита от ухода нижней границы ниже минимума логарифмической шкалы
            double yLow = TMath::Max(yData[i] - totalErr, yAxisMin * 1.001);
            double errLow = yData[i] - yLow;

            expBand->SetPoint(i, xData[i], yData[i]);
            // Ширина по X = 0.0, чтобы получить сплошную гладкую ленту между точками
            expBand->SetPointError(i, 0.0, 0.0, errLow, totalErr);
        }
        expBand->SetFillColorAlpha(kGray, 0.35);
        expBand->SetFillStyle(1001);
        expBand->SetLineWidth(0);
        expBand->Draw("E3 SAME");
    }

    std::vector<TGraphAsymmErrors*> loadedSimGraphs;
    std::vector<PhysOpt> loadedPhysOpts;

    int counter = 0;
    std::string energyStr = FormatEnergy(cfg.energy);

    for (const auto& phys : physOptions) {
        TChain* tree = new TChain("radial");
        TChain* treeLET = new TChain("LET");

        std::string searchPrefix = Form("radial_ion-%d-%d_%s-%s_%s_",
                                        cfg.Z, cfg.A, energyStr.c_str(), cfg.energyUnit.c_str(), phys.name.c_str());

        int filesCount = AddFilteredFilesToChain(tree, "output/root", searchPrefix);
        AddFilteredFilesToChain(treeLET, "output/root", searchPrefix);

        if (filesCount == 0 || tree->GetEntries() == 0) {
            std::cout << "  [Warning] No files found for: " << phys.legend << " (Prefix: " << searchPrefix << ")" << std::endl;
            delete tree;
            delete treeLET;
            continue;
        }

        double dose = 0, radius = 0, rmseDose = 0, meanLET = 0, rmseLET = 0;
        Int_t nbEvents = 0;
        tree->SetBranchAddress("dose", &dose);
        tree->SetBranchAddress("radius", &radius);
        tree->SetBranchAddress("rmse", &rmseDose);

        if (treeLET->GetEntries() > 0) {
            treeLET->SetBranchAddress("mean", &meanLET);
            treeLET->SetBranchAddress("rmse", &rmseLET);
            treeLET->SetBranchAddress("events", &nbEvents);
            treeLET->GetEntry(0);
        }

        pad1->cd();
        TGraphAsymmErrors* graph = new TGraphAsymmErrors();

        Long64_t nbEntries = tree->GetEntries();
        size_t xCounter = 0;
        tree->GetEntry(0);
        double prevRadius = radius;
        double prevDose = dose;
        double prevRmseDose = rmseDose;

        graph->SetPoint(0, radius, dose);
        graph->SetPointError(0, 0, 0, 0, 0);

        for (Long64_t j = 1; j < nbEntries; ++j) {
            tree->GetEntry(j);
            if (hasExpData) {
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
            }

            int nextIdx = graph->GetN();
            graph->SetPoint(nextIdx, radius, dose);
            graph->SetPointError(nextIdx, 0, 0, 0, 0);

            prevRadius = radius;
            prevDose = dose;
            prevRmseDose = rmseDose;
        }

        graph->SetLineWidth(lineWidth);
        graph->SetLineColor(phys.color);
        graph->Draw("L SAME");

        loadedSimGraphs.push_back(graph);
        loadedPhysOpts.push_back(phys);

        pad2->cd();
        if (hasExpData) {
            std::vector<double> res(n, 0.0);
            double sumChi2 = 0.0;
            double rmse = 0.0;

            for (size_t i = 0; i < n; ++i) {
                double xExp = xData[i];
                double yExp = yData[i];
                double simVal = graph->Eval(xExp);

                double simRMSE = 0.0;
                int nPoints = graph->GetN();
                for (int k = 0; k < nPoints; ++k) {
                    if (TMath::Abs(graph->GetX()[k] - xExp) < 1e-7) {
                        simRMSE = graph->GetErrorYhigh(k);
                        break;
                    }
                }

                double simErrorMean = (nbEvents > 0) ? (simRMSE / TMath::Sqrt(nbEvents)) : 0.0;
                double relExpErr = GetExpRelErr(xExp, cfg);
                double sigmaExp = relExpErr * yExp;
                double totalSigma2 = sigmaExp * sigmaExp + simErrorMean * simErrorMean;

                if (totalSigma2 > 0.0) {
                    sumChi2 += TMath::Power(yExp - simVal, 2) / totalSigma2;
                }

                res[i] = (yExp != 0.0) ? (yExp - simVal) / yExp : 0.0;
                rmse += TMath::Power(res[i], 2);
            }

            rmse = TMath::Sqrt(rmse / n);
            double normChi2 = (n > 0) ? (sumChi2 / n) : 0.0;

            allResults.push_back({cfg.name, cfg.energy, cfg.energyUnit, phys.legend, normChi2, rmse, nbEvents});

            if (counter == 0) {
                hFrame->SetStats(0);
                hFrame->Draw("AXIS");
                XaxisRes->SetLimits(xAxisMin, xAxisMax);
                XaxisRes->SetRangeUser(xAxisMin, xAxisMax);
                XaxisRes->SetTitle("Distance from the axis (nm)");
                XaxisRes->SetTitleSize(0.155);
                XaxisRes->SetLabelSize(0.09);
                XaxisRes->CenterTitle(true);
                XaxisRes->SetTickSize(6.95*cfg.yTickLength);
                XaxisRes->SetTitleOffset(0.88);

                YaxisRes->SetRangeUser(cfg.yAxisResMin, cfg.yAxisResMax);
                YaxisRes->SetTitle("#it{#eta} = [1 - Geant4/Ref]");
                YaxisRes->SetTickSize(cfg.yResTickLength);
                YaxisRes->SetTitleSize(0.082);
                YaxisRes->SetTitleOffset(0.30);
                YaxisRes->SetLabelSize(0.07);
                YaxisRes->SetLabelOffset(0.008);
                YaxisRes->SetNdivisions(305);
                YaxisRes->CenterTitle(true);

                TGraphErrors* errCorridor = new TGraphErrors(n + 2);
                double errLeft = GetExpRelErr(xAxisMin, cfg);
                errCorridor->SetPoint(0, xAxisMin, 0.0);
                errCorridor->SetPointError(0, 0.5, errLeft);

                for (size_t i = 0; i < n; ++i) {
                    double x = xData[i];
                    double relErr = GetExpRelErr(x, cfg);
                    errCorridor->SetPoint(i + 1, x, 0.0);
                    errCorridor->SetPointError(i + 1, 0.5, relErr);
                }

                double errRight = GetExpRelErr(xAxisMax, cfg);
                errCorridor->SetPoint(n + 1, xAxisMax, 0.0);
                errCorridor->SetPointError(n + 1, 0.5, errRight);

                errCorridor->SetFillColorAlpha(kGray, 0.4);
                errCorridor->SetFillStyle(1001);
                errCorridor->SetLineWidth(0);
                errCorridor->Draw("E3 SAME");

                TLine* zeroLine = new TLine(xAxisMin, 0.0, xAxisMax, 0.0);
                zeroLine->SetLineStyle(2);
                zeroLine->Draw("SAME");
            }

            TGraph* splineRes = new TGraph(n, xData.data(), res.data());
            splineRes->SetPoint(splineRes->GetN(), xAxisMin, splineRes->Eval(xAxisMin));
            splineRes->SetPoint(splineRes->GetN(), xAxisMax, splineRes->Eval(xAxisMax));
            splineRes->Sort();
            splineRes->SetLineWidth(lineWidth);
            splineRes->SetLineColor(phys.color);
            splineRes->Draw("L SAME");

        } else {
            if (counter == 0) {
                hFrame->SetStats(0);
                hFrame->Draw("AXIS");
                XaxisRes->SetLimits(xAxisMin, xAxisMax);
                XaxisRes->SetRangeUser(xAxisMin, xAxisMax);
                XaxisRes->SetTitle("Distance from the axis [nm]");
                XaxisRes->SetTitleSize(0.135);
                XaxisRes->SetLabelSize(0.09);
                XaxisRes->CenterTitle(true);

                YaxisRes->SetRangeUser(cfg.yAxisResMin, cfg.yAxisResMax);
                YaxisRes->SetTitle("#it{#eta} = [1 - Geant4/Ref]");
                YaxisRes->SetTickSize(cfg.yResTickLength);
                YaxisRes->SetTitleSize(0.082);
                YaxisRes->SetTitleOffset(0.30);
                YaxisRes->SetLabelSize(0.07);
                YaxisRes->CenterTitle(true);
            }
        }

        counter++;
        delete tree;
        delete treeLET;
    }

    pad2->cd();
    hFrame->Draw("SAME AXIS");
    pad2->RedrawAxis();
    pad2->Update();

    // 3. Точки эксперимента на Pad 1
    TGraphAsymmErrors* graphExp = nullptr;
    if (hasExpData) {
        pad1->cd();
        graphExp = new TGraphAsymmErrors(n);

        for (size_t i = 0; i < n; ++i) {
            double x = xData[i];
            double y = yData[i];
            double relErr = GetExpRelErr(x, cfg);
            double totalErr = relErr * y;

            graphExp->SetPoint(i, x, y);
            graphExp->SetPointError(i, 0, 0, totalErr, totalErr);
        }

        graphExp->SetMarkerStyle(25);
        graphExp->SetMarkerSize(3);
        graphExp->SetMarkerColor(kBlack);
        graphExp->SetLineColor(kBlack);
        graphExp->SetLineWidth(1);
        graphExp->Draw("P E SAME");
    }

    // 4. Двойная легенда: текст прижат вправо, значки в правой колонке
    pad1->cd();
    double legX1 = cfg.legX1;
    double legX2 = cfg.legX2;
    double iconWidth = 0.05;

    TLegend* legText = new TLegend(legX1, cfg.legY1, legX2 - iconWidth, cfg.legY2);
    legText->SetBorderSize(0);
    legText->SetFillStyle(0);
    legText->SetTextSize(0.035);
    legText->SetTextAlign(32);
    legText->SetMargin(0.0);
    legText->SetEntrySeparation(0.45);

    TLegend* legIcons = new TLegend(legX2 - iconWidth, cfg.legY1, legX2, cfg.legY2);
    legIcons->SetBorderSize(0);
    legIcons->SetFillStyle(0);
    legIcons->SetMargin(1.0);
    legIcons->SetEntrySeparation(0.45);

    // Эксперимент — на 1-е место в легенде
    if (hasExpData && graphExp) {
        TText metric;
        UInt_t w1 = 0, h1 = 0;
        UInt_t w2 = 0, h2 = 0;

        metric.SetTextFont(62);
        metric.SetTextSize(legText->GetTextSize() * 0.9);
        metric.GetTextExtent(w1, h1, cfg.dataAuthors.c_str());

        metric.SetTextFont(42);
        metric.SetTextSize(legText->GetTextSize() * 0.75);
        metric.GetTextExtent(w2, h2, cfg.dataType.c_str());

        double dx = 0.0;
        if (w2 > 0 && w1 > w2) {
            dx = (double)(w1 - w2) / (double)w2;
        }

        std::string expLabel = Form(
            "#splitline{#scale[0.9]{#bf{%s}}}{#kern[%.3f]{#scale[0.75]{#color[12]{%s}}}}",
            cfg.dataAuthors.c_str(),
            dx,
            cfg.dataType.c_str()
        );

        legText->AddEntry((TObject*)nullptr, expLabel.c_str(), "");
        legIcons->AddEntry(graphExp, "", "pe");
    }

    // Модели симуляции — следующими строками
    for (size_t i = 0; i < loadedSimGraphs.size(); ++i) {
        std::string modelLabel = Form("#scale[0.9]{#bf{%s}}", loadedPhysOpts[i].legend.c_str());
        legText->AddEntry((TObject*)nullptr, modelLabel.c_str(), "");
        legIcons->AddEntry(loadedSimGraphs[i], "", "l");
    }
    legText->Draw();
    legIcons->Draw();

    pad1->RedrawAxis();
    mainCanvas->Update();

    // Сохранение графиков в PDF
    fs::create_directories("output/plots/validation");
    std::string outputPath = "output/plots/validation/RDD_" + cfg.name + "_" + FormatEnergy(cfg.energy) + cfg.energyUnit + ".pdf";
    mainCanvas->SaveAs(outputPath.c_str());

    delete mainCanvas;
    std::cout << "  [Saved] Plot: " << outputPath << std::endl;
}

void SaveRDDValidationSummary(const std::vector<ValidationResult>& results, const std::string& filename) {
    std::ofstream out(filename);
    if (!out.is_open()) {
        std::cerr << "Cannot open summary file: " << filename << std::endl;
        return;
    }

    out << "Particle,Energy,Unit,PhysicsModel,ReducedChi2,RMSE,SimEvents\n";
    for (const auto& r : results) {
        out << r.particle << ","
            << r.energy << ","
            << r.energyUnit << ","
            << "\"" << r.model << "\","
            << std::fixed << std::setprecision(4) << r.reducedChi2 << ","
            << std::fixed << std::setprecision(6) << r.rmse << ","
            << r.nEvents << "\n";
    }
    out.close();

    std::cout << "\n========================================================================\n";
    std::cout << "                      RDD VALIDATION SUMMARY REPORT                      \n";
    std::cout << "========================================================================\n";
    std::cout << std::left << std::setw(12) << "Particle"
              << std::setw(14) << "Energy"
              << std::setw(16) << "Model"
              << std::setw(14) << "Red. Chi2"
              << std::setw(12) << "RMSE"
              << std::setw(12) << "SimEvents" << "\n";
    std::cout << "------------------------------------------------------------------------\n";
    for (const auto& r : results) {
        std::cout << std::left << std::setw(12) << r.particle
                  << std::setw(14) << (std::to_string(r.energy) + " " + r.energyUnit)
                  << std::setw(16) << r.model
                  << std::setw(14) << std::fixed << std::setprecision(2) << r.reducedChi2
                  << std::setw(12) << std::fixed << std::setprecision(4) << r.rmse
                  << std::setw(12) << r.nEvents << "\n";
    }
    std::cout << "========================================================================\n";
    std::cout << "Summary metadata exported to: " << filename << "\n";
}

void plotRDDValidation() {
    auto configs = LoadRDDConfigsFromCFG("input/experimental_data/rdd_particles.cfg");
    if (configs.empty()) {
        std::cerr << "No configuration entries loaded from input/experimental_data/rdd_particles.cfg!" << std::endl;
        return;
    }

    std::vector<ValidationResult> allResults;
    for (const auto& cfg : configs) {
        std::cout << "\n========================================\n";
        std::cout << "Processing RDD: " << cfg.name << " (" << cfg.energy << " " << cfg.energyUnit << ")\n";
        std::cout << "========================================\n";
        plotSingleRDD(cfg, allResults);
    }

    if (!allResults.empty()) {
        SaveRDDValidationSummary(allResults, "output/plots/validation/rdd_validation_summary.csv");
    }
    std::cout << "\nAll RDD validation plots generated and saved to 'output/plots/validation/'.\n";
}