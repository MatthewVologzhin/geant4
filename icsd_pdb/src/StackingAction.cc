#include "StackingAction.hh"
#include "G4Track.hh"
#include "G4Gamma.hh"

StackingAction::StackingAction() : G4UserStackingAction()
{};

StackingAction::~StackingAction()
{};

G4ClassificationOfNewTrack StackingAction::ClassifyNewTrack(const G4Track* track){
    if (track->GetDefinition() == G4Gamma::GammaDefinition() && track->GetParentID() > 0){
        return fKill;
    }
    return fUrgent;
}