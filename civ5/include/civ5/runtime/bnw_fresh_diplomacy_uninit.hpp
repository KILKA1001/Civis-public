#pragma once

#include "civ5/runtime/bnw_fresh_grand_strategy_uninit.hpp"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace civ5::runtime {

enum class BnwFreshDiplomacyAliasState : std::uint8_t {
    null_alias,
    bound_to_diplo_data,
    dangling_after_delete,
};

struct BnwFreshDiplomacyServiceState {
    // The owner is opaque here: Uninit never reads DiplomacyAIData fields.
    // It only deletes the entire object after nulling its member aliases.
    std::optional<std::vector<std::byte>> owned_diplo_data;
    std::vector<BnwFreshDiplomacyAliasState> null_written_aliases;
    // SDK constructor binds this alias to NULL, but Uninit omits its reset.
    BnwFreshDiplomacyAliasState other_player_major_opinion_alias =
        BnwFreshDiplomacyAliasState::null_alias;
    bool operator==(const BnwFreshDiplomacyServiceState &) const = default;
};

struct BnwFreshDiplomacyUninitDraft {
    BnwFreshDiplomacyServiceState state;
    std::vector<std::string_view> source_order;
    std::string selected_rules_sha256;
    bool member_effect_complete = false;
    bool archive_predecessor_verified = false;
    bool remaining_services_complete = false;
    bool initialized_by_gamecore = false;
    void rollback() noexcept;
};

// Expansion2 CvDiplomacyAI.cpp:610-774, in exact assignment order.
std::span<const std::string_view> bnw_fresh_diplomacy_uninit_aliases();
BnwFreshDiplomacyServiceState make_bnw_fresh_diplomacy_constructor_state();

// CvPlayer.cpp:673 dispatches immediately after GrandStrategyAI. The one
// omitted alias remains dangling after deletion if Init had bound it; that is
// retained as SDK behavior, not silently corrected. Aggregate traversal and
// Religions/remaining services remain blocked.
BnwFreshDiplomacyUninitDraft project_bnw_fresh_diplomacy_uninit(
    const BnwFreshFirstPlayerObjects &objects,
    const BnwFreshFullArchiveProjection &archive,
    const BnwFreshPoliciesUninitDraft &policies,
    const BnwFreshEconomicUninitDraft &economic,
    const BnwFreshMilitaryUninitDraft &military,
    const BnwFreshCitySpecializationUninitDraft &city_specialization,
    const BnwFreshWonderUninitDraft &wonder,
    const BnwFreshGrandStrategyUninitDraft &grand_strategy,
    std::string_view selected_rules_sha256,
    BnwFreshDiplomacyServiceState initial);

} // namespace civ5::runtime
