#pragma once

#include "G4UserStackingAction.hh"

class StackingAction : public G4UserStackingAction{
    public:
        StackingAction();
        ~StackingAction() override;
        virtual ClassificationOfNewTrack ClassifyNewTrack(const G4Track* track) override;
}