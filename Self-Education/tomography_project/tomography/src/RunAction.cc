#include <G4SystemOfUnits.hh>
#include <G4RootAnalysisManager.hh>
#include "RunAction.hh"


RunAction::RunAction(){
	fAnalysisManager = G4RootAnalysisManager::Instance();
	fAnalysisManager->SetDefaultFileType("root");
	fAnalysisManager->SetFileName("tomography");	
	fAnalysisManager->SetVerboseLevel(1);
	fAnalysisManager->SetActivation(true);	
};

RunAction::~RunAction(){};

void RunAction::Book(){
	fAnalysisManager->SetFirstHistoId(0);
	fAnalysisManager->CreateH2("2D Histogram", "2D Histogram of Cyclinder",
				   200, 0, 200,
				   200, 0, 200);
				   
	fAnalysisManager->OpenFile();
};

void RunAction::BeginOfRunAction(const G4Run* run){
	Book();
};

void RunAction::EndOfRunAction(const G4Run* run){
	if (fAnalysisManager->IsActive()){
		fAnalysisManager->Write();
		fAnalysisManager->CloseFile();
		fAnalysisManager->Clear();
	};
};
