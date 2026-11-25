// scripts/root/plotStoppingPowerValidation.C
// Execute under ROOT: root -l -q 'scripts/root/plotStoppingPowerValidation.C+'

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

struct AxisBound {
    bool isExp = true;
    double val = 0.0;
};

struct ValidationConfig {
    std::string section;
    std::string name;             // "proton", "alpha", "C12", "e-", "e+"
    std::string particleLabel;    // "proton", "alpha", "{}^{12}C", "e^{-}", "e^{+}"
    int nucleons = 1;             // A
    std::vector<std::string> physModels;
    AxisBound xAxisMin;
    AxisBound xAxisMax;
    AxisBound yAxisMin;
    AxisBound yAxisMax;
    double yAxisResMin = -0.20;
    double yAxisResMax = 0.20;
    double xTickLength = 0.015;
    double yTickLength = 0.015;
    double yResTickLength = 0.0193;
    bool isLogX = true;
    bool isLogY = true;
    std::string errType = "icru90_proton";
    std::string dataFile = "";
    std::string dataAuthors = "ICRU Report 90";
    std::string dataType = "SEMI-EMPIRICAL BENCHMARK";
    std::string legendOrder = "icon_text";
    double legX1 = 0.14;
    double legY1 = 0.48;
    double legX2 = 0.42;
    double legY2 = 0.88;
};

struct RefPoint {
    double energy; // MeV/u or MeV
    double val;    // Stopping power [MeV cm^2 g^-1]
    double relErr; // Relative uncertainty
};

struct PhysOpt {
    std::string key;
    std::string legend;
    Int_t color;
};

struct ValidationResult {
    std::string particle;
    std::string model;
    double reducedChi2;
    double rmse;
    Int_t nPoints;
};

// Функция удаления пробелов и табов по краям
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

// Разбор списка физических моделей через запятую
std::vector<std::string> ParseModelsList(const std::string& str) {
    std::vector<std::string> result;
    std::stringstream ss(str);
    std::string item;
    while (std::getline(ss, item, ',')) {
        std::string cleaned = Trim(item);
        if (!cleaned.empty()) {
            result.push_back(cleaned);
        }
    }
    return result;
}

// Расчет неопределенности ICRU Report 90 по ступенчатым интервалам энергий
double GetICRU90RelError(double energyU, const std::string& errType) {
    if (errType == "icru90_proton" || errType == "icru90_alpha") {
        if (energyU < 0.01) return 0.15;
        if (energyU < 0.10) return 0.10;
        if (energyU < 1.00) return 0.06;
        return 0.03;
    } else if (errType == "icru90_carbon") {
        if (energyU < 0.10) return 1.00;
        if (energyU < 1.00) return 0.30;
        if (energyU < 10.0) return 0.15;
        if (energyU < 100.0) return 0.05;
        return 0.02;
    } else if (errType == "icru90_lepton") {
        if (energyU < 0.01) return 0.05;
        if (energyU < 0.10) return 0.015;
        return 0.01;
    }
    return 0.05; // Дефолтное значение
}

// Загрузка конфигурационного файла формата INI/CFG
std::vector<ValidationConfig> LoadConfigsFromCFG(const std::string& filename) {
    std::vector<ValidationConfig> configs;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open configuration file: " << filename << std::endl;
        return configs;
    }

    std::string line;
    ValidationConfig currentCfg;
    bool hasSection = false;

    auto pushCurrent = [&]() {
        if (hasSection) {
            if (currentCfg.physModels.empty()) {
                currentCfg.physModels = {"DNA2", "DNA4", "DNA6", "DNA8", "S4"};
            }
            configs.push_back(currentCfg);
            currentCfg = ValidationConfig();
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
        else if (key == "nucleons") currentCfg.nucleons = std::stoi(val);
        else if (key == "models") currentCfg.physModels = ParseModelsList(val);
        else if (key == "x_min") currentCfg.xAxisMin = ParseBound(val);
        else if (key == "x_max") currentCfg.xAxisMax = ParseBound(val);
        else if (key == "y_min") currentCfg.yAxisMin = ParseBound(val);
        else if (key == "y_max") currentCfg.yAxisMax = ParseBound(val);
        else if (key == "y_res_min") currentCfg.yAxisResMin = std::stod(val);
        else if (key == "y_res_max") currentCfg.yAxisResMax = std::stod(val);
        else if (key == "is_log_x") currentCfg.isLogX = (std::stoi(val) != 0);
        else if (key == "is_log_y") currentCfg.isLogY = (std::stoi(val) != 0);
        else if (key == "err_type") currentCfg.errType = val;
        else if (key == "data_file") currentCfg.dataFile = val;
        else if (key == "authors") currentCfg.dataAuthors = val;
        else if (key == "data_type") currentCfg.dataType = val;
        else if (key == "legend_order" || key == "legend_style") currentCfg.legendOrder = val;
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

// Загрузка референсных данных ICRU90
std::vector<RefPoint> LoadReferenceData(const ValidationConfig& cfg) {
    std::vector<RefPoint> points;
    if (cfg.dataFile.empty() || cfg.dataFile == "none") return points;

    std::ifstream file(cfg.dataFile);
    if (!file.is_open()) {
        std::cerr << "  [Warning] Cannot open reference data file: " << cfg.dataFile << std::endl;
        return points;
    }

    std::string line;
    while (std::getline(file, line)) {
        line = Trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;
        std::replace(line.begin(), line.end(), ',', ' ');

        std::stringstream ss(line);
        double e = 0.0, v = 0.0, err = -1.0;
        if (ss >> e >> v) {
            if (!(ss >> err)) {
                // Если в файле 2 колонки, берем неопределенность по стандарту ICRU90
                err = GetICRU90RelError(e, cfg.errType);
            }
            points.push_back({e, v, err});
        }
    }
    file.close();

    if (!points.empty()) {
        std::sort(points.begin(), points.end(), [](const RefPoint& a, const RefPoint& b) {
            return a.energy < b.energy;
        });
        std::cout << "  [Info] Loaded " << points.size() << " reference points from: " << cfg.dataFile << std::endl;
    }
    return points;
}

// Метаданные доступных физических моделей
std::map<std::string, PhysOpt> GetPhysOptCatalog() {
    std::map<std::string, PhysOpt> m;
    m["DNA2"] = {"DNA2", "DNA Opt2", kRed};
    m["DNA4"] = {"DNA4", "DNA Opt4", kBlue};
    m["DNA6"] = {"DNA6", "DNA Opt6", kGreen + 2};
    m["DNA8"] = {"DNA8", "DNA Opt8", kMagenta + 2};
    m["S4"]   = {"S4",   "STD Opt4", kOrange + 7};
    return m;
}

void ProcessSingleValidation(const ValidationConfig& cfg, std::vector<ValidationResult>& allResults) {
    TH1::AddDirectory(kFALSE);
    gStyle->SetCanvasPreferGL(kTRUE);
    gStyle->SetPadBorderSize(0);

    auto refPoints = LoadReferenceData(cfg);
    bool hasRefData = !refPoints.empty();
    size_t n = refPoints.size();

    std::vector<double> xData(n), yData(n), relErrData(n);
    for (size_t i = 0; i < n; ++i) {
        xData[i] = refPoints[i].energy;
        yData[i] = refPoints[i].val;
        relErrData[i] = refPoints[i].relErr;
    }

    // Границы осей X и Y
    double xAxisMin = 0.001, xAxisMax = 1e5, yAxisMin = 1.0, yAxisMax = 1e4;
    if (hasRefData) {
        double expXMin = xData.front();
        double expXMax = xData.back();
        double expYMin = *std::min_element(yData.begin(), yData.end());
        double expYMax = *std::max_element(yData.begin(), yData.end());

        xAxisMin = cfg.xAxisMin.isExp ? expXMin : cfg.xAxisMin.val;
        xAxisMax = cfg.xAxisMax.isExp ? expXMax : cfg.xAxisMax.val;
        yAxisMin = cfg.yAxisMin.isExp ? (cfg.isLogY ? expYMin * 0.5 : expYMin * 0.9) : cfg.yAxisMin.val;
        yAxisMax = cfg.yAxisMax.isExp ? (cfg.isLogY ? expYMax * 2.0 : expYMax * 1.1) : cfg.yAxisMax.val;
    } else {
        xAxisMin = cfg.xAxisMin.isExp ? 0.001 : cfg.xAxisMin.val;
        xAxisMax = cfg.xAxisMax.isExp ? 1000.0 : cfg.xAxisMax.val;
        yAxisMin = cfg.yAxisMin.isExp ? 1.0 : cfg.yAxisMin.val;
        yAxisMax = cfg.yAxisMax.isExp ? 1e4 : cfg.yAxisMax.val;
    }

    double lineWidth = 1.5;
    double nbBins = 1;
    auto optCatalog = GetPhysOptCatalog();

    std::string canvasIdentifier = cfg.section;
    TCanvas* mainCanvas = new TCanvas("mainCanvas", canvasIdentifier.c_str(), 1920, 1080);

    TPad* pad1 = new TPad("pad1", "main plot", 0.0, 0.3, 1.0, 1.0);
    TPad* pad2 = new TPad("pad2", "residuals", 0.0, 0.0, 1.0, 0.3);

    double leftMargin = 0.10;
    double rightMargin = 0.10;

    pad1->SetLeftMargin(leftMargin);
    pad1->SetRightMargin(rightMargin);
    pad2->SetLeftMargin(leftMargin);
    pad2->SetRightMargin(rightMargin);

    if (cfg.isLogY) pad1->SetLogy();
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
    if (cfg.isLogY && cfg.isLogX) {
        yAxis->SetMoreLogLabels(kFALSE);
        pad1->Update();
        /*for (int i = -20; i < 20; i++) {
            if (std::abs(i % 2) == 1) {
                yAxis->ChangeLabelByValue(TMath::Power(10., i), -1, 0, -1, -1, -1, "");
            }
        }*/
        yAxis->ChangeLabelByValue(1.0, -1, -1, -1, -1, -1, "10^{0}");
        pad1->Modified();
    }

    // Заголовок оси Y в строгом международном стиле
    // yAxis->SetTitle("#splitline{Electronic stopping power}{#it{S}_{el}/#it{#rho} [MeV cm^{2} g^{-1}]}");
    yAxis->SetTitle("Electronic stopping power (keV/um)");
    yAxis->SetTickSize(cfg.yTickLength);
    yAxis->SetTitleSize(0.055);
    yAxis->SetTitleOffset(0.45);
    yAxis->CenterTitle(true);

    pad1->Update();
    pad1->RedrawAxis();

    // =========================================================================
    // 2. Серая полоса неопределенности и пунктирная линия ICRU90
    // =========================================================================
    TGraphAsymmErrors* expBand = nullptr;
    TGraph* expCentralLine = nullptr;

    if (hasRefData) {
        expBand = new TGraphAsymmErrors(n);
        expCentralLine = new TGraph(n);

        for (size_t i = 0; i < n; ++i) {
            double totalErr = relErrData[i] * yData[i];
            double yLow = cfg.isLogY ? TMath::Max(yData[i] - totalErr, yAxisMin * 1.001) : (yData[i] - totalErr);
            double errLow = yData[i] - yLow;

            expBand->SetPoint(i, xData[i], yData[i]);
            expBand->SetPointError(i, 0.0, 0.0, errLow, totalErr);
            expCentralLine->SetPoint(i, xData[i], yData[i]);
        }

        expBand->SetFillColorAlpha(kGray, 0.35);
        expBand->SetFillStyle(1001);
        expBand->SetLineWidth(0);
        expBand->Draw("E3 SAME");

        expCentralLine->SetLineColor(kBlack);
        expCentralLine->SetLineStyle(2);
        expCentralLine->SetLineWidth(lineWidth);
        expCentralLine->Draw("L SAME");
    }

    // =========================================================================
    // 3. Загрузка и отрисовка симуляционных моделей из .txt файлов
    // =========================================================================
    std::vector<TGraphAsymmErrors*> loadedSimGraphs;
    std::vector<PhysOpt> loadedPhysOpts;

    double rho = 0.998;
    int counter = 0;

    for (const auto& modelKey : cfg.physModels) {
        if (optCatalog.find(modelKey) == optCatalog.end()) continue;
        PhysOpt phys = optCatalog[modelKey];

        std::string simFilePath = "output/root/" + cfg.name + "_" + modelKey + ".txt";
        std::ifstream inputFile(simFilePath);

        if (!inputFile.is_open()) {
            std::cout << "  [Warning] Missing file: " << simFilePath << std::endl;
            continue;
        }

        std::vector<double> vecEnergy, vecSimVal, vecRMSE;
        Int_t totalEvents = 0;

        double energyVar, spVar, rmseVar, partNb;
        while (inputFile >> energyVar >> spVar >> rmseVar >> partNb) {
            vecEnergy.push_back(energyVar * 1e-6 / cfg.nucleons);
            vecSimVal.push_back(spVar * 10.0 / rho);
            vecRMSE.push_back(rmseVar * 10.0 / rho);
            totalEvents = static_cast<Int_t>(partNb);
        }
        inputFile.close();

        size_t nSim = vecEnergy.size();
        if (nSim == 0) continue;

        pad1->cd();
        TGraphAsymmErrors* graph = new TGraphAsymmErrors(nSim);
        for (size_t i = 0; i < nSim; ++i) {
            graph->SetPoint(i, vecEnergy[i], vecSimVal[i]);
            graph->SetPointError(i, 0, 0, 0, 0);
        }

        graph->SetLineWidth(lineWidth);
        graph->SetLineColor(phys.color);
        graph->Draw("L SAME");

        loadedSimGraphs.push_back(graph);
        loadedPhysOpts.push_back(phys);

        // =====================================================================
        // 4. Pad 2: Residuals и расчет Reduced Chi2 / RMSE
        // =====================================================================
        pad2->cd();
        if (hasRefData) {
            std::vector<double> res(n, 0.0);
            double sumChi2 = 0.0;
            double rmse = 0.0;
            int validPoints = 0;

            for (size_t i = 0; i < n; ++i) {
                double xRef = xData[i];
                double yRef = yData[i];
                double simVal = graph->Eval(xRef);

                double simRMSE = 0.0;
                int nPoints = graph->GetN();
                for (int k = 0; k < nPoints; ++k) {
                    if (TMath::Abs(graph->GetX()[k] - xRef) < 1e-7) {
                        simRMSE = vecRMSE[k];
                        break;
                    }
                }

                double simErrorMean = (totalEvents > 0) ? (simRMSE / TMath::Sqrt(totalEvents)) : 0.0;
                double sigmaRef = relErrData[i] * yRef;
                double totalSigma2 = sigmaRef * sigmaRef + simErrorMean * simErrorMean;

                if (totalSigma2 > 0.0) {
                    sumChi2 += TMath::Power(yRef - simVal, 2) / totalSigma2;
                    validPoints++;
                }

                res[i] = (yRef != 0.0) ? (yRef - simVal) / yRef : 0.0;
                rmse += TMath::Power(res[i], 2);
            }

            rmse = TMath::Sqrt(rmse / n);
            double normChi2 = (validPoints > 0) ? (sumChi2 / validPoints) : 0.0;

            allResults.push_back({cfg.particleLabel, phys.legend, normChi2, rmse, static_cast<Int_t>(n)});

            if (counter == 0) {
                hFrame->SetStats(0);
                hFrame->Draw("AXIS");
                XaxisRes->SetLimits(xAxisMin, xAxisMax);
                XaxisRes->SetRangeUser(xAxisMin, xAxisMax);

                if (cfg.nucleons > 4) {
                    XaxisRes->SetTitle("Energy (MeV/u)");
                } else {
                    XaxisRes->SetTitle("Energy (MeV)");
                }
                XaxisRes->SetTitleSize(0.155);
                XaxisRes->SetLabelSize(0.09);
                XaxisRes->CenterTitle(true);
                XaxisRes->SetTickSize(6.95 * cfg.yTickLength);
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

                // Серая полоса неопределенности референса на Pad 2
                TGraphErrors* errCorridor = new TGraphErrors(n + 2);
                errCorridor->SetPoint(0, xAxisMin, 0.0);
                errCorridor->SetPointError(0, 0.0, relErrData.front());

                for (size_t i = 0; i < n; ++i) {
                    errCorridor->SetPoint(i + 1, xData[i], 0.0);
                    errCorridor->SetPointError(i + 1, 0.0, relErrData[i]);
                }

                errCorridor->SetPoint(n + 1, xAxisMax, 0.0);
                errCorridor->SetPointError(n + 1, 0.0, relErrData.back());

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
                XaxisRes->SetTitle((cfg.nucleons > 1) ? "Energy [MeV/u]" : "Energy [MeV]");
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
    }

    pad2->cd();
    hFrame->Draw("SAME AXIS");
    pad2->RedrawAxis();
    pad2->Update();

    // =========================================================================
    // 5. Универсальная легенда: переключение режимов icon_text / text_icon
    // =========================================================================
    pad1->cd();

    const double entryHeight = 0.065; // Фиксированная высота строки
    size_t totalEntries = loadedSimGraphs.size() + (hasRefData ? 1 : 0);
    double dynamicLegY1 = cfg.legY2 - totalEntries * entryHeight;

    if (cfg.legendOrder == "text_icon") {
        // ---------------------------------------------------------------------
        // РЕЖИМ A: Текст в левой колонке (прижат вправо), маркеры в правой
        // ---------------------------------------------------------------------
        double iconWidth = 0.05;
        TLegend* legText = new TLegend(cfg.legX1, dynamicLegY1, cfg.legX2 - iconWidth, cfg.legY2);
        legText->SetBorderSize(0);
        legText->SetFillStyle(0);
        legText->SetTextSize(0.035);
        legText->SetTextAlign(32); // Прижатие текста вправо к значкам
        legText->SetMargin(0.0);
        legText->SetEntrySeparation(0.40);

        TLegend* legIcons = new TLegend(cfg.legX2 - iconWidth, dynamicLegY1, cfg.legX2, cfg.legY2);
        legIcons->SetBorderSize(0);
        legIcons->SetFillStyle(0);
        legIcons->SetMargin(1.0);  // Маркер занимает всю ширину колонки значков
        legIcons->SetEntrySeparation(0.40);

        // 1-я строка: Референс
        if (hasRefData && expCentralLine) {
            TText metric;
            UInt_t w1 = 0, h1 = 0, w2 = 0, h2 = 0;
            metric.SetTextFont(62);
            metric.SetTextSize(legText->GetTextSize() * 0.9);
            metric.GetTextExtent(w1, h1, cfg.dataAuthors.c_str());

            metric.SetTextFont(42);
            metric.SetTextSize(legText->GetTextSize() * 0.75);
            metric.GetTextExtent(w2, h2, cfg.dataType.c_str());

            double dx = (w2 > 0 && w1 > w2) ? ((double)(w1 - w2) / (double)w2) : 0.0;
            std::string expLabel = Form(
                "#splitline{#scale[0.9]{#bf{%s}}}{#kern[%.3f]{#scale[0.75]{#color[12]{%s}}}}",
                cfg.dataAuthors.c_str(), dx, cfg.dataType.c_str()
            );

            legText->AddEntry((TObject*)nullptr, expLabel.c_str(), "");
            legIcons->AddEntry(expCentralLine, "", "l");
        }

        // Строки моделей
        for (size_t i = 0; i < loadedSimGraphs.size(); ++i) {
            std::string modelLabel = Form("#scale[0.9]{#bf{%s}}", loadedPhysOpts[i].legend.c_str());
            legText->AddEntry((TObject*)nullptr, modelLabel.c_str(), "");
            legIcons->AddEntry(loadedSimGraphs[i], "", "l");
        }

        legText->Draw();
        legIcons->Draw();

    } else {
        // ---------------------------------------------------------------------
        // РЕЖИМ B: Маркеры в левой колонке, текст в правой (прижат влево)
        // ---------------------------------------------------------------------
        TLegend* legend = new TLegend(cfg.legX1, dynamicLegY1, cfg.legX2, cfg.legY2);
        legend->SetBorderSize(0);
        legend->SetFillStyle(0);
        legend->SetTextSize(0.035);
        legend->SetTextAlign(12); // Выравнивание текста влево
        legend->SetMargin(0.18);  // Ширина под маркеры слева
        legend->SetEntrySeparation(0.40);

        // 1-я строка: Референс
        if (hasRefData && expCentralLine) {
            std::string expLabel = Form(
                "#splitline{#scale[0.9]{#bf{%s}}}{#scale[0.75]{#color[12]{%s}}}",
                cfg.dataAuthors.c_str(), cfg.dataType.c_str()
            );
            legend->AddEntry(expCentralLine, expLabel.c_str(), "l");
        }

        // Строки моделей
        for (size_t i = 0; i < loadedSimGraphs.size(); ++i) {
            std::string modelLabel = Form("#scale[0.9]{#bf{%s}}", loadedPhysOpts[i].legend.c_str());
            legend->AddEntry(loadedSimGraphs[i], modelLabel.c_str(), "l");
        }

        legend->Draw();
    }

    pad1->RedrawAxis();
    mainCanvas->Update();

    // Сохранение графиков в PDF
    fs::create_directories("output/plots/validation");
    std::string outputPath = "output/plots/validation/" + cfg.section + ".svg";
    mainCanvas->SaveAs(outputPath.c_str());

    delete mainCanvas;
    std::cout << "  [Saved] Plot: " << outputPath << std::endl;
}

void SaveValidationSummary(const std::vector<ValidationResult>& results, const std::string& filename) {
    std::ofstream out(filename);
    if (!out.is_open()) {
        std::cerr << "Cannot open summary file: " << filename << std::endl;
        return;
    }

    out << "Particle,PhysicsModel,ReducedChi2,RMSE,PointsCount\n";
    for (const auto& r : results) {
        out << "\"" << r.particle << "\","
            << "\"" << r.model << "\","
            << std::fixed << std::setprecision(4) << r.reducedChi2 << ","
            << std::fixed << std::setprecision(6) << r.rmse << ","
            << r.nPoints << "\n";
    }
    out.close();

    std::cout << "\n========================================================================\n";
    std::cout << "       ELECTRONIC STOPPING POWER VALIDATION SUMMARY REPORT              \n";
    std::cout << "========================================================================\n";
    std::cout << std::left << std::setw(14) << "Particle"
              << std::setw(16) << "Model"
              << std::setw(14) << "Red. Chi2"
              << std::setw(12) << "RMSE"
              << std::setw(10) << "Points" << "\n";
    std::cout << "------------------------------------------------------------------------\n";
    for (const auto& r : results) {
        std::cout << std::left << std::setw(14) << r.particle
                  << std::setw(16) << r.model
                  << std::setw(14) << std::fixed << std::setprecision(2) << r.reducedChi2
                  << std::setw(12) << std::fixed << std::setprecision(4) << r.rmse
                  << std::setw(10) << r.nPoints << "\n";
    }
    std::cout << "========================================================================\n";
    std::cout << "Summary metadata exported to: " << filename << "\n";
}

void plotStoppingPowerValidation() {
    std::string configFile = "input/experimental_data/stopping_power.cfg";
    auto configs = LoadConfigsFromCFG(configFile);
    if (configs.empty()) {
        std::cerr << "No configuration entries loaded from " << configFile << "!" << std::endl;
        return;
    }

    std::vector<ValidationResult> allResults;
    for (const auto& cfg : configs) {
        std::cout << "\n========================================\n";
        std::cout << "Processing: " << cfg.section << " (" << cfg.particleLabel << ")\n";
        std::cout << "========================================\n";
        ProcessSingleValidation(cfg, allResults);
    }

    if (!allResults.empty()) {
        SaveValidationSummary(allResults, "output/plots/validation/stopping_power_validation_summary.csv");
    }
    std::cout << "\nAll Stopping Power validation plots generated and saved to 'output/plots/validation/'.\n";
}