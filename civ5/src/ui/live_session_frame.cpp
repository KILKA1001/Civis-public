#include "civ5/ui/live_session_frame.hpp"
#include "civ5/ui/bnw_engine_ui_turn_owner.hpp"
#include "civ5/ui/ui_owner_fences.hpp"
#include "civ5/runtime/bnw_unit_order_owners.hpp"
#include "civ5/runtime/bnw_game_activation_execution.hpp"
#include "civ5/ui/bnw_selected_notification_backend.hpp"
#include "civ5/ui/overview_screens.hpp"
#include "civ5/ui/system_screens.hpp"
#include <algorithm>
#include <limits>
#include <memory>
#include <set>
#include <stdexcept>
#include <utility>

namespace civ5::ui {
namespace {

std::optional<std::int64_t> unit_core_scalar(const saves::UnitCoreObservation &core,
                                              std::string_view name) {
    const auto field = std::find_if(core.fields.begin(), core.fields.end(),
        [&](const saves::UnitCoreField &item) { return item.name == name; });
    if (field == core.fields.end() || field->values.size() != 1) return std::nullopt;
    return field->values.front();
}

std::optional<WorldViewUnit> selected_live_unit(const runtime::LiveSessionState &state,
    std::int64_t player_id, std::int64_t unit_id,
    const LiveSessionUiSources &sources) {
    if (unit_id < 0 || !sources.database || !sources.localize_info ||
        !state.world.all_players || !state.world.map_projection ||
        state.maintenance.profile != content::TechnologyProfile::bnw)
        return std::nullopt;
    const auto army = state.maintenance.players.find(player_id);
    if (army == state.maintenance.players.end()) return std::nullopt;
    const auto maintained = army->second.units.find(unit_id);
    if (maintained == army->second.units.end() || maintained->second.type < 0)
        return std::nullopt;
    const saves::SavedUnitObservation *selected = nullptr;
    for (const auto &owner : state.world.all_players->players) {
        if (owner.saved_player_id != player_id) continue;
        for (const auto &unit : owner.units.units) {
            if (unit_core_scalar(unit.core, "m_iID") != unit_id) continue;
            if (selected) return std::nullopt;
            selected = &unit;
        }
    }
    if (!selected || selected->core.layout != saves::GameStateLayout::expansion2_version1 ||
        unit_core_scalar(selected->core, "m_eOwner") != player_id ||
        unit_core_scalar(selected->core, "m_bDeathDelay") != 0)
        return std::nullopt;
    const auto x = unit_core_scalar(selected->core, "m_iX");
    const auto y = unit_core_scalar(selected->core, "m_iY");
    const auto hash = unit_core_scalar(selected->core, "idUnitType");
    const auto &map = *state.world.map_projection;
    if (!x || !y || !hash || *x < 0 || *y < 0 ||
        static_cast<std::uint64_t>(*x) >= map.grid.width ||
        static_cast<std::uint64_t>(*y) >= map.grid.height)
        return std::nullopt;
    const auto plot = map.grid.index(*x, *y);
    if (plot >= map.cells.size() || !map.cells[plot].units ||
        std::count_if(map.cells[plot].units->begin(), map.cells[plot].units->end(),
            [&](const saves::SavedPlotUnitReference &unit) {
                return unit.owner == player_id && unit.id == unit_id;
            }) != 1)
        return std::nullopt;
    std::optional<std::string> name;
    std::uint64_t rows = 0;
    try {
        const auto report = sources.database->query_rows(
            {"SELECT Type, Description FROM Units WHERE ID = ? LIMIT 2",
             {maintained->second.type}},
            [&](const content::RulesRowView &row) {
                ++rows;
                const auto type = row.text("Type");
                const auto key = row.text("Description");
                if (type && key &&
                    content::saved_info_type_hash(*type) ==
                        static_cast<std::uint32_t>(*hash))
                    name = sources.localize_info(*key);
            }, 2);
        if (!report.all_succeeded() || rows != 1 || !name || name->empty() ||
            name->starts_with("TXT_KEY_")) return std::nullopt;
    } catch (const std::exception &) { return std::nullopt; }
    // Russian unit Description rows contain nominative and declined forms
    // separated by '|'. UnitPanel.lua displays unit:GetName(), which is the
    // resolved first form, rather than the raw catalog row. EN has no suffix.
    if (const auto divider = name->find('|'); divider != std::string::npos)
        name->erase(divider);
    if (name->empty()) return std::nullopt;
    // Current movement, modified strength and legal actions require live
    // GameCore queries. The read-only panel marks them N/A.
    return WorldViewUnit{unit_id, std::move(*name), {}, {}, {}};
}

std::vector<WorldViewNotification> current_notifications(
    const runtime::LiveSession &session, std::int64_t player_id,
    std::int32_t turn, const LiveSessionUiSources &sources) {
    if (!sources.active_notifications) return {};
    const auto feed = sources.active_notifications(session);
    if (!feed || feed->player_id != player_id || feed->turn != turn ||
        feed->owner_sequence == 0) return {};
    std::set<std::int64_t> ids;
    for (const auto &event : feed->events)
        if (event.id < 0 || event.text.empty() ||
            event.text.starts_with("TXT_KEY_") || !ids.insert(event.id).second)
            return {};
    return feed->events;
}

std::optional<PopupFrame> current_popups(const runtime::LiveSession &session,
    std::int64_t player_id, std::int32_t turn,
    const LiveSessionUiSources &sources) {
    if (!sources.active_popups) return std::nullopt;
    const auto feed = sources.active_popups(session);
    if (!feed || feed->player_id != player_id || feed->turn != turn ||
        feed->owner_sequence == 0 || feed->events.empty()) return std::nullopt;
    std::set<std::string> contexts;
    for (const auto &event : feed->events) {
        if (event.context_id.empty() || event.event_id < 0 ||
            !contexts.insert(event.context_id).second)
            return std::nullopt;
        if (event.context_id == "ChooseReligion") {
            const auto &owner = event.choose_religion;
            if (!owner || owner->player_id != player_id ||
                owner->turn != turn ||
                owner->owner_sequence != feed->owner_sequence ||
                owner->event_id != event.event_id ||
                owner->city_x < 0 || owner->city_y < 0 ||
                owner->selected_source_path.empty() ||
                (owner->mode != ChooseReligionMode::founding &&
                 owner->mode != ChooseReligionMode::enhancing))
                return std::nullopt;
            const auto player = session.state().cities.players.find(player_id);
            if (player == session.state().cities.players.end() ||
                std::none_of(player->second.cities.begin(), player->second.cities.end(),
                    [&](const auto &entry) {
                        const auto &city = entry.second;
                        return city.owner == player_id &&
                            city.x == owner->city_x && city.y == owner->city_y;
                    })) return std::nullopt;
        } else if (event.choose_religion) return std::nullopt;
        if (event.context_id == "ChooseArchaeology") {
            const auto &owner = event.choose_archaeology;
            if (!owner || owner->player_id != player_id ||
                owner->turn != turn ||
                owner->owner_sequence != feed->owner_sequence ||
                owner->event_id != event.event_id ||
                owner->unit_index < 0 || owner->dig_plot_index < 0 ||
                owner->selected_source_path.empty() || event.choices.empty() ||
                event.choices.size() > 3) return std::nullopt;
            bool has_primary = false;
            for (const auto &choice : event.choices) {
                const auto id = choice.id;
                if (owner->written_artifact ? (id != 4 && id != 5)
                                            : (id < 1 || id > 3))
                    return std::nullopt;
                has_primary |= id == (owner->written_artifact ? 4 : 1);
            }
            if (!has_primary) return std::nullopt;
        } else if (event.choose_archaeology) return std::nullopt;
        if (event.context_id == "ChoosePantheon") {
            const auto &owner = event.choose_pantheon;
            if (!owner || owner->player_id != player_id ||
                owner->turn != turn ||
                owner->owner_sequence != feed->owner_sequence ||
                owner->event_id != event.event_id || owner->data2 < 0 ||
                owner->mode != (owner->data2 > 0
                    ? ChoosePantheonMode::pantheon
                    : ChoosePantheonMode::reformation) ||
                owner->selected_source_path.empty() || event.choices.empty())
                return std::nullopt;
            for (const auto &choice : event.choices)
                if (choice.detail.empty()) return std::nullopt;
        } else if (event.choose_pantheon) return std::nullopt;
        std::set<std::int64_t> choices;
        for (const auto &choice : event.choices)
            if (choice.id < 0 || choice.label.empty() ||
                !choices.insert(choice.id).second)
                return std::nullopt;
    }
    return PopupFrame{player_id, feed->events, turn, feed->owner_sequence};
}

std::optional<int> owned_outgoing_trade_routes(const runtime::TreasuryContext &treasury,
                                                std::int64_t player_id) {
    std::size_t used = 0;
    for (const auto &route : treasury.trade_routes) {
        const bool empty = route.origin_x == -1 && route.origin_y == -1 &&
            route.destination_x == -1 && route.destination_y == -1 &&
            route.origin_owner == -1 && route.destination_owner == -1;
        if (empty) continue;
        // A partially decoded slot cannot be classified as established.
        if (route.origin_x < 0 || route.origin_y < 0 ||
            route.destination_x < 0 || route.destination_y < 0 ||
            route.origin_owner < 0 || route.destination_owner < 0)
            return std::nullopt;
        if (route.origin_owner == player_id) ++used;
    }
    if (used > static_cast<std::size_t>(std::numeric_limits<int>::max()))
        throw std::out_of_range("Live outgoing trade route count does not fit TopPanel");
    return static_cast<int>(used);
}

std::optional<bool> explicit_game_option(const runtime::ResearchContext &setup,
                                         std::string_view name) {
    std::optional<bool> value;
    for (const auto &option : setup.game_options) {
        const auto saved_name = std::string_view(option.name).substr(0, option.name.find('\0'));
        if (saved_name != name) continue;
        if (value || (option.value != 0 && option.value != 1)) return std::nullopt;
        value = option.value == 1;
    }
    return value;
}

bool borrows_live_research(const runtime::ResearchSelectionContext &research,
                           const runtime::LiveSessionState &state,
                           const content::TechnologyRules *technologies) {
    return technologies && &research.setup == &state.setup &&
           &research.players == &state.player_research &&
           &research.turn == &state.research_turn &&
           &research.technologies == technologies &&
           research.teams.data() == state.technologies.data() &&
           research.teams.size() == state.technologies.size();
}

class ReadOnlyChoiceServices final : public runtime::ResearchSelectionServices {
  public:
    ReadOnlyChoiceServices(const runtime::BnwResearchChoiceServices &services,
                           const runtime::LiveSessionState &snapshot)
        : services_(services), snapshot_(snapshot) {}
    std::optional<bool> test_all(std::string_view event, std::int64_t player,
                                 std::int64_t technology) override {
        if (!services_.lua_ready())
            throw runtime::ResearchSelectionUnavailable("Research Lua host became unavailable");
        return services_.lua_test_all_read_only(event, player, technology, snapshot_);
    }
    void research_queue_dirty(std::int64_t) override {
        throw runtime::ResearchSelectionUnavailable("Read-only chooser cannot mutate research");
    }
  private:
    const runtime::BnwResearchChoiceServices &services_;
    const runtime::LiveSessionState &snapshot_;
};

ProgressionTechPopupState capture_live_popup_candidates(
    const runtime::LiveSession &session, const ProgressionTechPopupEvent &event,
    const runtime::BnwResearchChoiceRules &rules,
    const runtime::BnwResearchChoiceServices &services,
    const content::TechnologyRules *technologies) {
    ProgressionTechPopupState unavailable;
    unavailable.player_id = event.player_id;
    unavailable.mode = event.mode;
    unavailable.steal_target_player = event.steal_target_player;
    unavailable.token = event.token;
    if (!technologies || &rules.technologies != technologies ||
        rules.technologies.profile != content::TechnologyProfile::bnw ||
        rules.defines.profile() != content::GlobalDefineProfile::bnw ||
        !rules.technologies.all_succeeded()) {
        unavailable.unavailable_reason = "Active BNW research rules do not match this UI frame";
        return unavailable;
    }
    if (!services.lua_ready || !services.lua_test_all_read_only) {
        unavailable.unavailable_reason = "Live BNW research Lua checks are unavailable";
        return unavailable;
    }
    try {
        if (!services.lua_ready()) {
            unavailable.unavailable_reason = "Live BNW research Lua checks are unavailable";
            return unavailable;
        }
    } catch (const std::exception &error) {
        unavailable.unavailable_reason = error.what();
        return unavailable;
    }
    const auto &live = session.state();
    const auto player = live.setup.players.find(event.player_id);
    if (player == live.setup.players.end() || !player->second.alive ||
        player->second.team != rules.active_team) {
        unavailable.unavailable_reason = "Active BNW research player/team is unavailable";
        return unavailable;
    }
    if (live.science.no_science) {
        unavailable.unavailable_reason = "No Science game option disables technology choices";
        return unavailable;
    }
    try {
        auto detached = live;
        ReadOnlyChoiceServices checks(services, detached);
        runtime::ResearchSelectionContext research{
            detached.research_selection, detached.research_turn,
            detached.player_research, detached.setup, rules.technologies, nullptr,
            std::span<const runtime::TeamTechnologies>(detached.technologies.data(),
                                                        detached.technologies.size()),
            detached.maintenance.team_eras,
            runtime::research_prerequisite_limits(rules.defines), checks,
            rules.active_team, detached.science.no_science};
        return capture_progression_tech_popup(research, event.player_id,
                                              event.mode, event.steal_target_player);
    } catch (const std::exception &error) {
        unavailable.unavailable_reason = error.what();
        return unavailable;
    }
}

} // namespace

WorldViewState apply_live_world_view_plot_feed(WorldViewState state,
    const saves::SavedMapProjection &map, const LiveWorldViewPlotFeed &feed,
    std::uint64_t frame_number, std::int32_t turn,
    const std::function<std::optional<std::int64_t>(std::uint64_t,
                                                   std::int64_t)> &resource_type,
    const std::function<std::optional<std::int64_t>(std::uint64_t,
                                                   std::int64_t)> &plot_yield) {
    state.plot_help.reset();
    state.resource_icons.clear();
    state.yield_icons.clear();
    if (!state.map_visibility || feed.player_id != state.player_id ||
        feed.team_id != state.map_visibility->team_id ||
        feed.turn != turn || feed.frame_number != frame_number || !frame_number ||
        state.map_visibility->width != map.grid.width ||
        state.map_visibility->height != map.grid.height ||
        state.map_visibility->plots.size() != map.cells.size()) return state;
    const auto revealed = [&](std::uint64_t plot) {
        return plot < map.cells.size() && map.cells[plot].index == plot &&
            state.map_visibility->plots[plot] != WorldViewPlotFog::hidden;
    };
    if (feed.hovered_plot && revealed(feed.hovered_plot->first) &&
        !feed.hovered_plot->second.empty() &&
        !feed.hovered_plot->second.starts_with("TXT_KEY_"))
        state.plot_help = feed.hovered_plot->second;
    const auto art = [](const LiveWorldViewPlotIcon &item, std::int32_t crop) {
        return item.icon.visible && !item.icon.texture.empty() &&
            !item.icon.label.starts_with("TXT_KEY_") &&
            item.icon.atlas.x >= 0 && item.icon.atlas.y >= 0 &&
            item.icon.atlas.width == crop && item.icon.atlas.height == crop;
    };
    std::set<std::uint64_t> resource_plots;
    for (const auto &item : feed.resources) {
        if (!revealed(item.plot) || item.type < 0 || !art(item, 64) ||
            !map.cells[item.plot].resource_type ||
            *map.cells[item.plot].resource_type != item.type ||
            !resource_type || !resource_plots.insert(item.plot).second)
            continue;
        try {
            if (resource_type(item.plot, feed.team_id) == item.type)
                state.resource_icons.push_back(item.icon);
        } catch (const std::exception &) {
            // A failed GameCore visibility query cannot reveal a resource.
        }
    }
    std::set<std::pair<std::uint64_t, std::int64_t>> yield_plots;
    for (const auto &item : feed.yields) {
        if (!revealed(item.plot) || item.type < 0 || item.type >= 6 ||
            item.value <= 0 || !art(item, 128) || !plot_yield ||
            !yield_plots.insert({item.plot, item.type}).second ||
            !map.cells[item.plot].cached_yields)
            continue;
        const auto cached = map.cells[item.plot].cached_yields->find(item.type);
        if (cached == map.cells[item.plot].cached_yields->end() ||
            cached->second != item.value) continue;
        try {
            if (plot_yield(item.plot, item.type) == item.value)
                state.yield_icons.push_back(item.icon);
        } catch (const std::exception &) {
            // Missing current yield is not a visible zero or an icon.
        }
    }
    return state;
}

UiFrameSnapshot capture_live_session_ui_frame(const runtime::LiveSession &session,
    std::uint64_t frame_number, std::int64_t active_player_id,
    const LiveSessionUiSources &sources) {
    if (active_player_id < 0)
        throw std::invalid_argument("Live UI active player ID must be nonnegative");
    const auto state_lease = session.read_state_lease();
    const auto &state = *state_lease;
    if (state.setup.game_turn < std::numeric_limits<std::int32_t>::min() ||
        state.setup.game_turn > std::numeric_limits<std::int32_t>::max())
        throw std::out_of_range("Live game turn does not fit UI frame");
    auto frame = UiFrameSnapshot(frame_number, active_player_id,
                                 static_cast<std::int32_t>(state.setup.game_turn));
    const auto captured_generation=session.state_generation();
    const auto generation_current=[&session,captured_generation] {
        return session.state_generation()==captured_generation;
    };
    UiOwnerFences owner_fences;
    owner_fences.checks.push_back(generation_current);
    const auto player = state.setup.players.find(active_player_id);
    if (player == state.setup.players.end() || !player->second.alive)
        return frame.with(std::make_shared<const UiOwnerFences>(std::move(owner_fences)));

    if (sources.game_menu_quick_save) {
        const auto visibility = sources.game_menu_quick_save(session, frame_number);
        if (visibility && visibility->player_id == active_player_id &&
            visibility->turn == state.setup.game_turn &&
            visibility->frame_number == frame_number &&
            visibility->owner_sequence) {
            SystemScreenFrame menu;
            if (const auto *published = frame.find<SystemScreenFrame>()) menu = *published;
            menu.game_menu_quick_save_visible = visibility->visible;
            frame = frame.with<SystemScreenFrame>(
                std::make_shared<const SystemScreenFrame>(std::move(menu)));
        }
    }
    if (sources.game_menu_load_game) {
        const auto visibility = sources.game_menu_load_game(session, frame_number);
        if (visibility && visibility->player_id == active_player_id &&
            visibility->turn == state.setup.game_turn &&
            visibility->frame_number == frame_number &&
            visibility->owner_sequence) {
            SystemScreenFrame menu;
            if (const auto *published = frame.find<SystemScreenFrame>()) menu = *published;
            menu.game_menu_load_game_visible = visibility->visible;
            frame = frame.with<SystemScreenFrame>(
                std::make_shared<const SystemScreenFrame>(std::move(menu)));
        }
    }

    WorldViewState world_view;
    world_view.player_id = active_player_id;
    const auto selected_source = sources.selected_unit;
    const auto captured_selection = selected_source ? selected_source() : std::optional<std::int64_t>{};
    if (!generation_current()) throw std::logic_error("Unit selection replaced live UI state");
    const auto selection_current = [selected_source, captured_selection, generation_current] {
        return generation_current() && (!selected_source || selected_source() == captured_selection) &&
            generation_current();
    };
    if (selected_source) owner_fences.checks.push_back(selection_current);
    if (captured_selection)
        world_view.selected_unit = selected_live_unit(state, active_player_id, *captured_selection, sources);
    if (world_view.selected_unit && sources.selected_unit_portrait) {
        const auto source = sources.selected_unit_portrait(session, frame_number);
        if (source && source->player_id == active_player_id &&
            source->unit_id == world_view.selected_unit->id &&
            source->turn == state.setup.game_turn &&
            source->frame_number == frame_number &&
            !source->portrait.source_key.empty() &&
            source->portrait.crop.x >= 0 && source->portrait.crop.y >= 0 &&
            source->portrait.crop.width > 0 && source->portrait.crop.height > 0)
            world_view.selected_unit->portrait = source->portrait;
    }
    if (world_view.selected_unit && sources.selected_unit_read_stats &&
        sources.selected_unit_read_stats_current && sources.unit_stats_database && sources.unit_stats_identity) {
        const auto id = world_view.selected_unit->id;
        std::optional<LiveWorldViewUnitReadStatsFeed> source;
        bool accepted = false;
        try {
            source = sources.selected_unit_read_stats(session, frame_number, id);
            if (!generation_current()) throw std::logic_error("Unit read replaced live UI state");
            accepted = source && source->player_id == active_player_id && source->unit_id == id &&
                source->turn == state.setup.game_turn && source->frame_number == frame_number &&
                source->source_world_revision == runtime::bnw_unit_order_world_revision(
                    state, *sources.unit_stats_identity, *sources.unit_stats_database) &&
                selection_current() && sources.selected_unit_read_stats_current(session, *source, id, frame_number) && selection_current();
        } catch (const std::exception &) { accepted = false; }
        if (!generation_current()) throw std::logic_error("Unit read replaced live UI state");
        if (accepted) {
            if (source->movement) world_view.selected_unit->movement = *source->movement;
            if (source->strength) world_view.selected_unit->strength = *source->strength;
            frame = frame.with(std::make_shared<const LiveWorldViewUnitReadStatsFeed>(*source));
            owner_fences.checks.push_back([current = sources.selected_unit_read_stats_current,
                &session, feed = *source, id, frame_number, selection_current] { return selection_current() && current(session, feed, id, frame_number) && selection_current(); });
        }
    }
    if (world_view.selected_unit && sources.selected_unit_stat_rows &&
        sources.selected_unit_stat_rows_current && sources.unit_stats_database && sources.unit_stats_identity) {
        const auto id=world_view.selected_unit->id;
        std::optional<BnwUnitPanelReadRows> rows;
        bool accepted=false;
        try {
            rows=sources.selected_unit_stat_rows(session,frame_number,id);
            if (!generation_current()) throw std::logic_error("Unit row read replaced live UI state");
            accepted=rows && rows->player_id==active_player_id && rows->unit_id==id &&
                rows->turn==state.setup.game_turn && rows->frame_number==frame_number && !rows->locale.empty() &&
                rows->source_world_revision==runtime::bnw_unit_order_world_revision(
                    state,*sources.unit_stats_identity,*sources.unit_stats_database) &&
                selection_current() && sources.selected_unit_stat_rows_current(session,*rows,id,frame_number) && selection_current();
        } catch (...) { accepted=false; }
        if (!generation_current()) throw std::logic_error("Unit row guard replaced live UI state");
        if (accepted) {
            world_view.selected_unit->stat_rows=rows->rows;
            frame=frame.with(std::make_shared<const BnwUnitPanelReadRows>(*rows));
            owner_fences.checks.push_back([current=sources.selected_unit_stat_rows_current,
                &session,feed=*rows,id,frame_number,selection_current] { return selection_current() && current(session,feed,id,frame_number) && selection_current(); });
        }
    }
    if (world_view.selected_unit && sources.selected_unit_stat_boxes) {
        const auto source = sources.selected_unit_stat_boxes(session, frame_number);
        if (source && source->player_id == active_player_id &&
            source->unit_id == world_view.selected_unit->id &&
            source->turn == state.setup.game_turn &&
            source->frame_number == frame_number)
            world_view.selected_unit->stat_boxes = source->boxes;
    }
    if (world_view.selected_unit && sources.selected_unit_actions) {
        const auto actions = sources.selected_unit_actions(session, frame_number);
        if (actions && actions->player_id == active_player_id &&
            actions->unit_id == world_view.selected_unit->id &&
            actions->turn == state.setup.game_turn &&
            actions->frame_number == frame_number &&
            actions->owner_sequence) {
            std::set<std::int64_t> ids;
            bool valid = true;
            for (const auto &item : actions->actions)
                if (item.id < 0 || item.label.empty() ||
                    item.label.starts_with("TXT_KEY_") ||
                    item.tooltip.starts_with("TXT_KEY_") ||
                    !item.target_screen.empty() || !ids.insert(item.id).second) {
                    valid = false;
                    break;
                }
            if (valid) world_view.selected_unit->actions = actions->actions;
        }
    }
    if (sources.info_corner_items) {
        const auto info = sources.info_corner_items(session);
        if (info && info->player_id == active_player_id &&
            info->turn == state.setup.game_turn && info->owner_sequence) {
            std::set<std::int64_t> ids;
            bool valid = true;
            for (const auto &item : info->items)
                if (item.id < 0 || item.label.empty() ||
                    item.label.starts_with("TXT_KEY_") ||
                    item.tooltip.starts_with("TXT_KEY_") ||
                    !ids.insert(item.id).second) {
                    valid = false;
                    break;
                }
            if (valid) world_view.info_corner = info->items;
        }
    }
    world_view.notifications = current_notifications(session, active_player_id,
        static_cast<std::int32_t>(state.setup.game_turn), sources);
    // The session owns the player and turn, but not camera projection,
    // notification creation or a complete End Turn executor. The selected
    // WorldView skin supplies its authored button label at paint time.
    world_view.end_turn_unavailable_reason = session.readiness().can_continue()
        ? "Live End Turn handler is not connected"
        : "Full BNW turn continuation is unavailable";
    if (state.plot_visibility.current) {
        if (!state.world.pregame)
            throw std::invalid_argument("Live BNW visibility requires selected pregame active player");
        const auto active = state.world.pregame->archive.integers.find("active_player");
        if (active == state.world.pregame->archive.integers.end() ||
            active->second != active_player_id)
            throw std::invalid_argument("Live BNW visibility active player differs from selected save");
        const auto &cache = runtime::bnw_live_plot_visibility_cache(state);
        const auto team = player->second.team;
        if (team < 0 || static_cast<std::uint64_t>(team) >= cache.team_slots)
            throw std::invalid_argument("Live BNW visibility active team is unavailable");
        if (!state.world.map_projection ||
            state.world.map_projection->cells.size() != cache.grid.size())
            throw std::invalid_argument("Live BNW visibility map projection is unavailable");
        WorldViewMapVisibility visibility;
        visibility.player_id = active_player_id;
        visibility.team_id = team;
        visibility.width = cache.grid.width;
        visibility.height = cache.grid.height;
        visibility.plots.reserve(cache.grid.size());
        for (std::size_t plot = 0; plot < cache.grid.size(); ++plot) {
            const auto &cell = state.world.map_projection->cells[plot];
            if (!cell.revealed_teams)
                throw std::invalid_argument("Live BNW visibility reveal state is unavailable");
            const auto count = cache.counts.at(plot * cache.wire_slots +
                                                static_cast<std::uint64_t>(team));
            if (count < 0)
                throw std::runtime_error("Live BNW visibility cache contains a negative count");
            visibility.plots.push_back(count > 0 ? WorldViewPlotFog::visible :
                cell.revealed_teams->contains(team) ? WorldViewPlotFog::explored :
                WorldViewPlotFog::hidden);
        }
        world_view.map_visibility = std::move(visibility);
    }
    if (sources.plot_feed && state.world.map_projection)
        if (const auto feed = sources.plot_feed(session, frame_number); feed)
            world_view = apply_live_world_view_plot_feed(std::move(world_view),
                *state.world.map_projection, *feed, frame_number,
                static_cast<std::int32_t>(state.setup.game_turn),
                [&](std::uint64_t plot, std::int64_t team) {
                    return sources.visible_resource_type
                        ? sources.visible_resource_type(session, plot, team)
                        : std::optional<std::int64_t>{};
                },
                [&](std::uint64_t plot, std::int64_t yield) {
                    return sources.current_plot_yield
                        ? sources.current_plot_yield(session, plot, yield)
                        : std::optional<std::int64_t>{};
                });
    frame = frame.with(std::make_shared<const WorldViewState>(std::move(world_view)));

    const auto cities = state.cities.players.find(active_player_id);
    if (cities != state.cities.players.end()) {
        const auto selected = sources.selected_city ? sources.selected_city() : std::nullopt;
        auto city = capture_city_screen_frame(state.cities, state.world,
                                               active_player_id, selected,
                                               sources.localize_city);
        if (sources.database)
            resolve_city_queue_names(city, *sources.database, sources.localize_info);
        if (sources.city_banner_appearance && sources.city_banner_appearance_current) {
            LiveCityBannerAppearances appearances;
            for (const auto &item : city.cities) {
                std::optional<BnwCityBannerAppearance> feed;
                bool accepted = false;
                try {
                    feed = sources.city_banner_appearance(session, active_player_id,
                        item.owner, item.id, frame_number);
                    if (!generation_current()) throw std::logic_error("City read replaced live UI state");
                    accepted = feed && feed->active_player_id == active_player_id &&
                        feed->player_id == item.owner && feed->city_id == item.id &&
                        feed->turn == state.setup.game_turn && feed->frame_number == frame_number &&
                        sources.city_banner_appearance_current(session, *feed);
                } catch (...) { accepted = false; }
                if (!generation_current()) throw std::logic_error("City read replaced live UI state");
                if (!accepted) continue;
                appearances.cities.emplace(std::pair{item.owner,item.id}, *feed);
                owner_fences.checks.push_back([current=sources.city_banner_appearance_current,
                    &session, receipt=*feed] { return current(session,receipt); });
            }
            frame = frame.with(std::make_shared<const LiveCityBannerAppearances>(std::move(appearances)));
        }
        frame = frame.with(std::make_shared<const CityScreenFrame>(std::move(city)));
    }

    if (sources.technologies && sources.database) {
        auto progression = capture_progression_frame(
            state.setup, state.player_research, state.technologies,
            *sources.technologies, *sources.database, active_player_id,
            sources.localize_info, &state.world);
        frame = frame.with(std::make_shared<const ProgressionFrameState>(
            std::move(progression)));
    }

    if (sources.active_tech_popup) {
        const auto event = sources.active_tech_popup(session);
        if (event && event->player_id == active_player_id) {
            ProgressionTechPopupState popup;
            popup.player_id = active_player_id;
            popup.mode = event->mode;
            popup.steal_target_player = event->steal_target_player;
            popup.token = event->token;
            if (event->token == 0)
                popup.unavailable_reason = "BNW popup event has no stable identity";
            else if (!sources.technologies || !sources.database)
                popup.unavailable_reason = "Active BNW progression rules are unavailable";
            else if (sources.research_choice_rules && sources.research_choice_services)
                popup = capture_live_popup_candidates(session, *event,
                    *sources.research_choice_rules, *sources.research_choice_services,
                    sources.technologies);
            else if (!sources.research_selection)
                popup.unavailable_reason = "Live BNW research selection backend is unavailable";
            else if (auto *research = sources.research_selection(session);
                     !research || !borrows_live_research(*research, state, sources.technologies))
                popup.unavailable_reason = "Research selection is not bound to this live session";
            else
                popup = capture_progression_tech_popup(*research, active_player_id,
                    event->mode, event->steal_target_player);
            popup.token = event->token;
            frame = frame.with(std::make_shared<const ProgressionTechPopupState>(
                std::move(popup)));
        }
    }

    if (sources.active_ideology_popup) {
        const auto event = sources.active_ideology_popup(session);
        if (event && event->player_id == active_player_id && event->token != 0)
            frame = frame.with(std::make_shared<const ProgressionIdeologyPopupEvent>(*event));
    }

    if (auto popups = current_popups(session, active_player_id,
            static_cast<std::int32_t>(state.setup.game_turn), sources))
        frame = frame.with(std::make_shared<const PopupFrame>(std::move(*popups)));

    std::optional<runtime::TreasuryNetQuote> overview_gold_quote;
    std::optional<runtime::ScienceQuote> overview_science_quote;
    if (cities != state.cities.players.end() &&
        state.treasury.players.contains(active_player_id)) {
        auto quotes = sources.quotes ? sources.quotes(session, frame_number) : LiveSessionUiQuotes{};
        if (!generation_current()) throw std::logic_error("Quote read replaced live UI state");
        if (sources.quotes_current && quotes.bnw_native_breakdowns.contains(active_player_id)) {
            bool accepted=false;
            try { accepted=sources.quotes_current(session,quotes,frame_number); }
            catch (...) { accepted=false; }
            if (!generation_current()) throw std::logic_error("Quote guard replaced live UI state");
            if (accepted) {
                owner_fences.checks.push_back([current=sources.quotes_current,&session,
                    captured=std::make_shared<const LiveSessionUiQuotes>(quotes),frame_number] {
                    return current(session,*captured,frame_number);
                });
            } else {
                quotes.gold.erase(active_player_id);quotes.science.erase(active_player_id);
                quotes.bnw_native_breakdowns.erase(active_player_id);
            }
        }
        if (const auto native = quotes.bnw_native_breakdowns.find(active_player_id);
            native != quotes.bnw_native_breakdowns.end() &&
            (native->second.owner_player_id != active_player_id ||
             native->second.owner_turn != state.setup.game_turn ||
             native->second.owner_frame_number != frame_number)) {
            quotes.gold.erase(active_player_id);
            quotes.science.erase(active_player_id);
        }
        if (const auto it = quotes.gold.find(active_player_id);
            it != quotes.gold.end() && it->second.expenses)
            overview_gold_quote = it->second;
        // The BNW science query may include the happiness modifier. Do not
        // publish a quote calculated against a dirty happiness cache.
        if (!session.bnw_unhappiness_current()) quotes.science.erase(active_player_id);
        if (const auto it = quotes.science.find(active_player_id);
            it != quotes.science.end())
            overview_science_quote = it->second;
        auto top_panel = capture_top_panel_live(state.setup, state.cities,
                                                 state.treasury, quotes.science,
                                                 quotes.gold);
        // These are restored GameOptions owned by this live session. The
        // authored TopPanel.lua shows their disabled labels even when the
        // corresponding numeric yield quote is unavailable.
        top_panel.no_science = state.science.no_science;
        top_panel.no_happiness = state.cities.no_happiness;
        // This option is owned by the current session's restored GameOptions.
        // Only an explicit, unambiguous value may claim Policies Off.
        top_panel.no_policies = explicit_game_option(
            state.setup, "GAMEOPTION_NO_POLICIES").value_or(false);
        if (auto active = top_panel.players.find(active_player_id);
            active != top_panel.players.end()) {
            active->second.outgoing_trade_routes =
                owned_outgoing_trade_routes(state.treasury, active_player_id);
            if (auto found = quotes.bnw_native_breakdowns.find(active_player_id);
                found != quotes.bnw_native_breakdowns.end() &&
                found->second.owner_player_id == active_player_id &&
                found->second.owner_turn == state.setup.game_turn &&
                found->second.owner_frame_number == frame_number)
                active->second.native_breakdown = found->second;
            if (auto found = quotes.trade.find(active_player_id); found != quotes.trade.end())
                active->second.trade = found->second;
            if (auto found = quotes.tourism.find(active_player_id); found != quotes.tourism.end())
                active->second.tourism = found->second;
            if (auto found = quotes.faith.find(active_player_id); found != quotes.faith.end())
                active->second.faith = found->second;
        }
        for (auto it = top_panel.players.begin(); it != top_panel.players.end();) {
            if (it->first != active_player_id) it = top_panel.players.erase(it);
            else ++it;
        }
        if (!top_panel.players.empty()) {
            // These displayed values are direct reads, not fields of the
            // Treasury/Science quotes (a positive balance can leave both equal).
            const auto balance=state.treasury.players.at(active_player_id).balance_times100;
            const auto city_count=state.cities.players.at(active_player_id).cities.size();
            const auto no_science=state.science.no_science;
            const auto no_happiness=state.cities.no_happiness;
            const auto no_policies=explicit_game_option(state.setup,"GAMEOPTION_NO_POLICIES");
            const auto routes=owned_outgoing_trade_routes(state.treasury,active_player_id);
            const auto turn=state.setup.game_turn;
            owner_fences.checks.push_back([&session,active_player_id,balance,city_count,
                no_science,no_happiness,no_policies,routes,turn] {
                const auto lease=session.read_state_lease();const auto &current=*lease;
                return current.setup.profile==content::TechnologyProfile::bnw &&
                    current.cities.profile==content::TechnologyProfile::bnw &&
                    current.treasury.profile==content::TechnologyProfile::bnw &&
                    current.setup.players.at(active_player_id).alive && current.setup.game_turn==turn &&
                    current.treasury.players.at(active_player_id).balance_times100==balance &&
                    current.cities.players.at(active_player_id).cities.size()==city_count &&
                    current.science.no_science==no_science && current.cities.no_happiness==no_happiness &&
                    explicit_game_option(current.setup,"GAMEOPTION_NO_POLICIES")==no_policies &&
                    owned_outgoing_trade_routes(current.treasury,active_player_id)==routes;
            });
            frame = frame.with(std::make_shared<const TopPanelLiveFrame>(
                std::move(top_panel)));
        }
    }
    auto overview = capture_overview_frame(state.setup, state.cities, state.treasury,
        active_player_id, &state.maintenance, sources.database, sources.localize_info,
        &state.world, sources.localize_city,
        overview_gold_quote ? &*overview_gold_quote : nullptr,
        overview_science_quote ? &*overview_science_quote : nullptr);
    frame = frame.with(std::make_shared<const OverviewFrameState>(std::move(overview)));
    if (!generation_current()) throw std::logic_error("Live UI state replaced during capture");
    // Check again after domain owners: resource observers may replace state.
    owner_fences.checks.push_back(generation_current);
    frame = frame.with(std::make_shared<const UiOwnerFences>(std::move(owner_fences)));
    if (sources.augment_snapshot) {
        if (!generation_current() || !ui_owners_current(frame))
            throw std::logic_error("Live UI base owners changed before augmentation");
        const auto number = frame.frame();
        const auto player = frame.active_player_id();
        const auto turn = frame.turn();
        const auto base_frame = frame; // Retain ALL original owners across callback replacement.
        frame = sources.augment_snapshot(session, std::move(frame));
        if (frame.frame() != number || frame.active_player_id() != player ||
            frame.turn() != turn || !generation_current() || !ui_owners_current(base_frame) ||
            !ui_owners_current(frame) || !ui_owners_current(base_frame) || !generation_current())
            throw std::logic_error("Live UI augmentation changed owner/frame identity");
    }
    return frame;
}

std::size_t apply_live_bnw_minimap_fog(MinimapTileGrid &grid,
                                      const WorldViewMapVisibility &visibility) {
    if (visibility.width != grid.width() || visibility.height != grid.height() ||
        visibility.plots.size() != static_cast<std::size_t>(grid.width()) * grid.height())
        throw std::invalid_argument("Live BNW minimap fog differs from visible map extent");
    for (const auto mode : visibility.plots)
        if (mode != WorldViewPlotFog::hidden && mode != WorldViewPlotFog::explored &&
            mode != WorldViewPlotFog::visible)
            throw std::invalid_argument("Live BNW minimap fog has an unknown plot state");
    const MinimapTileProjection projection{{1, 0, 0}, {0, 1, 0}};
    std::size_t opened = 0;
    for (std::size_t plot = 0; plot < visibility.plots.size(); ++plot) {
        if (visibility.plots[plot] == WorldViewPlotFog::hidden) continue;
        const auto x = static_cast<std::int32_t>(plot % grid.width());
        const auto y = static_cast<std::int32_t>(plot / grid.width());
        grid.clear_cell_fog(x - (y >> 1), y, projection);
        ++opened;
    }
    return opened;
}

LiveUiHost::FrameProvider make_live_session_frame_provider(
    const runtime::LiveSession &session, std::int64_t active_player_id,
    LiveSessionUiSources sources) {
    return [&session, active_player_id, sources = std::move(sources), number = std::uint64_t{0}]()
        mutable -> UiFrameSnapshot {
        if (number == std::numeric_limits<std::uint64_t>::max())
            throw std::overflow_error("Live UI frame number exhausted");
        return capture_live_session_ui_frame(session, ++number, active_player_id, sources);
    };
}

LiveBnwPreviewBinding bind_live_bnw_preview(LiveBnwPreviewInput input) {
    if (input.activation_owner && (!input.scene_identity ||
        input.activation_owner->identity.payload_sha256 != input.scene_identity->payload_sha256 ||
        input.activation_owner->identity.rules_sha256 != input.scene_identity->rules_sha256))
        throw std::invalid_argument("Live BNW activation owner does not belong to the selected source identity");
    const auto &state = input.session.state();
    const auto player = state.setup.players.find(input.active_player_id);
    if (input.active_player_id < 0 ||
        state.setup.profile != content::TechnologyProfile::bnw ||
        state.cities.profile != content::TechnologyProfile::bnw ||
        input.production_rules.profile != content::TechnologyProfile::bnw ||
        player == state.setup.players.end() || !player->second.alive)
        throw std::invalid_argument("Live BNW preview requires an active restored player and BNW rules");
    if (input.notification_owner &&
        !input.notification_owner->owns(input.session, input.active_player_id))
        throw std::invalid_argument("BNW notification owner belongs to another live session");
    const auto *production_database = input.frame_sources.database;
    auto production_localize = input.frame_sources.localize_info;
    std::shared_ptr<BnwCityProductionUiBackend> backend;
    if (input.notification_owner) {
        backend = std::make_shared<BnwSelectedNotificationBackend>(input.session,
            input.production_rules, std::move(input.production_services),
            std::move(input.production_preview), *input.notification_owner,
            input.notification_services ? *input.notification_services
                                        : BnwNotificationActivationServices{},
            production_database, std::move(production_localize));
    } else {
        backend = std::make_shared<BnwCityProductionUiBackend>(input.session,
            input.production_rules, std::move(input.production_services),
            std::move(input.production_preview), production_database,
            std::move(production_localize));
    }
    auto frames = make_live_session_frame_provider(input.session,
        input.active_player_id, std::move(input.frame_sources));
    return {std::move(frames), std::move(backend)};
}

} // namespace civ5::ui

namespace civ5::ui {
UiFrameSnapshot LiveNotificationFrameMerge::merge(const UiFrameSnapshot& snapshot,
 const std::vector<BnwEngineUiNotificationEffect>& entries,const std::vector<std::int64_t>& removed){
 const auto* original=snapshot.find<WorldViewState>();
 if(!original||!snapshot.active_player_id()||original->player_id!=*snapshot.active_player_id())
  throw std::runtime_error("Owned notification feed requires SAME actual WorldView actor");
 auto view=*original;
 const std::set<std::int64_t> tombstones(removed.begin(),removed.end());
 std::erase_if(view.notifications,[&](const WorldViewNotification& n){return tombstones.contains(n.id);});
 for(const auto& e:entries){
  if(e.kind!=BnwEngineUiNotificationEffectKind::add_notification||e.actor!=view.player_id||e.lookup<0||e.message.empty())
   throw std::runtime_error("Owned notification UI entry differs from actual Add");
  const auto found=std::find_if(view.notifications.begin(),view.notifications.end(),[&](const auto& n){return n.id==e.lookup;});
  // Preserve an existing actual authored cue/icon; this receiver cannot issue
  // native type art or audio from a caller ordinal.
  if(found!=view.notifications.end())found->text=e.message;
  else {WorldViewNotification n;n.id=e.lookup;n.text=e.message;
   n.requires_decision=e.type==0x1e4417c5U||e.type==0x489b80b6U||e.type==0x5acfa6f7U;
   view.notifications.push_back(std::move(n));}
 }
 std::set<std::int64_t> ids;
 for(const auto& n:view.notifications)if(n.id<0||!ids.insert(n.id).second)
  throw std::runtime_error("Owned notification frame has duplicate/invalid lookup");
 if(view.notifications.size()>100)throw std::runtime_error("Owned notification frame exceeds actual native ring");
 return snapshot.with<WorldViewState>(std::make_shared<const WorldViewState>(std::move(view)));
}
}
