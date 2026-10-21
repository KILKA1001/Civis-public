#include "civ5/runtime/bnw_happiness_buildings.hpp"
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
int main(int argc,char **argv) {
    check(argc==3 || argc==4,"Expected save, installation and optional SDK source root");
    if(argc==4) {const auto root=std::filesystem::path(argv[3])/"CvGameCoreDLL_Expansion2";
        check(identify_file(root/"CvPlayer.cpp").sha256=="9635fe516e590e133c3907ee25c13128fb666e3e7817c967e247affb700a952d","Player SDK pin differs");
        check(identify_file(root/"CvBuildingClasses.cpp").sha256=="de5121283887ca9011da25b4c27398b7ec083ad70b577676e8fed5dce26940e4","BuildingClasses SDK pin differs");
        check(identify_file(root/"CvReligionClasses.cpp").sha256=="50d96e758204221392c5497b6322162869ca2352fec5277dacfc177881d81093","Religion SDK pin differs");
        check(identify_file(root/"CvCity.cpp").sha256=="15e14918392e0f8e9de7e37758a659ed0bf7a571144daa57e83b1b999a548fee","City SDK pin differs");
    }
    auto selected=ui::bootstrap_selected_bnw_preview(argv[1],argv[2],0);check(static_cast<bool>(selected.preview),"Selected bootstrap failed");
    auto &host=*selected.preview.host;const auto p=host.active_player_id();const auto &source=host.session().state();
    const auto original=BnwBuildingHappinessRules::load(host.database());const auto baseline=quote_bnw_happiness_from_buildings(source,original,host.identity(),p);
    auto db=host.database().clone_gameplay();db->apply_sql("DELETE FROM Building_BuildingClassHappiness; DELETE FROM Policy_BuildingClassHappiness; UPDATE Policies SET ExtraHappiness=0,ExtraHappinessPerCity=0;"
        "INSERT INTO Building_BuildingClassHappiness(BuildingType,BuildingClassType,Happiness) VALUES('BUILDING_MONUMENT','BUILDINGCLASS_GRANARY',3),('BUILDING_MONUMENT','BUILDINGCLASS_MONUMENT',-100);"
        "INSERT INTO Policy_BuildingClassHappiness(PolicyType,BuildingClassType,Happiness) VALUES('POLICY_TRADITION','BUILDINGCLASS_MONUMENT',4),('POLICY_TRADITION','BUILDINGCLASS_GRANARY',-2);");
    const auto rules=BnwBuildingHappinessRules::load(*db);auto identity=host.identity();identity.rules_sha256=rules.rules_sha256();
    const auto types=db->cache_info_types();const auto entry=[&](const char *name)->const content::InfoTypeEntry & {const auto *e=types.find(name);check(e,"Fixture type absent");return *e;};
    const auto &generator=entry("BUILDING_MONUMENT"),&target=entry("BUILDING_GRANARY"),&opener=entry("POLICY_TRADITION"),&branch=entry("POLICY_BRANCH_TRADITION");
    auto state=source;auto &raw=state.world.all_players->players.at(p);raw.cities.cities.clear();raw.cities.header.wire_city_count=0;state.cities.players.at(p).cities.clear();
    named(raw.core.fields,"m_iHappinessPerGarrisonedUnitCount").values={0};named(raw.core.fields,"m_iHappinessPerXPopulation").values={0};named(raw.core.fields,"m_iHappinessPerXPolicies").values={1};
    named(raw.traits.fields,"m_iCityUnhappinessModifier").values={1};
    auto &policies=named(raw.strategy.blocks,"policies");auto &owned=named(policies.hashes,"has_policy");owned.rows={{opener.hash,{1}}};owned.wire_count=1;
    auto &blocked=named(policies.hashes,"branch_blocked");blocked.rows={{branch.hash,{0}}};blocked.wire_count=1;state.setup.players.at(p).policies={opener.id};
    add_city(state,p,7,8,10,7,generator,target,2,1,1,3);add_city(state,p,9,5,-4,9,generator,target,0,0,2,1);
    auto q=quote_bnw_happiness_from_buildings(state,rules,identity,p);
    check(q.cities.size()==2 && q.cities[0].policy==2 && q.cities[0].cap==5 && q.cities[0].total==5 && q.cities[1].total==-6 && q.local==-1 &&
          q.global_classes==15 && q.global_unmodded==16 && q.per_policies==1 && q.global==32,"Local cap, once-per-class policy, global generator/target count or negative happiness differs");
    blocked.rows[0].values={1};q=quote_bnw_happiness_from_buildings(state,rules,identity,p);
    check(q.local==1 && q.per_policies==1,"Blocked free opener affected class bonus or was excluded from owned policy divisor");blocked.rows[0].values={0};
    named(raw.traits.fields,"m_iCityUnhappinessModifier").values={0};check(quote_bnw_happiness_from_buildings(state,rules,identity,p).local==2,"Normal population cap differs");
    named(raw.traits.fields,"m_iCityUnhappinessModifier").values={-1};check(quote_bnw_happiness_from_buildings(state,rules,identity,p).local==-1,"Negative nonzero trait failed India branch");
    auto sumdb=db->clone_gameplay();sumdb->apply_sql("UPDATE Defines SET Value=2 WHERE Name='CITY_MAX_NUM_BUILDINGS'");const auto sum=BnwBuildingHappinessRules::load(*sumdb);auto sumid=identity;sumid.rules_sha256=sum.rules_sha256();
    check(quote_bnw_happiness_from_buildings(state,sum,sumid,p).global_classes==21,"CITY_MAX_NUM_BUILDINGS>1 failed real+free source sum");
    auto wrong=state;wrong.cities.players.at(p).cities.at(7).free_buildings[target.id]=2;rejects([&]{quote_bnw_happiness_from_buildings(wrong,rules,identity,p);},"Stale live free building count accepted");
    wrong=state;named(wrong.world.all_players->players.at(p).core.fields,"m_iHappinessPerGarrisonedUnitCount").values={1};rejects([&]{quote_bnw_happiness_from_buildings(wrong,rules,identity,p);},"Positive garrison became zero");
    wrong=state;auto &rel=named(named(wrong.world.all_players->players.at(p).cities.cities[0].tail.blocks,"religions").children,"m_ReligionStatus");rel.wire_count=1;
    saves::CityTailNode row{};row.name="religion";row.children={node("version",2),node("m_eReligion",1),node("m_iFollowers",8),node("m_iPressure",100)};rel.children={row};wrong.cities.players.at(p).cities.at(7).religions={{1,8,100}};
    rejects([&]{quote_bnw_happiness_from_buildings(wrong,rules,identity,p);},"Positive religion became zero");
    wrong=state;auto &a=named(named(wrong.world.all_players->players.at(p).cities.cities[0].production.blocks,"buildings").children,"m_paiNumRealBuilding");a.children[0].fields[0].value=0x12345678;
    rejects([&]{quote_bnw_happiness_from_buildings(wrong,rules,identity,p);},"Unresolved building hash accepted");
    wrong=state;wrong.world.all_players->players.at(p).cities.cities[0].production.context.bnw_city_version=5;rejects([&]{quote_bnw_happiness_from_buildings(wrong,rules,identity,p);},"Stale city version accepted");
    wrong=state;named(named(wrong.world.all_players->players.at(p).cities.cities[1].production.blocks,"before_strategy").fields,"m_iUnmoddedHappinessFromBuildings").value=INT32_MAX;
    rejects([&]{quote_bnw_happiness_from_buildings(wrong,rules,identity,p);},"Global cache native int overflow accepted");
    rejects([&]{quote_bnw_happiness_from_buildings(state,rules,host.identity(),p);},"Different rule identity accepted");
    check(quote_bnw_happiness_from_buildings(source,original,host.identity(),p).global==baseline.global && !host.session().readiness().can_continue(),"Building query changed source/readiness");
    std::cout<<"PASS selected buildings local="<<baseline.local<<" global="<<baseline.global<<"; isolated nonzero/free/class/policy/cap/stale/overflow fixtures\n";
}
