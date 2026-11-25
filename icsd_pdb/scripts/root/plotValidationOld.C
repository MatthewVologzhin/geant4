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
#include <algorithm>

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
#include <TGaxis.h>
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

// Оригинальные диапазон осей Y для pad2 из ICSD
ResidualRange GetResidualRange(const std::string& name) {
    if (name == "proton")   return {-3.2, 1.2};
    if (name == "deuteron") return {-5.4, 1.4};
    if (name == "alpha")    return {-0.73, 1.25};
    if (name == "Li6")      return {-6.0, 1.2};
    if (name == "Li7")      return {-6.0, 1.2};
    return {-1.45, 1.45}; // Для C12 и по умолчанию
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

// Загрузка конфигурационного файла
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
    gStyle->SetCanvasPreferGL(kTRUE);
    gStyle->SetPadBorderSize(0);
    
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
    
    // Оригинальные границы оси Y для pad1
    double yAxisMin = (cfg.name == "proton" || cfg.name == "Li6") ? 2e-5 : 2e-6;
    yAxisMin = (cfg.name == "Li7") ? 5e-5: yAxisMin;
    double yAxisMax = 1.0;
    
    // Внутренняя разметка гистограммы
    double histMin = -0.5;
    double histMax = cfg.xAxisMax + 0.5;
    unsigned nbBins = static_cast<unsigned>(std::round(histMax - histMin));
    
    // Границы кадра
    double xAxisMin = xData[0];
    double xAxisMax = xData[n-1];
    
    double lineWidth = 1.5;
    
    // Оригинальные границы оси Y для pad2
    ResidualRange resRange = GetResidualRange(cfg.name);
    double yAxisResMin = resRange.min;
    double yAxisResMax = resRange.max;
    
    // Физические листы
    const std::vector<PhysOpt> physOptions = {
        {"G4EmDNAPhysics_option2", "DNA Opt2", kRed},
        {"G4EmDNAPhysics_option4", "DNA Opt4", kBlue},
        {"G4EmDNAPhysics_option6", "DNA Opt6", kGreen + 2},
        {"G4EmDNAPhysics_option8", "DNA Opt8", kMagenta + 2}
    };
    
    std::string cleanName = cfg.name;
    TString canvasName = Form("c_%s", cleanName.c_str());
    
    double energyPerNucleon = cfg.energy / cfg.A;
    TString energyValStr = (std::abs(energyPerNucleon - std::round(energyPerNucleon)) < 0.01) ?
                            Form("%.0f", energyPerNucleon) : Form("%.2f", energyPerNucleon);
    TString canvasTitle = Form("Ionisation cluster size distribution of %s at E = %s %s/u",
                               cfg.particleLabel.c_str(), energyValStr.Data(), cfg.energyUnit.c_str());
    
    TCanvas* mainCanvas = new TCanvas(canvasName, canvasTitle, 1920, 1080);
    
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
    pad1->Draw();
    
    pad2->SetTopMargin(0);
    pad2->SetBottomMargin(0.3);
    pad2->Draw();
    
    std::vector<TObject*> garbage;
    
    TH1F* hFrame = new TH1F("hFrame", "", nbBins, histMin, histMax);
    garbage.push_back(hFrame);
    TAxis* XaxisRes = hFrame->GetXaxis();
    TAxis* YaxisRes = hFrame->GetYaxis();
    
    // Легенда в стиле RDD
    double legX1 = (cfg.name == "Li7") ? 0.3 : 0.725;
    double legY1 = (cfg.name == "Li7") ? 0.0 : 0.5;
    double legX2 = (cfg.name == "Li7") ? 0.5 : 0.9;
    double legY2 = (cfg.name == "Li7") ? 0.4 : 0.9;

    TLegend* legend = new TLegend(legX1, legY1, legX2, legY2);
    legend->SetTextSize(0.035);
    legend->SetBorderSize(1);
    legend->SetEntrySeparation(0.5);
    garbage.push_back(legend);
    
    int counter = 0;
    
    for (const auto& phys : physOptions) {
        TChain* chain = new TChain("ntuple_1");
        garbage.push_back(chain);
        
        std::string energyStr = FormatEnergy(cfg.energy);
        TString detectorName = GetNanodosimeterName(cfg.name);
        
        TString glob = Form("output/root/icsd_ion-%d-%d_%s-%s_%s_%s_PREDEFINED_*.root",
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
            TH1F* hFrame1 = new TH1F("hFrame1", canvasTitle.Data(), nbBins, histMin, histMax);
            hFrame1->SetStats(0);
            garbage.push_back(hFrame1);
            
            TAxis* xAxis = hFrame1->GetXaxis();
            TAxis* yAxis = hFrame1->GetYaxis();
            hFrame1->Draw("AXIS");
            
            xAxis->SetLimits(xAxisMin, xAxisMax);
            xAxis->SetRangeUser(xAxisMin, xAxisMax);
            xAxis->SetLabelSize(0);
            xAxis->SetTitleSize(0);
            xAxis->SetTickLength(0.03);
            
            yAxis->SetRangeUser(yAxisMin, yAxisMax);
            yAxis->SetTitle("Frequency");
            yAxis->SetTitleSize(0.07);
            yAxis->SetTitleOffset(0.40);
            yAxis->CenterTitle(true);
            
            graph->Draw("L SAME");
            
            // Центральный заголовок сверху в стиле RDD
            TLatex* texTitle = new TLatex(0.5, 0.955, canvasTitle.Data());
            texTitle->SetNDC();
            texTitle->SetTextAlign(22);
            texTitle->SetTextFont(42);
            texTitle->SetTextSize(0.060);
            texTitle->Draw();
            garbage.push_back(texTitle);
            
            // Дополнительные оси TGaxis для Pad 1
            TGaxis* topAxis = new TGaxis(xAxisMin, yAxisMax, xAxisMax, yAxisMax, xAxisMin, xAxisMax, 510, "S-");
            topAxis->SetTickSize(0.07);
            topAxis->SetTickLength(0.037);
            topAxis->SetLabelOffset(999);
            topAxis->Draw("SAME");
            garbage.push_back(topAxis);

            TGaxis* rightAxis = new TGaxis(xAxisMax, yAxisMin, xAxisMax, yAxisMax, yAxisMin, yAxisMax, 510, "+GU");
            rightAxis->Draw();
            garbage.push_back(rightAxis);
            
            pad1->Update();
            pad1->RedrawAxis();
        } else {
            graph->Draw("L SAME");
        }
        
        // Расчет хи-квадрат
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
        legend->AddEntry(graph, Form("#scale[0.9]{#bf{%s}  [  #bf{#chi^{2}} = %.2f ] }", phys.legend.c_str(), normChi2), "l");
        
        // Расчет остатков по формуле RDD: eta = [1 - Geant4 / Ref]
        std::vector<double> res(n, 0.0);
        for (int i = 0; i < n; ++i) {
            double simVal = SafeEval(graph, xData[i]);
            res[i] = (yData[i] != 0.0) ? (yData[i] - simVal) / yData[i] : 0.0;
        }
        
        pad2->cd();
        if (counter == 0) {
            hFrame->SetStats(0);
            hFrame->Draw("AXIS");
            
            XaxisRes->SetRangeUser(xAxisMin, xAxisMax);
            XaxisRes->SetLimits(xAxisMin, xAxisMax);
            XaxisRes->SetTitle("Ionisation number");
            XaxisRes->SetTitleSize(0.155);
            XaxisRes->SetLabelSize(0.09);
            XaxisRes->CenterTitle(true);
            XaxisRes->SetTickSize(0.07);
            XaxisRes->SetTitleOffset(0.88);
            
            YaxisRes->SetRangeUser(yAxisResMin, yAxisResMax);
            YaxisRes->SetTitle("#it{#eta} = [1 - Geant4/Ref]");
            YaxisRes->SetTitleSize(0.082);
            YaxisRes->SetTitleOffset(0.30);
            YaxisRes->SetLabelSize(0.07);
            YaxisRes->SetLabelOffset(0.008);
            YaxisRes->SetNdivisions(305);
            YaxisRes->CenterTitle(true);
            
            // Коридор ошибок остатков
            TGraphErrors* errCorridor = new TGraphErrors(n + 2);
            garbage.push_back(errCorridor);
            
            double relStatErr0 = (yData[0] > 0.0) ? TMath::Sqrt(yData[0] / totalNExp) / yData[0] : 0.0;
            double errLeft = TMath::Sqrt(relStatErr0*relStatErr0 + systemError*systemError);
            errCorridor->SetPoint(0, xAxisMin, 0.0);
            errCorridor->SetPointError(0, 0.5, errLeft);

            for (int i = 0; i < n; ++i) {
                double relStatErr = (yData[i] > 0.0) ? TMath::Sqrt(yData[i] / totalNExp) / yData[i] : 0.0;
                double totalRelErr = TMath::Sqrt(relStatErr*relStatErr + systemError*systemError);
                errCorridor->SetPoint(i + 1, xData[i], 0.0);
                errCorridor->SetPointError(i + 1, 0.5, totalRelErr);
            }

            double relStatErrN = (yData[n-1] > 0.0) ? TMath::Sqrt(yData[n-1] / totalNExp) / yData[n-1] : 0.0;
            double errRight = TMath::Sqrt(relStatErrN*relStatErrN + systemError*systemError);
            errCorridor->SetPoint(n + 1, xAxisMax, 0.0);
            errCorridor->SetPointError(n + 1, 0.5, errRight);

            errCorridor->SetFillColorAlpha(kGray, 0.4);
            errCorridor->SetFillStyle(1001);
            errCorridor->SetLineWidth(0);
            errCorridor->Draw("E3 SAME");
            
            TLine* zeroLine = new TLine(xAxisMin, 0.0, xAxisMax, 0.0);
            zeroLine->SetLineStyle(2);
            zeroLine->Draw("SAME");
            garbage.push_back(zeroLine);
            
            // Дополнительные оси TGaxis для Pad 2
            TGaxis* topAxisRes = new TGaxis(xAxisMin, yAxisResMax, xAxisMax, yAxisResMax, xAxisMin, xAxisMax, 510, "-S");
            topAxisRes->SetTickSize(0.07);
            topAxisRes->SetLabelOffset(999);
            topAxisRes->Draw("SAME");
            garbage.push_back(topAxisRes);

            TGaxis* rightAxisRes = new TGaxis(xAxisMax, yAxisResMin, xAxisMax, yAxisResMax,
                                             yAxisResMin, yAxisResMax,
                                             YaxisRes->GetNdivisions(), "+");
            rightAxisRes->SetTickSize(0.03);
            rightAxisRes->SetLabelOffset(999);
            rightAxisRes->Draw("SAME");
            garbage.push_back(rightAxisRes);
            
            pad2->RedrawAxis();
        }
        
        TGraph* splineRes = new TGraph(n, xData.data(), res.data());
        garbage.push_back(splineRes);
        splineRes->SetLineWidth(lineWidth);
        splineRes->SetLineColor(phys.color);
        splineRes->Draw("L SAME");
        
        counter++;
    }
    
    // Отрисовка экспериментальных точек на Pad 1
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
    
    pad1->cd();
    legend->Draw();
    mainCanvas->Update();
    
    // Сохранение графиков в PDF
    fs::create_directories("output/plots/validation");
    std::string outputPath = "output/plots/validation/ICSD_" + cfg.name + "_" + FormatEnergy(cfg.energy) + cfg.energyUnit + ".pdf";
    mainCanvas->SaveAs(outputPath.c_str());
    
    delete mainCanvas;
    for (auto obj : garbage) {
        delete obj;
    }
    std::cout << "  Plot saved: " << outputPath << std::endl;
}

void plotValidationOld() {
    auto configs = LoadConfigs("input/experimental_data/particles.txt");
    if (configs.empty()) {
        std::cerr << "No configurations loaded!" << std::endl;
        return;
    }
    for (const auto& cfg : configs) {
        std::cout << "\n========================================\n";
        std::cout << "Processing ICSD: " << cfg.name << " (" << cfg.energy << " " << cfg.energyUnit << ")\n";
        std::cout << "========================================\n";
        plotSingleParticle(cfg);
    }
    std::cout << "\nAll ICSD plots have been successfully generated and saved to 'output/plots/validation/'.\n";
}