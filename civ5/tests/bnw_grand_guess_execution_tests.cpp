#include "civ5/runtime/bnw_game_activation_execution.hpp"
#include "civ5/graphics/bnw_deferred_fog_client.hpp"
#include "civ5/graphics/bnw_scene_restore_receipt.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/bnw_player_pre_turn.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <algorithm>
#include <iostream>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
using namespace civ5;using namespace civ5::runtime;
namespace {
void check(bool b,const char *why){if(!b)throw std::runtime_error(why);}
auto &named(auto &rows,std::string_view name){for(auto &r:rows)if(r.name&&name==r.name)return r;throw std::runtime_error("Native activation fixture field absent");}
// Isolated formula/escrow fixture. These flags attest ONLY this fixture context,
// never the selected stock lifecycle (which remains missing in production).
struct FixtureEpochOwner final:BnwGameplayLuaEpochOwner {
    SavedContentProfile profile;SessionCheckpointIdentity identity;std::int64_t player=0;
    bool missing=false,verified=true,declare_ui=false;std::string ui_script;int forks=0,publishes=0;std::int32_t observed=-999,mutate=0;std::function<void(LiveSessionState &)> on_random;
    std::string script="GameEvents.GetFounderBenefitsReligion.Add(function(p)Map.Rand(1,'observe');return -1 end)";
    BnwGameplayLuaForkResult fork_at(const LiveSessionState &state) override {
        ++forks;if(missing)return {{},"required selected stock contexts/listeners not loaded"};
        auto e=std::make_unique<BnwTeamTechLuaEpoch>();e->world=std::make_unique<LiveSessionState>(state);e->source_world=&state;e->getter_world=e->world.get();
        e->identity=identity;e->selected_dlc=profile.enabled_dlc;e->selected_mods=profile.enabled_mods;
        e->all_selected_contexts_loaded=verified;e->all_listeners_registered=verified;e->all_external_writes_escrowed=verified;
        ScriptMapProvider map;map.grid_size=[]{return std::pair<std::int64_t,std::int64_t>{2,1};};
        map.random=[this,w=e->world.get()](std::int64_t,std::string_view) {
            auto &f=named(w->world.all_players->players.at(player).core.fields,"m_iHappiness");observed=f.values[0];
            f.values[0]+=mutate;w->unhappiness.players.at(player).happiness+=mutate;if(on_random)on_random(*w);return std::int64_t{0};
        };
        ScriptUiEventProvider ui;if(declare_ui)ui.notifications={"SerialEventGameDataDirty"};
        e->host=std::make_unique<LuaScriptHost>(ScriptBudget{8*1024*1024,1000000},ScriptSourceProvider{},ScriptModdingProvider{},ScriptGameplayProvider{},ScriptSaveDataProvider{},ScriptPregameProvider{},std::move(map),std::move(ui));
        e->host->execute("fixture-only",script+"\n"+ui_script,"=fixture-selected-prefix");e->host->complete_loading();return {std::move(e),{}};
    }
    BnwGameplayLuaPublishResult publish(std::unique_ptr<BnwTeamTechLuaEpoch>,LiveSessionState &) override {++publishes;return {false,false,"fixture never publishes"};}
};
}
int main(int argc,char **argv)try{
#ifdef _WIN32
 ::SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX|SEM_NOOPENFILEERRORBOX);
#endif
 check(argc==3||argc==4,"Expected selected save installation [SDKroot]");
 if(argc==4)for(const auto &pin:kBnwGrandGuessEmptySources)check(identify_file(std::filesystem::path(argv[3])/pin.path).sha256==pin.sha256,"Grand DoGuess source differs");
 auto selected=ui::bootstrap_selected_bnw_preview(argv[1],argv[2],0);check(static_cast<bool>(selected.preview),"Selected bootstrap unavailable");auto &host=*selected.preview.host;const auto p=host.active_player_id();const auto public_lease=host.session().read_state_lease();const auto public_generation=host.session().state_generation();const auto &original=*public_lease;
 auto art=saves::observe_saved_map_team_art(*original.world.pregame,*original.world.map_plots);const auto grid=original.world.map_header.grid;materials::FogOfWarState client(grid.width,grid.height,{},-1000.f,1000.f);const auto feed=scene::apply_saved_map_fog(grid,art,client);client.tick(0.f);
 auto scene_receipt=graphics::capture_bnw_renderer_scene_receipt(argv[1],identify_file(argv[1]).sha256,host.session().source(),art);check(scene_receipt.has_value(),"Actual renderer receipt missing");auto projection=*host.session().source().snapshot.map_projection;check(graphics::reconcile_bnw_renderer_scene_projection(projection,*scene_receipt,original),"Actual scene projection failed");const auto baseline=graphics::capture_bnw_fog_client_baseline(original,host.identity(),*scene_receipt,feed,client,"native-activation-happiness",false);
 BnwGameActivationOwner owner{};owner.identity=host.identity();owner.deferred_fog=make_bnw_deferred_fog_owner(baseline);owner.city_connections=make_bnw_activation_connections_import_owner(host.session().source(),host.identity());owner.pathfinder_cache=make_bnw_pathfinder_cache_control_import_owner(host.session().source(),host.identity());owner.builder_tasking=bind_bnw_activation_builder_original_import(host.session().source(),host.identity(),{false,false,false});auto diplomacy_read=attest_bnw_diplomacy_read_context(original,host.session().source(),host.identity(),host.database());owner.diplomacy_requests=bind_bnw_diplomacy_turn_read(original,std::move(diplomacy_read),host.database());
 const auto game_order=bnw_game_turn_order();std::vector<BnwTurnPhase> prefix(game_order.begin(),std::find(game_order.begin(),game_order.end(),BnwTurnPhase::game_activation));auto handlers=make_bnw_game_activation_source_player_handlers(host.database(),host.identity(),p);
 auto imported=execute_bnw_game_activation_until_happiness(original,owner,host.database(),host.identity(),prefix,handlers);check(!imported.draft&&!imported.pending_happiness&&imported.completed_steps.empty(),"Imported active human re-entered activation/DoTurn");
 // Isolated post-deactivation activation input fixture; actual native services
 // below execute on the selected source, not callbacks declaring fake success.
 auto source=original;for(auto &player:source.world.all_players->players)named(player.core.fields,"m_bTurnActive").values={0};named(source.world.all_players->players.at(p).core.fields,"m_bEndTurn").values={1};named(source.world.all_players->players.at(p).core.fields,"m_bAutoMoves").values={1};for(auto &unit:source.world.all_players->players.at(p).units.units)named(unit.core.fields,"m_bAITurnProcessed").values={1};

 FixtureEpochOwner lua;lua.profile=selected.profile;lua.identity=host.identity();lua.player=p;lua.missing=true;
 auto flow=execute_bnw_game_activation_until_happiness(source,owner,host.database(),host.identity(),prefix,handlers);
 check(flow.pending_happiness&&!flow.draft&&flow.completed_steps.size()==11,"Actual native activation prefix missing");
 check(continue_bnw_game_activation_happiness(flow,selected.profile,lua,host.database()).outcome==LiveActionOutcome::rejected&&flow.completed_steps.size()==11,"Missing stock Lua bypassed/replayed prior prefix");
 check(continue_bnw_game_activation_grand_world_averages(flow,host.database()).outcome==LiveActionOutcome::rejected,"Averages bypassed mandatory actual happiness/Builder/DoTurn");
 LiveSessionState *same_epoch_world=nullptr;lua.on_random=[&](LiveSessionState &w){same_epoch_world=&w;};lua.missing=false;lua.declare_ui=true;lua.ui_script="Events.SerialEventGameDataDirty.Add(function()end)";
 auto applied=continue_bnw_game_activation_happiness(flow,selected.profile,lua,host.database());check(applied.outcome==LiveActionOutcome::committed,applied.reason.c_str());
 check(continue_bnw_game_activation_player_prefix(flow,host.database()).outcome==LiveActionOutcome::committed,"Actual human prefix failed");
 check(continue_bnw_game_activation_player_revolts(flow,host.database()).outcome==LiveActionOutcome::committed,"Actual selected revolt/Maya branch failed");
 check(continue_bnw_game_activation_player_trade(flow,host.database()).outcome==LiveActionOutcome::committed,"Actual selected Trade failed");
 check(continue_bnw_game_activation_player_military(flow,host.database()).outcome==LiveActionOutcome::committed,"Actual Military Reset failed");
 check(continue_bnw_game_activation_grand_military_average(flow,host.database()).outcome==LiveActionOutcome::committed,"Actual selected cached military average failed");
 const auto &retained=flow.pending_happiness->activation_owner();const auto *world=&flow.pending_happiness->state();check(same_epoch_world==world&&lua.forks==2&&!lua.publishes,"Earlier native stages changed sole Lua epoch/world or published");
 const auto military=retained.player_do_turn_military->after_world_revision(),trade=retained.player_do_turn_trade->after_world_revision();
 const auto grandfather=bnw_grand_native_component_revision(*world,p);const auto game_rng=world->random.gameplay.state(),map_rng=world->random.map.state();
 check(continue_bnw_game_activation_grand_guess_empty(flow,host.database()).outcome==LiveActionOutcome::rejected&&!retained.grand_guess_empty,"Empty Guess accepted without private world-average journal");
 check(continue_bnw_game_activation_grand_own_culture(flow,selected.profile,host.database()).outcome==LiveActionOutcome::rejected&&!flow.pending_happiness->happiness().total_culture(),"Own culture bypassed mandatory DoGuess/averages");
 auto averaged=continue_bnw_game_activation_grand_world_averages(flow,host.database());check(averaged.outcome==LiveActionOutcome::committed,averaged.reason.c_str());
 check(retained.grand_world_averages&&retained.grand_world_averages->averages().culture_average==0&&retained.grand_world_averages->averages().tourism_average==0&&retained.grand_world_averages->averages().tech_average==1&&!retained.grand_guess_empty,"Private producer did not execute actual selected city source queries");
 const auto *average_address=&*retained.grand_world_averages;const auto query_revision=retained.grand_world_averages->query_revision();
 check(continue_bnw_game_activation_grand_world_averages(flow,host.database()).outcome==LiveActionOutcome::rejected&&&*retained.grand_world_averages==average_address,"Accepted query replayed/replaced private journal");
 // A late current-city cache change must retain averages/world/VM and refuse
 // only the next suffix. Restoring this explicit fixture operand permits retry.
 auto &cache=named(named(same_epoch_world->world.all_players->players.at(p).cities.cities.at(0).production.blocks,"buildings").fields,"m_iGreatWorksTourismModifier");const auto original_cache=cache.value;++cache.value;
 auto stale=continue_bnw_game_activation_grand_guess_empty(flow,host.database());check(stale.outcome==LiveActionOutcome::rejected&&stale.reason.find("world-average query inputs stale")!=std::string::npos&&!retained.grand_guess_empty&&&*retained.grand_world_averages==average_address&&retained.grand_world_averages->query_revision()==query_revision&&world==&flow.pending_happiness->state()&&lua.forks==2,"Late stale suffix erased/recomputed accepted averages/VM or created receipt");
 cache.value=original_cache;
 auto guessed=continue_bnw_game_activation_grand_guess_empty(flow,host.database());check(guessed.outcome==LiveActionOutcome::committed,guessed.reason.c_str());
 check(retained.grand_guess_empty&&retained.grand_guess_empty->inspected().size()==22&&retained.grand_guess_empty->inspected().at(1).met==false&&retained.grand_guess_empty->query_revision()==query_revision,"Actual source no-eligible22 loop receipt missing");
 check(continue_bnw_game_activation_grand_guess_empty(flow,host.database()).outcome==LiveActionOutcome::rejected&&retained.grand_guess_empty&&&*retained.grand_world_averages==average_address&&lua.forks==2,"Empty source loop replayed or journal/epoch replaced");
 check(world==&flow.pending_happiness->state()&&world->random.gameplay.state()==game_rng&&world->random.map.state()==map_rng&&bnw_grand_native_component_revision(*world,p)==grandfather&&retained.player_do_turn_trade->after_world_revision()==trade&&retained.player_do_turn_military->after_world_revision()==military&&retained.diplomacy_requests->operations()==1&&flow.completed_steps.size()==14&&flow.completed_player_phases==std::vector{BnwTurnPhase::player_cache,BnwTurnPhase::player_ai_pre}&&flow.player_turn_calls==1&&!flow.unit_turn_calls&&!flow.draft&&retained.completed.empty()&&host.session().state_generation()==public_generation,"Empty Guess altered prior receipts/native Grand/RNG or certified fullturn/publication");
 check(guessed.reason.find("own Grand DoTurn")!=std::string::npos&&guessed.reason.find("NO Grand/phase3/step15/fullturn")!=std::string::npos&&!flow.pending_happiness->happiness().ui_consumed(),"Mandatory downstream native/Lua/UI frontiers lost");
 auto foreign_profile=selected.profile;foreign_profile.enabled_dlc.clear();
 check(continue_bnw_game_activation_grand_own_culture(flow,foreign_profile,host.database()).outcome==LiveActionOutcome::rejected&&!flow.pending_happiness->happiness().total_culture()&&world==&flow.pending_happiness->state(),"Foreign profile consumed the enclosing epoch or retained a fake culture query");
 const auto total_result=continue_bnw_game_activation_grand_own_culture(flow,selected.profile,host.database());
 check(total_result.outcome==LiveActionOutcome::committed,total_result.reason.c_str());
 const auto *total=flow.pending_happiness->happiness().total_culture();
 const auto *first=flow.pending_happiness->grand_first_prefix();
 check(total&&total->status()==BnwTotalJonsCultureStatus::complete&&!total->epoch()&&
    total->quote().total==1&&total->quote().founder_calls==1&&first&&first->base==50&&
    first->culture_initial==100&&!first->category_quoted&&!first->before_random&&
    world==&flow.pending_happiness->state()&&lua.forks==2&&!lua.publishes,
    "Actual total culture/GetBase prefix lost SAME epoch, source ordering or mandatory category boundary");
 check(continue_bnw_game_activation_grand_own_culture(flow,selected.profile,host.database()).outcome==LiveActionOutcome::rejected&&
    total==flow.pending_happiness->happiness().total_culture()&&total->quote().founder_calls==1&&
    world->random.gameplay.state()==game_rng&&world->random.map.state()==map_rng&&
    flow.completed_steps.size()==14&&flow.completed_player_phases.size()==2&&!flow.draft&&
    retained.completed.empty()&&host.session().state_generation()==public_generation,
    "Completed culture replayed callback/RNG or granted Grand/fullturn/publication");
 // Exercise the enclosing failure path, after the real retained prefix.
 // A started hook must never expose the earlier rollback world as current.
 FixtureEpochOwner failing;failing.profile=selected.profile;failing.identity=host.identity();
 failing.player=p;failing.declare_ui=true;failing.ui_script="Events.SerialEventGameDataDirty.Add(function()end)";
 auto failed_flow=execute_bnw_game_activation_until_happiness(source,owner,host.database(),host.identity(),prefix,handlers);
 check(continue_bnw_game_activation_happiness(failed_flow,selected.profile,failing,host.database()).outcome==LiveActionOutcome::committed,"Failure fixture actual happiness prefix failed");
 check(continue_bnw_game_activation_player_prefix(failed_flow,host.database()).outcome==LiveActionOutcome::committed,"Failure fixture actual player prefix failed");
 check(continue_bnw_game_activation_player_revolts(failed_flow,host.database()).outcome==LiveActionOutcome::committed,"Failure fixture actual revolt prefix failed");
 check(continue_bnw_game_activation_player_trade(failed_flow,host.database()).outcome==LiveActionOutcome::committed,"Failure fixture actual Trade prefix failed");
 check(continue_bnw_game_activation_player_military(failed_flow,host.database()).outcome==LiveActionOutcome::committed,"Failure fixture actual Military prefix failed");
 check(continue_bnw_game_activation_grand_military_average(failed_flow,host.database()).outcome==LiveActionOutcome::committed,"Failure fixture actual military average failed");
 check(continue_bnw_game_activation_grand_world_averages(failed_flow,host.database()).outcome==LiveActionOutcome::committed,"Failure fixture actual world averages failed");
 check(continue_bnw_game_activation_grand_guess_empty(failed_flow,host.database()).outcome==LiveActionOutcome::committed,"Failure fixture actual DoGuess failed");
 unsigned failed_calls=0;failing.on_random=[&](LiveSessionState &){++failed_calls;throw std::runtime_error("culture fixture callback failure");};
 check(continue_bnw_game_activation_grand_own_culture(failed_flow,selected.profile,host.database()).outcome==LiveActionOutcome::rejected&&
    failed_calls==1&&failed_flow.pending_happiness->happiness().stage()==BnwActivationHappinessStage::failed&&
    failed_flow.pending_happiness->happiness().total_culture()->status()==BnwTotalJonsCultureStatus::discarded,
    "Dispatched culture failure did not invalidate enclosing attempt");
 bool state_refused=false;
 try{(void)failed_flow.pending_happiness->state();}catch(const std::runtime_error &){state_refused=true;}
 check(state_refused&&continue_bnw_game_activation_grand_own_culture(failed_flow,selected.profile,host.database()).outcome==LiveActionOutcome::rejected&&
    continue_bnw_game_activation_grand_world_averages(failed_flow,host.database()).outcome==LiveActionOutcome::rejected&&
    continue_bnw_game_activation_happiness(failed_flow,selected.profile,failing,host.database()).outcome==LiveActionOutcome::rejected&&
    failed_calls==1&&failing.forks==1&&!failing.publishes&&!failed_flow.draft&&
    host.session().state_generation()==public_generation,
    "Discarded culture exposed old state, replayed hooks/prefix or published");
 std::cout<<"PASS SAME retained registered-fixture epoch native activation14->playerprefix/revolts/Trade/Military->sealed averages once->actual source emptyGuess22->GetBase/Culture prefix and total culture1; stale/profile refusal/no replay/dispatched failure quarantine; no selected-stock/fullGrand/step15/readiness grant\n";
}catch(const std::exception &e){std::cerr<<"Grand retained executor FAIL: "<<e.what()<<'\n';return 1;}
