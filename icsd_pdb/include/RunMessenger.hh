#pragma once

#include "RunAction.hh"
#include "G4UImessenger.hh"
#include "globals.hh"

class G4UIcommand;
class G4UIdirectory;
class G4UIcmdWithAString;
class RunAction;

class RunMessenger : public G4UImessenger {
    public:
        RunMessenger(RunAction* pRunAction);
        ~RunMessenger() override;

        void SetNewValue(G4UIcommand* command, G4String newValue) override;

    private:
        RunAction* fpRunAction;
        G4UIdirectory* fpDirectory;
        G4UIcmdWithAString* fpFileNameCmd;
};