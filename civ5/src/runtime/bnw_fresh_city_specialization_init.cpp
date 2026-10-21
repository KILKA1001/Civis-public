#include "civ5/runtime/bnw_fresh_city_specialization_init.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <exception>
#include <limits>
#include <stdexcept>
#include <utility>

namespace civ5::runtime {
namespace {
void block(BnwFreshCitySpecializationInit &result,
           BnwFreshCitySpecializationBlockerCode code, std::string message) {
    result.blockers.push_back({code,std::move(message)});
}
void append(BnwFreshCitySpecializationInit &result,
            BnwFreshCitySpecializationStep step, std::int32_t player) {
    result.source_order.push_back({step,player});
}
} // namespace

bool BnwFreshCitySpecializationInit::has_blocker(
    BnwFreshCitySpecializationBlockerCode code) const {
    return std::any_of(blockers.begin(),blockers.end(),
                       [code](const auto &blocker) { return blocker.code == code; });
}

void BnwFreshCitySpecializationInit::rollback() noexcept {
    selected_specialization_slots = 0;
    players.clear();
    source_order.clear();
    city_specialization_init_complete = false;
    initialized_by_gamecore = false;
}

BnwFreshCitySpecializationInit draft_bnw_fresh_city_specialization_init(
    const BnwFreshGameContentResult &selected,
    const BnwFreshMilitaryAIInit &military,
    std::string_view selected_rules_sha256) {
    BnwFreshCitySpecializationInit result;
    if (!selected.content_ready() || !selected.activation || !selected.rules ||
        selected.profile.kind == ContentProfileKind::vanilla ||
        selected.activation->profile_kind != selected.profile.kind ||
        !military.arrays_and_reset_complete || military.initialized_by_gamecore ||
        !military.has_blocker(
            BnwFreshMilitaryInitBlockerCode::diplomacy_handle_and_remaining_ai_pending) ||
        military.players.empty() || selected_rules_sha256.empty() ||
        military.selected_rules_sha256 != selected_rules_sha256) {
        block(result,
              BnwFreshCitySpecializationBlockerCode::military_or_selected_rules_unverified,
              "Selected MilitaryAI reset must precede CitySpecializationAI::Init");
        return result;
    }
    for (std::size_t i = 0; i < military.players.size(); ++i) {
        if (military.players[i].player_id != static_cast<std::int32_t>(i)) {
            block(result,
                  BnwFreshCitySpecializationBlockerCode::military_or_selected_rules_unverified,
                  "CitySpecialization player order differs from InitPlayers");
            return result;
        }
    }
    result.selected_rules_sha256 = identify_bytes(
        selected.rules->gameplay_fingerprint_data()).sha256;
    if (result.selected_rules_sha256 != selected_rules_sha256) {
        block(result,
              BnwFreshCitySpecializationBlockerCode::military_or_selected_rules_unverified,
              "CitySpecialization and MilitaryAI must use the same selected rules");
        return result;
    }
    try {
        const auto infos = selected.rules->prefetch_info(
            "CitySpecializations", {}, {1024 * 1024});
        if (!infos.all_succeeded() ||
            infos.slots > std::numeric_limits<std::uint32_t>::max())
            throw std::runtime_error("Incomplete CitySpecializations Info");
        result.selected_specialization_slots = static_cast<std::uint32_t>(infos.slots);
    } catch (const std::exception &) {
        block(result,
              BnwFreshCitySpecializationBlockerCode::selected_specialization_info_unverified,
              "Selected CitySpecializations Info collection is unavailable");
        return result;
    }
    try {
      result.players.reserve(military.players.size());
      result.source_order.reserve(military.players.size() * 5);
      for (std::size_t index = 0; index < military.players.size(); ++index) {
        const auto player = static_cast<std::int32_t>(index);
        BnwFreshCitySpecializationState state;
        state.player_id = player;
        append(result,BnwFreshCitySpecializationStep::military_ai_dependency_pending,player);
        append(result,BnwFreshCitySpecializationStep::selected_specializations_and_player_bound,player);
        state.specializations_dirty = false;
        state.interrupt_wonders = false;
        state.next_specialization = -1;
        state.next_wonder = -1;
        state.wonder_city_id = -1;
        state.next_wonder_weight = 0;
        state.last_turn_evaluated = 0;
        append(result,BnwFreshCitySpecializationStep::flags_and_sentinels_reset,player);
        state.yield_weights.clear();
        state.production_subtype_weights.clear();
        append(result,BnwFreshCitySpecializationStep::weight_vectors_cleared,player);
        append(result,BnwFreshCitySpecializationStep::wonder_production_ai_pending,player);
        result.players.push_back(std::move(state));
      }
    } catch (const std::exception &) {
        result.rollback();
        block(result,BnwFreshCitySpecializationBlockerCode::state_allocation_failed,
              "CitySpecialization reset could not be owned atomically");
        return result;
    }
    result.city_specialization_init_complete = true;
    block(result,
          BnwFreshCitySpecializationBlockerCode::military_handle_and_wonder_ai_pending,
          "Real DiplomacyAI handle in MilitaryAI and WonderProductionAI::Init remain pending");
    return result;
}
} // namespace civ5::runtime
