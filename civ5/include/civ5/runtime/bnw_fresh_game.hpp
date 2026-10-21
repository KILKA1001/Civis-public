#pragma once
#include "civ5/runtime/bnw_creation_config.hpp"
#include "civ5/runtime/simulation_random.hpp"
#include "civ5/runtime/bnw_fresh_pregame_speed.hpp"
#include "civ5/runtime/bnw_new_game_setup.hpp"
#include "civ5/runtime/bnw_new_game_player_setup.hpp"
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>
namespace civ5::runtime {
class BnwFreshTacticalMap;
class BnwSelectedUserOptionsOwner;
class BnwFreshCityManager;
class BnwFreshAdvisorCounsel;
class BnwFreshAdvisorRecommender;
enum class BnwFreshGameError { parent_missing, wrong_thread, parent_retired,
    cache_path_unavailable, cache_effect_failed, native_rules_unavailable,
    already_attempted, poisoned, field_unwritten, wrong_phase };
class BnwFreshGameRefusal : public std::runtime_error {
 public:
    BnwFreshGameRefusal(BnwFreshGameError,const std::string&);
    BnwFreshGameError code() const noexcept { return code_; }
 private: BnwFreshGameError code_;
};
enum class BnwFreshGameBoundary { constructor_reset_complete, settler_site_init_required,
    failed_prefix, start_site_construction_required, start_positioner_construction_required, game_deals_init_required, game_religions_speed_source_required, game_culture_construction_required, tactical_map_construction_required, city_manager_reset_required, scoped_reset_complete_owned_cities_required, init_mp_options_roster_required, init_lock_mods_speed_required, constructor_scoped_init_victory_setup_required, init_turn_changed_hooks_required, pregame_menu_options_required, pregame_settings_source_required, pregame_civilization_info_required, pregame_last_speed_required, ordered_globals_player_colors_required, civis_turn_publication_required, civis_player_init_required, civis_player_reset_required, civis_first_player_homeland_required };
using BnwFreshGameScalar=std::variant<std::int32_t,std::uint32_t,bool,float,std::string>;
struct BnwFreshGameArray {
    // NULL allocation versus native allocated-but-unwritten slots remain distinct.
    std::optional<std::vector<std::optional<std::int32_t>>> slots;
    std::optional<std::vector<bool>> bool_slots;
};
struct BnwFreshGameObservation {
    BnwFreshGameBoundary boundary;
    std::map<std::string,BnwFreshGameScalar,std::less<>> scalars;
    std::map<std::string,BnwFreshGameArray,std::less<>> arrays;
    std::optional<std::vector<std::vector<std::int32_t>>> team_victory_rank;
    SimulationRandomState map_random, gameplay_random;
    std::filesystem::path local_cache_database;
    std::uint32_t cache_resets;
    std::vector<std::string> order;
};
// Actual private fresh allocation. Observations are detached diagnostics and
// cannot issue Game, map, session, player, Lua, or initialization authority.
class BnwFreshGame {
 public:
    static std::shared_ptr<BnwFreshGame> from_creation_config(
        std::shared_ptr<const BnwCreationConfig>);
    // Civis gameplay initialization from genuine activated Content on SAME
    // owned Game. It does not issue legacy Globals/Skin/Pregame receipts.
    // Ancient 0->0 reaches InitPlayers; nonzero turn publication is mandatory.
    void initialize_civis_single_player_setup(BnwNewGameSetupRequest);
    BnwNewGameSetupObservation civis_setup_observation()const;
    // Actual SAME Game teams and chosen roster prefix. Stops before genuine
    // player Reset/setAlive/AI; no map player API is issued from selected slots.
    void initialize_civis_team_roster_prefix(BnwNewGamePlayerSetupRequest);
    BnwNewGamePlayerSetupObservation civis_player_setup_observation()const;
    // Genuine existing executable first-player Reset prefix from privately
    // retained roster/Content, then complete actual Tactical Game source and stop before HomelandAI.
    void advance_civis_first_player_reset_prefix();
    // Actual early namespace/menu setup retained by SAME Game. Mandatory
    // options/settings frontiers throw; no separate supplied Pregame accepted.
    void advance_single_player_pregame_menu_prefix(std::filesystem::path selected_config_ini);
    void advance_single_player_pregame_menu_options_prefix();
    BnwFreshPregameSpeedObservation pregame_setup_observation()const;
    // Actual first host loader key from SAME Boot registry. Stops before
    // reached CivilizationInfo source or the next LastSpeed operation.
    void advance_pregame_settings_last_civilization_prefix(std::shared_ptr<const BnwSelectedUserOptionsOwner>);
    // One nonconstructor reset attempt, with the SAME selected Handicap.
    // Stops after Settler evaluator construction, BEFORE its mandatory Init.
    void advance_reset_prefix();
    // SAME allocated Settler: native inherited Init, once, then stop BEFORE
    // the mandatory Start evaluator construction/Init. Never repeats reset.
    void advance_settler_site_init();
    // Native Start construction and inherited Init on SAME Game allocation.
    // Stops BEFORE StartPositioner construction/Init/ComputeFlavorMultipliers.
    void advance_start_site_init();
    // Native StartPositioner constructor/Init on actual SAME owned Start.
    // Stops BEFORE next mandatory GameDeals Init; no region/fertility/map work.
    void advance_start_positioner_init();
    // Actual Deals clear3 and GameReligions constructor/first Init store.
    // Stops BEFORE unresolved actual CvPreGame::gameSpeed read; config
    // descriptors are not a performed Pregame setter or full Init authority.
    void advance_deals_game_religions_prefix();
    // Bind actual retained Pregame speed setter owner, then native religion
    // multiply/divide. Actual timer-localization errors preserve prior stores.
    void advance_game_religions_speed_init(std::filesystem::path selected_config_ini);
    // SAME retained Game: Culture constructor, Leagues constructor/empty Init,
    // Trade constructor/Init Reset5 twice. Stops BEFORE Tactical allocation.
    void advance_culture_leagues_trade_init();
    // Actual private constructors only; then reset tail, stopping before
    // missing genuine CityManager.Reset. Tactical.Init remains unexecuted.
    void advance_tactical_advisors_reset_tail();
    // Aliases retain SAME actual Game allocation; no public Init/mutation.
    std::shared_ptr<const BnwFreshTacticalMap> tactical_analysis_map() const;
    std::shared_ptr<const BnwFreshAdvisorCounsel> advisor_counsel() const;
    std::shared_ptr<const BnwFreshAdvisorRecommender> advisor_recommender() const;
    // Final scoped Reset on SAME pointer realm; no global city allocation,
    // Init/new session/map readiness is inferred from empty cache clear.
    void advance_city_manager_reset();
    // Consumes SAME private scoped reset return once. Actual fresh setters,
    // vote Init8, RNG init residues and era verification; no reset replay.
    // Stops BEFORE required MP/options/player-roster initialization branches.
    void advance_init_prefix(BnwFreshPregameInitRequest);
    // Native SINGLE_PLAYER branch only. Real MP setters and string options
    // with full enum Sync, then actual shuffle=false; no roster/RNG skip grant.
    // Stops BEFORE LOCK_MODS password and owned GameSpeed turn-start pipeline.
    void advance_single_player_options();
    // Scoped Globals constructor logging source; actual LOCK/password, turn
    // cache and reached scalar prefix. Pregame resetGame/setup is NOT issued.
    void advance_constructor_scoped_lock_mods_turn_prefix();
    std::shared_ptr<const BnwFreshCityManager> city_manager() const;
    // Genuine ordered Globals route begins BEFORE any menu/ResetSlots.
    // Full queue/tail mandatory; observations cannot authorize LoadSettings.
    void advance_ordered_globals_info_prefix(std::filesystem::path selected_config_ini);
    BnwFreshOrderedInfoObservation ordered_globals_info_observation()const;
    void require_current() const;
    BnwFreshGameObservation observe() const;
    // Separate diagnostic child trace, not a merged global execution history.
    std::vector<std::string> pregame_order() const;
 private:
    struct Impl;
    explicit BnwFreshGame(std::shared_ptr<Impl>);
    // Synchronous closed member calls from exact owning Impl; no callbacks,
    // borrowed facade stored in Impl, caller DTO or independently supplied realm.
    void city_manager_reset_from_impl(const Impl&);
    void city_manager_shutdown_from_impl(const Impl&);
    std::shared_ptr<Impl> impl_;
};
} // namespace civ5::runtime
