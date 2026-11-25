#pragma once

#include "G4UserTrackingAction.hh"

class G4Track;

class TrackingAction final : public G4UserTrackingAction
{
public:
    TrackingAction() = default;
    ~TrackingAction() override = default;

    void PreUserTrackingAction(const G4Track* track) override;
    void PostUserTrackingAction(const G4Track* track) override;
};