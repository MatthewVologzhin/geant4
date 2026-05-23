#pragma once
#include <G4UserSteppingAction.hh>
#include <G4Step.hh>
#include <G4Track.hh>
#include <G4AnalysisManager.hh>
#include "RunAction.hh"
#include "EventAction.hh"

class SteppingAction : public G4UserSteppingAction{
public:
	SteppingAction(RunAction*, EventAction*);
	~SteppingAction() override = default;
	void UserSteppingAction(const G4Step* step) override;
	G4double GetRadius(const G4Step* step);
	G4double GetRadiusFromAxis(const G4Step* step);
	G4double GetX(const G4Step* step);
	G4double GetY(const G4Step* step);
	G4double GetZ(const G4Step* step);

private:
	G4int verboseLevel = 1;
	G4double fIonLength = .0;
	G4double fIonEnergy = .0;
	G4double fRadius = .0;
	G4double fElectronLength = .0;
	G4double fElectronEnergy = .0;
	G4double fKineticEnergy;
	G4double fRadiusFromAxis;
	G4double fX, fY, fZ;
	G4double dE, dl;
	G4int fPDGCode, fTrackID;
	G4String fParticleName, fVolumeName, fProcessName;
	G4long fStepsNb = 0;
	G4Track* fpTrack;
	G4AnalysisManager* fpAnalysisManager;
	RunAction* fpRunAction;
	EventAction* fpEventAction;
	const DetectorConstruction* pDetectorConstruction;
};
