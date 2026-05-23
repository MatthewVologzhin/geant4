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
/// \file Run.cc
/// \brief Implementation of the Run class

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

#include "Run.hh"
#include "DetectorConstruction.hh"

#include "G4RunManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4UnitsTable.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

Run::Run(DetectorConstruction* det)
:fMyDetectorConstruction(det),
fSumLET(0.),
fSumLET2(0.),
fEventCount(0)
{
  G4int nbCyl = fMyDetectorConstruction->GetCylinderNumber();
  fCylDoseDeposit = new G4double[nbCyl];
  fCylDoseDeposit2 = new G4double[nbCyl];
  fCurrentEventDose = new G4double[nbCyl];
  for (G4int i=0; i<nbCyl; ++i){
    fCylDoseDeposit[i] = 0;
    fCylDoseDeposit2[i] = 0;
    fCurrentEventDose[i] = 0;
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

Run::~Run()
{
  delete[] fCylDoseDeposit;
  delete[] fCylDoseDeposit2;
  delete[] fCurrentEventDose;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void Run::Merge(const G4Run* run)
{
  const Run* localRun = static_cast<const Run*>(run);
  fSumLET += localRun->fSumLET;
  fSumLET2 += localRun->fSumLET2;
  fEventCount += localRun->fEventCount;

  G4int nbCyl = fMyDetectorConstruction->GetCylinderNumber();
  for (G4int i=0; i<nbCyl; ++i){
    fCylDoseDeposit[i]  += localRun->fCylDoseDeposit[i];
    fCylDoseDeposit2[i] += localRun->fCylDoseDeposit2[i];
  }

  G4Run::Merge(run);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void Run::AddEventData(G4double length, G4double edep){
  //double detectorLength = static_cast<const DetectorConstruction*>(G4RunManager::GetRunManager()->GetUserDetectorConstruction())->GetCylinderLength();
  double LET = edep/length;
  fSumLET += LET;
  fSumLET2 += LET * LET;
  ++fEventCount; 
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4double Run::GetMeanLET() const {
  return (fEventCount > 0) ? fSumLET/fEventCount : 0;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4double Run::GetRMSELET() const {
  if (fEventCount < 2) return 0;
  G4double meanLET = GetMeanLET();
  G4double variance = (fSumLET2 / fEventCount) - meanLET * meanLET;
  if (variance < 0.) return 0;
  return std::sqrt(variance*fEventCount/(fEventCount - 1));
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4double Run::GetRMSEDose(G4int i) const {
  G4int n = this->GetNumberOfEvent();
  if (n<2) return 0.;
  G4double sumDose = fCylDoseDeposit[i];
  G4double sumDose2 = fCylDoseDeposit2[i];
  G4double mean = sumDose/n;
  G4double variance = (sumDose2/n) - mean * mean;
  if (variance < 0.) return 0.;
  return std::sqrt(variance*n/(n-1));
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

G4int Run::GetEventCount() const{
  return fEventCount;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void Run::RecordEndOfEvent() {
  G4int nbCyl = fMyDetectorConstruction->GetCylinderNumber();
  for (G4int i=0; i<nbCyl; ++i) {
    if (fCurrentEventDose[i] > 0) {
      fCylDoseDeposit[i] += fCurrentEventDose[i];
      fCylDoseDeposit2[i] += fCurrentEventDose[i] * fCurrentEventDose[i];
      fCurrentEventDose[i] = 0; 
    }
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void Run::EndOfRun() const
{
  G4double dose=0;

  for (G4int i = 0; i < fMyDetectorConstruction->GetCylinderNumber(); i++)
    if (fCylDoseDeposit[i] > 0.) dose=dose+fCylDoseDeposit[i];

  G4cout << G4endl;
  G4cout << "- Summary --------------------------------------------------" << G4endl;
  G4cout << G4endl;
  G4cout << "  Total absorbed dose per primary (Gy)  = "
         << dose/(joule/kg) / this->GetNumberOfEvent() << G4endl;
  G4cout << G4endl;
  G4cout << "------------------------------------------------------------" << G4endl;
}
