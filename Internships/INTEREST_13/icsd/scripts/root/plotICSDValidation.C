// scripts/root/plotICSDValidation.C
// Execute under ROOT: root -l -q 'scripts/root/plotICSDValidation.C+'

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

struct ICSDConfig {
    std::string section;
    std::string name;
    std::string particleLabel;
    int Z = 0;
    int A = 0;
    double energy = 0.0;
    std::string energyUnit = "MeV";
    double xAxisMin = -0.5;
    double xAxisMax = 50.5;
    double yAxisMin = 1e-6;
    double yAxisMax = 1.0;
    double yAxisResMin = -1.5;
    double yAxisResMax = 1.5;
    double xTickLength = 0.015;
    double yTickLength = 0.015;
    double yResTickLength = 0.0193;
    double totalNExp = 1e4;
    double systemError = 0.00;
    double legX1 = 0.68;
    double legY1 = 0.50;
    double legX2 = 0.88;
    double legY2 = 0.88;
    std::string expDataFile = "";
    std::string dataAuthors = "Experimental Data";
    std::string dataType = "Experiment";
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
    Long64_t nSimEntries;
};

// Функция удаления лишних пробелов и табов по краям
std::string Trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

// Форматирование энергии (16.0 -> "16", 26.7 -> "26p7")
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

void SetIntegerAxisDivisions(TAxis* axis, double xMin, double xMax) {
    if (!axis) return;
    int range = static_cast<int>(std::round(xMax - xMin));
    if (range <= 0) return;

    if (range <= 15) {
        // Подписываются ВСЕ числа (0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11)
        axis->SetNdivisions(range, kFALSE);
    } 
    else if (range <= 20) {
        // 2. Li6, Li7 (0..23, 0..28):
        // Подписи каждые 5 единиц (0, 5, 10, 15, 20...)
        // n2 = 5 -> тики строго на КАЖДОМ целом числе (1, 2, 3, 4, 6, 7...)
        axis->SetNdivisions(210, kTRUE);
    } 
    else if (range <= 30) {
        // 2. Li6, Li7 (0..23, 0..28):
        // Подписи каждые 5 единиц (0, 5, 10, 15, 20...)
        // n2 = 5 -> тики строго на КАЖДОМ целом числе (1, 2, 3, 4, 6, 7...)
        axis->SetNdivisions(505, kTRUE);
    } 
    else {
        // 3. Alpha, C12 (0..42, 0..43, 0..50):
        // Подписи каждые 5 единиц (0, 5, 10, 15, 20, 25, 30, 35, 40...)
        // n2 = 5 -> тики строго на КАЖДОМ целом числе
        axis->SetNdivisions(510, kTRUE);
        
        // (Если хотите подписи только каждые 10 единиц: 0, 10, 20, 30, 40, 
        //  раскомментируйте строчку ниже вместо 510):
        // axis->SetNdivisions(1005, kTRUE);
    }
}

// Парсер конфигурационного файла в формате INI/CFG
std::vector<ICSDConfig> LoadICSDConfigsFromCFG(const std::string& filename) {
    std::vector<ICSDConfig> configs;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open configuration file: " << filename << std::endl;
        return configs;
    }

    std::string line;
    ICSDConfig currentCfg;
    bool hasSection = false;

    auto pushCurrent = [&]() {
        if (hasSection) {
            configs.push_back(currentCfg);
            currentCfg = ICSDConfig();
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
        else if (key == "x_min") currentCfg.xAxisMin = std::stod(val);
        else if (key == "x_max") currentCfg.xAxisMax = std::stod(val);
        else if (key == "y_min") currentCfg.yAxisMin = std::stod(val);
        else if (key == "y_max") currentCfg.yAxisMax = std::stod(val);
        else if (key == "y_res_min") currentCfg.yAxisResMin = std::stod(val);
        else if (key == "y_res_max") currentCfg.yAxisResMax = std::stod(val);
        else if (key == "total_n_exp") currentCfg.totalNExp = std::stod(val);
        else if (key == "syst_err") currentCfg.systemError = std::stod(val);
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

// Загрузка экспериментальных данных из CSV/TXT
std::vector<ExpPoint> LoadExperimentalData(const ICSDConfig& cfg) {
    std::vector<ExpPoint> points;
    if (cfg.expDataFile.empty() || cfg.expDataFile == "none") return points;

    std::ifstream file(cfg.expDataFile);
    if (!file.is_open()) {
        std::cout << "  [Warning] Experimental data file not found: " << cfg.expDataFile << std::endl;
        return points;
    }

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

    std::sort(points.begin(), points.end(), [](const ExpPoint& a, const ExpPoint& b) {
        return a.x < b.x;
    });
    std::cout << "  [Info] Loaded " << points.size() << " points from: " << cfg.expDataFile << std::endl;
    return points;
}

// Поиск ROOT файла симуляции
std::string FindRootFile(const std::string& particle, double energy, const std::string& energyUnit, const std::string& opt) {
    std::string eStr1 = FormatEnergy(energy);
    std::ostringstream ss;
    ss << energy;
    std::string eStr2 = ss.str();

    std::vector<std::string> candidates = {
        "output/root/ICSD_" + particle + "_" + eStr1 + "-" + energyUnit + "_" + opt + ".root",
        "output/root/ICSD_" + particle + "_" + eStr2 + "-" + energyUnit + "_" + opt + ".root",
        "root/ICSD_" + particle + "_" + eStr1 + "-" + energyUnit + "_" + opt + ".root",
        "root/ICSD_" + particle + "_" + eStr2 + "-" + energyUnit + "_" + opt + ".root"
    };

    for (const auto& path : candidates) {
        if (fs::exists(path)) return path;
    }
    return "";
}

void plotSingleICSD(const ICSDConfig& cfg, std::vector<ValidationResult>& allResults) {
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

    double xRangeMin = hasExpData ? std::floor(xData.front()) : cfg.xAxisMin;
double xRangeMax = hasExpData ? std::ceil(xData.back())   : cfg.xAxisMax;

    unsigned nbBins = static_cast<unsigned>(std::max(1.0, std::round(cfg.xAxisMax - cfg.xAxisMin)));
    double lineWidth = 1.5;

    const std::vector<PhysOpt> physOptions = {
        {"DNA2", "DNA2", "DNA Opt2", kRed},
        {"DNA4", "DNA4", "DNA Opt4", kBlue},
        {"DNA6", "DNA6", "DNA Opt6", kGreen + 2},
        {"DNA8", "DNA8", "DNA Opt8", kMagenta + 2}
    };

    std::string canvasIdentifier = "ICSD_" + cfg.name + "_" + FormatEnergy(cfg.energy) + cfg.energyUnit;
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
    pad1->Draw();

    pad2->SetTopMargin(0);
    pad2->SetBottomMargin(0.3);
    pad2->SetTickx(1);
    pad2->SetTicky(1);
    pad2->Draw();

    TH1F* hFrame = new TH1F("hFrame", "", nbBins, cfg.xAxisMin, cfg.xAxisMax);
    TAxis* XaxisRes = hFrame->GetXaxis();
    TAxis* YaxisRes = hFrame->GetYaxis();

    // 1. Отрисовка базовой рамки Pad 1
    pad1->cd();
    TH1F* hFrame1 = new TH1F("hFrame1", "", nbBins, cfg.xAxisMin, cfg.xAxisMax);
    hFrame1->SetStats(0);
    TAxis* xAxis = hFrame1->GetXaxis();
    TAxis* yAxis = hFrame1->GetYaxis();
    hFrame1->Draw("AXIS");

    xAxis->SetLimits(xRangeMin, xRangeMax);
    xAxis->SetRangeUser(xRangeMin, xRangeMax);
    xAxis->SetLabelSize(0);
    xAxis->SetTitleSize(0);
    xAxis->SetTickLength(2.95*cfg.yTickLength);
    SetIntegerAxisDivisions(xAxis, xRangeMin, xRangeMax);

    yAxis->SetRangeUser(cfg.yAxisMin, cfg.yAxisMax);
    yAxis->ChangeLabelByValue(cfg.yAxisMin, -1, -1, -1, -1, -1, " ");
    yAxis->ChangeLabelByValue(1.0, -1, -1, -1, -1, -1, "10^{0}");
    yAxis->SetTitle("Frequency");
    yAxis->SetTickLength(cfg.yTickLength);
    yAxis->SetTitleSize(0.07);
    yAxis->SetTitleOffset(0.40);
    yAxis->CenterTitle(true);

    pad1->Update();
    pad1->RedrawAxis();

    // 2. Серая область погрешности эксперимента на Pad 1 (Рисуется ДО линий симуляции)
    TGraphAsymmErrors* expBand = nullptr;
    if (hasExpData) {
        expBand = new TGraphAsymmErrors(n);
        for (size_t i = 0; i < n; ++i) {
            double statErr = (cfg.totalNExp > 0) ? TMath::Sqrt(yData[i] / cfg.totalNExp) : 0.0;
            double systErr = cfg.systemError * yData[i];
            double totalErr = TMath::Sqrt(statErr * statErr + systErr * systErr);

            double yLow = TMath::Max(yData[i] - totalErr, cfg.yAxisMin * 1.001);
            double errLow = yData[i] - yLow;

            expBand->SetPoint(i, xData[i], yData[i]);
            expBand->SetPointError(i, 0.5, 0.5, errLow, totalErr);
        }
        expBand->SetFillColorAlpha(kGray, 0.35);
        expBand->SetFillStyle(1001);
        expBand->SetLineWidth(0);
        expBand->Draw("E3 SAME");
    }

    // Векторы для хранения отрисованных симуляций (для передачи в легенду)
    std::vector<TGraphAsymmErrors*> loadedSimGraphs;
    std::vector<PhysOpt> loadedPhysOpts;

    int counter = 0;

    for (const auto& phys : physOptions) {
        std::string filePath = FindRootFile(cfg.name, cfg.energy, cfg.energyUnit, phys.fileSuffix);
        if (filePath.empty()) {
            std::cout << "  [Warning] File not found for: " << phys.name << std::endl;
            continue;
        }

        TFile* file = TFile::Open(filePath.c_str(), "READ");
        if (!file || file->IsZombie()) {
            std::cout << "  [Error] Failed to open: " << filePath << std::endl;
            if (file) delete file;
            continue;
        }

        TTree* tree = (TTree*)file->Get("ntuple_1");
        if (!tree) {
            std::cout << "  [Error] Tree 'ntuple_1' not found in: " << filePath << std::endl;
            file->Close();
            delete file;
            continue;
        }

        pad1->cd();
        double ion = 0;
        tree->SetBranchAddress("ionisations", &ion);

        TH1F* hist = new TH1F(Form("h_%s_%s", cfg.name.c_str(), phys.name.c_str()), "", nbBins, cfg.xAxisMin, cfg.xAxisMax);
        hist->Sumw2();

        Long64_t nbEntries = tree->GetEntries();
        for (Long64_t i = 0; i < nbEntries; ++i) {
            tree->GetEntry(i);
            hist->Fill(ion);
        }

        double normFactor = hist->Integral();
        if (normFactor > 0) hist->Scale(1.0 / normFactor);

        TGraphAsymmErrors* graph = new TGraphAsymmErrors();

        // Исключаем точки с нулевыми событиями (устраняем zero-drop артефакт)
        for (unsigned j = 1; j <= nbBins; ++j) {
            double center = hist->GetBinCenter(j);
            double content = hist->GetBinContent(j);
            double error = hist->GetBinError(j);
            if (content > cfg.yAxisMin * 0.1) {
                int nPt = graph->GetN();
                graph->SetPoint(nPt, center, content);
                graph->SetPointError(nPt, 0, 0, error, error);
            }
        }

        graph->SetLineWidth(lineWidth);
        graph->SetLineColor(phys.color);
        graph->Draw("L SAME");

        loadedSimGraphs.push_back(graph);
        loadedPhysOpts.push_back(phys);

        pad2->cd();
        if (hasExpData) {
            std::vector<double> res(n, 0.0);
            double rmse = 0.0;
            double chi2Sum = 0.0;
            double totalNSim = static_cast<double>(nbEntries);

            for (size_t i = 0; i < n; ++i) {
                double xExp = xData[i];
                double yExp = yData[i];
                int binSim = hist->FindBin(xExp);
                double simVal = hist->GetBinContent(binSim);
                double simErr = hist->GetBinError(binSim);

                res[i] = (yExp != 0.0) ? (yExp - simVal) / yExp : 0.0;
                rmse += TMath::Power(res[i], 2);

                double varSim = (totalNSim > 0) ? (simVal / totalNSim) : 0.0;
                double varExp = (cfg.totalNExp > 0) ? (yExp / cfg.totalNExp) : 0.0;
                double varSys = TMath::Power(cfg.systemError * yExp, 2);
                double totalVar = varSim + varExp + varSys;

                if (totalVar > 0) {
                    chi2Sum += TMath::Power(yExp - simVal, 2) / totalVar;
                }
            }

            rmse = TMath::Sqrt(rmse / n);
            double normChi2 = (n > 1) ? (chi2Sum / (n - 1)) : 0.0;

            allResults.push_back({cfg.name, cfg.energy, cfg.energyUnit, phys.legend, normChi2, rmse, nbEntries});

            if (counter == 0) {
                hFrame->SetStats(0);
                hFrame->Draw("AXIS");

                XaxisRes->SetLimits(xRangeMin, xRangeMax);
                XaxisRes->SetRangeUser(xRangeMin, xRangeMax);
                XaxisRes->SetTitle("Ionisation number");
                XaxisRes->SetTitleSize(0.155);
                XaxisRes->SetLabelSize(0.09);
                XaxisRes->CenterTitle(true);
                XaxisRes->SetTickSize(6.95*cfg.yTickLength);
                XaxisRes->SetTitleOffset(0.66);
                SetIntegerAxisDivisions(XaxisRes, xRangeMin, xRangeMax);

                YaxisRes->SetRangeUser(cfg.yAxisResMin, cfg.yAxisResMax);
                YaxisRes->SetTitle("#it{#eta} = [1 - Geant4/Ref]");
                YaxisRes->SetTickLength(cfg.yResTickLength);
                YaxisRes->SetTitleSize(0.082);
                YaxisRes->SetTitleOffset(0.35);
                YaxisRes->SetLabelSize(0.07);
                YaxisRes->CenterTitle(true);
                YaxisRes->SetNdivisions(305);

                TGraphErrors* errCorridor = new TGraphErrors(n);

                for (size_t i = 0; i < n; ++i) {
                    double x = xData[i];
                    double y = yData[i];
                    double relStatErr = (cfg.totalNExp > 0 && y > 0) ? (TMath::Sqrt(y / cfg.totalNExp) / y) : 0.0;
                    double relSystErr = cfg.systemError;
                    double totalRelErr = TMath::Sqrt(relStatErr * relStatErr + relSystErr * relSystErr);

                    errCorridor->SetPoint(i, x, 0.0);
                    errCorridor->SetPointError(i, 0.5, totalRelErr);
                }

                errCorridor->SetFillColorAlpha(kGray, 0.4);
                errCorridor->SetFillStyle(1001);
                errCorridor->SetLineWidth(0);
                errCorridor->Draw("E3 SAME");

                TLine* zeroLine = new TLine(xRangeMin, 0.0, xRangeMax, 0.0);
                zeroLine->SetLineStyle(2);
                zeroLine->Draw("SAME");

                //pad2->RedrawAxis();
            }


            TGraph* splineRes = new TGraph(n, xData.data(), res.data());
            splineRes->SetLineWidth(lineWidth);
            splineRes->SetLineColor(phys.color);
            splineRes->Draw("L SAME");

        } else {
            if (counter == 0) {
                hFrame->SetStats(0);
                hFrame->Draw("AXIS");
                XaxisRes->SetLimits(xRangeMin, xRangeMax);
                XaxisRes->SetRangeUser(xRangeMin, xRangeMax);
                XaxisRes->SetTitle("Ionisation number");
                XaxisRes->SetTitleSize(0.135);
                XaxisRes->SetLabelSize(0.09);
                XaxisRes->CenterTitle(true);

                YaxisRes->SetRangeUser(cfg.yAxisResMin, cfg.yAxisResMax);
                YaxisRes->SetTitle("#it{#eta} = [1 - Geant4/Ref]");
                YaxisRes->SetTitleSize(0.082);
                YaxisRes->SetTitleOffset(0.35);
                YaxisRes->SetLabelSize(0.07);
                YaxisRes->CenterTitle(true);
                pad2->RedrawAxis();
            }
        }

        counter++;
        file->Close();
        delete file;
    }

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
            double statErr = (cfg.totalNExp > 0) ? TMath::Sqrt(y / cfg.totalNExp) : 0.0;
            double systErr = cfg.systemError * y;
            double totalErr = TMath::Sqrt(statErr * statErr + systErr * systErr);

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

    // ЭКСПЕРИМЕНТ — НА 1-Е МЕСТО В ЛЕГЕНДЕ
    if (hasExpData && graphExp) {
        // 1. Измеряем точную физическую ширину обеих строк через движок ROOT
        TText metric;
        UInt_t w1 = 0, h1 = 0;
        UInt_t w2 = 0, h2 = 0;

        // Верхняя строка: жирный шрифт (font 62), масштаб 0.9
        metric.SetTextFont(62);
        metric.SetTextSize(legText->GetTextSize() * 0.9);
        metric.GetTextExtent(w1, h1, cfg.dataAuthors.c_str());

        // Нижняя строка: обычный шрифт (font 42), масштаб 0.75
        metric.SetTextFont(42);
        metric.SetTextSize(legText->GetTextSize() * 0.75);
        metric.GetTextExtent(w2, h2, cfg.dataType.c_str());

        // 2. Рассчитываем точный фактор сдвига
        double dx = 0.0;
        if (w2 > 0 && w1 > w2) {
            // --- ВАРИАНТ А: Выравнивание ПО ПРАВОМУ КРАЮ ---
            dx = (double)(w1 - w2) / (double)w2;

            // --- ВАРИАНТ Б: Выравнивание ПО ЦЕНТРУ (раскомментируйте, если нужно по центру) ---
            // dx = 0.5 * (double)(w1 - w2) / (double)w2;
        }

        // 3. Формируем строку с точным пиксельным сдвигом
        std::string expLabel = Form(
            "#splitline{#scale[0.9]{#bf{%s}}}{#kern[%.3f]{#scale[0.75]{#color[12]{%s}}}}", 
            cfg.dataAuthors.c_str(), 
            dx,
            cfg.dataType.c_str()
        );

        legText->AddEntry((TObject*)nullptr, expLabel.c_str(), "");
        legIcons->AddEntry(graphExp, "", "pe");
    }

    // 2. МОДЕЛИ СИМУЛЯЦИИ — СЛЕДУЮЩИМИ СТРОКАМИ
    for (size_t i = 0; i < loadedSimGraphs.size(); ++i) {
        std::string modelLabel = Form("#scale[0.9]{#bf{%s}}", loadedPhysOpts[i].legend.c_str());
        legText->AddEntry((TObject*)nullptr, modelLabel.c_str(), "");
        legIcons->AddEntry(loadedSimGraphs[i], "", "l");
    }
    legText->Draw();
    legIcons->Draw();

    pad1->RedrawAxis();
    mainCanvas->Update();

    fs::create_directories("output/plots/validation");
    std::string outputPath = "output/plots/validation/ICSD_" + cfg.name + "_" + FormatEnergy(cfg.energy) + cfg.energyUnit + ".svg";
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

    out << "Particle,Energy,Unit,PhysicsModel,ReducedChi2,RMSE,SimEvents\n";
    for (const auto& r : results) {
        out << r.particle << ","
            << r.energy << ","
            << r.energyUnit << ","
            << "\"" << r.model << "\","
            << std::fixed << std::setprecision(4) << r.reducedChi2 << ","
            << std::fixed << std::setprecision(6) << r.rmse << ","
            << r.nSimEntries << "\n";
    }
    out.close();

    std::cout << "\n========================================================================\n";
    std::cout << "                      ICSD VALIDATION SUMMARY REPORT                     \n";
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
                  << std::setw(12) << r.nSimEntries << "\n";
    }
    std::cout << "========================================================================\n";
    std::cout << "Summary metadata exported to: " << filename << "\n";
}

void plotICSDValidation() {
    auto configs = LoadICSDConfigsFromCFG("input/experimental_data/icsd_particles.cfg");
    if (configs.empty()) {
        std::cerr << "No configuration entries loaded from input/experimental_data/icsd_particles.cfg!" << std::endl;
        return;
    }

    std::vector<ValidationResult> allResults;
    for (const auto& cfg : configs) {
        std::cout << "\n========================================\n";
        std::cout << "Processing ICSD: " << cfg.name << " (" << cfg.energy << " " << cfg.energyUnit << ")\n";
        std::cout << "========================================\n";
        plotSingleICSD(cfg, allResults);
    }

    if (!allResults.empty()) {
        SaveValidationSummary(allResults, "output/plots/validation/icsd_validation_summary.csv");
    }
    std::cout << "\nAll ICSD validation plots generated and saved to 'output/plots/validation/'.\n";
}