// PrimaryGeneratorAction.cc
#include "PrimaryGeneratorAction.hh"

#include "DetectorConstruction.hh"
#include "PrimaryGeneratorMessenger.hh"

#include "G4Event.hh"
#include "G4ParticleDefinition.hh"
#include "G4ParticleTable.hh"
#include "G4SystemOfUnits.hh"
#include "Randomize.hh"
#include <cmath>

PrimaryGeneratorAction::PrimaryGeneratorAction(DetectorConstruction* det) : fDetector(det)
{
  G4int n_particle = 1;
  fBeamRadius = 0;
  fParticleGun = new G4ParticleGun(n_particle);
  SetDefaultKinematic();

  fGunMessenger = new PrimaryGeneratorMessenger(this);
}

PrimaryGeneratorAction::~PrimaryGeneratorAction()
{
  delete fParticleGun;
  delete fGunMessenger;
}

void PrimaryGeneratorAction::SetDefaultKinematic()
{
  G4ParticleTable* particleTable = G4ParticleTable::GetParticleTable();
  G4ParticleDefinition* particle = particleTable->FindParticle("e-");
  fParticleGun->SetParticleDefinition(particle);
  fParticleGun->SetParticleMomentumDirection(G4ThreeVector(0., 0., 1.));
  fParticleGun->SetParticleEnergy(1. * GeV);
  G4double z0 = -0.4999 * (fDetector->GetWorldSizeZ());
  fParticleGun->SetParticlePosition(G4ThreeVector(0.0, 0.0, z0));
}

void PrimaryGeneratorAction::GeneratePrimaries(G4Event* anEvent)
{
  G4ThreeVector oldPosition = fParticleGun->GetParticlePosition();
  G4double z0 = oldPosition.z();
  const G4double halfWorldZ = 0.5 * fDetector->GetWorldSizeZ();
  if (std::abs(z0) >= halfWorldZ) {
    z0 = -0.4999 * fDetector->GetWorldSizeZ();
  }

  if (fBeamRadius > 0.) {
    G4double x0 = (2. * G4UniformRand() - 1.) * fBeamRadius;
    G4double y0 = (2. * G4UniformRand() - 1.) * fBeamRadius;
    fParticleGun->SetParticlePosition(G4ThreeVector(x0, y0, z0));
    fParticleGun->GeneratePrimaryVertex(anEvent);
    fParticleGun->SetParticlePosition(oldPosition);
  } else {
    if (z0 != oldPosition.z()) {
      fParticleGun->SetParticlePosition(G4ThreeVector(oldPosition.x(), oldPosition.y(), z0));
      fParticleGun->GeneratePrimaryVertex(anEvent);
      fParticleGun->SetParticlePosition(oldPosition);
    } else {
      fParticleGun->GeneratePrimaryVertex(anEvent);
    }
  }
}