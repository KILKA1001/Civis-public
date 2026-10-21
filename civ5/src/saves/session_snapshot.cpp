#include "civ5/saves/session_snapshot.hpp"
#include "civ5/runtime/bnw_food_population_growth_checkpoint.hpp"
#include "civ5/saves/vp_roster_profile.hpp"
#include "civ5/runtime/binary_reader.hpp"
#include <limits>
#include <sstream>
#include <stdexcept>

namespace civ5::saves {
SavedSessionSnapshot observe_saved_session(std::span<const std::uint8_t> payload,
                                           const SavedSessionDecodeContext &context,
                                           GameStateBudget budget) {
    if (payload.size() > static_cast<std::uint64_t>(std::numeric_limits<std::streamoff>::max()))
        throw std::runtime_error("Saved session payload exceeds stream offset range");
    if (context.decode_supporting_classes &&
        (!context.decode_all_players || !context.decode_map_regions || !context.decode_teams))
        throw std::runtime_error("Supporting classes require map regions, teams and all players");
    if (context.decode_all_players && (!context.decode_map_regions || !context.decode_teams || !context.all_players))
        throw std::runtime_error("All-player snapshot requires map regions, teams and player context");
    if (context.decode_map_regions && !context.decode_map_plots)
        throw std::runtime_error("Map regions require map plots");
    if (context.decode_teams && !context.decode_map_regions)
        throw std::runtime_error("Teams require map regions");

    std::string storage(reinterpret_cast<const char *>(payload.data()), payload.size());
    std::istringstream stream(storage, std::ios::binary);
    runtime::BinaryReader input(stream, payload.size());
    SavedSessionSnapshot result;
    result.game_state = observe_game_state(input, context.layout, context.roster, budget);
    result.progression = observe_game_progression(input, context.layout, context.progression, budget);
    result.history = observe_game_history(input, context.layout, budget);
    result.religion_culture = observe_game_religion_culture(input, context.layout, budget);
    result.leagues = observe_game_leagues(input, context.layout, budget);
    result.trade = observe_game_trade(input, context.layout, context.trade, budget);
    result.final_systems = observe_game_final_systems(input, context.layout, budget);
    result.saved_database = observe_saved_database(input, 256 * 1024 * 1024,
                                                   context.skip_non_sqlite_image_for_map_diagnostic);
    const auto *vp_roster = vp_roster_dimensions(context.roster);
    result.map_header = observe_map_header(input, context.layout,
        vp_roster ? std::optional<std::uint64_t>(vp_roster->resource_slots) : std::nullopt, budget);
    if (context.decode_map_plots) {
        MapPlotBudget map_budget;
        map_budget.plots = budget.elements;
        map_budget.data.string_bytes = budget.string_bytes;
        map_budget.data.total_string_bytes = budget.total_string_bytes;
        // A map plot carries many fixed source arrays. Keep the map reader's
        // independent host element default instead of confusing it with the
        // smaller game-state collection budget.
        result.map_plots = observe_map_plots(input, result.map_header, context.plots, map_budget);
        result.map_projection = project_saved_map(result.map_header, *result.map_plots, context.terrain_features);
        result.map_terrain = project_saved_map_terrain(*result.map_projection);
    }
    if (context.decode_map_regions)
        result.map_regions = observe_map_regions(input, context.layout, context.regions, budget);
    if (context.decode_teams)
        result.teams = observe_teams(input, context.layout, context.teams, budget);
    if (context.decode_all_players)
        result.all_players = observe_all_players(
            input, context.layout, resolve_saved_player_context(result.progression, *context.all_players), budget);
    if (context.decode_supporting_classes)
        result.supporting_classes = observe_supporting_classes(
            input, context.layout, result.map_header.grid.size(),
            context.all_players->diplomacy.major, budget);
    if (context.decode_supporting_classes && input.remaining() != 0)
        throw std::runtime_error("Full saved-session decode left trailing payload bytes");
    result.payload_end_offset = input.position();
    return result;
}

SavedSessionSnapshot observe_loaded_session(const LoadedSave &source,
                                            const SavedSessionDecodeContext &context,
                                            GameStateBudget budget, PregameBudget setup_budget) {
    if(source.standalone_services) require_supported_standalone_services(*source.standalone_services);
    if(source.live_city_production&&!context.permit_live_production_overlay)
        throw std::runtime_error("Live city production requires an overlay-aware runtime consumer");
    if(source.live_team_research&&!context.permit_live_research_overlay)
        throw std::runtime_error("Live research requires an overlay-aware runtime consumer");
    if(source.live_treasury&&!context.permit_live_treasury_overlay)
        throw std::runtime_error("Live treasury requires an overlay-aware runtime consumer");
    if(source.live_research_turn&&!context.permit_live_research_turn_overlay)
        throw std::runtime_error("Live research turn requires an overlay-aware runtime consumer");
    if(source.live_research_selection&&!context.permit_live_research_selection_overlay)
        throw std::runtime_error("Live research selection requires an overlay-aware runtime consumer");
    if(source.live_simulation_random&&!context.permit_live_random_overlay)
        throw std::runtime_error("Live simulation RNG requires an overlay-aware runtime consumer");
    if(source.live_bnw_food_growth_journal) {
        if(!context.permit_live_bnw_food_growth_original_decode)
            throw std::runtime_error("Food growth journal requires unpublished original-payload decode and genuine once-owner restore");
        if(!source.live_bnw_population_totals)
            throw std::runtime_error("Food growth journal lacks population totals companion");
        runtime::validate_bnw_food_growth_journal_shape({47,0,*source.live_bnw_food_growth_journal});
    }
    if(source.live_bnw_population_totals &&
       !(source.live_bnw_food_growth_journal && context.permit_live_bnw_food_growth_original_decode))
        throw std::runtime_error("Population totals checkpoint requires genuine food/population continuation restore before session publication");
    if(source.live_bnw_unit_serial&&!context.permit_live_bnw_unit_serial_overlay)
        throw std::runtime_error("Unit serial checkpoint requires canonical allocator restore before session publication");
    if(source.live_bnw_game_clock&&!context.permit_live_bnw_game_clock_overlay)
        throw std::runtime_error("Live BNW game clock requires an overlay-aware runtime consumer");
    if(source.live_bnw_map_plots&&!context.permit_live_bnw_map_plots_overlay)
        throw std::runtime_error("Live BNW map plots require an overlay-aware runtime consumer");
    if(source.live_bnw_units&&!context.permit_live_bnw_units_overlay)
        throw std::runtime_error("Live BNW units require an overlay-aware runtime consumer");
    if(source.live_bnw_area_units&&!context.permit_live_bnw_area_units_overlay)
        throw std::runtime_error("Live BNW area units require an overlay-aware runtime consumer");
    if(source.live_bnw_player_proximity&&!context.permit_live_bnw_player_proximity_overlay)
        throw std::runtime_error("Live BNW player proximity requires an overlay-aware runtime consumer");
    if(source.live_bnw_team_meet&&!context.permit_live_bnw_team_meet_overlay)
        throw std::runtime_error("Live BNW team meet requires an overlay-aware runtime consumer");
    if(source.live_bnw_exploration_dirty&&!context.permit_live_bnw_exploration_dirty_overlay)
        throw std::runtime_error("Live BNW exploration dirty requires an overlay-aware runtime consumer");
    if(source.live_bnw_military_requests &&
       (!source.live_bnw_player_strategy || !context.permit_live_bnw_player_strategy_overlay))
        throw std::runtime_error("Military request counters require an ID23-overlay-aware live runtime consumer");
    if(source.live_bnw_player_strategy&&!context.permit_live_bnw_player_strategy_overlay)
        throw std::runtime_error("Live BNW player strategy requires an overlay-aware runtime consumer");
    if(source.live_bnw_player_core&&!context.permit_live_bnw_player_core_overlay)
        throw std::runtime_error("Live BNW player core requires an overlay-aware runtime consumer");
    if(source.live_bnw_city_core&&!context.permit_live_bnw_city_core_overlay)
        throw std::runtime_error("Live BNW city core requires an overlay-aware runtime consumer");
    if(source.live_bnw_city_production&&!context.permit_live_bnw_city_production_overlay)
        throw std::runtime_error("Live BNW city production requires an overlay-aware runtime consumer");
    if(source.live_bnw_city_management&&!context.permit_live_bnw_city_management_overlay)
        throw std::runtime_error("Live BNW city management requires an overlay-aware runtime consumer");
    if(source.live_bnw_city_tail&&!context.permit_live_bnw_city_tail_overlay)
        throw std::runtime_error("Live BNW city tail requires an overlay-aware runtime consumer");
    if(source.live_bnw_treasury_balance&&!context.permit_live_bnw_treasury_balance_overlay)
        throw std::runtime_error("Live BNW treasury balance requires an overlay-aware runtime consumer");
    if(source.live_bnw_team_progress&&!context.permit_live_bnw_team_progress_overlay)
        throw std::runtime_error("Live BNW team progress requires an overlay-aware runtime consumer");
    if(source.live_bnw_culture_bomb_timer&&!context.permit_live_bnw_culture_bomb_timer_overlay)
        throw std::runtime_error("Live BNW culture bomb timer requires an overlay-aware runtime consumer");
    if(source.live_bnw_research_queue&&!context.permit_live_bnw_research_queue_overlay)
        throw std::runtime_error("Live BNW research queue requires an overlay-aware runtime consumer");
    if(source.live_bnw_maintenance_expense&&!context.permit_live_bnw_maintenance_expense_overlay)
        throw std::runtime_error("Live BNW maintenance expense requires an overlay-aware runtime consumer");
    if(source.live_bnw_player_knowledge&&!context.permit_live_bnw_player_knowledge_overlay)
        throw std::runtime_error("Live BNW player knowledge requires an overlay-aware runtime consumer");
    if(source.live_bnw_player_administration&&!context.permit_live_bnw_player_administration_overlay)
        throw std::runtime_error("Live BNW player administration requires an overlay-aware runtime consumer");
    if(source.live_bnw_game_supporting&&!context.permit_live_bnw_game_supporting_overlay)
        throw std::runtime_error("Live BNW game supporting state requires an overlay-aware runtime consumer");
    if(source.live_bnw_game_score_replay&&!context.permit_live_bnw_game_score_replay_overlay)
        throw std::runtime_error("Live BNW game score and replay require an overlay-aware runtime consumer");
    if(source.live_bnw_game_deals&&!context.permit_live_bnw_game_deals_overlay)
        throw std::runtime_error("Live BNW game deals require an overlay-aware runtime consumer");
    if(source.live_bnw_team_turn&&!context.permit_live_bnw_team_turn_overlay)
        throw std::runtime_error("Live BNW team turn requires an overlay-aware runtime consumer");
    if(source.live_bnw_ui_transients&&!context.permit_live_bnw_ui_transients_overlay)
        throw std::runtime_error("Live BNW UI transients require an overlay-aware selected host");
    for(const auto &[id,bytes]:source.profile_sections)
        if(!context.permitted_profile_sections.contains(id))
            throw std::runtime_error("GameCore profile section requires a profile-aware runtime consumer");
    std::optional<PregameObservation> setup;
    if (source.source_prefix) setup = observe_pregame(*source.source_prefix, source.header, context.layout, setup_budget);
    auto result = observe_saved_session(source.bytes, context, budget);
    if(source.saved_database_override){
        if(result.saved_database.non_sqlite_image_skipped)
            throw std::runtime_error("Civis database cannot replace an unverified source boundary");
        validate_saved_database_image(*source.saved_database_override);
        result.saved_database.image=*source.saved_database_override;
        result.saved_database.quick_check_passed=true;
        result.saved_database.image_from_civis_section=true;
    }
    result.pregame = std::move(setup);
    return result;
}

SavedSession read_saved_session(const std::filesystem::path &path,
                                const SavedSessionDecodeContext &context,
                                SavePayloadBudget payload_budget,
                                CivisSaveBudget civis_budget,
                                HeaderBudget metadata_budget,
                                GameStateBudget state_budget) {
    SavedSession result;
    result.source = read_loaded_save(path, payload_budget, civis_budget, metadata_budget);
    result.snapshot = observe_loaded_session(result.source, context, state_budget);
    return result;
}
} // namespace civ5::saves
