//
// ********************************************************************
// * License and Disclaimer                                           *
// *                                                                  *
// * The  Geant4 software  is  copyright of the Copyright Holders  of *
// * the Geant4 Collaboration.  It is provided  under  the terms  and *
// * conditions of the Geant4 Software License,  included in the file *
// * LICENSE and available at  http://cern.ch/geant4/license .  These *
// * include a list of copyright holders.                             *
// *                                                                  *
// * Neither the authors of this software system, nor their employing *
// * institutes,nor the agencies providing financial support for this *
// * work  make  any representation or  warranty, express or implied, *
// * regarding  this  software system or assume any liability for its *
// * use.  Please see the license in the file  LICENSE  and URL above *
// * for the full disclaimer and the limitation of liability.         *
// *                                                                  *
// * This  code  implementation is the result of  the  scientific and *
// * technical work of the GEANT4 collaboration.                      *
// * By using,  copying,  modifying or  distributing the software (or *
// * any work based  on the software)  you  agree  to acknowledge its *
// * use  in  resulting  scientific  publications,  and indicate your *
// * acceptance of all terms of the Geant4 Software license.          *
// ********************************************************************
//
/// \file RunAction.cc
/// \brief Implementation of the RunAction class

// This example is provided by the Geant4-DNA collaboration
// Any report or published results obtained using the Geant4-DNA software
// shall cite the following Geant4-DNA collaboration publications:
// Med. Phys. 51 (2024) 5873-5889
// Med. Phys. 45 (2018) e722-e739
// Phys. Med. 31 (2015) 861-874
// Med. Phys. 37 (2010) 4692-4708
// Int. J. Model. Simul. Sci. Comput. 1 (2010) 157–178
//
// The Geant4-DNA web site is available at http://geant4-dna.org
//

#include "RunAction.hh"
#include "Run.hh"
#include "PrimaryGeneratorAction.hh"
#include "DetectorConstruction.hh"
#include "PhysicsList.hh"

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
G4String RunAction::fFinalFileName = "";
G4Mutex RunAction::fMutex = G4MUTEX_INITIALIZER;

RunAction::RunAction(DetectorConstruction* det)
:fMyDetectorConstruction(det)
{
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
{}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4Run* RunAction::GenerateRun()
{
  fRun = new Run(fMyDetectorConstruction);
  return fRun;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void RunAction::BeginOfRunAction(const G4Run*)
{
  CreateRootFile();
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
    ChangeRootName();
  }
};

void RunAction::ChangeRootName() const
{ 
  auto analysisManager = G4AnalysisManager::Instance();
  G4AutoLock lock(&fMutex);
  if (fFinalFileName != "") 
  {
      G4String tempName = folder_name + "/" + "temp_running_file.root";
      G4String finalName = fFinalFileName + ".root";

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

void RunAction::CreateRootFile()
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
                  
                  fFinalFileName = folder_name + "/RDD_" + partName + "_" + enName  + "_" + physName;
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
      analysisManager->OpenFile(folder_name + "/" + "temp_running_file");
  }
}