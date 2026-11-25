#include <iostream>
#include <map>
#include <vector>
#include <string>
#include <cmath>

#include "TFile.h"
#include "TTree.h"
#include "TGraphErrors.h"
#include "TCanvas.h"
#include "TLegend.h"
#include "TAxis.h"
#include "TStyle.h"
#include "TROOT.h"
#include "TColor.h"

struct SpeciesInfoAOS {
    Int_t fNEvent = 0;
    Int_t fNumber = 0;
    Double_t fG = 0.;
    Double_t fG2 = 0.;
    std::string fName = "";
};

struct SpeciesInfoSOA {
    std::vector<Double_t> fG;
    std::vector<Double_t> fGerr;
    std::vector<Double_t> fTime;
    std::string fName;
};

void plotG_time(const char* filename = "Species0.root")
{
    // Отключаем интерактивные всплывающие окна
    gROOT->SetBatch(kTRUE);

    gStyle->SetOptStat(0);
    gStyle->SetPadTickX(1);
    gStyle->SetPadTickY(1);

    TFile* file = TFile::Open(filename);
    if (!file || file->IsZombie()) {
        std::cerr << "Error: Cannot open file " << filename << std::endl;
        return;
    }

    TTree* tree = (TTree*)file->Get("species");
    if (!tree) {
        std::cerr << "Error: Tree 'species' not found in " << filename << std::endl;
        file->Close();
        return;
    }

    Double_t timeA, sumG, sumG2;
    Int_t speciesID, number, nEvent;
    char speciesName[500];

    tree->SetBranchAddress("speciesID", &speciesID);
    tree->SetBranchAddress("number", &number);
    tree->SetBranchAddress("nEvent", &nEvent);
    tree->SetBranchAddress("speciesName", speciesName);
    tree->SetBranchAddress("time", &timeA);
    tree->SetBranchAddress("sumG", &sumG);
    tree->SetBranchAddress("sumG2", &sumG2);

    Long64_t nentries = tree->GetEntries();
    if (nentries == 0) {
        std::cerr << "No entries found in tree!" << std::endl;
        file->Close();
        return;
    }

    // 1. Сбор статистики
    std::map<int, std::map<double, SpeciesInfoAOS>> speciesTimeInfo;

    for (Long64_t j = 0; j < nentries; j++) {
        tree->GetEntry(j);
        SpeciesInfoAOS& infoAOS = speciesTimeInfo[speciesID][timeA];
        infoAOS.fNumber += number;
        infoAOS.fG += sumG;
        infoAOS.fG2 += sumG2;
        infoAOS.fNEvent += nEvent;
        infoAOS.fName = speciesName;
    }

    // 2. Расчет G-значений и погрешностей
    std::map<Int_t, SpeciesInfoSOA> speciesInfo;
    std::vector<TGraphErrors*> graphs;

    // Палитра контрастных цветов
    std::vector<int> colors = {kBlack, kRed+1, kBlue+1, kGreen+2, kMagenta+1, kOrange+7, kCyan+2, kViolet-4, kAzure+1};

    for (auto& pairID : speciesTimeInfo) {
        const Int_t sID = pairID.first;
        SpeciesInfoSOA& info = speciesInfo[sID];

        auto it2 = pairID.second.begin();
        info.fName = it2->second.fName;

        const size_t nPoints = pairID.second.size();
        info.fG.resize(nPoints);
        info.fGerr.resize(nPoints);
        info.fTime.resize(nPoints);

        int i2 = 0;
        for (auto it = pairID.second.begin(); it != pairID.second.end(); ++it, ++i2) {
            SpeciesInfoAOS& infoAOS = it->second;
            Double_t nEv = infoAOS.fNEvent;
            Double_t meanG = (nEv > 0) ? (infoAOS.fG / nEv) : 0.;
            Double_t errG = 0.;
            if (nEv > 1) {
                Double_t var = (infoAOS.fG2 / nEv - meanG * meanG) / (nEv - 1);
                errG = (var > 0) ? std::sqrt(var) : 0.;
            }

            info.fG[i2] = meanG;
            info.fGerr[i2] = errG;
            info.fTime[i2] = it->first; // Время в нс
        }

        TGraphErrors* gr = new TGraphErrors(nPoints, info.fTime.data(), info.fG.data(), nullptr, info.fGerr.data());
        gr->SetName(Form("g_%s", info.fName.c_str()));
        gr->SetTitle(info.fName.c_str());

        int col = colors[sID % colors.size()];
        gr->SetLineColor(col);
        gr->SetMarkerColor(col);
        gr->SetMarkerStyle(20 + (sID % 10));
        gr->SetMarkerSize(0.8);
        gr->SetLineWidth(2);

        gr->GetXaxis()->SetTitle("Time (ns)");
        gr->GetYaxis()->SetTitle("G value (molecules / 100 eV)");

        graphs.push_back(gr);
    }

    // 3. Сохранение сводного графика (все частицы на одном холсте)
    TCanvas* cAll = new TCanvas("cAll", "All Species G(t)", 900, 600);
    cAll->SetLogx();
    cAll->SetLogy();
    cAll->SetGrid();

    TLegend* leg = new TLegend(0.75, 0.60, 0.93, 0.88);
    leg->SetBorderSize(1);
    leg->SetFillColor(kWhite);

    bool first = true;
    for (auto* gr : graphs) {
        if (first) {
            gr->GetYaxis()->SetRangeUser(0.0, 4.5); // Стандартный диапазон G-значений
            gr->Draw("ALP");
            first = false;
        } else {
            gr->Draw("LP SAME");
        }
        leg->AddEntry(gr, gr->GetTitle(), "lep");
    }
    leg->Draw();
    cAll->SaveAs("G_all_species.png");
    cAll->SaveAs("G_all_species.pdf");

    // 4. Сохранение каждого графика в отдельный файл
    for (auto* gr : graphs) {
        TCanvas* cSingle = new TCanvas(Form("c_%s", gr->GetTitle()), gr->GetTitle(), 800, 600);
        cSingle->SetLogx();
        cSingle->SetLogy();
        cSingle->SetGrid();
        gr->Draw("ALP");
        cSingle->SaveAs(Form("G_%s.png", gr->GetTitle()));
        delete cSingle;
    }

    // 5. Сохранение графиков в ROOT-файл для дальнейшего анализа
    TFile* outFile = TFile::Open("G_graphs.root", "RECREATE");
    for (auto* gr : graphs) {
        gr->Write();
    }
    outFile->Close();

    std::cout << "Successfully generated:" << std::endl;
    std::cout << " -> G_all_species.png / .pdf" << std::endl;
    std::cout << " -> Individual G_<species>.png files" << std::endl;
    std::cout << " -> G_graphs.root" << std::endl;

    file->Close();
}