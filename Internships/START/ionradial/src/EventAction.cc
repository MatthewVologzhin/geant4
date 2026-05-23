#include <G4UserEventAction.hh>
#include <G4Event.hh>
#include <G4ParticleDefinition.hh>
#include <G4RunManager.hh>
#include <G4SystemOfUnits.hh>
#include "globals.hh"
#include "RunAction.hh"
#include "EventAction.hh"


EventAction::EventAction(RunAction* run) : pRunAction(run){
	verboseLevel = 0;
	fpAnalysisManager = G4AnalysisManager::Instance();
};


void EventAction::BeginOfEventAction(const G4Event* event){
	pRunAction->GetParticleINFO(event);
	fEdep = .0;
};

void EventAction::EndOfEventAction(const G4Event* event){
	fpConstDet = G4RunManager::GetRunManager()->GetUserDetectorConstruction();
	fpDet = dynamic_cast<const DetectorConstruction*>(fpConstDet);
	LETvalue	   = (fEdep/keV)/(fpDet->GetCylinderLength()/um);
	fpAnalysisManager->FillNtupleDColumn(5, 0, LETvalue);
	fpAnalysisManager->AddNtupleRow(5);
};

void EventAction::AddEdep(G4double dE){
	fEdep += dE;
};
