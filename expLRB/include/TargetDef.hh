#pragma once

#include <string_view>
#include <cstdint>
#include "globals.hh"

enum class DetectorRole : uint8_t 
{
    Nucleus = 0,
    Cytoplasm = 1,
    Membrane = 2,
    WholeCell = 3,
    BioMedium = 4
};

constexpr std::string_view ToHitsCollectionName(DetectorRole role) noexcept 
{
    switch (role) {
        case DetectorRole::Nucleus:   return "NucleusHitsCollection";
        case DetectorRole::Cytoplasm: return "CytoplasmHitsCollection";
        case DetectorRole::Membrane:  return "MembraneHitsCollection";
        case DetectorRole::WholeCell: return "CellHitsCollection";
        case DetectorRole::BioMedium: return "MediumHitsCollection";
    }
    return "UnknownHitsCollection";
}

constexpr std::string_view ToRoleName(DetectorRole role) noexcept 
{
    switch (role) {
        case DetectorRole::Nucleus:   return "nucleus";
        case DetectorRole::Cytoplasm: return "cytoplasm";
        case DetectorRole::Membrane:  return "membrane";
        case DetectorRole::WholeCell: return "cell";
        case DetectorRole::BioMedium: return "medium";
    }
    return "unknown";
}