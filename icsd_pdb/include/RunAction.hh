#pragma once

#include "G4UserRunAction.hh"
#include "G4RunManager.hh"

#include "globals.hh"

class RunMessenger;

class RunAction : public G4UserRunAction
{
  public:
    RunAction();
    virtual ~RunAction();
    virtual void BeginOfRunAction(const G4Run*);
    virtual void EndOfRunAction(const G4Run*);

    void SetOutputFileName(const G4String& name){fFileName = name;};

  private:
    G4String fFileName;
    RunMessenger* fpMessenger;
};
