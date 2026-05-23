#pragma once
#include <G4UserTrackingAction.hh>
#include <G4AnalysisManager.hh>
#include "RunAction.hh"
#include "globals.hh"

class TrackingAction : public G4UserTrackingAction{
public:
	TrackingAction(RunAction*);
	~TrackingAction() override = default;

	void PreUserTrackingAction(const G4Track*) override;
	void PostUserTrackingAction(const G4Track*) override;
	
private:
	G4AnalysisManager* pAnalysisManager;
	RunAction* pRunAction;
};
