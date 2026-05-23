#pragma once

#include <G4VUserDetectorConstruction.hh>
#include <G4NistManager.hh>
#include <G4Material.hh>
#include <G4UIcmdWithADoubleAndUnit.hh>
#include <G4UIdirectory.hh>
#include <G4VPhysicalVolume.hh>
#include <G4VUserDetectorConstruction.hh>
#include <G4UImessenger.hh>
#include <G4Orb.hh>
#include <G4Box.hh>
#include <G4Tubs.hh>
#include <G4LogicalVolume.hh>

class DetectorConstruction 
	: public G4VUserDetectorConstruction, G4UImessenger{
public:
	DetectorConstruction();
	~DetectorConstruction() override=default;
	void SetNewValue(G4UIcommand*, G4String) override;
	void SetWorldZSize(G4double);
	void SetCylinderRadius(G4double);
	void SetCylinderLength(G4double);
	G4double GetCylinderRadius() const {return fCylinderRadius;};
	G4double GetCylinderLength() const {return fCylinderLength;};
	G4VPhysicalVolume* Construct() override;

private:
	G4double fCylinderRadius, fCylinderLength, fWorldZSize;
	G4NistManager* fpNistManager;
	G4Material* fpCylinderMaterial;
	G4Material* fpWorldMaterial;
	G4Tubs* fpSolidCylinder;
	G4Box* fpSolidWorld;
	G4LogicalVolume* fpLogicWorld;
	G4LogicalVolume* fpLogicCylinder;
	G4VPhysicalVolume* fpWorld;
	G4VPhysicalVolume* fpCylinder;
	G4VisAttributes* fpCylinderVis;
	G4VisAttributes* fpWorldVis;
	G4UIdirectory* fpWorldDir;
	G4UIcmdWithADoubleAndUnit* fpCylinderRadiusCmd; 
	G4UIcmdWithADoubleAndUnit* fpWorldZSizeCmd;
	G4UIcmdWithADoubleAndUnit* fpCylinderLengthCmd;
	G4Region* fpTargetRegion;
}; 
