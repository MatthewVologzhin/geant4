#include <G4VUserActionInitialization.hh>
#include "PrimaryGeneratorAction.hh"
#include "SteppingAction.hh"
#include "TrackingAction.hh"
#include "EventAction.hh"
#include "RunAction.hh"
#include "ActionInitialization.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

ActionInitialization::ActionInitialization() 
	: G4VUserActionInitialization(){}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

ActionInitialization::~ActionInitialization()
{}

void ActionInitialization::BuildForMaster() const
{
	RunAction* pRunAction = new RunAction();
	SetUserAction(pRunAction);
}

void ActionInitialization::Build() const
{
	RunAction* pRunAction = new RunAction();
	EventAction* pEventAction = new EventAction(pRunAction);
	SteppingAction* pSteppingAction = new SteppingAction(pRunAction, pEventAction);
	TrackingAction* pTrackingAction = new TrackingAction(pRunAction);
	SetUserAction(new PrimaryGeneratorAction());
	SetUserAction(pSteppingAction);
	SetUserAction(pTrackingAction);
	SetUserAction(pEventAction);
	SetUserAction(pRunAction);
	
}
