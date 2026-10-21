#include "civ5/runtime/bnw_city_current_food_rate.hpp"
#include "civ5/runtime/bnw_city_registry.hpp"
#include "civ5/runtime/bnw_area_registry.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <iostream>
#include <thread>
#include <type_traits>
using namespace civ5;using namespace civ5::runtime;
namespace {
void need(bool b,const char* m){if(!b)throw std::runtime_error(m);}
const auto& named(const auto& rows,std::string_view name){for(const auto& r:rows)if(r.name&&std::string_view(r.name)==name)return r;throw std::runtime_error("Genuine fixture field absent");}
template<class F>void refuses(F&& f){bool denied=false;try{f();}catch(const BnwCityCurrentFoodRateUnavailable&){denied=true;}need(denied,"Current FOOD owner admitted unavailable authority");}
void run(const std::filesystem::path& save,const std::filesystem::path& game){
 const auto before=identify_file(save).sha256;auto boot=ui::bootstrap_selected_bnw_preview(save,game,0,true);
 if(boot.preview.blocker)throw std::runtime_error(boot.preview.blocker->reason);
 std::shared_ptr<ui::SelectedBnwPreviewHost> host(std::move(boot.preview.host));need(bool(host),"Genuine immutable selected host absent");
 auto session=std::shared_ptr<const LiveSession>(host,&host->session());
 auto database=std::shared_ptr<const content::RulesDatabase>(host,&host->database());
 auto rules=std::make_shared<const BnwCityFoodRateRules>(BnwCityFoodRateRules::load(database,host->identity().rules_sha256));
 std::optional<BnwCityCurrentFoodRate> retained;std::int64_t owner=-1,id=-1;std::size_t observed=0;
 const auto& state=host->session().state();
 for(const auto& p:state.world.all_players->players){for(const auto& raw:p.cities.cities){
  const auto city=named(raw.core.fields,"m_iID").values.at(0);
  const auto process=named(raw.core.fields,"m_aiProductionToYieldModifier").values.at(0);
  if(process!=0){refuses([&]{(void)inspect_bnw_city_current_food_rate(session,rules,p.saved_player_id,city);});continue;}
  auto result=inspect_bnw_city_current_food_rate(session,rules,p.saved_player_id,city);const auto value=result.value_times100();
  auto indexed=inspect_bnw_city_current_food_rate(session,rules,p.saved_player_id,city&8191);
  need(indexed.value_times100()==value,"Genuine native full/index city rate differs");
  // Independently exercise canonical borrowed lookup on the genuine map and
  // compare this production readnode to the preserved wide formula where its
  // reached intermediates were verified native-safe by the new node.
  auto math=state.cities;auto& m=math.players.at(p.saved_player_id);auto& c=m.cities.at(city);
  const auto* capital=resolve_bnw_city_yield_registry(state.world,state.cities,static_cast<std::int32_t>(p.saved_player_id),static_cast<std::int32_t>(m.capital));m.capital=capital?capital->id:-1;
  const auto* area=resolve_bnw_area_registry(state.world,static_cast<std::int32_t>(c.area));c.area=area?named(area->fields,"id").value:-1;
  const auto old_rules=load_city_yield_rules(host->database(),content::TechnologyProfile::bnw);
  need(city_yield_times100(math,old_rules,host->defines(),p.saved_player_id,city,0).total_times100==value,"Genuine FOOD owner and preserved wide formula differ");
  std::cout<<"Genuine FOOD owner="<<p.saved_player_id<<" city="<<city<<" rateTimes100="<<value<<"\n";
  if(!retained){retained=std::move(result);owner=p.saved_player_id;id=city;}
  if(++observed==3)break;
 }if(observed==3)break;}
 need(retained&&observed>0,"No actual process0 FOOD branch observed");
 bool thread_denied=false;std::thread worker([&]{try{(void)retained->value_times100();}catch(const BnwCityCurrentFoodRateUnavailable&){thread_denied=true;}});worker.join();need(thread_denied,"Foreign thread read source FOOD node");
 // A generic detached restoration does not possess the private import identity.
 auto generic=std::make_shared<const LiveSession>(LiveSession::from_restored_bnw(host->session().source(),host->session().state()));
 refuses([&]{(void)inspect_bnw_city_current_food_rate(generic,rules,owner,id);});generic.reset();
 refuses([&]{(void)inspect_bnw_city_current_food_rate(session,rules,owner,id+8192);});
 auto checkpoint=host->session().checkpoint();host->session().restore(checkpoint);
 refuses([&]{(void)retained->value_times100();});
 retained=inspect_bnw_city_current_food_rate(session,rules,owner,id);
 // The aliases carry the actual host control block, not no-op borrowed owners.
 auto mutable_session=std::shared_ptr<LiveSession>(host,&host->session());host.reset();database.reset();session.reset();
 need(retained->value_times100()==retained->value_times100(),"Strong actual source lifetime failed");
 (void)mutable_session->mutable_state(); // sanctioned revocation only, no mutation
 refuses([&]{(void)retained->value_times100();});
 need(identify_file(save).sha256==before,"Original save changed");
 std::cout<<"PASS actual original process0 FOOD readnodes observed="<<observed<<", current immutable rules, canonical source handles, thread/allocation/generation/lifetime/revocation; no refresh or growth authority\n";
}
}
static_assert(!std::is_default_constructible_v<BnwCityCurrentFoodRate>);
static_assert(!std::is_default_constructible_v<BnwCityFoodRateRules>);
int main(int argc,char** argv){try{need(argc==3,"Expected selected original BNW save and canonical shared installation");run(argv[1],argv[2]);return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
