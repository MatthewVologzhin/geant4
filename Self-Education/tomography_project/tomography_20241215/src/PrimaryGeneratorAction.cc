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
	fpParticleGun->SetParticlePosition(G4ThreeVector(-40.*CLHEP::cm, .0, .0));
};

PrimaryGeneratorAction::~PrimaryGeneratorAction() = default;

void PrimaryGeneratorAction::GeneratePrimaries(G4Event* anEvent){
	const G4double pi = 3.1415;
	G4double maxAngle = 4*(pi/180);
	G4double maxRadius = 1/cos(maxAngle);
	G4double radius = maxRadius*G4UniformRand();
	G4double phi = 2*pi*G4UniformRand();
	G4double y = radius*std::cos(phi);
	G4double z = radius*std::sin(phi);
	fpParticleGun->SetParticleMomentumDirection(G4ThreeVector(1.0, y, z));
	fpParticleGun->GeneratePrimaryVertex(anEvent);
};
