// scripts/root/plotValidation.C
// Execute: root -l -q 'scripts/root/plotValidation.C+'

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <filesystem>
#include <cmath>
#include <map>

// ROOT Headers
#include <TCanvas.h>
#include <TPad.h>
#include <TFile.h>
#include <TTree.h>
#include <TChain.h>
#include <TH1F.h>
#include <TGraphAsymmErrors.h>
#include <TGraphErrors.h>
#include <TLine.h>
#include <TLegend.h>
#include <TLatex.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TMath.h>
#include <TString.h>

namespace fs = std::filesystem;

struct ParticleConfig {
    std::string name;
    std::string particleLabel;
    int Z = 0;
    int A = 0;
    double energy = 0.0;
    std::string energyUnit;
    std::string source;
    double xAxisMax = 0.0;
    double systemError = 0.0;
    std::string expDataFile;
    std::string dataAuthors;
};

struct ExpPoint {
    double x;
    double y;
};

struct PhysOpt {
    std::string name;
    std::string legend;
    Int_t color;
};

struct ResidualRange {
    double min;
    double max;
};

// LaTeX-форматирование для изотопов и легких ионов
TString GetLatexFormula(const std::string& name) {
    if (name == "proton")   return "p";
    if (name == "deuteron") return "d";
    if (name == "alpha")    return "#alpha";
    if (name == "Li6")      return "^{6}Li";
    if (name == "Li7")      return "^{7}Li";
    if (name == "C12")      return "^{12}C";
    return name.c_str();
}

// Связывание частицы с ее валидационным нанодозиметром
TString GetNanodosimeterName(const std::string& name) {
    if (name == "proton" || name == "deuteron" || name == "Li6" || name == "Li7") {
        return "StarTrack";
    }
    return "PTB"; // Для alpha и C12
}

// Автоматическое определение totalNExp для каждой частицы
double GetTotalNExp(const std::string& name) {
    if (name == "proton")   return 5e4;
    if (name == "deuteron") return 2.5e4;
    return 1e4; // Для alpha, Li6, Li7, C12
}

// Диапазоны отношений Geant4 / Ref (с учетом непопадания хвостов)
ResidualRange GetResidualRange(const std::string& name) {
    if (name == "proton")   return {0.0, 2.0};
    if (name == "deuteron") return {0.0, 3.5};
    if (name == "alpha")    return {0.4, 1.6};
    if (name == "Li6")      return {0.0, 4.0};
    if (name == "Li7")      return {0.0, 4.5};
    return {0.0, 2.0}; // Для C12 и по умолчанию
}

// Безопасное форматирование энергии (например, 20.0 -> "20", 26.7 -> "26p7")
std::string FormatEnergy(double energy) {
    double intPart = 0.0;
    double fracPart = std::modf(energy, &intPart);
    if (std::abs(fracPart) < 0.01) {
        return std::to_string(static_cast<int>(intPart));
    } else {
        int frac = static_cast<int>(std::round(fracPart * 10.0));
        return std::to_string(static_cast<int>(intPart)) + "p" + std::to_string(frac);
    }
}

// Загрузка конфигурационного файла (C++17)
std::vector<ParticleConfig> LoadConfigs(const std::string& filename) {
    std::vector<ParticleConfig> configs;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open configuration file: " << filename << std::endl;
        return configs;
    }
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        size_t first = line.find_first_not_of(" \t\r\n");
        if (first == std::string::npos || line[first] == '#') continue;

        std::stringstream ss(line);
        ParticleConfig cfg;
        if (ss >> cfg.name 
               >> cfg.particleLabel 
               >> cfg.Z 
               >> cfg.A 
               >> cfg.energy 
               >> cfg.energyUnit 
               >> cfg.source 
               >> cfg.xAxisMax 
               >> cfg.systemError 
               >> cfg.expDataFile) {
            
            std::string authors;
            std::getline(ss, authors);
            size_t start = authors.find_first_not_of(" \t\r\n");
            if (start != std::string::npos) {
                size_t end = authors.find_last_not_of(" \t\r\n");
                cfg.dataAuthors = authors.substr(start, end - start + 1);
            } else {
                cfg.dataAuthors = "Reference Data";
            }
            configs.push_back(cfg);
        } else {
            std::cerr << "Warning: Failed to parse line: " << line << std::endl;
        }
    }
    return configs;
}

// Загрузка экспериментальных данных
std::vector<ExpPoint> LoadExperimentalData(const std::string& filename) {
    std::vector<ExpPoint> points;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open experimental data file: " << filename << std::endl;
        return points;
    }
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        size_t first = line.find_first_not_of(" \t\r\n");
        if (first == std::string::npos || line[first] == '#') continue;

        std::stringstream ss(line);
        double x = 0.0, y = 0.0;
        if (ss >> x >> y) {
            points.push_back({x, y});
        }
    }
    return points;
}

// Безопасное вычисление интерполяции
double SafeEval(TGraphAsymmErrors* g, double x) {
    int n = g->GetN();
    if (n == 0) return 0.0;
    double xMin = 0.0, yMin = 0.0, xMax = 0.0, yMax = 0.0;
    g->GetPoint(0, xMin, yMin);
    g->GetPoint(n - 1, xMax, yMax);
    if (x < xMin || x > xMax) return 0.0;
    double val = g->Eval(x);
    return (val < 0.0) ? 0.0 : val;
}

void plotSingleParticle(const ParticleConfig& cfg) {
    // ========== СОВРЕМЕННЫЙ СТИЛЬ CERN (ATLAS/CMS) ==========
    gStyle->SetCanvasPreferGL(kTRUE);
    gStyle->SetPadBorderSize(0);
    gStyle->SetFrameBorderMode(0);
    gStyle->SetCanvasBorderMode(0);
    
    // Автоматическая прорисовка засечек внутрь на всех сторонах кадра
    gStyle->SetPadTickX(1); 
    gStyle->SetPadTickY(1);
    
    gStyle->SetPadGridX(0);
    gStyle->SetPadGridY(0);
    gStyle->SetTextFont(42);
    gStyle->SetLabelFont(42, "XYZ");
    gStyle->SetTitleFont(42, "XYZ");
    
    // Чтение экспериментальных точек
    auto expPoints = LoadExperimentalData(cfg.expDataFile);
    int n = static_cast<int>(expPoints.size());
    if (n == 0) {
        std::cerr << "ERROR: Empty or missing experimental file " << cfg.expDataFile << std::endl;
        return;
    }
    
    std::vector<double> xData(n), yData(n);
    for (int i = 0; i < n; ++i) {
        xData[i] = expPoints[i].x;
        yData[i] = expPoints[i].y;
    }
    
    double totalNExp = GetTotalNExp(cfg.name);
    double systemError = cfg.systemError;
    double yAxisMin = (cfg.name == "proton" || cfg.name == "Li6") ? 1e-5 : 1e-6;
    double yAxisMax = 1.0;
    
    // Внутренняя разметка гистограммы (для сохранения целых бинов)
    double histMin = -0.5;
    double histMax = cfg.xAxisMax + 0.5;
    unsigned nbBins = static_cast<unsigned>(std::round(histMax - histMin));
    
    // Строгие рамки кадра (от 0.0 до последней экспериментальной точки)
    double xAxisMin = xData[0];
    double xAxisMax = xData[n-1];
    
    double lineWidth = 1.5;
    ResidualRange resRange = GetResidualRange(cfg.name);
    double yAxisResMin = resRange.min;
    double yAxisResMax = resRange.max;
    
    // Относительные высоты панелей
    double pad1Height = 0.7;
    double pad2Height = 0.3;
    double scaleFactor = pad1Height / pad2Height; 
    
    // Список физических листов
    const std::vector<PhysOpt> physOptions = {
        {"G4EmDNAPhysics_option2", "DNA Opt2", kRed},
        {"G4EmDNAPhysics_option4", "DNA Opt4", kBlue},
        {"G4EmDNAPhysics_option6", "DNA Opt6", kGreen+2},
        {"G4EmDNAPhysics_option8", "DNA Opt8", kMagenta+2}
    };
    
    std::string cleanName = cfg.name;
    TString canvasName = Form("c_%s", cleanName.c_str());
    
    double energyPerNucleon = cfg.energy / cfg.A;
    TString energyValStr = (std::abs(energyPerNucleon - std::round(energyPerNucleon)) < 0.01) ?
                            Form("%.0f", energyPerNucleon) : Form("%.2f", energyPerNucleon);
    TString canvasTitle = Form("Ionisation cluster size distribution of %s at E = %s %s/u",
                               cfg.particleLabel.c_str(), energyValStr.Data(), cfg.energyUnit.c_str());
    
    TCanvas* c = new TCanvas(canvasName, canvasTitle, 1920, 1080);
    
    TPad* pad1 = new TPad(Form("pad1_%s", cleanName.c_str()), "main", 0.0, 0.3, 1.0, 1.0);
    TPad* pad2 = new TPad(Form("pad2_%s", cleanName.c_str()), "resid", 0.0, 0.0, 1.0, 0.3);
    
    pad1->SetLogy();
    pad1->SetBottomMargin(0.0); 
    pad1->SetTopMargin(0.05);   
    pad1->SetLeftMargin(0.12);  
    pad1->SetRightMargin(0.05);
    pad1->Draw();
    
    pad2->SetTopMargin(0.0);
    pad2->SetBottomMargin(0.3);
    pad2->SetLeftMargin(0.12);
    pad2->SetRightMargin(0.05);
    pad2->Draw();
    
    std::vector<TObject*> garbage;
    
    TH1F* hFrame = new TH1F(Form("hFrame_%s", cleanName.c_str()), "", nbBins, histMin, histMax);
    garbage.push_back(hFrame);
    
    // Прижимаем легенду вплотную к правой границе (0.945 при RightMargin = 0.05)
    TLegend* legend = new TLegend(0.755, 0.65, 0.945, 0.93);
    legend->SetTextSize(0.035);
    legend->SetTextFont(42);
    legend->SetBorderSize(0);  
    legend->SetFillStyle(0);   
    legend->SetEntrySeparation(0.3);
    garbage.push_back(legend);
    
    int counter = 0;
    for (const auto& phys : physOptions) {
        TChain* chain = new TChain("ntuple_1");
        garbage.push_back(chain);
        
        std::string energyStr = FormatEnergy(cfg.energy);
        TString detectorName = GetNanodosimeterName(cfg.name);
        
        // Находит ровно ОДИН правильный файл нанодозиметра, исключая биологические воксели
        TString glob = Form("results/output/icsd_ion-%d-%d_%s-%s_%s_%s_PREDEFINED_*.root",
                            cfg.Z, cfg.A, energyStr.c_str(), cfg.energyUnit.c_str(), phys.name.c_str(), detectorName.Data());
        chain->Add(glob);
        
        if (chain->GetListOfFiles()->GetEntries() == 0) {
            std::cout << "  No files found for: " << phys.name << " (Glob: " << glob << ")" << std::endl;
            continue;
        }
        
        double totalNSim = static_cast<double>(chain->GetEntries());
        if (totalNSim <= 0) {
            std::cout << "  Empty chain for: " << phys.name << std::endl;
            continue;
        }
        
        std::cout << "  " << phys.name << ": " << totalNSim << " entries loaded." << std::endl;
        
        pad1->cd();
        TString histName = Form("h_%s_%s", cleanName.c_str(), phys.name.c_str());
        TH1F* hist = new TH1F(histName, "", nbBins, histMin, histMax);
        hist->Sumw2();
        garbage.push_back(hist);
        
        chain->Draw(Form("ionisations >> %s", histName.Data()), "", "goff");
        
        double normFactor = hist->Integral();
        if (normFactor > 0.0) hist->Scale(1.0 / normFactor);
        
        TGraphAsymmErrors* graph = new TGraphAsymmErrors();
        garbage.push_back(graph);
        
        for (int j = 1; j <= static_cast<int>(nbBins); ++j) {
            double content = hist->GetBinContent(j);
            if (content > 0.0) {
                double center = hist->GetBinCenter(j);
                double error = hist->GetBinError(j);
                int pt = graph->GetN();
                graph->SetPoint(pt, center, content);
                graph->SetPointError(pt, 0, 0, error, error);
            }
        }
        
        graph->SetLineWidth(lineWidth);
        graph->SetLineColor(phys.color);
        
        if (counter == 0) {
            TH1F* hFrame1 = new TH1F(Form("hFrame1_%s", cleanName.c_str()), "", nbBins, histMin, histMax);
            hFrame1->SetStats(0);
            garbage.push_back(hFrame1);
            hFrame1->Draw("AXIS");
            
            TAxis* xAxis = hFrame1->GetXaxis();
            TAxis* yAxis = hFrame1->GetYaxis();
            
            // Начинаем строго с 0.0 и до последней экспериментальной точки
            xAxis->SetLimits(xAxisMin, xAxisMax);
            xAxis->SetRangeUser(xAxisMin, xAxisMax);
            xAxis->SetLabelSize(0);
            xAxis->SetTitleSize(0);
            xAxis->SetTickLength(0.02); 
            
            yAxis->SetRangeUser(yAxisMin, yAxisMax);
            yAxis->SetTickLength(0.02); 
            
            // Скрываем нижнюю метку оси Y, чтобы не налагалась на pad2
            yAxis->ChangeLabel(1, -1, -1, -1, -1, -1, " ");
            
            yAxis->SetTitle("Frequency");
            yAxis->SetTitleSize(0.042);
            yAxis->SetTitleOffset(0.72); // Придвигаем вплотную к графику
            yAxis->CenterTitle(true);
            
            graph->Draw("L SAME");
            
            // Водяной знак опущен в самый низ pad1 (к стыку панелей)
            TLatex* lModel = new TLatex();
            lModel->SetNDC();
            lModel->SetTextFont(42);
            lModel->SetTextSize(0.040);
            lModel->DrawLatex(0.14, 0.105, "#bf{Geant4-DNA} #it{Simulation}");
            
            TString particleFormula = GetLatexFormula(cfg.name);
            lModel->DrawLatex(0.14, 0.045, Form("%s, E = %s %s/u", particleFormula.Data(), energyValStr.Data(), cfg.energyUnit.c_str()));
            garbage.push_back(lModel);
            
            pad1->Update();
            pad1->RedrawAxis();
        } else {
            graph->Draw("L SAME");
        }
        
        // Математически корректный расчет хи-квадрат
        double normChi2 = 0.0;
        for (int i = 0; i < n; ++i) {
            double simVal = SafeEval(graph, xData[i]);
            double denom = (simVal / totalNSim) + (yData[i] / totalNExp) + TMath::Power(systemError * yData[i], 2);
            if (denom > 0.0) {
                double chi2_i = TMath::Power(yData[i] - simVal, 2) / denom;
                normChi2 += chi2_i;
            }
        }
        normChi2 /= (n - 1);
        legend->AddEntry(graph, Form("#scale[0.9]{#bf{%s}  [ #chi^{2} = %5.2f ]}", phys.legend.c_str(), normChi2), "l");
        
        // Отношение Geant4 / Ref
        std::vector<double> res(n, 0.0);
        for (int i = 0; i < n; ++i) {
            double simVal = SafeEval(graph, xData[i]);
            res[i] = (yData[i] != 0.0) ? (simVal / yData[i]) : 1.0; 
        }
        
        pad2->cd();
        if (counter == 0) {
            hFrame->SetStats(0);
            hFrame->Draw("AXIS");
            
            TAxis* xa = hFrame->GetXaxis();
            TAxis* ya = hFrame->GetYaxis();
            
            xa->SetLimits(xAxisMin, xAxisMax);
            xa->SetRangeUser(xAxisMin, xAxisMax);
            xa->SetTitle("Ionisation number");
            xa->SetTitleSize(0.042 * scaleFactor);
            xa->SetLabelSize(0.038 * scaleFactor);
            xa->CenterTitle(true);
            
            // ВЕРТИКАЛЬНЫЕ ЗАСЕЧКИ (X) теперь умножаются на scaleFactor для абсолютного равенства с pad1!
            xa->SetTickSize(0.02 * scaleFactor); 
            xa->SetTitleOffset(0.80);
            
            ya->SetRangeUser(yAxisResMin, yAxisResMax);
            ya->SetTitle("#eta = Geant4 / Ref"); 
            ya->SetTitleSize(0.042 * scaleFactor);
            ya->SetTitleOffset(0.72 / scaleFactor); // Идеально выровнено с Frequency по вертикали
            ya->SetLabelSize(0.038 * scaleFactor);
            
            // ГОРИЗОНТАЛЬНЫЕ ЗАСЕЧКИ (Y) не требуют умножения на scaleFactor
            ya->SetTickSize(0.02); 
            ya->CenterTitle(true);
            ya->SetNdivisions(305);
            
            // Скрываем верхнюю метку шкалы pad2, чтобы она не перекрывалась границей pad1
            ya->ChangeLabel(-1, -1, -1, -1, -1, -1, " ");
            
            // Коридор ошибок, начинающийся и заканчивающийся строго по осям
            TGraphErrors* errCorridor = new TGraphErrors(n);
            garbage.push_back(errCorridor);
            for (int i = 0; i < n; ++i) {
                double relStatErr = (yData[i] > 0.0) ? TMath::Sqrt(yData[i] / totalNExp) / yData[i] : 0.0;
                double totalRelErr = TMath::Sqrt(relStatErr*relStatErr + systemError*systemError);
                errCorridor->SetPoint(i, xData[i], 1.0); // Центрировано на 1.0
                errCorridor->SetPointError(i, 0.5, totalRelErr);
            }
            errCorridor->SetFillColorAlpha(kGray, 0.4);
            errCorridor->SetFillStyle(1001);
            errCorridor->SetLineWidth(0);
            errCorridor->Draw("E3 SAME");
            
            // Линия сравнения на уровне 1.0
            TLine* oneLine = new TLine(xAxisMin, 1.0, xAxisMax, 1.0);
            oneLine->SetLineStyle(2);
            oneLine->Draw("SAME");
            garbage.push_back(oneLine);
            
            pad2->RedrawAxis();
        }
        
        TGraph* splineRes = new TGraph(n, xData.data(), res.data());
        splineRes->SetLineWidth(lineWidth);
        splineRes->SetLineColor(phys.color);
        splineRes->Draw("L SAME");
        garbage.push_back(splineRes);
        
        counter++;
    }
    
    // Отрисовка экспериментальных точек
    pad1->cd();
    TGraphAsymmErrors* graphExp = new TGraphAsymmErrors(n);
    garbage.push_back(graphExp);
    for (int i = 0; i < n; ++i) {
        double statErr = TMath::Sqrt(yData[i] / totalNExp);
        double systErr = systemError * yData[i];
        double totalErr = TMath::Sqrt(statErr*statErr + systErr*systErr);
        graphExp->SetPoint(i, xData[i], yData[i]);
        graphExp->SetPointError(i, 0, 0, totalErr, totalErr);
    }
    graphExp->SetMarkerStyle(25);
    graphExp->SetMarkerSize(3);
    graphExp->SetMarkerColor(kBlack);
    graphExp->Draw("P E SAME");
    
    legend->AddEntry(graphExp, Form("#scale[0.9]{#bf{%s}}", cfg.dataAuthors.c_str()), "p");
    legend->Draw();
    
    pad1->Update();
    pad1->RedrawAxis();
    pad2->cd();
    pad2->Update();
    pad2->RedrawAxis();
    
    // Экспорт в PDF
    fs::create_directories("results/plots/");
    std::string outputPath = "results/plots/ICSD_" + cfg.name + "_" + FormatEnergy(cfg.energy) + cfg.energyUnit + ".pdf";
    c->SaveAs(outputPath.c_str());
    
    // Полное освобождение памяти
    delete c;
    for (auto obj : garbage) {
        delete obj;
    }
    std::cout << "  Plot saved: " << outputPath << std::endl;
}

void plotValidation() {
    auto configs = LoadConfigs("input/experimental_data/particles.txt");
    if (configs.empty()) {
        std::cerr << "No configurations loaded!" << std::endl;
        return;
    }
    for (const auto& cfg : configs) {
        std::cout << "\n========================================\n";
        std::cout << "Processing: " << cfg.name << " (" << cfg.energy << " " << cfg.energyUnit << ")\n";
        std::cout << "========================================\n";
        plotSingleParticle(cfg);
    }
    std::cout << "\nAll plots have been successfully generated and saved to 'results/plots/'.\n";
}