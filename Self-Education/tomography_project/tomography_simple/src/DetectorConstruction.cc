#include "DetectorConstruction.hh"
#include "G4SystemOfUnits.hh"
#include "G4UserLimits.hh"
#include "G4NistManager.hh"
#include "G4RunManager.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"	
#include "G4VSolid.hh"
#include "G4VPhysicalVolume.hh"
#include "G4RotationMatrix.hh"
#include "G4PVPlacement.hh"
#include "G4LogicalVolume.hh"

DetectorConstruction::DetectorConstruction(){
	G4NistManager* pMan = G4NistManager::Instance();
	fTubsMaterial = pMan->FindOrBuildMaterial("G4_Al");
	fDetectorMaterial = pMan->FindOrBuildMaterial("G4_C");
	fWorldMaterial = pMan->FindOrBuildMaterial("G4_AIR");
	
	fMessenger = new G4GenericMessenger(this, "/detector/", "Detector construction");
	fMessenger->DeclareProperty("worldSizeX", 	fWorldSizeX, 		"Size of the World box along axis X");
	fMessenger->DeclareProperty("worldSizeY", 	fWorldSizeY, 		"Size of the World box along axis Y");
	fMessenger->DeclareProperty("worldSizeZ", 	fWorldSizeZ, 		"Size of the World box along axis Z");
	fMessenger->DeclareProperty("tubsRadius", 	fTubsRadius, 		"Radius of the Tube");
	fMessenger->DeclareProperty("tubsHeight", 	fTubsHeight, 		"Heigth of the Tube");
	fMessenger->DeclareProperty("detectorHeight", 	fDetectorHeight, 	"Height of each detector");
	
	fWorldSizeX = 100*CLHEP::cm;
	fWorldSizeY = 20*CLHEP::cm;
	fWorldSizeZ = 10*CLHEP::cm;
	fTubsRadius = 3.0*CLHEP::cm;
	fTubsHeight = 8.0*CLHEP::cm;
	fDetectorHeight = 0.2*CLHEP::cm;
};

G4VPhysicalVolume* DetectorConstruction::Construct(){
	G4Box* pSolidWorld = new G4Box("world", fWorldSizeX, fWorldSizeY, fWorldSizeZ);
	G4LogicalVolume* pLogicWorld = new G4LogicalVolume(pSolidWorld, fWorldMaterial, "logical world");
	G4VPhysicalVolume* pPhysWorld = new G4PVPlacement(0, G4ThreeVector(), "physical world", pLogicWorld, 0, false, 0);
	
	G4Tubs* pSolidTubs = new G4Tubs("tube", 0, fTubsRadius, fTubsHeight, 0, 360*deg);
	G4LogicalVolume* pLogicTubs = new G4LogicalVolume(pSolidTubs, fTubsMaterial, "logical tubs");
	G4RotationMatrix* pRotation = new G4RotationMatrix();
	pRotation->rotateX(90.*deg);
	pRotation->rotateY(.0*deg);
	pRotation->rotateZ(.0*deg);
	G4VPhysicalVolume* pPhysTubs = new G4PVPlacement(pRotation, G4ThreeVector(95.0*CLHEP::cm, 0, 0), pLogicTubs, "physical tube", pLogicWorld, false, 0);
		
	G4Box* pSolidDetector = new G4Box("detector", fDetectorHeight, fWorldSizeY, fWorldSizeZ);
	G4LogicalVolume* pLogicDetector = new G4LogicalVolume(pSolidDetector, fDetectorMaterial, "logical detector");	
	G4VPhysicalVolume* pPhysDetector = new G4PVPlacement(0, G4ThreeVector(fWorldSizeX, 0, 0) , pLogicDetector, "physical detector", pLogicWorld, false, 0);
	
	return pPhysWorld;	
};
