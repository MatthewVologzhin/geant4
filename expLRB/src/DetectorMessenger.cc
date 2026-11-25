// DetectorMessenger.cc
#include "DetectorMessenger.hh"
#include "DetectorConstruction.hh"

#include "G4UIcmdWithADoubleAndUnit.hh"
#include "G4UIcmdWithAString.hh"
#include "G4UIdirectory.hh"
#include "G4UIcommand.hh"
#include "G4UIparameter.hh"
#include <sstream>

DetectorMessenger::DetectorMessenger(DetectorConstruction* Det) : fDetector(Det)
{
    fTestemDir = std::make_unique<G4UIdirectory>("/expLRB/");
    fTestemDir->SetGuidance("UI commands specific to this example.");

    fDetDir = std::make_unique<G4UIdirectory>("/expLRB/det/", false);
    fDetDir->SetGuidance("detector construction commands");

    fWorldMaterCmd = std::make_unique<G4UIcmdWithAString>("/expLRB/det/setWorldMat", this);
    fWorldMaterCmd->SetGuidance("Select Material of the World.");
    fWorldMaterCmd->SetParameterName("wchoice", false);
    fWorldMaterCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
    fWorldMaterCmd->SetToBeBroadcasted(false);

    fWorldZCmd = std::make_unique<G4UIcmdWithADoubleAndUnit>("/expLRB/det/setWorldZ", this);
    fWorldZCmd->SetGuidance("Set Z size of the World");
    fWorldZCmd->SetParameterName("WSizeZ", false);
    fWorldZCmd->SetRange("WSizeZ>0.");
    fWorldZCmd->SetUnitCategory("Length");
    fWorldZCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
    fWorldZCmd->SetToBeBroadcasted(false);

    fWorldXYCmd = std::make_unique<G4UIcmdWithADoubleAndUnit>("/expLRB/det/setWorldXY", this);
    fWorldXYCmd->SetGuidance("Set sizeXY of the World");
    fWorldXYCmd->SetParameterName("WSizeXY", false);
    fWorldXYCmd->SetRange("WSizeXY>0.");
    fWorldXYCmd->SetUnitCategory("Length");
    fWorldXYCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
    fWorldXYCmd->SetToBeBroadcasted(false);

    fPreBioCutCmd = std::make_unique<G4UIcmdWithADoubleAndUnit>("/expLRB/det/setPreBioCut", this);
    fPreBioCutCmd->SetGuidance("Set production cut on the last passive layer before biosample (default: 10 um).");
    fPreBioCutCmd->SetParameterName("preBioCut", false);
    fPreBioCutCmd->SetRange("preBioCut>=0.");
    fPreBioCutCmd->SetUnitCategory("Length");
    fPreBioCutCmd->SetDefaultUnit("um");
    fPreBioCutCmd->AvailableForStates(G4State_PreInit);
    fPreBioCutCmd->SetToBeBroadcasted(false);

    fAddLayerCmd = std::make_unique<G4UIcommand>("/expLRB/det/addLayer", this);
    fAddLayerCmd->SetGuidance("Add a new layer with specific sizes and material");
    fAddLayerCmd->SetGuidance("Format: dx dy dz <unit> material x y z <unit>");
    fAddLayerCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

    fAddLayerCmd->SetParameter(new G4UIparameter("dx", 'd', false));
    fAddLayerCmd->SetParameter(new G4UIparameter("dy", 'd', false));
    fAddLayerCmd->SetParameter(new G4UIparameter("dz", 'd', false));
    auto* dUnit = new G4UIparameter("dUnit", 's', false);
    dUnit->SetDefaultValue("mm");
    fAddLayerCmd->SetParameter(dUnit);
    fAddLayerCmd->SetParameter(new G4UIparameter("layerMaterial", 's', false));
    fAddLayerCmd->SetParameter(new G4UIparameter("posX", 'd', false));
    fAddLayerCmd->SetParameter(new G4UIparameter("posY", 'd', false));
    fAddLayerCmd->SetParameter(new G4UIparameter("posZ", 'd', false));
    auto* posUnit = new G4UIparameter("posUnit", 's', false);
    posUnit->SetDefaultValue("mm");
    fAddLayerCmd->SetParameter(posUnit);
}

DetectorMessenger::~DetectorMessenger() = default;

void DetectorMessenger::SetNewValue(G4UIcommand* command, G4String newValue)
{
    if (command == fWorldMaterCmd.get()) {
        fDetector->SetWorldMaterial(newValue);
    } else if (command == fWorldZCmd.get()) {
        fDetector->SetWorldSizeZ(fWorldZCmd->GetNewDoubleValue(newValue));
    } else if (command == fWorldXYCmd.get()) {
        fDetector->SetWorldSizeXY(fWorldXYCmd->GetNewDoubleValue(newValue));
    } else if (command == fPreBioCutCmd.get()) {
        fDetector->SetPreBioCut(fPreBioCutCmd->GetNewDoubleValue(newValue));
    } else if (command == fAddLayerCmd.get()) {
        std::istringstream is(newValue);

        G4double sizeX{0.}, sizeY{0.}, sizeZ{0.};
        G4String sizeUnit, materialName, posUnit;
        G4double posX{0.}, posY{0.}, posZ{0.};
        is >> sizeX >> sizeY >> sizeZ >> sizeUnit
           >> materialName >> posX >> posY >> posZ >> posUnit;

        const G4double sizeScale = G4UIcommand::ValueOf(sizeUnit.c_str());
        const G4double posScale  = G4UIcommand::ValueOf(posUnit.c_str());

        fDetector->AddLayer(
            G4ThreeVector(sizeScale * sizeX, sizeScale * sizeY, sizeScale * sizeZ),
            materialName,
            G4ThreeVector(posScale * posX, posScale * posY, posScale * posZ));
    }
}