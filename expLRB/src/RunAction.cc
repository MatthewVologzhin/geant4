// RunAction.cc
#include "RunAction.hh"
#include "RunMessenger.hh"
#include "DetectorConstruction.hh"
#include "BioDetectorConstruction.hh"
#include "ScoringProfile.hh"

#include "G4AnalysisManager.hh"
#include "G4AccumulableManager.hh"
#include "G4Threading.hh"
#include "G4Run.hh"
#include "G4SystemOfUnits.hh"

#include <string>

namespace {

// Creates one canonical H1 with its historical definition (index order comes
// from iterating CanonicalHistograms() under the active profile).
void CreateHistogram(G4AnalysisManager* analysis, const std::string& name)
{
    constexpr int kSpecBins = 200;
    constexpr double kEminMeV = 1.0e-3;
    constexpr double kEmaxMeV = 2.0e3;

    if (name == "ICSD_Event_Total")
        analysis->CreateH1(name, "Event Ionization Cluster Size;Ions per Event;Entries", 500, 0, 50000);
    else if (name == "ICSD_Target_EM")
        analysis->CreateH1(name, "Target EM Cluster Size;Ions;Entries", 500, 0, 10000);
    else if (name == "ICSD_Target_Had")
        analysis->CreateH1(name, "Target Hadronic Cluster Size;Ions;Entries", 500, 0, 10000);
    else if (name == "ICSD_Target_Total")
        analysis->CreateH1(name, "Target Ionization Cluster Size;Ions;Entries", 500, 0, 10000);
    else if (name == "SpecIn_AllHad")
        analysis->CreateH1(name, "Secondary Hadron KinE Entering Target;E_{kin} [MeV];Entries", kSpecBins, kEminMeV, kEmaxMeV, "none", "none", "log");
    else if (name == "SpecIn_Proton")
        analysis->CreateH1(name, "Protons Entering Target;E_{kin} [MeV];Entries", kSpecBins, kEminMeV, kEmaxMeV, "none", "none", "log");
    else if (name == "SpecIn_Deuteron")
        analysis->CreateH1(name, "Deuterons Entering Target;E_{kin} [MeV];Entries", kSpecBins, kEminMeV, kEmaxMeV, "none", "none", "log");
    else if (name == "SpecIn_Triton")
        analysis->CreateH1(name, "Tritons Entering Target;E_{kin} [MeV];Entries", kSpecBins, kEminMeV, kEmaxMeV, "none", "none", "log");
    else if (name == "SpecIn_He3")
        analysis->CreateH1(name, "Helium-3 Entering Target;E_{kin} [MeV];Entries", kSpecBins, kEminMeV, kEmaxMeV, "none", "none", "log");
    else if (name == "SpecIn_Alpha")
        analysis->CreateH1(name, "Alphas Entering Target;E_{kin} [MeV];Entries", kSpecBins, kEminMeV, kEmaxMeV, "none", "none", "log");
    else if (name == "SpecIn_Heavy")
        analysis->CreateH1(name, "Heavy Ions (Z>=3) Entering Target;E_{kin} [MeV];Entries", kSpecBins, kEminMeV, kEmaxMeV, "none", "none", "log");
    else if (name == "SpecIn_Neutron")
        analysis->CreateH1(name, "Neutrons Entering Target;E_{kin} [MeV];Entries", kSpecBins, kEminMeV, kEmaxMeV, "none", "none", "log");
    else if (name == "SpecIn_HeavyPerNucl")
        analysis->CreateH1(name, "Heavy Ions Entering Target;E_{kin}/A [MeV/u];Entries", kSpecBins, 1.0e-3, 200.0, "none", "none", "log");
    else if (name == "SpecIn_Electron")
        analysis->CreateH1(name, "Secondary Electrons Entering Target;E_{kin} [MeV];Entries", kSpecBins, kEminMeV, kEmaxMeV, "none", "none", "log");
    else if (name == "HadronVertexZ")
        analysis->CreateH1(name, "Hadron Birth Z Position;Z [mm];Entries", 600, 0.0, 30.0);
    else
        G4Exception("CreateHistogram", "UnknownHistogram", FatalException,
                    ("Unknown canonical histogram: " + name).c_str());
}

}  // namespace

RunAction::RunAction(const DetectorConstruction* det) : fDetector(det)
{
    if (G4Threading::IsMasterThread()) {
        fMessenger = std::make_unique<RunMessenger>();
    }

    auto* accManager = G4AccumulableManager::Instance();
    accManager->Register(fEdepEM);
    accManager->Register(fEdepHad);

    const auto& profile = GetActiveScoringProfile();

    auto* analysis = G4AnalysisManager::Instance();
    analysis->SetVerboseLevel(1);
    analysis->SetNtupleMerging(true);

    // Histograms in canonical order, restricted to the active profile groups.
    for (const auto& h : CanonicalHistograms()) {
        if (!profile.Has(h.group)) continue;
        CreateHistogram(analysis, h.name);
    }

    // BioData tree: the "standard" profile emits exactly the historical
    // columns 0..34; other profiles emit a contiguous subset / superset.
    analysis->CreateNtuple("BioData", "Species-Resolved Output for LRB JINR");
    for (const auto& col : CanonicalNtupleColumns()) {
        if (!profile.Has(col.group)) continue;
        if (col.isInt) analysis->CreateNtupleIColumn(col.name);
        else           analysis->CreateNtupleDColumn(col.name);
    }
    analysis->FinishNtuple();

    G4cout << "=== [expLRB] Scoring profile: " << ScoringProfileDescription(profile.name) << " ===" << G4endl;
}

RunAction::~RunAction() = default;

void RunAction::BeginOfRunAction(const G4Run* run)
{
    G4AccumulableManager::Instance()->Reset();

    auto* analysis = G4AnalysisManager::Instance();
    analysis->OpenFile(fgFileName);

    if (IsMaster()) {
        G4cout << "=== Run " << run->GetRunID() << " started. Output: "
               << fgFileName << " ===" << G4endl;
    }
}

void RunAction::EndOfRunAction(const G4Run* run)
{
    G4AccumulableManager::Instance()->Merge();

    auto* analysis = G4AnalysisManager::Instance();
    analysis->Write();
    analysis->CloseFile();

    if (IsMaster()) {
        const G4double edepEM  = fEdepEM.GetValue();
        const G4double edepHad = fEdepHad.GetValue();
        const G4double edepTot = edepEM + edepHad;

        G4cout << "=== Run " << run->GetRunID() << " completed. Output: " << fgFileName << " ===" << G4endl;
        if (fDetector && fDetector->GetBioDetector()) {
            const auto* bio = fDetector->GetBioDetector();
            const G4double mass = bio->GetScoringMass();
            const size_t nCells = bio->GetActualCellCount();
            if (mass > 0.0) {
                const G4double doseEM  = edepEM  / mass;
                const G4double doseHad = edepHad / mass;
                const G4double doseTot = edepTot / mass;
                const G4double hadShare = (edepTot > 0.0) ? (100.0 * edepHad / edepTot) : 0.0;

                G4cout << "  Packed cells: " << nCells
                       << " | Scoring mass: " << mass / mg << " mg\n"
                       << "  Dose_EM:      " << doseEM  / gray << " Gy\n"
                       << "  Dose_Had:     " << doseHad / gray << " Gy (" << hadShare << " %)\n"
                       << "  Dose_Total:   " << doseTot / gray << " Gy" << G4endl;
            }
        }
    }
}