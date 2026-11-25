// DetectorConstruction.cc
#include "DetectorConstruction.hh"
#include "DetectorMessenger.hh"
#include "MaterialMessenger.hh"
#include "BioDetectorConstruction.hh"
#include "BioSensitiveDetector.hh"

#include "G4Box.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4NistManager.hh"
#include "G4SDManager.hh"
#include "G4Region.hh"
#include "G4ProductionCuts.hh"
#include "G4SystemOfUnits.hh"
#include "G4VisAttributes.hh"

DetectorConstruction::DetectorConstruction()
{
    DefineMaterials();
    SetWorldMaterial("G4_Galactic");

    fDetectorMessenger = std::make_unique<DetectorMessenger>(this);
    fMaterialMessenger = std::make_unique<MaterialMessenger>(this);
    fBioDetector       = std::make_unique<BioDetectorConstruction>(this);
}

DetectorConstruction::~DetectorConstruction() = default;

void DetectorConstruction::DefineMaterials()
{
    G4NistManager::Instance()->FindOrBuildMaterial("G4_Galactic");
    G4NistManager::Instance()->FindOrBuildMaterial("G4_AIR");
    G4NistManager::Instance()->FindOrBuildMaterial("G4_WATER");
    G4NistManager::Instance()->FindOrBuildMaterial("G4_Ta");
    G4NistManager::Instance()->FindOrBuildMaterial("G4_Fe");
    G4NistManager::Instance()->FindOrBuildMaterial("G4_Al");
    G4NistManager::Instance()->FindOrBuildMaterial("G4_POLYETHYLENE");
}

void DetectorConstruction::AddLayer(G4ThreeVector sizeVector, G4String matName, G4ThreeVector posVector)
{
    auto* mat = G4NistManager::Instance()->FindOrBuildMaterial(matName);
    if (!mat) {
        G4ExceptionDescription ed;
        ed << "Material [" << matName << "] does not exist! Layer was not added.";
        G4Exception("DetectorConstruction::AddLayer", "MaterialNotFound", FatalException, ed);
        return;
    }
    fLayers.push_back({matName, mat, sizeVector, posVector});
}

G4VPhysicalVolume* DetectorConstruction::Construct()
{
    if (fPhysiWorld) return fPhysiWorld;

    fSolidWorld = new G4Box("World", 0.5 * fWorldSizeX, 0.5 * fWorldSizeY, 0.5 * fWorldSizeZ);
    fLogicWorld = new G4LogicalVolume(fSolidWorld, fWorldMaterial, "World");
    fLogicWorld->SetVisAttributes(G4VisAttributes::GetInvisible());

    fPhysiWorld = new G4PVPlacement(nullptr, G4ThreeVector(), fLogicWorld, "World", nullptr, false, 0, false);

    // Пассивные поглотители и фильтры
    for (size_t i = 0; i < fLayers.size(); ++i) {
        auto* solid = new G4Box("solid_layer_" + std::to_string(i),
                                0.5 * fLayers[i].size.x(), 0.5 * fLayers[i].size.y(), 0.5 * fLayers[i].size.z());
        auto* logic = new G4LogicalVolume(solid, fLayers[i].material, "logic_layer_" + fLayers[i].name);
        new G4PVPlacement(nullptr, fLayers[i].pos, logic, "phys_layer_" + fLayers[i].name,
                          fLogicWorld, false, static_cast<G4int>(i), false);

        // Последний слой перед биообразцом (стенка пробирки) получает отдельный cut
        // для корректного равновесия дельта-электронов без замедления счёта в передних фильтрах.
        if (i + 1 == fLayers.size() && fPreBioCut > 0.0) {
            auto* preBioRegion = new G4Region("PreBioRegion");
            logic->SetRegion(preBioRegion);
            preBioRegion->AddRootLogicalVolume(logic);
            auto* cuts = new G4ProductionCuts();
            cuts->SetProductionCut(fPreBioCut);
            preBioRegion->SetProductionCuts(cuts);
            G4cout << "### [DetectorConstruction] PreBioRegion cut set to "
                   << fPreBioCut / CLHEP::um << " um on layer: " << fLayers[i].name << G4endl;
        }
    }

    if (fBioDetector && fBioDetector->HasBioLayer()) {
        fBioDetector->Construct(fLogicWorld);
    }

    return fPhysiWorld;
}

void DetectorConstruction::ConstructSDandField()
{
    if (!fBioDetector) return;

    auto targetLVs = fBioDetector->GetScoringLogicalVolumes();
    if (targetLVs.empty()) return;

    auto* sdManager = G4SDManager::GetSDMpointer();
    const G4String sdName = "BioSD";
    const G4String hcName = fBioDetector->GetHitsCollectionName();
    const DetectorRole role = fBioDetector->GetScoreTarget();

    auto* bioSD = new BioSensitiveDetector(sdName, hcName, role, fBioDetector.get());
    sdManager->AddNewDetector(bioSD);

    for (auto* lv : targetLVs) {
        if (lv) SetSensitiveDetector(lv, bioSD);
    }
}

void DetectorConstruction::SetWorldMaterial(const G4String& materialChoice)
{
    auto* mat = G4NistManager::Instance()->FindOrBuildMaterial(materialChoice);
    if (mat && fWorldMaterial != mat) {
        fWorldMaterial = mat;
        if (fLogicWorld) fLogicWorld->SetMaterial(fWorldMaterial);
    }
}

void DetectorConstruction::SetWorldSizeZ(G4double val)
{
    fWorldSizeZ = val;
    ComputeGeomParameters();
}

void DetectorConstruction::SetWorldSizeXY(G4double val)
{
    fWorldSizeX = val;
    fWorldSizeY = val;
    ComputeGeomParameters();
}

void DetectorConstruction::ComputeGeomParameters()
{
    if (fPhysiWorld) ChangeGeometry();
}

void DetectorConstruction::ChangeGeometry()
{
    if (fSolidWorld) {
        fSolidWorld->SetXHalfLength(0.5 * fWorldSizeX);
        fSolidWorld->SetYHalfLength(0.5 * fWorldSizeY);
        fSolidWorld->SetZHalfLength(0.5 * fWorldSizeZ);
    }
}