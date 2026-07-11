// scripts/root/plotGeometryComparison.C
// Execute: root -l -q 'scripts/root/plotGeometryComparison.C+'

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <filesystem>
#include <cmath>
#include <map>
#include <algorithm>
#include <numeric>

// ROOT Headers
#include <TROOT.h>
#include <TCanvas.h>
#include <TPad.h>
#include <TH1F.h>
#include <TBox.h>
#include <TLegend.h>
#include <TLatex.h>
#include <TStyle.h>
#include <TSystem.h>
#include <TMath.h>
#include <TString.h>

namespace fs = std::filesystem;

// РќРѕСЂРјР°Р»РёР·Р°С†РёСЏ РЅР°Р·РІР°РЅРёР№ РјР°РєСЂРѕРјРѕР»РµРєСѓР» Рє РµРґРёРЅРѕРјСѓ РІРёРґСѓ
std::string NormalizeMolecule(const std::string& input) {
    std::string s = input;
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    if (s == "1kx5" || s == "histone") return "histone";
    if (s == "6wht" || s == "nmda") return "nmda";
    if (s == "4v6x" || s == "ribosome") return "ribosome";
    if (s == "7un1" || s == "cytoskeleton") return "cytoskeleton";
    return s;
}

// РљСЂР°СЃРёРІС‹Рµ РЅР°Р·РІР°РЅРёСЏ РјР°РєСЂРѕРјРѕР»РµРєСѓР» РґР»СЏ РїРѕРґРїРёСЃРµР№ РїРѕ РѕСЃРё X
std::string GetDisplayMoleculeName(const std::string& s) {
    if (s == "histone") return "Histone (1kx5)";
    if (s == "nmda") return "NMDA (6WHT)";
    if (s == "ribosome") return "Ribosome (4V6X)";
    if (s == "cytoskeleton") return "Cytoskeleton (7un1)";
    return s;
}

// Р¤РѕСЂРјР°С‚РёСЂРѕРІР°РЅРёРµ С‡РёСЃРµР» СЂР°Р·РґРµР»РёС‚РµР»СЏРјРё С‚С‹СЃСЏС‡ (РґР»СЏ СѓРґРѕР±СЃС‚РІР° С‡С‚РµРЅРёСЏ) [4]
std::string FormatWithCommas(int value) {
    std::string numWithCommas = std::to_string(value);
    int insertPosition = numWithCommas.length() - 3;
    while (insertPosition > 0) {
        numWithCommas.insert(insertPosition, ",");
        insertPosition -= 3;
    }
    return numWithCommas;
}

int CountAtoms(const std::string& pdbCode) {
    std::string path = "output/csv/" + pdbCode + ".csv";
    std::ifstream f(path);
    if (!f.is_open()) {
        std::string lowerCode = pdbCode;
        std::transform(lowerCode.begin(), lowerCode.end(), lowerCode.begin(), ::tolower);
        path = "output/csv/" + lowerCode + ".csv";
        f.open(path);
    }
    if (!f.is_open()) {
        std::string upperCode = pdbCode;
        std::transform(upperCode.begin(), upperCode.end(), upperCode.begin(), ::toupper);
        path = "output/csv/" + upperCode + ".csv";
        f.open(path);
    }
    
    if (!f.is_open()) {
        std::cerr << "Warning: Could not open CSV file to count atoms for: " << pdbCode << std::endl;
        return -1;
    }
    
    int lines = 0;
    std::string line;
    while (std::getline(f, line)) {
        if (!line.empty()) {
            lines++;
        }
    }
    f.close();
    
    return (lines > 0) ? (lines - 1) : 0;
}

void plotGeometryComparisonTime() {
    // РџРѕР»РЅРѕСЃС‚СЊСЋ РѕС‚РєР»СЋС‡Р°РµРј Р·Р°РїСѓСЃРє GUI-РѕРєРѕРЅ РґР»СЏ С„РѕРЅРѕРІРѕРіРѕ СЃРѕС…СЂР°РЅРµРЅРёСЏ [2]
    gROOT->SetBatch(kTRUE);

    // ========== РќРђРЎРўР РћР™РљРђ РЎРўРР›РЇ CERN (ATLAS/CMS) ==========
    gStyle->SetCanvasPreferGL(kTRUE);
    gStyle->SetPadBorderSize(0);
    gStyle->SetFrameBorderMode(0);
    gStyle->SetCanvasBorderMode(0);
    gStyle->SetPadTickX(1); 
    gStyle->SetPadTickY(1);
    gStyle->SetTextFont(42);
    gStyle->SetLabelFont(42, "XYZ");
    gStyle->SetTitleFont(42, "XYZ");

    // Р’РµРєС‚РѕСЂ РјРµС‚РѕРґРѕРІ РїРѕСЃС‚СЂРѕРµРЅРёСЏ РіРµРѕРјРµС‚СЂРёРё
    const std::vector<std::string> targetMethods = {
        "BOOLEAN", "VOXEL_1-angstrom", "VOXEL_5-angstrom", "VOXEL_10-angstrom", "PREDEFINED"
    };

    // РќР°Р·РІР°РЅРёСЏ РјРµС‚РѕРґРѕРІ РґР»СЏ Р»РµРіРµРЅРґС‹ (СЃ РєРµСЂРЅРёРЅРіРѕРј Р·РЅР°РєР° Р°РЅРіСЃС‚СЂРµРјР°)
    const std::vector<std::string> methodLabels = {
        "BOOLEAN", "VOXEL 1#kern[2.8]{ }#AA", "VOXEL 5#kern[2.8]{ }#AA", "VOXEL 10#kern[2.8]{ }#AA", "PREDEFINED"
    };

    // Р¦РІРµС‚Р° РґР»СЏ СЃС‚РѕР»Р±С†РѕРІ
    std::map<std::string, Int_t> methodColors = {
        {"BOOLEAN", kBlack},
        {"VOXEL_1-angstrom", kBlue-4},
        {"VOXEL_5-angstrom", kGreen-3},
        {"VOXEL_10-angstrom", kOrange+1},
        {"PREDEFINED", kRed-4}
    };

    // Р§С‚РµРЅРёРµ CSV РґР°РЅРЅС‹С… РёР· РїР°РїРєРё build [3]
    std::ifstream infile("build/geometry_benchmark_all.csv");
    if (!infile.is_open()) {
        std::cerr << "Error: Could not open build/geometry_benchmark_all.csv" << std::endl;
        return;
    }

    // [РњР°РєСЂРѕРјРѕР»РµРєСѓР»Р°][РњРµС‚РѕРґ] -> СЃРїРёСЃРѕРє РІСЂРµРјРµРЅРЅС‹С… Р·Р°РјРµСЂРѕРІ
    std::map<std::string, std::map<std::string, std::vector<double>>> rawData;
    std::string line;

    // РџСЂРѕРїСѓСЃРє Р·Р°РіРѕР»РѕРІРєР° CSV
    if (std::getline(infile, line)) { }

    while (std::getline(infile, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string geomType, method, voxelSizeStr, timeStr;
        if (std::getline(ss, geomType, ',') &&
            std::getline(ss, method, ',') &&
            std::getline(ss, voxelSizeStr, ',') &&
            std::getline(ss, timeStr, ',')) {
            
            try {
                double voxelSize = std::stod(voxelSizeStr);
                double timeVal = std::stod(timeStr);
                
                std::string molecule = NormalizeMolecule(geomType);
                std::string methodType = "";
                
                if (method == "BOOLEAN") {
                    methodType = "BOOLEAN";
                } else if (method == "PREDEFINED") {
                    methodType = "PREDEFINED";
                } else if (method == "VOXEL") {
                    int size = std::round(voxelSize);
                    if (size == 1) methodType = "VOXEL_1-angstrom";
                    else if (size == 5) methodType = "VOXEL_5-angstrom";
                    else if (size == 10) methodType = "VOXEL_10-angstrom";
                }
                
                if (!methodType.empty()) {
                    rawData[molecule][methodType].push_back(timeVal);
                }
            } catch (...) {
                // РРіРЅРѕСЂРёСЂСѓРµРј РѕС€РёР±РєРё РїР°СЂСЃРёРЅРіР° РЅРµРїРѕР»РЅС‹С… СЃС‚СЂРѕРє
            }
        }
    }
    infile.close();

    // Р Р°СЃС‡РµС‚ СЃСЂРµРґРЅРµРіРѕ РІСЂРµРјРµРЅРё
    std::map<std::string, std::map<std::string, double>> meanTimes;
    for (const auto& [mol, methodMap] : rawData) {
        for (const auto& [method, times] : methodMap) {
            if (times.empty()) continue;
            double sum = std::accumulate(times.begin(), times.end(), 0.0);
            meanTimes[mol][method] = sum / times.size();
        }
    }

    // РЎРїРёСЃРѕРє РјРѕР»РµРєСѓР» РЅР° РѕСЃРё X
    std::vector<std::string> molecules = {"histone", "nmda", "ribosome", "cytoskeleton"};
    int numMolecules = molecules.size();

    // РљР°СЂС‚Р° СЃРѕРѕС‚РІРµС‚СЃС‚РІРёСЏ РјР°РєСЂРѕРјРѕР»РµРєСѓР»С‹ Рё РµРµ PDB РєРѕРґР° [4]
    std::map<std::string, std::string> molToPdb = {
        {"histone", "1kx5"},
        {"nmda", "6WHT"},
        {"ribosome", "4V6X"},
        {"cytoskeleton", "7un1"}
    };

    std::map<std::string, int> atomCounts;
    for (const auto& mol : molecules) {
        if (molToPdb.count(mol)) {
            atomCounts[mol] = CountAtoms(molToPdb[mol]);
        } else {
            atomCounts[mol] = -1;
        }
    }

    // Р”РёРЅР°РјРёС‡РµСЃРєРёР№ РїРѕРёСЃРє РіСЂР°РЅРёС† РѕСЃРё Y
    double minTime = 1e9, maxTime = -1e9;
    for (const auto& [mol, methodMap] : meanTimes) {
        for (const auto& [method, t] : methodMap) {
            if (t < minTime) minTime = t;
            if (t > maxTime) maxTime = t;
        }
    }
    double yAxisMin = TMath::Power(10, std::floor(std::log10(minTime) - 0.5));
    double yAxisMax = TMath::Power(10, std::ceil(std::log10(maxTime) + 0.5));
    if (yAxisMin < 1e-4) yAxisMin = 1e-4;
    std::vector<TObject*> garbage;

    // РЎРѕР·РґР°РЅРёРµ С…РѕР»СЃС‚Р° 1920x1080
    TCanvas* c = new TCanvas("c_geom_time_comparison", "Geometry Construction Benchmarks", 1920, 1080);

    // РЎРјРµС‰РµРЅРёРµ pad1 РІРЅРёР· (СЃ 0.08 РґРѕ 0.05) Рё СѓРјРµРЅСЊС€РµРЅРёРµ BottomMargin (СЃ 0.15 РґРѕ 0.13) РґР»СЏ СѓСЃС‚СЂР°РЅРµРЅРёСЏ РїСѓСЃС‚РѕС‚С‹ [5]
    TPad* pad1 = new TPad("pad1_main", "main", 0.0, 0.05, 1.0, 1.0);
    pad1->SetLogy();
    pad1->SetBottomMargin(0.13); // РљРѕРјРїР°РєС‚РЅРѕРµ РїРѕР»Рµ РґР»СЏ РїРѕРґРїРёСЃРµР№ (РІ NDC)
    pad1->SetTopMargin(0.08);   
    pad1->SetLeftMargin(0.12);  
    pad1->SetRightMargin(0.03);
    pad1->Draw();

    pad1->cd();

    // РћРїРѕСЂРЅС‹Р№ С„СЂРµР№Рј РЅР° 4 Р±РёРЅР°
    TH1F* hFrame = new TH1F("hFrame", "", numMolecules, 0.5, numMolecules + 0.5);
    hFrame->SetStats(0);
    garbage.push_back(hFrame);
    hFrame->Draw("AXIS");

    TAxis* xAxis = hFrame->GetXaxis();
    TAxis* yAxis = hFrame->GetYaxis();
    
    xAxis->SetLimits(0.5, numMolecules + 0.5); 
    xAxis->SetRangeUser(0.5, numMolecules + 0.5);
    xAxis->SetTickLength(0.0); 
    
    // РЎРєСЂС‹РІР°РµРј РЅР°С‚РёРІРЅС‹Р№ С‚РµРєСЃС‚ РјРµС‚РѕРє, С‡С‚РѕР±С‹ РЅР°СЂРёСЃРѕРІР°С‚СЊ РїСЂРѕС„РµСЃСЃРёРѕРЅР°Р»СЊРЅС‹Рµ РґРІСѓС…СЃС‚СЂРѕС‡РЅС‹Рµ РјРµС‚РєРё С‡РµСЂРµР· TLatex [4]
    xAxis->SetLabelSize(0); 

    // РќР°СЃС‚СЂРѕР№РєР° РѕСЃРё Y
    yAxis->SetRangeUser(yAxisMin, yAxisMax);
    yAxis->SetTickLength(0.02);
    yAxis->SetTitle("Geometry Construction Time [s]");
    yAxis->SetTitleSize(0.045);
    yAxis->SetTitleOffset(1.1);
    yAxis->CenterTitle(true);

    // РћС‚СЂРёСЃРѕРІРєР° СЃС‚РѕР»Р±С†РѕРІ TBox РІРЅСѓС‚СЂРё РєР°Р¶РґРѕР№ РіСЂСѓРїРїС‹
    int M = targetMethods.size(); // 5 РјРµС‚РѕРґРѕРІ

    for (int i = 0; i < numMolecules; ++i) {
        std::string mol = molecules[i];
        double binCenter = hFrame->GetXaxis()->GetBinCenter(i + 1);
        double binWidth = hFrame->GetXaxis()->GetBinWidth(i + 1);
        
        // РљР°Р¶РґР°СЏ РіСЂСѓРїРїР° Р·Р°РЅРёРјР°РµС‚ 65% РѕС‚ С€РёСЂРёРЅС‹ Р±РёРЅР°, 35% вЂ” СЌС‚Рѕ СЂР°Р·РґРµР»СЏСЋС‰РёР№ "РІРѕР·РґСѓС…"
        double currentGroupWidth = 0.65 * binWidth;
        double currentBarWidth = currentGroupWidth / M;

        for (int j = 0; j < M; ++j) {
            std::string method = targetMethods[j];
            if (!meanTimes[mol].count(method)) continue;

            double tVal = meanTimes[mol][method];

            double x1 = binCenter - currentGroupWidth / 2.0 + j * currentBarWidth;
            double x2 = binCenter - currentGroupWidth / 2.0 + (j + 1) * currentBarWidth;

            TBox* box = new TBox(x1, yAxisMin, x2, tVal);
            box->SetFillColor(methodColors[method]);
            box->SetLineColor(kBlack);
            box->SetLineWidth(1);
            box->Draw("SAME");
            garbage.push_back(box);
        }
    }

    // РџРµСЂРµСЂРёСЃРѕРІРєР° РѕСЃРµР№ РїРѕРІРµСЂС… РіРёСЃС‚РѕРіСЂР°РјРј
    pad1->Update();
    pad1->RedrawAxis();

    // Р’РѕР·РІСЂР°С‚ РЅР° Canvas РґР»СЏ РІС‹РІРѕРґР° Р»РµРіРµРЅРґ, РІРѕРґСЏРЅРѕРіРѕ Р·РЅР°РєР° Рё СЂСѓС‡РЅС‹С… С‚РµРєСЃС‚РѕРІС‹С… РјРµС‚РѕРє РїРѕРґ РѕСЃСЊСЋ X
    c->cd();

    // РћС‚СЂРёСЃРѕРІРєР° РґРІСѓС…СЃС‚СЂРѕС‡РЅС‹С… РјРµС‚РѕРє РѕСЃРµР№ РІ NDC РґР»СЏ РёРґРµР°Р»СЊРЅРѕРіРѕ РІС‹СЂР°РІРЅРёРІР°РЅРёСЏ [4]
    double leftM = pad1->GetLeftMargin();
    double rightM = pad1->GetRightMargin();
    double xMin = 0.5;
    double xMax = numMolecules + 0.5;

    TLatex* axisLabels = new TLatex();
    axisLabels->SetNDC();
    axisLabels->SetTextFont(42);
    axisLabels->SetTextAlign(22); // Р’С‹СЂР°РІРЅРёРІР°РЅРёРµ РїРѕ С†РµРЅС‚СЂСѓ РїРѕ РіРѕСЂРёР·РѕРЅС‚Р°Р»Рё Рё РІРµСЂС‚РёРєР°Р»Рё

    for (int i = 0; i < numMolecules; ++i) {
        std::string mol = molecules[i];
        double binCenter = hFrame->GetXaxis()->GetBinCenter(i + 1);
        double xNDC = leftM + (1.0 - leftM - rightM) * (binCenter - xMin) / (xMax - xMin);

        // РЎС‚СЂРѕРєР° 1: РРјСЏ РјРѕР»РµРєСѓР»С‹
        axisLabels->SetTextSize(0.024);
        axisLabels->SetTextColor(kBlack);
        axisLabels->DrawLatex(xNDC, 0.11, GetDisplayMoleculeName(mol).c_str());

        // РЎС‚СЂРѕРєР° 2: РљРѕР»РёС‡РµСЃС‚РІРѕ Р°С‚РѕРјРѕРІ [4]
        axisLabels->SetTextSize(0.019);
        axisLabels->SetTextColor(kGray+2); // РќРµР№С‚СЂР°Р»СЊРЅС‹Р№ СЃРµСЂС‹Р№ С†РІРµС‚ РґР»СЏ РІС‚РѕСЂРѕСЃС‚РµРїРµРЅРЅС‹С… РґР°РЅРЅС‹С…
        int count = atomCounts[mol];
        TString countStr = (count >= 0) ? Form("N = %s atoms", FormatWithCommas(count).c_str()) : "N = unknown";
        axisLabels->DrawLatex(xNDC, 0.075, countStr.Data());
    }

    // Р’РѕРґСЏРЅРѕР№ Р·РЅР°Рє СЃР»РµРІР° РІРЅРёР·Сѓ (РІС‹СЂРѕРІРЅРµРЅ РїРѕ Р»РµРІРѕР№ РіСЂР°РЅРёС†Рµ СЂР°РјРєРё РіСЂР°С„РёРєР°: 0.12) [5]
    double watermarkX = 0.12;
    TLatex* lModel = new TLatex();
    lModel->SetNDC();
    lModel->SetTextFont(42);
    lModel->SetTextSize(0.020);
    lModel->DrawLatex(watermarkX, 0.025, "#bf{Geant4-DNA}#kern[0.1]{#it{Simulation}}");
    lModel->DrawLatex(watermarkX, 0.008, "Geometry Comparison (Construction Time)");
    garbage.push_back(lModel);

    // Р“РѕСЂРёР·РѕРЅС‚Р°Р»СЊРЅР°СЏ Р»РµРіРµРЅРґР° СЃРїСЂР°РІР° РІРЅРёР·Сѓ (РІС‹СЂРѕРІРЅРµРЅР° РїРѕ РїСЂР°РІРѕР№ РіСЂР°РЅРёС†Рµ СЂР°РјРєРё РіСЂР°С„РёРєР°: 1.0 - 0.03 = 0.97) [5]
    double legendX2 = 0.97;
    double legendX1 = legendX2 - 0.55; 
    TLegend* legend = new TLegend(legendX1, 0.01, legendX2, 0.045);
    legend->SetTextSize(0.018);
    legend->SetTextFont(42);
    legend->SetBorderSize(0);
    legend->SetFillStyle(0);
    legend->SetNColumns(targetMethods.size());

    for (const auto& method : targetMethods) {
        TBox* dummyBox = new TBox();
        dummyBox->SetFillColor(methodColors[method]);
        dummyBox->SetLineColor(kBlack);
        dummyBox->SetLineWidth(1);
        legend->AddEntry(dummyBox, Form("#scale[1.1]{#bf{%s}}", methodLabels[std::find(targetMethods.begin(), targetMethods.end(), method) - targetMethods.begin()].c_str()), "f");
        garbage.push_back(dummyBox);
    }
    legend->Draw();

    c->Update();

    fs::create_directories("output/plots/geometry_comparison");
    std::string outputPath = "output/plots/geometry_comparison/Geometry_Build_Time_Comparison.svg";
    c->SaveAs(outputPath.c_str());

    delete c;
    for (auto obj : garbage) delete obj;
    
    std::cout << "\n====================================================================" << std::endl;
    std::cout << "[SUCCESS] Geometry benchmark plot saved to: " << outputPath << std::endl;
    std::cout << "====================================================================" << std::endl;
}
