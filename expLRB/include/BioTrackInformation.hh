#pragma once
#include "G4VUserTrackInformation.hh"
#include <cstdint>
#include <string_view>

enum class OriginChannel : uint8_t {
    PrimaryEM = 0,         // Unperturbed primary 20Ne beam + its direct delta-rays
    HadronicCascade = 1    // Born in nuclear interaction + ALL descendants
};

enum class FragmentSpecies : uint8_t {
    None = 0,              // Applicable for PrimaryEM
    Proton = 1,            // 1H (cascade or recoil)
    Deuteron = 2,          // 2H
    Triton = 3,            // 3H
    Helium3 = 4,           // 3He
    Alpha = 5,             // 4He
    HeavyIon = 6,          // Residual nuclei Z >= 3 (Li, Be, B, C, N, O, F, Ne fragments)
    Neutron = 7,           // Neutrons
    DeexcitationGamma = 8, // Gammas from nuclear de-excitation
    Other = 9,
    kCount = 10
};

enum class SubChannel : uint8_t {
    PrimaryIonDirect = 0,        // Primary 20Ne ion (TrackID == 1)
    PrimaryDeltaRays = 1,        // Delta-rays from the primary ion and their cascade
    HadronicFragmentsDirect = 2, // Nuclei/nucleons born in hadronic reactions (p, d, t, 3He, alpha, Z>=3, n)
    HadronicDeltaRays = 3,       // Delta-rays born after hadronic physics and their cascade
    kCount = 4
};

class BioTrackInformation final : public G4VUserTrackInformation {
public:
    BioTrackInformation(OriginChannel origin = OriginChannel::PrimaryEM,
                        FragmentSpecies species = FragmentSpecies::None,
                        SubChannel subChannel = SubChannel::PrimaryDeltaRays) noexcept
        : fOrigin(origin), fSpecies(species), fSubChannel(subChannel) {}
    ~BioTrackInformation() override = default;

    void SetOrigin(OriginChannel o) noexcept { fOrigin = o; }
    [[nodiscard]] OriginChannel GetOrigin() const noexcept { return fOrigin; }
    [[nodiscard]] bool IsHadronic() const noexcept { return fOrigin == OriginChannel::HadronicCascade; }

    void SetSpecies(FragmentSpecies s) noexcept { fSpecies = s; }
    [[nodiscard]] FragmentSpecies GetSpecies() const noexcept { return fSpecies; }

    void SetSubChannel(SubChannel s) noexcept { fSubChannel = s; }
    [[nodiscard]] SubChannel GetSubChannel() const noexcept { return fSubChannel; }

private:
    OriginChannel   fOrigin{OriginChannel::PrimaryEM};
    FragmentSpecies fSpecies{FragmentSpecies::None};
    SubChannel      fSubChannel{SubChannel::PrimaryDeltaRays};
};
