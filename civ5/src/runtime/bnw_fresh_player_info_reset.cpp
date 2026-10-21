#include "civ5/runtime/bnw_fresh_player_info_reset.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>

namespace civ5::runtime {
namespace {
std::optional<std::int32_t> native_constant(
    std::span<const content::GameplayDefine> constants, std::string_view name) {
    std::optional<std::int32_t> result;
    for (const auto &constant : constants) {
        if (constant.name != name) continue;
        if (result || constant.value <= 0 ||
            constant.value > std::numeric_limits<std::int32_t>::max())
            return std::nullopt;
        result = static_cast<std::int32_t>(constant.value);
    }
    return result;
}
std::uint32_t selected_slots(const content::RulesDatabase &rules,
                             const char *table) {
    const auto report = rules.prefetch_info(table, {}, {1024 * 1024});
    if (!report.all_succeeded() ||
        report.slots > std::numeric_limits<std::uint32_t>::max())
        throw std::invalid_argument(std::string("Unresolved Info collection: ") + table);
    return static_cast<std::uint32_t>(report.slots);
}
void block(BnwFreshPlayerInfoReset &result,
           BnwFreshPlayerInfoBlockerCode code, std::string message) {
    result.blockers.push_back({code, std::move(message)});
}
void append(BnwFreshPlayerInfoReset &result,
            BnwFreshPlayerInfoArray array, std::int32_t player) {
    result.source_order.push_back({array,player});
}
} // namespace

bool BnwFreshPlayerInfoReset::has_blocker(
    BnwFreshPlayerInfoBlockerCode code) const {
    return std::any_of(blockers.begin(), blockers.end(),
                       [code](const auto &blocker) { return blocker.code == code; });
}

void BnwFreshPlayerInfoReset::rollback() noexcept {
    sizes = {};
    players.clear();
    source_order.clear();
    selected_arrays_complete = false;
    initialized_by_gamecore = false;
}

BnwFreshPlayerInfoReset draft_bnw_fresh_player_info_reset(
    const BnwFreshGameContentResult &selected,
    const BnwFreshPlayerResetPrefix &early,
    std::string_view selected_rules_sha256,
    std::span<const content::GameplayDefine> native_constants,
    std::uint64_t maximum_total_cells) {
    BnwFreshPlayerInfoReset result;
    const auto max_civ_players = native_constant(
        native_constants,"MAX_CIV_PLAYERS");
    if (!selected.content_ready() || !selected.activation || !selected.rules ||
        selected.profile.kind == ContentProfileKind::vanilla ||
        selected.activation->profile_kind != selected.profile.kind ||
        !early.early_arrays_complete || early.initialized_by_gamecore ||
        !early.has_blocker(
            BnwFreshPlayerResetBlockerCode::resource_info_and_remaining_reset_pending) ||
        selected_rules_sha256.empty() ||
        early.selected_rules_sha256 != selected_rules_sha256 ||
        early.players.empty() || !max_civ_players ||
        *max_civ_players >= static_cast<std::int32_t>(early.players.size())) {
        block(result,
              BnwFreshPlayerInfoBlockerCode::early_reset_or_selected_rules_unverified,
              "Selected early CvPlayer reset and native MAX_CIV_PLAYERS are required");
        return result;
    }
    for (std::size_t i = 0; i < early.players.size(); ++i) {
        if (early.players[i].id != static_cast<std::int32_t>(i)) {
            block(result,
                  BnwFreshPlayerInfoBlockerCode::early_reset_or_selected_rules_unverified,
                  "Early player IDs must follow CvGame::InitPlayers order");
            return result;
        }
    }
    result.selected_rules_sha256 = identify_bytes(
        selected.rules->gameplay_fingerprint_data()).sha256;
    if (result.selected_rules_sha256 != selected_rules_sha256) {
        block(result,
              BnwFreshPlayerInfoBlockerCode::early_reset_or_selected_rules_unverified,
              "Player arrays and early reset must use the same selected rules");
        return result;
    }
    try {
        auto &s = result.sizes;
        s.resources = selected_slots(*selected.rules,"Resources");
        s.improvements = selected_slots(*selected.rules,"Improvements");
        s.unit_combats = selected_slots(*selected.rules,"UnitCombatInfos");
        s.buildings = selected_slots(*selected.rules,"Buildings");
        s.promotions = selected_slots(*selected.rules,"UnitPromotions");
        s.unit_classes = selected_slots(*selected.rules,"UnitClasses");
        s.building_classes = selected_slots(*selected.rules,"BuildingClasses");
        s.projects = selected_slots(*selected.rules,"Projects");
        s.hurries = selected_slots(*selected.rules,"HurryInfos");
        s.vote_sources = selected_slots(*selected.rules,"VoteSources");
        s.civ_players = static_cast<std::uint32_t>(*max_civ_players);
        if (s.resources == 0 || s.improvements == 0)
            throw std::invalid_argument("SDK requires Resource and Improvement Infos");
    } catch (const std::exception &) {
        result.rollback();
        block(result,
              BnwFreshPlayerInfoBlockerCode::selected_info_collections_unverified,
              "Selected Resource/Improvement and related Player Info collections are incomplete");
        return result;
    }
    const auto &s = result.sizes;
    const std::uint64_t per_player = 7ULL * s.resources + s.improvements +
        2ULL * s.unit_combats + s.buildings + s.promotions +
        2ULL * s.unit_classes + 2ULL * s.building_classes +
        s.projects + 2ULL * s.hurries + s.vote_sources + s.civ_players;
    if (per_player > maximum_total_cells / early.players.size()) {
        result.rollback();
        block(result,BnwFreshPlayerInfoBlockerCode::allocation_budget_exceeded,
              "Selected Player Info arrays exceed the fresh GameCore allocation budget");
        return result;
    }
    try {
        result.players.reserve(early.players.size());
        result.source_order.reserve(early.players.size() * 23);
        for (std::size_t index = 0; index < early.players.size(); ++index) {
            const auto id = static_cast<std::int32_t>(index);
            BnwFreshPlayerInfoState player;
            player.early = early.players[index];
            append(result,BnwFreshPlayerInfoArray::early_reset_prefix,id);
            player.resource_used.assign(s.resources,0);
            append(result,BnwFreshPlayerInfoArray::resource_used,id);
            player.resource_total.assign(s.resources,0);
            append(result,BnwFreshPlayerInfoArray::resource_total,id);
            player.resource_gifted_to_minors.assign(s.resources,0);
            append(result,BnwFreshPlayerInfoArray::resource_gifted_to_minors,id);
            player.resource_export.assign(s.resources,0);
            append(result,BnwFreshPlayerInfoArray::resource_export,id);
            player.resource_import.assign(s.resources,0);
            append(result,BnwFreshPlayerInfoArray::resource_import,id);
            player.resource_from_minors.assign(s.resources,0);
            append(result,BnwFreshPlayerInfoArray::resource_from_minors,id);
            player.resource_siphoned.assign(s.resources,0);
            append(result,BnwFreshPlayerInfoArray::resource_siphoned,id);
            player.improvement_count.assign(s.improvements,0);
            append(result,BnwFreshPlayerInfoArray::improvement_count,id);
            player.unit_combat_production.assign(s.unit_combats,0);
            append(result,BnwFreshPlayerInfoArray::unit_combat_production,id);
            player.unit_combat_experience.assign(s.unit_combats,0);
            append(result,BnwFreshPlayerInfoArray::unit_combat_experience,id);
            player.free_building.assign(s.buildings,0);
            append(result,BnwFreshPlayerInfoArray::free_building,id);
            player.free_promotion.assign(s.promotions,0);
            append(result,BnwFreshPlayerInfoArray::free_promotion,id);
            player.unit_class_count.assign(s.unit_classes,0);
            append(result,BnwFreshPlayerInfoArray::unit_class_count,id);
            player.unit_class_making.assign(s.unit_classes,0);
            append(result,BnwFreshPlayerInfoArray::unit_class_making,id);
            player.building_class_count.assign(s.building_classes,0);
            append(result,BnwFreshPlayerInfoArray::building_class_count,id);
            player.building_class_making.assign(s.building_classes,0);
            append(result,BnwFreshPlayerInfoArray::building_class_making,id);
            player.project_making.assign(s.projects,0);
            append(result,BnwFreshPlayerInfoArray::project_making,id);
            player.hurry_count.assign(s.hurries,0);
            append(result,BnwFreshPlayerInfoArray::hurry_count,id);
            player.hurry_modifier.assign(s.hurries,0);
            append(result,BnwFreshPlayerInfoArray::hurry_modifier,id);
            player.loyal_member.assign(s.vote_sources,true);
            append(result,BnwFreshPlayerInfoArray::loyal_member,id);
            player.gets_science_from_player.assign(s.civ_players,false);
            append(result,BnwFreshPlayerInfoArray::gets_science_from_player,id);
            append(result,BnwFreshPlayerInfoArray::ai_services_pending,id);
            result.players.push_back(std::move(player));
        }
    } catch (const std::exception &) {
        result.rollback();
        block(result,BnwFreshPlayerInfoBlockerCode::allocation_budget_exceeded,
              "Selected Player Info arrays could not be owned atomically");
        return result;
    }
    result.selected_arrays_complete = true;
    block(result,BnwFreshPlayerInfoBlockerCode::ai_services_and_remaining_reset_pending,
          "Economic/Military/Diplomacy/Religion/Tech/Policy and other Player AI services precede the remaining reset");
    return result;
}
} // namespace civ5::runtime
