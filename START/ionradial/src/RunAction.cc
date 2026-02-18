#include <G4UserRunAction.hh>
#include <G4AnalysisManager.hh>
#include <G4AccumulableManager.hh>
#include <G4SystemOfUnits.hh>
#include <G4Event.hh>
#include <G4ParticleDefinition.hh>
#include <G4SystemOfUnits.hh>
#include <G4UnitsTable.hh>
#include <G4Types.hh>
#include <G4Accumulable.hh>
#include <G4RunManager.hh>
#include <filesystem>
#include <algorithm>
#include "TTree.h"
#include "TFile.h"
#include "TH1F.h"
#include "TNtuple.h"
#include "TROOT.h"
#include "RunAction.hh"

RunAction::RunAction(){
	fpAccumulableManager = G4AccumulableManager::Instance();
	fpAccumulableManager->SetVerboseLevel(0);
	fpAccumulableManager->Register(fEnergy);
	fpAccumulableManager->Register(fDepositedEnergy);
	fpAccumulableManager->Register(fIonsNb);
	fpAccumulableManager->Register(fElectronsNb);
	fpAccumulableManager->Register(fSecondaryElectronsNb);
	fpAccumulableManager->Register(&fMaxSecondaryElectronsEnergy);
	fpAccumulableManager->Register(fGammasNb);
	fpAccumulableManager->Register(fOuterIonsNb);
	fpAccumulableManager->Register(fOuterElectronsNb);
	fpAccumulableManager->Register(fIonSteps);
	fpAccumulableManager->Register(&fIonName);
	fpAccumulableManager->Register(&fProcessesSet);
	
	fpAnalysisManager = G4AnalysisManager::Instance();
	fpAnalysisManager->SetFirstNtupleId(1);
	fpAnalysisManager->SetFirstHistoId(1);
};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

void RunAction::BeginOfRunAction(const G4Run* run){	
	
	fStartTime = std::chrono::system_clock::now();
	
	fVerboseLevel = G4RunManager::GetRunManager()->GetVerboseLevel();
	
	fEnergy.Reset();
	fDepositedEnergy.Reset();
	fIonsNb.Reset();
	fElectronsNb.Reset();
	fGammasNb.Reset();
	fOuterIonsNb.Reset();
	fOuterElectronsNb.Reset();
	fSecondaryElectronsNb.Reset();
	fMaxSecondaryElectronsEnergy.Reset();
	fProcessesSet.Reset();
	fIonSteps.Reset();
	
	Book();
};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

RunAction::~RunAction(){};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

void RunAction::EndOfRunAction(const G4Run* run){
	fEndTime = std::chrono::system_clock::now();
	
	fpAccumulableManager = G4AccumulableManager::Instance();
    fpAccumulableManager->Merge();

    if (not(IsMaster())){			
		constDet = G4RunManager::GetRunManager()->GetUserDetectorConstruction();
		det = dynamic_cast<const DetectorConstruction*>(constDet);
		cylinderLength = G4BestUnit(det->GetCylinderLength()/mm, "Length");
		cylinderRadius = G4BestUnit(det->GetCylinderRadius()/mm, "Length");
		LETvalue	   = (fDepositedEnergy.GetValue()/keV)/(det->GetCylinderLength()/um)/fIonsNb.GetValue();
		fpAnalysisManager->FillNtupleDColumn(3, 0, fIonsNb.GetValue());
		fpAnalysisManager->FillNtupleDColumn(3, 1, fEnergy.GetValue());
		fpAnalysisManager->FillNtupleDColumn(3, 2, LETvalue);
		fpAnalysisManager->AddNtupleRow(3);
	}
	
    CreateRootFile(); 
    Output();
};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

void RunAction::Book(){
	SetHistoFileName("ionradial");
	fpAnalysisManager->SetVerboseLevel(0);
	fpAnalysisManager->SetActivation(true);
	
	fpAnalysisManager->SetNtupleMerging(true);

	//1
	fpAnalysisManager->CreateNtuple("doseData", "Energy deposition of all particles");     
    fpAnalysisManager->CreateNtupleDColumn("energy");
	fpAnalysisManager->CreateNtupleDColumn("radius");
	fpAnalysisManager->CreateNtupleDColumn("x");
	fpAnalysisManager->CreateNtupleDColumn("y");
	fpAnalysisManager->CreateNtupleDColumn("z");
	//2
	fpAnalysisManager->CreateNtuple("ionEDep", "Ions energy deposition along axis");     
    fpAnalysisManager->CreateNtupleDColumn("energy");
	fpAnalysisManager->CreateNtupleDColumn("length");
	fpAnalysisManager->CreateNtupleDColumn("radius");
	//3
	fpAnalysisManager->CreateNtuple("ionInitialData", "Ions initial data");     
    fpAnalysisManager->CreateNtupleDColumn("number");
    fpAnalysisManager->CreateNtupleDColumn("energy"); 
    fpAnalysisManager->CreateNtupleDColumn("LET"); 
    //4 (for Integer)
    fpAnalysisManager->CreateNtuple("particlesNb", "Number of diverse particles");
    fpAnalysisManager->CreateNtupleIColumn("number");
    //5
    fpAnalysisManager->CreateNtuple("totalLET", "Total LET");
    fpAnalysisManager->CreateNtupleDColumn("LET");
    
    

	fpAnalysisManager->FinishNtuple();
	
	fpAnalysisManager->CreateH1("test", "", 1, 0, 1);
	fpAnalysisManager->OpenFile();
};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

void RunAction::SetHistoFileName(std::string fileName){
	
	fpAnalysisManager->SetDefaultFileType("root");
	std::string folderName = "root";
	if (!std::filesystem::exists(folderName)){
		std::filesystem::create_directory(folderName);
	}
	path = folderName + "/" + fileName;
	fpAnalysisManager->SetFileName(path);
};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

void RunAction::CreateRootFile(){
	G4String physName = "";
	// Get PhysicsList name
	const G4VUserPhysicsList* constPhysList = G4RunManager::GetRunManager()->GetUserPhysicsList();
	const PhysicsList* physList = dynamic_cast<const PhysicsList*>(constPhysList);
	physicsListName = 	physList->GetPhysicsListName();	
	if (physicsListName == "G4EmDNAPhysics"){
		physName = "";
	} else if (physicsListName == "G4EmDNAPhysics_option2"){
		physName = "DNA2";
	} else if (physicsListName == "G4EmDNAPhysics_option4"){
		physName = "DNA4";
	} else if (physicsListName == "G4EmDNAPhysics_option6"){
		physName = "DNA6";
	} else if (physicsListName == "G4EmDNAPhysics_option8"){
		physName = "DNA8";
	} else if (physicsListName == "G4EmStandardPhysics_option4"){
		physName = "S4";
	} else {
		physName = "No option";
	}
	
	
	if (fpAnalysisManager->IsActive()){		
		fpAnalysisManager->Write();
		fpAnalysisManager->CloseFile();
		fpAnalysisManager->Clear();
		if (IsMaster()){		
			std::string energyName = G4BestUnit(fEnergy.GetValue()/fIonsNb.GetValue(), "Energy");
			std::string ionName = fIonName.GetValue();
			std::replace(energyName.begin(), energyName.end(), ' ', '_');
			oldFileName = path + ".root";                                                                                                                       
			newFileName = path + "-" + ionName + "-" + energyName + "-" + physName + ".root";
			interFileName = path + "-" + energyName + "-inter" + ".root";
			if (!std::filesystem::exists(newFileName)){
					std::filesystem::rename(oldFileName, newFileName);
			} else {
					std::filesystem::rename(newFileName, interFileName);
					const std::string command = "hadd " + newFileName.string() + ' ' + interFileName.string() + ' ' + oldFileName.string();
					G4int result = system(command.c_str());
					std::filesystem::remove(interFileName);
			}
			std::filesystem::path fileName = newFileName;
			if (std::filesystem::exists(fileName)) {
				std::string command = "root -b -q 'scripts/simplifyRoot.C(\"" + fileName.string() + "\")'";
				G4int result = system(command.c_str());
				if (result == 0) {
					G4cout << "Successfully completed simplification of Root-file: " << fileName.string() << G4endl;
				} else {
					G4cout << "Warning during execution of simplification script on Root-file: " << fileName.string() << ". Return code: " << result << G4endl;
				}
			} else {
				G4cout << "Error: File " << fileName.string() << " does not exist." << G4endl;
			}
		}
	}
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

void RunAction::GetParticleINFO(const G4Event* event){
	/* Fetch initial energy & Initial particle name*/
	fEnergy += event->GetPrimaryVertex()->GetPrimary()->GetKineticEnergy();
	fIonName.SetValue(event->GetPrimaryVertex()->GetPrimary()->GetParticleDefinition()->GetParticleName());
	
	/* Ions number calculation */
	++fIonsNb;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

void RunAction::GetParticleINFO(const G4Step* step){
	/* Get some names of modeling */
	G4Track* track = step->GetTrack();
	G4String volumeName   = step->GetTrack()->GetVolume()->GetName();
	auto pdgCode = track->GetDefinition()->GetPDGEncoding();
	G4String particleName = step->GetTrack()->GetParticleDefinition()->GetParticleName();
	auto process = step->GetPreStepPoint()->GetProcessDefinedStep();
	G4String processName;
	if (process){
		processName = process->GetProcessName();
		fProcessesSet.Insert(processName);	
	}
	auto pPostVolume = step->GetPostStepPoint()->GetPhysicalVolume();
	if (pPostVolume == nullptr){
		if (abs(pdgCode) == 11 /* electrons & positrons */){
			++fOuterElectronsNb;
		} else if (pdgCode == 22 /* gammas*/){
		} else {
			++fOuterIonsNb;
		}
		
	}
	
	/* Change IT if you need to calculate eDep also for ion */
	G4double eDep = step->GetTotalEnergyDeposit();
	if (eDep > 0){
		fDepositedEnergy += eDep;
	}
	
	if (not(abs(pdgCode) == 11 or pdgCode == 22)){
		++fIonSteps;
	}
	
}


void RunAction::GetElectronINFO(const G4Track* track){
	++fElectronsNb;
	auto pdgCode =track->GetParticleDefinition()->GetPDGEncoding();
	if (pdgCode == 11) {
            // Получаем процесс, который создал этот трек
            const G4VProcess* creatorProcess = track->GetCreatorProcess();
            if (creatorProcess) {
                std::string procName = creatorProcess->GetProcessName();
                if (procName == "ionIoni" || procName == "hIoni" || 
					procName == "alpha_G4DNAIonisation" ||
					procName == "alpha+_G4DNAIonisation" ||
					procName == "helium_G4DNAIonisation" ||
					procName == "proton_G4DNAIonisation"||
					procName == "hydrogen_G4DNAIonisation"||
					procName == "GenericIon_G4DNAIonisation") {
						++fSecondaryElectronsNb;
						auto kinEnergy =track->GetKineticEnergy();
						if (fMaxSecondaryElectronsEnergy.GetValue() < kinEnergy){
							fMaxSecondaryElectronsEnergy.SetValue(kinEnergy);
						}
                }
            }
        }
	
}

void RunAction::GetGammaINFO(const G4Track* track){
	++fGammasNb;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

void RunAction::Output(){
	/* List of process that uses in modeling */
	
	// Output 
	if (IsMaster() and fVerboseLevel > -1){
		G4String elapsedTime = 		G4BestUnit((fEndTime - fStartTime).count(), "Time");
		G4String energyName  = 		G4BestUnit(fEnergy.GetValue()/fIonsNb.GetValue(), "Energy");
		G4String ionName = 			fIonName.GetValue();
		G4String ionsNb =	 		std::to_string(fIonsNb.GetValue());
		G4String ionSteps =	 		std::to_string(fIonSteps.GetValue()/fIonsNb.GetValue());
		G4String electronsNb =		std::to_string(fElectronsNb.GetValue()/fIonsNb.GetValue());
		G4String secondaryElectronsNb =	std::to_string(fSecondaryElectronsNb.GetValue()/fIonsNb.GetValue());
		G4String maxSecondaryElectronEnergy = G4BestUnit(fMaxSecondaryElectronsEnergy.GetValue(), "Energy");
		G4String gammasNb =			std::to_string(fGammasNb.GetValue());
		G4String outIonsNb = 		std::to_string(fOuterIonsNb.GetValue());
		G4String outElectronsNb = 	std::to_string(fOuterElectronsNb.GetValue());
		constDet = G4RunManager::GetRunManager()->GetUserDetectorConstruction();
		det = dynamic_cast<const DetectorConstruction*>(constDet);
		cylinderLength = G4BestUnit(det->GetCylinderLength()/mm, "Length");
		cylinderRadius = G4BestUnit(det->GetCylinderRadius()/mm, "Length");
		LETvalue	   = (fDepositedEnergy.GetValue()/keV)/(det->GetCylinderLength()/um)/fIonsNb.GetValue();
		
		G4cout << "---------========= Output of the Run =========---------" << "\n"
			   << "Physics list:\t\t" 	<< physicsListName					<< "\n"
			   << "Cylinder's length:\t"<< cylinderLength 					<< "\n"
			   << "Cylinder's radius:\t"<< cylinderRadius					<< "\n"
			   << "Ion name: \t\t" 		<< ionName		 					<< "\n"
			   << "Initial energy: \t" 	<< energyName 						<< "\n"
			   << "Ions number: \t\t"	<< ionsNb 							<< "\n" 
			   << "Fleet out ions:\t\t" << outIonsNb						<< "\n"
			   << "Mean ion steps:\t\t" << ionSteps							<< "\n"
			   << "Mean e- number:\t\t"	<< electronsNb	 					<< "\n"
			   << "Mean 2'ry e- number:\t"<< secondaryElectronsNb	 		<< "\n"
			   << "Max 2'ry e- energy:\t"<< maxSecondaryElectronEnergy		<< "\n"
			   << "Fleet out e-:\t\t"   << outElectronsNb					<< "\n"
			   << "Gamma number:\t\t"	<< gammasNb		 					<< "\n"
			   << "Elapsed time:\t\t" 	<< elapsedTime 						<< "\n"
			   << "LET: \t\t\t" 		<< LETvalue << " " << "keV/um"		<< "\n"
			   << "Processes list:\t\t";
			   for (const auto& processName : fProcessesSet.GetValue()){
			       G4cout << processName << " ";
			   } G4cout << G4endl;	
			   G4cout << "---------=====================================---------" << G4endl;
	}
	
}


