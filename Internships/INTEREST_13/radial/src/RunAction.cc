#include "RunAction.hh"
#include "Run.hh"
#include "PrimaryGeneratorAction.hh"
#include "DetectorConstruction.hh"
#include "PhysicsList.hh"
#include "RunMessenger.hh"

#include "G4AnalysisManager.hh"
#include "G4Run.hh"
#include "G4RunManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4VUserPhysicsList.hh"
#include "G4ParticleGun.hh"
#include "G4UnitsTable.hh"
#include "G4AutoLock.hh"
#include "G4Tubs.hh"

#include <algorithm>
#include <filesystem>

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
RunAction::RunAction(DetectorConstruction* det)
:fMyDetectorConstruction(det), fFileName("output")
{
  fpRunMessenger = new RunMessenger(this);
  if (isMaster)
  {
    G4cout << "##### Create analysis manager " << "  " << this << G4endl;
    auto analysisManager = G4AnalysisManager::Instance();

    analysisManager->SetDefaultFileType("root");
    analysisManager->SetFirstNtupleId(1);

    // Create ntuple
    analysisManager->CreateNtuple("radial", "radial");
    analysisManager->CreateNtupleDColumn("radius");
    analysisManager->CreateNtupleDColumn("dose");
    analysisManager->CreateNtupleDColumn("rmse");
    analysisManager->FinishNtuple();
    analysisManager->CreateNtuple("LET", "LET");
    analysisManager->CreateNtupleDColumn("mean");
    analysisManager->CreateNtupleDColumn("rmse");
    analysisManager->CreateNtupleIColumn("events");
    analysisManager->FinishNtuple();
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

RunAction::~RunAction()
{
  delete fpRunMessenger;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4Run* RunAction::GenerateRun()
{
  fRun = new Run(fMyDetectorConstruction);
  return fRun;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void RunAction::BeginOfRunAction(const G4Run*)
{
  auto analysisManager = G4AnalysisManager::Instance();

  std::filesystem::path filePath(fFileName.c_str());
  if (filePath.has_parent_path()){
      std::filesystem::create_directories(filePath.parent_path());
  }

  analysisManager->SetFileName(fFileName);
  analysisManager->OpenFile();
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void RunAction::EndOfRunAction(const G4Run* run)
{
  if (isMaster)
  {
    // Display results from merged local runs
     const Run* myRun = static_cast<const Run*>(run);
  
    // Если по какой-то причине приведение не удалось, выходим
    if (!myRun) return;
    myRun->EndOfRun();

    // Fill ntuple
    auto analysisManager = G4AnalysisManager::Instance();

    G4double cumulatedDeposit = 0;
    G4double doseRMSE = 0;

    // Loop on cylinders and collect dose from merged local runs
    G4int nbCyl = fMyDetectorConstruction->GetCylinderNumber();

    for (G4int i = 0; i < nbCyl ; i++)
    {
      cumulatedDeposit = myRun->GetCylDoseDeposit(i);
      doseRMSE = myRun->GetRMSEDose(i);

      G4LogicalVolume* logicCylinder = fMyDetectorConstruction->GetCylinderLogical(i);
      G4Tubs* solidCylinder = static_cast<G4Tubs*>(logicCylinder->GetSolid());
      if (solidCylinder){
        G4double rIn = solidCylinder->GetInnerRadius();
        G4double rOut = solidCylinder->GetOuterRadius();
        G4double rPoint = std::sqrt(rIn*rOut);
        if (cumulatedDeposit > 0.)
        {
          analysisManager->FillNtupleDColumn
            (1,0,rPoint/nm);
          analysisManager->FillNtupleDColumn
            (1,1,cumulatedDeposit/myRun->GetNumberOfEvent()/gray);
            analysisManager->FillNtupleDColumn
            (1,2,doseRMSE/gray);
          analysisManager->AddNtupleRow(1);
        }
      }
    }
    
    // Collect LET values
    analysisManager->FillNtupleDColumn(2, 0, myRun->GetMeanLET()/(keV/um));
    analysisManager->FillNtupleDColumn(2, 1, myRun->GetRMSELET()/(keV/um));
    analysisManager->FillNtupleIColumn(2, 2, myRun->GetEventCount());
    analysisManager->AddNtupleRow(2);

    // Save histograms
    analysisManager->Write();
    analysisManager->CloseFile();
    analysisManager->Clear();
  }
}; 