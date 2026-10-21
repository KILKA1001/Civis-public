#include "civ5/runtime/bnw_fresh_military_ai_init.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/bnw_native_max_rows.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace civ5::runtime {
namespace {
void block(BnwFreshMilitaryAIInit &result,
           BnwFreshMilitaryInitBlockerCode code, std::string message) {
    result.blockers.push_back({code,std::move(message)});
}
void append(BnwFreshMilitaryAIInit &result, BnwFreshMilitaryInitStep step,
            std::int32_t player_id) {
    result.source_order.push_back({step,player_id});
}
std::uint64_t selected_slots(const content::RulesDatabase &rules,
                             const char *table) {
    const auto report = rules.prefetch_info(table, {}, {1024 * 1024});
    if (!report.all_succeeded())
        throw std::invalid_argument(std::string("Unresolved MilitaryAI Info: ") + table);
    return report.slots;
}
} // namespace

bool BnwFreshMilitaryAIInit::has_blocker(
    BnwFreshMilitaryInitBlockerCode code) const {
    return std::any_of(blockers.begin(),blockers.end(),
                       [code](const auto &blocker) { return blocker.code == code; });
}

void BnwFreshMilitaryAIInit::rollback() noexcept {
    players.clear();
    source_order.clear();
    arrays_and_reset_complete = false;
    initialized_by_gamecore = false;
}

BnwFreshMilitaryAIInit draft_bnw_fresh_military_ai_init(
    const BnwFreshGameContentResult &selected,
    const BnwFreshPlayerInfoReset &player_info,
    const BnwFreshEconomicAIInit &economic,
    std::string_view selected_rules_sha256,
    std::uint64_t maximum_total_cells) {
    BnwFreshMilitaryAIInit result;
    if (!selected.content_ready() || !selected.activation || !selected.rules ||
        selected.profile.kind == ContentProfileKind::vanilla ||
        selected.activation->profile_kind != selected.profile.kind ||
        !player_info.selected_arrays_complete ||
        !economic.economic_ai_init_complete ||
        player_info.initialized_by_gamecore || economic.initialized_by_gamecore ||
        !economic.has_blocker(
            BnwFreshEconomicInitBlockerCode::military_and_remaining_ai_pending) ||
        selected_rules_sha256.empty() ||
        player_info.selected_rules_sha256 != selected_rules_sha256 ||
        economic.selected_rules_sha256 != selected_rules_sha256 ||
        player_info.players.empty() ||
        player_info.players.size() != economic.players.size() ||
        player_info.sizes.civ_players == 0) {
        block(result,
              BnwFreshMilitaryInitBlockerCode::player_or_economic_init_unverified,
              "Selected fresh Player/EconomicAI state must precede MilitaryAI::Init");
        return result;
    }
    for (std::size_t i = 0; i < economic.players.size(); ++i) {
        if (economic.players[i].player_id != static_cast<std::int32_t>(i) ||
            player_info.players[i].early.id != static_cast<std::int32_t>(i)) {
            block(result,
                  BnwFreshMilitaryInitBlockerCode::player_or_economic_init_unverified,
                  "MilitaryAI Player IDs must match source-order InitPlayers");
            return result;
        }
    }
    result.selected_rules_sha256 = identify_bytes(
        selected.rules->gameplay_fingerprint_data()).sha256;
    if (result.selected_rules_sha256 != selected_rules_sha256) {
        block(result,
              BnwFreshMilitaryInitBlockerCode::player_or_economic_init_unverified,
              "MilitaryAI must use the same selected rules as Player/EconomicAI");
        return result;
    }
    std::uint64_t strategies = 0, flavors = 0;
    try {
        strategies = selected_slots(*selected.rules,"AIMilitaryStrategies");
        flavors = static_cast<std::uint64_t>(detail::bnw_native_max_rows(*selected.rules,"Flavors"));
    } catch (const std::exception &) {
        block(result,BnwFreshMilitaryInitBlockerCode::selected_military_info_unverified,
              "Selected MilitaryAI strategy or Flavor Info is unavailable");
        return result;
    }
    const auto opponents = player_info.sizes.civ_players;
    // SDK allocates last-turn arrays to MAX_CIV_PLAYERS but Reset indexes
    // those same arrays with GetNumMilitaryAIStrategies. More strategies
    // would write outside the allocation, so no result may be published.
    if (strategies > opponents) {
        block(result,BnwFreshMilitaryInitBlockerCode::sdk_opponent_array_overflow,
              "Selected MilitaryAI strategies exceed SDK MAX_CIV_PLAYERS last-turn arrays");
        return result;
    }
    if (strategies > std::numeric_limits<std::int32_t>::max() ||
        flavors > std::numeric_limits<std::int32_t>::max() ||
        2 * strategies + flavors + 4ULL * opponents >
            maximum_total_cells / player_info.players.size()) {
        block(result,BnwFreshMilitaryInitBlockerCode::allocation_budget_exceeded,
              "Selected MilitaryAI arrays exceed the fresh GameCore allocation budget");
        return result;
    }
    try {
        result.players.reserve(player_info.players.size());
        result.source_order.reserve(player_info.players.size() * 6);
        for (std::size_t index = 0; index < player_info.players.size(); ++index) {
            const auto id = static_cast<std::int32_t>(index);
            BnwFreshMilitaryAIState player;
            player.player_id = id;
            append(result,BnwFreshMilitaryInitStep::selected_strategies_and_player_bound,id);
            append(result,BnwFreshMilitaryInitStep::diplomacy_ai_handle_pending,id);
            player.using_strategy.assign(static_cast<std::size_t>(strategies),false);
            player.turn_strategy_adopted.assign(
                static_cast<std::size_t>(strategies),-1);
            player.temp_flavors.assign(static_cast<std::size_t>(flavors),std::nullopt);
            player.last_war_state.assign(opponents,std::nullopt);
            player.last_military_threat.assign(opponents,std::nullopt);
            player.last_military_strength.assign(opponents,std::nullopt);
            player.last_target_value.assign(opponents,std::nullopt);
            append(result,BnwFreshMilitaryInitStep::strategy_flavor_and_opponent_arrays_allocated,id);
            // CvMilitaryAI::Reset zeros these counters and gives the defense
            // and army enum fields their source NO_* sentinels.
            player.total_threat_weight = 1;
            player.army_type_being_built = -1;
            player.land_defense_state = player.naval_defense_state = -1;
            append(result,BnwFreshMilitaryInitStep::scalar_counters_reset,id);
            for (std::size_t strategy = 0; strategy < strategies; ++strategy) {
                player.last_war_state[strategy] = -1;
                player.last_military_threat[strategy] = -1;
                player.last_military_strength[strategy] = -1;
                player.last_target_value[strategy] = -1;
            }
            append(result,BnwFreshMilitaryInitStep::strategy_and_opponent_prefix_reset,id);
            append(result,BnwFreshMilitaryInitStep::city_specialization_pending,id);
            result.players.push_back(std::move(player));
        }
    } catch (const std::exception &) {
        result.rollback();
        block(result,BnwFreshMilitaryInitBlockerCode::allocation_budget_exceeded,
              "Selected MilitaryAI arrays could not be owned atomically");
        return result;
    }
    result.arrays_and_reset_complete = true;
    block(result,BnwFreshMilitaryInitBlockerCode::diplomacy_handle_and_remaining_ai_pending,
          "Real CvDiplomacyAI owner and later CitySpecialization/Player AI services remain");
    return result;
}
} // namespace civ5::runtime
