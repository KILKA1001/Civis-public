#include "civ5/runtime/bnw_city_current_production_rate.hpp"
#include "civ5/runtime/bnw_city_current_science_rate.hpp"
#include "civ5/runtime/bnw_city_registry.hpp"
#include "civ5/runtime/bnw_area_registry.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <algorithm>
#include <climits>
#include <iostream>
#include <optional>
#include <string_view>
#include <thread>
#include <type_traits>
using namespace civ5;using namespace civ5::runtime;
namespace {
void need(bool b,const char* m){if(!b)throw std::runtime_error(m);}
template<class F>void refuses(F&& f){bool rejected=false;try{f();}catch(const BnwCityCurrentProductionRateUnavailable&){rejected=true;}need(rejected,"Current PRODUCTION stale/generic owner admitted");}
template<class F>void overflow(F&& f){bool rejected=false;try{f();}catch(const CityYieldUnavailable&){rejected=true;}need(rejected,"Native PRODUCTION prefix overflow admitted");}
template<class F>void cycle(F&& f){bool rejected=false;try{f();}catch(const std::exception& e){const auto text=std::string_view(e.what());need(text=="Cyclic city production-to-yield rule"||text=="Native PRODUCTION recursive self-process unavailable before base","Unexpected self-process error");rejected=true;}need(rejected,"PRODUCTION recursive process admitted");}
const auto& named(const auto& rows,const char* name){using R=typename std::remove_cvref_t<decltype(rows)>::value_type;const R* out=nullptr;for(const auto& r:rows)if(r.name&&std::string_view(r.name)==name){need(!out,"Fixture duplicate field");out=&r;}need(out,"Fixture field absent");return *out;}
std::int64_t scalar(const auto& fields,const char* name){const auto& f=named(fields,name);need(f.values.size()==1,"Fixture scalar shape");return f.values.front();}
void pure(const content::GlobalDefineCache& d){
 // Controlled detached arithmetic is a read-model witness, never an issuer.
 CityYieldContext s;s.profile=content::TechnologyProfile::bnw;s.yield_slots=6;s.no_happiness=true;auto& p=s.players[0];auto& c=p.cities[0];c.owner=0;c.id=0;c.population=2;auto& v=c.yields[1];CityYieldRules r;r.profile=content::TechnologyProfile::bnw;
 const auto calc=[&]{return detail::bnw_native_production_rate_math(s,r,d,0,0);};
 v.terrain=2;v.per_population=50;need(calc()==300,"Native PRODUCTION units");c.resistance=true;c.razing=true;need(calc()==300,"PRODUCTION suppressed by resistance/raze");c.resistance=false;c.razing=false; // no anarchy operand exists in this native getter
 v.process_modifier=1;v.terrain=INT64_MAX;cycle([&]{(void)calc();});cycle([&]{(void)detail::bnw_native_production_rate_prefix_math(s,0,0);});v={};v.terrain=INT32_MAX;v.buildings=1;v.misc=-1;overflow([&]{(void)calc();});v={};v.terrain=INT32_MAX/100+1;overflow([&]{(void)calc();});
 v={};v.terrain=1;s.no_happiness=false;p.excess_happiness=INT32_MIN;overflow([&]{(void)calc();});s.no_happiness=true;need(calc()==100,"NO_HAPPINESS did not short-circuit excess");
 r.building_class_slots=4;r.building_slots=4;p.civilization=0;r.civilization_buildings[0]={{0,0},{1,-1},{2,2},{3,3}};r.building_trade_modifiers={{0,5},{2,7}};c.real_buildings={{0,1},{2,1},{3,1}};s.players[1].minor=true;
 saves::TradeRouteObservation route;route.origin_owner=0;route.destination_owner=1;route.origin_x=route.origin_y=route.destination_x=route.destination_y=-1;route.origin_yields.assign(6,0);route.destination_yields.assign(6,0);s.trade_routes={route,route};
 need(calc()==114,"Last qualifying class or empty-entry route count changed"); // BuildingInfo hole3 skips after GetNumBuilding; -1 mapping skips entirely.
 c.real_buildings.clear();c.free_buildings={{0,1}};need(calc()==110,"Free building count omitted");c.free_buildings.clear();need(calc()==100,"Reset-zero count became building");
 const auto* maximum=d.find("CITY_MAX_NUM_BUILDINGS");need(maximum&&maximum->lookup.error.empty()&&std::holds_alternative<std::int64_t>(maximum->lookup.value),"Actual CITY_MAX_NUM_BUILDINGS absent");const auto limit=std::get<std::int64_t>(maximum->lookup.value);
 c.real_buildings={{0,INT32_MAX}};c.free_buildings={{0,1}};if(limit<=1){need(calc()==110,"Native max replaced by count sum");}else{overflow([&]{(void)calc();});}c.real_buildings={{0,1}};c.free_buildings.clear();r.building_trade_modifiers[0]=INT32_MAX;overflow([&]{(void)calc();});
 r.building_trade_modifiers.clear();s.trade_routes.clear();c.real_buildings.clear();r.civilization_buildings.clear();v.terrain=2;auto& science=c.yields[3];science.terrain=1;science.process_modifier=25;need(detail::bnw_native_science_rate_math(s,r,d,0,0)==150,"SCIENCE process skipped actual normal Production");science.process_modifier=-25;need(detail::bnw_native_science_rate_math(s,r,d,0,0)==50,"Signed process conversion changed");science.process_modifier=INT32_MAX;overflow([&]{(void)detail::bnw_native_science_rate_math(s,r,d,0,0);});science.process_modifier=0;v.terrain=INT64_MAX;need(detail::bnw_native_science_rate_math(s,r,d,0,0)==100,"SCIENCE zero process eagerly read Production");
 s.profile=content::TechnologyProfile::vp542;overflow([&]{(void)calc();});std::cout<<"PASS detached native Production/Science process, count max/sum, nullable Info and route order; no issuer\n";
}
void count_read_math(const content::InfoTypeCatalog& types,const CityYieldRules& rules){
 const auto* b=types.find("BUILDING_MONUMENT");const auto* other=types.find("YIELD_FOOD");need(b&&b->table=="Buildings"&&b->id>=0&&static_cast<std::uint64_t>(b->id)<rules.building_slots&&other,"Genuine count metadata unavailable");
 saves::CityProductionRecord rows{"m_paiNumRealBuilding",0,0,128,{},{},{},{}};rows.fields.push_back({"wire_count",4});
 const auto add=[&](std::uint32_t hash,std::optional<std::int64_t> value){saves::CityProductionRecord row{"row",rows.children.size(),rows.children.size()*8,rows.children.size()*8+8,{},{},{},{}};row.fields.push_back({"hash",hash});if(value){row.arrays.push_back({"values",1,{*value}});}rows.children.push_back(std::move(row));};
 std::uint32_t unknown=1;while(unknown<32&&types.find_hash(unknown)){++unknown;}need(!types.find_hash(unknown),"Bounded genuine unknown hash witness unavailable");
 add(b->hash,2);add(0,std::nullopt);add(unknown,99);add(b->hash,5);
 const auto read=[&]{return detail::bnw_native_building_counts_reset_read_math(rows,rules.building_slots,types);};const auto result=read();need(result.at(static_cast<std::size_t>(b->id))==5,"Native Building duplicate did not assign last");for(std::size_t i=0;i<result.size();++i){if(i!=static_cast<std::size_t>(b->id)){need(result[i]==0,"Native Reset omitted slot/Info hole not zero");}}
 rows.children.at(1).arrays.push_back({"values",1,{1}});overflow([&]{(void)read();});rows.children.at(1).arrays.clear();rows.children.at(2).arrays.front().values.front()=INT64_MAX;overflow([&]{(void)read();});rows.children.at(2).arrays.front().values.front()=99;
 rows.children.at(2).fields.front().value=other->hash;overflow([&]{(void)read();});rows.children.at(2).fields.front().value=unknown;
 overflow([&]{(void)detail::bnw_native_building_counts_reset_read_math(rows,static_cast<std::uint64_t>(b->id),types);});rows.children.clear();rows.fields.front().value=-1;const auto reset=read();need(std::all_of(reset.begin(),reset.end(),[](auto n){return n==0;}),"Native negative wire loop changed");
 std::cout<<"PASS controlled detached native Reset/Read math with genuine activated Building metadata; no current owner\n";
}
std::shared_ptr<ui::SelectedBnwPreviewHost> boot(const std::filesystem::path& save,const std::filesystem::path& game){auto b=ui::bootstrap_selected_bnw_preview(save,game,0,true);if(b.preview.blocker)throw std::runtime_error(b.preview.blocker->reason);std::shared_ptr<ui::SelectedBnwPreviewHost> h(std::move(b.preview.host));need(bool(h),"Actual Host absent");return h;}
void run(const std::filesystem::path& save,const std::filesystem::path& game){
 const auto before=identify_file(save).sha256;auto host=boot(save,game);pure(host->defines());count_read_math(host->types(),load_city_yield_rules(host->database(),content::TechnologyProfile::bnw));const auto& s=host->session().state();std::size_t seen=0,positive=0,resistance_or_raze=0,science_process=0,self_cycle=0,null_hash=0,building_rows=0;
 std::optional<BnwCityCurrentProductionRate> retained;std::int64_t owner0=-1,city0=-1;std::int32_t value0=0;
 for(const auto& p:s.world.all_players->players){for(const auto& raw:p.cities.cities){if(seen==24){break;}++seen;const auto owner=scalar(raw.core.fields,"m_eOwner"),id=scalar(raw.core.fields,"m_iID");need(owner>=0&&owner<64&&id>=0&&id<=INT32_MAX,"Actual city native bounds");const auto& modifiers=named(raw.core.fields,"m_aiProductionToYieldModifier").values;need(modifiers.size()==6,"Actual process span");
  if(modifiers.at(1)!=0){bool rejected=false;try{(void)inspect_bnw_city_current_production_rate(host,owner,id);}catch(const BnwCityCurrentProductionRateUnavailable& e){need(std::string_view(e.what())=="Current PRODUCTION recursive self-process unavailable before base","Unexpected genuine Production refusal");rejected=true;}need(rejected,"Actual self-process did not refuse");++self_cycle;continue;}
  auto full=inspect_bnw_city_current_production_rate(host,owner,id),index=inspect_bnw_city_current_production_rate(host,owner,id&8191);const auto value=full.value_times100();need(index.value_times100()==value,"Genuine Production full/index mismatch");if(value>0){++positive;}if(scalar(raw.core.fields,"m_iResistanceTurns")>0||scalar(raw.core.fields,"m_iRazingTurns")>0){++resistance_or_raze;}
  // Wider model equality is only a witness after the genuine source/native proof.
  auto math=s.cities;auto& mp=math.players.at(owner);auto& mc=mp.cities.at(id);const auto* cap=resolve_bnw_city_registry(s.world,s.cities,static_cast<std::int32_t>(owner),static_cast<std::int32_t>(mp.capital));mp.capital=cap?scalar(cap->core.fields,"m_iID"):-1;const auto* area=resolve_bnw_area_registry(s.world,static_cast<std::int32_t>(mc.area));mc.area=area?named(area->fields,"id").value:-1;const auto rules=load_city_yield_rules(host->database(),content::TechnologyProfile::bnw);need(city_yield_times100(math,rules,host->defines(),owner,id,1).total_times100==value,"Genuine Production differs from preserved wider formula");
  const auto& buildings=named(raw.production.blocks,"buildings");for(const auto* name:{"m_paiNumRealBuilding","m_paiNumFreeBuilding"})for(const auto& row:named(buildings.children,name).children){++building_rows;if(named(row.fields,"hash").value==0){++null_hash;}}
  if(modifiers.at(3)!=0&&scalar(raw.core.fields,"m_iResistanceTurns")<=0&&scalar(raw.core.fields,"m_iRazingTurns")<=0){auto science=inspect_bnw_city_current_science_rate(host,owner,id);need(science.value_times100()==city_yield_times100(math,rules,host->defines(),owner,id,3).total_times100,"Actual Science process differs");++science_process;}
  std::cout<<"Genuine Production owner="<<owner<<" city="<<id<<" rateTimes100="<<value<<'\n';const auto stale=(id&~std::int64_t{8191})==8192?(id&8191)|16384:(id&8191)|8192;refuses([&]{(void)inspect_bnw_city_current_production_rate(host,owner,stale);});if(!retained){owner0=owner;city0=id;value0=value;retained=std::move(full);}
 }if(seen==24){break;}}
 need(retained.has_value(),"No actual supported Production branch");std::cout<<"Production positive="<<positive<<(positive?" observed":" UNOBSERVED")<<" resistance/raze="<<resistance_or_raze<<(resistance_or_raze?" observed":" UNOBSERVED")<<" genuine Science process="<<science_process<<(science_process?" observed":" UNOBSERVED")<<" self-cycle="<<self_cycle<<(self_cycle?" observed":" UNOBSERVED")<<" Building Read rows="<<building_rows<<" null-hash="<<null_hash<<(null_hash?" observed":" UNOBSERVED")<<'\n';
 bool read=false,bind=false;std::thread worker([&]{try{(void)retained->value_times100();}catch(const BnwCityCurrentProductionRateUnavailable&){read=true;}try{(void)inspect_bnw_city_current_production_rate(host,owner0,city0);}catch(const BnwCityCurrentProductionRateUnavailable&){bind=true;}});worker.join();need(read&&bind&&retained->value_times100()==value0,"Foreign thread poisoned healthy source");refuses([&]{(void)inspect_bnw_city_current_production_rate({},0,0);});refuses([&]{(void)inspect_bnw_city_current_production_rate(host,INT64_MAX,city0);});refuses([&]{(void)inspect_bnw_city_current_production_rate(host,owner0,INT64_MIN);});
 const auto cp=host->session().checkpoint();host->session().restore(cp);refuses([&]{(void)retained->value_times100();});retained=inspect_bnw_city_current_production_rate(host,owner0,city0);need(retained->value_times100()==value0,"Restore changed normal rate");auto moved=std::move(*retained);refuses([&]{(void)retained->value_times100();});need(moved.value_times100()==value0,"Moving query lost actual source");retained=std::move(moved);
 auto generic=LiveSession::from_restored_bnw(host->session().source(),host->session().state());auto facade=std::make_shared<LiveSession>(std::move(host->session()));refuses([&]{(void)retained->value_times100();});refuses([&]{(void)inspect_bnw_city_current_production_rate(host,owner0,city0);});host->session()=std::move(generic);refuses([&]{(void)inspect_bnw_city_current_production_rate(host,owner0,city0);});facade.reset();retained.reset();host.reset();
 auto parent=boot(save,game);auto sole=inspect_bnw_city_current_production_rate(parent,owner0,city0);const auto weak=std::weak_ptr<ui::SelectedBnwPreviewHost>(parent);parent.reset();need(!weak.expired()&&sole.value_times100()==value0,"Production did not retain sole actual parent");auto writer=weak.lock();need(bool(writer),"Retained parent absent");(void)writer->session().mutable_state();refuses([&]{(void)sole.value_times100();});need(identify_file(save).sha256==before,"Original save changed");std::cout<<"PASS genuine Production lifecycle/source-only read; no order completion/store/refresh\n";
}
}
static_assert(!std::is_default_constructible_v<BnwCityCurrentProductionRate>);
int main(int argc,char** argv){try{need(argc==3,"Expected selected BNW save and shared installation");run(argv[1],argv[2]);return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
