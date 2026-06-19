#include "ActionInitialization.hh"
#include "DetectorConstruction.hh"
#include "PrimaryGeneratorAction.hh"
#include "PhysicsList.hh"

#include "G4RunManagerFactory.hh"
#include "G4Types.hh"
#include "G4UIExecutive.hh"
#include "G4UImanager.hh"
#include "G4UItcsh.hh"
#include "G4UIterminal.hh"
#include "G4VisExecutive.hh"

int main(int argc, char** argv)
{
const G4int fThreads = 40;

	long seed = ((long) time(NULL));
	long enterseed = ((long) time(NULL)) + seed;
	G4Random::setTheSeed(enterseed);
	G4Random::showEngineStatus();

	auto pRunManager = G4RunManagerFactory::CreateRunManager();
	pRunManager->SetNumberOfThreads(fThreads);
	
	pRunManager->SetUserInitialization(new DetectorConstruction());
	auto pPhysList = new PhysicsList();
	pRunManager->SetUserInitialization(pPhysList);
	auto pGenerator = new PrimaryGeneratorAction();
	pRunManager->SetUserInitialization(new ActionInitialization());
	
	G4UImanager* pUImanager = G4UImanager::GetUIpointer();
	
	if (argc > 1){
		G4String command = "/control/execute ";
		G4String fileName = argv[1];
		pUImanager->ApplyCommand(command+fileName);
	} else {
		G4VisManager* pVisManager = new G4VisExecutive;
		pVisManager->Initialize();
		G4UIExecutive* ui = new G4UIExecutive(argc, argv);
		pUImanager->ApplyCommand("/control/execute vis.mac");
		ui->SessionStart();
		delete ui;
		delete pVisManager;
	}
	
	delete pRunManager;
	
	return 0;

  return 0;
}
