// ScoringProfile.cc
#include "ScoringProfile.hh"

#include "G4Exception.hh"
#include "globals.hh"

#include <algorithm>
#include <cctype>
#include <map>
#include <sstream>

namespace {

ScoringProfile MakeProfile(std::string name, std::initializer_list<ScoreGroup> on)
{
    ScoringProfile p;
    p.name = std::move(name);
    for (const ScoreGroup g : on) p.groups[static_cast<std::size_t>(g)] = true;
    return p;
}

using RegistryType = std::map<std::string, ScoringProfile, std::less<>>;

const RegistryType& Registry()
{
    static const RegistryType reg = [] {
        RegistryType m;
        m.emplace("minimal", MakeProfile("minimal", {
            ScoreGroup::DoseCore,
            ScoreGroup::ICSD,
        }));
        m.emplace("standard", MakeProfile("standard", {
            ScoreGroup::DoseCore,
            ScoreGroup::DoseBySpecies,
            ScoreGroup::NonDNA,
            ScoreGroup::SubChannels,
            ScoreGroup::SpectraBySpecies,
            ScoreGroup::ElectronSpectrum,
            ScoreGroup::ICSD,
            ScoreGroup::HadronVertexZ,
        }));
        // "full" is the umbrella for additive groups; new groups join it first.
        m.emplace("full", MakeProfile("full", {
            ScoreGroup::DoseCore,
            ScoreGroup::DoseBySpecies,
            ScoreGroup::NonDNA,
            ScoreGroup::SubChannels,
            ScoreGroup::SpectraBySpecies,
            ScoreGroup::ElectronSpectrum,
            ScoreGroup::ICSD,
            ScoreGroup::HadronVertexZ,
        }));
        return m;
    }();
    return reg;
}

const ScoringProfile*& ActivePtr()
{
    static const ScoringProfile* active = &Registry().at("standard");
    return active;
}

bool& ActiveLocked()
{
    static bool locked = false;
    return locked;
}

std::string Lower(std::string_view s)
{
    std::string out(s);
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

}  // namespace

bool ScoringProfileExists(std::string_view name)
{
    return Registry().find(std::string(name)) != Registry().end();
}

std::optional<std::string> CanonicalScoringProfileName(std::string_view name)
{
    const std::string key = Lower(name);
    if (Registry().find(key) != Registry().end()) return key;
    return std::nullopt;
}

const ScoringProfile& GetScoringProfile(std::string_view name)
{
    const auto canon = CanonicalScoringProfileName(name);
    if (!canon) {
        G4Exception("ScoringProfile::GetScoringProfile", "UnknownScoringProfile",
                    FatalException,
                    ("Unknown scoring profile '" + std::string(name) + "'.").c_str());
    }
    return Registry().at(*canon);
}

std::vector<std::string> ScoringProfileNames()
{
    std::vector<std::string> names;
    names.reserve(Registry().size());
    for (const auto& entry : Registry()) names.push_back(entry.first);
    return names;
}

std::string ScoringProfileDescription(std::string_view name)
{
    const auto& p = GetScoringProfile(name);
    std::ostringstream os;
    os << p.name << " (groups:";
    const std::pair<ScoreGroup, const char*> labels[] = {
        {ScoreGroup::DoseCore, "dose-core"},
        {ScoreGroup::DoseBySpecies, "dose-by-species"},
        {ScoreGroup::NonDNA, "nonDNA"},
        {ScoreGroup::SubChannels, "subchannels"},
        {ScoreGroup::SpectraBySpecies, "spectra-by-species"},
        {ScoreGroup::ElectronSpectrum, "electron-spectrum"},
        {ScoreGroup::ICSD, "ICSD"},
        {ScoreGroup::HadronVertexZ, "hadron-vertex-z"},
    };
    for (const auto& entry : labels) {
        if (p.Has(entry.first)) os << ' ' << entry.second;
    }
    os << ')';
    return os.str();
}

void SetActiveScoringProfile(std::string_view name)
{
    if (ActiveLocked()) return;
    (void)GetScoringProfile(name);  // validates (fatal on unknown)
    ActivePtr() = &Registry().at(*CanonicalScoringProfileName(name));
}

void ForceActiveScoringProfile(std::string_view name)
{
    (void)GetScoringProfile(name);
    ActivePtr() = &Registry().at(*CanonicalScoringProfileName(name));
    ActiveLocked() = true;
}

const ScoringProfile& GetActiveScoringProfile()
{
    return *ActivePtr();
}

const std::vector<NtupleColumnSpec>& CanonicalNtupleColumns()
{
    // Order == the historical BioData layout: the "standard" profile must emit
    // exactly indices 0..34. Append new columns at the end.
    static const std::vector<NtupleColumnSpec> cols = {
        {"EventID", ScoreGroup::DoseCore, true},                       // 0
        {"Edep_Total", ScoreGroup::DoseCore, false},                   // 1
        {"Edep_EM", ScoreGroup::DoseCore, false},                      // 2
        {"Edep_HadTotal", ScoreGroup::DoseCore, false},                // 3
        {"Edep_Proton", ScoreGroup::DoseBySpecies, false},             // 4
        {"Edep_Deuteron", ScoreGroup::DoseBySpecies, false},           // 5
        {"Edep_Triton", ScoreGroup::DoseBySpecies, false},             // 6
        {"Edep_Helium3", ScoreGroup::DoseBySpecies, false},            // 7
        {"Edep_Alpha", ScoreGroup::DoseBySpecies, false},              // 8
        {"Edep_HeavyIon", ScoreGroup::DoseBySpecies, false},           // 9
        {"Edep_Neutron", ScoreGroup::DoseBySpecies, false},            // 10
        {"Edep_Other", ScoreGroup::DoseBySpecies, false},              // 11
        {"Ions_Total", ScoreGroup::DoseCore, true},                    // 12
        {"Ions_EM", ScoreGroup::DoseCore, true},                       // 13
        {"Ions_HadTotal", ScoreGroup::DoseCore, true},                 // 14
        {"Ions_Proton", ScoreGroup::DoseBySpecies, true},              // 15
        {"Ions_Deuteron", ScoreGroup::DoseBySpecies, true},            // 16
        {"Ions_Triton", ScoreGroup::DoseBySpecies, true},              // 17
        {"Ions_Helium3", ScoreGroup::DoseBySpecies, true},             // 18
        {"Ions_Alpha", ScoreGroup::DoseBySpecies, true},               // 19
        {"Ions_HeavyIon", ScoreGroup::DoseBySpecies, true},            // 20
        {"Ions_Neutron", ScoreGroup::DoseBySpecies, true},             // 21
        {"Ions_Other", ScoreGroup::DoseBySpecies, true},               // 22
        {"HitTargetCount", ScoreGroup::DoseCore, true},                // 23
        {"Ions_NonDNA", ScoreGroup::NonDNA, true},                     // 24
        {"Ions_NonDNA_Deuteron", ScoreGroup::NonDNA, true},            // 25
        {"Ions_NonDNA_Triton", ScoreGroup::NonDNA, true},              // 26
        {"Edep_PrimaryIonDirect", ScoreGroup::SubChannels, false},        // 27
        {"Edep_PrimaryDeltaRays", ScoreGroup::SubChannels, false},        // 28
        {"Edep_HadronicFragmentsDirect", ScoreGroup::SubChannels, false}, // 29
        {"Edep_HadronicDeltaRays", ScoreGroup::SubChannels, false},       // 30
        {"Ions_PrimaryIonDirect", ScoreGroup::SubChannels, true},         // 31
        {"Ions_PrimaryDeltaRays", ScoreGroup::SubChannels, true},         // 32
        {"Ions_HadronicFragmentsDirect", ScoreGroup::SubChannels, true},  // 33
        {"Ions_HadronicDeltaRays", ScoreGroup::SubChannels, true},        // 34
    };
    return cols;
}

const std::vector<HistogramSpec>& CanonicalHistograms()
{
    // Order == the historical H1 layout: "standard" must emit indices 0..14.
    static const std::vector<HistogramSpec> hists = {
        {"ICSD_Event_Total", ScoreGroup::ICSD},
        {"ICSD_Target_EM", ScoreGroup::ICSD},
        {"ICSD_Target_Had", ScoreGroup::ICSD},
        {"ICSD_Target_Total", ScoreGroup::ICSD},
        {"SpecIn_AllHad", ScoreGroup::SpectraBySpecies},
        {"SpecIn_Proton", ScoreGroup::SpectraBySpecies},
        {"SpecIn_Deuteron", ScoreGroup::SpectraBySpecies},
        {"SpecIn_Triton", ScoreGroup::SpectraBySpecies},
        {"SpecIn_He3", ScoreGroup::SpectraBySpecies},
        {"SpecIn_Alpha", ScoreGroup::SpectraBySpecies},
        {"SpecIn_Heavy", ScoreGroup::SpectraBySpecies},
        {"SpecIn_Neutron", ScoreGroup::SpectraBySpecies},
        {"SpecIn_HeavyPerNucl", ScoreGroup::SpectraBySpecies},
        {"SpecIn_Electron", ScoreGroup::ElectronSpectrum},
        {"HadronVertexZ", ScoreGroup::HadronVertexZ},
    };
    return hists;
}

int ActiveColumnIndex(std::string_view name)
{
    const auto& prof = GetActiveScoringProfile();
    int idx = 0;
    for (const auto& col : CanonicalNtupleColumns()) {
        if (!prof.Has(col.group)) continue;
        if (name == col.name) return idx;
        ++idx;
    }
    return -1;
}

int ActiveHistIndex(std::string_view name)
{
    const auto& prof = GetActiveScoringProfile();
    int idx = 0;
    for (const auto& h : CanonicalHistograms()) {
        if (!prof.Has(h.group)) continue;
        if (name == h.name) return idx;
        ++idx;
    }
    return -1;
}
