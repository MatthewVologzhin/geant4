// RunMessenger.cc
#include "RunMessenger.hh"
#include "RunAction.hh"

#include "G4UIdirectory.hh"
#include "G4UIcmdWithAString.hh"
#include "G4UIcmdWithAnInteger.hh"
#include "G4UIcmdWithoutParameter.hh"
#include "G4RunManager.hh"
#include "G4Exception.hh"

RunMessenger::RunMessenger()
{
    fOutputDir = std::make_unique<G4UIdirectory>("/bio/output/");
    fOutputDir->SetGuidance("Output data storage configuration.");

    fFileNameCmd = std::make_unique<G4UIcmdWithAString>("/bio/output/fileName", this);
    fFileNameCmd->SetGuidance("Set ROOT filename: /bio/output/fileName run1.root");
    fFileNameCmd->SetParameterName("fileName", false);
    fFileNameCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

    fRunDir = std::make_unique<G4UIdirectory>("/bio/run/");
    fRunDir->SetGuidance("Run control: plan an event count and start a run.");

    fSetEventsCmd = std::make_unique<G4UIcmdWithAnInteger>("/bio/run/setEvents", this);
    fSetEventsCmd->SetGuidance("Set the number of events for the next /bio/run/beamOn.");
    fSetEventsCmd->SetParameterName("nEvents", false);
    fSetEventsCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

    fBeamOnCmd = std::make_unique<G4UIcmdWithoutParameter>("/bio/run/beamOn", this);
    fBeamOnCmd->SetGuidance("Run BeamOn with the event count set by /bio/run/setEvents.");
    fBeamOnCmd->AvailableForStates(G4State_Idle);
}

RunMessenger::~RunMessenger() = default;

void RunMessenger::SetNewValue(G4UIcommand* command, G4String newValue)
{
    if (command == fFileNameCmd.get()) {
        RunAction::SetOutputFileName(newValue);
    }
    else if (command == fSetEventsCmd.get()) {
        const G4int n = G4UIcmdWithAnInteger::GetNewIntValue(newValue.c_str());
        if (n <= 0) {
            G4Exception("RunMessenger::SetNewValue", "InvalidEventCount",
                        FatalException, "Event count must be positive.");
        }
        SetPlannedEvents(n);
    }
    else if (command == fBeamOnCmd.get()) {
        const G4int n = GetPlannedEvents();
        if (n <= 0) {
            G4Exception("RunMessenger::SetNewValue", "NoPlannedEvents",
                        FatalException,
                        "No event count planned: call /bio/run/setEvents <N> first.");
        }
        auto* runManager = G4RunManager::GetRunManager();
        if (!runManager) {
            G4Exception("RunMessenger::SetNewValue", "NoRunManager",
                        FatalException, "G4RunManager is not available.");
        }
        runManager->BeamOn(n);
    }
}
