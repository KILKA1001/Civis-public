#pragma once

#include "civ5/runtime/bnw_fresh_religion_ai_uninit.hpp"
#include "civ5/runtime/bnw_fresh_player_tail_containers.hpp"
#include "civ5/runtime/bnw_fresh_player_entities_uninit.hpp"
#include "civ5/runtime/bnw_fresh_player_operations_uninit.hpp"
#include "civ5/runtime/bnw_fresh_player_post_operations.hpp"
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace civ5::runtime {

// An owned, detached snapshot. Calling the projector consumes a copy/move of
// this snapshot; it never mutates a live CvPlayer or the caller's original.
struct BnwFreshPlayerUninitPrefixInput {
    std::vector<content::GameplayDefine> native_constants;
    std::vector<BnwArchiveMemberState> archive_members;
    BnwFreshPoliciesServiceState policies;
    BnwFreshEconomicServiceState economic;
    BnwFreshMilitaryServiceState military;
    BnwFreshCitySpecializationServiceState city_specialization;
    BnwFreshWonderServiceState wonder;
    BnwFreshGrandStrategyServiceState grand_strategy;
    BnwFreshDiplomacyServiceState diplomacy;
    BnwFreshReligionsServiceState religions;
    BnwFreshReligionAiServiceState religion_ai;
    BnwFreshEspionageServiceState espionage;
    BnwFreshEspionageAiServiceState espionage_ai;
    BnwFreshTradeServiceState trade;
    BnwFreshTradeAiServiceState trade_ai;
    BnwFreshLeagueAiServiceState league_ai;
    BnwFreshPlayerTechsServiceState player_techs;
    BnwFreshFlavorManagerServiceState flavor_manager;
    BnwFreshTacticalAiServiceState tactical_ai;
    BnwFreshMidAiUninitInput mid_ai;
    BnwFreshPostConnectionsUninitInput post_connections;
    BnwFreshPlayerTailContainersState tail_containers;
    BnwFreshFreeListState cities;
    BnwFreshFreeListState units;
    BnwFreshPlayerOperationsState operations;
    BnwFreshPlayerPostOperationsState post_operations;
};

enum class BnwFreshPlayerUninitPrefixEventKind {
    archive_reset_projection,
    vector_clear,
    container_clear,
    service_uninit,
};
struct BnwFreshPlayerUninitPrefixEvent {
    BnwFreshPlayerUninitPrefixEventKind kind;
    std::string_view sdk_member;
    bool source_effect_verified;
    bool operator==(const BnwFreshPlayerUninitPrefixEvent &) const = default;
};

enum class BnwFreshPlayerUninitPrefixBlocker {
    none,
    archive_projection_invalid,
    early_vector_clear_invalid,
    service_projection_invalid,
    archive_reset_traversal_pending,
    entities_container_uninit_invalid,
    ai_operations_uninit_invalid,
    post_operations_invalid,
    archive_clear_delta_pending,
};

struct BnwFreshPlayerUninitPrefixDraft {
    BnwFreshFullArchiveProjection archive;
    BnwFreshPoliciesUninitDraft policies;
    BnwFreshEconomicUninitDraft economic;
    BnwFreshMilitaryUninitDraft military;
    BnwFreshCitySpecializationUninitDraft city_specialization;
    BnwFreshWonderUninitDraft wonder;
    BnwFreshGrandStrategyUninitDraft grand_strategy;
    BnwFreshDiplomacyUninitDraft diplomacy;
    BnwFreshReligionsUninitDraft religions;
    BnwFreshReligionAiUninitDraft religion_ai;
    BnwFreshEspionageUninitDraft espionage;
    BnwFreshEspionageAiUninitDraft espionage_ai;
    BnwFreshTradeUninitDraft trade;
    BnwFreshTradeAiUninitDraft trade_ai;
    BnwFreshLeagueAiUninitDraft league_ai;
    BnwFreshPlayerTechsUninitDraft player_techs;
    BnwFreshFlavorManagerUninitDraft flavor_manager;
    BnwFreshTacticalAiUninitDraft tactical_ai;
    BnwFreshMidAiUninitDraft mid_ai;
    BnwFreshPostConnectionsUninitDraft post_connections;
    BnwFreshPlayerTailContainersDraft tail_containers;
    BnwFreshPlayerEntitiesDraft entities;
    BnwFreshPlayerOperationsDraft operations;
    BnwFreshPlayerPostOperationsDraft post_operations;
    std::vector<BnwFreshPlayerUninitPrefixEvent> source_order;
    std::string selected_rules_sha256;
    std::string_view blocked_at;
    std::vector<BnwFreshPlayerUninitPrefixBlocker> unresolved;
    BnwFreshPlayerUninitPrefixBlocker blocker =
        BnwFreshPlayerUninitPrefixBlocker::none;
    bool detached_effects_complete = false;
    bool archive_traversal_verified = false;
    bool uninit_complete = false;
    bool initialized_by_gamecore = false;
    void rollback() noexcept;
};

std::optional<BnwFreshPlayerUninitPrefixInput>
make_bnw_fresh_player_uninit_prefix_input(
    std::span<const content::GameplayDefine> native_constants);

// Source order: CvPlayer::reset -> m_syncArchive.reset (semantic projection,
// traversal still unknown) -> 21 early vector clears -> service calls through
// optional CvDangerPlots::Uninit -> three yield-vector clears and three linked
// lists, then fresh empty City and Unit freelists. The detached effects are
// atomic. The live operation destructor is guarded. The post-operation plots
// and conquered flags are cleared; FAutoArchive::clearDelta is next.
// No game is published.
BnwFreshPlayerUninitPrefixDraft project_bnw_fresh_player_uninit_prefix(
    const BnwFreshFirstPlayerObjects &objects,
    std::string_view selected_rules_sha256,
    BnwFreshPlayerUninitPrefixInput initial);

} // namespace civ5::runtime
