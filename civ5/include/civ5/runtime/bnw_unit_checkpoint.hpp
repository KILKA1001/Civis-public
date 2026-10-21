#pragma once
#include "civ5/runtime/bnw_map_plot_checkpoint.hpp"

namespace civ5::runtime {

// Common CivisSaveSectionId::live_bnw_units. The writer emits version two:
// one detached unit relocation and/or signed i32 m_iMoves change. Version one
// remains readable and never writes moves. Neither version performs setXY,
// validates movement legality, delivers UI effects, or recovers live once/VM
// history. A supplied diagnostic snapshot is not continuation authority.
inline constexpr std::uint32_t bnw_unit_checkpoint_section_id=
    static_cast<std::uint32_t>(saves::CivisSaveSectionId::live_bnw_units);

saves::CivisSaveSection make_bnw_unit_checkpoint(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity);

// Version three is a batch of signed moves-only changes. Coordinates, native
// collection headers, full IDs, every other unit operand and plot rosters stay
// unchanged. This encodes detached data; it never exports live Pending/VM state.
saves::CivisSaveSection make_bnw_unit_moves_checkpoint(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity);

// Version four persists source-ordered scalar i32/boolean changes of existing
// units. Full IDs/owner/XY/type, every collection header, vector, tail and plot
// roster stay unchanged. Combat/promotion-derived caches and live effects are
// not restored by this detached data codec; full continuation remains blocked.
saves::CivisSaveSection make_bnw_unit_scalar_checkpoint(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity);

// Version five preserves the actual BNW pre_city CLinkList ordered full IDs.
// UnitCycleOrder is a plain unsaved short in the SDK; no synthetic UnitCore
// field is introduced. Existing unit/FLTA/map identities and all other player
// administration operands are immutable. This detached codec cannot export
// an unpublished newborn or recover Init/VM history.
saves::CivisSaveSection make_bnw_unit_cycle_checkpoint(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity);

// Pure source identity/unchanged-unit inspection reused by section45. Neither
// operation issues a live owner, an ID18 delta, a reset or continuation grant.
std::string bnw_unit_checkpoint_original_identity_digest(
    const saves::SavedSessionSnapshot &source);
void require_bnw_unit_checkpoint_unchanged_except_serial_mask(
    const saves::SavedSessionSnapshot &serial_masked_world,
    const saves::SavedSessionSnapshot &source);

void restore_bnw_unit_checkpoint(
    const saves::CivisSaveSection &section,
    saves::SavedSessionSnapshot &world,
    const SessionCheckpointIdentity &identity);

// A map checkpoint alone rejects changed plot unit handles. Stage the unit
// section first, mask only its proven relocation roster delta for the map writer
// (pure moves leaves the map view untouched and requires the map codec's exact
// source-derived no-op), then
// restore both sections to one detached copy before publishing it.
struct BnwMapUnitCheckpointPair {
    saves::CivisSaveSection map,units;
};
BnwMapUnitCheckpointPair make_bnw_map_unit_checkpoint_pair(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity,
    const saves::SavedTerrainFeatureResolver &features,
    const content::RulesDatabase *rules=nullptr,
    const content::InfoTypeCatalog *types=nullptr,
    const content::GlobalDefineCache *defines=nullptr);

BnwMapUnitCheckpointPair make_bnw_map_unit_moves_checkpoint_pair(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity,
    const saves::SavedTerrainFeatureResolver &features,
    const content::RulesDatabase *rules=nullptr,
    const content::InfoTypeCatalog *types=nullptr,
    const content::GlobalDefineCache *defines=nullptr);

BnwMapUnitCheckpointPair make_bnw_map_unit_scalar_checkpoint_pair(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity,
    const saves::SavedTerrainFeatureResolver &features,
    const content::RulesDatabase *rules=nullptr,
    const content::InfoTypeCatalog *types=nullptr,
    const content::GlobalDefineCache *defines=nullptr);

void restore_bnw_map_unit_checkpoint_pair(
    const saves::CivisSaveSection *map,
    const saves::CivisSaveSection *units,
    saves::SavedSessionSnapshot &world,
    const SessionCheckpointIdentity &identity,
    const saves::SavedTerrainFeatureResolver &features,
    const content::RulesDatabase *rules=nullptr,
    const content::InfoTypeCatalog *types=nullptr,
    const content::GlobalDefineCache *defines=nullptr);

} // namespace civ5::runtime
