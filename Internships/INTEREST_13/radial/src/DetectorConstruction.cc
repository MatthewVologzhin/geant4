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
/// \file DetectorConstruction.cc
/// \brief Implementation of the DetectorConstruction class

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

#include "DetectorConstruction.hh"
#include "DetectorMessenger.hh"

#include "G4RunManager.hh"
#include "G4SystemOfUnits.hh"
#include "G4VisAttributes.hh"
#include "G4Tubs.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

DetectorConstruction::DetectorConstruction(PhysicsList* pl)
{
  // Create commands for interactive definition of the detector
  fDetectorMessenger = new DetectorMessenger(this, pl);

  // World size
  fWorldRadius = 1*um;
  fWorldOffsetLength = 0;

  // Cylinders
  fCylinderLength = 10*um;
  fCylinderMinRadius = 1e-1*nm;
  fCylinderNumber = 20;

  // Materials
  G4NistManager* man = G4NistManager::Instance();
  G4Material* H2O = man->FindOrBuildMaterial("G4_WATER");
  G4Material* galactic = man->FindOrBuildMaterial("G4_Galactic");

  fWaterMaterial = H2O;
  fVacuumMaterial = galactic;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

DetectorConstruction::~DetectorConstruction()
{
  delete fDetectorMessenger;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

void DetectorConstruction::DefineMaterials()
{
  // Water is defined from NIST material database
  G4NistManager* man = G4NistManager::Instance();

  G4Material* H2O = man->FindOrBuildMaterial("G4_WATER");
  G4Material* galactic = man->FindOrBuildMaterial("G4_Galactic");

  fWaterMaterial = H2O;
  fVacuumMaterial = galactic;

  G4cout << *(G4Material::GetMaterialTable()) << G4endl;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

G4VPhysicalVolume* DetectorConstruction::Construct()
{
  if (fPhysiWorld) return fPhysiWorld;

  // World volume

  G4Tubs* solidWorld =
    new G4Tubs("World", 0., fWorldRadius, fWorldOffsetLength + (fCylinderLength / 2), 0., 360*deg);

  fLogicWorld = new G4LogicalVolume(solidWorld,  // its solid
                                    fWaterMaterial,  // its material
                                    "World");  // its name

  fPhysiWorld = new G4PVPlacement(0,  // no rotation
                                  G4ThreeVector(),  // at (0,0,0)
                                  "World",  // its name
                                  fLogicWorld,  // its logical volume
                                  0,  // its mother volume
                                  false,  // no boolean operation
                                  0);  // copy number

  // Visualization attributes - white
  G4VisAttributes* worldVisAtt = new G4VisAttributes(G4Colour(1.0, 1.0, 1.0));
  worldVisAtt->SetVisibility(true);
  fLogicWorld->SetVisAttributes(worldVisAtt);

  G4cout << G4endl;
  G4cout
    << "*******************************************************************"
    << G4endl;
  G4cout << "*** NUMBER OF HOLLOW CYLINDERS = "<< fCylinderNumber << G4endl;
  G4cout
    << "*******************************************************************"
    << G4endl;
  G4cout << G4endl;

  fLogicCylinders.clear(); // Очистить, если вызывается повторно
  G4double rIn; G4double rOut;
  for (G4int i = 0; i < fCylinderNumber; i++) {
    if (fCylinderLog == true){
      rIn = fCylinderMinRadius*std::pow(fWorldRadius/fCylinderMinRadius, (G4double)i/fCylinderNumber);
      rOut = fCylinderMinRadius*std::pow(fWorldRadius/fCylinderMinRadius, (G4double)(i+1)/fCylinderNumber);
    } else {
      rIn = (fWorldRadius-fCylinderMinRadius)/fCylinderNumber*i;
      rOut = (fWorldRadius-fCylinderMinRadius)/fCylinderNumber*(i+1);
    }

    G4Tubs* solidCylinder =
      new G4Tubs("Cylinder", rIn, rOut, fCylinderLength / 2, 0., 360*deg);

    G4LogicalVolume* logCyl = new G4LogicalVolume(solidCylinder, fWaterMaterial, "Cylinder");
    fLogicCylinders.push_back(logCyl);


    new G4PVPlacement(nullptr, G4ThreeVector(0, 0, 0), fLogicCylinders[i],
      "Cylinder", fLogicWorld, false, i, true);

    fLogicCylinders[i]->SetVisAttributes(worldVisAtt);
  }

  // Shows how to introduce a 20 eV tracking cut
  // logicWorld->SetUserLimits(new G4UserLimits(DBL_MAX,DBL_MAX,DBL_MAX,20*eV));

  return fPhysiWorld;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void DetectorConstruction::SetMaterial(const G4String& materialChoice)
{
  G4Material* pttoMaterial =
    G4NistManager::Instance()->FindOrBuildMaterial(materialChoice);

  if (pttoMaterial) {
    fWaterMaterial = pttoMaterial;
    if (fLogicWorld) fLogicWorld->SetMaterial(fWaterMaterial);
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void DetectorConstruction::SetWorldRadius(const G4double& value)
{
  fWorldRadius = value;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void DetectorConstruction::SetCylinderLength(const G4double& value)
{
  fCylinderLength = value;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void DetectorConstruction::SetWorldOffsetLength(const G4double& value)
{
  fWorldOffsetLength = value;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void DetectorConstruction::SetCylinderNumber(const G4int& value)
{
  fCylinderNumber = value;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void DetectorConstruction::SetCylinderMinRadius(const G4double& value)
{
  fCylinderMinRadius = value;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void DetectorConstruction::SetCylinderLog(const bool& value)
{
  fCylinderLog = value;
}