#pragma once

#include "G4ParticleGun.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "globals.hh"

class G4Event;
class DetectorConstruction;
class PrimaryGeneratorMessenger;

class PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction
{
  public:
    PrimaryGeneratorAction(DetectorConstruction*);
    ~PrimaryGeneratorAction() override;

  public:
    void SetDefaultKinematic();
    void SetBeamRadius(G4double val) { fBeamRadius = val; };
    void GeneratePrimaries(G4Event*) override;
    G4ParticleGun* GetParticleGun() { return fParticleGun; };

  private:
    G4ParticleGun* fParticleGun = nullptr;
    DetectorConstruction* fDetector = nullptr;
    G4double fBeamRadius;

    PrimaryGeneratorMessenger* fGunMessenger = nullptr;
};
