#pragma once

#include <G4VUserPrimaryGeneratorAction.hh>
#include <memory>

class G4ParticleGun;

class PrimaryGeneratorAction : public G4VUserPrimaryGeneratorAction{
public:
	PrimaryGeneratorAction();
	~PrimaryGeneratorAction() override = default;
	void GeneratePrimaries(G4Event*) override;

private:
	std::unique_ptr<G4ParticleGun> fpParticleGun;
};
