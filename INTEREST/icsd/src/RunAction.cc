#include "RunAction.hh"
#include "PrimaryGeneratorAction.hh"
#include "PhysicsList.hh"
#include "G4VUserPhysicsList.hh"
#include "G4RunManager.hh"
#include "G4AnalysisManager.hh"
#include "G4ParticleGun.hh"
#include "G4UnitsTable.hh"
#include "G4AutoLock.hh"
#include "G4EmParameters.hh"
#include <filesystem>
#include <algorithm> // Для std::replace

// Инициализация статических переменных
G4String RunAction::fFinalFileName = "";
G4Mutex RunAction::fMutex = G4MUTEX_INITIALIZER;

RunAction::RunAction() : G4UserRunAction()
{
  auto analysisManager = G4AnalysisManager::Instance();
  folder_name = "root";

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
}

RunAction::~RunAction() {}

void RunAction::BeginOfRunAction(const G4Run* run)
{
  auto analysisManager = G4AnalysisManager::Instance();
  G4String physName = "Unknown";
  const G4VUserPhysicsList* constPhysList = pRunManager->GetUserPhysicsList();
  
  if (constPhysList) {
      const PhysicsList* physList = dynamic_cast<const PhysicsList*>(constPhysList);
      if (physList) {
          G4String physicsListName = physList->GetPhysicsListName();    
          if (physicsListName == "G4EmDNAPhysics") physName = "DNA";
          else if (physicsListName == "G4EmDNAPhysics_option2") physName = "DNA2";
          else if (physicsListName == "G4EmDNAPhysics_option4") physName = "DNA4";
          else if (physicsListName == "G4EmDNAPhysics_option6") physName = "DNA6";
          else if (physicsListName == "G4EmDNAPhysics_option8") physName = "DNA8";
          else if (physicsListName == "G4EmStandardPhysics_option4") physName = "S4";
          else physName = "No_option";
      }
  }

  if (!G4Threading::IsMasterThread()) 
  {
      const auto* pBaseGen = pRunManager->GetUserPrimaryGeneratorAction();
      const auto* pMyGen = dynamic_cast<const PrimaryGeneratorAction*>(pBaseGen);
        
      if (pMyGen) {
          const G4ParticleGun* pGun = pMyGen->GetParticleGun();
          if (pGun) {
              G4AutoLock lock(&fMutex);
              if (fFinalFileName == "") {
                  G4String partName = pGun->GetParticleDefinition()->GetParticleName();
                  G4double energy = pGun->GetParticleEnergy();
                  G4String enName = G4BestUnit(energy, "Energy");
                  std::replace(enName.begin(), enName.end(), ' ', '-');
                  
                  fFinalFileName = folder_name + "/ICSD_" + partName + "_" + enName  + "_" + physName;
              }
          }
      }
  }

  if (G4Threading::IsMasterThread()) 
  {
      G4AutoLock lock(&fMutex);
      fFinalFileName = ""; 
      
      if (!std::filesystem::exists(folder_name)){
            std::filesystem::create_directory(folder_name);
      }
  }
  analysisManager->OpenFile(folder_name + "/" + "temp_running_file");
}

void RunAction::EndOfRunAction(const G4Run*)
{
  G4EmParameters* pDNAParams = G4EmParameters::Instance();
  G4bool isMultipleIonisation = pDNAParams->DNAMultipleIonisation();
  auto analysisManager = G4AnalysisManager::Instance();
  
  analysisManager->Write();
  analysisManager->CloseFile();

  if (G4Threading::IsMasterThread()) 
  {
      G4AutoLock lock(&fMutex);
      
      if (fFinalFileName != "") 
      {
          G4String tempName = folder_name + "/" + "temp_running_file.root";
          //G4String finalName = fFinalFileName + ".root";
          G4String finalName = fFinalFileName;
          if (isMultipleIonisation) finalName += "_MultiIoni.root";
          else finalName += ".root";

          G4cout << ">>> Renaming " << tempName << " to " << finalName << G4endl;

          try {
              if (std::filesystem::exists(finalName.c_str())) {
                  std::filesystem::remove(finalName.c_str());
              }
              
              if (std::filesystem::exists(tempName.c_str())) {
                  std::filesystem::rename(tempName.c_str(), finalName.c_str());
              } else {
                  G4cerr << ">>> Error: Temporary file not found: " << tempName << G4endl;
              }
          } catch (const std::filesystem::filesystem_error& e) {
              G4cerr << ">>> Filesystem error: " << e.what() << G4endl;
          }
      }
      else 
      {
          G4cout << ">>> Warning: Could not determine particle name (PrimaryGenerator not found or Run empty), keeping temp filename." << G4endl;
      }
  }
}
