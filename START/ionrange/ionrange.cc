#include <G4RunManagerFactory.hh>
#include <G4UImanager.hh>
#include <G4UIExecutive.hh>
#include <G4VisManager.hh>
#include <G4VisExecutive.hh>
#include "ActionInitialization.hh"
#include "PrimaryGeneratorAction.hh"
#include "DetectorConstruction.hh"
#include "PhysicsList.hh"

int main(int argc, char** argv){

	const G4int fThreads = 40;

	long seed = ((long) time(NULL));
	long enterseed = ((long) time(NULL)) + seed;
	G4Random::setTheSeed(enterseed);
	G4Random::showEngineStatus();

	auto pRunManager = G4RunManagerFactory::CreateRunManager();
	pRunManager->SetNumberOfThreads(fThreads);
	
	auto pPhysList = new PhysicsList();
	pRunManager->SetUserInitialization(pPhysList);
	auto pGenerator = new PrimaryGeneratorAction();
	pRunManager->SetUserInitialization(new DetectorConstruction());
	pRunManager->SetUserInitialization(new ActionInitialization());
	
	G4UImanager* pUImanager = G4UImanager::GetUIpointer();
	
	if (argc > 1){
		G4String command = "/control/execute ";
		G4String fileName = argv[1];
		pUImanager->ApplyCommand(command+fileName);
	} else {
		std::unique_ptr<G4VisManager> pVisManager(new G4VisExecutive());
		pVisManager->Initialize();
		std::unique_ptr<G4UIExecutive> pUI(new G4UIExecutive(argc, argv));
		pUImanager->ApplyCommand("/control/execute vis.mac");
		pUI->SessionStart();
	}
	
	return 0;
};
