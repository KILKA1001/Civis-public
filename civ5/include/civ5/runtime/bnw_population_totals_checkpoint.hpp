#pragma once
#include "civ5/runtime/bnw_city_tail_checkpoint.hpp"
#include "civ5/runtime/bnw_player_core_checkpoint.hpp"

namespace civ5::runtime {
// Reserved by Root. Component data only: native growth is still pending after
// its totals stores. This codec never issues a food/growth/cache/turn owner.
inline constexpr std::uint32_t bnw_population_totals_checkpoint_section_id=46;
struct BnwPopulationTotalsCheckpointBundle {
    saves::CivisSaveSection totals,players;
    BnwCityCheckpointQuadruple cities;
};
void validate_bnw_population_totals_checkpoint_shape(const saves::CivisSaveSection&);
void validate_bnw_population_totals_checkpoint_companions(const saves::CivisSaveDocument&);
// Existing partial player/city exports cover neither these totals nor a growth
// once-owner. When the original has the full decoded domain, refuse its loss.
// Incomplete diagnostic snapshots retain their existing uncovered domains.
void require_bnw_population_totals_export_unchanged(
    const saves::SavedSessionSnapshot&,const saves::SavedSessionSnapshot&);
BnwPopulationTotalsCheckpointBundle make_bnw_population_totals_checkpoint_bundle(
    const saves::SavedSessionSnapshot& current,
    const saves::SavedSessionSnapshot& original,
    const SessionCheckpointIdentity&);
// Numeric comparison only, using one supplied snapshot as its own baseline.
// It neither changes any world nor issues a restored Food/Population owner.
// Ordinary changed-only make/restore and standalone46 refusal are unchanged.
BnwPopulationTotalsCheckpointBundle make_bnw_population_totals_baseline_checkpoint_bundle(
    const saves::SavedSessionSnapshot&,const SessionCheckpointIdentity&);
// Produces detached data only, after every companion/hash/roster/count check.
// Ordinary live import must remain quarantined until its actual once-owner
// continuation is restored. No public-live mutation/partial store occurs here.
saves::SavedSessionSnapshot restore_bnw_population_totals_checkpoint_bundle(
    const BnwPopulationTotalsCheckpointBundle&,
    const saves::SavedSessionSnapshot& original,
    const SessionCheckpointIdentity&);
}
