// RunAction.hh
#pragma once

#include "G4UserRunAction.hh"
#include "G4Accumulable.hh"
#include "globals.hh"
#include <memory>

class G4Run;
class RunMessenger;
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

    G4Accumulable<G4double> fEdepEM{0.0};
    G4Accumulable<G4double> fEdepHad{0.0};
};