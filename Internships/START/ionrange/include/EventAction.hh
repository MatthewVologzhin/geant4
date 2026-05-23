#pragma once
#include <G4UserEventAction.hh>
#include <G4Event.hh>
#include <G4AnalysisManager.hh>
#include "RunAction.hh"
#include "DetectorConstruction.hh"

class EventAction : public G4UserEventAction{
public:
	EventAction(RunAction*);
	~EventAction() override = default;

	void BeginOfEventAction(const G4Event*) override;
	void EndOfEventAction(const G4Event*) override;
	void AddEdep(G4double);
	void FillRoot(G4double, G4double, G4double);
	
private:
	G4int verboseLevel = 0;
	G4double fEdep; 
	RunAction* pRunAction;
	const G4VUserDetectorConstruction* fpConstDet;
	const DetectorConstruction* fpDet;
	G4double fInitialEnergy, fIonLength, fIonRadius, fIonEDep;
	G4AnalysisManager* fpAnalysisManager;
	G4int PDGCode;
	std::chrono::time_point<std::chrono::system_clock> fStartTime, fEndTime;
};

