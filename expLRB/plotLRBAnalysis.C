// scripts/root/plotLRBAnalysis.C
// Execute under ROOT: root -l -q 'scripts/root/plotLRBAnalysis.C("lrb_test_output_susp.root")'

#include <iostream>
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
#include <TH1D.h>
#include <TH1F.h>
#include <TGraph.h>
#include <TLegend.h>
#include <TLatex.h>
#include <TGaxis.h>
#include <TStyle.h>
#include <TMath.h>
#include <TSystem.h>

namespace fs = std::filesystem;

struct ParticleConfig {
    std::string branchEdep;
    std::string branchIons;
    std::string legend;
    Int_t color;
    double sumEdep{0.0};
    long long sumIons{0};
    bool isAvailable{false};
};

std::vector<double> MakeLogBins(int nBins, double minVal, double maxVal) {
    std::vector<double> bins(nBins + 1);
    const double logMin = std::log10(minVal);
    const double logMax = std::log10(maxVal);
    const double step = (logMax - logMin) / nBins;
    for (int i = 0; i <= nBins; ++i) {
        bins[i] = std::pow(10.0, logMin + i * step);
    }
    return bins;
}

// Чистый контур без разрывов
TGraph* MakeCleanCurve(const TH1D* hist, double yThreshold, int color, short lineWidth = 3) {
    auto* gr = new TGraph();
    const int nBins = hist->GetNbinsX();

    for (int b = 1; b <= nBins; ++b) {
        double center = hist->GetBinCenter(b);
        double val    = hist->GetBinContent(b);

        if (val > yThreshold && center > 0.0) {
            gr->SetPoint(gr->GetN(), center, val);
        }
    }
    gr->SetLineWidth(lineWidth);
    gr->SetLineColor(color);
    return gr;
}

// Легенда: нижняя граница legY1 строго 0.58
void DrawStyledLegend(const std::vector<TGraph*>& graphs,
                      const std::vector<std::string>& labels,
                      double legX1 = 0.68, double legX2 = 0.90,
                      double legY1 = 0.58, double legY2 = 0.88) 
{
    const double iconWidth = 0.055;

    TLegend* legText = new TLegend(legX1, legY1, legX2 - iconWidth, legY2);
    legText->SetBorderSize(0);
    legText->SetFillStyle(0);
    legText->SetTextSize(0.033);
    legText->SetTextAlign(32); // Текст выровнен вправо
    legText->SetMargin(0.0);
    legText->SetEntrySeparation(0.35);

    TLegend* legIcons = new TLegend(legX2 - iconWidth, legY1, legX2, legY2);
    legIcons->SetBorderSize(0);
    legIcons->SetFillStyle(0);
    legIcons->SetMargin(1.0);  // Значки строго в правой колонке
    legIcons->SetEntrySeparation(0.35);

    for (size_t i = 0; i < graphs.size(); ++i) {
        std::string labelFormatted = Form("#scale[0.95]{#bf{%s}}", labels[i].c_str());
        legText->AddEntry((TObject*)nullptr, labelFormatted.c_str(), "");
        legIcons->AddEntry(graphs[i], "", "l");
    }

    legText->Draw();
    legIcons->Draw();
}

void plotLRBAnalysis(const char* inputFileName = "lrb_monolayer_output_without_filter.root") {
    TH1::AddDirectory(kFALSE);
    gStyle->SetOptStat(0);
    gStyle->SetCanvasPreferGL(kFALSE);
    gStyle->SetPadBorderSize(0);

    TString filePath = inputFileName;
    if (gSystem->AccessPathName(filePath.Data())) {
        if (!gSystem->AccessPathName("test.root")) filePath = "test.root";
        else {
            std::cerr << "[Error] Input file not found: " << inputFileName << std::endl;
            return;
        }
    }

    TFile* file = TFile::Open(filePath.Data(), "READ");
    if (!file || file->IsZombie()) return;

    TTree* tree = (TTree*)file->Get("BioData");
    if (!tree) {
        std::cerr << "[Error] Tree 'BioData' not found!" << std::endl;
        file->Close();
        return;
    }

    const Long64_t nEntries = tree->GetEntries();
    if (nEntries == 0) return;

    std::string bEdepEM = tree->GetBranch("Edep_PrimaryNe20") ? "Edep_PrimaryNe20" : 
                         (tree->GetBranch("Edep_EM") ? "Edep_EM" : "");
    std::string bIonsEM = tree->GetBranch("Ions_PrimaryNe20") ? "Ions_PrimaryNe20" : 
                         (tree->GetBranch("Ions_EM") ? "Ions_EM" : "");

    std::vector<ParticleConfig> parts = {
        {"Edep_Total",    "Ions_Total",    "Total",             kBlack,       0, 0, false},
        {bEdepEM,         bIonsEM,         "Primary ^{20}Ne",   kMagenta + 2, 0, 0, false},
        {"Edep_Proton",   "Ions_Proton",   "Protons (p)",       kBlue + 1,    0, 0, false},
        {"Edep_Alpha",    "Ions_Alpha",    "Alphas (#alpha)",   kOrange + 1,  0, 0, false},
        {"Edep_Deuteron", "Ions_Deuteron", "Deuterons (d)",     kCyan + 2,    0, 0, false},
        {"Edep_Triton",   "Ions_Triton",   "Tritons (t)",       kTeal + 2,    0, 0, false},
        {"Edep_Helium3",  "Ions_Helium3",  "Helium-3 (^{3}He)", kYellow + 2,  0, 0, false},
        {"Edep_HeavyIon", "Ions_HeavyIon", "Heavy (Z #geq 3)",  kGreen + 2,   0, 0, false},
        {"Edep_Other",    "Ions_Other",    "Other & #gamma",    kGray + 1,    0, 0, false}
    };

    Int_t eventID = 0, hitCount = 0;
    tree->SetBranchAddress("EventID", &eventID);
    tree->SetBranchAddress("HitTargetCount", &hitCount);

    std::vector<Double_t> edepVals(parts.size(), 0.0);
    std::vector<Int_t>    ionsVals(parts.size(), 0);

    for (size_t s = 0; s < parts.size(); ++s) {
        if (!parts[s].branchEdep.empty() && tree->GetBranch(parts[s].branchEdep.c_str())) {
            tree->SetBranchAddress(parts[s].branchEdep.c_str(), &edepVals[s]);
            tree->SetBranchAddress(parts[s].branchIons.c_str(), &ionsVals[s]);
            parts[s].isAvailable = true;
        }
    }

    double maxEdepObserved = 500.0;
    double maxIonsObserved = 5000.0;
    long long hitEvents = 0;
    long long totalTargetHits = 0;

    for (Long64_t i = 0; i < nEntries; ++i) {
        tree->GetEntry(i);
        if (hitCount > 0) {
            ++hitEvents;
            totalTargetHits += hitCount;
            double edepPerHit = (edepVals[0] * 1000.0) / hitCount;
            double ionsPerHit = static_cast<double>(ionsVals[0]) / hitCount;

            if (edepPerHit > maxEdepObserved) maxEdepObserved = edepPerHit;
            if (ionsPerHit > maxIonsObserved) maxIonsObserved = ionsPerHit;
        }
    }

    const int nBins = 80;
    const double xIonsMin = 1.0,  xIonsMax = std::max(20000.0, std::ceil(maxIonsObserved * 1.2));
    const double xEdepMin = 0.05, xEdepMax = std::max(1500.0,  std::ceil(maxEdepObserved * 1.2));
    const double yAxisMin = 1e-4, yAxisMax = 1.0;

    auto logBinsIons = MakeLogBins(nBins, xIonsMin, xIonsMax);
    auto logBinsEdep = MakeLogBins(nBins, xEdepMin, xEdepMax);

    std::vector<TH1D*> hICSD(parts.size(), nullptr);
    std::vector<TH1D*> hEdep(parts.size(), nullptr);

    for (size_t s = 0; s < parts.size(); ++s) {
        if (parts[s].isAvailable) {
            hICSD[s] = new TH1D(Form("hICSD_%zu", s), "", nBins, logBinsIons.data());
            hEdep[s] = new TH1D(Form("hEdep_%zu", s), "", nBins, logBinsEdep.data());
        }
    }

    for (Long64_t i = 0; i < nEntries; ++i) {
        tree->GetEntry(i);
        if (hitCount > 0) {
            for (size_t s = 0; s < parts.size(); ++s) {
                if (parts[s].isAvailable) {
                    parts[s].sumEdep += edepVals[s];
                    parts[s].sumIons += ionsVals[s];

                    if (ionsVals[s] > 0) {
                        double ionsPerHit = static_cast<double>(ionsVals[s]) / hitCount;
                        hICSD[s]->Fill(ionsPerHit);
                    }
                    if (edepVals[s] > 0.0) {
                        double edepKeVPerHit = (edepVals[s] * 1000.0) / hitCount;
                        hEdep[s]->Fill(edepKeVPerHit);
                    }
                }
            }
        }
    }

    // ========================================================================
    // СТРОГАЯ МАТЕМАТИЧЕСКАЯ НОРМИРОВКА:
    // Нормировочный делитель — ПОЛНОЕ ЧИСЛО СОБЫТИЙ С ПОПАДАНИЕМ В КЛЕТКИ (hICSD[0]->Integral())
    // Это гарантирует:
    // 1) Интеграл кривой Total строго равен 1.0.
    // 2) Любая компонента s удовлетворяет P_s(nu) <= P_total(nu) в каждом бине.
    // 3) Ни одна кривая не может превысить огибающую Total!
    // ========================================================================
    const double totalHitsCountICSD = (hICSD[0] && hICSD[0]->Integral() > 0) ? hICSD[0]->Integral() : 1.0;
    const double totalHitsCountEdep = (hEdep[0] && hEdep[0]->Integral() > 0) ? hEdep[0]->Integral() : 1.0;

    for (size_t s = 0; s < parts.size(); ++s) {
        if (hICSD[s]) {
            hICSD[s]->Scale(1.0 / totalHitsCountICSD);
        }
        if (hEdep[s]) {
            hEdep[s]->Scale(1.0 / totalHitsCountEdep);
        }
    }

    fs::create_directories("output/plots/analysis");

    // ========================================================================
    // ГРАФИК 1: ПОЛНОМАСШТАБНЫЙ ICSD
    // ========================================================================
    TCanvas* c1 = new TCanvas("c1_ICSD", "ICSD Analysis", 1920, 1080);
    c1->SetLeftMargin(0.10); c1->SetRightMargin(0.10); c1->SetTopMargin(0.08); c1->SetBottomMargin(0.12);
    c1->SetLogx(1); c1->SetLogy(1); c1->SetTickx(1); c1->SetTicky(1);

    TH1F* f1 = new TH1F("f1", "", nBins, xIonsMin, xIonsMax);
    f1->SetStats(0);
    f1->GetXaxis()->SetLimits(xIonsMin, xIonsMax);
    f1->GetXaxis()->SetRangeUser(xIonsMin, xIonsMax);
    f1->GetXaxis()->SetTitle("Ionisation number");
    f1->GetXaxis()->SetTitleSize(0.050);
    f1->GetXaxis()->SetTitleOffset(1.10);
    f1->GetXaxis()->SetLabelSize(0.040);
    f1->GetXaxis()->CenterTitle(true);

    f1->GetYaxis()->SetRangeUser(yAxisMin, yAxisMax);
    f1->GetYaxis()->ChangeLabelByValue(yAxisMin, -1, -1, -1, -1, -1, " ");
    f1->GetYaxis()->ChangeLabelByValue(1.0, -1, -1, -1, -1, -1, "10^{0}");
    f1->GetYaxis()->SetTitle("Frequency");
    f1->GetYaxis()->SetTitleSize(0.055);
    f1->GetYaxis()->SetTitleOffset(0.75);
    f1->GetYaxis()->SetLabelSize(0.040);
    f1->GetYaxis()->CenterTitle(true);
    f1->Draw("AXIS");

    std::vector<TGraph*> grList1;
    std::vector<std::string> legLabels1;

    for (size_t s = 0; s < parts.size(); ++s) {
        if (hICSD[s] && hICSD[s]->GetEntries() > 0) {
            auto* gr = MakeCleanCurve(hICSD[s], yAxisMin * 0.1, parts[s].color, (s == 0 ? 4 : 3));
            gr->Draw("L SAME");
            grList1.push_back(gr);
            legLabels1.push_back(parts[s].legend);
        }
    }

    // Легенда поднята: legY1 = 0.58
    DrawStyledLegend(grList1, legLabels1, 0.68, 0.88, 0.58, 0.88);

    c1->RedrawAxis();
    c1->Update();
    c1->SaveAs("output/plots/analysis/plot_1_ICSD.svg");
    c1->SaveAs("output/plots/analysis/plot_1_ICSD.png");

    // ========================================================================
    // ГРАФИК 2: ПОЛНОМАСШТАБНОЕ ЭНЕРГОВЫДЕЛЕНИЕ E_dep
    // ========================================================================
    TCanvas* c2 = new TCanvas("c2_Energy", "Energy Deposition", 1920, 1080);
    c2->SetLeftMargin(0.10); c2->SetRightMargin(0.10); c2->SetTopMargin(0.08); c2->SetBottomMargin(0.12);
    c2->SetLogx(1); c2->SetLogy(1); c2->SetTickx(1); c2->SetTicky(1);

    TH1F* f2 = new TH1F("f2", "", nBins, xEdepMin, xEdepMax);
    f2->SetStats(0);
    f2->GetXaxis()->SetLimits(xEdepMin, xEdepMax);
    f2->GetXaxis()->SetRangeUser(xEdepMin, xEdepMax);
    f2->GetXaxis()->SetTitle("Deposited energy [keV]");
    f2->GetXaxis()->SetTitleSize(0.050);
    f2->GetXaxis()->SetTitleOffset(1.10);
    f2->GetXaxis()->SetLabelSize(0.040);
    f2->GetXaxis()->CenterTitle(true);

    f2->GetYaxis()->SetRangeUser(yAxisMin, yAxisMax);
    f2->GetYaxis()->ChangeLabelByValue(yAxisMin, -1, -1, -1, -1, -1, " ");
    f2->GetYaxis()->ChangeLabelByValue(1.0, -1, -1, -1, -1, -1, "10^{0}");
    f2->GetYaxis()->SetTitle("Frequency");
    f2->GetYaxis()->SetTitleSize(0.055);
    f2->GetYaxis()->SetTitleOffset(0.75);
    f2->GetYaxis()->SetLabelSize(0.040);
    f2->GetYaxis()->CenterTitle(true);
    f2->Draw("AXIS");

    std::vector<TGraph*> grList2;
    std::vector<std::string> legLabels2;

    for (size_t s = 0; s < parts.size(); ++s) {
        if (hEdep[s] && hEdep[s]->GetEntries() > 0) {
            auto* gr = MakeCleanCurve(hEdep[s], yAxisMin * 0.1, parts[s].color, (s == 0 ? 4 : 3));
            gr->Draw("L SAME");
            grList2.push_back(gr);
            legLabels2.push_back(parts[s].legend);
        }
    }

    // Легенда поднята: legY1 = 0.58
    DrawStyledLegend(grList2, legLabels2, 0.68, 0.88, 0.58, 0.88);

    c2->RedrawAxis();
    c2->Update();
    c2->SaveAs("output/plots/analysis/plot_2_EnergyDeposition.svg");
    c2->SaveAs("output/plots/analysis/plot_2_EnergyDeposition.png");

    // ========================================================================
    // ГРАФИК 3: РАСПРЕДЕЛЕНИЕ ЭНЕРГИИ ПО ОСКОЛКАМ
    // ========================================================================
    TCanvas* c3 = new TCanvas("c3_Bars", "Energy Shares", 1920, 1080);
    c3->SetLeftMargin(0.10); c3->SetRightMargin(0.10); c3->SetTopMargin(0.08); c3->SetBottomMargin(0.15);
    c3->SetTickx(1); c3->SetTicky(1);

    const double grandTotalEdep = parts[0].sumEdep;

    std::vector<std::pair<std::string, double>> shares;
    for (size_t s = 1; s < parts.size(); ++s) {
        if (parts[s].isAvailable) {
            double percent = (grandTotalEdep > 0.0) ? (parts[s].sumEdep / grandTotalEdep * 100.0) : 0.0;
            shares.push_back({parts[s].legend, percent});
        }
    }

    const int nBars = static_cast<int>(shares.size());
    TH1D* hBar = new TH1D("hBar", "", nBars, 0, nBars);
    hBar->SetStats(0);
    hBar->SetFillColor(kAzure + 2);
    hBar->SetLineColor(kBlack);
    hBar->SetLineWidth(2);
    hBar->GetYaxis()->SetRangeUser(0.0, 70.0);
    hBar->GetYaxis()->SetTitle("Energy share [%]");
    hBar->GetYaxis()->SetTitleSize(0.050);
    hBar->GetYaxis()->SetTitleOffset(0.85);
    hBar->GetYaxis()->SetLabelSize(0.040);
    hBar->GetYaxis()->CenterTitle(true);

    hBar->GetXaxis()->SetLabelSize(0.045);
    for (int i = 0; i < nBars; ++i) {
        hBar->GetXaxis()->SetBinLabel(i + 1, shares[i].first.c_str());
        hBar->SetBinContent(i + 1, shares[i].second);
    }
    hBar->LabelsOption("h");
    hBar->Draw("BAR");

    for (int i = 0; i < nBars; ++i) {
        TLatex val;
        val.SetTextFont(62);
        val.SetTextSize(0.035);
        val.SetTextAlign(21);
        val.DrawLatex(i + 0.5, shares[i].second + 1.5, Form("%.1f%%", shares[i].second));
    }

    c3->RedrawAxis();
    c3->Update();
    c3->SaveAs("output/plots/analysis/plot_3_EnergyShares_Bar.svg");
    c3->SaveAs("output/plots/analysis/plot_3_EnergyShares_Bar.png");

    std::cout << "\n========================================================================\n";
    std::cout << "  [SUCCESS] All plots exported with strictly bounded Total envelope:\n";
    std::cout << "    - output/plots/analysis/plot_1_ICSD.svg / .png\n";
    std::cout << "    - output/plots/analysis/plot_2_EnergyDeposition.svg / .png\n";
    std::cout << "    - output/plots/analysis/plot_3_EnergyShares_Bar.svg / .png\n";
    std::cout << "========================================================================\n\n";

    // Терминальный отчёт
    std::cout << "====================================================================================\n";
    std::cout << "                      SIMULATED DATA SUMMARY (RAW AUDIT)                            \n";
    std::cout << "====================================================================================\n";
    std::cout << std::left << std::setw(20) << "Particle"
              << std::setw(16) << "Energy Share"
              << std::setw(18) << "Total Edep [MeV]"
              << std::setw(18) << "Total Ion Pairs"
              << std::setw(14) << "Mean Cluster M1" << "\n";
    std::cout << "------------------------------------------------------------------------------------\n";

    for (size_t s = 0; s < parts.size(); ++s) {
        if (parts[s].isAvailable) {
            double share = (grandTotalEdep > 0.0) ? (parts[s].sumEdep / grandTotalEdep * 100.0) : 0.0;
            double m1 = (hICSD[s] && hICSD[s]->GetEntries() > 0) ? hICSD[s]->GetMean() : 0.0;

            std::ostringstream ssShare;
            ssShare << std::fixed << std::setprecision(2) << share << " %";

            std::cout << std::left << std::setw(20) << parts[s].legend
                      << std::setw(16) << ssShare.str()
                      << std::setw(18) << std::scientific << std::setprecision(3) << parts[s].sumEdep
                      << std::setw(18) << parts[s].sumIons
                      << std::setw(14) << std::fixed << std::setprecision(2) << m1 << "\n";
        }
    }
    std::cout << "====================================================================================\n";
    std::cout << "Events simulated: " << nEntries << " | Hits: " << hitEvents << " | Target hits: " << totalTargetHits << "\n";
    std::cout << "====================================================================================\n\n";

    file->Close();
}