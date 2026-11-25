// EventAction.cc
#include "EventAction.hh"
#include "DetectorConstruction.hh"
#include "BioDetectorConstruction.hh"
#include "RunAction.hh"
#include "CellHit.hh"
#include "BioTrackInformation.hh"
#include "ScoringProfile.hh"

#include "G4Event.hh"
#include "G4SDManager.hh"
#include "G4HCofThisEvent.hh"
#include "G4AnalysisManager.hh"
#include <array>
#include <vector>

namespace {

// Column indices of the active scoring profile, resolved once (they are
// deterministic for the run). A value of -1 means the owning group is disabled.
struct ColumnIndexCache {
    int eventID{-1};
    int edepTotal{-1}, edepEM{-1}, edepHad{-1};
    std::array<int, static_cast<size_t>(FragmentSpecies::kCount)> edepSpecies{};
    std::array<int, static_cast<size_t>(FragmentSpecies::kCount)> ionsSpecies{};
    int ionsTotal{-1}, ionsEM{-1}, ionsHad{-1}, hitTargetCount{-1};
    int ionsNonDNA{-1}, ionsNonDNADeuteron{-1}, ionsNonDNATriton{-1};
    std::array<int, static_cast<size_t>(SubChannel::kCount)> edepSub{};
    std::array<int, static_cast<size_t>(SubChannel::kCount)> ionsSub{};
};

const ColumnIndexCache& Columns()
{
    static const ColumnIndexCache c = [] {
        ColumnIndexCache c;
        c.eventID    = ActiveColumnIndex("EventID");
        c.edepTotal  = ActiveColumnIndex("Edep_Total");
        c.edepEM     = ActiveColumnIndex("Edep_EM");
        c.edepHad    = ActiveColumnIndex("Edep_HadTotal");
        c.edepSpecies[static_cast<size_t>(FragmentSpecies::Proton)]   = ActiveColumnIndex("Edep_Proton");
        c.edepSpecies[static_cast<size_t>(FragmentSpecies::Deuteron)] = ActiveColumnIndex("Edep_Deuteron");
        c.edepSpecies[static_cast<size_t>(FragmentSpecies::Triton)]   = ActiveColumnIndex("Edep_Triton");
        c.edepSpecies[static_cast<size_t>(FragmentSpecies::Helium3)]  = ActiveColumnIndex("Edep_Helium3");
        c.edepSpecies[static_cast<size_t>(FragmentSpecies::Alpha)]    = ActiveColumnIndex("Edep_Alpha");
        c.edepSpecies[static_cast<size_t>(FragmentSpecies::HeavyIon)] = ActiveColumnIndex("Edep_HeavyIon");
        c.edepSpecies[static_cast<size_t>(FragmentSpecies::Neutron)]  = ActiveColumnIndex("Edep_Neutron");
        c.edepSpecies[static_cast<size_t>(FragmentSpecies::Other)]    = ActiveColumnIndex("Edep_Other");
        c.ionsTotal  = ActiveColumnIndex("Ions_Total");
        c.ionsEM     = ActiveColumnIndex("Ions_EM");
        c.ionsHad    = ActiveColumnIndex("Ions_HadTotal");
        c.ionsSpecies[static_cast<size_t>(FragmentSpecies::Proton)]   = ActiveColumnIndex("Ions_Proton");
        c.ionsSpecies[static_cast<size_t>(FragmentSpecies::Deuteron)] = ActiveColumnIndex("Ions_Deuteron");
        c.ionsSpecies[static_cast<size_t>(FragmentSpecies::Triton)]   = ActiveColumnIndex("Ions_Triton");
        c.ionsSpecies[static_cast<size_t>(FragmentSpecies::Helium3)]  = ActiveColumnIndex("Ions_Helium3");
        c.ionsSpecies[static_cast<size_t>(FragmentSpecies::Alpha)]    = ActiveColumnIndex("Ions_Alpha");
        c.ionsSpecies[static_cast<size_t>(FragmentSpecies::HeavyIon)] = ActiveColumnIndex("Ions_HeavyIon");
        c.ionsSpecies[static_cast<size_t>(FragmentSpecies::Neutron)]  = ActiveColumnIndex("Ions_Neutron");
        c.ionsSpecies[static_cast<size_t>(FragmentSpecies::Other)]    = ActiveColumnIndex("Ions_Other");
        c.hitTargetCount = ActiveColumnIndex("HitTargetCount");
        c.ionsNonDNA         = ActiveColumnIndex("Ions_NonDNA");
        c.ionsNonDNADeuteron = ActiveColumnIndex("Ions_NonDNA_Deuteron");
        c.ionsNonDNATriton   = ActiveColumnIndex("Ions_NonDNA_Triton");
        c.edepSub[static_cast<size_t>(SubChannel::PrimaryIonDirect)]        = ActiveColumnIndex("Edep_PrimaryIonDirect");
        c.edepSub[static_cast<size_t>(SubChannel::PrimaryDeltaRays)]        = ActiveColumnIndex("Edep_PrimaryDeltaRays");
        c.edepSub[static_cast<size_t>(SubChannel::HadronicFragmentsDirect)] = ActiveColumnIndex("Edep_HadronicFragmentsDirect");
        c.edepSub[static_cast<size_t>(SubChannel::HadronicDeltaRays)]       = ActiveColumnIndex("Edep_HadronicDeltaRays");
        c.ionsSub[static_cast<size_t>(SubChannel::PrimaryIonDirect)]        = ActiveColumnIndex("Ions_PrimaryIonDirect");
        c.ionsSub[static_cast<size_t>(SubChannel::PrimaryDeltaRays)]        = ActiveColumnIndex("Ions_PrimaryDeltaRays");
        c.ionsSub[static_cast<size_t>(SubChannel::HadronicFragmentsDirect)] = ActiveColumnIndex("Ions_HadronicFragmentsDirect");
        c.ionsSub[static_cast<size_t>(SubChannel::HadronicDeltaRays)]       = ActiveColumnIndex("Ions_HadronicDeltaRays");
        return c;
    }();
    return c;
}

}  // namespace

EventAction::EventAction(const DetectorConstruction* det, RunAction* runAction)
    : fDetector(det), fRunAction(runAction) {}

void EventAction::BeginOfEventAction(const G4Event*)
{
    if (fCollID < 0 && fDetector && fDetector->GetBioDetector()) {
        const auto hcName = fDetector->GetBioDetector()->GetHitsCollectionName();
        fCollID = G4SDManager::GetSDMpointer()->GetCollectionID(hcName);
    }
}

void EventAction::EndOfEventAction(const G4Event* event)
{
    G4HCofThisEvent* hce = event->GetHCofThisEvent();
    if (!hce || fCollID < 0) return;

    auto* hitsColl = static_cast<CellHitsCollection*>(hce->GetHC(fCollID));
    const G4int nHits = hitsColl ? static_cast<G4int>(hitsColl->entries()) : 0;

    G4double totalEdepEM{0.0}, totalEdepHad{0.0};
    G4int totalIonsEM{0}, totalIonsHad{0}, totalIons{0};
    G4int totalIonsNonDNA{0};

    std::array<G4double, static_cast<size_t>(FragmentSpecies::kCount)> eBySpecies{};
    std::array<G4int,    static_cast<size_t>(FragmentSpecies::kCount)> iBySpecies{};
    std::array<G4int,    static_cast<size_t>(FragmentSpecies::kCount)> nonDnaBySpecies{};

    std::array<G4double, static_cast<size_t>(SubChannel::kCount)> eBySub{};
    std::array<G4int,    static_cast<size_t>(SubChannel::kCount)> iBySub{};

    std::vector<G4int> clusterSizeEM, clusterSizeHad, clusterSizeTotal;
    if (nHits > 0) {
        clusterSizeEM.reserve(nHits);
        clusterSizeHad.reserve(nHits);
        clusterSizeTotal.reserve(nHits);

        for (G4int i = 0; i < nHits; ++i) {
            const CellHit* hit = (*hitsColl)[i];

            const G4double eEM  = hit->GetEdepEM();
            const G4double eHad = hit->GetEdepHad();
            const G4int iEM     = hit->GetIonsEM();
            const G4int iHad    = hit->GetIonsHad();
            const G4int iTot    = hit->GetIonsTotal();

            totalEdepEM  += eEM;
            totalEdepHad += eHad;
            totalIonsEM  += iEM;
            totalIonsHad += iHad;
            totalIons    += iTot;

            if (iHad > 0) clusterSizeHad.push_back(iHad);
            if (iEM > 0)  clusterSizeEM.push_back(iEM);
            if (iTot > 0) clusterSizeTotal.push_back(iTot);

            totalIonsNonDNA += hit->GetIonsNonDNA();

            for (size_t s = 0; s < static_cast<size_t>(FragmentSpecies::kCount); ++s) {
                const auto spec = static_cast<FragmentSpecies>(s);
                eBySpecies[s] += hit->GetEdepSpecies(spec);
                iBySpecies[s] += hit->GetIonsSpecies(spec);
                nonDnaBySpecies[s] += hit->GetIonsNonDNASpecies(spec);
            }

            for (size_t c = 0; c < static_cast<size_t>(SubChannel::kCount); ++c) {
                const auto sub = static_cast<SubChannel>(c);
                eBySub[c] += hit->GetEdepSubChannel(sub);
                iBySub[c] += hit->GetIonsSubChannel(sub);
            }
        }
    }

    if (fRunAction) {
        fRunAction->AddEventEdep(totalEdepEM, totalEdepHad);
    }

    auto* analysis = G4AnalysisManager::Instance();
    const auto& c = Columns();

    auto fillI = [&](int idx, G4int v) { if (idx >= 0) analysis->FillNtupleIColumn(idx, v); };
    auto fillD = [&](int idx, G4double v) { if (idx >= 0) analysis->FillNtupleDColumn(idx, v); };

    fillI(c.eventID, event->GetEventID());
    fillD(c.edepTotal, totalEdepEM + totalEdepHad);
    fillD(c.edepEM, totalEdepEM);
    fillD(c.edepHad, totalEdepHad);

    fillD(c.edepSpecies[static_cast<size_t>(FragmentSpecies::Proton)],   eBySpecies[static_cast<size_t>(FragmentSpecies::Proton)]);
    fillD(c.edepSpecies[static_cast<size_t>(FragmentSpecies::Deuteron)], eBySpecies[static_cast<size_t>(FragmentSpecies::Deuteron)]);
    fillD(c.edepSpecies[static_cast<size_t>(FragmentSpecies::Triton)],   eBySpecies[static_cast<size_t>(FragmentSpecies::Triton)]);
    fillD(c.edepSpecies[static_cast<size_t>(FragmentSpecies::Helium3)],  eBySpecies[static_cast<size_t>(FragmentSpecies::Helium3)]);
    fillD(c.edepSpecies[static_cast<size_t>(FragmentSpecies::Alpha)],    eBySpecies[static_cast<size_t>(FragmentSpecies::Alpha)]);
    fillD(c.edepSpecies[static_cast<size_t>(FragmentSpecies::HeavyIon)], eBySpecies[static_cast<size_t>(FragmentSpecies::HeavyIon)]);
    fillD(c.edepSpecies[static_cast<size_t>(FragmentSpecies::Neutron)],  eBySpecies[static_cast<size_t>(FragmentSpecies::Neutron)]);
    fillD(c.edepSpecies[static_cast<size_t>(FragmentSpecies::Other)],
          eBySpecies[static_cast<size_t>(FragmentSpecies::Other)] +
          eBySpecies[static_cast<size_t>(FragmentSpecies::DeexcitationGamma)]);

    fillI(c.ionsTotal, totalIons);
    fillI(c.ionsEM, totalIonsEM);
    fillI(c.ionsHad, totalIonsHad);

    fillI(c.ionsSpecies[static_cast<size_t>(FragmentSpecies::Proton)],   iBySpecies[static_cast<size_t>(FragmentSpecies::Proton)]);
    fillI(c.ionsSpecies[static_cast<size_t>(FragmentSpecies::Deuteron)], iBySpecies[static_cast<size_t>(FragmentSpecies::Deuteron)]);
    fillI(c.ionsSpecies[static_cast<size_t>(FragmentSpecies::Triton)],   iBySpecies[static_cast<size_t>(FragmentSpecies::Triton)]);
    fillI(c.ionsSpecies[static_cast<size_t>(FragmentSpecies::Helium3)],  iBySpecies[static_cast<size_t>(FragmentSpecies::Helium3)]);
    fillI(c.ionsSpecies[static_cast<size_t>(FragmentSpecies::Alpha)],    iBySpecies[static_cast<size_t>(FragmentSpecies::Alpha)]);
    fillI(c.ionsSpecies[static_cast<size_t>(FragmentSpecies::HeavyIon)], iBySpecies[static_cast<size_t>(FragmentSpecies::HeavyIon)]);
    fillI(c.ionsSpecies[static_cast<size_t>(FragmentSpecies::Neutron)],  iBySpecies[static_cast<size_t>(FragmentSpecies::Neutron)]);
    fillI(c.ionsSpecies[static_cast<size_t>(FragmentSpecies::Other)],
          iBySpecies[static_cast<size_t>(FragmentSpecies::Other)] +
          iBySpecies[static_cast<size_t>(FragmentSpecies::DeexcitationGamma)]);

    fillI(c.hitTargetCount, nHits);
    fillI(c.ionsNonDNA, totalIonsNonDNA);
    fillI(c.ionsNonDNADeuteron, nonDnaBySpecies[static_cast<size_t>(FragmentSpecies::Deuteron)]);
    fillI(c.ionsNonDNATriton, nonDnaBySpecies[static_cast<size_t>(FragmentSpecies::Triton)]);

    fillD(c.edepSub[static_cast<size_t>(SubChannel::PrimaryIonDirect)],        eBySub[static_cast<size_t>(SubChannel::PrimaryIonDirect)]);
    fillD(c.edepSub[static_cast<size_t>(SubChannel::PrimaryDeltaRays)],        eBySub[static_cast<size_t>(SubChannel::PrimaryDeltaRays)]);
    fillD(c.edepSub[static_cast<size_t>(SubChannel::HadronicFragmentsDirect)], eBySub[static_cast<size_t>(SubChannel::HadronicFragmentsDirect)]);
    fillD(c.edepSub[static_cast<size_t>(SubChannel::HadronicDeltaRays)],       eBySub[static_cast<size_t>(SubChannel::HadronicDeltaRays)]);
    fillI(c.ionsSub[static_cast<size_t>(SubChannel::PrimaryIonDirect)],        iBySub[static_cast<size_t>(SubChannel::PrimaryIonDirect)]);
    fillI(c.ionsSub[static_cast<size_t>(SubChannel::PrimaryDeltaRays)],        iBySub[static_cast<size_t>(SubChannel::PrimaryDeltaRays)]);
    fillI(c.ionsSub[static_cast<size_t>(SubChannel::HadronicFragmentsDirect)], iBySub[static_cast<size_t>(SubChannel::HadronicFragmentsDirect)]);
    fillI(c.ionsSub[static_cast<size_t>(SubChannel::HadronicDeltaRays)],       iBySub[static_cast<size_t>(SubChannel::HadronicDeltaRays)]);

    analysis->AddNtupleRow();

    const int hIcsdEvent = ActiveHistIndex("ICSD_Event_Total");
    if (hIcsdEvent >= 0 && totalIons > 0) analysis->FillH1(hIcsdEvent, totalIons);
    const int hIcsdEM  = ActiveHistIndex("ICSD_Target_EM");
    const int hIcsdHad = ActiveHistIndex("ICSD_Target_Had");
    const int hIcsdTot = ActiveHistIndex("ICSD_Target_Total");
    if (hIcsdEM >= 0)  for (const auto sz : clusterSizeEM)    analysis->FillH1(hIcsdEM, sz);
    if (hIcsdHad >= 0) for (const auto sz : clusterSizeHad)   analysis->FillH1(hIcsdHad, sz);
    if (hIcsdTot >= 0) for (const auto sz : clusterSizeTotal) analysis->FillH1(hIcsdTot, sz);
}
