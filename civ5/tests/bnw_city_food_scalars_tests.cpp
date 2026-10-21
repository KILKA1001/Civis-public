#include "civ5/runtime/bnw_city_food_scalars.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/saves/loaded_save.hpp"
#include <filesystem>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <type_traits>
using namespace civ5;
namespace {
void check(bool b,const char* m){if(!b)throw std::runtime_error(m);}
template<class F> void refuses(F&& f){bool b=false;try{f();}catch(const std::exception&){b=true;}check(b,"Invalid food scalar operation accepted");}
saves::CityCoreField& field(saves::SavedCityObservation& c,std::string_view n){for(auto& f:c.core.fields)if(f.name&&std::string_view(f.name)==n)return f;throw std::runtime_error("Selected raw field absent");}
struct Pick{std::size_t p,c;};
Pick choose(const saves::SavedSessionSnapshot& w){for(std::size_t p=0;p<w.all_players->players.size();++p)if(!w.all_players->players[p].cities.cities.empty())return {p,0};throw std::runtime_error("Founded city absent");}
saves::SavedCityObservation& raw(saves::SavedSessionSnapshot& w,Pick p){return w.all_players->players[p.p].cities.cities[p.c];}
auto owner(saves::SavedSessionSnapshot& world,const saves::SavedSessionSnapshot& source,const runtime::SessionCheckpointIdentity& id,Pick p){return runtime::import_bnw_city_food_scalars(source,world,id,world.all_players->players[p.p].saved_player_id,field(raw(world,p),"m_iID").values.front());}
void selected(const std::filesystem::path& path){
 const auto savehash=runtime::identify_file(path).sha256;
 saves::SavedSessionDecodeContext dc;dc.decode_map_regions=true;dc.decode_teams=true;dc.decode_all_players=true;dc.decode_supporting_classes=true;dc.all_players=saves::AllPlayersContext{};
 auto loaded=saves::read_saved_session(path,dc);
 check(loaded.snapshot.all_players&&loaded.snapshot.all_players->players.size()==64,"Complete decoded BNW roster absent");
 const runtime::SessionCheckpointIdentity id{runtime::identify_bytes(loaded.source.bytes).sha256,std::string(64,'a')};
 auto source=std::move(loaded.snapshot);const auto p=choose(source);
 auto world=source;auto o=owner(world,source,id,p);const auto before=o->current();
 auto query=runtime::stage_bnw_city_food_scalar_change(o,153);
 auto journal=runtime::apply_bnw_city_food_scalar_change(std::move(query));
 check(journal.before()==before&&journal.after()==o->current(),"Journal/raw mirror disagreement");
 check(o->current().food_times100==before.food_times100+153&&o->current().food_kept==before.food_kept+1,"Native scalar source delta mismatch");
 check(field(raw(world,p),"m_iFood").values.front()==o->current().food_times100&&field(raw(world,p),"m_iFoodKept").values.front()==o->current().food_kept,"Real decoded raw city not changed");
 check(o->current().population==before.population&&!journal.full_growth_ready()&&std::string_view(journal.growth_frontier()).find("population")!=std::string_view::npos,"Scalar writer claimed growth/population effects");
 refuses([&]{(void)runtime::apply_bnw_city_food_scalar_change(std::move(query));});
 auto cp=runtime::make_bnw_city_core_checkpoint(world,source,id);auto restored=source;
 runtime::restore_bnw_city_core_checkpoint(cp,restored,source,id);
 auto restored_owner=owner(restored,source,id,p);check(restored_owner->current()==journal.after(),"Food raw/mirror checkpoint roundtrip mismatch");
 auto q1=runtime::stage_bnw_city_food_scalar_change(o,1);auto q2=runtime::stage_bnw_city_food_scalar_change(o,2);
 (void)runtime::apply_bnw_city_food_scalar_change(std::move(q1));const auto after=o->current();
 refuses([&]{(void)runtime::apply_bnw_city_food_scalar_change(std::move(q2));});check(o->current()==after,"Stale query partially wrote food");
 auto moved=runtime::stage_bnw_city_food_scalar_change(o,0);auto valid=std::move(moved);
 refuses([&]{(void)runtime::apply_bnw_city_food_scalar_change(std::move(moved));});
 auto zero=runtime::apply_bnw_city_food_scalar_change(std::move(valid));check(zero.after()==after,"Zero setter changed scalar values");
 bool thread_refused=false;std::thread t([&]{try{(void)runtime::stage_bnw_city_food_scalar_change(o,1);}catch(const std::exception&){thread_refused=true;}});t.join();check(thread_refused,"Foreign thread accepted");
 // Transparent finite setter fixtures retain the actual decoded city/schema.
 // These are NOT refreshed yield, growth threshold or native doGrowth receipts.
 for(const auto delta:{-1,-99,-100,-199,0,99,100}){
  auto finite=source;field(raw(finite,p),"m_iFood").values.front()=0;field(raw(finite,p),"m_iFoodKept").values.front()=10;
  auto f=owner(finite,source,id,p);auto q=runtime::stage_bnw_city_food_scalar_change(f,delta);
  auto j=runtime::apply_bnw_city_food_scalar_change(std::move(q));
  check(j.after().food_times100==delta&&j.after().food_kept==10+delta/100&&j.after().food()==delta/100,"Negative fractional source truncation mismatch");
  check(j.after().population==j.before().population&&!j.full_growth_ready(),"Setter crossed mandatory growth effects");
 }
 {
  auto stale=source;auto f=owner(stale,source,id,p);auto q=runtime::stage_bnw_city_food_scalar_change(f,10);
  ++field(raw(stale,p),"m_iFood").values.front();const auto keep=field(raw(stale,p),"m_iFoodKept").values.front();
  refuses([&]{(void)runtime::apply_bnw_city_food_scalar_change(std::move(q));});check(field(raw(stale,p),"m_iFoodKept").values.front()==keep,"Raw stale refusal partially wrote kept");
 }
 {
  auto stale=source;auto f=owner(stale,source,id,p);auto q=runtime::stage_bnw_city_food_scalar_change(f,10);
  // Selected nonmirror scalar is guarded by the codec's complete preflight.
  auto& health=field(raw(stale,p),"m_iDamage").values.front();++health;const auto food=f->current().food_times100;
  refuses([&]{(void)runtime::apply_bnw_city_food_scalar_change(std::move(q));});check(f->current().food_times100==food,"Checkpoint stale refusal partially wrote selected food");
 }
 {
  auto expired=source;auto f=owner(expired,source,id,p);auto q=runtime::stage_bnw_city_food_scalar_change(f,1);f.reset();
  refuses([&]{(void)runtime::apply_bnw_city_food_scalar_change(std::move(q));});
 }
 for(const auto name:{"m_iFood","m_iFoodKept"}){
  auto edge=source;field(raw(edge,p),name).values.front()=INT32_MAX;auto f=owner(edge,source,id,p);const auto old=f->current();
  refuses([&]{(void)runtime::stage_bnw_city_food_scalar_change(f,100);});check(f->current()==old,"Overflow partially changed world");
 }
 refuses([&]{(void)runtime::stage_bnw_city_food_scalar_change(o,std::int64_t(INT32_MAX)+1);});
 check(runtime::identify_file(path).sha256==savehash,"Original Civ5Save changed");
 check(field(raw(source,p),"m_iFood").values.front()==before.food_times100,"Immutable source city changed");
 std::cout<<"PASS genuine decoded BNW food/kept scalar writes, negative fractions, stale/one-shot/thread/overflow guards, checkpoint roundtrip; full doGrowth unavailable\n";
}
}
static_assert(!std::is_default_constructible_v<runtime::BnwCityFoodScalarOwner>);
static_assert(!std::is_default_constructible_v<runtime::BnwCityFoodScalarQuery>);
static_assert(!std::is_copy_constructible_v<runtime::BnwCityFoodScalarQuery>);
static_assert(std::is_nothrow_move_constructible_v<runtime::BnwCityFoodScalarQuery>);
static_assert(!std::is_default_constructible_v<runtime::BnwCityFoodScalarJournal>);
int main(int argc,char** argv){try{check(argc==2,"Supply selected BNW save");selected(argv[1]);return 0;}catch(const std::exception& e){std::cerr<<"FAIL food scalars: "<<e.what()<<'\n';return 1;}}
