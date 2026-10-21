#include "civ5/runtime/bnw_activation_happiness.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <iostream>
using namespace civ5;using namespace civ5::runtime;
namespace {
void check(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
auto &named(auto &rows,std::string_view name){for(auto &r:rows)if(r.name&&name==r.name)return r;throw std::runtime_error("Missing activation fixture field");}
// Isolated formula/escrow fixture. These flags attest ONLY this fixture context,
// never the selected stock lifecycle (which remains missing in production).
struct FixtureEpochOwner final:BnwGameplayLuaEpochOwner {
    SavedContentProfile profile;SessionCheckpointIdentity identity;std::int64_t player=0;BnwTeamTechLuaEpoch *last_epoch=nullptr;
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
        e->host->execute("fixture-only",script+"\n"+ui_script,"=fixture-selected-prefix");e->host->complete_loading();last_epoch=e.get();return {std::move(e),{}};
    }
    BnwGameplayLuaPublishResult publish(std::unique_ptr<BnwTeamTechLuaEpoch>,LiveSessionState &) override {++publishes;return {false,false,"fixture never publishes"};}
};
}
int main(int argc,char **argv){
 check(argc==3||argc==4,"Expected selected save installation [SDKroot]");if(argc==4)check(identify_file(std::filesystem::path(argv[3])/"CvGameCoreDLL_Expansion2/CvPlayer.cpp").sha256=="9635fe516e590e133c3907ee25c13128fb666e3e7817c967e247affb700a952d","Source activation/happiness SDK differs");
 auto selected=ui::bootstrap_selected_bnw_preview(argv[1],argv[2],0);check(static_cast<bool>(selected.preview),"Selected bootstrap failed");auto &host=*selected.preview.host;const auto p=host.active_player_id();const auto &source=host.session().state();const auto original=source.unhappiness.players.at(p).happiness;
 auto state=source;auto connections=make_bnw_activation_connections_import_owner(host.session().source(),host.identity());check(update_bnw_activation_city_connections(state,connections,host.database(),p).outcome==LiveActionOutcome::committed,"Real source connections failed");check(update_bnw_activation_city_connection_gold(state,connections,p).outcome==LiveActionOutcome::committed,"Real source connection gold failed");
 // Isolated activation-prefix receipt fixture: genuine imported active human
 // remains rejected by enclosing activation preflight. This fixture verifies
 // ONLY binding/lifetime on actual selected state, not full activation parity.
 using S=BnwGameActivationStep;const std::vector<S> done={S::set_turn_active,S::publish_end_turn_dirty,S::clear_end_turn,S::unit_attrition,S::active_player_deferred_fog,S::active_turn_count_log,S::publish_player_turn_start,S::all_units_unprocessed,S::pathfinder_cache_enter,S::city_connections,S::city_connection_gold};
 named(state.world.all_players->players.at(p).core.fields,"m_bEndTurn").values={0};state.player_read_tail.active_player_ids={p};
 check(!stage_bnw_activation_happiness(state,connections,host.database(),host.identity(),p,{}).owner,"Imported active shortcut accepted without native prefix receipt");
 auto missing=connections;missing.players.at(p).updated_turn.reset();check(!stage_bnw_activation_happiness(state,missing,host.database(),host.identity(),p,done).owner,"Happiness accepted without SAME current source connection update");
 auto prepared=stage_bnw_activation_happiness(state,connections,host.database(),host.identity(),p,done);if(!prepared.owner)throw std::runtime_error(prepared.blocker);auto &o=*prepared.owner;check(o.native_prefix_executions()==1&&o.state().unhappiness.players.at(p).happiness==12&&!o.native_update_complete(),"Retained native prefix not stored once");const auto *prefix_world=&o.state();
 FixtureEpochOwner fixture;fixture.identity=host.identity();fixture.profile=selected.profile;fixture.player=p;fixture.missing=true;
 check(continue_bnw_activation_happiness(o,selected.profile,fixture,host.database()).outcome==LiveActionOutcome::rejected&&o.stage()==BnwActivationHappinessStage::awaiting_selected_lua&&&o.state()==prefix_world&&o.religion_dispatches()==0,"Missing stock epoch discarded/repeated native prefix");
 fixture.missing=false;fixture.verified=false;check(continue_bnw_activation_happiness(o,selected.profile,fixture,host.database()).outcome==LiveActionOutcome::rejected&&&o.state()==prefix_world&&o.religion_dispatches()==0,"Partial registry dispatched or lost retained source prefix");
 fixture.verified=true;fixture.declare_ui=true;fixture.ui_script="Events.SerialEventGameDataDirty.Add(function()print('activation-private-ui')end)";
 check(continue_bnw_activation_happiness(o,selected.profile,fixture,host.database()).outcome==LiveActionOutcome::committed&&o.native_update_complete()&&o.religion_dispatches()==1&&o.native_prefix_executions()==1&&fixture.forks==3&&!o.ui_consumed(),"Sole retained activation epoch not completed natively");const auto *sole_world=&o.state();
 check(continue_bnw_activation_happiness(o,selected.profile,fixture,host.database()).outcome==LiveActionOutcome::rejected&&fixture.forks==3&&&o.state()==sole_world,"Native DoUpdate/religion fork repeated after completion");
 fixture.last_epoch->all_listeners_registered=false;
 check(consume_bnw_activation_happiness_ui(o,host.database()).outcome==LiveActionOutcome::rejected&&o.stage()==BnwActivationHappinessStage::native_suffix_dirty_pending&&o.native_update_complete()&&!o.ui_consumed()&&&o.state()==sole_world&&o.religion_dispatches()==1&&fixture.forks==3,"Pre-dispatch UI refusal destroyed accepted activation world or replayed native/Lua prefix");
 fixture.last_epoch->all_listeners_registered=true;
 const auto delivered=consume_bnw_activation_happiness_ui(o,host.database());if(delivered.outcome==LiveActionOutcome::rejected)throw std::runtime_error(delivered.reason);check(delivered.outcome==LiveActionOutcome::committed&&o.ui_consumed()&&&o.state()==sole_world&&fixture.forks==3&&fixture.publishes==0,"Deferred UI consumed another VM/world or published independently");
 check(consume_bnw_activation_happiness_ui(o,host.database()).outcome==LiveActionOutcome::rejected,"Activation UI receipt replayed");
 auto failed=stage_bnw_activation_happiness(state,connections,host.database(),host.identity(),p,done);fixture.script="GameEvents.GetFounderBenefitsReligion.Add(function(p)Map.Rand(1,'write');error('late-activation')end)";fixture.mutate=5;
 check(continue_bnw_activation_happiness(*failed.owner,selected.profile,fixture,host.database()).outcome==LiveActionOutcome::rejected&&failed.owner->stage()==BnwActivationHappinessStage::failed&&!failed.owner->native_update_complete()&&failed.owner->state().unhappiness.players.at(p).happiness==12,"Failure retained effectful Lua world");const auto attempts=fixture.forks;
 check(continue_bnw_activation_happiness(*failed.owner,selected.profile,fixture,host.database()).outcome==LiveActionOutcome::rejected&&fixture.forks==attempts,"Effectful failed activation dispatched again");
 check(source.unhappiness.players.at(p).happiness==original&&!host.session().readiness().can_continue(),"Retained owner changed source/readiness");
 std::cout<<"PASS isolated activation-prefix binding on actual selected state; native prefix once, missing/partial stock epoch retained, SAME single fixture religion/UI VM, no retry/replay/publish and atomic error discard\n";
}
