#include "StackingAction.hh"

StackingAction::StackingAction() : G4UserStackingAction()
{};

StackingAction::~StackingAction()
{};

StackingAction::ClassifyNewTrack(const G4Track* track) override{
    if (track->GetDefinition() == G4Gamma::GammaDefinition() && track->GetParent() > 0){
        return fKill
    }
    return fUrgent;
}