#include "bnw_major_team_city_strength_fixture.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/runtime/bnw_city_registry.hpp"
#include <algorithm>
#include <map>
#include <string_view>
namespace civ5::runtime::tests {
namespace {
void check(bool value,const char* why){if(!value)throw std::runtime_error(why);}
const auto& field(const auto& fields,std::string_view name){
 const typename std::remove_reference_t<decltype(fields)>::value_type* found=nullptr;
 for(const auto& item:fields)if(item.name&&std::string_view(item.name)==name){check(!found,"Strength fixture duplicate field");found=&item;}
 check(found,"Strength fixture missing field");return *found;
}
std::vector<std::pair<std::int32_t,std::int32_t>> ordered_cities(const LiveSessionState& state,std::int64_t team){
 const auto& teams=state.world.pregame->archive.integer_arrays.at("team_types");
 check(teams.size()==64&&state.world.all_players->players.size()==64,"Strength fixture genuine roster missing");
 std::vector<std::pair<std::int32_t,std::int32_t>> result;
 for(std::int32_t owner=0;owner<64;++owner){const auto& p=state.world.all_players->players.at(static_cast<std::size_t>(owner));
  const auto& alive=field(p.core.fields,"m_bAlive").values;
  check(alive.size()==1&&(alive[0]==0||alive[0]==1),"Strength fixture actual alive malformed");
  if(!alive[0]||teams.at(static_cast<std::size_t>(owner))!=team)continue;
  (void)resolve_bnw_city_registry(state.world,state.cities,owner,0);
  std::vector<std::int32_t> ids;
  for(const auto& city:p.cities.cities){const auto& id=field(city.core.fields,"m_iID").values;
   check(id.size()==1&&id[0]>=0&&id[0]<=INT32_MAX,"Strength fixture actual city ID invalid");ids.push_back(static_cast<std::int32_t>(id[0]));}
  std::sort(ids.begin(),ids.end(),[](auto a,auto b){return (a&8191)<(b&8191);});
  for(const auto id:ids)result.emplace_back(owner,id);
 }return result;
}
bool unchanged_except_strength(const saves::SavedCityObservation& before,const saves::SavedCityObservation& after){
 if(before.core.fields.size()!=after.core.fields.size())return false;
 for(std::size_t i=0;i<before.core.fields.size();++i){const auto& a=before.core.fields[i];const auto& b=after.core.fields[i];
  if(std::string_view(a.name?a.name:"")!=std::string_view(b.name?b.name:"")||a.encoding!=b.encoding||a.wire_count!=b.wire_count||
   a.row_counts!=b.row_counts||a.row_keys!=b.row_keys||a.hashes!=b.hashes||a.bytes!=b.bytes)return false;
  if(std::string_view(a.name?a.name:"")!="m_iStrengthValue"&&a.values!=b.values)return false;
 }return true;
}
}
struct BnwMajorStrengthFixtureCapture::Impl {
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host;
 std::shared_ptr<const LiveSessionState> lease;
 std::uint64_t generation=0,revision=0,occurrence=0;
 std::size_t events=0;
 std::int64_t turn=-1,team=-1,technology=-1;
 SimulationRandomState rng,map_rng;
 std::vector<std::pair<std::int32_t,std::int32_t>> order;
 std::map<std::pair<std::int32_t,std::int32_t>,saves::SavedCityObservation> cities;
};
BnwMajorStrengthFixtureCapture::BnwMajorStrengthFixtureCapture(std::unique_ptr<Impl> p):impl_(std::move(p)){}
BnwMajorStrengthFixtureCapture::~BnwMajorStrengthFixtureCapture()=default;
std::shared_ptr<const BnwMajorStrengthFixtureCapture> BnwMajorStrengthFixtureCapture::capture_before(
 const ui::SelectedBnwReadOnlyPreview& preview,const BnwMajorTeamHasTechPending& pending){
 auto p=std::make_unique<Impl>();
 preview.with_owned_host_camera_input([&](auto host,const auto*){
  pending.require_stored_current(*host);
  check(pending.phase()==BnwMajorTeamHasTechPhase::city_strength_pending&&pending.city_strength_updates().empty(),
   "UNAVAILABLE genuine TeamSetHasTech return or strength already stored");
  p->host=host;p->lease=host->session().read_state_lease();p->generation=host->session().state_generation();
  p->revision=p->lease->ui_source_revision;p->turn=p->lease->setup.game_turn;
  p->team=pending.command().team;p->technology=pending.command().technology;
  p->rng=p->lease->random.gameplay.state();p->map_rng=p->lease->random.map.state();
  p->occurrence=p->lease->city_dirty_next_occurrence;p->events=p->lease->city_dirty_events.size();
  p->order=ordered_cities(*p->lease,p->team);
  for(const auto& [owner,id]:p->order){const auto* city=resolve_bnw_city_registry(p->lease->world,p->lease->cities,owner,id);
   check(city,"Strength fixture genuine native city missing");p->cities.emplace(std::pair{owner,id},*city);}
  pending.require_stored_current(*host);
 });return std::shared_ptr<const BnwMajorStrengthFixtureCapture>(new BnwMajorStrengthFixtureCapture(std::move(p)));
}
void BnwMajorStrengthFixtureCapture::verify_after(const ui::SelectedBnwReadOnlyPreview& preview,const BnwMajorTeamHasTechPending& pending)const{
 const auto& p=*impl_;
 preview.with_owned_host_camera_input([&](auto host,const auto*){
  check(host.get()==p.host.get()&&!host.owner_before(p.host)&&!p.host.owner_before(host),"Strength fixture actual Host substituted");
  pending.require_stored_current(*host);
  check(pending.phase()==BnwMajorTeamHasTechPhase::resource_reveal_pending,"Strength fixture missing actual per-city dirty returns");
  check(host->session().read_state_lease()==p.lease&&host->session().state_generation()==p.generation&&
   pending.command().team==p.team&&pending.command().technology==p.technology,"Strength fixture real native world replaced");
  const auto updates=pending.city_strength_updates();const auto& state=*p.lease;
  check(updates.size()==p.order.size()&&ordered_cities(state,p.team)==p.order,"Strength fixture native loop did not visit exact living members/FLTA cities");
  for(std::size_t i=0;i<updates.size();++i){const auto& update=updates[i];const auto key=p.order[i];
   check(key==std::pair{update.owner,update.city}&&update.dirty_occurrence==p.occurrence+i,"Strength fixture native city/dirty order differs");
   const auto* city=resolve_bnw_city_registry(state.world,state.cities,update.owner,update.city);
   check(city&&field(city->core.fields,"m_iStrengthValue").values==std::vector<std::int64_t>{update.after}&&
    field(p.cities.at(key).core.fields,"m_iStrengthValue").values==std::vector<std::int64_t>{update.before}&&
    unchanged_except_strength(p.cities.at(key),*city),"Strength fixture native stores changed unrelated city operands");
   const auto& event=state.city_dirty_events.at(p.events+i);
   check(event.occurrence==update.dirty_occurrence&&event.generation==p.generation&&event.owner==update.owner&&event.city==update.city&&
    event.kind==BnwCityDirtyKind::city_info&&event.value,"Strength fixture missing genuine ordered dirty event");
  }
  check(state.ui_source_revision==p.revision+updates.size()&&state.city_dirty_next_occurrence==p.occurrence+updates.size()&&
   state.city_dirty_events.size()==p.events+updates.size()&&state.setup.game_turn==p.turn&&
   state.random.gameplay.state()==p.rng&&state.random.map.state()==p.map_rng,"Strength fixture unexpected clock/RNG/revision/queue mutation");
  check(!host->session().readiness().can_continue(),"Strength prefix falsely granted full BNW continuation");
 });
}
}
