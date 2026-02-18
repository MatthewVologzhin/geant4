#include <G4Track.hh>
#include <G4GenericIon.hh>
#include <G4Electron.hh>
#include <G4Gamma.hh>
#include <G4AnalysisManager.hh>
#include "TrackingAction.hh"

TrackingAction::TrackingAction(RunAction* run) : pRunAction(run){
	pAnalysisManager = G4AnalysisManager::Instance();
};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void TrackingAction::PreUserTrackingAction(const G4Track* pTrack){
	G4int flagParticle = -1;
	G4ParticleDefinition* partDef = pTrack->GetDynamicParticle()->GetDefinition();
	if (partDef == G4Electron::ElectronDefinition()){
		flagParticle = 1;
		pRunAction->GetElectronINFO(pTrack);
	} else if (partDef == G4Gamma::GammaDefinition()){
		flagParticle = 2;
		pRunAction->GetGammaINFO(pTrack);
	} else if (partDef->GetParticleType() == "nucleus" || partDef->GetParticleType() == "DNAion"){ 
		flagParticle = 3;
	} else {
		//G4cout << partDef->GetParticleName() << " " << partDef->GetParticleType() << G4endl;
	}
};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

void TrackingAction::PostUserTrackingAction(const G4Track*){;}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

