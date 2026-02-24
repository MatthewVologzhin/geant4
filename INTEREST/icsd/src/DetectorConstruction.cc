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
// This example is provided by the Geant4-DNA collaboration
// Any report or published results obtained using the Geant4-DNA software
// shall cite the following Geant4-DNA collaboration publication:
// Med. Phys. 37 (2010) 4692-4708
// J. Comput. Phys. 274 (2014) 841-882
// The Geant4-DNA web site is available at http://geant4-dna.org
//
//
/// \file DetectorConstruction.cc
/// \brief Implementation of the DetectorConstruction class

#include "DetectorConstruction.hh"

#include "DetectorMessenger.hh"
#include "G4ProductionCuts.hh"
#include "G4RunManager.hh"
#include "G4SystemOfUnits.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

DetectorConstruction::DetectorConstruction() : G4VUserDetectorConstruction()
{
  fWorldSize = 1. * um; 
  fDiameter = 2.3 * nm;
  fHeight = 6.1 * nm;
  fEfficiency = 0.2;
  fGeomType = "JetCounter";
  fDetectorMessenger = new DetectorMessenger(this);
}

void DetectorConstruction::DefineMaterials()
{
  G4NistManager* man = G4NistManager::Instance();
  G4Material* H2O = man->FindOrBuildMaterial("G4_WATER");
  fpWaterMaterial = H2O;

  // Имитация Майлара через плотную воду (1.4 г/см3)
  // Это нужно для правильной работы физики Geant4-DNA внутри стенки
  G4double mylarDensity = 1.4 * g / cm3;
  fpMylarLikeWater = new G4Material("MylarLikeWater", mylarDensity, 1);
  fpMylarLikeWater->AddMaterial(H2O, 1.0);
}

DetectorConstruction::~DetectorConstruction()
{
  delete fDetectorMessenger;
}

G4VPhysicalVolume* DetectorConstruction::Construct()
{
  DefineMaterials();
  return ConstructDetector();
}

void DetectorConstruction::SetDiameter(G4double val)
{
  fDiameter = val;
  G4RunManager::GetRunManager()->GeometryHasBeenModified();
}

void DetectorConstruction::SetHeight(G4double val)
{
  fHeight = val;
  G4RunManager::GetRunManager()->GeometryHasBeenModified();
}

void DetectorConstruction::SetEfficiency(G4double val)
{
  fEfficiency = val;
}

G4VPhysicalVolume* DetectorConstruction::ConstructDetector()
{
  G4Material* worldMaterial = fpWaterMaterial;
  G4Material* targetMaterial = fpWaterMaterial;
  G4Material* wallMaterial = fpMylarLikeWater;

  // 1. Создаем Мир
  fpSolidWorld = new G4Box("World", fWorldSize / 2, fWorldSize / 2, fWorldSize / 2);
  fpLogicWorld = new G4LogicalVolume(fpSolidWorld, worldMaterial, "World");
  fpPhysiWorld = new G4PVPlacement(0, G4ThreeVector(), "World", fpLogicWorld, 0, false, 0);

  // 2. Логика построения в зависимости от типа эксперимента
  if (fGeomType == "JetCounter" || fGeomType == "dna") {
    // Толщина стенки Майлара (в эквиваленте ~150 нм достаточно для электронного равновесия)
    G4double wallThick = 150. * nm;

    // Сначала создаем Стенку (Wall)
    G4Tubs* solidWall = new G4Tubs("Wall", 0, (fDiameter/2. + wallThick), 
                                   (fHeight/2. + wallThick), 0, 360*degree);
    G4LogicalVolume* logicWall = new G4LogicalVolume(solidWall, wallMaterial, "Wall");
    
    // Помещаем Стенку в Мир
    G4VPhysicalVolume* physiWall = new G4PVPlacement(0, G4ThreeVector(), "Wall", 
                                                     logicWall, fpPhysiWorld, false, 0);

    // Помещаем Мишень (Target) ВНУТРЬ Стенки
    G4Tubs* solidTarget = new G4Tubs("Target", 0, fDiameter/2., fHeight/2., 0, 360*degree);
    G4LogicalVolume* logicTarget = new G4LogicalVolume(solidTarget, targetMaterial, "Target");
    
    new G4PVPlacement(0, G4ThreeVector(), "Target", logicTarget, physiWall, false, 0);
    
    // Визуализация
    logicWall->SetVisAttributes(new G4VisAttributes(G4Colour(0, 1, 0, 0.2))); // Зеленая прозрачная стенка
    logicTarget->SetVisAttributes(new G4VisAttributes(G4Colour(1, 0, 0)));    // Красная мишень

  } else {
    // Для StarTrack или макро-геометрии (БЕЗ стенки)
    G4Tubs* solidTarget = new G4Tubs("Target", 0, fDiameter/2., fHeight/2., 0, 360*degree);
    G4LogicalVolume* logicTarget = new G4LogicalVolume(solidTarget, targetMaterial, "Target");
    
    new G4PVPlacement(0, G4ThreeVector(), "Target", logicTarget, fpPhysiWorld, false, 0);
    
    logicTarget->SetVisAttributes(new G4VisAttributes(G4Colour(1, 0, 0)));
  }

  return fpPhysiWorld;
}

void DetectorConstruction::SetGeometry(const G4String& name)
{
  fGeomType = name;

  if (fGeomType == "JetCounter") {
    fDiameter = 2.3 * nm;
    fHeight = 6.1 * nm;
    fEfficiency = 1;
    G4cout << "-> Set to JetCounter: Wall added, D=2.3, H=3.4, Eff=0.57" << G4endl;
  } 
  else if (fGeomType == "StarTrack") {
    fDiameter = 20.0 * nm;
    fHeight = 20.0 * nm;
    fEfficiency = 0.20;
    G4cout << "-> Set to StarTrack: NO Wall, D=20, H=20, Eff=0.20" << G4endl;
  }

  G4RunManager::GetRunManager()->GeometryHasBeenModified();
}