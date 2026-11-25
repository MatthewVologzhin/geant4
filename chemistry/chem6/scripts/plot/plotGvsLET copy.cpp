#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <cmath>
#include <algorithm>
#include <regex>
#include <filesystem>
#include <memory>

// ROOT Headers
#include "TFile.h"
#include "TGraphErrors.h"
#include "TGraph.h"
#include "TMultiGraph.h"
#include "TCanvas.h"
#include "TLegend.h"
#include "TAxis.h"
#include "TStyle.h"
#include "TROOT.h"
#include "TColor.h"
#include "TF1.h"
#include "TMath.h"

// Geant4 Core & RunManager
#include "globals.hh"
#include "G4SystemOfUnits.hh"
#include "G4RunManager.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4VModularPhysicsList.hh"
#include "G4VUserActionInitialization.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4NistManager.hh"
#include "G4Material.hh"
#include "G4Element.hh"

// Geant4 Particles & EM Physics
#include "G4ParticleTable.hh"
#include "G4IonTable.hh"
#include "G4ParticleDefinition.hh"
#include "G4Proton.hh"
#include "G4Alpha.hh"
#include "G4Electron.hh"
#include "G4Gamma.hh"
#include "G4GenericIon.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "G4EmParameters.hh"
#include "G4EmCalculator.hh"

namespace fs = std::filesystem;

// ----------------------------------------------------------------------
// 1. Geant4 Minimal Infrastructure
// ----------------------------------------------------------------------
class MinimalDetectorConstruction : public G4VUserDetectorConstruction {
public:
    G4VPhysicalVolume* Construct() override {
        G4NistManager* nist = G4NistManager::Instance();
        G4Material* water = nist->FindOrBuildMaterial("G4_WATER");
        G4Box* solidWorld = new G4Box("World", 1.0 * m, 1.0 * m, 1.0 * m);
        G4LogicalVolume* logicWorld = new G4LogicalVolume(solidWorld, water, "World");
        return new G4PVPlacement(nullptr, G4ThreeVector(), logicWorld, "World", nullptr, false, 0);
    }
};

class MinimalPhysicsList : public G4VModularPhysicsList {
public:
    MinimalPhysicsList() {
        SetVerboseLevel(0);
        G4EmParameters::Instance()->SetVerbose(0);
        RegisterPhysics(new G4EmStandardPhysics_option4(0));
    }
};

class DummyPrimaryGenerator : public G4VUserPrimaryGeneratorAction {
public:
    void GeneratePrimaries(G4Event*) override {}
};

class DummyActionInitialization : public G4VUserActionInitialization {
public:
    void Build() const override {
        SetUserAction(new DummyPrimaryGenerator());
    }
    void BuildForMaster() const override {}
};

// ----------------------------------------------------------------------
// 2. Data Structures
// ----------------------------------------------------------------------
struct DataPoint {
    Double_t energy_MeV;
    Double_t let_sim;
    Double_t let_sim_err;
    Double_t let_g4calc;
    Double_t g_val;
    Double_t g_err;
};

struct SpeciesDataSet {
    std::string name;
    std::vector<DataPoint> highE_points;
    std::vector<DataPoint> lowE_points;
};

struct ExpDataEntry {
    std::string sectionName;
    std::string particle;
    int Z = 0;
    int A = 0;
    std::string species;
    std::string dataFile;
    std::string authors;
    int markerStyle = 29;
    int markerColor = 1;
    double markerSize = 1.8;
    std::vector<std::pair<double, double>> points;
};

// ----------------------------------------------------------------------
// 3. Helper Parsers & Particle Resolver
// ----------------------------------------------------------------------
std::string Trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

Double_t ParseEnergyToMeV(const std::string& valStr, const std::string& unitStr) {
    Double_t val = std::stod(valStr);
    std::string unit = unitStr;
    std::transform(unit.begin(), unit.end(), unit.begin(), ::tolower);

    if (unit == "ev")  return val * 1.0e-6;
    if (unit == "kev") return val * 1.0e-3;
    if (unit == "mev") return val * 1.0;
    if (unit == "gev") return val * 1.0e3;
    if (unit == "tev") return val * 1.0e6;
    return val;
}

void ParseParticleZA(const std::string& partStr, int& Z, int& A) {
    Z = 1; A = 1;
    if (partStr == "proton") { Z = 1; A = 1; return; }
    if (partStr == "alpha")  { Z = 2; A = 4; return; }
    if (partStr == "electron" || partStr == "e-") { Z = 0; A = 0; return; }

    std::regex ionRegex(R"(ion-(\d+)-(\d+))", std::regex::icase);
    std::smatch match;
    if (std::regex_match(partStr, match, ionRegex)) {
        Z = std::stoi(match[1].str());
        int secondVal = std::stoi(match[2].str());
        A = secondVal;
        G4NistManager* nist = G4NistManager::Instance();
        if (A <= Z) {
            A = static_cast<int>(std::round(nist->GetAtomicMassAmu(Z)));
            if (A < 1) A = 2 * Z;
        }
    }
}

std::string GetParticleDisplayName(const std::string& partStr) {
    if (partStr == "proton") return "p";
    if (partStr == "alpha") return "^{4}He";
    if (partStr == "e-" || partStr == "electron") return "e^{-}";

    int Z = 0, A = 0;
    ParseParticleZA(partStr, Z, A);
    if (Z > 0) {
        G4NistManager* nist = G4NistManager::Instance();
        G4Element* elem = nist->FindOrBuildElement(Z);
        std::string elemSymbol = elem ? std::string(elem->GetSymbol().data()) : ("Z=" + std::to_string(Z));
        return "^{" + std::to_string(A) + "}" + elemSymbol;
    }
    return partStr;
}

const G4ParticleDefinition* ResolveParticle(const std::string& partStr) {
    G4ParticleTable* pTable = G4ParticleTable::GetParticleTable();
    G4IonTable* ionTable = pTable->GetIonTable();

    if (partStr == "proton") return G4Proton::ProtonDefinition();
    if (partStr == "alpha")  return G4Alpha::AlphaDefinition();
    if (partStr == "e-" || partStr == "electron") return G4Electron::ElectronDefinition();
    if (partStr == "gamma")  return G4Gamma::GammaDefinition();

    int Z = 0, A = 0;
    ParseParticleZA(partStr, Z, A);
    if (Z > 0 && A > 0) {
        if (Z == 1 && A == 1) return G4Proton::ProtonDefinition();
        if (Z == 2 && A == 4) return G4Alpha::AlphaDefinition();
        return ionTable->GetIon(Z, A, 0.0);
    }

    return pTable->FindParticle(partStr);
}

// ----------------------------------------------------------------------
// 4. Честный расчет LET через Geant4 ICRU 90 (без подгонок и упрощений)
// ----------------------------------------------------------------------
Double_t ComputeLET_G4(G4EmCalculator* calc, const std::string& partStr, Double_t energyMeV, G4Material* mat) {
    if (!calc || !mat || energyMeV <= 0.0) return 0.0;

    const G4ParticleDefinition* pDef = ResolveParticle(partStr);
    if (!pDef) return 0.0;

    // Ограничение снизу порогом таблиц EM (1 keV)
    G4double eClamped = std::max(energyMeV * MeV, 1.0 * keV);

    // ComputeElectronicDEDX возвращает unrestricted dE/dx (LET_inf) в единицах энергии/длины
    G4double dedx = calc->ComputeElectronicDEDX(eClamped, pDef, mat);
    
    // Перевод в [keV / um]
    return dedx / (keV / micrometer);
}

Double_t FindBraggPeakEnergy(G4EmCalculator* calc, const std::string& partStr, G4Material* mat) {
    int Z = 1, A = 1;
    ParseParticleZA(partStr, Z, A);

    Double_t maxDEDX = -1.0;
    Double_t bestEnergyMeV = 0.8 * A;

    const int nScan = 1000;
    const Double_t logMin = std::log10(0.01 * A);  // 10 keV/u
    const Double_t logMax = std::log10(500.0 * A); // 500 MeV/u
    const Double_t step = (logMax - logMin) / nScan;

    for (int i = 0; i <= nScan; ++i) {
        Double_t E = std::pow(10.0, logMin + i * step);
        Double_t dedx = ComputeLET_G4(calc, partStr, E, mat);
        if (dedx > maxDEDX) {
            maxDEDX = dedx;
            bestEnergyMeV = E;
        }
    }
    return bestEnergyMeV;
}

// ----------------------------------------------------------------------
// 5. Загрузка экспериментальных данных из Config
// ----------------------------------------------------------------------
std::vector<ExpDataEntry> LoadExperimentalConfig(const std::string& cfgPath) {
    std::vector<ExpDataEntry> entries;
    if (!fs::exists(cfgPath)) {
        std::cout << "[Info] No experimental config found at " << cfgPath << ". Skipping exp overlay.\n";
        return entries;
    }

    std::ifstream file(cfgPath);
    if (!file.is_open()) return entries;

    std::string line;
    ExpDataEntry current;
    bool inSection = false;

    auto pushCurrent = [&]() {
        if (inSection && !current.dataFile.empty()) {
            if (fs::exists(current.dataFile)) {
                std::ifstream df(current.dataFile);
                std::string dline;
                while (std::getline(df, dline)) {
                    dline = Trim(dline);
                    if (dline.empty() || dline[0] == '#') continue;
                    std::istringstream iss(dline);
                    double x, y;
                    if (iss >> x >> y) {
                        current.points.push_back({x, y});
                    }
                }
            }
            entries.push_back(current);
        }
        current = ExpDataEntry();
    };

    while (std::getline(file, line)) {
        line = Trim(line);
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;

        if (line.front() == '[' && line.back() == ']') {
            pushCurrent();
            inSection = true;
            current.sectionName = line.substr(1, line.size() - 2);
        } else if (inSection) {
            auto eqPos = line.find('=');
            if (eqPos != std::string::npos) {
                std::string key = Trim(line.substr(0, eqPos));
                std::string val = Trim(line.substr(eqPos + 1));
                if (key == "particle") current.particle = val;
                else if (key == "Z") current.Z = std::stoi(val);
                else if (key == "A") current.A = std::stoi(val);
                else if (key == "species") current.species = val;
                else if (key == "data_file") current.dataFile = val;
                else if (key == "authors") current.authors = val;
                else if (key == "marker_style") current.markerStyle = std::stoi(val);
                else if (key == "marker_color") current.markerColor = std::stoi(val);
                else if (key == "marker_size") current.markerSize = std::stod(val);
            }
        }
    }
    pushCurrent();
    return entries;
}

// ----------------------------------------------------------------------
// 6. 4PL Logistic Fit
// ----------------------------------------------------------------------
TF1* FitUniversalLogistic(TGraphErrors* gr, const std::string& fitName, Color_t color, Int_t lineStyle) {
    if (!gr || gr->GetN() < 4) return nullptr;

    Double_t* x = gr->GetX();
    Double_t* y = gr->GetY();
    int n = gr->GetN();

    Double_t xMin = x[0], xMax = x[n - 1];
    Double_t yMax = *std::max_element(y, y + n);

    if (xMin > xMax) std::swap(xMin, xMax);
    if (xMin <= 0.0) xMin = 1e-3;

    TF1* fitFunc = new TF1(fitName.c_str(),
        "[0] + ([1] - [0]) / (1.0 + TMath::Power(10.0, ([3]) * (TMath::Log10(x) - [2])))",
        xMin, xMax);

    fitFunc->SetParNames("Bottom", "Top", "LogEC50", "HillSlope");

    Double_t initBottom = (y[0] > y[n - 1]) ? y[n - 1] : y[0];
    Double_t initTop    = (y[0] > y[n - 1]) ? y[0]     : y[n - 1];
    Double_t initLogEC50 = 0.5 * (std::log10(xMin) + std::log10(xMax));
    Double_t initSlope   = (y[0] > y[n - 1]) ? 1.2 : -1.2;

    fitFunc->SetParameter(0, initBottom);
    fitFunc->SetParameter(1, initTop);
    fitFunc->SetParameter(2, initLogEC50);
    fitFunc->SetParameter(3, initSlope);

    fitFunc->SetParLimits(0, 0.0, std::max(5.0, yMax * 1.5));
    fitFunc->SetParLimits(1, 0.0, std::max(10.0, yMax * 2.0));
    fitFunc->SetParLimits(2, std::log10(xMin) - 1.0, std::log10(xMax) + 1.0);

    fitFunc->SetLineColor(color);
    fitFunc->SetLineWidth(2);
    fitFunc->SetLineStyle(lineStyle);

    gr->Fit(fitFunc, "Q R B M EX0");
    return fitFunc;
}

std::vector<std::string> DiscoverAllParticles(const std::string& dataDir) {
    std::set<std::string> particles;
    std::regex filePattern(R"(chem6_([^_]+)_([0-9\.]+)-([A-Za-z]+)_.*\.txt)");

    if (!fs::exists(dataDir)) return {};

    for (const auto& entry : fs::directory_iterator(dataDir)) {
        if (!entry.is_regular_file()) continue;
        std::string filename = entry.path().filename().string();
        std::smatch match;
        if (std::regex_match(filename, match, filePattern)) {
            particles.insert(match[1].str());
        }
    }
    return std::vector<std::string>(particles.begin(), particles.end());
}

// ----------------------------------------------------------------------
// 7. Построение графиков и валидация
// ----------------------------------------------------------------------
void ProcessParticleValidation(const std::string& targetPartStr,
                               const std::string& dataDir,
                               const std::string& outputPlotsDir,
                               const std::vector<ExpDataEntry>& expConfigs,
                               G4EmCalculator* emCalculator,
                               G4Material* water)
{
    std::string displayName = GetParticleDisplayName(targetPartStr);
    int targetZ = 0, targetA = 0;
    ParseParticleZA(targetPartStr, targetZ, targetA);

    std::cout << "\n--------------------------------------------------------\n";
    std::cout << "--> Processing Particle: " << targetPartStr << " (" << displayName << ")\n";

    Double_t E_BraggPeak_MeV = FindBraggPeakEnergy(emCalculator, targetPartStr, water);
    std::cout << "--> Dynamic Bragg Peak (ICRU 90): E = " << E_BraggPeak_MeV << " MeV\n";

    std::regex filePattern(R"(chem6_)" + targetPartStr + R"(_([0-9\.]+)-([A-Za-z]+)_.*\.txt)");

    std::map<Int_t, std::string> tagToName;
    std::map<Int_t, std::vector<DataPoint>> rawExtractedData;

    for (const auto& entry : fs::directory_iterator(dataDir)) {
        if (!entry.is_regular_file()) continue;
        std::string filename = entry.path().filename().string();
        std::smatch match;

        if (std::regex_match(filename, match, filePattern)) {
            std::string eValStr = match[1].str();
            std::string eUnitStr = match[2].str();
            Double_t eMeV = ParseEnergyToMeV(eValStr, eUnitStr);
            Double_t let_g4calc = ComputeLET_G4(emCalculator, targetPartStr, eMeV, water);

            std::ifstream file(entry.path());
            if (!file.is_open()) continue;

            std::string dummy, string_key, string_value;
            Double_t let_sim = 0.0, let_sim_sigma = 0.0;

            while (file >> dummy >> let_sim >> dummy >> let_sim_sigma) {
                std::getline(file, dummy);
                if (!std::getline(file, string_key)) break;
                std::istringstream keyStream(string_key);

                if (!std::getline(file, string_value)) break;
                std::istringstream valStream(string_value);

                std::string specName;
                Int_t tagID;
                Double_t gVal, gValSigma;

                while (keyStream >> specName >> tagID && valStream >> gVal >> gValSigma) {
                    tagToName[tagID] = specName;

                    DataPoint pt;
                    pt.energy_MeV = eMeV;
                    pt.let_sim = let_sim;
                    pt.let_sim_err = let_sim_sigma;
                    pt.let_g4calc = let_g4calc;
                    pt.g_val = gVal;
                    pt.g_err = gValSigma;

                    rawExtractedData[tagID].push_back(pt);
                }
            }
            file.close();
        }
    }

    if (rawExtractedData.empty()) {
        std::cerr << "[Warning] No data found for particle: " << targetPartStr << std::endl;
        return;
    }

    std::map<Int_t, SpeciesDataSet> speciesSets;
    for (auto& pair : rawExtractedData) {
        Int_t tag = pair.first;
        speciesSets[tag].name = tagToName[tag];

        for (const auto& pt : pair.second) {
            if (pt.let_sim <= 0.05 || pt.let_sim > 10000.0) continue;

            if (pt.energy_MeV >= E_BraggPeak_MeV) {
                speciesSets[tag].highE_points.push_back(pt);
            } else {
                speciesSets[tag].lowE_points.push_back(pt);
            }
        }
    }

    std::string outRootPath = outputPlotsDir + "/G_vs_LET_" + targetPartStr + ".root";
    std::unique_ptr<TFile> outFile(TFile::Open(outRootPath.c_str(), "RECREATE"));

    for (auto& pair : speciesSets) {
        SpeciesDataSet& sData = pair.second;
        if (sData.highE_points.empty() && sData.lowE_points.empty()) continue;

        std::vector<Double_t> xH_s, yH_s, exH_s, eyH_s;
        std::vector<Double_t> xH_g, yH_g, exH_g, eyH_g;
        for (const auto& p : sData.highE_points) {
            xH_s.push_back(p.let_sim);     yH_s.push_back(p.g_val);
            exH_s.push_back(p.let_sim_err); eyH_s.push_back(p.g_err);
            xH_g.push_back(p.let_g4calc);  yH_g.push_back(p.g_val);
            exH_g.push_back(0.0);          eyH_g.push_back(p.g_err);
        }

        std::vector<Double_t> xL_s, yL_s, exL_s, eyL_s;
        std::vector<Double_t> xL_g, yL_g, exL_g, eyL_g;
        for (const auto& p : sData.lowE_points) {
            xL_s.push_back(p.let_sim);     yL_s.push_back(p.g_val);
            exL_s.push_back(p.let_sim_err); eyL_s.push_back(p.g_err);
            xL_g.push_back(p.let_g4calc);  yL_g.push_back(p.g_val);
            exL_g.push_back(0.0);          eyL_g.push_back(p.g_err);
        }

        TGraphErrors* grHigh_sim = new TGraphErrors(xH_s.size(), xH_s.data(), yH_s.data(), exH_s.data(), eyH_s.data());
        grHigh_sim->Sort();
        grHigh_sim->SetName(Form("grHigh_sim_%s", sData.name.c_str()));
        grHigh_sim->SetLineColor(kRed+1);
        grHigh_sim->SetMarkerColor(kRed+1);
        grHigh_sim->SetMarkerStyle(20);
        grHigh_sim->SetMarkerSize(1.1);
        grHigh_sim->SetLineWidth(2);

        TGraphErrors* grHigh_g4 = new TGraphErrors(xH_g.size(), xH_g.data(), yH_g.data(), exH_g.data(), eyH_g.data());
        grHigh_g4->Sort();
        grHigh_g4->SetName(Form("grHigh_G4calc_%s", sData.name.c_str()));
        grHigh_g4->SetLineColor(kBlue+1);
        grHigh_g4->SetMarkerColor(kBlue+1);
        grHigh_g4->SetMarkerStyle(21);
        grHigh_g4->SetMarkerSize(1.1);
        grHigh_g4->SetLineWidth(2);

        TGraphErrors* grLow_sim = new TGraphErrors(xL_s.size(), xL_s.data(), yL_s.data(), exL_s.data(), eyL_s.data());
        grLow_sim->Sort();
        grLow_sim->SetName(Form("grLow_sim_%s", sData.name.c_str()));
        grLow_sim->SetLineColor(kOrange+7);
        grLow_sim->SetMarkerColor(kOrange+7);
        grLow_sim->SetMarkerStyle(24);
        grLow_sim->SetMarkerSize(1.0);
        grLow_sim->SetLineWidth(2);
        grLow_sim->SetLineStyle(7);

        TGraphErrors* grLow_g4 = new TGraphErrors(xL_g.size(), xL_g.data(), yL_g.data(), exL_g.data(), eyL_g.data());
        grLow_g4->Sort();
        grLow_g4->SetName(Form("grLow_G4calc_%s", sData.name.c_str()));
        grLow_g4->SetLineColor(kAzure+7);
        grLow_g4->SetMarkerColor(kAzure+7);
        grLow_g4->SetMarkerStyle(25);
        grLow_g4->SetMarkerSize(1.0);
        grLow_g4->SetLineWidth(2);
        grLow_g4->SetLineStyle(7);

        FitUniversalLogistic(grHigh_sim, "fitHigh_sim_" + sData.name, kRed+1, 1);
        FitUniversalLogistic(grHigh_g4,  "fitHigh_g4_"  + sData.name, kBlue+1, 1);
        FitUniversalLogistic(grLow_sim,  "fitLow_sim_"  + sData.name, kOrange+7, 7);
        FitUniversalLogistic(grLow_g4,   "fitLow_g4_"   + sData.name, kAzure+7, 7);

        TCanvas c(Form("c_%s_%s", targetPartStr.c_str(), sData.name.c_str()), sData.name.c_str(), 1050, 620);
        c.SetRightMargin(0.34);
        c.SetLeftMargin(0.12);
        c.SetBottomMargin(0.12);
        c.SetLogx();
        c.SetGrid();

        TMultiGraph mg;
        if (grHigh_sim->GetN() > 0) mg.Add(grHigh_sim, "P");
        if (grHigh_g4->GetN() > 0)  mg.Add(grHigh_g4, "P");
        if (grLow_sim->GetN() > 0)  mg.Add(grLow_sim, "P");
        if (grLow_g4->GetN() > 0)   mg.Add(grLow_g4, "P");

        TLegend leg(0.67, 0.42, 0.98, 0.89);
        leg.SetBorderSize(0);
        leg.SetFillStyle(4050);
        leg.SetFillColorAlpha(kWhite, 0.6);
        leg.SetTextSize(0.026);
        leg.SetHeader(Form("Species: %s (%s)", sData.name.c_str(), displayName.c_str()), "C");

        if (grHigh_sim->GetN() > 0) leg.AddEntry(grHigh_sim, Form("High-E (E #geq %.2f MeV, sim)", E_BraggPeak_MeV), "lp");
        if (grHigh_g4->GetN() > 0)  leg.AddEntry(grHigh_g4,  Form("High-E (E #geq %.2f MeV, G4Calc)", E_BraggPeak_MeV), "lp");
        if (grLow_sim->GetN() > 0)  leg.AddEntry(grLow_sim,  Form("Low-E  (E < %.2f MeV, sim)", E_BraggPeak_MeV), "lp");
        if (grLow_g4->GetN() > 0)   leg.AddEntry(grLow_g4,   Form("Low-E  (E < %.2f MeV, G4Calc)", E_BraggPeak_MeV), "lp");

        // Поиск строго соответствующих экспериментальных данных
        std::vector<TGraph*> createdExpGraphs;
        for (const auto& exp : expConfigs) {
            bool partMatch = (exp.Z == targetZ && exp.A == targetA) || 
                             (exp.particle == targetPartStr);
            bool specMatch = (sData.name.find(exp.species) != std::string::npos);

            if (partMatch && specMatch && !exp.points.empty()) {
                std::vector<Double_t> expX, expY;
                for (const auto& pt : exp.points) {
                    expX.push_back(pt.first);
                    expY.push_back(pt.second);
                }
                TGraph* grExp = new TGraph(expX.size(), expX.data(), expY.data());
                grExp->SetName(Form("grExp_%s_%s", exp.sectionName.c_str(), sData.name.c_str()));
                grExp->SetMarkerStyle(exp.markerStyle);
                grExp->SetMarkerSize(exp.markerSize);
                grExp->SetMarkerColor(exp.markerColor);
                mg.Add(grExp, "P");
                leg.AddEntry(grExp, Form("Exp (%s)", exp.authors.c_str()), "p");
                createdExpGraphs.push_back(grExp);
            }
        }

        mg.Draw("A");
        mg.GetXaxis()->SetTitle("LET (keV/#mum)");
        mg.GetYaxis()->SetTitle("G value (molecules / 100 eV)");
        mg.GetXaxis()->SetTitleSize(0.045);
        mg.GetYaxis()->SetTitleSize(0.045);

        leg.Draw();

        std::string outPlotPath = outputPlotsDir + "/G_vs_LET_4curves_" + targetPartStr + "_" + sData.name + ".png";
        c.SaveAs(outPlotPath.c_str());

        // Запись в ROOT файл
        outFile->cd();
        grHigh_sim->Write();
        grHigh_g4->Write();
        grLow_sim->Write();
        grLow_g4->Write();
        for (auto* grExp : createdExpGraphs) {
            grExp->Write();
        }
    }

    outFile->Write();
    outFile->Close();
    std::cout << "[Saved] " << targetPartStr << " plots and ROOT file in: " << outputPlotsDir << "\n";
}

// ----------------------------------------------------------------------
// 8. Main Lifecycle Entry Point
// ----------------------------------------------------------------------
int main(int argc, char** argv) {
    std::string targetPart = (argc > 1) ? argv[1] : "ALL";
    std::string dataDir    = (argc > 2) ? argv[2] : "output/data";
    std::string plotsDir   = (argc > 3) ? argv[3] : "output/plots";
    std::string expCfg     = "input/experimental_data/g_let_particles.cfg";

    fs::create_directories(plotsDir);

    // 1. Инициализация Geant4 RunManager
    auto runManager = std::make_unique<G4RunManager>();
    runManager->SetVerboseLevel(0);
    runManager->SetUserInitialization(new MinimalDetectorConstruction());
    runManager->SetUserInitialization(new MinimalPhysicsList());
    runManager->SetUserInitialization(new DummyActionInitialization());
    
    runManager->Initialize();
    runManager->BeamOn(0); // Финализация G4LossTableManager и G4ProductionCutsTable

    // 2. Инициализация калькулятора в динамической памяти
    G4NistManager* nist = G4NistManager::Instance();
    G4Material* water = nist->FindOrBuildMaterial("G4_WATER");
    auto emCalculator = std::make_unique<G4EmCalculator>();

    // 3. Загрузка конфигурации экспериментов
    std::vector<ExpDataEntry> expConfigs = LoadExperimentalConfig(expCfg);

    if (targetPart == "ALL" || targetPart.empty()) {
        std::vector<std::string> allParticles = DiscoverAllParticles(dataDir);
        if (allParticles.empty()) {
            std::cerr << "[Error] No data files found in " << dataDir << std::endl;
            return 1;
        }

        std::cout << "========================================================\n";
        std::cout << "[Auto-Mode] Processing " << allParticles.size() << " particles: ";
        for (const auto& p : allParticles) std::cout << p << " ";
        std::cout << "\n========================================================\n";

        for (const auto& p : allParticles) {
            ProcessParticleValidation(p, dataDir, plotsDir, expConfigs, emCalculator.get(), water);
        }
    } else {
        ProcessParticleValidation(targetPart, dataDir, plotsDir, expConfigs, emCalculator.get(), water);
    }

    // 4. Корректная последовательность деинициализации
    emCalculator.reset();
    runManager.reset();

    std::cout << "\n[Success] All validation tasks completed successfully without leaks.\n";
    return 0;
}