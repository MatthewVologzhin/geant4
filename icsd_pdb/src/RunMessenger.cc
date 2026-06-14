#include "RunMessenger.hh"
#include "RunAction.hh"
#include "G4UIdirectory.hh"
#include "G4UIcmdWithAString.hh"

RunMessenger::RunMessenger(RunAction* runAction) : fpRunAction(runAction){
    fpDirectory = new G4UIdirectory("/icsd/analysis/");
    fpDirectory->SetGuidance("Control of analysis: name");

    fpFileNameCmd = new G4UIcmdWithAString("/icsd/analysis/setFileName", this);
    fpFileNameCmd->SetGuidance("Change a name of the output file");
    fpFileNameCmd->SetParameterName("fileName", false);
    fpFileNameCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
}

RunMessenger::~RunMessenger(){
    delete fpFileNameCmd;
    delete fpDirectory;
}

void RunMessenger::SetNewValue(G4UIcommand* command, G4String newValue){
    if (command == fpFileNameCmd){
        fpRunAction->SetOutputFileName(newValue);
    }
}