#pragma once

#include "G4UserSteppingAction.hh"
#include "G4RootAnalysisManager.hh"
#include "G4Step.hh"

class DetectorConstruction;
class EventAction;
class RunAction;

class SteppingAction : public G4UserSteppingAction{
public:
	SteppingAction(DetectorConstruction* detector,
		       EventAction* event,
		       RunAction* run);
	~SteppingAction() override;
	
	void UserSteppingAction(const G4Step* step);
	G4int Split(G4String str, const G4String delimiter, const G4int index);

private:
	DetectorConstruction* fDetector;
	RunAction* fRunAction;
	EventAction* fEventAction;
	G4RootAnalysisManager* fAnalysisManager = nullptr;
};
