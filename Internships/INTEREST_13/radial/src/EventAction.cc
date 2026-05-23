#include "EventAction.hh"
#include "Run.hh"
#include "G4RunManager.hh"

void EventAction::BeginOfEventAction(const G4Event* pEvent){
    fLength = .0;
    fEdep = .0;
};

void EventAction::EndOfEventAction(const G4Event* pEvent){
    Run *pRun = (Run*)G4RunManager::GetRunManager()->GetNonConstCurrentRun();
    if (fLength > 0){
        pRun->AddEventData(fLength, fEdep);
    }
    pRun->RecordEndOfEvent();
};