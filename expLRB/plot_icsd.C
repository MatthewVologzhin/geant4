// plot_icsd.C
#include <TFile.h>
#include <TH1D.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <TStyle.h>
#include <iostream>

void plot_icsd(const char* filename = "lrb_vis_output.root")
{
    gStyle->SetOptStat(0); // Отключаем стандартный блок статистики

    TFile* file = TFile::Open(filename, "READ");
    if (!file || file->IsZombie()) {
        std::cerr << "Error: Cannot open file " << filename << std::endl;
        return;
    }

    // Извлекаем гистограммы
    auto* hTotal = file->Get<TH1D>("ICSD_Nucleus_Total");
    auto* hHad   = file->Get<TH1D>("ICSD_Nucleus_Had");
    auto* hEM    = file->Get<TH1D>("ICSD_Nucleus_EM");

    if (!hTotal || !hHad || !hEM) {
        std::cerr << "Error: Histograms not found in " << filename << std::endl;
        file->ls();
        return;
    }

    auto* c1 = new TCanvas("c_icsd", "ICSD Comparison", 900, 700);
    c1->SetGrid();
    c1->SetLogy(); // Логарифмический масштаб критически важен для ICSD

    // Настройка стилей
    hTotal->SetTitle("Ionization Cluster Size Distribution (ICSD) in Nucleus;Cluster Size (#nu);Entries");
    hTotal->SetLineColor(kBlack);
    hTotal->SetLineWidth(3);

    hHad->SetLineColor(kBlue + 1);
    hHad->SetLineWidth(2);
    hHad->SetLineStyle(1);

    hEM->SetLineColor(kRed + 1);
    hEM->SetLineWidth(2);
    hEM->SetLineStyle(2);

    // Отрисовка
    hTotal->Draw("HIST");
    hHad->Draw("HIST SAME");
    hEM->Draw("HIST SAME");

    // Легенда с выводом среднего размера кластера <nu>
    auto* leg = new TLegend(0.55, 0.70, 0.88, 0.88);
    leg->SetBorderSize(1);
    leg->SetFillColor(kWhite);
    leg->AddEntry(hTotal, Form("Total (#LT#nu#GT = %.2f)", hTotal->GetMean()), "l");
    leg->AddEntry(hHad,   Form("Hadronic (#LT#nu#GT = %.2f)", hHad->GetMean()), "l");
    leg->AddEntry(hEM,    Form("EM (#LT#nu#GT = %.2f)", hEM->GetMean()), "l");
    leg->Draw();

    c1->Update();
    c1->SaveAs("icsd_comparison.png");
}