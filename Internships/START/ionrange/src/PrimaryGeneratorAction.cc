#include <G4VUserPrimaryGeneratorAction.hh>
#include <G4ParticleGun.hh>
#include <G4RunManager.hh>
#include <G4ThreeVector.hh>
#include <G4ParticleTable.hh>
#include <G4ParticleDefinition.hh>
#include <G4Event.hh>
#include "globals.hh"
#include "DetectorConstruction.hh"
#include "PrimaryGeneratorAction.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

PrimaryGeneratorAction::PrimaryGeneratorAction() 
	: G4VUserPrimaryGeneratorAction()
	, fpParticleGun(nullptr)
{	
	G4int fNParticle = 1;
	fpParticleGun.reset(new G4ParticleGun(fNParticle));
	
	G4ParticleTable* fpParticleTable = G4ParticleTable::GetParticleTable();
	G4ParticleDefinition* fpParticle = fpParticleTable->FindParticle("alpha");
	
	fpParticleGun->SetParticleDefinition(fpParticle);
	fpParticleGun->SetParticleEnergy(100*CLHEP::keV);
	fpParticleGun->SetParticleMomentumDirection(G4ThreeVector(.0,.0,.1));
	fpParticleGun->SetParticlePosition(G4ThreeVector(.0,.0,.0));
};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void PrimaryGeneratorAction::GeneratePrimaries(G4Event* anEvent){
	const DetectorConstruction* pDetectorConstruction = 
			static_cast<const DetectorConstruction*>
			(G4RunManager::GetRunManager()->GetUserDetectorConstruction());
			
	double x, y, z;
	x = pDetectorConstruction->GetCylinderRadius();
	y = pDetectorConstruction->GetCylinderRadius();
	z = pDetectorConstruction->GetCylinderLength();
	fpParticleGun->SetParticlePosition(G4ThreeVector(0, 0, -z));
	
	fpParticleGun->GeneratePrimaryVertex(anEvent);
};
