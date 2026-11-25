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
struct SimConfig {
    std::string physList;
    std::string chemList;
    std::string chemMethod;
    std::string multiIon;

    bool operator<(const SimConfig& o) const {
        if (physList != o.physList) return physList < o.physList;
        if (chemList != o.chemList) return chemList < o.chemList;
        if (chemMethod != o.chemMethod) return chemMethod < o.chemMethod;
        return multiIon < o.multiIon;
    }

    std::string GetShortLabel() const {
        std::string p = physList;
        auto posP = p.find("option");
        if (posP != std::string::npos) p = "P" + p.substr(posP + 6);

        std::string c = chemList;
        auto posC = c.find("option");
        if (posC != std::string::npos) c = "C" + c.substr(posC + 6);

        std::string mi = (multiIon == "MI-ON") ? "ON" : "OFF";
        return p + " " + c + " " + chemMethod + " " + mi;
    }

    std::string GetIdString() const {
        return physList + "_" + chemList + "_" + chemMethod + "_" + multiIon;
    }
};

struct ConfigVisualAttr {
    Color_t color;
    Style_t markerStyle;
    Style_t lineStyle;
    Width_t lineWidth;
};

ConfigVisualAttr GetVisualAttributes(const SimConfig& cfg) {
    ConfigVisualAttr attr;
    attr.lineWidth = 2;

    if (cfg.physList.find("option2") != std::string::npos) {
        attr.color = (cfg.chemList.find("option1") != std::string::npos) ? (kAzure + 2) : (kBlue + 2);
    } else if (cfg.physList.find("option4") != std::string::npos) {
        attr.color = (cfg.chemList.find("option1") != std::string::npos) ? (kTeal + 2) : (kGreen + 2);
    } else if (cfg.physList.find("option6") != std::string::npos) {
        attr.color = (cfg.chemList.find("option1") != std::string::npos) ? (kOrange + 7) : (kRed + 1);
    } else if (cfg.physList.find("option8") != std::string::npos) {
        attr.color = (cfg.chemList.find("option1") != std::string::npos) ? (kViolet + 2) : (kMagenta + 2);
    } else {
        attr.color = kGray + 2;
    }

    bool isChem1 = (cfg.chemList.find("option1") != std::string::npos);
    bool isIRT = (cfg.chemMethod == "IRT");

    if (isChem1) {
        attr.markerStyle = isIRT ? 20 : 24;
    } else {
        attr.markerStyle = isIRT ? 21 : 25;
    }

    attr.lineStyle = (cfg.multiIon == "MI-ON") ? 1 : 7;
    return attr;
}

struct DataPoint {
    Double_t energy_MeV;
    Double_t let_g4calc;
    Double_t g_val;
    Double_t g_err;
};

struct ExpDataEntry {
    std::string sourceFile;
    int Z = 0;
    int A = 0;
    std::string canonicalSpecies;
    std::string label;
    int markerStyle = 29;
    int markerColor = kBlack;
    double markerSize = 2.2;
    std::vector<std::pair<double, double>> points;
};

// ----------------------------------------------------------------------
// 3. Строгая санитизация и нормализация
// ----------------------------------------------------------------------
std::string Trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

// Преобразует любые UTF-8 / не-ASCII строки в безопасные ASCII идентификаторы для ROOT
std::string SanitizeRootName(const std::string& name) {
    std::string clean = "";
    for (unsigned char c : name) {
        if (std::isalnum(c) || c == '_') {
            clean += c;
        } else if (c == '^' || c == '-' || c == '+' || c == '.') {
            clean += '_';
        }
    }
    if (clean.empty()) clean = "species";
    return clean;
}

// Каноническое сопоставление радикалов
std::string GetCanonicalSpecies(const std::string& raw) {
    std::string s = raw;
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    std::string clean = "";
    for (char c : s) {
        if (std::isalnum(c)) clean += c;
    }

    if (clean.find("eaq") != std::string::npos || clean.find("e_aq") != std::string::npos) return "eaq";
    if (clean == "h2o2") return "h2o2";
    if (clean.find("ho2") != std::string::npos) return "ho2";
    if (clean.find("oh") != std::string::npos) return "oh";
    if (clean == "h2") return "h2";
    if (clean == "h" || clean.find("hdot") != std::string::npos) return "h";

    return clean;
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
    std::string p = partStr;
    std::transform(p.begin(), p.end(), p.begin(), ::tolower);

    if (p == "proton" || p == "p") { Z = 1; A = 1; return; }
    if (p == "alpha" || p == "he4" || p == "he" || p == "ion-2-2" || p == "ion-2-4") { Z = 2; A = 4; return; }
    if (p == "electron" || p == "e-") { Z = 0; A = 0; return; }

    std::regex ionRegex(R"(ion-(\d+)-(\d+))", std::regex::icase);
    std::smatch match;
    if (std::regex_match(partStr, match, ionRegex)) {
        Z = std::stoi(match[1].str());
        A = std::stoi(match[2].str());
        if (Z == 2 && (A == 2 || A == 0)) { A = 4; }
        else if (A <= Z && Z > 0) {
            G4NistManager* nist = G4NistManager::Instance();
            A = static_cast<int>(std::round(nist->GetAtomicMassAmu(Z)));
            if (A < 1) A = 2 * Z;
        }
        return;
    }

    std::regex elemRegex(R"(([a-zA-Z]+)(\d+))");
    if (std::regex_match(p, match, elemRegex)) {
        std::string elemSym = match[1].str();
        A = std::stoi(match[2].str());
        elemSym[0] = std::toupper(elemSym[0]);
        G4NistManager* nist = G4NistManager::Instance();
        G4Element* elem = nist->FindOrBuildElement(elemSym);
        if (elem) {
            Z = static_cast<int>(std::round(elem->GetZ()));
        }
        return;
    }
}

std::string GetParticleDisplayName(const std::string& partStr) {
    if (partStr == "proton" || partStr == "p") return "p";
    if (partStr == "alpha" || partStr == "ion-2-2" || partStr == "ion-2-4" || partStr == "he4") return "^{4}He^{2+}";
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

    if (partStr == "proton" || partStr == "p") return G4Proton::ProtonDefinition();
    if (partStr == "alpha" || partStr == "he4" || partStr == "ion-2-2" || partStr == "ion-2-4") return G4Alpha::AlphaDefinition();
    if (partStr == "e-" || partStr == "electron") return G4Electron::ElectronDefinition();
    if (partStr == "gamma") return G4Gamma::GammaDefinition();

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
// 4. Расчет LET
// ----------------------------------------------------------------------
Double_t ComputeLET_G4(G4EmCalculator* calc, const std::string& partStr, Double_t energyMeV, G4Material* mat) {
    if (!calc || !mat || energyMeV <= 0.0) return 0.0;
    const G4ParticleDefinition* pDef = ResolveParticle(partStr);
    if (!pDef) return 0.0;

    G4double eClamped = std::max(energyMeV * MeV, 1.0 * keV);
    G4double dedx = calc->ComputeElectronicDEDX(eClamped, pDef, mat);
    return dedx / (keV / micrometer);
}

// ----------------------------------------------------------------------
// 5. Автозагрузка экспериментальных файлов
// ----------------------------------------------------------------------
std::string FindValidExpDir(const std::string& userPath) {
    std::vector<std::string> candidates = {
        userPath,
        "input/experimental_data",
        "../input/experimental_data",
        "../../input/experimental_data"
    };
    for (const auto& p : candidates) {
        if (fs::exists(p) && fs::is_directory(p)) return p;
    }
    return userPath;
}

std::vector<ExpDataEntry> LoadAllExperimentalFiles(const std::string& rawExpDir) {
    std::vector<ExpDataEntry> entries;
    std::string expDir = FindValidExpDir(rawExpDir);

    if (!fs::exists(expDir)) {
        std::cerr << "\n[ERROR] Experimental directory NOT FOUND: " << rawExpDir << std::endl;
        return entries;
    }

    std::cout << "\n========================================================";
    std::cout << "\n[INFO] Loading Experimental Data from: " << fs::canonical(expDir).string() << "\n";

    std::regex expPattern(R"(g_([a-zA-Z0-9]+)_([a-zA-Z0-9]+)_([a-zA-Z]+)(\d{4})\.txt)");
    const int expMarkerStyles[] = {29, 33, 34, 21, 22, 23};
    int markerIdx = 0;

    for (const auto& entry : fs::directory_iterator(expDir)) {
        if (!entry.is_regular_file()) continue;
        std::string filename = entry.path().filename().string();
        std::smatch match;

        if (std::regex_match(filename, match, expPattern)) {
            std::string partStr = match[1].str();
            std::string specStr = match[2].str();
            std::string author  = match[3].str();
            std::string year    = match[4].str();

            author[0] = std::toupper(author[0]);

            ExpDataEntry exp;
            exp.sourceFile = filename;
            ParseParticleZA(partStr, exp.Z, exp.A);
            exp.canonicalSpecies = GetCanonicalSpecies(specStr);
            exp.label = "Exp: " + author + " et al. (" + year + ")";
            exp.markerStyle = expMarkerStyles[(markerIdx++) % 6];
            exp.markerColor = kBlack;
            exp.markerSize = 2.2;

            std::ifstream file(entry.path());
            if (!file.is_open()) continue;

            std::string line;
            while (std::getline(file, line)) {
                line = Trim(line);
                if (line.empty() || line[0] == '#' || line[0] == ';') continue;
                std::istringstream iss(line);
                double x, y;
                if (iss >> x >> y) {
                    exp.points.push_back({x, y});
                }
            }

            if (!exp.points.empty()) {
                entries.push_back(exp);
                std::cout << "  + [Found] " << filename << " | Particle (Z=" << exp.Z << ", A=" << exp.A 
                          << ") | Species: '" << exp.canonicalSpecies << "' | Points: " << exp.points.size() << "\n";
            }
        }
    }
    std::cout << "========================================================\n";
    return entries;
}

// ----------------------------------------------------------------------
// 6. 4PL Logistic Fit
// ----------------------------------------------------------------------
TF1* FitUniversalLogistic(TGraphErrors* gr, const std::string& fitName, Color_t color, Int_t lineStyle) {
    if (!gr || gr->GetN() < 3) return nullptr;

    Double_t* x = gr->GetX();
    Double_t* y = gr->GetY();
    int n = gr->GetN();

    Double_t xMin = x[0], xMax = x[n - 1];
    Double_t yMax = *std::max_element(y, y + n);

    if (xMin > xMax) std::swap(xMin, xMax);
    if (xMin <= 0.0) xMin = 1e-3;

    // Безопасное имя функции для ROOT
    std::string safeName = SanitizeRootName(fitName);

    TF1* fitFunc = new TF1(safeName.c_str(),
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

    fitFunc->SetParLimits(0, 0.0, std::max(6.0, yMax * 1.5));
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
    std::regex filePattern(R"(chem6_([^_]+)_[0-9\.]+-[A-Za-z]+_.*_LET\.txt)");

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
// 7. Построение графиков и сведение
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

    std::cout << "\n--> Processing Target Particle: " << targetPartStr 
              << " (" << displayName << " | Z=" << targetZ << ", A=" << targetA << ")\n";

    std::regex filePattern(
        R"(chem6_)" + targetPartStr + 
        R"(_([0-9\.]+)-([A-Za-z]+)_(G4EmDNAPhysics_option\d+)_(G4EmDNAChemistry_option\d+)_(IRT_syn|IRT)_(MI-ON|MI-OFF).*_LET\.txt)"
    );

    std::map<Int_t, std::string> tagToName;
    std::map<Int_t, std::map<SimConfig, std::vector<DataPoint>>> rawData;

    for (const auto& entry : fs::directory_iterator(dataDir)) {
        if (!entry.is_regular_file()) continue;
        std::string filename = entry.path().filename().string();
        std::smatch match;

        if (std::regex_match(filename, match, filePattern)) {
            std::string eValStr = match[1].str();
            std::string eUnitStr = match[2].str();
            
            SimConfig cfg;
            cfg.physList   = match[3].str();
            cfg.chemList   = match[4].str();
            cfg.chemMethod = match[5].str();
            cfg.multiIon   = match[6].str();

            Double_t eMeV = ParseEnergyToMeV(eValStr, eUnitStr);
            Double_t let_g4calc = ComputeLET_G4(emCalculator, targetPartStr, eMeV, water);

            std::ifstream file(entry.path());
            if (!file.is_open()) continue;

            std::string dummy, string_key, string_value;
            Double_t let_sim_file = 0.0, let_sim_sigma = 0.0;

            while (file >> dummy >> let_sim_file >> dummy >> let_sim_sigma) {
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
                    pt.let_g4calc = let_g4calc;
                    pt.g_val      = gVal;
                    pt.g_err      = gValSigma;

                    rawData[tagID][cfg].push_back(pt);
                }
            }
            file.close();
        }
    }

    if (rawData.empty()) {
        std::cerr << "[Warning] No simulation data files matched for: " << targetPartStr << std::endl;
        return;
    }

    std::string safePartName = SanitizeRootName(targetPartStr);
    std::string outRootPath = outputPlotsDir + "/G_vs_LET_" + safePartName + ".root";
    std::unique_ptr<TFile> outFile(TFile::Open(outRootPath.c_str(), "RECREATE"));

    for (const auto& specPair : rawData) {
        Int_t tag = specPair.first;
        std::string rawSpecName = tagToName[tag];
        std::string safeSpecName = SanitizeRootName(rawSpecName);
        std::string canonSpecies = GetCanonicalSpecies(rawSpecName);
        const auto& configMap = specPair.second;

        // Создаем динамический TCanvas
        TCanvas* c = new TCanvas(
            Form("c_%s_%s", safePartName.c_str(), safeSpecName.c_str()), 
            Form("G vs LET - %s (%s)", rawSpecName.c_str(), displayName.c_str()), 
            1450, 850
        );
        
        c->SetRightMargin(0.38);
        c->SetLeftMargin(0.10);
        c->SetBottomMargin(0.12);
        c->SetTopMargin(0.08);
        c->SetLogx();
        c->SetGrid();

        TMultiGraph* mg = new TMultiGraph();
        mg->SetName(Form("mg_%s_%s", safePartName.c_str(), safeSpecName.c_str()));
        mg->SetTitle(Form("%s Yield vs LET in Water (%s);LET (keV/#mum);G-value (molecules / 100 eV)", 
                         rawSpecName.c_str(), displayName.c_str()));

        TLegend* leg = new TLegend(0.63, 0.12, 0.99, 0.92);
        leg->SetNColumns(2);
        leg->SetBorderSize(1);
        leg->SetFillStyle(1001);
        leg->SetFillColorAlpha(kWhite, 0.90);
        leg->SetTextSize(0.021);
        leg->SetHeader(Form("Species: %s | %s", rawSpecName.c_str(), displayName.c_str()), "C");

        std::vector<TGraphErrors*> createdSimGraphs;
        std::vector<TF1*> createdFits;

        for (const auto& cfgPair : configMap) {
            const SimConfig& cfg = cfgPair.first;
            const std::vector<DataPoint>& points = cfgPair.second;
            if (points.empty()) continue;

            std::vector<Double_t> xVal, yVal, xErr, yErr;
            for (const auto& p : points) {
                xVal.push_back(p.let_g4calc);
                yVal.push_back(p.g_val);
                xErr.push_back(0.0);
                yErr.push_back(p.g_err);
            }

            std::string safeId = SanitizeRootName(cfg.GetIdString());
            TGraphErrors* gr = new TGraphErrors(xVal.size(), xVal.data(), yVal.data(), xErr.data(), yErr.data());
            gr->Sort();
            gr->SetName(Form("gr_%s_%s", safeSpecName.c_str(), safeId.c_str()));

            ConfigVisualAttr attr = GetVisualAttributes(cfg);
            gr->SetMarkerColor(attr.color);
            gr->SetLineColor(attr.color);
            gr->SetMarkerStyle(attr.markerStyle);
            gr->SetMarkerSize(1.1);
            gr->SetLineWidth(attr.lineWidth);

            std::string fitName = "fit_" + safeSpecName + "_" + safeId;
            TF1* f = FitUniversalLogistic(gr, fitName, attr.color, attr.lineStyle);
            if (f) createdFits.push_back(f);

            mg->Add(gr, "P");
            leg->AddEntry(gr, cfg.GetShortLabel().c_str(), "lp");
            createdSimGraphs.push_back(gr);
        }

        // Наложение экспериментов (добавляются в mg последними, чтобы быть поверх)
        std::vector<TGraph*> createdExpGraphs;
        int expCountForCanvas = 0;

        for (const auto& exp : expConfigs) {
            bool partMatch = (exp.Z == targetZ && exp.A == targetA);
            bool specMatch = (exp.canonicalSpecies == canonSpecies);

            if (partMatch && specMatch && !exp.points.empty()) {
                std::vector<Double_t> expX, expY;
                for (const auto& pt : exp.points) {
                    expX.push_back(pt.first);
                    expY.push_back(pt.second);
                }

                std::string expId = SanitizeRootName(exp.canonicalSpecies + "_" + exp.sourceFile);
                TGraph* grExp = new TGraph(expX.size(), expX.data(), expY.data());
                grExp->SetName(Form("grExp_%s", expId.c_str()));
                grExp->SetMarkerStyle(exp.markerStyle);
                grExp->SetMarkerSize(exp.markerSize);
                grExp->SetMarkerColor(exp.markerColor);
                grExp->SetLineColor(exp.markerColor);
                
                mg->Add(grExp, "P");
                leg->AddEntry(grExp, exp.label.c_str(), "p");
                createdExpGraphs.push_back(grExp);
                expCountForCanvas++;

                std::cout << "  -> [MATCH & DRAW] Applied " << exp.label 
                          << " (" << exp.points.size() << " points) onto canvas '" << rawSpecName << "'\n";
            }
        }

        if (expCountForCanvas == 0) {
            std::cout << "  -> [No Exp] No benchmark files found for species '" << rawSpecName 
                      << "' (canonical: '" << canonSpecies << "')\n";
        }

        // Отрисовка холста
        c->cd();
        mg->Draw("AP");
        mg->GetXaxis()->SetTitle("LET (keV/#mum)");
        mg->GetYaxis()->SetTitle("G value (molecules / 100 eV)");
        mg->GetXaxis()->SetTitleSize(0.04);
        mg->GetYaxis()->SetTitleSize(0.04);
        mg->GetXaxis()->SetLabelSize(0.035);
        mg->GetYaxis()->SetLabelSize(0.035);

        leg->Draw();

        std::string outPlotPath = outputPlotsDir + "/G_vs_LET_all_models_" + safePartName + "_" + safeSpecName + ".png";
        c->SaveAs(outPlotPath.c_str());

        // Запись в ROOT файл
        if (outFile && outFile->IsOpen()) {
            outFile->cd();
            for (auto* g : createdSimGraphs) g->Write();
            for (auto* g : createdExpGraphs) g->Write();
        }

        // Корректное освобождение памяти холста
        delete c;
    }

    if (outFile && outFile->IsOpen()) {
        outFile->Write();
        outFile->Close();
    }
    std::cout << "[Saved] Plots & ROOT file: " << outRootPath << "\n";
}

// ----------------------------------------------------------------------
// 8. Main
// ----------------------------------------------------------------------
int main(int argc, char** argv) {
    std::string targetPart = (argc > 1) ? argv[1] : "ALL";
    std::string dataDir    = (argc > 2) ? argv[2] : "output/data";
    std::string plotsDir   = (argc > 3) ? argv[3] : "output/plots";
    std::string expDir     = (argc > 4) ? argv[4] : "input/experimental_data";

    fs::create_directories(plotsDir);

    auto runManager = std::make_unique<G4RunManager>();
    runManager->SetVerboseLevel(0);
    runManager->SetUserInitialization(new MinimalDetectorConstruction());
    runManager->SetUserInitialization(new MinimalPhysicsList());
    runManager->SetUserInitialization(new DummyActionInitialization());
    
    runManager->Initialize();
    runManager->BeamOn(0);

    G4NistManager* nist = G4NistManager::Instance();
    G4Material* water = nist->FindOrBuildMaterial("G4_WATER");
    auto emCalculator = std::make_unique<G4EmCalculator>();

    // Загрузка экспериментов
    std::vector<ExpDataEntry> expConfigs = LoadAllExperimentalFiles(expDir);

    if (targetPart == "ALL" || targetPart.empty()) {
        std::vector<std::string> allParticles = DiscoverAllParticles(dataDir);
        if (allParticles.empty()) {
            std::cerr << "[Error] No data files found in " << dataDir << std::endl;
            return 1;
        }

        std::cout << "\n[Auto-Mode] Discovered " << allParticles.size() << " particles in dataDir: ";
        for (const auto& p : allParticles) std::cout << p << " ";
        std::cout << "\n";

        for (const auto& p : allParticles) {
            ProcessParticleValidation(p, dataDir, plotsDir, expConfigs, emCalculator.get(), water);
        }
    } else {
        ProcessParticleValidation(targetPart, dataDir, plotsDir, expConfigs, emCalculator.get(), water);
    }

    emCalculator.reset();
    runManager.reset();

    std::cout << "\n[SUCCESS] Completed all validation plots successfully.\n";
    return 0;
}