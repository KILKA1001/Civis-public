#include "civ5/runtime/bnw_happiness_update_prefix.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/runtime/file_identity.hpp"
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
    bool missing=false,verified=true;int forks=0,publishes=0;std::int32_t observed=-999,mutate=0;
    std::string script="GameEvents.GetFounderBenefitsReligion.Add(function(p)Map.Rand(1,'observe');return -1 end)";
    BnwGameplayLuaForkResult fork_at(const LiveSessionState &state) override {
        ++forks;if(missing)return {{},"required selected stock contexts/listeners not loaded"};
        auto e=std::make_unique<BnwTeamTechLuaEpoch>();e->world=std::make_unique<LiveSessionState>(state);e->source_world=&state;e->getter_world=e->world.get();
        e->identity=identity;e->selected_dlc=profile.enabled_dlc;e->selected_mods=profile.enabled_mods;
        e->all_selected_contexts_loaded=verified;e->all_listeners_registered=verified;e->all_external_writes_escrowed=verified;
        ScriptMapProvider map;map.grid_size=[]{return std::pair<std::int64_t,std::int64_t>{2,1};};
        map.random=[this,w=e->world.get()](std::int64_t,std::string_view) {
            auto &f=named(w->world.all_players->players.at(player).core.fields,"m_iHappiness");observed=f.values[0];
            f.values[0]+=mutate;w->unhappiness.players.at(player).happiness+=mutate;return std::int64_t{0};
        };
        e->host=std::make_unique<LuaScriptHost>(ScriptBudget{8*1024*1024,1000000},ScriptSourceProvider{},ScriptModdingProvider{},ScriptGameplayProvider{},ScriptSaveDataProvider{},ScriptPregameProvider{},std::move(map));
        e->host->execute("fixture-only",script,"=fixture-selected-prefix");e->host->complete_loading();return {std::move(e),{}};
    }
    BnwGameplayLuaPublishResult publish(std::unique_ptr<BnwTeamTechLuaEpoch>,LiveSessionState &) override {++publishes;return {false,false,"fixture never publishes"};}
};
}
int main(int argc,char **argv) {
    check(argc==3 || argc==4,"Expected selected save, installation and optional SDK source root");
    if(argc==4) {const auto root=std::filesystem::path(argv[3])/"CvGameCoreDLL_Expansion2";
        check(identify_file(root/"CvPlayer.cpp").sha256=="9635fe516e590e133c3907ee25c13128fb666e3e7817c967e247affb700a952d","DoUpdateHappiness source order SDK pin differs");
        check(identify_file(root/"CvInfos.cpp").sha256=="b520062d7ee5bf67485b9bcc3b5940cab6616d3d56847999a6c11cde834c0e2c","Handicap HappinessDefault SDK pin differs");
    }
    auto selected=ui::bootstrap_selected_bnw_preview(argv[1],argv[2],0);
    check(static_cast<bool>(selected.preview),"Prefix selected bootstrap failed");auto &host=*selected.preview.host;const auto p=host.active_player_id();const auto &source=host.session().state();
    const auto rules=BnwHappinessPrefixRules::load(host.database());const auto input=attest_bnw_happiness_prefix_input(source,rules,host.identity(),p);
    const auto old=named(source.world.all_players->players.at(p).core.fields,"m_iHappiness").values[0];
    auto executed=execute_bnw_happiness_native_prefix(source,input,rules);check(executed.prefix && executed.prefix->stores().size()==6,"Actual selected happiness native prefix failed");
    const auto &d=*executed.prefix;const auto current=d.stores().back().happiness;
    check(named(d.state().world.all_players->players.at(p).core.fields,"m_iHappiness").values[0]==current && d.state().unhappiness.players.at(p).happiness==current &&
          !d.state().cities.players.at(p).excess_happiness,"Private prefix stores did not preserve raw/owned cache coherence or invalidated excess");
    FixtureEpochOwner missing;missing.profile=selected.profile;missing.identity=host.identity();missing.player=p;missing.missing=true;
    const auto religion=BnwReligionHappinessRules::load(host.database());auto blocked=continue_bnw_happiness_prefix_religion(d,rules,selected.profile,missing,religion);
    check(!blocked.store && !blocked.religion.query.epoch && blocked.frontier.find("required selected stock")!=std::string::npos,"Missing mandatory stock epoch bypassed");
    auto stale=source;named(stale.world.all_players->players.at(p).core.fields,"m_iHappiness").values[0]+=1;stale.unhappiness.players.at(p).happiness+=1;
    check(!execute_bnw_happiness_native_prefix(stale,input,rules).prefix,"Coherent changed cache was accepted by old input token");
    stale=source;stale.cities.players.at(p).excess_happiness.reset();check(!execute_bnw_happiness_native_prefix(stale,input,rules).prefix,"Changed excess projection escaped input cache digest");
    stale=source;stale.unhappiness.players.at(p).happiness+=1;rejects([&]{attest_bnw_happiness_prefix_input(stale,rules,host.identity(),p);},"Stale happiness echo accepted");
    auto db=host.database().clone_gameplay();db->apply_sql("UPDATE HandicapInfos SET HappinessDefault=7; UPDATE Policies SET ExtraHappiness=0,ExtraHappinessPerCity=0;"
        "UPDATE Policies SET ExtraHappiness=3,ExtraHappinessPerCity=2 WHERE Type='POLICY_TRADITION';"
        "DELETE FROM Building_BuildingClassHappiness;DELETE FROM Policy_BuildingClassHappiness;"
        "INSERT INTO Building_BuildingClassHappiness(BuildingType,BuildingClassType,Happiness) VALUES('BUILDING_MONUMENT','BUILDINGCLASS_GRANARY',3);"
        "INSERT INTO Policy_BuildingClassHappiness(PolicyType,BuildingClassType,Happiness) VALUES('POLICY_TRADITION','BUILDINGCLASS_MONUMENT',4),('POLICY_TRADITION','BUILDINGCLASS_GRANARY',-2);");
    const auto altered=BnwHappinessPrefixRules::load(*db);auto identity=host.identity();identity.rules_sha256=altered.rules_sha256();auto state=source;
    const auto types=db->cache_info_types();const auto entry=[&](const char *name)->const content::InfoTypeEntry & {const auto *e=types.find(name);check(e,"Prefix fixture type absent");return *e;};
    const auto &generator=entry("BUILDING_MONUMENT"),&target=entry("BUILDING_GRANARY"),&opener=entry("POLICY_TRADITION"),&branch=entry("POLICY_BRANCH_TRADITION");
    auto &raw=state.world.all_players->players.at(p);raw.cities.cities.clear();raw.cities.header.wire_city_count=0;state.cities.players.at(p).cities.clear();
    named(raw.core.fields,"m_iHappinessPerGarrisonedUnitCount").values={0};named(raw.core.fields,"m_iHappinessPerXPopulation").values={0};named(raw.core.fields,"m_iHappinessPerXPolicies").values={1};named(raw.core.fields,"m_iHappinessPerCity").values={4};
    named(raw.traits.fields,"m_iCityUnhappinessModifier").values={1};auto &policy=named(raw.strategy.blocks,"policies");auto &owned=named(policy.hashes,"has_policy");owned.rows={{opener.hash,{1}}};owned.wire_count=1;
    auto &branches=named(policy.hashes,"branch_blocked");branches.rows={{branch.hash,{0}}};branches.wire_count=1;state.setup.players.at(p).policies={opener.id};
    add_city(state,p,7,8,10,7,generator,target,2,1,1,3);add_city(state,p,9,5,-4,9,generator,target,0,0,2,1);
    const auto &silk=entry("RESOURCE_SILK");
    for(const char *name:{"m_paiNumResourceUsed","m_paiNumResourceTotal","m_paiResourceExport","m_paiResourceImport","m_paiResourceFromMinors","m_paiResourcesSiphoned"}) {
        auto &array=named(raw.core.fields,name);for(auto &value:array.values)value=0;
    }
    auto &resource_total=named(raw.core.fields,"m_paiNumResourceTotal");resource_total.hashes={silk.hash};resource_total.values={1};resource_total.wire_count=1;
    named(raw.core.fields,"m_iExtraHappinessPerLuxury").values={0};named(raw.core.fields,"m_iMinorResourceBonusCount").values={0};named(raw.traits.fields,"m_iLuxuryHappinessRetention").values={0};
    const auto actor_input=attest_bnw_happiness_prefix_input(state,altered,identity,p);auto positive=execute_bnw_happiness_native_prefix(state,actor_input,altered);
    check(positive.prefix && positive.prefix->stores().size()==6,"Positive formula prefix failed");const auto &stores=positive.prefix->stores();
    check(stores[0].happiness==7 && stores[1].operand==4 && stores[2].operand==-1 && stores[3].operand==32 && stores[4].operand==7 && stores[5].operand==8 &&
          stores[5].happiness==7+stores[1].operand-1+32+7+8,"Native source-ordered local/global/policy/percity stores differ");
    auto late_state=state;named(late_state.world.all_players->players.at(p).core.fields,"m_iHappinessPerCity").values={INT32_MAX};
    const auto late_token=attest_bnw_happiness_prefix_input(late_state,altered,identity,p);
    auto late_failure=execute_bnw_happiness_native_prefix(late_state,late_token,altered);
    check(!late_failure.prefix && named(late_state.world.all_players->players.at(p).core.fields,"m_iHappiness").values[0]==old && late_state.unhappiness.players.at(p).happiness==old,
          "Last source multiplication failure retained earlier cache writes");
    FixtureEpochOwner fixture;fixture.profile=selected.profile;fixture.identity=identity;fixture.player=p;fixture.mutate=20;
    const auto rrules=BnwReligionHappinessRules::load(*db);auto continued=continue_bnw_happiness_prefix_religion(*positive.prefix,altered,selected.profile,fixture,rrules);
    check(continued.store && continued.religion.query.epoch && fixture.forks==1 && fixture.publishes==0 && fixture.observed==stores.back().happiness &&
          continued.store->happiness==stores.back().happiness+20 && continued.store->operand==0 &&
          continued.religion.query.epoch->world->unhappiness.players.at(p).happiness==continued.store->happiness,"Lua did not see intermediate cache or same-epoch native store lost callback effect");
    check(named(positive.prefix->state().world.all_players->players.at(p).core.fields,"m_iHappiness").values[0]==stores.back().happiness,"Callback mutated source prefix instead of fork");
    fixture.script="GameEvents.GetFounderBenefitsReligion.Add(function(p)Map.Rand(1,'mutate');error('failure after escrow')end)";
    auto failed=continue_bnw_happiness_prefix_religion(*positive.prefix,altered,selected.profile,fixture,rrules);
    check(!failed.store && !failed.religion.query.epoch && named(state.world.all_players->players.at(p).core.fields,"m_iHappiness").values[0]==old && fixture.publishes==0,"Failed callback leaked intermediate stores or VM effects");
    fixture.script="GameEvents.GetFounderBenefitsReligion.Add(function(p)Map.Rand(1,'overflow');return -1 end)";fixture.mutate=INT32_MAX-stores.back().happiness;
    auto exactmax=continue_bnw_happiness_prefix_religion(*positive.prefix,altered,selected.profile,fixture,rrules);check(exactmax.store && exactmax.store->happiness==INT32_MAX,"Exact native max rejected");
    fixture.mutate+=1;auto overflow=continue_bnw_happiness_prefix_religion(*positive.prefix,altered,selected.profile,fixture,rrules);check(!overflow.store && !overflow.religion.query.epoch,"Native cache overflow retained VM/world");
    auto latedb=db->clone_gameplay();latedb->apply_sql("UPDATE Policies SET ExtraHappiness=2147483647 WHERE Type='POLICY_TRADITION'");const auto late=BnwHappinessPrefixRules::load(*latedb);auto lateid=identity;lateid.rules_sha256=late.rules_sha256();
    // Validation/query overflow before or inside private stores never mutates caller.
    rejects([&]{attest_bnw_happiness_prefix_input(state,late,lateid,p);},"Native policy overflow accepted");
    check(named(source.world.all_players->players.at(p).core.fields,"m_iHappiness").values[0]==old && !host.session().readiness().can_continue(),"Native prefix changed source/readiness");
    std::cout<<"PASS selected private happiness prefix="<<current<<"; six source stores, positive building/policy/city terms, cache digest, mandatory Lua, same-epoch effects and rollback\n";
}
