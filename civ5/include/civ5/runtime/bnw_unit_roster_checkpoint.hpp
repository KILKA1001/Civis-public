#pragma once
#include "civ5/runtime/bnw_unit_checkpoint.hpp"

namespace civ5::runtime {

// Proposed common section ID. This is a detached single-unit removal
// projection, not CvUnit::kill or a playable command.
inline constexpr std::uint32_t bnw_unit_roster_checkpoint_section_id=29;

struct BnwUnitRemoval {
    std::uint64_t player_index=0,unit_index=0,plot_index=0,area_index=0;
    std::int64_t owner=0,id=0,area_id=0;
};

// Expansion2 FFreeListTrashArray::RemoveAt plus serialized plot and CvArea
// accounting. Rejects incomplete source rosters, references and counters.
BnwUnitRemoval plan_bnw_unit_removal(
    const saves::SavedSessionSnapshot &source,
    std::uint64_t player_index,std::uint64_t unit_index);

// Returns a new decoded observation. Does not publish a live-world mutation.
saves::SavedSessionSnapshot project_bnw_unit_removal(
    const saves::SavedSessionSnapshot &source,const BnwUnitRemoval &removal);

saves::CivisSaveSection make_bnw_unit_roster_removal_checkpoint(
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity,
    const BnwUnitRemoval &removal);

saves::SavedSessionSnapshot restore_bnw_unit_roster_removal_checkpoint(
    const saves::CivisSaveSection &section,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity);

// ID17 map, ID18 unit relocation and ID19 area transfer overlap this section.
// A future explicit composition may replace this rejection.
void reject_bnw_unit_removal_companion_overlap(
    const std::vector<saves::CivisSaveSection> &sections);

} // namespace civ5::runtime
