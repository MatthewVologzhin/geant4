#include <G4SystemOfUnits.hh>
#include <G4RootAnalysisManager.hh>
#include "RunAction.hh"
#include "DetectorConstruction.hh"

RunAction::RunAction(DetectorConstruction* fDetector) : fpDetector(fDetector){
	fAnalysisManager = G4RootAnalysisManager::Instance();
	fAnalysisManager->SetDefaultFileType("root");
	fAnalysisManager->SetFileName("tomography");	
	fAnalysisManager->SetVerboseLevel(1);
	fAnalysisManager->SetActivation(true);
	fWorldSizeY = fpDetector->GetWorldSizeY();
	fWorldSizeZ = fpDetector->GetWorldSizeZ();
	detectorResolutionY = 200;
	detectorResolutionZ = 200;
};

RunAction::~RunAction(){};

void RunAction::Book(){
	fAnalysisManager->SetFirstHistoId(0);
	
	fAnalysisManager->CreateH2("2D Histogram", "2D Histogram of Cyclinder",
				   detectorResolutionY, -fWorldSizeY, fWorldSizeY,
				   detectorResolutionZ, -fWorldSizeZ, fWorldSizeZ);
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
