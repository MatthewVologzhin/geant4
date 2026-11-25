
#include <G4EmDNAPhysics.hh>
#include <G4EmDNAPhysics_option2.hh>
#include <G4EmDNAPhysics_option4.hh>
#include <G4EmDNAPhysics_option6.hh>
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
						   SetEnergyRange(10*eV, 1*GeV);
	G4EmParameters* param = G4EmParameters::Instance();
	param->SetMinEnergy(10 * eV);
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
			      
	G4VPhysicsConstructor* pNewPhysics = nullptr;
	if (name == fPhysName){
		return;}
       if (name == "G4EmDNAPhysics"){
		
		pNewPhysics = new G4EmDNAPhysics(verboseLevel);
	} else if (name == "G4EmDNAPhysics_option2"){
		pNewPhysics = new G4EmDNAPhysics_option2(verboseLevel);
	} else if (name == "G4EmDNAPhysics_option4"){
		pNewPhysics = new G4EmDNAPhysics_option4(verboseLevel);
	} else if (name == "G4EmDNAPhysics_option6"){
		pNewPhysics = new G4EmDNAPhysics_option6(verboseLevel);
	} else if (name == "G4EmDNAPhysics_option8"){
		pNewPhysics = new G4EmDNAPhysics_option8(verboseLevel);
	} else if (name == "G4EmStandardPhysics_option0"){
		pNewPhysics = new G4EmStandardPhysics(verboseLevel);
	} else if (name == "G4EmStandardPhysics_option4"){
		pNewPhysics = new G4EmStandardPhysics_option4(verboseLevel);
	} else if (name == "G4EmLivermorePhysics"){
		pNewPhysics = new G4EmLivermorePhysics(verboseLevel);
		
	} else {
		G4cout << "PhysicsList::RegisterPhysicsConstructor: <" << name << ">"
	   << " fails - name is not defined" << G4endl;
	return;
  }
  
  if (pNewPhysics){
      ReplacePhysics(pNewPhysics);
      fPhysName = name;
      fpEmPhysicsList = pNewPhysics;
  }
};

void PhysicsList::SetNewValue(G4UIcommand* command, G4String value){
	if (command == fpPhysicsUI){
		RegisterPhysicsConstructor(value);
	}
};
