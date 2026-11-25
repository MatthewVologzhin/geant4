// PhysicsList.hh
#pragma once

#include "G4VModularPhysicsList.hh"
#include "globals.hh"
#include <CLHEP/Units/SystemOfUnits.h>
#include <memory>
#include <vector>

class PhysicsListMessenger;
class G4VPhysicsConstructor;

class PhysicsList final : public G4VModularPhysicsList
{
public:
    PhysicsList();
    ~PhysicsList() override;

    PhysicsList(const PhysicsList&) = delete;
    PhysicsList& operator=(const PhysicsList&) = delete;

    void ConstructParticle() override;
    void ConstructProcess() override;

    void SetHadronicPhysics(const G4String& name);
    void EnableHadronicPhysics(G4bool enable) noexcept { fUseHadronic = enable; }
    void SetEMPhysics(const G4String& name);
    void EnableDNAActivator(G4bool enable) noexcept { fUseDNAActivator = enable; }
    void SetLowestElectronEnergy(G4double energy) noexcept { fLowestElectronEnergy = energy; }

private:
    void BuildHadronicConstructors();

    std::unique_ptr<PhysicsListMessenger> fMessenger;

    G4String fHadName{"QGSP_BIC_HP"};
    G4String fEmName{"EM_Opt4"};
    G4bool   fUseHadronic{true};
    G4bool   fUseDNAActivator{true};
    G4double fLowestElectronEnergy{7.4 * CLHEP::eV};

    std::unique_ptr<G4VPhysicsConstructor>              fDnaActivator;
    std::vector<std::unique_ptr<G4VPhysicsConstructor>> fHadronicConstructors;
};