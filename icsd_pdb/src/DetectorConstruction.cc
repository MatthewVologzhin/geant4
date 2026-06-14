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
#include "G4Tokenizer.hh"
#include "G4StateManager.hh"

#include <fstream>

struct AtomData {
    G4ThreeVector pos;
    G4double radius;
};

DetectorConstruction::DetectorConstruction() : G4VUserDetectorConstruction()
{
  fWorldSize = 100 * nm; 
  fHeight = 6.1 * nm;
  fInnerRadius = 0. * nm;
  fOuterRadius = 1.15 * nm;
  fWaterDensity = 1. * g/cm3;
  fEfficiency = 1.0;
  fVoxelSize = 5. * angstrom; 
  fGeomType = "PTB";
  fDetectorMessenger = new DetectorMessenger(this);
  LoadRadii();
}

void DetectorConstruction::DefineMaterials()
{
  G4NistManager* man = G4NistManager::Instance();
  fpWorldMaterial = man->FindOrBuildMaterial("G4_WATER");
  
  if (std::abs(fWaterDensity - 1.0 * g/cm3) > 1e-4 * g/cm3) {
    G4String matName = "G4_WATER_DENSE_" + std::to_string(fWaterDensity / (g/cm3));  
    fpWaterMaterial = G4Material::GetMaterial(matName, false);
    if (!fpWaterMaterial) {
      fpWaterMaterial = man->BuildMaterialWithNewDensity(matName, "G4_WATER", fWaterDensity);
    }
  } else {
    fpWaterMaterial = fpWorldMaterial;
  }
}

DetectorConstruction::~DetectorConstruction()
{
  delete fDetectorMessenger;
}

void DetectorConstruction::SetDiameter(G4double val)
{
  fOuterRadius = val/2;
  if (G4StateManager::GetStateManager()->GetCurrentState() != G4State_PreInit){
	  G4RunManager::GetRunManager()->ReinitializeGeometry();
  }
}

void DetectorConstruction::SetVoxelSize(G4double val)
{
  fVoxelSize = val/2;
  if (G4StateManager::GetStateManager()->GetCurrentState() != G4State_PreInit){
	  G4RunManager::GetRunManager()->ReinitializeGeometry();
  }
}

void DetectorConstruction::SetHeight(G4double val)
{
  fHeight = val;
  if (G4StateManager::GetStateManager()->GetCurrentState() != G4State_PreInit){
	  G4RunManager::GetRunManager()->ReinitializeGeometry();
  }
}

void DetectorConstruction::SetEfficiency(G4double val)
{
  fEfficiency = val;
  if (G4StateManager::GetStateManager()->GetCurrentState() != G4State_PreInit){
	  G4RunManager::GetRunManager()->ReinitializeGeometry();
  }
}

void DetectorConstruction::SetDensity(G4double val)
{
  fWaterDensity = val;
  if (G4StateManager::GetStateManager()->GetCurrentState() != G4State_PreInit){
	  G4RunManager::GetRunManager()->ReinitializeGeometry();
  }
}

void DetectorConstruction::LoadRadii()
{
    // Alvarez, S. (2013). A cartography of the van der Waals territories. 
    // Dalton Transactions, 42(24), 8617-8636.
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
    
    fRadiiMap["FE"] = 2.44 * angstrom;
    fRadiiMap["CO"] = 2.40 * angstrom;
    fRadiiMap["NI"] = 2.40 * angstrom;
    fRadiiMap["CU"] = 2.38 * angstrom;
    fRadiiMap["ZN"] = 2.39 * angstrom;
    fRadiiMap["AG"] = 2.53 * angstrom;
    fRadiiMap["AU"] = 2.32 * angstrom;
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
  fpPhysiWorld = new G4PVPlacement(0, G4ThreeVector(), fpLogicWorld, "World", 0, false, 0, true);
  
  
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
  else {
    G4String path_csv = "results/data/" + fGeomType + ".csv";
    std::ifstream in(path_csv);
    if (!in.is_open()) {
      G4Exception("DetectorConstruction::ConstructDetector", "FileNotFound",
                  FatalException, ("Could not open file: " + path_csv).c_str());
    }

    G4String line, element;
    G4double x, y, z;
    
    std::vector<AtomData> atoms;
    auto radiiEnd = fRadiiMap.end();

    // 1. Считываем все атомы в вектор в памяти
    G4double minX = kInfinity, maxX = -kInfinity;
    G4double minY = kInfinity, maxY = -kInfinity;
    G4double minZ = kInfinity, maxZ = -kInfinity;

    while (std::getline(in, line)) {
      G4Tokenizer token(line);
      element = token(",");
      if (element == "element") continue;

      auto radiusIt = fRadiiMap.find(element);
      if (radiusIt == radiiEnd) continue;

      try {
        x = std::stod(token(",")) * angstrom;
        y = std::stod(token(",")) * angstrom;
        z = std::stod(token(",")) * angstrom;
      } catch (const std::exception& e) {
        continue;
      }

      G4double r = radiusIt->second;
      atoms.push_back({G4ThreeVector(x, y, z), r});

      // Рассчитываем габариты всей молекулы
      minX = std::min(minX, x - r);
      maxX = std::max(maxX, x + r);
      minY = std::min(minY, y - r);
      maxY = std::max(maxY, y + r);
      minZ = std::min(minZ, z - r);
      maxZ = std::max(maxZ, z + r);
    }
    in.close();
    
    // Создаем единственный логический объем для вокселя
    G4Box* pSolidVoxel = new G4Box("solid_Voxel", fVoxelSize/2., fVoxelSize/2., fVoxelSize/2.);
    G4LogicalVolume* pLogicVoxel = new G4LogicalVolume(pSolidVoxel, fpWaterMaterial, "logic_Voxel");
    
    G4VisAttributes* pVisVoxel = new G4VisAttributes(G4Colour(1.0, 0.0, 0.0));
    pVisVoxel->SetForceSolid(true);
    pLogicVoxel->SetVisAttributes(pVisVoxel);

    // 3. Заполняем 3D сетку
    G4int voxelCount = 0;
    
    for (G4double vx = minX + fVoxelSize/2.; vx < maxX; vx += fVoxelSize) {
        for (G4double vy = minY + fVoxelSize/2.; vy < maxY; vy += fVoxelSize) {
            for (G4double vz = minZ + fVoxelSize/2.; vz < maxZ; vz += fVoxelSize) {
                
                G4ThreeVector voxelPos(vx, vy, vz);
                G4bool isInsideBiomolecule = false;

                // Проверяем, находится ли центр данного вокселя внутри хотя бы одного атома
                for (const auto& atom : atoms) {
                    G4double dx = voxelPos.x() - atom.pos.x();
                    G4double dy = voxelPos.y() - atom.pos.y();
                    G4double dz = voxelPos.z() - atom.pos.z();
                    G4double dist2 = dx*dx + dy*dy + dz*dz;

                    if (dist2 < atom.radius * atom.radius) {
                        isInsideBiomolecule = true;
                        break;
                    }
                }

                // Если воксель внутри структуры — размещаем его
                if (isInsideBiomolecule) {
                    // Размещение регулярной сетки неперекрывающихся кубов оптимизируется навигатором Geant4 автоматически
                    new G4PVPlacement(0, voxelPos, pLogicVoxel, "Target", fpLogicWorld, false, voxelCount++, false);
                }
            }
        }
    }
    
    G4cout << "-> Voxel phantom built. Placed " << voxelCount << " voxels of size " << fVoxelSize/nm << " nm" << G4endl;
    pLogicTarget = nullptr; // Объемы уже размещены напрямую в fpLogicWorld
  }

  if (!pLogicTarget && pSolidTarget) {
    pLogicTarget = new G4LogicalVolume(pSolidTarget, fpWaterMaterial, "Target");
  }

  if (pLogicTarget) {
    new G4PVPlacement(0, G4ThreeVector(), pLogicTarget, "Target", fpLogicWorld, false, 0);
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
  else {
	if (std::abs(fWaterDensity - 1.0 * g/cm3) < 1e-4 * g/cm3) {
      fWaterDensity = 1.407 * g/cm3;
    }
    fEfficiency = 1.0;
    G4cout << "-> Geometry set to: " << fGeomType 
           << " (Loading from CSV, Target Density=" << fWaterDensity / (g/cm3) << " g/cm3)" << G4endl;
  }
  
  if (G4StateManager::GetStateManager()->GetCurrentState() != G4State_PreInit){
	  G4RunManager::GetRunManager()->ReinitializeGeometry();
  }
}
