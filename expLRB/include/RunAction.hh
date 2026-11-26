// RunAction.hh
#pragma once

#include "G4UserRunAction.hh"
#include "G4Accumulable.hh"
#include "globals.hh"
#include <memory>
#include <vector>

class G4Run;
class RunMessenger;
class EnergyLossMessenger;
class DetectorConstruction;

class RunAction final : public G4UserRunAction
{
public:
    explicit RunAction(const DetectorConstruction* det = nullptr);
    ~RunAction() override;

    void BeginOfRunAction(const G4Run*) override;
    void EndOfRunAction(const G4Run*) override;

    void AddEventEdep(G4double edepEM, G4double edepHad) noexcept {
        fEdepEM  += edepEM;
        fEdepHad += edepHad;
    }

    // Dynamic per-fragment (Z,A) vectors backing the BioData *vector* branches
    // (Frag_Z/Frag_A/Frag_Edep/Frag_EdepDelta/Frag_Ions). Filled once per event.
    void SetFragments(const std::vector<int>& z, const std::vector<int>& a,
                      const std::vector<double>& edep,
                      const std::vector<double>& edepDelta,
                      const std::vector<int>& ions) {
        fFragZ = z; fFragA = a; fFragEdep = edep; fFragEdepDelta = edepDelta; fFragIons = ions;
    }

    static void SetOutputFileName(const G4String& name) {
        if (!fgFileNameLocked) fgFileName = name;
    }
    static void ForceOutputFileName(const G4String& name) {
        fgFileName = name;
        fgFileNameLocked = true;
    }
    [[nodiscard]] static const G4String& GetOutputFileName() noexcept { return fgFileName; }

private:
    static inline G4String fgFileName{"lrb_simulation_output.root"};
    static inline bool     fgFileNameLocked{false};
    const DetectorConstruction* fDetector{nullptr};
    std::unique_ptr<RunMessenger> fMessenger;
    std::unique_ptr<EnergyLossMessenger> fEnergyLossMessenger;

    G4Accumulable<G4double> fEdepEM{0.0};
    G4Accumulable<G4double> fEdepHad{0.0};

    std::vector<int>    fFragZ, fFragA, fFragIons;
    std::vector<double> fFragEdep, fFragEdepDelta;
};