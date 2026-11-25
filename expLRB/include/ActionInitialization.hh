// ActionInitialization.hh
#pragma once

#include "G4VUserActionInitialization.hh"

class DetectorConstruction;

class ActionInitialization final : public G4VUserActionInitialization
{
public:
    explicit ActionInitialization(DetectorConstruction* det);
    ~ActionInitialization() override = default;

    void BuildForMaster() const override;
    void Build() const override;

private:
    DetectorConstruction* fDetector{nullptr};
};