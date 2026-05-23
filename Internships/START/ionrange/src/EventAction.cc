#include <G4UserEventAction.hh>
#include <G4Event.hh>
#include <G4ParticleDefinition.hh>
#include <G4RunManager.hh>
#include <G4SystemOfUnits.hh>
#include <G4UnitsTable.hh>
#include "globals.hh"
#include "RunAction.hh"
#include "EventAction.hh"


EventAction::EventAction(RunAction* run) : pRunAction(run){
	verboseLevel = 0;
	fpAnalysisManager = G4AnalysisManager::Instance();
};


void EventAction::BeginOfEventAction(const G4Event* event){
	fStartTime = std::chrono::system_clock::now();
	fInitialEnergy = event->GetPrimaryVertex()->GetPrimary()->GetKineticEnergy();
	pRunAction->GetParticleINFO(event);
	fEdep = .0;
};

void EventAction::EndOfEventAction(const G4Event* event){
	fEndTime = std::chrono::system_clock::now();
	PDGCode = event->GetPrimaryVertex()->GetPrimary()->GetPDGcode();
	
	fpAnalysisManager->FillNtupleDColumn(2, 0, fInitialEnergy);
	fpAnalysisManager->FillNtupleDColumn(2, 1, (fEndTime-fStartTime).count()/s);
	fpAnalysisManager->AddNtupleRow(2);
};

void EventAction::AddEdep(G4double dE){
	fEdep += dE;
};

void EventAction::FillRoot(G4double range, G4double radius, G4double totalEDep){
	fIonLength = range;
	fIonRadius = radius;
	fIonEDep = totalEDep;
	if (!(abs(PDGCode) == 11 || PDGCode == 22) && fIonLength > 1e-10){
		fpAnalysisManager->FillNtupleDColumn(1, 0, fInitialEnergy);
		fpAnalysisManager->FillNtupleDColumn(1, 1, fIonLength);
		fpAnalysisManager->AddNtupleRow(1);
		
		fpAnalysisManager->FillNtupleDColumn(3, 0, fInitialEnergy);
		fpAnalysisManager->FillNtupleDColumn(3, 1, fIonRadius/fIonLength);
		fpAnalysisManager->AddNtupleRow(3);
		
		//fpAnalysisManager->FillNtupleDColumn(4, 0, fInitialEnergy);
		//fpAnalysisManager->FillNtupleDColumn(4, 1, fIonEDep/fIonLength);
		//fpAnalysisManager->AddNtupleRow(4);
	}
}
