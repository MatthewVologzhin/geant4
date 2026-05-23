#pragma once
#include <G4UserRunAction.hh>
#include <G4AccumulableManager.hh>
#include <G4AnalysisManager.hh>
#include <G4AccValue.hh>
#include <G4Event.hh>
#include <G4Step.hh>
#include <G4Types.hh>
#include <G4Accumulable.hh>
#include <G4VModularPhysicsList.hh>
#include <filesystem>
#include <vector>
#include <ctime>
#include <set>
#include "PhysicsList.hh"
#include "DetectorConstruction.hh"
#include "AccString.hh"
#include "AccMax.hh"
#include "AccSet.hh"

class RunAction : public G4UserRunAction{
public:
	RunAction();
	~RunAction() override;
	/* Starting user actions in the Runs */
	void BeginOfRunAction(const G4Run*) override;
	/* Final user actions in the Runs */
	void EndOfRunAction(const G4Run*) override;
	/* Creating of Ntuple */
	void Book();
	/* Get Initial energy & Initial particle name */
	void GetParticleINFO(const G4Event*);
	/* Get Names: Volume, Particle, Process */
	void GetParticleINFO(const G4Step*);
	/* Get electron INFO */
	void GetElectronINFO(const G4Track*);
	/* Get gamma INFO */
	void GetGammaINFO(const G4Track*);
	/* Output in Master Thread */
	void Output();
	/* Set initial Root file name */
	void SetHistoFileName(std::string name);
	/* Creating new Root file with specific name */
	void CreateRootFile();
	
private:
	AccString fIonName;
	G4AccValue<G4int> fIonsNb, fElectronsNb, fGammasNb, fOuterIonsNb, fOuterElectronsNb, fIonSteps, fSecondaryElectronsNb;
	G4AccValue<G4double> fEnergy, fDepositedEnergy;
	AccMax fMaxSecondaryElectronsEnergy;
	AccSet fProcessesSet;
	G4AccumulableManager* fpAccumulableManager;
	G4AnalysisManager* fpAnalysisManager;
	std::string path;	
	G4String physicsListName;
	const G4VUserDetectorConstruction* constDet;
	const DetectorConstruction* det;
	G4String cylinderLength;
	G4String cylinderRadius;
	G4double LETvalue;
	
	std::filesystem::path newFileName, interFileName, oldFileName;
	
	G4int fVerboseLevel;
	
	std::chrono::time_point<std::chrono::system_clock> fStartTime, fEndTime;
};
