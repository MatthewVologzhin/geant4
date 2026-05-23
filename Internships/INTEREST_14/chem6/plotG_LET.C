#include <TGraphErrors.h>
#include <TCanvas.h>
#include <TColor.h>
#include <TAxis.h>
#include <TF1.h>
#include <TLegend.h>
#include <TLatex.h>
#include <Math/MinimizerOptions.h>
#include <TMath.h>
#include <TH1F.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <iostream>

using namespace std;

// Структура для хранения сырых данных из файла
struct SpeciesInfo {
    vector<Double_t> fG;
    vector<Double_t> fGerr;
    vector<Double_t> fLET;
    vector<Double_t> fLETerr;
    string fName;
};

// Структура конфигурации для каждого файла (путь, легенда, стиль)
struct DatasetConfig {
    string filePath;
    string legendLabel;
    Color_t color;
    Style_t markerStyle;
};

// Функция для парсинга одного файла
map<Int_t, SpeciesInfo> ParseDataFile(const string& filename) {
    map<Int_t, SpeciesInfo> speciesData;
    ifstream file(filename.c_str(), ios::in);
    
    if (!file.is_open()) {
        cerr << "Error: File not found: " << filename << endl;
        return speciesData; // возвращаем пустой map
    }

    string dummy, string_key, string_value, name;
    Double_t LET, LET_sigma, Gvalue, Gvalue_sigma;
    Int_t tag;

    while (file >> dummy >> LET >> dummy >> LET_sigma) {
        getline(file, dummy);
        if (!getline(file, string_key)) break;
        istringstream key(string_key);
        if (!getline(file, string_value)) break;
        istringstream value(string_value);
        
        while (key >> name >> tag && value >> Gvalue >> Gvalue_sigma) {
            speciesData[tag].fName = name;
            speciesData[tag].fG.push_back(Gvalue);
            speciesData[tag].fGerr.push_back(Gvalue_sigma);
            speciesData[tag].fLET.push_back(LET);
            speciesData[tag].fLETerr.push_back(LET_sigma);
        }
    }
    file.close();
    return speciesData;
}

void plotG_LET() {
    ROOT::Math::MinimizerOptions::SetDefaultMinimizer("Minuit");

    // =====================================================================
    // 1. КОНФИГУРАЦИЯ ФАЙЛОВ ДЛЯ ОТРИСОВКИ
    // Добавляйте сюда сколько угодно путей!
    // Формат: {"Путь", "Название в легенде", Цвет, Стиль маркера}
    // =====================================================================
    vector<DatasetConfig> datasets = {
        {"output_1us/all_data_center.txt",      "Geant4-DNA (CENTER): ^{12}C^{6+}", kBlue, 22}, // 22 - Треугольник вверх
        {"output_border/all_data_border.txt",   "Geant4-DNA (BORDER): ^{12}C^{6+}", kRed,  22},
        {"data_chem3_IRT.txt",   "Geant4-DNA (BORDER, IRT): ^{12}C^{6+}", kBlack,  22}// Пример добавления нового файла:
        // {"all_data_without_MI.txt",        "Geant4-DNA (No MI): ^{12}C^{6+}",  kGreen+2, 23} 
    };

    // Настройки стиля
    gStyle->SetOptStat(0);
    gStyle->SetOptFit(0);
    gStyle->SetPadTickX(1);
    gStyle->SetPadTickY(1);
    gStyle->SetTextFont(42);

    TCanvas *c1 = new TCanvas("c1", "Reference Style Plot", 800, 700);
    c1->SetLogx();
    c1->SetGrid();
    c1->SetLeftMargin(0.12);
    c1->SetBottomMargin(0.12);

    // Создаем пустой "каркас" графика для фиксации осей
    TH1F *frame = c1->DrawFrame(0.5, 0.0, 2e3, 0.12); // Оси: X_min, Y_min, X_max, Y_max
    frame->GetXaxis()->SetTitle("LET [keV/#mum]");
    frame->GetYaxis()->SetTitle("G [Species / 100 eV]");
    frame->GetXaxis()->SetTitleSize(0.045);
    frame->GetYaxis()->SetTitleSize(0.045);
    frame->GetYaxis()->SetTitleOffset(1.2);
    frame->GetXaxis()->SetLabelSize(0.04);
    frame->GetYaxis()->SetLabelSize(0.04);

    // Легенда
    TLegend *leg = new TLegend(0.15, 0.70, 0.55, 0.88);
    leg->SetBorderSize(1);
    leg->SetTextSize(0.03);

    // Векторы для хранения указателей (чтобы объекты не удалились из памяти)
    vector<TGraphErrors*> graphs;
    vector<TF1*> fits;

    // =====================================================================
    // 2. ГЛАВНЫЙ ЦИКЛ ПО ВСЕМ ФАЙЛАМ
    // =====================================================================
    for (size_t i = 0; i < datasets.size(); ++i) {
        auto& config = datasets[i];
        
        // Читаем файл
        map<Int_t, SpeciesInfo> speciesData = ParseDataFile(config.filePath);
        if (speciesData.empty()) continue; // Пропускаем, если файл не прочитан

        // Ищем нужные теги (7: HO2 и 12: O2-)
        if (speciesData.count(7) == 0 || speciesData.count(12) == 0) {
            cerr << "Warning: Tags 7 or 12 not found in " << config.filePath << endl;
            continue;
        }

        auto& h = speciesData[7];
        auto& o = speciesData[12];
        
        vector<Double_t> sum_G, sum_Gerr, sum_LET, sum_LETerr;

        // Суммируем
        for (size_t j = 0; j < h.fG.size(); ++j) {
            sum_G.push_back(h.fG[j] + o.fG[j]);
            sum_Gerr.push_back(TMath::Sqrt(h.fGerr[j]*h.fGerr[j] + o.fGerr[j]*o.fGerr[j]));
            sum_LET.push_back(h.fLET[j]);
            sum_LETerr.push_back(h.fLETerr[j]);
        }

        // Создаем график
        TGraphErrors* gr = new TGraphErrors(sum_G.size(), sum_LET.data(), sum_G.data(), 
                                            sum_LETerr.data(), sum_Gerr.data());
        
        gr->SetMarkerStyle(config.markerStyle);
        gr->SetMarkerSize(1.5);
        gr->SetMarkerColor(config.color);
        gr->SetLineColor(config.color);
        gr->SetLineWidth(2);

        // Создаем функцию фитирования с уникальным именем
        TString fitName = Form("fit_%zu", i);
        TF1 *fitFunc = new TF1(fitName, "[0]*pow(x, [1])", sum_LET.front(), sum_LET.back());
        fitFunc->SetParameters(0.01, 0.5);
        fitFunc->SetLineColor(config.color);
        fitFunc->SetLineWidth(3);
        
        gr->Fit(fitFunc, "QR"); // Q - тихо, R - в заданном диапазоне
        
        // Отрисовка поверх фрейма (опция "P" - точки)
        gr->Draw("P SAME");
        
        // Сохраняем в память
        graphs.push_back(gr);
        fits.push_back(fitFunc);

        // Добавляем в легенду (точки и линию)
        leg->AddEntry(gr, config.legendLabel.c_str(), "p");
        leg->AddEntry(fitFunc, (config.legendLabel + " (fit)").c_str(), "l");
    }

    leg->Draw();

    // Добавляем надпись формулы
    TLatex *tex = new TLatex(1e2, 0.10, "HO_{2}^{#bullet} + O_{2}^{-}");
    tex->SetTextSize(0.05);
    tex->Draw();

    c1->SaveAs("Summed_Species_C12.pdf");
    c1->SaveAs("Summed_Species_C12.png");

    cout << "Done! Plot with multiple datasets created." << endl;
}