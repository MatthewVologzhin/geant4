// ActionInitialization.cc
#include "ActionInitialization.hh"
#include "DetectorConstruction.hh"
#include "PrimaryGeneratorAction.hh"
#include "RunAction.hh"
#include "EventAction.hh"
#include "TrackingAction.hh"

ActionInitialization::ActionInitialization(DetectorConstruction* det) : fDetector(det) {}

void ActionInitialization::BuildForMaster() const
{
    SetUserAction(new RunAction(fDetector));
}

void ActionInitialization::Build() const
{
    SetUserAction(new PrimaryGeneratorAction(fDetector));
    auto* runAction = new RunAction(fDetector);
    SetUserAction(runAction);
    SetUserAction(new EventAction(fDetector, runAction));
    SetUserAction(new TrackingAction());
}