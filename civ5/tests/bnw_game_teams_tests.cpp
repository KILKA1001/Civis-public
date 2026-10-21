#include "civ5/runtime/bnw_game_teams.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <algorithm>
#include <climits>
#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace {
using namespace civ5;
using namespace civ5::runtime;
void check(bool value,const char *reason) {
    if(!value) throw std::runtime_error(reason);
}
LiveSessionState fixture() {
    saves::SavedSessionSnapshot world;
    constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
    world.game_state.layout=bnw;
    world.game_state.flags.push_back({"circumnavigated",false});
    world.map_header.layout=bnw;
    world.map_header.grid={2,1};
    world.teams.emplace();
    world.teams->layout=bnw;
    world.teams->teams.resize(64);
    world.pregame.emplace();
    world.pregame->layout=bnw;
    auto &minor=world.pregame->archive.flag_arrays["minor_nation_civs"];
    minor.resize(64);
    minor[22]=true;
    ResearchContext setup;
    std::vector<TeamTechnologies> techs;
    for(std::int64_t id=0;id<64;++id) {
        const auto alive=id==0 || id==1 || id==22 || id==63;
        auto &team=world.teams->teams[static_cast<std::size_t>(id)];
        team.index=id;
        team.fields={{"id",id},{"alive_count",alive?1:0},
                     {"saved_techs",2}};
        team.arrays={{"at_war",1,std::vector<std::int64_t>(64)},
                     {"turns_at_war",1,std::vector<std::int64_t>(64)},
                     {"turns_locked_at_war",1,std::vector<std::int64_t>(64)},
                     {"no_trade_tech",1,std::vector<std::int64_t>(2)},
                     {"tech_hashes",1,{
                         content::saved_info_type_hash("TECH_ALPHA"),
                         content::saved_info_type_hash("TECH_BETA")}}};
        ResearchPlayerState player;
        player.team=id;
        player.alive=alive;
        setup.players.emplace(id,player);
        techs.emplace_back(content::TechnologyProfile::bnw,id,2);
    }
    auto &zero=world.teams->teams[0];
    zero.arrays[0].values[1]=1;
    zero.arrays[1].values[1]=2;
    zero.arrays[2].values[1]=3;
    zero.arrays[3].values[1]=1;
    world.teams->teams[1].arrays[1].values[0]=5;
    techs[0].set_no_trade(1,true);
    return {std::move(world),std::move(setup),std::move(techs),
        PlayerResearchContext{},CityYieldContext{},BnwUnhappinessContext{},
        MaintenanceContext{},TreasuryContext{},ScienceContext{},ResearchTurnState{},
        {SimulationRandom(RandomAlgorithm::bnw,{5,0,0}),
         SimulationRandom(RandomAlgorithm::bnw,{7,0,0})}};
}
bool has(const BnwTeamTurnPlan &plan,BnwTeamTurnStepKind kind,
         std::int64_t team,std::int64_t target=-1,
         std::int64_t before=0,std::int64_t after=0) {
    return std::any_of(plan.steps.begin(),plan.steps.end(),[&](const auto &step) {
        return step.kind==kind && step.team==team && step.target==target &&
               step.before==before && step.after==after;
    });
}
content::TechnologyRules technologies() {
    content::TechnologyRules result;
    result.collection.rows=result.collection.slots=2;
    result.entries[0].base.type="TECH_ALPHA";
    result.entries[1].base.type="TECH_BETA";
    return result;
}
void selected(const std::filesystem::path &save,
              const std::filesystem::path &root) {
    auto bootstrap=ui::bootstrap_selected_bnw_preview(save,root,0);
    if(bootstrap.preview.blocker)
        throw std::runtime_error(bootstrap.preview.blocker->reason);
    check(bootstrap.preview.host!=nullptr,"Selected BNW host unavailable");
    auto &host=*bootstrap.preview.host;
    auto state=host.session().state();
    const BnwTeamTurnActiveRules rules{host.database(),host.technologies(),host.defines()};
    const auto preflight=preflight_bnw_game_teams_safe_branch(state,rules);
    check(preflight.plan.alive_teams==7 && preflight.barbarian_grants==1 &&
          preflight.minor_grants==0 && !preflight.awards_present &&
          !preflight.circumnavigation_possible && !preflight.safe,
          "Selected BNW team-turn effects changed");
    check(has(preflight.plan,BnwTeamTurnStepKind::barbarian_technology_gate,63) &&
          has(preflight.plan,BnwTeamTurnStepKind::minor_technology_gate,22) &&
          has(preflight.plan,BnwTeamTurnStepKind::minor_technology_gate,23) &&
          has(preflight.plan,BnwTeamTurnStepKind::minor_technology_gate,24) &&
          has(preflight.plan,BnwTeamTurnStepKind::minor_technology_gate,25),
          "Selected BNW technology branches disappeared from team plan");
    const auto before=state.world.teams->teams[0].arrays;
    const auto rng=state.random.gameplay.state();
    const auto result=make_bnw_game_teams_handler(rules).run(state,-1);
    check(result.outcome==(preflight.safe ? LiveActionOutcome::committed :
                                            LiveActionOutcome::rejected) &&
          state.random.gameplay.state()==rng,
          "Selected BNW team branch outcome or RNG changed");
    if(!preflight.safe)
        check(state.world.teams->teams[0].arrays[0].values==before[0].values &&
              state.world.teams->teams[0].arrays[1].values==before[1].values,
              "Incomplete selected BNW team turn mutated world");
    std::cout<<"selected BNW alive teams="<<preflight.plan.alive_teams
             <<" preflight steps="<<preflight.plan.steps.size()
             <<" barbarian grants="<<preflight.barbarian_grants
             <<" minor grants="<<preflight.minor_grants
             <<" awards="<<preflight.awards_present
             <<" circumnavigation="<<preflight.circumnavigation_possible
             <<" safe="<<preflight.safe<<'\n';
}
} // namespace

int main(int argc,char **argv) {
    check(argc==1 || argc==3,"Expected optional selected BNW save and game root");
    content::RulesDatabase database;
    database.apply_sql("CREATE TABLE Defines(Name TEXT,Value TEXT);"
                       "INSERT INTO Defines VALUES('BARBARIAN_TECH_PERCENT','75');"
                       "INSERT INTO Defines VALUES('MINOR_CIV_TECH_PERCENT','40');"
                       "CREATE TABLE SmallAwards(ID INTEGER PRIMARY KEY,Type TEXT);"
                       "CREATE TABLE Technologies(ID INTEGER PRIMARY KEY,Type TEXT);"
                       "INSERT INTO Technologies VALUES(0,'TECH_ALPHA');"
                       "INSERT INTO Technologies VALUES(1,'TECH_BETA');");
    auto tech_rules=technologies();
    auto defines=database.cache_global_defines(content::GlobalDefineProfile::bnw);
    const BnwTeamTurnActiveRules rules{database,tech_rules,defines};
    auto state=fixture();
    const auto plan=plan_bnw_game_teams_prefix(state);
    check(plan.alive_teams==4,"BNW team preflight missed live teams");
    check(plan.steps.size()>=13 &&
          plan.steps[0].kind==BnwTeamTurnStepKind::turns_at_war &&
          plan.steps[0].team==0 && plan.steps[0].target==1 &&
          plan.steps[1].kind==BnwTeamTurnStepKind::turns_locked_at_war &&
          plan.steps[2].kind==BnwTeamTurnStepKind::clear_no_trade_technology &&
          plan.steps[3].kind==BnwTeamTurnStepKind::small_awards_gate &&
          plan.steps[4].kind==BnwTeamTurnStepKind::circumnavigation_gate,
          "BNW major war/lock/trade source order changed");
    check(has(plan,BnwTeamTurnStepKind::turns_at_war,1,0,5,0) &&
          has(plan,BnwTeamTurnStepKind::minor_technology_gate,22) &&
          has(plan,BnwTeamTurnStepKind::barbarian_technology_gate,63),
          "BNW major/minor/barbarian branches were omitted");
    auto safe=preflight_bnw_game_teams_safe_branch(state,rules);
    check(safe.safe && !safe.barbarian_grants && !safe.minor_grants &&
          !safe.awards_present && !safe.circumnavigation_possible,
          "No-effect BNW technology/award/circumference branch was rejected");
    auto handler=make_bnw_game_teams_handler(rules);
    check(handler.phase==BnwTurnPhase::game_teams && handler.run &&
          !handler.after_commit,"BNW game team handler was not bound");
    const auto rng=state.random.gameplay.state();
    check(handler.run(state,-1).outcome==LiveActionOutcome::committed &&
          state.world.teams->teams[0].arrays[1].values[1]==3 &&
          state.world.teams->teams[0].arrays[2].values[1]==2 &&
          state.world.teams->teams[1].arrays[1].values[0]==0 &&
          state.world.teams->teams[0].arrays[3].values[1]==0 &&
          !state.technologies[0].at(1).no_trade &&
          state.random.gameplay.state()==rng,
          "BNW safe team turn did not apply source-ordered counters and no-trade");
    check(handler.run(state,0).outcome==LiveActionOutcome::rejected,
          "BNW game teams accepted a player phase");
    // DETACHED numeric fixtures, deliberately including abnormal imported
    // self values. Not a legal-game producer, Original parity or actual cursor.
    // The source setters still execute their runtime guard in release builds.
    {
        auto guarded=fixture();auto& self=guarded.world.teams->teams[0];
        self.arrays[0].values[0]=1;self.arrays[1].values[0]=4;self.arrays[2].values[0]=2;
        const auto self_plan=plan_bnw_game_teams_prefix(guarded);
        check(!has(self_plan,BnwTeamTurnStepKind::turns_at_war,0,0,4,5)&&
              !has(self_plan,BnwTeamTurnStepKind::turns_locked_at_war,0,0,2,1),
              "Plan omitted actual nonzero self setter refusal");
        check(has(self_plan,BnwTeamTurnStepKind::turns_at_war,0,1,2,3)&&
              has(self_plan,BnwTeamTurnStepKind::turns_locked_at_war,0,1,3,2),
              "Self guard blocked actual nonself war/lock writes");
        check(handler.run(guarded,-1).outcome==LiveActionOutcome::committed&&
              guarded.world.teams->teams[0].arrays[1].values[0]==4&&
              guarded.world.teams->teams[0].arrays[2].values[0]==2&&
              guarded.world.teams->teams[0].arrays[1].values[1]==3&&
              guarded.world.teams->teams[0].arrays[2].values[1]==2,
              "Actual numeric application changed nonzero self value or skipped nonself pair");
    }
    {
        auto allowed=fixture();auto& self=allowed.world.teams->teams[0];
        self.arrays[1].values[0]=7;self.arrays[2].values[0]=1;
        const auto self_plan=plan_bnw_game_teams_prefix(allowed);
        check(has(self_plan,BnwTeamTurnStepKind::turns_at_war,0,0,7,0)&&
              has(self_plan,BnwTeamTurnStepKind::turns_locked_at_war,0,0,1,0),
              "Plan incorrectly refused source zero self setters");
        check(handler.run(allowed,-1).outcome==LiveActionOutcome::committed&&
              allowed.world.teams->teams[0].arrays[1].values[0]==0&&
              allowed.world.teams->teams[0].arrays[2].values[0]==0,
              "Source allowed zero self stores did not occur");
    }
    {
        auto rejected=fixture();auto& self=rejected.world.teams->teams[0];
        self.arrays[0].values[0]=1;self.arrays[1].values[0]=INT32_MAX;
        // Change reads/adds before Set. Its self guard cannot hide an overflow.
        bool overflow=false;try{(void)plan_bnw_game_teams_prefix(rejected);}catch(const std::exception&){overflow=true;}
        check(overflow&&handler.run(rejected,-1).outcome==LiveActionOutcome::rejected&&
              rejected.world.teams->teams[0].arrays[1].values[0]==INT32_MAX,
              "Source overflow was hidden behind nonzero self no-store guard");
    }
    {
        auto zero=fixture();auto& self=zero.world.teams->teams[0];
        self.arrays[0].values[0]=1;self.arrays[1].values[0]=-1;
        check(has(plan_bnw_game_teams_prefix(zero),BnwTeamTurnStepKind::turns_at_war,0,0,-1,0)&&
              handler.run(zero,-1).outcome==LiveActionOutcome::committed&&
              zero.world.teams->teams[0].arrays[1].values[0]==0,
              "Source allowed zero self write from Change was rejected");
    }
    auto blocked=fixture();
    blocked.technologies[0].set_has_tech(0,true);
    const auto acquisition=preflight_bnw_game_teams_safe_branch(blocked,rules);
    check(!acquisition.safe && acquisition.barbarian_grants>0 &&
          acquisition.minor_grants>0 &&
          handler.run(blocked,-1).outcome==LiveActionOutcome::rejected &&
          blocked.world.teams->teams[0].arrays[1].values[1]==2,
          "BNW technology acquisition was skipped or partially mutated world");
    blocked=fixture();
    blocked.world.map_header.wrap_x=true;
    blocked.world.map_projection.emplace();
    blocked.world.map_projection->grid=blocked.world.map_header.grid;
    for(std::uint64_t x=0;x<2;++x) {
        saves::SavedMapCellProjection cell;
        cell.index=cell.x=x;
        cell.y=0;
        cell.revealed_teams=std::set<std::int64_t>{0};
        blocked.world.map_projection->cells.push_back(cell);
    }
    const auto circumference=preflight_bnw_game_teams_safe_branch(blocked,rules);
    check(!circumference.safe && circumference.circumnavigation_possible &&
          handler.run(blocked,-1).outcome==LiveActionOutcome::rejected,
          "BNW circumnavigation effects were skipped");
    auto divergent=fixture();
    divergent.world.teams->teams[0].arrays[3].values[1]=0;
    check(handler.run(divergent,-1).outcome==LiveActionOutcome::rejected &&
          divergent.technologies[0].at(1).no_trade,
          "Divergent raw and owned no-trade flags were committed");
    auto transposed=fixture();
    transposed.world.teams->teams[0].arrays[3].values={1,0};
    check(handler.run(transposed,-1).outcome==LiveActionOutcome::rejected &&
          transposed.technologies[0].at(1).no_trade &&
          transposed.world.teams->teams[0].arrays[3].values[0]==1,
          "Same-count raw no-trade flag on a different technology was accepted");
    content::RulesDatabase award_database;
    award_database.apply_sql("CREATE TABLE SmallAwards(ID INTEGER PRIMARY KEY,Type TEXT);"
                             "INSERT INTO SmallAwards VALUES(0,'TEST_AWARD');"
                             "CREATE TABLE Technologies(ID INTEGER PRIMARY KEY,Type TEXT);"
                             "INSERT INTO Technologies VALUES(0,'TECH_ALPHA');"
                             "INSERT INTO Technologies VALUES(1,'TECH_BETA');");
    const BnwTeamTurnActiveRules award_rules{award_database,tech_rules,defines};
    auto award_state=fixture();
    const auto award=preflight_bnw_game_teams_safe_branch(award_state,award_rules);
    check(award.awards_present && !award.safe &&
          make_bnw_game_teams_handler(award_rules).run(award_state,-1).outcome==
              LiveActionOutcome::rejected,
          "Active BNW SmallAwards were skipped");
    auto malformed=fixture();
    malformed.world.teams->teams[0].arrays[0].values.pop_back();
    bool failed=false;
    try {(void)plan_bnw_game_teams_prefix(malformed);} catch(const std::runtime_error &) {failed=true;}
    check(failed && handler.run(malformed,-1).outcome==LiveActionOutcome::rejected,
          "Malformed BNW team relation was accepted");
    if(argc==3) selected(argv[1],argv[2]);
}
