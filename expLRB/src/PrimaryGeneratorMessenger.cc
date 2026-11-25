#include "PrimaryGeneratorMessenger.hh"

#include "PrimaryGeneratorAction.hh"

#include "G4UIcmdWithADouble.hh"
#include "G4UIcmdWithoutParameter.hh"
#include "G4UIdirectory.hh"

PrimaryGeneratorMessenger::PrimaryGeneratorMessenger(PrimaryGeneratorAction* Gun) : fAction(Gun)
{
  fGunDir = new G4UIdirectory("/expLRB/gun/");
  fGunDir->SetGuidance("gun control");

  fDefaultCmd = new G4UIcmdWithoutParameter("/expLRB/gun/setDefault", this);
  fDefaultCmd->SetGuidance("set/reset the kinematic defined in PrimaryGenerator");
  fDefaultCmd->AvailableForStates(G4State_PreInit, G4State_Idle);

  fBeamRadius = new G4UIcommand("/expLRB/gun/setRadius", this);
  fBeamRadius->SetGuidance("Size of the beam");
  G4UIparameter* paramRadius = new G4UIparameter("radius", 'd', false);
  paramRadius->SetDefaultValue("0.");
  paramRadius->SetParameterRange("radius>=0.");
  fBeamRadius->SetParameter(paramRadius);
  G4UIparameter* paramRadiusUnit = new G4UIparameter("radiusUnit", 's', false);
  paramRadiusUnit->SetDefaultValue("mm");
  fBeamRadius->SetParameter(paramRadiusUnit);
  fBeamRadius->AvailableForStates(G4State_Idle);
}

PrimaryGeneratorMessenger::~PrimaryGeneratorMessenger()
{
  delete fDefaultCmd;
  delete fBeamRadius;
  delete fGunDir;
}

void PrimaryGeneratorMessenger::SetNewValue(G4UIcommand* command, G4String newValue)
{
  if (command == fDefaultCmd) {
    fAction->SetDefaultKinematic();
  }

  if (command == fBeamRadius) {
    std::istringstream is(newValue);

    G4double radius;
    G4String radiusUnit;
    is >> radius >> radiusUnit;
    G4double radiusScale = G4UIcommand::ValueOf(radiusUnit.c_str());

    fAction->SetBeamRadius(radius*radiusScale);
  }
}
