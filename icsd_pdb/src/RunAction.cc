#include "RunAction.hh"
#include "RunMessenger.hh"

#include "G4RunManager.hh"
#include "G4AnalysisManager.hh"
#include "G4ParticleGun.hh"
#include "G4UnitsTable.hh"
#include "G4EmParameters.hh"

#include <filesystem>

RunAction::RunAction() : G4UserRunAction(), fFileName("output"){
    fpMessenger = new RunMessenger(this);
        
    auto analysisManager = G4AnalysisManager::Instance();

    analysisManager->SetDefaultFileType("root");
    analysisManager->SetNtupleMerging(true); 
    analysisManager->SetVerboseLevel(1);

    // --- Ntuple 1 ---
    analysisManager->SetFirstNtupleId(1);
    analysisManager->CreateNtuple("ntuple_1", "ICSD");
    analysisManager->CreateNtupleDColumn(1, "ionisations");
    analysisManager->CreateNtupleDColumn(1, "ionIonisations");
    analysisManager->CreateNtupleDColumn(1, "electronIonisations");
    analysisManager->CreateNtupleDColumn(1, "multipleIonisations");
    analysisManager->FinishNtuple(1);

    // --- Ntuple 2 ---
    analysisManager->CreateNtuple("ntuple_2", "physics");
    analysisManager->CreateNtupleDColumn(2, "EventID");
    analysisManager->CreateNtupleDColumn(2, "flagProcess");
    analysisManager->CreateNtupleDColumn(2, "x");
    analysisManager->CreateNtupleDColumn(2, "y");
    analysisManager->CreateNtupleDColumn(2, "z");
    analysisManager->CreateNtupleDColumn(2, "totalEnergyDeposit");
    analysisManager->FinishNtuple(2);

    // --- Ntuple 3 ---
    analysisManager->CreateNtuple("ntuple_3", "multipleionisations");
    analysisManager->CreateNtupleDColumn(3, "multipleIonisations");
    analysisManager->FinishNtuple(3);

    analysisManager->SetActivation(true);
}

RunAction::~RunAction(){
    delete fpMessenger;
}

void RunAction::BeginOfRunAction(const G4Run* run){
    auto analysisManager = G4AnalysisManager::Instance();
    
    std::filesystem::path filePath(fFileName.c_str());
    if (filePath.has_parent_path()){
        std::filesystem::create_directories(filePath.parent_path());
    }

    analysisManager->SetFileName(fFileName);
    analysisManager->OpenFile();
}

void RunAction::EndOfRunAction(const G4Run*){
    //G4EmParameters* pDNAParams = G4EmParameters::Instance();
    //G4bool isMultipleIonisation = pDNAParams->DNAMultipleIonisation();
    auto analysisManager = G4AnalysisManager::Instance();
    
    analysisManager->Write();
    analysisManager->CloseFile();
}
