// ScoringProfile.hh
//
// Named scoring profiles select which output groups RunAction creates and which
// EventAction / sensitive-detector / tracking fill. The canonical column order
// below is exactly the historical BioData layout, so the "standard" profile
// reproduces the existing tree (columns 0..34) unchanged. New groups must be
// appended at the end of the canonical lists (append-only discipline).
#pragma once

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

// Logical output groups.
enum class ScoreGroup : unsigned {
    DoseCore = 0,      // EventID, Edep/Ions Total/EM/HadTotal, HitTargetCount
    DoseBySpecies,     // Edep_*/Ions_* per FragmentSpecies
    NonDNA,            // Ions_NonDNA*
    SubChannels,       // Edep_*/Ions_* per SubChannel
    SpectraBySpecies,  // SpecIn_* entrance spectra (H1)
    ElectronSpectrum,  // SpecIn_Electron (H1)
    ICSD,              // ICSD_* cluster histograms (H1)
    HadronVertexZ,     // HadronVertexZ (H1)
    kCount
};

inline constexpr std::size_t kScoreGroupCount = static_cast<std::size_t>(ScoreGroup::kCount);

struct ScoringProfile {
    std::string name;
    std::array<bool, kScoreGroupCount> groups{};

    [[nodiscard]] bool Has(ScoreGroup g) const noexcept {
        return groups[static_cast<std::size_t>(g)];
    }
};

// --- registry ---------------------------------------------------------------
[[nodiscard]] bool ScoringProfileExists(std::string_view name);
[[nodiscard]] std::optional<std::string> CanonicalScoringProfileName(std::string_view name);
[[nodiscard]] const ScoringProfile& GetScoringProfile(std::string_view name);
[[nodiscard]] std::vector<std::string> ScoringProfileNames();
[[nodiscard]] std::string ScoringProfileDescription(std::string_view name);

// --- active profile (set from CLI/UI before RunAction is built) -------------
void SetActiveScoringProfile(std::string_view name);
void ForceActiveScoringProfile(std::string_view name);  // locked: later UI calls ignored
[[nodiscard]] const ScoringProfile& GetActiveScoringProfile();

// --- canonical BioData columns (order == the historical 0..34 layout) --------
struct NtupleColumnSpec {
    const char* name;
    ScoreGroup  group;
    bool        isInt;
};
[[nodiscard]] const std::vector<NtupleColumnSpec>& CanonicalNtupleColumns();

struct HistogramSpec {
    const char* name;
    ScoreGroup  group;
};
[[nodiscard]] const std::vector<HistogramSpec>& CanonicalHistograms();

// Index of a canonical column/histogram in the *active* profile, or -1 when the
// owning group is disabled (so callers can skip disabled fills).
[[nodiscard]] int ActiveColumnIndex(std::string_view name);
[[nodiscard]] int ActiveHistIndex(std::string_view name);
