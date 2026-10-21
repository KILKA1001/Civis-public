#pragma once
#include "civ5/runtime/bnw_unit_checkpoint.hpp"

namespace civ5::runtime {

// Common CivisSaveSectionId::live_bnw_area_units.
inline constexpr std::uint32_t bnw_area_unit_checkpoint_section_id=
    static_cast<std::uint32_t>(saves::CivisSaveSectionId::live_bnw_area_units);

struct BnwAreaUnitTransfer {
    std::int64_t owner=-1,from_area=-1,to_area=-1;
    std::uint64_t from_plot=0,to_plot=0;
    std::int64_t from_units=0,from_owner_units=0;
    std::int64_t to_units=0,to_owner_units=0;
};

// CvUnit::setXY calls CvArea::changeUnitsPerPlayer(owner,-1) before changing
// coordinates, then (+1) after adding the unit to the destination plot.
// This is the isolated, deterministic CvArea accounting slice only.
BnwAreaUnitTransfer plan_bnw_area_unit_transfer(
    const saves::SavedSessionSnapshot &world,std::int64_t owner,
    std::uint64_t from_plot,std::uint64_t to_plot);
void apply_bnw_area_unit_transfer(
    saves::SavedSessionSnapshot &world,const BnwAreaUnitTransfer &transfer);

saves::CivisSaveSection make_bnw_area_unit_checkpoint(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity,
    const BnwAreaUnitTransfer &transfer);
void restore_bnw_area_unit_checkpoint(
    const saves::CivisSaveSection &section,
    saves::SavedSessionSnapshot &world,
    const SessionCheckpointIdentity &identity);

struct BnwMapUnitAreaCheckpointTriplet {
    saves::CivisSaveSection map,units,area;
};
BnwMapUnitAreaCheckpointTriplet make_bnw_map_unit_area_checkpoint_triplet(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity,
    const BnwAreaUnitTransfer &transfer,
    const saves::SavedTerrainFeatureResolver &features,
    const content::RulesDatabase *rules=nullptr,
    const content::InfoTypeCatalog *types=nullptr,
    const content::GlobalDefineCache *defines=nullptr);
void restore_bnw_map_unit_area_checkpoint_triplet(
    const saves::CivisSaveSection *map,
    const saves::CivisSaveSection *units,
    const saves::CivisSaveSection *area,
    saves::SavedSessionSnapshot &world,
    const SessionCheckpointIdentity &identity,
    const saves::SavedTerrainFeatureResolver &features,
    const content::RulesDatabase *rules=nullptr,
    const content::InfoTypeCatalog *types=nullptr,
    const content::GlobalDefineCache *defines=nullptr);

} // namespace civ5::runtime
