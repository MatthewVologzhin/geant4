#pragma once

#include "G4VUserDetectorConstruction.hh"
#include "G4Material.hh"
#include "G4GenericMessenger.hh"
#include <memory>

class DetectorConstruction : public G4VUserDetectorConstruction
{
public:
	DetectorConstruction();
        ~DetectorConstruction() override=default;
    	G4VPhysicalVolume* Construct() override;
	
public:
	G4double fWorldSizeX, fWorldSizeY, fWorldSizeZ;
	G4double fTubsRadius, fTubsHeight;
	G4double fDetectorHeight;
	
	G4double GetWorldSizeX(){return fWorldSizeX;};
	G4double GetWorldSizeY(){return fWorldSizeY;};
	G4double GetWorldSizeZ(){return fWorldSizeZ;};
	G4double GetTubsRadius(){return fTubsRadius;};
	G4double GetTubsHeigth(){return fTubsHeight;};
	

private:
	G4Material* fTubsMaterial;
	G4Material* fDetectorMaterial;
	G4Material* fWorldMaterial;
	G4GenericMessenger* fMessenger;
};	
