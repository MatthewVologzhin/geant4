#pragma once
#include "G4VHit.hh"
#include "G4THitsCollection.hh"
#include "G4Allocator.hh"
#include "BioTrackInformation.hh"
#include <array>

class CellHit : public G4VHit {
public:
    CellHit() = default;
    explicit CellHit(G4int cellID) noexcept : fCellID{cellID} {}
    ~CellHit() override = default;

    inline void* operator new(size_t);
    inline void operator delete(void* hit);

    void AddHit(OriginChannel orig, FragmentSpecies spec, SubChannel sub, G4double edep,
                G4int ions, G4int nonDnaIons) noexcept {
        if (orig == OriginChannel::PrimaryEM) {
            fEdepEM += edep;
            fIonsEM += ions;
        } else {
            fEdepHad += edep;
            fIonsHad += ions;
            const auto idx = static_cast<size_t>(spec);
            if (idx < fEdepSpecies.size()) {
                fEdepSpecies[idx] += edep;
                fIonsSpecies[idx] += ions;
            }
        }
        const auto subIdx = static_cast<size_t>(sub);
        if (subIdx < fEdepSubChannel.size()) {
            fEdepSubChannel[subIdx] += edep;
            fIonsSubChannel[subIdx] += ions;
        }
        fIonsNonDNA += nonDnaIons;
        const auto idx = static_cast<size_t>(spec);
        if (idx < fIonsNonDNASpecies.size()) {
            fIonsNonDNASpecies[idx] += nonDnaIons;
        }
    }

    [[nodiscard]] G4int GetCellID() const noexcept { return fCellID; }
    [[nodiscard]] G4double GetEdepEM() const noexcept { return fEdepEM; }
    [[nodiscard]] G4double GetEdepHad() const noexcept { return fEdepHad; }
    [[nodiscard]] G4double GetEdepTotal() const noexcept { return fEdepEM + fEdepHad; }

    [[nodiscard]] G4int GetIonsEM() const noexcept { return fIonsEM; }
    [[nodiscard]] G4int GetIonsHad() const noexcept { return fIonsHad; }
    [[nodiscard]] G4int GetIonsTotal() const noexcept { return fIonsEM + fIonsHad; }

    [[nodiscard]] G4int GetIonsNonDNA() const noexcept { return fIonsNonDNA; }

    [[nodiscard]] G4double GetEdepSpecies(FragmentSpecies s) const noexcept {
        return fEdepSpecies[static_cast<size_t>(s)];
    }
    [[nodiscard]] G4int GetIonsSpecies(FragmentSpecies s) const noexcept {
        return fIonsSpecies[static_cast<size_t>(s)];
    }
    [[nodiscard]] G4int GetIonsNonDNASpecies(FragmentSpecies s) const noexcept {
        return fIonsNonDNASpecies[static_cast<size_t>(s)];
    }
    [[nodiscard]] G4double GetEdepSubChannel(SubChannel s) const noexcept {
        return fEdepSubChannel[static_cast<size_t>(s)];
    }
    [[nodiscard]] G4int GetIonsSubChannel(SubChannel s) const noexcept {
        return fIonsSubChannel[static_cast<size_t>(s)];
    }

private:
    G4double fEdepEM{0.0};
    G4double fEdepHad{0.0};
    G4int    fIonsEM{0};
    G4int    fIonsHad{0};
    G4int    fIonsNonDNA{0};

    std::array<G4double, static_cast<size_t>(FragmentSpecies::kCount)> fEdepSpecies{};
    std::array<G4int,    static_cast<size_t>(FragmentSpecies::kCount)> fIonsSpecies{};
    std::array<G4int,    static_cast<size_t>(FragmentSpecies::kCount)> fIonsNonDNASpecies{};

    std::array<G4double, static_cast<size_t>(SubChannel::kCount)> fEdepSubChannel{};
    std::array<G4int,    static_cast<size_t>(SubChannel::kCount)> fIonsSubChannel{};

    G4int fCellID{-1};
};

using CellHitsCollection = G4THitsCollection<CellHit>;
inline G4ThreadLocal G4Allocator<CellHit>* CellHitsAllocator;

inline void* CellHit::operator new(size_t) {
    if (!CellHitsAllocator) CellHitsAllocator = new G4Allocator<CellHit>;
    return static_cast<void*>(CellHitsAllocator->MallocSingle());
}

inline void CellHit::operator delete(void* hit) {
    CellHitsAllocator->FreeSingle(static_cast<CellHit*>(hit));
}
