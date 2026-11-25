// BioSensitiveDetector.hh
#pragma once

#include "G4VSensitiveDetector.hh"
#include "CellHit.hh"
#include "TargetDef.hh"
#include <unordered_map>

class G4Step;
class G4TouchableHistory;
class G4HCofThisEvent;
class G4VProcess;
class BioDetectorConstruction;

class BioSensitiveDetector final : public G4VSensitiveDetector
{
public:
    BioSensitiveDetector(const G4String& name,
                         const G4String& hitsCollectionName,
                         DetectorRole role,
                         const BioDetectorConstruction* bioDet);
    ~BioSensitiveDetector() override = default;

    void Initialize(G4HCofThisEvent* hce) override;
    G4bool ProcessHits(G4Step* step, G4TouchableHistory* touchable) override;
    void EndOfEvent(G4HCofThisEvent* hce) override;

private:
    [[nodiscard]] G4int CalculateIonizations(const G4VProcess* proc) const noexcept;
    [[nodiscard]] G4int CalculateNonDNAIonizations(const G4VProcess* proc) const noexcept;
    [[nodiscard]] G4int ResolveVolumeIndex(const G4TouchableHistory* touchable) const noexcept;
    void RecordEntranceSpectrum(const G4Step* step, const G4TouchableHistory* touchable) const;

    DetectorRole fRole;
    const BioDetectorConstruction* fBioDet{nullptr};
    G4int fCollectionID{-1};
    CellHitsCollection* fHitsCollection{nullptr};
    std::unordered_map<G4int, CellHit*> fHitMap;
};