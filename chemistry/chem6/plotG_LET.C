#include <iostream>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <string>

#include "TFile.h"
#include "TGraphErrors.h"
#include "TCanvas.h"
#include "TLegend.h"
#include "TAxis.h"
#include "TStyle.h"
#include "TROOT.h"
#include "TColor.h"

struct SpeciesInfo {
    std::vector<Double_t> fG;
    std::vector<Double_t> fGerr;
    std::vector<Double_t> fLET;
    std::vector<Double_t> fLETerr;
    std::string fName;
};

void plotG_LET(const char* filename = "Species.txt")
{
    // Отключаем открытие окон GUI для работы на нодах / без X11
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

    // 1. Чтение данных из Species.txt
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

    // 2. Построение TGraphErrors
    std::vector<TGraphErrors*> graphs;
    std::vector<int> colors = {kBlack, kRed+1, kBlue+1, kGreen+2, kMagenta+1, kOrange+7, kCyan+2, kViolet-4, kAzure+1};

    int colorIdx = 0;
    for (auto& pair : speciesInfo) {
        Int_t tagID = pair.first;
        SpeciesInfo& info = pair.second;

        TGraphErrors* gr = new TGraphErrors(info.fG.size(),
                                            info.fLET.data(),
                                            info.fG.data(),
                                            info.fLETerr.data(),
                                            info.fGerr.data());
        gr->Sort();

        gr->SetName(Form("gr_%s", info.fName.c_str()));
        gr->SetTitle(info.fName.c_str());

        int col = colors[colorIdx % colors.size()];
        gr->SetLineColor(col);
        gr->SetMarkerColor(col);
        gr->SetMarkerStyle(20 + (tagID % 10));
        gr->SetMarkerSize(1.0);
        gr->SetLineWidth(2);

        gr->GetXaxis()->SetTitle("LET (keV/#mum)");
        gr->GetYaxis()->SetTitle("G value (molecules / 100 eV)");

        graphs.push_back(gr);
        colorIdx++;
    }

    // 3. Сводный график всех компонентов (G vs LET)
    TCanvas* cAll = new TCanvas("cAll", "G values vs LET", 900, 600);
    cAll->SetLogx();
    cAll->SetGrid();

    TLegend* leg = new TLegend(0.78, 0.58, 0.95, 0.88);
    leg->SetBorderSize(1);
    leg->SetFillColor(kWhite);

    bool first = true;
    for (auto* gr : graphs) {
        if (first) {
            gr->GetYaxis()->SetRangeUser(0.0, 4.5);
            gr->Draw("ALP");
            first = false;
        } else {
            gr->Draw("LP SAME");
        }
        leg->AddEntry(gr, gr->GetTitle(), "lep");
    }
    leg->Draw();

    cAll->SaveAs("output/plots/G_vs_LET_all.png");
    cAll->SaveAs("output/plots/G_vs_LET_all.pdf");

    // 4. Одиночные графики для каждого вещества
    for (auto* gr : graphs) {
        TCanvas* cSingle = new TCanvas(Form("c_%s", gr->GetTitle()), gr->GetTitle(), 800, 600);
        cSingle->SetLogx();
        cSingle->SetGrid();
        gr->Draw("AP");
        cSingle->SaveAs(Form("output/plots/G_vs_LET_%s.png", gr->GetTitle()));
        delete cSingle;
    }

    // 5. Сохранение графиков в ROOT-файл
    TFile* outFile = TFile::Open("output/plots/G_vs_LET.root", "RECREATE");
    for (auto* gr : graphs) {
        gr->Write();
    }
    outFile->Close();

    std::cout << "Successfully generated:" << std::endl;
    std::cout << " -> output/plots/G_vs_LET_all.png / .pdf" << std::endl;
    std::cout << " -> Individual output/plots/G_vs_LET_<species>.png files" << std::endl;
    std::cout << " -> output/plots/G_vs_LET.root" << std::endl;
}