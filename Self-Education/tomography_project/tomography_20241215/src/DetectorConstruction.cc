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
	fMessenger->DeclareProperty("numDetectorsY", 	numDetectorsY, 		"Number of the detectors along axis Y");
	fMessenger->DeclareProperty("numDetectorsZ", 	numDetectorsZ, 		"Number of the detectors along axis Z");
	fMessenger->DeclareProperty("detectorLengthY", 	fDetectorLengthY, 	"Length of each detector along axis Y");
	fMessenger->DeclareProperty("detectorLengthZ", 	fDetectorLengthZ, 	"Length of each detector along axis Z");
	fMessenger->DeclareProperty("detectorHeight", 	fDetectorHeight, 	"Height of each detector");
	
	fWorldSizeX = 200*CLHEP::cm;
	fWorldSizeY = 80*CLHEP::cm;
	fWorldSizeZ = 80*CLHEP::cm;
	fTubsRadius = 2.0*CLHEP::cm;
	fTubsHeight = 6.0*CLHEP::cm;
	numDetectorsY = 40;
	numDetectorsZ = 40;
	fDetectorLengthY = 2*fWorldSizeY/numDetectorsY;
	fDetectorLengthZ = 2*fWorldSizeZ/numDetectorsZ;
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
	G4VPhysicalVolume* pPhysTubs = new G4PVPlacement(pRotation, G4ThreeVector(-35.0*CLHEP::cm, 0, 0), pLogicTubs, "physical tube", pLogicWorld, false, 0);
		
	G4Box* pSolidDetector = new G4Box("detector", fDetectorHeight, fDetectorLengthY, fDetectorLengthZ);
	G4LogicalVolume* pLogicDetector = new G4LogicalVolume(pSolidDetector, fDetectorMaterial, "logical detector");
	/*G4VPhysicalVolume* pPhysDetector = new G4PVPlacement(0, G4ThreeVector(fWorldSizeX-fDetectorHeigth/2, 0, 0), pLogicDetector, "physical detector", pLogicWorld, false, 0); */
	for (size_t i=1; i < numDetectorsY; i++){
	for (size_t j=1; j < numDetectorsZ; j++){
		G4double xPos = fWorldSizeX - fDetectorHeight/2;
		G4double yPos = -fWorldSizeY + i*fDetectorLengthY;
		G4double zPos = -fWorldSizeZ + j*fDetectorLengthZ;
		G4ThreeVector position = G4ThreeVector(xPos, yPos, zPos);
		new G4PVPlacement(0, position, pLogicDetector, std::to_string(i), pLogicWorld, false, 0);
	}}
	
	return pPhysWorld;	
};
