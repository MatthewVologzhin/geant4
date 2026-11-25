// DetectorConstruction.hh
#pragma once

#include "G4VUserDetectorConstruction.hh"
#include "globals.hh"
#include "G4SystemOfUnits.hh"
#include "G4ThreeVector.hh"
#include <memory>
#include <vector>

class G4Box;
class G4LogicalVolume;
class G4VPhysicalVolume;
class G4Material;
class DetectorMessenger;
class MaterialMessenger;
class BioDetectorConstruction;

struct Layer {
    G4String name;
    G4Material* material{nullptr};
    G4ThreeVector size;
    G4ThreeVector pos;
};

class DetectorConstruction final : public G4VUserDetectorConstruction
{
public:
    DetectorConstruction();
    ~DetectorConstruction() override;

    DetectorConstruction(const DetectorConstruction&) = delete;
    DetectorConstruction& operator=(const DetectorConstruction&) = delete;

    G4VPhysicalVolume* Construct() override;
    void ConstructSDandField() override;

    void SetWorldMaterial(const G4String&);
    void SetWorldSizeZ(G4double);
    void SetWorldSizeXY(G4double);
    void SetPreBioCut(G4double val) noexcept { fPreBioCut = val; }
    void AddLayer(G4ThreeVector, G4String, G4ThreeVector);

    [[nodiscard]] const BioDetectorConstruction* GetBioDetector() const noexcept { return fBioDetector.get(); }
    [[nodiscard]] G4double GetWorldSizeX() const noexcept { return fWorldSizeX; }
    [[nodiscard]] G4double GetWorldSizeY() const noexcept { return fWorldSizeY; }
    [[nodiscard]] G4double GetWorldSizeZ() const noexcept { return fWorldSizeZ; }
    [[nodiscard]] G4double GetPreBioCut() const noexcept { return fPreBioCut; }
    [[nodiscard]] G4LogicalVolume* GetWorldLogicalVolume() const noexcept { return fLogicWorld; }

private:
    void DefineMaterials();
    void ComputeGeomParameters();
    void ChangeGeometry();

    G4double fWorldSizeX{60.0 * CLHEP::mm};
    G4double fWorldSizeY{60.0 * CLHEP::mm};
    G4double fWorldSizeZ{100.0 * CLHEP::mm};
    G4double fPreBioCut{10.0 * CLHEP::um};

    G4Material*        fWorldMaterial{nullptr};
    G4Box*             fSolidWorld{nullptr};
    G4LogicalVolume*   fLogicWorld{nullptr};
    G4VPhysicalVolume* fPhysiWorld{nullptr};

    std::unique_ptr<BioDetectorConstruction> fBioDetector;
    std::unique_ptr<DetectorMessenger>       fDetectorMessenger;
    std::unique_ptr<MaterialMessenger>       fMaterialMessenger;
    std::vector<Layer>                       fLayers;
};