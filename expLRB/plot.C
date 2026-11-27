// plot.C
// Запуск: root -l -q 'plot.C+' (или '../plot.C+')

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <TCanvas.h>
#include <TDirectory.h>
#include <TFile.h>
#include <TGraph.h>
#include <TGraphAsymmErrors.h>
#include <TH1D.h>
#include <TLatex.h>
#include <TLegend.h>
#include <TLine.h>
#include <TMath.h>
#include <TPad.h>
#include <TROOT.h>
#include <TString.h>
#include <TStyle.h>
#include <TTree.h>

namespace fs = std::filesystem;

// ============================================================================
// 1. ЦЕНТРАЛЬНЫЙ БЛОК КОНФИГУРАЦИИ (ВСЁ НАСТРАИВАЕТСЯ ЗДЕСЬ)
// ============================================================================
struct ScenarioConfig {
    std::string tag;              // Идентификатор для имён файлов и папок в output.root
    std::string legendLabel;      // Подпись в легенде
    std::string rootFile;         // Путь к входному .root файлу
    double      nCells;           // Число клеток (0 -> не переводить в Гр, выводить MeV)
    double      rNucleusUm;       // Радиус мишени [мкм]
    double      rhoGcm3;          // Плотность мишени [г/см3]
    Int_t       color;
    Int_t       lineStyle;
};

struct PlotConfig {
    bool        enabled{true};
    bool        autoRange{true};  // true -> границы осей строятся строго по данным файла
    bool        logX{false};
    bool        logY{true};
    double      xMin{0.0}, xMax{1.0};
    double      yMin{1e-6}, yMax{1.0};
    double      ratioMin{0.0}, ratioMax{2.5};
    int         rebin{1};
    std::string titlePrefix;
    std::string xTitle;
    std::string yTitle;
    std::string ratioTitle{"Ratio to Ref"};
    double      legX1{0.55}, legY1{0.62}, legX2{0.92}, legY2{0.89};
    double      lineWidth{2.0};
    double      textSize{0.032};
};

struct AnalysisConfig {
    // Выходные пути и формат
    std::string outputDir      = "output/plots/analysis";
    std::string outputRootFile = "output/plots/analysis/lrb_analysis_results.root";
    std::string outputCsvFile  = "output/plots/analysis/lrb_dosimetric_audit.csv";
    std::string imageFormat    = "svg"; // svg, pdf, png

    // Размеры холстов и поля
    int    canvasWidth      = 1920;
    int    canvasHeight     = 1080;
    int    gridCanvasWidth  = 2400;
    int    gridCanvasHeight = 1600;
    double leftMargin       = 0.11;
    double rightMargin      = 0.05;
    double topMargin        = 0.09; // Место под заголовок над рамкой
    double bottomMargin     = 0.15; // Запас под нижние индексы (E_{kin}, E_{dep})

    // Нормировка на число первичных частиц
    double      normPrimaries = 1.0e6;
    std::string normLabel     = "10^{6} primaries";

    // Входные файлы: сгенерировано scripts/gen_macros.sh из config/runs.conf.
    // Чтобы изменить список сценариев — правьте config/runs.conf и перегенерируйте.
    std::vector<ScenarioConfig> scenarios = {
#include "config/plot_scenarios.inc"
    };

    // Настройки графиков
    PlotConfig plotDoseBars {
        true, true, false, true, 0, 10, 1e-6, 1.0, 0, 2.5, 1,
        "Dose by Particle Family",
        "Particle family",
        "Absorbed dose [Gy per 10^{6} primaries]",
        "", 0.56, 0.72, 0.92, 0.89, 1.5, 0.032
    };

    PlotConfig plotDoseShareBars {
        true, false, false, false, 0, 10, 0.0, 115.0, 0, 2.5, 1,
        "Dose Share by Particle Family",
        "Particle family",
        "Share of total absorbed dose [%]",
        "", 0.56, 0.72, 0.92, 0.89, 1.5, 0.032
    };

    PlotConfig plotSubChannels {
        true, true, false, true, 0, 4, 1e-6, 1.0, 0, 2.5, 1,
        "Energy Deposition by Physical Sub-Channel",
        "Physical sub-channel",
        "Absorbed dose [Gy per 10^{6} primaries]",
        "", 0.50, 0.66, 0.92, 0.88, 1.5, 0.030
    };

    PlotConfig plotEdepEventDist {
        true, true, true, true, 1e-4, 1e2, 1e-7, 1.0, 0, 2.5, 1,
        "Event Energy Deposition Distribution",
        "Energy deposited in target per event, E_{dep} [MeV]",
        "dN / d(ln E_{dep}) per primary",
        "", 0.58, 0.68, 0.92, 0.88, 2.0, 0.032
    };

    PlotConfig plotEdepBySpecies {
        true, true, true, true, 1e-4, 1e2, 1e-7, 1.0, 0, 2.5, 1,
        "Energy Deposition by Particle Family",
        "Energy deposited in target per event, E_{dep} [MeV]",
        "dN / d(ln E_{dep}) per primary",
        "", 0.14, 0.34, 0.45, 0.88, 2.0, 0.030
    };

    PlotConfig plotEdepBySpeciesLinear {
        false, true, false, false, 0.0, 5.0, 0.0, 0.0, 0, 2.5, 1,
        "Energy Deposition by Particle Family (linear, direct-flight peak)",
        "Energy deposited in target, E_{dep} [MeV]",
        "Events per primary (dN/dE)",
        "", 0.14, 0.45, 0.45, 0.88, 2.0, 0.030
    };

    PlotConfig plotFragCounts {
        true, true, false, true, 0, 8, 1.0, 1e7, 0, 2.5, 1,
        "Secondary Fragment Multiplicity",
        "Secondary fragment family",
        "Counts per 10^{6} primaries",
        "", 0.54, 0.71, 0.92, 0.88, 1.5, 0.030
    };

    PlotConfig plotICSD {
        true, true, false, true, 0.0, 10000.0, 1e-6, 1.0, 0.0, 2.5, 4,
        "ICSD in Target Volume",
        "Ionisation cluster size, #nu",
        "Normalized frequency per hit target",
        "Ratio to Ref", 0.58, 0.68, 0.92, 0.88, 2.0, 0.032
    };

    PlotConfig plotICSDFull {
        true, true, true, true, 1.0, 1e5, 1e-7, 1.0, 0.0, 2.5, 1,
        "Full-Range Event ICSD (BioData)",
        "Ionisation cluster size per event, #nu",
        "dN / d(ln #nu) per primary",
        "", 0.58, 0.68, 0.92, 0.88, 2.0, 0.032
    };

    PlotConfig plotSpectra {
        true, true, true, true, 1e-3, 2e3, 1e-8, 1.0, 0.0, 2.5, 1,
        "Entrance Kinetic Energy Spectra",
        "Kinetic energy at target entrance, E_{kin} [MeV]",
        "dN / d(ln E) per primary",
        "Ratio to Ref", 0.14, 0.36, 0.46, 0.88, 2.0, 0.030
    };

    PlotConfig plotKinematicsMeVu {
        true, true, true, true, 1e-2, 300.0, 1e-8, 1.0, 0.0, 2.5, 1,
        "Product Kinematics (All Entrance Spectra in E_{kin}/A)",
        "Kinetic energy per nucleon at entrance, E_{kin}/A [MeV/u]",
        "dN / d(ln(E/A)) per primary",
        "", 0.14, 0.38, 0.46, 0.88, 2.2, 0.030
    };

    PlotConfig plotSpectraGrid {
        false, true, true, true, 1e-3, 2e3, 1e-8, 1.0, 0.0, 2.5, 1,
        "All Entrance Energy Spectra Overview",
        "E_{kin} [MeV] (or E_{kin}/A [MeV/u] on panel 9)",
        "dN / d(ln E) per primary",
        "", 0.50, 0.70, 0.90, 0.88, 1.8, 0.042
    };

    PlotConfig plotHeavyPerNucl {
        false, true, true, true, 1e-3, 200.0, 1e-8, 1.0, 0.0, 2.5, 1,
        "Heavy Fragment Spectra (Z #geq 3)",
        "Kinetic energy per nucleon, E_{kin}/A [MeV/u]",
        "dN / d(ln(E/A)) per primary",
        "Ratio to Ref", 0.14, 0.75, 0.48, 0.88, 2.0, 0.032
    };

    PlotConfig plotCompareDose {
        true, true, false, true, 0, 10, 1e-6, 1.0, 0.0, 2.5, 1,
        "Dose Comparison Across Scenarios",
        "Particle family",
        "Absorbed dose [Gy per 10^{6} primaries]",
        "", 0.50, 0.68, 0.92, 0.88, 1.5, 0.030
    };

    PlotConfig plotCompareFrag {
        true, true, false, true, 0, 8, 1.0, 1e7, 0, 2.5, 1,
        "Secondary Fragment Yield Comparison",
        "Secondary fragment family",
        "Target entrances per 10^{6} primaries",
        "", 0.50, 0.68, 0.92, 0.88, 1.5, 0.030
    };

    // HadronVertexZ comparison needs its own axes/units: the overlay must be in
    // z [mm], not the entrance-spectra E_kin axis it used to borrow.
    PlotConfig plotCompareHVZ {
        true, true, false, true, 0, 30, 1.0, 1e7, 0.0, 2.5, 1,
        "Hadronic Vertex Z Comparison",
        "z [mm]",
        "Hadronic vertices per 10^{6} primaries",
        "", 0.52, 0.66, 0.93, 0.88, 1.8, 0.026
    };
};

// ============================================================================
// 2. СТРУКТУРЫ ДАННЫХ (СТРОГО ИЗ ROOT-ФАЙЛА)
// ============================================================================
enum class FamilyIdx : size_t {
    Total = 0, PrimaryEM, HadTotal,
    Proton, Alpha, Deuteron, Triton, Helium3, HeavyIon, Neutron, Other,
    kCount
};

struct FamilyStats {
    std::string name;
    double   edepMeV{0.0};
    Long64_t nHitEvents{0};
    Long64_t totalIons{0};
    double   sumIons{0.0};
    double   sumIons2{0.0};
    Long64_t c1{0}, c100{0}, c500{0}, c1000{0};

    void Add(double edep, int ions) {
        edepMeV += edep;
        if (ions > 0) {
            ++nHitEvents;
            totalIons += ions;
            const double d = static_cast<double>(ions);
            sumIons  += d;
            sumIons2 += d * d;
            if (ions >= 1)    ++c1;
            if (ions >= 100)  ++c100;
            if (ions >= 500)  ++c500;
            if (ions >= 1000) ++c1000;
        }
    }

    double MeanM1() const { return (nHitEvents > 0) ? (sumIons / nHitEvents) : 0.0; }
    double DoseQ()  const { return (sumIons > 0.0) ? (sumIons2 / sumIons) : 0.0; }
    double Frac(Long64_t cnt) const { return (nHitEvents > 0) ? (static_cast<double>(cnt) / nHitEvents) : 0.0; }
};

struct ScenarioData {
    ScenarioConfig cfg;
    Long64_t       nEvents{0};
    double         targetMassKg{0.0};
    bool           hasMass{false};
    bool           hasSubChannels{false};  // true if the run's profile wrote the sub-channel columns

    static constexpr size_t kSubCount = 4;
    std::array<double,   kSubCount> subEdepMeV{};
    std::array<Long64_t, kSubCount> subIonCount{};

    std::array<FamilyStats, static_cast<size_t>(FamilyIdx::kCount)> fam;
    std::array<std::vector<double>, static_cast<size_t>(FamilyIdx::kCount)> hitEdepByFam;
    std::vector<int> hitIonsTot, hitIonsEM, hitIonsHad;

    TH1D* hIcsdEventTot{nullptr};
    TH1D* hIcsdTargetTot{nullptr};
    TH1D* hIcsdTargetEM{nullptr};
    TH1D* hIcsdTargetHad{nullptr};

    struct SpecEntry {
        std::string key;
        std::string label;
        double      massNumberA{1.0};
        Int_t       color{kBlack};
        Int_t       style{1};
        TH1D*       hist{nullptr};
        double      entries{0.0};
        double      meanMeV{0.0};
        double      maxMeV{0.0};
    };
    std::vector<SpecEntry> spectra;
    TH1D* hHeavyPerNucl{nullptr};

    // Dynamic per-fragment (Z,A) deposits from the vector branches Frag_*
    // ("fragments" group). Key = Z*1000 + A; value = {direct MeV, delta MeV}.
    bool hasFragments{false};
    std::map<long, std::pair<double, double>> fragEdep;
    std::map<long, long> fragIons;

    // Per-event deposits for the direct/delta species split (sparse: only >0
    // values are stored). evSubEdep: 0=primary direct, 1=primary delta-rays,
    // 2=fragment direct, 3=fragment delta-rays. evFragZ*: per fragment element Z.
    std::array<std::vector<double>, kSubCount> evSubEdep;
    std::map<int, std::vector<double>> evFragZDirect;
    std::map<int, std::vector<double>> evFragZDelta;
    std::vector<double> evPrimaryTotal;                // per-event primary (direct+delta)
    std::map<int, std::vector<double>> evFragZTotal;   // Z -> per-event total

    // Hadronic-interaction birth position along the beam axis (H1, mm).
    TH1D* hHadronVertexZ{nullptr};
};

// ============================================================================
// 3. ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ
// ============================================================================
static void SetupStyle() {
    TH1::AddDirectory(kFALSE);
    gStyle->SetOptStat(0);
    gStyle->SetCanvasPreferGL(kTRUE);
    gStyle->SetPadBorderSize(0);
    gStyle->SetFrameLineWidth(1);
}

static std::string TrimCopy(const std::string& s) {
    const auto b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return {};
    const auto e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

// Optional cosmetic overrides from config/analysis.conf (KEY = VALUE, # comments).
// Unknown keys are reported and ignored; absent file -> built-in defaults.
static void ApplyAnalysisConfFile(AnalysisConfig& ac, const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        std::cout << "[conf] " << path << " not found; using built-in defaults\n";
        return;
    }
    std::string line;
    int applied = 0;
    while (std::getline(in, line)) {
        const auto hash = line.find('#');
        if (hash != std::string::npos) line = line.substr(0, hash);
        const auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        const std::string key = TrimCopy(line.substr(0, eq));
        const std::string val = TrimCopy(line.substr(eq + 1));
        if (key.empty()) continue;
        if (key == "output-dir")            ac.outputDir = val;
        else if (key == "output-root-file") ac.outputRootFile = val;
        else if (key == "output-csv-file")  ac.outputCsvFile = val;
        else if (key == "image-format")     ac.imageFormat = val;
        else if (key == "norm-primaries")   ac.normPrimaries = std::stod(val);
        else if (key == "norm-label")       ac.normLabel = val;
        else { std::cerr << "[conf] unknown key '" << key << "' in " << path << "\n"; continue; }
        ++applied;
    }
    std::cout << "[conf] applied " << applied << " override(s) from " << path << "\n";
}

static double ComputeTargetMassKg(const ScenarioConfig& sc) {
    if (sc.nCells <= 0.0 || sc.rNucleusUm <= 0.0 || sc.rhoGcm3 <= 0.0) return 0.0;
    const double rCm = sc.rNucleusUm * 1.0e-4;
    const double volCm3 = (4.0 / 3.0) * TMath::Pi() * rCm * rCm * rCm;
    return sc.nCells * volCm3 * sc.rhoGcm3 * 1.0e-3;
}

static double EdepMeVToGy(double edepMeV, double massKg) {
    constexpr double kMeVToJ = 1.602176634e-13;
    return edepMeV * kMeVToJ / massKg;
}

static std::string SavePlot(TCanvas* c, const AnalysisConfig& ac, const std::string& baseName) {
    std::string path = ac.outputDir + "/" + baseName + "." + ac.imageFormat;
    c->SaveAs(path.c_str());
    return path;
}

static TGraphAsymmErrors* HistToCleanGraph(const TH1D* h, double yThreshold, bool scaleByLogBinWidth) {
    auto* g = new TGraphAsymmErrors();
    if (!h) return g;

    for (int b = 1; b <= h->GetNbinsX(); ++b) {
        double content = h->GetBinContent(b);
        double err     = h->GetBinError(b);
        if (content <= 0.0) continue;

        if (scaleByLogBinWidth) {
            const double low  = h->GetBinLowEdge(b);
            const double high = low + h->GetBinWidth(b);
            if (low <= 0.0 || high <= low) continue;
            const double dlnE = std::log(high / low);
            content /= dlnE;
            err     /= dlnE;
        }

        if (content > yThreshold) {
            const int pt = g->GetN();
            g->SetPoint(pt, h->GetBinCenter(b), content);
            g->SetPointError(pt, 0.0, 0.0, err, err);
        }
    }
    return g;
}

static void UpdateBoundsFromHist(const TH1D* h, bool scaleByLogBinWidth,
                                 double& xMin, double& xMax, double& yMinPos, double& yMax) {
    if (!h) return;
    for (int b = 1; b <= h->GetNbinsX(); ++b) {
        double c = h->GetBinContent(b);
        if (c <= 0.0) continue;
        const double low  = h->GetBinLowEdge(b);
        const double high = low + h->GetBinWidth(b);
        if (scaleByLogBinWidth) {
            if (low <= 0.0 || high <= low) continue;
            c /= std::log(high / low);
        }
        xMin    = std::min(xMin, low);
        xMax    = std::max(xMax, high);
        yMinPos = std::min(yMinPos, c);
        yMax    = std::max(yMax, c);
    }
}

static TH1D* ScaleHistAxisX(const TH1D* src, double divFactor, const char* newName) {
    if (!src || divFactor <= 0.0) return nullptr;
    const TAxis* ax = src->GetXaxis();
    const int nb = ax->GetNbins();
    std::vector<double> edges(nb + 1);
    for (int i = 0; i <= nb; ++i) {
        edges[i] = ax->GetBinLowEdge(i + 1) / divFactor;
    }
    auto* hOut = new TH1D(newName, "", nb, edges.data());
    hOut->Sumw2();
    for (int b = 1; b <= nb; ++b) {
        hOut->SetBinContent(b, src->GetBinContent(b));
        hOut->SetBinError(b, src->GetBinError(b));
    }
    return hOut;
}

static TH1D* BuildLogHistFromDoubles(const std::vector<double>& vals, double lo, double hi,
                                     int nLogBins, double scaleWeight, const char* name) {
    std::vector<double> edges(nLogBins + 1);
    const double lLo = std::log10(lo), lHi = std::log10(hi);
    for (int i = 0; i <= nLogBins; ++i) {
        edges[i] = std::pow(10.0, lLo + (lHi - lLo) * i / nLogBins);
    }
    auto* h = new TH1D(name, "", nLogBins, edges.data());
    h->Sumw2();
    for (double v : vals) {
        if (v > 0.0) h->Fill(v);
    }
    if (scaleWeight != 1.0) h->Scale(scaleWeight);
    return h;
}

static TH1D* BuildLinearHistFromDoubles(const std::vector<double>& vals, double lo, double hi,
                                        int nBins, double scaleWeight, const char* name) {
    auto* h = new TH1D(name, "", nBins, lo, hi);
    h->Sumw2();
    for (double v : vals) {
        if (v >= lo && v <= hi) h->Fill(v);
    }
    if (scaleWeight != 1.0) h->Scale(scaleWeight);
    return h;
}

static void DrawDualLegend(TPad* pad,
                           const std::vector<TObject*>& objs,
                           const std::vector<std::string>& labels,
                           const std::vector<std::string>& opts,
                           double x1, double y1, double x2, double y2, double textSize,
                           int nColumns = 1) {
    if (objs.empty()) return;
    pad->cd();
    const double iconWidth = 0.050;
    const double gap       = 0.015;

    auto* legText = new TLegend(x1, y1, x2 - iconWidth - gap, y2);
    legText->SetBorderSize(0);
    legText->SetFillStyle(0);
    legText->SetTextSize(textSize);
    legText->SetTextAlign(32);
    legText->SetMargin(0.0);
    legText->SetEntrySeparation(0.40);
    if (nColumns > 1) legText->SetNColumns(nColumns);

    auto* legIcons = new TLegend(x2 - iconWidth, y1, x2, y2);
    legIcons->SetBorderSize(0);
    legIcons->SetFillStyle(0);
    legIcons->SetMargin(1.0);
    legIcons->SetEntrySeparation(0.40);
    if (nColumns > 1) legIcons->SetNColumns(nColumns);

    for (size_t i = 0; i < objs.size(); ++i) {
        std::string fmtLabel = Form("#scale[0.9]{#bf{%s}}", labels[i].c_str());
        legText->AddEntry((TObject*)nullptr, fmtLabel.c_str(), "");
        legIcons->AddEntry(objs[i], "", opts[i].c_str());
    }
    legText->Draw();
    legIcons->Draw();
}

// Заголовок строго в верхнем поле (вне области кривых и легенды — никогда не обрезается)
static void DrawHeaderBanner(TPad* pad, const std::string& text, double xLeft, double yTop = 0.942, double size = 0.032) {
    pad->cd();
    TLatex tex;
    tex.SetNDC();
    tex.SetTextFont(42);
    tex.SetTextAlign(12);
    tex.SetTextSize(size);
    tex.DrawLatex(xLeft, yTop, text.c_str());
}

// Distinct per-scenario colour assigned by position, so comparison overlays that
// would otherwise reuse the auto-generated palette (monolayer/suspension share
// colours) stay readable.
static Int_t DistinctColorByIndex(int i) {
    static const Int_t pal[] = {
        kBlue + 1,   kRed + 1,     kGreen + 2,  kOrange + 7, kMagenta + 1,
        kCyan + 2,   kViolet + 1,  kTeal + 2,   kPink + 1,   kAzure + 1,
        kSpring + 3, kViolet + 6,  kOrange - 3, kGreen + 3,  kRed - 4, kBlue - 7
    };
    const int n = static_cast<int>(sizeof(pal) / sizeof(pal[0]));
    return pal[((i % n) + n) % n];
}

// Compact a verbose scenario label so crowded comparison legends stay legible.
static std::string ShortLegendLabel(const std::string& s) {
    std::string r = s;
    auto rep = [&](const std::string& a, const std::string& b) {
        for (size_t p = r.find(a); p != std::string::npos; p = r.find(a, p + b.size())) {
            r.replace(p, a.size(), b);
        }
    };
    rep("no hadronic", "nohad");
    rep("no filter",   "nF");
    rep("no cells",    "nC");
    rep("Monolayer",   "ML");
    rep("Suspension",  "Susp");
    rep("hadronic",    "had");
    rep("filter",      "F");
    rep("cells",       "C");
    for (size_t p = r.find(", "); p != std::string::npos; p = r.find(", ", p)) r.replace(p, 2, " ");
    return r;
}

// Single multi-column legend (icon + label together). Used where many entries
// (comparison overlays/bars) would otherwise overflow a one-column legend box.
static void DrawColumnLegend(TPad* pad,
                             const std::vector<TObject*>& objs,
                             const std::vector<std::string>& labels,
                             const std::vector<std::string>& opts,
                             double x1, double y1, double x2, double y2,
                             double textSize, int nColumns) {
    if (objs.empty()) return;
    pad->cd();
    auto* leg = new TLegend(x1, y1, x2, y2);
    leg->SetBorderSize(1);
    leg->SetFillStyle(1001);
    leg->SetFillColor(kWhite);
    leg->SetTextSize(textSize);
    leg->SetEntrySeparation(0.10);
    if (nColumns > 1) leg->SetNColumns(nColumns);
    for (size_t i = 0; i < objs.size(); ++i) {
        leg->AddEntry(objs[i], labels[i].c_str(), opts[i].c_str());
    }
    leg->Draw();
}

static TH1D* CloneHistStrict(TFile* f, const char* name, const std::string& tag) {
    auto* h = dynamic_cast<TH1D*>(f->Get(name));
    if (!h) return nullptr;
    auto* c = static_cast<TH1D*>(h->Clone(Form("%s_%s", name, tag.c_str())));
    c->SetDirectory(nullptr);
    return c;
}

// ============================================================================
// 4. ЧТЕНИЕ ROOT-ФАЙЛА (СТРОГО БЕЗ ПОДМЕНОК)
// ============================================================================
static bool LoadScenarioData(const ScenarioConfig& sc, ScenarioData& out) {
    if (!fs::exists(sc.rootFile)) {
        std::cerr << "[ERROR] File does not exist: " << sc.rootFile << std::endl;
        return false;
    }

    TFile* file = TFile::Open(sc.rootFile.c_str(), "READ");
    if (!file || file->IsZombie()) {
        std::cerr << "[ERROR] Failed to open ROOT file: " << sc.rootFile << std::endl;
        if (file) delete file;
        return false;
    }

    TTree* tree = dynamic_cast<TTree*>(file->Get("BioData"));
    if (!tree) {
        std::cerr << "[ERROR] Required TTree 'BioData' missing in: " << sc.rootFile << std::endl;
        file->Close();
        delete file;
        return false;
    }

    // Core columns exist in every scoring profile. Optional groups (species,
    // nonDNA, sub-channels) are read only when the run's profile wrote them, so
    // a "minimal" profile run still analyses cleanly instead of hard-erroring.
    const std::vector<const char*> coreBranches = {
        "EventID", "Edep_Total", "Edep_EM", "Edep_HadTotal",
        "Ions_Total", "Ions_EM", "Ions_HadTotal", "HitTargetCount"
    };
    for (const char* br : coreBranches) {
        if (!tree->GetBranch(br)) {
            std::cerr << "[ERROR] Required core branch '" << br << "' missing in 'BioData' of file: " << sc.rootFile << std::endl;
            file->Close();
            delete file;
            return false;
        }
    }
    auto hasBranch = [&](const char* br) { return tree->GetBranch(br) != nullptr; };
    auto setIfPresent = [&](const char* br, void* addr) {
        if (hasBranch(br)) tree->SetBranchAddress(br, addr);
    };

    out.cfg = sc;
    out.targetMassKg = ComputeTargetMassKg(sc);
    out.hasMass = (out.targetMassKg > 0.0);
    out.nEvents = tree->GetEntries();

    const std::array<std::string, static_cast<size_t>(FamilyIdx::kCount)> famNames = {
        "Total", "Primary EM (^{20}Ne)", "Hadronic Total",
        "Protons (p)", "Alphas (#alpha)", "Deuterons (d)", "Tritons (t)",
        "Helium-3 (^{3}He)", "Heavy (Z#geq3)", "Neutrons (n)", "Other & #gamma"
    };
    for (size_t i = 0; i < famNames.size(); ++i) out.fam[i].name = famNames[i];

    Double_t eTot = 0, eEM = 0, eHad = 0, eP = 0, eD = 0, eT = 0, eHe3 = 0, eA = 0, eHvy = 0, eN = 0, eOth = 0;
    Int_t    iTot = 0, iEM = 0, iHad = 0, iP = 0, iD = 0, iT = 0, iHe3 = 0, iA = 0, iHvy = 0, iN = 0, iOth = 0;

    Double_t subEdep[4] = {0, 0, 0, 0};
    Int_t    subIons[4] = {0, 0, 0, 0};

    tree->SetBranchAddress("Edep_Total",    &eTot);
    tree->SetBranchAddress("Edep_EM",       &eEM);
    tree->SetBranchAddress("Edep_HadTotal", &eHad);
    tree->SetBranchAddress("Ions_Total",    &iTot);
    tree->SetBranchAddress("Ions_EM",       &iEM);
    tree->SetBranchAddress("Ions_HadTotal", &iHad);

    setIfPresent("Edep_Proton",   &eP);
    setIfPresent("Edep_Deuteron", &eD);
    setIfPresent("Edep_Triton",   &eT);
    setIfPresent("Edep_Helium3",  &eHe3);
    setIfPresent("Edep_Alpha",    &eA);
    setIfPresent("Edep_HeavyIon", &eHvy);
    setIfPresent("Edep_Neutron",  &eN);
    setIfPresent("Edep_Other",    &eOth);

    setIfPresent("Ions_Proton",   &iP);
    setIfPresent("Ions_Deuteron", &iD);
    setIfPresent("Ions_Triton",   &iT);
    setIfPresent("Ions_Helium3",  &iHe3);
    setIfPresent("Ions_Alpha",    &iA);
    setIfPresent("Ions_HeavyIon", &iHvy);
    setIfPresent("Ions_Neutron",  &iN);
    setIfPresent("Ions_Other",    &iOth);

    out.hasSubChannels = hasBranch("Edep_PrimaryIonDirect") && hasBranch("Ions_PrimaryIonDirect");
    if (out.hasSubChannels) {
        tree->SetBranchAddress("Edep_PrimaryIonDirect",        &subEdep[0]);
        tree->SetBranchAddress("Edep_PrimaryDeltaRays",        &subEdep[1]);
        tree->SetBranchAddress("Edep_HadronicFragmentsDirect", &subEdep[2]);
        tree->SetBranchAddress("Edep_HadronicDeltaRays",       &subEdep[3]);
        tree->SetBranchAddress("Ions_PrimaryIonDirect",        &subIons[0]);
        tree->SetBranchAddress("Ions_PrimaryDeltaRays",        &subIons[1]);
        tree->SetBranchAddress("Ions_HadronicFragmentsDirect", &subIons[2]);
        tree->SetBranchAddress("Ions_HadronicDeltaRays",       &subIons[3]);
    }

    auto pushE = [&](FamilyIdx idx, double val) {
        if (val > 0.0) out.hitEdepByFam[static_cast<size_t>(idx)].push_back(val);
    };

    // Dynamic per-fragment (Z,A) vector branches (optional group).
    std::vector<int>*    bFragZ = nullptr;
    std::vector<int>*    bFragA = nullptr;
    std::vector<double>* bFragE = nullptr;
    std::vector<double>* bFragEd = nullptr;
    std::vector<int>*    bFragI = nullptr;
    out.hasFragments = hasBranch("Frag_Z") && hasBranch("Frag_A") &&
                       hasBranch("Frag_Edep") && hasBranch("Frag_EdepDelta");
    if (out.hasFragments) {
        tree->SetBranchAddress("Frag_Z",         &bFragZ);
        tree->SetBranchAddress("Frag_A",         &bFragA);
        tree->SetBranchAddress("Frag_Edep",      &bFragE);
        tree->SetBranchAddress("Frag_EdepDelta", &bFragEd);
        if (hasBranch("Frag_Ions")) tree->SetBranchAddress("Frag_Ions", &bFragI);
    }

    for (Long64_t ev = 0; ev < out.nEvents; ++ev) {
        tree->GetEntry(ev);
        out.fam[static_cast<size_t>(FamilyIdx::Total)].Add(eTot, iTot);
        out.fam[static_cast<size_t>(FamilyIdx::PrimaryEM)].Add(eEM, iEM);
        out.fam[static_cast<size_t>(FamilyIdx::HadTotal)].Add(eHad, iHad);
        out.fam[static_cast<size_t>(FamilyIdx::Proton)].Add(eP, iP);
        out.fam[static_cast<size_t>(FamilyIdx::Alpha)].Add(eA, iA);
        out.fam[static_cast<size_t>(FamilyIdx::Deuteron)].Add(eD, iD);
        out.fam[static_cast<size_t>(FamilyIdx::Triton)].Add(eT, iT);
        out.fam[static_cast<size_t>(FamilyIdx::Helium3)].Add(eHe3, iHe3);
        out.fam[static_cast<size_t>(FamilyIdx::HeavyIon)].Add(eHvy, iHvy);
        out.fam[static_cast<size_t>(FamilyIdx::Neutron)].Add(eN, iN);
        out.fam[static_cast<size_t>(FamilyIdx::Other)].Add(eOth, iOth);

        pushE(FamilyIdx::Total, eTot);
        pushE(FamilyIdx::PrimaryEM, eEM);
        pushE(FamilyIdx::HadTotal, eHad);
        pushE(FamilyIdx::Proton, eP);
        pushE(FamilyIdx::Alpha, eA);
        pushE(FamilyIdx::Deuteron, eD);
        pushE(FamilyIdx::Triton, eT);
        pushE(FamilyIdx::Helium3, eHe3);
        pushE(FamilyIdx::HeavyIon, eHvy);
        pushE(FamilyIdx::Neutron, eN);
        pushE(FamilyIdx::Other, eOth);

        for (size_t c = 0; c < ScenarioData::kSubCount; ++c) {
            out.subEdepMeV[c] += subEdep[c];
            out.subIonCount[c] += subIons[c];
            if (subEdep[c] > 0.0) out.evSubEdep[c].push_back(subEdep[c]);
        }
        // Primary total = ion's own deposit + its delta-rays (both are EM origin).
        if (subEdep[0] + subEdep[1] > 0.0) out.evPrimaryTotal.push_back(subEdep[0] + subEdep[1]);

        if (out.hasFragments && bFragZ && bFragA && bFragE && bFragEd) {
            size_t k = bFragZ->size();
            k = std::min(k, bFragA->size());
            k = std::min(k, bFragE->size());
            k = std::min(k, bFragEd->size());
            std::map<int, std::pair<double, double>> perZ;  // Z -> {direct, delta}
            for (size_t j = 0; j < k; ++j) {
                const int  Z  = (*bFragZ)[j];
                const long key = static_cast<long>(Z) * 1000L + (*bFragA)[j];
                auto& slot = out.fragEdep[key];
                const double ed = (*bFragEd)[j];
                slot.second += ed;                       // delta
                slot.first  += (*bFragE)[j] - ed;        // direct
                perZ[Z].first  += (*bFragE)[j] - ed;
                perZ[Z].second += ed;
                if (bFragI && j < bFragI->size()) out.fragIons[key] += (*bFragI)[j];
            }
            for (const auto& kv : perZ) {
                const double tot = kv.second.first + kv.second.second;
                if (tot > 0.0) out.evFragZTotal[kv.first].push_back(tot);
                if (kv.second.first  > 0.0) out.evFragZDirect[kv.first].push_back(kv.second.first);
                if (kv.second.second > 0.0) out.evFragZDelta[kv.first].push_back(kv.second.second);
            }
        }

        if (iTot > 0) out.hitIonsTot.push_back(iTot);
        if (iEM  > 0) out.hitIonsEM.push_back(iEM);
        if (iHad > 0) out.hitIonsHad.push_back(iHad);
    }

    out.hIcsdEventTot  = CloneHistStrict(file, "ICSD_Event_Total", sc.tag);
    out.hIcsdTargetTot = CloneHistStrict(file, "ICSD_Target_Total", sc.tag);
    out.hIcsdTargetEM  = CloneHistStrict(file, "ICSD_Target_EM", sc.tag);
    out.hIcsdTargetHad = CloneHistStrict(file, "ICSD_Target_Had", sc.tag);

    struct SpecMeta { const char* name; const char* label; double A; Int_t col; Int_t st; };
    const std::vector<SpecMeta> specList = {
        {"SpecIn_AllHad",   "All secondary hadrons", 1.0, kBlack,       1},
        {"SpecIn_Proton",   "Protons (p)",           1.0, kBlue + 1,    1},
        {"SpecIn_Alpha",    "Alphas (#alpha)",       4.0, kRed + 1,     1},
        {"SpecIn_Deuteron", "Deuterons (d)",         2.0, kGreen + 2,   1},
        {"SpecIn_Triton",   "Tritons (t)",           3.0, kMagenta + 1, 1},
        {"SpecIn_He3",      "Helium-3 (^{3}He)",     3.0, kOrange + 7,  1},
        {"SpecIn_Neutron",  "Neutrons (n)",          1.0, kGray + 2,    2}
    };

    for (const auto& sm : specList) {
        ScenarioData::SpecEntry entry;
        entry.key         = sm.name;
        entry.label       = sm.label;
        entry.massNumberA = sm.A;
        entry.color       = sm.col;
        entry.style       = sm.st;
        entry.hist        = CloneHistStrict(file, sm.name, sc.tag);
        if (entry.hist) {
            entry.entries = entry.hist->GetEntries();
            entry.meanMeV = (entry.entries > 0) ? entry.hist->GetMean() : 0.0;
            for (int b = entry.hist->GetNbinsX(); b >= 1; --b) {
                if (entry.hist->GetBinContent(b) > 0.0) {
                    entry.maxMeV = entry.hist->GetBinCenter(b);
                    break;
                }
            }
        }
        out.spectra.push_back(std::move(entry));
    }
    out.hHeavyPerNucl = CloneHistStrict(file, "SpecIn_HeavyPerNucl", sc.tag);
    out.hHadronVertexZ = CloneHistStrict(file, "HadronVertexZ", sc.tag);

    file->Close();
    delete file;
    return true;
}

// ============================================================================
// 5. ПЕЧАТЬ ТАБЛИЦЫ АУДИТА И ЭКСПОРТ CSV
// ============================================================================
static void PrintAuditTable(const ScenarioData& d, const AnalysisConfig& ac) {
    const double totEdep = d.fam[static_cast<size_t>(FamilyIdx::Total)].edepMeV;
    const double scale   = (d.nEvents > 0) ? (ac.normPrimaries / static_cast<double>(d.nEvents)) : 0.0;
    const char*  valCol  = d.hasMass ? "Dose [Gy]" : "Edep [MeV]";

    std::cout << "\n========================================================================================================================\n";
    std::cout << "  LRB JINR AUDIT | " << d.cfg.legendLabel << " | N_events = " << d.nEvents
              << " | Norm = " << ac.normPrimaries << "\n";
    std::cout << "========================================================================================================================\n";
    std::cout << std::left
              << std::setw(22) << "Particle Family"
              << std::setw(13) << "Dose Share"
              << std::setw(15) << valCol
              << std::setw(12) << "Mean M1"
              << std::setw(12) << "Dose Q"
              << std::setw(11) << "F(>=1)"
              << std::setw(11) << "F(>=100)"
              << std::setw(11) << "F(>=500)"
              << std::setw(11) << "F(>=1000)" << "\n";
    std::cout << "------------------------------------------------------------------------------------------------------------------------\n";

    for (const auto& f : d.fam) {
        const double share = (totEdep > 0.0) ? (100.0 * f.edepMeV / totEdep) : 0.0;
        const double val   = d.hasMass ? (EdepMeVToGy(f.edepMeV, d.targetMassKg) * scale)
                                       : (f.edepMeV * scale);
        std::cout << std::left  << std::setw(22) << f.name
                  << std::setw(13) << Form("%.2f %%", share)
                  << std::scientific << std::setprecision(3) << std::setw(15) << val
                  << std::fixed << std::setprecision(2)
                  << std::setw(12) << f.MeanM1()
                  << std::setw(12) << f.DoseQ()
                  << std::setprecision(4)
                  << std::setw(11) << f.Frac(f.c1)
                  << std::setw(11) << f.Frac(f.c100)
                  << std::setw(11) << f.Frac(f.c500)
                  << std::setw(11) << f.Frac(f.c1000) << "\n";
    }
    std::cout << "========================================================================================================================\n";
}

static void ExportCSV(const std::vector<ScenarioData>& all, const AnalysisConfig& ac) {
    std::ofstream out(ac.outputCsvFile);
    if (!out.is_open()) {
        std::cerr << "[ERROR] Cannot write CSV: " << ac.outputCsvFile << std::endl;
        return;
    }

    out << "Scenario,Label,N_Events,TargetMassKg,ParticleFamily,DoseSharePct,Edep_MeV_Norm,Dose_Gy_Norm,"
           "HitEvents,TotalIons,Mean_M1,Dose_Q,F_ge_1,F_ge_100,F_ge_500,F_ge_1000,"
           "SubEdepMeV_PrimaryIonDirect,SubEdepMeV_PrimaryDeltaRays,"
           "SubEdepMeV_HadronicFragmentsDirect,SubEdepMeV_HadronicDeltaRays\n";

    for (const auto& d : all) {
        const double totEdep = d.fam[static_cast<size_t>(FamilyIdx::Total)].edepMeV;
        const double scale   = (d.nEvents > 0) ? (ac.normPrimaries / static_cast<double>(d.nEvents)) : 0.0;
        for (const auto& f : d.fam) {
            const double share = (totEdep > 0.0) ? (100.0 * f.edepMeV / totEdep) : 0.0;
            const double eNorm = f.edepMeV * scale;
            const double dNorm = d.hasMass ? (EdepMeVToGy(f.edepMeV, d.targetMassKg) * scale) : 0.0;
            out << d.cfg.tag << ",\"" << d.cfg.legendLabel << "\"," << d.nEvents << ","
                << std::scientific << std::setprecision(6) << d.targetMassKg << ",\""
                << f.name << "\"," << std::fixed << std::setprecision(4) << share << ","
                << std::scientific << std::setprecision(6) << eNorm << "," << dNorm << ","
                << f.nHitEvents << "," << f.totalIons << ","
                << std::fixed << std::setprecision(3) << f.MeanM1() << "," << f.DoseQ() << ","
                << std::setprecision(5) << f.Frac(f.c1) << "," << f.Frac(f.c100) << ","
                << f.Frac(f.c500) << "," << f.Frac(f.c1000);
            const double subNorm = (d.nEvents > 0) ? (ac.normPrimaries / static_cast<double>(d.nEvents)) : 0.0;
            for (size_t c = 0; c < ScenarioData::kSubCount; ++c) {
                out << "," << std::scientific << std::setprecision(6) << (d.subEdepMeV[c] * subNorm);
            }
            out << "\n";
        }
    }
    out.close();
    std::cout << "[Saved CSV] " << ac.outputCsvFile << "\n";
}

// ============================================================================
// 6. ГРАФИКИ ДЛЯ ОДНОГО СЦЕНАРИЯ
// ============================================================================
static void PlotSingleDoseBars(const ScenarioData& d, const AnalysisConfig& ac, TDirectory* outDir) {
    if (d.nEvents <= 0) return;

    const std::vector<FamilyIdx> order = {
        FamilyIdx::Total, FamilyIdx::PrimaryEM, FamilyIdx::HadTotal,
        FamilyIdx::Alpha, FamilyIdx::Proton, FamilyIdx::Helium3,
        FamilyIdx::Deuteron, FamilyIdx::Triton, FamilyIdx::HeavyIon,
        FamilyIdx::Neutron, FamilyIdx::Other
    };
    const int nBins = static_cast<int>(order.size());
    const double scale = ac.normPrimaries / static_cast<double>(d.nEvents);
    const double totEdep = d.fam[static_cast<size_t>(FamilyIdx::Total)].edepMeV;

    if (ac.plotDoseBars.enabled) {
        const auto& pc = ac.plotDoseBars;
        auto* hDose = new TH1D(Form("hDose_%s", d.cfg.tag.c_str()), "", nBins, 0.0, static_cast<double>(nBins));
        double minPos = std::numeric_limits<double>::max(), maxVal = 0.0;

        for (int i = 0; i < nBins; ++i) {
            const auto& f = d.fam[static_cast<size_t>(order[i])];
            const double val = d.hasMass ? (EdepMeVToGy(f.edepMeV, d.targetMassKg) * scale)
                                         : (f.edepMeV * scale);
            hDose->SetBinContent(i + 1, val);
            hDose->GetXaxis()->SetBinLabel(i + 1, f.name.c_str());
            if (val > 0.0) { minPos = std::min(minPos, val); maxVal = std::max(maxVal, val); }
        }

        if (maxVal > 0.0) {
            auto* c = new TCanvas(Form("c_dose_%s", d.cfg.tag.c_str()), pc.titlePrefix.c_str(), ac.canvasWidth, ac.canvasHeight);
            auto* p = new TPad("p", "", 0, 0, 1, 1);
            p->SetLeftMargin(ac.leftMargin); p->SetRightMargin(ac.rightMargin);
            p->SetTopMargin(ac.topMargin);   p->SetBottomMargin(0.20);
            p->SetLogy(pc.logY); p->SetTickx(1); p->SetTicky(1);
            p->Draw(); p->cd();

            const double yMin = pc.autoRange ? std::pow(10.0, std::floor(std::log10(minPos)) - 0.5) : pc.yMin;
            const double yMax = pc.autoRange ? std::pow(10.0, std::ceil(std::log10(maxVal)) + 0.8)  : pc.yMax;
            hDose->GetYaxis()->SetRangeUser(yMin, yMax);
            hDose->GetXaxis()->SetLabelSize(0.034);
            hDose->GetXaxis()->LabelsOption("u");
            hDose->GetYaxis()->SetTitle(d.hasMass ? pc.yTitle.c_str() : Form("Energy deposited [MeV per %s]", ac.normLabel.c_str()));
            hDose->GetYaxis()->SetTitleSize(0.042);
            hDose->GetYaxis()->SetTitleOffset(1.12);
            hDose->GetYaxis()->SetLabelSize(0.036);
            hDose->GetYaxis()->CenterTitle(true);
            hDose->SetFillColor(d.cfg.color); hDose->SetFillStyle(1001);
            hDose->SetLineColor(kBlack); hDose->SetLineWidth(2);
            hDose->SetBarWidth(0.75); hDose->SetBarOffset(0.125);
            hDose->Draw("BAR");

            TLatex valTxt;
            valTxt.SetTextFont(42); valTxt.SetTextSize(0.023); valTxt.SetTextAlign(21);
            for (int i = 1; i <= nBins; ++i) {
                const double v = hDose->GetBinContent(i);
                if (v > 0.0) valTxt.DrawLatex(hDose->GetBinCenter(i), v * (pc.logY ? 1.25 : 1.03), Form("%.2e", v));
            }
            DrawHeaderBanner(p, Form("#bf{%s}  |  %s", pc.titlePrefix.c_str(), d.cfg.legendLabel.c_str()), ac.leftMargin, 0.942, pc.textSize);
            p->RedrawAxis();
            c->Update();
            std::cout << "  [Saved] " << SavePlot(c, ac, "Dose_BySpecies_" + d.cfg.tag) << "\n";
            delete c;
        }
        outDir->cd();
        hDose->Write();
        delete hDose;
    }

    if (ac.plotDoseShareBars.enabled && totEdep > 0.0) {
        const auto& pc = ac.plotDoseShareBars;
        auto* hShare = new TH1D(Form("hDoseShare_%s", d.cfg.tag.c_str()), "", nBins - 1, 0.0, static_cast<double>(nBins - 1));
        for (int i = 1; i < nBins; ++i) {
            const auto& f = d.fam[static_cast<size_t>(order[i])];
            hShare->SetBinContent(i, 100.0 * f.edepMeV / totEdep);
            hShare->GetXaxis()->SetBinLabel(i, f.name.c_str());
        }

        auto* c = new TCanvas(Form("c_share_%s", d.cfg.tag.c_str()), pc.titlePrefix.c_str(), ac.canvasWidth, ac.canvasHeight);
        auto* p = new TPad("p", "", 0, 0, 1, 1);
        p->SetLeftMargin(ac.leftMargin); p->SetRightMargin(ac.rightMargin);
        p->SetTopMargin(ac.topMargin);   p->SetBottomMargin(0.20);
        p->SetTickx(1); p->SetTicky(1);
        p->Draw(); p->cd();

        hShare->GetYaxis()->SetRangeUser(pc.yMin, pc.yMax);
        hShare->GetXaxis()->SetLabelSize(0.034);
        hShare->GetXaxis()->LabelsOption("u");
        hShare->GetYaxis()->SetTitle(pc.yTitle.c_str());
        hShare->GetYaxis()->SetTitleSize(0.042);
        hShare->GetYaxis()->SetTitleOffset(1.05);
        hShare->GetYaxis()->SetLabelSize(0.036);
        hShare->GetYaxis()->CenterTitle(true);
        hShare->SetFillColor(kTeal + 1); hShare->SetFillStyle(1001);
        hShare->SetLineColor(kBlack); hShare->SetLineWidth(2);
        hShare->SetBarWidth(0.75); hShare->SetBarOffset(0.125);
        hShare->Draw("BAR");

        TLatex pctTxt;
        pctTxt.SetTextFont(42); pctTxt.SetTextSize(0.025); pctTxt.SetTextAlign(21);
        for (int i = 1; i <= hShare->GetNbinsX(); ++i) {
            const double v = hShare->GetBinContent(i);
            pctTxt.DrawLatex(hShare->GetBinCenter(i), v + 1.8, Form("%.2f%%", v));
        }
        DrawHeaderBanner(p, Form("#bf{%s}  |  %s", pc.titlePrefix.c_str(), d.cfg.legendLabel.c_str()), ac.leftMargin, 0.942, pc.textSize);
        p->RedrawAxis();
        c->Update();
        std::cout << "  [Saved] " << SavePlot(c, ac, "DoseShare_" + d.cfg.tag) << "\n";
        delete c;

        outDir->cd();
        hShare->Write();
        delete hShare;
    }
}

static void PlotSingleSubChannels(const ScenarioData& d, const AnalysisConfig& ac, TDirectory* outDir) {
    if (!ac.plotSubChannels.enabled || d.nEvents <= 0) return;
    const auto& pc = ac.plotSubChannels;

    const char* labels[ScenarioData::kSubCount] = {
        "Primary ion direct", "Primary #delta-rays",
        "Hadronic frag. direct", "Hadronic #delta-rays"
    };
    const double scale = ac.normPrimaries / static_cast<double>(d.nEvents);

    double minPos = std::numeric_limits<double>::max(), maxVal = 0.0;
    auto* h = new TH1D(Form("hSub_%s", d.cfg.tag.c_str()), "",
                       static_cast<int>(ScenarioData::kSubCount), 0.0,
                       static_cast<double>(ScenarioData::kSubCount));
    for (size_t i = 0; i < ScenarioData::kSubCount; ++i) {
        const double val = d.hasMass ? EdepMeVToGy(d.subEdepMeV[i], d.targetMassKg) * scale
                                     : d.subEdepMeV[i] * scale;
        h->SetBinContent(static_cast<int>(i) + 1, val);
        h->GetXaxis()->SetBinLabel(static_cast<int>(i) + 1, labels[i]);
        if (val > 0.0) { minPos = std::min(minPos, val); maxVal = std::max(maxVal, val); }
    }
    if (maxVal > 0.0) {
        auto* c = new TCanvas(Form("c_sub_%s", d.cfg.tag.c_str()), pc.titlePrefix.c_str(), ac.canvasWidth, ac.canvasHeight);
        auto* p = new TPad("p", "", 0, 0, 1, 1);
        p->SetLeftMargin(ac.leftMargin); p->SetRightMargin(ac.rightMargin);
        p->SetTopMargin(ac.topMargin);   p->SetBottomMargin(0.20);
        p->SetLogy(pc.logY); p->SetTickx(1); p->SetTicky(1);
        p->Draw(); p->cd();

        const double yMin = pc.autoRange ? std::pow(10.0, std::floor(std::log10(minPos)) - 0.5) : pc.yMin;
        const double yMax = pc.autoRange ? std::pow(10.0, std::ceil(std::log10(maxVal)) + 0.8)  : pc.yMax;
        h->GetYaxis()->SetRangeUser(yMin, yMax);
        h->GetXaxis()->SetLabelSize(0.034);
        h->GetXaxis()->LabelsOption("u");
        h->GetYaxis()->SetTitle(d.hasMass ? pc.yTitle.c_str() : Form("Energy deposited [MeV per %s]", ac.normLabel.c_str()));
        h->GetYaxis()->SetTitleSize(0.042);
        h->GetYaxis()->SetTitleOffset(1.12);
        h->GetYaxis()->SetLabelSize(0.036);
        h->GetYaxis()->CenterTitle(true);
        h->SetFillColor(d.cfg.color); h->SetFillStyle(1001);
        h->SetLineColor(kBlack); h->SetLineWidth(2);
        h->SetBarWidth(0.75); h->SetBarOffset(0.125);
        h->Draw("BAR");

        TLatex valTxt;
        valTxt.SetTextFont(42); valTxt.SetTextSize(0.025); valTxt.SetTextAlign(21);
        for (int i = 1; i <= static_cast<int>(ScenarioData::kSubCount); ++i) {
            const double v = h->GetBinContent(i);
            if (v > 0.0) valTxt.DrawLatex(h->GetBinCenter(i), v * (pc.logY ? 1.25 : 1.03), Form("%.2e", v));
        }
        DrawHeaderBanner(p, Form("#bf{%s}  |  %s", pc.titlePrefix.c_str(), d.cfg.legendLabel.c_str()), ac.leftMargin, 0.942, pc.textSize);
        p->RedrawAxis();
        c->Update();
        std::cout << "  [Saved] " << SavePlot(c, ac, "Edep_BySubChannel_" + d.cfg.tag) << "\n";
        delete c;
    }
    outDir->cd();
    h->Write();
    delete h;
}

// Dynamic per-fragment breakdown: dose deposited by each hadron fragment,
// split into the fragment's own deposit ("direct") and its delta-rays ("delta").
// Grouped by element (Z); isotopes are merged for readability.
static void PlotSingleFragments(const ScenarioData& d, const AnalysisConfig& ac, TDirectory* outDir) {
    if (!ac.plotSubChannels.enabled || d.nEvents <= 0 || !d.hasFragments || d.fragEdep.empty()) return;
    const auto& pc = ac.plotSubChannels;
    const double scale = ac.normPrimaries / static_cast<double>(d.nEvents);

    auto symbol = [](int z) -> const char* {
        static const char* s[] = {"n","H","He","Li","Be","B","C","N","O","F","Ne","Na","Mg","Al","Si","P","S","Cl","Ar","K","Ca"};
        return (z >= 0 && z < 21) ? s[z] : "?";
    };

    std::map<int, std::pair<double, double>> byZ;  // Z -> {direct MeV, delta MeV}
    for (const auto& kv : d.fragEdep) {
        const int Z = static_cast<int>(kv.first / 1000L);
        byZ[Z].first  += kv.second.first;
        byZ[Z].second += kv.second.second;
    }
    std::vector<std::pair<int, std::pair<double, double>>> vec(byZ.begin(), byZ.end());
    std::sort(vec.begin(), vec.end(),
              [](const auto& a, const auto& b) {
                  return (a.second.first + a.second.second) > (b.second.first + b.second.second);
              });
    const int n = static_cast<int>(vec.size());
    if (n == 0) return;

    auto toDose = [&](double edepMeV) { return d.hasMass ? EdepMeVToGy(edepMeV, d.targetMassKg) * scale
                                                         : edepMeV * scale; };

    auto elemColor = [](int z) -> Int_t {
        switch (z) {
            case 1: return kBlue + 1;    case 2: return kRed + 1;
            case 3: return kGreen + 2;   case 4: return kOrange + 7;
            case 5: return kMagenta + 1; case 6: return kCyan + 2;
            case 7: return kViolet + 1;  case 8: return kTeal + 2;
            default: return kGray + 2;
        }
    };

    // Frame for axes/bins; the bars are drawn per element so each element has a
    // distinct colour (previously all elements shared the same two colours).
    auto* hF = new TH1D(Form("hFragF_%s", d.cfg.tag.c_str()), "", n, 0, n);
    double maxVal = 0.0;
    for (int i = 0; i < n; ++i) {
        maxVal = std::max(maxVal, toDose(vec[i].second.first + vec[i].second.second));
        hF->GetXaxis()->SetBinLabel(i + 1, Form("%s (Z=%d)", symbol(vec[i].first), vec[i].first));
    }
    if (maxVal <= 0.0) { delete hF; return; }

    auto* c = new TCanvas(Form("c_frag_%s", d.cfg.tag.c_str()), pc.titlePrefix.c_str(),
                          ac.canvasWidth, ac.canvasHeight);
    auto* p = new TPad("p", "", 0, 0, 1, 1);
    p->SetLeftMargin(ac.leftMargin); p->SetRightMargin(ac.rightMargin);
    p->SetTopMargin(ac.topMargin);   p->SetBottomMargin(0.22);
    p->SetLogy(pc.logY); p->SetTickx(1); p->SetTicky(1);
    p->Draw(); p->cd();

    hF->GetXaxis()->LabelsOption("u"); hF->GetXaxis()->SetLabelSize(0.030);
    hF->GetYaxis()->SetTitle(d.hasMass ? pc.yTitle.c_str()
                                       : Form("Energy deposited [MeV per %s]", ac.normLabel.c_str()));
    hF->GetYaxis()->SetTitleSize(0.042); hF->GetYaxis()->SetTitleOffset(1.12);
    hF->SetMinimum(pc.logY ? pc.yMin : 0.0);
    hF->SetMaximum(pc.logY ? maxVal * 40.0 : maxVal * 1.2);
    hF->SetStats(0);
    hF->Draw("AXIS");

    // Stacked solid segments: direct deposit at the bottom (element colour), the
    // delta-ray part as a cap on top (uniform contrasting grey). Solid fills are
    // used on purpose: ROOT's SVG backend does not preserve hatched fills, which
    // previously made the delta-ray contribution invisible.
    const Int_t deltaCol = kGray + 1;
    const int nCols = (n > 7) ? 2 : 1;
    auto* leg = new TLegend(nCols == 2 ? 0.52 : 0.56, 0.30, 0.93, 0.90);
    leg->SetBorderSize(0); leg->SetTextSize(0.026);
    leg->SetEntrySeparation(0.15);
    if (nCols > 1) leg->SetNColumns(nCols);
    for (int i = 0; i < n; ++i) {
        const int Z = vec[i].first;
        const double dv = toDose(vec[i].second.first);
        const double lv = toDose(vec[i].second.second);
        const Int_t col = elemColor(Z);
        // cumulative (direct + delta) first: full bar drawn in the delta shade ...
        if (lv > 0.0) {
            auto* hb = new TH1D(Form("hFragD_%d_%s", i, d.cfg.tag.c_str()), "", 1, i, i + 1);
            hb->SetBinContent(1, dv + lv);
            hb->SetFillColor(deltaCol); hb->SetFillStyle(1001); hb->SetLineColor(kBlack); hb->SetLineWidth(1);
            hb->SetBarWidth(1.0); hb->SetBarOffset(0.0); hb->Draw("BAR SAME");
        }
        // ... then the direct part solid on top, so the visible cap = delta-rays.
        if (dv > 0.0) {
            auto* hb = new TH1D(Form("hFragB_%d_%s", i, d.cfg.tag.c_str()), "", 1, i, i + 1);
            hb->SetBinContent(1, dv);
            hb->SetFillColor(col); hb->SetFillStyle(1001); hb->SetLineColor(kBlack); hb->SetLineWidth(1);
            hb->SetBarWidth(1.0); hb->SetBarOffset(0.0); hb->Draw("BAR SAME");
        }
        auto* hleg = new TH1D(Form("hFragL_%d_%s", i, d.cfg.tag.c_str()), "", 1, 0, 1);
        hleg->SetFillColor(col); hleg->SetFillStyle(1001); hleg->SetLineColor(kBlack);
        leg->AddEntry(hleg, Form("%s (Z=%d)", symbol(Z), Z), "f");
    }
    // Explicit delta-ray key so the top cap is unambiguous in the legend.
    auto* hDelta = new TH1D(Form("hFragDelta_%s", d.cfg.tag.c_str()), "", 1, 0, 1);
    hDelta->SetFillColor(deltaCol); hDelta->SetFillStyle(1001); hDelta->SetLineColor(kBlack);
    leg->AddEntry(hDelta, "#delta-rays (top cap)", "f");
    leg->Draw();
    DrawHeaderBanner(p, Form("#bf{Fragment energy deposition by element}  |  %s",
                             d.cfg.legendLabel.c_str()), ac.leftMargin, 0.942, pc.textSize);
    p->RedrawAxis(); c->Update();
    std::cout << "  [Saved] " << SavePlot(c, ac, "Edep_ByFragment_" + d.cfg.tag) << "\n";
    delete c;
    outDir->cd(); hF->Write(); delete hF;
}

// Hadronic interaction vertices along the beam axis z: where in the tract (which
// filter layer) hadronic processes are born -> "which filter carries the threat".
static void PlotSingleHadronVertexZ(const ScenarioData& d, const AnalysisConfig& ac, TDirectory* outDir) {
    if (d.nEvents <= 0 || !d.hHadronVertexZ || d.hHadronVertexZ->GetEntries() <= 0.0) return;
    const auto& pc = ac.plotSpectra;
    auto* h = static_cast<TH1D*>(d.hHadronVertexZ->Clone(Form("hHvz_%s", d.cfg.tag.c_str())));
    h->SetDirectory(nullptr);
    h->SetTitle("");  // drop the ROOT auto-title so it cannot collide with the header banner
    if (pc.rebin > 1) h->Rebin(pc.rebin);
    h->Scale(ac.normPrimaries / static_cast<double>(d.nEvents));

    auto* c = new TCanvas(Form("c_hvz_%s", d.cfg.tag.c_str()), "Hadronic vertices vs z",
                          ac.canvasWidth, ac.canvasHeight);
    auto* p = new TPad("p", "", 0, 0, 1, 1);
    p->SetLeftMargin(ac.leftMargin); p->SetRightMargin(ac.rightMargin);
    p->SetTopMargin(ac.topMargin);   p->SetBottomMargin(ac.bottomMargin);
    p->SetLogy(pc.logY); p->SetTickx(1); p->SetTicky(1);
    p->Draw(); p->cd();
    h->SetLineColor(d.cfg.color); h->SetLineWidth(2);
    h->SetFillColor(d.cfg.color); h->SetFillStyle(1001);
    h->GetXaxis()->SetTitle("z [mm]");
    h->GetYaxis()->SetTitle(Form("Hadronic vertices per %s", ac.normLabel.c_str()));
    h->GetYaxis()->SetTitleSize(0.042); h->GetYaxis()->SetTitleOffset(1.15);
    h->Draw("HIST");
    DrawHeaderBanner(p, Form("#bf{Hadronic vertices vs z}  |  %s", d.cfg.legendLabel.c_str()),
                     ac.leftMargin, 0.942, pc.textSize);
    p->RedrawAxis(); c->Update();
    std::cout << "  [Saved] " << SavePlot(c, ac, "HadronVertexZ_" + d.cfg.tag) << "\n";
    delete c;
    outDir->cd(); h->Write(); delete h;
}

static void PlotSingleEdepEventDist(const ScenarioData& d, const AnalysisConfig& ac, TDirectory* outDir) {
    const auto& vTot = d.hitEdepByFam[static_cast<size_t>(FamilyIdx::Total)];
    const auto& vEM  = d.hitEdepByFam[static_cast<size_t>(FamilyIdx::PrimaryEM)];
    const auto& vHad = d.hitEdepByFam[static_cast<size_t>(FamilyIdx::HadTotal)];
    if (!ac.plotEdepEventDist.enabled || vTot.empty() || d.nEvents <= 0) return;

    const auto& pc = ac.plotEdepEventDist;
    double eMin = std::numeric_limits<double>::max(), eMax = 0.0;
    for (double v : vTot) { eMin = std::min(eMin, v); eMax = std::max(eMax, v); }
    for (double v : vEM)  { eMin = std::min(eMin, v); eMax = std::max(eMax, v); }
    for (double v : vHad) { eMin = std::min(eMin, v); eMax = std::max(eMax, v); }
    if (eMax <= eMin) return;

    const double lo = pc.autoRange ? (eMin * 0.8) : pc.xMin;
    const double hi = pc.autoRange ? (eMax * 1.2) : pc.xMax;
    const double w  = 1.0 / static_cast<double>(d.nEvents);

    auto* hTot = BuildLogHistFromDoubles(vTot, lo, hi, 120, w, Form("hEdepEvTot_%s", d.cfg.tag.c_str()));
    auto* hEM  = BuildLogHistFromDoubles(vEM,  lo, hi, 120, w, Form("hEdepEvEM_%s",  d.cfg.tag.c_str()));
    auto* hHad = BuildLogHistFromDoubles(vHad, lo, hi, 120, w, Form("hEdepEvHad_%s", d.cfg.tag.c_str()));

    double xMinD = lo, xMaxD = hi, yMinPos = std::numeric_limits<double>::max(), yMaxD = 0.0;
    UpdateBoundsFromHist(hTot, true, xMinD, xMaxD, yMinPos, yMaxD);
    UpdateBoundsFromHist(hEM,  true, xMinD, xMaxD, yMinPos, yMaxD);
    UpdateBoundsFromHist(hHad, true, xMinD, xMaxD, yMinPos, yMaxD);
    if (yMaxD <= 0.0) { delete hTot; delete hEM; delete hHad; return; }

    const double yMin = pc.autoRange ? std::pow(10.0, std::floor(std::log10(yMinPos)) - 0.3) : pc.yMin;
    const double yMax = pc.autoRange ? std::pow(10.0, std::ceil(std::log10(yMaxD)) + 0.6)    : pc.yMax;

    auto* c = new TCanvas(Form("c_edep_dist_%s", d.cfg.tag.c_str()), pc.titlePrefix.c_str(), ac.canvasWidth, ac.canvasHeight);
    auto* p = new TPad("p", "", 0, 0, 1, 1);
    p->SetLeftMargin(ac.leftMargin); p->SetRightMargin(ac.rightMargin);
    p->SetTopMargin(ac.topMargin);   p->SetBottomMargin(ac.bottomMargin);
    p->SetLogx(pc.logX); p->SetLogy(pc.logY); p->SetTickx(1); p->SetTicky(1);
    p->Draw(); p->cd();

    auto* hF = new TH1D(Form("hF_edep_%s", d.cfg.tag.c_str()), "", 100, lo, hi);
    hF->Draw("AXIS");
    hF->GetXaxis()->SetTitle(pc.xTitle.c_str());
    hF->GetXaxis()->SetTitleSize(0.042); hF->GetXaxis()->SetTitleOffset(1.18);
    hF->GetXaxis()->SetLabelSize(0.036); hF->GetXaxis()->CenterTitle(true);
    hF->GetYaxis()->SetRangeUser(yMin, yMax);
    hF->GetYaxis()->SetTitle(pc.yTitle.c_str());
    hF->GetYaxis()->SetTitleSize(0.042); hF->GetYaxis()->SetTitleOffset(1.08);
    hF->GetYaxis()->SetLabelSize(0.036); hF->GetYaxis()->CenterTitle(true);

    std::vector<TObject*> objs; std::vector<std::string> labs, opts;
    auto* gTot = HistToCleanGraph(hTot, yMin * 0.1, true);
    gTot->SetLineColor(kBlack); gTot->SetLineWidth(pc.lineWidth); gTot->Draw("L SAME");
    objs.push_back(gTot); labs.push_back("Total E_{dep}"); opts.push_back("l");

    if (hHad->GetEntries() > 0) {
        auto* gHad = HistToCleanGraph(hHad, yMin * 0.1, true);
        gHad->SetLineColor(kRed + 1); gHad->SetLineWidth(pc.lineWidth); gHad->Draw("L SAME");
        objs.push_back(gHad); labs.push_back("Hadronic E_{dep}"); opts.push_back("l");
    }
    if (hEM->GetEntries() > 0) {
        auto* gEM = HistToCleanGraph(hEM, yMin * 0.1, true);
        gEM->SetLineColor(kBlue + 1); gEM->SetLineStyle(2); gEM->SetLineWidth(pc.lineWidth); gEM->Draw("L SAME");
        objs.push_back(gEM); labs.push_back("Primary EM E_{dep}"); opts.push_back("l");
    }

    DrawDualLegend(p, objs, labs, opts, pc.legX1, pc.legY1, pc.legX2, pc.legY2, pc.textSize);
    DrawHeaderBanner(p, Form("#bf{%s}  |  %s", pc.titlePrefix.c_str(), d.cfg.legendLabel.c_str()), ac.leftMargin, 0.942, pc.textSize);
    p->RedrawAxis();
    c->Update();
    std::cout << "  [Saved] " << SavePlot(c, ac, "Edep_Distribution_" + d.cfg.tag) << "\n";
    delete c;

    outDir->cd();
    hTot->Write(); hEM->Write(); hHad->Write();
    delete hTot; delete hEM; delete hHad; delete hF;
}

static void PlotSingleEdepBySpecies(const ScenarioData& d, const AnalysisConfig& ac, TDirectory* outDir) {
    if (!ac.plotEdepBySpecies.enabled || d.nEvents <= 0) return;
    const auto& pc = ac.plotEdepBySpecies;

    struct SpCurve { FamilyIdx idx; const char* label; Int_t col; Int_t st; };
    const std::vector<SpCurve> curves = {
        {FamilyIdx::PrimaryEM, "Primary EM (^{20}Ne)", kGray + 2,    2},
        {FamilyIdx::Proton,    "Protons (p)",          kBlue + 1,    1},
        {FamilyIdx::Alpha,     "Alphas (#alpha)",      kRed + 1,     1},
        {FamilyIdx::Deuteron,  "Deuterons (d)",        kGreen + 2,   1},
        {FamilyIdx::Triton,    "Tritons (t)",          kMagenta + 1, 1},
        {FamilyIdx::Helium3,   "Helium-3 (^{3}He)",    kOrange + 7,  1},
        {FamilyIdx::HeavyIon,  "Heavy (Z #geq 3)",     kCyan + 2,    1},
        {FamilyIdx::Neutron,   "Neutrons (n)",         kViolet + 1,  2}
    };

    double eMin = std::numeric_limits<double>::max(), eMax = 0.0;
    for (const auto& sc : curves) {
        for (double v : d.hitEdepByFam[static_cast<size_t>(sc.idx)]) {
            eMin = std::min(eMin, v);
            eMax = std::max(eMax, v);
        }
    }
    if (eMax <= eMin) return;

    const double lo = pc.autoRange ? (eMin * 0.8) : pc.xMin;
    const double hi = pc.autoRange ? (eMax * 1.2) : pc.xMax;
    const double w  = 1.0 / static_cast<double>(d.nEvents);

    std::vector<std::pair<const SpCurve*, TH1D*>> hists;
    double xMinD = lo, xMaxD = hi, yMinPos = std::numeric_limits<double>::max(), yMaxD = 0.0;
    for (const auto& sc : curves) {
        const auto& vec = d.hitEdepByFam[static_cast<size_t>(sc.idx)];
        if (vec.empty()) continue;
        auto* h = BuildLogHistFromDoubles(vec, lo, hi, 110, w,
                                          Form("hEdepSp_%d_%s", static_cast<int>(sc.idx), d.cfg.tag.c_str()));
        UpdateBoundsFromHist(h, true, xMinD, xMaxD, yMinPos, yMaxD);
        hists.emplace_back(&sc, h);
    }
    if (hists.empty() || yMaxD <= 0.0) return;

    const double yMin = pc.autoRange ? std::pow(10.0, std::floor(std::log10(yMinPos)) - 0.3) : pc.yMin;
    const double yMax = pc.autoRange ? std::pow(10.0, std::ceil(std::log10(yMaxD)) + 0.6)    : pc.yMax;

    auto* c = new TCanvas(Form("c_edep_sp_%s", d.cfg.tag.c_str()), pc.titlePrefix.c_str(), ac.canvasWidth, ac.canvasHeight);
    auto* p = new TPad("p", "", 0, 0, 1, 1);
    p->SetLeftMargin(ac.leftMargin); p->SetRightMargin(ac.rightMargin);
    p->SetTopMargin(ac.topMargin);   p->SetBottomMargin(ac.bottomMargin);
    p->SetLogx(pc.logX); p->SetLogy(pc.logY); p->SetTickx(1); p->SetTicky(1);
    p->Draw(); p->cd();

    auto* hF = new TH1D(Form("hF_edep_sp_%s", d.cfg.tag.c_str()), "", 100, lo, hi);
    hF->Draw("AXIS");
    hF->GetXaxis()->SetTitle(pc.xTitle.c_str());
    hF->GetXaxis()->SetTitleSize(0.042); hF->GetXaxis()->SetTitleOffset(1.18);
    hF->GetXaxis()->SetLabelSize(0.036); hF->GetXaxis()->CenterTitle(true);
    hF->GetYaxis()->SetRangeUser(yMin, yMax);
    hF->GetYaxis()->SetTitle(pc.yTitle.c_str());
    hF->GetYaxis()->SetTitleSize(0.042); hF->GetYaxis()->SetTitleOffset(1.08);
    hF->GetYaxis()->SetLabelSize(0.036); hF->GetYaxis()->CenterTitle(true);

    std::vector<TObject*> objs; std::vector<std::string> labs, opts;
    for (const auto& pair : hists) {
        auto* g = HistToCleanGraph(pair.second, yMin * 0.1, true);
        g->SetLineColor(pair.first->col);
        g->SetLineStyle(pair.first->st);
        g->SetLineWidth(pc.lineWidth);
        g->Draw("L SAME");
        objs.push_back(g); labs.push_back(pair.first->label); opts.push_back("l");
    }

    DrawDualLegend(p, objs, labs, opts, pc.legX1, pc.legY1, pc.legX2, pc.legY2, pc.textSize);
    DrawHeaderBanner(p, Form("#bf{%s}  |  %s", pc.titlePrefix.c_str(), d.cfg.legendLabel.c_str()), ac.leftMargin, 0.942, pc.textSize);
    p->RedrawAxis();
    c->Update();
    std::cout << "  [Saved] " << SavePlot(c, ac, "Edep_BySpecies_" + d.cfg.tag) << "\n";
    delete c;

    outDir->cd();
    for (auto& pair : hists) { pair.second->Write(); delete pair.second; }
    delete hF;
}

// Direct vs delta-ray split of the per-event energy-deposit spectrum, resolved
// by species/element: solid = the particle's own deposit, dashed = its delta-rays.
// This shows which component produces each peak (ionisation vs radiative/EM tail)
// and replaces the coarse "Heavy (Z>=3)" lump with per-element curves.
static void PlotSingleEdepDirectDelta(const ScenarioData& d, const AnalysisConfig& ac, TDirectory* outDir) {
    if (!ac.plotEdepBySpecies.enabled || d.nEvents <= 0) return;
    const auto& pc = ac.plotEdepBySpecies;

    auto symbol = [](int z) -> const char* {
        static const char* s[] = {"n","H","He","Li","Be","B","C","N","O","F","Ne"};
        return (z >= 0 && z <= 10) ? s[z] : "?";
    };
    auto elemColor = [](int z) -> Int_t {
        switch (z) {
            case 1: return kBlue + 1;   case 2: return kRed + 1;
            case 3: return kGreen + 2;  case 4: return kOrange + 7;
            case 5: return kMagenta + 1;case 6: return kCyan + 2;
            case 7: return kViolet + 1; case 8: return kTeal + 2;
            default: return kGray + 2;
        }
    };

    // Per component: TOTAL deposit (solid) and its SECONDARY/delta-ray part
    // (dashed); the primary-particle part is the difference of the two.
    struct Comp { std::string label; Int_t col; const std::vector<double>* total; const std::vector<double>* delta; };
    std::vector<Comp> comps;
    comps.push_back({"^{20}Ne (primary)", kBlack, &d.evPrimaryTotal, &d.evSubEdep[1]});
    for (int Z = 1; Z <= 8; ++Z) {
        const std::vector<double>* ctot = nullptr; const std::vector<double>* cdel = nullptr;
        auto itt = d.evFragZTotal.find(Z); if (itt != d.evFragZTotal.end()) ctot = &itt->second;
        auto itl = d.evFragZDelta.find(Z); if (itl != d.evFragZDelta.end()) cdel = &itl->second;
        if (!ctot && !cdel) continue;
        comps.push_back({std::string(symbol(Z)) + " (total/#delta)", elemColor(Z), ctot, cdel});
    }
    if (comps.size() < 2) return;

    double eMin = std::numeric_limits<double>::max(), eMax = 0.0;
    for (const auto& c : comps)
        for (const std::vector<double>* v : {c.total, c.delta}) if (v) for (double x : *v) { eMin = std::min(eMin, x); eMax = std::max(eMax, x); }
    if (eMax <= eMin) return;
    const double lo = eMin * 0.8, hi = eMax * 1.2;
    const double w  = 1.0 / static_cast<double>(d.nEvents);

    struct Curve { const Comp* c; TH1D* h; bool delta; };
    std::vector<Curve> curves;
    double xMinD = lo, xMaxD = hi, yMinPos = std::numeric_limits<double>::max(), yMaxD = 0.0;
    for (const auto& c : comps) {
        if (c.total && !c.total->empty()) {
            auto* h = BuildLogHistFromDoubles(*c.total, lo, hi, 110, w, Form("hDDt_%s_%zu", d.cfg.tag.c_str(), curves.size()));
            UpdateBoundsFromHist(h, true, xMinD, xMaxD, yMinPos, yMaxD);
            curves.push_back({&c, h, false});
        }
        if (c.delta && !c.delta->empty()) {
            auto* h = BuildLogHistFromDoubles(*c.delta, lo, hi, 110, w, Form("hDDl_%s_%zu", d.cfg.tag.c_str(), curves.size()));
            UpdateBoundsFromHist(h, true, xMinD, xMaxD, yMinPos, yMaxD);
            curves.push_back({&c, h, true});
        }
    }
    if (curves.empty() || yMaxD <= 0.0) { for (auto& cu : curves) delete cu.h; return; }

    const double yMin = std::pow(10.0, std::floor(std::log10(yMinPos)) - 0.3);
    const double yMax = std::pow(10.0, std::ceil(std::log10(yMaxD)) + 0.6);

    auto* c = new TCanvas(Form("c_edep_dd_%s", d.cfg.tag.c_str()), pc.titlePrefix.c_str(), ac.canvasWidth, ac.canvasHeight);
    auto* p = new TPad("p", "", 0, 0, 1, 1);
    p->SetLeftMargin(ac.leftMargin); p->SetRightMargin(ac.rightMargin);
    p->SetTopMargin(ac.topMargin);   p->SetBottomMargin(ac.bottomMargin);
    p->SetLogx(pc.logX); p->SetLogy(pc.logY); p->SetTickx(1); p->SetTicky(1);
    p->Draw(); p->cd();

    auto* hF = new TH1D(Form("hF_edep_dd_%s", d.cfg.tag.c_str()), "", 100, xMinD, xMaxD);
    hF->Draw("AXIS");
    hF->GetXaxis()->SetTitle(pc.xTitle.c_str());
    hF->GetXaxis()->SetTitleSize(0.042); hF->GetXaxis()->SetTitleOffset(1.18);
    hF->GetXaxis()->SetLabelSize(0.036); hF->GetXaxis()->CenterTitle(true);
    hF->GetYaxis()->SetRangeUser(yMin, yMax);
    hF->GetYaxis()->SetTitle(pc.yTitle.c_str());
    hF->GetYaxis()->SetTitleSize(0.042); hF->GetYaxis()->SetTitleOffset(1.08);
    hF->GetYaxis()->SetLabelSize(0.036); hF->GetYaxis()->CenterTitle(true);

    std::vector<TObject*> objs; std::vector<std::string> labs, opts;
    std::set<std::string> seen;
    for (const auto& cu : curves) {
        auto* g = HistToCleanGraph(cu.h, yMin * 0.1, true);
        g->SetLineColor(cu.c->col);
        g->SetLineStyle(cu.delta ? 2 : 1);
        g->SetLineWidth(pc.lineWidth);
        g->Draw("L SAME");
        if (seen.insert(cu.c->label).second) { objs.push_back(g); labs.push_back(cu.c->label); opts.push_back("l"); }
    }
    DrawDualLegend(p, objs, labs, opts, pc.legX1, pc.legY1, pc.legX2, pc.legY2, pc.textSize);
    DrawHeaderBanner(p, Form("#bf{Total (solid) vs #delta-rays (dashed)}  |  %s",
                             d.cfg.legendLabel.c_str()), ac.leftMargin, 0.942, pc.textSize);
    p->RedrawAxis(); c->Update();
    std::cout << "  [Saved] " << SavePlot(c, ac, "Edep_BySpecies_DirectDelta_" + d.cfg.tag) << "\n";
    delete c;
    outDir->cd(); for (auto& cu : curves) { cu.h->Write(); delete cu.h; } delete hF;
}

static void PlotSingleEdepBySpeciesLinear(const ScenarioData& d, const AnalysisConfig& ac, TDirectory* outDir) {
    if (!ac.plotEdepBySpeciesLinear.enabled || d.nEvents <= 0) return;
    const auto& pc = ac.plotEdepBySpeciesLinear;

    struct SpCurve { FamilyIdx idx; const char* label; Int_t col; Int_t st; };
    const std::vector<SpCurve> curves = {
        {FamilyIdx::PrimaryEM, "Primary EM (^{20}Ne)", kGray + 2,    2},
        {FamilyIdx::Proton,    "Protons (p)",          kBlue + 1,    1},
        {FamilyIdx::Alpha,     "Alphas (#alpha)",      kRed + 1,     1},
        {FamilyIdx::Deuteron,  "Deuterons (d)",        kGreen + 2,   1},
        {FamilyIdx::Triton,    "Tritons (t)",          kMagenta + 1, 1},
        {FamilyIdx::Helium3,   "Helium-3 (^{3}He)",    kOrange + 7,  1},
        {FamilyIdx::HeavyIon,  "Heavy (Z #geq 3)",     kCyan + 2,    1},
        {FamilyIdx::Neutron,   "Neutrons (n)",         kViolet + 1,  2}
    };

    // Data-driven linear x-window when autoRange is set; a fixed 0..5 MeV window
    // would leave the low-Edep cell scenarios (E_dep ~ 1e-4 MeV) invisible at 0.
    double dataMin = std::numeric_limits<double>::max(), dataMax = 0.0;
    if (pc.autoRange) {
        for (const auto& sc : curves) {
            for (const double v : d.hitEdepByFam[static_cast<size_t>(sc.idx)]) {
                if (v > 0.0) { dataMin = std::min(dataMin, v); dataMax = std::max(dataMax, v); }
            }
        }
    }
    double lo = pc.xMin, hi = pc.xMax;
    if (pc.autoRange && dataMax > 0.0) {
        lo = std::max(0.0, dataMin * 0.8);
        hi = dataMax * 1.1;
        if (hi <= lo) hi = lo + 1.0;
    }
    if (hi <= lo) return;
    const double w = 1.0 / static_cast<double>(d.nEvents);

    std::vector<std::pair<const SpCurve*, TH1D*>> hists;
    double yMaxD = 0.0;
    for (const auto& sc : curves) {
        const auto& vec = d.hitEdepByFam[static_cast<size_t>(sc.idx)];
        if (vec.empty()) continue;
        auto* h = BuildLinearHistFromDoubles(vec, lo, hi, 250, w,
                                             Form("hEdepSpLin_%d_%s", static_cast<int>(sc.idx), d.cfg.tag.c_str()));
        yMaxD = std::max(yMaxD, h->GetMaximum());
        hists.emplace_back(&sc, h);
    }
    if (hists.empty() || yMaxD <= 0.0) return;

    const double yMax = yMaxD * 1.15;

    auto* c = new TCanvas(Form("c_edep_splin_%s", d.cfg.tag.c_str()), pc.titlePrefix.c_str(), ac.canvasWidth, ac.canvasHeight);
    auto* p = new TPad("p", "", 0, 0, 1, 1);
    p->SetLeftMargin(ac.leftMargin); p->SetRightMargin(ac.rightMargin);
    p->SetTopMargin(ac.topMargin);   p->SetBottomMargin(ac.bottomMargin);
    p->SetTickx(1); p->SetTicky(1);
    p->Draw(); p->cd();

    auto* hF = new TH1D(Form("hF_edep_splin_%s", d.cfg.tag.c_str()), "", 100, lo, hi);
    hF->SetMinimum(0.0);
    hF->SetMaximum(yMax);
    hF->Draw("AXIS");
    hF->GetXaxis()->SetTitle(pc.xTitle.c_str());
    hF->GetXaxis()->SetTitleSize(0.042); hF->GetXaxis()->SetTitleOffset(1.18);
    hF->GetXaxis()->SetLabelSize(0.036); hF->GetXaxis()->CenterTitle(true);
    hF->GetYaxis()->SetTitle(pc.yTitle.c_str());
    hF->GetYaxis()->SetTitleSize(0.042); hF->GetYaxis()->SetTitleOffset(1.08);
    hF->GetYaxis()->SetLabelSize(0.036); hF->GetYaxis()->CenterTitle(true);

    std::vector<TObject*> objs; std::vector<std::string> labs, opts;
    for (const auto& pair : hists) {
        pair.second->SetLineColor(pair.first->col);
        pair.second->SetLineStyle(pair.first->st);
        pair.second->SetLineWidth(pc.lineWidth);
        pair.second->Draw("HIST SAME");
        objs.push_back(pair.second); labs.push_back(pair.first->label); opts.push_back("l");
    }

    DrawDualLegend(p, objs, labs, opts, pc.legX1, pc.legY1, pc.legX2, pc.legY2, pc.textSize);
    DrawHeaderBanner(p, Form("#bf{%s}  |  %s", pc.titlePrefix.c_str(), d.cfg.legendLabel.c_str()), ac.leftMargin, 0.942, pc.textSize);
    p->RedrawAxis();
    c->Update();
    std::cout << "  [Saved] " << SavePlot(c, ac, "Edep_BySpecies_Linear_" + d.cfg.tag) << "\n";
    delete c;

    outDir->cd();
    for (auto& pair : hists) { pair.second->Write(); delete pair.second; }
    delete hF;
}

static void PlotSingleFragmentCounts(const ScenarioData& d, const AnalysisConfig& ac, TDirectory* outDir) {
    if (!ac.plotFragCounts.enabled || d.nEvents <= 0) return;
    const auto& pc = ac.plotFragCounts;

    struct FragMap { const char* specKey; FamilyIdx famIdx; const char* shortName; };
    const std::vector<FragMap> list = {
        {"SpecIn_Proton",   FamilyIdx::Proton,   "p"},
        {"SpecIn_Alpha",    FamilyIdx::Alpha,    "#alpha"},
        {"SpecIn_Deuteron", FamilyIdx::Deuteron, "d"},
        {"SpecIn_Triton",   FamilyIdx::Triton,   "t"},
        {"SpecIn_He3",      FamilyIdx::Helium3,  "^{3}He"},
        {"SpecIn_Heavy",    FamilyIdx::HeavyIon, "Heavy (Z#geq3)"},
        {"SpecIn_Neutron",  FamilyIdx::Neutron,  "n"}
    };

    const int nB = static_cast<int>(list.size());
    const double scale = ac.normPrimaries / static_cast<double>(d.nEvents);

    auto* hEntrances = new TH1D(Form("hFragEntrances_%s", d.cfg.tag.c_str()), "", nB, 0.0, static_cast<double>(nB));
    auto* hHitEvents = new TH1D(Form("hFragHitEv_%s",     d.cfg.tag.c_str()), "", nB, 0.0, static_cast<double>(nB));

    double minPos = std::numeric_limits<double>::max(), maxVal = 0.0;
    double sumEntrances = 0.0;
    bool hasEntrances = false;

    for (int i = 0; i < nB; ++i) {
        double ent = 0.0;
        for (const auto& sp : d.spectra) {
            if (sp.key == list[i].specKey && sp.hist) {
                ent = sp.entries * scale;
                hasEntrances = true;
                break;
            }
        }
        sumEntrances += ent;
        const double hitEv = static_cast<double>(d.fam[static_cast<size_t>(list[i].famIdx)].nHitEvents) * scale;

        hEntrances->SetBinContent(i + 1, ent);
        hHitEvents->SetBinContent(i + 1, hitEv);
        hEntrances->GetXaxis()->SetBinLabel(i + 1, list[i].shortName);
        hHitEvents->GetXaxis()->SetBinLabel(i + 1, list[i].shortName);

        if (ent > 0.0)   { minPos = std::min(minPos, ent);   maxVal = std::max(maxVal, ent); }
        if (hitEv > 0.0) { minPos = std::min(minPos, hitEv); maxVal = std::max(maxVal, hitEv); }
    }

    if (maxVal <= 0.0) { delete hEntrances; delete hHitEvents; return; }

    auto* c = new TCanvas(Form("c_frag_%s", d.cfg.tag.c_str()), pc.titlePrefix.c_str(), ac.canvasWidth, ac.canvasHeight);
    auto* p = new TPad("p", "", 0, 0, 1, 1);
    p->SetLeftMargin(ac.leftMargin); p->SetRightMargin(ac.rightMargin);
    p->SetTopMargin(ac.topMargin);   p->SetBottomMargin(0.15);
    p->SetLogy(pc.logY); p->SetTickx(1); p->SetTicky(1);
    p->Draw(); p->cd();

    const double yMin = pc.autoRange ? std::pow(10.0, std::floor(std::log10(minPos)) - 0.5) : pc.yMin;
    const double yMax = pc.autoRange ? std::pow(10.0, std::ceil(std::log10(maxVal)) + 1.0)  : pc.yMax;

    std::vector<TObject*> objs; std::vector<std::string> labs, opts;
    if (hasEntrances) {
        hEntrances->GetYaxis()->SetRangeUser(yMin, yMax);
        hEntrances->GetXaxis()->SetLabelSize(0.042);
        hEntrances->GetYaxis()->SetTitle(pc.yTitle.c_str());
        hEntrances->GetYaxis()->SetTitleSize(0.042); hEntrances->GetYaxis()->SetTitleOffset(1.08);
        hEntrances->GetYaxis()->SetLabelSize(0.036); hEntrances->GetYaxis()->CenterTitle(true);
        hEntrances->SetFillColor(kAzure + 1); hEntrances->SetLineColor(kAzure + 2);
        hEntrances->SetBarWidth(0.36); hEntrances->SetBarOffset(0.11);
        hEntrances->Draw("BAR");

        hHitEvents->SetFillColor(kOrange + 1); hHitEvents->SetLineColor(kOrange + 2);
        hHitEvents->SetBarWidth(0.36); hHitEvents->SetBarOffset(0.53);
        hHitEvents->Draw("BAR SAME");

        objs.push_back(hEntrances); labs.push_back("Target boundary entrances"); opts.push_back("f");
        objs.push_back(hHitEvents); labs.push_back("Ionizing events (Ions > 0)");  opts.push_back("f");
    } else {
        hHitEvents->GetYaxis()->SetRangeUser(yMin, yMax);
        hHitEvents->GetXaxis()->SetLabelSize(0.042);
        hHitEvents->GetYaxis()->SetTitle(pc.yTitle.c_str());
        hHitEvents->GetYaxis()->SetTitleSize(0.042); hHitEvents->GetYaxis()->SetTitleOffset(1.08);
        hHitEvents->GetYaxis()->SetLabelSize(0.036); hHitEvents->GetYaxis()->CenterTitle(true);
        hHitEvents->SetFillColor(kOrange + 1); hHitEvents->SetLineColor(kOrange + 2);
        hHitEvents->SetBarWidth(0.70); hHitEvents->SetBarOffset(0.15);
        hHitEvents->Draw("BAR");
        objs.push_back(hHitEvents); labs.push_back("Ionizing events (Ions > 0)"); opts.push_back("f");
    }

    TLatex lbl;
    lbl.SetTextFont(42); lbl.SetTextSize(0.019); lbl.SetTextAlign(21);
    for (int i = 1; i <= nB; ++i) {
        if (hasEntrances) {
            const double v1 = hEntrances->GetBinContent(i);
            const double pct = (sumEntrances > 0.0) ? (100.0 * v1 / sumEntrances) : 0.0;
            if (v1 > 0.0) lbl.DrawLatex(i - 0.71, v1 * 1.25, Form("#splitline{%.0f}{(%.1f%%)}", v1, pct));
            const double v2 = hHitEvents->GetBinContent(i);
            if (v2 > 0.0) lbl.DrawLatex(i - 0.29, v2 * 1.25, Form("%.0f", v2));
        } else {
            const double v2 = hHitEvents->GetBinContent(i);
            if (v2 > 0.0) lbl.DrawLatex(i - 0.50, v2 * 1.25, Form("%.0f", v2));
        }
    }

    DrawDualLegend(p, objs, labs, opts, pc.legX1, pc.legY1, pc.legX2, pc.legY2, pc.textSize);
    DrawHeaderBanner(p, Form("#bf{%s}  |  %s", pc.titlePrefix.c_str(), d.cfg.legendLabel.c_str()), ac.leftMargin, 0.942, pc.textSize);
    p->RedrawAxis();
    c->Update();
    std::cout << "  [Saved] " << SavePlot(c, ac, "FragmentCounts_" + d.cfg.tag) << "\n";
    delete c;

    outDir->cd();
    if (hasEntrances) hEntrances->Write();
    hHitEvents->Write();
    delete hEntrances; delete hHitEvents;
}

static void PlotSingleICSD(const ScenarioData& d, const AnalysisConfig& ac, TDirectory* outDir) {
    if (ac.plotICSD.enabled && d.hIcsdTargetTot && d.hIcsdTargetTot->Integral() > 0.0) {
        const auto& pc = ac.plotICSD;

        auto* hTot = static_cast<TH1D*>(d.hIcsdTargetTot->Clone(Form("hIcsdTot_%s", d.cfg.tag.c_str())));
        auto* hEM  = d.hIcsdTargetEM  ? static_cast<TH1D*>(d.hIcsdTargetEM->Clone(Form("hIcsdEM_%s", d.cfg.tag.c_str())))   : nullptr;
        auto* hHad = d.hIcsdTargetHad ? static_cast<TH1D*>(d.hIcsdTargetHad->Clone(Form("hIcsdHad_%s", d.cfg.tag.c_str()))) : nullptr;

        if (pc.rebin > 1) {
            hTot->Rebin(pc.rebin);
            if (hEM)  hEM->Rebin(pc.rebin);
            if (hHad) hHad->Rebin(pc.rebin);
        }

        const double norm = hTot->Integral();
        hTot->Scale(1.0 / norm);
        if (hEM)  hEM->Scale(1.0 / norm);
        if (hHad) hHad->Scale(1.0 / norm);

        double xMinD = std::numeric_limits<double>::max(), xMaxD = 0.0;
        double yMinPos = std::numeric_limits<double>::max(), yMaxD = 0.0;
        UpdateBoundsFromHist(hTot, false, xMinD, xMaxD, yMinPos, yMaxD);
        UpdateBoundsFromHist(hEM,  false, xMinD, xMaxD, yMinPos, yMaxD);
        UpdateBoundsFromHist(hHad, false, xMinD, xMaxD, yMinPos, yMaxD);

        const double xMin = pc.autoRange ? 0.0 : pc.xMin;
        const double xMax = pc.autoRange ? (xMaxD * 1.05) : pc.xMax;
        const double yMin = pc.autoRange ? std::pow(10.0, std::floor(std::log10(yMinPos)) - 0.3) : pc.yMin;
        const double yMax = pc.autoRange ? std::pow(10.0, std::ceil(std::log10(yMaxD)) + 0.5)    : pc.yMax;

        auto* c = new TCanvas(Form("c_icsd_%s", d.cfg.tag.c_str()), pc.titlePrefix.c_str(), ac.canvasWidth, ac.canvasHeight);
        auto* p = new TPad("p", "", 0, 0, 1, 1);
        p->SetLeftMargin(ac.leftMargin); p->SetRightMargin(ac.rightMargin);
        p->SetTopMargin(ac.topMargin);   p->SetBottomMargin(ac.bottomMargin);
        p->SetLogx(pc.logX); p->SetLogy(pc.logY); p->SetTickx(1); p->SetTicky(1);
        p->Draw(); p->cd();

        auto* hF = new TH1D(Form("hF_icsd_%s", d.cfg.tag.c_str()), "", 100, xMin, xMax);
        hF->Draw("AXIS");
        hF->GetXaxis()->SetTitle(pc.xTitle.c_str());
        hF->GetXaxis()->SetTitleSize(0.042); hF->GetXaxis()->SetTitleOffset(1.15);
        hF->GetXaxis()->SetLabelSize(0.036); hF->GetXaxis()->CenterTitle(true);
        hF->GetYaxis()->SetRangeUser(yMin, yMax);
        hF->GetYaxis()->SetTitle(pc.yTitle.c_str());
        hF->GetYaxis()->SetTitleSize(0.042); hF->GetYaxis()->SetTitleOffset(1.08);
        hF->GetYaxis()->SetLabelSize(0.036); hF->GetYaxis()->CenterTitle(true);

        std::vector<TObject*> objs; std::vector<std::string> labs, opts;
        auto* gTot = HistToCleanGraph(hTot, yMin * 0.1, false);
        gTot->SetLineColor(kBlack); gTot->SetLineWidth(pc.lineWidth); gTot->Draw("L SAME");
        objs.push_back(gTot); labs.push_back("Total (EM + Hadronic)"); opts.push_back("l");

        if (hHad && hHad->Integral() > 0.0) {
            auto* gHad = HistToCleanGraph(hHad, yMin * 0.1, false);
            gHad->SetLineColor(kRed + 1); gHad->SetLineWidth(pc.lineWidth); gHad->Draw("L SAME");
            objs.push_back(gHad); labs.push_back("Hadronic fragments"); opts.push_back("l");
        }
        if (hEM && hEM->Integral() > 0.0) {
            auto* gEM = HistToCleanGraph(hEM, yMin * 0.1, false);
            gEM->SetLineColor(kBlue + 1); gEM->SetLineStyle(2); gEM->SetLineWidth(pc.lineWidth); gEM->Draw("L SAME");
            objs.push_back(gEM); labs.push_back("Primary ^{20}Ne EM"); opts.push_back("l");
        }

        DrawDualLegend(p, objs, labs, opts, pc.legX1, pc.legY1, pc.legX2, pc.legY2, pc.textSize);
        DrawHeaderBanner(p, Form("#bf{%s}  |  %s", pc.titlePrefix.c_str(), d.cfg.legendLabel.c_str()), ac.leftMargin, 0.942, pc.textSize);
        p->RedrawAxis();
        c->Update();
        std::cout << "  [Saved] " << SavePlot(c, ac, "ICSD_Breakdown_" + d.cfg.tag) << "\n";
        delete c;

        outDir->cd();
        hTot->Write(); if (hEM) hEM->Write(); if (hHad) hHad->Write();
        delete hTot; delete hEM; delete hHad; delete hF;
    }

    // Полный диапазон ICSD по событиям прямо из BioData (без обрезки на 10 000)
    if (ac.plotICSDFull.enabled && !d.hitIonsTot.empty() && d.nEvents > 0) {
        const auto& pc = ac.plotICSDFull;
        int maxI = 1;
        for (int v : d.hitIonsTot) maxI = std::max(maxI, v);
        for (int v : d.hitIonsEM)  maxI = std::max(maxI, v);
        for (int v : d.hitIonsHad) maxI = std::max(maxI, v);

        const double lo = pc.autoRange ? 0.8 : pc.xMin;
        const double hi = pc.autoRange ? (static_cast<double>(maxI) * 1.2) : pc.xMax;
        const double w  = 1.0 / static_cast<double>(d.nEvents);

        auto toDbl = [](const std::vector<int>& src) {
            std::vector<double> dst; dst.reserve(src.size());
            for (int v : src) dst.push_back(static_cast<double>(v));
            return dst;
        };

        auto* hTot = BuildLogHistFromDoubles(toDbl(d.hitIonsTot), lo, hi, 110, w, Form("hIcsdFullTot_%s", d.cfg.tag.c_str()));
        auto* hEM  = BuildLogHistFromDoubles(toDbl(d.hitIonsEM),  lo, hi, 110, w, Form("hIcsdFullEM_%s",  d.cfg.tag.c_str()));
        auto* hHad = BuildLogHistFromDoubles(toDbl(d.hitIonsHad), lo, hi, 110, w, Form("hIcsdFullHad_%s", d.cfg.tag.c_str()));

        double xMinD = lo, xMaxD = hi, yMinPos = std::numeric_limits<double>::max(), yMaxD = 0.0;
        UpdateBoundsFromHist(hTot, true, xMinD, xMaxD, yMinPos, yMaxD);
        UpdateBoundsFromHist(hEM,  true, xMinD, xMaxD, yMinPos, yMaxD);
        UpdateBoundsFromHist(hHad, true, xMinD, xMaxD, yMinPos, yMaxD);

        if (yMaxD > 0.0) {
            const double yMin = pc.autoRange ? std::pow(10.0, std::floor(std::log10(yMinPos)) - 0.3) : pc.yMin;
            const double yMax = pc.autoRange ? std::pow(10.0, std::ceil(std::log10(yMaxD)) + 0.6)    : pc.yMax;

            auto* c = new TCanvas(Form("c_icsdfull_%s", d.cfg.tag.c_str()), pc.titlePrefix.c_str(), ac.canvasWidth, ac.canvasHeight);
            auto* p = new TPad("p", "", 0, 0, 1, 1);
            p->SetLeftMargin(ac.leftMargin); p->SetRightMargin(ac.rightMargin);
            p->SetTopMargin(ac.topMargin);   p->SetBottomMargin(ac.bottomMargin);
            p->SetLogx(pc.logX); p->SetLogy(pc.logY); p->SetTickx(1); p->SetTicky(1);
            p->Draw(); p->cd();

            auto* hF = new TH1D(Form("hF_icsdfull_%s", d.cfg.tag.c_str()), "", 100, lo, hi);
            hF->Draw("AXIS");
            hF->GetXaxis()->SetTitle(pc.xTitle.c_str());
            hF->GetXaxis()->SetTitleSize(0.042); hF->GetXaxis()->SetTitleOffset(1.15);
            hF->GetXaxis()->SetLabelSize(0.036); hF->GetXaxis()->CenterTitle(true);
            hF->GetYaxis()->SetRangeUser(yMin, yMax);
            hF->GetYaxis()->SetTitle(pc.yTitle.c_str());
            hF->GetYaxis()->SetTitleSize(0.042); hF->GetYaxis()->SetTitleOffset(1.08);
            hF->GetYaxis()->SetLabelSize(0.036); hF->GetYaxis()->CenterTitle(true);

            std::vector<TObject*> objs; std::vector<std::string> labs, opts;
            auto* gTot = HistToCleanGraph(hTot, yMin * 0.1, true);
            gTot->SetLineColor(kBlack); gTot->SetLineWidth(pc.lineWidth); gTot->Draw("L SAME");
            objs.push_back(gTot); labs.push_back("Total (EM + Hadronic)"); opts.push_back("l");

            if (hHad->GetEntries() > 0) {
                auto* gHad = HistToCleanGraph(hHad, yMin * 0.1, true);
                gHad->SetLineColor(kRed + 1); gHad->SetLineWidth(pc.lineWidth); gHad->Draw("L SAME");
                objs.push_back(gHad); labs.push_back("Hadronic fragments"); opts.push_back("l");
            }
            if (hEM->GetEntries() > 0) {
                auto* gEM = HistToCleanGraph(hEM, yMin * 0.1, true);
                gEM->SetLineColor(kBlue + 1); gEM->SetLineStyle(2); gEM->SetLineWidth(pc.lineWidth); gEM->Draw("L SAME");
                objs.push_back(gEM); labs.push_back("Primary ^{20}Ne EM"); opts.push_back("l");
            }

            DrawDualLegend(p, objs, labs, opts, pc.legX1, pc.legY1, pc.legX2, pc.legY2, pc.textSize);
            DrawHeaderBanner(p, Form("#bf{%s}  |  %s", pc.titlePrefix.c_str(), d.cfg.legendLabel.c_str()), ac.leftMargin, 0.942, pc.textSize);
            p->RedrawAxis();
            c->Update();
            std::cout << "  [Saved] " << SavePlot(c, ac, "ICSD_FullRange_BioData_" + d.cfg.tag) << "\n";
            delete c;
            delete hF;
        }
        outDir->cd();
        hTot->Write(); hEM->Write(); hHad->Write();
        delete hTot; delete hEM; delete hHad;
    }
}

static void PlotSingleSpectraAndKinematics(const ScenarioData& d, const AnalysisConfig& ac, TDirectory* outDir) {
    if (d.nEvents <= 0) return;

    // ------------------------------------------------------------------------
    // 1. Все спектры в полной энергии [MeV] на одном графике
    // ------------------------------------------------------------------------
    double xMinD = std::numeric_limits<double>::max(), xMaxD = 0.0;
    double yMinPos = std::numeric_limits<double>::max(), yMaxD = 0.0;

    std::vector<std::pair<const ScenarioData::SpecEntry*, TH1D*>> normHists;
    for (const auto& sp : d.spectra) {
        if (!sp.hist || sp.entries <= 0.0) continue;
        auto* h = static_cast<TH1D*>(sp.hist->Clone(Form("%s_norm_%s", sp.key.c_str(), d.cfg.tag.c_str())));
        h->Scale(1.0 / static_cast<double>(d.nEvents));
        UpdateBoundsFromHist(h, true, xMinD, xMaxD, yMinPos, yMaxD);
        normHists.emplace_back(&sp, h);
    }
    if (normHists.empty()) return;

    std::cout << "\n  [Entrance Energy Summary] " << d.cfg.legendLabel << "\n";
    for (const auto& p : normHists) {
        std::cout << "    " << std::left << std::setw(24) << p.first->label
                  << "Entries = " << std::setw(10) << static_cast<long long>(p.first->entries)
                  << "Mean = " << std::fixed << std::setprecision(2) << std::setw(8) << p.first->meanMeV << " MeV"
                  << "MaxBin = " << std::setw(8) << p.first->maxMeV << " MeV\n";
    }

    if (ac.plotSpectra.enabled) {
        const auto& pc = ac.plotSpectra;
        const double xMin = pc.autoRange ? (xMinD * 0.8) : pc.xMin;
        const double xMax = pc.autoRange ? (xMaxD * 1.2) : pc.xMax;
        const double yMin = pc.autoRange ? std::pow(10.0, std::floor(std::log10(yMinPos)) - 0.3) : pc.yMin;
        const double yMax = pc.autoRange ? std::pow(10.0, std::ceil(std::log10(yMaxD)) + 0.6)    : pc.yMax;

        auto* c = new TCanvas(Form("c_spec_%s", d.cfg.tag.c_str()), pc.titlePrefix.c_str(), ac.canvasWidth, ac.canvasHeight);
        auto* p = new TPad("p", "", 0, 0, 1, 1);
        p->SetLeftMargin(ac.leftMargin); p->SetRightMargin(ac.rightMargin);
        p->SetTopMargin(ac.topMargin);   p->SetBottomMargin(ac.bottomMargin);
        p->SetLogx(pc.logX); p->SetLogy(pc.logY); p->SetTickx(1); p->SetTicky(1);
        p->Draw(); p->cd();

        auto* hF = new TH1D(Form("hF_spec_%s", d.cfg.tag.c_str()), "", 100, xMin, xMax);
        hF->Draw("AXIS");
        hF->GetXaxis()->SetTitle(pc.xTitle.c_str());
        hF->GetXaxis()->SetTitleSize(0.042); hF->GetXaxis()->SetTitleOffset(1.18);
        hF->GetXaxis()->SetLabelSize(0.036); hF->GetXaxis()->CenterTitle(true);
        hF->GetYaxis()->SetRangeUser(yMin, yMax);
        hF->GetYaxis()->SetTitle(pc.yTitle.c_str());
        hF->GetYaxis()->SetTitleSize(0.042); hF->GetYaxis()->SetTitleOffset(1.08);
        hF->GetYaxis()->SetLabelSize(0.036); hF->GetYaxis()->CenterTitle(true);

        std::vector<TObject*> objs; std::vector<std::string> labs, opts;
        for (const auto& pair : normHists) {
            auto* g = HistToCleanGraph(pair.second, yMin * 0.1, true);
            g->SetLineColor(pair.first->color);
            g->SetLineStyle(pair.first->style);
            g->SetLineWidth(pc.lineWidth);
            g->Draw("L SAME");
            objs.push_back(g); labs.push_back(pair.first->label); opts.push_back("l");
        }

        DrawDualLegend(p, objs, labs, opts, pc.legX1, pc.legY1, pc.legX2, pc.legY2, pc.textSize);
        DrawHeaderBanner(p, Form("#bf{%s}  |  %s", pc.titlePrefix.c_str(), d.cfg.legendLabel.c_str()), ac.leftMargin, 0.942, pc.textSize);
        p->RedrawAxis();
        c->Update();
        std::cout << "  [Saved] " << SavePlot(c, ac, "EntranceSpectra_" + d.cfg.tag) << "\n";
        delete c; delete hF;
    }

    // ------------------------------------------------------------------------
    // 2. НОВЫЙ ГРАФИК: Все спектры в единой шкале E_{kin}/A [MeV/u] (кинематика!)
    // ------------------------------------------------------------------------
    if (ac.plotKinematicsMeVu.enabled) {
        const auto& pc = ac.plotKinematicsMeVu;
        double kxMin = std::numeric_limits<double>::max(), kxMax = 0.0;
        double kyMinPos = std::numeric_limits<double>::max(), kyMax = 0.0;

        struct KinCurve { std::string label; Int_t col; Int_t st; TH1D* hist; };
        std::vector<KinCurve> kCurves;

        for (const auto& pair : normHists) {
            if (pair.first->key == "SpecIn_AllHad" || pair.first->key == "SpecIn_Heavy") continue;
            auto* hScaled = ScaleHistAxisX(pair.second, pair.first->massNumberA,
                                           Form("hKin_%s_%s", pair.first->key.c_str(), d.cfg.tag.c_str()));
            UpdateBoundsFromHist(hScaled, true, kxMin, kxMax, kyMinPos, kyMax);
            kCurves.push_back({pair.first->label, pair.first->color, pair.first->style, hScaled});
        }

        if (d.hHeavyPerNucl && d.hHeavyPerNucl->GetEntries() > 0) {
            auto* hHvy = static_cast<TH1D*>(d.hHeavyPerNucl->Clone(Form("hKin_Heavy_%s", d.cfg.tag.c_str())));
            hHvy->Scale(1.0 / static_cast<double>(d.nEvents));
            UpdateBoundsFromHist(hHvy, true, kxMin, kxMax, kyMinPos, kyMax);
            kCurves.push_back({"Heavy (Z #geq 3)", kCyan + 2, 1, hHvy});
        }

        if (!kCurves.empty() && kyMax > 0.0) {
            const double xMin = pc.autoRange ? (kxMin * 0.8) : pc.xMin;
            const double xMax = pc.autoRange ? (kxMax * 1.2) : pc.xMax;
            const double yMin = pc.autoRange ? std::pow(10.0, std::floor(std::log10(kyMinPos)) - 0.3) : pc.yMin;
            const double yMax = pc.autoRange ? std::pow(10.0, std::ceil(std::log10(kyMax)) + 0.6)    : pc.yMax;

            auto* c = new TCanvas(Form("c_kin_%s", d.cfg.tag.c_str()), pc.titlePrefix.c_str(), ac.canvasWidth, ac.canvasHeight);
            auto* p = new TPad("p", "", 0, 0, 1, 1);
            p->SetLeftMargin(ac.leftMargin); p->SetRightMargin(ac.rightMargin);
            p->SetTopMargin(ac.topMargin);   p->SetBottomMargin(ac.bottomMargin);
            p->SetLogx(pc.logX); p->SetLogy(pc.logY); p->SetTickx(1); p->SetTicky(1);
            p->Draw(); p->cd();

            auto* hF = new TH1D(Form("hF_kin_%s", d.cfg.tag.c_str()), "", 100, xMin, xMax);
            hF->Draw("AXIS");
            hF->GetXaxis()->SetTitle(pc.xTitle.c_str());
            hF->GetXaxis()->SetTitleSize(0.042); hF->GetXaxis()->SetTitleOffset(1.18);
            hF->GetXaxis()->SetLabelSize(0.036); hF->GetXaxis()->CenterTitle(true);
            hF->GetYaxis()->SetRangeUser(yMin, yMax);
            hF->GetYaxis()->SetTitle(pc.yTitle.c_str());
            hF->GetYaxis()->SetTitleSize(0.042); hF->GetYaxis()->SetTitleOffset(1.08);
            hF->GetYaxis()->SetLabelSize(0.036); hF->GetYaxis()->CenterTitle(true);

            std::vector<TObject*> objs; std::vector<std::string> labs, opts;
            for (const auto& kc : kCurves) {
                auto* g = HistToCleanGraph(kc.hist, yMin * 0.1, true);
                g->SetLineColor(kc.col); g->SetLineStyle(kc.st); g->SetLineWidth(pc.lineWidth);
                g->Draw("L SAME");
                objs.push_back(g); labs.push_back(kc.label); opts.push_back("l");
            }

            DrawDualLegend(p, objs, labs, opts, pc.legX1, pc.legY1, pc.legX2, pc.legY2, pc.textSize);
            DrawHeaderBanner(p, Form("#bf{%s}  |  %s", pc.titlePrefix.c_str(), d.cfg.legendLabel.c_str()), ac.leftMargin, 0.942, pc.textSize);
            p->RedrawAxis();
            c->Update();
            std::cout << "  [Saved] " << SavePlot(c, ac, "Kinematics_AllSpectra_MeVu_" + d.cfg.tag) << "\n";
            delete c; delete hF;
        }
        outDir->cd();
        for (auto& kc : kCurves) { kc.hist->Write(); delete kc.hist; }
    }

    // ------------------------------------------------------------------------
    // 3. НОВЫЙ ГРАФИК: Обзорная сетка 3x3 со всеми 9 спектрами раздельно
    // ------------------------------------------------------------------------
    if (ac.plotSpectraGrid.enabled) {
        const auto& pc = ac.plotSpectraGrid;

        struct PanelData { std::string title; Int_t col; Int_t st; TH1D* hist; bool perNucl; };
        std::vector<PanelData> panels;
        for (const auto& pair : normHists) {
            panels.push_back({pair.first->label, pair.first->color, pair.first->style, pair.second, false});
        }
        if (d.hHeavyPerNucl && d.hHeavyPerNucl->GetEntries() > 0) {
            auto* hHN = static_cast<TH1D*>(d.hHeavyPerNucl->Clone(Form("hGrid_HeavyPerNucl_%s", d.cfg.tag.c_str())));
            hHN->Scale(1.0 / static_cast<double>(d.nEvents));
            panels.push_back({"Heavy (Z #geq 3) [E_{kin}/A]", kCyan + 2, 1, hHN, true});
        }

        if (!panels.empty()) {
            auto* cGrid = new TCanvas(Form("c_grid_%s", d.cfg.tag.c_str()), pc.titlePrefix.c_str(),
                                      ac.gridCanvasWidth, ac.gridCanvasHeight);
            cGrid->Divide(3, 3, 0.006, 0.008);

            for (size_t i = 0; i < panels.size() && i < 9; ++i) {
                TPad* pad = static_cast<TPad*>(cGrid->cd(static_cast<int>(i + 1)));
                pad->SetLeftMargin(0.14); pad->SetRightMargin(0.05);
                pad->SetTopMargin(0.11);  pad->SetBottomMargin(0.15);
                pad->SetLogx(pc.logX); pad->SetLogy(pc.logY);
                pad->SetTickx(1); pad->SetTicky(1);

                double pxMin = std::numeric_limits<double>::max(), pxMax = 0.0;
                double pyMinPos = std::numeric_limits<double>::max(), pyMax = 0.0;
                UpdateBoundsFromHist(panels[i].hist, true, pxMin, pxMax, pyMinPos, pyMax);
                if (pyMax <= 0.0) continue;

                const double x1 = pc.autoRange ? (pxMin * 0.8) : (panels[i].perNucl ? 1e-3 : pc.xMin);
                const double x2 = pc.autoRange ? (pxMax * 1.2) : (panels[i].perNucl ? 200.0 : pc.xMax);
                const double y1 = pc.autoRange ? std::pow(10.0, std::floor(std::log10(pyMinPos)) - 0.3) : pc.yMin;
                const double y2 = pc.autoRange ? std::pow(10.0, std::ceil(std::log10(pyMax)) + 0.6)    : pc.yMax;

                auto* hF = new TH1D(Form("hF_g%zu_%s", i, d.cfg.tag.c_str()), "", 100, x1, x2);
                hF->Draw("AXIS");
                hF->GetXaxis()->SetTitle(panels[i].perNucl ? "E_{kin}/A [MeV/u]" : "E_{kin} [MeV]");
                hF->GetXaxis()->SetTitleSize(0.050); hF->GetXaxis()->SetTitleOffset(1.15);
                hF->GetXaxis()->SetLabelSize(0.044); hF->GetXaxis()->CenterTitle(true);
                hF->GetYaxis()->SetRangeUser(y1, y2);
                hF->GetYaxis()->SetTitle("dN / d(ln E) per primary");
                hF->GetYaxis()->SetTitleSize(0.050); hF->GetYaxis()->SetTitleOffset(1.22);
                hF->GetYaxis()->SetLabelSize(0.044); hF->GetYaxis()->CenterTitle(true);

                auto* g = HistToCleanGraph(panels[i].hist, y1 * 0.1, true);
                g->SetLineColor(panels[i].col); g->SetLineStyle(panels[i].st); g->SetLineWidth(pc.lineWidth);
                g->Draw("L SAME");

                TLatex info;
                info.SetNDC(); info.SetTextFont(42);
                info.SetTextSize(0.048); info.SetTextAlign(12);
                info.DrawLatex(0.14, 0.945, Form("#bf{%s}", panels[i].title.c_str()));

                info.SetTextSize(0.040);
                const char* unit = panels[i].perNucl ? "MeV/u" : "MeV";
                info.DrawLatex(0.18, 0.82, Form("N = %.0f | Mean = %.1f %s",
                                                panels[i].hist->GetEntries(), panels[i].hist->GetMean(), unit));
                pad->RedrawAxis();
            }

            cGrid->Update();
            std::cout << "  [Saved] " << SavePlot(cGrid, ac, "AllSpectra_Grid3x3_" + d.cfg.tag) << "\n";
            delete cGrid;
            if (panels.back().perNucl) delete panels.back().hist;
        }
    }

    outDir->cd();
    for (auto& pair : normHists) { pair.second->Write(); delete pair.second; }

    // 4. Отдельный график Heavy E/A
    if (ac.plotHeavyPerNucl.enabled && d.hHeavyPerNucl && d.hHeavyPerNucl->GetEntries() > 0) {
        const auto& pch = ac.plotHeavyPerNucl;
        auto* hHN = static_cast<TH1D*>(d.hHeavyPerNucl->Clone(Form("SpecIn_HeavyPerNucl_norm_%s", d.cfg.tag.c_str())));
        hHN->Scale(1.0 / static_cast<double>(d.nEvents));

        double hxMin = std::numeric_limits<double>::max(), hxMax = 0.0;
        double hyMinPos = std::numeric_limits<double>::max(), hyMax = 0.0;
        UpdateBoundsFromHist(hHN, true, hxMin, hxMax, hyMinPos, hyMax);

        const double x1 = pch.autoRange ? (hxMin * 0.8) : pch.xMin;
        const double x2 = pch.autoRange ? (hxMax * 1.2) : pch.xMax;
        const double y1 = pch.autoRange ? std::pow(10.0, std::floor(std::log10(hyMinPos)) - 0.3) : pch.yMin;
        const double y2 = pch.autoRange ? std::pow(10.0, std::ceil(std::log10(hyMax)) + 0.5)    : pch.yMax;

        auto* cH = new TCanvas(Form("c_hvy_%s", d.cfg.tag.c_str()), pch.titlePrefix.c_str(), ac.canvasWidth, ac.canvasHeight);
        auto* pH = new TPad("pH", "", 0, 0, 1, 1);
        pH->SetLeftMargin(ac.leftMargin); pH->SetRightMargin(ac.rightMargin);
        pH->SetTopMargin(ac.topMargin);   pH->SetBottomMargin(ac.bottomMargin);
        pH->SetLogx(pch.logX); pH->SetLogy(pch.logY); pH->SetTickx(1); pH->SetTicky(1);
        pH->Draw(); pH->cd();

        auto* hFH = new TH1D(Form("hFH_%s", d.cfg.tag.c_str()), "", 100, x1, x2);
        hFH->Draw("AXIS");
        hFH->GetXaxis()->SetTitle(pch.xTitle.c_str());
        hFH->GetXaxis()->SetTitleSize(0.042); hFH->GetXaxis()->SetTitleOffset(1.18);
        hFH->GetXaxis()->SetLabelSize(0.036); hFH->GetXaxis()->CenterTitle(true);
        hFH->GetYaxis()->SetRangeUser(y1, y2);
        hFH->GetYaxis()->SetTitle(pch.yTitle.c_str());
        hFH->GetYaxis()->SetTitleSize(0.042); hFH->GetYaxis()->SetTitleOffset(1.08);
        hFH->GetYaxis()->SetLabelSize(0.036); hFH->GetYaxis()->CenterTitle(true);

        auto* gH = HistToCleanGraph(hHN, y1 * 0.1, true);
        gH->SetLineColor(kCyan + 2); gH->SetLineWidth(pch.lineWidth); gH->Draw("L SAME");

        DrawDualLegend(pH, {gH}, {"Heavy ions (Z #geq 3)"}, {"l"}, pch.legX1, pch.legY1, pch.legX2, pch.legY2, pch.textSize);
        DrawHeaderBanner(pH, Form("#bf{%s}  |  %s", pch.titlePrefix.c_str(), d.cfg.legendLabel.c_str()), ac.leftMargin, 0.942, pch.textSize);
        pH->RedrawAxis();
        cH->Update();
        std::cout << "  [Saved] " << SavePlot(cH, ac, "HeavyPerNucl_" + d.cfg.tag) << "\n";
        delete cH;

        outDir->cd();
        hHN->Write();
        delete hHN; delete hFH;
    }
}

// ============================================================================
// 7. СРАВНИТЕЛЬНЫЕ ГРАФИКИ МЕЖДУ СЦЕНАРИЯМИ
// ============================================================================
static void PlotCompareGroupedBars(const std::vector<ScenarioData>& all, const AnalysisConfig& ac,
                                   bool isDoseMode, TDirectory* cmpDir) {
    if (all.size() < 2) return;
    const auto& pc = isDoseMode ? ac.plotCompareDose : ac.plotCompareFrag;
    if (!pc.enabled) return;

    const int nS = static_cast<int>(all.size());

    struct Item { FamilyIdx fam; const char* specKey; const char* label; };
    std::vector<Item> items;
    if (isDoseMode) {
        items = {
            {FamilyIdx::Total,     "", "Total"},
            {FamilyIdx::PrimaryEM, "", "EM ^{20}Ne"},
            {FamilyIdx::HadTotal,  "", "Hadronic"},
            {FamilyIdx::Alpha,     "", "#alpha"},
            {FamilyIdx::Proton,    "", "p"},
            {FamilyIdx::Helium3,   "", "^{3}He"},
            {FamilyIdx::Deuteron,  "", "d"},
            {FamilyIdx::Triton,    "", "t"},
            {FamilyIdx::HeavyIon,  "", "Heavy"},
            {FamilyIdx::Neutron,   "", "n"}
        };
    } else {
        items = {
            {FamilyIdx::Proton,   "SpecIn_Proton",   "p"},
            {FamilyIdx::Alpha,    "SpecIn_Alpha",    "#alpha"},
            {FamilyIdx::Deuteron, "SpecIn_Deuteron", "d"},
            {FamilyIdx::Triton,   "SpecIn_Triton",   "t"},
            {FamilyIdx::Helium3,  "SpecIn_He3",      "^{3}He"},
            {FamilyIdx::HeavyIon, "SpecIn_Heavy",    "Heavy (Z#geq3)"},
            {FamilyIdx::Neutron,  "SpecIn_Neutron",  "n"}
        };
    }

    const int nB = static_cast<int>(items.size());
    std::vector<TH1D*> hists(nS, nullptr);
    double minPos = std::numeric_limits<double>::max(), maxVal = 0.0;
    bool allHaveMass = true;
    for (const auto& d : all) if (!d.hasMass) allHaveMass = false;

    for (int s = 0; s < nS; ++s) {
        const auto& d = all[s];
        const double scale = (d.nEvents > 0) ? (ac.normPrimaries / static_cast<double>(d.nEvents)) : 0.0;
        hists[s] = new TH1D(Form("hCmpBar_%s_%s", isDoseMode ? "Dose" : "Frag", d.cfg.tag.c_str()),
                            "", nB, 0.0, static_cast<double>(nB));
        for (int b = 0; b < nB; ++b) {
            double val = 0.0;
            if (isDoseMode) {
                const double eMeV = d.fam[static_cast<size_t>(items[b].fam)].edepMeV;
                val = allHaveMass ? (EdepMeVToGy(eMeV, d.targetMassKg) * scale) : (eMeV * scale);
            } else {
                for (const auto& sp : d.spectra) {
                    if (sp.key == items[b].specKey && sp.hist) { val = sp.entries * scale; break; }
                }
            }
            hists[s]->SetBinContent(b + 1, val);
            hists[s]->GetXaxis()->SetBinLabel(b + 1, items[b].label);
            if (val > 0.0) { minPos = std::min(minPos, val); maxVal = std::max(maxVal, val); }
        }
    }

    if (maxVal <= 0.0) { for (auto* h : hists) delete h; return; }

    auto* c = new TCanvas(Form("c_cmp_%s", isDoseMode ? "dose" : "frag"), pc.titlePrefix.c_str(), ac.canvasWidth, ac.canvasHeight);
    auto* p = new TPad("p", "", 0, 0, 1, 1);
    p->SetLeftMargin(ac.leftMargin); p->SetRightMargin(ac.rightMargin);
    p->SetTopMargin(ac.topMargin);   p->SetBottomMargin(0.18);
    p->SetLogy(pc.logY); p->SetTickx(1); p->SetTicky(1);
    p->Draw(); p->cd();

    const double yMin = pc.autoRange ? std::pow(10.0, std::floor(std::log10(minPos)) - 0.5) : pc.yMin;
    const double yMax = pc.autoRange ? std::pow(10.0, std::ceil(std::log10(maxVal)) + 0.9)  : pc.yMax;
    const double bw = 0.80 / nS;

    std::vector<TObject*> objs; std::vector<std::string> labs, opts;
    for (int s = 0; s < nS; ++s) {
        hists[s]->GetYaxis()->SetRangeUser(yMin, yMax);
        hists[s]->GetXaxis()->SetLabelSize(0.036);
        if (isDoseMode) hists[s]->GetXaxis()->LabelsOption("u");
        hists[s]->GetYaxis()->SetTitle(
            isDoseMode ? (allHaveMass ? pc.yTitle.c_str() : Form("Energy deposited [MeV per %s]", ac.normLabel.c_str()))
                       : pc.yTitle.c_str());
        hists[s]->GetYaxis()->SetTitleSize(0.042); hists[s]->GetYaxis()->SetTitleOffset(1.10);
        hists[s]->GetYaxis()->SetLabelSize(0.036); hists[s]->GetYaxis()->CenterTitle(true);

        hists[s]->SetFillColor(DistinctColorByIndex(s));
        hists[s]->SetFillStyle(1001);
        hists[s]->SetLineColor(kBlack);
        hists[s]->SetLineWidth(1);
        hists[s]->SetBarWidth(bw * 0.92);
        hists[s]->SetBarOffset(0.10 + s * bw);
        hists[s]->Draw(s == 0 ? "BAR" : "BAR SAME");

        objs.push_back(hists[s]); labs.push_back(ShortLegendLabel(all[s].cfg.legendLabel)); opts.push_back("f");
    }

    DrawColumnLegend(p, objs, labs, opts, 0.30, 0.58, 0.99, 0.90, 0.021, nS > 8 ? 3 : 2);
    DrawHeaderBanner(p, Form("#bf{%s}", pc.titlePrefix.c_str()), ac.leftMargin, 0.942, pc.textSize);
    p->RedrawAxis();
    c->Update();
    std::string outName = isDoseMode ? "Compare_DoseBySpecies" : "Compare_FragmentYields";
    std::cout << "  [Saved] " << SavePlot(c, ac, outName) << "\n";
    delete c;

    cmpDir->cd();
    for (auto* h : hists) { h->Write(); delete h; }
}

static void PlotCompareOverlayWithRatio(const std::vector<ScenarioData>& all,
                                        const AnalysisConfig& ac,
                                        const PlotConfig& pc,
                                        const char* histKey,
                                        bool isICSD,
                                        const std::string& outName,
                                        TDirectory* cmpDir) {
    if (!pc.enabled || all.size() < 2) return;

    std::vector<TH1D*> hists;
    std::vector<const ScenarioConfig*> cfgs;
    double xMinD = std::numeric_limits<double>::max(), xMaxD = 0.0;
    double yMinPos = std::numeric_limits<double>::max(), yMaxD = 0.0;

    for (const auto& d : all) {
        const TH1D* src = nullptr;
        if (isICSD) {
            src = d.hIcsdTargetTot;
        } else if (std::string(histKey) == "HadronVertexZ") {
            src = d.hHadronVertexZ;
        } else {
            for (const auto& sp : d.spectra) {
                if (sp.key == histKey) { src = sp.hist; break; }
            }
        }
        if (!src || src->GetEntries() <= 0) continue;

        auto* h = static_cast<TH1D*>(src->Clone(Form("hCmp_%s_%s", histKey, d.cfg.tag.c_str())));
        if (pc.rebin > 1) h->Rebin(pc.rebin);

        if (isICSD) {
            const double norm = h->Integral();
            if (norm > 0.0) h->Scale(1.0 / norm);
        } else if (d.nEvents > 0) {
            h->Scale(1.0 / static_cast<double>(d.nEvents));
        }

        UpdateBoundsFromHist(h, !isICSD, xMinD, xMaxD, yMinPos, yMaxD);
        hists.push_back(h);
        cfgs.push_back(&d.cfg);
    }

    if (hists.size() < 2 || yMaxD <= 0.0) {
        for (auto* h : hists) delete h;
        return;
    }

    const double xMin = pc.autoRange ? (isICSD ? 0.0 : xMinD * 0.8) : pc.xMin;
    const double xMax = pc.autoRange ? (xMaxD * (isICSD ? 1.05 : 1.2)) : pc.xMax;
    const double yMin = pc.autoRange ? std::pow(10.0, std::floor(std::log10(yMinPos)) - 0.3) : pc.yMin;
    const double yMax = pc.autoRange ? std::pow(10.0, std::ceil(std::log10(yMaxD)) + 0.6)    : pc.yMax;

    // Single-pad overlay (the old ratio pad was only for comparison with
    // experimental data and is no longer wanted).
    auto* c = new TCanvas(Form("c_cmp_%s", outName.c_str()), outName.c_str(), ac.canvasWidth, ac.canvasHeight);
    auto* pad1 = new TPad("pad1", "main", 0.0, 0.0, 1.0, 1.0);
    pad1->SetLeftMargin(ac.leftMargin); pad1->SetRightMargin(ac.rightMargin);
    pad1->SetTopMargin(ac.topMargin);   pad1->SetBottomMargin(ac.bottomMargin);
    pad1->SetLogy(pc.logY);
    if (pc.logX) pad1->SetLogx();
    pad1->SetTickx(1); pad1->SetTicky(1);
    pad1->Draw(); pad1->cd();

    auto* hF1 = new TH1D(Form("hF1_%s", outName.c_str()), "", 100, xMin, xMax);
    hF1->Draw("AXIS");
    hF1->GetXaxis()->SetTitle(pc.xTitle.c_str());
    hF1->GetXaxis()->SetTitleSize(0.042); hF1->GetXaxis()->SetTitleOffset(1.18);
    hF1->GetXaxis()->SetLabelSize(0.036); hF1->GetXaxis()->CenterTitle(true);
    hF1->GetYaxis()->SetRangeUser(yMin, yMax);
    hF1->GetYaxis()->SetTitle(pc.yTitle.c_str());
    hF1->GetYaxis()->SetTitleSize(0.042); hF1->GetYaxis()->SetTitleOffset(1.08);
    hF1->GetYaxis()->SetLabelSize(0.036); hF1->GetYaxis()->CenterTitle(true);

    std::vector<TObject*> objs; std::vector<std::string> labs, opts;
    for (size_t i = 0; i < hists.size(); ++i) {
        auto* g = HistToCleanGraph(hists[i], yMin * 0.1, !isICSD);
        g->SetLineColor(DistinctColorByIndex(static_cast<int>(i)));
        g->SetLineStyle(cfgs[i]->lineStyle);
        g->SetLineWidth(pc.lineWidth);
        g->Draw("L SAME");
        objs.push_back(g); labs.push_back(ShortLegendLabel(cfgs[i]->legendLabel)); opts.push_back("l");
    }
    DrawDualLegend(pad1, objs, labs, opts, pc.legX1, pc.legY1, pc.legX2, pc.legY2, pc.textSize,
                   hists.size() > 8 ? 2 : 1);
    DrawHeaderBanner(pad1, Form("#bf{%s}", pc.titlePrefix.c_str()), ac.leftMargin, 0.942, pc.textSize);
    pad1->RedrawAxis();
    c->Update();
    std::cout << "  [Saved] " << SavePlot(c, ac, outName) << "\n";
    delete c;

    cmpDir->cd();
    for (auto* h : hists) { h->Write(); delete h; }
    delete hF1;
}

// ============================================================================
// 8. ТОЧКА ВХОДА
// ============================================================================
void analyzeLRB() {
    SetupStyle();
    AnalysisConfig ac;
    ApplyAnalysisConfFile(ac, "config/analysis.conf");

    fs::create_directories(ac.outputDir);
    if (fs::path(ac.outputRootFile).has_parent_path()) {
        fs::create_directories(fs::path(ac.outputRootFile).parent_path());
    }

    std::vector<ScenarioData> loaded;
    for (const auto& sc : ac.scenarios) {
        // Validation micro-runs (test_*) are not part of the production figure
        // set; skip them so they do not add noise to the plots.
        if (sc.tag.rfind("test_", 0) == 0) { std::cout << "\n>>> Skipping validation run: " << sc.tag << "\n"; continue; }
        std::cout << "\n>>> Loading: " << sc.rootFile << " ...\n";
        ScenarioData data;
        if (LoadScenarioData(sc, data)) {
            PrintAuditTable(data, ac);
            loaded.push_back(std::move(data));
        }
    }

    if (loaded.empty()) {
        std::cerr << "[FATAL] No scenarios could be loaded. Execution stopped." << std::endl;
        return;
    }

    TFile* outRoot = TFile::Open(ac.outputRootFile.c_str(), "RECREATE");
    if (!outRoot || outRoot->IsZombie()) {
        std::cerr << "[ERROR] Cannot create output ROOT file: " << ac.outputRootFile << std::endl;
        return;
    }

    for (const auto& d : loaded) {
        TDirectory* sub = outRoot->mkdir(d.cfg.tag.c_str());
        PlotSingleDoseBars(d, ac, sub);
        if (d.hasSubChannels) PlotSingleSubChannels(d, ac, sub);
        PlotSingleEdepEventDist(d, ac, sub);
        PlotSingleEdepBySpecies(d, ac, sub);
        PlotSingleEdepDirectDelta(d, ac, sub);
        PlotSingleEdepBySpeciesLinear(d, ac, sub);
        PlotSingleFragmentCounts(d, ac, sub);
        PlotSingleICSD(d, ac, sub);
        PlotSingleSpectraAndKinematics(d, ac, sub);
        if (d.hasFragments) PlotSingleFragments(d, ac, sub);
        PlotSingleHadronVertexZ(d, ac, sub);
    }

    if (loaded.size() >= 2) {
        TDirectory* cmpDir = outRoot->mkdir("Comparisons");
        PlotCompareGroupedBars(loaded, ac, true,  cmpDir);
        PlotCompareGroupedBars(loaded, ac, false, cmpDir);
        PlotCompareOverlayWithRatio(loaded, ac, ac.plotICSD,    "ICSD_Target_Total", true,  "Compare_ICSD_Target_Total", cmpDir);
        PlotCompareOverlayWithRatio(loaded, ac, ac.plotSpectra, "SpecIn_Proton",     false, "Compare_SpecIn_Proton",     cmpDir);
        PlotCompareOverlayWithRatio(loaded, ac, ac.plotSpectra, "SpecIn_Alpha",      false, "Compare_SpecIn_Alpha",      cmpDir);
        PlotCompareOverlayWithRatio(loaded, ac, ac.plotCompareHVZ, "HadronVertexZ", false, "Compare_HadronVertexZ", cmpDir);
    }

    outRoot->Write();
    outRoot->Close();
    delete outRoot;

    ExportCSV(loaded, ac);
    std::cout << "\n[Done] Output ROOT: " << ac.outputRootFile
              << "\n[Done] Plots dir:   " << ac.outputDir << "\n";
}

void plot() {
    analyzeLRB();
}