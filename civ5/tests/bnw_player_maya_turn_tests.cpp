#include "civ5/runtime/bnw_player_maya_turn.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/live_session.hpp"
#include <iostream>
#include <stdexcept>

namespace {
using namespace civ5;
using namespace civ5::runtime;
void check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}
SessionCheckpointIdentity identity(const content::RulesDatabase &db) {
    return {"fixture", identify_bytes(db.gameplay_fingerprint_data()).sha256};
}
LiveSessionState fixture(const char *leader) {
    LiveSessionState state{saves::SavedSessionSnapshot{}, ResearchContext{}, {},
        PlayerResearchContext{}, CityYieldContext{}, BnwUnhappinessContext{},
        MaintenanceContext{}, TreasuryContext{}, ScienceContext{}, ResearchTurnState{},
        {SimulationRandom(RandomAlgorithm::bnw, {3,0,0}),
         SimulationRandom(RandomAlgorithm::bnw, {5,0,0})}};
    constexpr auto bnw = saves::GameStateLayout::expansion2_version1;
    state.setup.profile = content::TechnologyProfile::bnw;
    state.setup.players[0].alive = true;
    state.world.game_state.layout = bnw;
    state.world.pregame.emplace().layout = bnw;
    state.world.pregame->slots.string_arrays["leader_keys"] = {leader};
    state.world.all_players.emplace().layout = bnw;
    auto &saved = state.world.all_players->players.emplace_back();
    saved.saved_player_id = 0;
    saved.core.layout = bnw;
    state.research_turn.players[0].leader_type = leader;
    return state;
}
void blocked(LiveSessionState &state, const content::RulesDatabase &db,
             BnwMayaTurnBlocker blocker) {
    const auto rng = state.random.gameplay.state();
    const auto leader = state.world.pregame->slots.string_arrays.at("leader_keys").at(0);
    bool seen = false;
    try { (void)apply_bnw_player_maya_turn(state, 0, db, identity(db)); }
    catch (const BnwMayaTurnError &error) { seen = error.blocker() == blocker; }
    check(seen, "wrong BNW Maya typed blocker");
    check(state.random.gameplay.state() == rng &&
          state.world.pregame->slots.string_arrays.at("leader_keys").at(0) == leader,
          "blocked Maya step changed RNG or saved leader");
}
} // namespace

int main() {
    try {
        content::RulesDatabase db;
        db.apply_sql("CREATE TABLE Leaders(ID INTEGER, Type TEXT);"
            "INSERT INTO Leaders VALUES(0,'LEADER_GANDHI'),(1,'LEADER_PACAL');"
            "CREATE TABLE Traits(ID INTEGER, Type TEXT, MayaCalendarBonuses INTEGER);"
            "INSERT INTO Traits VALUES(0,'TRAIT_ORDINARY',0),"
            "(1,'TRAIT_MAYA',1),(2,'TRAIT_MAYA_LATER',1);"
            "CREATE TABLE Leader_Traits(LeaderType TEXT,TraitType TEXT);"
            "INSERT INTO Leader_Traits VALUES('LEADER_GANDHI','TRAIT_ORDINARY'),"
            "('LEADER_PACAL','TRAIT_MAYA');");
        {
            auto state = fixture("LEADER_GANDHI");
            const auto rng = state.random.gameplay.state();
            const auto result = apply_bnw_player_maya_turn(state, 0, db, identity(db));
            check(result.player_id == 0 && !result.uses_maya_calendar &&
                  state.random.gameplay.state() == rng,
                  "non-Maya leader should leave calendar and RNG untouched");
        }
        {
            auto state = fixture("LEADER_PACAL");
            blocked(state, db, BnwMayaTurnBlocker::maya_calendar_and_boost);
        }
        {
            auto state = fixture("LEADER_UNKNOWN");
            blocked(state, db, BnwMayaTurnBlocker::leader);
        }
        {
            auto state = fixture("LEADER_GANDHI");
            state.research_turn.players[0].leader_type = "LEADER_PACAL";
            blocked(state, db, BnwMayaTurnBlocker::leader);
        }
        {
            auto state = fixture("LEADER_GANDHI");
            const auto wrong = SessionCheckpointIdentity{"fixture", "wrong"};
            bool seen = false;
            try { (void)apply_bnw_player_maya_turn(state, 0, db, wrong); }
            catch (const BnwMayaTurnError &error) {
                seen = error.blocker() == BnwMayaTurnBlocker::selected_rules;
            }
            check(seen, "selected rules mismatch did not fail closed");
        }
        std::cout << "BNW Maya non-calendar turn branch PASS\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
