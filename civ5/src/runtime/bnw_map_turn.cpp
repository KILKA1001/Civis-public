#include "civ5/runtime/bnw_map_turn.hpp"
#include <limits>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace civ5::runtime {
namespace {
constexpr auto bnw = saves::GameStateLayout::expansion2_version1;

saves::NamedSaveInteger &field(saves::PlotObservation &plot, std::string_view name) {
    saves::NamedSaveInteger *found = nullptr;
    for (auto &candidate : plot.fields)
        if (candidate.name && name == candidate.name) {
            if (found) throw std::runtime_error("Duplicate BNW map-turn plot field");
            found = &candidate;
        }
    if (!found) throw std::runtime_error("Missing BNW map-turn plot field: " + std::string(name));
    return *found;
}

saves::PlayerCoreField &land_score(saves::SavedPlayerObservation &player) {
    saves::PlayerCoreField *found = nullptr;
    for (auto &candidate : player.core.fields)
        if (candidate.name && std::string_view(candidate.name) == "m_iTotalLandScored") {
            if (found) throw std::runtime_error("Duplicate BNW player land score");
            found = &candidate;
        }
    if (!found || found->encoding != saves::PlayerCoreEncoding::i32 ||
        found->values.size() != 1)
        throw std::runtime_error("Missing BNW player land score");
    return *found;
}

saves::SavedPlayerObservation &player_for(LiveSessionState &state, std::int64_t id) {
    saves::SavedPlayerObservation *found = nullptr;
    for (auto &candidate : state.world.all_players->players)
        if (candidate.saved_player_id == id) {
            if (found) throw std::runtime_error("Duplicate BNW map-turn player ID");
            found = &candidate;
        }
    if (!found) throw std::runtime_error("BNW plot owner has no saved player");
    return *found;
}

LiveActionResult rejected(std::uint64_t index, std::string reason) {
    return {LiveActionOutcome::rejected,
            "BNW map plot " + std::to_string(index) + ": " + std::move(reason)};
}
} // namespace

bool BnwUnitValidityServices::complete() const noexcept {
    return exists && delayed_death && at_plot && cargo && in_combat &&
        valid_domain && can_enter_territory && visible_enemy && invisible &&
        friendly_units_of_type && unit_team && plot_team && jump && kill;
}

BnwMapTurnRules resolve_bnw_map_turn_rules(const content::GlobalDefineCache &defines) {
    if (defines.profile() != content::GlobalDefineProfile::bnw)
        throw std::runtime_error("BNW map turn requires activated BNW defines");
    const auto required = [&](std::string_view name) {
        const auto *entry = defines.find(name);
        if (!entry || !entry->lookup.found || !entry->lookup.error.empty() ||
            !std::holds_alternative<std::int64_t>(entry->lookup.value))
            throw std::runtime_error("BNW map turn define is unavailable: " +
                                     std::string(name));
        return std::get<std::int64_t>(entry->lookup.value);
    };
    const auto threshold = required("OWNERSHIP_SCORE_DURATION_THRESHOLD");
    const auto unit_limit = required("PLOT_UNIT_LIMIT");
    if (threshold <= 0 || threshold > std::numeric_limits<std::int16_t>::max() ||
        unit_limit < 0 || unit_limit > std::numeric_limits<std::int32_t>::max())
        throw std::runtime_error("BNW map turn defines exceed source ranges");
    return {threshold, unit_limit};
}

LiveActionResult verify_bnw_plot_units(
    LiveSessionState &state, std::uint64_t plot_index,
    std::int64_t plot_unit_limit, const BnwUnitValidityServices &services) {
    if (plot_unit_limit < 0)
        return rejected(plot_index, "unit validity requires the selected plot limit");
    const auto &world = state.world;
    if (world.game_state.layout != bnw || !world.map_projection ||
        plot_index >= world.map_projection->cells.size() ||
        world.map_projection->cells.size() != world.map_header.grid.size())
        return rejected(plot_index, "unit validity requires an owned BNW map");
    const auto &cell = world.map_projection->cells.at(static_cast<std::size_t>(plot_index));
    if (cell.index != plot_index || !cell.units || !cell.owner)
        return rejected(plot_index, "unit validity requires a current plot roster and owner");
    const bool owned = *cell.owner >= 0;
    // CvPlot copies its linked list once. A jump or kill during pass one must
    // not change the handles visited by either pass.
    const auto old_units = *cell.units;
    if (old_units.empty()) return {LiveActionOutcome::committed,{}};
    if (!services.complete())
        return rejected(plot_index, "unit validity requires all GameCore services");
    std::set<std::pair<std::int64_t, std::int64_t>> seen;
    for (const auto handle : old_units)
        if (handle.owner < 0 || handle.id < 0 ||
            !seen.insert({handle.owner, handle.id}).second)
            return rejected(plot_index, "invalid or duplicate plot unit handle");
    try {
        const auto ask = [&](const auto &query, saves::SavedPlotUnitReference handle,
                             const char *name) {
            const auto answer = query(state, plot_index, handle);
            if (!answer) throw std::runtime_error(std::string(name) + " is unresolved");
            return *answer;
        };
        const auto relocate = [&](saves::SavedPlotUnitReference handle) {
            const auto jumped = services.jump(state, plot_index, handle);
            if (!jumped) throw std::runtime_error("jumpToNearestValidPlot is unresolved");
            if (*jumped) return LiveActionResult{LiveActionOutcome::committed, {}};
            auto killed = services.kill(state, plot_index, handle);
            if (killed.outcome != LiveActionOutcome::committed)
                return rejected(plot_index, "unit kill: " + killed.reason);
            return killed;
        };
        for (const auto handle : old_units) {
            if (!ask(services.exists, handle, "unit identity") ||
                ask(services.delayed_death, handle, "delayed death") ||
                !ask(services.at_plot, handle, "unit position") ||
                ask(services.cargo, handle, "cargo") ||
                ask(services.in_combat, handle, "combat"))
                continue;
            const auto friendly = ask(services.friendly_units_of_type, handle,
                                      "friendly unit count");
            if (friendly < 0) throw std::runtime_error("friendly unit count is negative");
            if (friendly > plot_unit_limit) {
                const auto moved = services.jump(state, plot_index, handle);
                if (!moved) throw std::runtime_error("jumpToNearestValidPlot is unresolved");
                if (!*moved) {
                    const auto killed = services.kill(state, plot_index, handle);
                    if (killed.outcome != LiveActionOutcome::committed)
                        return rejected(plot_index, "unit kill: " + killed.reason);
                    continue; // SDK sets pLoopUnit = NULL only in this case.
                }
            }
            // SDK still evaluates this against the original plot after a
            // successful stack-limit jump, then may jump a second time.
            if (!ask(services.valid_domain, handle, "unit domain") ||
                !ask(services.can_enter_territory, handle, "territory entry")) {
                const auto moved = relocate(handle);
                if (moved.outcome != LiveActionOutcome::committed) return moved;
            }
        }
        if (owned) {
            for (const auto handle : old_units) {
                if (!ask(services.exists, handle, "unit identity") ||
                    ask(services.delayed_death, handle, "delayed death") ||
                    !ask(services.at_plot, handle, "unit position") ||
                    ask(services.in_combat, handle, "combat"))
                    continue;
                if (ask(services.unit_team, handle, "unit team") !=
                        ask(services.plot_team, handle, "plot team") &&
                    ask(services.visible_enemy, handle, "visible enemy") &&
                    !ask(services.invisible, handle, "unit invisibility")) {
                    const auto moved = relocate(handle);
                    if (moved.outcome != LiveActionOutcome::committed) return moved;
                }
            }
        }
    } catch (const std::exception &error) {
        return rejected(plot_index, error.what());
    }
    return {LiveActionOutcome::committed, {}};
}

LiveActionResult run_bnw_map_turn_phase(
    LiveSessionState &state, std::int64_t ownership_score_duration_threshold,
    const BnwMapTurnServices &services) {
    if (!services.verify_units)
        return {LiveActionOutcome::rejected, "BNW map turn requires unit validity service"};
    if (ownership_score_duration_threshold <= 0)
        return {LiveActionOutcome::rejected, "BNW ownership score threshold is invalid"};
    auto &world = state.world;
    if (world.game_state.layout != bnw || world.map_header.layout != bnw ||
        !world.map_plots || world.map_plots->layout != bnw ||
        !world.map_projection || !world.all_players ||
        world.all_players->layout != bnw ||
        world.map_plots->plots.size() != world.map_header.grid.size() ||
        world.map_projection->cells.size() != world.map_header.grid.size())
        return {LiveActionOutcome::rejected, "BNW map turn requires complete owned map and players"};

    // CvMap::doTurn -> CvPlot::doTurn: duration setters, unit validity, then
    // SetWorldAnchor(NO_WORLD_ANCHOR). Keep callbacks in precisely that order.
    for (std::uint64_t index = 0; index < world.map_header.grid.size(); ++index) {
        auto &plot = world.map_plots->plots.at(static_cast<std::size_t>(index));
        const auto xy = world.map_header.grid.coordinates(index);
        if (plot.index != index || plot.x != xy[0] || plot.y != xy[1])
            return rejected(index, "plot identity changed");
        auto &owner = field(plot, "owner").value;
        auto &plot_type = field(plot, "plot_type").value;
        auto &ownership = field(plot, "ownership_duration").value;
        auto &improvement = field(plot, "improvement_duration").value;
        auto &anchor = field(plot, "world_anchor").value;
        const auto anchor_data = field(plot, "world_anchor_data").value;
        const auto &projected = world.map_projection->cells.at(static_cast<std::size_t>(index));
        if (projected.index != index || projected.owner != owner ||
            projected.plot_type != plot_type)
            return rejected(index, "raw and projected plot disagree");
        if (projected.x != xy[0] || projected.y != xy[1] || !projected.units)
            return rejected(index, "live plot unit roster is unresolved");
        if (!plot.version || *plot.version < 1 || *plot.version > 7 ||
            owner < -1 || owner > 127 || plot_type < 0 || plot_type > 3 ||
            anchor < -1 || anchor > 127 || anchor_data < -128 || anchor_data > 127 ||
            ownership < 0 || ownership > std::numeric_limits<std::int16_t>::max() ||
            improvement < 0 || improvement > std::numeric_limits<std::int16_t>::max())
            return rejected(index, "plot field exceeds BNW source range");

        if (owner != -1) {
            if (ownership == std::numeric_limits<std::int16_t>::max())
                return rejected(index, "ownership duration exceeds save range");
            const bool score_crossed = ownership < ownership_score_duration_threshold &&
                ownership + 1 >= ownership_score_duration_threshold && plot_type != 3;
            ++ownership;
            if (score_crossed) {
                auto &score = land_score(player_for(state, owner)).values.front();
                if (score < 0 || score >= std::numeric_limits<std::int32_t>::max())
                    return rejected(index, "player land score exceeds save range");
                ++score;
            }
        }
        bool has_improvement = false;
        if (*plot.version >= 5) {
            const auto hash = field(plot, "improvement_type_hash").value;
            if (hash < 0 || hash > std::numeric_limits<std::uint32_t>::max())
                return rejected(index, "improvement hash exceeds BNW source range");
            has_improvement = hash != 0;
            if (has_improvement && (!projected.improvement_type ||
                                    *projected.improvement_type < 0))
                return rejected(index, "active rules did not resolve improvement hash");
        } else {
            const auto type = field(plot, "improvement_type").value;
            if (type < -1 || type > 127)
                return rejected(index, "improvement ID exceeds BNW source range");
            has_improvement = type != -1;
            if (projected.improvement_type && *projected.improvement_type != type)
                return rejected(index, "raw and projected improvement disagree");
        }
        if (has_improvement) {
            if (improvement == std::numeric_limits<std::int16_t>::max())
                return rejected(index, "improvement duration exceeds save range");
            ++improvement;
        }
        const auto old_anchor = anchor;
        auto checked = services.verify_units(state, index);
        if (checked.outcome != LiveActionOutcome::committed)
            return rejected(index, "verifyUnitValidPlot: " + checked.reason);

        // SetWorldAnchor(NO_WORLD_ANCHOR) sends the removal event while the
        // previous anchor is still available. The service stages its detached
        // wire writes and event; an absent anchor only clears both wire fields.
        auto &after_units = state.world.map_plots->plots.at(static_cast<std::size_t>(index));
        if (field(after_units, "world_anchor").value != old_anchor ||
            field(after_units, "world_anchor_data").value != anchor_data)
            return rejected(index, "unit validity changed the world anchor");
        if (old_anchor != -1) {
            if (!services.stage_anchor_removal)
                return rejected(index, "world anchor removal service is unavailable");
            auto staged = services.stage_anchor_removal(state, index, old_anchor);
            if (staged.outcome != LiveActionOutcome::committed)
                return rejected(index, "world anchor removal: " + staged.reason);
            auto &after_stage = state.world.map_plots->plots.at(
                static_cast<std::size_t>(index));
            if (field(after_stage, "world_anchor").value != -1 ||
                field(after_stage, "world_anchor_data").value != -1)
                return rejected(index, "world anchor service did not clear both fields");
        } else {
            field(after_units, "world_anchor").value = -1;
            field(after_units, "world_anchor_data").value = -1;
        }
    }
    return {LiveActionOutcome::committed, {}};
}

BnwTurnHandler make_bnw_map_turn_handler(
    std::int64_t ownership_score_duration_threshold,
    BnwMapTurnServices services) {
    return {BnwTurnPhase::game_map,
            [ownership_score_duration_threshold, services = std::move(services)](
                LiveSessionState &state, std::int64_t player) {
                if (player != -1)
                    return LiveActionResult{LiveActionOutcome::rejected,
                                            "BNW map turn is a game phase"};
                return run_bnw_map_turn_phase(
                    state, ownership_score_duration_threshold, services);
            }};
}

} // namespace civ5::runtime
