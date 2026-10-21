#include "civ5/runtime/bnw_city_specialist_counts.hpp"
#include "civ5/runtime/bnw_city_current_food_difference.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <iostream>
#include <thread>
#include <type_traits>
using namespace civ5;using namespace civ5::runtime;
namespace {
void need(bool b,const char* m){if(!b)throw std::runtime_error(m);}
const auto& named(const auto& rows,std::string_view name){for(const auto& r:rows)if(r.name&&std::string_view(r.name)==name)return r;throw std::runtime_error("Actual count fixture field absent");}
template<class F>void refuses(F&& f){bool denied=false;try{f();}catch(const BnwCityCurrentFoodRateUnavailable&){denied=true;}need(denied,"Closed specialist source accepted unavailable rate authority");}
void run(const std::filesystem::path& save,const std::filesystem::path& game){
 const auto before=identify_file(save).sha256;auto boot=ui::bootstrap_selected_bnw_preview(save,game,0,true);if(boot.preview.blocker)throw std::runtime_error(boot.preview.blocker->reason);
 std::shared_ptr<ui::SelectedBnwPreviewHost> host(std::move(boot.preview.host));need(bool(host),"Actual immutable selected host missing");
 auto session=std::shared_ptr<const LiveSession>(host,&host->session());auto db=std::shared_ptr<const content::RulesDatabase>(host,&host->database());
 auto rules=std::make_shared<const BnwCityFoodRateRules>(BnwCityFoodRateRules::load(db,host->identity().rules_sha256));
 std::size_t observed=0,half_unhappy=0,half_unhappy_closed=0,half_food=0,half_closed=0,readable=0,frontiers=0;
 std::optional<BnwCityCurrentFoodRate> retained;std::int64_t owner=-1,id=-1;
 const auto& state=session->state();
 for(const auto& p:state.world.all_players->players){for(const auto& c:p.cities.cities){if(observed==24)break;++observed;
  const auto city=named(c.core.fields,"m_iID").values.at(0);
  const bool uh=state.unhappiness.players.at(p.saved_player_id).half_specialists;
  const bool food=named(p.core.fields,"m_iHalfSpecialistFoodCount").values.at(0)>0;
  if(uh)++half_unhappy;if(food)++half_food;
  try{
   auto node=inspect_bnw_city_current_food_rate(session,rules,p.saved_player_id,city);const auto rate=node.value_times100();
   auto index=inspect_bnw_city_current_food_rate(session,rules,p.saved_player_id,city&8191);need(index.value_times100()==rate,"Actual specialist parent native handle resolution differs");++readable;
   if(uh&&!state.cities.no_happiness&&!state.unhappiness.players.at(p.saved_player_id).cities.at(city).ignore)++half_unhappy_closed;
   if(!retained){retained=std::move(node);owner=p.saved_player_id;id=city;}
  }catch(const BnwCityCurrentFoodRateUnavailable& e){++frontiers;std::cout<<"Actual rate/count frontier owner="<<p.saved_player_id<<" city="<<city<<" reason="<<e.what()<<'\n';}
  if(food){
   try{auto diff=inspect_bnw_city_current_food_difference(session,rules,p.saved_player_id,city,false);need(diff.value_times100()<=0,"Positive incomplete modifiers were issued");++half_closed;}
   catch(const BnwCityCurrentFoodDifferenceUnavailable& e){const std::string_view reason=e.what();if(reason=="Positive FOOD difference requires actual capital/city/religion/peace/happiness/WLTKD growth modifier owners")++half_closed;else{++frontiers;std::cout<<"Actual half-food frontier owner="<<p.saved_player_id<<" city="<<city<<" reason="<<reason<<'\n';}}
  }
 }if(observed==24)break;}
 need(observed>0,"No actual city observed");
 if(retained){
  bool foreign=false;std::thread worker([&]{try{(void)retained->value_times100();}catch(const BnwCityCurrentFoodRateUnavailable&){foreign=true;}});worker.join();need(foreign,"Foreign thread read closed count parent");
  auto generic=std::make_shared<const LiveSession>(LiveSession::from_restored_bnw(session->source(),session->state()));refuses([&]{(void)inspect_bnw_city_current_food_rate(generic,rules,owner,id);});
  auto checkpoint=host->session().checkpoint();host->session().restore(checkpoint);refuses([&]{(void)retained->value_times100();});
  retained=inspect_bnw_city_current_food_rate(session,rules,owner,id);auto mutable_session=std::shared_ptr<LiveSession>(host,&host->session());host.reset();session.reset();db.reset();(void)retained->value_times100();
  (void)mutable_session->mutable_state();refuses([&]{(void)retained->value_times100();});
 }
 need(identify_file(save).sha256==before,"Original save changed");
 std::cout<<"SOURCE SCENES observed="<<observed<<" readableRate="<<readable<<" halfUnhappinessPolicy="<<half_unhappy<<" reachedHalfUnhappinessClosed="<<half_unhappy_closed<<" halfFoodPolicy="<<half_food<<" reachedHalfFoodClosed="<<half_closed<<" typedFrontiers="<<frontiers<<'\n';
 if(!half_unhappy_closed)std::cout<<"UNOBSERVED actual supported reached half-specialist unhappiness scene\n";
 if(!half_food||!half_closed)std::cout<<"UNOBSERVED actual supported reached half-specialist food scene\n";
 if(!retained)std::cout<<"UNOBSERVED current-rate lifetime/generation branch\n";
 std::cout<<"No detached issuing state, count/current/fresh flag, citizens writer, refresh, growth or continuation grant\n";
}
}
static_assert(!std::is_default_constructible_v<BnwCitySpecialistCounts>);
int main(int argc,char** argv){try{need(argc==3,"Expected original BNW save and canonical shared installation");run(argv[1],argv[2]);return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
