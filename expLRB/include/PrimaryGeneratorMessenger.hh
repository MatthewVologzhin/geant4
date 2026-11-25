#pragma once

#include "G4UImessenger.hh"
#include "globals.hh"

class PrimaryGeneratorAction;
class G4UIdirectory;
class G4UIcmdWithoutParameter;
class G4UIcmdWithADouble;
class G4UIcommand;

class PrimaryGeneratorMessenger : public G4UImessenger
{
  public:
    PrimaryGeneratorMessenger(PrimaryGeneratorAction*);
    ~PrimaryGeneratorMessenger() override;

    void SetNewValue(G4UIcommand*, G4String) override;

  private:
    PrimaryGeneratorAction* fAction = nullptr;

    G4UIdirectory* fGunDir = nullptr;
    G4UIcmdWithoutParameter* fDefaultCmd = nullptr;
    G4UIcommand* fBeamRadius = nullptr;
};
