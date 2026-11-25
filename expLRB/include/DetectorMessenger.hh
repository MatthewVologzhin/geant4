// DetectorMessenger.hh
#pragma once

#include "G4UImessenger.hh"
#include "globals.hh"
#include <memory>

class DetectorConstruction;
class G4UIdirectory;
class G4UIcmdWithAString;
class G4UIcmdWithADoubleAndUnit;
class G4UIcommand;

class DetectorMessenger final : public G4UImessenger
{
public:
    explicit DetectorMessenger(DetectorConstruction*);
    ~DetectorMessenger() override;

    void SetNewValue(G4UIcommand*, G4String) override;

private:
    DetectorConstruction* fDetector{nullptr};

    std::unique_ptr<G4UIdirectory>             fTestemDir;
    std::unique_ptr<G4UIdirectory>             fDetDir;
    std::unique_ptr<G4UIcmdWithAString>        fWorldMaterCmd;
    std::unique_ptr<G4UIcmdWithADoubleAndUnit> fWorldZCmd;
    std::unique_ptr<G4UIcmdWithADoubleAndUnit> fWorldXYCmd;
    std::unique_ptr<G4UIcmdWithADoubleAndUnit> fPreBioCutCmd;
    std::unique_ptr<G4UIcommand>               fAddLayerCmd;
};