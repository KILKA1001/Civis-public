#pragma once
#include "civ5/saves/all_players.hpp"
#include "civ5/saves/game_history.hpp"
#include "civ5/saves/game_leagues.hpp"
#include "civ5/saves/game_progression.hpp"
#include "civ5/saves/game_religion_culture.hpp"
#include "civ5/saves/game_tail.hpp"
#include "civ5/saves/game_trade.hpp"
#include "civ5/saves/loaded_save.hpp"
#include "civ5/saves/pregame.hpp"
#include "civ5/saves/map_regions.hpp"
#include "civ5/saves/map_state.hpp"
#include "civ5/saves/supporting_classes.hpp"
#include "civ5/saves/team_state.hpp"
#include <set>

namespace civ5::saves {
struct SavedSessionDecodeContext {
    // These dimensions describe the selected source GameCore wire contract.
    // They are never reused as host allocation limits.
    GameStateLayout layout = GameStateLayout::expansion2_version1;
    RosterWireDimensions roster{64, 64};
    ProgressionWireDimensions progression{63, 5};
    TradeWireDimensions trade{22, 6};
    PlotWireDimensions plots{80, 80, 22, 6, 1};
    RegionWireDimensions regions{};
    TeamWireDimensions teams{};

    // The map header is always needed by the session boundary. Later object
    // collections are opt-in because their source dimensions depend on rules.
    bool decode_map_plots = true;
    bool decode_map_regions = false;
    bool decode_teams = false;
    bool decode_all_players = false;
    bool decode_supporting_classes = false;
    // Opt in only when the caller will also apply LoadedSave's live section to
    // the common city runtime. Source-only observation must not look current.
    bool permit_live_production_overlay = false;
    bool permit_live_research_overlay = false;
    bool permit_live_treasury_overlay = false;
    bool permit_live_research_turn_overlay = false;
    bool permit_live_research_selection_overlay = false;
    bool permit_live_random_overlay = false;
    bool permit_live_bnw_game_clock_overlay = false;
    bool permit_live_bnw_map_plots_overlay = false;
    bool permit_live_bnw_units_overlay = false;
    // Original source decode only; genuine LiveSession must restore the allocator.
    bool permit_live_bnw_unit_serial_overlay = false;
    // Decode immutable OriginalPayload only. Food47 still requires the private
    // unpublished Host/Session parent+child installer before publication.
    bool permit_live_bnw_food_growth_original_decode = false;
    bool permit_live_bnw_area_units_overlay = false;
    bool permit_live_bnw_player_proximity_overlay = false;
    bool permit_live_bnw_team_meet_overlay = false;
    bool permit_live_bnw_exploration_dirty_overlay = false;
    bool permit_live_bnw_player_strategy_overlay = false;
    bool permit_live_bnw_player_core_overlay = false;
    bool permit_live_bnw_city_core_overlay = false;
    bool permit_live_bnw_city_production_overlay = false;
    bool permit_live_bnw_city_management_overlay = false;
    bool permit_live_bnw_city_tail_overlay = false;
    bool permit_live_bnw_treasury_balance_overlay = false;
    bool permit_live_bnw_team_progress_overlay = false;
    bool permit_live_bnw_culture_bomb_timer_overlay = false;
    bool permit_live_bnw_research_queue_overlay = false;
    bool permit_live_bnw_maintenance_expense_overlay = false;
    bool permit_live_bnw_player_knowledge_overlay = false;
    bool permit_live_bnw_player_administration_overlay = false;
    bool permit_live_bnw_game_supporting_overlay = false;
    bool permit_live_bnw_game_score_replay_overlay = false;
    bool permit_live_bnw_game_deals_overlay = false;
    bool permit_live_bnw_team_turn_overlay = false;
    bool permit_live_bnw_ui_transients_overlay = false;
    // A profile consumer must explicitly claim every opaque section it can
    // restore. Common session observation never interprets these bytes.
    std::set<std::uint32_t> permitted_profile_sections;
    // Read-only map diagnostic only. A non-SQLite declared database image is
    // left unread, and the snapshot must not be treated as loadable gameplay.
    bool skip_non_sqlite_image_for_map_diagnostic = false;
    std::optional<AllPlayersContext> all_players;
    // Optional feature lookup from the selected saved-content profile. Without
    // it, cells with unresolved features retain only a non-final base mask.
    SavedTerrainFeatureResolver terrain_features;
};

struct SavedSessionSnapshot {
    // Absent for payload-only observation and legacy imports without a prefix.
    std::optional<PregameObservation> pregame;
    GameStateObservation game_state;
    GameProgressionObservation progression;
    GameHistoryObservation history;
    GameReligionCultureObservation religion_culture;
    GameLeaguesObservation leagues;
    GameTradeObservation trade;
    GameFinalSystemsObservation final_systems;
    SavedDatabaseObservation saved_database;
    MapHeaderObservation map_header;
    std::optional<MapPlotsObservation> map_plots;
    std::optional<SavedMapProjection> map_projection;
    std::optional<SavedMapTerrainSource> map_terrain;
    std::optional<MapRegionsObservation> map_regions;
    std::optional<TeamsObservation> teams;
    std::optional<AllPlayersObservation> all_players;
    std::optional<SupportingClassesObservation> supporting_classes;
    std::uint64_t payload_end_offset = 0;
};

// One source container and its owned decoded observations. Logic and graphics
// consumers should keep this pair together so they cannot accidentally use
// different payloads or save headers for the same session.
struct SavedSession {
    LoadedSave source;
    SavedSessionSnapshot snapshot;
};

// Read the serialized session in source order into one owned x64 snapshot.
// No load hooks, RNG advancement, ID remapping, Lua or native DLL code runs.
// The caller supplies source dimensions from the selected rules profile; no
// byte-search or original 32-bit capacity is used to infer them.
SavedSessionSnapshot observe_saved_session(std::span<const std::uint8_t> payload,
                                           const SavedSessionDecodeContext &context,
                                           GameStateBudget budget = {});

// Decode setup and payload from the same source. A missing legacy prefix stays
// absent; a present but unsupported/corrupt setup is never replaced by defaults.
SavedSessionSnapshot observe_loaded_session(const LoadedSave &source,
                                            const SavedSessionDecodeContext &context,
                                            GameStateBudget budget = {}, PregameBudget setup_budget = {});

// Read one original or Civis container and decode its session from the same
// loaded bytes. No original executable, DLL, Lua or load hook is invoked.
SavedSession read_saved_session(const std::filesystem::path &path,
                                const SavedSessionDecodeContext &context,
                                SavePayloadBudget payload_budget = {},
                                CivisSaveBudget civis_budget = {},
                                HeaderBudget metadata_budget = {},
                                GameStateBudget state_budget = {});
} // namespace civ5::saves
