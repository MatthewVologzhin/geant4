// scripts/root/plotGeometryComparison.C
// Execute: root -l -q 'scripts/root/plotGeometryComparison.C+'

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
#include <TLine.h>
#include <TLegend.h>
#include <TLatex.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TMath.h>
#include <TString.h>

namespace fs = std::filesystem;

struct MoleculeConfig {
    std::string name;       // "histone", "NMDA", "ribosome", "cytoskeleton"
    std::string pdbCode;    // "1kx5", "6WHT", "4V6X", "7un1"
    std::string labelName;  // "Histone", "NMDA", "Ribosome", "Cytoskeleton"
    double maxScanX;        // Максимальный предел сканирования по умолчанию (резервный)
};

struct ParticleRun {
    std::string name;        // "proton", "alpha", etc.
    int Z;
    int A;
    double energy;           // в MeV
    std::string energyUnit;  // "MeV"
};

struct ModelConfig {
    std::string type;       // "BOOLEAN", "VOXEL_1-angstrom", "VOXEL_5-angstrom", "VOXEL_10-angstrom", "PREDEFINED"
    std::string legend;     // Текст в легенде
    Int_t color;            // Цвет линии
};

struct ResidualRange {
    double min;
    double max;
};

// Диапазоны отношений Geant4 / Ref
ResidualRange GetResidualRange(const std::string& name) {
    if (name == "proton")   return {0.0, 2.0};
    if (name == "deuteron") return {0.0, 3.5};
    if (name == "alpha")    return {0.4, 1.6};
    if (name == "Li6")      return {0.0, 4.0};
    if (name == "Li7")      return {0.0, 4.5};
    if (name == "histone")      return {0.0, 4.5};
    if (name == "NMDA")         return {0.0, 4.5};
    if (name == "ribosome")     return {0.0, 4.5};
    if (name == "cytoskeleton") return {0.0, 4.5};
    return {0.0, 2.0}; 
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

// Конструктор пути к файлу на основе шаблона имени
std::string GetFilePathPattern(const MoleculeConfig& mol, const ParticleRun& run, const std::string& physName, const std::string& modelType) {
    std::string energyStr = FormatEnergy(run.energy);
    if (modelType == "PREDEFINED") {
        return Form("output/root/icsd_ion-%d-%d_%s-%s_%s_%s_PREDEFINED_*.root",
                    run.Z, run.A, energyStr.c_str(), run.energyUnit.c_str(), physName.c_str(), mol.labelName.c_str());
    } else {
        return Form("output/root/icsd_ion-%d-%d_%s-%s_%s_%s_%s_*.root",
                    run.Z, run.A, energyStr.c_str(), run.energyUnit.c_str(), physName.c_str(), mol.pdbCode.c_str(), modelType.c_str());
    }
}

void compareGeometries(const MoleculeConfig& mol, const ParticleRun& run, const std::string& phys) {
    // ========== НАСТРОЙКА СТИЛЯ CERN (ATLAS/CMS) ==========
    gStyle->SetCanvasPreferGL(kTRUE);
    gStyle->SetPadBorderSize(0);
    gStyle->SetFrameBorderMode(0);
    gStyle->SetCanvasBorderMode(0);
    gStyle->SetPadTickX(1); 
    gStyle->SetPadTickY(1);
    gStyle->SetPadGridX(0);
    gStyle->SetPadGridY(0);
    gStyle->SetTextFont(42);
    gStyle->SetLabelFont(42, "XYZ");
    gStyle->SetTitleFont(42, "XYZ");

    double pad1Height = 0.7;
    double pad2Height = 0.3;
    double scaleFactor = pad1Height / pad2Height;

    double lineWidth = 1.5;
    double yAxisMin = (run.name == "proton" || run.name == "Li6") ? 1e-5 : 1e-6;
    double yAxisMax = 1.0;

    // Модели геометрии для сравнения
    const std::vector<ModelConfig> models = {
        {"BOOLEAN", "Boolean", kBlack},
        {"VOXEL_1-angstrom", "Voxel 1 #AA", kBlue},
        {"VOXEL_5-angstrom", "Voxel 5 #AA", kGreen+2},
        {"VOXEL_10-angstrom", "Voxel 10 #AA", kOrange+1},
        {"PREDEFINED", "Primitive", kRed}
    };

    std::vector<TObject*> garbage;

    // Шаг 1: Автоматический поиск наилучшего доступного эталона для текущего физ. листа
    std::string refModelType = "";
    TChain* refChain = nullptr;
    
    const std::vector<std::string> refPreference = {
        "BOOLEAN",
        "VOXEL_1-angstrom",
        "VOXEL_5-angstrom",
        "VOXEL_10-angstrom",
        "PREDEFINED"
    };

    for (const auto& refType : refPreference) {
        TChain* testChain = new TChain("ntuple_1");
        testChain->Add(GetFilePathPattern(mol, run, phys, refType).c_str());
        if (testChain->GetListOfFiles()->GetEntries() > 0 && testChain->GetEntries() > 0) {
            refModelType = refType;
            refChain = testChain;
            garbage.push_back(refChain);
            std::cout << "  [Reference]: Selected " << refType << " (" << phys << ") as the baseline reference." << std::endl;
            break;
        }
        delete testChain;
    }

    if (!refChain) {
        std::cerr << "  Warning: No geometry files found for " << mol.name << " (" << run.name << ") under " << phys << std::endl;
        return;
    }

    // Шаг 1.5: Определение максимального значения ионизаций во всех файлах для полного распределения
    double absoluteMaxIon = 0.0;
    for (const auto& m : models) {
        TChain* testChain = new TChain("ntuple_1");
        testChain->Add(GetFilePathPattern(mol, run, phys, m.type).c_str());
        if (testChain->GetListOfFiles()->GetEntries() > 0 && testChain->GetEntries() > 0) {
            double maxVal = testChain->GetMaximum("ionisations");
            if (maxVal > absoluteMaxIon) {
                absoluteMaxIon = maxVal;
            }
        }
        delete testChain;
    }

    if (absoluteMaxIon <= 0.0) {
        absoluteMaxIon = mol.maxScanX; // Резервный вариант, если файлы пусты
    }

    // Шаг 2: Загрузка всех доступных геометрий для текущего физического листа
    double histMin = -0.5;
    double histMax = absoluteMaxIon + 0.5;
    unsigned nbBins = static_cast<unsigned>(std::round(histMax - histMin));

    struct LoadedModel {
        std::string type;
        std::string phys;
        std::string legend;
        Int_t color;
        Int_t style; // 1 = Solid
        TH1F* hist;
        TGraphAsymmErrors* graph;
        double totalEntries;
    };

    std::vector<LoadedModel> loadedModels;
    Int_t lStyle = 1; // Сплошная линия для всех геометрий на раздельных графиках
    std::string physSuffix;
    if (phys == "G4EmDNAPhysics_option2"){
        physSuffix = "Opt2";
    } else if (phys == "G4EmDNAPhysics_option4"){
        physSuffix = "Opt4";
    } else if (phys == "G4EmDNAPhysics_option6") {
        physSuffix = "Opt6";
    } else if (phys == "G4EmDNAPhysics_option8"){
        physSuffix = "Opt8";
    }

    for (const auto& m : models) {
        TChain* chain = new TChain("ntuple_1");
        garbage.push_back(chain);
        chain->Add(GetFilePathPattern(mol, run, phys, m.type).c_str());

        if (chain->GetListOfFiles()->GetEntries() == 0 || chain->GetEntries() == 0) {
            continue; 
        }

        double totalEntries = static_cast<double>(chain->GetEntries());
        TString hName = Form("h_%s_%s_%s_%s", mol.name.c_str(), physSuffix.c_str(), m.type.c_str(), run.name.c_str());
        TH1F* hist = new TH1F(hName, "", nbBins, histMin, histMax);
        hist->Sumw2();
        garbage.push_back(hist);

        chain->Draw(Form("ionisations >> %s", hName.Data()), "", "goff");

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

        loadedModels.push_back({m.type, phys, m.legend, m.color, lStyle, hist, graph, totalEntries});
        std::cout << "    Loaded: " << m.type << " | " << physSuffix << " (" << totalEntries << " events)" << std::endl;
    }

    if (loadedModels.size() < 2) {
        std::cout << "  Warning: Only one geometry model available for " << mol.name << ". Skipping." << std::endl;
        for (auto obj : garbage) delete obj;
        return;
    }

    // Динамический поиск границ по оси X для растяжения графика
    double globalMinX = 999.0;
    double globalMaxX = -999.0;
    for (const auto& lm : loadedModels) {
        for (int b = 1; b <= lm.hist->GetNbinsX(); ++b) {
            if (lm.hist->GetBinContent(b) > 0.0) {
                double binCenter = lm.hist->GetBinCenter(b);
                if (binCenter < globalMinX) globalMinX = binCenter;
                if (binCenter > globalMaxX) globalMaxX = binCenter;
            }
        }
    }
    double xAxisMin = (globalMinX < 999.0) ? std::max(0.0, std::floor(globalMinX - 0.5)) : 0.0;
    double xAxisMax = (globalMaxX > -999.0) ? std::ceil(globalMaxX + 0.5) : absoluteMaxIon;

    // Поиск эталона внутри вектора
    LoadedModel* refModel = nullptr;
    for (auto& lm : loadedModels) {
        if (lm.type == refModelType && lm.phys == phys) {
            refModel = &lm;
            break;
        }
    }

    // Расчет энергии на нуклон
    double energyPerNucleon = run.energy / run.A;
    TString energyValStr = (std::abs(energyPerNucleon - std::round(energyPerNucleon)) < 0.01) ?
                            Form("%.0f", energyPerNucleon) : Form("%.2f", energyPerNucleon);

    // Шаг 3: Инициализация холста и расчет оптимального положения легенды
    TString canvasTitle = Form("Geometry comparison of %s (%s) at E = %s %s/u",
                               mol.labelName.c_str(), physSuffix.c_str(), energyValStr.Data(), run.energyUnit.c_str());
    
    TCanvas* c = new TCanvas(Form("c_geom_%s_%s_%s", mol.name.c_str(), run.name.c_str(), physSuffix.c_str()), canvasTitle, 1920, 1080);

    TPad* pad1 = new TPad(Form("pad1_%s", mol.name.c_str()), "main", 0.0, 0.3, 1.0, 1.0);
    TPad* pad2 = new TPad(Form("pad2_%s", mol.name.c_str()), "resid", 0.0, 0.0, 1.0, 0.3);
    
    pad1->SetLogy();
    pad1->SetBottomMargin(0.0); 
    pad1->SetTopMargin(0.05);   
    pad1->SetLeftMargin(0.08);  
    pad1->SetRightMargin(0.03);
    pad1->Draw();
    
    pad2->SetTopMargin(0.0);
    pad2->SetBottomMargin(0.3);
    pad2->SetLeftMargin(0.08);
    pad2->SetRightMargin(0.03);
    pad2->Draw();

    // Функция перевода пользовательских координат осей pad1 в NDC для детекции перекрытий
    auto ToNDC = [&](double x, double y, double& xNDC, double& yNDC) {
        double left = pad1->GetLeftMargin();
        double right = pad1->GetRightMargin();
        double bottom = pad1->GetBottomMargin();
        double top = pad1->GetTopMargin();
        
        xNDC = left + (1.0 - left - right) * (x - xAxisMin) / (xAxisMax - xAxisMin);
        yNDC = bottom + (1.0 - bottom - top) * (std::log10(y) - std::log10(yAxisMin)) / (std::log10(yAxisMax) - std::log10(yAxisMin));
    };

    struct LegendPos {
        std::string name;
        double x1, y1, x2, y2;
    };

    // Сетка из 6 возможных позиций легенды, безопасных по отношению к водяному знаку
    std::vector<LegendPos> candidates = {
        {"Top-Right", 0.74, 0.55, 0.94, 0.93},
        {"Top-Left", 0.15, 0.55, 0.44, 0.93},
        {"Bottom-Right", 0.74, 0.12, 0.94, 0.50},
        {"Middle-Right", 0.74, 0.30, 0.94, 0.68},
        {"Middle-Left", 0.15, 0.30, 0.44, 0.68},
        {"Bottom-Center", 0.45, 0.12, 0.74, 0.50}
    };

    // Алгоритм поиска позиции с абсолютным минимумом пересекающих точек графиков
    int bestIdx = 0;
    int minPoints = 999999;

    for (size_t k = 0; k < candidates.size(); ++k) {
        int count = 0;
        double bx1 = candidates[k].x1 - 0.01;
        double bx2 = candidates[k].x2 + 0.01;
        double by1 = candidates[k].y1 - 0.01;
        double by2 = candidates[k].y2 + 0.01;

        for (const auto& lm : loadedModels) {
            for (int i = 0; i < lm.graph->GetN(); ++i) {
                double x, y;
                lm.graph->GetPoint(i, x, y);
                if (x < xAxisMin || x > xAxisMax || y <= 0.0) continue;
                
                double xNDC, yNDC;
                ToNDC(x, y, xNDC, yNDC);
                
                if (xNDC >= bx1 && xNDC <= bx2 && yNDC >= by1 && yNDC <= by2) {
                    count++;
                }
            }
        }
        
        if (count < minPoints) {
            minPoints = count;
            bestIdx = k;
        }
    }

    double legX1 = candidates[bestIdx].x1;
    double legY1 = candidates[bestIdx].y1;
    double legX2 = candidates[bestIdx].x2;
    double legY2 = candidates[bestIdx].y2;
    std::cout << "  [Legend Selected]: " << candidates[bestIdx].name << " (" << minPoints << " overlapping points detected)." << std::endl;

    TLegend* legend = new TLegend(legX1, legY1, legX2, legY2);
    legend->SetTextSize(0.030);
    legend->SetTextFont(42);
    legend->SetBorderSize(0);  
    legend->SetFillStyle(0);   
    legend->SetEntrySeparation(0.25);
    garbage.push_back(legend);

    // Шаг 4: Отрисовка основного графика (pad1)
    pad1->cd();
    TH1F* hFrame1 = new TH1F(Form("hFrame1_%s", mol.name.c_str()), "", nbBins, histMin, histMax);
    hFrame1->SetStats(0);
    garbage.push_back(hFrame1);
    hFrame1->Draw("AXIS");
    
    TAxis* xAxis = hFrame1->GetXaxis();
    TAxis* yAxis = hFrame1->GetYaxis();
    xAxis->SetLimits(xAxisMin, xAxisMax);
    xAxis->SetRangeUser(xAxisMin, xAxisMax);
    xAxis->SetLabelSize(0);
    xAxis->SetTitleSize(0);
    xAxis->SetTickLength(0.02); 

    yAxis->SetRangeUser(yAxisMin, yAxisMax);
    yAxis->SetTickLength(0.02);
    yAxis->ChangeLabel(1, -1, -1, -1, -1, -1, " "); // Убираем наложение на стыке
    yAxis->SetTitle("Frequency");
    yAxis->SetTitleSize(0.042);
    yAxis->SetTitleOffset(0.72);
    yAxis->CenterTitle(true);

    // Строим полупрозрачные области статистической погрешности (1-sigma и 2-sigma) вокруг эталона в pad1
    TGraphAsymmErrors* refBand2Sig = new TGraphAsymmErrors();
    garbage.push_back(refBand2Sig);
    TGraphAsymmErrors* refBand1Sig = new TGraphAsymmErrors();
    garbage.push_back(refBand1Sig);

    for (int i = 0; i < refModel->graph->GetN(); ++i) {
        double x, y;
        refModel->graph->GetPoint(i, x, y);
        if (x < xAxisMin || x > xAxisMax) continue;

        double yErr = refModel->graph->GetErrorYhigh(i);

        // Безопасный расчет нижних границ для логарифмического масштаба
        double yLow1 = TMath::Max(y - yErr, yAxisMin * 1.01);
        double errLow1 = y - yLow1;
        if (errLow1 < 0.0) errLow1 = 0.0;

        double yLow2 = TMath::Max(y - 2.0 * yErr, yAxisMin * 1.01);
        double errLow2 = y - yLow2;
        if (errLow2 < 0.0) errLow2 = 0.0;

        int ptIdx = refBand2Sig->GetN();
        refBand2Sig->SetPoint(ptIdx, x, y);
        refBand2Sig->SetPointError(ptIdx, 0.5, 0.5, errLow2, 2.0 * yErr); // 2-sigma коридор

        refBand1Sig->SetPoint(ptIdx, x, y);
        refBand1Sig->SetPointError(ptIdx, 0.5, 0.5, errLow1, 1.0 * yErr); // 1-sigma коридор
    }

    // Отрисовка коридоров
    refBand2Sig->SetFillColorAlpha(kGray, 0.3);
    refBand2Sig->SetFillStyle(1001);
    refBand2Sig->SetLineWidth(0);
    refBand2Sig->Draw("E3 SAME");

    refBand1Sig->SetFillColorAlpha(kGray+1, 0.4);
    refBand1Sig->SetFillStyle(1001);
    refBand1Sig->SetLineWidth(0);
    refBand1Sig->Draw("E3 SAME");

    for (auto& lm : loadedModels) {
        lm.graph->SetLineWidth(lineWidth);
        lm.graph->SetLineColor(lm.color);
        lm.graph->SetLineStyle(lm.style); 
        lm.graph->Draw("L SAME");

        // Вычисляем хи-квадрат относительно эталона
        double normChi2 = 0.0;
        int ptsEvaluated = 0;
        for (int i = 0; i < refModel->graph->GetN(); ++i) {
            double x, yRef;
            refModel->graph->GetPoint(i, x, yRef);
            if (x < xAxisMin || x > xAxisMax) continue;

            double simVal = SafeEval(lm.graph, x);
            double denom = (simVal / lm.totalEntries) + (yRef / refModel->totalEntries);
            if (denom > 0.0) {
                double chi2_i = TMath::Power(yRef - simVal, 2) / denom;
                normChi2 += chi2_i;
                ptsEvaluated++;
            }
        }
        if (ptsEvaluated > 1) normChi2 /= (ptsEvaluated - 1);

        if (lm.type == refModelType && lm.phys == phys) {
            legend->AddEntry(lm.graph, Form("#scale[0.9]{#bf{%s}#kern[1.8]{ }(Ref)}", lm.legend.c_str()), "l");
        } else {
            legend->AddEntry(lm.graph, Form("#scale[0.9]{#bf{%s}  [ #chi^{2} = %4.2f ]}", lm.legend.c_str(), normChi2), "l");
        }
    }

    // Теневые коридоры добавляем в легенду
    legend->AddEntry(refBand1Sig, "#scale[0.9]{#bf{Ref. Stat. Uncertainty (1#kern[1.8]{ }#sigma)}}", "f");
    legend->AddEntry(refBand2Sig, "#scale[0.9]{#bf{Ref. Stat. Uncertainty (2#kern[1.8]{ }#sigma)}}", "f");

    // Нанесение строгого водяного знака в левый нижний угол pad1
    TLatex* lModel = new TLatex();
    lModel->SetNDC();
    lModel->SetTextFont(42);
    lModel->SetTextSize(0.040);
    lModel->DrawLatex(0.115, 0.17, "#bf{Geant4-DNA}#kern[0.1]{#it{Simulation}}");
    
    TString particleFormula = GetLatexFormula(run.name);
    lModel->DrawLatex(0.115, 0.11, Form("%s, E = %s %s/u", particleFormula.Data(), energyValStr.Data(), run.energyUnit.c_str()));
    
    std::string physLabel;
    if (phys == "G4EmDNAPhysics_option2"){
        physLabel = "G4EmDNAPhysics (Opt2)";
    } else if (phys == "G4EmDNAPhysics_option4"){
        physLabel = "G4EmDNAPhysics (Opt4)";
    } else if (phys == "G4EmDNAPhysics_option6") {
        physLabel = "G4EmDNAPhysics (Opt6)";
    } else if (phys == "G4EmDNAPhysics_option8"){
        physLabel = "G4EmDNAPhysics (Opt8)";
    }
    
    lModel->DrawLatex(0.115, 0.05, physLabel.c_str());
    garbage.push_back(lModel);

    legend->Draw();

    // Шаг 5: Отрисовка графиков отношений (pad2)
    pad2->cd();
    TH1F* hFrame2 = new TH1F(Form("hFrame2_%s", mol.name.c_str()), "", nbBins, histMin, histMax);
    hFrame2->SetStats(0);
    garbage.push_back(hFrame2);
    hFrame2->Draw("AXIS");

    TAxis* xa = hFrame2->GetXaxis();
    TAxis* ya = hFrame2->GetYaxis();
    
    xa->SetLimits(xAxisMin, xAxisMax);
    xa->SetRangeUser(xAxisMin, xAxisMax);
    xa->SetTitle("Ionisation number");
    xa->SetTitleSize(0.042 * scaleFactor);
    xa->SetLabelSize(0.038 * scaleFactor);
    xa->CenterTitle(true);
    xa->SetTickSize(0.02 * scaleFactor); 
    xa->SetTitleOffset(0.80);

    ResidualRange rRange = GetResidualRange(mol.name);
    ya->SetRangeUser(rRange.min, rRange.max);
    
    std::string refLabel = (refModelType == "BOOLEAN") ? "Boolean" : "Voxel 5 #AA";
    ya->SetTitle(Form("#eta = Model / %s", refLabel.c_str()));
    ya->SetTitleSize(0.030 * scaleFactor); 
    ya->SetTitleOffset(0.95 / scaleFactor); 
    ya->SetLabelSize(0.028 * scaleFactor); 
    ya->SetTickSize(0.02); 
    ya->CenterTitle(true);
    ya->SetNdivisions(305);
    //ya->ChangeLabel(-1, -1, -1, -1, -1, -1, " "); 

    // Нанесение направляющей сетки эталона
    TLine* oneLine = new TLine(xAxisMin, 1.0, xAxisMax, 1.0);
    oneLine->SetLineStyle(2);
    oneLine->SetLineWidth(1);
    oneLine->SetLineColor(kBlack);
    oneLine->Draw("SAME");
    garbage.push_back(oneLine);

    // Построение серых областей относительной статистической погрешности эталона в pad2 вокруг 1.0
    TGraphErrors* ratioBand2Sig = new TGraphErrors();
    TGraphErrors* ratioBand1Sig = new TGraphErrors();
    garbage.push_back(ratioBand2Sig);
    garbage.push_back(ratioBand1Sig);

    for (int i = 0; i < refModel->graph->GetN(); ++i) {
        double x, yRef;
        refModel->graph->GetPoint(i, x, yRef);
        if (x < xAxisMin || x > xAxisMax) continue;

        double yErr = refModel->graph->GetErrorYhigh(i);
        double relErr = (yRef > 0.0) ? (yErr / yRef) : 0.0;

        int ptIdx = ratioBand2Sig->GetN();
        ratioBand2Sig->SetPoint(ptIdx, x, 1.0);
        ratioBand2Sig->SetPointError(ptIdx, 0.5, 2.0 * relErr); // 2-sigma относительный коридор

        ratioBand1Sig->SetPoint(ptIdx, x, 1.0);
        ratioBand1Sig->SetPointError(ptIdx, 0.5, 1.0 * relErr); // 1-sigma относительный коридор
    }

    ratioBand2Sig->SetFillColorAlpha(kGray, 0.3);
    ratioBand2Sig->SetFillStyle(1001);
    ratioBand2Sig->SetLineWidth(0);
    ratioBand2Sig->Draw("E3 SAME");

    ratioBand1Sig->SetFillColorAlpha(kGray+1, 0.4);
    ratioBand1Sig->SetFillStyle(1001);
    ratioBand1Sig->SetLineWidth(0);
    ratioBand1Sig->Draw("E3 SAME");

    // Направляющие линии границ допусков
    /*TLine* plus10Line = new TLine(xAxisMin, 1.1, xAxisMax, 1.1);
    plus10Line->SetLineStyle(3);
    plus10Line->SetLineColor(kGray+1);
    plus10Line->Draw("SAME");
    garbage.push_back(plus10Line);

    TLine* minus10Line = new TLine(xAxisMin, 0.9, xAxisMax, 0.9);
    minus10Line->SetLineStyle(3);
    minus10Line->SetLineColor(kGray+1);
    minus10Line->Draw("SAME");
    garbage.push_back(minus10Line);

    TLine* plus20Line = new TLine(xAxisMin, 1.2, xAxisMax, 1.2);
    plus20Line->SetLineStyle(3);
    plus20Line->SetLineColor(kGray);
    plus20Line->Draw("SAME");
    garbage.push_back(plus20Line);

    TLine* minus20Line = new TLine(xAxisMin, 0.8, xAxisMax, 0.8);
    minus20Line->SetLineStyle(3);
    minus20Line->SetLineColor(kGray);
    minus20Line->Draw("SAME");
    garbage.push_back(minus20Line);*/

    // Построение линий отношений
    for (const auto& lm : loadedModels) {
        TGraph* splineRes = new TGraph();
        garbage.push_back(splineRes);

        for (int i = 0; i < refModel->graph->GetN(); ++i) {
            double x, yRef;
            refModel->graph->GetPoint(i, x, yRef);
            if (x < xAxisMin || x > xAxisMax) continue;

            double simVal = SafeEval(lm.graph, x);
            double ratio = (yRef > 0.0) ? (simVal / yRef) : 1.0;
            splineRes->SetPoint(splineRes->GetN(), x, ratio);
        }

        splineRes->SetLineWidth(lineWidth);
        splineRes->SetLineColor(lm.color);
        splineRes->SetLineStyle(lm.style); 
        splineRes->Draw("L SAME");
    }

    pad1->Update();
    pad1->RedrawAxis();
    pad2->Update();
    pad2->RedrawAxis();

    // Сохранение результатов в output/plots/geometry_comparison
    fs::create_directories("output/plots/geometry_comparison");
    std::string outputPath = "output/plots/geometry_comparison/ICSD_Comp_" + mol.name + "_" + run.name + "_" + FormatEnergy(run.energy) + run.energyUnit + "_" + physSuffix + ".svg";
    c->SaveAs(outputPath.c_str());

    // Освобождение памяти
    delete c;
    for (auto obj : garbage) {
        delete obj;
    }
    std::cout << "  Plot saved: " << outputPath << std::endl;
}

void plotGeometryComparison() {
    // Список макромолекул
    const std::vector<MoleculeConfig> molecules = {
        {"histone", "1kx5", "Histone", 30.0},
        {"NMDA", "6WHT", "NMDA", 45.0},
        {"ribosome", "4V6X", "Ribosome", 50.0},
        {"cytoskeleton", "7un1", "Cytoskeleton", 35.0}
    };

    // Список физических ранов
    const std::vector<ParticleRun> runs = {
        {"proton", 1, 1, 1.0, "MeV"},
        {"deuteron", 1, 2, 2.0, "MeV"},
        {"alpha", 2, 4, 4.0, "MeV"},
        {"Li6", 3, 6, 6.0, "MeV"},
        {"Li7", 3, 7, 7.0, "MeV"},
        {"C12", 6, 12, 12.0, "MeV"},
        {"neon", 10, 20, 20.0, "MeV"}
    };

    // Физические листы
    const std::vector<std::string> physOptions = {
        "G4EmDNAPhysics_option2",
        "G4EmDNAPhysics_option4",
        "G4EmDNAPhysics_option6",
        "G4EmDNAPhysics_option8"
    };

    for (const auto& mol : molecules) {
        for (const auto& run : runs) {
            for (const auto& phys : physOptions) {
                std::cout << "\n========================================\n";
                std::cout << "Comparing: " << mol.name << " | " << run.name << " | " << phys << "\n";
                std::cout << "========================================\n";
                compareGeometries(mol, run, phys);
            }
        }
    }
}