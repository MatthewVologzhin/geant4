#include <TFile.h>
#include <TTree.h>
#include <TTreeReader.h>
#include <TTreeReaderValue.h>
#include <TH1D.h>
#include <TCanvas.h>
#include <TLegend.h>
#include <TPaveText.h>
#include <TStyle.h>
#include <TMath.h>
#include <TSystem.h>

#include <iostream>
#include <iomanip>
#include <vector>
#include <string>
#include <algorithm>

namespace BioConstants {
    constexpr double MeV_to_Joule = 1.602176634e-13;
    constexpr double um_to_cm = 1.0e-4;
    constexpr double BeamArea_cm2 = 1.4 * 1.4; // 1.96 cm2
    constexpr double NucleusRadius_um = 3.1;
    constexpr double NucleusDensity_g_cm3 = 1.0;
    constexpr double MeanChord_um = (4.0 / 3.0) * NucleusRadius_um; // 4.133 um

    inline double GetSingleNucleusMassKg() {
        const double r_cm = NucleusRadius_um * um_to_cm;
        const double vol_cm3 = (4.0 / 3.0) * TMath::Pi() * std::pow(r_cm, 3);
        return vol_cm3 * NucleusDensity_g_cm3 * 1.0e-3;
    }
}

void analyze_lrb(const char* inputFileName = "lrb_test_output.root")
{
    gStyle->SetOptStat(0);
    gStyle->SetFrameLineWidth(2);
    gStyle->SetTitleFont(42, "XYZ");
    gStyle->SetLabelFont(42, "XYZ");
    gStyle->SetTitleSize(0.045, "XYZ");
    gStyle->SetLabelSize(0.040, "XYZ");
    gStyle->SetPadTickX(1);
    gStyle->SetPadTickY(1);
    gStyle->SetPadLeftMargin(0.13);
    gStyle->SetPadBottomMargin(0.12);

    TString resolvedPath = inputFileName;
    if (gSystem->AccessPathName(resolvedPath.Data())) {
        if (!gSystem->AccessPathName("test.root")) resolvedPath = "test.root";
        else {
            std::cerr << "[ERROR] File '" << inputFileName << "' not found!" << std::endl;
            return;
        }
    }

    auto* fIn = TFile::Open(resolvedPath.Data(), "READ");
    if (!fIn || fIn->IsZombie()) return;

    auto* tree = fIn->Get<TTree>("BioData");
    if (!tree) {
        std::cerr << "[ERROR] Tree 'BioData' not found!" << std::endl;
        return;
    }

    TTreeReader reader("BioData", fIn);
    TTreeReaderValue<Double_t> r_eTot(reader, "Edep_Total");
    TTreeReaderValue<Double_t> r_eNe(reader,  "Edep_PrimaryNe20");
    TTreeReaderValue<Double_t> r_eP(reader,   "Edep_Proton");
    TTreeReaderValue<Double_t> r_eD(reader,   "Edep_Deuteron");
    TTreeReaderValue<Double_t> r_eT(reader,   "Edep_Triton");
    TTreeReaderValue<Double_t> r_eHe3(reader, "Edep_Helium3");
    TTreeReaderValue<Double_t> r_eA(reader,   "Edep_Alpha");
    TTreeReaderValue<Double_t> r_eIon(reader, "Edep_HeavyIon");
    TTreeReaderValue<Double_t> r_eOth(reader, "Edep_Other");

    TTreeReaderValue<Int_t> r_iTot(reader, "Ions_Total");
    TTreeReaderValue<Int_t> r_iNe(reader,  "Ions_PrimaryNe20");
    TTreeReaderValue<Int_t> r_iP(reader,   "Ions_Proton");
    TTreeReaderValue<Int_t> r_iD(reader,   "Ions_Deuteron");
    TTreeReaderValue<Int_t> r_iT(reader,   "Ions_Triton");
    TTreeReaderValue<Int_t> r_iHe3(reader, "Ions_Helium3");
    TTreeReaderValue<Int_t> r_iA(reader,   "Ions_Alpha");
    TTreeReaderValue<Int_t> r_iIon(reader, "Ions_HeavyIon");
    TTreeReaderValue<Int_t> r_iOth(reader, "Ions_Other");
    TTreeReaderValue<Int_t> r_hits(reader, "HitTargetCount");

    const long long totalEvents = tree->GetEntries();
    if (totalEvents == 0) return;

    // Автоматическое определение максимального размера кластера
    const double maxIons = std::max(5000.0, tree->GetMaximum("Ions_Total") * 1.15);

    // Гистограммы ICSD: 200 бинов от 1 до maxIons
    auto* h_icsd_tot = new TH1D("h_icsd_tot", "Species-Resolved ICSD;Cluster Size #nu [ion pairs];Frequency per Event", 200, 1.0, maxIons);
    auto* h_icsd_p   = new TH1D("h_icsd_p",   "Protons",  200, 1.0, maxIons);
    auto* h_icsd_d   = new TH1D("h_icsd_d",   "Deuterons",200, 1.0, maxIons);
    auto* h_icsd_t   = new TH1D("h_icsd_t",   "Tritons",  200, 1.0, maxIons);
    auto* h_icsd_a   = new TH1D("h_icsd_a",   "Alphas",   200, 1.0, maxIons);
    auto* h_icsd_ion = new TH1D("h_icsd_ion", "Heavy Fragments (Z#geq3)", 200, 1.0, maxIons);

    // Спектр линейной энергии y
    auto* h_lineal_y = new TH1D("h_lineal_y", "Lineal Energy Spectrum;Lineal Energy y [keV/#mum];y #upoint d(y)", 100, 0.1, 1000.0);

    double sumE_Tot = 0, sumE_Ne = 0, sumE_P = 0, sumE_D = 0, sumE_T = 0, sumE_He3 = 0, sumE_A = 0, sumE_Ion = 0, sumE_Oth = 0;
    long long totalHits = 0;

    while (reader.Next()) {
        const double eTot = *r_eTot;
        const int nHits   = *r_hits;

        sumE_Tot += eTot;
        sumE_Ne  += *r_eNe;
        sumE_P   += *r_eP;
        sumE_D   += *r_eD;
        sumE_T   += *r_eT;
        sumE_He3 += *r_eHe3;
        sumE_A   += *r_eA;
        sumE_Ion += *r_eIon;
        sumE_Oth += *r_eOth;

        if (nHits > 0 && eTot > 0.0) {
            totalHits += nHits;
            const int iTot = *r_iTot;
            if (iTot > 0) h_icsd_tot->Fill(iTot);
            if (*r_iP > 0)   h_icsd_p->Fill(*r_iP);
            if (*r_iD > 0)   h_icsd_d->Fill(*r_iD);
            if (*r_iT > 0)   h_icsd_t->Fill(*r_iT);
            if (*r_iA > 0)   h_icsd_a->Fill(*r_iA);
            if (*r_iIon > 0) h_icsd_ion->Fill(*r_iIon);

            const double y = (eTot * 1.0e3) / (nHits * BioConstants::MeanChord_um);
            h_lineal_y->Fill(y, y); // Взвешивание y*d(y)
        }
    }

    // НОРМИРОВКА НА ЧИСЛО СОБЫТИЙ: P(nu) = N_bin / (N_events * BinWidth)
    auto NormalizeToEvents = [totalEvents](TH1D* h) {
        if (!h || totalEvents == 0) return;
        const double binWidth = h->GetBinWidth(1);
        h->Scale(1.0 / (totalEvents * binWidth));
    };

    NormalizeToEvents(h_icsd_tot);
    NormalizeToEvents(h_icsd_p);
    NormalizeToEvents(h_icsd_d);
    NormalizeToEvents(h_icsd_t);
    NormalizeToEvents(h_icsd_a);
    NormalizeToEvents(h_icsd_ion);

    // Дозиметрический баланс
    const double m_single_kg = BioConstants::GetSingleNucleusMassKg();
    const double m_total_kg = (totalHits > 0) ? (totalHits * m_single_kg) : m_single_kg;
    const double z_mean_Gy = (sumE_Tot * BioConstants::MeV_to_Joule) / m_total_kg;

    // ========================================================================
    // CANVAS 1: SPECIES-RESOLVED NANODOSIMETRY
    // ========================================================================
    auto* c1 = new TCanvas("c1", "Species-Resolved Nanodosimetry", 1600, 950);
    c1->Divide(2, 2);

    // Pad 1: Полный ICSD по всем осколкам
    c1->cd(1);
    gPad->SetLogy(1);
    gPad->SetLogx(1);
    h_icsd_tot->SetLineColor(kBlack);
    h_icsd_tot->SetLineWidth(3);
    h_icsd_tot->GetXaxis()->SetRangeUser(1.0, maxIons);
    h_icsd_tot->Draw("HIST");

    h_icsd_p->SetLineColor(kBlue);
    h_icsd_p->SetLineWidth(2);
    h_icsd_p->Draw("HIST SAME");

    h_icsd_a->SetLineColor(kOrange + 1);
    h_icsd_a->SetLineWidth(2);
    h_icsd_a->Draw("HIST SAME");

    h_icsd_d->SetLineColor(kCyan + 2);
    h_icsd_d->SetLineWidth(2);
    h_icsd_d->Draw("HIST SAME");

    h_icsd_t->SetLineColor(kTeal + 2);
    h_icsd_t->SetLineWidth(2);
    h_icsd_t->Draw("HIST SAME");

    h_icsd_ion->SetLineColor(kMagenta + 2);
    h_icsd_ion->SetLineWidth(2);
    h_icsd_ion->Draw("HIST SAME");

    auto* l1 = new TLegend(0.52, 0.58, 0.88, 0.88);
    l1->SetBorderSize(0);
    l1->SetFillStyle(0);
    l1->AddEntry(h_icsd_tot, "Total ICSD", "l");
    l1->AddEntry(h_icsd_p,   "Protons (1H)", "l");
    l1->AddEntry(h_icsd_a,   "Alphas (4He)", "l");
    l1->AddEntry(h_icsd_d,   "Deuterons (2H)", "l");
    l1->AddEntry(h_icsd_t,   "Tritons (3H)", "l");
    l1->AddEntry(h_icsd_ion, "Heavy (Z#geq3)", "l");
    l1->Draw();

    // Pad 2: Кумулятивное распределение повреждений F_k (до maxIons)
    c1->cd(2);
    gPad->SetLogy(1);
    gPad->SetLogx(1);
    auto* h_cum = static_cast<TH1D*>(h_icsd_tot->Clone("h_cum"));
    h_cum->SetTitle("Cumulative Damage Probability F_{k};Cluster Size Threshold k;P(#nu #geq k)");
    h_cum->Reset();
    const double totInt = h_icsd_tot->Integral(1, h_icsd_tot->GetNbinsX());
    for (int b = 1; b <= h_icsd_tot->GetNbinsX(); ++b) {
        h_cum->SetBinContent(b, h_icsd_tot->Integral(b, h_icsd_tot->GetNbinsX()) / totInt);
    }
    h_cum->SetLineColor(kRed + 1);
    h_cum->SetLineWidth(3);
    h_cum->GetXaxis()->SetRangeUser(1.0, maxIons);
    h_cum->GetYaxis()->SetRangeUser(1e-4, 1.05);
    h_cum->Draw("HIST");

    // Pad 3: Микродозиметрический спектр y * d(y)
    c1->cd(3);
    gPad->SetLogx(1);
    h_lineal_y->SetLineColor(kViolet + 1);
    h_lineal_y->SetLineWidth(2);
    h_lineal_y->Draw("HIST");

    // Pad 4: Сводка ядерной фрагментации
    c1->cd(4);
    auto* pt = new TPaveText(0.10, 0.12, 0.90, 0.88, "NDC");
    pt->SetBorderSize(1);
    pt->SetFillColor(kWhite);
    pt->SetTextAlign(12);
    pt->SetTextFont(42);
    pt->AddText("FRAGMENTATION CONTRIBUTION TO DOSE");
    pt->AddLine(0.0, 0.80, 1.0, 0.80);
    pt->AddText(Form("Hit Cell Specific Energy <z>: %.4e Gy", z_mean_Gy));
    if (sumE_Tot > 0.0) {
        pt->AddText(Form("Primary ^{20}Ne Beam:       %.2f %%", (sumE_Ne  / sumE_Tot) * 100.0));
        pt->AddText(Form("Protons (1H):              %.2f %%", (sumE_P   / sumE_Tot) * 100.0));
        pt->AddText(Form("Alphas (4He):              %.2f %%", (sumE_A   / sumE_Tot) * 100.0));
        pt->AddText(Form("Deuterons (2H):            %.2f %%", (sumE_D   / sumE_Tot) * 100.0));
        pt->AddText(Form("Tritons (3H):              %.2f %%", (sumE_T   / sumE_Tot) * 100.0));
        pt->AddText(Form("Helium-3 (3He):            %.2f %%", (sumE_He3 / sumE_Tot) * 100.0));
        pt->AddText(Form("Heavy Ions (Li-F, Z#geq3):   %.2f %%", (sumE_Ion / sumE_Tot) * 100.0));
        pt->AddText(Form("Neutrons & Other:          %.2f %%", ((sumE_Oth) / sumE_Tot) * 100.0));
    }
    pt->Draw();

    c1->SaveAs("Species_Nanodosimetry_LRB.pdf");
    c1->SaveAs("Species_Nanodosimetry_LRB.png");

    std::cout << "[SUCCESS] Analysis completed. Figures saved to Species_Nanodosimetry_LRB (.pdf, .png)\n";
}