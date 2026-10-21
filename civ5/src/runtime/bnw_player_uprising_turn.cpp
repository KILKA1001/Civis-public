#include "civ5/runtime/bnw_player_uprising_turn.hpp"
#include "civ5/runtime/bnw_unhappiness.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/content/rules_database.hpp"
#include <limits>
#include <optional>
#include <string_view>
#include <type_traits>
#include <variant>

namespace civ5::runtime {
namespace {
using B=BnwUprisingTurnBlocker;
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
static_assert(std::is_nothrow_move_assignable_v<SimulationRandom>);
void require(bool condition,B blocker,const char *message) {
    if(!condition) throw BnwUprisingTurnError(blocker,message);
}
const saves::PlayerCoreField &field(const saves::PlayerCoreObservation &core,
                                    std::string_view name) {
    const saves::PlayerCoreField *result=nullptr;
    for(const auto &candidate:core.fields)
        if(candidate.name && std::string_view(candidate.name)==name) {
            require(!result,B::player,"duplicate BNW uprising player field");
            result=&candidate;
        }
    require(result && result->values.size()==1,B::player,
            "BNW uprising player field unavailable");
    return *result;
}
std::int32_t i32(std::int64_t value,B blocker,const char *message) {
    require(value>=std::numeric_limits<std::int32_t>::min() &&
            value<=std::numeric_limits<std::int32_t>::max(),blocker,message);
    return static_cast<std::int32_t>(value);
}
std::int32_t define(const content::GlobalDefineCache &cache,std::string_view name) {
    const auto *entry=cache.find(name);
    require(entry && entry->lookup.found && entry->lookup.error.empty() &&
            std::holds_alternative<std::int64_t>(entry->lookup.value),
            B::selected_rules,"selected BNW uprising define unavailable");
    return i32(std::get<std::int64_t>(entry->lookup.value),B::selected_rules,
               "selected BNW uprising define exceeds source int");
}
std::int32_t train_percent(const content::RulesDatabase &rules,
                           std::int64_t speed_id) {
    std::optional<std::int64_t> selected;
    const auto report=rules.query_rows({
        "SELECT TrainPercent FROM GameSpeeds WHERE ID = ?",{speed_id}},
        [&](const auto &row) {
            require(!selected,B::game_speed,"duplicate selected BNW game speed");
            selected=row.integer_at(0);
        },2);
    require(report.all_succeeded() && report.rows==1 && selected,
            B::game_speed,"selected BNW game speed unavailable");
    const auto value=i32(*selected,B::game_speed,
                         "BNW game speed TrainPercent exceeds source int");
    require(value>=0,B::game_speed,"negative BNW game speed TrainPercent");
    return value;
}
} // namespace

BnwUprisingTurnStep apply_bnw_player_uprising_turn(
    LiveSessionState &state,std::int64_t player_id,
    const content::RulesDatabase &rules,const SessionCheckpointIdentity &identity) {
    require(state.setup.profile==content::TechnologyProfile::bnw &&
            state.cities.profile==content::TechnologyProfile::bnw &&
            state.world.game_state.layout==bnw &&
            state.world.all_players && state.world.all_players->layout==bnw &&
            state.random.gameplay.algorithm()==RandomAlgorithm::bnw,
            B::profile,"BNW uprising turn requires restored Expansion2 profile");
    require(!identity.rules_sha256.empty() &&
            identify_bytes(rules.gameplay_fingerprint_data()).sha256==identity.rules_sha256,
            B::selected_rules,"BNW uprising selected rules identity differs");
    const auto defines=rules.cache_global_defines(content::GlobalDefineProfile::bnw);
    const auto threshold=define(defines,"VERY_UNHAPPY_THRESHOLD");
    const auto setup=state.setup.players.find(player_id);
    require(setup!=state.setup.players.end() && setup->second.alive,
            B::player,"BNW uprising requires alive restored player");
    saves::SavedPlayerObservation *saved=nullptr;
    for(auto &candidate:state.world.all_players->players)
        if(candidate.saved_player_id==player_id) {
            require(!saved,B::player,"duplicate saved BNW player ID");
            saved=&candidate;
        }
    require(saved && saved->core.layout==bnw,B::player,
            "BNW uprising saved player unavailable");
    const auto &id=field(saved->core,"m_eID");
    const auto &alive=field(saved->core,"m_bAlive");
    const auto &counter=field(saved->core,"m_iUprisingCounter");
    require(id.encoding==saves::PlayerCoreEncoding::i32 &&
            id.values[0]==player_id &&
            alive.encoding==saves::PlayerCoreEncoding::boolean &&
            alive.values[0]==1 &&
            counter.encoding==saves::PlayerCoreEncoding::i32,
            B::player,"BNW uprising player identity or counter encoding differs");
    const auto before=i32(counter.values[0],B::player,
                          "BNW uprising counter exceeds source int");
    auto setup_copy=state.setup;
    bool no_happiness=false;
    try {no_happiness=setup_copy.game_option_value(rules,"GAMEOPTION_NO_HAPPINESS")==1;}
    catch(const std::exception &) {
        throw BnwUprisingTurnError(B::selected_rules,
                                   "BNW no-happiness option unresolved");
    }
    require(state.cities.no_happiness==no_happiness,B::happiness,
            "BNW live happiness option differs from selected setup");
    bool very_unhappy=false;
    if(!no_happiness) {
        try {
            const auto unhappiness_rules=load_bnw_unhappiness_rules(rules);
            const auto quote=bnw_unhappiness(state.unhappiness,state.setup,
                                             state.cities,unhappiness_rules,
                                             defines,player_id);
            very_unhappy=quote.excess_happiness<=threshold;
        } catch(const std::exception &) {
            throw BnwUprisingTurnError(B::happiness,
                                       "BNW live excess happiness unavailable");
        }
    }
    if(!very_unhappy)
        return {player_id,before,before,BnwUprisingTurnBranch::not_very_unhappy,0};
    if(before==1)
        throw BnwUprisingTurnError(B::uprising_spawn,
            "BNW DoUprising spawn/plot effects and following reset are unavailable");
    if(before>1) {
        for(auto &candidate:saved->core.fields)
            if(candidate.name && std::string_view(candidate.name)=="m_iUprisingCounter") {
                candidate.values[0]=before-1;
                break;
            }
        return {player_id,before,static_cast<std::int32_t>(before-1),
                BnwUprisingTurnBranch::decrement,0};
    }

    // DoResetUprisingCounter(true). Validate all selected inputs before a
    // speculative draw. CvRandom::get consumes exactly one call even at 0/1.
    const auto minimum=define(defines,"UPRISING_COUNTER_MIN");
    const auto possible=define(defines,"UPRISING_COUNTER_POSSIBLE");
    require(possible>=0 && possible<=std::numeric_limits<std::uint16_t>::max(),
            B::source_integer_range,"BNW uprising RNG bound exceeds source u16");
    const auto speed=train_percent(rules,state.setup.speed);
    auto draft=state.random.gameplay;
    std::uint64_t extra;
    try {extra=draft.draw(static_cast<std::uint16_t>(possible));}
    catch(const std::exception &) {
        throw BnwUprisingTurnError(B::source_integer_range,
                                   "BNW uprising RNG draft unavailable");
    }
    auto turns=static_cast<std::int64_t>(minimum)+static_cast<std::int64_t>(extra);
    turns=i32(turns,B::source_integer_range,
              "BNW uprising reset addition overflows source int");
    if(speed>100) {
        turns=i32(turns*static_cast<std::int64_t>(speed),B::source_integer_range,
                  "BNW uprising speed multiplication overflows source int");
        turns/=100;
    }
    turns/=2;
    if(turns<=0) turns=1;
    state.random.gameplay=std::move(draft);
    for(auto &candidate:saved->core.fields)
        if(candidate.name && std::string_view(candidate.name)=="m_iUprisingCounter") {
            candidate.values[0]=turns;
            break;
        }
    return {player_id,before,static_cast<std::int32_t>(turns),
            BnwUprisingTurnBranch::first_seed,1};
}
} // namespace civ5::runtime
