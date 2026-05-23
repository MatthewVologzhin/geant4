
#include <G4Types.hh>
#include <G4RunManagerFactory.hh>
#include <G4UImanager.hh>
#include <G4UIExecutive.hh>
#include <G4VisExecutive.hh>
#include "ActionInitialization.hh"
#include "DetectorConstruction.hh"
#include "PhysicsList.hh"


//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

int main(int argc,char** argv)
{
    long seed = ((long) time(NULL));
    long enterseed = ((long) time(NULL)) + seed;

    G4Random::setTheSeed(enterseed);
    G4Random::showEngineStatus();

    std::unique_ptr<G4RunManager> pRunManager(G4RunManagerFactory::CreateRunManager());
    pRunManager->SetNumberOfThreads(4);//by default

    DetectorConstruction* pDetector = new DetectorConstruction();
    pRunManager->SetUserInitialization(pDetector);

    PhysicsList* pPhysList = new PhysicsList;
    pRunManager->SetUserInitialization(pPhysList);
    pRunManager->SetUserInitialization(new ActionInitialization(pDetector));
    
    pRunManager->Initialize();

    G4UImanager* pUImanager = G4UImanager::GetUIpointer();

    if (argc > 1)
    {
// batch mode
        G4String command = "/control/execute ";
        G4String fileName = argv[1];
        pUImanager->ApplyCommand(command+fileName);
    }
    else
    {
// interactive mode
	std::unique_ptr<G4VisManager> pVisuManager(new G4VisExecutive);
	pVisuManager->Initialize();
        std::unique_ptr<G4UIExecutive> pUi(new G4UIExecutive(argc, argv));
        pUImanager->ApplyCommand("/control/execute vis.mac");
        pUi->SessionStart();
    }
    return 0;
}
