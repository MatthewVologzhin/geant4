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
#include "G4MultiUnion.hh"
#include "G4Tokenizer.hh"

#include <fstream>

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
  LoadRadii();
}

void DetectorConstruction::DefineMaterials()
{
  G4NistManager* man = G4NistManager::Instance();
  fpWorldMaterial = man->FindOrBuildMaterial("G4_WATER");

  // Создаем уникальное имя на основе плотности
  std::string densityStr = std::to_string(fWaterDensity / (g/cm3));
  G4String matName = "G4_WATER_" + densityStr;

  // Проверяем, существует ли уже материал с таким именем/плотностью
  fpWaterMaterial = G4Material::GetMaterial(matName, false);

  if (!fpWaterMaterial) {
    // Если это стандартная вода 1.0 г/см3, можно взять из NIST
    if (std::abs(fWaterDensity - 1.0 * g/cm3) < 1e-4 * g/cm3) {
      fpWaterMaterial = man->FindOrBuildMaterial("G4_WATER");
    } else {
      // Создаем модифицированную воду с уникальным именем
      fpWaterMaterial = new G4Material(matName, fWaterDensity, 2);
      fpWaterMaterial->AddElement(man->FindOrBuildElement("H"), 2);
      fpWaterMaterial->AddElement(man->FindOrBuildElement("O"), 1);
    }
  }
}

DetectorConstruction::~DetectorConstruction()
{
  delete fDetectorMessenger;
  // Очистка fpVisTarget удалена, так как визуализация теперь локальная
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

void DetectorConstruction::LoadRadii()
{
    // Данные взяты из Table 1 статьи: 
    // Alvarez, S. (2013). A cartography of the van der Waals territories. 
    // Dalton Transactions, 42(24), 8617-8636.

    // Основные элементы (Органика и ДНК)
    fRadiiMap["H"]  = 1.20 * angstrom;
    fRadiiMap["HE"] = 1.43 * angstrom;
    fRadiiMap["LI"] = 2.12 * angstrom;
    fRadiiMap["BE"] = 1.98 * angstrom;
    fRadiiMap["B"]  = 1.91 * angstrom;
    fRadiiMap["C"]  = 1.77 * angstrom;
    fRadiiMap["N"]  = 1.66 * angstrom;
    fRadiiMap["O"]  = 1.50 * angstrom;
    fRadiiMap["F"]  = 1.46 * angstrom;
    fRadiiMap["NE"] = 1.58 * angstrom;
    
    fRadiiMap["NA"] = 2.50 * angstrom;
    fRadiiMap["MG"] = 2.51 * angstrom;
    fRadiiMap["MN"] = 2.45 * angstrom;
    fRadiiMap["AL"] = 2.25 * angstrom;
    fRadiiMap["SI"] = 2.19 * angstrom;
    fRadiiMap["P"]  = 1.90 * angstrom;
    fRadiiMap["S"]  = 1.89 * angstrom;
    fRadiiMap["CL"] = 1.82 * angstrom;
    fRadiiMap["AR"] = 1.83 * angstrom;

    fRadiiMap["K"]  = 2.73 * angstrom;
    fRadiiMap["CA"] = 2.62 * angstrom;
    
    // Металлы (часто встречаются в подложках или мишенях)
    fRadiiMap["FE"] = 2.44 * angstrom;
    fRadiiMap["CO"] = 2.40 * angstrom;
    fRadiiMap["NI"] = 2.40 * angstrom;
    fRadiiMap["CU"] = 2.38 * angstrom;
    fRadiiMap["ZN"] = 2.39 * angstrom;
    fRadiiMap["AG"] = 2.53 * angstrom;
    fRadiiMap["AU"] = 2.32 * angstrom;
    
    // Галогены и другие
    fRadiiMap["AS"] = 1.88 * angstrom;
    fRadiiMap["SE"] = 1.82 * angstrom;
    fRadiiMap["BR"] = 1.86 * angstrom;
    fRadiiMap["I"]  = 2.04 * angstrom;

    G4cout << "-> Van der Waals radii loaded (Source: Alvarez 2013)" << G4endl;
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
  fpLogicWorld = new G4LogicalVolume(solidWorld, fpWorldMaterial, "World");
  fpPhysiWorld = new G4PVPlacement(0, G4ThreeVector(), "World", fpLogicWorld, 0, false, 0);

  // 2. Target configuration
  G4VSolid* pSolidTarget = nullptr;
  G4LogicalVolume* pLogicTarget = nullptr;
  
  if (fGeomType == "Histone" || fGeomType == "Ribosome") {
    // Spherical targets
    pSolidTarget = new G4Orb("Target", fOuterRadius);
  } 
  else if (fGeomType == "Cytoskeleton" || fGeomType == "NMDA" ||
           fGeomType == "StarTrack" || fGeomType == "PTB") {
    // Cylinder targets (PTB, StarTrack, Cytoskeleton, NMDA)
    pSolidTarget = new G4Tubs("Target", fInnerRadius, fOuterRadius, fHeight/2., 0, 360*degree);
  }
  else{
    G4String path_csv = "results/data/" + fGeomType + ".csv";
    std::ifstream in(path_csv); // Here files-csv: "1kx5"
    if (!in.is_open()) {
      G4Exception("DetectorConstruction::ConstructDetector", "FileNotFound",
                  FatalException, ("Could not open file: " + path_csv).c_str());
    }
    G4String line, element;
    G4double x, y, z;
    
    // Испольуем std::map вместо std::unordered_map для совместимости с G4String
    std::map<G4String, G4LogicalVolume*> logicElements;

    G4int copyNo = 0;
    G4LogicalVolume* pLogicOrb;
    auto radiiEnd = fRadiiMap.end();
    while (std::getline(in, line)){
      G4Tokenizer token(line);
      element = token(",");
      if (element == "element") continue;
      auto radiusIt = fRadiiMap.find(element);
      if (radiusIt == radiiEnd) {
        G4Exception("DetectorConstruction::ConstructDetector", "ElementNotFound",
                    FatalException, ("Element " + element + " not found in Radii table.").c_str());
      }
      if (copyNo % 10 == 0) G4cout << "Number of atoms: " << copyNo << G4endl;
      auto itLogic = logicElements.find(element);
      if (itLogic == logicElements.end()){
        G4Orb* pSolidOrb = new G4Orb("solid_" + element, radiusIt->second);
        pLogicOrb = new G4LogicalVolume(pSolidOrb, fpWaterMaterial, "log_" + element);
        G4VisAttributes* pVisTarget = new G4VisAttributes(G4Colour(1.0, 0.0, 0.0));
        pVisTarget->SetForceSolid(true);
        pLogicOrb->SetVisAttributes(pVisTarget);
        logicElements[element] = pLogicOrb;
      } else {
        pLogicOrb = itLogic->second;
      }
      
      try {
        x = std::stod(token(",")) * angstrom;
        y = std::stod(token(",")) * angstrom;
        z = std::stod(token(",")) * angstrom;
      } catch (const std::exception& e) {
        G4Exception("DetectorConstruction::ConstructDetector", "ParsingError",
                    FatalException, ("Failed to convert coordinates to double in line: " + line).c_str());
      }
      
      new G4PVPlacement(0, G4ThreeVector(x, y, z), pLogicOrb, "Target", fpLogicWorld, false, copyNo++, false);
    }
    in.close();
  }

  if (!pLogicTarget && pSolidTarget) {
    pLogicTarget = new G4LogicalVolume(pSolidTarget, fpWaterMaterial, "Target");
  }

  if (pLogicTarget) {
    new G4PVPlacement(0, G4ThreeVector(), "Target", pLogicTarget, fpPhysiWorld, false, 0);
    G4VisAttributes* pVisTarget = new G4VisAttributes(G4Colour(1.0, 0.0, 0.0));
    pVisTarget->SetForceSolid(true);
    pLogicTarget->SetVisAttributes(pVisTarget);
  }

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