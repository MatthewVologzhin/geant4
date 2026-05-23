#include "SteppingAction.hh"
#include "G4SystemOfUnits.hh"
#include "RunAction.hh"
#include "EventAction.hh"

SteppingAction::SteppingAction(DetectorConstruction* detector, EventAction* event, RunAction* run) :
	fDetector(detector),
	fEventAction(event),
	fRunAction(run)
{};  

SteppingAction::~SteppingAction()
{};

void SteppingAction::UserSteppingAction(const G4Step* step){
	G4Track* track = step->GetTrack();
	G4String materialName = track->GetVolume()->
				    GetLogicalVolume()->GetMaterial()->GetName();
	G4String volumeName = track->GetVolume()->GetName();
	fAnalysisManager = G4RootAnalysisManager::Instance();
	if (materialName != "G4_AIR" and materialName != "G4_Al"){
		G4double kineticEnergy = track->GetKineticEnergy();
		G4int y_coord = Split(volumeName, "_", 1);
		G4int z_coord = Split(volumeName, "_", 2);	
		fAnalysisManager->FillH2(0, y_coord, z_coord, 1);
		track->SetTrackStatus(fStopAndKill);
	};
};

G4int SteppingAction::Split(G4String str, const G4String delimiter, G4int index){
	G4int pos = str.find(delimiter);
	if (index == 1){
		str = str.substr(0, pos);
	} else {
		str = str.substr(pos+1); 
	};
	return std::stoi(str);
};
