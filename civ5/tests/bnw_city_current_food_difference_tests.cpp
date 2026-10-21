#include "civ5/runtime/bnw_city_current_food_difference.hpp"
#include "civ5/runtime/bnw_city_current_food_consumption.hpp"
#include "civ5/runtime/bnw_city_food_production_order.hpp"
#include "civ5/runtime/bnw_city_specialist_counts.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <climits>
#include <iostream>
#include <thread>
#include <type_traits>
using namespace civ5;using namespace civ5::runtime;
namespace {
void need(bool b,const char* m){if(!b)throw std::runtime_error(m);}
std::int32_t field(const auto& city,std::string_view name){
 const saves::CityCoreField* out=nullptr;
 for(const auto& r:city.core.fields)if(r.name&&std::string_view(r.name)==name){need(!out,"Duplicate actual fixture scalar");out=&r;}
 need(out&&out->values.size()==1&&out->values.front()>=INT32_MIN&&out->values.front()<=INT32_MAX,"Actual fixture scalar/native width absent");return static_cast<std::int32_t>(out->values.front());
}
template<class F>std::string refusal(F&& f){try{f();}catch(const BnwCityCurrentFoodDifferenceUnavailable& e){return e.what();}throw std::runtime_error("Source FOOD difference admitted unavailable owner");}
std::int32_t native(std::int64_t n){need(n>=INT32_MIN&&n<=INT32_MAX,"Specialist native32 operand/intermediate unavailable");return static_cast<std::int32_t>(n);}
const auto& named(const auto& rows,std::string_view name){for(const auto& r:rows)if(r.name&&std::string_view(r.name)==name)return r;throw std::runtime_error("Actual pure oracle field absent");}
// Explicit pure EXPECTATION from the authentic unchanged snapshot. This never
// supplies counts/flags to the private owner or edits an issuing live state.
std::int32_t half_food_expectation(const auto& raw,const content::RulesDatabase& db){
 const auto definitions=db.cache_global_defines(content::GlobalDefineProfile::bnw);
 const auto define=[&](const char* name){const auto* d=definitions.find(name);need(d&&d->lookup.found&&d->lookup.error.empty()&&std::holds_alternative<std::int64_t>(d->lookup.value),"Actual pure oracle define absent");return native(std::get<std::int64_t>(d->lookup.value));};
 const auto per_pop=define("FOOD_CONSUMPTION_PER_POPULATION");auto total=native(std::int64_t(field(raw,"m_iPopulation"))*per_pop);
 const auto types=db.cache_info_types();need(types.report().all_succeeded()&&!types.report().name_overwrites&&!types.report().hash_collisions,"Specialist native global hash collision/overwrite provenance unavailable");
 const auto report=db.prefetch_info("Specialists",[](auto slot,auto id,const auto&){need(slot<=16*1024*1024&&id==static_cast<std::int64_t>(slot),"Specialist native slot/authored ID provenance unavailable");return true;});need(report.all_succeeded()&&!report.overlapping_ids&&report.slots<=16*1024*1024,"Specialist actual native Info span unavailable");
 std::vector<detail::BnwSpecialistMathRow> rows;const auto& citizens=named(raw.management.blocks,"citizens");const auto& counts=named(citizens.hashes,"m_aiSpecialistCounts");
 need(counts.wire_count>=0&&static_cast<std::uint64_t>(counts.wire_count)==counts.rows.size(),"Specialist unsigned native wire count differs");
 for(const auto& row:counts.rows){need(row.type_hash<=UINT32_MAX,"Specialist raw hash exceeds native uint32");if(!row.type_hash){need(row.values.empty(),"Specialist null hash carries a value");rows.push_back({{}, {}});continue;}need(row.values.size()==1,"Specialist nonnull raw row lacks exactly one value");const auto value=native(row.values.front());const auto* type=types.find_hash(row.type_hash);if(!type){rows.push_back({{},value});continue;}need(type->table=="Specialists","Specialist reached foreign-table global remap provenance unavailable");rows.push_back({native(type->id),value});}
 const auto count=detail::bnw_specialist_reset_read_math(static_cast<std::int32_t>(report.slots),define("DEFAULT_SPECIALIST"),rows).total;
 return native(std::int64_t(total)-native(std::int64_t(count)*per_pop)/2);
}
void run(const std::filesystem::path& save,const std::filesystem::path& game){
 const auto before=identify_file(save).sha256;auto boot=ui::bootstrap_selected_bnw_preview(save,game,0,true);
 if(boot.preview.blocker)throw std::runtime_error(boot.preview.blocker->reason);
 std::shared_ptr<ui::SelectedBnwPreviewHost> host(std::move(boot.preview.host));need(bool(host),"Genuine immutable selected host absent");
 auto session=std::shared_ptr<const LiveSession>(host,&host->session());
 auto database=std::shared_ptr<const content::RulesDatabase>(host,&host->database());
 const auto sha=host->identity().rules_sha256;
 auto rules=std::make_shared<const BnwCityFoodRateRules>(BnwCityFoodRateRules::load(database,sha));
 std::optional<BnwCityCurrentFoodDifference> retained;std::int64_t first_owner=-1,first_id=-1,retained_owner=-1,retained_id=-1;
 bool retained_bottom=true;std::int32_t retained_value=0;
 std::size_t cities=0,supported_nonpositive=0,supported_positive=0,foodproduction=0,bottom=0,positive_refusals=0,source_refusals=0,positive_religion_status=0,positive_wltkd_counter=0;
 const auto& state=session->state();
 for(const auto& player:state.world.all_players->players){for(const auto& raw:player.cities.cities){
  if(cities==24)break;++cities;
  const auto owner=player.saved_player_id,id=std::int64_t(field(raw,"m_iID"));
  if(first_owner==-1){first_owner=owner;first_id=id;}
  // Independent expectations read only this authentic imported state. Neither
  // queue, owner, policy, food stock nor any issuing session operand is edited.
  std::optional<std::int32_t> rate,consume;bool prod=false;std::string source_error;
  try{
   const auto& orders=state.cities.players.at(owner).cities.at(id).production_orders;need(orders.has_value(),"Actual fixture queue absent");
   std::shared_ptr<const BnwCityFoodProductionRules> units;
   if(!orders->empty()&&orders->front().type==0)units=BnwCityFoodProductionRules::load(*database,sha);
   const auto head=inspect_bnw_city_food_production_order(state,std::move(units),sha,owner,id);prod=head.food_production();
   rate=inspect_bnw_city_current_food_rate(session,rules,owner,id).value_times100();
   if(named(player.core.fields,"m_iHalfSpecialistFoodCount").values.at(0)>0)consume=half_food_expectation(raw,*database);
   else consume=inspect_bnw_city_current_food_consumption(state,*database,sha,owner,id,0).value();
  }catch(const std::exception& e){source_error=e.what();}
  if(!source_error.empty()){
   need(refusal([&]{(void)inspect_bnw_city_current_food_difference(session,rules,owner,id);})==source_error,"Closed source frontier differs from actual child getter");
   ++source_refusals;std::cout<<"Actual FOOD difference owner="<<owner<<" city="<<id<<" sourceRefusal="<<source_error<<'\n';continue;
  }
  if(prod)++foodproduction;
  for(bool at_bottom:{false,true}){
   std::optional<std::int32_t> pop,stock;if(at_bottom){pop=field(raw,"m_iPopulation");if(*pop==1)stock=field(raw,"m_iFood");}
   const auto expected=detail::bnw_food_difference_unmodified_times100(prod,*rate,*consume,at_bottom,pop,stock);
   if(at_bottom&&pop==std::optional<std::int32_t>{1}&&stock&&*stock/100==0)++bottom;
   if(expected>0){
    // Authentic issuing state is unchanged. A missing source leaf remains
    // reported; it is never filled with a detached test DTO or invented flag.
    try{
     auto result=inspect_bnw_city_current_food_difference(session,rules,owner,id,at_bottom);const auto value=result.value_times100();
     auto index=inspect_bnw_city_current_food_difference(session,rules,owner,id&8191,at_bottom);need(index.value_times100()==value,"Native index/full positive source differs");++supported_positive;
     if(!state.cities.players.at(owner).cities.at(id).religions.empty())++positive_religion_status;
     if(field(raw,"m_iWeLoveTheKingDayCounter")>0)++positive_wltkd_counter;
     if(!retained){retained=std::move(result);retained_owner=owner;retained_id=id;retained_bottom=at_bottom;retained_value=value;}
     std::cout<<"Actual positive FOOD owner="<<owner<<" city="<<id<<" unmodified="<<expected<<" modified="<<value<<'\n';
    }catch(const BnwCityCurrentFoodDifferenceUnavailable& e){
     const std::string message=e.what();need(message!="Positive FOOD difference requires actual capital/city/religion/peace/happiness/WLTKD growth modifier owners","Old blanket positive frontier survived");
     need(refusal([&]{(void)inspect_bnw_city_current_food_difference(session,rules,owner,id&8191,at_bottom);})==message,"Native index/full positive source frontier differs");++positive_refusals;
     std::cout<<"Actual positive source refusal="<<message<<'\n';
    }
   }else{
    auto result=inspect_bnw_city_current_food_difference(session,rules,owner,id,at_bottom);need(result.value_times100()==expected,"Actual source FOOD difference differs");
    auto index=inspect_bnw_city_current_food_difference(session,rules,owner,id&8191,at_bottom);need(index.value_times100()==expected,"Native index/full difference differs");++supported_nonpositive;
    if(!retained){retained=std::move(result);retained_owner=owner;retained_id=id;retained_bottom=at_bottom;retained_value=expected;}
   }
   std::cout<<"Actual FOOD difference owner="<<owner<<" city="<<id<<" foodProduction="<<prod<<" bBottom="<<at_bottom<<" unmodifiedTimes100="<<expected<<'\n';
  }
 }if(cities==24)break;}
 need(cities>0,"No authentic imported city observed");
 (void)refusal([&]{(void)inspect_bnw_city_current_food_difference(session,rules,first_owner,INT64_MAX);});
 (void)refusal([&]{(void)inspect_bnw_city_current_food_difference(session,rules,INT64_MIN,first_id);});
 (void)refusal([&]{(void)inspect_bnw_city_current_food_difference(session,rules,first_owner,first_id+8192);});
 auto generic=std::make_shared<const LiveSession>(LiveSession::from_restored_bnw(session->source(),session->state()));
 (void)refusal([&]{(void)inspect_bnw_city_current_food_difference(generic,rules,first_owner,first_id);});generic.reset();
 bool foreign_denied=false;std::thread worker([&]{try{(void)inspect_bnw_city_current_food_difference(session,rules,first_owner,first_id);}catch(const BnwCityCurrentFoodDifferenceUnavailable&){foreign_denied=true;}});worker.join();need(foreign_denied,"Foreign thread borrowed actual source");
 if(retained){
  auto moved=std::move(*retained);(void)refusal([&]{(void)retained->value_times100();});retained=std::move(moved);
  auto checkpoint=host->session().checkpoint();host->session().restore(checkpoint);(void)refusal([&]{(void)retained->value_times100();});
  retained=inspect_bnw_city_current_food_difference(session,rules,retained_owner,retained_id,retained_bottom);need(retained->value_times100()==retained_value,"Genuine checkpoint numeric result changed");
  auto mutable_session=std::shared_ptr<LiveSession>(host,&host->session());host.reset();session.reset();database.reset();need(retained->value_times100()==retained_value,"Closed child did not retain exact host/session/DB lifetime");
  (void)mutable_session->mutable_state();(void)refusal([&]{(void)retained->value_times100();});
 }
 need(identify_file(save).sha256==before,"Original save changed");
 std::cout<<"PASS actual cities="<<cities<<" supportedNonpositive="<<supported_nonpositive<<" supportedPositive="<<supported_positive<<" foodProduction="<<foodproduction<<" bBottomPop1WholeFood0="<<bottom<<" reachedPositiveSourceRefusal="<<positive_refusals<<" childSourceRefusal="<<source_refusals<<" positiveCityReligionStatusPresent="<<positive_religion_status<<" positiveCityWltkdCounterAbove0="<<positive_wltkd_counter<<'\n';
 if(!supported_nonpositive)std::cout<<"UNOBSERVED genuine successful nonpositive/lifetime/generation branch\n";
 if(!foodproduction)std::cout<<"UNOBSERVED genuine food-production city\n";
 if(!bottom)std::cout<<"UNOBSERVED genuine bBottom population1 wholeFood0\n";
 if(!supported_positive)std::cout<<"UNOBSERVED genuine completed positive numeric source\n";
 if(!positive_religion_status)std::cout<<"UNOBSERVED positive city with actual religion statuses (does not certify found majority/secondary)\n";
 if(!positive_wltkd_counter)std::cout<<"UNOBSERVED positive city WLTKD counter (counter presence alone does not certify reached happy branch)\n";
 std::cout<<"No growth/refresh/fresh-city/predecessor/turn grant\n";
}
}
static_assert(!std::is_default_constructible_v<BnwCityCurrentFoodDifference>);
int main(int argc,char** argv){try{need(argc==3,"Expected original BNW save and canonical shared installation");run(argv[1],argv[2]);return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
