
#include <G4EmDNAPhysics.hh>
#include <G4EmDNAPhysics_option1.hh>
#include <G4EmDNAPhysics_option2.hh>
#include <G4EmDNAPhysics_option3.hh>
#include <G4EmDNAPhysics_option4.hh>
#include <G4EmDNAPhysics_option5.hh>
#include <G4EmDNAPhysics_option6.hh>
#include <G4EmDNAPhysics_option7.hh>
#include <G4EmDNAPhysics_option8.hh>
#include <G4EmStandardPhysics.hh>
#include <G4EmStandardPhysics_option4.hh>
#include <G4EmLivermorePhysics.hh>
#include <G4SystemOfUnits.hh>
#include <G4ParticleDefinition.hh>
#include <G4EmParameters.hh>
#include "globals.hh"
#include "PhysicsList.hh"

PhysicsList::PhysicsList() : G4VModularPhysicsList(){
	/* Remove it to obtain more control in simulation
	 * using macrofiles */
	//G4double currentDefaultCut = 1. * CLHEP::um;
	//SetDefaultCutValue(currentDefaultCut);
	G4ProductionCutsTable::GetProductionCutsTable()->
						   SetEnergyRange(100*eV, 1*GeV);
	G4EmParameters* param = G4EmParameters::Instance();
	param->SetMinEnergy(100 * eV);
	param->SetMaxEnergy(1 * GeV);
	SetVerboseLevel(1);					   
	
	AddTransportation();
	//fpEmPhysicsList = new G4EmLivermorePhysics();
	fpEmPhysicsList = new G4EmDNAPhysics_option2(verboseLevel);
	RegisterPhysics(fpEmPhysicsList);
	
	fpPhysDir = new G4UIdirectory("/physics/");
	fpPhysDir->SetGuidance("Change paramaters of Physics List");
	fpPhysicsUI = new G4UIcmdWithAString("/physics/setPhysics", this);
	fpPhysicsUI->AvailableForStates(G4State_PreInit);
};

void PhysicsList::RegisterPhysicsConstructor(const G4String& name){
	if (verboseLevel > 0){
		G4cout << "========= Register Physics: " << 
			      name <<
			      "=========" << G4endl;}
	if (name == fPhysName){
		return;}
       if (name == "G4EmDNAPhysics"){
		delete fpEmPhysicsList;
		fpEmPhysicsList = new G4EmDNAPhysics(verboseLevel);
		fPhysName = name;
	} else if (name == "G4EmDNAPhysics_option1"){
		delete fpEmPhysicsList;
		fpEmPhysicsList = new G4EmDNAPhysics_option1(verboseLevel);
		fPhysName = name;
	} else if (name == "G4EmDNAPhysics_option2"){
		delete fpEmPhysicsList;
		fpEmPhysicsList = new G4EmDNAPhysics_option2(verboseLevel);
		fPhysName = name;
	} else if (name == "G4EmDNAPhysics_option3"){
		delete fpEmPhysicsList;
		fpEmPhysicsList = new G4EmDNAPhysics_option3(verboseLevel);
		fPhysName = name;
	} else if (name == "G4EmDNAPhysics_option4"){
		delete fpEmPhysicsList;
		fpEmPhysicsList = new G4EmDNAPhysics_option4(verboseLevel);
		fPhysName = name;
	} else if (name == "G4EmDNAPhysics_option5"){
		delete fpEmPhysicsList;
		fpEmPhysicsList = new G4EmDNAPhysics_option5(verboseLevel);
		fPhysName = name;
	} else if (name == "G4EmDNAPhysics_option6"){
		delete fpEmPhysicsList;
		fpEmPhysicsList = new G4EmDNAPhysics_option6(verboseLevel);
		fPhysName = name;
        } else if (name == "G4EmDNAPhysics_option7"){
		delete fpEmPhysicsList;
		fpEmPhysicsList = new G4EmDNAPhysics_option7(verboseLevel);
		fPhysName = name;
	} else if (name == "G4EmDNAPhysics_option8"){
		delete fpEmPhysicsList;
		fpEmPhysicsList = new G4EmDNAPhysics_option8(verboseLevel);
		fPhysName = name;
	} else if (name == "G4EmStandardPhysics_option0"){
		delete fpEmPhysicsList;
		fpEmPhysicsList = new G4EmStandardPhysics(verboseLevel);
		fPhysName = name;
	} else if (name == "G4EmStandardPhysics_option4"){
		delete fpEmPhysicsList;
		fpEmPhysicsList = new G4EmStandardPhysics_option4(verboseLevel);
		fPhysName = name;
	} else if (name == "G4EmLivermorePhysics"){
		delete fpEmPhysicsList;
		fpEmPhysicsList = new G4EmLivermorePhysics(verboseLevel);
		fPhysName = name;
	} else {
		G4cout << "PhysicsList::RegisterPhysicsConstructor: <" << name << ">"
	   << " fails - name is not defined" << G4endl;
  }
};

void PhysicsList::SetNewValue(G4UIcommand* command, G4String value){
	if (command == fpPhysicsUI){
		RegisterPhysicsConstructor(value);
	}
};
