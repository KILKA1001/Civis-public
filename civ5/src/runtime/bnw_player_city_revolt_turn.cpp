#include "civ5/runtime/bnw_player_city_revolt_turn.hpp"
#include "civ5/runtime/bnw_unhappiness.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/content/rules_database.hpp"
#include <limits>
#include <string_view>
#include <variant>

namespace civ5::runtime {
namespace {
using B=BnwCityRevoltTurnBlocker;
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void require(bool condition,B blocker,const char *message) {
    if(!condition) throw BnwCityRevoltTurnError(blocker,message);
}
const saves::PlayerCoreField &field(const saves::PlayerCoreObservation &core,
                                    std::string_view name) {
    const saves::PlayerCoreField *result=nullptr;
    for(const auto &candidate:core.fields)
        if(candidate.name && std::string_view(candidate.name)==name) {
            require(!result,B::player,"duplicate BNW city revolt player field");
            result=&candidate;
        }
    require(result && result->values.size()==1,B::player,
            "BNW city revolt player field unavailable");
    return *result;
}
std::int32_t i32(std::int64_t value,B blocker,const char *message) {
    require(value>=std::numeric_limits<std::int32_t>::min() &&
            value<=std::numeric_limits<std::int32_t>::max(),blocker,message);
    return static_cast<std::int32_t>(value);
}
std::int32_t define(const content::GlobalDefineCache &cache,
                    std::string_view name) {
    const auto *entry=cache.find(name);
    require(entry && entry->lookup.found && entry->lookup.error.empty() &&
            std::holds_alternative<std::int64_t>(entry->lookup.value),
            B::selected_rules,"selected BNW city revolt define unavailable");
    return i32(std::get<std::int64_t>(entry->lookup.value),B::selected_rules,
               "selected BNW city revolt define exceeds source int");
}
} // namespace

BnwCityRevoltTurnStep apply_bnw_player_city_revolt_turn(
    LiveSessionState &state,std::int64_t player_id,
    const content::RulesDatabase &rules,const SessionCheckpointIdentity &identity) {
    require(state.setup.profile==content::TechnologyProfile::bnw &&
            state.cities.profile==content::TechnologyProfile::bnw &&
            state.world.game_state.layout==bnw &&
            state.world.all_players && state.world.all_players->layout==bnw &&
            state.random.gameplay.algorithm()==RandomAlgorithm::bnw,
            B::profile,"BNW city revolt requires restored Expansion2 profile");
    require(!identity.rules_sha256.empty() &&
            identify_bytes(rules.gameplay_fingerprint_data()).sha256==identity.rules_sha256,
            B::selected_rules,"BNW city revolt selected rules identity differs");
    const auto defines=rules.cache_global_defines(content::GlobalDefineProfile::bnw);
    const auto threshold=define(defines,"SUPER_UNHAPPY_THRESHOLD");
    const auto setup=state.setup.players.find(player_id);
    require(setup!=state.setup.players.end() && setup->second.alive,
            B::player,"BNW city revolt requires alive restored player");
    saves::SavedPlayerObservation *saved=nullptr;
    for(auto &candidate:state.world.all_players->players)
        if(candidate.saved_player_id==player_id) {
            require(!saved,B::player,"duplicate saved BNW player ID");
            saved=&candidate;
        }
    require(saved && saved->core.layout==bnw,B::player,
            "BNW city revolt saved player unavailable");
    const auto &id=field(saved->core,"m_eID");
    const auto &alive=field(saved->core,"m_bAlive");
    const auto &counter=field(saved->core,"m_iCityRevoltCounter");
    require(id.encoding==saves::PlayerCoreEncoding::i32 &&
            id.values[0]==player_id &&
            alive.encoding==saves::PlayerCoreEncoding::boolean &&
            alive.values[0]==1 &&
            counter.encoding==saves::PlayerCoreEncoding::i32,
            B::player,"BNW city revolt player identity or counter encoding differs");
    const auto before=i32(counter.values[0],B::player,
                          "BNW city revolt counter exceeds source int");

    auto setup_copy=state.setup;
    bool no_happiness=false;
    try {no_happiness=setup_copy.game_option_value(rules,"GAMEOPTION_NO_HAPPINESS")==1;}
    catch(const std::exception &) {
        throw BnwCityRevoltTurnError(B::selected_rules,
                                     "BNW no-happiness option unresolved");
    }
    require(state.cities.no_happiness==no_happiness,B::happiness,
            "BNW live happiness option differs from selected setup");
    bool active=false;
    if(!no_happiness) {
        try {
            const auto unhappiness_rules=load_bnw_unhappiness_rules(rules);
            const auto quote=bnw_unhappiness(state.unhappiness,state.setup,
                                             state.cities,unhappiness_rules,
                                             defines,player_id);
            (void)i32(quote.excess_happiness,B::happiness,
                      "BNW city revolt excess happiness exceeds source int");
            (void)i32(quote.public_opinion,B::happiness,
                      "BNW city revolt public opinion exceeds source int");
            active=quote.excess_happiness<=threshold && quote.public_opinion>0;
        } catch(const std::exception &) {
            throw BnwCityRevoltTurnError(B::happiness,
                                         "BNW live super-unhappiness unavailable");
        }
    }
    if(!active) return {player_id,before,before,BnwCityRevoltTurnBranch::inactive};
    if(before==1)
        throw BnwCityRevoltTurnError(B::city_revolt_and_transfer,
            "BNW DoCityRevolt notifications, acquireCity and unit relocation unavailable");
    if(before<=0)
        throw BnwCityRevoltTurnError(B::city_target_and_notification,
            "BNW revolt reset needs target ideology, recipient and notification effects");
    for(auto &candidate:saved->core.fields)
        if(candidate.name && std::string_view(candidate.name)=="m_iCityRevoltCounter") {
            candidate.values[0]=before-1;
            break;
        }
    return {player_id,before,static_cast<std::int32_t>(before-1),
            BnwCityRevoltTurnBranch::decrement};
}
} // namespace civ5::runtime
