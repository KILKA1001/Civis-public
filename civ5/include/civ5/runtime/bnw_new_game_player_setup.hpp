#pragma once
#include "civ5/runtime/bnw_new_game_setup_rules.hpp"
#include "civ5/runtime/bnw_fresh_team_info_init.hpp"
#include <array>
#include <map>
#include <string>
#include <optional>
#include <utility>
#include <vector>
#include <thread>
namespace civ5::runtime {
class BnwFreshGame;
class BnwFreshTacticalMap;
struct BnwNewGameMajorChoice {
    BnwFreshSlotStatus status=BnwFreshSlotStatus::closed;
    std::string civilization,leader;
    std::optional<std::int32_t> team;
    std::optional<std::string> player_color;
};
struct BnwNewGameMinorChoice {
    std::string minor_civilization;
    std::optional<std::int32_t> team;
};
struct BnwNewGamePlayerSetupRequest {
    std::array<BnwNewGameMajorChoice,22> majors;
    std::vector<BnwNewGameMinorChoice> minors;
    std::int32_t active_player=0;
};
struct BnwNewGamePregameSlot {
    BnwFreshSlotStatus status=BnwFreshSlotStatus::closed;
    std::int32_t team=-1,civilization=-1,leader=-1,minor_type=-1,handicap=-1,color=-1,net_id=-1;
    bool minor=false;
};
enum class BnwNewGamePlayerSetupBoundary { failed_prefix, player_reset_required, first_player_homeland_required };
struct BnwNewGamePlayerSetupObservation {
    BnwNewGamePlayerSetupBoundary boundary=BnwNewGamePlayerSetupBoundary::failed_prefix;
    std::array<BnwNewGamePregameSlot,64> roster;
    std::array<std::int32_t,80> color_workspace{};
    // Detached diagnostics of actually owned initialized Team storage. These
    // copies cannot issue a Team/Player/Map source or mutate the owning Game.
    std::vector<BnwFreshTeamInfoArrays> teams;
    std::size_t initialized_teams=0;
    std::optional<std::int32_t> first_player_id,first_player_tactical_priority_turn;
    std::optional<bool> first_player_tactical_reset_map_null;
    std::vector<std::string> first_player_reset_order;
    bool first_player_tactical_game_map_bound=false;
    std::map<std::string,std::int32_t,std::less<>> first_player_tactical_integers;
    std::optional<float> first_player_tactical_flavor_dampening;
    std::vector<std::pair<std::string,std::int32_t>> first_player_tactical_cached_types;
    std::string selected_rules_sha256;
    std::vector<std::string> order,mandatory;
};
// No public factory, mutator or source getter. Only SAME FreshGame can own this
// realm. A selected slot is not an initialized/ever-alive Player.
class BnwNewGamePlayerSetup final {
 public:
    ~BnwNewGamePlayerSetup();
    BnwNewGamePlayerSetup(const BnwNewGamePlayerSetup&)=delete;
 private:
    friend class BnwFreshGame;
    BnwNewGamePlayerSetup(const BnwFreshGame&,std::shared_ptr<const BnwNewGameSetupRules>,
                         const content::RulesDatabase&,BnwNewGamePlayerSetupRequest);
    void advance_team_roster_prefix(const BnwFreshGame&);
    void advance_first_player_reset_prefix(const BnwFreshGame&,
        std::shared_ptr<const BnwFreshGameContentResult>,const BnwFreshTacticalMap&,std::int32_t actual_game_handicap);
    BnwNewGamePlayerSetupObservation observe_from_game(const BnwFreshGame&)const;
    void require_same_game(const BnwFreshGame&)const;
    struct Impl;std::unique_ptr<Impl> impl_;
};
}
