#include <G4Track.hh>
#include <G4Step.hh>
#include <G4SystemOfUnits.hh>
#include <G4UnitsTable.hh>
#include <G4AnalysisManager.hh>
#include <G4VProcess.hh>
#include <G4RunManager.hh>
#include "RunAction.hh"
#include "SteppingAction.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

SteppingAction::SteppingAction(RunAction* run, EventAction* event) : fpRunAction(run), fpEventAction(event){
	verboseLevel = -1;
	fpAnalysisManager = G4AnalysisManager::Instance();
	pDetectorConstruction = static_cast<const DetectorConstruction*>
			(G4RunManager::GetRunManager()->GetUserDetectorConstruction());
			
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
	// some conditions for catching //
	if (!(abs(fPDGCode) == 11 || fPDGCode == 22 /*ions*/)){
		fIonLength += dl;
		fIonEnergy += dE;
	} else if (abs(fPDGCode) == 11 /*electrons & positrons*/) {
		fElectronLength += dl;
		fElectronEnergy += dE;
	} else if (fPDGCode == 22 /*gamma*/){
		/* some ntuples */
	} else /*other particles*/{
		G4cout << "Unknown particle: " << fParticleName << G4endl;
	}
	
	++fStepsNb;
	if (fKineticEnergy == .0){
		if (abs(fPDGCode) == 11 /*electron & positrons*/){
			if (verboseLevel > 0){
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
			}
			if (not(fElectronLength == .0 or fElectronEnergy == .0)){
				/* some ntuples */
			} else {
				/* some ntuples */
			};
			fElectronEnergy = .0;
			fElectronLength = .0;

		} else if (fPDGCode == 22 /*gamma*/){
			/* some ntuples */
		} else /*other particles*/{
			if (verboseLevel > 0){
				G4cout << "\n" <<"========= Stopping particle info =========" << "\n" 
					   << "Particle name: \t" << fParticleName << "\n"
					   << "Track ID: \t" 	  << fpTrack->GetTrackID() << "\n"
					   << "Volume name: \t"   << fVolumeName << "\n"
					   << "Steps number: \t"  << fStepsNb << "\n"
					   << "Process name: \t"  << fProcessName << "\n"
					   << "Track length: \t"  << G4BestUnit(fIonLength, "Length") << "\n"
					   << "Radius: \t" 		  << G4BestUnit(GetRadius(step), "Length") << "\n"
					   << "Dep. energy: \t"   << G4BestUnit(fIonEnergy, "Energy") << "\n"
					   << "LET: \t\t" 		  << G4BestUnit(fIonEnergy/fIonLength, "Energy/Length") << "\n"//<< G4endl;
					   << "Z:\t\t" << fZ << G4endl;
			}
			
			fpEventAction->FillRoot(fIonLength, fRadius, fIonEnergy + fElectronEnergy);
			
			if (!(fIonLength == .0 || fIonEnergy == .0)){
				/* some ntuples */
			} else {
				/* some ntuples */
			};
			fIonEnergy = .0;
			fIonLength = .0;
		};
	};
	
	fpRunAction->GetParticleINFO(step);
};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

G4double SteppingAction::GetRadius(const G4Step* step){
	double x, y, z;
	x = pDetectorConstruction->GetCylinderRadius();
	y = pDetectorConstruction->GetCylinderRadius();
	z = pDetectorConstruction->GetCylinderLength();
	auto p0 = G4ThreeVector(.0, .0, -z);
	auto p1 = step->GetPreStepPoint()->GetPosition();
	auto p2 = step->GetPostStepPoint()->GetPosition();
	auto p  = -p0 + p1 + (p2 - p1)*G4UniformRand();
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

