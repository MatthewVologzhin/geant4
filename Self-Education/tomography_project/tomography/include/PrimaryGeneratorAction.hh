#pragma once

#include "G4VUserPrimaryGeneratorAction.hh"
#include "DetectorConstruction.hh"
#include <memory>

class G4ParticleGun;
class DetectorConstruction;

class PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction{
public:
	PrimaryGeneratorAction();
	~PrimaryGeneratorAction() override;
	void GeneratePrimaries(G4Event* event) override;

private:
	std::unique_ptr<G4ParticleGun> fpParticleGun;
	DetectorConstruction* fpDet;
/*	G4double fWorldSizeX = fpDet->GetWorldSizeX();
	G4double fWorldSizeY = fpDet->GetWorldSizeY();
	G4double fWorldSizeZ = fpDet->GetWorldSizeZ();*/
};
