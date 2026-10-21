#include "civ5/runtime/bnw_retained_turn_cursor.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/runtime/bnw_live_rng_export.hpp"
#include "civ5/runtime/bnw_barbarians_begin.hpp"
#include "civ5/runtime/bnw_game_supporting_checkpoint.hpp"
#include "civ5/runtime/bnw_world_religion_progress.hpp"
#include "civ5/runtime/bnw_game_score.hpp"
#include "civ5/runtime/bnw_game_score_checkpoint.hpp"
#include "civ5/runtime/bnw_game_clock.hpp"
#include "civ5/runtime/bnw_game_clock_checkpoint.hpp"
#include "civ5/runtime/bnw_game_deals.hpp"
#include "civ5/runtime/bnw_game_deals_checkpoint.hpp"
#include "civ5/runtime/bnw_player_core_checkpoint.hpp"
#include "civ5/runtime/bnw_game_active_team_read.hpp"
#include "civ5/runtime/bnw_team_turn_pair.hpp"
#include "civ5/ui/bnw_client_frame.hpp"
#include "bnw_selected_city_test_locale.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <algorithm>
#include <functional>
#include <memory>
#include <cstddef>
#include <barrier>
#include <climits>
#include <iostream>
#include <stdexcept>
#include <sstream>
#include <thread>
#include <type_traits>
#include <vector>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
using namespace civ5;
using namespace civ5::runtime;
namespace {
void need(bool value,const char* why){if(!value)throw std::runtime_error(why);}
template<class F>void refuses(F&& f){bool refused=false;try{f();}catch(const std::exception&){refused=true;}
    need(refused,"Invalid retained turn use accepted");}
std::int64_t qpc(){LARGE_INTEGER v{};need(QueryPerformanceCounter(&v)!=0,"Test actual QPC unavailable");return v.QuadPart;}
std::shared_ptr<LiveSession> clone(const ui::SelectedBnwPreviewHost& host){
    // Actual selected source + full owned state copy; no added completion flag.
    // This test clone does not acquire a native utility or playable readiness.
    auto value=std::make_shared<LiveSession>(LiveSession::from_restored_bnw(host.session().source(),host.session().state()));
    return value; // Genuine destruction now distinguishes reused storage.
}
void prefix(const std::shared_ptr<const ui::SelectedBnwPreviewHost>& host){
    auto cursor=BnwRetainedTurnCursor::bind(host);
    auto session=std::shared_ptr<const LiveSession>(host,&host->session());
    need(BnwRetainedTurnCursor::bind(session)==cursor,"Host/direct routes created different prefix owners");
    const auto timer=cursor->timer_owner();const auto initial=timer->read(timer->current_use());
    const auto before_view=cursor->view();
    need(before_view.frontier==BnwRetainedTurnFrontier::timer_reset&&!before_view.timer_attempted&&before_view.operation==0&&
         initial.reset==BnwGameTurnTimerReset::construction&&initial.operation==0,"Bind executed engine reset prematurely");
    const auto map_rng=session->state().random.map.state(),game_rng=session->state().random.gameplay.state();
    const auto turn=session->state().setup.game_turn;
    const auto raw_elapsed=std::get<saves::Expansion2PrefixObservation>(session->state().world.game_state.prefix).scalars[1];
    const auto before=qpc();const auto reset=cursor->execute_timer_prefix();const auto after=qpc();
    need(reset.current_start>=before&&reset.current_start<=reset.game_turn_start&&reset.game_turn_start<=after&&
         reset.pause_delta==0.0f&&reset.current_stopped_elapsed==0.0f&&reset.game_turn_stopped_elapsed==0.0f&&
         reset.reset==BnwGameTurnTimerReset::engine_begin&&reset.operation==1,"Prefix did not execute actual ordered timer stores");
    const auto view=cursor->view();
    need(view.frontier==BnwRetainedTurnFrontier::selection_cycle_owner_unavailable&&view.timer_attempted&&view.operation==1,
         "Prefix falsely passed selection-cycle frontier");
    refuses([&]{(void)cursor->execute_timer_prefix();});
    need(BnwRetainedTurnCursor::bind(host)==cursor&&BnwRetainedTurnCursor::bind(session)==cursor,
         "Repeated binding did not retain actual pending owner");
    const auto unchanged=timer->read(timer->current_use());
    need(unchanged.current_start==reset.current_start&&unchanged.game_turn_start==reset.game_turn_start&&unchanged.operation==1,
         "Repeat replayed timer reset");
    need(session->state().random.map.state()==map_rng&&session->state().random.gameplay.state()==game_rng&&
         session->state().setup.game_turn==turn&&
         std::get<saves::Expansion2PrefixObservation>(session->state().world.game_state.prefix).scalars[1]==raw_elapsed,
         "Prefix changed RNG/game clock");
    need(!session->readiness().can_continue(),"Prefix granted full continuation");
}
ui::UiContextRegistry cycle_registry(){
    return ui::UiContextRegistry::from_documents(content::read_xml_metadata(
        R"(<Context><LuaContext ID="WorldView" FileName="WorldView"/></Context>)"),
        content::read_xml_metadata("<Context/>"));
}
// Test-only activation cache, scoped to this invocation and exact input paths.
// It holds SQL snapshots/reports/wire dimensions, never a Host, Session, cache,
// Pregame owner, native package, cursor, or callback retaining a fixture world.
class CycleRuleFixture;
thread_local const CycleRuleFixture* cycle_rule_fixture=nullptr;
class CycleRuleFixture final {
  public:
    CycleRuleFixture(const ui::SelectedBnwBootstrapResult& bootstrap,
        const ui::SelectedBnwPreviewHost& initial,const std::filesystem::path& save,
        const std::filesystem::path& game,const std::string& original)
        :save_(save),game_(game),thread_(std::this_thread::get_id()){
        if(cycle_rule_fixture)throw std::runtime_error("Nested cycle rules fixture");
        if(!bootstrap.activation||!bootstrap.postprocess||bootstrap.gamecore_sha256.empty())
            throw std::runtime_error("Cycle rules fixture actual bootstrap reports absent");
        if(!initial.session().state().world.all_players)
            throw std::runtime_error("Cycle rules fixture actual source dimensions absent");
        selected_=identify_file(save_);
        if(selected_.sha256!=original)
            throw std::runtime_error("Cycle rules fixture selected source changed");
        activation_=*bootstrap.activation;
        postprocess_=*bootstrap.postprocess;
        source_.gamecore_executable=game_/"Assets"/"DLC"/"Expansion2"/"CvGameCore_Expansion2.dll";
        source_.gamecore_sha256=bootstrap.gamecore_sha256;
        source_.context.decode_map_regions=true;
        source_.context.decode_teams=true;
        source_.context.decode_all_players=true;
        source_.context.decode_supporting_classes=true;
        source_.context.all_players=initial.session().state().world.all_players->context;
    source_.context.permit_live_production_overlay=initial.session().source().source.live_city_production.has_value();
    source_.context.permit_live_research_overlay=initial.session().source().source.live_team_research.has_value();
    source_.context.permit_live_treasury_overlay=initial.session().source().source.live_treasury.has_value();
    source_.context.permit_live_research_turn_overlay=initial.session().source().source.live_research_turn.has_value();
    source_.context.permit_live_research_selection_overlay=initial.session().source().source.live_research_selection.has_value();
    source_.context.permit_live_random_overlay=initial.session().source().source.live_simulation_random.has_value();
    source_.context.permit_live_bnw_game_clock_overlay=initial.session().source().source.live_bnw_game_clock.has_value();
    source_.context.permit_live_bnw_map_plots_overlay=initial.session().source().source.live_bnw_map_plots.has_value();
    source_.context.permit_live_bnw_units_overlay=initial.session().source().source.live_bnw_units.has_value();
    source_.context.permit_live_bnw_area_units_overlay=initial.session().source().source.live_bnw_area_units.has_value();
    source_.context.permit_live_bnw_player_proximity_overlay=initial.session().source().source.live_bnw_player_proximity.has_value();
    source_.context.permit_live_bnw_team_meet_overlay=initial.session().source().source.live_bnw_team_meet.has_value();
    source_.context.permit_live_bnw_exploration_dirty_overlay=initial.session().source().source.live_bnw_exploration_dirty.has_value();
    source_.context.permit_live_bnw_player_strategy_overlay=initial.session().source().source.live_bnw_player_strategy.has_value();
    source_.context.permit_live_bnw_player_core_overlay=initial.session().source().source.live_bnw_player_core.has_value();
    source_.context.permit_live_bnw_city_core_overlay=initial.session().source().source.live_bnw_city_core.has_value();
    source_.context.permit_live_bnw_city_production_overlay=initial.session().source().source.live_bnw_city_production.has_value();
    source_.context.permit_live_bnw_city_management_overlay=initial.session().source().source.live_bnw_city_management.has_value();
    source_.context.permit_live_bnw_city_tail_overlay=initial.session().source().source.live_bnw_city_tail.has_value();
    source_.context.permit_live_bnw_treasury_balance_overlay=initial.session().source().source.live_bnw_treasury_balance.has_value();
    source_.context.permit_live_bnw_team_progress_overlay=initial.session().source().source.live_bnw_team_progress.has_value();
    source_.context.permit_live_bnw_culture_bomb_timer_overlay=initial.session().source().source.live_bnw_culture_bomb_timer.has_value();
    source_.context.permit_live_bnw_research_queue_overlay=initial.session().source().source.live_bnw_research_queue.has_value();
    source_.context.permit_live_bnw_maintenance_expense_overlay=initial.session().source().source.live_bnw_maintenance_expense.has_value();
    source_.context.permit_live_bnw_player_knowledge_overlay=initial.session().source().source.live_bnw_player_knowledge.has_value();
    source_.context.permit_live_bnw_player_administration_overlay=initial.session().source().source.live_bnw_player_administration.has_value();
    source_.context.permit_live_bnw_game_supporting_overlay=initial.session().source().source.live_bnw_game_supporting.has_value();
    source_.context.permit_live_bnw_game_score_replay_overlay=initial.session().source().source.live_bnw_game_score_replay.has_value();
    source_.context.permit_live_bnw_game_deals_overlay=initial.session().source().source.live_bnw_game_deals.has_value();
    source_.context.permit_live_bnw_team_turn_overlay=initial.session().source().source.live_bnw_team_turn.has_value();
    source_.context.permit_live_bnw_ui_transients_overlay=initial.session().source().source.live_bnw_ui_transients.has_value();
        // clone_gameplay uses sqlite3_backup for gameplay AND Localization;
        // it retains no source connection/Host. Seal only this private clone.
        auto database=initial.database().clone_gameplay();
        database->seal_immutable_preview_fingerprints();
        database_=std::move(database);
        require_selected_current();
        cycle_rule_fixture=this;
    }
    ~CycleRuleFixture(){cycle_rule_fixture=nullptr;}
    CycleRuleFixture(const CycleRuleFixture&)=delete;
    CycleRuleFixture& operator=(const CycleRuleFixture&)=delete;
    bool matches(const std::filesystem::path& save,const std::filesystem::path& game)const{
        return save==save_&&game==game_;
    }
    ui::SelectedBnwPreviewResult fresh()const{
        if(std::this_thread::get_id()!=thread_)
            throw std::runtime_error("Cycle rules fixture used on foreign thread");
        require_selected_current();
        // The existing genuine factory remeasures this actual GameCore DLL,
        // clones the SQL database, loads NEW caches, decodes this file again,
        // and restores NEW Session/Pregame/activation/transient owners.
        auto result=ui::make_selected_bnw_preview_host(save_,*database_,activation_,
            postprocess_,source_,0,true);
        require_selected_current();
        return result;
    }
  private:
    void require_selected_current()const{
        const auto actual=identify_file(save_);
        if(actual.bytes!=selected_.bytes||actual.sha256!=selected_.sha256)
            throw std::runtime_error("Cycle rules fixture original input changed");
    }
    const std::filesystem::path save_,game_;
    const std::thread::id thread_;
    FileIdentity selected_;
    std::unique_ptr<const content::RulesDatabase> database_;
    content::SavedRulesActivationReport activation_;
    content::GameplayPostprocessReport postprocess_;
    ui::BnwSourceDecodeContract source_;
};
std::shared_ptr<ui::SelectedBnwPreviewHost> cycle_host(const std::filesystem::path& save,
    const std::filesystem::path& game){
    ui::SelectedBnwPreviewResult preview;
    if(cycle_rule_fixture&&cycle_rule_fixture->matches(save,game)){
        preview=cycle_rule_fixture->fresh();
    }else{
        // Modified Civis files and other input paths keep full activation.
        auto boot=ui::bootstrap_selected_bnw_preview(save,game,0,true);
        preview=std::move(boot.preview);
    }
    if(preview.blocker)throw std::runtime_error(preview.blocker->reason);
    std::shared_ptr<ui::SelectedBnwPreviewHost> result(std::move(preview.host));
    need(bool(result),"Cycle actual Host absent");
    return result; // No process lifetime retention of full fixture worlds.
}
void actual_cycle(const std::filesystem::path& save,const std::filesystem::path& game){
    const auto host=cycle_host(save,game);
    const auto other=cycle_host(save,game);
    auto client=ui::BnwClientFrameOwner::from_selected_host(host,test_detail::selected_city_frame_sources(host));
    auto wrong_client=ui::BnwClientFrameOwner::from_selected_host(other,test_detail::selected_city_frame_sources(other));
    auto wrong_ui=wrong_client->create_selected_ui_host(cycle_registry(),nullptr);
    need(!host->session().has_bnw_operation_checkpoint_history(),"Fresh actual Host already has retained history");
    const auto baseline=host->export_import_clock_checkpoint();
    need(!baseline.full_persistence,"Baseline partial export falsely claimed full persistence");
    const auto checkpoint=host->session().checkpoint(); // Captured BEFORE timer/cursor construction.
    const auto cursor=BnwRetainedTurnCursor::bind(std::shared_ptr<const ui::SelectedBnwPreviewHost>(host));
    need(host->session().has_bnw_operation_checkpoint_history(),"Timer constructor lost partial-save history");
    refuses([&]{(void)host->export_import_clock_checkpoint();}); // BEFORE reset or UI setter.
    ui::LiveUiHost detached(cycle_registry(),[]{return ui::UiFrameSnapshot(0,std::nullopt,std::nullopt);},nullptr);
    refuses([&]{cursor->execute_selection_cycle_prefix(detached);});
    refuses([&]{cursor->execute_engine_utility_prefix();});
    refuses([&]{cursor->execute_barbarians_begin_prefix();});
    need(!cursor->view().barbarians_attempted&&!cursor->view().engine_utility_attempted&&!cursor->view().selection_cycle_attempted&&cursor->view().operation==0,
        "No predecessor UI call stored an attempt");
    (void)cursor->execute_timer_prefix();
    refuses([&]{cursor->execute_selection_cycle_prefix(detached);});
    refuses([&]{cursor->execute_selection_cycle_prefix(*wrong_ui);});
    refuses([&]{cursor->execute_engine_utility_prefix();});
    need(!cursor->view().engine_utility_attempted&&!cursor->view().selection_cycle_attempted&&cursor->view().operation==1,
        "Missing/foreign canonical UI changed once history");
    auto actual_ui=client->create_selected_ui_host(cycle_registry(),nullptr);
    refuses([&]{(void)cursor->no_selection_list_cycle(*actual_ui);});
    const ui::ClientViewport viewport{1024,768,1024,768,96};
    const auto before=client->capture(viewport,{});
    const auto map_rng=host->session().state().random.map.state();
    const auto game_rng=host->session().state().random.gameplay.state();
    const auto old_city=actual_ui->selected_city();
    cursor->execute_selection_cycle_prefix(*actual_ui);
    need(!cursor->no_selection_list_cycle(*actual_ui),"Actual Civis selection-cycle reset not stored");
    const auto view=cursor->view();
    need(view.selection_cycle_attempted&&view.timer_attempted&&view.operation==2&&
        view.frontier==BnwRetainedTurnFrontier::engine_utility_do_turn_unavailable,
        "Cycle reset crossed unavailable engine utility");
    need(!client->is_current(before),"UI reset left old canonical frame current");
    const auto after=client->capture(viewport,{});
    need(after->stamp.identity.selection_revision==before->stamp.identity.selection_revision+1&&
        actual_ui->selected_city()==old_city&&host->session().state().random.map.state()==map_rng&&
        host->session().state().random.gameplay.state()==game_rng,
        "Cycle setter changed selection/RNG or wrong revision");
    refuses([&]{cursor->execute_selection_cycle_prefix(*actual_ui);});
    need(client->is_current(after)&&cursor->view().operation==2,"Repeated setter changed canonical frame");
    refuses([&]{(void)cursor->no_selection_list_cycle(*wrong_ui);});
    const auto utility_turn=host->session().state().setup.game_turn;
    const auto utility_raw_elapsed=std::get<saves::Expansion2PrefixObservation>(
        host->session().state().world.game_state.prefix).scalars[1];
    const auto utility_state=host->session().read_state_lease();
    const auto utility_timer=cursor->timer_owner()->read(cursor->timer_owner()->current_use());
    cursor->execute_engine_utility_prefix();
    const auto utility=cursor->view();
    need(utility.engine_utility_attempted&&utility.operation==3&&
         utility.frontier==BnwRetainedTurnFrontier::barbarians_begin,
         "Actual synchronous source utility did not stop before barbarians");
    const auto utility_after_timer=cursor->timer_owner()->read(cursor->timer_owner()->current_use());
    need(client->is_current(after)&&!cursor->no_selection_list_cycle(*actual_ui)&&
         host->session().read_state_lease()==utility_state&&
         host->session().state().setup.game_turn==utility_turn&&
         std::get<saves::Expansion2PrefixObservation>(host->session().state().world.game_state.prefix).scalars[1]==utility_raw_elapsed&&
         host->session().state().random.map.state()==map_rng&&host->session().state().random.gameplay.state()==game_rng&&
         utility_after_timer.current_start==utility_timer.current_start&&
         utility_after_timer.game_turn_start==utility_timer.game_turn_start&&utility_after_timer.operation==utility_timer.operation,
         "Authenticated empty utility body changed world/frame/RNG/clocks/timer");
    refuses([&]{cursor->execute_engine_utility_prefix();});
    need(cursor->view().operation==3&&client->is_current(after),"Repeated utility changed once history or frame");
    refuses([&]{(void)host->export_import_clock_checkpoint();});
    // Generic restored session can still exercise its legacy timer-only port,
    // but SAME hashes do not make another actual Host UI its receiver.
    auto generic=clone(*host);auto generic_cursor=BnwRetainedTurnCursor::bind(generic);
    (void)generic_cursor->execute_timer_prefix();
    refuses([&]{generic_cursor->execute_selection_cycle_prefix(*actual_ui);});
    refuses([&]{generic_cursor->execute_engine_utility_prefix();});
    need(!generic_cursor->view().engine_utility_attempted&&!generic_cursor->view().selection_cycle_attempted&&generic_cursor->view().operation==1,
        "Generic session fabricated canonical UI completion");
    client.reset();
    refuses([&]{(void)cursor->no_selection_list_cycle(*actual_ui);});
    need(cursor->view().frontier==BnwRetainedTurnFrontier::quarantined&&cursor->view().engine_utility_attempted,
        "Retired canonical receiver lost utility history or left cursor executable");
    refuses([&]{cursor->execute_engine_utility_prefix();});
    host->session().restore(checkpoint);
    need(host->session().has_bnw_operation_checkpoint_history(),"Restore dropped unsaved retained history");
    refuses([&]{cursor->execute_engine_utility_prefix();});
    need(cursor->view().engine_utility_attempted&&cursor->view().operation==3,"Restore lost attempted utility history");
    refuses([&]{(void)host->export_import_clock_checkpoint();});
    auto moved=std::make_shared<LiveSession>(std::move(*generic));
    need(generic->has_bnw_operation_checkpoint_history()&&moved->has_bnw_operation_checkpoint_history(),
        "Move constructor dropped source/destination history");
    *moved=std::move(*clone(*other));
    need(moved->has_bnw_operation_checkpoint_history(),"Move assignment overwrote unsaved history");
    // Direct timer creation also owns unsaved Start/reset history; it cannot
    // bypass the partial-checkpoint guard by avoiding Cursor.bind.
    const auto direct_host=cycle_host(save,game);
    auto direct_session=std::shared_ptr<const LiveSession>(direct_host,&direct_host->session());
    const auto direct_baseline=direct_host->export_import_clock_checkpoint();
    need(!direct_baseline.full_persistence&&!direct_host->session().has_bnw_operation_checkpoint_history(),
        "Fresh direct-timer baseline export/history unexpected");
    const auto direct_state=direct_session->read_state_lease();bool direct_foreign=false;
    std::thread direct_worker([&]{try{(void)BnwGameTurnTimerOwner::create(direct_session);}
        catch(const std::exception&){direct_foreign=true;}});direct_worker.join();
    need(direct_foreign&&!direct_host->session().has_bnw_operation_checkpoint_history()&&
        direct_session->read_state_lease()==direct_state,"Foreign direct creation borrowed/stored live timer state");
    const auto direct_timer=BnwGameTurnTimerOwner::create(direct_session);
    need(direct_host->session().has_bnw_operation_checkpoint_history(),"Direct timer history not guarded");
    refuses([&]{(void)direct_host->export_import_clock_checkpoint();});
    (void)direct_timer->reset_for_current_player();
    need(direct_host->session().has_bnw_operation_checkpoint_history(),"Direct timer reset discarded history");
    auto direct_moved=std::make_shared<LiveSession>(std::move(direct_host->session()));
    need(direct_host->session().has_bnw_operation_checkpoint_history()&&
        direct_moved->has_bnw_operation_checkpoint_history(),"Direct timer move dropped sticky history");
    refuses([&]{(void)direct_timer->read(direct_timer->current_use());});
    // Foreign thread quarantines this separate cursor before reading actual UI.
    auto foreign_cursor=BnwRetainedTurnCursor::bind(std::shared_ptr<const ui::SelectedBnwPreviewHost>(other));
    (void)foreign_cursor->execute_timer_prefix();
    const auto foreign_before=wrong_client->capture(viewport,{});bool rejected=false;
    std::thread worker([&]{try{foreign_cursor->execute_selection_cycle_prefix(*wrong_ui);}
        catch(const std::exception&){rejected=true;}});worker.join();
    need(rejected&&!foreign_cursor->view().selection_cycle_attempted&&wrong_client->is_current(foreign_before),
        "Foreign thread stored UI cycle flag");
    need(!host->session().readiness().can_continue(),"Cycle reset granted complete continuation");
}
void utility_guards(const std::filesystem::path& save,const std::filesystem::path& game){
    // Each real selected Host has its own timer/UI prefix. No generic restored
    // clone or completion callback is allowed to create operation 3 permission.
    for(int mode=0;mode!=6;++mode){
        const auto host=cycle_host(save,game);
        const auto& header=host->session().source().source.header;
        need(header.build=="1.0.3.279 (403694)"&&header.build_id=="403694"&&header.mods.empty(),
             "Utility scenario needs the authenticated stock source fixture");
        if(mode==0){
            // A different original build refuses in the stock parser BEFORE
            // an actual selected Host can exist. Only copied bytes are changed.
            const auto& prefix=host->session().source().source.source_prefix;
            need(prefix&&prefix->size()>12,"Stock source prefix absent");
            std::string bad(prefix->begin(),prefix->end());
            need(bad[12]=='1',"Stock header build offset differs");bad[12]='9';
            refuses([&]{std::istringstream stream(bad,std::ios::binary);
                BinaryReader reader(stream,bad.size());(void)saves::read_header(reader);});
        }
        auto client=ui::BnwClientFrameOwner::from_selected_host(host,test_detail::selected_city_frame_sources(host));
        auto actual_ui=client->create_selected_ui_host(cycle_registry(),nullptr);
        // Test generic binding FIRST; only the later SAME genuine Host bind
        // installs the private implementation context, not UI observations.
        auto session=std::shared_ptr<const LiveSession>(host,&host->session());
        auto cursor=BnwRetainedTurnCursor::bind(session);
        (void)cursor->execute_timer_prefix();cursor->execute_selection_cycle_prefix(*actual_ui);
        refuses([&]{cursor->execute_engine_utility_prefix();});
        need(cursor->view().operation==2&&!cursor->view().engine_utility_attempted,
             "Generic session route inferred utility context from real UI");
        need(BnwRetainedTurnCursor::bind(std::shared_ptr<const ui::SelectedBnwPreviewHost>(host))==cursor,
             "Actual Host context changed the retained cursor");
        const auto before=client->capture({1024,768,1024,768,96},{});
        if(mode==0){
            cursor->execute_engine_utility_prefix();cursor->retire();
            refuses([&]{cursor->execute_engine_utility_prefix();});
            need(cursor->view().engine_utility_attempted&&cursor->view().operation==3,
                 "Retirement discarded actual utility once history");
        }else if(mode==1){
            client.reset();refuses([&]{cursor->execute_engine_utility_prefix();});
        }else if(mode==2){
            (void)std::const_pointer_cast<BnwGameTurnTimerOwner>(cursor->timer_owner())->reset_for_current_player();
            refuses([&]{cursor->execute_engine_utility_prefix();});
        }else if(mode==3){
            bool rejected=false;std::thread worker([&]{try{cursor->execute_engine_utility_prefix();}
                catch(const std::exception&){rejected=true;}});worker.join();
            need(rejected&&client->is_current(before),"Foreign utility attempt borrowed or changed UI");
        }else if(mode==4){
            // A writer attempt permanently poisons the actual sealed DB scope.
            refuses([&]{const_cast<content::RulesDatabase&>(host->database()).apply_sql("UPDATE Defines SET Value=Value WHERE 0");});
            refuses([&]{cursor->execute_engine_utility_prefix();});
        }else{
            const auto checkpoint=host->session().checkpoint();
            host->session().restore(checkpoint);
            refuses([&]{cursor->execute_engine_utility_prefix();});
            refuses([&]{(void)BnwRetainedTurnCursor::bind(std::shared_ptr<const ui::SelectedBnwPreviewHost>(host));});
        }
        if(mode!=0)need(!cursor->view().engine_utility_attempted&&cursor->view().operation==2,
             "Invalid utility context stored operation 3");
        need(!host->session().readiness().can_continue(),"Utility prefix issued full turn readiness");
    }
}
void nonstock_public_factory(const std::filesystem::path& save,const std::filesystem::path& game){
    auto bootstrap=ui::bootstrap_selected_bnw_preview(save,game,0,true);
    need(bootstrap.preview.host&&bootstrap.activation&&bootstrap.postprocess,
         "Nonstock regression actual stock activation/import absent");
    std::shared_ptr<ui::SelectedBnwPreviewHost> stock(std::move(bootstrap.preview.host));
    // CREATE_NEW makes the tiny readable source fixture private. Never touch
    // the actual original DLL or replace a pre-existing file.
    const auto path=std::filesystem::temp_directory_path()/
        ("bnw194-nonstock-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(qpc())+".bin");
    const HANDLE file=CreateFileW(path.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    need(file!=INVALID_HANDLE_VALUE,"Nonstock fixture CREATE_NEW failed");
    const char bytes[]="readable nonstock GameCore source194 regression";DWORD written=0;
    const BOOL stored=WriteFile(file,bytes,static_cast<DWORD>(sizeof(bytes)),&written,nullptr);
    const BOOL closed=CloseHandle(file);
    struct Cleanup {std::filesystem::path path;~Cleanup(){std::error_code error;std::filesystem::remove(path,error);}} cleanup{path};
    need(stored&&closed&&written==sizeof(bytes),"Nonstock fixture write failed");
    ui::BnwSourceDecodeContract source;
    source.gamecore_executable=path;source.gamecore_sha256=identify_file(path).sha256;
    source.context.decode_map_regions=true;source.context.decode_teams=true;
    source.context.decode_all_players=true;source.context.decode_supporting_classes=true;
    need(stock->session().state().world.all_players.has_value(),"Actual decoded source dimensions absent");
    source.context.all_players=stock->session().state().world.all_players->context;
    source.context.permit_live_production_overlay=stock->session().source().source.live_city_production.has_value();
    source.context.permit_live_research_overlay=stock->session().source().source.live_team_research.has_value();
    source.context.permit_live_treasury_overlay=stock->session().source().source.live_treasury.has_value();
    source.context.permit_live_research_turn_overlay=stock->session().source().source.live_research_turn.has_value();
    source.context.permit_live_research_selection_overlay=stock->session().source().source.live_research_selection.has_value();
    source.context.permit_live_random_overlay=stock->session().source().source.live_simulation_random.has_value();
    source.context.permit_live_bnw_game_clock_overlay=stock->session().source().source.live_bnw_game_clock.has_value();
    source.context.permit_live_bnw_map_plots_overlay=stock->session().source().source.live_bnw_map_plots.has_value();
    source.context.permit_live_bnw_units_overlay=stock->session().source().source.live_bnw_units.has_value();
    source.context.permit_live_bnw_area_units_overlay=stock->session().source().source.live_bnw_area_units.has_value();
    source.context.permit_live_bnw_player_proximity_overlay=stock->session().source().source.live_bnw_player_proximity.has_value();
    source.context.permit_live_bnw_team_meet_overlay=stock->session().source().source.live_bnw_team_meet.has_value();
    source.context.permit_live_bnw_exploration_dirty_overlay=stock->session().source().source.live_bnw_exploration_dirty.has_value();
    source.context.permit_live_bnw_player_strategy_overlay=stock->session().source().source.live_bnw_player_strategy.has_value();
    source.context.permit_live_bnw_player_core_overlay=stock->session().source().source.live_bnw_player_core.has_value();
    source.context.permit_live_bnw_city_core_overlay=stock->session().source().source.live_bnw_city_core.has_value();
    source.context.permit_live_bnw_city_production_overlay=stock->session().source().source.live_bnw_city_production.has_value();
    source.context.permit_live_bnw_city_management_overlay=stock->session().source().source.live_bnw_city_management.has_value();
    source.context.permit_live_bnw_city_tail_overlay=stock->session().source().source.live_bnw_city_tail.has_value();
    source.context.permit_live_bnw_treasury_balance_overlay=stock->session().source().source.live_bnw_treasury_balance.has_value();
    source.context.permit_live_bnw_team_progress_overlay=stock->session().source().source.live_bnw_team_progress.has_value();
    source.context.permit_live_bnw_culture_bomb_timer_overlay=stock->session().source().source.live_bnw_culture_bomb_timer.has_value();
    source.context.permit_live_bnw_research_queue_overlay=stock->session().source().source.live_bnw_research_queue.has_value();
    source.context.permit_live_bnw_maintenance_expense_overlay=stock->session().source().source.live_bnw_maintenance_expense.has_value();
    source.context.permit_live_bnw_player_knowledge_overlay=stock->session().source().source.live_bnw_player_knowledge.has_value();
    source.context.permit_live_bnw_player_administration_overlay=stock->session().source().source.live_bnw_player_administration.has_value();
    source.context.permit_live_bnw_game_supporting_overlay=stock->session().source().source.live_bnw_game_supporting.has_value();
    source.context.permit_live_bnw_game_score_replay_overlay=stock->session().source().source.live_bnw_game_score_replay.has_value();
    source.context.permit_live_bnw_game_deals_overlay=stock->session().source().source.live_bnw_game_deals.has_value();
    source.context.permit_live_bnw_team_turn_overlay=stock->session().source().source.live_bnw_team_turn.has_value();
    source.context.permit_live_bnw_ui_transients_overlay=stock->session().source().source.live_bnw_ui_transients.has_value();
    auto result=ui::make_selected_bnw_preview_host(save,stock->database(),*bootstrap.activation,
        *bootstrap.postprocess,source,0,true);
    if(result.blocker)throw std::runtime_error(result.blocker->reason);
    need(bool(result.host),"Public matching-hash nonstock source did not reach genuine Host");
    std::shared_ptr<ui::SelectedBnwPreviewHost> host(std::move(result.host));
    need(host->database().immutable_preview_fingerprints_current(),"Nonstock actual Host rules not immutable");
    auto client=ui::BnwClientFrameOwner::from_selected_host(host,test_detail::selected_city_frame_sources(host));
    auto actual_ui=client->create_selected_ui_host(cycle_registry(),nullptr);
    auto cursor=BnwRetainedTurnCursor::bind(std::shared_ptr<const ui::SelectedBnwPreviewHost>(host));
    (void)cursor->execute_timer_prefix();cursor->execute_selection_cycle_prefix(*actual_ui);
    need(!cursor->no_selection_list_cycle(*actual_ui),"Nonstock genuine SAME UI operation 2 absent");
    const auto frame=client->capture({1024,768,1024,768,96},{});
    const auto rng=host->session().state().random.gameplay.state();
    refuses([&]{cursor->execute_engine_utility_prefix();});
    need(!cursor->view().engine_utility_attempted&&cursor->view().operation==2&&
         cursor->view().frontier==BnwRetainedTurnFrontier::engine_utility_do_turn_unavailable&&
         client->is_current(frame)&&host->session().state().random.gameplay.state()==rng,
         "Actual nonstock factory source granted utility permission or changed world/UI");
    need(!host->session().readiness().can_continue(),"Nonstock source granted full readiness");
}
std::int64_t actual_improvement(const saves::SavedMapCellProjection& cell,const ui::SelectedBnwPreviewHost& host){
    if(cell.improvement_type)return *cell.improvement_type;
    need(cell.improvement_type_hash.has_value(),"Barbarian test improvement unresolved");
    if(!*cell.improvement_type_hash)return -1;
    const auto* type=host.types().find_hash(*cell.improvement_type_hash);
    need(type&&type->table=="Improvements","Barbarian test hash absent from actual rules");return type->id;
}
void actual_barbarians(const std::filesystem::path& save,const std::filesystem::path& game){
    const auto baseline=cycle_host(save,game);const auto camp=resolve_bnw_barbarian_camp_improvement(baseline->defines());
    auto world=baseline->session().state().world;
    need(world.supporting_classes&&world.map_projection&&world.supporting_classes->barbarians.size()>=4,
         "Actual barbarian supporting fixture absent");
    // A real versioned Civis supporting overlay supplies counter data before
    // genuine import; it does not install completion, debug or UI authority.
    for(std::size_t index=0;index<world.supporting_classes->barbarians.size();++index){
        auto& row=world.supporting_classes->barbarians[index];row.units_spawned=9;
        row.counter=actual_improvement(world.map_projection->cells[index],*baseline)==camp?3:
            (index%4==0?2:index%4==1?-4:index%4==2?-1:0);
    }
    auto document=saves::make_civis_import_document(baseline->session().source().source,identify_file(save).sha256);
    const auto supporting_id=static_cast<std::uint32_t>(saves::CivisSaveSectionId::live_bnw_game_supporting);
    std::erase_if(document.sections,[&](const auto& row){return row.id==supporting_id;});
    document.sections.push_back(make_bnw_game_supporting_checkpoint(world,baseline->session().source().snapshot,baseline->identity()));
    const auto path=std::filesystem::temp_directory_path()/
        ("bnw197-counters-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(qpc())+".CivisSave");
    saves::write_civis_save(path,document); // Exclusive create, never original save.
    struct Cleanup {std::filesystem::path path;~Cleanup(){std::error_code error;std::filesystem::remove(path,error);}} cleanup{path};
    const auto host=cycle_host(path,game);
    auto client=ui::BnwClientFrameOwner::from_selected_host(host,test_detail::selected_city_frame_sources(host));
    auto actual_ui=client->create_selected_ui_host(cycle_registry(),nullptr);
    auto cursor=BnwRetainedTurnCursor::bind(std::shared_ptr<const ui::SelectedBnwPreviewHost>(host));
    refuses([&]{cursor->execute_barbarians_begin_prefix();});
    (void)cursor->execute_timer_prefix();cursor->execute_selection_cycle_prefix(*actual_ui);
    refuses([&]{cursor->execute_barbarians_begin_prefix();});
    need(!cursor->view().barbarians_attempted&&cursor->view().operation==2,"Barbarian preflight fabricated predecessor");
    cursor->execute_engine_utility_prefix();
    const auto frame=client->capture({1024,768,1024,768,96},{});
    const auto allocation=host->session().read_state_lease();const auto generation=host->session().state_generation();
    const auto before=allocation->world.supporting_classes->barbarians;
    const auto map_rng=allocation->random.map.state(),game_rng=allocation->random.gameplay.state();
    const auto turn=allocation->setup.game_turn;const auto timer=cursor->timer_owner()->read(cursor->timer_owner()->current_use());
    const auto revision=allocation->ui_source_revision;
    cursor->execute_barbarians_begin_prefix();
    need(cursor->view().barbarians_attempted&&cursor->view().operation==4&&
         cursor->view().frontier==BnwRetainedTurnFrontier::game_cache_update,
         "Actual barbarian return crossed next cache/technology boundary");
    const auto now=host->session().read_state_lease();
    std::size_t camps=0,removed=0,cooldowns=0;
    for(std::size_t index=0;index<before.size();++index){
        const auto& old=before[index];const auto& row=now->world.supporting_classes->barbarians[index];
        auto counter=old.counter,spawned=old.units_spawned;
        if(counter>0){if(actual_improvement(now->world.map_projection->cells[index],*host)==camp){--counter;++camps;}
            else{counter=-1;spawned=-1;++removed;}}
        else if(counter<-1){++counter;++cooldowns;}
        need(row.counter==counter&&row.units_spawned==spawned&&row.attacked==old.attacked,
             "Actual live barbarian pair differs from source branch");
    }
    const auto after_timer=cursor->timer_owner()->read(cursor->timer_owner()->current_use());
    need(now==allocation&&host->session().state_generation()==generation&&client->is_current(frame)&&
         now->ui_source_revision==revision&&now->random.map.state()==map_rng&&now->random.gameplay.state()==game_rng&&
         now->setup.game_turn==turn&&after_timer.current_start==timer.current_start&&after_timer.game_turn_start==timer.game_turn_start&&
         after_timer.operation==timer.operation&&!cursor->no_selection_list_cycle(*actual_ui),
         "Counter-only leaf changed allocation/actual debug/UI/timer/RNG/clock");
    need(removed&&cooldowns,"Actual mixed-counter fixture failed to cover cleared/cooldown branches");
    std::cout<<"actual live barbarians camps="<<camps<<" cleared="<<removed<<" cooldowns="<<cooldowns<<'\n';
    refuses([&]{cursor->execute_barbarians_begin_prefix();});
    need(cursor->view().operation==4&&client->is_current(frame),"Repeated barbarian prefix changed world/frame");
    refuses([&]{(void)host->export_import_clock_checkpoint();});
    client.reset();refuses([&]{cursor->execute_barbarians_begin_prefix();});
    need(cursor->view().barbarians_attempted&&cursor->view().operation==4&&
         cursor->view().frontier==BnwRetainedTurnFrontier::quarantined,"Retired UI discarded op4 history");
}
void barbarian_guards(const std::filesystem::path& save,const std::filesystem::path& game){
    for(int mode=0;mode!=6;++mode){
        const auto host=cycle_host(save,game);
        auto client=ui::BnwClientFrameOwner::from_selected_host(host,test_detail::selected_city_frame_sources(host));
        auto actual_ui=client->create_selected_ui_host(cycle_registry(),nullptr);
        auto cursor=BnwRetainedTurnCursor::bind(std::shared_ptr<const ui::SelectedBnwPreviewHost>(host));
        (void)cursor->execute_timer_prefix();cursor->execute_selection_cycle_prefix(*actual_ui);cursor->execute_engine_utility_prefix();
        const auto rows=host->session().state().world.supporting_classes->barbarians;
        if(mode==0){bool refused=false;std::thread worker([&]{try{cursor->execute_barbarians_begin_prefix();}catch(const std::exception&){refused=true;}});
            worker.join();need(refused,"Foreign barbarian call accepted");}
        else if(mode==1){const auto checkpoint=host->session().checkpoint();host->session().restore(checkpoint);refuses([&]{cursor->execute_barbarians_begin_prefix();});}
        else if(mode==2){client.reset();refuses([&]{cursor->execute_barbarians_begin_prefix();});}
        else if(mode==3){cursor->retire();refuses([&]{cursor->execute_barbarians_begin_prefix();});}
        else{
            // Public mutation invalidates the genuine retained owner first.
            // Forged shape/operand data cannot manufacture permission to store.
            auto& world=host->session().mutable_state().world;
            if(mode==4)world.map_projection->cells.pop_back();
            else world.map_plots->plots.front().fields.clear();
            refuses([&]{cursor->execute_barbarians_begin_prefix();});
        }
        need(!cursor->view().barbarians_attempted&&cursor->view().operation==3,"Invalid barbarian call recorded stores");
        const auto& after=host->session().state().world.supporting_classes->barbarians;
        need(after.size()==rows.size(),"Invalid barbarian call changed row shape");
        for(std::size_t index=0;index<rows.size();++index)need(after[index].counter==rows[index].counter&&
            after[index].units_spawned==rows[index].units_spawned,"Invalid barbarian call changed earlier row");
    }
}
std::int64_t actual_and_slots(const ui::SelectedBnwPreviewHost& host){
    const auto* entry=host.defines().find("NUM_AND_TECH_PREREQS");
    need(entry&&entry->lookup.found&&entry->kind==content::GlobalDefineKind::integer&&
         entry->lookup.sqlite_code==0&&entry->lookup.error.empty()&&!entry->retained_previous,
         "Actual religion fixture AND prerequisite define unavailable");
    const auto* slots=std::get_if<std::int64_t>(&entry->lookup.value);
    need(slots&&*slots>0&&*slots<=1024,"Actual religion fixture AND prerequisite count unsupported");return *slots;
}
void actual_religion_cache(const std::filesystem::path& save,const std::filesystem::path& game){
    const auto baseline=cycle_host(save,game);
    // Cold capture preserves genuine original-import moves/busy owners through
    // Section41. A bare payload import intentionally has no such Civis owners.
    const auto cold=baseline->export_import_clock_checkpoint();
    need(!cold.full_persistence,"Cold fixture falsely claims full turn persistence");
    const auto transient_id=static_cast<std::uint32_t>(saves::CivisSaveSectionId::live_bnw_ui_transients);
    const saves::CivisSaveSection* cold_transient=nullptr;
    for(const auto& row:cold.document.sections)if(row.id==transient_id){
        need(!cold_transient,"Cold fixture has duplicate transient sections");cold_transient=&row;}
    need(cold_transient,"Actual Original cold capture omitted genuine UI transients");
    // The independently source-audited formula leaf is the value oracle here;
    // this regression verifies its genuine retained binding and actual stores.
    const auto expected=compute_bnw_world_religion_tech_progress(
        baseline->session().state(),baseline->technologies(),actual_and_slots(*baseline));
    for(bool change:{true,false}){
        auto world=baseline->session().state().world;
        const auto stale=expected==-1?std::int64_t{7}:std::int64_t{-1};
        const auto imported=change?stale:expected;
        std::get<saves::Expansion2PrefixObservation>(world.game_state.prefix).scalars[23]=imported;
        // Supporting data alone supplies no cursor permission or UI ownership.
        // Preserve the real cold owner's Section41 while replacing only ID37.
        auto document=cold.document;
        const auto id=static_cast<std::uint32_t>(saves::CivisSaveSectionId::live_bnw_game_supporting);
        std::erase_if(document.sections,[&](const auto& row){return row.id==id;});
        document.sections.push_back(make_bnw_game_supporting_checkpoint(world,baseline->session().source().snapshot,baseline->identity()));
        const auto path=std::filesystem::temp_directory_path()/
            ("bnw201-religion-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(qpc())+".CivisSave");
        saves::write_civis_save(path,document);
        struct Cleanup {std::filesystem::path path;~Cleanup(){std::error_code error;std::filesystem::remove(path,error);}} cleanup{path};
        const auto host=cycle_host(path,game);
        (void)host->session().bnw_game_debug_mode(); // Public authenticated getter refuses a retired debug owner.
        need(host->database().immutable_preview_fingerprints_current(),
             "Religion fixture lacks actual immutable Host/debug owner");
        need(std::get<saves::Expansion2PrefixObservation>(host->session().state().world.game_state.prefix).scalars[23]==imported,
             "Actual supporting codec/factory did not admit requested cache value");
        const auto oracle=compute_bnw_world_religion_tech_progress(host->session().state(),host->technologies(),actual_and_slots(*host));
        need(oracle==expected,"Supporting cache-only overlay changed source religion operands");
        const auto baseline_export=host->export_import_clock_checkpoint();
        need(!baseline_export.full_persistence,"Fresh cache fixture falsely claims durable turn history");
        const saves::CivisSaveSection* restored_transient=nullptr;
        for(const auto& row:baseline_export.document.sections)if(row.id==transient_id){
            need(!restored_transient,"Restored cache fixture duplicated transient section");restored_transient=&row;}
        need(restored_transient&&restored_transient->flags==cold_transient->flags&&
             restored_transient->bytes==cold_transient->bytes,
             "Cache-only fixture lost or changed actual cold moves/busy transients");
        auto client=ui::BnwClientFrameOwner::from_selected_host(host,test_detail::selected_city_frame_sources(host));
        auto actual_ui=client->create_selected_ui_host(cycle_registry(),nullptr);
        auto cursor=BnwRetainedTurnCursor::bind(std::shared_ptr<const ui::SelectedBnwPreviewHost>(host));
        refuses([&]{cursor->execute_game_cache_prefix();});refuses([&]{cursor->execute_world_religion_tech_progress_prefix();});
        need(cursor->view().operation==0&&!cursor->view().game_cache_attempted&&!cursor->view().religion_progress_attempted,
             "Missing predecessors granted operation5/6");
        (void)cursor->execute_timer_prefix();cursor->execute_selection_cycle_prefix(*actual_ui);
        cursor->execute_engine_utility_prefix();cursor->execute_barbarians_begin_prefix();
        refuses([&]{cursor->execute_world_religion_tech_progress_prefix();});
        const auto state=host->session().read_state_lease();const auto generation=host->session().state_generation();
        const auto scalars=std::get<saves::Expansion2PrefixObservation>(state->world.game_state.prefix).scalars;
        const auto revision=state->ui_source_revision;const auto turn=state->setup.game_turn;
        const auto map_rng=state->random.map.state(),game_rng=state->random.gameplay.state();
        const auto timer=cursor->timer_owner()->read(cursor->timer_owner()->current_use());
        const auto barbarians=state->world.supporting_classes->barbarians;
        const auto city=actual_ui->selected_city();const auto frame=client->capture({1024,768,1024,768,96},{});
        cursor->execute_game_cache_prefix();
        need(cursor->view().game_cache_attempted&&!cursor->view().religion_progress_attempted&&cursor->view().operation==5&&
             cursor->view().frontier==BnwRetainedTurnFrontier::world_religion_tech_progress&&
             std::get<saves::Expansion2PrefixObservation>(state->world.game_state.prefix).scalars==scalars&&
             state->ui_source_revision==revision&&client->is_current(frame),
             "Empty SDK cache step changed world or crossed religion frontier");
        refuses([&]{cursor->execute_game_cache_prefix();});
        cursor->execute_world_religion_tech_progress_prefix();
        need(cursor->view().game_cache_attempted&&cursor->view().religion_progress_attempted&&cursor->view().operation==6&&
             cursor->view().frontier==BnwRetainedTurnFrontier::game_update_score,
             "Religion prefix crossed actual updateScore frontier");
        auto after_scalars=std::get<saves::Expansion2PrefixObservation>(state->world.game_state.prefix).scalars;
        need(after_scalars[23]==oracle,"Actual SAME allocation cache store differs from source formula");
        after_scalars[23]=scalars[23];need(after_scalars==scalars,"Religion leaf changed another raw Game scalar");
        const auto after_timer=cursor->timer_owner()->read(cursor->timer_owner()->current_use());
        (void)host->session().bnw_game_debug_mode();
        need(host->session().read_state_lease()==state&&host->session().state_generation()==generation&&
             actual_ui->selected_city()==city&&
             !cursor->no_selection_list_cycle(*actual_ui)&&state->setup.game_turn==turn&&
             state->random.map.state()==map_rng&&state->random.gameplay.state()==game_rng&&
             after_timer.operation==timer.operation&&after_timer.current_start==timer.current_start&&
             after_timer.game_turn_start==timer.game_turn_start&&
             state->ui_source_revision==revision+(change?1:0)&&client->is_current(frame)==!change,
             "Religion leaf changed lifecycle/timer/RNG/clock or lost cache snapshot invalidation");
        for(std::size_t index=0;index<barbarians.size();++index){
            const auto& old=barbarians[index];const auto& now=state->world.supporting_classes->barbarians[index];
            need(old.counter==now.counter&&old.units_spawned==now.units_spawned&&old.attacked==now.attacked,
                 "Religion leaf changed prior barbarian counters");
        }
        cursor->require_current();const auto fresh=client->capture({1024,768,1024,768,96},{});
        need(client->is_current(fresh),"Religion cache retired SAME actual frame/input owner");
        refuses([&]{cursor->execute_game_cache_prefix();});refuses([&]{cursor->execute_world_religion_tech_progress_prefix();});
        need(cursor->view().operation==6&&client->is_current(fresh)&&state->ui_source_revision==revision+(change?1:0),
             "Repeated cache operation changed revision or discarded history");
        refuses([&]{(void)host->export_import_clock_checkpoint();});
        need(host->session().has_bnw_operation_checkpoint_history()&&!host->session().readiness().can_continue(),
             "Cache encoding falsely granted durable operation history or continuation");
        client.reset();refuses([&]{cursor->execute_world_religion_tech_progress_prefix();});
        need(cursor->view().operation==6&&cursor->view().religion_progress_attempted&&
             cursor->view().frontier==BnwRetainedTurnFrontier::quarantined,
             "UI retirement discarded genuine religion operation history");
    }
}
void game_cache_guards(const std::filesystem::path& save,const std::filesystem::path& game){
    for(bool religion:{false,true})for(int mode=0;mode<7;++mode){
        const auto host=cycle_host(save,game);
        auto client=ui::BnwClientFrameOwner::from_selected_host(host,test_detail::selected_city_frame_sources(host));
        auto actual_ui=client->create_selected_ui_host(cycle_registry(),nullptr);
        auto cursor=BnwRetainedTurnCursor::bind(std::shared_ptr<const ui::SelectedBnwPreviewHost>(host));
        (void)cursor->execute_timer_prefix();cursor->execute_selection_cycle_prefix(*actual_ui);
        cursor->execute_engine_utility_prefix();cursor->execute_barbarians_begin_prefix();
        if(religion)cursor->execute_game_cache_prefix();
        const auto state=host->session().read_state_lease();
        const auto value=std::get<saves::Expansion2PrefixObservation>(state->world.game_state.prefix).scalars[23];
        const auto revision=state->ui_source_revision;
        const auto invoke=[&]{if(religion)cursor->execute_world_religion_tech_progress_prefix();else cursor->execute_game_cache_prefix();};
        if(mode==0){const auto checkpoint=host->session().checkpoint();host->session().restore(checkpoint);refuses(invoke);}
        else if(mode==1){client.reset();refuses(invoke);}
        else if(mode==2){cursor->retire();refuses(invoke);}
        else if(mode==3){bool refused=false;std::thread worker([&]{try{invoke();}catch(const std::exception&){refused=true;}});
            worker.join();need(refused,"Foreign cache invocation accepted");}
        else if(mode==4){(void)std::const_pointer_cast<BnwGameTurnTimerOwner>(cursor->timer_owner())->reset_for_current_player();refuses(invoke);}
        else if(mode==5){refuses([&]{const_cast<content::RulesDatabase&>(host->database()).apply_sql("UPDATE Defines SET Value=Value WHERE 0");});refuses(invoke);}
        else{
            // Public mutation retires genuine owners. This proves no permission
            // can be forged; it is not an isolated malformed-input oracle.
            std::get<saves::Expansion2PrefixObservation>(host->session().mutable_state().world.game_state.prefix).version=2;
            refuses(invoke);
        }
        const auto view=cursor->view();
        need(view.operation==(religion?5:4)&&!view.religion_progress_attempted&&view.game_cache_attempted==religion&&
             std::get<saves::Expansion2PrefixObservation>(state->world.game_state.prefix).scalars[23]==value&&
             state->ui_source_revision==revision,"Invalid cache invocation stored scalar/revision/once permission");
    }
    const auto host=cycle_host(save,game);auto session=clone(*host);auto generic=BnwRetainedTurnCursor::bind(session);
    (void)generic->execute_timer_prefix();
    refuses([&]{generic->execute_game_cache_prefix();});refuses([&]{generic->execute_world_religion_tech_progress_prefix();});
    need(generic->view().operation==1&&!generic->view().game_cache_attempted&&!generic->view().religion_progress_attempted,
         "Generic timer-only session granted actual cache phase");
}
bool actual_score_dirty(const LiveSessionState& state){
    const saves::NamedSaveFlag* found=nullptr;
    for(const auto& flag:state.world.game_state.flags)if(flag.name&&std::string_view(flag.name)=="score_dirty"){
        need(!found,"Actual score fixture duplicate flag");found=&flag;
    }
    need(found,"Actual score fixture missing flag");return found->value;
}
void to_score_frontier(const std::shared_ptr<BnwRetainedTurnCursor>& cursor,const ui::LiveUiHost& ui){
    (void)cursor->execute_timer_prefix();cursor->execute_selection_cycle_prefix(ui);
    cursor->execute_engine_utility_prefix();cursor->execute_barbarians_begin_prefix();
    cursor->execute_game_cache_prefix();cursor->execute_world_religion_tech_progress_prefix();
}
void actual_score(const std::filesystem::path& save,const std::filesystem::path& game,bool require_dirty=false,int expected_changed=-1){
    const auto host=cycle_host(save,game);
    auto client=ui::BnwClientFrameOwner::from_selected_host(host,test_detail::selected_city_frame_sources(host));
    auto ui=client->create_selected_ui_host(cycle_registry(),nullptr);
    auto cursor=BnwRetainedTurnCursor::bind(std::shared_ptr<const ui::SelectedBnwPreviewHost>(host));
    need(!cursor->score_dirty(*ui).has_value(),"Owned UI inferred an imported score dirty flag");
    refuses([&]{cursor->execute_game_score_prefix();});to_score_frontier(cursor,*ui);
    const auto state=host->session().read_state_lease();const bool dirty=actual_score_dirty(*state);
    need(!require_dirty||dirty,"Diagnostic clock codec fixture did not import dirty score flag");
    const auto generation=host->session().state_generation(),revision=state->ui_source_revision;
    const auto* players=state->world.all_players->players.data();
    const auto* scores=state->world.game_state.roster->arrays[3].data();
    const auto* fields=state->world.all_players->players[0].tail.fields.data();
    const auto rng=state->random.gameplay.state(),map_rng=state->random.map.state();const auto turn=state->setup.game_turn;
    auto oracle=*state; // Numeric/replay comparison only; never supplies live authority.
    update_bnw_game_score(oracle,host->database(),host->types(),host->defines());
    std::size_t changed=0;
    for(std::size_t id=0;id<63;++id)if(state->world.game_state.roster->arrays[3][id]!=oracle.world.game_state.roster->arrays[3][id])++changed;
    need(expected_changed<0||changed==static_cast<std::size_t>(expected_changed),"Synthetic score codec changed-score count differs from controlled input");
    const auto setter_count=cursor->score_dirty_setter_count(*ui);
    const auto expected=make_bnw_game_score_checkpoint(oracle.world,host->session().source().snapshot,host->identity());
    const auto frame=client->capture({1024,768,1024,768,96},{});
    cursor->execute_game_score_prefix();
    need(cursor->view().operation==7&&cursor->view().score_attempted&&cursor->view().frontier==BnwRetainedTurnFrontier::game_deals_turn,
         "Score crossed next deals frontier or lost actual once history");
    need(!actual_score_dirty(*state)&&state->world.game_state.roster->arrays==oracle.world.game_state.roster->arrays,
         "Actual score/ranks/source flag differ from audited numeric updater");
    const auto actual=make_bnw_game_score_checkpoint(state->world,host->session().source().snapshot,host->identity());
    need(actual.bytes==expected.bytes,"Actual score replay differs from source codec comparison");
    need(host->session().read_state_lease()==state&&host->session().state_generation()==generation&&
         state->world.all_players->players.data()==players&&state->world.game_state.roster->arrays[3].data()==scores&&
         state->world.all_players->players[0].tail.fields.data()==fields&&
         state->random.gameplay.state()==rng&&state->random.map.state()==map_rng&&state->setup.game_turn==turn&&
         state->ui_source_revision==revision+(dirty?1:0)&&client->is_current(frame)==!dirty,
         "Actual score replaced SAME allocation/storage or changed RNG/clock/frame contract");
    const auto ui_flag=cursor->score_dirty(*ui);
    need(cursor->score_dirty_setter_count(*ui)==setter_count+(dirty?changed:0),"Actual typed Score_DIRTY_BIT setter occurrence count differs from changed players");
    need(dirty&&changed?ui_flag.has_value()&&*ui_flag:!ui_flag.has_value(),
         "Score UI flag omitted actual setter or inferred a flag on source no-setter path");
    refuses([&]{cursor->execute_game_score_prefix();});refuses([&]{(void)host->export_import_clock_checkpoint();});
    need(cursor->view().operation==7&&host->session().has_bnw_operation_checkpoint_history()&&!host->session().readiness().can_continue(),
         "Score replay granted duplicate or durable full-turn authority");
    if(expected_changed>=0)std::cout<<"SYNTHETIC codec input: actual owned dirty score; setter count "<<changed<<"\n";
    else std::cout<<(dirty?"Original selected input: DIRTY score branch exercised\n":"Original selected input: CLEAN return; original dirty parity remains untested\n");
}
// SYNTHETIC/CODEC INPUT PROVENANCE: diagnostic publication effects only.
// Neither this generic clock producer nor its event record is actual engine
// clock completion, an Original dirty save, or score/cursor authority.
void diagnostic_clock_score_fixture(const std::filesystem::path& save,const std::filesystem::path& game){
    const auto baseline=cycle_host(save,game);auto diagnostic=clone(*baseline);
    const auto old_turn=diagnostic->state().setup.game_turn;
    const auto old_elapsed=diagnostic->state().maintenance.elapsed_turns;
    std::vector<std::string> effects;
    const auto current=[&]{need(diagnostic->state().setup.game_turn==old_turn+1&&
        diagnostic->state().world.pregame->archive.integers.at("game_turn")==old_turn+1&&
        diagnostic->state().maintenance.elapsed_turns==old_elapsed+1&&actual_score_dirty(diagnostic->state()),
        "Diagnostic clock publication operands differ from committed producer");};
    BnwGameClockServices services;
    services.net_debug_log=[&](std::string_view message){current();
        need(message=="Game Turn "+std::to_string(old_turn+1)+"\n","Diagnostic clock log operand mismatch");effects.emplace_back(message);};
    services.turn_timer_dirty=[&]{current();effects.emplace_back("turn_timer");};
    services.game_data_dirty=[&]{current();effects.emplace_back("game_data");};
    services.gameplay_turn_changed=[&](std::int64_t turn){current();need(turn==old_turn+1,"Diagnostic turn operand mismatch");effects.emplace_back("turn_changed");};
    services.end_turn_timer_reset=[&]{current();effects.emplace_back("timer_reset");};
    const auto result=apply_live_action(*diagnostic,[&](LiveSessionState& state){return run_bnw_game_clock_phase(state,services);});
    need(result.outcome==LiveActionOutcome::committed&&diagnostic->state().game_clock.pending&&effects.empty(),
         "Diagnostic clock failed to stage real numeric producer before publication");
    refuses([&]{(void)make_bnw_game_clock_checkpoint(diagnostic->state(),baseline->session().source().snapshot,baseline->identity());});
    const auto published=publish_bnw_game_clock(*diagnostic,services);
    need(published.outcome==LiveActionOutcome::committed&&!diagnostic->state().game_clock.pending&&
         effects==std::vector<std::string>{"Game Turn "+std::to_string(old_turn+1)+"\n","turn_timer","game_data","turn_changed","timer_reset"},
         "Diagnostic clock did not execute all five effects in source order");
    need(publish_bnw_game_clock(*diagnostic,services).outcome==LiveActionOutcome::rejected&&effects.size()==5,
         "Diagnostic clock publication replay repeated an effect");
    // Normalize score/replay data with the existing diagnostic numeric producer.
    // Controlled second input deliberately stales one score through Section38;
    // this is SYNTHETIC admitted data, never an Original score parity claim.
    auto numeric=diagnostic->state();
    update_bnw_game_score(numeric,baseline->database(),baseline->types(),baseline->defines());
    for(bool stale:{false,true}){
        auto world=numeric.world;
        if(stale){auto& score=world.game_state.roster->arrays[3][0];
            need(score>=INT32_MIN&&score<=INT32_MAX,"Synthetic baseline score exceeds source i32");
            score+=score==INT32_MAX?-1:1;
        }
        auto document=saves::make_civis_import_document(baseline->session().source().source,identify_file(save).sha256);
        const auto clock_id=static_cast<std::uint32_t>(saves::CivisSaveSectionId::live_bnw_game_clock);
        std::erase_if(document.sections,[&](const auto& row){return row.id==clock_id||row.id==bnw_game_score_checkpoint_section_id;});
        document.sections.push_back(make_bnw_game_clock_checkpoint(diagnostic->state(),baseline->session().source().snapshot,baseline->identity()));
        document.sections.push_back(make_bnw_game_score_checkpoint(world,baseline->session().source().snapshot,baseline->identity()));
        const auto path=std::filesystem::temp_directory_path()/
            ("bnw205-synthetic-codec-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(qpc())+".CivisSave");
        saves::write_civis_save(path,document);
        struct Cleanup{std::filesystem::path path;~Cleanup(){std::error_code error;std::filesystem::remove(path,error);}} cleanup{path};
        actual_score(path,game,true,stale?1:0); // NEW genuine factory/Host/UI.
    }
}
void to_deals_frontier(const std::shared_ptr<BnwRetainedTurnCursor>& cursor,const ui::LiveUiHost& ui){
    to_score_frontier(cursor,ui);cursor->execute_game_score_prefix();
}
// Detached numeric oracle: never a retained issuer or a callback acknowledgement.
struct DealSourceExpected {bool ending=false,renewal_changed=false,dirty=false;};
DealSourceExpected deal_source_expected(const LiveSessionState& state){
    DealSourceExpected result;std::int64_t from=-1,to=-1;
    for(const auto& deal:state.world.history.deals.groups[1])for(const auto& item:deal.items){
        if(item.final_turn>-1&&item.final_turn==state.setup.game_turn){
            result.ending=true;result.renewal_changed=result.renewal_changed||*item.to_renewed;
            from=item.from_player;to=from==deal.from_player?deal.to_player:deal.from_player;
        }
    }
    for(const auto& deal:state.world.history.deals.groups[1])for(const auto& item:deal.items){
        if(item.final_turn<0||item.final_turn==state.setup.game_turn)continue;
        from=item.from_player;if(item.type==13)break;
    }
    if(result.ending){const auto active=detail::bnw_game_active_player_read(state);result.dirty=from==active||to==active;}
    return result;
}
LiveActionResult resource_deal_oracle(LiveSessionState& state,const ui::SelectedBnwPreviewHost& host){
    auto trial=state;auto old=make_bnw_game_deals_handler().run(trial,-1);
    if(old.outcome!=LiveActionOutcome::rejected){state=std::move(trial);return old;}
    if(old.reason.find("resource validity")==std::string::npos&&old.reason.find("expiring deal item")==std::string::npos)return old;
    try{
        // Value oracle has no actual UI issuer, import continuation or capability.
        // Resolve ALL mandatory expiration gates before touching its detached rows.
        for(const auto& deal:state.world.history.deals.groups[1])for(const auto& item:deal.items){
            if(item.final_turn>-1&&item.final_turn==state.setup.game_turn){
                const auto to=item.from_player==deal.from_player?deal.to_player:deal.from_player;
                (void)detail::bnw_player_team_read(state,static_cast<std::int32_t>(item.from_player));
                (void)detail::bnw_player_team_read(state,static_cast<std::int32_t>(to));
                if(!*item.from_renewed)return {LiveActionOutcome::rejected,"non-renewed expiry"};
            }
        }
        for(const auto& deal:state.world.history.deals.groups[1]){
            bool invalid=false,peace=false;
            for(const auto& item:deal.items){
                if(item.final_turn<0||item.final_turn==state.setup.game_turn)continue;
                if(item.type==3){if(compute_bnw_deal_resource_total_diagnostic(state,host.database(),host.types(),host.defines(),item.from_player,item.data[0])<0)invalid=true;}
                else if(item.type==13){peace=true;break;}
            }
            if(invalid&&!peace)return {LiveActionOutcome::rejected,"negative resource requires cancellation"};
        }
        for(auto& deal:state.world.history.deals.groups[1])for(auto& item:deal.items)
            if(item.final_turn>-1&&item.final_turn==state.setup.game_turn)item.to_renewed=false;
        auto current=state.world.history.deals.groups[1];state.world.history.deals.groups[1].clear();
        for(auto& deal:current)state.world.history.deals.groups[deal.final_turn<=state.setup.game_turn?2:1].push_back(std::move(deal));
        return {LiveActionOutcome::committed,{}};
    }catch(const std::exception& e){return {LiveActionOutcome::rejected,e.what()};}
}
void actual_deals(const std::filesystem::path& save,const std::filesystem::path& game,int controlled=-1){
    const auto host=cycle_host(save,game);
    auto client=ui::BnwClientFrameOwner::from_selected_host(host,test_detail::selected_city_frame_sources(host));
    auto ui=client->create_selected_ui_host(cycle_registry(),nullptr);
    auto cursor=BnwRetainedTurnCursor::bind(std::shared_ptr<const ui::SelectedBnwPreviewHost>(host));
    refuses([&]{cursor->execute_game_deals_prefix();});
    need(cursor->view().operation==0&&!cursor->view().deals_attempted,"Deals skipped retained predecessors");
    to_deals_frontier(cursor,*ui);const auto state=host->session().read_state_lease();
    if(controlled>=10&&controlled<=13){
        const auto& resource=state->world.history.deals.groups[1].at(0).items.at(0);
        const auto loaded=compute_bnw_deal_resource_total_diagnostic(*state,host->database(),host->types(),host->defines(),resource.from_player,resource.data[0]);
        need(loaded==(controlled==10?9:controlled==11?0:-1),"Actual NEW Host did not restore sealed resource core input");
    }
    const auto generation=host->session().state_generation(),revision=state->ui_source_revision;
    const auto turn=state->setup.game_turn;const auto rng=state->random.gameplay.state(),map_rng=state->random.map.state();
    const auto ui_calls=cursor->score_dirty_setter_count(*ui);const auto ui_flag=cursor->score_dirty(*ui);
    const auto game_data_calls=cursor->game_data_dirty_setter_count(*ui);const auto game_data_flag=cursor->game_data_dirty(*ui);
    need(game_data_calls==0&&!game_data_flag,"New actual UI inferred an imported GameData flag");
    const auto source_effects=deal_source_expected(*state);
    if(controlled>=20&&controlled<=26&&controlled!=25){
        need(source_effects.ending&&source_effects.dirty==(controlled==20||controlled==23||controlled==24),
             "Renewed fixture did not exercise intended independent LAST-player UI predicate");
        need(source_effects.renewal_changed==(controlled!=24),"Renewed fixture world-change branch differs");
    }
    const auto* deals=&state->world.history.deals;const auto* current=&deals->groups[1];const auto* history=&deals->groups[2];
    const auto* current_storage=current->data();const auto old_history_size=history->size();const auto before=make_bnw_game_deals_checkpoint(state->world,host->session().source().snapshot,host->identity());
    const auto frame=client->capture({1024,768,1024,768,96},{});
    // Existing safe-branch calculation is a value oracle only, never live authority.
    auto oracle=*state;const auto outcome=resource_deal_oracle(oracle,*host);
    bool refused=false;std::string refusal;
    try{cursor->execute_game_deals_prefix();}catch(const std::exception& error){refused=true;refusal=error.what();}
    need(refused==(outcome.outcome==LiveActionOutcome::rejected),"Actual deals branch differs from existing source branch preflight");
    const auto actual=make_bnw_game_deals_checkpoint(state->world,host->session().source().snapshot,host->identity());
    if(refused){
        need(actual.bytes==before.bytes&&cursor->view().operation==7&&!cursor->view().deals_attempted&&
             cursor->view().frontier==BnwRetainedTurnFrontier::game_deals_turn&&state->ui_source_revision==revision&&client->is_current(frame),
             "Missing deal effect changed live lists/revision or consumed operation8");
        if(controlled==4)need(refusal.find("expiry")!=std::string::npos,"Expiration scan failed to precede resource-validity scan");
        if(controlled==12)need(refusal.find("negative resource")!=std::string::npos,"Negative resource skipped cancellation frontier");
        if(controlled==25||controlled==27)need(refusal.find("non-renewed expiry")!=std::string::npos,"Unsupported ending effects did not refuse before renewal stores");
        need(controlled<0||controlled==4||controlled==5||controlled==6||controlled==12||controlled==14||controlled==25||controlled==27,"Synthetic safe complete deal branch refused");
        need(cursor->game_data_dirty_setter_count(*ui)==game_data_calls&&cursor->game_data_dirty(*ui)==game_data_flag,"Refused ending consumed mandatory GameData UI");
    }else{
        need(controlled!=4&&controlled!=12&&controlled!=14&&controlled!=25&&controlled!=27,"Synthetic required ending effects were treated as completed");
        const auto expected=make_bnw_game_deals_checkpoint(oracle.world,host->session().source().snapshot,host->identity());
        const bool changed=oracle.world.history.deals.groups[2].size()!=old_history_size||source_effects.renewal_changed;
        need(cursor->game_data_dirty_setter_count(*ui)==game_data_calls+(source_effects.dirty?1:0),"Actual GameData typed setter count disagrees with LAST players");
        const auto game_data_actual=cursor->game_data_dirty(*ui);
        need(source_effects.dirty?game_data_actual.has_value()&&*game_data_actual:game_data_actual==game_data_flag,"Actual GameData flag omitted setter or inferred import");
        need(state->ui_source_revision==revision+(changed?1:0)&&client->is_current(frame)==!(changed||source_effects.dirty),
             "Deal migration snapshot invalidation differs from visible history change");
        need(actual.bytes==expected.bytes&&cursor->view().operation==8&&cursor->view().deals_attempted&&
             cursor->view().frontier==BnwRetainedTurnFrontier::teams_turn,"Deal migration/order/frontier differs from source oracle");
        refuses([&]{cursor->execute_game_deals_prefix();});
    }
    need(host->session().read_state_lease()==state&&host->session().state_generation()==generation&&
         &state->world.history.deals==deals && &state->world.history.deals.groups[1]==current&&
         &state->world.history.deals.groups[2]==history&&current->data()==current_storage&&
         state->setup.game_turn==turn&&state->random.gameplay.state()==rng&&state->random.map.state()==map_rng&&
         cursor->score_dirty_setter_count(*ui)==ui_calls&&cursor->score_dirty(*ui)==ui_flag,
         "Deals replaced SAME allocation/vector objects or changed clock/RNG/score UI");
    cursor->require_current();const auto fresh=client->capture({1024,768,1024,768,96},{});
    (void)host->session().bnw_game_debug_mode();
    need(client->is_current(fresh),
         "Deals revision retired SAME input/debug owner or fresh frame");
    refuses([&]{(void)host->export_import_clock_checkpoint();});
    need(!host->session().readiness().can_continue()&&host->session().has_bnw_operation_checkpoint_history(),
         "Deal list codec falsely granted durable retained turn or continuation");
    std::cout<<(controlled<0?"Selected original deals: ":"SYNTHETIC deal-codec input: ")<<
        (refused?refusal:"complete reached nonnegative/peace-protected resource and list branch")<<'\n';
}
void synthetic_deals(const std::filesystem::path& save,const std::filesystem::path& game){
    const auto baseline=cycle_host(save,game);const auto turn=baseline->session().state().setup.game_turn;
    need(turn>=0&&turn<=INT32_MAX-3,"Synthetic deal fixture requires source turn headroom");
    const auto make_item=[&](std::int64_t type,std::int64_t final){saves::TradedItemObservation item{};
        item.version=2;item.type=type;item.final_turn=final;item.from_player=0;item.from_renewed=false;item.to_renewed=false;return item;};
    const auto make_deal=[&](std::int64_t final){saves::DealObservation deal{};
        deal.version=3;deal.from_player=0;deal.to_player=1;deal.final_turn=final;
        deal.considering_renewal=false;deal.checked_renewal=false;deal.cancelled=false;
        deal.items={make_item(0,-1)};return deal;};
    for(int mode=0;mode<7;++mode){
        // Explicit SYNTHETIC ID39 input, admitted before a NEW actual factory.
        auto world=baseline->session().state().world;auto& current=world.history.deals.groups[1];current.clear();
        if(mode==1)current={make_deal(turn+3)};
        if(mode==2)current={make_deal(turn),make_deal(turn+3),make_deal(turn-1),make_deal(turn+2)};
        if(mode>=3){auto resource=make_deal(turn+3);resource.items={make_item(13,turn+1),make_item(3,turn+1)};
            if(mode==4){resource.items.erase(resource.items.begin());auto expiry=make_deal(turn+3);expiry.items={make_item(1,turn)};current={resource,expiry};}
            else if(mode==5){resource.items.erase(resource.items.begin());current={resource};}
            else if(mode==6){resource.items.front().final_turn=-1;current={resource};}
            else current={resource};
        }
        auto document=saves::make_civis_import_document(baseline->session().source().source,identify_file(save).sha256);
        std::erase_if(document.sections,[&](const auto& row){return row.id==bnw_game_deals_checkpoint_section_id;});
        document.sections.push_back(make_bnw_game_deals_checkpoint(world,baseline->session().source().snapshot,baseline->identity()));
        const auto path=std::filesystem::temp_directory_path()/
            ("bnw206-synthetic-deals-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(qpc())+".CivisSave");
        saves::write_civis_save(path,document);
        struct Cleanup{std::filesystem::path path;~Cleanup(){std::error_code error;std::filesystem::remove(path,error);}}cleanup{path};
        actual_deals(path,game,mode);
    }
}
saves::PlayerCoreField& resource_test_field(LiveSessionState& s,std::string_view name){
    auto& fields=s.world.all_players->players.at(0).core.fields;
    for(auto& field:fields)if(field.name&&std::string_view(field.name)==name)return field;
    throw std::runtime_error("Resource fixture actual field absent");
}
constexpr const char* resource_arrays[]={"m_paiNumResourceTotal","m_paiResourceImport","m_paiResourceFromMinors","m_paiResourcesSiphoned","m_paiResourceExport"};
void resource_test_slot(LiveSessionState& s,std::uint64_t hash,const std::array<std::int64_t,5>& values){
    for(std::size_t a=0;a<5;++a){auto& field=resource_test_field(s,resource_arrays[a]);std::size_t at=0;std::optional<std::size_t> last;
        for(auto row:field.hashes){if(!row)continue;need(at<field.values.size(),"Fixture saved hash payload absent");if(row==hash)last=at;++at;}
        need(last.has_value()&&at==field.values.size(),"Fixture cannot add missing known slot to sealed ID24 schema");field.values.at(*last)=values[a];
    }
    resource_test_field(s,"m_iStrategicResourceMod").values.at(0)=0;
    resource_test_field(s,"m_iMinorResourceBonusCount").values.at(0)=0;
}
void resource_total_cases(const std::filesystem::path& save,const std::filesystem::path& game){
    const auto baseline=cycle_host(save,game);const auto original=baseline->session().state();
    std::optional<std::int64_t> resource;std::uint64_t hash=0;
    // Choose an actual allocated strategic resource, with genuine saved coverage.
    const auto report=baseline->database().prefetch_info("Resources",[&](auto slot,auto,const auto& row){
        if(resource||row.integer("ResourceUsage")!=1)return true;
        const auto name=row.text("Type");if(!name)return true;auto candidate=original;
        need(slot<=static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max()),"Resource fixture slot outside source integer range");
        const auto resource_id=static_cast<std::int64_t>(slot);
        const auto h=content::saved_info_type_hash(*name);
        try{resource_test_slot(candidate,h,{4,2,3,1,1});
            need(compute_bnw_deal_resource_total_diagnostic(candidate,baseline->database(),baseline->types(),baseline->defines(),0,resource_id)==9,"Actual resource fixture formula differs");
            resource=resource_id;hash=h;
        }catch(const std::exception&){/* Report uncovered resource, never infer Reset0. */}
        return true;
    });
    need(report.all_succeeded()&&resource.has_value(),"Selected save lacks explicit strategic-resource fixture coverage");
    const auto calculate=[&](const LiveSessionState& s){return compute_bnw_deal_resource_total_diagnostic(s,baseline->database(),baseline->types(),baseline->defines(),0,*resource);};
    // DETACHED diagnostic schema tests. These altered hash rows are deliberately
    // NOT serialized, admitted by the decoder, or used as actual cursor inputs.
    auto numeric=original;resource_test_slot(numeric,hash,{4,2,3,1,1});need(calculate(numeric)==9,"Resource addition/subtraction order differs");
    auto& base=resource_test_field(numeric,resource_arrays[0]);std::uint64_t unknown=1;
    while(baseline->types().find_hash(unknown))++unknown;
    base.hashes={hash,0,unknown,hash};base.values={3,99,7};base.wire_count=4;
    need(calculate(numeric)==12,"Serialized last-wins/null/unknown resource consumption differs");
    const auto valid=numeric;
    {auto bad=valid;auto& f=resource_test_field(bad,resource_arrays[0]);f.hashes={unknown};f.values={0};f.wire_count=1;refuses([&]{(void)calculate(bad);});}
    {auto bad=valid;resource_test_field(bad,resource_arrays[0]).values.pop_back();refuses([&]{(void)calculate(bad);});}
    {auto bad=valid;resource_test_field(bad,resource_arrays[0]).values.push_back(1);refuses([&]{(void)calculate(bad);});}
    {auto bad=valid;resource_test_field(bad,resource_arrays[0]).hashes[0]=std::uint64_t(UINT32_MAX)+1;refuses([&]{(void)calculate(bad);});}
    {auto bad=valid;resource_test_field(bad,resource_arrays[0]).values.back()=std::int64_t(INT32_MAX)+1;refuses([&]{(void)calculate(bad);});}
    {auto bad=valid;resource_test_field(bad,resource_arrays[0]).wire_count=3;refuses([&]{(void)calculate(bad);});}
    {auto bad=valid;auto& fields=bad.world.all_players->players[0].core.fields;fields.push_back(resource_test_field(bad,"m_eID"));refuses([&]{(void)calculate(bad);});}
    {auto bad=valid;resource_test_field(bad,"m_eID").values[0]=1;refuses([&]{(void)calculate(bad);});}
    {auto bad=original;resource_test_slot(bad,hash,{INT32_MAX,1,0,0,1});refuses([&]{(void)calculate(bad);});} // later subtraction cannot hide source overflow
    {auto v=original;resource_test_slot(v,hash,{3,0,0,0,0});resource_test_field(v,"m_iStrategicResourceMod").values[0]=-50;
        need(calculate(v)==-1,"Strategic source multiply/divide signed truncation differs");}
    {auto bad=original;resource_test_slot(bad,hash,{INT32_MAX,0,0,0,0});resource_test_field(bad,"m_iStrategicResourceMod").values[0]=2;refuses([&]{(void)calculate(bad);});}
    {auto v=original;resource_test_slot(v,hash,{0,0,-3,0,0});resource_test_field(v,"m_iMinorResourceBonusCount").values[0]=1;
        const auto* entry=baseline->defines().find("MINOR_POLICY_RESOURCE_MULTIPLIER");
        need(entry&&entry->kind==content::GlobalDefineKind::integer,"Actual minor-resource define absent");
        const auto* multiplier=std::get_if<std::int64_t>(&entry->lookup.value);need(multiplier&&*multiplier>=INT32_MIN&&*multiplier<=INT32_MAX,"Actual multiplier source width absent");
        const auto product=-3LL * *multiplier;
        if(product>=INT32_MIN&&product<=INT32_MAX)need(calculate(v)==product/100,"Minor resource source multiply/divide order differs");
        else refuses([&]{(void)calculate(v);});
    }
    std::optional<std::uint64_t> foreign;
    baseline->database().prefetch_info("Technologies",[&](auto,auto,const auto& row){const auto name=row.text("Type");if(name){const auto h=content::saved_info_type_hash(*name);const auto* info=baseline->types().find_hash(h);if(info&&info->table!="Resources")foreign=h;}return true;});
    need(foreign.has_value(),"Fixture known foreign hash absent");
    {auto bad=valid;resource_test_field(bad,resource_arrays[0]).hashes[2]=*foreign;refuses([&]{(void)calculate(bad);});} // actual known mapping, not invented collision authority
    // Genuine NEW Host inputs use sealed existing ID24 values + full ID39.
    const auto turn=original.setup.game_turn;need(turn>=0&&turn<=INT32_MAX-3,"Resource fixture turn headroom unavailable");
    for(int mode=10;mode<=15;++mode){auto input=original;
        resource_test_slot(input,hash,mode==10?std::array<std::int64_t,5>{4,2,3,1,1}:mode==11?std::array<std::int64_t,5>{0,0,0,0,0}:std::array<std::int64_t,5>{0,0,0,0,1});
        auto item=[&](std::int64_t type,std::int64_t final){saves::TradedItemObservation v{};v.version=2;v.type=type;v.final_turn=final;v.from_player=0;v.from_renewed=false;v.to_renewed=false;v.data[0]=*resource;return v;};
        saves::DealObservation deal{};deal.version=3;deal.from_player=0;deal.to_player=1;deal.final_turn=turn;deal.considering_renewal=false;deal.checked_renewal=false;deal.cancelled=false;
        deal.items={item(3,turn+1)};
        if(mode==13)deal.items.push_back(item(13,turn+1)); // negative then later peace suppresses cancellation
        if(mode==14){deal.items[0].data[0]=INT32_MAX;deal.items.push_back(item(1,turn));} // expiry MUST precede invalid allocation query
        if(mode==15){deal.items[0].data[0]=INT32_MAX;deal.items.insert(deal.items.begin(),item(13,turn+1));} // earlier peace prevents getter entirely
        input.world.history.deals.groups[1]={deal};
        auto document=saves::make_civis_import_document(baseline->session().source().source,identify_file(save).sha256);
        std::erase_if(document.sections,[&](const auto& row){return row.id==bnw_game_deals_checkpoint_section_id||row.id==bnw_player_core_checkpoint_section_id||row.id==20;});
        document.sections.push_back(make_bnw_player_core_checkpoint(input.world,baseline->session().source().snapshot,baseline->identity(),true));
        document.sections.push_back(make_bnw_game_deals_checkpoint(input.world,baseline->session().source().snapshot,baseline->identity()));
        const auto path=std::filesystem::temp_directory_path()/("bnw207-resource-codec-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(qpc())+".CivisSave");
        saves::write_civis_save(path,document);
        struct Cleanup{std::filesystem::path path;~Cleanup(){std::error_code error;std::filesystem::remove(path,error);}}cleanup{path};
        actual_deals(path,game,mode);
    }
}
void renewed_expiry_cases(const std::filesystem::path& save,const std::filesystem::path& game){
    const auto baseline=cycle_host(save,game);const auto& state=baseline->session().state();
    const auto active=detail::bnw_game_active_player_read(state);need(active>=0&&active<64,"Renewed positive fixture needs actual active player");
    const auto other=(active+1)%64,third=(active+2)%64,fourth=(active+3)%64;
    const auto turn=state.setup.game_turn;need(turn>=0&&turn<=INT32_MAX-3,"Renewed fixture turn headroom unavailable");
    const auto make_deal=[&](std::int64_t from,std::int64_t to,bool renewed,bool to_renewed,std::int64_t final){
        saves::DealObservation deal{};deal.version=3;deal.from_player=from;deal.to_player=to;deal.final_turn=turn+3;
        deal.considering_renewal=false;deal.checked_renewal=false;deal.cancelled=false;
        saves::TradedItemObservation item{};item.version=2;item.type=1;item.from_player=from;item.final_turn=final;item.from_renewed=renewed;item.to_renewed=to_renewed;
        deal.items={item};return deal;
    };
    for(int mode=20;mode<=27;++mode){
        auto world=state.world;auto& current=world.history.deals.groups[1];
        current={make_deal(active,other,true,true,turn)};
        if(mode==21)current={make_deal(third,other,true,true,turn)};
        if(mode==22)current.push_back(make_deal(third,fourth,false,false,turn+1)); // LAST from overwrites active expiry participant
        if(mode==23){current={make_deal(third,active,true,true,turn),make_deal(other,fourth,false,false,turn+1)};} // LAST to stays active
        if(mode==24)*current[0].items[0].to_renewed=false; // no world change, real UI still mandatory
        if(mode==25)*current[0].items[0].from_renewed=false;
        if(mode==26)current.push_back(make_deal(third,fourth,true,true,turn)); // LAST expiry controls both operands
        if(mode==27)current.push_back(make_deal(third,fourth,false,true,turn)); // ALL preflight refuses before earlier clear
        auto document=saves::make_civis_import_document(baseline->session().source().source,identify_file(save).sha256);
        std::erase_if(document.sections,[&](const auto& row){return row.id==bnw_game_deals_checkpoint_section_id;});
        document.sections.push_back(make_bnw_game_deals_checkpoint(world,baseline->session().source().snapshot,baseline->identity()));
        const auto path=std::filesystem::temp_directory_path()/("bnw208-renewed-codec-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(qpc())+".CivisSave");
        saves::write_civis_save(path,document);
        struct Cleanup{std::filesystem::path path;~Cleanup(){std::error_code error;std::filesystem::remove(path,error);}}cleanup{path};
        actual_deals(path,game,mode); // NEW actual immutable factory/UI, not mutable Host bypass.
    }
}
const saves::NamedSaveInteger& team_test_field(const saves::TeamObservation& team,std::string_view name){
    const saves::NamedSaveInteger* result=nullptr;
    for(const auto& row:team.fields)if(row.name&&std::string_view(row.name)==name){need(!result,"Duplicate test team field");result=&row;}
    need(result,"Actual test team field absent");return *result;
}
saves::PlotArrayObservation& team_test_array(saves::TeamObservation& team,std::string_view name){
    saves::PlotArrayObservation* result=nullptr;
    for(auto& row:team.arrays)if(row.name&&std::string_view(row.name)==name){need(!result,"Duplicate test team array");result=&row;}
    need(result&&result->columns==1&&result->values.size()==64,"Actual test team source64 array absent");return *result;
}
std::optional<std::int32_t> first_alive_test(const LiveSessionState& state){
    need(state.world.teams&&state.world.teams->teams.size()==64,"Actual test team roster absent");
    for(std::int32_t i=0;i<64;++i){const auto& team=state.world.teams->teams[static_cast<std::size_t>(i)];
        need(team.index==static_cast<std::uint64_t>(i)&&team_test_field(team,"id").value==i,"Actual test team ID mismatch");
        if(team_test_field(team,"alive_count").value>0)return i;
    }return std::nullopt;
}
void to_teams_frontier(const std::shared_ptr<BnwRetainedTurnCursor>& cursor,const ui::LiveUiHost& ui){
    to_deals_frontier(cursor,ui);cursor->execute_game_deals_prefix();
}
void actual_first_team_war(const std::filesystem::path& save,const std::filesystem::path& game,int controlled=-1){
    const auto host=cycle_host(save,game);
    auto client=ui::BnwClientFrameOwner::from_selected_host(host,test_detail::selected_city_frame_sources(host));
    auto ui=client->create_selected_ui_host(cycle_registry(),nullptr);
    auto cursor=BnwRetainedTurnCursor::bind(std::shared_ptr<const ui::SelectedBnwPreviewHost>(host));
    refuses([&]{cursor->execute_first_alive_team_war_prefix();});
    need(cursor->view().operation==0&&!cursor->view().first_team_war_attempted&&!cursor->view().first_team,"War prefix skipped actual predecessors");
    try{to_teams_frontier(cursor,*ui);}catch(const std::exception& e){
        need(controlled<0,"Synthetic counter fixture failed mandatory retained predecessor");
        need(!cursor->view().first_team_war_attempted,"Blocked predecessor consumed team operation");
        std::cout<<"Original selected team prefix unreached: "<<e.what()<<'\n';return;
    }
    const auto lease=host->session().read_state_lease();const auto revision=lease->ui_source_revision;
    const auto generation=host->session().state_generation();const auto first=first_alive_test(*lease);
    auto oracle=lease->world;bool changed=false;
    if(first&&*first!=63){
        auto& team=oracle.teams->teams[static_cast<std::size_t>(*first)];const auto at_war=team_test_array(team,"at_war").values;
        auto& war=team_test_array(team,"turns_at_war").values;auto& locked=team_test_array(team,"turns_locked_at_war").values;
        for(std::size_t i=0;i<63;++i){
            const auto old=war[i],old_lock=locked[i];
            if(at_war[i]){need(old<INT32_MAX,"Controlled source war arithmetic overflow");if(i!=static_cast<std::size_t>(*first)||old+1==0)war[i]=old+1;}
            else war[i]=0;
            if(old_lock>0&&(i!=static_cast<std::size_t>(*first)||old_lock-1==0))locked[i]=old_lock-1;
            changed=changed||war[i]!=old||locked[i]!=old_lock;
        }
    }
    const auto before=make_bnw_team_turn_checkpoint(lease->world,host->session().source().snapshot,host->identity());
    const auto* roster=lease->world.teams->teams.data();const auto frame=client->capture({1024,768,1024,768,96},{});
    const auto rng=lease->random.gameplay.state(),map_rng=lease->random.map.state();const auto clock=lease->setup.game_turn;
    const auto score_calls=cursor->score_dirty_setter_count(*ui),game_calls=cursor->game_data_dirty_setter_count(*ui);
    const auto score_flag=cursor->score_dirty(*ui),game_flag=cursor->game_data_dirty(*ui);
    const std::vector<std::int64_t>* war_object=nullptr;const std::vector<std::int64_t>* lock_object=nullptr;
    const std::int64_t* war_storage=nullptr;const std::int64_t* lock_storage=nullptr;
    if(first&&*first!=63){const auto& team=lease->world.teams->teams[static_cast<std::size_t>(*first)];for(const auto& row:team.arrays){
        if(row.name&&std::string_view(row.name)=="turns_at_war"){war_object=&row.values;war_storage=row.values.data();}
        if(row.name&&std::string_view(row.name)=="turns_locked_at_war"){lock_object=&row.values;lock_storage=row.values.data();}
    }}
    if(!first||*first==63){
        bool refused=false;std::string why;try{cursor->execute_first_alive_team_war_prefix();}catch(const std::exception& e){refused=true;why=e.what();}
        need(refused&&why.find(first?"DoBarbarianTech":"map-turn")!=std::string::npos&&cursor->view().operation==8&&
             !cursor->view().first_team_war_attempted&&!cursor->view().first_team&&
             cursor->view().frontier==BnwRetainedTurnFrontier::teams_turn,
             "Mandatory barbarian/no-team producer became an acknowledged skip");
        need(before.bytes==make_bnw_team_turn_checkpoint(lease->world,host->session().source().snapshot,host->identity()).bytes&&
             revision==lease->ui_source_revision&&client->is_current(frame),"Rejected team branch changed world or frames");
        std::cout<<"Original conditional team refusal: "<<why<<'\n';return;
    }
    if(controlled==0||controlled==1)need(changed,"Nondefault synthetic lock fixture failed to require an actual store");
    cursor->execute_first_alive_team_war_prefix();
    const auto expected=make_bnw_team_turn_checkpoint(oracle,host->session().source().snapshot,host->identity());
    need(expected.bytes==make_bnw_team_turn_checkpoint(lease->world,host->session().source().snapshot,host->identity()).bytes,
         "Actual first-team counter source values or other teams changed");
    need(cursor->view().operation==9&&cursor->view().first_team_war_attempted&&cursor->view().first_team==first&&
         cursor->view().frontier==BnwRetainedTurnFrontier::first_team_minor_technology,
         "War prefix missed source team identity or advanced through minor technology/team completion");
    need(lease->ui_source_revision==revision+(changed?1:0)&&client->is_current(frame)==!changed,
         "Actual counter change did not invalidate old frame or unchanged branch invalidated it");
    need(host->session().read_state_lease()==lease&&host->session().state_generation()==generation&&
         lease->world.teams->teams.data()==roster&&war_object->data()==war_storage&&lock_object->data()==lock_storage&&
         lease->random.gameplay.state()==rng&&lease->random.map.state()==map_rng&&lease->setup.game_turn==clock&&
         cursor->score_dirty_setter_count(*ui)==score_calls&&cursor->game_data_dirty_setter_count(*ui)==game_calls&&
         cursor->score_dirty(*ui)==score_flag&&cursor->game_data_dirty(*ui)==game_flag,
         "Counter prefix replaced actual arrays or changed RNG/clock/UI setter state");
    if(controlled>=0){const auto& team=lease->world.teams->teams[static_cast<std::size_t>(*first)];const auto self=static_cast<std::size_t>(*first);
        std::optional<std::int64_t> value;for(const auto& row:team.arrays)if(row.name&&std::string_view(row.name)=="turns_locked_at_war")value=row.values[self];
        need(value==(controlled==0?2:0),"Actual synthetic self locked nonzero refusal/allowed zero differs");
    }
    refuses([&]{cursor->execute_first_alive_team_war_prefix();});refuses([&]{(void)host->export_import_clock_checkpoint();});
    const auto fresh=client->capture({1024,768,1024,768,96},{});(void)host->session().bnw_game_debug_mode();
    need(client->is_current(fresh)&&host->session().has_bnw_operation_checkpoint_history()&&!host->session().readiness().can_continue(),
         "War prefix retired actual owner or granted durable complete turn");
    std::cout<<(controlled<0?"Original selected":"SYNTHETIC ID40+ID8")<<" actual first team "<<*first<<" counters changed="<<changed<<'\n';
    if(controlled==2&&changed)std::cout<<"Unchanged counter branch not covered by this actual at_war input; source relations were not fabricated\n";
}
void team_war_owner_guards(const std::filesystem::path& save,const std::filesystem::path& game);
void team_war_codec_cases(const std::filesystem::path& save,const std::filesystem::path& game){
    const auto baseline=cycle_host(save,game);const auto original=baseline->session().state();const auto first=first_alive_test(original);
    need(first&&*first!=63,"Selected source has no nonbarbarian actual first team for counter fixture");
    const auto make=[&](const LiveSessionState& state){return make_bnw_team_turn_checkpoint_pair(state,baseline->session().source().snapshot,
        original.technologies,original.player_research,baseline->technologies(),baseline->types(),baseline->identity());};
    for(int mode=0;mode<3;++mode){auto state=original;auto& team=state.world.teams->teams[static_cast<std::size_t>(*first)];const auto self=static_cast<std::size_t>(*first);
        auto& war=team_test_array(team,"turns_at_war").values;auto& lock=team_test_array(team,"turns_locked_at_war").values;
        need(team_test_array(team,"at_war").values[self]==0,"Synthetic self counter fixture requires actual peaceful self relation; codec cannot change at_war");
        if(mode<2){war[self]=7;lock[self]=mode==0?2:1;const auto opponent=(self+1)%63;war[opponent]=4;lock[opponent]=3;}
        else{for(std::size_t i=0;i<63;++i){war[i]=0;lock[i]=0;}} // unchanged branch conditional on actual no-war data
        state.world.history.deals.groups[1].clear(); // explicitly controlled ID39 INPUT, not a live deal-ending producer
        auto pair=make(state);
        {auto bad=state;team_test_array(bad.world.teams->teams[static_cast<std::size_t>(*first)],"turns_at_war").values[0]=std::int64_t(INT32_MAX)+1;refuses([&]{(void)make(bad);});}
        auto document=saves::make_civis_import_document(baseline->session().source().source,identify_file(save).sha256);
        const auto owned_id=static_cast<std::uint32_t>(saves::CivisSaveSectionId::live_team_research);
        std::erase_if(document.sections,[&](const auto& row){return row.id==bnw_team_turn_checkpoint_section_id||row.id==owned_id||row.id==bnw_game_deals_checkpoint_section_id;});
        document.sections.push_back(pair.raw);document.sections.push_back(pair.owned);
        document.sections.push_back(make_bnw_game_deals_checkpoint(state.world,baseline->session().source().snapshot,baseline->identity()));
        const auto path=std::filesystem::temp_directory_path()/("bnw209-war-codec-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(qpc())+".CivisSave");
        saves::write_civis_save(path,document);
        struct Cleanup{std::filesystem::path path;~Cleanup(){std::error_code error;std::filesystem::remove(path,error);}}cleanup{path};
        actual_first_team_war(path,game,mode); // NEW genuine selected Host/world/UI.
        if(mode==0)team_war_owner_guards(path,game); // Genuine positive codec input, not conditional retail guard skip.
    }
    // Exact codec boundary: ID40 does not carry alive_count or at_war. A
    // detached alteration cannot fabricate genuine barbarian-first authority.
    auto omitted=original;
    for(auto& team:omitted.world.teams->teams)for(auto& field:team.fields)
        if(field.name&&std::string_view(field.name)=="alive_count")field.value=team.index==63?1:0;
    const auto pair=make(omitted);const auto restored=restore_bnw_team_turn_checkpoint_pair(pair,baseline->session().source().snapshot,
        original.setup,original.technologies,original.player_research,baseline->technologies(),baseline->types(),baseline->identity());
    for(std::size_t i=0;i<64;++i)need(team_test_field(restored.world.teams->teams[static_cast<std::size_t>(i)],"alive_count").value==
        team_test_field(baseline->session().source().snapshot.teams->teams[static_cast<std::size_t>(i)],"alive_count").value,"Counter codec falsely restored unsupported alive_count mutation");
    std::cout<<"Barbarian-first/no-team actual fixture unavailable: existing counter codec does not carry alive_count; no fabricated coverage\n";
}
void team_war_owner_guards(const std::filesystem::path& save,const std::filesystem::path& game){
    for(int mode=0;mode<6;++mode){
        const auto host=cycle_host(save,game);auto client=ui::BnwClientFrameOwner::from_selected_host(host,test_detail::selected_city_frame_sources(host));
        auto ui=client->create_selected_ui_host(cycle_registry(),nullptr);auto cursor=BnwRetainedTurnCursor::bind(std::shared_ptr<const ui::SelectedBnwPreviewHost>(host));
        to_teams_frontier(cursor,*ui);
        const auto lease=host->session().read_state_lease();const auto revision=lease->ui_source_revision;
        const auto before=make_bnw_team_turn_checkpoint(lease->world,host->session().source().snapshot,host->identity());
        const auto invoke=[&]{cursor->execute_first_alive_team_war_prefix();};
        if(mode==0){const auto cp=host->session().checkpoint();host->session().restore(cp);refuses(invoke);}
        else if(mode==1){client.reset();refuses(invoke);}
        else if(mode==2){bool refused=false;std::thread worker([&]{try{invoke();}catch(const std::exception&){refused=true;}});worker.join();need(refused,"Foreign-thread counter producer accepted");}
        else if(mode==3){refuses([&]{const_cast<content::RulesDatabase&>(host->database()).apply_sql("UPDATE Defines SET Value=Value WHERE 0");});refuses(invoke);}
        else if(mode==4){(void)std::const_pointer_cast<BnwGameTurnTimerOwner>(cursor->timer_owner())->reset_for_current_player();refuses(invoke);}
        else{cursor->retire();refuses(invoke);}
        need(before.bytes==make_bnw_team_turn_checkpoint(lease->world,host->session().source().snapshot,host->identity()).bytes&&
             lease->ui_source_revision==revision&&cursor->view().operation==8&&!cursor->view().first_team_war_attempted&&!cursor->view().first_team,
             "Retired/foreign counter producer changed scalars/revision/attempt");
    }
    const auto host=cycle_host(save,game);auto generic=BnwRetainedTurnCursor::bind(clone(*host));(void)generic->execute_timer_prefix();
    refuses([&]{generic->execute_first_alive_team_war_prefix();});need(!generic->view().first_team_war_attempted,"Generic facade acquired actual counter producer");
}
bool actual_minor_test(const LiveSessionState& state,std::int32_t team){
    const auto& teams=state.world.pregame->archive.integer_arrays.at("team_types");
    const auto& flags=state.world.pregame->archive.flag_arrays.at("minor_nation_civs");
    for(std::size_t i=22;i<64;++i){const auto& player=state.world.all_players->players[i];
        std::optional<std::int64_t> alive;for(const auto& f:player.core.fields)if(f.name&&std::string_view(f.name)=="m_bAlive"){
            need(!alive&&f.values.size()==1,"Test actual player scalar duplicate/missing");alive=f.values[0];}
        need(alive&&(*alive==0||*alive==1),"Test actual alive scalar absent");
        if(*alive&&teams[i]==team)return flags[i];
    }return false;
}
void to_no_trade_frontier(const std::shared_ptr<BnwRetainedTurnCursor>& cursor,const ui::LiveUiHost& ui){
    to_teams_frontier(cursor,ui);cursor->execute_first_alive_team_war_prefix();
}
std::vector<std::int64_t> small_award_state(const LiveSessionState& state){
    std::vector<std::int64_t> values;
    for(const auto& team:state.world.teams->teams){values.push_back(static_cast<std::int64_t>(team.index));
        for(const auto& f:team.fields)if(f.name&&std::string_view(f.name)=="victory_points")values.push_back(f.value);
        for(const auto& a:team.hashes)if(a.name&&std::string_view(a.name)=="small_award_achieved"){
            values.push_back(a.wire_count);for(const auto& r:a.rows){values.push_back(static_cast<std::int64_t>(r.type_hash));
                values.push_back(static_cast<std::int64_t>(r.values.size()));values.insert(values.end(),r.values.begin(),r.values.end());}}}
    return values;
}
struct CircumnavigationExpected{
    std::optional<BnwRetainedCircumnavigationReturn> returned;
    std::optional<std::uint64_t> axis;
};
CircumnavigationExpected actual_circumnavigation_expected(const LiveSessionState& state,std::int32_t team){
    need(team>=0&&team<64,"Circumnavigation oracle team outside actual source roster");
    std::optional<bool> completed;
    for(const auto& f:state.world.game_state.flags)if(f.name&&std::string_view(f.name)=="circumnavigated"){
        need(!completed,"Duplicate actual global circumnavigation test flag");completed=f.value;}
    need(completed.has_value(),"Actual BNW circumnavigation test flag absent");
    if(*completed)return {BnwRetainedCircumnavigationReturn::already_circumnavigated,{}};
    const auto& map=state.world.map_header;
    if(!map.wrap_x&&!map.wrap_y)return {BnwRetainedCircumnavigationReturn::map_not_wrapped,{}};
    const auto total=map.grid.size();need(total<=INT32_MAX/2,"Actual circumnavigation oracle native arithmetic unavailable");
    if(map.land_plots>static_cast<std::int64_t>(total*2/3))return {BnwRetainedCircumnavigationReturn::land_density,{}};
    // Independent full axis coverage reduction from genuine decoded bits,
    // then choose the greatest missing axis. Production scans downward and
    // can return early; this oracle never supplies production permission.
    std::vector<std::uint64_t> coverage(static_cast<std::size_t>(map.wrap_x?map.grid.width:map.grid.height),0);
    for(const auto& plot:state.world.map_plots->plots){const saves::PlotArrayObservation* bits=nullptr;
        for(const auto& a:plot.arrays)if(a.name&&std::string_view(a.name)=="revealed_words"){need(!bits,"Duplicate actual reveal test array");bits=&a;}
        need(bits&&bits->values.size()==4&&bits->columns==1&&bits->values[static_cast<std::size_t>(team/32)]>=0&&bits->values[static_cast<std::size_t>(team/32)]<=UINT32_MAX,
             "Actual revealed source bits absent");
        const bool revealed=(static_cast<std::uint32_t>(bits->values[static_cast<std::size_t>(team/32)])&(std::uint32_t{1}<<(team%32)))!=0;
        if(revealed)++coverage.at(static_cast<std::size_t>(map.wrap_x?plot.x:plot.y));
    }
    CircumnavigationExpected result;
    for(std::size_t axis=0;axis<coverage.size();++axis)if(coverage[axis]==0){
        result.returned=map.wrap_x?BnwRetainedCircumnavigationReturn::unrevealed_x_column:BnwRetainedCircumnavigationReturn::unrevealed_y_row;
        result.axis=axis;}
    return result;
}
std::vector<std::int64_t> circumnavigation_raw_state(const LiveSessionState& state){
    std::vector<std::int64_t> result;
    for(const auto& flag:state.world.game_state.flags)result.push_back(flag.value);
    for(const auto& value:state.world.game_state.controls)result.push_back(value.value);
    result.insert(result.end(),{state.world.map_header.land_plots,state.world.map_header.wrap_x,state.world.map_header.wrap_y});
    for(const auto& plot:state.world.map_plots->plots)for(const auto& a:plot.arrays)if(a.name&&std::string_view(a.name)=="revealed_words")
        result.insert(result.end(),a.values.begin(),a.values.end());
    for(const auto& cell:state.world.map_projection->cells){need(cell.revealed_teams.has_value(),"Actual owned reveal set absent");
        result.push_back(static_cast<std::int64_t>(cell.revealed_teams->size()));result.insert(result.end(),cell.revealed_teams->begin(),cell.revealed_teams->end());}
    for(const auto& team:state.world.teams->teams)for(const auto& a:team.arrays)if(a.name&&std::string_view(a.name)=="extra_moves")
        result.insert(result.end(),a.values.begin(),a.values.end());
    return result;
}
const saves::PlotArrayObservation& next_team_test_array(const saves::TeamObservation& team,std::string_view name){
    const saves::PlotArrayObservation* result=nullptr;
    for(const auto& row:team.arrays)if(row.name&&std::string_view(row.name)==name){need(!result,"Duplicate actual next-team test array");result=&row;}
    need(result&&result->columns==1&&result->values.size()==64,"Actual next-team test source64 array absent");return *result;
}
void exercise_actual_next_team_war(const std::shared_ptr<ui::SelectedBnwPreviewHost>& host,
    const std::unique_ptr<ui::BnwClientFrameOwner>& client,const ui::LiveUiHost& ui,
    const std::shared_ptr<BnwRetainedTurnCursor>& cursor,int controlled=-1){
    const auto prior=cursor->view();need(prior.operation==13&&prior.first_team_return_attempted,"Next-team test lacks actual return");
    const auto lease=host->session().read_state_lease();const auto revision=lease->ui_source_revision,generation=host->session().state_generation();
    const auto before=make_bnw_team_turn_checkpoint(lease->world,host->session().source().snapshot,host->identity());
    const auto raw=circumnavigation_raw_state(*lease),awards=small_award_state(*lease);
    const auto rng=lease->random.gameplay.state(),maprng=lease->random.map.state();const auto turn=lease->setup.game_turn;
    const auto frame=client->capture({1024,768,1024,768,96},{});
    const auto score_calls=cursor->score_dirty_setter_count(ui),game_calls=cursor->game_data_dirty_setter_count(ui);
    auto oracle=lease->world;bool changed=false,overflow=false;
    std::optional<std::int32_t> selected;
    for(const auto& team:lease->world.teams->teams){const auto id=team_test_field(team,"id").value;
        if(id>*prior.first_team&&team_test_field(team,"alive_count").value>0&&(!selected||id<*selected))selected=static_cast<std::int32_t>(id);}
    need(selected==prior.next_alive_team&&prior.team_loop_next_slot==selected.value_or(64),"Next-team independent minimum-alive selection differs");
    const std::int64_t* war_storage=nullptr;const std::int64_t* locked_storage=nullptr;
    if(selected&&*selected!=63){const auto id=static_cast<std::size_t>(*selected);auto& team=oracle.teams->teams[id];
        const auto relation=team_test_array(team,"at_war").values;
        auto& war=team_test_array(team,"turns_at_war").values;auto& locked=team_test_array(team,"turns_locked_at_war").values;
        const auto& actual=lease->world.teams->teams[id];
        war_storage=next_team_test_array(actual,"turns_at_war").values.data();locked_storage=next_team_test_array(actual,"turns_locked_at_war").values.data();
        for(std::size_t target=0;target<63;++target){const auto old=war[target],oldlock=locked[target];
            if(relation[target]&&old==INT32_MAX){overflow=true;break;}
            const auto candidate=relation[target]?old+1:0;
            if(target!=id||candidate==0)war[target]=candidate;
            if(oldlock>0&&(target!=id||oldlock-1==0))locked[target]=oldlock-1;
            changed=changed||old!=war[target]||oldlock!=locked[target];
        }
    }
    bool refused=false;std::string why;try{cursor->execute_next_alive_team_war_prefix();}catch(const std::exception& e){refused=true;why=e.what();}
    const auto after=cursor->view();
    if(!selected||*selected==63||overflow){need(refused&&after.operation==13&&!after.next_team_war_attempted&&!after.next_team&&
        after.frontier==prior.frontier&&before.bytes==make_bnw_team_turn_checkpoint(lease->world,host->session().source().snapshot,host->identity()).bytes&&
        lease->ui_source_revision==revision&&client->is_current(frame),"Unsupported/overflow next-team branch consumed attempt or partially changed world");
        if(selected&&*selected==63)need(why.find("mandatory DoBarbarianTech BEFORE operation14")!=std::string::npos,"Barbarian next-team became war skip");
        if(overflow)need(why.find("source-i32 overflow BEFORE self setter guard")!=std::string::npos,"Overflow next-team refusal lost actual arithmetic order");
    }else{need(!refused&&after.operation==14&&after.next_team_war_attempted&&after.next_team==selected&&
        after.frontier==BnwRetainedTurnFrontier::next_team_minor_technology&&after.first_team==prior.first_team&&
        after.team_loop_next_slot==prior.team_loop_next_slot&&after.next_alive_team==prior.next_alive_team,
        "Actual next-team war missed pending source identity or passed mandatory minor technology");
        need(make_bnw_team_turn_checkpoint(oracle,host->session().source().snapshot,host->identity()).bytes==
             make_bnw_team_turn_checkpoint(lease->world,host->session().source().snapshot,host->identity()).bytes,
             "Next-team scalar stores differ from independent source oracle or changed another team");
        need(lease->ui_source_revision==revision+(changed?1:0)&&client->is_current(frame)==!changed,"Next-team revision/frame invalidation differs");
        const auto& actual=lease->world.teams->teams[static_cast<std::size_t>(*selected)];
        need(next_team_test_array(actual,"turns_at_war").values.data()==war_storage&&next_team_test_array(actual,"turns_locked_at_war").values.data()==locked_storage,
             "Next-team scalar array storage replaced");
        if(controlled==0||controlled==1)need(changed&&next_team_test_array(actual,"turns_locked_at_war").values[static_cast<std::size_t>(*selected)]==(controlled==0?2:0),
             "Controlled next-team self setter nonzero suppression/zero permission differs");
        refuses([&]{cursor->execute_next_alive_team_war_prefix();});
    }
    need(host->session().read_state_lease()==lease&&host->session().state_generation()==generation&&
         raw==circumnavigation_raw_state(*lease)&&awards==small_award_state(*lease)&&rng==lease->random.gameplay.state()&&
         maprng==lease->random.map.state()&&turn==lease->setup.game_turn&&score_calls==cursor->score_dirty_setter_count(ui)&&
         game_calls==cursor->game_data_dirty_setter_count(ui),"Next-team war changed map/awards/RNG/clocks/UI setters or world allocation");
    need(BnwRetainedTurnCursor::bind(std::shared_ptr<const ui::SelectedBnwPreviewHost>(host))==cursor,"Next-team phase reconstructed sticky cursor");
    refuses([&]{(void)host->export_import_clock_checkpoint();});need(!host->session().readiness().can_continue(),"Next-team war granted complete team/map/game continuation");
    std::cout<<"Actual next-team war selected="<<selected.value_or(-1)<<" refused="<<refused<<" counters_changed="<<(!refused&&changed)<<" controlled="<<controlled<<'\n';
    if(controlled==2&&changed)std::cout<<"Next-team no-effect case unavailable under actual at_war input; source relation not fabricated\n";
}
void exercise_actual_first_team_return(const std::shared_ptr<ui::SelectedBnwPreviewHost>& host,
    const std::unique_ptr<ui::BnwClientFrameOwner>& client,const ui::LiveUiHost& ui,
    const std::shared_ptr<BnwRetainedTurnCursor>& cursor){
    const auto prior=cursor->view();need(prior.operation==12&&prior.first_team,"Team-return test lacks actual operation12");
    const auto lease=host->session().read_state_lease();const auto revision=lease->ui_source_revision,generation=host->session().state_generation();
    const auto before=make_bnw_team_turn_checkpoint(lease->world,host->session().source().snapshot,host->identity());
    const auto raw=circumnavigation_raw_state(*lease),awards=small_award_state(*lease);
    const auto teams=lease->world.pregame->archive.integer_arrays.at("team_types");
    const auto rng=lease->random.gameplay.state(),maprng=lease->random.map.state();const auto turn=lease->setup.game_turn;
    const auto frame=client->capture({1024,768,1024,768,96},{});
    const auto score_calls=cursor->score_dirty_setter_count(ui),game_calls=cursor->game_data_dirty_setter_count(ui);
    // Independent expected selection via full roster: select minimum positive
    // alive ID greater than first, rather than using the production scan result.
    std::optional<std::int32_t> next;
    for(const auto& team:lease->world.teams->teams){const auto id=team_test_field(team,"id").value;
        need(id>=0&&id<64,"Actual test next-team ID outside native roster");
        if(id>*prior.first_team&&team_test_field(team,"alive_count").value>0&&(!next||id<*next))next=static_cast<std::int32_t>(id);}
    // Independent predicate oracle: collect all leader candidates and choose
    // minimum alive ID, otherwise minimum member; aggregate reached alliances.
    const auto core_value=[](const saves::SavedPlayerObservation& value,std::string_view key){
        const saves::PlayerCoreField* found=nullptr;
        for(const auto& row:value.core.fields)if(row.name&&std::string_view(row.name)==key){need(!found,"Duplicate team-return oracle scalar");found=&row;}
        need(found&&found->values.size()==1,"Actual team-return oracle scalar absent");return found->values[0];};
    std::optional<std::int64_t> living,member;
    for(const auto& value:lease->world.all_players->players){const auto id=core_value(value,"m_eID");
        need(id>=0&&id<64,"Actual team-return oracle player outside roster");
        if(teams.at(static_cast<std::size_t>(id))!=*prior.first_team)continue;
        if(!member||id<*member)member=id;
        const auto alive=core_value(value,"m_bAlive");need(alive==0||alive==1,"Actual oracle alive flag not boolean");
        if(alive&&(!living||id<*living))living=id;}
    const auto leader=living.value_or(member.value_or(-1));
    const saves::PlotArrayObservation* war=nullptr;
    for(const auto& a:lease->world.teams->teams.at(static_cast<std::size_t>(*prior.first_team)).arrays)
        if(a.name&&std::string_view(a.name)=="at_war"){need(!war,"Duplicate oracle war array");war=&a;}
    need(war&&war->values.size()==64,"Actual oracle war array missing");
    bool admissible=true;std::uint64_t assertions=0;
    for(std::size_t slot=22;slot<63;++slot){const auto target=teams.at(slot);
        if(target<0){admissible=false;break;}need(target<64,"Actual oracle team outside roster");
        if(!war->values.at(static_cast<std::size_t>(target)))continue;
        const auto& value=lease->world.all_players->players.at(slot);const saves::PlayerPlanningRecord* minor=nullptr;
        for(const auto& block:value.planning.blocks)if(block.name&&std::string_view(block.name)=="minor_civ_ai"){
            need(!minor,"Duplicate actual MinorCivAI oracle block");minor=&block;}
        need(minor,"Actual MinorCivAI oracle block absent");const saves::NamedSaveInteger* ally=nullptr;
        for(const auto& f:minor->fields)if(f.name&&std::string_view(f.name)=="m_eAlly"){need(!ally,"Duplicate actual oracle ally");ally=&f;}
        need(ally,"Actual oracle Ally absent");if(ally->value==leader){admissible=false;break;}++assertions;}
    bool refused=false;std::string why;
    try{cursor->execute_first_team_return_and_advance_prefix();}catch(const std::exception& error){refused=true;why=error.what();}
    const auto after=cursor->view();
    if(refused){need(!admissible&&why.find("BEFORE operation13")!=std::string::npos&&after.operation==12&&!after.first_team_return_attempted&&
         !after.first_team_tail_assertions&&!after.team_loop_next_slot&&!after.next_alive_team&&
         after.frontier==BnwRetainedTurnFrontier::first_team_post_circumnavigation,
         "Unproved team-return diagnostic branch consumed phase or became success");}
    else{need(admissible&&after.operation==13&&after.first_team_return_attempted&&after.first_team_tail_assertions==assertions&&
         after.team_loop_next_slot==next.value_or(64)&&after.next_alive_team==next&&
         after.frontier==(next?BnwRetainedTurnFrontier::remaining_teams_turn:BnwRetainedTurnFrontier::map_turn_unavailable),
         "Actual team-return did not retain exact next source loop slot");
         refuses([&]{cursor->execute_first_team_return_and_advance_prefix();});
         need(BnwRetainedTurnCursor::bind(std::shared_ptr<const ui::SelectedBnwPreviewHost>(host))==cursor,"Team return rebuilt sticky owner");}
    need(host->session().read_state_lease()==lease&&host->session().state_generation()==generation&&lease->ui_source_revision==revision&&
         before.bytes==make_bnw_team_turn_checkpoint(lease->world,host->session().source().snapshot,host->identity()).bytes&&
         raw==circumnavigation_raw_state(*lease)&&awards==small_award_state(*lease)&&
         teams==lease->world.pregame->archive.integer_arrays.at("team_types")&&client->is_current(frame)&&
         rng==lease->random.gameplay.state()&&maprng==lease->random.map.state()&&turn==lease->setup.game_turn&&
         score_calls==cursor->score_dirty_setter_count(ui)&&game_calls==cursor->game_data_dirty_setter_count(ui),
         "Team-return/no-effect scan changed world/rules/frame/RNG/turn/UI setters");
    refuses([&]{(void)host->export_import_clock_checkpoint();});
    need(!host->session().readiness().can_continue(),"Team return granted all-team/map/player/full-turn readiness");
    std::cout<<"Actual first-team return refused="<<refused<<" next="<<after.team_loop_next_slot.value_or(-1)<<" no full-turn/native parity claim\n";
    if(!refused)exercise_actual_next_team_war(host,client,ui,cursor);
}
void exercise_actual_circumnavigation(const std::shared_ptr<ui::SelectedBnwPreviewHost>& host,
    const std::unique_ptr<ui::BnwClientFrameOwner>& client,const ui::LiveUiHost& ui,
    const std::shared_ptr<BnwRetainedTurnCursor>& cursor){
    const auto view=cursor->view();need(view.operation==11&&view.first_team,"Circumnavigation test lacks actual predecessor");
    const auto lease=host->session().read_state_lease();const auto revision=lease->ui_source_revision,generation=host->session().state_generation();
    const auto expected=actual_circumnavigation_expected(*lease,*view.first_team);const auto raw=circumnavigation_raw_state(*lease);
    const auto awards=small_award_state(*lease);const auto team_bytes=make_bnw_team_turn_checkpoint(lease->world,host->session().source().snapshot,host->identity());
    const auto* plots=lease->world.map_plots->plots.data();const auto* projection=lease->world.map_projection->cells.data();
    const auto rng=lease->random.gameplay.state(),maprng=lease->random.map.state();const auto turn=lease->setup.game_turn;
    const auto score_calls=cursor->score_dirty_setter_count(ui),game_calls=cursor->game_data_dirty_setter_count(ui);
    const auto score_flag=cursor->score_dirty(ui),game_flag=cursor->game_data_dirty(ui);const auto frame=client->capture({1024,768,1024,768,96},{});
    if(expected.returned){cursor->execute_first_team_circumnavigation_prefix();const auto after=cursor->view();
        need(after.operation==12&&after.first_team_circumnavigation_attempted&&after.first_team_circumnavigation_return==expected.returned&&
             after.first_team_circumnavigation_axis==expected.axis&&after.frontier==BnwRetainedTurnFrontier::first_team_post_circumnavigation,
             "Actual circumnavigation source early-return reason/descending axis or frontier differs");
        refuses([&]{cursor->execute_first_team_circumnavigation_prefix();});
        need(BnwRetainedTurnCursor::bind(std::shared_ptr<const ui::SelectedBnwPreviewHost>(host))==cursor,"Completed actual circumnavigation replay got another owner");
    }else{bool refused=false;std::string why;try{cursor->execute_first_team_circumnavigation_prefix();}catch(const std::exception& e){refused=true;why=e.what();}
        const auto after=cursor->view();need(refused&&why.find("mandatory makeCircumnavigated/moves/message/Lua/replay/checkpoint owners")!=std::string::npos&&
             after.operation==11&&!after.first_team_circumnavigation_attempted&&!after.first_team_circumnavigation_return&&!after.first_team_circumnavigation_axis&&
             after.frontier==BnwRetainedTurnFrontier::first_team_circumnavigation,
             "Reached circumnavigation became successful skip/store or consumed operation12");}
    need(host->session().read_state_lease()==lease&&host->session().state_generation()==generation&&
         lease->world.map_plots->plots.data()==plots&&lease->world.map_projection->cells.data()==projection&&
         raw==circumnavigation_raw_state(*lease)&&awards==small_award_state(*lease)&&
         team_bytes.bytes==make_bnw_team_turn_checkpoint(lease->world,host->session().source().snapshot,host->identity()).bytes&&
         lease->ui_source_revision==revision&&client->is_current(frame)&&lease->random.gameplay.state()==rng&&lease->random.map.state()==maprng&&
         lease->setup.game_turn==turn&&cursor->score_dirty_setter_count(ui)==score_calls&&cursor->game_data_dirty_setter_count(ui)==game_calls&&
         cursor->score_dirty(ui)==score_flag&&cursor->game_data_dirty(ui)==game_flag,
         "Actual circumnavigation changed global flag/winner/map/reveal/moves/awards/RNG/clock/UI/frame");
    refuses([&]{(void)host->export_import_clock_checkpoint();});need(!host->session().readiness().can_continue(),"Circumnavigation granted full-team/full-turn continuation");
    if(expected.returned)exercise_actual_first_team_return(host,client,ui,cursor);
    std::cout<<"Actual selected circumnavigation early-return="<<expected.returned.has_value()<<" axis="<<expected.axis.value_or(UINT64_MAX)<<" Original parity not inferred\n";
}
bool to_next_team_war_frontier(const std::shared_ptr<ui::SelectedBnwPreviewHost>& host,
    const std::shared_ptr<BnwRetainedTurnCursor>& cursor,const ui::LiveUiHost& ui){
    to_no_trade_frontier(cursor,ui);cursor->execute_first_team_nonminor_no_trade_prefix();cursor->execute_first_team_small_awards_prefix();
    const auto map=actual_circumnavigation_expected(host->session().state(),*cursor->view().first_team);
    if(!map.returned){std::cout<<"Next-team op14 genuine predecessor unavailable: actual circumnavigation effects required\n";return false;}
    cursor->execute_first_team_circumnavigation_prefix();
    try{cursor->execute_first_team_return_and_advance_prefix();}catch(const std::exception& e){
        need(cursor->view().operation==12&&!cursor->view().first_team_return_attempted,"Unsupported op13 branch consumed predecessor");
        std::cout<<"Next-team op14 genuine predecessor unavailable: "<<e.what()<<'\n';return false;}
    return true;
}
void next_team_war_owner_guards(const std::filesystem::path& save,const std::filesystem::path& game){
    for(int mode=0;mode<6;++mode){const auto host=cycle_host(save,game);auto client=ui::BnwClientFrameOwner::from_selected_host(host,test_detail::selected_city_frame_sources(host));
        auto ui=client->create_selected_ui_host(cycle_registry(),nullptr);auto cursor=BnwRetainedTurnCursor::bind(std::shared_ptr<const ui::SelectedBnwPreviewHost>(host));
        refuses([&]{cursor->execute_next_alive_team_war_prefix();});need(!cursor->view().next_team_war_attempted,"Next-team war skipped actual predecessors");
        if(!to_next_team_war_frontier(host,cursor,*ui))break;
        const auto lease=host->session().read_state_lease();const auto revision=lease->ui_source_revision;
        const auto before=make_bnw_team_turn_checkpoint(lease->world,host->session().source().snapshot,host->identity());
        const auto invoke=[&]{cursor->execute_next_alive_team_war_prefix();};
        if(mode==0){const auto cp=host->session().checkpoint();host->session().restore(cp);refuses(invoke);}
        else if(mode==1){client.reset();refuses(invoke);}
        else if(mode==2){bool refused=false;std::thread worker([&]{try{invoke();}catch(const std::exception&){refused=true;}});worker.join();need(refused,"Foreign next-team war accepted");}
        else if(mode==3){refuses([&]{const_cast<content::RulesDatabase&>(host->database()).apply_sql("UPDATE Defines SET Value=Value WHERE 0");});refuses(invoke);}
        else if(mode==4){(void)std::const_pointer_cast<BnwGameTurnTimerOwner>(cursor->timer_owner())->reset_for_current_player();refuses(invoke);}
        else{cursor->retire();refuses(invoke);}
        need(cursor->view().operation==13&&!cursor->view().next_team_war_attempted&&!cursor->view().next_team&&lease->ui_source_revision==revision&&
             before.bytes==make_bnw_team_turn_checkpoint(lease->world,host->session().source().snapshot,host->identity()).bytes,
             "Invalid next-team owner partially changed world/revision or consumed phase");
    }
    const auto host=cycle_host(save,game);auto generic=BnwRetainedTurnCursor::bind(clone(*host));(void)generic->execute_timer_prefix();
    refuses([&]{generic->execute_next_alive_team_war_prefix();});need(!generic->view().next_team_war_attempted,"Generic facade acquired next-team war capability");
}
void next_team_war_codec_cases(const std::filesystem::path& save,const std::filesystem::path& game){
    const auto baseline=cycle_host(save,game);const auto original=baseline->session().state();const auto first=first_alive_test(original);
    need(first.has_value(),"Controlled next-team fixture requires actual alive team");std::optional<std::int32_t> next;
    for(const auto& team:original.world.teams->teams){const auto id=team_test_field(team,"id").value;
        if(id>*first&&team_test_field(team,"alive_count").value>0&&(!next||id<*next))next=static_cast<std::int32_t>(id);}
    if(!next||*next==63){std::cout<<"Controlled next-team nonbarbarian case unavailable under actual alive roster\n";return;}
    const auto next_id=static_cast<std::size_t>(*next);const auto self=next_id;
    auto copy=original.world.teams->teams[next_id];const auto relation=team_test_array(copy,"at_war").values;
    std::optional<std::size_t> atwar;for(std::size_t target=0;target<63;++target)if(relation[target]){atwar=target;break;}
    for(int mode=0;mode<4;++mode){if(mode==3&&!atwar){std::cout<<"Genuine op14 overflow branch unavailable: no actual at_war edge; codec cannot fabricate relations\n";continue;}
        auto state=original;auto& team=state.world.teams->teams[next_id];auto& war=team_test_array(team,"turns_at_war").values;
        auto& locked=team_test_array(team,"turns_locked_at_war").values;
        if(mode<2){need(!relation[self],"Controlled next-team self fixture requires actual peaceful self");war[self]=7;locked[self]=mode==0?2:1;
            const auto opponent=(self+1)%63;war[opponent]=4;locked[opponent]=3;}
        else if(mode==2){for(std::size_t target=0;target<63;++target){war[target]=0;locked[target]=0;}}
        else war[*atwar]=INT32_MAX;
        state.world.history.deals.groups[1].clear(); // Controlled persisted INPUT, not a live ending effect.
        const auto make=[&](const LiveSessionState& value){return make_bnw_team_turn_checkpoint_pair(value,baseline->session().source().snapshot,
            original.technologies,original.player_research,baseline->technologies(),baseline->types(),baseline->identity());};
        auto malformed=state;team_test_array(malformed.world.teams->teams[next_id],"turns_at_war").values[0]=std::int64_t(INT32_MAX)+1;
        refuses([&]{(void)make(malformed);}); // Codec refuses invalid source width before genuine import.
        const auto pair=make(state);auto document=saves::make_civis_import_document(baseline->session().source().source,identify_file(save).sha256);
        const auto owned_id=static_cast<std::uint32_t>(saves::CivisSaveSectionId::live_team_research);
        std::erase_if(document.sections,[&](const auto& row){return row.id==bnw_team_turn_checkpoint_section_id||row.id==owned_id||row.id==bnw_game_deals_checkpoint_section_id;});
        document.sections.push_back(pair.raw);document.sections.push_back(pair.owned);
        document.sections.push_back(make_bnw_game_deals_checkpoint(state.world,baseline->session().source().snapshot,baseline->identity()));
        const auto path=std::filesystem::temp_directory_path()/("bnw214-next-war-codec-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(qpc())+".CivisSave");
        saves::write_civis_save(path,document);
        struct Cleanup{std::filesystem::path path;~Cleanup(){std::error_code error;std::filesystem::remove(path,error);}}cleanup{path};
        const auto host=cycle_host(path,game);auto client=ui::BnwClientFrameOwner::from_selected_host(host,test_detail::selected_city_frame_sources(host));
        auto ui=client->create_selected_ui_host(cycle_registry(),nullptr);auto cursor=BnwRetainedTurnCursor::bind(std::shared_ptr<const ui::SelectedBnwPreviewHost>(host));
        if(to_next_team_war_frontier(host,cursor,*ui))exercise_actual_next_team_war(host,client,*ui,cursor,mode);
        if(mode==0)next_team_war_owner_guards(path,game);
    }
}
// AUTHORED UNRUN. Called only at a naturally reached grant on the imported
// source. No alive/Pregame/HasTech fixture mutation creates this branch.
void check_natural_minor_due_plan(const BnwRetainedMinorTechnologyPlan& plan,
    const LiveSessionState& state,const ui::SelectedBnwPreviewHost& host){
    need(plan.team>=0&&plan.team<63&&plan.technology>=0&&
         static_cast<std::uint64_t>(plan.technology)<host.technologies().collection.slots&&
         !state.technologies[static_cast<std::size_t>(plan.team)].has_tech(plan.technology),
         "Natural due plan has no actual missing source technology");
    std::vector<std::int32_t> majors;
    for(std::int32_t team=0;team<63;++team)
        if(team_test_field(state.world.teams->teams[static_cast<std::size_t>(team)],"alive_count").value>0&&!actual_minor_test(state,team))majors.push_back(team);
    const auto* percent=host.defines().find("MINOR_CIV_TECH_PERCENT");
    need(percent&&std::holds_alternative<std::int64_t>(percent->lookup.value),"Natural due oracle actual percent absent");
    const auto threshold=std::max<std::int64_t>(1,static_cast<std::int64_t>(majors.size())*std::get<std::int64_t>(percent->lookup.value)/100);
    const auto owners=[&](std::int64_t tech){return std::count_if(majors.begin(),majors.end(),[&](std::int32_t team){return state.technologies[static_cast<std::size_t>(team)].has_tech(tech);});};
    need(plan.alive_major_teams==static_cast<std::int32_t>(majors.size())&&plan.teams_needed==threshold&&
         plan.major_owners==owners(plan.technology)&&plan.major_owners>=threshold,
         "Natural due plan changed native major count/threshold/owner condition");
    for(std::int64_t tech=0;tech<plan.technology;++tech)
        need(state.technologies[static_cast<std::size_t>(plan.team)].has_tech(tech)||owners(tech)<threshold,
             "Natural due plan skipped an earlier required source technology");
    const auto& team_types=state.world.pregame->archive.integer_arrays.at("team_types");
    std::vector<std::int32_t> members,living;
    for(std::size_t slot=0;slot<64;++slot){
        if(team_types[slot]!=plan.team)continue;
        members.push_back(static_cast<std::int32_t>(slot));
        const saves::PlayerCoreField* alive=nullptr;
        for(const auto& row:state.world.all_players->players[slot].core.fields)
            if(row.name&&std::string_view(row.name)=="m_bAlive"){need(!alive,"Natural leader oracle duplicate alive");alive=&row;}
        need(alive&&alive->values.size()==1,"Natural leader oracle actual alive absent");
        if(alive->values[0])living.push_back(static_cast<std::int32_t>(slot));
    }
    const auto expected=!living.empty()?*std::min_element(living.begin(),living.end()):
                        (!members.empty()?*std::min_element(members.begin(),members.end()):-1);
    need(plan.leader==expected,"Natural due plan did not use original alive-first Pregame leader order");
}
void team_queue_codec_cases(const std::filesystem::path& save,const std::filesystem::path& game){
    std::size_t observed_minor_due_plans=0;
    const auto baseline=cycle_host(save,game);const auto original=baseline->session().state();
    const auto first=first_alive_test(original);need(first.has_value(),"Queue fixture requires actual living source team");
    std::vector<std::int32_t> ascending;std::optional<std::int32_t> minor_team;bool actual_gap=false;
    for(std::int32_t slot=*first+1;slot<64;++slot){const auto& team=original.world.teams->teams[static_cast<std::size_t>(slot)];
        if(team_test_field(team,"alive_count").value<=0){actual_gap=true;continue;}
        ascending.push_back(slot);if(slot!=63&&actual_minor_test(original,slot)&&!minor_team)minor_team=slot;}
    std::cout<<"Actual queue source alive successors="<<ascending.size()<<" actual dead-slot gap="<<actual_gap<<'\n';
    const auto make=[&](const LiveSessionState& state){return make_bnw_team_turn_checkpoint_pair(state,baseline->session().source().snapshot,
        original.technologies,original.player_research,baseline->technologies(),baseline->types(),baseline->identity());};
    for(int mode=-1;mode<2;++mode){
        if(mode==1&&!minor_team){std::cout<<"Actual minor no-acquisition owner fixture unavailable: source has no alive minor successor\n";continue;}
        auto input=original;
        if(mode>=0){
            input.world.history.deals.groups[1].clear(); // Persisted INPUT, not live deal completion authority.
            for(const auto id:ascending){if(id==63)continue;auto& team=input.world.teams->teams[static_cast<std::size_t>(id)];
                std::vector<std::int64_t>* flags=nullptr;const std::vector<std::int64_t>* hashes=nullptr;
                for(auto& row:team.arrays){if(row.name&&std::string_view(row.name)=="no_trade_tech")flags=&row.values;
                    if(row.name&&std::string_view(row.name)=="tech_hashes")hashes=&row.values;}
                need(flags&&hashes&&flags->size()==hashes->size(),"Queue fixture actual raw technology span absent");
                for(std::size_t row=0;row<hashes->size();++row){const auto hash=(*hashes)[row];
                    need(hash>=0&&hash<=UINT32_MAX,"Queue fixture technology hash outside source width");
                    if(!hash){(*flags)[row]=0;continue;}
                    const auto* type=baseline->types().find_hash(static_cast<std::uint64_t>(hash));
                    need(type&&type->table=="Technologies"&&type->id>=0&&type->id<=INT32_MAX,"Queue fixture actual technology mapping absent");
                    (*flags)[row]=1;input.technologies[static_cast<std::size_t>(id)].set_no_trade(type->id,true);
                }
            }
            if(mode==1){
                // A detached changed-HasTech INPUT is deliberately rejected by
                // the paired codec; no positive import/acquisition is claimed.
                const auto slots=baseline->technologies().collection.slots;need(slots<=INT32_MAX,"Minor input source tech count width");
                auto& owned=input.technologies[static_cast<std::size_t>(*minor_team)];
                bool missing=false;
                for(std::uint64_t slot=0;slot<slots;++slot)if(!owned.has_tech(static_cast<std::int64_t>(slot))){missing=true;break;}
                if(!missing){
                    std::cout<<"Negative changed-HasTech fixture unavailable: actual minor already owns every technology; no-op alltrue is not malformed\n";
                    continue;
                }
                for(std::uint64_t slot=0;slot<slots;++slot)owned.set_has_tech(static_cast<std::int64_t>(slot),true);
                // ID40+8 owns no-trade/counters/points/awards, not native HasTech
                // acquisition. This forged detached difference MUST fail pairing.
                refuses([&]{(void)make(input);});
                std::cout<<"Alltrue minor detached HasTech fixture refused by actual paired codec; positive no-acquisition remains natural-source conditional\n";
                continue;
            }
        }
        auto path=save;std::optional<std::filesystem::path> temporary;
        if(mode>=0){const auto pair=make(input);auto document=saves::make_civis_import_document(baseline->session().source().source,identify_file(save).sha256);
            const auto owned_id=static_cast<std::uint32_t>(saves::CivisSaveSectionId::live_team_research);
            std::erase_if(document.sections,[&](const auto& row){return row.id==bnw_team_turn_checkpoint_section_id||row.id==owned_id||row.id==bnw_game_deals_checkpoint_section_id;});
            document.sections.push_back(pair.raw);document.sections.push_back(pair.owned);
            document.sections.push_back(make_bnw_game_deals_checkpoint(input.world,baseline->session().source().snapshot,baseline->identity()));
            path=std::filesystem::temp_directory_path()/("bnw215-queue-codec-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(qpc())+".CivisSave");
            temporary=path;saves::write_civis_save(path,document);
        }
        struct Cleanup{std::optional<std::filesystem::path> path;~Cleanup(){if(path){std::error_code error;std::filesystem::remove(*path,error);}}}cleanup{temporary};
        const auto host=cycle_host(path,game);auto client=ui::BnwClientFrameOwner::from_selected_host(host,test_detail::selected_city_frame_sources(host));
        auto ui=client->create_selected_ui_host(cycle_registry(),nullptr);auto cursor=BnwRetainedTurnCursor::bind(std::shared_ptr<const ui::SelectedBnwPreviewHost>(host));
        refuses([&]{(void)cursor->pending_minor_technology_plan();});
        refuses([&]{cursor->execute_pending_team_no_trade_prefix();});refuses([&]{cursor->execute_pending_team_small_awards_prefix();});
        refuses([&]{cursor->execute_pending_team_circumnavigation_prefix();});refuses([&]{cursor->execute_pending_team_return_and_advance_prefix();});
        need(cursor->view().operation==0&&!cursor->view().active_team,"Queue typed steps skipped actual predecessors");
        try{if(!to_next_team_war_frontier(host,cursor,*ui)){std::cout<<"Actual queue source predecessor unavailable, mode="<<mode<<'\n';continue;}}
        catch(const std::exception& e){std::cout<<"Actual queue source predecessor unavailable, mode="<<mode<<": "<<e.what()<<'\n';continue;}
        const auto lease=host->session().read_state_lease();auto oracle=*lease;
        const auto revision=lease->ui_source_revision,generation=host->session().state_generation();
        const auto rng=lease->random.gameplay.state(),maprng=lease->random.map.state();const auto turn=lease->setup.game_turn;
        const auto map=circumnavigation_raw_state(*lease),awards=small_award_state(*lease);
        const auto frame=client->capture({1024,768,1024,768,96},{});
        const auto score_calls=cursor->score_dirty_setter_count(*ui),game_calls=cursor->game_data_dirty_setter_count(*ui);
        const auto diagnostic=cursor->execute_pending_teams_until_boundary();const auto after=cursor->view();
        need(diagnostic.before.operation==13&&diagnostic.after.operation==after.operation&&diagnostic.after.frontier==after.frontier&&
             diagnostic.returned_operations==after.operation-13&&diagnostic.stop!=BnwRetainedTeamsStop::quarantined&&
             !diagnostic.blocker.empty(),"Queue diagnostic lost actual committed prefix or claimed rollback/success");
        // Independent expected traversal is the sorted complete source roster,
        // not a returned producer-selected team list. Successful returns form
        // its initial segment; a pending team is exactly the next minimum alive.
        need(after.completed_teams>=1&&after.completed_teams<=64&&after.last_completed_team.has_value(),"Queue return history missing");
        std::size_t completed=0;
        for(const auto id:ascending){if(id<=*after.last_completed_team)++completed;}
        need(after.completed_teams==completed+1,"Queue skipped/repeated an actual living source team");
        std::optional<std::int32_t> pending;
        for(const auto id:ascending)if(id>*after.last_completed_team&&(!pending||id<*pending))pending=id;
        need(after.next_alive_team==pending&&after.team_loop_next_slot==pending.value_or(64),"Queue source scan skipped dead/alive gap or fabricated exhaustion");
        if(after.active_team)need(pending==after.active_team&&*after.active_team!=63,"Queue active source identity differs from pending minimum");
        // Only actually completed teams and the actual pending team's reached
        // war/no-trade phases are applied to this detached value oracle.
        for(const auto id:ascending){if(id==63)continue;
            const bool returned=id<=*after.last_completed_team;const bool active=after.active_team==id;
            if(!returned&&!active)continue;
            const auto index=static_cast<std::size_t>(id);auto& team=oracle.world.teams->teams[index];
            const auto relation=team_test_array(team,"at_war").values;auto& war=team_test_array(team,"turns_at_war").values;
            auto& locked=team_test_array(team,"turns_locked_at_war").values;
            for(std::size_t target=0;target<63;++target){const auto candidate=relation[target]?war[target]+1:0;
                need(candidate<=INT32_MAX,"Queue expected actual counter overflow branch unavailable");
                if(target!=index||candidate==0)war[target]=candidate;
                if(locked[target]>0&&(target!=index||locked[target]-1==0))--locked[target];}
            const bool cleared=returned||(active&&(after.frontier==BnwRetainedTurnFrontier::pending_team_small_awards||
                after.frontier==BnwRetainedTurnFrontier::pending_team_circumnavigation||after.frontier==BnwRetainedTurnFrontier::pending_team_return_tail));
            if(cleared){std::vector<std::int64_t>* flags=nullptr;const std::vector<std::int64_t>* hashes=nullptr;
                for(auto& row:team.arrays){if(row.name&&std::string_view(row.name)=="no_trade_tech")flags=&row.values;
                    if(row.name&&std::string_view(row.name)=="tech_hashes")hashes=&row.values;}
                need(flags&&hashes&&flags->size()==hashes->size(),"Queue oracle raw technology span absent");
                for(std::size_t row=0;row<hashes->size();++row){const auto hash=(*hashes)[row];need(hash>=0&&hash<=UINT32_MAX,"Queue oracle hash width");
                    if(!hash)continue;const auto* type=host->types().find_hash(static_cast<std::uint64_t>(hash));
                    need(type&&type->table=="Technologies","Queue oracle actual hash mapping absent");
                    (*flags)[row]=0;oracle.technologies[index].set_no_trade(type->id,false);}
            }
        }
        const auto make_current=[&](const LiveSessionState& value){return make_bnw_team_turn_checkpoint_pair(value,host->session().source().snapshot,
            original.technologies,original.player_research,host->technologies(),host->types(),host->identity());};
        const auto actual_pair=make_current(*lease),expected_pair=make_current(oracle);
        need(actual_pair.raw.bytes==expected_pair.raw.bytes&&actual_pair.owned.bytes==expected_pair.owned.bytes,
             "Queue committed source counter/no-trade order or unexecuted team state differs from independent oracle");
        need(host->session().read_state_lease()==lease&&host->session().state_generation()==generation&&rng==lease->random.gameplay.state()&&
             maprng==lease->random.map.state()&&turn==lease->setup.game_turn&&map==circumnavigation_raw_state(*lease)&&awards==small_award_state(*lease)&&
             score_calls==cursor->score_dirty_setter_count(*ui)&&game_calls==cursor->game_data_dirty_setter_count(*ui)&&
             client->is_current(frame)==(revision==lease->ui_source_revision),"Queue changed unrelated world/RNG/clock/UI allocation or frame freshness");
        if(after.frontier==BnwRetainedTurnFrontier::remaining_teams_turn&&pending==63){need(!after.active_team&&
             diagnostic.blocker.find("mandatory DoBarbarianTech BEFORE operation14")!=std::string::npos,"Queue skipped actual barbarian technology boundary");}
        if(after.frontier==BnwRetainedTurnFrontier::map_turn_unavailable)need(!pending&&!after.active_team&&diagnostic.stop==BnwRetainedTeamsStop::map_boundary,
             "Queue actual exhaustion executed or skipped required map turn");
        if(after.active_team&&actual_minor_test(*lease,*after.active_team)&&after.frontier==BnwRetainedTurnFrontier::next_team_minor_technology){
            need(diagnostic.blocker.find("DoMinorCivTech/setHasTech")!=std::string::npos,"Minor acquisition became an acknowledged no-effect skip");
            const auto blocked=make_current(*lease);const auto operation=after.operation;
            const auto blocked_rng=lease->random.gameplay.state(),blocked_map_rng=lease->random.map.state();
            const auto blocked_revision=lease->ui_source_revision;
            const auto due=cursor->pending_minor_technology_plan();
            need(due&&due->team==*after.active_team,"Reached natural minor grant has no SAME first due plan");
            check_natural_minor_due_plan(*due,*lease,*host);++observed_minor_due_plans;
            need(cursor->pending_minor_technology_plan()==due&&cursor->view().operation==operation&&
                 cursor->view().frontier==after.frontier&&blocked_rng==lease->random.gameplay.state()&&
                 blocked_map_rng==lease->random.map.state()&&blocked_revision==lease->ui_source_revision,
                 "Read-only due plan advanced history or mutated world/RNG");
            const auto after_query=make_current(*lease);
            need(after_query.raw.bytes==blocked.raw.bytes&&after_query.owned.bytes==blocked.owned.bytes,
                 "Read-only due plan wrote paired HasTech/no-trade state");
            refuses([&]{cursor->execute_pending_team_no_trade_prefix();});const auto repeated=make_current(*lease);
            need(cursor->pending_minor_technology_plan()==due,"Mandatory refusal changed first due source plan");
            need(cursor->view().operation==operation&&blocked.raw.bytes==repeated.raw.bytes&&blocked.owned.bytes==repeated.owned.bytes,
                 "Reached minor grant retry consumed phase or partially cleared no-trade state");
        }
        if(minor_team){
            if(*after.last_completed_team>=*minor_team)std::cout<<"Natural genuine imported minor no-acquisition + actual SmallAwards early return reached\n";
            else std::cout<<"Natural minor no-acquisition scenario unavailable at prior mandatory boundary: "<<diagnostic.blocker<<'\n';
        }
        if(after.completed_teams>=3)std::cout<<"Genuine ascending queue returned at least three actual teams\n";
        else std::cout<<"Three-team queue branch unavailable: returned="<<after.completed_teams<<" blocker="<<diagnostic.blocker<<'\n';
        need(BnwRetainedTurnCursor::bind(std::shared_ptr<const ui::SelectedBnwPreviewHost>(host))==cursor,"Queue rebuilt sticky source history");
        refuses([&]{(void)host->export_import_clock_checkpoint();});need(!host->session().readiness().can_continue(),"Queue diagnostic granted full map/player/game continuation");
        std::cout<<"Queue mode="<<mode<<" actual committed operations="<<diagnostic.returned_operations<<" completed teams="<<after.completed_teams
                 <<" pending="<<pending.value_or(64)<<" blocker="<<diagnostic.blocker<<'\n';
    }
    if(!observed_minor_due_plans)std::cout<<"Due minor-tech positive query fixture UNAVAILABLE: natural source did not reach required grant; no effect-owner/HasTech/alive/Pregame fixture fabricated\n";
    std::cout<<"Zero-major/forced-gaps/forced-last63/map fixtures unavailable through existing codecs: alive_count/minor/at_war not fabricated\n";
}
void first_team_return_owner_guards(const std::filesystem::path& save,const std::filesystem::path& game){
    for(int mode=0;mode<6;++mode){const auto host=cycle_host(save,game);auto client=ui::BnwClientFrameOwner::from_selected_host(host,test_detail::selected_city_frame_sources(host));
        auto ui=client->create_selected_ui_host(cycle_registry(),nullptr);auto cursor=BnwRetainedTurnCursor::bind(std::shared_ptr<const ui::SelectedBnwPreviewHost>(host));
        refuses([&]{cursor->execute_first_team_return_and_advance_prefix();});need(!cursor->view().first_team_return_attempted,"Team return skipped predecessors");
        to_no_trade_frontier(cursor,*ui);cursor->execute_first_team_nonminor_no_trade_prefix();cursor->execute_first_team_small_awards_prefix();
        const auto expected=actual_circumnavigation_expected(host->session().state(),*cursor->view().first_team);
        if(!expected.returned){std::cout<<"Actual selected map requires mandatory circumnavigation effects: op13 owner case unavailable\n";break;}
        cursor->execute_first_team_circumnavigation_prefix();const auto lease=host->session().read_state_lease();const auto revision=lease->ui_source_revision;
        const auto before=make_bnw_team_turn_checkpoint(lease->world,host->session().source().snapshot,host->identity());
        const auto invoke=[&]{cursor->execute_first_team_return_and_advance_prefix();};
        if(mode==0){const auto cp=host->session().checkpoint();host->session().restore(cp);refuses(invoke);}
        else if(mode==1){client.reset();refuses(invoke);}
        else if(mode==2){bool refused=false;std::thread worker([&]{try{invoke();}catch(const std::exception&){refused=true;}});worker.join();need(refused,"Foreign team-return accepted");}
        else if(mode==3){refuses([&]{const_cast<content::RulesDatabase&>(host->database()).apply_sql("UPDATE Defines SET Value=Value WHERE 0");});refuses(invoke);}
        else if(mode==4){(void)std::const_pointer_cast<BnwGameTurnTimerOwner>(cursor->timer_owner())->reset_for_current_player();refuses(invoke);}
        else{cursor->retire();refuses(invoke);}
        need(cursor->view().operation==12&&!cursor->view().first_team_return_attempted&&!cursor->view().first_team_tail_assertions&&
             !cursor->view().team_loop_next_slot&&!cursor->view().next_alive_team&&lease->ui_source_revision==revision&&
             before.bytes==make_bnw_team_turn_checkpoint(lease->world,host->session().source().snapshot,host->identity()).bytes,
             "Invalid team-return owner mutated world or consumed phase");
    }
    const auto host=cycle_host(save,game);auto generic=BnwRetainedTurnCursor::bind(clone(*host));(void)generic->execute_timer_prefix();
    refuses([&]{generic->execute_first_team_return_and_advance_prefix();});need(!generic->view().first_team_return_attempted,"Generic owner acquired team-return capability");
}
void circumnavigation_owner_guards(const std::filesystem::path& save,const std::filesystem::path& game){
    for(int mode=0;mode<6;++mode){const auto host=cycle_host(save,game);auto client=ui::BnwClientFrameOwner::from_selected_host(host,test_detail::selected_city_frame_sources(host));
        auto ui=client->create_selected_ui_host(cycle_registry(),nullptr);auto cursor=BnwRetainedTurnCursor::bind(std::shared_ptr<const ui::SelectedBnwPreviewHost>(host));
        refuses([&]{cursor->execute_first_team_circumnavigation_prefix();});need(!cursor->view().first_team_circumnavigation_attempted,"Circumnavigation skipped predecessors");
        to_no_trade_frontier(cursor,*ui);cursor->execute_first_team_nonminor_no_trade_prefix();cursor->execute_first_team_small_awards_prefix();
        const auto lease=host->session().read_state_lease();const auto revision=lease->ui_source_revision;const auto before=circumnavigation_raw_state(*lease);
        const auto invoke=[&]{cursor->execute_first_team_circumnavigation_prefix();};
        if(mode==0){const auto cp=host->session().checkpoint();host->session().restore(cp);refuses(invoke);}
        else if(mode==1){client.reset();refuses(invoke);}
        else if(mode==2){bool refused=false;std::thread worker([&]{try{invoke();}catch(const std::exception&){refused=true;}});worker.join();need(refused,"Foreign circumnavigation accepted");}
        else if(mode==3){refuses([&]{const_cast<content::RulesDatabase&>(host->database()).apply_sql("UPDATE Defines SET Value=Value WHERE 0");});refuses(invoke);}
        else if(mode==4){(void)std::const_pointer_cast<BnwGameTurnTimerOwner>(cursor->timer_owner())->reset_for_current_player();refuses(invoke);}
        else{cursor->retire();refuses(invoke);}
        need(cursor->view().operation==11&&!cursor->view().first_team_circumnavigation_attempted&&!cursor->view().first_team_circumnavigation_return&&
             !cursor->view().first_team_circumnavigation_axis&&lease->ui_source_revision==revision&&before==circumnavigation_raw_state(*lease),
             "Invalid circumnavigation owner changed source data or consumed phase");
    }
    const auto host=cycle_host(save,game);auto generic=BnwRetainedTurnCursor::bind(clone(*host));(void)generic->execute_timer_prefix();
    refuses([&]{generic->execute_first_team_circumnavigation_prefix();});need(!generic->view().first_team_circumnavigation_attempted,"Generic owner acquired circumnavigation permission");
}
void exercise_actual_small_awards(const std::shared_ptr<ui::SelectedBnwPreviewHost>& host,
    const std::unique_ptr<ui::BnwClientFrameOwner>& client,const ui::LiveUiHost& ui,
    const std::shared_ptr<BnwRetainedTurnCursor>& cursor){
    const auto lease=host->session().read_state_lease();const auto view=cursor->view();
    need(view.operation==10&&view.first_team&&view.first_team_is_minor==false,"Small-award test lacks actual predecessor");
    const auto first=*view.first_team;const auto revision=lease->ui_source_revision,generation=host->session().state_generation();
    const auto before=small_award_state(*lease);
    const auto raw=make_bnw_team_turn_checkpoint(lease->world,host->session().source().snapshot,host->identity());
    const auto* teams_data=lease->world.teams->teams.data();const auto* players_data=lease->world.all_players->players.data();
    const auto frame=client->capture({1024,768,1024,768,96},{});
    const auto rng=lease->random.gameplay.state(),maprng=lease->random.map.state();const auto turn=lease->setup.game_turn;
    const auto score_calls=cursor->score_dirty_setter_count(ui),game_calls=cursor->game_data_dirty_setter_count(ui);
    const auto score_flag=cursor->score_dirty(ui),game_flag=cursor->game_data_dirty(ui);
    // Independent actual SQL row enumeration, not a supplied completion bool.
    struct Row{std::int64_t slot,cities,population;std::string type;};std::vector<Row> rows;
    const auto query=host->database().query_rows({"SELECT ID,Type,NumCities,CityPopulation FROM SmallAwards WHERE ID > -1 ORDER BY ID",{}},
        [&](const content::RulesRowView& r){const auto type=r.text("Type");need(type.has_value(),"Actual SmallAwards type unavailable");
            rows.push_back({r.integer("ID"),r.integer("NumCities"),r.integer("CityPopulation"),*type});});
    need(query.all_succeeded(),"Actual SmallAwards SQL query failed");
    std::uint64_t slots=0;bool reached=false;
    const auto city_field=[](const saves::SavedCityObservation& city,std::string_view name){std::optional<std::int64_t> result;
        for(const auto& f:city.core.fields)if(f.name&&std::string_view(f.name)==name){need(!result&&f.values.size()==1,"Actual city source field duplicate/missing");result=f.values[0];}
        need(result.has_value(),"Actual city population source absent");return *result;};
    for(const auto& row:rows){need(row.slot>=0&&static_cast<std::uint64_t>(row.slot)>=slots,"Actual stock SmallAwards slot overlap");slots=static_cast<std::uint64_t>(row.slot+1);
        const auto* type=host->types().find(row.type);need(type&&type->table=="SmallAwards"&&type->id==row.slot,"Actual award type disagrees");
        std::optional<bool> achieved;
        for(const auto& a:lease->world.teams->teams[static_cast<std::size_t>(first)].hashes)if(a.name&&std::string_view(a.name)=="small_award_achieved")
            for(const auto& r:a.rows)if(r.type_hash==type->hash){need(!achieved&&r.values.size()==1,"Actual award raw duplicate/missing");achieved=r.values[0]!=0;}
        need(achieved.has_value(),"Actual award raw slot unavailable");if(*achieved)continue;
        for(std::size_t player=0;player<22;++player)if(lease->world.pregame->archive.integer_arrays.at("team_types")[player]==first){
            const auto& cities=lease->world.all_players->players[player].cities;std::optional<std::int64_t> last,free;
            for(const auto& f:cities.header.fields){if(f.name&&std::string_view(f.name)=="last_index")last=f.value;
                if(f.name&&std::string_view(f.name)=="free_list_count")free=f.value;}
            need(last&&free,"Actual source city GetCount operands absent");
            reached=reached||(row.cities>0&&*last-*free+1>=row.cities);
            if(row.population>0)for(const auto& city:cities.cities)reached=reached||city_field(city,"m_iPopulation")>=row.population;
        }
    }
    if(reached){std::string why;bool refused=false;try{cursor->execute_first_team_small_awards_prefix();}catch(const std::exception& e){refused=true;why=e.what();}
        need(refused&&why.find("mandatory award/victory_points/notifications/checkpoint owner")!=std::string::npos&&
             cursor->view().operation==10&&!cursor->view().first_team_small_awards_attempted&&
             !cursor->view().first_team_small_award_slots&&cursor->view().frontier==BnwRetainedTurnFrontier::first_team_small_awards,
             "Reached award became skip/store or consumed operation11 before effect owner");
    }else{cursor->execute_first_team_small_awards_prefix();
        need(cursor->view().operation==11&&cursor->view().first_team_small_awards_attempted&&cursor->view().first_team_small_award_slots==slots&&
             cursor->view().frontier==BnwRetainedTurnFrontier::first_team_circumnavigation,
             "Actual no-effect SmallAwards phase failed or advanced past circumnavigation");
        refuses([&]{cursor->execute_first_team_small_awards_prefix();});
    }
    need(host->session().read_state_lease()==lease&&host->session().state_generation()==generation&&
         lease->world.teams->teams.data()==teams_data&&lease->world.all_players->players.data()==players_data&&
         small_award_state(*lease)==before&&raw.bytes==make_bnw_team_turn_checkpoint(lease->world,host->session().source().snapshot,host->identity()).bytes&&
         lease->ui_source_revision==revision&&client->is_current(frame)&&lease->random.gameplay.state()==rng&&lease->random.map.state()==maprng&&
         lease->setup.game_turn==turn&&cursor->score_dirty_setter_count(ui)==score_calls&&cursor->game_data_dirty_setter_count(ui)==game_calls&&
         cursor->score_dirty(ui)==score_flag&&cursor->game_data_dirty(ui)==game_flag,
         "Actual SmallAwards read-only completion/refusal changed award/points/world/RNG/UI/frame");
    refuses([&]{(void)host->export_import_clock_checkpoint();});need(!host->session().readiness().can_continue(),"SmallAwards granted full-turn persistence/continuation");
    std::cout<<"Actual selected SmallAwards rows="<<rows.size()<<" slots="<<slots<<" reached="<<reached<<" native Original parity not inferred\n";
    if(!reached)exercise_actual_circumnavigation(host,client,ui,cursor);
}
void small_award_owner_guards(const std::filesystem::path& save,const std::filesystem::path& game){
    for(int mode=0;mode<6;++mode){const auto host=cycle_host(save,game);auto client=ui::BnwClientFrameOwner::from_selected_host(host,test_detail::selected_city_frame_sources(host));
        auto ui=client->create_selected_ui_host(cycle_registry(),nullptr);auto cursor=BnwRetainedTurnCursor::bind(std::shared_ptr<const ui::SelectedBnwPreviewHost>(host));
        refuses([&]{cursor->execute_first_team_small_awards_prefix();});need(!cursor->view().first_team_small_awards_attempted,"SmallAwards skipped predecessors");
        to_no_trade_frontier(cursor,*ui);cursor->execute_first_team_nonminor_no_trade_prefix();
        const auto lease=host->session().read_state_lease();const auto revision=lease->ui_source_revision;const auto before=small_award_state(*lease);
        const auto invoke=[&]{cursor->execute_first_team_small_awards_prefix();};
        if(mode==0){const auto cp=host->session().checkpoint();host->session().restore(cp);refuses(invoke);}
        else if(mode==1){client.reset();refuses(invoke);}
        else if(mode==2){bool refused=false;std::thread worker([&]{try{invoke();}catch(const std::exception&){refused=true;}});worker.join();need(refused,"Foreign SmallAwards accepted");}
        else if(mode==3){refuses([&]{const_cast<content::RulesDatabase&>(host->database()).apply_sql("UPDATE Defines SET Value=Value WHERE 0");});refuses(invoke);}
        else if(mode==4){(void)std::const_pointer_cast<BnwGameTurnTimerOwner>(cursor->timer_owner())->reset_for_current_player();refuses(invoke);}
        else{cursor->retire();refuses(invoke);}
        need(cursor->view().operation==10&&!cursor->view().first_team_small_awards_attempted&&!cursor->view().first_team_small_award_slots&&
             lease->ui_source_revision==revision&&small_award_state(*lease)==before,"Invalid SmallAwards owner mutated or consumed phase");
    }
    const auto host=cycle_host(save,game);auto generic=BnwRetainedTurnCursor::bind(clone(*host));(void)generic->execute_timer_prefix();
    refuses([&]{generic->execute_first_team_small_awards_prefix();});need(!generic->view().first_team_small_awards_attempted,"Generic owner acquired SmallAwards capability");
}
void actual_first_no_trade(const std::filesystem::path& save,const std::filesystem::path& game,int controlled=-1){
    const auto host=cycle_host(save,game);auto client=ui::BnwClientFrameOwner::from_selected_host(host,test_detail::selected_city_frame_sources(host));
    auto ui=client->create_selected_ui_host(cycle_registry(),nullptr);auto cursor=BnwRetainedTurnCursor::bind(std::shared_ptr<const ui::SelectedBnwPreviewHost>(host));
    refuses([&]{cursor->execute_first_team_nonminor_no_trade_prefix();});need(!cursor->view().first_team_no_trade_attempted,"No-trade skipped predecessors");
    try{to_no_trade_frontier(cursor,*ui);}catch(const std::exception& e){need(controlled<0,"Synthetic no-trade input failed actual predecessors");
        std::cout<<"Original selected no-trade unreached: "<<e.what()<<'\n';return;}
    const auto lease=host->session().read_state_lease();const auto first=*cursor->view().first_team;const auto minor=actual_minor_test(*lease,first);
    const auto generation=host->session().state_generation(),revision=lease->ui_source_revision;
    const auto frame=client->capture({1024,768,1024,768,96},{});
    // ID8 baseline technologies cannot include acquired flags; only no_trade
    // differs here. Use Original restored source as codec identity companion.
    const auto original_tech=restore_team_technologies(*host->session().source().snapshot.teams,host->technologies(),host->types());
    const auto pair_of=[&](const LiveSessionState& value){return make_bnw_team_turn_checkpoint_pair(value,host->session().source().snapshot,
        original_tech,value.player_research,host->technologies(),host->types(),host->identity());};
    const auto raw_before=make_bnw_team_turn_checkpoint(lease->world,host->session().source().snapshot,host->identity());
    const auto* owned=&lease->technologies[static_cast<std::size_t>(first)];const auto* owned_roster=lease->technologies.data();const auto* team_roster=lease->world.teams->teams.data();
    const auto* raw_data=static_cast<const std::int64_t*>(nullptr);
    for(const auto& row:lease->world.teams->teams[static_cast<std::size_t>(first)].arrays)if(row.name&&std::string_view(row.name)=="no_trade_tech")raw_data=row.values.data();
    const auto rng=lease->random.gameplay.state(),maprng=lease->random.map.state();const auto turn=lease->setup.game_turn;
    const auto score_calls=cursor->score_dirty_setter_count(*ui),game_calls=cursor->game_data_dirty_setter_count(*ui);
    const auto score_flag=cursor->score_dirty(*ui),game_flag=cursor->game_data_dirty(*ui);
    if(minor){bool refused=false;std::string why;try{cursor->execute_first_team_nonminor_no_trade_prefix();}catch(const std::exception& e){refused=true;why=e.what();}
        need(refused&&why.find("DoMinorCivTech")!=std::string::npos&&cursor->view().operation==9&&!cursor->view().first_team_no_trade_attempted&&
             !cursor->view().first_team_is_minor&&client->is_current(frame)&&lease->ui_source_revision==revision&&
             raw_before.bytes==make_bnw_team_turn_checkpoint(lease->world,host->session().source().snapshot,host->identity()).bytes,
             "True minor mandatory technology grant became a successful skip or store");return;}
    auto expected=*lease;bool changed=false;
    for(auto& row:expected.world.teams->teams[static_cast<std::size_t>(first)].arrays)if(row.name&&std::string_view(row.name)=="no_trade_tech"){
        for(auto& value:row.values){changed=changed||value!=0;value=0;}}
    for(std::int64_t slot=0;slot<static_cast<std::int64_t>(expected.technologies[static_cast<std::size_t>(first)].slots());++slot)
        if(expected.technologies[static_cast<std::size_t>(first)].at(slot).no_trade)expected.technologies[static_cast<std::size_t>(first)].set_no_trade(slot,false);
    if(controlled>=0)need(changed==(controlled==0),"Synthetic changed/unchanged no-trade input lost its actual flag");
    cursor->execute_first_team_nonminor_no_trade_prefix();const auto after=pair_of(*lease),want=pair_of(expected);
    need(after.raw.bytes==want.raw.bytes&&after.owned.bytes==want.owned.bytes,
         "Actual paired no-trade clear differs or changed another team/owned technology value");
    need(cursor->view().operation==10&&cursor->view().first_team_no_trade_attempted&&cursor->view().first_team_is_minor==false&&
         cursor->view().frontier==BnwRetainedTurnFrontier::first_team_small_awards,
         "No-trade did not record actual nonminor predicate or advanced past small awards");
    need(host->session().read_state_lease()==lease&&host->session().state_generation()==generation&& &lease->technologies[static_cast<std::size_t>(first)]==owned&&
         lease->technologies.data()==owned_roster&&lease->world.teams->teams.data()==team_roster&&
         lease->ui_source_revision==revision+(changed?1:0)&&client->is_current(frame)==!changed&&
         lease->random.gameplay.state()==rng&&lease->random.map.state()==maprng&&lease->setup.game_turn==turn&&
         cursor->score_dirty_setter_count(*ui)==score_calls&&cursor->game_data_dirty_setter_count(*ui)==game_calls&&
         cursor->score_dirty(*ui)==score_flag&&cursor->game_data_dirty(*ui)==game_flag,
         "No-trade replaced actual world/owner or changed clock/RNG/UI setters/currentness");
    for(const auto& row:lease->world.teams->teams[static_cast<std::size_t>(first)].arrays)if(row.name&&std::string_view(row.name)=="no_trade_tech")
        need(row.values.data()==raw_data,"Actual no-trade raw storage replaced");
    refuses([&]{cursor->execute_first_team_nonminor_no_trade_prefix();});refuses([&]{(void)host->export_import_clock_checkpoint();});
    need(client->is_current(client->capture({1024,768,1024,768,96},{}))&&!host->session().readiness().can_continue(),"No-trade broke fresh frame or granted full turn");
    std::cout<<(controlled<0?"Original selected":"SYNTHETIC paired ID40+8")<<" actual no-trade changed="<<changed<<'\n';
    exercise_actual_small_awards(host,client,*ui,cursor);
}
void no_trade_owner_guards(const std::filesystem::path& save,const std::filesystem::path& game){
    for(int mode=0;mode<6;++mode){const auto host=cycle_host(save,game);auto client=ui::BnwClientFrameOwner::from_selected_host(host,test_detail::selected_city_frame_sources(host));
        auto ui=client->create_selected_ui_host(cycle_registry(),nullptr);auto cursor=BnwRetainedTurnCursor::bind(std::shared_ptr<const ui::SelectedBnwPreviewHost>(host));
        to_no_trade_frontier(cursor,*ui);const auto lease=host->session().read_state_lease();const auto revision=lease->ui_source_revision;
        const auto before=make_bnw_team_turn_checkpoint(lease->world,host->session().source().snapshot,host->identity());
        const auto invoke=[&]{cursor->execute_first_team_nonminor_no_trade_prefix();};
        if(mode==0){const auto cp=host->session().checkpoint();host->session().restore(cp);refuses(invoke);}
        else if(mode==1){client.reset();refuses(invoke);}
        else if(mode==2){bool refused=false;std::thread worker([&]{try{invoke();}catch(const std::exception&){refused=true;}});worker.join();need(refused,"Foreign no-trade accepted");}
        else if(mode==3){refuses([&]{const_cast<content::RulesDatabase&>(host->database()).apply_sql("UPDATE Defines SET Value=Value WHERE 0");});refuses(invoke);}
        else if(mode==4){(void)std::const_pointer_cast<BnwGameTurnTimerOwner>(cursor->timer_owner())->reset_for_current_player();refuses(invoke);}
        else{cursor->retire();refuses(invoke);}
        need(cursor->view().operation==9&&!cursor->view().first_team_no_trade_attempted&&!cursor->view().first_team_is_minor&&lease->ui_source_revision==revision&&
             before.bytes==make_bnw_team_turn_checkpoint(lease->world,host->session().source().snapshot,host->identity()).bytes,"Invalid no-trade owner mutated or consumed prefix");
    }
    const auto host=cycle_host(save,game);auto generic=BnwRetainedTurnCursor::bind(clone(*host));(void)generic->execute_timer_prefix();
    refuses([&]{generic->execute_first_team_nonminor_no_trade_prefix();});need(!generic->view().first_team_no_trade_attempted,"Generic owner acquired no-trade capability");
}
void no_trade_codec_cases(const std::filesystem::path& save,const std::filesystem::path& game){
    const auto baseline=cycle_host(save,game);const auto original=baseline->session().state();const auto first=first_alive_test(original);
    need(first&&*first<63&&!actual_minor_test(original,*first),"Selected source has no first nonminor team for genuine clear fixture");
    const auto make=[&](const LiveSessionState& state){return make_bnw_team_turn_checkpoint_pair(state,baseline->session().source().snapshot,
        original.technologies,original.player_research,baseline->technologies(),baseline->types(),baseline->identity());};
    for(int mode=0;mode<2;++mode){auto state=original;auto& team=state.world.teams->teams[static_cast<std::size_t>(*first)];std::vector<std::int64_t>* flags=nullptr;const std::vector<std::int64_t>* hashes=nullptr;
        for(auto& row:team.arrays){if(row.name&&std::string_view(row.name)=="no_trade_tech")flags=&row.values;
            if(row.name&&std::string_view(row.name)=="tech_hashes")hashes=&row.values;}
        need(flags&&hashes&&flags->size()==hashes->size(),"Actual no-trade hash span absent");std::optional<std::int64_t> chosen;
        for(std::size_t row=0;row<hashes->size();++row){const auto hash=(*hashes)[row];if(!hash)continue;
            need(hash>=0&&hash<=UINT32_MAX,"Actual no-trade hash outside source integer range");
            const auto* type=baseline->types().find_hash(static_cast<std::uint64_t>(hash));need(type&&type->table=="Technologies","Actual no-trade type absent");
            const bool value=mode==0&&!chosen;(*flags)[row]=value;state.technologies[static_cast<std::size_t>(*first)].set_no_trade(type->id,value);if(value)chosen=type->id;}
        need(mode!=0||chosen,"No actual slot for changed no-trade fixture");
        state.world.history.deals.groups[1].clear();auto pair=make(state);
        {auto bad=state;for(auto& row:bad.world.teams->teams[static_cast<std::size_t>(*first)].arrays)if(row.name&&std::string_view(row.name)=="no_trade_tech")row.values[0]=2;
            refuses([&]{(void)make(bad);});} // real paired codec rejects nonboolean, not a fake live setter
        auto document=saves::make_civis_import_document(baseline->session().source().source,identify_file(save).sha256);
        const auto owned_id=static_cast<std::uint32_t>(saves::CivisSaveSectionId::live_team_research);
        std::erase_if(document.sections,[&](const auto& row){return row.id==bnw_team_turn_checkpoint_section_id||row.id==owned_id||row.id==bnw_game_deals_checkpoint_section_id;});
        document.sections.push_back(pair.raw);document.sections.push_back(pair.owned);
        document.sections.push_back(make_bnw_game_deals_checkpoint(state.world,baseline->session().source().snapshot,baseline->identity()));
        const auto path=std::filesystem::temp_directory_path()/("bnw-no-trade-codec-"+std::to_string(GetCurrentProcessId())+"-"+std::to_string(qpc())+".CivisSave");
        saves::write_civis_save(path,document);struct Cleanup{std::filesystem::path path;~Cleanup(){std::error_code error;std::filesystem::remove(path,error);}}cleanup{path};
        actual_first_no_trade(path,game,mode);if(mode==0)no_trade_owner_guards(path,game);if(mode==0)small_award_owner_guards(path,game);if(mode==0)circumnavigation_owner_guards(path,game);if(mode==0)first_team_return_owner_guards(path,game);
    }
    std::cout<<"True-minor first-team fixture unavailable: no Pregame/alive producer or codec was fabricated; mandatory branch remains explicit refusal\n";
}
void deal_guards(const std::filesystem::path& save,const std::filesystem::path& game){
    for(int mode=0;mode<7;++mode){
        const auto host=cycle_host(save,game);
        auto client=ui::BnwClientFrameOwner::from_selected_host(host,test_detail::selected_city_frame_sources(host));
        auto ui=client->create_selected_ui_host(cycle_registry(),nullptr);
        auto cursor=BnwRetainedTurnCursor::bind(std::shared_ptr<const ui::SelectedBnwPreviewHost>(host));
        to_deals_frontier(cursor,*ui);const auto state=host->session().read_state_lease();
        const auto before=make_bnw_game_deals_checkpoint(state->world,host->session().source().snapshot,host->identity());const auto revision=state->ui_source_revision;
        const auto invoke=[&]{cursor->execute_game_deals_prefix();};
        if(mode==0){const auto cp=host->session().checkpoint();host->session().restore(cp);refuses(invoke);}
        else if(mode==1){client.reset();refuses(invoke);}
        else if(mode==2){cursor->retire();refuses(invoke);}
        else if(mode==3){bool refused=false;std::thread thread([&]{try{invoke();}catch(const std::exception&){refused=true;}});thread.join();need(refused,"Foreign deal call accepted");}
        else if(mode==4){(void)std::const_pointer_cast<BnwGameTurnTimerOwner>(cursor->timer_owner())->reset_for_current_player();refuses(invoke);}
        else if(mode==5){refuses([&]{const_cast<content::RulesDatabase&>(host->database()).apply_sql("UPDATE Defines SET Value=Value WHERE 0");});refuses(invoke);}
        else{(void)host->session().mutable_state();refuses(invoke);}
        const auto after=make_bnw_game_deals_checkpoint(state->world,host->session().source().snapshot,host->identity());
        need(before.bytes==after.bytes&&state->ui_source_revision==revision&&cursor->view().operation==7&&!cursor->view().deals_attempted,
             "Invalid deals call changed lists/revision or consumed operation8");
    }
    const auto host=cycle_host(save,game);auto generic=BnwRetainedTurnCursor::bind(clone(*host));
    (void)generic->execute_timer_prefix();refuses([&]{generic->execute_game_deals_prefix();});
    need(generic->view().operation==1&&!generic->view().deals_attempted,"Generic timer-only facade granted actual deals phase");
}
void score_guards(const std::filesystem::path& save,const std::filesystem::path& game){
    for(int mode=0;mode<7;++mode){
        const auto host=cycle_host(save,game);
        auto client=ui::BnwClientFrameOwner::from_selected_host(host,test_detail::selected_city_frame_sources(host));
        auto ui=client->create_selected_ui_host(cycle_registry(),nullptr);
        auto cursor=BnwRetainedTurnCursor::bind(std::shared_ptr<const ui::SelectedBnwPreviewHost>(host));
        to_score_frontier(cursor,*ui);const auto state=host->session().read_state_lease();
        const auto dirty=actual_score_dirty(*state);const auto revision=state->ui_source_revision;
        const auto arrays=state->world.game_state.roster->arrays;
        const auto invoke=[&]{cursor->execute_game_score_prefix();};
        if(mode==0){const auto cp=host->session().checkpoint();host->session().restore(cp);refuses(invoke);}
        else if(mode==1){client.reset();refuses(invoke);}
        else if(mode==2){cursor->retire();refuses(invoke);}
        else if(mode==3){bool refused=false;std::thread t([&]{try{invoke();}catch(const std::exception&){refused=true;}});t.join();need(refused,"Foreign score accepted");}
        else if(mode==4){(void)std::const_pointer_cast<BnwGameTurnTimerOwner>(cursor->timer_owner())->reset_for_current_player();refuses(invoke);}
        else if(mode==5){refuses([&]{const_cast<content::RulesDatabase&>(host->database()).apply_sql("UPDATE Defines SET Value=Value WHERE 0");});refuses(invoke);}
        else{(void)host->session().mutable_state();refuses(invoke);}
        need(cursor->view().operation==6&&!cursor->view().score_attempted&&actual_score_dirty(*state)==dirty&&
             state->world.game_state.roster->arrays==arrays&&state->ui_source_revision==revision,
             "Invalid score call stored arrays/flag/revision or granted once completion");
    }
    const auto host=cycle_host(save,game);auto generic=BnwRetainedTurnCursor::bind(clone(*host));
    (void)generic->execute_timer_prefix();refuses([&]{generic->execute_game_score_prefix();});
    need(!generic->view().score_attempted&&generic->view().operation==1,"Generic timer-only facade granted score phase");
}
void aliases_and_lifetime(const ui::SelectedBnwPreviewHost& host){
    auto session=clone(host);std::weak_ptr<const LiveSession> life=session;
    struct OtherOwner {std::shared_ptr<const LiveSession> retained;};
    auto other=std::make_shared<OtherOwner>(OtherOwner{session});
    auto alias=std::shared_ptr<const LiveSession>(other,session.get());
    need(alias.get()==session.get()&&alias.owner_before(session)!=session.owner_before(alias),"Test needs distinct legitimate controlblocks");
    auto cursor=BnwRetainedTurnCursor::bind(session);
    need(BnwRetainedTurnCursor::bind(alias)==cursor,"Alternative owner bypassed same-facade ledger");
    const auto timer=cursor->timer_owner();const auto reset=cursor->execute_timer_prefix();
    cursor.reset();
    refuses([&]{(void)BnwRetainedTurnCursor::bind(session);});
    refuses([&]{(void)BnwRetainedTurnCursor::bind(alias);});
    refuses([&]{(void)timer->read(timer->current_use());});
    need(reset.operation==1,"Genuine reset not retained before handle destruction");
    alias.reset();other.reset();session.reset();
    need(!life.expired(),"Returned timer lost exact retained session lifetime");
    // timer's own strong lifetime ends at this function's end; registry holds
    // only weak facade owners and must not make that session immortal.
}
void alias_expiry(const ui::SelectedBnwPreviewHost& host){
    auto actual=clone(host);
    struct Owner {std::shared_ptr<LiveSession> actual;};
    {auto owner=std::make_shared<Owner>(Owner{actual});
     auto alias=std::shared_ptr<const LiveSession>(owner,actual.get());
     auto cursor=BnwRetainedTurnCursor::bind(alias);(void)cursor->execute_timer_prefix();}
    refuses([&]{(void)BnwRetainedTurnCursor::bind(actual);});
}
void genuine_birth_and_move(const ui::SelectedBnwPreviewHost& host){
    // Deterministic SAME storage reuse, not an allocator-luck assumption.
    alignas(LiveSession) std::array<std::byte,sizeof(LiveSession)> storage{};
    unsigned destroyed=0;
    LiveSession* const address=reinterpret_cast<LiveSession*>(storage.data());
    const auto construct=[&]{
        auto* actual=std::construct_at(address,LiveSession::from_restored_bnw(host.session().source(),host.session().state()));
        return std::shared_ptr<LiveSession>(actual,[&](LiveSession* p){std::destroy_at(p);++destroyed;});
    };
    std::weak_ptr<const LiveSession> old_life;
    {auto first=construct();old_life=first;auto cursor=BnwRetainedTurnCursor::bind(first);
     (void)cursor->execute_timer_prefix();cursor.reset();
     refuses([&]{(void)BnwRetainedTurnCursor::bind(first);});}
    need(old_life.expired()&&destroyed==1,"Actual first facade/world remained immortal");
    {auto next=construct();need(next.get()==address,"Test failed actual facade storage reuse");
     auto cursor=BnwRetainedTurnCursor::bind(next);need(!cursor->view().timer_attempted&&cursor->view().operation==0,
          "Genuine new birth inherited dead same-address history");
     need(cursor->execute_timer_prefix().operation==1,"Recycled genuine facade failed real first timer operation");}
    need(destroyed==2,"Recycled facade did not undergo genuine destruction");
    // A first request from a foreign thread cannot issue authority to itself.
    {auto session=clone(host);bool rejected=false;
     std::thread foreign([&]{try{(void)BnwRetainedTurnCursor::bind(session);}catch(const std::exception& e){
         rejected=std::string_view(e.what()).find("before live borrow")!=std::string_view::npos;}});foreign.join();
     need(rejected&&!session->has_bnw_operation_checkpoint_history(),"Foreign first issue granted token/history");
     auto cursor=BnwRetainedTurnCursor::bind(session);need(!cursor->view().timer_attempted,"Foreign first issue executed timer");}
    // Direct Timer::create bypasses the cursor ledger lock. Simultaneous
    // genuine first admission must leave its lazy slot exclusively on the
    // constructor owner thread; foreign rejection reads no lazy slot/world.
    for(int iteration=0;iteration<4;++iteration){
        auto session=clone(host);std::barrier rendezvous(2);bool rejected=false;
        const auto map_rng=session->state().random.map.state();
        const auto game_rng=session->state().random.gameplay.state();
        std::jthread foreign([&]{rendezvous.arrive_and_wait();
            try{(void)BnwGameTurnTimerOwner::create(session);}
            catch(const std::exception& e){rejected=std::string_view(e.what()).find("before live borrow")!=std::string_view::npos;}});
        rendezvous.arrive_and_wait();auto owner=BnwGameTurnTimerOwner::create(session);foreign.join();
        need(rejected,"Simultaneous foreign timer first issue accepted or borrowed live world");
        const auto sample=owner->read(owner->current_use());
        need(sample.operation==0&&sample.reset==BnwGameTurnTimerReset::construction,
             "Foreign first issue poisoned owner or ran an engine reset");
        auto peer=BnwGameTurnTimerOwner::create(session);
        need(peer->read(peer->current_use()).operation==0,
             "Simultaneous refusal changed standalone two-timer admission");
        need(session->state().random.map.state()==map_rng&&session->state().random.gameplay.state()==game_rng,
             "Simultaneous timer admission consumed actual RNG");
    }
    // Both genuinely unissued objects may move before the first operation.
    {auto source=clone(host);auto destination=std::make_shared<LiveSession>(std::move(*source));
     auto cursor=BnwRetainedTurnCursor::bind(destination);need(cursor->execute_timer_prefix().operation==1,
          "Unissued genuine move prevented first retained operation");}
    // Restore changes allocation; moving afterwards must not create fresh once history.
    {auto source=clone(host);auto cursor=BnwRetainedTurnCursor::bind(source);auto cp=source->checkpoint();
     (void)cursor->execute_timer_prefix();source->restore(cp);cursor.reset();
     auto destination=std::make_shared<LiveSession>(std::move(*source));
     refuses([&]{(void)BnwRetainedTurnCursor::bind(destination);});
     refuses([&]{(void)BnwGameTurnTimerOwner::create(destination);});}
    // Issued source -> unissued destination shares the source's retired identity.
    {auto source=clone(host);auto cursor=BnwRetainedTurnCursor::bind(source);(void)cursor->execute_timer_prefix();
     auto destination=clone(host);*destination=std::move(*source);
     refuses([&]{(void)BnwRetainedTurnCursor::bind(destination);});
     need(cursor->view().frontier==BnwRetainedTurnFrontier::quarantined,"Move assignment lost source once quarantine");}
    // Issued destination -> unissued source never replaces the destination identity.
    {auto destination=clone(host);auto cursor=BnwRetainedTurnCursor::bind(destination);
     auto source=clone(host);*destination=std::move(*source);
     refuses([&]{(void)BnwRetainedTurnCursor::bind(destination);});
     need(cursor->view().frontier==BnwRetainedTurnFrontier::quarantined&&!cursor->view().timer_attempted,
          "Destination overwrite reset original once flags");}
    // Self move assignment does not retire an otherwise current owner.
    {auto session=clone(host);auto cursor=BnwRetainedTurnCursor::bind(session);*session=std::move(*session);
     need(BnwRetainedTurnCursor::bind(session)==cursor,"Self move assignment replaced actual identity");}
    // Removing fixture workarounds releases both full Host and Session allocations.
}
void no_immortal_session(const ui::SelectedBnwPreviewHost& host){
    std::weak_ptr<const LiveSession> life;
    struct Container {std::array<std::uint8_t,4096> padding{};LiveSession session;
        explicit Container(const ui::SelectedBnwPreviewHost& h)
            :session(LiveSession::from_restored_bnw(h.session().source(),h.session().state())){}};
    {auto owner=std::make_shared<Container>(host);
     auto session=std::shared_ptr<LiveSession>(owner,&owner->session);life=session;
     auto cursor=BnwRetainedTurnCursor::bind(session);owner.reset();
     auto timer=cursor->timer_owner();(void)cursor->execute_timer_prefix();session.reset();
     need(!life.expired(),"Cursor failed to retain session");cursor.reset();
     need(!life.expired(),"Retained actual timer failed to retain session");timer.reset();}
    need(life.expired(),"Registry formed an immortal session ownership cycle");
}
void stale(const ui::SelectedBnwPreviewHost& host){
    for(bool attempted:{false,true}){
        auto session=clone(host);auto cursor=BnwRetainedTurnCursor::bind(session);
        const auto checkpoint=session->checkpoint();
        if(attempted)(void)cursor->execute_timer_prefix();
        session->restore(checkpoint);
        const auto v=cursor->view();
        need(v.frontier==BnwRetainedTurnFrontier::quarantined&&v.timer_attempted==attempted,
             "Checkpoint restore lost retained history/quarantine");
        refuses([&]{(void)cursor->execute_timer_prefix();});refuses([&]{(void)BnwRetainedTurnCursor::bind(session);});
    }
    {auto session=clone(host);auto cursor=BnwRetainedTurnCursor::bind(session);(void)cursor->execute_timer_prefix();
     auto moved=std::make_shared<LiveSession>(std::move(*session));
     refuses([&]{(void)BnwRetainedTurnCursor::bind(moved);});
     need(cursor->view().frontier==BnwRetainedTurnFrontier::quarantined,"Moved facade was dereferenced or accepted");
     *session=std::move(*moved);refuses([&]{(void)BnwRetainedTurnCursor::bind(session);});
     refuses([&]{(void)cursor->execute_timer_prefix();});}
    {auto session=clone(host);auto cursor=BnwRetainedTurnCursor::bind(session);cursor->retire();
     need(cursor->view().frontier==BnwRetainedTurnFrontier::retired,"Retired private history not observable");
     refuses([&]{(void)BnwRetainedTurnCursor::bind(session);});}
}
void identities(const ui::SelectedBnwPreviewHost& host){
    refuses([]{(void)BnwRetainedTurnCursor::bind(std::shared_ptr<const LiveSession>{});});
    refuses([]{(void)BnwRetainedTurnCursor::bind(std::shared_ptr<const ui::SelectedBnwPreviewHost>{});});
    const auto changed=[&](const std::function<void(LiveSessionState&)>& mutate){
        auto session=clone(host);auto cursor=BnwRetainedTurnCursor::bind(session);mutate(session->mutable_state());
        need(cursor->view().frontier==BnwRetainedTurnFrontier::quarantined&&!cursor->view().timer_attempted,
             "Changed identity did not refuse before reset");
        refuses([&]{(void)cursor->execute_timer_prefix();});refuses([&]{(void)BnwRetainedTurnCursor::bind(session);});
    };
    changed([](auto& s){std::get<saves::Expansion2PrefixObservation>(s.world.game_state.prefix).scalars[21]=1;});
    changed([](auto& s){s.world.map_header.guid[0]^=1;});
    changed([](auto& s){s.world.pregame->engine_guid[0]^=1;});
    changed([](auto& s){++s.setup.game_turn;});
    changed([](auto& s){s.world.pregame->archive.integers.erase("game_turn");});
    changed([](auto& s){s.world.pregame.reset();});
    changed([](auto& s){s.setup.game_type=1;});
    {auto session=clone(host);std::get<saves::Expansion2PrefixObservation>(session->mutable_state().world.game_state.prefix).scalars[21]=1;
     refuses([&]{(void)BnwRetainedTurnCursor::bind(session);});}
    {auto session=clone(host);session->mutable_state().world.pregame->archive_version=5;
     refuses([&]{(void)BnwRetainedTurnCursor::bind(session);});}
    {auto session=clone(host);auto cursor=BnwRetainedTurnCursor::bind(session);
     *session=std::move(*clone(host));refuses([&]{(void)cursor->execute_timer_prefix();});}
}
void fixture_host_release(const std::filesystem::path& save,const std::filesystem::path& game){
    std::weak_ptr<const ui::SelectedBnwPreviewHost> host_life;
    std::weak_ptr<const LiveSession> session_life;
    {auto host=cycle_host(save,game);host_life=host;
     auto session=std::shared_ptr<const LiveSession>(host,&host->session());session_life=session;
     auto cursor=BnwRetainedTurnCursor::bind(host);(void)cursor->execute_timer_prefix();}
    need(host_life.expired()&&session_life.expired(),"Fixture helpers retained full Host/session after local scope");
}
void threads(const ui::SelectedBnwPreviewHost& host){
    for(int mode=0;mode!=3;++mode){
        auto session=clone(host);auto cursor=BnwRetainedTurnCursor::bind(session);bool refused=false;
        std::thread foreign([&]{try{
            if(mode==0)(void)BnwRetainedTurnCursor::bind(session);
            else if(mode==1)(void)cursor->execute_timer_prefix();else(void)cursor->view();
        }catch(const std::exception& e){refused=std::string_view(e.what()).find("before live borrow")!=std::string_view::npos;}});
        foreign.join();need(refused,"Foreign call did not reject before live borrow");
        need(cursor->view().frontier==BnwRetainedTurnFrontier::quarantined&&!cursor->view().timer_attempted,
             "Foreign attempt lost quarantine or executed prefix");
        refuses([&]{(void)cursor->execute_timer_prefix();});
    }
}
}
int main(int argc,char** argv){try{
    static_assert(!std::is_default_constructible_v<BnwRetainedTurnCursor>);
    static_assert(!std::is_copy_constructible_v<BnwRetainedTurnCursor>);
    static_assert(!std::is_invocable_v<decltype(&BnwRetainedTurnCursor::execute_timer_prefix),BnwRetainedTurnCursor&,bool>);
    static_assert(!std::is_invocable_v<decltype(&BnwRetainedTurnCursor::execute_engine_utility_prefix),BnwRetainedTurnCursor&,bool>);
    static_assert(!std::is_invocable_v<decltype(&BnwRetainedTurnCursor::execute_engine_utility_prefix),BnwRetainedTurnCursor&,std::function<void()>>);
    static_assert(!std::is_invocable_v<decltype(&BnwRetainedTurnCursor::execute_barbarians_begin_prefix),BnwRetainedTurnCursor&,std::int64_t>);
    static_assert(!std::is_invocable_v<decltype(&BnwRetainedTurnCursor::execute_barbarians_begin_prefix),BnwRetainedTurnCursor&,std::function<void()>>);
    static_assert(!std::is_invocable_v<decltype(&BnwRetainedTurnCursor::execute_next_alive_team_war_prefix),BnwRetainedTurnCursor&,std::int32_t>);
    static_assert(!std::is_invocable_v<decltype(&BnwRetainedTurnCursor::execute_next_alive_team_war_prefix),BnwRetainedTurnCursor&,bool>);
    static_assert(!std::is_invocable_v<decltype(&BnwRetainedTurnCursor::execute_pending_team_no_trade_prefix),BnwRetainedTurnCursor&,std::int32_t>);
    static_assert(!std::is_invocable_v<decltype(&BnwRetainedTurnCursor::execute_pending_team_war_prefix),BnwRetainedTurnCursor&,bool>);
    static_assert(!std::is_invocable_v<decltype(&BnwRetainedTurnCursor::execute_pending_teams_until_boundary),BnwRetainedTurnCursor&,bool>);
    need(argc==3,"Supply selected BNW save and canonical game-assets root");
    const std::filesystem::path save=argv[1];const auto original=identify_file(save).sha256;
    auto bootstrap=ui::bootstrap_selected_bnw_preview(save,std::filesystem::path(argv[2]),0);
    if(bootstrap.preview.blocker)throw std::runtime_error(bootstrap.preview.blocker->reason);
    need(static_cast<bool>(bootstrap.preview.host),"Actual selected host absent");
    std::shared_ptr<const ui::SelectedBnwPreviewHost> host(std::move(bootstrap.preview.host));
    const CycleRuleFixture rules_fixture(bootstrap,*host,save,std::filesystem::path(argv[2]),original);
    actual_deals(save,std::filesystem::path(argv[2]));synthetic_deals(save,std::filesystem::path(argv[2]));resource_total_cases(save,std::filesystem::path(argv[2]));renewed_expiry_cases(save,std::filesystem::path(argv[2]));actual_first_team_war(save,std::filesystem::path(argv[2]));next_team_war_codec_cases(save,std::filesystem::path(argv[2]));team_queue_codec_cases(save,std::filesystem::path(argv[2]));team_war_codec_cases(save,std::filesystem::path(argv[2]));actual_first_no_trade(save,std::filesystem::path(argv[2]));no_trade_codec_cases(save,std::filesystem::path(argv[2]));deal_guards(save,std::filesystem::path(argv[2]));actual_score(save,std::filesystem::path(argv[2]));diagnostic_clock_score_fixture(save,std::filesystem::path(argv[2]));score_guards(save,std::filesystem::path(argv[2]));actual_religion_cache(save,std::filesystem::path(argv[2]));game_cache_guards(save,std::filesystem::path(argv[2]));actual_barbarians(save,std::filesystem::path(argv[2]));barbarian_guards(save,std::filesystem::path(argv[2]));nonstock_public_factory(save,std::filesystem::path(argv[2]));actual_cycle(save,std::filesystem::path(argv[2]));utility_guards(save,std::filesystem::path(argv[2]));prefix(host);aliases_and_lifetime(*host);alias_expiry(*host);stale(*host);identities(*host);threads(*host);no_immortal_session(*host);genuine_birth_and_move(*host);fixture_host_release(save,std::filesystem::path(argv[2]));
    need(identify_file(save).sha256==original,"Original source file changed");
    std::cout<<"BNW retained timer + SAME UI + exact retail403694 empty utility PASS; actual barbarian counters; frontier BEFORE caches/full turn unavailable\n";return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
