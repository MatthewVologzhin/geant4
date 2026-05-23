#include "PrimaryGeneratorAction.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleDefinition.hh"
#include "G4ParticleTable.hh"
#include "G4SystemOfUnits.hh"
#include "G4RandomTools.hh"
#include "DetectorConstruction.hh"


PrimaryGeneratorAction::PrimaryGeneratorAction() : G4VUserPrimaryGeneratorAction(), fpParticleGun(nullptr){
	G4int numParticles = 1;
	G4double fParticleEnergy = 511*CLHEP::keV;
	fpParticleGun.reset(new G4ParticleGun(numParticles));
	
	G4ParticleTable* fParticleTable = G4ParticleTable::GetParticleTable();
	G4ParticleDefinition* fParticle = fParticleTable->FindParticle("gamma");
	
	fpParticleGun->SetParticleDefinition(fParticle);
	fpParticleGun->SetParticleEnergy(fParticleEnergy);
	fpParticleGun->SetParticlePosition(G4ThreeVector(.0, .0, .0));
};

PrimaryGeneratorAction::~PrimaryGeneratorAction() = default;

void PrimaryGeneratorAction::GeneratePrimaries(G4Event* anEvent){
	G4double maxAngle = 1.5*degree;
	G4double theta = (2*G4UniformRand()-1)*maxAngle;
	G4double radius = sin(theta);
	G4double phi = 360*G4UniformRand()*degree;
	G4double y = radius*cos(phi);
	G4double z = radius*sin(phi);
	fpParticleGun->SetParticleMomentumDirection(G4ThreeVector(1.0, y, z));
	fpParticleGun->GeneratePrimaryVertex(anEvent);
};
