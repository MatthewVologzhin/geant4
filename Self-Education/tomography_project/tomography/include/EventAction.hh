#pragma once
#include "G4UserEventAction.hh"
#include "G4Event.hh"


class RunAction;

class EventAction : public G4UserEventAction{
public:
	EventAction(RunAction*);
	~EventAction() override;
	
	void BeginOfEventAction(const G4Event*);
	void EndOfEventAction(const G4Event*);
	
private:
	RunAction* fRunAction;
};
