#include "G4UserRunAction.hh"
#include "G4RunManager.hh"
#include "G4String.hh"
#include "G4Threading.hh" // <--- Для мьютексов
#include <G4VModularPhysicsList.hh>

class RunAction : public G4UserRunAction
{
  public:
    RunAction();
    virtual ~RunAction();

    virtual void BeginOfRunAction(const G4Run*);
    virtual void EndOfRunAction(const G4Run*);

  private:
    // Статическая переменная, общая для всех потоков (Master и Workers)
    static G4String fFinalFileName;
    // Мьютекс для безопасной записи
    static G4Mutex fMutex;
    std::string folder_name = "root";
    G4RunManager* pRunManager = G4RunManager::GetRunManager();
};
