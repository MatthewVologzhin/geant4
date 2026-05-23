#pragma once
#include "G4UserEventAction.hh"
#include "globals.hh"

class EventAction : public G4UserEventAction{
    public:
        EventAction() : fLength(0.), fEdep(0.){};
        void BeginOfEventAction(const G4Event*) override;
        void EndOfEventAction(const G4Event*) override;
        void AddData(G4double len, G4double edep){fLength += len; fEdep += edep;};

    private:
        G4double fLength;
        G4double fEdep;
};