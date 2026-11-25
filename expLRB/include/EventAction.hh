// EventAction.hh
#pragma once

#include "G4UserEventAction.hh"
#include "globals.hh"

class DetectorConstruction;
class RunAction;

class EventAction final : public G4UserEventAction
{
public:
    explicit EventAction(const DetectorConstruction* det, RunAction* runAction = nullptr);
    ~EventAction() override = default;

    void BeginOfEventAction(const G4Event*) override;
    void EndOfEventAction(const G4Event*) override;

private:
    const DetectorConstruction* fDetector{nullptr};
    RunAction* fRunAction{nullptr};
    G4int fCollID{-1};
};