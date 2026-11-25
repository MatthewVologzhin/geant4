// EnergyLossMessenger.hh
#pragma once

#include "G4UImessenger.hh"
#include "globals.hh"
#include <memory>

class G4UIdirectory;
class G4UIcmdWithAString;

// Read-only diagnostic: analytic electronic/nuclear stopping power from
// G4EmCalculator for an arbitrary ion/material/energy. It does not alter the
// physics configuration and must be invoked after /run/initialize (Idle state).
class EnergyLossMessenger final : public G4UImessenger
{
public:
    EnergyLossMessenger();
    ~EnergyLossMessenger() override;

    void SetNewValue(G4UIcommand* command, G4String newValue) override;

private:
    std::unique_ptr<G4UIdirectory>      fDir;
    std::unique_ptr<G4UIcmdWithAString> fPrintCmd;
};
