// BioDetectorConstruction.hh
#pragma once

#include "G4UImessenger.hh"
#include "G4UIdirectory.hh"
#include "G4UIcommand.hh"
#include "G4UIcmdWithAString.hh"
#include "G4UIcmdWithADoubleAndUnit.hh"
#include "G4ThreeVector.hh"
#include "globals.hh"
#include "TargetDef.hh"
#include <CLHEP/Units/SystemOfUnits.h>
#include <optional>
#include <memory>
#include <string_view>
#include <vector>

class G4LogicalVolume;
class G4Material;
class G4NistManager;
class DetectorConstruction;

struct BioLayerConfig {
    std::optional<G4Material*> material;
    std::optional<G4String> physics;
    std::optional<long long int> nCells;
    std::optional<G4ThreeVector> pos;
    std::optional<G4ThreeVector> size;

    [[nodiscard]] bool IsReady() const noexcept {
        return material.has_value() && nCells.has_value() && pos.has_value() && size.has_value();
    }
};

struct CellParameters {
    std::optional<double> radius_nucleus;
    std::optional<double> radius_cytoplasm;
    std::optional<double> radius_membrane;
    std::optional<double> density_nucleus;
    std::optional<double> density_cytoplasm;
    std::optional<double> density_membrane;
    std::optional<G4Material*> material_nucleus;
    std::optional<G4Material*> material_cytoplasm;
    std::optional<G4Material*> material_membrane;
    std::optional<G4String> physics;

    [[nodiscard]] bool IsReady() const noexcept {
        return radius_nucleus.has_value() && radius_cytoplasm.has_value() && radius_membrane.has_value() &&
               density_nucleus.has_value() && density_cytoplasm.has_value() && density_membrane.has_value() &&
               material_nucleus.has_value() && material_cytoplasm.has_value() && material_membrane.has_value();
    }
};

class BioDetectorConstruction final : public G4UImessenger
{
public:
    explicit BioDetectorConstruction(DetectorConstruction*);
    ~BioDetectorConstruction() override;

    void Construct(G4LogicalVolume* logicalMotherVolume);
    void SetNewValue(G4UIcommand*, G4String) override;

    [[nodiscard]] bool HasBioLayer() const noexcept { return fBioLayerConfig.IsReady(); }
    [[nodiscard]] DetectorRole GetScoreTarget() const noexcept { return fScoreTarget; }
    [[nodiscard]] std::vector<G4LogicalVolume*> GetScoringLogicalVolumes() const;
    [[nodiscard]] G4String GetHitsCollectionName() const noexcept;

    [[nodiscard]] size_t GetActualCellCount() const noexcept { return fActualCellCount; }
    [[nodiscard]] G4double GetScoringMass() const;

    [[nodiscard]] G4LogicalVolume* GetNucleusLogicalVolume() const noexcept { return fLogicNucleus; }
    [[nodiscard]] G4LogicalVolume* GetCytoplasmLogicalVolume() const noexcept { return fLogicCytoplasm; }
    [[nodiscard]] G4LogicalVolume* GetCellLogicalVolume() const noexcept { return fLogicCell; }
    [[nodiscard]] G4LogicalVolume* GetMediumLogicalVolume() const noexcept { return fLogicMedium; }

private:
    void SetCellGeometryCmd(std::string_view);
    void SetCellDensityCmd(std::string_view);
    void SetCellMaterialCmd(std::string_view);
    void CreateBioLayerCmd(std::string_view);
    void SetBioLayerSizeCmd(std::string_view);
    void SetBioLayerPosCmd(std::string_view);
    void SetPhysicsBioLayer(std::string_view);
    void SetPhysicsCell(std::string_view);
    void SetScoreTargetCmd(std::string_view);
    void SetScoreProfileCmd(std::string_view);

    CellParameters fCellParameters;
    BioLayerConfig fBioLayerConfig;
    DetectorRole   fScoreTarget{DetectorRole::Nucleus};

    G4double fCellCut{10.0 * CLHEP::nm};
    G4double fBioLayerCut{0.5 * CLHEP::um};
    size_t   fActualCellCount{0};

    std::unique_ptr<G4UIdirectory>             fBioDirectory, fCellDirectory, fBioLayerDirectory, fScoreDirectory;
    std::unique_ptr<G4UIcommand>               fCellGeometryCmd, fCellMaterialsCmd, fCellDensitiesCmd, fPhysicsCellCmd;
    std::unique_ptr<G4UIcmdWithADoubleAndUnit> fCellCutCmd, fBioLayerCutCmd;
    std::unique_ptr<G4UIcommand>               fCreateBioLayerCmd, fSizeBioLayerCmd, fPosBioLayerCmd, fPhysicsBioLayerCmd;
    std::unique_ptr<G4UIcmdWithAString>        fScoreTargetCmd, fScoreProfileCmd;
    std::unique_ptr<G4UIcommand>               fScoreListCmd;

    G4NistManager* fNist{nullptr};
    DetectorConstruction* fDetector{nullptr};
    G4LogicalVolume* fLogicNucleus{nullptr};
    G4LogicalVolume* fLogicCytoplasm{nullptr};
    G4LogicalVolume* fLogicCell{nullptr};
    G4LogicalVolume* fLogicMedium{nullptr};
};