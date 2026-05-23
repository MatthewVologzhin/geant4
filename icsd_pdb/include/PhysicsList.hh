#pragma once

#include <G4UIcommand.hh>
#include <G4UIcmdWithAString.hh>
#include <G4UImessenger.hh>
#include <G4UIdirectory.hh>
#include <G4VModularPhysicsList.hh>

class G4VPhysicsConstructor;

class PhysicsList : public G4VModularPhysicsList, public G4UImessenger{
public:
	explicit PhysicsList();
	~PhysicsList() override = default;
	
	void SetNewValue(G4UIcommand* command, G4String value) override;

	void RegisterPhysicsConstructor(const G4String& name);
	
	G4String GetPhysicsListName() const {return fPhysName;};
	
private:
	G4VPhysicsConstructor* fpEmPhysicsList = nullptr;
	G4String fPhysName;
	G4UIdirectory* fpPhysDir;

	G4UIcmdWithAString* fpPhysicsUI = nullptr;
};
