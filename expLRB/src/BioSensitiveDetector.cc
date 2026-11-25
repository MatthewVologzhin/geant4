// BioSensitiveDetector.cc
#include "BioSensitiveDetector.hh"
#include "BioDetectorConstruction.hh"
#include "BioTrackInformation.hh"
#include "ScoringProfile.hh"

#include "G4VProcess.hh"
#include "G4TouchableHistory.hh"
#include "G4EmProcessSubType.hh"
#include "G4LowEnergyEmProcessSubType.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4SDManager.hh"
#include "G4AnalysisManager.hh"
#include "G4LogicalVolume.hh"
#include "G4Box.hh"
#include "G4SystemOfUnits.hh"
#include <cmath>

BioSensitiveDetector::BioSensitiveDetector(const G4String& name,
                                           const G4String& hitsCollectionName,
                                           DetectorRole role,
                                           const BioDetectorConstruction* bioDet)
    : G4VSensitiveDetector(name), fRole(role), fBioDet(bioDet)
{
    collectionName.insert(hitsCollectionName);
    fHitMap.reserve(512);
}

void BioSensitiveDetector::Initialize(G4HCofThisEvent* hce)
{
    fHitsCollection = new CellHitsCollection(SensitiveDetectorName, collectionName[0]);

    if (fCollectionID < 0) {
        fCollectionID = G4SDManager::GetSDMpointer()->GetCollectionID(collectionName[0]);
    }
    hce->AddHitsCollection(fCollectionID, fHitsCollection);
    fHitMap.clear();
}

G4int BioSensitiveDetector::ResolveVolumeIndex(const G4TouchableHistory* touchable) const noexcept
{
    if (!touchable) return -1;
    if (fRole == DetectorRole::BioMedium) return 0;

    const G4LogicalVolume* cellLV = fBioDet ? fBioDet->GetCellLogicalVolume() : nullptr;
    const G4int depth = touchable->GetHistoryDepth();

    if (cellLV) {
        if (fRole == DetectorRole::Nucleus && depth >= 2 &&
            touchable->GetVolume(2)->GetLogicalVolume() == cellLV) {
            return touchable->GetCopyNumber(2);
        }
        if (fRole == DetectorRole::Cytoplasm && depth >= 1 &&
            touchable->GetVolume(1)->GetLogicalVolume() == cellLV) {
            return touchable->GetCopyNumber(1);
        }
        if (fRole == DetectorRole::Membrane &&
            touchable->GetVolume(0)->GetLogicalVolume() == cellLV) {
            return touchable->GetCopyNumber(0);
        }
        for (G4int d = 0; d <= depth; ++d) {
            const auto* phys = touchable->GetVolume(d);
            if (phys && phys->GetLogicalVolume() == cellLV) {
                return touchable->GetCopyNumber(d);
            }
        }
    }
    return touchable->GetCopyNumber(0);
}

G4int BioSensitiveDetector::CalculateIonizations(const G4VProcess* proc) const noexcept
{
    if (!proc) return 0;
    switch (proc->GetProcessSubType()) {
        case fLowEnergyIonisation:          return 1;
        case fLowEnergyDoubleIonisation:    return 2;
        case fLowEnergyTripleIonisation:    return 3;
        case fLowEnergyQuadrupleIonisation: return 4;
        default:                            return 0;
    }
}

G4int BioSensitiveDetector::CalculateNonDNAIonizations(const G4VProcess* proc) const noexcept
{
    if (proc && proc->GetProcessSubType() == fIonisation) {
        return 1;
    }
    return 0;
}

void BioSensitiveDetector::RecordEntranceSpectrum(const G4Step* step,
                                                  const G4TouchableHistory* touchable) const
{
    const auto* track = step->GetTrack();
    const auto* info  = static_cast<const BioTrackInformation*>(track->GetUserInformation());

    const G4int pdg = track->GetDefinition()->GetPDGEncoding();
    const bool isElectron = (std::abs(pdg) == 11);
    if (pdg == 22) return;
    if (!isElectron && (!info || !info->IsHadronic())) return;

    const G4StepPoint* pre = step->GetPreStepPoint();
    const G4double ekinMeV = pre->GetKineticEnergy() / CLHEP::MeV;
    if (ekinMeV <= 0.0) return;

    if (fRole != DetectorRole::Nucleus && fBioDet) {
        const G4LogicalVolume* cellLV = fBioDet->GetCellLogicalVolume();
        const G4int depth = touchable->GetHistoryDepth();

        if (fRole == DetectorRole::BioMedium) {
            const auto* mediumLV = fBioDet->GetMediumLogicalVolume();
            if (!mediumLV) return;
            const auto* box = static_cast<const G4Box*>(mediumLV->GetSolid());
            const auto& transform = touchable->GetHistory()->GetTransform(0);
            const G4ThreeVector localPos = transform.TransformPoint(pre->GetPosition());
            constexpr G4double tol = 1.0e-5 * CLHEP::mm;
            const bool onOuterWall =
                (std::abs(std::abs(localPos.x()) - box->GetXHalfLength()) < tol) ||
                (std::abs(std::abs(localPos.y()) - box->GetYHalfLength()) < tol) ||
                (std::abs(std::abs(localPos.z()) - box->GetZHalfLength()) < tol);
            if (!onOuterWall) return;
        } else {
            if (fRole == DetectorRole::WholeCell &&
                pre->GetPhysicalVolume()->GetLogicalVolume() != cellLV) {
                return;
            }
            G4int cellDepth = -1;
            for (G4int d = 0; d <= depth; ++d) {
                if (touchable->GetVolume(d)->GetLogicalVolume() == cellLV) {
                    cellDepth = d;
                    break;
                }
            }
            if (cellDepth < 0) return;
            const auto& transform = touchable->GetHistory()->GetTransform(cellDepth);
            const G4ThreeVector localPos = transform.TransformPoint(pre->GetPosition());
            const G4ThreeVector localDir = transform.TransformAxis(pre->GetMomentumDirection());
            if (localPos.dot(localDir) >= 0.0) return;
        }
    }

    auto* ana = G4AnalysisManager::Instance();
    const auto& profile = GetActiveScoringProfile();

    if (isElectron) {
        if (profile.Has(ScoreGroup::ElectronSpectrum)) {
            const int idx = ActiveHistIndex("SpecIn_Electron");
            if (idx >= 0) ana->FillH1(idx, ekinMeV);
        }
        return;
    }

    if (!profile.Has(ScoreGroup::SpectraBySpecies)) return;

    const int iAll = ActiveHistIndex("SpecIn_AllHad");
    if (iAll >= 0) ana->FillH1(iAll, ekinMeV);

    auto fillBy = [&](const char* name) {
        const int idx = ActiveHistIndex(name);
        if (idx >= 0) ana->FillH1(idx, ekinMeV);
    };

    switch (info->GetSpecies()) {
        case FragmentSpecies::Proton:   fillBy("SpecIn_Proton"); break;
        case FragmentSpecies::Deuteron: fillBy("SpecIn_Deuteron"); break;
        case FragmentSpecies::Triton:   fillBy("SpecIn_Triton"); break;
        case FragmentSpecies::Helium3:  fillBy("SpecIn_He3"); break;
        case FragmentSpecies::Alpha:    fillBy("SpecIn_Alpha"); break;
        case FragmentSpecies::HeavyIon: {
            fillBy("SpecIn_Heavy");
            const G4int A = track->GetDefinition()->GetAtomicMass();
            if (A > 0) {
                const int idx = ActiveHistIndex("SpecIn_HeavyPerNucl");
                if (idx >= 0) ana->FillH1(idx, ekinMeV / static_cast<G4double>(A));
            }
            break;
        }
        case FragmentSpecies::Neutron:  fillBy("SpecIn_Neutron"); break;
        default: break;
    }
}

G4bool BioSensitiveDetector::ProcessHits(G4Step* step, G4TouchableHistory*)
{
    const auto* pre = step->GetPreStepPoint();
    const auto* touchable = static_cast<const G4TouchableHistory*>(pre->GetTouchable());

    if (pre->GetStepStatus() == fGeomBoundary) {
        RecordEntranceSpectrum(step, touchable);
    }

    const G4double edep = step->GetTotalEnergyDeposit();
    if (edep <= 0.0) return false;

    const G4int targetID = ResolveVolumeIndex(touchable);
    if (targetID < 0) return false;

    const auto* track = step->GetTrack();
    const auto* info  = static_cast<const BioTrackInformation*>(track->GetUserInformation());

    const OriginChannel orig   = info ? info->GetOrigin()  : OriginChannel::PrimaryEM;
    const FragmentSpecies spec = info ? info->GetSpecies() : FragmentSpecies::None;
    const SubChannel sub       = info ? info->GetSubChannel() : SubChannel::PrimaryDeltaRays;

    const G4VProcess* proc = step->GetPostStepPoint()->GetProcessDefinedStep();
    const G4int nIons = CalculateIonizations(proc);
    const G4int nStandardIons = CalculateNonDNAIonizations(proc);

    auto it = fHitMap.find(targetID);
    CellHit* hit = (it != fHitMap.end()) ? it->second : nullptr;

    if (!hit) {
        hit = new CellHit(targetID);
        fHitsCollection->insert(hit);
        fHitMap.emplace(targetID, hit);
    }

    hit->AddHit(orig, spec, sub, edep, nIons, nStandardIons);
    return true;
}

void BioSensitiveDetector::EndOfEvent(G4HCofThisEvent*) {}