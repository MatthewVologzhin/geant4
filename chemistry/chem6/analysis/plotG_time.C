#include <TFile.h>
#include <TTree.h>
#include <TGraphErrors.h>
#include <TCanvas.h>
#include <TAxis.h>
#include <TLegend.h>
#include <TStyle.h>
#include <TLatex.h>
#include <TMath.h>
#include <iostream>
#include <vector>
#include <map>

using namespace std;

// --- Структуры данных ---
struct DatasetConfig {
    string  filePath;
    string  legendLabel;
    Color_t color;
    Style_t markerStyle;
};

struct SpeciesPlotTime {
    vector<Int_t> tags;       // Теги видов (напр. {11} для O2)
    TString       beautyName; // Название для графиков
    Double_t      yMin, yMax;
    Bool_t        logY;
};

// Внутренняя структура для аккумуляции данных из TTree
struct Accumulator {
    Accumulator() : nEvent(0), sumG(0.), sumG2(0.) {}
    Int_t    nEvent;
    Double_t sumG;
    Double_t sumG2;
};

// Функция парсинга конкретного вида из конкретного ROOT файла
TGraphErrors* ExtractTimeData(const string& filename, const vector<Int_t>& targetTags, Color_t color, Style_t marker) {
    TFile* file = TFile::Open(filename.c_str(), "READ");
    if (!file || file->IsZombie()) {
        cerr << "Warning: Cannot open file " << filename << endl;
        return nullptr;
    }

    TTree* tree = (TTree*)file->Get("species");
    if (!tree) {
        cerr << "Warning: No 'species' tree in " << filename << endl;
        file->Close();
        return nullptr;
    }

    Int_t speciesID, nEvent;
    Double_t timeA, sumG, sumG2;
    tree->SetBranchAddress("speciesID", &speciesID);
    tree->SetBranchAddress("nEvent", &nEvent);
    tree->SetBranchAddress("time", &timeA);
    tree->SetBranchAddress("sumG", &sumG);
    tree->SetBranchAddress("sumG2", &sumG2);

    // В Geant4-DNA MT данные могут быть разбиты по потокам для одного и того же time bin.
    // Нам нужно их саккумулировать. Карта: time -> Accumulator
    map<Double_t, Accumulator> aggregatedData;

    Long64_t nentries = tree->GetEntries();
    for (Long64_t j = 0; j < nentries; j++) {
        tree->GetEntry(j);
        
        // Проверяем, относится ли прочитанный вид к нашему списку тегов
        bool match = false;
        for (int tag : targetTags) {
            if (speciesID == tag) { match = true; break; }
        }
        if (!match) continue;

        // Если это наш вид, добавляем его в сумматор для данного момента времени
        Accumulator& acc = aggregatedData[timeA];
        acc.nEvent += nEvent;
        acc.sumG   += sumG;
        acc.sumG2  += sumG2;
    }

    file->Close();
    delete file;

    if (aggregatedData.empty()) return nullptr;

    vector<Double_t> vTime, vG, vGerr;
    for (auto const& [t_val, acc] : aggregatedData) {
        if (acc.nEvent == 0) continue;
        
        Double_t meanG = acc.sumG / acc.nEvent;
        Double_t gErr = 0.0;
        if (acc.nEvent > 1) {
            // Формула стандартного отклонения
            Double_t variance = (acc.sumG2 / acc.nEvent - meanG * meanG) / (acc.nEvent - 1);
            if (variance > 0) gErr = sqrt(variance);
        }
        
        // Geant4 обычно хранит время в пикосекундах (ps). 
        // Статья требует график в секундах (10^-12 до 10^-6 s).
        // Если ваше дерево хранит в наносекундах (ns), множитель: 1e-9.
        // Если в пикосекундах (ps), множитель: 1e-12. 
        // В вашем старом коде была подпись "ns". Предположим, что это ns:
        Double_t timeInSeconds = t_val * 1e-9; 
        
        vTime.push_back(timeInSeconds);
        vG.push_back(meanG);
        vGerr.push_back(gErr);
    }

    TGraphErrors* gr = new TGraphErrors(vTime.size(), vTime.data(), vG.data(), nullptr, vGerr.data());
    gr->SetMarkerStyle(marker);
    gr->SetMarkerColor(color);
    gr->SetLineColor(color);
    gr->SetMarkerSize(1.2);
    
    return gr;
}


void plotG_time() {
    gStyle->SetOptStat(0);
    gStyle->SetCanvasPreferGL(kTRUE);
    
    // Внимание! Для графиков времени мы сравниваем файлы для КОНКРЕТНОЙ ЭНЕРГИИ.
    // Например, воссоздаем Fig 14.11a (Carbon 24 MeV).
    // Подставьте сюда реальные имена сгенерированных ROOT-файлов из вашей папки output/
    vector<DatasetConfig> datasets = {
        {"results/output/chem6_ion-12_15-MeV_G4EmDNAPhysics_option2_G4EmDNAChemistry_option3_IRT_MI_P-OFF_C-OFF_3768_29_0.root", 
         "^{12}C^{6+} 15 MeV (Phys ON | Chem ON)", kBlue, 20},
         
        {"results/output/chem6_ion-12_15-MeV_G4EmDNAPhysics_option2_G4EmDNAChemistry_option3_IRT_MI_P-OFF_C-OFF_3768_189_0.root", 
         "^{12}C^{6+} 24 MeV (Phys OFF | Chem OFF)", kRed, 24},
         
        {"results/output/chem6_proton_20-MeV_G4EmDNAPhysics_option2_G4EmDNAChemistry_option3_IRT_MI_P-OFF_C-OFF_3768_0.root", 
         "^{1}H^{+} 300 MeV (Low-LET Limit)", kBlack, 22}
    };

    vector<SpeciesPlotTime> plots = {
        // Тег 11 = O2. График от 10^-12 до 10^-6 s.
        {{11}, "O_{2}", 1e-6, 1.0, kTRUE},
        {{6},  "H_{2}O_{2}", 0.1, 1.5, kFALSE},
        {{1},  "^{.}OH", 0.1, 6.0, kFALSE}
    };

    TString pdfFile = "results/plots/TimeEvolution_Validation.pdf";
    TCanvas* c1 = new TCanvas("c1", "Time Evolution", 800, 600);
    
    c1->Print(pdfFile + "["); // Открываем PDF

    for (size_t p = 0; p < plots.size(); p++) {
        c1->Clear();
        gPad->SetLeftMargin(0.12);
        gPad->SetBottomMargin(0.12);
        gPad->SetLogx(); // Время всегда логарифмическое
        if (plots[p].logY) gPad->SetLogy();
        gPad->SetGrid();

        // Рисуем оси. X от 1 пс (10^-12 с) до 1 мкс (10^-6 с)
        TH1F* frame = gPad->DrawFrame(1e-12, plots[p].yMin, 1e-6, plots[p].yMax);
        frame->GetXaxis()->SetTitle("Time [s]");
        frame->GetYaxis()->SetTitle(Form("G(%s) [molecules / 100 eV]", plots[p].beautyName.Data()));
        
        TLegend* leg = new TLegend(0.40, 0.75, 0.88, 0.88);
        leg->SetBorderSize(1);
        leg->SetTextSize(0.035);

        for (const auto& ds : datasets) {
            TGraphErrors* gr = ExtractTimeData(ds.filePath, plots[p].tags, ds.color, ds.markerStyle);
            if (gr) {
                gr->Draw("P SAME");
                leg->AddEntry(gr, ds.legendLabel.c_str(), "p");
            }
        }
        leg->Draw();

        c1->Print(pdfFile); // Сохраняем страницу
    }

    c1->Print(pdfFile + "]"); // Закрываем PDF
    delete c1;
    cout << "Time evolution plots generated: " << pdfFile << endl;
}