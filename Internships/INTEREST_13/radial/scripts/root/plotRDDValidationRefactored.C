// scripts/root/plotRDDValidation.C
// Execute under ROOT: root -l -q 'scripts/root/plotRDDValidation.C+'

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <filesystem>
#include <cmath>
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
#include <TText.h>
#include <TStyle.h>
#include <TMath.h>
#include <TString.h>

namespace fs = std::filesystem;

struct AxisBound { bool isExp = true; double val = 0.0; };

struct RDDConfig {
    std::string section, name, particleLabel, energyUnit = "MeV";
    int Z = 0, A = 0;
    double energy = 0.0;
    AxisBound xAxisMin, xAxisMax, yAxisMin, yAxisMax;
    double yAxisResMin = -1.5, yAxisResMax = 1.5;
    double xTickLength = 0.015, yTickLength = 0.015, yResTickLength = 0.0193;
    bool isLogX = false;
    std::string expRelErrType = "threshold", expDataFile = "", dataAuthors = "Experimental Data", dataType = "Experiment";
    double legX1 = 0.68, legY1 = 0.50, legX2 = 0.88, legY2 = 0.88;
};

struct ExpPoint { double x, y; };
struct PhysOpt { std::string name, fileSuffix, legend; Int_t color; };
struct ValidationResult { std::string particle; double energy; std::string energyUnit, model; double reducedChi2, rmse; Int_t nEvents; };

// Вспомогательные функции форматирования
std::string Trim(const std::string& str) {
    auto f = str.find_first_not_of(" \t\r\n"), l = str.find_last_not_of(" \t\r\n");
    return (f == std::string::npos) ? "" : str.substr(f, l - f + 1);
}

std::string FormatEnergy(double energy) {
    if (std::abs(energy - std::round(energy)) < 1e-4) return std::to_string(static_cast<long long>(std::round(energy)));
    std::ostringstream ss; ss << energy;
    std::string s = ss.str();
    if (auto p = s.find('.'); p != std::string::npos) s.replace(p, 1, "p");
    return s;
}

AxisBound ParseBound(const std::string& token) {
    std::string s = Trim(token);
    if (s == "exp" || s == "EXP" || s == "auto" || s.empty()) return {true, 0.0};
    try { return {false, std::stod(s)}; } catch (...) { return {true, 0.0}; }
}

// Загрузка конфигурации CFG/INI
std::vector<RDDConfig> LoadRDDConfigsFromCFG(const std::string& filename) {
    std::vector<RDDConfig> configs;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open config: " << filename << std::endl;
        return configs;
    }

    std::string line;
    RDDConfig cur;
    bool hasSec = false;

    auto push = [&]() { if (hasSec) { configs.push_back(cur); cur = RDDConfig(); hasSec = false; } };

    while (std::getline(file, line)) {
        line = Trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;
        if (line.front() == '[' && line.back() == ']') {
            push();
            cur.section = Trim(line.substr(1, line.size() - 2));
            hasSec = true;
            continue;
        }
        auto eq = line.find('=');
        if (eq == std::string::npos || !hasSec) continue;

        std::string k = Trim(line.substr(0, eq)), v = Trim(line.substr(eq + 1));
        if (k == "name") cur.name = v;
        else if (k == "label") cur.particleLabel = v;
        else if (k == "Z") cur.Z = std::stoi(v);
        else if (k == "A") cur.A = std::stoi(v);
        else if (k == "energy") cur.energy = std::stod(v);
        else if (k == "unit") cur.energyUnit = v;
        else if (k == "x_min") cur.xAxisMin = ParseBound(v);
        else if (k == "x_max") cur.xAxisMax = ParseBound(v);
        else if (k == "y_min") cur.yAxisMin = ParseBound(v);
        else if (k == "y_max") cur.yAxisMax = ParseBound(v);
        else if (k == "y_res_min") cur.yAxisResMin = std::stod(v);
        else if (k == "y_res_max") cur.yAxisResMax = std::stod(v);
        else if (k == "is_log_x") cur.isLogX = (std::stoi(v) != 0);
        else if (k == "err_type") cur.expRelErrType = v;
        else if (k == "data_file") cur.expDataFile = v;
        else if (k == "authors") cur.dataAuthors = v;
        else if (k == "data_type") cur.dataType = v;
        else if (k == "legend_pos") {
            std::replace(v.begin(), v.end(), ',', ' ');
            std::stringstream(v) >> cur.legX1 >> cur.legY1 >> cur.legX2 >> cur.legY2;
        }
    }
    push();
    return configs;
}

// Загрузка экспериментальных точек
std::vector<ExpPoint> LoadExperimentalData(const RDDConfig& cfg) {
    std::vector<ExpPoint> pts;
    if (cfg.expDataFile.empty() || cfg.expDataFile == "none") return pts;

    std::ifstream file(cfg.expDataFile);
    if (!file.is_open()) return pts;

    std::string line;
    while (std::getline(file, line)) {
        line = Trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;
        std::replace(line.begin(), line.end(), ',', ' ');
        std::stringstream ss(line);
        double x = 0.0, y = 0.0;
        if (ss >> x >> y) pts.push_back({x, y});
    }
    std::sort(pts.begin(), pts.end(), [](const auto& a, const auto& b) { return a.x < b.x; });
    std::cout << "  [Info] Loaded " << pts.size() << " points from: " << cfg.expDataFile << std::endl;
    return pts;
}

// Относительная погрешность эксперимента по статьям
double GetExpRelErr(double x, const RDDConfig& cfg) {
    if (cfg.expRelErrType == "fixed0.10") return 0.10;
    if (cfg.expRelErrType == "fixed0.15") return 0.15;
    if (cfg.expRelErrType == "fixed0.20") return 0.20;
    if (cfg.name == "alpha" && std::abs(cfg.energy - 1.0) < 0.1) return (x < 5.0) ? 0.50 : 0.35;
    return (x < 5.0) ? 0.40 : 0.20;
}

// Фильтрация воркеров Geant4 MT с помощью std::filesystem (C++17)
int AddFilteredFilesToChain(TChain* chain, const fs::path& dirPath, const std::string& searchPrefix) {
    if (!fs::exists(dirPath)) return 0;
    int count = 0;
    for (const auto& entry : fs::directory_iterator(dirPath)) {
        if (!entry.is_regular_file()) continue;
        auto fname = entry.path().filename().string();
        if (fname.size() < 5 || fname.rfind(".root") != fname.size() - 5) continue;

        bool isWorker = false;
        for (int t = 0; t < 64; ++t) {
            if (fname.find("_t" + std::to_string(t) + ".root") != std::string::npos ||
                fname.find("_t" + std::to_string(t) + "_") != std::string::npos) {
                isWorker = true;
                break;
            }
        }
        if (isWorker) continue;

        if (fname.find(searchPrefix) != std::string::npos) {
            chain->Add(entry.path().string().c_str());
            count++;
        }
    }
    return count;
}

void plotSingleRDD(const RDDConfig& cfg, std::vector<ValidationResult>& allResults) {
    TH1::AddDirectory(kFALSE);
    gStyle->SetCanvasPreferGL(kTRUE);
    gStyle->SetPadBorderSize(0);

    auto expPoints = LoadExperimentalData(cfg);
    bool hasExp = !expPoints.empty();
    size_t n = expPoints.size();

    std::vector<double> xData(n), yData(n);
    for (size_t i = 0; i < n; ++i) { xData[i] = expPoints[i].x; yData[i] = expPoints[i].y; }

    // Границы осей: X - строго min..max эксперимента
    double expXMin = hasExp ? xData.front() : 0.1, expXMax = hasExp ? xData.back() : 100.0;
    double expYMin = hasExp ? *std::min_element(yData.begin(), yData.end()) : 1.0;
    double expYMax = hasExp ? *std::max_element(yData.begin(), yData.end()) : 1e7;

    double xAxisMin = cfg.xAxisMin.isExp ? expXMin : cfg.xAxisMin.val;
    double xAxisMax = cfg.xAxisMax.isExp ? expXMax : cfg.xAxisMax.val;
    double yAxisMin = cfg.yAxisMin.isExp ? (expYMin * 0.5) : cfg.yAxisMin.val;
    double yAxisMax = cfg.yAxisMax.isExp ? (expYMax * 2.0) : cfg.yAxisMax.val;

    const std::vector<PhysOpt> physOptions = {
        {"G4EmDNAPhysics_option2", "DNA2", "DNA Opt2", kRed},
        {"G4EmDNAPhysics_option4", "DNA4", "DNA Opt4", kBlue},
        {"G4EmDNAPhysics_option6", "DNA6", "DNA Opt6", kGreen + 2},
        {"G4EmDNAPhysics_option8", "DNA8", "DNA Opt8", kMagenta + 2}
    };

    auto mainCanvas = new TCanvas("mainCanvas", ("RDD_" + cfg.name + "_" + FormatEnergy(cfg.energy)).c_str(), 1920, 1080);
    auto pad1 = new TPad("pad1", "main plot", 0.0, 0.3, 1.0, 1.0);
    auto pad2 = new TPad("pad2", "residuals", 0.0, 0.0, 1.0, 0.3);

    for (auto* p : {pad1, pad2}) {
        p->SetLeftMargin(0.10); p->SetRightMargin(0.10);
        p->SetTickx(1); p->SetTicky(1);
        if (cfg.isLogX) p->SetLogx();
        p->Draw();
    }
    pad1->SetLogy(); pad1->SetBottomMargin(0);
    pad2->SetTopMargin(0); pad2->SetBottomMargin(0.3);

    // 1. Отрисовка базовой рамки Pad 1
    pad1->cd();
    auto hFrame1 = new TH1F("hFrame1", "", 1, xAxisMin, xAxisMax);
    hFrame1->SetStats(0);
    hFrame1->Draw("AXIS");
    hFrame1->GetXaxis()->SetLimits(xAxisMin, xAxisMax);
    hFrame1->GetXaxis()->SetRangeUser(xAxisMin, xAxisMax);
    hFrame1->GetXaxis()->SetLabelSize(0);
    hFrame1->GetXaxis()->SetTickLength(2.95 * cfg.yTickLength);

    auto yAxis = hFrame1->GetYaxis();
    yAxis->SetRangeUser(yAxisMin, yAxisMax);
    if (cfg.isLogX) {
        yAxis->SetMoreLogLabels(kFALSE);
        pad1->Update();
        for (int i = -20; i < 20; ++i) if (std::abs(i % 2) == 1) yAxis->ChangeLabelByValue(std::pow(10., i), -1, 0, -1, -1, -1, "");
        yAxis->ChangeLabelByValue(1.0, -1, -1, -1, -1, -1, "10^{0}");
        pad1->Modified();
    }
    yAxis->SetTitle("Dose [Gy]");
    yAxis->SetTickLength(cfg.yTickLength);
    yAxis->SetTitleSize(0.07);
    yAxis->SetTitleOffset(0.40);
    yAxis->CenterTitle(true);
    pad1->Update(); pad1->RedrawAxis();

    // 2. Серая область погрешности эксперимента на Pad 1
    if (hasExp) {
        auto expBand = new TGraphAsymmErrors(static_cast<int>(n));
        for (size_t i = 0; i < n; ++i) {
            double totErr = GetExpRelErr(xData[i], cfg) * yData[i];
            double yLow = std::max(yData[i] - totErr, yAxisMin * 1.001);
            expBand->SetPoint(i, xData[i], yData[i]);
            expBand->SetPointError(i, 0.0, 0.0, yData[i] - yLow, totErr);
        }
        expBand->SetFillColorAlpha(kGray, 0.35);
        expBand->SetFillStyle(1001);
        expBand->SetLineWidth(0);
        expBand->Draw("E3 SAME");
    }

    std::vector<TGraphAsymmErrors*> loadedSimGraphs;
    std::vector<PhysOpt> loadedPhysOpts;
    auto hFrame = new TH1F("hFrame", "", 1, xAxisMin, xAxisMax);
    int counter = 0;

    for (const auto& phys : physOptions) {
        auto tree = new TChain("radial");
        auto treeLET = new TChain("LET");
        std::string searchPrefix = Form("radial_ion-%d-%d_%s-%s_%s_", cfg.Z, cfg.A, FormatEnergy(cfg.energy).c_str(), cfg.energyUnit.c_str(), phys.name.c_str());

        if (AddFilteredFilesToChain(tree, "output/root", searchPrefix) == 0 || tree->GetEntries() == 0) {
            std::cout << "  [Warning] No files found for: " << phys.legend << std::endl;
            delete tree; delete treeLET;
            continue;
        }
        AddFilteredFilesToChain(treeLET, "output/root", searchPrefix);

        double dose = 0, radius = 0, rmseDose = 0;
        Int_t nbEvents = 0;
        tree->SetBranchAddress("dose", &dose);
        tree->SetBranchAddress("radius", &radius);
        tree->SetBranchAddress("rmse", &rmseDose);
        if (treeLET->GetEntries() > 0) {
            treeLET->SetBranchAddress("events", &nbEvents);
            treeLET->GetEntry(0);
        }

        pad1->cd();
        auto graph = new TGraphAsymmErrors();
        Long64_t nbEntries = tree->GetEntries();
        size_t xCounter = 0;
        tree->GetEntry(0);
        double prevR = radius, prevD = dose, prevRMSE = rmseDose;
        graph->SetPoint(0, radius, dose);

        for (Long64_t j = 1; j < nbEntries; ++j) {
            tree->GetEntry(j);
            if (hasExp) {
                while (xCounter < n && xData[xCounter] <= radius) {
                    if (xData[xCounter] >= prevR) {
                        double t = (radius == prevR) ? 0 : (xData[xCounter] - prevR) / (radius - prevR);
                        int idx = graph->GetN();
                        graph->SetPoint(idx, xData[xCounter], prevD + t * (dose - prevD));
                        graph->SetPointError(idx, 0, 0, prevRMSE + t * (rmseDose - prevRMSE), prevRMSE + t * (rmseDose - prevRMSE));
                    }
                    xCounter++;
                }
            }
            int idx = graph->GetN();
            graph->SetPoint(idx, radius, dose);
            prevR = radius; prevD = dose; prevRMSE = rmseDose;
        }

        graph->SetLineWidth(1);
        graph->SetLineColor(phys.color);
        graph->Draw("L SAME");
        loadedSimGraphs.push_back(graph);
        loadedPhysOpts.push_back(phys);

        // Pad 2: Residuals и Chi2
        pad2->cd();
        if (hasExp) {
            std::vector<double> res(n, 0.0);
            double sumChi2 = 0.0, rmse = 0.0;

            for (size_t i = 0; i < n; ++i) {
                double xExp = xData[i], yExp = yData[i], simVal = graph->Eval(xExp), simRMSE = 0.0;
                for (int k = 0; k < graph->GetN(); ++k) {
                    if (std::abs(graph->GetX()[k] - xExp) < 1e-7) { simRMSE = graph->GetErrorYhigh(k); break; }
                }
                double simErr = (nbEvents > 0) ? (simRMSE / std::sqrt(nbEvents)) : 0.0;
                double sigExp = GetExpRelErr(xExp, cfg) * yExp;
                double totSig2 = sigExp * sigExp + simErr * simErr;

                if (totSig2 > 0.0) sumChi2 += std::pow(yExp - simVal, 2) / totSig2;
                res[i] = (yExp != 0.0) ? (yExp - simVal) / yExp : 0.0;
                rmse += std::pow(res[i], 2);
            }
            rmse = std::sqrt(rmse / n);
            allResults.push_back({cfg.name, cfg.energy, cfg.energyUnit, phys.legend, (n > 0 ? sumChi2 / n : 0.0), rmse, nbEvents});

            if (counter == 0) {
                hFrame->SetStats(0); hFrame->Draw("AXIS");
                auto Xres = hFrame->GetXaxis(); auto Yres = hFrame->GetYaxis();
                Xres->SetLimits(xAxisMin, xAxisMax); Xres->SetRangeUser(xAxisMin, xAxisMax);
                Xres->SetTitle("Distance from the axis [nm]"); Xres->SetTitleSize(0.155); Xres->SetLabelSize(0.09);
                Xres->CenterTitle(true); Xres->SetTickLength(0.07); Xres->SetTitleOffset(0.88);

                Yres->SetRangeUser(cfg.yAxisResMin, cfg.yAxisResMax);
                Yres->SetTitle("#it{#eta} = [1 - Geant4/Ref]"); Yres->SetTitleSize(0.082); Yres->SetTitleOffset(0.30);
                Yres->SetLabelSize(0.07); Yres->SetLabelOffset(0.008); Yres->SetTickLength(cfg.yResTickLength);
                Yres->SetNdivisions(305); Yres->CenterTitle(true);

                auto errCorridor = new TGraphErrors(static_cast<int>(n + 2));
                errCorridor->SetPoint(0, xAxisMin, 0.0);
                errCorridor->SetPointError(0, 0.5, GetExpRelErr(xAxisMin, cfg));
                for (size_t i = 0; i < n; ++i) {
                    errCorridor->SetPoint(i + 1, xData[i], 0.0);
                    errCorridor->SetPointError(i + 1, 0.5, GetExpRelErr(xData[i], cfg));
                }
                errCorridor->SetPoint(n + 1, xAxisMax, 0.0);
                errCorridor->SetPointError(n + 1, 0.5, GetExpRelErr(xAxisMax, cfg));
                errCorridor->SetFillColorAlpha(kGray, 0.4); errCorridor->SetFillStyle(1001); errCorridor->SetLineWidth(0);
                errCorridor->Draw("E3 SAME");

                auto zeroLine = new TLine(xAxisMin, 0.0, xAxisMax, 0.0);
                zeroLine->SetLineStyle(2); zeroLine->Draw("SAME");
            }

            auto splineRes = new TGraph(static_cast<int>(n), xData.data(), res.data());
            splineRes->SetLineWidth(1); splineRes->SetLineColor(phys.color);
            splineRes->Draw("L SAME");
        }
        counter++;
        delete tree; delete treeLET;
    }

    pad2->cd();
    hFrame->Draw("SAME AXIS");
    pad2->RedrawAxis(); pad2->Update();

    // 3. Точки эксперимента на Pad 1
    TGraphAsymmErrors* graphExp = nullptr;
    if (hasExp) {
        pad1->cd();
        graphExp = new TGraphAsymmErrors(static_cast<int>(n));
        for (size_t i = 0; i < n; ++i) {
            double totErr = GetExpRelErr(xData[i], cfg) * yData[i];
            graphExp->SetPoint(i, xData[i], yData[i]);
            graphExp->SetPointError(i, 0, 0, totErr, totErr);
        }
        graphExp->SetMarkerStyle(25); graphExp->SetMarkerSize(3); graphExp->SetMarkerColor(kBlack);
        graphExp->SetLineColor(kBlack); graphExp->SetLineWidth(1);
        graphExp->Draw("P E SAME");
    }

    // 4. Двойная легенда со сдвигом через GetTextExtent
    pad1->cd();
    double iconW = 0.05;
    auto legText = new TLegend(cfg.legX1, cfg.legY1, cfg.legX2 - iconW, cfg.legY2);
    auto legIcons = new TLegend(cfg.legX2 - iconW, cfg.legY1, cfg.legX2, cfg.legY2);

    for (auto* l : {legText, legIcons}) {
        l->SetBorderSize(0); l->SetFillStyle(0); l->SetEntrySeparation(0.45);
    }
    legText->SetTextSize(0.035); legText->SetTextAlign(32); legText->SetMargin(0.0);
    legIcons->SetMargin(1.0);

    if (hasExp && graphExp) {
        TText metric; UInt_t w1 = 0, h1 = 0, w2 = 0, h2 = 0;
        metric.SetTextFont(62); metric.SetTextSize(legText->GetTextSize() * 0.9);
        metric.GetTextExtent(w1, h1, cfg.dataAuthors.c_str());
        metric.SetTextFont(42); metric.SetTextSize(legText->GetTextSize() * 0.75);
        metric.GetTextExtent(w2, h2, cfg.dataType.c_str());

        double dx = (w2 > 0 && w1 > w2) ? static_cast<double>(w1 - w2) / w2 : 0.0;
        std::string expLabel = Form("#splitline{#scale[0.9]{#bf{%s}}}{#kern[%.3f]{#scale[0.75]{#color[12]{%s}}}}",
                                    cfg.dataAuthors.c_str(), dx, cfg.dataType.c_str());
        legText->AddEntry(static_cast<TObject*>(nullptr), expLabel.c_str(), "");
        legIcons->AddEntry(graphExp, "", "pe");
    }

    for (size_t i = 0; i < loadedSimGraphs.size(); ++i) {
        legText->AddEntry(static_cast<TObject*>(nullptr), Form("#scale[0.9]{#bf{%s}}", loadedPhysOpts[i].legend.c_str()), "");
        legIcons->AddEntry(loadedSimGraphs[i], "", "l");
    }
    legText->Draw(); legIcons->Draw();

    pad1->RedrawAxis();
    mainCanvas->Update();

    fs::create_directories("output/plots/validation");
    std::string outputPath = "output/plots/validation/RDD_" + cfg.name + "_" + FormatEnergy(cfg.energy) + cfg.energyUnit + ".svg";
    mainCanvas->SaveAs(outputPath.c_str());
    delete mainCanvas;
    std::cout << "  [Saved] Plot: " << outputPath << std::endl;
}

void SaveRDDValidationSummary(const std::vector<ValidationResult>& results, const std::string& filename) {
    std::ofstream out(filename);
    if (!out.is_open()) return;

    out << "Particle,Energy,Unit,PhysicsModel,ReducedChi2,RMSE,SimEvents\n";
    for (const auto& r : results) {
        out << r.particle << "," << r.energy << "," << r.energyUnit << ",\"" << r.model << "\","
            << std::fixed << std::setprecision(4) << r.reducedChi2 << ","
            << std::fixed << std::setprecision(6) << r.rmse << "," << r.nEvents << "\n";
    }
    out.close();

    std::cout << "\n========================================================================\n"
              << "                      RDD VALIDATION SUMMARY REPORT                      \n"
              << "========================================================================\n"
              << std::left << std::setw(12) << "Particle" << std::setw(14) << "Energy"
              << std::setw(16) << "Model" << std::setw(14) << "Red. Chi2"
              << std::setw(12) << "RMSE" << std::setw(12) << "SimEvents\n"
              << "------------------------------------------------------------------------\n";
    for (const auto& r : results) {
        std::cout << std::left << std::setw(12) << r.particle
                  << std::setw(14) << (std::to_string(r.energy) + " " + r.energyUnit)
                  << std::setw(16) << r.model
                  << std::setw(14) << std::fixed << std::setprecision(2) << r.reducedChi2
                  << std::setw(12) << std::fixed << std::setprecision(4) << r.rmse
                  << std::setw(12) << r.nEvents << "\n";
    }
    std::cout << "========================================================================\n"
              << "Summary exported to: " << filename << "\n";
}

void plotRDDValidationRefactored() {
    auto configs = LoadRDDConfigsFromCFG("input/experimental_data/rdd_particles.cfg");
    if (configs.empty()) {
        std::cerr << "No configuration entries loaded!" << std::endl;
        return;
    }
    std::vector<ValidationResult> allResults;
    for (const auto& cfg : configs) {
        std::cout << "\n========================================\n"
                  << "Processing RDD: " << cfg.name << " (" << cfg.energy << " " << cfg.energyUnit << ")\n"
                  << "========================================\n";
        plotSingleRDD(cfg, allResults);
    }
    if (!allResults.empty()) {
        SaveRDDValidationSummary(allResults, "output/plots/validation/rdd_validation_summary.csv");
    }
    std::cout << "\nAll RDD validation plots generated and saved.\n";
}