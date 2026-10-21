#include "civ5/runtime/bnw_happiness_ui_pump.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <iostream>
using namespace civ5;using namespace civ5::runtime;
namespace {
void check(bool ok,const char *why) {if(!ok)throw std::runtime_error(why);}
template<class F>void rejects(F &&f,const char *why) {try{f();}catch(const std::exception &){return;}throw std::runtime_error(why);}
auto &named(auto &rows,std::string_view name) {for(auto &row:rows)if(row.name && name==row.name)return row;throw std::runtime_error("Missing happiness fixture field");}
void scalar(auto &fields,const char *name,std::int64_t n) {typename std::remove_reference_t<decltype(fields)>::value_type f{};f.name=name;f.values={n};fields.push_back(f);}
saves::CityTailNode node(const char *name,std::int64_t n) {saves::CityTailNode v{};v.name=name;v.encoding="i32";v.values={n};return v;}
void add_city(LiveSessionState &s,std::int64_t p,std::int64_t id,std::int32_t population,std::int32_t base,std::int32_t global,
              const content::InfoTypeEntry &generator,const content::InfoTypeEntry &target,std::int32_t rg,std::int32_t fg,std::int32_t rt,std::int32_t ft) {
    constexpr auto bnw=saves::GameStateLayout::expansion2_version1;saves::SavedCityObservation raw{};
    raw.core.layout=bnw;raw.core.version=6;raw.core.start_offset=2000+id*100;raw.core.end_offset=raw.core.start_offset+80;
    scalar(raw.core.fields,"m_iID",id);scalar(raw.core.fields,"m_eOwner",p);scalar(raw.core.fields,"m_iPopulation",population);scalar(raw.core.fields,"m_bPuppet",0);
    raw.production.layout=bnw;raw.production.context.bnw_city_version=6;
    saves::CityProductionRecord buildings{};buildings.name="buildings";buildings.fields={{"version",1}};
    CityYieldState live;live.owner=p;live.id=id;live.population=population;live.building_counts_resolved=true;live.source_core=std::pair{raw.core.start_offset,raw.core.end_offset};
    const auto counts=[&](const char *name,std::int32_t g,std::int32_t t,YieldValues &current) {
        saves::CityProductionRecord a{};a.name=name;a.fields={{"wire_count",3}};
        for(const auto &[hash,value]:std::vector<std::pair<std::uint32_t,std::int32_t>>{{generator.hash,g},{target.hash,t},{target.hash,t}}) {
            saves::CityProductionRecord row{};row.name="row";row.fields={{"hash",hash}};row.arrays={{"values",1,{value}}};a.children.push_back(row);
        }buildings.children.push_back(a);current[generator.id]=g;current[target.id]=t;
    };counts("m_paiNumRealBuilding",rg,rt,live.real_buildings);counts("m_paiNumFreeBuilding",fg,ft,live.free_buildings);
    saves::CityProductionRecord cache{};cache.name="before_strategy";cache.fields={{"m_iBaseHappinessFromBuildings",base},{"m_iUnmoddedHappinessFromBuildings",global}};
    raw.production.blocks={buildings,cache};raw.tail.layout=bnw;raw.tail.context.bnw_city_version=6;
    saves::CityTailNode rel{};rel.name="religions";rel.children={node("version",3)};
    saves::CityTailNode status{};status.name="m_ReligionStatus";status.wire_count=0;rel.children.push_back(status);raw.tail.blocks={rel};
    auto &collection=s.world.all_players->players.at(p).cities;collection.cities.push_back(raw);collection.header.wire_city_count=collection.cities.size();s.cities.players.at(p).cities[id]=live;
}
}

namespace {
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

namespace {
void city_operands(LiveSessionState &s,std::int64_t p,std::int64_t id,std::int64_t x){auto &collection=s.world.all_players->players.at(p).cities;auto &saved=collection.cities.back();check(named(saved.core.fields,"m_iID").values[0]==id,"City fixture identity differs");
 scalar(saved.core.fields,"m_iX",x);scalar(saved.core.fields,"m_iY",0);scalar(saved.core.fields,"m_bOccupied",0);scalar(saved.core.fields,"m_iNoOccupiedUnhappinessCount",0);saved.management.layout=saves::GameStateLayout::expansion2_version1;saves::CityManagementBlock citizens{};citizens.name="citizens";saves::TeamHashedArray specialists{};specialists.name="m_aiSpecialistCounts";specialists.wire_count=0;citizens.hashes={specialists};saved.management.blocks={citizens};
 auto &city=s.cities.players.at(p).cities.at(id);city.x=x;city.y=0;s.unhappiness.players.at(p).cities[id]={};auto &cell=s.world.map_projection->cells.at(x);cell.owner=p;cell.is_city=true;cell.plot_city=saves::SavedPlotCityReference{p,id};cell.feature_type=-1;cell.feature_type_hash.reset();auto &plot=s.world.map_plots->plots.at(x);named(plot.fields,"plot_city_owner").value=p;named(plot.fields,"plot_city_id").value=id;named(plot.fields,"owner").value=p;
}
}
int main(int argc,char **argv){
 check(argc==3||argc==4,"Expected selected save, installation and optional SDK source root");if(argc==4){const auto sdk=std::filesystem::path(argv[3]);check(identify_file(sdk/"CvGameCoreDLL_Expansion2/CvPlayer.cpp").sha256=="9635fe516e590e133c3907ee25c13128fb666e3e7817c967e247affb700a952d","Native happiness source order SDK differs");check(identify_file(sdk/"CvGameCoreDLL_Expansion2/CvTreasury.cpp").sha256=="31ac211d20fa658bbac11cbc0029af5f90b917308b579887aeda5af1d1af63a2","Native cached route getter SDK differs");check(identify_file(sdk/"CvGameCoreDLL_Expansion2/CvCityConnections.cpp").sha256=="627feb0c0bcf0d435a02b75853ac1704a6f56db7851ebf98e26ef513596f9a36","Native route serialization SDK differs");check(identify_file(sdk/"FirePlace/include/FireWorks/FFreeListTrashArray.h").sha256=="957dd24055204895ab479f3a1577113c60f1a18303a9bb4cf9c5a4b716c507f8","Native capital FLTA lookup SDK differs");}
 auto selected=ui::bootstrap_selected_bnw_preview(argv[1],argv[2],0);check(static_cast<bool>(selected.preview),"Suffix selected bootstrap failed");auto &host=*selected.preview.host;const auto p=host.active_player_id();const auto &source=host.session().state();const auto original=source.unhappiness.players.at(p).happiness;
 const auto prefix_rules=BnwHappinessPrefixRules::load(host.database());const auto suffix_rules=BnwHappinessSuffixRules::load(host.database());const auto religion_rules=BnwReligionHappinessRules::load(host.database());
 auto input=attest_bnw_happiness_prefix_input(source,prefix_rules,host.identity(),p);auto prefix=execute_bnw_happiness_native_prefix(source,input,prefix_rules);check(static_cast<bool>(prefix.prefix),"Selected suffix prefix unavailable");auto client=start_bnw_happiness_game_data_client(source,prefix_rules,host.identity(),p);
 FixtureEpochOwner fixture;fixture.profile=selected.profile;fixture.identity=host.identity();fixture.player=p;fixture.missing=true;
 auto blocked=execute_bnw_happiness_remaining_suffix(*prefix.prefix,prefix_rules,selected.profile,fixture,religion_rules,suffix_rules,client,host.database());check(!blocked.native_cache_suffix_complete&&!blocked.continuation.religion.query.epoch&&!blocked.receiver&&!client.dirty(),"Missing selected stock epoch bypassed or dirty consumed");
 fixture.missing=false;auto selected_fixture=execute_bnw_happiness_remaining_suffix(*prefix.prefix,prefix_rules,selected.profile,fixture,religion_rules,suffix_rules,client,host.database());
 if(!selected_fixture.native_cache_suffix_complete)throw std::runtime_error("Isolated registered callback on actual selected world: "+selected_fixture.frontier);
 check(selected_fixture.continuation.religion.query.epoch&&selected_fixture.receiver&&selected_fixture.receiver->dirty()&&selected_fixture.receiver->pending_fanout()&&!selected_fixture.mandatory_game_data_fanout_complete&&fixture.publishes==0&&!client.dirty(),"Private native suffix/dirty fanout ownership differs");
 auto db=host.database().clone_gameplay();db->apply_sql("UPDATE HandicapInfos SET HappinessDefault=7,NumCitiesUnhappinessMod=100,PopulationUnhappinessMod=100,AIUnhappinessPercent=100; UPDATE Policies SET ExtraHappiness=0,ExtraHappinessPerCity=0; DELETE FROM Building_BuildingClassHappiness;DELETE FROM Policy_BuildingClassHappiness;"
 "UPDATE Defines SET Value=2 WHERE Name='HAPPINESS_PER_NATURAL_WONDER'; UPDATE Defines SET Value=2 WHERE Name LIKE 'FRIENDS_HAPPINESS_FLAT_BONUS_AMOUNT_%'; UPDATE Defines SET Value=5 WHERE Name LIKE 'ALLIES_HAPPINESS_FLAT_BONUS_AMOUNT_%'; UPDATE Defines SET Value=1 WHERE Name LIKE 'FRIENDS_HAPPINESS_PER_LUXURY_BONUS_AMOUNT_%'; UPDATE Defines SET Value=3 WHERE Name LIKE 'ALLIES_HAPPINESS_PER_LUXURY_BONUS_AMOUNT_%'; UPDATE Resources SET Happiness=4 WHERE Type='RESOURCE_GOLD';");
 const auto pr=BnwHappinessPrefixRules::load(*db);const auto sr=BnwHappinessSuffixRules::load(*db);const auto rr=BnwReligionHappinessRules::load(*db);auto identity=host.identity();identity.rules_sha256=pr.rules_sha256();auto state=source;const auto types=db->cache_info_types();const auto entry=[&](const char *type)->const content::InfoTypeEntry &{const auto *e=types.find(type);check(e,"Suffix fixture type absent");return *e;};
 const auto &gen=entry("BUILDING_MONUMENT"),&target=entry("BUILDING_GRANARY"),&gold=entry("RESOURCE_GOLD");auto &raw=state.world.all_players->players.at(p);raw.cities.cities.clear();raw.cities.header.wire_city_count=0;state.cities.players.at(p).cities.clear();state.unhappiness.players.at(p).cities.clear();
 for(const char *name:{"m_iHappinessPerGarrisonedUnitCount","m_iHappinessPerXPopulation","m_iHappinessPerXPolicies","m_iHappinessPerCity","m_iExtraHappinessPerLuxury","m_iMinorResourceBonusCount"})named(raw.core.fields,name).values={0};
 named(raw.traits.fields,"m_iCityUnhappinessModifier").values={0};named(raw.traits.fields,"m_iPopulationUnhappinessModifier").values={0};state.unhappiness.players.at(p).trait_cities_modifier=0;state.unhappiness.players.at(p).trait_population_modifier=0;
 auto &policies=named(raw.strategy.blocks,"policies");named(policies.hashes,"has_policy").rows.clear();named(policies.hashes,"has_policy").wire_count=0;state.setup.players.at(p).policies.clear();
 add_city(state,p,8192,8,0,0,gen,target,0,0,0,0);city_operands(state,p,8192,0);add_city(state,p,8193,5,0,0,gen,target,0,0,0,0);city_operands(state,p,8193,1);
 raw.cities.header.source_slots=4;named(raw.cities.header.fields,"num_slots").value=4;named(raw.cities.header.fields,"last_index").value=1;raw.cities.header.next_free_indices={-1,-1,3,-1};named(raw.core.fields,"m_iCapitalCityID").values={8192};state.cities.players.at(p).capital=8192;
 named(raw.core.fields,"m_iHappinessPerTradeRouteCount").values={250};named(raw.core.fields,"m_iCityConnectionHappiness").values={77};named(raw.core.fields,"m_iHappinessFromLeagues").values={6};
 auto &connections=named(raw.planning.blocks,"city_connections");named(connections.fields,"route_infos_dimension").value=2;named(connections.fields,"city_ids_wire_count").value=2;named(connections.arrays,"city_plot_ids").values={0,1};named(connections.arrays,"route_infos").values={-1,0,-1,1,-1,0,-1,0};
 auto connection=quote_bnw_city_connection_happiness(state,p);check(connection.capital==8192&&connection.connected_count==1&&connection.total==2,"Native capital/route/percentage truncation differs");
 auto bare=state;named(bare.world.all_players->players.at(p).core.fields,"m_iCapitalCityID").values={0};bare.cities.players.at(p).capital=0;check(quote_bnw_city_connection_happiness(bare,p).total==2,"Bare FLTA slot did not resolve current generation city");
 auto stale_cap=state;named(stale_cap.world.all_players->players.at(p).core.fields,"m_iCapitalCityID").values={16384};stale_cap.cities.players.at(p).capital=16384;check(!quote_bnw_city_connection_happiness(stale_cap,p).capital&&quote_bnw_city_connection_happiness(stale_cap,p).total==0,"Stale generation resolved capital");
 auto best_only=state;named(named(best_only.world.all_players->players.at(p).planning.blocks,"city_connections").arrays,"route_infos").values[3]=4;check(quote_bnw_city_connection_happiness(best_only,p).total==0,"Best-only bit was treated as HAS_ANY_ROUTE");
 for(const char *name:{"m_paiNumResourceUsed","m_paiNumResourceTotal","m_paiResourceExport","m_paiResourceImport","m_paiResourceFromMinors","m_paiResourcesSiphoned"}){auto &f=named(raw.core.fields,name);f.hashes.clear();f.values.clear();f.wire_count=0;}auto &resource=named(raw.core.fields,"m_paiNumResourceTotal");resource.hashes={gold.hash};resource.values={1};resource.wire_count=1;
 const auto team=state.setup.players.at(p).team;named(state.world.teams->teams.at(team).fields,"natural_wonders").value=3;named(raw.traits.fields,"m_iNaturalWonderHappinessModifier").values={50};
 constexpr std::int64_t minor=22;for(std::int64_t m=22;m<63;++m){state.setup.players.at(m).alive=m==minor;named(state.world.all_players->players.at(m).core.fields,"m_bAlive").values={m==minor};}
 for(const auto &[type,trait]:sr.minors().minor_traits())if(trait==3){state.world.pregame->archive.string_arrays.at("minor_civ_types").at(minor)=type;break;}
 auto &ai=named(state.world.all_players->players.at(minor).planning.blocks,"minor_civ_ai");named(ai.fields,"m_eAlly").value=p;state.treasury.players.at(minor).ally=p;named(ai.arrays,"m_aiFriendshipWithMajorTimes100").values.at(p)=3000;
 const auto mt=state.setup.players.at(minor).team;named(state.world.teams->teams.at(team).arrays,"at_war").values.at(mt)=0;state.player_research.teams.at(team).at_war.erase(mt);
 // Detached source-math reads: no callback, completion issuer or store.
 using Read=detail::BnwHappinessPostReligionRead;
 const auto reached=[&](const LiveSessionState &w,Read stage){return detail::bnw_happiness_post_religion_reached_read(w,sr,identity,p,stage);};
 check(reached(state,Read::leagues).operand==6,"Reached league cache getter differs");
 check(reached(state,Read::connections_cache).connections->total==2,"Reached connection getter differs");
 check(reached(state,Read::connections_add).operand==77,"Connection add read unissued/recomputed value");
 auto skipped=state;skipped.world.pregame->archive.integers["active_player"]=-1;
 skipped.world.all_players.reset();skipped.unhappiness.players.clear();skipped.cities.players.clear();
 const auto not_local=reached(skipped,Read::achievement_condition);
 check(!not_local.unhappiness&&!not_local.achievement_requested,"Nonlocal achievement read future excess operands");
 skipped.world.pregame->archive.integers["active_player"]=p;
 rejects([&]{(void)reached(skipped,Read::achievement_condition);},"Local achievement bypassed mandatory actual unhappiness source");
 auto bad_identity=identity;bad_identity.rules_sha256="wrong";
 rejects([&]{(void)detail::bnw_happiness_post_religion_reached_read(state,sr,bad_identity,p,Read::leagues);},"Mismatched suffix rule epoch accepted");
 auto wide=state;named(wide.world.all_players->players.at(p).core.fields,"m_iHappinessFromLeagues").values={INT32_MAX+std::int64_t{1}};
 rejects([&]{(void)reached(wide,Read::leagues);},"Wide native reached league value accepted");
 check(reached(wide,Read::connections_add).operand==77,"Unreached malformed league blocked current connection getter");
 auto token=attest_bnw_happiness_prefix_input(state,pr,identity,p);auto positive_prefix=execute_bnw_happiness_native_prefix(state,token,pr);check(static_cast<bool>(positive_prefix.prefix),"Positive suffix prefix unavailable");auto ui=start_bnw_happiness_game_data_client(state,pr,identity,p);fixture.identity=identity;fixture.on_random={};
 const auto run=[&](const BnwHappinessPrefixDraft &d,const BnwHappinessGameDataOwner &receiver){return execute_bnw_happiness_remaining_suffix(d,pr,selected.profile,fixture,rr,sr,receiver,*db);};
 auto positive=run(*positive_prefix.prefix,ui);if(!positive.native_cache_suffix_complete)throw std::runtime_error("Positive suffix failed: "+positive.frontier);
 check(positive.stores.size()==7&&positive.stores[0].operand==9&&positive.stores[1].operand==11&&positive.stores[2].operand==6&&positive.connections->total==2&&positive.batch->receipt().happiness==39&&positive.batch->receipt().excess==20&&positive.unhappiness->total==19,"Source-ordered wonders/minors/leagues/connections/unhappiness sum differs");
 check(!positive.mandatory_game_data_fanout_complete&&positive.receiver->dirty()&&positive.receiver->revision()==1&&positive.receiver->receipts().size()==1&&positive.receiver->observed_happiness()==39&&!ui.dirty(),"Actual private dirty receiver did not change retained view/cache");
 auto received=ui;apply_bnw_happiness_game_data_batch(*positive.continuation.religion.query.epoch->world,received,*db,*positive.batch);rejects([&]{apply_bnw_happiness_game_data_batch(*positive.continuation.religion.query.epoch->world,received,*db,*positive.batch);},"GameData receipt replay accepted");
 auto after=*positive.continuation.religion.query.epoch->world;after.cities.players.at(p).cities.at(8192).population++;rejects([&]{auto fresh=ui;apply_bnw_happiness_game_data_batch(after,fresh,*db,*positive.batch);},"Changed current city population escaped dirty world seal");
 check(!run(*positive_prefix.prefix,*positive.receiver).native_cache_suffix_complete,"Pending fanout owner reused as new native update");
 auto no_provider=run(*positive_prefix.prefix,ui);auto missing_provider=pump_bnw_happiness_game_data(std::move(no_provider),*db);check(!missing_provider.staged&&missing_provider.dispatch_attempted,"Unknown UI notification provider bypassed");
 fixture.declare_ui=true;auto no_listeners=run(*positive_prefix.prefix,ui);auto empty=pump_bnw_happiness_game_data(std::move(no_listeners),*db);check(!empty.staged&&empty.dispatch_attempted,"Empty selected UI registry counted as completed feed");
 fixture.ui_script="Events.SerialEventGameDataDirty.Add(function()print('actual-private-dirty-listener')end)";
 auto pending=run(*positive_prefix.prefix,ui);const auto *same_epoch=pending.continuation.religion.query.epoch.get();const auto forks=fixture.forks;auto pumped=pump_bnw_happiness_game_data(std::move(pending),*db);
 check(pumped.staged&&pumped.listeners==1&&pumped.dispatch_attempted&&pumped.staged->mandatory_game_data_fanout_complete&&!pumped.staged->receiver->dirty()&&!pumped.staged->receiver->pending_fanout()&&pumped.staged->continuation.religion.query.epoch.get()==same_epoch&&fixture.forks==forks&&fixture.publishes==0,"Deferred dirty delivery did not use SAME private VM/world and actual consumer");
 check(!pump_bnw_happiness_game_data(std::move(*pumped.staged),*db).staged,"Delivered GameData transaction replayed");
 auto partial=run(*positive_prefix.prefix,ui);const auto *partial_epoch=partial.continuation.religion.query.epoch.get();partial.continuation.religion.query.epoch->all_listeners_registered=false;auto unavailable=pump_bnw_happiness_game_data(std::move(partial),*db);check(!unavailable.staged&&!unavailable.dispatch_attempted&&unavailable.pending&&unavailable.pending->continuation.religion.query.epoch.get()==partial_epoch&&unavailable.pending->receiver->pending_fanout(),"Partial stock registry dispatched or destroyed accepted native epoch");
 unavailable.pending->continuation.religion.query.epoch->all_listeners_registered=true;auto recovered=pump_bnw_happiness_game_data(std::move(*unavailable.pending),*db);check(recovered.staged&&!recovered.pending&&recovered.dispatch_attempted&&recovered.listeners==1&&recovered.staged->continuation.religion.query.epoch.get()==partial_epoch,"Recovered registry forked/replayed instead of delivering SAME accepted epoch once");
 auto rng_pending=run(*positive_prefix.prefix,ui);const auto *rng_epoch=rng_pending.continuation.religion.query.epoch.get();auto &rng_world=*rng_pending.continuation.religion.query.epoch->world;const auto before_rng=rng_world.random.gameplay.state();(void)rng_world.random.gameplay.draw(1);const auto changed_rng=rng_world.random.gameplay.state();auto stale_rng=pump_bnw_happiness_game_data(std::move(rng_pending),*db);check(!stale_rng.staged&&stale_rng.pending&&!stale_rng.dispatch_attempted&&stale_rng.pending->continuation.religion.query.epoch.get()==rng_epoch&&stale_rng.pending->continuation.religion.query.epoch->world->random.gameplay.state()==changed_rng&&changed_rng!=before_rng,"RNG stale seal bypassed or lost accepted current world before dispatch");
 fixture.ui_script="Events.SerialEventGameDataDirty.Add(function()Map.Rand(1,'UI-write');error('dirty-ui-failure')end)";fixture.mutate=100;
 auto before_ui_error=run(*positive_prefix.prefix,ui);auto ui_error=pump_bnw_happiness_game_data(std::move(before_ui_error),*db);check(!ui_error.staged&&!ui_error.pending&&ui_error.dispatch_attempted&&!ui.dirty()&&source.unhappiness.players.at(p).happiness==original,"Failed UI fanout kept callback world/native stores");
 fixture.ui_script="Events.SerialEventGameDataDirty.Add(function()Map.Rand(1,'reentrant-world')end)";fixture.mutate=1;
 auto before_reentry=run(*positive_prefix.prefix,ui);auto reentry=pump_bnw_happiness_game_data(std::move(before_reentry),*db);check(!reentry.staged&&reentry.dispatch_attempted,"Unowned reentrant gameplay/UI dirty change silently cleared");
 fixture.mutate=0;fixture.declare_ui=false;fixture.ui_script.clear();
 fixture.on_random=[p](LiveSessionState &w){named(w.world.all_players->players.at(p).core.fields,"m_iHappinessFromLeagues").values={16};named(w.world.all_players->players.at(p).core.fields,"m_iHappinessPerGarrisonedUnitCount").values={1};};auto changed=run(*positive_prefix.prefix,ui);check(changed.native_cache_suffix_complete&&changed.batch->receipt().happiness==49,"Suffix lost Lua effect or incorrectly re-evaluated local/garrison happiness");
 fixture.on_random=[p](LiveSessionState &w){w.world.all_players->players.at(p).cities.cities.back().management.blocks.clear();};auto late=run(*positive_prefix.prefix,ui);check(!late.native_cache_suffix_complete&&!late.continuation.religion.query.epoch&&!late.receiver&&!ui.dirty(),"Late required unhappiness failure retained earlier callback/suffix stores");
 fixture.on_random=[p](LiveSessionState &w){named(w.world.all_players->players.at(p).core.fields,"m_iHappinessFromLeagues").values={INT32_MAX};};auto overflow=run(*positive_prefix.prefix,ui);check(!overflow.native_cache_suffix_complete&&!overflow.continuation.religion.query.epoch&&!overflow.receiver,"Late league native overflow retained partial epoch");
 fixture.on_random={};fixture.mutate=100;auto achievement=run(*positive_prefix.prefix,ui);check(achievement.native_cache_suffix_complete&&achievement.batch->receipt().achievement_xp2_45_requested&&achievement.batch->receipt().excess==120&&achievement.stores[achievement.stores.size()-2].stage==BnwHappinessSuffixStage::achievement_request,"Local >=100 native achievement conditional/order differs");
 fixture.mutate=0;fixture.script="GameEvents.GetFounderBenefitsReligion.Add(function(p)Map.Rand(1,'late');error('callback fail')end)";auto fail=run(*positive_prefix.prefix,ui);check(!fail.native_cache_suffix_complete&&!fail.continuation.religion.query.epoch&&fixture.publishes==0,"Lua error retained suffix effects");
 check(source.unhappiness.players.at(p).happiness==original&&!host.session().readiness().can_continue(),"Suffix changed actual source/readiness");
 std::cout<<"PASS actual selected native getters with isolated registered Lua only; positive complete cache suffix39/excess20, native route slots/bits, staged dirty receiver, source order and atomic rollback; actual SAME-epoch deferred fixture UI delivery/error/unknown registry/replay; production stock epoch remains blocked\n";
}
