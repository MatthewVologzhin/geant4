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
		x_coord = 0.5*(step->GetPostStepPoint()->GetPosition().x() + step->GetPreStepPoint()->GetPosition().x());
		y_coord = 0.5*(step->GetPostStepPoint()->GetPosition().y() + step->GetPreStepPoint()->GetPosition().y());
		z_coord = 0.5*(step->GetPostStepPoint()->GetPosition().z() + step->GetPreStepPoint()->GetPosition().z());
		cosTheta = x_coord/(sqrt(x_coord*x_coord+y_coord*y_coord+z_coord*z_coord));
		fAnalysisManager->FillH2(0, y_coord, z_coord, kineticEnergy);
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
