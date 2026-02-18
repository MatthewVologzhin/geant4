#include <G4GenericMessenger.hh>
#include <G4UIdirectory.hh>
#include <G4UIcommand.hh>
#include <G4SystemOfUnits.hh>
#include <G4UIcmdWithADoubleAndUnit.hh>
#include <G4NistManager.hh>
#include <G4Box.hh>
#include <G4Orb.hh>
#include <G4Tubs.hh>
#include <G4LogicalVolume.hh>
#include <G4VPhysicalVolume.hh>
#include <G4PVPlacement.hh>
#include <G4ThreeVector.hh>
#include <G4GeometryManager.hh>
#include <G4Colour.hh>
#include <G4VisAttributes.hh>
#include <G4RunManager.hh>
#include <G4UnitsTable.hh>
#include <G4UserLimits.hh>
#include <G4Region.hh>
#include <G4ProductionCuts.hh>
#include <G4RegionStore.hh>
#include <cmath>
#include "globals.hh"
#include "DetectorConstruction.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

DetectorConstruction::DetectorConstruction(){
	fpWorldDir = new G4UIdirectory("/detector/");
	fpCylinderRadiusCmd = new G4UIcmdWithADoubleAndUnit("/detector/setCylinderRadius", this);
	fpCylinderLengthCmd = new G4UIcmdWithADoubleAndUnit("/detector/setCylinderLength", this);
	fpWorldZSizeCmd = new G4UIcmdWithADoubleAndUnit("/detector/setWorldZSize", this);

	fpWorldDir->SetGuidance("Control parameters of system");
	fpCylinderRadiusCmd->SetDefaultUnit("nm");
	fpCylinderLengthCmd->SetDefaultUnit("nm");
	fpWorldZSizeCmd->SetDefaultUnit("nm");
	
	fpNistManager = G4NistManager::Instance();
	fpCylinderMaterial = fpNistManager->FindOrBuildMaterial("G4_WATER");
	fpWorldMaterial = fpNistManager->FindOrBuildMaterial("G4_Galactic");
	
	fCylinderRadius = 5*CLHEP::mm;
	fCylinderLength = 5000*CLHEP::nm;
	fWorldZSize = 60*CLHEP::nm;

	fpCylinderVis = new G4VisAttributes(G4Colour(0.,0.05,1.,0.23));
	fpCylinderVis->SetVisibility(true);
	fpCylinderVis->SetForceSolid(true);
	/* Set a grid on the geometry object */
	//fpObjVis->SetForceWireframe(true);
	fpWorldVis = new G4VisAttributes(G4Colour(0.,0.,1.,0.3));
	fpWorldVis->SetVisibility(false);
	/* Set a grid/solid on the world */
	//fpWorldVis->SetForceWireframe(true);
	//fpWorldVis->SetForceSolid(true);
};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

G4VPhysicalVolume* DetectorConstruction::Construct(){
	fpSolidWorld = new G4Box("solidWorld", 
							fCylinderRadius,
							fCylinderRadius,
							fWorldZSize);
	fpLogicWorld = new G4LogicalVolume(fpSolidWorld,
									   fpWorldMaterial,
									   "logicWorld");
	fpLogicWorld->SetVisAttributes(fpWorldVis);
	fpWorld = new G4PVPlacement(0,
								G4ThreeVector(),
								fpLogicWorld,
								"world",
								0,
								false,
								0);
	
	fpSolidCylinder = new G4Tubs("solidCylinder",
								 0, fCylinderRadius,
								 fWorldZSize/2,
								 0, 2*M_PI);
	fpLogicCylinder = new G4LogicalVolume(fpSolidCylinder,
										  fpCylinderMaterial,
										  "logicCylinder");
	fpLogicCylinder->SetVisAttributes(fpCylinderVis);
	fpCylinder = new G4PVPlacement(nullptr,
								   G4ThreeVector(),
								   fpLogicCylinder,
								   "cylinder",
								   fpLogicWorld,
							       false,
							       0);
	
	/* Detailed modeling description of the target object (cylinder) */
	/*fpLogicCylinder->SetUserLimits(new G4UserLimits(1*nm, DBL_MAX, DBL_MAX, 11*eV));
	if (!fpTargetRegion){
		fpTargetRegion = new G4Region("regionTarget");
		G4ProductionCuts* cuts = new G4ProductionCuts();
	  	cuts->SetProductionCut(0.7 * mm, "gamma");
 		cuts->SetProductionCut(0.7 * mm, "e-");
		cuts->SetProductionCut(0.7 * mm, "e+");
	  	cuts->SetProductionCut(1 * micrometer, "proton");
  		fpTargetRegion->SetProductionCuts(cuts);
		fpTargetRegion->AddRootLogicalVolume(fpLogicCylinder);
	}*/
		
	return fpWorld;
}; 

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

void DetectorConstruction::SetNewValue(G4UIcommand* command, G4String value){
	if (command == fpCylinderRadiusCmd){
		SetCylinderRadius(fpCylinderRadiusCmd->GetNewDoubleValue(value)); 
	} else if (command == fpWorldZSizeCmd){
		SetWorldZSize(fpWorldZSizeCmd->GetNewDoubleValue(value));
	} else if (command == fpCylinderLengthCmd){
		SetCylinderLength(fpCylinderLengthCmd->GetNewDoubleValue(value));
	} else {
		G4cout << "========= " <<
				  "Error in local command" <<
				  " =========" << G4endl;
	};
};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

void DetectorConstruction::SetCylinderRadius(G4double value){
	G4GeometryManager::GetInstance()->OpenGeometry();
	
	fCylinderRadius = value;
	G4cout << "========= " <<
			  "The cyclinder radius has been modified: " <<
			  G4BestUnit(fCylinderRadius, "Length") <<
			  "=========" << G4endl;
			  
	G4RunManager::GetRunManager()->GeometryHasBeenModified();
};

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo....

void DetectorConstruction::SetWorldZSize(G4double value){
	G4GeometryManager::GetInstance()->OpenGeometry();
	
	fWorldZSize = value;

	G4cout << "========= " <<
				  "The world Z size has been modified: " <<
				  G4BestUnit(fWorldZSize, "Length") <<
				  "=========" << G4endl;
				  
	G4RunManager::GetRunManager()->GeometryHasBeenModified();
};

void DetectorConstruction::SetCylinderLength(G4double value){
	G4GeometryManager::GetInstance()->OpenGeometry();
	
	fCylinderLength = value;
	if (fCylinderLength > fWorldZSize){
                fWorldZSize = fCylinderLength;
                G4cout << "========= "
                       << "The radius size has been modified: "
                       << G4BestUnit(fCylinderLength, "Length")
                       << "=========" << G4endl;
    }	
	G4cout << "========= " 
		   << "The cylinder length has been modified: " 
		   << G4BestUnit(fCylinderLength, "Length") 
		   << "=========" << G4endl;
				  
	G4RunManager::GetRunManager()->GeometryHasBeenModified();
};
