#include <TGraphErrors.h>
#include <TCanvas.h>
#include <TAxis.h>
#include <TF1.h>
#include <TLegend.h>
#include <TStyle.h>
#include <TPad.h>
#include <TLatex.h>
#include <TMath.h>
#include <TH1F.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <iostream>

using namespace std;

// --- Структуры данных (без изменений, кроме удаления лишних полей) ---
struct SpeciesPlot {
    vector<Int_t> tags;
    TString       beautyName;
    Double_t      yMin, yMax;
    Double_t      xMin, xMax;
    Bool_t        logY;
};

struct SpeciesInfo {
    vector<TString>  fName;
    vector<Double_t> fG, fGerr, fLET, fLETerr;
};

SpeciesInfo operator+(const SpeciesInfo& a, const SpeciesInfo& b){
    if (a.fG.size() != b.fG.size()) return a; // Защита от ошибок
    SpeciesInfo result;
    const Int_t n = a.fG.size();
    for (Int_t i=0; i<n; i++){
        result.fG.push_back(a.fG[i] + b.fG[i]);
        result.fGerr.push_back(TMath::Sqrt(a.fGerr[i]*a.fGerr[i] + b.fGerr[i]*b.fGerr[i]));
        result.fLET.push_back(a.fLET[i]);
        result.fLETerr.push_back(a.fLETerr[i]);
    }
    result.fName.push_back(a.fName[0] + " + " + b.fName[0]);
    return result;
}

map<Int_t, SpeciesInfo> ParseDataFile(const string& filename) {
    map<Int_t, SpeciesInfo> speciesData;
    ifstream file(filename.c_str());
    if (!file.is_open()) return speciesData; // Возвращаем пустую мапу, если файла нет

    string dummy, string_key, string_val, name;
    Double_t LET, LET_err, G, Gerr;
    Int_t tag;

    while (file >> dummy >> LET >> dummy >> LET_err) {
        getline(file, dummy);
        if (!getline(file, string_key) || !getline(file, string_val)) break;

        istringstream k_stream(string_key), v_stream(string_val);
        while (k_stream >> name >> tag && v_stream >> G >> Gerr) {
            speciesData[tag].fName.push_back(name);
            speciesData[tag].fG.push_back(G);
            speciesData[tag].fGerr.push_back(Gerr);
            speciesData[tag].fLET.push_back(LET);
            speciesData[tag].fLETerr.push_back(LET_err);
        }
    }
    return speciesData;
}

// --- Основная функция ---
void plotG_LET() {
    // 1. Настройки отрисовки и списки параметров
    gStyle->SetOptStat(0);
    gStyle->SetCanvasPreferGL(kTRUE);
    
    vector<TString> particles = {"proton", "alpha", "ion-12", "ion-20"};
    vector<TString> pLabels   = {"^{1}H^{+}", "^{4}He^{2+}", "^{12}C^{6+}", "^{20}Ne^{10+}"};
    
    vector<TString> physLists = {"option2", "option4", "option6", "option8"};
    
    // 4 состояния: Названия для файлов, Имена в легенде, Цвет, Тип маркера
    struct MiConfig { TString fileTag; TString legLabel; Color_t col; Style_t mark; };
    vector<MiConfig> miStates = {
        {"MI_P-OFF_C-OFF", "Phys OFF | Chem OFF", kBlack,   20}, // Круг
        {"MI_P-OFF_C-ON",  "Phys OFF | Chem ON",  kRed,     24}, // Откр. круг
        {"MI_P-ON_C-OFF",  "Phys ON  | Chem OFF", kBlue,    21}, // Квадрат
        {"MI_P-ON_C-ON",   "Phys ON  | Chem ON",  kGreen+2, 22}  // Треугольник
    };

    vector<SpeciesPlot> plotsToValidate = {
        {{11}, "O_{2}",                 0.001, 1.5,  0.1, 5000.0, kTRUE}, 
        {{7, 12}, "HO_{2}^{.} / O_{2}^{-}", 0.01,  0.5,  0.1, 5000.0, kTRUE},
        {{6}, "H_{2}O_{2}",             0.6,   1.5,  0.1, 5000.0, kFALSE}, 
        {{1}, "^{.}OH",                 0.05,  10.0, 0.1, 5000.0, kTRUE}
    };

    TString pdfFile = "results/plots/Validation_All_States.pdf";
    int pageIdx = 0;
    int totalPages = particles.size() * plotsToValidate.size();

    // 2. Цикл генерации страниц
    for (size_t p = 0; p < particles.size(); p++) {
        for (size_t sp = 0; sp < plotsToValidate.size(); sp++) {
            const auto& plot = plotsToValidate[sp];
            
            TCanvas* c1 = new TCanvas("c1", "Validation", 1200, 900);
            c1->Divide(2, 2); // 4 панели под 4 физических листа
            
            vector<TGraphErrors*> graphsToKeep; // Храним указатели, чтобы они не удалились

            for (size_t ph = 0; ph < physLists.size(); ph++) {
                c1->cd(ph + 1);
                gPad->SetLeftMargin(0.12); gPad->SetBottomMargin(0.12);
                gPad->SetLogx(); if (plot.logY) gPad->SetLogy();
                gPad->SetGrid();

                TH1F* frame = gPad->DrawFrame(plot.xMin, plot.yMin, plot.xMax, plot.yMax);
                frame->GetXaxis()->SetTitle("LET [keV/#mum]");
                frame->GetYaxis()->SetTitle(Form("G(%s) [molecules / 100 eV]", plot.beautyName.Data()));
                frame->SetTitle(Form("DNA %s", physLists[ph].Data()));

                TLegend* leg = new TLegend(0.40, 0.70, 0.88, 0.88);
                leg->SetBorderSize(1); leg->SetTextSize(0.035);

                // Отрисовка 4-х состояний MI в текущем физическом листе
                for (const auto& mi : miStates) {
                    TString fileName = Form("results/data/%s_G4EmDNAPhysics_%s_G4EmDNAChemistry_option3_IRT_%s.txt", 
                                            particles[p].Data(), physLists[ph].Data(), mi.fileTag.Data());
                    
                    auto data = ParseDataFile(fileName.Data());
                    if (data.empty()) continue; // Пропускаем, если файла нет

                    SpeciesInfo spec;
                    bool first = true;
                    for (int tag : plot.tags) {
                        if (data.find(tag) == data.end()) continue;
                        if (first) { spec = data[tag]; first = false; } 
                        else       { spec = spec + data[tag]; }
                    }
                    if (spec.fG.empty()) continue;

                    TGraphErrors* gr = new TGraphErrors(spec.fG.size(), spec.fLET.data(), spec.fG.data(), 
                                                        spec.fLETerr.data(), spec.fGerr.data());
                    gr->SetMarkerStyle(mi.mark);
                    gr->SetMarkerColor(mi.col);
                    gr->SetLineColor(mi.col);
                    gr->SetMarkerSize(1.2);
                    gr->Draw("P SAME");
                    
                    graphsToKeep.push_back(gr);
                    leg->AddEntry(gr, mi.legLabel, "p");
                }
                leg->Draw();
            }

            // Заголовок на всю страницу
            c1->cd(0);
            TLatex* tex = new TLatex(0.5, 0.96, Form("Particle: %s   |   Species: %s", pLabels[p].Data(), plot.beautyName.Data()));
            tex->SetNDC(); tex->SetTextAlign(22); tex->SetTextSize(0.035); tex->Draw();

            // Сохранение в PDF
            TString printOpt = pdfFile;
            if (pageIdx == 0) printOpt += "(";
            else if (pageIdx == totalPages - 1) printOpt += ")";
            c1->Print(printOpt);
            
            delete c1;
            pageIdx++;
        }
    }
}