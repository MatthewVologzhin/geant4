// PhysicsList.cc
#include "PhysicsList.hh"
#include "PhysicsListMessenger.hh"

#include "G4DecayPhysics.hh"
#include "G4EmDNAPhysicsActivator.hh"
#include "G4DNAGenericIonsManager.hh"
#include "G4EmParameters.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "G4PhysicsConstructorRegistry.hh"
#include "G4ProductionCutsTable.hh"
#include "G4StateManager.hh"
#include "G4SystemOfUnits.hh"

PhysicsList::PhysicsList()
{
    fMessenger = std::make_unique<PhysicsListMessenger>(this);
    SetVerboseLevel(1);
    SetDefaultCutValue(1.0 * mm);

    RegisterPhysics(new G4DecayPhysics());
    RegisterPhysics(new G4EmStandardPhysics_option4());

    fDnaActivator = std::make_unique<G4EmDNAPhysicsActivator>();
    BuildHadronicConstructors();
}

PhysicsList::~PhysicsList() = default;

void PhysicsList::SetHadronicPhysics(const G4String& name)
{
    if (G4StateManager::GetStateManager()->GetCurrentState() != G4State_PreInit) {
        G4Exception("PhysicsList::SetHadronicPhysics", "NotPreInit",
                    FatalException, "/physics/setHadronic must be called in PreInit before /run/initialize.");
        return;
    }

    if (name == "none" || name == "off" || name == "DISABLED" || name == "false") {
        fUseHadronic = false;
        fHadName = "none";
        G4cout << "### [PhysicsList] Hadronic physics set to: DISABLED" << G4endl;
        return;
    }

    if (name != "QGSP_BIC_HP" && name != "FTFP_BERT_HP" &&
        name != "QBBC" && name != "INCLXX_HP" &&
        name != "QGSP_BIC" && name != "QGSP_BERT_HP" &&
        name != "FTF_BIC" && name != "Shielding")
    {
        G4Exception("PhysicsList::SetHadronicPhysics", "InvalidHadronicName",
                    FatalException, ("Unknown hadronic physics option: " + name).c_str());
        return;
    }

    fUseHadronic = true;
    if (fHadName != name) {
        fHadName = name;
        BuildHadronicConstructors();
    }
    G4cout << "### [PhysicsList] Hadronic physics set to: " << fHadName << G4endl;
}

void PhysicsList::SetEMPhysics(const G4String& name)
{
    if (G4StateManager::GetStateManager()->GetCurrentState() != G4State_PreInit) {
        G4Exception("PhysicsList::SetEMPhysics", "NotPreInit",
                    FatalException, "/physics/setEM must be called in PreInit before /run/initialize.");
        return;
    }

    G4String ctorName;
    if (name == "EM_Opt4")                                    ctorName = "G4EmStandardPhysics_option4";
    else if (name == "EM_Opt3")                               ctorName = "G4EmStandardPhysics_option3";
    else if (name == "EM_Opt0" || name == "emstandard_opt0")  ctorName = "G4EmStandardPhysics";
    else if (name == "DNA_Opt0")                              ctorName = "G4EmDNAPhysics";
    else if (name == "DNA_Opt2")                              ctorName = "G4EmDNAPhysics_option2";
    else if (name == "DNA_Opt4")                              ctorName = "G4EmDNAPhysics_option4";
    else if (name == "DNA_Opt6")                              ctorName = "G4EmDNAPhysics_option6";
    else if (name == "DNA_Opt8")                              ctorName = "G4EmDNAPhysics_option8";
    else {
        G4Exception("PhysicsList::SetEMPhysics", "InvalidEMName",
                    FatalException, ("Unknown EM physics option: " + name).c_str());
        return;
    }

    auto* registry = G4PhysicsConstructorRegistry::Instance();
    auto* emCtor = registry->GetPhysicsConstructor(ctorName);
    if (!emCtor) {
        G4Exception("PhysicsList::SetEMPhysics", "RegistryLookupFailed",
                    FatalException, ("Failed to construct EM physics: " + ctorName).c_str());
        return;
    }

    ReplacePhysics(emCtor);
    emCtor->ConstructParticle();
    fEmName = name;
    G4cout << "### [PhysicsList] EM base physics set to: " << fEmName << G4endl;
}

void PhysicsList::BuildHadronicConstructors()
{
    fHadronicConstructors.clear();
    if (fHadName == "none") return;

    auto* registry = G4PhysicsConstructorRegistry::Instance();
    auto add = [this, registry](const G4String& ctorName) {
        auto* ctor = registry->GetPhysicsConstructor(ctorName);
        if (!ctor) {
            G4Exception("PhysicsList::BuildHadronicConstructors", "RegistryLookupFailed",
                        FatalException, ("Constructor not found in registry: " + ctorName).c_str());
            return;
        }
        fHadronicConstructors.emplace_back(ctor);
    };

    if (fHadName == "QGSP_BIC_HP") {
        add("G4HadronElasticPhysicsHP");
        add("G4StoppingPhysics");
        add("G4IonPhysics");
        add("G4HadronPhysicsQGSP_BIC_HP");
    } else if (fHadName == "FTFP_BERT_HP") {
        add("G4HadronElasticPhysicsHP");
        add("G4StoppingPhysics");
        add("G4IonPhysics");
        add("G4HadronPhysicsFTFP_BERT_HP");
    } else if (fHadName == "QBBC") {
        add("G4HadronElasticPhysicsXS");
        add("G4StoppingPhysics");
        add("G4IonPhysics");
        add("G4HadronInelasticQBBC");
    } else if (fHadName == "INCLXX_HP") {
        add("G4HadronElasticPhysicsHP");
        add("G4StoppingPhysics");
        add("G4IonPhysics");
        add("G4HadronPhysicsINCLXX");
    } else if (fHadName == "QGSP_BIC") {
        add("G4HadronElasticPhysics");
        add("G4StoppingPhysics");
        add("G4IonPhysics");
        add("G4HadronPhysicsQGSP_BIC");
    } else if (fHadName == "QGSP_BERT_HP") {
        add("G4HadronElasticPhysicsVI");
        add("G4StoppingPhysics");
        add("G4IonPhysics");
        add("G4HadronPhysicsQGSP_BERT_HP");
    } else if (fHadName == "FTF_BIC") {
        add("G4HadronElasticPhysics");
        add("G4StoppingPhysics");
        add("G4IonPhysics");
        add("G4HadronPhysicsFTF_BIC");
    } else if (fHadName == "Shielding") {
        add("G4HadronElasticPhysicsHP");
        add("G4StoppingPhysics");
        add("G4IonPhysics");
        add("G4HadronPhysicsShielding");
    }
}

void PhysicsList::ConstructParticle()
{
    G4VModularPhysicsList::ConstructParticle();

    for (auto& ctor : fHadronicConstructors) {
        if (ctor) ctor->ConstructParticle();
    }
    if (fDnaActivator) {
        fDnaActivator->ConstructParticle();
    }

    auto* dnaIons = G4DNAGenericIonsManager::Instance();
    dnaIons->GetIon("alpha++");
    dnaIons->GetIon("alpha+");
    dnaIons->GetIon("helium");
    dnaIons->GetIon("hydrogen");
}

void PhysicsList::ConstructProcess()
{
    // Вызывается строго во время /run/initialize после всех PreInit-команд макроса.
    G4EmParameters::Instance()->SetLowestElectronEnergy(fLowestElectronEnergy);
    G4ProductionCutsTable::GetProductionCutsTable()->SetEnergyRange(100.0 * eV, 100.0 * GeV);

    G4VModularPhysicsList::ConstructProcess();

    if (fUseDNAActivator && fDnaActivator) {
        fDnaActivator->ConstructProcess();
    }

    if (fUseHadronic && fHadName != "none") {
        for (auto& ctor : fHadronicConstructors) {
            if (ctor) ctor->ConstructProcess();
        }
    }
}