#pragma once

#include "G4UserStackingAction.hh"

class StackingAction : public G4UserStackingAction{
    public:
        StackingAction();
        ~StackingAction() override;
        virtual G4ClassificationOfNewTrack ClassifyNewTrack(const G4Track* track) override;
};