#include "civ5/runtime/bnw_fresh_team_info_init.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>
#include <utility>

namespace civ5::runtime {
namespace {
void block(BnwFreshTeamInfoInit &result,
           BnwFreshTeamInfoBlockerCode code, std::string message) {
    result.blockers.push_back({code, std::move(message)});
}

std::uint32_t selected_slots(const content::RulesDatabase &rules,
                             const char *table) {
    const auto report = rules.prefetch_info(table, {}, {1024 * 1024});
    if (!report.all_succeeded() ||
        report.slots > std::numeric_limits<std::uint32_t>::max())
        throw std::invalid_argument(std::string("Unresolved Info collection: ") + table);
    return static_cast<std::uint32_t>(report.slots);
}

bool fixed_order_valid(const BnwFreshTeamInitPrefix &fixed) {
    if (fixed.source_order.size() != fixed.teams.size() * 6) return false;
    constexpr std::array<BnwFreshTeamInitStep,6> expected{
        BnwFreshTeamInitStep::batch_free_and_pointer_clear,
        BnwFreshTeamInitStep::team_techs_uninit,
        BnwFreshTeamInitStep::scalar_reset,
        BnwFreshTeamInitStep::fixed_array_reset,
        BnwFreshTeamInitStep::team_id_write,
        BnwFreshTeamInitStep::selected_info_batch_pending};
    for (std::size_t team = 0; team < fixed.teams.size(); ++team) {
        if (fixed.teams[team].id != static_cast<std::int32_t>(team) ||
            fixed.teams[team].dynamic_info_arrays_allocated) return false;
        for (std::size_t step = 0; step < expected.size(); ++step) {
            if (fixed.source_order[team * expected.size() + step] !=
                BnwFreshTeamInitEvent{expected[step],
                    static_cast<std::int32_t>(team)}) return false;
        }
    }
    return true;
}

void append(BnwFreshTeamInfoInit &result, BnwFreshTeamInfoStep step,
            std::int32_t team) {
    result.source_order.push_back({step,team});
}
} // namespace

bool BnwFreshTeamInfoInit::has_blocker(BnwFreshTeamInfoBlockerCode code) const {
    return std::any_of(blockers.begin(), blockers.end(),
                       [code](const auto &blocker) { return blocker.code == code; });
}

void BnwFreshTeamInfoInit::rollback() noexcept {
    sizes = {};
    teams.clear();
    source_order.clear();
    info_and_tech_arrays_complete = false;
    initialized_by_gamecore = false;
}

BnwFreshTeamInfoInit draft_bnw_fresh_team_info_init(
    const BnwFreshGameContentResult &selected,
    const BnwFreshTeamInitPrefix &fixed,
    std::string_view selected_rules_sha256,
    std::uint64_t maximum_total_cells) {
    BnwFreshTeamInfoInit result;
    if (!selected.content_ready() || !selected.activation || !selected.rules ||
        selected.profile.kind == ContentProfileKind::vanilla ||
        selected.activation->profile_kind != selected.profile.kind ||
        selected_rules_sha256.empty() ||
        !fixed.fixed_reset_complete || fixed.initialized_by_gamecore ||
        fixed.selected_rules_sha256 != selected_rules_sha256 ||
        fixed.teams.empty() || !fixed_order_valid(fixed) ||
        !fixed.has_blocker(BnwFreshTeamInitBlockerCode::selected_info_batch_pending) ||
        !fixed.has_blocker(BnwFreshTeamInitBlockerCode::team_techs_init_pending)) {
        block(result,
              BnwFreshTeamInfoBlockerCode::fixed_prefix_or_selected_rules_unverified,
              "Selected BNW rules and complete fresh Team fixed reset are required");
        return result;
    }
    result.selected_rules_sha256 = identify_bytes(
        selected.rules->gameplay_fingerprint_data()).sha256;
    if (result.selected_rules_sha256 != selected_rules_sha256) {
        block(result,
              BnwFreshTeamInfoBlockerCode::fixed_prefix_or_selected_rules_unverified,
              "Team Info and fixed arrays must use the same selected rules");
        return result;
    }
    try {
        auto &s = result.sizes;
        s.vote_sources = selected_slots(*selected.rules,"VoteSources");
        s.victories = selected_slots(*selected.rules,"Victories");
        s.small_awards = selected_slots(*selected.rules,"SmallAwards");
        s.routes = selected_slots(*selected.rules,"Routes");
        s.builds = selected_slots(*selected.rules,"Builds");
        s.projects = selected_slots(*selected.rules,"Projects");
        s.unit_classes = selected_slots(*selected.rules,"UnitClasses");
        s.building_classes = selected_slots(*selected.rules,"BuildingClasses");
        s.buildings = selected_slots(*selected.rules,"Buildings");
        s.terrains = selected_slots(*selected.rules,"Terrains");
        s.improvements = selected_slots(*selected.rules,"Improvements");
        s.technologies = selected_slots(*selected.rules,"Technologies");
    } catch (const std::exception &) {
        result.rollback();
        block(result,
              BnwFreshTeamInfoBlockerCode::selected_info_collections_unverified,
              "A selected BNW Team or GameTechs Info collection is unavailable");
        return result;
    }
    const auto &s = result.sizes;
    const std::uint64_t per_team =
        s.vote_sources + 3ULL * s.victories + s.small_awards + s.routes +
        s.builds + 4ULL * s.projects + s.unit_classes + s.building_classes +
        s.buildings + s.terrains + 18ULL * s.improvements +
        4ULL * s.technologies + 3ULL * fixed.teams.size();
    if (per_team > maximum_total_cells / fixed.teams.size()) {
        result.rollback();
        block(result,BnwFreshTeamInfoBlockerCode::allocation_budget_exceeded,
              "Selected Team Info arrays exceed the fresh GameCore allocation budget");
        return result;
    }
    try {
        result.teams.reserve(fixed.teams.size());
        result.source_order.reserve(fixed.teams.size() * 13);
        for (std::size_t index = 0; index < fixed.teams.size(); ++index) {
            const auto id = static_cast<std::int32_t>(index);
            BnwFreshTeamInfoArrays team;
            team.fixed = fixed.teams[index];
            append(result,BnwFreshTeamInfoStep::fixed_batch_clear,id);
            append(result,BnwFreshTeamInfoStep::fixed_techs_uninit,id);
            append(result,BnwFreshTeamInfoStep::fixed_scalars,id);
            append(result,BnwFreshTeamInfoStep::fixed_arrays,id);
            append(result,BnwFreshTeamInfoStep::team_id_write,id);
            append(result,BnwFreshTeamInfoStep::info_sizes_collected,id);
            team.vote_eligibility.assign(s.vote_sources,0);
            team.can_launch.assign(s.victories,false);
            team.victory_achieved.assign(s.victories,false);
            team.small_award_achieved.assign(s.small_awards,false);
            team.route_change.assign(s.routes,0);
            team.build_time_change.assign(s.builds,0);
            team.project_count.assign(s.projects,0);
            team.project_default_art.assign(s.projects,0);
            team.project_making.assign(s.projects,0);
            team.unit_class_count.assign(s.unit_classes,0);
            team.building_class_count.assign(s.building_classes,0);
            team.obsolete_building_count.assign(s.buildings,0);
            team.terrain_trade_count.assign(s.terrains,0);
            team.victory_countdown.assign(s.victories,-1);
            team.improvement_yield.assign(s.improvements,
                std::vector<std::int32_t>(6,0));
            team.improvement_no_fresh_water_yield.assign(s.improvements,
                std::vector<std::int32_t>(6,0));
            team.improvement_fresh_water_yield.assign(s.improvements,
                std::vector<std::int32_t>(6,0));
            append(result,BnwFreshTeamInfoStep::info_batch_allocated,id);
            append(result,BnwFreshTeamInfoStep::info_batch_values_reset,id);
            team.turn_team_met.assign(fixed.teams.size(),-1);
            team.turns_at_war.assign(fixed.teams.size(),0);
            team.turns_locked_into_war.assign(fixed.teams.size(),0);
            append(result,BnwFreshTeamInfoStep::turn_relation_arrays_reset,id);
            append(result,BnwFreshTeamInfoStep::improvement_yields_reset,id);
            team.has_tech.assign(s.technologies,false);
            team.no_trade_tech.assign(s.technologies,false);
            team.research_progress.assign(s.technologies,0);
            team.tech_count.assign(s.technologies,0);
            team.last_tech_acquired = -1;
            append(result,BnwFreshTeamInfoStep::team_techs_init_and_reset,id);
            team.project_art_types.assign(s.projects,{});
            append(result,BnwFreshTeamInfoStep::project_art_allocated,id);
            team.revealed_resources.clear();
            append(result,BnwFreshTeamInfoStep::revealed_resources_cleared,id);
            team.fixed.dynamic_info_arrays_allocated = true;
            result.teams.push_back(std::move(team));
        }
    } catch (const std::exception &) {
        result.rollback();
        block(result,BnwFreshTeamInfoBlockerCode::allocation_budget_exceeded,
              "Selected Team Info batch could not be owned atomically");
        return result;
    }
    result.info_and_tech_arrays_complete = true;
    block(result,BnwFreshTeamInfoBlockerCode::remaining_gamecore_init_pending,
          "CvGame::InitPlayers colors, slot Info and CvPlayerAI::init/setAlive still require live GameCore effects");
    return result;
}
} // namespace civ5::runtime
