#include "civ5/runtime/bnw_fresh_economic_ai_init.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/bnw_native_max_rows.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace civ5::runtime {
namespace {
void block(BnwFreshEconomicAIInit &result,
           BnwFreshEconomicInitBlockerCode code, std::string message) {
    result.blockers.push_back({code,std::move(message)});
}
void append(BnwFreshEconomicAIInit &result, BnwFreshEconomicInitStep step,
            std::int32_t player_id) {
    result.source_order.push_back({step,player_id});
}
std::uint64_t selected_slots(const content::RulesDatabase &rules,
                             const char *table) {
    const auto report = rules.prefetch_info(table, {}, {1024 * 1024});
    if (!report.all_succeeded())
        throw std::invalid_argument(std::string("Unresolved EconomicAI Info: ") + table);
    return report.slots;
}
std::int32_t selected_define(const content::RulesDatabase &rules,
                             std::string_view name) {
    const auto value = rules.global_define(name,content::GlobalDefineKind::integer);
    if (!value.found || !value.error.empty() ||
        !std::holds_alternative<std::int64_t>(value.value))
        throw std::invalid_argument("Missing EconomicAI cached define");
    const auto integer = std::get<std::int64_t>(value.value);
    if (integer < std::numeric_limits<std::int32_t>::min() ||
        integer > std::numeric_limits<std::int32_t>::max())
        throw std::invalid_argument("EconomicAI cached define exceeds SDK int");
    return static_cast<std::int32_t>(integer);
}
} // namespace

bool BnwFreshEconomicAIInit::has_blocker(
    BnwFreshEconomicInitBlockerCode code) const {
    return std::any_of(blockers.begin(),blockers.end(),
                       [code](const auto &blocker) { return blocker.code == code; });
}

void BnwFreshEconomicAIInit::rollback() noexcept {
    players.clear();
    source_order.clear();
    economic_ai_init_complete = false;
    initialized_by_gamecore = false;
}

BnwFreshEconomicAIInit draft_bnw_fresh_economic_ai_init(
    const BnwFreshGameContentResult &selected,
    const BnwFreshPlayerInfoReset &player_info,
    std::string_view selected_rules_sha256,
    std::uint64_t maximum_total_cells) {
    BnwFreshEconomicAIInit result;
    if (!selected.content_ready() || !selected.activation || !selected.rules ||
        selected.profile.kind == ContentProfileKind::vanilla ||
        selected.activation->profile_kind != selected.profile.kind ||
        !player_info.selected_arrays_complete ||
        player_info.initialized_by_gamecore ||
        !player_info.has_blocker(
            BnwFreshPlayerInfoBlockerCode::ai_services_and_remaining_reset_pending) ||
        player_info.players.empty() || selected_rules_sha256.empty() ||
        player_info.selected_rules_sha256 != selected_rules_sha256) {
        block(result,
              BnwFreshEconomicInitBlockerCode::player_info_or_selected_rules_unverified,
              "Selected fresh Player Info arrays must precede EconomicAI::Init");
        return result;
    }
    for (std::size_t i = 0; i < player_info.players.size(); ++i) {
        if (player_info.players[i].early.id != static_cast<std::int32_t>(i)) {
            block(result,
                  BnwFreshEconomicInitBlockerCode::player_info_or_selected_rules_unverified,
                  "Player Info order differs from CvGame::InitPlayers");
            return result;
        }
    }
    result.selected_rules_sha256 = identify_bytes(
        selected.rules->gameplay_fingerprint_data()).sha256;
    if (result.selected_rules_sha256 != selected_rules_sha256) {
        block(result,
              BnwFreshEconomicInitBlockerCode::player_info_or_selected_rules_unverified,
              "EconomicAI and Player Info must use the same selected rules");
        return result;
    }
    std::uint64_t strategies = 0, flavors = 0;
    std::int32_t minimum_fertility = 0, early_city_target = 0;
    try {
        strategies = selected_slots(*selected.rules,"AIEconomicStrategies");
        flavors = static_cast<std::uint64_t>(detail::bnw_native_max_rows(*selected.rules,"Flavors"));
        minimum_fertility = selected_define(
            *selected.rules,"AI_STRATEGY_MINIMUM_SETTLE_FERTILITY");
        early_city_target = selected_define(
            *selected.rules,"AI_STRATEGY_EARLY_EXPANSION_NUM_CITIES_LIMIT");
    } catch (const std::exception &) {
        block(result,
              BnwFreshEconomicInitBlockerCode::economic_strategy_or_define_unverified,
              "Selected EconomicAI strategies, Flavors or cached Defines are unavailable");
        return result;
    }
    if (strategies > std::numeric_limits<std::int32_t>::max() ||
        flavors > std::numeric_limits<std::int32_t>::max() ||
        (2 * strategies + flavors + 12) >
            maximum_total_cells / player_info.players.size()) {
        block(result,BnwFreshEconomicInitBlockerCode::allocation_budget_exceeded,
              "Selected EconomicAI arrays exceed the fresh GameCore allocation budget");
        return result;
    }
    try {
        result.players.reserve(player_info.players.size());
        result.source_order.reserve(player_info.players.size() * 9);
        for (std::size_t index = 0; index < player_info.players.size(); ++index) {
            const auto id = static_cast<std::int32_t>(index);
            BnwFreshEconomicAIState player;
            player.player_id = id;
            append(result,BnwFreshEconomicInitStep::selected_strategies_and_player_bound,id);
            player.using_strategy.assign(static_cast<std::size_t>(strategies),false);
            player.turn_strategy_adopted.assign(
                static_cast<std::size_t>(strategies),-1);
            player.temp_flavors.assign(static_cast<std::size_t>(flavors),std::nullopt);
            append(result,BnwFreshEconomicInitStep::strategy_and_flavor_arrays_allocated,id);
            player.yields.fill(0);
            player.requested_savings.clear();
            append(result,BnwFreshEconomicInitStep::yields_and_requests_cleared,id);
            append(result,BnwFreshEconomicInitStep::strategy_and_yield_reset,id);
            player.exploration_plots_dirty = true;
            append(result,BnwFreshEconomicInitStep::exploration_and_goody_reset,id);
            for (std::int32_t type = 0; type < 6; ++type)
                player.requested_savings.push_back({type,0,0});
            append(result,BnwFreshEconomicInitStep::purchase_requests_reset,id);
            player.explorers_disbanded = 0;
            player.recon_state = player.naval_recon_state = -1;
            player.last_turn_worker_disbanded = -1;
            player.visible_antiquity_sites = 0;
            append(result,BnwFreshEconomicInitStep::counters_and_recon_reset,id);
            player.minimum_settle_fertility = minimum_fertility;
            player.early_city_number_target = early_city_target;
            append(result,BnwFreshEconomicInitStep::cached_defines_loaded,id);
            append(result,BnwFreshEconomicInitStep::military_ai_pending,id);
            result.players.push_back(std::move(player));
        }
    } catch (const std::exception &) {
        result.rollback();
        block(result,BnwFreshEconomicInitBlockerCode::allocation_budget_exceeded,
              "Selected EconomicAI state could not be owned atomically");
        return result;
    }
    result.economic_ai_init_complete = true;
    block(result,BnwFreshEconomicInitBlockerCode::military_and_remaining_ai_pending,
          "CvMilitaryAI::Init and subsequent Player services have not run");
    return result;
}
} // namespace civ5::runtime
