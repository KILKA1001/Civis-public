#pragma once
#include "civ5/runtime/bnw_new_game_setup_rules.hpp"
#include "civ5/runtime/bnw_map_script.hpp"
#include <set>
namespace civ5::runtime {
// Actual menu inputs. They cannot issue initialized players, a map or a session.
struct BnwNewGameSetupRequest {
    std::uint32_t map_seed=0,sync_seed=0;
    std::int32_t max_turns=0;
    std::map<std::string,bool,std::less<>> game_options;
    std::set<std::string,std::less<>> disabled_victories;
    BnwMapSourceKey map_source;
    // Explicit per-script OptionID/value choices. Defaults are supplied by
    // the actual script/menu owner; never inferred from Climate or SeaLevel.
    std::map<std::int32_t,std::int32_t> map_custom_options;
};
enum class BnwNewGameSetupBoundary { failed_prefix, turn_publication_required, players_init_required };
struct BnwNewGameSetupObservation {
    BnwNewGameSetupBoundary boundary=BnwNewGameSetupBoundary::failed_prefix;
    BnwCreationMetadata selected;
    BnwMapSourceKey map_source;
    std::map<std::int32_t,std::int32_t> map_custom_options;
    std::map<std::string,bool,std::less<>> game_options;
    std::vector<bool> victory_valid;
    std::int32_t game_turn=0,max_turns=0;
    std::optional<std::int32_t> pending_game_turn,start_turn,default_end_turn,estimated_end_turn,start_year;
    std::string admin_password;
    bool quick_combat=false;
    std::optional<std::vector<std::array<std::int32_t,3>>> goody_memory;
    SimulationRandomState map_random,gameplay_random;
    std::vector<std::string> mandatory;
};
}
