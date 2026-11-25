// plot_edep.C
#include <TFile.h>
#include <TTree.h>
#include <TH1D.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <TStyle.h>
#include <iostream>

void plot_edep(const char* filename = "lrb_monolayer_output_without_filter.root")
{
    gStyle->SetOptStat(0);

    TFile* file = TFile::Open(filename, "READ");
    if (!file || file->IsZombie()) {
        std::cerr << "Error: Cannot open file " << filename << std::endl;
        return;
    }

    auto* tree = file->Get<TTree>("BioData");
    if (!tree) {
        std::cerr << "Error: TTree 'BioData' not found!" << std::endl;
        return;
    }

    // Определяем максимальное энерговыделение для динамического диапазона
    double maxEdep = tree->GetMaximum("TotalEdep");
    if (maxEdep <= 0.0) maxEdep = 1.0;

    // Создаем гистограммы с одинаковым биннингом
    const int nBins = 150;
    auto* hTotal = new TH1D("hEdepTotal", "Energy Deposition Spectrum;E_{dep} [MeV];Events", nBins, 0, maxEdep * 1.05);
    auto* hHad   = new TH1D("hEdepHad",   "E_{dep} Hadronic;E_{dep} [MeV];Events", nBins, 0, maxEdep * 1.05);
    auto* hEM    = new TH1D("hEdepEM",    "E_{dep} EM;E_{dep} [MeV];Events", nBins, 0, maxEdep * 1.05);

    // Заполняем проекциями из дерева
    tree->Project("hEdepTotal", "TotalEdep");
    tree->Project("hEdepHad",   "TotalEdepHad");
    tree->Project("hEdepEM",    "TotalEdepEM");

    auto* c2 = new TCanvas("c_edep", "Edep Comparison", 900, 700);
    c2->SetGrid();
    c2->SetLogy();

    // Настройка отображения
    hTotal->SetLineColor(kBlack);
    hTotal->SetLineWidth(3);

    hHad->SetLineColor(kBlue + 1);
    hHad->SetLineWidth(2);
    hHad->SetLineStyle(1);

    hEM->SetLineColor(kRed + 1);
    hEM->SetLineWidth(2);
    hEM->SetLineStyle(2);

    hTotal->Draw("HIST");
    hHad->Draw("HIST SAME");
    hEM->Draw("HIST SAME");

    auto* leg = new TLegend(0.52, 0.70, 0.88, 0.88);
    leg->SetBorderSize(1);
    leg->SetFillColor(kWhite);
    leg->AddEntry(hTotal, Form("Total E_{dep} (Mean = %.3f MeV)", hTotal->GetMean()), "l");
    leg->AddEntry(hHad,   Form("Hadronic (Mean = %.3f MeV)", hHad->GetMean()), "l");
    leg->AddEntry(hEM,    Form("EM (Mean = %.3f MeV)", hEM->GetMean()), "l");
    leg->Draw();

    c2->Update();
    c2->SaveAs("edep_comparison.png");
}