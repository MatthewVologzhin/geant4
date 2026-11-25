#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <string>
#include <cmath>
#include <algorithm>

#include "TFile.h"
#include "TGraphErrors.h"
#include "TGraph.h"
#include "TCanvas.h"
#include "TLegend.h"
#include "TAxis.h"
#include "TStyle.h"
#include "TROOT.h"
#include "TColor.h"
#include "TF1.h"
#include "TMath.h"

// ----------------------------------------------------------------------
// 1. Таблица ICRU Report 90 (Table A.12: Alpha particles in liquid water)
//    Плотность rho = 0.998 g/cm3. Stot/rho переводится в LET (keV/um):
//    LET [keV/um] = (Stot/rho) [MeV cm2/g] * 0.998 * 0.1
// ----------------------------------------------------------------------
const std::vector<std::pair<Double_t, Double_t>> icru90_table = {
    {0.0010, 3.271E+02}, {0.0015, 3.306E+02}, {0.0020, 3.343E+02},
    {0.0030, 3.436E+02}, {0.0040, 3.546E+02}, {0.0050, 3.666E+02},
    {0.0060, 3.793E+02}, {0.0080, 4.049E+02}, {0.0100, 4.304E+02},
    {0.0150, 4.916E+02}, {0.0200, 5.483E+02}, {0.0300, 6.506E+02},
    {0.0400, 7.411E+02}, {0.0500, 8.229E+02}, {0.0600, 8.979E+02},
    {0.0800, 1.032E+03}, {0.1000, 1.150E+03}, {0.1500, 1.397E+03},
    {0.2000, 1.593E+03}, {0.3000, 1.881E+03}, {0.4000, 2.068E+03},
    {0.5000, 2.183E+03}, {0.6000, 2.244E+03}, {0.8000, 2.260E+03},
    {1.0000, 2.193E+03}, {1.5000, 1.879E+03}, {2.0000, 1.600E+03},
    {3.0000, 1.240E+03}, {4.0000, 1.022E+03}, {5.0000, 8.754E+02},
    {6.0000, 7.693E+02}, {8.0000, 6.243E+02}, {10.000, 5.294E+02},
    {15.000, 3.896E+02}, {20.000, 3.121E+02}, {30.000, 2.269E+02},
    {40.000, 1.803E+02}, {50.000, 1.507E+02}, {60.000, 1.301E+02},
    {80.000, 1.030E+02}, {100.00, 8.597E+01}, {150.00, 6.197E+01},
    {200.00, 4.925E+01}, {300.00, 3.587E+01}, {400.00, 2.886E+01},
    {500.00, 2.453E+01}, {600.00, 2.157E+01}, {800.00, 1.781E+01},
    {1000.0, 1.551E+01}
};

// Функция лог-лог линейной интерполяции LET по ICRU 90 [keV/um]
Double_t GetICRU90_LET(Double_t energyMeV) {
    constexpr Double_t rho = 0.998; // g/cm3
    if (energyMeV <= icru90_table.front().first) {
        return icru90_table.front().second * rho * 0.1;
    }
    if (energyMeV >= icru90_table.back().first) {
        return icru90_table.back().second * rho * 0.1;
    }

    for (size_t i = 0; i < icru90_table.size() - 1; ++i) {
        if (energyMeV >= icru90_table[i].first && energyMeV <= icru90_table[i+1].first) {
            Double_t logE1 = std::log(icru90_table[i].first);
            Double_t logE2 = std::log(icru90_table[i+1].first);
            Double_t logS1 = std::log(icru90_table[i].second);
            Double_t logS2 = std::log(icru90_table[i+1].second);

            Double_t logE = std::log(energyMeV);
            Double_t logS = logS1 + (logS2 - logS1) * (logE - logE1) / (logE2 - logE1);

            return std::exp(logS) * rho * 0.1; // keV/um
        }
    }
    return 0.0;
}

// ----------------------------------------------------------------------
// 2. Список моделированных энергий из макроса (27 точек)
// ----------------------------------------------------------------------
const std::vector<Double_t> simEnergies = {
    0.05, 0.08, 0.10, 0.15, 0.20, 0.30, 0.40,               // Low-E
    0.50, 0.65, 0.80, 1.00, 1.25, 1.50, 1.75,               // Bragg Peak
    2.0, 3.0, 5.0, 7.0, 10.0, 15.0, 20.0,                   // Mid-E
    30.0, 50.0, 80.0, 120.0, 200.0, 300.0, 400.0           // High-E
};

struct SpeciesInfo {
    std::vector<Double_t> fG;
    std::vector<Double_t> fGerr;
    std::vector<Double_t> fLET;
    std::vector<Double_t> fLETerr;
    std::string fName;
};

void plotG_LET_new(const char* filename = "Species.txt")
{
    gROOT->SetBatch(kTRUE);
    gStyle->SetOptStat(0);
    gStyle->SetPadTickX(1);
    gStyle->SetPadTickY(1);

    std::ifstream file(filename, std::ios::in);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open data file " << filename << std::endl;
        return;
    }

    std::map<Int_t, SpeciesInfo> speciesInfo;
    Int_t tag, runID = 0;
    Double_t LET, LET_sigma, Gvalue, Gvalue_sigma;
    std::string name, dummy, string_key, string_value;

    while (file >> dummy >> LET >> dummy >> LET_sigma) {
        std::getline(file, dummy);
        if (!std::getline(file, string_key)) break;
        std::istringstream key(string_key);

        if (!std::getline(file, string_value)) break;
        std::istringstream value(string_value);

        while (key >> name >> tag && value >> Gvalue >> Gvalue_sigma) {
            speciesInfo[tag].fName = name;
            speciesInfo[tag].fG.resize(runID + 1);
            speciesInfo[tag].fGerr.resize(runID + 1);
            speciesInfo[tag].fLET.resize(runID + 1);
            speciesInfo[tag].fLETerr.resize(runID + 1);

            speciesInfo[tag].fG[runID] = Gvalue;
            speciesInfo[tag].fGerr[runID] = Gvalue_sigma;
            speciesInfo[tag].fLET[runID] = LET;
            speciesInfo[tag].fLETerr[runID] = LET_sigma;
        }
        runID++;
    }
    file.close();

    if (speciesInfo.empty()) {
        std::cerr << "Error: No data parsed from " << filename << std::endl;
        return;
    }

    // ----------------- ЭКСПЕРИМЕНТАЛЬНЫЕ ДАННЫЕ -----------------
    std::map<std::string, std::vector<std::pair<Double_t, Double_t>>> expData;

    expData["H2O2"] = {
        {2.303610859682018, 0.6789473281342558},
        {25.89953165005833, 0.8438596388248462},
        {77.79753739882783, 0.9578947287700694},
        {92.41277586844313, 0.9912279891370915},
        {153.4186500389956, 1.0105263087447878}
    };

    expData["OH"] = {
        {2.2734483574053708, 2.585987309031949},
        {8.940321810947394,  1.893231363046946},
        {16.676183285434124, 1.2171946495114825},
        {20.150534343092,    0.9963158936707494},
        {26.91466431293175,  0.8708168157414999},
        {35.157847875713095, 0.74531718092217},
        {49.23633324927532,  0.6465510544296015}
    };

    expData["e_aq"] = {
        {2.2152353280707215, 2.417461100100777},
        {26.058909851098594, 1.210654587903572},
        {34.477163619513995, 1.500987851462824},
        {42.53173789440824,  1.308598232373987},
        {56.710864325290686, 1.095220649788746},
        {79.84677674930694,  0.8643533290975076},
        {109.8294258273216,  0.65097630205068},
        {146.44336694190608, 0.4725787512158478},
        {201.43294068202178, 0.3256632845102257}
    };

    // Структура для хранения 4 кривых на каждый радикал
    struct QuadSpeciesGraphs {
        TGraphErrors* grHigh_chem6; // 1. E >= 0.8 MeV (chem6 LET)
        TGraphErrors* grHigh_icru;  // 2. E >= 0.8 MeV (ICRU90 LET)
        TGraphErrors* grLow_chem6;  // 3. E < 0.8 MeV (chem6 LET)
        TGraphErrors* grLow_icru;   // 4. E < 0.8 MeV (ICRU90 LET)
        std::string name;
    };
    std::vector<QuadSpeciesGraphs> quadGraphs;

    // Пик Брэгга для альфы соответствует E ≈ 0.8 МэВ (индекс 9 в массиве simEnergies)
    const size_t peakIdx = 9;

    for (auto& pair : speciesInfo) {
        Int_t tagID = pair.first;
        SpeciesInfo& info = pair.second;
        size_t nTot = info.fG.size();
        if (nTot < 2) continue;

        // Вектора для 4 кривых
        std::vector<Double_t> xH_c6, yH_c6, exH_c6, eyH_c6;
        std::vector<Double_t> xH_ic, yH_ic, exH_ic, eyH_ic;
        std::vector<Double_t> xL_c6, yL_c6, exL_c6, eyL_c6;
        std::vector<Double_t> xL_ic, yL_ic, exL_ic, eyL_ic;

        for (size_t i = 0; i < nTot; ++i) {
            Double_t E = (i < simEnergies.size()) ? simEnergies[i] : 1.0;
            Double_t let_icru = GetICRU90_LET(E);

            if (E >= 0.8) {
                // High-E ветка (Pre-Bragg)
                if (info.fLET[i] > 0.1 && info.fLET[i] < 350.0) {
                    xH_c6.push_back(info.fLET[i]);
                    yH_c6.push_back(info.fG[i]);
                    exH_c6.push_back(info.fLETerr[i]);
                    eyH_c6.push_back(info.fGerr[i]);

                    xH_ic.push_back(let_icru);
                    yH_ic.push_back(info.fG[i]);
                    exH_ic.push_back(0.0);
                    eyH_ic.push_back(info.fGerr[i]);
                }
            } else {
                // Low-E ветка (Post-Bragg, E < 0.8 MeV)
                if (info.fLET[i] > 0.1 && info.fLET[i] < 350.0) {
                    xL_c6.push_back(info.fLET[i]);
                    yL_c6.push_back(info.fG[i]);
                    exL_c6.push_back(info.fLETerr[i]);
                    eyL_c6.push_back(info.fGerr[i]);

                    xL_ic.push_back(let_icru);
                    yL_ic.push_back(info.fG[i]);
                    exL_ic.push_back(0.0);
                    eyL_ic.push_back(info.fGerr[i]);
                }
            }
        }

        // 1. High-E (chem6 LET) - Красный
        TGraphErrors* grHigh_chem6 = new TGraphErrors(xH_c6.size(), xH_c6.data(), yH_c6.data(), exH_c6.data(), eyH_c6.data());
        grHigh_chem6->Sort();
        grHigh_chem6->SetName(Form("grHigh_c6_%s", info.fName.c_str()));
        grHigh_chem6->SetLineColor(kRed+1);
        grHigh_chem6->SetMarkerColor(kRed+1);
        grHigh_chem6->SetMarkerStyle(20);
        grHigh_chem6->SetMarkerSize(1.1);
        grHigh_chem6->SetLineWidth(2);

        // 2. High-E (ICRU90 LET) - Синий
        TGraphErrors* grHigh_icru = new TGraphErrors(xH_ic.size(), xH_ic.data(), yH_ic.data(), exH_ic.data(), eyH_ic.data());
        grHigh_icru->Sort();
        grHigh_icru->SetName(Form("grHigh_ic_%s", info.fName.c_str()));
        grHigh_icru->SetLineColor(kBlue+1);
        grHigh_icru->SetMarkerColor(kBlue+1);
        grHigh_icru->SetMarkerStyle(21);
        grHigh_icru->SetMarkerSize(1.1);
        grHigh_icru->SetLineWidth(2);

        // 3. Low-E (chem6 LET) - Оранжевый пунктир
        TGraphErrors* grLow_chem6 = new TGraphErrors(xL_c6.size(), xL_c6.data(), yL_c6.data(), exL_c6.data(), eyL_c6.data());
        grLow_chem6->Sort();
        grLow_chem6->SetName(Form("grLow_c6_%s", info.fName.c_str()));
        grLow_chem6->SetLineColor(kOrange+7);
        grLow_chem6->SetMarkerColor(kOrange+7);
        grLow_chem6->SetMarkerStyle(24);
        grLow_chem6->SetMarkerSize(1.0);
        grLow_chem6->SetLineWidth(2);
        grLow_chem6->SetLineStyle(7);

        // 4. Low-E (ICRU90 LET) - Голубой пунктир
        TGraphErrors* grLow_icru = new TGraphErrors(xL_ic.size(), xL_ic.data(), yL_ic.data(), exL_ic.data(), eyL_ic.data());
        grLow_icru->Sort();
        grLow_icru->SetName(Form("grLow_ic_%s", info.fName.c_str()));
        grLow_icru->SetLineColor(kAzure+7);
        grLow_icru->SetMarkerColor(kAzure+7);
        grLow_icru->SetMarkerStyle(25);
        grLow_icru->SetMarkerSize(1.0);
        grLow_icru->SetLineWidth(2);
        grLow_icru->SetLineStyle(7);

        // ----------------- ФИТИРОВАНИЕ 4 КРИВЫХ -----------------
        Double_t maxG = 0;
        for (auto val : yH_c6) if (val > maxG) maxG = val;

        if (maxG > 0.05) {
            // Фит High-E (chem6)
            if (grHigh_chem6->GetN() >= 4) {
                Double_t* x = grHigh_chem6->GetX();
                Double_t* y = grHigh_chem6->GetY();
                int N = grHigh_chem6->GetN();
                TF1* f1 = new TF1(Form("f1_%s", info.fName.c_str()),
                                  "[0] + [1] / (1.0 + TMath::Exp([3] * (TMath::Log(x) - TMath::Log([2]))))",
                                  x[0], x[N-1]);
                f1->SetParameters(y[N-1], y[0] - y[N-1], 40.0, 1.2);
                f1->SetParLimits(0, 0.0, 5.0);
                f1->SetLineColor(kRed+1);
                f1->SetLineWidth(2);
                grHigh_chem6->Fit(f1, "Q R B");
            }
            // Фит High-E (ICRU90)
            if (grHigh_icru->GetN() >= 4) {
                Double_t* x = grHigh_icru->GetX();
                Double_t* y = grHigh_icru->GetY();
                int N = grHigh_icru->GetN();
                TF1* f2 = new TF1(Form("f2_%s", info.fName.c_str()),
                                  "[0] + [1] / (1.0 + TMath::Exp([3] * (TMath::Log(x) - TMath::Log([2]))))",
                                  x[0], x[N-1]);
                f2->SetParameters(y[N-1], y[0] - y[N-1], 40.0, 1.2);
                f2->SetParLimits(0, 0.0, 5.0);
                f2->SetLineColor(kBlue+1);
                f2->SetLineWidth(2);
                grHigh_icru->Fit(f2, "Q R B");
            }
            // Фит Low-E (chem6)
            if (grLow_chem6->GetN() >= 3) {
                Double_t* x = grLow_chem6->GetX();
                Double_t* y = grLow_chem6->GetY();
                int N = grLow_chem6->GetN();
                TF1* f3 = new TF1(Form("f3_%s", info.fName.c_str()),
                                  "[0] + [1] / (1.0 + TMath::Exp([3] * (TMath::Log(x) - TMath::Log([2]))))",
                                  x[0], x[N-1]);
                f3->SetParameters(y[N-1], y[0] - y[N-1], 80.0, 1.2);
                f3->SetParLimits(0, 0.0, 5.0);
                f3->SetLineColor(kOrange+7);
                f3->SetLineWidth(2);
                f3->SetLineStyle(7);
                grLow_chem6->Fit(f3, "Q R B");
            }
            // Фит Low-E (ICRU90)
            if (grLow_icru->GetN() >= 3) {
                Double_t* x = grLow_icru->GetX();
                Double_t* y = grLow_icru->GetY();
                int N = grLow_icru->GetN();
                TF1* f4 = new TF1(Form("f4_%s", info.fName.c_str()),
                                  "[0] + [1] / (1.0 + TMath::Exp([3] * (TMath::Log(x) - TMath::Log([2]))))",
                                  x[0], x[N-1]);
                f4->SetParameters(y[N-1], y[0] - y[N-1], 80.0, 1.2);
                f4->SetParLimits(0, 0.0, 5.0);
                f4->SetLineColor(kAzure+7);
                f4->SetLineWidth(2);
                f4->SetLineStyle(7);
                grLow_icru->Fit(f4, "Q R B");
            }
        }

        quadGraphs.push_back({grHigh_chem6, grHigh_icru, grLow_chem6, grLow_icru, info.fName});
    }

    // ----------------- ОТРИСОВКА ГРАФИКОВ (ПО 4 КРИВЫХ) -----------------
    for (auto& qg : quadGraphs) {
        TCanvas* c = new TCanvas(Form("c_%s", qg.name.c_str()), qg.name.c_str(), 1000, 600);
        c->SetRightMargin(0.32); // Свободное место справа под легенду
        c->SetLogx();
        c->SetGrid();

        // Задаем заголовок осей
        qg.grHigh_chem6->GetXaxis()->SetTitle("LET (keV/#mum)");
        qg.grHigh_chem6->GetYaxis()->SetTitle("G value (molecules / 100 eV)");
        //qg.grHigh_chem6->GetYaxis()->SetRangeUser(0.0, 4.0);

        // Рисуем все 4 кривые
        qg.grHigh_chem6->Draw("AP");
        qg.grHigh_icru->Draw("P SAME");
        qg.grLow_chem6->Draw("P SAME");
        qg.grLow_icru->Draw("P SAME");

        // Вынесенная легенда справа
        TLegend* leg = new TLegend(0.69, 0.45, 0.98, 0.88);
        leg->SetBorderSize(0);
        leg->SetFillStyle(4050);
        leg->SetFillColorAlpha(kWhite, 0.5);
        leg->SetTextSize(0.026);

        leg->AddEntry(qg.grHigh_chem6, "High-E (E > 0.8 MeV, chem6)", "lp");
        leg->AddEntry(qg.grHigh_icru,  "High-E (E > 0.8 MeV, ICRU 90)", "lp");
        leg->AddEntry(qg.grLow_chem6,  "Low-E  (E < 0.8 MeV, chem6)", "lp");
        leg->AddEntry(qg.grLow_icru,   "Low-E  (E < 0.8 MeV, ICRU 90)", "lp");

        // Наложение экспериментальных точек
        for (const auto& expPair : expData) {
            if (qg.name.find(expPair.first) != std::string::npos) {
                std::vector<Double_t> expX, expY;
                for (const auto& pt : expPair.second) {
                    expX.push_back(pt.first);
                    expY.push_back(pt.second);
                }
                TGraph* grExp = new TGraph(expX.size(), expX.data(), expY.data());
                grExp->SetMarkerStyle(29);
                grExp->SetMarkerSize(1.9);
                grExp->SetMarkerColor(kBlack);
                grExp->Draw("P SAME");
                leg->AddEntry(grExp, "Exp (Yamashita et al.)", "p");
                break;
            }
        }

        leg->Draw();
        c->SaveAs(Form("output/plots/G_vs_LET_4curves_%s.png", qg.name.c_str()));
        delete leg;
        delete c;
    }

    // Сохранение в ROOT-файл
    TFile* outFile = TFile::Open("output/plots/G_vs_LET_ICRU90.root", "RECREATE");
    for (auto& qg : quadGraphs) {
        qg.grHigh_chem6->Write();
        qg.grHigh_icru->Write();
        qg.grLow_chem6->Write();
        qg.grLow_icru->Write();
    }
    outFile->Close();

    std::cout << "Successfully generated 4-curve validation plots with ICRU Report 90 data in output/plots/" << std::endl;
}