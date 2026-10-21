#include "civ5/runtime/bnw_player_cramped_turn.hpp"
#include "civ5/runtime/bnw_player_scalar_prefix.hpp"
#include "civ5/runtime/bnw_player_pre_turn.hpp"
#include "civ5/runtime/bnw_turn_scheduler.hpp"
#include "civ5/runtime/bnw_live_plot_impassability.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace {
using namespace civ5;
using namespace civ5::runtime;
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void check(bool condition,const char *message) {
    if(!condition) throw std::runtime_error(message);
}
void field(saves::PlayerCoreObservation &core,const char *name,
           saves::PlayerCoreEncoding encoding,std::int64_t value) {
    saves::PlayerCoreField entry{};
    entry.name=name;entry.encoding=encoding;entry.values={value};
    core.fields.push_back(std::move(entry));
}
void field(saves::CityCoreObservation &core,const char *name,std::int64_t value) {
    saves::CityCoreField entry{};
    entry.name=name;entry.values={value};
    core.fields.push_back(std::move(entry));
}
std::int64_t cramped(const LiveSessionState &state) {
    for(const auto &entry:state.world.all_players->players.front().core.fields)
        if(entry.name && std::string_view(entry.name)=="m_bCramped")
            return entry.values.at(0);
    throw std::runtime_error("fixture has no cramped flag");
}
LiveSessionState fixture(const content::RulesDatabase &rules,
                         const SessionCheckpointIdentity &identity) {
    LiveSessionState state{saves::SavedSessionSnapshot{},ResearchContext{}, {},
        PlayerResearchContext{},CityYieldContext{},BnwUnhappinessContext{},
        MaintenanceContext{},TreasuryContext{},ScienceContext{},ResearchTurnState{},
        {SimulationRandom(RandomAlgorithm::bnw,{1,0,0}),
         SimulationRandom(RandomAlgorithm::bnw,{2,0,0})}};
    state.setup.profile=state.cities.profile=content::TechnologyProfile::bnw;
    state.setup.players[5].alive=true;
    state.setup.players[5].slot_status=3;
    state.setup.game_options.push_back({"GAMEOPTION_NO_HAPPINESS",1});
    state.cities.no_happiness=true;
    state.world.game_state.layout=state.world.map_header.layout=bnw;
    state.world.pregame.emplace().layout=bnw;
    state.world.pregame->slots.string_arrays["leader_keys"]={
        "", "", "", "", "", "LEADER_GANDHI"};
    state.world.pregame->slots.string_arrays["civilization_keys"]={
        "", "", "", "", "", "CIVILIZATION_INDIA"};
    state.research_turn.profile=content::TechnologyProfile::bnw;
    state.research_turn.players[5].leader_type="LEADER_GANDHI";
    state.world.map_header.grid={5,5};
    auto &all=state.world.all_players.emplace();all.layout=bnw;
    auto &player=all.players.emplace_back();
    player.saved_player_id=5;player.core.layout=bnw;player.cities.layout=bnw;
    field(player.core,"m_eID",saves::PlayerCoreEncoding::i32,5);
    field(player.core,"m_bAlive",saves::PlayerCoreEncoding::boolean,1);
    field(player.core,"m_bCramped",saves::PlayerCoreEncoding::boolean,1);
    field(player.core,"m_iCultureBombTimer",saves::PlayerCoreEncoding::i32,3);
    field(player.core,"m_iConversionTimer",saves::PlayerCoreEncoding::i32,2);
    field(player.core,"m_iTurnsSinceSettledLastCity",saves::PlayerCoreEncoding::i32,5);
    field(player.core,"m_iConscriptCount",saves::PlayerCoreEncoding::i32,4);
    field(player.core,"m_iUprisingCounter",saves::PlayerCoreEncoding::i32,3);
    field(player.core,"m_iCityRevoltCounter",saves::PlayerCoreEncoding::i32,3);
    field(player.core,"m_ePersonalityType",saves::PlayerCoreEncoding::i32,0);
    all.context.player_count=64;
    auto &city=player.cities.cities.emplace_back();
    city.core.layout=bnw;city.core.start_offset=10;city.core.end_offset=20;
    field(city.core,"m_iID",9);field(city.core,"m_eOwner",5);
    field(city.core,"m_iX",2);field(city.core,"m_iY",2);
    auto &live=state.cities.players[5].cities[9];
    live.id=9;live.owner=5;live.x=2;live.y=2;
    live.source_core=std::pair<std::uint64_t,std::uint64_t>{10,20};
    auto &map=state.world.map_projection.emplace();map.grid={5,5};
    auto &plots=state.world.map_plots.emplace();plots.layout=bnw;
    for(std::uint64_t y=0;y<5;++y)
        for(std::uint64_t x=0;x<5;++x) {
            const auto index=y*5+x;
            auto &cell=map.cells.emplace_back();
            cell.index=index;cell.x=x;cell.y=y;
            cell.owner=-1;cell.plot_type=2;cell.terrain_type=0;
            cell.feature_type=-1;
            auto &saved=plots.plots.emplace_back();
            saved.index=index;saved.x=x;saved.y=y;
        }
    auto &center=map.cells[map.grid.index(2,2)];
    center.owner=5;center.is_city=true;
    center.plot_city=saves::SavedPlotCityReference{5,9};
    map.cells[map.grid.index(1,2)].plot_type=3; // ocean
    map.cells[map.grid.index(3,2)].plot_type=0; // mountain
    map.cells[map.grid.index(1,1)].feature_type=0; // impassable
    rebuild_bnw_live_plot_impassability(state,rules,rules.cache_info_types(),identity);
    return state;
}
void blocked(LiveSessionState &state,const content::RulesDatabase &rules,
             const SessionCheckpointIdentity &identity,
             BnwCrampedTurnBlocker expected) {
    bool hit=false;
    try {(void)apply_bnw_player_cramped_turn(state,5,rules,identity);}
    catch(const BnwCrampedTurnError &error) {hit=error.blocker()==expected;}
    check(hit,"wrong BNW cramped typed blocker");
    check(cramped(state)==1,"blocked cramped turn changed saved flag");
}
void synthetic() {
    content::RulesDatabase rules;
    rules.apply_sql("CREATE TABLE Defines(Name TEXT,Value INTEGER);"
                    "INSERT INTO Defines VALUES('CRAMPED_RANGE_FROM_CITY',1),"
                    "('CRAMPED_USABLE_PLOT_PERCENT',25),"
                    "('VERY_UNHAPPY_THRESHOLD',-10),"
                    "('SUPER_UNHAPPY_THRESHOLD',-20);"
                    "CREATE TABLE Terrains(ID INTEGER,Type TEXT,Impassable BOOLEAN);"
                    "INSERT INTO Terrains VALUES(0,'TERRAIN_OPEN',0);"
                    "CREATE TABLE Features(ID INTEGER,Type TEXT,Impassable BOOLEAN);"
                    "INSERT INTO Features VALUES(0,'FEATURE_BLOCKED',1);"
                    "CREATE TABLE Leaders(ID INTEGER,Type TEXT);"
                    "INSERT INTO Leaders VALUES(0,'LEADER_GANDHI'),(1,'LEADER_PACAL');"
                    "CREATE TABLE Traits(ID INTEGER,Type TEXT,MayaCalendarBonuses INTEGER);"
                    "INSERT INTO Traits VALUES(0,'TRAIT_ORDINARY',0),(1,'TRAIT_MAYA',1);"
                    "CREATE TABLE Leader_Traits(LeaderType TEXT,TraitType TEXT);"
                    "INSERT INTO Leader_Traits VALUES('LEADER_GANDHI','TRAIT_ORDINARY'),"
                    "('LEADER_PACAL','TRAIT_MAYA');");
    const SessionCheckpointIdentity identity{
        "fixture",identify_bytes(rules.gameplay_fingerprint_data()).sha256};
    auto state=fixture(rules,identity);
    const auto step=apply_bnw_player_cramped_turn(state,5,rules,identity);
    check(step.wrote && step.before && !step.after &&
          step.total_nearby==6 && step.usable_nearby==3 && cramped(state)==0,
          "BNW cramped hex traversal or threshold differs from source");
    auto no_city=fixture(rules,identity);
    no_city.world.all_players->players[0].cities.cities.clear();
    no_city.cities.players[5].cities.clear();
    auto &center=no_city.world.map_projection->cells[12];
    center.is_city=false;center.plot_city.reset();
    const auto none=apply_bnw_player_cramped_turn(no_city,5,rules,identity);
    check(!none.wrote && none.before && none.after && cramped(no_city)==1,
          "BNW zero-total source branch rewrote cramped flag");
    auto stale=fixture(rules,identity);
    stale.plot_impassability.current=false;
    blocked(stale,rules,identity,BnwCrampedTurnBlocker::plot_impassability);
    auto missing_city=fixture(rules,identity);
    missing_city.world.map_projection->cells[12].plot_city.reset();
    blocked(missing_city,rules,identity,BnwCrampedTurnBlocker::city_roster);
    auto wrong_rules=fixture(rules,identity);
    auto mismatch=identity;mismatch.rules_sha256="wrong";
    blocked(wrong_rules,rules,mismatch,BnwCrampedTurnBlocker::selected_rules);
    auto combined=fixture(rules,identity);
    const auto full=apply_bnw_player_scalar_and_cramped_prefix(
        combined,5,5,rules,identity);
    check(full.scalars.culture_bomb.after==2 &&
          full.scalars.conversion.after==1 &&
          full.scalars.conversion.post_commit_dirty.has_value() &&
          full.scalars.settled_city.after==6 &&
          full.scalars.conscript.after==0 &&
          full.cramped.wrote && !full.cramped.after && cramped(combined)==0,
          "BNW scalar and cramped source-order prefix failed");
    auto rejected=fixture(rules,identity);
    rejected.plot_impassability.current=false;
    bool caught=false;
    try { (void)apply_bnw_player_scalar_and_cramped_prefix(
        rejected,5,5,rules,identity); }
    catch(const BnwCrampedTurnError &error) {
        caught=error.blocker()==BnwCrampedTurnBlocker::plot_impassability;
    }
    check(caught && cramped(rejected)==1,
          "cramped rejection did not roll back whole prefix");
    const auto &saved=rejected.world.all_players->players.front().core.fields;
    auto value=[&](std::string_view name) {
        for(const auto &entry:saved)
            if(entry.name && name==entry.name) return entry.values.at(0);
        throw std::runtime_error("missing combined prefix field");
    };
    check(value("m_iCultureBombTimer")==3 &&
          value("m_iConversionTimer")==2 &&
          value("m_iTurnsSinceSettledLastCity")==5 &&
          value("m_iConscriptCount")==4,
          "cramped rejection leaked prior scalar changes");
    auto early=fixture(rules,identity);
    const auto whole=apply_bnw_human_player_early_turn(
        early,5,5,rules,identity);
    check(whole.initial.scalars.culture_bomb.after==2 &&
          whole.initial.cramped.wrote && !whole.initial.cramped.after &&
          whole.uprising.branch==BnwUprisingTurnBranch::not_very_unhappy &&
          whole.city_revolt.branch==BnwCityRevoltTurnBranch::inactive &&
          !whole.maya.uses_maya_calendar &&
          early.random.gameplay.state().calls==0,
          "BNW early player source-order composition failed");
    auto maya_rejected=fixture(rules,identity);
    maya_rejected.world.pregame->slots.string_arrays["leader_keys"][5]=
        "LEADER_PACAL";
    maya_rejected.research_turn.players[5].leader_type="LEADER_PACAL";
    bool maya_blocked=false;
    try {(void)apply_bnw_player_early_turn_prefix(
        maya_rejected,5,5,rules,identity);}
    catch(const BnwMayaTurnError &error) {
        maya_blocked=error.blocker()==BnwMayaTurnBlocker::maya_calendar_and_boost;
    }
    check(maya_blocked && cramped(maya_rejected)==1 &&
          maya_rejected.random.gameplay.state().calls==0,
          "late Maya rejection did not roll back whole early player segment");
    const auto &blocked_core=maya_rejected.world.all_players->players.front().core.fields;
    auto blocked_value=[&](std::string_view name) {
        for(const auto &entry:blocked_core)
            if(entry.name && name==entry.name) return entry.values.at(0);
        throw std::runtime_error("missing early prefix field");
    };
    check(blocked_value("m_iCultureBombTimer")==3 &&
          blocked_value("m_iConversionTimer")==2 &&
          blocked_value("m_iTurnsSinceSettledLastCity")==5 &&
          blocked_value("m_iConscriptCount")==4,
          "late Maya rejection leaked earlier player mutations");
    auto ai=fixture(rules,identity);
    ai.setup.players[5].slot_status=1;
    bool ai_blocked=false;
    try {(void)apply_bnw_human_player_early_turn(ai,5,5,rules,identity);}
    catch(const BnwPlayerPreTurnError &error) {
        ai_blocked=error.blocker()==BnwPlayerPreTurnBlocker::ai_pre_effects;
    }
    check(ai_blocked && cramped(ai)==1 &&
          ai.random.gameplay.state().calls==0,
          "BNW AI pre-turn missing effects did not reject before mutation");
    auto early_handlers=make_bnw_human_player_early_turn_handlers(
        rules,identity,-1);
    check(early_handlers[0].phase==BnwTurnPhase::player_cache &&
          early_handlers[1].phase==BnwTurnPhase::player_ai_pre &&
          early_handlers[2].phase==BnwTurnPhase::player_timers_and_revolts,
          "BNW human pre-turn handler phases differ from SDK order");
    auto bound=fixture(rules,identity);
    for(auto &handler:early_handlers)
        check(handler.run(bound,5).outcome==LiveActionOutcome::committed,
              "BNW human pre-turn handler rejected a supported phase");
    bool culture_bomb_decremented=false;
    for(const auto &entry:bound.world.all_players->players.front().core.fields)
        if(entry.name && std::string_view(entry.name)=="m_iCultureBombTimer")
            culture_bomb_decremented=entry.values.at(0)==2;
    check(cramped(bound)==0 && culture_bomb_decremented &&
          bound.random.gameplay.state().calls==0,
          "BNW early source effects were not bound to scheduler phases");
    auto active=fixture(rules,identity);
    const auto active_handlers=make_bnw_human_player_early_turn_handlers(
        rules,identity,5);
    const auto dirty=active_handlers[2].run(active,5);
    check(dirty.outcome==LiveActionOutcome::rejected &&
          dirty.reason.find("UI dirty")!=std::string::npos &&
          cramped(active)==1 && active.random.gameplay.state().calls==0,
          "active conversion timer lost its required UI signal or changed state");
    auto ai_bound=fixture(rules,identity);
    ai_bound.setup.players[5].slot_status=1;
    check(early_handlers[0].run(ai_bound,5).outcome==LiveActionOutcome::committed &&
          early_handlers[1].run(ai_bound,5).outcome==LiveActionOutcome::rejected &&
          cramped(ai_bound)==1,
          "AI pre-turn handler accepted unavailable AI effects");
    auto scheduler_state=fixture(rules,identity);
    saves::SavedSession source;
    source.snapshot=scheduler_state.world;
    auto session=LiveSession::from_restored_bnw(
        std::move(source),std::move(scheduler_state));
    std::vector<BnwTurnHandler> schedule(
        early_handlers.begin(),early_handlers.end());
    const auto missing=run_bnw_player_turn(session,5,schedule);
    check(missing.outcome==LiveActionOutcome::rejected &&
          cramped(session.state())==1,
          "incomplete BNW player schedule published early player state");
    bool reached_next=false;
    for(const auto phase:bnw_player_turn_order()) {
        if(phase==BnwTurnPhase::player_achievements) continue;
        if(phase<=BnwTurnPhase::player_timers_and_revolts) continue;
        schedule.push_back({phase,[&reached_next,phase](LiveSessionState &draft,
                                                        std::int64_t)->LiveActionResult {
            if(phase==BnwTurnPhase::player_trade_strategy_diplomacy) {
                reached_next=true;
                const auto &saved=draft.world.all_players->players.front().core.fields;
                bool decremented=false;
                for(const auto &field:saved)
                    if(field.name && std::string_view(field.name)=="m_iConversionTimer")
                        decremented=field.values.at(0)==1;
                if(!decremented || cramped(draft)!=0)
                    return {LiveActionOutcome::rejected,
                            "early source phase did not reach next boundary"};
                return {LiveActionOutcome::rejected,
                        "later BNW trade and diplomacy effects unavailable"};
            }
            return {LiveActionOutcome::committed,{}};
        }});
    }
    const auto stopped=run_bnw_player_turn(session,5,schedule);
    check(reached_next && stopped.outcome==LiveActionOutcome::rejected &&
          stopped.reason.find("player_trade_strategy_diplomacy")!=std::string::npos &&
          cramped(session.state())==1 &&
          session.state().random.gameplay.state().calls==0,
          "late BNW phase rejection did not roll back bound early player phases");
    auto make_session=[&]() {
        auto state=fixture(rules,identity);
        saves::SavedSession saved;
        saved.snapshot=state.world;
        return LiveSession::from_restored_bnw(
            std::move(saved),std::move(state));
    };
    auto complete_schedule=[&](BnwPlayerEarlyTurnServices services) {
        auto first=make_bnw_human_player_early_turn_handlers(
            rules,identity,5,std::move(services));
        std::vector<BnwTurnHandler> complete(first.begin(),first.end());
        // Test instrumentation supplies the later phase boundaries. Runtime
        // never installs successful substitutes for those missing mechanics.
        for(const auto phase:bnw_player_turn_order()) {
            if(phase<=BnwTurnPhase::player_timers_and_revolts ||
               phase==BnwTurnPhase::player_achievements) continue;
            complete.push_back({phase,[](LiveSessionState &,
                                          std::int64_t)->LiveActionResult {
                return {LiveActionOutcome::committed,{}};
            }});
        }
        return complete;
    };
    auto published_session=make_session();
    int deliveries=0;
    BnwPlayerEarlyTurnServices delivery;
    delivery.dirty_after_commit=[&](std::int64_t player,
                                    BnwConversionTimerDirtyBit bit) {
        check(player==5 && bit==BnwConversionTimerDirtyBit::misc_buttons,
              "BNW conversion UI publication sent wrong intent");
        bool decremented=false;
        for(const auto &entry:published_session.state().world.all_players->players.front().core.fields)
            if(entry.name && std::string_view(entry.name)=="m_iConversionTimer")
                decremented=entry.values.at(0)==1;
        check(decremented,"BNW conversion UI callback ran before turn commit");
        ++deliveries;
    };
    auto complete=complete_schedule(delivery);
    const auto committed=run_bnw_player_turn(published_session,5,complete);
    check(committed.outcome==LiveActionOutcome::committed && deliveries==1 &&
          !published_session.state().player_early_publication,
          "BNW active conversion UI effect was not delivered after commit");
    auto rolled_back=make_session();
    auto reject_later=complete_schedule(delivery);
    for(auto &entry:reject_later)
        if(entry.phase==BnwTurnPhase::player_trade_strategy_diplomacy)
            entry.run=[](LiveSessionState &,std::int64_t)->LiveActionResult {
                return {LiveActionOutcome::rejected,"later phase unavailable"};
            };
    check(run_bnw_player_turn(rolled_back,5,reject_later).outcome==
              LiveActionOutcome::rejected && deliveries==1 &&
          !rolled_back.state().player_early_publication,
          "rejected BNW turn delivered its conversion UI effect");
    auto uncertain_session=make_session();
    int attempted=0;
    BnwPlayerEarlyTurnServices throwing;
    throwing.dirty_after_commit=[&](std::int64_t,
                                    BnwConversionTimerDirtyBit) {
        ++attempted;
        throw std::runtime_error("UI host stopped");
    };
    auto uncertain_schedule=complete_schedule(throwing);
    const auto uncertain=run_bnw_player_turn(uncertain_session,5,uncertain_schedule);
    check(uncertain.outcome==LiveActionOutcome::committed &&
          uncertain.reason.find("publication uncertain")!=std::string::npos &&
          attempted==1 && uncertain_session.state().player_early_publication &&
          uncertain_session.state().player_early_publication->uncertain,
          "throwing BNW UI callback did not retain uncertain committed intent");
    check(run_bnw_player_turn(uncertain_session,5,uncertain_schedule).outcome==
              LiveActionOutcome::rejected && attempted==1,
          "uncertain BNW UI publication was replayed on the next turn");
}
// Optional selected-source diagnostic. Keep the fast boundary test runnable
// alone: the full selected preview bootstrap can stall in a direct-link test
// binary on the shared Windows host and is verified separately on main.
void selected(const std::filesystem::path &save,const std::filesystem::path &root) {
    auto bootstrap=ui::bootstrap_selected_bnw_preview(save,root,0);
    if(bootstrap.preview.blocker)
        throw std::runtime_error(bootstrap.preview.blocker->reason);
    check(bootstrap.preview.host!=nullptr,"selected Gandhi host unavailable");
    auto &host=*bootstrap.preview.host;
    auto input=host.preview_input();
    check(input.frame_sources.database!=nullptr,"selected BNW rules unavailable");
    auto detached=host.session().state();
    const auto id=host.active_player_id();
    const auto step=apply_bnw_player_cramped_turn(
        detached,id,*input.frame_sources.database,host.identity());
    check(step.player_id==id && step.wrote && step.total_nearby>0 &&
          step.usable_nearby<=step.total_nearby,
          "selected Gandhi did not exercise live cramped traversal");
    check(detached.player_culture_bomb_timers.at(id)==0,
          "BNW CvPlayer::Read did not restore reset-only culture bomb timer");
    const auto early=apply_bnw_human_player_early_turn(
        detached,id,id,*input.frame_sources.database,host.identity());
    check(early.initial.cramped.total_nearby==step.total_nearby &&
          early.initial.cramped.usable_nearby==step.usable_nearby &&
          early.uprising.player_id==id && early.city_revolt.player_id==id &&
          early.maya.player_id==id,
          "selected Gandhi early player segment did not reach Maya check");
    const auto handlers=make_bnw_human_player_early_turn_handlers(
        *input.frame_sources.database,host.identity(),id);
    auto bound=host.session().state();
    for(const auto &handler:handlers) {
        const auto outcome=handler.run(bound,id);
        check(outcome.outcome==LiveActionOutcome::committed,
              "selected Gandhi BNW early scheduler phase rejected");
    }
    check(bound.world.all_players->players.size()==detached.world.all_players->players.size(),
          "selected Gandhi early handler changed player roster");
    std::cout << "selected Gandhi cramped total=" << step.total_nearby
              << " usable=" << step.usable_nearby
              << " before=" << step.before << " after=" << step.after << '\n';
}
} // namespace

int main(int argc,char **argv) {
    try {
        check(argc==1 || argc==3,
              "Supply no paths for boundaries or selected Gandhi save and game root");
        synthetic();
        if(argc==3) selected(argv[1],argv[2]);
        std::cout << "BNW CvPlayer::DoUpdateCramped parity PASS\n";
    } catch(const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
