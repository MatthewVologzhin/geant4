#include "TrackingAction.hh"
#include "BioTrackInformation.hh"
#include "ScoringProfile.hh"

#include "G4Track.hh"
#include "G4TrackingManager.hh"
#include "G4VProcess.hh"
#include "G4ParticleDefinition.hh"
#include "G4TrackVector.hh"
#include "G4AnalysisManager.hh"
#include "G4SystemOfUnits.hh"
#include <cstdlib>

namespace {
FragmentSpecies IdentifyFragment(const G4ParticleDefinition* def) {
    if (!def) return FragmentSpecies::Other;
    const G4String& name = def->GetParticleName();
    const G4int Z = def->GetAtomicNumber();

    if (name == "proton")   return FragmentSpecies::Proton;
    if (name == "deuteron") return FragmentSpecies::Deuteron;
    if (name == "triton")   return FragmentSpecies::Triton;
    if (name == "He3")      return FragmentSpecies::Helium3;
    if (name == "alpha")    return FragmentSpecies::Alpha;
    if (name == "neutron")  return FragmentSpecies::Neutron;
    if (name == "gamma")    return FragmentSpecies::DeexcitationGamma;
    if (Z >= 3)             return FragmentSpecies::HeavyIon;

    return FragmentSpecies::Other;
}

bool IsElectron(const G4ParticleDefinition* def) {
    if (!def) return false;
    return std::abs(def->GetPDGEncoding()) == 11;
}

bool IsPrimaryIonLike(const G4ParticleDefinition* def) {
    if (!def) return false;
    if (def->GetPDGEncoding() == 1000100200) return true;
    return def->GetAtomicNumber() == 10 && def->GetBaryonNumber() == 20;
}
}

void TrackingAction::PreUserTrackingAction(const G4Track* track) {
    const auto* creator = track->GetCreatorProcess();

    // Z-координата рождения частиц, рожденных в адронных реакциях
    if (creator && creator->GetProcessType() == fHadronic) {
        const int idx = ActiveHistIndex("HadronVertexZ");
        if (idx >= 0) G4AnalysisManager::Instance()->FillH1(idx, track->GetVertexPosition().z() / CLHEP::mm);
    }

    // Если трек уже получил информацию от родителя — ничего не перетираем
    if (track->GetUserInformation()) return;

    // Первичный ион Ne-20: трек от генератора (ParentID == 0) с нуклонным составом пучка.
    // Перезарядка (Z/A те же, меняется только заряд оболочки) НЕ переводит ион в адронную физику.
    const bool fromGenerator = (track->GetParentID() == 0);
    if ((fromGenerator || track->GetTrackID() == 1) && IsPrimaryIonLike(track->GetDefinition())) {
        const_cast<G4Track*>(track)->SetUserInformation(
            new BioTrackInformation(OriginChannel::PrimaryEM, FragmentSpecies::None,
                                    SubChannel::PrimaryIonDirect));
        return;
    }

    // Страховочная инициализация: проверяем процесс создания
    const bool isHad = (creator && creator->GetProcessType() == fHadronic);

    if (isHad) {
        const FragmentSpecies sp = IdentifyFragment(track->GetDefinition());
        const SubChannel sub = IsElectron(track->GetDefinition())
                                   ? SubChannel::HadronicDeltaRays
                                   : SubChannel::HadronicFragmentsDirect;
        auto* info = new BioTrackInformation(OriginChannel::HadronicCascade, sp, sub);
        info->SetFragmentZA(track->GetDefinition()->GetAtomicNumber(),
                            track->GetDefinition()->GetAtomicMass());
        const_cast<G4Track*>(track)->SetUserInformation(info);
    } else {
        const_cast<G4Track*>(track)->SetUserInformation(
            new BioTrackInformation(OriginChannel::PrimaryEM, FragmentSpecies::None,
                                    SubChannel::PrimaryDeltaRays));
    }
}

void TrackingAction::PostUserTrackingAction(const G4Track* track) {
    const auto* parentInfo = static_cast<const BioTrackInformation*>(track->GetUserInformation());
    if (!parentInfo) return;

    G4TrackVector* secondaries = fpTrackingManager->GimmeSecondaries();
    if (!secondaries || secondaries->empty()) return;

    const bool parentIsHadronic = parentInfo->IsHadronic();
    const FragmentSpecies parentSpecies = parentInfo->GetSpecies();
    const SubChannel parentSub = parentInfo->GetSubChannel();

    for (auto* sec : *secondaries) {
        if (!sec || sec->GetUserInformation()) continue;

        const auto* creator = sec->GetCreatorProcess();
        const bool createdByHadronic = (creator && creator->GetProcessType() == fHadronic);
        const bool secIsElectron = IsElectron(sec->GetDefinition());

        // Перезарядка первичного иона: тот же нуклонный состав (Z/A) без ядерной реакции —
        // трек сохраняет канал Primary_Ion_Direct, а не попадает в адронную физику.
        if (parentSub == SubChannel::PrimaryIonDirect && IsPrimaryIonLike(sec->GetDefinition())) {
            sec->SetUserInformation(
                new BioTrackInformation(OriginChannel::PrimaryEM, FragmentSpecies::None,
                                        SubChannel::PrimaryIonDirect));
            continue;
        }

        if (parentIsHadronic || createdByHadronic) {
            // Если частица рождена прямо в адронной реакции — она начинает НОВУЮ ветвь осколка.
            // Если родитель уже был адронным осколком, а процесс электромагнитный (дельта-электрон),
            // вторичная частица наследует ЕГО тип осколка.
            FragmentSpecies secSpecies = parentSpecies;
            if (createdByHadronic) {
                secSpecies = IdentifyFragment(sec->GetDefinition());
            }

            SubChannel secSub;
            if (createdByHadronic) {
                secSub = secIsElectron ? SubChannel::HadronicDeltaRays
                                       : SubChannel::HadronicFragmentsDirect;
            } else {
                secSub = secIsElectron ? SubChannel::HadronicDeltaRays : parentSub;
            }

            auto* info = new BioTrackInformation(OriginChannel::HadronicCascade, secSpecies, secSub);
            if (createdByHadronic) {  // a new fragment: identity from its own definition
                info->SetFragmentZA(sec->GetDefinition()->GetAtomicNumber(),
                                    sec->GetDefinition()->GetAtomicMass());
            } else {  // delta-ray / descendant: inherit the parent fragment's (Z,A)
                info->SetFragmentZA(parentInfo->GetFragmentZ(), parentInfo->GetFragmentA());
            }
            sec->SetUserInformation(info);
        } else {
            // Чистая электромагнитная линия первичного пучка: дельта-электроны и их каскад
            const SubChannel secSub = secIsElectron ? SubChannel::PrimaryDeltaRays : parentSub;
            sec->SetUserInformation(
                new BioTrackInformation(OriginChannel::PrimaryEM, FragmentSpecies::None, secSub));
        }
    }
}
