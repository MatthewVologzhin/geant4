// scripts/root/plotTimeComparison.C
// Execute: root -l -q 'scripts/root/plotTimeComparison.C+'

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <filesystem>
#include <cmath>
#include <map>
#include <algorithm>

// ROOT Headers
#include <TCanvas.h>
#include <TPad.h>
#include <TFile.h>
#include <TTree.h>
#include <TChain.h>
#include <TH1F.h>
#include <TBox.h>
#include <TLine.h>
#include <TLegend.h>
#include <TLatex.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TMath.h>
#include <TString.h>

namespace fs = std::filesystem;

struct RunData {
    double totalEvents = 0.0;
    double totalCpuTime = 0.0; // Исключительно User CPU time для аппаратной независимости
    double totalRealTime = 0.0;
};

struct ValidationRun {
    std::string key;
    std::string ion;
    std::string energy;
    std::string label;
};

// Фиксированная конфигурация валидационных запусков (StarTrack и PTB)
const std::vector<ValidationRun> validationConfigs = {
    {"proton_20", "ion-1-1", "20-MeV", "p (20 MeV)"},
    {"deuteron_16", "ion-1-2", "16-MeV", "d (16 MeV)"},
    {"alpha_4", "ion-2-4", "4-MeV", "#alpha (4 MeV)"},
    {"Li6_48", "ion-3-6", "48-MeV", "^{6}Li (48 MeV)"},
    {"Li7_26.7", "ion-3-7", "26.7-MeV", "^{7}Li (26.7 MeV)"},
    {"C12_88", "ion-6-12", "88-MeV", "^{12}C (88 MeV)"}
};

// Нормализация названий макромолекул и детекторов к единому виду
std::string NormalizeMolecule(const std::string& input) {
    std::string s = input;
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    if (s == "1kx5" || s == "histone") return "histone";
    if (s == "6wht" || s == "nmda") return "nmda";
    if (s == "4v6x" || s == "ribosome") return "ribosome";
    if (s == "7un1" || s == "cytoskeleton") return "cytoskeleton";
    if (s == "startrack") return "startrack";
    if (s == "ptb") return "ptb";
    return s;
}

// Форматирование названий для красивого вывода на графиках
std::string GetDisplayMoleculeName(const std::string& s) {
    if (s == "histone") return "Histone (1kx5)";
    if (s == "nmda") return "NMDA (6WHT)";
    if (s == "ribosome") return "Ribosome (4V6X)";
    if (s == "cytoskeleton") return "Cytoskeleton (7un1)";
    if (s == "startrack") return "StarTrack Nanodosimeter";
    if (s == "ptb") return "PTB Nanodosimeter";
    return s;
}

// LaTeX-подписи изотопов
TString GetLatexFormula(const std::string& name) {
    if (name == "proton")   return "p";
    if (name == "deuteron") return "d";
    if (name == "alpha")    return "#alpha";
    if (name == "Li6")      return "^{6}Li";
    if (name == "Li7")      return "^{7}Li";
    if (name == "C12")      return "^{12}C";
    if (name == "neon")     return "^{20}Ne";
    return name.c_str();
}

std::string GetParticleLabel(const std::string& ionToken) {
    if (ionToken == "ion-1-1") return "proton";
    if (ionToken == "ion-1-2") return "deuteron";
    if (ionToken == "ion-2-4") return "alpha";
    if (ionToken == "ion-3-6") return "Li6";
    if (ionToken == "ion-3-7") return "Li7";
    if (ionToken == "ion-6-12") return "C12";
    if (ionToken == "ion-10-20") return "neon";
    return ionToken;
}

std::string GetValidationKey(const std::string& ion, const std::string& energy) {
    for (const auto& v : validationConfigs) {
        if (v.ion == ion && v.energy == energy) {
            return v.key;
        }
    }
    return "";
}

std::vector<std::string> splitString(const std::string& str, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(str);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

// Парсинг содержимого лог-файла Geant4
bool ParseLogFile(const std::string& filePath, double& numEvents, double& userTime, double& sysTime, double& realTime) {
    std::ifstream infile(filePath);
    if (!infile.is_open()) return false;

    std::string line;
    numEvents = 0.0;
    userTime = -1.0;
    sysTime = -1.0;
    realTime = -1.0;

    while (std::getline(infile, line)) {
        if (line.find("Number of events processed :") != std::string::npos) {
            size_t pos = line.find_last_of(":");
            if (pos != std::string::npos) {
                try {
                    numEvents = std::stod(line.substr(pos + 1));
                } catch (...) {
                    numEvents = 0.0;
                }
            }
        }
        if (line.find("User=") != std::string::npos && line.find("Real=") != std::string::npos) {
            size_t userPos = line.find("User=");
            size_t realPos = line.find("Real=");
            size_t sysPos = line.find("Sys=");
            
            if (userPos != std::string::npos && realPos != std::string::npos && sysPos != std::string::npos) {
                std::string userStr = line.substr(userPos + 5, realPos - (userPos + 5) - 1);
                std::string realStr = line.substr(realPos + 5, sysPos - (realPos + 5) - 1);
                size_t cpuPos = line.find("[Cpu=");
                std::string sysStr = line.substr(sysPos + 4, cpuPos - (sysPos + 4) - 1);
                
                try {
                    userTime = std::stod(userStr);
                    realTime = std::stod(realStr);
                    sysTime = std::stod(sysStr);
                } catch (...) {
                    // Игнорируем ошибки парсинга неполных файлов
                }
            }
        }
    }
    return (numEvents > 0.0 && userTime >= 0.0);
}

// Отрисовка объединенного валидационного графика (StarTrack + PTB)
void drawValidationTime(const std::map<std::string, std::map<std::string, RunData>>& validationData) {
    gStyle->SetCanvasPreferGL(kTRUE);
    gStyle->SetPadBorderSize(0);
    gStyle->SetFrameBorderMode(0);
    gStyle->SetCanvasBorderMode(0);
    gStyle->SetPadTickX(1); 
    gStyle->SetPadTickY(1);
    gStyle->SetTextFont(42);
    gStyle->SetLabelFont(42, "XYZ");
    gStyle->SetTitleFont(42, "XYZ");

    double pad1Height = 0.7;
    double pad2Height = 0.3;
    double scaleFactor = pad1Height / pad2Height;

    const std::vector<std::string> allPhysLists = {
        "G4EmDNAPhysics_option2",
        "G4EmDNAPhysics_option4",
        "G4EmDNAPhysics_option6",
        "G4EmDNAPhysics_option8"
    };

    std::map<std::string, Int_t> physColors = {
        {"G4EmDNAPhysics_option2", kRed-4},
        {"G4EmDNAPhysics_option4", kBlue-4},
        {"G4EmDNAPhysics_option6", kGreen-3},
        {"G4EmDNAPhysics_option8", kMagenta-4}
    };

    std::map<std::string, std::string> physLegends = {
        {"G4EmDNAPhysics_option2", "DNA Opt2"},
        {"G4EmDNAPhysics_option4", "DNA Opt4"},
        {"G4EmDNAPhysics_option6", "DNA Opt6"},
        {"G4EmDNAPhysics_option8", "DNA Opt8"}
    };

    std::vector<std::string> activePhysLists;
    for (const auto& phys : allPhysLists) {
        bool hasData = false;
        for (const auto& run : validationConfigs) {
            if (validationData.count(run.key) && validationData.at(run.key).count(phys)) {
                hasData = true;
                break;
            }
        }
        if (hasData) {
            activePhysLists.push_back(phys);
        }
    }

    if (activePhysLists.empty()) return;

    double minTime = 1e9;
    double maxTime = -1e9;
    for (const auto& run : validationConfigs) {
        if (!validationData.count(run.key)) continue;
        for (const auto& phys : activePhysLists) {
            if (!validationData.at(run.key).count(phys)) continue;
            double t = validationData.at(run.key).at(phys).totalCpuTime / validationData.at(run.key).at(phys).totalEvents;
            if (t < minTime) minTime = t;
            if (t > maxTime) maxTime = t;
        }
    }

    double yAxisMin = TMath::Power(10, std::floor(std::log10(minTime) - 0.5));
    double yAxisMax = TMath::Power(10, std::ceil(std::log10(maxTime) + 0.5));

    double maxSlowdown = 1.0;
    for (const auto& run : validationConfigs) {
        if (!validationData.count(run.key)) continue;
        double refT = 0.0;
        std::string refPhys = "G4EmDNAPhysics_option2";
        if (validationData.at(run.key).count(refPhys)) {
            refT = validationData.at(run.key).at(refPhys).totalCpuTime / validationData.at(run.key).at(refPhys).totalEvents;
        }
        if (refT <= 0.0) continue;

        for (const auto& phys : activePhysLists) {
            if (!validationData.at(run.key).count(phys)) continue;
            double t = validationData.at(run.key).at(phys).totalCpuTime / validationData.at(run.key).at(phys).totalEvents;
            double s = t / refT;
            if (s > maxSlowdown) maxSlowdown = s;
        }
    }

    double yAxisResMin = 0.0;
    double yAxisResMax = std::max(2.0, std::ceil(maxSlowdown * 1.2));

    std::vector<TObject*> garbage;

    TCanvas* c = new TCanvas("c_time_validation", "Validation CPU Performance", 1920, 1080);
    TPad* pad1 = new TPad("pad1_val", "main", 0.0, 0.3, 1.0, 1.0);
    TPad* pad2 = new TPad("pad2_val", "resid", 0.0, 0.0, 1.0, 0.3);
    
    pad1->SetLogy();
    pad1->SetBottomMargin(0.0); 
    pad1->SetTopMargin(0.05);   
    pad1->SetLeftMargin(0.14);  
    pad1->SetRightMargin(0.03);
    pad1->Draw();
    
    pad2->SetTopMargin(0.0);
    pad2->SetBottomMargin(0.3);
    pad2->SetLeftMargin(0.14);
    pad2->SetRightMargin(0.03);
    pad2->Draw();

    // ВОССТАНОВЛЕНО: Легенда в верхнем левом углу для Validation
    TLegend* legend = new TLegend(0.175, 0.65, 0.35, 0.90);
    legend->SetTextSize(0.028);
    legend->SetTextFont(42);
    legend->SetBorderSize(0);  
    legend->SetFillStyle(0);   
    legend->SetEntrySeparation(0.25);
    garbage.push_back(legend);

    pad1->cd();
    // Нативная 6-биновая рамка для идеального выравнивания меток валидации
    TH1F* hFrame1 = new TH1F("hFrame1_val", "", 6, 0.5, 6.5);
    hFrame1->SetStats(0);
    garbage.push_back(hFrame1);
    hFrame1->Draw("AXIS");

    TAxis* xAxis = hFrame1->GetXaxis();
    TAxis* yAxis = hFrame1->GetYaxis();
    xAxis->SetLimits(0.5, 6.5); 
    xAxis->SetRangeUser(0.5, 6.5);
    xAxis->SetLabelSize(0);
    xAxis->SetTitleSize(0);
    xAxis->SetTickLength(0);

    yAxis->SetRangeUser(yAxisMin, yAxisMax);
    yAxis->SetTickLength(0.02);
    yAxis->ChangeLabel(1, -1, -1, -1, -1, -1, " ");
    yAxis->SetTitle("User CPU Time per particle [s]");
    yAxis->SetTitleSize(0.042);
    yAxis->SetTitleOffset(0.95);
    yAxis->CenterTitle(true);

    pad2->cd();
    TH1F* hFrame2 = new TH1F("hFrame2_val", "", 6, 0.5, 6.5);
    hFrame2->SetStats(0);
    garbage.push_back(hFrame2);
    hFrame2->Draw("AXIS");

    TAxis* xa = hFrame2->GetXaxis();
    TAxis* ya = hFrame2->GetYaxis();
    xa->SetLimits(0.5, 6.5);
    xa->SetRangeUser(0.5, 6.5);
    xa->SetTitle("Validation Particle Run");
    xa->SetTitleSize(0.042 * scaleFactor);
    xa->SetLabelSize(0.032 * scaleFactor); 
    xa->SetLabelOffset(0.015);
    xa->SetTickSize(0); 

    for (int i = 0; i < 6; ++i) {
        xa->SetBinLabel(i + 1, validationConfigs[i].label.c_str());
    }
    xa->LabelsOption("h"); 

    ya->SetRangeUser(yAxisResMin, yAxisResMax);
    ya->SetTitle("Slowdown Factor"); 
    ya->SetTitleSize(0.030 * scaleFactor);
    ya->SetTitleOffset(0.95 / scaleFactor); 
    ya->SetLabelSize(0.028 * scaleFactor);
    ya->SetTickSize(0.02);
    ya->CenterTitle(true);
    ya->SetNdivisions(305);
    ya->ChangeLabel(-1, -1, -1, -1, -1, -1, " ");

    TLine* baselineLine = new TLine(0.5, 1.0, 6.5, 1.0);
    baselineLine->SetLineStyle(2);
    baselineLine->SetLineWidth(1);
    baselineLine->SetLineColor(kBlack);
    baselineLine->Draw("SAME");
    garbage.push_back(baselineLine);

    int M = activePhysLists.size();

    for (int i = 0; i < 6; ++i) {
        // Зазор 5% от краев достигается через сжатие группы до 0.52
        double binCenter = hFrame2->GetXaxis()->GetBinCenter(i + 1);
        double binWidth = hFrame2->GetXaxis()->GetBinWidth(i + 1);
        double currentGroupWidth = 0.52 * binWidth;
        double currentBarWidth = currentGroupWidth / M;

        std::string runKey = validationConfigs[i].key;
        if (!validationData.count(runKey)) continue;

        double refT = 0.0;
        std::string refPhys = "G4EmDNAPhysics_option2";
        if (validationData.at(runKey).count(refPhys)) {
            refT = validationData.at(runKey).at(refPhys).totalCpuTime / validationData.at(runKey).at(refPhys).totalEvents;
        }

        for (int j = 0; j < M; ++j) {
            std::string phys = activePhysLists[j];
            if (!validationData.at(runKey).count(phys)) continue;

            double numEvents = validationData.at(runKey).at(phys).totalEvents;
            double cpuTime = validationData.at(runKey).at(phys).totalCpuTime;
            double timePerEvent = cpuTime / numEvents;

            double x1 = binCenter - currentGroupWidth/2.0 + j * currentBarWidth;
            double x2 = binCenter - currentGroupWidth/2.0 + (j + 1) * currentBarWidth;

            pad1->cd();
            TBox* box1 = new TBox(x1, yAxisMin, x2, timePerEvent);
            box1->SetFillColor(physColors[phys]);
            box1->SetLineColor(kBlack);
            box1->SetLineWidth(1);
            box1->Draw("SAME");
            garbage.push_back(box1);

            pad2->cd();
            double slowdown = (refT > 0.0) ? (timePerEvent / refT) : 1.0;
            TBox* box2 = new TBox(x1, 0.0, x2, slowdown);
            box2->SetFillColor(physColors[phys]);
            box2->SetLineColor(kBlack);
            box2->SetLineWidth(1);
            box2->Draw("SAME");
            garbage.push_back(box2);
        }
    }

    pad1->cd();
    for (const auto& phys : activePhysLists) {
        TBox* dummyBox = new TBox();
        dummyBox->SetFillColor(physColors[phys]);
        dummyBox->SetLineColor(kBlack);
        dummyBox->SetLineWidth(1);
        legend->AddEntry(dummyBox, Form("#scale[0.9]{#bf{%s}}", physLegends[phys].c_str()), "f");
        garbage.push_back(dummyBox);
    }
    legend->Draw();

    // ВОССТАНОВЛЕНО: Водяной знак в верхнем правом углу для Validation + Кернинг
    TLatex* lModel = new TLatex();
    lModel->SetNDC();
    lModel->SetTextFont(42);
    lModel->SetTextSize(0.040);
    lModel->DrawLatex(0.70, 0.85, "#bf{Geant4-DNA}#kern[0.1]{#it{Simulation}}");
    lModel->DrawLatex(0.70, 0.79, "Validation setups (PREDEFINED)");
    garbage.push_back(lModel);

    // НАСТРОЙКА: Оси поверх гистограмм
    pad1->Update();
    pad1->RedrawAxis();
    pad2->Update();
    pad2->RedrawAxis();

    fs::create_directories("output/plots/performance");
    std::string outputPath = "output/plots/performance/CPU_Comp_Validation.svg";
    c->SaveAs(outputPath.c_str());
    delete c;
    for (auto obj : garbage) delete obj;
    std::cout << "  Validation performance plot saved: " << outputPath << std::endl;
}

// Отрисовка объединенного многопанельного холста по макромолекулам
void drawGeometryComparison(const std::string& molName, 
                            const std::map<std::string, std::map<std::string, std::map<std::string, RunData>>>& molMap) {
    gStyle->SetCanvasPreferGL(kTRUE);
    gStyle->SetPadBorderSize(0);
    gStyle->SetFrameBorderMode(0);
    gStyle->SetCanvasBorderMode(0);
    gStyle->SetPadTickX(1); 
    gStyle->SetPadTickY(1);
    gStyle->SetTextFont(42);
    gStyle->SetLabelFont(42, "XYZ");
    gStyle->SetTitleFont(42, "XYZ");

    double pad1Height = 0.7;
    double pad2Height = 0.3;
    double scaleFactor = pad1Height / pad2Height;

    const std::vector<std::string> particleOrdering = {
        "proton", "deuteron", "alpha", "Li6", "Li7", "C12"
    };

    std::vector<std::string> activeParticles;
    for (const auto& p : particleOrdering) {
        if (molMap.count(p)) activeParticles.push_back(p);
    }
    for (const auto& [p, val] : molMap) {
        if (std::find(activeParticles.begin(), activeParticles.end(), p) == activeParticles.end()) {
            activeParticles.push_back(p);
        }
    }

    int numPlots = activeParticles.size();
    if (numPlots == 0) return;

    // Сетка sub-pad'ов для 6 графиков — строго 2 ряда и 3 колонки
    int rows = 2, cols = 3;
    if (numPlots <= 2) { rows = 1; cols = 2; }
    else if (numPlots <= 4) { rows = 2; cols = 2; }
    else if (numPlots <= 6) { rows = 2; cols = 3; }  
    else { rows = 3; cols = 3; } 

    // НАСТРОЙКА: Сжатие нижнего поля до 5.5% для уменьшения расстояния до легенды
    double y_margin_bottom = 0.055;
    double available_grid_height = 1.0 - y_margin_bottom;

    double W_cell = 1.0 / cols;
    double H_cell = available_grid_height / rows;

    const std::vector<std::string> targetMethods = {
        "BOOLEAN", "VOXEL_1-angstrom", "VOXEL_5-angstrom", "VOXEL_10-angstrom", "PREDEFINED"
    };

    // НАСТРОЙКА: Специальный микро-кернинг для предотвращения наложения символа ангстрема
    const std::vector<std::string> methodLabels = {
        "BOOLEAN", "VOXEL 1#kern[2.8]{ }#AA", "VOXEL 5#kern[2.8]{ }#AA", "VOXEL 10#kern[2.8]{ }#AA", "PREDEFINED"
    };

    const std::vector<std::string> allPhysLists = {
        "G4EmDNAPhysics_option2",
        "G4EmDNAPhysics_option4",
        "G4EmDNAPhysics_option6",
        "G4EmDNAPhysics_option8"
    };

    std::map<std::string, Int_t> physColors = {
        {"G4EmDNAPhysics_option2", kRed-4},
        {"G4EmDNAPhysics_option4", kBlue-4},
        {"G4EmDNAPhysics_option6", kGreen-3},
        {"G4EmDNAPhysics_option8", kMagenta-4}
    };

    std::map<std::string, std::string> physLegends = {
        {"G4EmDNAPhysics_option2", "DNA Opt2"},
        {"G4EmDNAPhysics_option4", "DNA Opt4"},
        {"G4EmDNAPhysics_option6", "DNA Opt6"},
        {"G4EmDNAPhysics_option8", "DNA Opt8"}
    };

    std::vector<std::string> activePhysLists;
    for (const auto& phys : allPhysLists) {
        bool hasData = false;
        for (const auto& part : activeParticles) {
            const auto& pMap = molMap.at(part);
            for (const auto& method : targetMethods) {
                if (pMap.count(method) && pMap.at(method).count(phys)) {
                    hasData = true;
                    break;
                }
            }
        }
        if (hasData) activePhysLists.push_back(phys);
    }

    std::vector<TObject*> garbage;

    TString canvasTitle = Form("CPU Performance comparison for %s", molName.c_str());
    TCanvas* c = new TCanvas(Form("c_grid_time_%s", molName.c_str()), canvasTitle, 1920, 1080);

    for (int k = 0; k < numPlots; ++k) {
        c->cd(); 

        std::string particle = activeParticles[k];
        const auto& pMap = molMap.at(particle);

        int r = k / cols;
        int col = k % cols;

        double y_bottom = y_margin_bottom + (rows - 1 - r) * H_cell;
        double y_top = y_margin_bottom + (rows - r) * H_cell;
        double x_left = col * W_cell;
        double x_right = (col + 1) * W_cell;

        double y_split = y_bottom + 0.3 * H_cell;

        TPad* pad1 = new TPad(Form("pad1_%s_%s", molName.c_str(), particle.c_str()), "main", x_left, y_split, x_right, y_top);
        TPad* pad2 = new TPad(Form("pad2_%s_%s", molName.c_str(), particle.c_str()), "resid", x_left, y_bottom, x_right, y_split);

        pad1->SetLogy();
        pad1->SetBottomMargin(0.0); 
        
        // НАСТРОЙКА: Сжатие зазоров по вертикали (TopMargin урезан с 0.12 до 0.09)
        pad1->SetTopMargin(0.09);   
        
        // НАСТРОЙКА: Сжатие зазоров по горизонтали (LeftMargin 0.14 для col 0, и плотные 0.04 для col > 0)
        double leftMargin = (col == 0) ? 0.14 : 0.04;
        pad1->SetLeftMargin(leftMargin);  
        pad1->SetRightMargin(0.03);
        pad1->Draw();
        
        pad2->SetTopMargin(0.0);
        
        // НАСТРОЙКА: Сжатие зазоров по вертикали (BottomMargin урезан с 0.35 до 0.24)
        pad2->SetBottomMargin(0.24); 
        pad2->SetLeftMargin(leftMargin);
        pad2->SetRightMargin(0.03);
        pad2->Draw();

        // Динамический поиск границ по осям Y
        double minTime = 1e9, maxTime = -1e9;
        for (const auto& method : targetMethods) {
            if (!pMap.count(method)) continue;
            for (const auto& phys : activePhysLists) {
                if (!pMap.at(method).count(phys)) continue;
                double t = pMap.at(method).at(phys).totalCpuTime / pMap.at(method).at(phys).totalEvents;
                if (t < minTime) minTime = t;
                if (t > maxTime) maxTime = t;
            }
        }

        double yAxisMin = TMath::Power(10, std::floor(std::log10(minTime) - 0.5));
        double yAxisMax = TMath::Power(10, std::ceil(std::log10(maxTime) + 0.5));

        std::string refMethod = "";
        if (pMap.count("BOOLEAN")) refMethod = "BOOLEAN";
        else if (pMap.count("PREDEFINED")) refMethod = "PREDEFINED";
        else if (!pMap.empty()) refMethod = pMap.begin()->first;

        std::string refPhys = "G4EmDNAPhysics_option2";
        if (std::find(activePhysLists.begin(), activePhysLists.end(), refPhys) == activePhysLists.end()) {
            refPhys = activePhysLists[0];
        }

        double globalRefTime = 0.0;
        if (!refMethod.empty() && pMap.count(refMethod) && pMap.at(refMethod).count(refPhys)) {
            globalRefTime = pMap.at(refMethod).at(refPhys).totalCpuTime / pMap.at(refMethod).at(refPhys).totalEvents;
        }

        double maxSlowdown = 1.0;
        for (const auto& method : targetMethods) {
            if (!pMap.count(method)) continue;
            for (const auto& phys : activePhysLists) {
                if (!pMap.at(method).count(phys)) continue;
                double t = pMap.at(method).at(phys).totalCpuTime / pMap.at(method).at(phys).totalEvents;
                if (globalRefTime > 0.0) {
                    double s = t / globalRefTime;
                    if (s > maxSlowdown) maxSlowdown = s;
                }
            }
        }

        double yAxisResMin = 0.0;
        double yAxisResMax = std::max(2.0, std::ceil(maxSlowdown * 1.2));

        pad1->cd();
        // Нативная 5-биновая сетка для идеальной центровки подписей BOOLEAN, VOXEL...
        TH1F* hFrame1 = new TH1F(Form("hFrame1_%s_%s", molName.c_str(), particle.c_str()), "", 5, 0.5, 5.5);
        hFrame1->SetStats(0);
        garbage.push_back(hFrame1);
        hFrame1->Draw("AXIS");

        TAxis* xAxis = hFrame1->GetXaxis();
        TAxis* yAxis = hFrame1->GetYaxis();
        xAxis->SetLimits(0.5, 5.5); 
        xAxis->SetRangeUser(0.5, 5.5);
        xAxis->SetLabelSize(0);
        xAxis->SetTitleSize(0);
        xAxis->SetTickLength(0); 

        yAxis->SetRangeUser(yAxisMin, yAxisMax);
        yAxis->SetTickLength(0.02);
        yAxis->ChangeLabel(1, -1, -1, -1, -1, -1, " ");

        // НАСТРОЙКА: Вернули численные метки (SetLabelSize) на все Y-оси, скрыв только дублирующийся текст (SetTitle)
        yAxis->SetLabelSize(0.06);
        if (col == 0) {
            yAxis->SetTitle("User CPU Time [s]");
            yAxis->SetTitleSize(0.065);
            yAxis->SetTitleOffset(0.95);
        } else {
            yAxis->SetTitle("");
        }
        yAxis->CenterTitle(true);

        // Динамический расчет центра заголовка над графиком с учетом измененных индивидуальных Left/RightMargin
        double titleX = pad1->GetLeftMargin() + (1.0 - pad1->GetLeftMargin() - pad1->GetRightMargin()) / 2.0;
        TLatex* pTitle = new TLatex();
        pTitle->SetNDC();
        pTitle->SetTextFont(62);
        pTitle->SetTextSize(0.10);
        pTitle->SetTextAlign(22);
        pTitle->DrawLatex(titleX, 0.96, GetLatexFormula(particle).Data());
        garbage.push_back(pTitle);

        pad2->cd();
        TH1F* hFrame2 = new TH1F(Form("hFrame2_%s_%s", molName.c_str(), particle.c_str()), "", 5, 0.5, 5.5);
        hFrame2->SetStats(0);
        garbage.push_back(hFrame2);
        hFrame2->Draw("AXIS");

        TAxis* xa = hFrame2->GetXaxis();
        TAxis* ya = hFrame2->GetYaxis();
        xa->SetLimits(0.5, 5.5);
        xa->SetRangeUser(0.5, 5.5);
        xa->SetLabelSize(0.042 * scaleFactor); 
        xa->SetLabelOffset(0.015);
        xa->SetTitleSize(0.05 * scaleFactor);
        xa->SetTickSize(0); 
        xa->SetTitleOffset(0.9);

        for (int b = 0; b < 5; ++b) {
            xa->SetBinLabel(b + 1, methodLabels[b].c_str());
        }
        xa->LabelsOption("h"); 

        ya->SetRangeUser(yAxisResMin, yAxisResMax);
        ya->SetTickSize(0.02);
        ya->CenterTitle(true);
        ya->SetNdivisions(305);
        //ya->ChangeLabel(-1, -1, -1, -1, -1, -1, " ");

        // НАСТРОЙКА: Вернули численные метки (SetLabelSize) на все оси Slowdown, скрыв только заголовок для col > 0
        ya->SetLabelSize(0.045 * scaleFactor);
        if (col == 0) {
            ya->SetTitle("Slowdown Factor"); 
            ya->SetTitleSize(0.045 * scaleFactor);
            ya->SetTitleOffset(0.95 / scaleFactor); 
        } else {
            ya->SetTitle("");
        }

        TLine* baselineLine = new TLine(0.5, 1.0, 5.5, 1.0);
        baselineLine->SetLineStyle(2);
        baselineLine->SetLineWidth(1);
        baselineLine->SetLineColor(kBlack);
        baselineLine->Draw("SAME");
        garbage.push_back(baselineLine);

        int M = activePhysLists.size();

        for (int i = 0; i < 5; ++i) {
            // Центрирование гистограмм строго по центрам бинов
            double binCenter = hFrame2->GetXaxis()->GetBinCenter(i + 1);
            double binWidth = hFrame2->GetXaxis()->GetBinWidth(i + 1);
            // Приближение к осям и плотное прижатие столбцов внутри групп
            double currentGroupWidth = 0.52 * binWidth;
            double currentBarWidth = currentGroupWidth / M;

            std::string method = targetMethods[i];

            for (int j = 0; j < M; ++j) {
                std::string phys = activePhysLists[j];
                if (!pMap.count(method) || !pMap.at(method).count(phys)) continue;

                double numEvents = pMap.at(method).at(phys).totalEvents;
                double cpuTime = pMap.at(method).at(phys).totalCpuTime;
                double timePerEvent = cpuTime / numEvents;

                double x1 = binCenter - currentGroupWidth/2.0 + j * currentBarWidth;
                double x2 = binCenter - currentGroupWidth/2.0 + (j + 1) * currentBarWidth;

                pad1->cd();
                TBox* box1 = new TBox(x1, yAxisMin, x2, timePerEvent);
                box1->SetFillColor(physColors[phys]);
                box1->SetLineColor(kBlack);
                box1->SetLineWidth(1);
                box1->Draw("SAME");
                garbage.push_back(box1);

                pad2->cd();
                double slowdown = (globalRefTime > 0.0) ? (timePerEvent / globalRefTime) : 1.0;
                TBox* box2 = new TBox(x1, 0.0, x2, slowdown);
                box2->SetFillColor(physColors[phys]);
                box2->SetLineColor(kBlack);
                box2->SetLineWidth(1);
                box2->Draw("SAME");
                garbage.push_back(box2);
            }
        }

        // НАСТРОЙКА: Отрисовка осей поверх гистограмм во избежание наложений
        pad1->Update();
        pad1->RedrawAxis();
        pad2->Update();
        pad2->RedrawAxis();
    }

    c->cd();

    // НАСТРОЙКА: Водяной знак сбоку, выровненный строго по левой рамке первого графика + микро-кернинг
    double watermarkX = (1.0 / cols) * 0.14;
    TLatex* lModel = new TLatex();
    lModel->SetNDC();
    lModel->SetTextFont(42);
    lModel->SetTextSize(0.020);
    lModel->DrawLatex(watermarkX, 0.033, "#bf{Geant4-DNA}#kern[0.1]{#it{Simulation}}");
    lModel->DrawLatex(watermarkX, 0.012, "Geometry: " + TString(GetDisplayMoleculeName(molName).c_str()));
    garbage.push_back(lModel);

    // НАСТРОЙКА: Выравнивание легенды по правому краю рамки правого графика
    double legendX2 = 1.0 - (1.0 / cols) * 0.03;
    double legendX1 = legendX2 - 0.45; 
    TLegend* legend = new TLegend(legendX1, 0.01, legendX2, 0.055);
    legend->SetTextSize(0.018);
    legend->SetTextFont(42);
    legend->SetBorderSize(0);
    legend->SetFillStyle(0);
    legend->SetNColumns(activePhysLists.size());

    for (const auto& phys : activePhysLists) {
        TBox* dummyBox = new TBox();
        dummyBox->SetFillColor(physColors[phys]);
        dummyBox->SetLineColor(kBlack);
        dummyBox->SetLineWidth(1);
        legend->AddEntry(dummyBox, Form("#scale[1.1]{#bf{%s}}", physLegends[phys].c_str()), "f");
        garbage.push_back(dummyBox);
    }
    legend->Draw();

    c->Update();

    fs::create_directories("output/plots/performance");
    std::string outputPath = "output/plots/performance/CPU_Comp_" + molName + ".svg";
    c->SaveAs(outputPath.c_str());

    delete c;
    for (auto obj : garbage) delete obj;
    std::cout << "  Multi-pad performance plot saved: " << outputPath << std::endl;
}

void plotTimeComparison() {
    std::string logDir = "output/logs";
    if (!fs::exists(logDir)) {
        std::cerr << "Error: Logs directory not found: " << logDir << std::endl;
        return;
    }

    // Иерархическая структура группировки: [Макромолекула][Частица][Метод_Геометрии][Физический_Лист] -> Данные
    std::map<std::string, std::map<std::string, std::map<std::string, std::map<std::string, RunData>>>> gData;
    std::map<std::string, std::map<std::string, RunData>> validationData;

    std::cout << "Processing log files in: " << logDir << std::endl;

    for (const auto& entry : fs::directory_iterator(logDir)) {
        if (entry.path().extension() != ".out") continue;

        std::string filename = entry.path().filename().string();
        std::vector<std::string> tokens = splitString(filename, '_');

        if (tokens.size() < 8) continue;

        // Извлечение параметров из имени файла
        std::string partToken = tokens[1]; 
        std::string physList  = tokens[3] + "_" + tokens[4]; 
        std::string molToken  = tokens[5]; 
        std::string geomToken = tokens[6]; 
        std::string sizeToken = tokens[7]; 

        std::string particle = GetParticleLabel(partToken);
        std::string molecule = NormalizeMolecule(molToken);

        if (particle == "neon") continue;

        std::string methodType = "";
        if (geomToken == "BOOLEAN") methodType = "BOOLEAN";
        else if (geomToken == "PREDEFINED") methodType = "PREDEFINED";
        else if (geomToken == "VOXEL") {
            if (sizeToken.find("1-angstrom") != std::string::npos) methodType = "VOXEL_1-angstrom";
            else if (sizeToken.find("5-angstrom") != std::string::npos) methodType = "VOXEL_5-angstrom";
            else if (sizeToken.find("10-angstrom") != std::string::npos) methodType = "VOXEL_10-angstrom";
        }

        if (methodType.empty()) continue;

        double events = 0.0, userTime = 0.0, sysTime = 0.0, realTime = 0.0;
        if (ParseLogFile(entry.path().string(), events, userTime, sysTime, realTime)) {
            // Исключительное накопление User CPU
            if (molecule == "startrack" || molecule == "ptb") {
                std::string valKey = GetValidationKey(partToken, tokens[2]);
                if (!valKey.empty()) {
                    validationData[valKey][physList].totalEvents += events;
                    validationData[valKey][physList].totalCpuTime += userTime;
                    validationData[valKey][physList].totalRealTime += realTime;
                }
            } else {
                gData[molecule][particle][methodType][physList].totalEvents += events;
                gData[molecule][particle][methodType][physList].totalCpuTime += userTime;
                gData[molecule][particle][methodType][physList].totalRealTime += realTime;
            }
        }
    }

    // 1. Построение единого валидационного графика
    if (!validationData.empty()) {
        std::cout << "\nGenerating validation performance plot..." << std::endl;
        drawValidationTime(validationData);
    }

    // 2. Построение объединенных по геометриям графиков
    for (const auto& [molecule, partMap] : gData) {
        std::cout << "\nGenerating multi-pad plot for geometry: " << molecule << std::endl;
        drawGeometryComparison(molecule, partMap);
    }
    std::cout << "\nAll performance benchmarks successfully generated and saved to 'output/plots/performance/'." << std::endl;
}