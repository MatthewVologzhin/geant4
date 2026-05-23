#pragma once
#include "G4VModularPhysicsList.hh"
#include "globals.hh"
#include <memory>

class G4VPhysicsConstructor;
class G4EmDNAChemistry_option2;

class PhysicsList: public G4VModularPhysicsList
{
public:
    PhysicsList();
    ~PhysicsList() override;
    
    void ConstructParticle() override;
    void ConstructProcess() override;

    void RegisterConstructor(const G4String& name);

private:
    std::unique_ptr<G4VPhysicsConstructor>    fDNAPhysicsList;
    std::unique_ptr<G4EmDNAChemistry_option2> fChemistryList_option2;
    G4String                                  fPhysDNAName;
};
