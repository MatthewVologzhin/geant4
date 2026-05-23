#include "DetectorConstruction.hh"
#include "DetectorMessenger.hh"
#include "G4SystemOfUnits.hh"
#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4Orb.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4VisAttributes.hh"

DetectorConstruction::DetectorConstruction() : G4VUserDetectorConstruction()
{
  fWorldSize = 100 * nm; 
  fHeight = 6.1 * nm;
  fInnerRadius = 0. * nm;
  fOuterRadius = 1.15 * nm;
  fWaterDensity = 1. * g/cm3;
  fEfficiency = 1.0;
  fGeomType = "PTB";
  fDetectorMessenger = new DetectorMessenger(this);
}

void DetectorConstruction::DefineMaterials()
{
  G4NistManager* man = G4NistManager::Instance();
  
  G4String matName = "G4_WATER";

  // 1. Проверяем, существует ли уже материал с таким именем
  G4Material* existingMat = G4Material::GetMaterial(matName, false);

  if (existingMat) {
    // Если плотность уже совпадает — просто используем его
    if (std::abs(existingMat->GetDensity() - fWaterDensity) < 0.001 * g/cm3) {
      fpWaterMaterial = existingMat;
      return;
    } else {
      // Если плотность другая — переименовываем старый объект, чтобы освободить имя
      existingMat->SetName("G4_WATER_OLD_" + std::to_string(existingMat->GetDensity()/(g/cm3)));
    }
  }

  fpWaterMaterial = new G4Material(matName, fWaterDensity, 2);
  fpWaterMaterial->AddElement(man->FindOrBuildElement("H"), 2);
  fpWaterMaterial->AddElement(man->FindOrBuildElement("O"), 1);

  G4cout << "-> Created Material: " << fpWaterMaterial->GetName() 
         << " with density: " << fpWaterMaterial->GetDensity() / (g/cm3) 
         << " g/cm3" << G4endl;
}

DetectorConstruction::~DetectorConstruction()
{
  delete fDetectorMessenger;
}

void DetectorConstruction::SetDiameter(G4double val)
{
  fOuterRadius = val/2;
  G4RunManager::GetRunManager()->ReinitializeGeometry();
}

void DetectorConstruction::SetHeight(G4double val)
{
  fHeight = val;
  G4RunManager::GetRunManager()->ReinitializeGeometry();
}

void DetectorConstruction::SetEfficiency(G4double val)
{
  fEfficiency = val;
  G4RunManager::GetRunManager()->ReinitializeGeometry();
}

void DetectorConstruction::SetDensity(G4double val)
{
  fWaterDensity = val;
  G4RunManager::GetRunManager()->ReinitializeGeometry();
}


G4VPhysicalVolume* DetectorConstruction::Construct()
{
  DefineMaterials();
  return ConstructDetector();
}

G4VPhysicalVolume* DetectorConstruction::ConstructDetector()
{
  // 1. World creation
  G4Box* solidWorld = new G4Box("World", fWorldSize/2, fWorldSize/2, fWorldSize/2);
  fpLogicWorld = new G4LogicalVolume(solidWorld, fpWaterMaterial, "World");
  fpPhysiWorld = new G4PVPlacement(0, G4ThreeVector(), "World", fpLogicWorld, 0, false, 0);

  // 2. Target configuration
  G4VSolid* solidTarget = nullptr;

  if (fGeomType == "Histone" || fGeomType == "Ribosome") {
    // Spherical targets
    solidTarget = new G4Orb("Target", fOuterRadius);
  } 
  else {
    // Cylinder targets (PTB, StarTrack, Cytoskeleton, NMDA)
    solidTarget = new G4Tubs("Target", fInnerRadius, fOuterRadius, fHeight/2., 0, 360*degree);
  }

  G4LogicalVolume* logicTarget = new G4LogicalVolume(solidTarget, fpWaterMaterial, "Target");
  new G4PVPlacement(0, G4ThreeVector(), "Target", logicTarget, fpPhysiWorld, false, 0);

  // Visualization
  G4VisAttributes* visTarget = new G4VisAttributes(G4Colour(1.0, 0.0, 0.0));
  visTarget->SetForceSolid(true);
  logicTarget->SetVisAttributes(visTarget);

  return fpPhysiWorld;
}

void DetectorConstruction::SetGeometry(const G4String& name)
{
  fGeomType = name;

  if (fGeomType == "PTB") {
    fInnerRadius = 0. * nm;
    fOuterRadius = 1.15 * nm; // D=2.3
    fHeight = 6.1 * nm;
    fEfficiency = 1.0;
    fWaterDensity = 1.0 * g/cm3;
    G4cout << "-> Geometry set to: " << fGeomType << " (Eff=" << fEfficiency << ")" << G4endl;
  } 
  else if (fGeomType == "StarTrack") {
    fInnerRadius = 0. * nm;
    fOuterRadius = 10. * nm; // D=20
    fHeight = 20.0 * nm;
    fEfficiency = 0.20;
    fWaterDensity = 1.0 * g/cm3;
    G4cout << "-> Geometry set to: " << fGeomType << " (Eff=" << fEfficiency << ")" << G4endl;
  }
  else if (fGeomType == "Cytoskeleton") {
    fInnerRadius = 7.5 * nm;
    fOuterRadius = 12.5 * nm;
    fHeight = 8. * nm;
    fEfficiency = 1.0;
    fWaterDensity = 1.407 * g/cm3;
    G4cout << "-> Geometry set to: " << fGeomType 
         << " (Target Density=" << fWaterDensity / (g/cm3) << " g/cm3)" << G4endl;
  }
  else if (fGeomType == "NMDA") {
    fInnerRadius = 0. * nm;
    fOuterRadius = 4. * nm;
    fHeight = 18. * nm;
    fEfficiency = 1.0;
    fWaterDensity = 1.407 * g/cm3;
    G4cout << "-> Geometry set to: " << fGeomType 
         << " (Target Density=" << fWaterDensity / (g/cm3) << " g/cm3)" << G4endl;
  }
  else if (fGeomType == "Histone") {
    fInnerRadius = 0. * nm;
    fOuterRadius = 2.5 * nm;
    fHeight = 5.0 * nm;      
    fEfficiency = 1.0;
    fWaterDensity = 1.407 * g/cm3;
    G4cout << "-> Geometry set to: " << fGeomType 
         << " (Target Density=" << fWaterDensity / (g/cm3) << " g/cm3)" << G4endl;
  }
  else if (fGeomType == "Ribosome") {
    fInnerRadius = 0. * nm;
    fOuterRadius = 12.5 * nm;
    fHeight = 25.0 * nm;
    fEfficiency = 1.0;
    fWaterDensity = 1.407 * g/cm3;
     G4cout << "-> Geometry set to: " << fGeomType 
         << " (Target Density=" << fWaterDensity / (g/cm3) << " g/cm3)" << G4endl;
  }
  G4RunManager::GetRunManager()->ReinitializeGeometry();
}
