// PhysicsListMessenger.hh
#pragma once

#include "G4UImessenger.hh"
#include "G4UIdirectory.hh"
#include "G4UIcmdWithAString.hh"
#include "G4UIcmdWithABool.hh"
#include "G4UIcmdWithADoubleAndUnit.hh"
#include <memory>

class PhysicsList;

class PhysicsListMessenger final : public G4UImessenger
{
public:
    explicit PhysicsListMessenger(PhysicsList* pPhys);
    ~PhysicsListMessenger() override;

    void SetNewValue(G4UIcommand*, G4String) override;

private:
    PhysicsList* fPhysicsList{nullptr};
    std::unique_ptr<G4UIdirectory>             fPhysDir;
    std::unique_ptr<G4UIcmdWithAString>        fHadPhysCmd;
    std::unique_ptr<G4UIcmdWithABool>          fEnableHadCmd;
    std::unique_ptr<G4UIcmdWithAString>        fEmPhysCmd;
    std::unique_ptr<G4UIcmdWithABool>          fDnaActivatorCmd;
    std::unique_ptr<G4UIcmdWithADoubleAndUnit> fLowestElEnergyCmd;
};