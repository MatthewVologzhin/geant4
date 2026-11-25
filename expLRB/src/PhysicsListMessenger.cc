// PhysicsListMessenger.cc
#include "PhysicsListMessenger.hh"
#include "PhysicsList.hh"

PhysicsListMessenger::PhysicsListMessenger(PhysicsList* pPhys) : fPhysicsList(pPhys)
{
    fPhysDir = std::make_unique<G4UIdirectory>("/physics/");
    fPhysDir->SetGuidance("Custom Modular Physics configuration.");

    fHadPhysCmd = std::make_unique<G4UIcmdWithAString>("/physics/setHadronic", this);
    fHadPhysCmd->SetGuidance("Set Hadronic physics: QGSP_BIC_HP, FTFP_BERT_HP, QBBC, INCLXX_HP or 'none'");
    fHadPhysCmd->SetParameterName("HadName", false);
    fHadPhysCmd->SetCandidates("QGSP_BIC_HP FTFP_BERT_HP QBBC INCLXX_HP none off DISABLED false");
    fHadPhysCmd->AvailableForStates(G4State_PreInit);

    fEnableHadCmd = std::make_unique<G4UIcmdWithABool>("/physics/enableHadronic", this);
    fEnableHadCmd->SetGuidance("Enable or disable Hadronic physics completely.");
    fEnableHadCmd->SetParameterName("enableHad", false);
    fEnableHadCmd->SetDefaultValue(true);
    fEnableHadCmd->AvailableForStates(G4State_PreInit);

    fEmPhysCmd = std::make_unique<G4UIcmdWithAString>("/physics/setEM", this);
    fEmPhysCmd->SetGuidance("Set EM base physics: EM_Opt4, EM_Opt3, EM_Opt0, DNA_Opt0, DNA_Opt2, DNA_Opt4, DNA_Opt6, DNA_Opt8");
    fEmPhysCmd->SetParameterName("EmName", false);
    fEmPhysCmd->SetCandidates("EM_Opt4 EM_Opt3 EM_Opt0 emstandard_opt0 DNA_Opt0 DNA_Opt2 DNA_Opt4 DNA_Opt6 DNA_Opt8");
    fEmPhysCmd->AvailableForStates(G4State_PreInit);

    fDnaActivatorCmd = std::make_unique<G4UIcmdWithABool>("/physics/enableDNAActivator", this);
    fDnaActivatorCmd->SetGuidance("Enable Geant4-DNA Activator in registered target regions.");
    fDnaActivatorCmd->SetParameterName("enableDNA", false);
    fDnaActivatorCmd->SetDefaultValue(true);
    fDnaActivatorCmd->AvailableForStates(G4State_PreInit);

    fLowestElEnergyCmd = std::make_unique<G4UIcmdWithADoubleAndUnit>("/physics/setLowestElectronEnergy", this);
    fLowestElEnergyCmd->SetGuidance("Set lowest electron tracking energy (default: 7.4 eV for DNA).");
    fLowestElEnergyCmd->SetParameterName("energy", false);
    fLowestElEnergyCmd->SetUnitCategory("Energy");
    fLowestElEnergyCmd->SetDefaultUnit("eV");
    fLowestElEnergyCmd->SetRange("energy>=0.");
    fLowestElEnergyCmd->AvailableForStates(G4State_PreInit);
}

PhysicsListMessenger::~PhysicsListMessenger() = default;

void PhysicsListMessenger::SetNewValue(G4UIcommand* command, G4String newValue)
{
    if (command == fHadPhysCmd.get()) {
        fPhysicsList->SetHadronicPhysics(newValue);
    } else if (command == fEnableHadCmd.get()) {
        fPhysicsList->EnableHadronicPhysics(fEnableHadCmd->GetNewBoolValue(newValue));
    } else if (command == fEmPhysCmd.get()) {
        fPhysicsList->SetEMPhysics(newValue);
    } else if (command == fDnaActivatorCmd.get()) {
        fPhysicsList->EnableDNAActivator(fDnaActivatorCmd->GetNewBoolValue(newValue));
    } else if (command == fLowestElEnergyCmd.get()) {
        fPhysicsList->SetLowestElectronEnergy(fLowestElEnergyCmd->GetNewDoubleValue(newValue));
    }
}