#include <G4Track.hh>
#include <G4Step.hh>
#include <G4SystemOfUnits.hh>
#include <G4UnitsTable.hh>
#include <G4AnalysisManager.hh>
#include <G4VProcess.hh>
#include "RunAction.hh"
#include "SteppingAction.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

SteppingAction::SteppingAction(RunAction* run, EventAction* event) : fpRunAction(run), fpEventAction(event){
	verboseLevel = -1;
	fpAnalysisManager = G4AnalysisManager::Instance();
};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

//SteppingAction::~SteppingAction(){};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

void SteppingAction::UserSteppingAction(const G4Step* step){
	fpTrack = step->GetTrack();
	fTrackID = fpTrack->GetTrackID();
	fPDGCode = fpTrack->GetDefinition()->GetPDGEncoding();
	fKineticEnergy = fpTrack->GetKineticEnergy();
	fParticleName = fpTrack->GetDefinition()->GetParticleName();
	fVolumeName = fpTrack->GetVolume()->GetName();
	fRadius = GetRadius(step);
	fRadiusFromAxis = GetRadiusFromAxis(step);
	fX = GetX(step);
	fY = GetY(step);
	fZ = GetZ(step);
	fProcessName = step->GetPostStepPoint()->GetProcessDefinedStep()->GetProcessName();
	dE = step->GetTotalEnergyDeposit();
	dl = step->GetStepLength();
	
	// Dose calculation collection //
	if (dE > 0){
		fpEventAction->AddEdep(dE);
		if (fPDGCode == 11){
			fpAnalysisManager->FillNtupleDColumn(1, 0, dE/joule);
			fpAnalysisManager->FillNtupleDColumn(1, 1, fRadiusFromAxis/nm);
			fpAnalysisManager->FillNtupleDColumn(1, 2, fX/nm);
			fpAnalysisManager->FillNtupleDColumn(1, 3, fY/nm);
			fpAnalysisManager->FillNtupleDColumn(1, 4, fZ/nm);
			fpAnalysisManager->AddNtupleRow(1);
		}
	}
	
	// some conditions for catching //
	if (abs(fPDGCode) != 11 and fPDGCode != 22 /*ions*/){
		fLength += dl;
		fEnergy += dE;
		fpAnalysisManager->FillNtupleDColumn(2, 0, dE/MeV);
		fpAnalysisManager->FillNtupleDColumn(2, 1, fLength/nm);
		fpAnalysisManager->FillNtupleDColumn(2, 2, fRadiusFromAxis/nm);
		fpAnalysisManager->AddNtupleRow(2);
	} else if (fPDGCode == 11 /*electrons & positrons*/) {
		fElectronLength += dl;
		fElectronEnergy += dE;
	} else if (fPDGCode == 22 /*gamma*/){
		/* some ntuples */
	} else /*other particles*/{
		G4cout << "Unknown particle: " << fParticleName << G4endl;
	}
	
	++fStepsNb;
	if (fKineticEnergy == .0 and verboseLevel > 0){
		if (abs(fPDGCode) == 11 /*electron & positrons*/){
			G4cout << "\n" << "========= Stopping particle info =========" << "\n" 
			       << "Particle name: \t" << fParticleName << "\n"
			       << "Track ID: \t" 	  << fpTrack->GetTrackID() << "\n"
				   << "Volume name: \t"   << fVolumeName << "\n"
				   << "Steps number: \t"  << fStepsNb << "\n"
				   << "Process name: \t"  << fProcessName << "\n"
				   << "Track length: \t"  << G4BestUnit(fElectronLength, "Length") << "\n"
				   << "Radius: \t" 		  << G4BestUnit(GetRadius(step), "Length") << "\n"
				   << "Dep. energy: \t"   << G4BestUnit(fElectronEnergy, "Energy") << "\n" 
				   << "LET: \t\t" 		  << G4BestUnit(fElectronEnergy/fElectronLength, "Energy/Length") << G4endl;
			if (not(fElectronLength == .0 or fElectronEnergy == .0)){
				/* some ntuples */
			} else {
				/* some ntuples */
			};
			fElectronEnergy = .0;
			fElectronLength = .0;

		} else if (fPDGCode == 22 /*gamma*/){
			G4cout << "\n\nDevelop gamma condition!\n\n" << G4endl;
		} else /*other particles*/{
			G4cout << "\n" <<"========= Stopping particle info =========" << "\n" 
				   << "Particle name: \t" << fParticleName << "\n"
				   << "Track ID: \t" 	  << fpTrack->GetTrackID() << "\n"
			       << "Volume name: \t"   << fVolumeName << "\n"
				   << "Steps number: \t"  << fStepsNb << "\n"
				   << "Process name: \t"  << fProcessName << "\n"
				   << "Track length: \t"  << G4BestUnit(fLength, "Length") << "\n"
				   << "Radius: \t" 		  << G4BestUnit(GetRadius(step), "Length") << "\n"
				   << "Dep. energy: \t"   << G4BestUnit(fEnergy, "Energy") << "\n"
				   << "LET: \t\t" 		  << G4BestUnit(fEnergy/fLength, "Energy/Length") << "\n"//<< G4endl;
				   << "Z:\t\t" << fZ << G4endl;
			if (not(fLength == .0 or fEnergy == .0)){
				/* some ntuples */
			} else {
				/* some ntuples */
			};
			fEnergy = .0;
			fLength = .0;
		};
	};
	
	fpRunAction->GetParticleINFO(step);
};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

G4double SteppingAction::GetRadius(const G4Step* step){
	auto p1 = step->GetPreStepPoint()->GetPosition();
	auto p2 = step->GetPostStepPoint()->GetPosition();
	auto p  = p1 + (p2 - p1)*G4UniformRand();
	return p.mag();
};

G4double SteppingAction::GetRadiusFromAxis(const G4Step* step){
	auto p1 = step->GetPreStepPoint()->GetPosition();
	auto p2 = step->GetPostStepPoint()->GetPosition();
	auto p  = p1 + (p2 - p1)*G4UniformRand();
	return sqrt(p.x()*p.x() + p.y()*p.y());
}

G4double SteppingAction::GetX(const G4Step* step){
	auto p1 = step->GetPreStepPoint()->GetPosition();
	auto p2 = step->GetPostStepPoint()->GetPosition();
	auto p  = p1 + (p2 - p1)*G4UniformRand();
	return p.x();
}

G4double SteppingAction::GetY(const G4Step* step){
	auto p1 = step->GetPreStepPoint()->GetPosition();
	auto p2 = step->GetPostStepPoint()->GetPosition();
	auto p  = p1 + (p2 - p1)*G4UniformRand();
	return p.y();
}

G4double SteppingAction::GetZ(const G4Step* step){
	auto p1 = step->GetPreStepPoint()->GetPosition();
	auto p2 = step->GetPostStepPoint()->GetPosition();
	auto p  = p1 + (p2 - p1)*G4UniformRand();
	return p.z();
}

