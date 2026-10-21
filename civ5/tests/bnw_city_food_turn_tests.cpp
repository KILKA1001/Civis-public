#include "civ5/runtime/bnw_city_food_turn.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <array>
#include <climits>
#include <iostream>
#include <string_view>
#include <type_traits>
using namespace civ5;
namespace {
void check(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
template<class F>void refuses(F&& f){bool failed=false;try{f();}catch(const std::exception&){failed=true;}check(failed,"Invalid city food-turn accepted");}
template<class F>void needs_food(F&& f){bool failed=false;try{f();}catch(const runtime::BnwCityFoodTurnUnavailable& e){failed=true;check(std::string_view(e.what()).find("refreshed food yield")!=std::string_view::npos,"Mandatory food producer frontier lost");}check(failed,"Normal city skipped mandatory food producers");}
auto& named(auto& rows,std::string_view name){for(auto& r:rows)if(r.name&&std::string_view(r.name)==name)return r;throw std::runtime_error("Selected raw field missing");}
struct Pick{std::size_t player_slot,city_slot;std::int64_t owner,id;};
Pick choose(const runtime::LiveSessionState& s){
 for(std::size_t p=0;p<s.world.all_players->players.size();++p){const auto& player=s.world.all_players->players[p];if(!player.cities.cities.empty())return {p,0,player.saved_player_id,named(player.cities.cities.front().core.fields,"m_iID").values.front()};}
 throw std::runtime_error("Selected founded city missing");
}
auto& raw(auto& s,const Pick& p){return s.world.all_players->players.at(p.player_slot).cities.cities.at(p.city_slot);}
auto& live(auto& s,const Pick& p){return s.cities.players.at(p.owner).cities.at(p.id);}
void razing(runtime::LiveSessionState& s,const Pick& p,std::int64_t n){named(raw(s,p).core.fields,"m_iRazingTurns").values.front()=n;live(s,p).razing=n>0;}
auto food(const auto& s,const Pick& p){return std::array<std::int64_t,3>{named(raw(s,p).core.fields,"m_iFood").values.front(),named(raw(s,p).core.fields,"m_iFoodKept").values.front(),named(raw(s,p).core.fields,"m_iPopulation").values.front()};}
void selected(const std::filesystem::path& path,const std::filesystem::path& game_root){
 const auto source_hash=runtime::identify_file(path).sha256;
 auto boot=ui::bootstrap_selected_bnw_preview(path,game_root,0);
 if(boot.preview.blocker)throw std::runtime_error(boot.preview.blocker->reason);
 check(bool(boot.preview.host),"Actual selected BNW host missing");
 const auto& baseline=boot.preview.host->session().state();const auto p=choose(baseline);const auto original_food=food(baseline,p);
 {
  auto actual=baseline;const auto turns=named(raw(actual,p).core.fields,"m_iRazingTurns").values.front();
  if(turns>0)check(runtime::apply_bnw_city_food_turn(actual,p.owner,p.id).razing_turns()==turns,"Actual imported razing branch differs");
  else needs_food([&]{(void)runtime::apply_bnw_city_food_turn(actual,p.owner,p.id);});
  check(food(actual,p)==original_food,"Unmodified imported branch changed food/population");
 }
 // Explicit finite source-predicate fixtures on a genuinely decoded/current
 // city registry. These are not native fresh-yield/population-turn receipts.
 for(const auto turns:{std::int64_t(1),std::int64_t(7),std::int64_t(INT32_MAX)}){
  auto s=baseline;razing(s,p,turns);const auto before=food(s,p);
  const auto step=runtime::apply_bnw_city_food_turn(s,p.owner,p.id);
  check(step.owner()==p.owner&&step.city()==p.id&&step.razing_turns()==turns&&step.returned_for_razing()&&!step.changed_food_or_population()&&!step.full_growth_ready(),"Source IsRazing branch/result differs");
  check(food(s,p)==before,"Razing branch changed food/population");
  const auto index=runtime::apply_bnw_city_food_turn(s,p.owner,p.id&8191);
  check(index.city()==p.id,"Native index-only FLTA handle did not resolve actual city");
  // Source early return must not inspect any later food/yield/queue operand.
  named(raw(s,p).core.fields,"m_iFood").name="unreachable_food";
  named(raw(s,p).core.fields,"m_iFoodKept").name="unreachable_food_kept";
  named(raw(s,p).core.fields,"m_iPopulation").name="unreachable_population";
  live(s,p).production_orders.reset();live(s,p).yields.clear();
  check(runtime::apply_bnw_city_food_turn(s,p.owner,p.id).returned_for_razing(),"Razing branch read unreachable growth operand");
 }
 for(const auto turns:{std::int64_t(0),std::int64_t(-1),std::int64_t(INT32_MIN)}){
  auto s=baseline;razing(s,p,turns);const auto before=food(s,p);
  needs_food([&]{(void)runtime::apply_bnw_city_food_turn(s,p.owner,p.id);});
  check(food(s,p)==before,"Normal refusal partially changed food/population");
  // A public saved zero and bottom/population-one case cannot mint freshness.
  named(raw(s,p).core.fields,"m_iFood").values.front()=0;
  named(raw(s,p).core.fields,"m_iPopulation").values.front()=1;live(s,p).population=1;
  needs_food([&]{(void)runtime::apply_bnw_city_food_turn(s,p.owner,p.id);});
 }
 {
  auto s=baseline;razing(s,p,1);live(s,p).razing=false;const auto before=food(s,p);
  refuses([&]{(void)runtime::apply_bnw_city_food_turn(s,p.owner,p.id);});check(food(s,p)==before,"Stale razing mirror wrote state");
  razing(s,p,0);live(s,p).razing=true;refuses([&]{(void)runtime::apply_bnw_city_food_turn(s,p.owner,p.id);});
 }
 {
  auto s=baseline;razing(s,p,1);const auto before=food(s,p);
  const auto foreign_full_id=(p.id&8191)|((p.id&~std::int64_t(8191))==8192?std::int64_t(16384):std::int64_t(8192));
  for(const auto handle:{std::int64_t(-1),std::int64_t(INT32_MAX)+1,foreign_full_id})refuses([&]{(void)runtime::apply_bnw_city_food_turn(s,p.owner,handle);});
  for(const auto owner:{std::int64_t(-1),std::int64_t(64),std::int64_t(INT32_MAX)+1})refuses([&]{(void)runtime::apply_bnw_city_food_turn(s,owner,p.id);});
  check(food(s,p)==before,"Absent/malformed handle wrote state");
 }
 for(const auto kind:{0,1,2,3}){
  auto s=baseline;razing(s,p,1);auto& fields=raw(s,p).core.fields;
  if(kind==0)fields.push_back(named(fields,"m_iRazingTurns"));
  if(kind==1)named(fields,"m_iRazingTurns").encoding=saves::CityCoreEncoding::vector_i32;
  if(kind==2)named(fields,"m_iRazingTurns").values.front()=std::int64_t(INT32_MAX)+1;
  if(kind==3)named(fields,"m_iRazingTurns").name="missing_razing";
  refuses([&]{(void)runtime::apply_bnw_city_food_turn(s,p.owner,p.id);});
 }
 {
  auto s=baseline;razing(s,p,1);live(s,p).source_core.reset();refuses([&]{(void)runtime::apply_bnw_city_food_turn(s,p.owner,p.id);});
  s=baseline;razing(s,p,1);s.setup.profile=content::TechnologyProfile::vp542;refuses([&]{(void)runtime::apply_bnw_city_food_turn(s,p.owner,p.id);});
  s=baseline;s.world.all_players.reset();refuses([&]{(void)runtime::apply_bnw_city_food_turn(s,p.owner,p.id);});
 }
 check(food(baseline,p)==original_food&&runtime::identify_file(path).sha256==source_hash,"Original session/save changed");
 std::cout<<"PASS actual decoded city IsRazing source early return, FLTA/raw-owned guards, unreachable food operands and normal fresh-food refusal; full doGrowth unavailable\n";
}
}
static_assert(!std::is_default_constructible_v<runtime::BnwCityFoodTurnStep>);
int main(int argc,char** argv){try{check(argc==3,"Expected existing BNW save and canonical game root");selected(argv[1],argv[2]);return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
