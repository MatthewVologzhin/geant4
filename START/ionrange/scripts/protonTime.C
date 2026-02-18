#include <filesystem>
#include <algorithm>
#include <cmath>
#include <TFile.h>
#include <TTree.h>
#include <TGraph.h>
#include <TCanvas.h>
#include <TPad.h>
#include <TLegend.h>
#include <TAxis.h>
#include <TGaxis.h>
#include <TStyle.h>
#include <TString.h>
#include <TText.h>
#include <TPaveText.h>
#include <TMath.h>
#include <map>
#include <vector>
#include <iostream>

namespace fs = std::filesystem;

const std::string FormCanvasName(std::string);

struct Parameters{
    std::map<std::string, Int_t> colors;
    std::map<std::string, std::string> paths;
    std::map<std::string, TString> legends;
    std::vector<std::string> names;
};

void protonTime(){
    /* Picture quality configuration */
    gStyle->SetCanvasPreferGL(kTRUE);
    gStyle->SetPadBorderSize(0);                                                                                                          
    gStyle->SetOptStat(0);

    /* Parameters setup */
    Parameters parameters;
    // parameters.names = {"DNA2", "DNA4", "DNA6", "DNA8", "S4"}; // Раскомментируйте нужные
    parameters.names = {"DNA2"}; 
    parameters.paths["DNA2"]  = "root/ionrange-proton-DNA2.root";
    parameters.paths["DNA4"]  = "root/ionrange-proton-DNA4.root";
    parameters.paths["DNA6"]  = "root/ionrange-proton-DNA6.root";
    parameters.paths["DNA8"]  = "root/ionrange-proton-DNA8.root";
    parameters.paths["S4"]    = "root/ionrange-proton-S4.root";
    parameters.colors["DNA2"] = kAzure-14;
    parameters.colors["DNA4"] = kAzure-7;
    parameters.colors["DNA6"] = kAzure;
    parameters.colors["DNA8"] = kAzure+7;
    parameters.colors["S4"]   = kRed;
    
    parameters.legends["DNA2"] = "DNA Opt2";
    parameters.legends["DNA4"] = "DNA Opt4";
    parameters.legends["DNA6"] = "DNA Opt6";
    parameters.legends["DNA8"] = "DNA Opt8";
    parameters.legends["S4"]   = "EM Standard Opt4";

    TString authors = "Vologzhin & Ivanchenko & Batmunkh (2025)";

    /* Layout Parameters */
    double lineWidth = 1.5;
    double globalTextSize = 0.032; // Уменьшенный размер шрифта для вместимости
    
    double xAxisMin = 0.0010; 
    double xAxisMax = 40.0 * 1000.0; 
    
    // Position legend calculation
    // ИЗМЕНЕНИЕ: Anchor point изменен с 1000.0 на 10.0, чтобы сдвинуть легенду влево
    double legendAnchorEnergy = 1000; 
    double xMinLegend = 0.1 + 0.8*(log10(legendAnchorEnergy) - log10(xAxisMin))/(log10(xAxisMax) - log10(xAxisMin));
    double xMaxLegend = 0.95; // Немного расширили вправо (было 0.9)

    /* Canvas name from file name */
    std::string canvasName = FormCanvasName(parameters.paths["DNA2"]);
    std::string particleName = canvasName;
    canvasName = "Time " + canvasName; 
    
    TCanvas *mainCanvas = new TCanvas("mainCanvas", canvasName.c_str(), 1920, 1080);    
    
    /* Single Pad initialization */
    TPad *pad1 = new TPad("pad1", "main plot", 0.0, 0.0, 1.0, 1.0); 
    pad1->SetLogx();
    pad1->SetLogy(); 
    pad1->SetBottomMargin(0.12); 
    // Увеличим правое поле, чтобы текст точно не обрезался краем окна
    pad1->SetRightMargin(0.05); 
    pad1->Draw();
    pad1->cd();

    // Легенда
    // Координаты (X1, Y1, X2, Y2) в NDC (0 to 1)
    // Опустили верхнюю границу до 0.88, чтобы оставить место для авторов сверху
    TLegend *legend = new TLegend(xMinLegend, 0.6, xMaxLegend, 0.9);
    legend->SetTextSize(0.015); 
    legend->SetBorderSize(1);
    legend->SetFillStyle(0); // Прозрачный фон (опционально, если хотите белый - уберите эту строку)
    
    std::vector<TGraph*> graphs;
    std::vector<std::string> graphNames;

    double globalMinY = 1e99;
    double globalMaxY = -1e99;

    int counter = 0;
    for (auto name : parameters.names){
        /* Trees initialization */
        std::string path = parameters.paths[name];
        TFile *f = new TFile(path.c_str());
        if (f->IsZombie()){
            std::cerr << "Error opening file: " << path << std::endl;
            continue;
        } 
        
        TTree *treeTime = (TTree*)f->Get("calculationTime");
        if (!treeTime) {
             std::cerr << "Tree 'calculationTime' not found in " << path << std::endl;
             continue;
        }

        double energy, timeVal; 
        treeTime->SetBranchAddress("energy", &energy);
        treeTime->SetBranchAddress("time", &timeVal); 
    
        /* Подсчет полного времени (Summary) */
        double totalTime = 0.0;
        Long64_t nEntries = treeTime->GetEntries();
        for (Long64_t k = 0; k < nEntries; ++k) {
            treeTime->GetEntry(k);
            totalTime += timeVal;
        }

        /* Data processing for Graph */
        double energy1, energy2;
        std::vector<long> nbParticlesVector;
        std::vector<double> energyVector;
        long particlesNb = 1;

        for (Long64_t i=0; i < nEntries; ++i){
            treeTime->GetEntry(i);
            energy1 = energy;
            if (i < nEntries - 1) {
                treeTime->GetEntry(i+1);
                energy2 = energy;
                if (energy1 == energy2){
                    ++particlesNb;
                } else {
                    nbParticlesVector.push_back(particlesNb);
                    energyVector.push_back(energy1);
                    particlesNb = 1;
                }
            } else {
                 nbParticlesVector.push_back(particlesNb);
                 energyVector.push_back(energy1);
            }
        }
        
        TGraph* graphTime = new TGraph();
        long nbRuns = nbParticlesVector.size();
        double mean;
        long index = 0;
        
        for (size_t i=0; i<nbRuns; i++){
            mean = 0;
            long oldIndex = index;
            long nb = nbParticlesVector[i];
            
            for (size_t j=0; j<nb; j++){
                treeTime->GetEntry(index);
                mean += timeVal;    
                ++index;
            }
            mean = mean/nb;
            index = oldIndex + nb; 

            graphTime->SetPoint(graphTime->GetN(), energyVector[i], mean);
            
            if (mean < globalMinY) globalMinY = mean;
            if (mean > globalMaxY) globalMaxY = mean;
        }
        
        graphTime->SetLineWidth(lineWidth);
        graphTime->SetLineColor(parameters.colors[name]);
        graphTime->SetTitle(("Calculation time for " + particleName).c_str());
        
        graphs.push_back(graphTime);
        graphNames.push_back(name);
        
        // Формат вывода в легенду
        TString legendEntry = Form("#bf{%s} | #bf{Summary} = %.2f s", parameters.legends[name].Data(), totalTime);
        legend->AddEntry(graphTime, legendEntry, "l");
        
        counter++;
    }

    /* Set Global Y Axis Limits */
    double yAxisMin = globalMinY * 0.5;
    double yAxisMax = globalMaxY * 5.0; // Чуть больше места сверху для легенды

    /* Draw Graphs */
    for (size_t i = 0; i < graphs.size(); ++i) {
        TGraph* g = graphs[i];
        
        TAxis *Xaxis = g->GetXaxis();
        TAxis *Yaxis = g->GetYaxis();
        
        Xaxis->SetLimits(xAxisMin, xAxisMax);
        Xaxis->SetRangeUser(xAxisMin, xAxisMax);
        Yaxis->SetLimits(yAxisMin, yAxisMax);
        Yaxis->SetRangeUser(yAxisMin, yAxisMax);
        
        Xaxis->SetTitle("Initial particle energy #it{E}, [MeV]");
        Yaxis->SetTitle("Calculation time #it{t}, [s]");
        
        Yaxis->SetTitleSize(0.062);
        Yaxis->SetTitleOffset(0.65);
        Xaxis->SetTitleSize(0.05);   
        Xaxis->SetTitleOffset(1.1);
        
        Xaxis->CenterTitle(true); 
        Yaxis->CenterTitle(true); 

        if (i == 0){
            g->Draw("AL");
        } else {
            g->Draw("L SAME");
        }
    }

    /* Draw Authors Text */
    // Располагаем текст НАД легендой.
    // Легенда заканчивается на Y=0.88.
    // Авторов рисуем от 0.88 до 0.93.
    // X координаты совпадают с легендой (xMinLegend, xMaxLegend)
    TPaveText* authorsBox = new TPaveText(0.985*xMinLegend, 0.895, xMaxLegend, 0.935, "NDC");
    authorsBox->AddText(authors);
    authorsBox->SetFillColor(0);     
    authorsBox->SetBorderSize(0);    
    authorsBox->SetTextAlign(12);    // 1(Left) 2(Center Y) - выравнивание по левому краю
    authorsBox->SetTextFont(42);     
    authorsBox->SetTextSize(0.0217); // Тот же размер, что и у легенды
    authorsBox->Draw();
        
    /* Top axis (Pad 1) */
    double xMin = xAxisMin;
    double xMax = xAxisMax;
    double yMin = yAxisMin;
    double yMax = yAxisMax;
    
    TString chopt = "SN-G";
    int numDecadesForTGaxis = (int)ceil(log10(xMax) - log10(xMin));
    if (numDecadesForTGaxis < 1) numDecadesForTGaxis = 1;

    TGaxis *topAxis = new TGaxis(xMin, yMax, xMax, yMax,
                                 xMin, xMax,
                                 numDecadesForTGaxis,
                                 chopt.Data());
    topAxis->SetTickSize(0.04);
    topAxis->SetLineColor(kBlack);
    topAxis->SetLineWidth(1);
    topAxis->SetTickLength(0.02);
    topAxis->SetLabelOffset(999);
    topAxis->Draw("SAME");
    
    /* Right axis (Pad 1) */
    TString choptRight = "+GU";
    int numDecadesY = (int)ceil(log10(yMax) - log10(yMin));
    if (numDecadesY < 1) numDecadesY = 1;
    
    TGaxis *rightAxis = new TGaxis(xMax, yMin, xMax, yMax,
                                   yMin, yMax,
                                   numDecadesY,
                                   choptRight.Data());
    rightAxis->Draw();

    /* Legend Draw */
    legend->Draw();

    mainCanvas->Update();

    /* Save the plot */
    std::string folderName = "plots";
    if (!fs::exists(folderName)){
        fs::create_directory(folderName);
    }

    mainCanvas->SaveAs(("plots/" + canvasName + ".pdf").c_str());
}

const std::string FormCanvasName(std::string path){
    std::string ionName;
    size_t nameIndex;
    ionName  = path.substr(14,-1);
    nameIndex = std::find(begin(ionName), end(ionName), '-') - begin(ionName);
    ionName = ionName.substr(0, nameIndex);
    return ionName;
};
