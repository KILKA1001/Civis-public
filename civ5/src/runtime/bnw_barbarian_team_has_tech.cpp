#include "civ5/runtime/bnw_selected_user_options_owner.hpp"
#include "civ5/runtime/bnw_game_active_team_read.hpp"
#include "civ5/runtime/bnw_happiness_update_prefix.hpp"
#include "civ5/runtime/bnw_happiness_update_suffix.hpp"
#include <cmath>
#include "civ5/runtime/bnw_team_tech_best_route.hpp"
#include "civ5/runtime/bnw_team_tech_route_build.hpp"
#include "civ5/runtime/bnw_barbarian_team_has_tech.hpp"
#include <array>
#include <climits>
#include <stdexcept>

namespace civ5::runtime {

BnwBarbarianTeamHasTechResult write_bnw_barbarian_team_has_tech(
    TeamTechnologies &barbarians,std::int64_t technology,LuaScriptHost *host) {
    if(barbarians.profile()!=content::TechnologyProfile::bnw ||
       barbarians.team()!=63 || technology<0 || technology>INT32_MAX ||
       static_cast<std::uint64_t>(technology)>=barbarians.slots())
        return {BnwBarbarianTeamHasTechStatus::rejected,false,
                "BNW barbarian TeamSetHasTech identity or technology invalid"};
    if(barbarians.has_tech(technology))
        return {BnwBarbarianTeamHasTechStatus::rejected,false,
                "BNW barbarian already owns technology"};
    if(!host || !host->loading_complete())
        return {BnwBarbarianTeamHasTechStatus::rejected,false,
                "BNW barbarian TeamSetHasTech requires loaded gameplay Lua"};

    bool handled=false;
    try {
        const auto changed=barbarians.set_has_tech(
            technology,true,[&](std::int64_t team,std::int64_t tech,bool owned) {
                const std::array<ScriptValue,3> args{team,tech,owned};
                const auto outcome=host->call_hook("TeamSetHasTech",args);
                if(!outcome.errors.empty())
                    throw ScriptError("BNW TeamSetHasTech callback failed: " +
                                      outcome.errors.front());
                handled=outcome.value.has_value();
            });
        if(!changed)
            return {BnwBarbarianTeamHasTechStatus::uncertain,handled,
                    "BNW barbarian technology changed during preflight"};
        return {BnwBarbarianTeamHasTechStatus::committed,handled,{}};
    } catch(const std::exception &error) {
        return {BnwBarbarianTeamHasTechStatus::uncertain,handled,error.what()};
    }
}

} // namespace civ5::runtime


#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/runtime/bnw_minor_city_strength.hpp"
#include "civ5/runtime/bnw_city_registry.hpp"
#include <algorithm>
#include <atomic>
#include <limits>
#include <map>
#include <optional>
#include <thread>
#include <type_traits>
#include <typeindex>
namespace civ5::runtime {
namespace {
constexpr auto major_bnw=saves::GameStateLayout::expansion2_version1;
void major_need(bool b,const char* s){if(!b)throw BnwMajorTeamHasTechUnavailable(s);}
template<class C>const typename C::value_type& major_named(const C& rows,std::string_view key){
 const typename C::value_type* result=nullptr;
 for(const auto& r:rows)if(r.name&&std::string_view(r.name)==key){major_need(!result,"Duplicate BNW major tech wire operand");result=&r;}
 major_need(result,"Missing BNW major tech wire operand");return *result;
}
template<class C>typename C::value_type& major_named(C& rows,std::string_view key){
 typename C::value_type* result=nullptr;
 for(auto& r:rows)if(r.name&&std::string_view(r.name)==key){major_need(!result,"Duplicate BNW major tech mutable operand");result=&r;}
 major_need(result,"Missing BNW major tech mutable operand");return *result;
}
bool major_same_owned(const TeamTechnologies& a,const TeamTechnologies& b){
 return a.profile()==b.profile()&&a.team()==b.team()&&a.slots()==b.slots()&&a.last_tech()==b.last_tech()&&
  a.nondefault_values()==b.nondefault_values()&&a.profile_extensions().values.empty()&&b.profile_extensions().values.empty();
}
}
struct BnwMajorTeamHasTechPending::Impl {
 struct Registry {std::shared_ptr<Impl> pending;};
 const std::thread::id thread=std::this_thread::get_id();
 mutable std::atomic<bool> retired{false},busy{false};
 std::weak_ptr<const ui::SelectedBnwPreviewHost> host;
 std::weak_ptr<LiveSessionState> state;
 const LiveSession* facade=nullptr;
 std::function<void()> facade_current;
 std::function<bool()> rules_current;
 const saves::SavedSession* source=nullptr;
 std::uint64_t generation=0,revision=0;
 std::vector<std::int64_t> pregame_teams;
 std::vector<bool> pregame_minor;
 BnwMajorTeamHasTechCommand request;
 BnwMajorTeamHasTechPhase stage=BnwMajorTeamHasTechPhase::team_set_has_tech_pending;
 std::unique_ptr<TeamTechnologies> after_owned;
 saves::TeamObservation after_raw;
 enum class Attempt { none,team_set_has_tech,team_tech_researched };
 Attempt attempt=Attempt::none;
 bool registered=false,city_queue_ready=false;
 std::vector<std::pair<std::int32_t,std::int32_t>> city_queue;
 std::size_t next_city=0;
 std::vector<BnwMajorTeamCityStrengthUpdate> strengths;
 std::optional<BnwMajorProcessTechPrefixObservation> process_prefix;
 std::weak_ptr<const BnwMajorPlayerTraitsOwner> traits_owner;
 std::int32_t traits_actor=-1;
 std::optional<BnwMajorPlayerTraitsResetEffect> traits_effect;
 std::optional<BnwMajorPolicyFreeUnitEffect> policy_free_unit;
 bool researched_completed=false;
 std::optional<BnwMajorAfterResearchEffect> after_research;
 std::unique_ptr<BnwTeamTechPlayerResearchPrepared> player_research;
 std::optional<BnwMajorAnnouncementShareEffect> announcement_share;
 BnwMajorTechMessageQueue* message_queue=nullptr;
 std::optional<BnwMajorEspionagePrefixEffect> espionage_prefix;
 BnwMajorTechEspionageCache* espionage_cache=nullptr;
 std::vector<saves::PlayerEspionageRecord*> espionage_lists;
 std::unique_ptr<BnwHappinessPrefixRules> happiness_rules;
 std::optional<BnwMajorHappinessPrefixEffect> happiness_prefix;
 std::optional<std::int32_t> happiness_stored;
 bool happiness_founder_attempt=false,happiness_foreign_attempt=false;
 std::int64_t next_happiness_owner=0;
 std::optional<std::int32_t> happiness_expected_excess;
 std::vector<BnwMajorHappinessSuffixEffect> happiness_suffixes;
 BnwMajorHappinessPublication* happiness_publication=nullptr;
 BnwMajorTechUiTransportQueue* ui_transport=nullptr;
 std::vector<BnwMajorTechUiTransportEvent> ui_expected;
 bool ui_initialized=false;
 void publish_ui(std::vector<BnwMajorTechUiTransportEvent> batch,LiveSessionState& s){
  major_need(ui_transport&&ui_expected.size()+batch.size()<=26&&s.ui_source_revision<=UINT64_MAX-batch.size(),
   "BNW genuine UI transport capacity/revision exhausted");
  const auto first=ui_expected.size();
  for(std::size_t i=0;i<batch.size();++i){auto& event=batch[i];event.team=request.team;event.technology=request.technology;
   event.turn=s.setup.game_turn;event.generation=generation;event.occurrence=static_cast<std::uint64_t>(first+i)+1;
  }
  // All string/vector allocations and diagnostic copies precede publication.
  ui_expected.insert(ui_expected.end(),batch.begin(),batch.end());
  for(auto& event:batch){ui_transport->events.at(ui_transport->written++)=std::move(event);++s.ui_source_revision;}
  revision=s.ui_source_revision;
 }

 static std::vector<std::pair<std::int32_t,std::int32_t>> queue(const LiveSessionState& s,std::int64_t team){
  major_need(s.world.pregame&&s.world.all_players&&s.world.all_players->players.size()==64,
   "BNW major strength actual member roster missing");
  const auto& teams=s.world.pregame->archive.integer_arrays.at("team_types");
  major_need(teams.size()==64,"BNW major strength actual GetTeam roster missing");
  std::vector<std::pair<std::int32_t,std::int32_t>> result;
  for(std::int32_t owner=0;owner<64;++owner){const auto& player=s.world.all_players->players.at(static_cast<std::size_t>(owner));
   major_need(player.core.layout==major_bnw&&player.index==static_cast<std::uint64_t>(owner)&&player.saved_player_id==owner&&
    major_named(player.core.fields,"m_eID").values==std::vector<std::int64_t>{owner},"BNW major strength actual member ID differs");
   const auto& alive=major_named(player.core.fields,"m_bAlive").values;
   major_need(alive.size()==1&&(alive[0]==0||alive[0]==1),"BNW major strength actual alive encoding invalid");
   if(!alive[0]||teams.at(static_cast<std::size_t>(owner))!=team)continue;
   major_need(s.setup.players.at(owner).alive&&s.setup.players.at(owner).team==team,
    "BNW major strength raw-owned living team member differs");
   // Validate Count/capacity/free slots and owned roster even when empty.
   // The lookup validates the whole authentic registry before returning null.
   (void)resolve_bnw_city_registry(s.world,s.cities,owner,0);
   std::vector<std::int32_t> ids;
   for(const auto& raw:player.cities.cities){const auto& id=major_named(raw.core.fields,"m_iID").values;
    major_need(id.size()==1&&id[0]>=0&&id[0]<=INT32_MAX,"BNW major strength native city ID invalid");
    const auto value=static_cast<std::int32_t>(id[0]);
    major_need(resolve_bnw_city_registry(s.world,s.cities,owner,value)==&raw,
     "BNW major strength actual FLTA city/source/owned binding differs");ids.push_back(value);
   }
   // CvPlayer firstCity/nextCity -> FLTA BeginIter/NextIter: slot order,
   // not vector order or full generation-bearing ID order.
   std::sort(ids.begin(),ids.end(),[](auto a,auto b){return (a&8191)<(b&8191);});
   for(const auto id:ids)result.emplace_back(owner,id);
  }return result;
 }
 struct Guard {
  std::shared_ptr<Impl> p;
  explicit Guard(std::shared_ptr<Impl> value):p(std::move(value)){
   major_need(p&&p->thread==std::this_thread::get_id(),"BNW major tech foreign thread before source borrow");
   major_need(!p->retired.load(),"BNW major tech pending retired");
   if(p->busy.exchange(true)){p->retired.store(true);throw BnwMajorTeamHasTechUnavailable("BNW major tech pending reentry");}
  }
  ~Guard(){p->busy.store(false);}
 };
 static saves::TeamObservation& raw(LiveSessionState& s,std::int64_t team){
  major_need(s.world.teams&&s.world.teams->layout==major_bnw,"BNW major tech actual raw roster missing");
  saves::TeamObservation* found=nullptr;
  for(auto& row:s.world.teams->teams)if(major_named(row.fields,"id").value==team){major_need(!found,"BNW major tech duplicate raw team");found=&row;}
  major_need(found,"BNW major tech actual raw team missing");return *found;
 }
 static TeamTechnologies& owned(LiveSessionState& s,std::int64_t team){
  TeamTechnologies* found=nullptr;
  for(auto& row:s.technologies)if(row.team()==team){major_need(!found,"BNW major tech duplicate owned team");found=&row;}
  major_need(found,"BNW major tech actual owned team missing");return *found;
 }
 static std::size_t wire_slot(const saves::TeamObservation& r,const ui::SelectedBnwPreviewHost& h,std::int64_t tech){
  const auto& hashes=major_named(r.arrays,"tech_hashes");const auto& flags=major_named(r.arrays,"has_tech");
  major_need(hashes.values.size()==flags.values.size(),"BNW major tech raw flag/hash span mismatch");
  const auto* info=h.technologies().find(static_cast<std::uint64_t>(tech));
  major_need(info&&!info->base.type.empty(),"BNW major tech selected Info missing");
  const auto* mapped=h.types().find(info->base.type);
  major_need(mapped&&mapped->table=="Technologies"&&mapped->id==tech&&
   mapped->hash==content::saved_info_type_hash(info->base.type),"BNW major tech canonical type remap changed");
  std::optional<std::size_t> result;
  for(std::size_t i=0;i<hashes.values.size();++i)if(hashes.values[i]==static_cast<std::int64_t>(mapped->hash)){
   major_need(!result,"BNW major tech duplicate requested wire flag");result=i;}
  major_need(result.has_value(),"BNW major tech requested flag absent from actual wire");return *result;
 }
 std::shared_ptr<const ui::SelectedBnwPreviewHost> check(const ui::SelectedBnwPreviewHost* supplied=nullptr)const{
  major_need(thread==std::this_thread::get_id()&&!retired.load(),"BNW major tech source thread/lifetime retired");
  const auto h=host.lock();const auto s=state.lock();
  major_need(h&&s&&(!supplied||h.get()==supplied),"BNW major tech foreign Host or dropped source");
  facade_current();
  const auto lease=h->session().read_state_lease();
  major_need(&h->session()==facade&&lease.get()==s.get()&&!lease.owner_before(s)&&!s.owner_before(lease)&&
   &h->session().source()==source&&h->session().state_generation()==generation&&
   s->ui_source_revision==revision&&s->setup.game_turn==request.expected_turn,
   "BNW major tech SAME allocation/generation/source/revision changed");
  major_need(rules_current&&rules_current(),"BNW major tech selected rules changed");
  (void)h->session().bnw_game_debug_mode();
  major_need(s->world.pregame&&s->world.pregame->layout==major_bnw,
   "BNW major actual Pregame owner disappeared");
  const auto& archive=s->world.pregame->archive;
  const auto teams=archive.integer_arrays.find("team_types");
  const auto minors=archive.flag_arrays.find("minor_nation_civs");
  major_need(teams!=archive.integer_arrays.end()&&minors!=archive.flag_arrays.end()&&
   teams->second==pregame_teams&&minors->second==pregame_minor&&pregame_teams.size()==64&&pregame_minor.size()==64&&
   pregame_teams.at(static_cast<std::size_t>(request.player))==request.team&&
   !pregame_minor.at(static_cast<std::size_t>(request.player)),"BNW major actual Pregame GetTeam/minor binding changed");
  major_need(s->world.all_players&&s->world.all_players->layout==major_bnw&&s->world.all_players->players.size()==64,
   "BNW major actual raw player roster disappeared");
  const auto& native_actor=s->world.all_players->players.at(static_cast<std::size_t>(request.player));
  major_need(native_actor.saved_player_id==request.player&&native_actor.index==static_cast<std::uint64_t>(request.player)&&
   major_named(native_actor.core.fields,"m_eID").values==std::vector<std::int64_t>{request.player}&&
   major_named(native_actor.core.fields,"m_bAlive").values==std::vector<std::int64_t>{1},
   "BNW major actual raw player ID/alive changed");
  const auto& actor=s->setup.players.at(request.player);
  major_need(actor.alive&&actor.team==request.team&&s->player_research.major_players.contains(request.player)&&
   !s->player_research.players.at(request.player).pregame_minor,"BNW major tech actor/major relation changed");
  if(registered){
   const auto* registry=s->setup.extensions.find<Registry>();
   major_need(registry&&registry->pending.get()==this,"BNW major tech pending history replaced");
   const auto decoded=restore_team_technologies(raw(*s,request.team),h->technologies(),h->types());
   major_need(after_owned&&major_same_owned(owned(*s,request.team),*after_owned)&&major_same_owned(decoded,*after_owned)&&
    decoded.remap().all_mapped()&&!decoded.remap().duplicates,"BNW major tech paired poststate changed");
   const auto& current=raw(*s,request.team);
   major_need(major_named(current.arrays,"has_tech").values==major_named(after_raw.arrays,"has_tech").values&&
    major_named(current.arrays,"tech_hashes").values==major_named(after_raw.arrays,"tech_hashes").values&&
    major_named(current.fields,"last_tech").value==major_named(after_raw.fields,"last_tech").value,
    "BNW major tech exact stored flags/hash/last-tech changed");
  }
  if(city_queue_ready){
   major_need(queue(*s,request.team)==city_queue,"BNW major strength actual native iteration roster changed");
   for(const auto& update:strengths){const auto* raw=resolve_bnw_city_registry(s->world,s->cities,update.owner,update.city);
    major_need(raw&&major_named(raw->core.fields,"m_iStrengthValue").values==std::vector<std::int64_t>{update.after},
     "BNW major strength stored raw cache changed");
    const auto* stored=s->cities.players.at(update.owner).cities.at(update.city).extensions.find<BnwMinorCityStrengthScratch::LiveStored>();
    major_need(stored&&stored->strength==update.after&&stored->known_technologies==update.known_technologies&&stored->generation==generation,
     "BNW major strength stored owned cache changed");
   }
  }
  if(after_research){
   major_need(major_named(s->world.game_state.flags,"end_game_tech_researched").value==after_research->ends_game_after,
    "BNW actual post-researched EndsGame scalar changed");
   if(after_research->best_route_stored)major_need(major_named(raw(*s,request.team).fields,"best_route").value==after_research->best_route_after,
    "BNW actual post-researched best route scalar changed");
  }
  if(player_research)player_research->require_stored(*s);
  if(message_queue)major_need(announcement_share&&s->setup.extensions.find<BnwMajorTechMessageQueue>()==message_queue&&
   message_queue->messages==announcement_share->messages&&message_queue->written==message_queue->messages.size(),
   "BNW genuine AddMessage queue/source contents changed");
  if(espionage_cache){
   major_need(espionage_prefix&&s->setup.extensions.find<BnwMajorTechEspionageCache>()==espionage_cache&&
    espionage_cache->player==request.player&&espionage_cache->generation==generation&&espionage_lists.size()==22,
    "BNW actual espionage paired native mirror/source differs");
   const auto& native=major_named(s->world.all_players->players.at(static_cast<std::size_t>(request.player)).espionage.blocks,"player_espionage");
   major_need(major_named(native.arrays,"m_aiMaxTechCost").values==espionage_cache->maximum_cost&&
    major_named(native.arrays,"m_aiNumTechsToStealList").values==espionage_cache->number_to_steal,
    "BNW espionage paired maximum/steal counters changed");
   for(std::size_t i=0;i<22;++i){
    const saves::PlayerEspionageRecord* child=nullptr;
    for(const auto& row:native.children)if(row.name&&std::string_view(row.name)=="stealable_techs"&&row.index==i){major_need(!child,"BNW duplicate espionage target after publication");child=&row;}
    major_need(child==espionage_lists[i]&&child&&major_named(child->arrays,"techs").values==espionage_cache->lists.at(i)&&
     major_named(child->fields,"techs_wire_count").value==static_cast<std::int64_t>(espionage_cache->lists.at(i).size()),
     "BNW actual paired espionage list/target shape changed");
   }
  }
  if(happiness_stored){
   major_need(happiness_prefix&&happiness_prefix->player>=0&&happiness_prefix->player<22,
    "BNW genuine happiness member receipt differs");
   const auto actor=happiness_prefix->player;const auto& f=major_named(s->world.all_players->players.at(static_cast<std::size_t>(actor)).core.fields,"m_iHappiness");
   major_need(f.encoding==saves::PlayerCoreEncoding::i32&&f.values==std::vector<std::int64_t>{*happiness_stored}&&
    s->unhappiness.players.at(actor).happiness==*happiness_stored&&s->cities.players.at(actor).excess_happiness==happiness_expected_excess,
    "BNW genuine paired happiness scalar/invalidation changed");
  }
  if(happiness_publication){
   major_need(s->setup.extensions.find<BnwMajorHappinessPublication>()==happiness_publication&&
    happiness_publication->events.size()==22&&happiness_publication->written==happiness_suffixes.size()&&happiness_publication->pending_ui_consumption,
    "BNW genuine happiness dirty publication/history replaced");
   for(std::size_t i=0;i<happiness_suffixes.size();++i){const auto& effect=happiness_suffixes[i];const auto& event=happiness_publication->events[i];
    const auto& core=s->world.all_players->players.at(static_cast<std::size_t>(effect.player)).core;
    major_need(!effect.stores.empty()&&event==BnwMajorHappinessDirtyEvent{effect.player,effect.stores.back().happiness,effect.connection_after,effect.excess,generation,i+1,
     BnwTeamTechResearchDirtyBit::game_data,true,effect.achievement_requested}&&
     major_named(core.fields,"m_iHappiness").values==std::vector<std::int64_t>{event.happiness}&&
     major_named(core.fields,"m_iCityConnectionHappiness").values==std::vector<std::int64_t>{event.connections}&&
     s->unhappiness.players.at(effect.player).happiness==event.happiness&&s->cities.players.at(effect.player).excess_happiness==event.excess,
     "BNW genuine happiness paired completed member/dirty operands changed");
   }
  }
  if(ui_transport){
   major_need(s->setup.extensions.find<BnwMajorTechUiTransportQueue>()==ui_transport&&ui_transport->pending_consumer&&
    ui_transport->events.size()==26&&ui_transport->written==ui_expected.size(),"BNW genuine tech UI transport replaced/false completion");
   for(std::size_t i=0;i<ui_expected.size();++i)major_need(ui_transport->events.at(i)==ui_expected.at(i),
    "BNW genuine tech UI transport operands/source order changed");
  }
  facade_current();return h;
 }
};
BnwMajorTeamHasTechPending::BnwMajorTeamHasTechPending(std::shared_ptr<Impl> p):impl_(std::move(p)){}
BnwMajorTeamHasTechPending::~BnwMajorTeamHasTechPending()=default;
std::shared_ptr<const BnwMajorTeamHasTechPending> BnwMajorTeamHasTechPending::begin_nonrepeat_acquisition(
 const ui::SelectedBnwReadOnlyPreview& preview,const BnwMajorTeamHasTechCommand& command){
 std::shared_ptr<const BnwMajorTeamHasTechPending> result;
 preview.with_owned_host_camera_input([&](auto h,const auto*){
  result=begin_for_source_host(std::move(h),command);
 });major_need(static_cast<bool>(result),"BNW major selected source handoff absent");return result;
}
std::shared_ptr<const BnwMajorTeamHasTechPending> BnwMajorTeamHasTechPending::begin_for_source_host(
 std::shared_ptr<const ui::SelectedBnwPreviewHost> h,const BnwMajorTeamHasTechCommand& command,
 std::shared_ptr<const BnwMajorPlayerTraitsOwner>* issued_traits_owner){
 std::shared_ptr<const BnwMajorTeamHasTechPending> result;
  major_need(!issued_traits_owner||!*issued_traits_owner,"BNW pre-flag Traits output storage already occupied");
  major_need(h&&command.team>=0&&command.team<22&&command.player>=0&&command.player<22&&command.technology>=0&&
   command.technology<=INT32_MAX,"BNW major nonrepeat acquisition request outside supported domain");
  auto p=std::make_shared<Impl>();p->host=h;p->request=command;p->facade=h->session_.get();
  auto& session=*h->session_;
  major_need(session.bnw_game_debug_owner_current(),"BNW major acquisition genuine Game source unavailable");
  const auto token=session.retain_bnw_facade_lifetime();p->facade_current=[token]{token->require_current();};
  p->state=session.state_;p->source=session.source_.get();p->generation=session.state_generation_;
  p->revision=session.state_->ui_source_revision;
  // Ordinary selected Hosts own an unsealed activated clone. The immutable
  // fast guard exists only for the explicitly sealed Early-package route.
  // Both paths retain the SAME real Host/DB and exact selected rules identity.
  const auto* actual_rules=&h->database();
  const auto rules_identity=h->identity().rules_sha256;
  major_need(!rules_identity.empty()&&actual_rules->gameplay_fingerprint_sha256()==rules_identity,
   "BNW major acquisition selected rules identity differs");
  const auto sealed_current=actual_rules->immutable_preview_fingerprint_currentness();
  const std::weak_ptr<const ui::SelectedBnwPreviewHost> rules_host=h;
  p->rules_current=[rules_host,actual_rules,rules_identity,sealed_current] {
   const auto current=rules_host.lock();
   if(!current||&current->database()!=actual_rules||current->identity().rules_sha256!=rules_identity)return false;
   // A poisoned sealed scope must never fall back to unsealed hash admission.
   if(sealed_current)return sealed_current();
   return actual_rules->gameplay_fingerprint_sha256()==rules_identity;
  };
  auto& s=*session.state_;
  major_need(s.world.pregame&&s.world.pregame->layout==major_bnw,
   "BNW major acquisition actual Pregame owner missing");
  const auto& archive=s.world.pregame->archive;
  const auto teams=archive.integer_arrays.find("team_types");
  const auto minors=archive.flag_arrays.find("minor_nation_civs");
  major_need(teams!=archive.integer_arrays.end()&&teams->second.size()==64&&
   minors!=archive.flag_arrays.end()&&minors->second.size()==64,
   "BNW major acquisition complete Pregame GetTeam/minor arrays missing");
  p->pregame_teams=teams->second;p->pregame_minor=minors->second;
  major_need(command.expected_generation==p->generation&&command.expected_ui_source_revision==p->revision&&
   command.expected_turn==s.setup.game_turn&&p->revision<std::numeric_limits<std::uint64_t>::max(),"BNW major acquisition stale/overflow command");
  major_need(s.setup.profile==content::TechnologyProfile::bnw&&h->technologies().profile==content::TechnologyProfile::bnw&&
   h->technologies().all_succeeded()&&h->types().report().all_succeeded()&&!h->types().report().hash_collisions&&
   !h->types().report().name_overwrites,"BNW major acquisition rules/type catalog incomplete");
  const auto* info=h->technologies().find(static_cast<std::uint64_t>(command.technology));
  major_need(info&&info->booleans.contains("Repeat")&&!info->booleans.at("Repeat"),"BNW major repeat/missing technology requires prior repeat effects");
  // The only pre-SetHasTech achievement branch is TECH_SATELLITES. Keep it
  // pending conservatively; no Huns/world-size/Steam achievement skip flag.
  major_need(info->base.type!="TECH_SATELLITES","BNW Satellites pre-flag achievement branch requires actual source evaluation");
  major_need(!s.game_clock.pending&&!s.player_research_publication&&!s.player_early_publication,
   "BNW major acquisition prior native publications unresolved");
  major_need(s.world.all_players&&s.world.all_players->layout==major_bnw&&
   static_cast<std::size_t>(command.player)<s.world.all_players->players.size(),"BNW major acquisition raw actor missing");
  const auto& raw_player=s.world.all_players->players.at(static_cast<std::size_t>(command.player));
  major_need(raw_player.saved_player_id==command.player&&raw_player.index==static_cast<std::uint64_t>(command.player)&&
   major_named(raw_player.core.fields,"m_eID").values==std::vector<std::int64_t>{command.player}&&
   major_named(raw_player.core.fields,"m_bAlive").values==std::vector<std::int64_t>{1},"BNW major acquisition raw/owned actor identity differs");
  p->check();
  auto& raw=Impl::raw(s,command.team);auto& owned=Impl::owned(s,command.team);
  const auto decoded=restore_team_technologies(raw,h->technologies(),h->types());
  major_need(major_same_owned(decoded,owned)&&decoded.remap().all_mapped()&&!decoded.remap().duplicates&&
   owned.profile()==content::TechnologyProfile::bnw&&owned.slots()==h->technologies().collection.slots&&
   !owned.has_tech(command.technology),"BNW major actual paired prestate incomplete or technology already owned");
  auto& flags=major_named(raw.arrays,"has_tech");auto& last=major_named(raw.fields,"last_tech");
  const auto slot=Impl::wire_slot(raw,*h,command.technology);
  major_need(flags.values.at(slot)==0&&major_named(raw.fields,"alive_count").value>0,
   "BNW major native flag or living team differs from actual request");
  // CvTeam::isMinorCiv examines actual living slots22..63. Do not infer
  // major-team classification from team ID or the selected actor alone.
  major_need(s.world.all_players->players.size()==64,"BNW major full actual player roster missing");
  bool minor_team=false;
  for(std::size_t id=22;id<64;++id){const auto& player=s.world.all_players->players[id];
   const auto& alive=major_named(player.core.fields,"m_bAlive");
   major_need(player.index==id&&player.saved_player_id==static_cast<std::int64_t>(id)&&
    major_named(player.core.fields,"m_eID").values==std::vector<std::int64_t>{static_cast<std::int64_t>(id)}&&
    alive.values.size()==1&&(alive.values[0]==0||alive.values[0]==1),"BNW major full actual player ID/alive invalid");
   if(alive.values[0]&&p->pregame_teams[id]==command.team){
    if(p->pregame_minor[id]){minor_team=true;break;}
    minor_team=false;break;
   }
  }
  major_need(!minor_team,"BNW actual minor team requires its own acquisition owner");
  const auto* old_registry=s.setup.extensions.template find<Impl::Registry>();
  major_need(!old_registry||!old_registry->pending,"BNW major prior technology continuation unresolved; no replay/overlap");
  std::shared_ptr<const BnwMajorPlayerTraitsOwner> prepared_traits;
  if(issued_traits_owner){
   for(std::int32_t actor=0;actor<64;++actor){
    const auto& member=s.world.all_players->players.at(static_cast<std::size_t>(actor));
    const auto& ever=major_named(member.core.fields,"m_bEverAlive").values;
    major_need(ever.size()==1&&(ever[0]==0||ever[0]==1)&&s.setup.players.at(actor).ever_alive==(ever[0]!=0)&&
     s.setup.players.at(actor).team==p->pregame_teams.at(static_cast<std::size_t>(actor)),"BNW pre-flag actual member binding differs");
    if(!ever[0]||p->pregame_teams.at(static_cast<std::size_t>(actor))!=command.team)continue;
    prepared_traits=BnwMajorPlayerTraitsOwner::from_source_host(h,actor);p->traits_actor=actor;break;
   }
   major_need(static_cast<bool>(prepared_traits),"BNW pre-flag first genuine Traits member unavailable");
   prepared_traits->require_current();p->traits_owner=prepared_traits;p->check();
  }
  const auto old_flags=flags.values;const auto old_last=last.value;
  p->after_owned=std::make_unique<TeamTechnologies>(owned);
  major_need(p->after_owned->set_has_tech(command.technology,true),"BNW major staged flag unchanged");
  p->after_raw=raw;major_named(p->after_raw.arrays,"has_tech").values[slot]=1;
  major_named(p->after_raw.fields,"last_tech").value=command.technology;
  const auto after_decoded=restore_team_technologies(p->after_raw,h->technologies(),h->types());
  major_need(major_same_owned(after_decoded,*p->after_owned),"BNW major staged paired poststate differs");
  auto staged_values=p->after_owned->nondefault_values();
  // Validate replace_live_values's entire finite throw domain BEFORE stores.
  for(const auto& [id,value]:staged_values)major_need(id<owned.slots()&&id<=static_cast<std::uint64_t>(INT64_MAX)&&
   value!=TeamTechnologyValue{},"BNW major staged native slot invalid");
  std::map<std::type_index,std::any> staged;
  staged.emplace(std::type_index(typeid(Impl::Registry)),std::any(Impl::Registry{p}));
  auto node=staged.extract(staged.begin());
  result=std::shared_ptr<const BnwMajorTeamHasTechPending>(new BnwMajorTeamHasTechPending(p));
  p->check();
  const auto final_decoded=restore_team_technologies(raw,h->technologies(),h->types());
  major_need(major_same_owned(final_decoded,owned)&&major_same_owned(owned,decoded)&&flags.values==old_flags&&last.value==old_last&&
   s.setup.extensions.template find<Impl::Registry>()==old_registry,"BNW major source operands changed during preparation");
  p->check();
  static_assert(std::is_nothrow_move_assignable_v<decltype(staged_values)>);
  // Actual TeamTechnologies address and SAME LiveSession allocation remain.
  // This existing setter only checks the prevalidated domain then moves map
  // values and assigns last_. It performs no callbacks or further allocation.
  owned.replace_live_values(command.technology,std::move(staged_values));
  flags.values[slot]=1;last.value=command.technology;
  auto& extensions=s.setup.extensions.values;const auto found=extensions.find(node.key());
  if(found==extensions.end())extensions.insert(std::move(node));else found->second.swap(node.mapped());
  ++s.ui_source_revision;++p->revision;p->registered=true;
  if(issued_traits_owner){*issued_traits_owner=std::move(prepared_traits);}
  // Intentionally no Lua call/no Traits reset here. The real SourceOwner must
  // adopt this actual new revision and dispatch TeamSetHasTech on SAME VM.
 major_need(static_cast<bool>(result),"BNW major selected source handoff absent");return result;
}
BnwMajorTeamHasTechPhase BnwMajorTeamHasTechPending::phase()const {
 Impl::Guard g(impl_);return impl_->stage;
}
const BnwMajorTeamHasTechCommand& BnwMajorTeamHasTechPending::command()const {
 Impl::Guard g(impl_);return impl_->request;
}
std::uint64_t BnwMajorTeamHasTechPending::after_revision()const {
 Impl::Guard g(impl_);return impl_->revision;
}
void BnwMajorTeamHasTechPending::require_stored_current(const ui::SelectedBnwPreviewHost& h)const {
 Impl::Guard g(impl_);try{impl_->check(&h);}catch(...){impl_->retired.store(true);throw;}
}
void BnwMajorTeamHasTechPending::begin_team_set_has_tech_dispatch(const ui::SelectedBnwPreviewHost& h)const {
 Impl::Guard g(impl_);try{
  major_need(impl_->stage==BnwMajorTeamHasTechPhase::team_set_has_tech_pending&&impl_->attempt==Impl::Attempt::none,"BNW major TeamSetHasTech repeat or uncertain attempt");
  impl_->check(&h);impl_->attempt=Impl::Attempt::team_set_has_tech;impl_->stage=BnwMajorTeamHasTechPhase::uncertain;
 }catch(...){impl_->retired.store(true);throw;}
}
void BnwMajorTeamHasTechPending::finish_team_set_has_tech_dispatch(const ui::SelectedBnwPreviewHost& h)const {
 Impl::Guard g(impl_);try{
  major_need(impl_->stage==BnwMajorTeamHasTechPhase::uncertain&&impl_->attempt==Impl::Attempt::team_set_has_tech,"BNW major TeamSetHasTech actual attempt missing");
  impl_->check(&h);impl_->attempt=Impl::Attempt::none;impl_->stage=BnwMajorTeamHasTechPhase::city_strength_pending;
 }catch(...){impl_->retired.store(true);throw;}
}
}

namespace civ5::runtime {
std::vector<BnwMajorTeamCityStrengthUpdate> BnwMajorTeamHasTechPending::city_strength_updates()const {
 Impl::Guard guard(impl_);return impl_->strengths;
}
std::optional<BnwMajorTeamCityStrengthUpdate> BnwMajorTeamHasTechPending::advance_city_strength(
 const ui::SelectedBnwPreviewHost& supplied)const {
 const auto p=impl_;Impl::Guard guard(p);
 try{
  major_need(p->stage==BnwMajorTeamHasTechPhase::city_strength_pending,
   "BNW major city strength requires actual completed TeamSetHasTech/previous owned dirty publication");
  const auto host=p->check(&supplied);const auto state=p->state.lock();major_need(static_cast<bool>(state),"BNW major strength actual world expired");
  if(!p->city_queue_ready){auto queue=Impl::queue(*state,p->request.team);p->check(&supplied);
   p->city_queue=std::move(queue);p->city_queue_ready=true;}
  major_need(p->next_city<=p->city_queue.size(),"BNW major native city iterator corrupt");
  if(p->next_city==p->city_queue.size()){
   p->check(&supplied);p->stage=BnwMajorTeamHasTechPhase::resource_reveal_pending;return std::nullopt;
  }
  const auto [owner,city]=p->city_queue[p->next_city];
  const auto q=BnwMinorCityStrengthScratch::quote_same_live(*state,*host,owner,city);
  auto& owned=state->cities.players.at(owner).cities.at(city);
  const auto* old=owned.extensions.find<BnwMinorCityStrengthScratch::LiveStored>();
  major_need(!old||(old->generation==p->generation&&old->strength==q.before),"BNW major strength prior owned cache stale");
  std::map<std::type_index,std::any> staged;
  staged.emplace(std::type_index(typeid(BnwMinorCityStrengthScratch::LiveStored)),
   std::any(BnwMinorCityStrengthScratch::LiveStored{q.after,q.known,p->generation}));
  auto node=staged.extract(staged.begin());
  const auto occurrence=state->city_dirty_next_occurrence;
  major_need(occurrence>0&&occurrence<UINT64_MAX&&p->revision<UINT64_MAX,"BNW major strength occurrence/revision overflow");
  auto events=state->city_dirty_events;std::uint64_t previous=0;
  for(const auto& event:events){major_need(event.generation==p->generation&&event.occurrence>previous&&event.occurrence<occurrence,
    "BNW major strength dirty event ordering changed");previous=event.occurrence;}
  events.push_back({occurrence,p->generation,owner,city,-1,-1,BnwCityDirtyKind::city_info,true});
  const BnwMajorTeamCityStrengthUpdate result{owner,city,q.before,q.after,q.known,q.plot,occurrence};
  auto updates=p->strengths;updates.push_back(result);
  saves::CityCoreField* target=nullptr;
  for(auto& raw:state->world.all_players->players.at(static_cast<std::size_t>(owner)).cities.cities)
   if(&raw==q.raw)target=&major_named(raw.core.fields,"m_iStrengthValue");
  major_need(target&&target->values==std::vector<std::int64_t>{q.before},"BNW major strength actual destination changed");
  p->check(&supplied);
  const auto final=BnwMinorCityStrengthScratch::quote_same_live(*state,*host,owner,city);
  major_need(final.raw==q.raw&&final.city==q.city&&final.before==q.before&&final.after==q.after&&final.known==q.known&&final.plot==q.plot&&
   owned.extensions.find<BnwMinorCityStrengthScratch::LiveStored>()==old&&state->city_dirty_events.size()+1==events.size()&&
   state->city_dirty_next_occurrence==occurrence,"BNW major strength actual formula/target changed during preparation");
  p->check(&supplied);
  // All allocations/SQL/formula checks precede these callback-free stores.
  // Native base and hill writes have no intervening callback; final value is
  // paired with this owned cache before the following actual CityInfo event.
  target->values[0]=q.after;
  auto& extensions=owned.extensions.values;const auto existing=extensions.find(node.key());
  if(existing==extensions.end())extensions.insert(std::move(node));else existing->second.swap(node.mapped());
  state->city_dirty_events.swap(events);state->city_dirty_next_occurrence=occurrence+1;
  ++state->ui_source_revision;++p->revision;p->strengths.swap(updates);
  // DLLUI::setDirty only queues invalidation; no synchronous Lua/draw.
  // The real retained CityInfo event is published above before advancing.
  ++p->next_city;p->stage=BnwMajorTeamHasTechPhase::city_strength_pending;
  return result;
 }catch(...){p->retired.store(true);throw;}
}
}

namespace civ5::runtime {
namespace {
struct MajorResourceRule {std::int64_t reveal=-1,trade=-1;};
std::int64_t major_rule_ref(const content::RulesRowView& row,const char* column,
 const content::InfoTypeCatalog& types,const char* table) {
 major_need(row.has_column(column),"BNW process resource/rule column unavailable");
 const auto text=row.text(column);if(!text)return -1;
 const auto* mapped=types.find(*text);
 major_need(mapped&&mapped->table==table&&mapped->id>=0&&mapped->id<=INT32_MAX&&
  mapped->hash==content::saved_info_type_hash(*text),"BNW process rule reference unresolved");
 return mapped->id;
}
std::uint64_t major_complete_infos(const content::RulesDatabase& db,const content::InfoTypeCatalog& types,
 const char* table,const std::function<void(std::uint64_t,const content::RulesRowView&)>& visit={}) {
 const auto report=db.prefetch_info(table,[&](auto slot,auto,const auto& row){
  const auto type=row.text("Type");const auto* mapped=type?types.find(*type):nullptr;
  major_need(mapped&&mapped->table==table&&mapped->id>=0&&static_cast<std::uint64_t>(mapped->id)==slot&&
   mapped->hash==content::saved_info_type_hash(*type),"BNW process complete Info catalogue/type mismatch");
  if(visit){visit(slot,row);}
  return true;
 });
 major_need(report.all_succeeded()&&!report.overlapping_ids&&report.slots<=INT32_MAX,
  "BNW process complete Info catalogue unavailable");return report.rows;
}
void major_zero_improvement_relation(const content::RulesDatabase& db,
 const char* table,const std::string& improvement,std::int64_t selected_technology) {
 std::map<std::int64_t,std::int64_t> values;
 // Exact CvImprovementEntry cache statement, per improvement and all techs.
 // Replacing its WHERE with TechType=? can change duplicate overwrite order.
 const auto sql=std::string("select Yields.ID as YieldID, Technologies.ID as TechID, Yield from ")+table+
  " inner join Yields on YieldType = Yields.Type inner join Technologies on TechType = Technologies.Type where ImprovementType = ?";
 const auto report=db.query_rows({sql,{improvement}},[&](const auto& row){
  const auto yield=row.integer_at(0),technology=row.integer_at(1),value=row.integer_at(2);
  major_need(technology>=0&&technology<=INT32_MAX&&yield>=0&&yield<6&&value>=INT32_MIN&&value<=INT32_MAX,
   "BNW process improvement native relation domain unavailable");
  if(technology==selected_technology){values[yield]=value;}
 });
 major_need(report.all_succeeded(),"BNW process complete improvement relation query failed");
 for(const auto& [key,value]:values){(void)key;major_need(value==0,"BNW nonzero improvement yield requires actual map/city updates before continuation");}
}
}
std::optional<BnwMajorProcessTechPrefixObservation> BnwMajorTeamHasTechPending::process_prefix_observation()const {
 Impl::Guard guard(impl_);return impl_->process_prefix;
}
void BnwMajorTeamHasTechPending::advance_resource_and_process_prefix(const ui::SelectedBnwPreviewHost& supplied)const {
 const auto p=impl_;Impl::Guard guard(p);
 try {
  major_need(p->stage==BnwMajorTeamHasTechPhase::resource_reveal_pending&&!p->process_prefix,
   "BNW major resource/process prefix missing genuine hook/cities or already consumed");
  const auto host=p->check(&supplied);const auto state=p->state.lock();major_need(static_cast<bool>(state),"BNW actual process world retired");
  const auto& db=host->database();const auto& types=host->types();
  const auto* tech=host->technologies().find(static_cast<std::uint64_t>(p->request.technology));
  major_need(tech&&!tech->base.type.empty(),"BNW actual process technology absent");
  BnwMajorProcessTechPrefixObservation observed;
  std::map<std::int64_t,MajorResourceRule> resources;
  observed.resource_rules=major_complete_infos(db,types,"Resources",[&](auto slot,const auto& row){
   const auto reveal=major_rule_ref(row,"TechReveal",types,"Technologies");
   const auto trade=major_rule_ref(row,"TechCityTrade",types,"Technologies");
   major_need(resources.emplace(static_cast<std::int64_t>(slot),MajorResourceRule{reveal,trade}).second,
    "BNW process duplicate Resource Info");
   if(reveal==p->request.technology)++observed.reveal_rules;
   if(trade==p->request.technology)++observed.trade_rules;
  });
  major_need(state->world.map_plots&&state->world.map_projection&&state->world.map_plots->layout==major_bnw,
   "BNW process full actual raw/projected map unavailable");
  const auto& map=*state->world.map_projection;const auto& raw=*state->world.map_plots;
  const auto count=state->world.map_header.grid.size();
  major_need(count>0&&count<=INT32_MAX&&map.grid.width==state->world.map_header.grid.width&&
   map.grid.height==state->world.map_header.grid.height&&map.cells.size()==count&&raw.plots.size()==count,
   "BNW process source numPlots/full object counts differ");
  for(std::size_t i=0;i<raw.plots.size();++i) {
   const auto& plot=raw.plots[i];const auto& cell=map.cells[i];const auto coordinates=map.grid.coordinates(i);
   major_need(plot.index==i&&cell.index==i&&plot.x==coordinates[0]&&plot.y==coordinates[1]&&
    cell.x==plot.x&&cell.y==plot.y,"BNW resource native plotByIndex/map binding differs");
   const auto hash=major_named(plot.fields,"resource_type_hash").value;
   const auto owner=major_named(plot.fields,"owner").value;
   major_need(hash>=0&&hash<=UINT32_MAX&&cell.resource_type_hash&&*cell.resource_type_hash==static_cast<std::uint64_t>(hash)&&
    cell.owner&&*cell.owner==owner&&owner>=-1&&owner<64,"BNW resource paired raw/projection identity unresolved");
   ++observed.map_plots;
   if(hash==0){major_need(!cell.resource_type||*cell.resource_type==-1,"BNW NO_RESOURCE projection substituted");continue;}
   const auto* mapped=types.find_hash(static_cast<std::uint32_t>(hash));
   major_need(mapped&&mapped->table=="Resources"&&resources.contains(mapped->id)&&
    (!cell.resource_type||*cell.resource_type==mapped->id),"BNW actual plot resource not in complete selected catalogue");
   const auto rule=resources.at(mapped->id);
   if(rule.reveal==p->request.technology) {
    const auto art=saves::observe_saved_plot_team_art(plot,major_bnw,static_cast<std::uint64_t>(p->request.team));
    major_need(art.revealed&&cell.revealed_teams&&cell.revealed_teams->contains(p->request.team)==*art.revealed,
     "BNW resource actual team reveal operand unresolved");
    major_need(!*art.revealed,"BNW revealed matching resource requires actual link/layout/notification effects");
   }
   if(rule.trade==p->request.technology&&owner!=-1&&p->pregame_teams.at(static_cast<std::size_t>(owner))==p->request.team) {
    const auto city_owner=major_named(plot.fields,"plot_city_owner").value;
    const auto city_id=major_named(plot.fields,"plot_city_id").value;
    const bool city=city_owner!=-1&&city_id!=-1;
    major_need(cell.plot_city&&cell.plot_city->owner==city_owner&&cell.plot_city->id==city_id,
     "BNW resource actual city handle/projection differs");
    major_need(cell.improvement_type_hash||cell.improvement_type,"BNW resource actual improvement unknown");
    const bool improvement=cell.improvement_type_hash?*cell.improvement_type_hash!=0:*cell.improvement_type!=-1;
    major_need(!city&&!improvement,"BNW matching connected resource requires actual player total and resource city links");
   }
  }
  // SDK processTech starts here only after the genuine complete map loop.
  const auto flag=[&](const char* name){const auto i=tech->booleans.find(name);
   major_need(i!=tech->booleans.end(),"BNW process actual Technology boolean missing");return i->second;};
  const auto integer=[&](const char* name){const auto i=tech->integers.find(name);
   major_need(i!=tech->integers.end(),"BNW process actual Technology integer missing");return i->second;};
  major_need(!flag("AllowsWorldCongress"),"BNW technology requires actual World Congress setter");
  major_need(!flag("TriggersArchaeologicalSites"),"BNW technology requires actual archaeology site creation");
  constexpr const char* scalar_flags[]={"ExtraWaterSeeFrom","MapCentering","MapTrading","TechTrading","GoldTrading",
   "AllowEmbassyTradingAllowed","OpenBordersTradingAllowed","DefensivePactTradingAllowed","ResearchAgreementTradingAllowed",
   "TradeAgreementTradingAllowed","PermanentAllianceTradingAllowed","BridgeBuilding","WaterWork","AllowsEmbarking",
   "AllowsDefensiveEmbarking","EmbarkedAllWaterPassage"};
  for(const auto name:scalar_flags)major_need(!flag(name),"BNW nonzero technology team scalar requires actual setter/effects");
  major_need(integer("EmbarkedMoveChange")==0,"BNW embarked movement requires actual unit/pathing effects");
  const auto calls=prepare_bnw_route_build_calls(Impl::raw(*state,p->request.team),db,types,p->request.technology,tech->base.type);
  for(const auto& call:calls) {
   major_need(call.delta==0&&call.before==call.after,"BNW nonzero route/build change needs paired live route/build owner");
   if(call.kind==BnwTechRouteBuildKind::route_movement)++observed.routes;else ++observed.builds;
  }
  const auto& moves=major_named(Impl::raw(*state,p->request.team).arrays,"extra_moves");
  major_need(moves.columns==1&&moves.values.size()==5&&state->world.teams->dimensions.domains==5&&
   tech->domain_extra_moves.extent>=5&&tech->domain_extra_moves.default_value==0,"BNW actual domain array unavailable");
  for(std::uint64_t domain=0;domain<5;++domain)major_need(moves.values[domain]>=0&&moves.values[domain]<=INT32_MAX&&
   tech->domain_extra_moves.at(domain)==0,"BNW nonzero domain movement requires actual unit/pathing effects");
  observed.buildings=major_complete_infos(db,types,"Buildings",[&](auto,const auto& row){
   major_need(major_rule_ref(row,"ObsoleteTech",types,"Technologies")!=p->request.technology,
    "BNW technology obsoletes a building; real team/city effects required");
   major_need(major_rule_ref(row,"EnhancedYieldTech",types,"Technologies")!=p->request.technology,
    "BNW technology enhances building yield; actual city yield effects required");
  });
  major_need(major_complete_infos(db,types,"Yields")==6,"BNW actual native six-yield Info roster differs");
  observed.improvements=major_complete_infos(db,types,"Improvements",[&](auto,const auto& row){
   const auto improvement=row.text("Type");major_need(improvement.has_value(),"BNW process Improvement type missing");
   major_zero_improvement_relation(db,"Improvement_TechYieldChanges",*improvement,p->request.technology);
   major_zero_improvement_relation(db,"Improvement_TechNoFreshWaterYieldChanges",*improvement,p->request.technology);
   major_zero_improvement_relation(db,"Improvement_TechFreshWaterYieldChanges",*improvement,p->request.technology);
  });
  observed.promotions=major_complete_infos(db,types,"UnitPromotions",[&](auto id,const auto&){
   major_need(tech->free_promotions.default_value==0&&id<tech->free_promotions.extent&&tech->free_promotions.at(id)==0,
    "BNW tech free promotion requires actual free-promotion counters/units");
  });
  constexpr const char* player_modifiers[]={"FeatureProductionModifier","UnitFortificationModifier","UnitBaseHealModifier",
   "WorkerSpeedModifier","InfluenceSpreadModifier","ExtraVotesPerDiplomat"};
  constexpr const char* native_fields[]={"m_iFeatureProductionModifier","m_iUnitFortificationModifier","m_iUnitBaseHealModifier",
   "m_iWorkerSpeedModifier","m_iInfluenceSpreadModifier","m_iExtraVotesPerDiplomat"};
  for(std::int32_t id=0;id<64;++id) {
   const auto& player=state->world.all_players->players.at(static_cast<std::size_t>(id));
   const auto& ever=major_named(player.core.fields,"m_bEverAlive").values;
   major_need(ever.size()==1&&(ever[0]==0||ever[0]==1)&&state->setup.players.at(id).ever_alive==(ever[0]!=0)&&
    state->setup.players.at(id).team==p->pregame_teams.at(static_cast<std::size_t>(id)),"BNW actual ever-alive member/raw ownership differs");
   if(!ever[0]||p->pregame_teams.at(static_cast<std::size_t>(id))!=p->request.team)continue;
   for(std::size_t i=0;i<6;++i) {
    const auto& value=major_named(player.core.fields,native_fields[i]).values;
    major_need(value.size()==1&&value[0]>=INT32_MIN&&value[0]<=INT32_MAX&&integer(player_modifiers[i])==0,
     "BNW nonzero player scalar requires actual paired player effect");
   }
   observed.first_traits_player=id;break;
  }
  major_need(observed.first_traits_player>=0,"BNW no-ever-alive team requires separate post-player map/Lua suffix");
  p->check(&supplied);
  // All genuine objects/rules were inspected. These zero-delta source calls
  // change no game value. Only this opaque actual SDK cursor advances; no UI
  // revision/visibility/flags/RNG stores or invented Traits Reset completion.
  p->process_prefix=observed;p->stage=BnwMajorTeamHasTechPhase::player_traits_pending;
 }catch(...){p->retired.store(true);throw;}
}
}

namespace civ5::runtime {
std::optional<BnwMajorPlayerTraitsResetEffect> BnwMajorTeamHasTechPending::traits_reset_effect()const{
 Impl::Guard guard(impl_);return impl_->traits_effect;
}
void BnwMajorTeamHasTechPending::advance_first_player_traits(const ui::SelectedBnwPreviewHost& supplied)const{
 const auto p=impl_;Impl::Guard guard(p);
 try{
  major_need(p->stage==BnwMajorTeamHasTechPhase::player_traits_pending&&p->process_prefix&&!p->traits_effect,
   "BNW Traits requires the genuine resource/process predecessor, once");
  const auto host=p->check(&supplied);const auto state=p->state.lock();
  const auto traits=p->traits_owner.lock();
  major_need(state&&traits&&p->traits_actor==p->process_prefix->first_traits_player,
   "BNW actual pre-flag Traits owner retired or native member differs");
  const auto before=p->revision;
  major_need(before<UINT64_MAX&&p->request.expected_ui_source_revision<before&&
   before-p->request.expected_ui_source_revision==1+p->strengths.size(),
   "BNW Traits predecessor revision not exact paired HasTech/city stores");
  // Reserve uncertainty before the first real Traits world write. Failures
  // after a committed Reset may never replay or claim policy-cost completion.
  p->stage=BnwMajorTeamHasTechPhase::uncertain;
  const auto effect=traits->reset_init_for_pending(host,p->generation,
      p->request.expected_ui_source_revision,before,p->request.technology);
  p->traits_effect=effect;
  major_need(effect.player==p->traits_actor&&effect.technology==p->request.technology&&effect.change==1&&
   state->ui_source_revision==before+1,"BNW native Traits effect/revision differs");
  traits->require_current();
  p->revision=before+1;p->check(&supplied);
  p->stage=BnwMajorTeamHasTechPhase::policy_cost_pending;
 }catch(...){p->retired.store(true);throw;}
}
}

namespace civ5::runtime {
std::optional<BnwMajorPolicyFreeUnitEffect> BnwMajorTeamHasTechPending::policy_free_unit_effect()const{
 Impl::Guard guard(impl_);return impl_->policy_free_unit;
}
void BnwMajorTeamHasTechPending::advance_policy_and_post_player_prefix(const ui::SelectedBnwPreviewHost& supplied)const{
 const auto p=impl_;Impl::Guard guard(p);
 try{
  major_need(p->stage==BnwMajorTeamHasTechPhase::policy_cost_pending&&p->traits_effect&&!p->policy_free_unit,
   "BNW numeric policy recompute requires the actual first Traits Reset/Init, once");
  const auto host=p->check(&supplied);const auto state=p->state.lock();const auto traits=p->traits_owner.lock();
  major_need(state&&traits,"BNW actual native Traits owner retired before policy recompute");
  const auto before=p->revision;major_need(before<UINT64_MAX,"BNW policy prefix revision overflow");
  p->stage=BnwMajorTeamHasTechPhase::uncertain;
  const auto effect=traits->recompute_policy_and_first_free_unit(host,p->request.technology);p->policy_free_unit=effect;
  major_need(effect.player==p->traits_actor&&effect.technology==p->request.technology&&state->ui_source_revision==before+1,
   "BNW actual policy/query effect source/revision differs");
  traits->require_current();p->revision=before+1;p->check(&supplied);
  if(effect.first_free_unit_class!=-1){p->stage=BnwMajorTeamHasTechPhase::first_trait_unit_pending;return;}
  p->stage=BnwMajorTeamHasTechPhase::remaining_process_players_pending;
  // Source MAX_PLAYERS order: no later member may be silently skipped.
  for(std::int32_t actor=p->traits_actor+1;actor<64;++actor){
   const auto& raw=state->world.all_players->players.at(static_cast<std::size_t>(actor));
   const auto& ever=major_named(raw.core.fields,"m_bEverAlive").values;
   major_need(raw.index==static_cast<std::uint64_t>(actor)&&raw.saved_player_id==actor&&
    major_named(raw.core.fields,"m_eID").values==std::vector<std::int64_t>{actor}&&ever.size()==1&&
    (ever[0]==0||ever[0]==1)&&state->setup.players.at(actor).ever_alive==(ever[0]!=0)&&
    state->setup.players.at(actor).team==p->pregame_teams.at(static_cast<std::size_t>(actor)),"BNW actual remaining member roster differs");
   if(ever[0]&&p->pregame_teams.at(static_cast<std::size_t>(actor))==p->request.team){p->check(&supplied);return;}
  }
  p->stage=BnwMajorTeamHasTechPhase::resource_plot_yields_pending;
  major_need(state->world.map_plots&&state->world.map_projection,"BNW post-player actual full map absent");
  std::map<std::int64_t,std::int64_t> reveals;
  (void)major_complete_infos(host->database(),host->types(),"Resources",[&](auto id,const auto& row){
   reveals.emplace(static_cast<std::int64_t>(id),major_rule_ref(row,"TechReveal",host->types(),"Technologies"));
  });
  const auto& plots=state->world.map_plots->plots;const auto& map=*state->world.map_projection;
  major_need(plots.size()==state->world.map_header.grid.size()&&map.cells.size()==plots.size()&&
   map.grid.width==state->world.map_header.grid.width&&map.grid.height==state->world.map_header.grid.height,
   "BNW second native map loop object counts differ");
  for(std::size_t i=0;i<plots.size();++i){const auto& raw=plots[i];const auto& cell=map.cells[i];const auto xy=map.grid.coordinates(i);
   const auto hash=major_named(raw.fields,"resource_type_hash").value;
   major_need(raw.index==i&&cell.index==i&&raw.x==xy[0]&&raw.y==xy[1]&&cell.x==raw.x&&cell.y==raw.y&&
    hash>=0&&hash<=UINT32_MAX&&cell.resource_type_hash&&*cell.resource_type_hash==static_cast<std::uint64_t>(hash),
    "BNW second map resource raw/projection identity differs");
   if(!hash)continue;
   const auto* type=host->types().find_hash(static_cast<std::uint32_t>(hash));
   major_need(type&&type->table=="Resources"&&reveals.contains(type->id),"BNW second map Resource Info unresolved");
   // Unlike the initial reveal loop, updateYield is unconditional on visibility.
   if(reveals.at(type->id)==p->request.technology){p->check(&supplied);return;}
  }
  p->stage=BnwMajorTeamHasTechPhase::city_routes_pending;
  bool reached_city=false;
  (void)major_complete_infos(host->database(),host->types(),"Builds",[&](auto,const auto& row){
   if(reached_city)return;
   if(major_rule_ref(row,"PrereqTech",host->types(),"Technologies")!=p->request.technology)return;
   if(major_rule_ref(row,"RouteType",host->types(),"Routes")==-1)return;
   for(const auto& raw:plots){
    const auto owner=major_named(raw.fields,"plot_city_owner").value,id=major_named(raw.fields,"plot_city_id").value;
    if(owner==-1||id==-1)continue;
    major_need(owner>=0&&owner<64&&id>=0&&id<=INT32_MAX,"BNW native plot city reference malformed");
    const auto* city=resolve_bnw_city_registry(state->world,state->cities,static_cast<std::int32_t>(owner),static_cast<std::int32_t>(id));
    if(!city)continue; // Genuine stale handle lookup returns NULL, as source.
    major_need(state->setup.players.at(owner).team==p->pregame_teams.at(static_cast<std::size_t>(owner)),"BNW route city GetTeam raw/owned differs");
    if(p->pregame_teams.at(static_cast<std::size_t>(owner))==p->request.team){reached_city=true;break;}
   }
  });
  p->check(&supplied);if(reached_city)return;
  p->stage=BnwMajorTeamHasTechPhase::team_tech_researched_pending;
 }catch(...){p->retired.store(true);throw;}
}
}

namespace civ5::runtime {
bool BnwMajorTeamHasTechPending::team_tech_researched_completed()const{
 Impl::Guard guard(impl_);return impl_->researched_completed;
}
std::optional<BnwMajorAfterResearchEffect> BnwMajorTeamHasTechPending::after_research_effect()const{
 Impl::Guard guard(impl_);return impl_->after_research;
}
void BnwMajorTeamHasTechPending::begin_team_tech_researched_dispatch(const ui::SelectedBnwPreviewHost& h)const{
 Impl::Guard guard(impl_);
 try{
  major_need(impl_->stage==BnwMajorTeamHasTechPhase::team_tech_researched_pending&&
   impl_->attempt==Impl::Attempt::none&&!impl_->researched_completed&&impl_->policy_free_unit,
   "BNW TeamTechResearched requires genuine completed map loops, once");
  impl_->check(&h);impl_->attempt=Impl::Attempt::team_tech_researched;
  impl_->stage=BnwMajorTeamHasTechPhase::uncertain;
 }catch(...){impl_->retired.store(true);throw;}
}
void BnwMajorTeamHasTechPending::finish_team_tech_researched_dispatch(const ui::SelectedBnwPreviewHost& h)const{
 Impl::Guard guard(impl_);
 try{
  major_need(impl_->stage==BnwMajorTeamHasTechPhase::uncertain&&
   impl_->attempt==Impl::Attempt::team_tech_researched&&!impl_->researched_completed,
   "BNW actual TeamTechResearched attempt absent or repeated");
  impl_->check(&h);impl_->attempt=Impl::Attempt::none;impl_->researched_completed=true;
  impl_->stage=BnwMajorTeamHasTechPhase::ends_game_pending;
 }catch(...){impl_->retired.store(true);throw;}
}
void BnwMajorTeamHasTechPending::advance_after_team_tech_researched(const ui::SelectedBnwPreviewHost& supplied)const{
 const auto p=impl_;Impl::Guard guard(p);
 try{
  major_need(p->stage==BnwMajorTeamHasTechPhase::ends_game_pending&&p->researched_completed&&!p->after_research,
   "BNW after-researched suffix requires the real SAME-VM dispatch, once");
  const auto host=p->check(&supplied);const auto state=p->state.lock();
  major_need(static_cast<bool>(state),"BNW after-researched actual world retired");
  const auto* tech=host->technologies().find(static_cast<std::uint64_t>(p->request.technology));
  major_need(tech&&!tech->base.type.empty(),"BNW after-researched actual Tech Info absent");
  const auto ends=tech->booleans.find("EndsGame");
  major_need(ends!=tech->booleans.end(),"BNW actual EndsGame rule absent");
  auto& end_flag=major_named(state->world.game_state.flags,"end_game_tech_researched");
  BnwMajorAfterResearchEffect effect;
  effect.ends_game_before=end_flag.value;effect.ends_game_after=ends->second?true:end_flag.value;
  major_need(p->revision<UINT64_MAX,"BNW EndsGame revision overflow");
  // CvGame::SetEndGameTechResearched is a scalar assignment, not a victory
  // resolver. Reserve uncertainty before it; its result never grants victory.
  p->stage=BnwMajorTeamHasTechPhase::uncertain;
  end_flag.value=effect.ends_game_after;
  if(effect.ends_game_before!=effect.ends_game_after){++state->ui_source_revision;++p->revision;}
  p->after_research=effect;p->check(&supplied);
  p->stage=BnwMajorTeamHasTechPhase::artifact_notifications_pending;
  bool artifact_reveal=false;
  (void)major_complete_infos(host->database(),host->types(),"Resources",[&](auto,const auto& row){
   const auto type=row.text("Type");major_need(type.has_value(),"BNW actual artifact type absent");
   if(*type=="RESOURCE_ARTIFACTS"||*type=="RESOURCE_HIDDEN_ARTIFACTS")
    if(major_rule_ref(row,"TechReveal",host->types(),"Technologies")==p->request.technology)artifact_reveal=true;
  });
  // Independent of WBScenario/turn: neither resource's TechReveal matches,
  // hence the original inner map loop is unreachable. Never fake Add().
  if(artifact_reveal){p->check(&supplied);return;}
  p->stage=BnwMajorTeamHasTechPhase::era_change_pending;
  auto& stored=*p->after_research;
  const auto& raw=Impl::raw(*state,p->request.team);
  const auto current=major_named(raw.fields,"era").value;
  std::uint64_t era_slots=0;
  const auto eras=major_complete_infos(host->database(),host->types(),"Eras",[&](auto id,const auto&){
   major_need(id==era_slots,"BNW actual Era catalogue has unresolved source slots");++era_slots;
  });
  major_need(current>=0&&static_cast<std::uint64_t>(current)<eras&&tech->era>=0&&
   static_cast<std::uint64_t>(tech->era)<eras,"BNW actual current/technology era outside complete Era Infos");
  stored.current_era=current;stored.requested_era=current;
  bool all_earlier=true;
  const auto tech_count=major_complete_infos(host->database(),host->types(),"Technologies",[&](auto id,const auto& row){
   const auto* other=host->technologies().find(id);
   const auto other_type=row.text("Type");
   major_need(other&&other_type&&other->base.type==*other_type&&other->era>=0&&
    static_cast<std::uint64_t>(other->era)<eras&&
    other->era==major_rule_ref(row,"Era",host->types(),"Eras"),"BNW complete Technology/Era Info remap differs");
   if(other->era<=current&&!Impl::owned(*state,p->request.team).has_tech(static_cast<std::int64_t>(id))){
    all_earlier=false;if(stored.missing_earlier_technology==-1)stored.missing_earlier_technology=static_cast<std::int64_t>(id);
   }
  });
  major_need(tech_count==host->technologies().collection.slots&&host->technologies().all_succeeded(),
   "BNW complete technology era scan differs from genuine loaded Infos");
  if(current<tech->era)stored.requested_era=tech->era;
  else if(all_earlier&&static_cast<std::uint64_t>(current)+1<eras)stored.requested_era=current+1;
  // SetCurrentEra has notifications, minor bonuses, map/unit/art and Lua
  // effects. Stop BEFORE it. Cached current era is never its substitute.
  if(stored.requested_era!=current){p->check(&supplied);return;}
  p->stage=BnwMajorTeamHasTechPhase::map_reveal_pending;
  const auto visible=tech->booleans.find("MapVisible");
  major_need(visible!=tech->booleans.end(),"BNW actual MapVisible rule absent");
  if(visible->second){p->check(&supplied);return;}
  p->stage=BnwMajorTeamHasTechPhase::best_route_pending;
  std::vector<BnwBuildRouteRule> builds;
  (void)major_complete_infos(host->database(),host->types(),"Routes");
  (void)major_complete_infos(host->database(),host->types(),"Builds",[&](auto id,const auto& row){
   major_need(id==builds.size(),"BNW actual native Build source order differs");
   builds.push_back({static_cast<std::int64_t>(id),major_rule_ref(row,"RouteType",host->types(),"Routes"),
     major_rule_ref(row,"PrereqTech",host->types(),"Technologies")});
  });
  // Reuse the genuine pure DoUpdateBestRoute evaluator (>=, last tie wins).
  const auto selected=select_bnw_best_route(builds,Impl::owned(*state,p->request.team),host->technologies());
  auto& best=major_named(Impl::raw(*state,p->request.team).fields,"best_route");
  major_need(best.value>=-1&&best.value<=INT32_MAX&&selected.route>=-1&&selected.route<=INT32_MAX,
   "BNW best route actual scalar out of native range");
  if(best.value!=-1){bool found=false;
   for(const auto& entry:host->types().entries())if(entry.second.table=="Routes"&&entry.second.id==best.value)found=true;
   major_need(found,"BNW previous best route not in complete selected catalogue");
  }
  major_need(p->revision<UINT64_MAX,"BNW best route revision overflow");
  stored.best_route_before=best.value;stored.best_route_after=selected.route;
  stored.winning_build=selected.winning_build;stored.route_value=selected.value;stored.builds=builds.size();
  p->check(&supplied);p->stage=BnwMajorTeamHasTechPhase::uncertain;
  best.value=selected.route;
  if(stored.best_route_before!=stored.best_route_after){++state->ui_source_revision;++p->revision;}
  stored.best_route_stored=true;p->check(&supplied);
  // Next actual source call is per-living-member popResearch followed by
  // unconditional invalidateYieldRankCache. Neither is granted here.
  p->stage=BnwMajorTeamHasTechPhase::player_research_pending;
 }catch(...){p->retired.store(true);throw;}
}
}

namespace civ5::runtime {
std::vector<BnwTeamTechPlayerResearchEffect> BnwMajorTeamHasTechPending::player_research_effects()const{
 Impl::Guard guard(impl_);return impl_->player_research?impl_->player_research->effects():std::vector<BnwTeamTechPlayerResearchEffect>{};
}
void BnwMajorTeamHasTechPending::advance_player_research_and_first_reward(const ui::SelectedBnwPreviewHost& supplied)const{
 const auto p=impl_;Impl::Guard guard(p);
 try{
  major_need(p->stage==BnwMajorTeamHasTechPhase::player_research_pending&&p->researched_completed&&
   p->after_research&&p->after_research->best_route_stored&&!p->player_research,
   "BNW popResearch requires real hook/best route prefix, once");
  const auto host=p->check(&supplied);const auto state=p->state.lock();
  major_need(static_cast<bool>(state),"BNW actual research world retired");
  auto prepared=BnwTeamTechPlayerResearchPrepared::prepare(*state,host->types(),host->technologies(),
   p->request.team,p->request.technology,p->generation);
  p->check(&supplied);p->stage=BnwMajorTeamHasTechPhase::uncertain;
  prepared->apply(*state);p->revision=state->ui_source_revision;p->player_research=std::move(prepared);
  p->check(&supplied);p->stage=BnwMajorTeamHasTechPhase::first_research_reward_pending;
  // CvGame::countKnownTechNumTeams uses EVER-alive teams, not only alive ones.
  std::int64_t known=0;
  if(p->request.first){
   major_need(state->world.teams&&state->world.teams->teams.size()==64&&state->technologies.size()==64,
    "BNW first-research real 64-team roster unavailable");
   for(std::int64_t team=0;team<64;++team){
    const auto& raw=Impl::raw(*state,team);const auto& owned=Impl::owned(*state,team);
    const auto ever=major_named(raw.fields,"ever_alive_count").value;
    major_need(ever>=0&&ever<=64,"BNW actual team ever-alive count invalid");
    const auto decoded=restore_team_technologies(raw,host->technologies(),host->types());
    major_need(decoded.remap().all_mapped()&&!decoded.remap().duplicates&&major_same_owned(decoded,owned),
     "BNW first-research full raw/owned team technology remap differs");
    if(ever&&owned.has_tech(p->request.technology))++known;
   }
   if(known==1){
    const auto* tech=host->technologies().find(static_cast<std::uint64_t>(p->request.technology));
    major_need(tech,"BNW actual first-research technology absent");
    const auto free=tech->integers.find("FirstFreeTechs");
    major_need(free!=tech->integers.end()&&free->second>=INT32_MIN&&free->second<=INT32_MAX&&tech->first_free_unit_class>=-1,
     "BNW actual first-research rewards unresolved");
    if(tech->first_free_unit_class!=-1||free->second>0){p->check(&supplied);return;}
   }
  }
  // Native bFirstResource=false when no actual first unit/free-tech branch.
  // No clearResearchQueue is reached in this branch.
  p->stage=BnwMajorTeamHasTechPhase::technology_announcement_pending;
  const auto final=major_named(state->world.game_state.flags,"final_initialized").value;
  if(p->request.announce&&final){p->check(&supplied);return;}
  // Next live-team updateTechShare is mandatory even without announcement.
  p->check(&supplied);p->stage=BnwMajorTeamHasTechPhase::technology_share_pending;
 }catch(...){p->retired.store(true);throw;}
}
}

namespace civ5::runtime {
std::optional<BnwMajorAnnouncementShareEffect> BnwMajorTeamHasTechPending::announcement_share_effect()const{
 Impl::Guard guard(impl_);return impl_->announcement_share;
}
void BnwMajorTeamHasTechPending::advance_announcements_and_sharing(const ui::SelectedBnwPreviewHost& supplied)const{
 const auto p=impl_;Impl::Guard guard(p);
 try{
  major_need(p->player_research&&p->researched_completed&&!p->announcement_share&&
   (p->stage==BnwMajorTeamHasTechPhase::technology_announcement_pending||
    p->stage==BnwMajorTeamHasTechPhase::technology_share_pending),
   "BNW announcements require genuine player-research prefix exactly once");
  const auto host=p->check(&supplied);const auto s=p->state.lock();
  major_need(static_cast<bool>(s),"BNW announcement real world retired");
  BnwMajorAnnouncementShareEffect result;result.before_revision=s->ui_source_revision;
  const auto* tech=host->technologies().find(static_cast<std::uint64_t>(p->request.technology));
  major_need(tech&&!tech->base.text.key.empty(),"BNW announcement actual tech TextKey unavailable");
  const bool announce=p->request.announce&&major_named(s->world.game_state.flags,"final_initialized").value;
  const auto active=detail::bnw_game_active_player_read(*s);
  if(announce){
   // CvPreGame::isNetworkMultiplayerGame reads the actual imported GameTypes.
   // This owner is explicitly the single-player BNW port, not an MP default.
   major_need(s->world.pregame&&s->world.pregame->archive.integers.at("game_type")==s->setup.game_type&&
    s->setup.game_type==0,"BNW AddMessage single-player source game type unavailable");
   for(std::int32_t actor=0;actor<64;++actor){
    const auto& raw=s->world.all_players->players.at(static_cast<std::size_t>(actor));
    const auto& alive=major_named(raw.core.fields,"m_bAlive").values;
    major_need(raw.saved_player_id==actor&&alive.size()==1&&(alive[0]==0||alive[0]==1)&&
     s->setup.players.at(actor).alive==(alive[0]!=0)&&
     s->setup.players.at(actor).team==p->pregame_teams.at(static_cast<std::size_t>(actor)),
     "BNW AddMessage living-player raw/owned identity differs");
    if(alive[0]&&p->pregame_teams.at(static_cast<std::size_t>(actor))==p->request.team&&actor==active)
     result.messages.push_back({0,actor,p->request.team,p->request.technology,-1,false,
      s->setup.game_turn,p->generation,"TXT_KEY_MISC_YOU_DISCOVERED_TECH",{tech->base.text.key}});
   }
  }
  // Prepare both expected messages and the actual queue before publication.
  major_need(!s->setup.extensions.find<BnwMajorTechMessageQueue>(),"BNW announcement queue already issued/foreign");
  major_need(result.messages.empty()||s->ui_source_revision<UINT64_MAX,"BNW announcement revision overflow");
  BnwMajorTechMessageQueue queue;queue.messages=result.messages;
  p->announcement_share.emplace(std::move(result));
  p->check(&supplied);p->stage=BnwMajorTeamHasTechPhase::uncertain;
  p->message_queue=&s->setup.extensions.emplace<BnwMajorTechMessageQueue>(std::move(queue));
  p->message_queue->written=p->message_queue->messages.size();
  if(p->message_queue->written)++s->ui_source_revision;
  p->revision=s->ui_source_revision;p->announcement_share->after_revision=p->revision;p->check(&supplied);
  auto& observed=*p->announcement_share;
  if(announce){
   std::map<std::int64_t,std::int64_t> reveals;
   observed.resources_checked=major_complete_infos(host->database(),host->types(),"Resources",[&](auto id,const auto& row){
    reveals.emplace(static_cast<std::int64_t>(id),major_rule_ref(row,"TechReveal",host->types(),"Technologies"));
   });
   major_need(s->world.map_plots&&s->world.map_projection&&
    s->world.map_plots->plots.size()==s->world.map_header.grid.size()&&
    s->world.map_projection->cells.size()==s->world.map_plots->plots.size(),
    "BNW announcement actual full map unavailable");
   for(std::size_t i=0;i<s->world.map_plots->plots.size();++i){
    const auto& raw=s->world.map_plots->plots[i];const auto& cell=s->world.map_projection->cells[i];
    const auto owner=major_named(raw.fields,"owner").value,hash=major_named(raw.fields,"resource_type_hash").value;
    major_need(raw.index==i&&cell.index==i&&cell.owner&&*cell.owner==owner&&owner>=-1&&owner<64&&
     hash>=0&&hash<=UINT32_MAX&&cell.resource_type_hash&&*cell.resource_type_hash==static_cast<std::uint64_t>(hash),
     "BNW announcement plot owner/resource source binding differs");
    ++observed.plots_checked;
    if(owner!=active||owner<0||p->pregame_teams.at(static_cast<std::size_t>(owner))!=p->request.team||hash==0)continue;
    const auto* mapped=host->types().find_hash(static_cast<std::uint32_t>(hash));
    major_need(mapped&&mapped->table=="Resources"&&reveals.contains(mapped->id),"BNW announcement resource remap unresolved");
    if(reveals.at(mapped->id)==p->request.technology){
     // The source then reads isForceRevealedResource/findCity and AddPlotMessage.
     // Initial AddMessage is already retained; no replay of that prefix.
     p->stage=BnwMajorTeamHasTechPhase::resource_announcement_pending;p->check(&supplied);return;
    }
   }
   // Source does not require alive here. The true major-team active player's
   // complete FLTA city roster is inspected before declaring no demand call.
   for(std::int32_t actor=0;actor<22;++actor){
    if(actor!=active||p->pregame_teams.at(static_cast<std::size_t>(actor))!=p->request.team)continue;
    (void)resolve_bnw_city_registry(s->world,s->cities,actor,0);
    for(const auto& city:s->world.all_players->players.at(static_cast<std::size_t>(actor)).cities.cities){
     const auto& demand=major_named(city.core.fields,"m_iResourceDemanded");
     major_need(demand.encoding==saves::CityCoreEncoding::hash32&&demand.values.size()==1&&
      demand.values[0]>=0&&demand.values[0]<=UINT32_MAX,"BNW actual demanded resource hash unresolved");
     if(demand.values[0]==0)continue;
     const auto* mapped=host->types().find_hash(static_cast<std::uint32_t>(demand.values[0]));
     major_need(mapped&&mapped->table=="Resources"&&reveals.contains(mapped->id),"BNW demanded Resource Info unavailable");
     if(reveals.at(mapped->id)==p->request.technology){p->stage=BnwMajorTeamHasTechPhase::resource_announcement_pending;p->check(&supplied);return;}
    }
   }
  }
  observed.announcement_complete=true;
  // Existing native threshold loop is reused for any actual selected Tech ID.
  // It stops before the first recursive setHasTech, never marks that executed.
  observed.sharing=detail::quote_bnw_team_tech_share_checks(*s,host->technologies(),host->types(),p->request.team,p->request.technology);
  for(const auto& check:observed.sharing)if(check.would_grant){
   observed.recursive_team=check.recipient;p->stage=BnwMajorTeamHasTechPhase::technology_share_pending;p->check(&supplied);return;
  }
  observed.sharing_complete=true;
  // Native firstCity/nextCity can run for a dead major on this team too.
  for(std::int32_t actor=0;actor<22;++actor)if(p->pregame_teams.at(static_cast<std::size_t>(actor))==p->request.team){
   (void)resolve_bnw_city_registry(s->world,s->cities,actor,0);
   observed.culture_cities_checked+=s->world.all_players->players.at(static_cast<std::size_t>(actor)).cities.cities.size();
  }
  if(const auto actor=detail::quote_bnw_team_tech_first_owed_culture_player(*s,p->request.team)){
   observed.owed_culture_player=*actor;p->stage=BnwMajorTeamHasTechPhase::owed_culture_building_pending;p->check(&supplied);return;
  }
  observed.culture_complete=true;
  if(announce&&detail::bnw_player_human_read(*s,static_cast<std::int32_t>(p->request.player),host->database())){
   const auto& raw=s->world.all_players->players.at(static_cast<std::size_t>(p->request.player));
   const auto& founded=major_named(raw.core.fields,"m_bFoundedFirstCity").values;
   major_need(founded.size()==1&&(founded[0]==0||founded[0]==1)&&
    s->research_turn.players.at(p->request.player).founded_first_city==(founded[0]!=0),"BNW native IsResearch paired founded flag differs");
   if(founded[0]&&s->player_research.players.at(p->request.player).current_research()==-1){
    p->stage=BnwMajorTeamHasTechPhase::research_choice_notification_pending;p->check(&supplied);return;
   }
  }
  // Next real BuildStealableTechList/CanResearch effect; no false completion.
  p->stage=BnwMajorTeamHasTechPhase::espionage_research_pending;p->check(&supplied);
 }catch(...){p->retired.store(true);throw;}
}
}

namespace civ5::runtime {
std::optional<BnwMajorEspionagePrefixEffect> BnwMajorTeamHasTechPending::espionage_prefix_effect()const{
 Impl::Guard guard(impl_);return impl_->espionage_prefix;
}
void BnwMajorTeamHasTechPending::advance_espionage_prefix(const ui::SelectedBnwPreviewHost& supplied)const{
 const auto p=impl_;Impl::Guard guard(p);
 try{
  major_need(p->stage==BnwMajorTeamHasTechPhase::espionage_research_pending&&p->announcement_share&&
   p->announcement_share->culture_complete&&!p->espionage_prefix,"BNW espionage requires genuine native share/culture prefix once");
  const auto host=p->check(&supplied);const auto s=p->state.lock();major_need(static_cast<bool>(s),"BNW real espionage world retired");
  const auto actor=p->request.player;
  BnwMajorEspionagePrefixEffect effect;effect.player=actor;effect.before_revision=s->ui_source_revision;
  // Genuine major command admission already rejects minor/barbarian actor;
  // re-read rather than accepting an external 'has espionage' claim.
  major_need(actor>=0&&actor<22&&!p->pregame_minor.at(static_cast<std::size_t>(actor)),"BNW espionage actual acquiring major unavailable");
  auto& player=s->world.all_players->players.at(static_cast<std::size_t>(actor));
  major_need(player.espionage.layout==major_bnw,"BNW live espionage native archive profile differs");
  auto& espionage=major_named(player.espionage.blocks,"player_espionage");
  auto& costs=major_named(espionage.arrays,"m_aiMaxTechCost");auto& number=major_named(espionage.arrays,"m_aiNumTechsToStealList");
  const auto outer=major_named(espionage.fields,"m_aaPlayerStealableTechList_wire_count").value;
  major_need(outer==22&&costs.columns==1&&number.columns==1&&costs.values.size()==22&&number.values.size()==22&&
   major_named(espionage.fields,"m_aiMaxTechCost_wire_count").value==22&&
   major_named(espionage.fields,"m_aiNumTechsToStealList_wire_count").value==22,"BNW actual espionage target spans unavailable");
  BnwMajorTechEspionageCache prepared;prepared.player=actor;prepared.generation=p->generation;
  prepared.lists.resize(22);prepared.maximum_cost=costs.values;prepared.number_to_steal=number.values;
  std::vector<saves::PlayerEspionageRecord*> lists(22,nullptr);
  for(auto& child:espionage.children)if(child.name&&std::string_view(child.name)=="stealable_techs"){
   major_need(child.index<22&&!lists.at(static_cast<std::size_t>(child.index)),"BNW espionage target ID duplicated/outside source domain");
   lists.at(static_cast<std::size_t>(child.index))=&child;
   const auto& values=major_named(child.arrays,"techs");const auto count=major_named(child.fields,"techs_wire_count").value;
   major_need(values.columns==1&&count>=0&&static_cast<std::uint64_t>(count)==values.values.size(),"BNW espionage actual tech list shape differs");
   for(const auto tech:values.values)major_need(tech>=0&&host->technologies().find(static_cast<std::uint64_t>(tech)),"BNW espionage saved tech ID unresolved");
   prepared.lists.at(static_cast<std::size_t>(child.index))=values.values;
  }
  major_need(std::all_of(lists.begin(),lists.end(),[](const auto* q){return q!=nullptr;}),"BNW actual espionage target list incomplete");
  major_need(!s->setup.extensions.find<BnwMajorTechEspionageCache>(),"BNW espionage native mirror already issued/foreign");
  const auto restrictions=restore_research_selection(s->world,s->setup,host->database());
  major_need(restrictions.profile==s->research_selection.profile&&restrictions.disabled_technologies==s->research_selection.disabled_technologies&&
   restrictions.players.size()==s->research_selection.players.size(),"BNW actual civilization tech restriction SQL/owned oracle differs");
  for(const auto& entry:restrictions.players){const auto& other=s->research_selection.players.at(entry.first);
   major_need(entry.second.civilization==other.civilization&&entry.second.advanced_start_points==other.advanced_start_points,
    "BNW actual player civilization/advanced-start research state differs");
  }
  const auto& founded=major_named(player.core.fields,"m_bFoundedFirstCity").values;
  const auto& advanced=major_named(player.core.fields,"m_iAdvancedStartPoints").values;
  major_need(founded.size()==1&&(founded[0]==0||founded[0]==1)&&advanced.size()==1&&
   s->research_turn.players.at(actor).founded_first_city==(founded[0]!=0)&&
   advanced[0]==restrictions.players.at(actor).advanced_start_points,"BNW espionage native CanResearch inputs unpaired");
  const auto& civilization=restrictions.players.at(actor).civilization;
  major_need(civilization.has_value(),"BNW espionage current Civilization Info unavailable");
  const auto limits=research_prerequisite_limits(host->defines());
  major_need(host->technologies().all_succeeded()&&host->technologies().collection.slots<=INT32_MAX&&
   host->technologies().collection.slots>0&&s->ui_source_revision<=UINT64_MAX-66,"BNW espionage technology span/revision overflow");
  const auto& mine=Impl::owned(*s,p->request.team);
  // All native list/mirror storage and expected metadata exist before clear().
  p->check(&supplied);p->stage=BnwMajorTeamHasTechPhase::uncertain;
  p->espionage_cache=&s->setup.extensions.emplace<BnwMajorTechEspionageCache>(std::move(prepared));
  p->espionage_prefix.emplace(effect);p->espionage_lists=std::move(lists);
  for(std::size_t target=0;target<22;++target){
   auto& child=*p->espionage_lists[target];auto& list=major_named(child.arrays,"techs").values;
   const bool changed=!list.empty();list.clear();major_named(child.fields,"techs_wire_count").value=0;
   p->espionage_cache->lists[target].clear();++p->espionage_prefix->targets_cleared;
   if(changed)++s->ui_source_revision;
   p->revision=s->ui_source_revision;p->espionage_prefix->after_revision=p->revision;p->check(&supplied);
   const auto& target_player=s->world.all_players->players.at(target);
   const auto& alive=major_named(target_player.core.fields,"m_bAlive").values;
   major_need(alive.size()==1&&(alive[0]==0||alive[0]==1)&&s->setup.players.at(static_cast<std::int64_t>(target)).alive==(alive[0]!=0)&&
    target_player.saved_player_id==static_cast<std::int64_t>(target),"BNW espionage actual target alive/ID differs");
   if(alive[0]){
    const auto& other=Impl::owned(*s,p->pregame_teams.at(target));
    for(std::uint64_t tech_id=0;tech_id<host->technologies().collection.slots;++tech_id){
     const auto tech=static_cast<std::int64_t>(tech_id);
     if(!other.has_tech(tech))continue;
     ++p->espionage_prefix->known_technologies_checked;
     const auto* info=host->technologies().find(tech_id);
     if(!info||(!founded[0]&&advanced[0]<0)||mine.has_tech(tech)||
        !research_prerequisites_met(host->technologies(),mine,tech,limits,false))continue;
     const auto disabled=info->booleans.find("Disable");
     major_need(disabled!=info->booleans.end(),"BNW espionage actual Disable rule missing");
     if(disabled->second||restrictions.disabled_technologies.at(*civilization).contains(tech))continue;
     // Native next mandatory calls: PlayerCanEverResearch, then (if accepted)
     // PlayerCanResearch, then push_back + GetResearchCost *100. Never infer
     // their result from absence of listeners or a previously stored max cost.
     p->espionage_prefix->first_hook_target=static_cast<std::int64_t>(target);
     p->espionage_prefix->first_hook_technology=tech;p->stage=BnwMajorTeamHasTechPhase::espionage_research_pending;
     p->check(&supplied);return;
    }
    if(costs.values[target]!=-1){costs.values[target]=-1;p->espionage_cache->maximum_cost[target]=-1;++s->ui_source_revision;}
   }
   // Dead target returns before max-cost store; the caller still resets empty
   // number_to_steal. This preserves the native asymmetry precisely.
   if(number.values[target]!=0){number.values[target]=0;p->espionage_cache->number_to_steal[target]=0;++s->ui_source_revision;}
   ++p->espionage_prefix->targets_completed;p->revision=s->ui_source_revision;
   p->espionage_prefix->after_revision=p->revision;p->check(&supplied);
  }
  p->espionage_prefix->complete=true;p->stage=BnwMajorTeamHasTechPhase::major_happiness_pending;p->check(&supplied);
 }catch(...){p->retired.store(true);throw;}
}
}

namespace civ5::runtime {
namespace {
std::int32_t major_happiness_int(std::int64_t n){
 major_need(n>=INT32_MIN&&n<=INT32_MAX,"BNW actual happiness native int overflow");return static_cast<std::int32_t>(n);
}
std::int32_t major_happiness_scalar(const auto& raw,std::string_view key){
 const auto& f=major_named(raw.core.fields,key);
 major_need(f.encoding==saves::PlayerCoreEncoding::i32&&f.values.size()==1,"BNW actual happiness native scalar unavailable");
 return major_happiness_int(f.values[0]);
}
}
std::optional<BnwMajorHappinessPrefixEffect> BnwMajorTeamHasTechPending::happiness_prefix_effect()const{
 Impl::Guard guard(impl_);return impl_->happiness_prefix;
}
void BnwMajorTeamHasTechPending::advance_happiness_prefix(const ui::SelectedBnwPreviewHost& supplied)const{
 const auto p=impl_;Impl::Guard guard(p);
 try{
  major_need(p->stage==BnwMajorTeamHasTechPhase::major_happiness_pending&&p->espionage_prefix&&
   p->espionage_prefix->complete&&!p->happiness_prefix,"BNW happiness requires genuine complete espionage interval once");
  const auto host=p->check(&supplied);const auto s=p->state.lock();major_need(bool(s),"BNW happiness world retired");
  std::int64_t actor=-1;
  // CvTeam::setHasTech scans ALL 22 majors in ID order; only living team members
  // invoke DoUpdateHappiness. This is independent of the acquiring player.
  for(std::size_t i=static_cast<std::size_t>(p->next_happiness_owner);i<22;++i){
   const auto& raw=s->world.all_players->players.at(i);const auto& alive=major_named(raw.core.fields,"m_bAlive").values;
   major_need(raw.saved_player_id==static_cast<std::int64_t>(i)&&alive.size()==1&&(alive[0]==0||alive[0]==1)&&
    s->setup.players.at(static_cast<std::int64_t>(i)).alive==(alive[0]!=0)&&
    s->setup.players.at(static_cast<std::int64_t>(i)).team==p->pregame_teams.at(i),"BNW happiness actual living-major roster differs");
   if(alive[0]&&p->pregame_teams.at(i)==p->request.team){actor=static_cast<std::int64_t>(i);break;}
  }
  major_need(actor>=0,"BNW genuine acquiring team has no living happiness member");
  auto rules=std::make_unique<BnwHappinessPrefixRules>(BnwHappinessPrefixRules::load(host->database()));
  major_need(rules->rules_sha256()==host->identity().rules_sha256,"BNW happiness actual selected rules differ");
  auto& raw=s->world.all_players->players.at(static_cast<std::size_t>(actor));
  BnwMajorHappinessPrefixEffect effect;effect.player=actor;effect.before_revision=s->ui_source_revision;
  effect.initial_happiness=major_happiness_scalar(raw,"m_iHappiness");effect.stores.reserve(7);
  major_need(s->unhappiness.players.at(actor).happiness==effect.initial_happiness&&s->ui_source_revision<=UINT64_MAX-7,
   "BNW happiness paired native cache/revision unavailable");
  p->check(&supplied);p->stage=BnwMajorTeamHasTechPhase::uncertain;
  p->happiness_rules=std::move(rules);p->happiness_prefix.emplace(std::move(effect));
  p->happiness_stored=p->happiness_prefix->initial_happiness;
  for(const auto stage:{BnwHappinessNativeStore::Stage::handicap,BnwHappinessNativeStore::Stage::resources,
   BnwHappinessNativeStore::Stage::cities,BnwHappinessNativeStore::Stage::buildings,
   BnwHappinessNativeStore::Stage::policies,BnwHappinessNativeStore::Stage::per_city}){
   const auto item=detail::bnw_happiness_prefix_reached_stage_math(*s,*p->happiness_rules,host->identity(),actor,stage);
   auto& field=major_named(raw.core.fields,"m_iHappiness");
   const bool changed=field.values[0]!=item.happiness||s->cities.players.at(actor).excess_happiness.has_value();
   // Storage was reserved before the first paired store. No callback or clone.
   p->happiness_prefix->stores.push_back(item);
   field.values[0]=item.happiness;s->unhappiness.players.at(actor).happiness=item.happiness;
   s->cities.players.at(actor).excess_happiness.reset();p->happiness_stored=item.happiness;
   if(changed)++s->ui_source_revision;
   p->revision=s->ui_source_revision;p->happiness_prefix->after_revision=p->revision;p->check(&supplied);
  }
  p->stage=BnwMajorTeamHasTechPhase::happiness_founder_pending;p->check(&supplied);
 }catch(...){p->retired.store(true);throw;}
}
std::int64_t BnwMajorTeamHasTechPending::begin_happiness_founder_dispatch(const ui::SelectedBnwPreviewHost& supplied)const{
 const auto p=impl_;Impl::Guard guard(p);
 try{
  p->check(&supplied);
  major_need(p->happiness_prefix&&p->happiness_prefix->stores.size()==6&&
   ((p->stage==BnwMajorTeamHasTechPhase::happiness_founder_pending&&p->happiness_prefix->founder_callbacks==0)||
    (p->stage==BnwMajorTeamHasTechPhase::happiness_foreign_founder_pending&&p->happiness_prefix->founder_callbacks==1&&p->happiness_prefix->religion)),
   "BNW happiness founder actual native call missing/repeated");
  p->happiness_foreign_attempt=p->stage==BnwMajorTeamHasTechPhase::happiness_foreign_founder_pending;
  p->happiness_founder_attempt=true;p->stage=BnwMajorTeamHasTechPhase::uncertain;
  return p->happiness_prefix->player;
 }catch(...){p->retired.store(true);throw;}
}
void BnwMajorTeamHasTechPending::finish_happiness_founder_dispatch(
 const ui::SelectedBnwPreviewHost& supplied,std::optional<std::int64_t> actual_accumulator)const{
 const auto p=impl_;Impl::Guard guard(p);
 try{
  const auto host=p->check(&supplied);const auto s=p->state.lock();
  major_need(s&&p->stage==BnwMajorTeamHasTechPhase::uncertain&&p->happiness_founder_attempt&&p->happiness_prefix,
   "BNW founder requires actual reserved SAME-VM attempt");
  const auto actor=p->happiness_prefix->player;
  const auto model=BnwFounderReligionReadModel::bind(*s);
  std::int32_t religion=-1;
  if(actual_accumulator)religion=major_happiness_int(*actual_accumulator);
  else{const auto created=model.religion_created_by_player(actor);if(model.eligible_for_founder_benefits(created,actor))religion=created;}
  ++p->happiness_prefix->founder_callbacks;p->happiness_founder_attempt=false;
  std::int32_t amount=0;
  if(p->happiness_foreign_attempt){
   auto& terms=*p->happiness_prefix->religion;terms.foreign_founder_religion=religion;terms.founder_callback_calls=2;
   if(religion>0)terms.peaceful_foreign_followers=BnwReligionFollowersReadModel::bind(*s).peaceful_foreign_followers(actor,religion);
   terms.foreign_happiness=major_happiness_int(static_cast<std::int64_t>(terms.peaceful_foreign_followers)/terms.peaceful_foreign_divisor);
   terms.total=major_happiness_int(static_cast<std::int64_t>(terms.total)+terms.foreign_happiness);amount=terms.total;
  }else{
   p->happiness_prefix->founder_religion=religion;
   const auto* entry=religion==-1?nullptr:model.religion(religion,-1);
   if(entry){
    const auto rules=BnwReligionHappinessRules::load(host->database());
    major_need(rules.rules_sha256()==host->identity().rules_sha256&&s->cities.religions.size()==s->world.religion_culture.religions.size(),
     "BNW happiness actual founded rules/registry differs");
    for(std::size_t i=0;i<s->cities.religions.size();++i){
     const auto& raw=s->world.religion_culture.religions[i];const auto& owned=s->cities.religions[i];std::vector<std::int64_t> expected;
     for(const auto id:rules.resolved_beliefs(raw))if(id>=0&&static_cast<std::uint64_t>(id)<rules.belief_slots())expected.push_back(id);
     major_need(raw.religion==owned.religion&&raw.founder==owned.founder&&owned.beliefs==expected,"BNW happiness actual founded belief mirror differs");
    }
    const auto cities=BnwReligionFollowersReadModel::bind(*s);BnwReligionHappinessTerms terms;
    terms.at_peace=cities.at_peace(actor);const auto beliefs=rules.terms(*entry,terms.at_peace);
    terms.player_happiness=beliefs.player_happiness;terms.per_following_city=beliefs.per_following_city;
    terms.peaceful_foreign_divisor=beliefs.peaceful_foreign_divisor;terms.following_cities=cities.cities_following(religion);
    const float following=static_cast<float>(static_cast<float>(terms.following_cities)*terms.per_following_city);
    major_need(std::isfinite(following)&&static_cast<double>(following)>=INT32_MIN&&static_cast<double>(following)<=INT32_MAX,
     "BNW religious following-city native float conversion overflow");
    terms.following_happiness=static_cast<std::int32_t>(following);
    terms.total=major_happiness_int(static_cast<std::int64_t>(terms.player_happiness)+terms.following_happiness);
    p->happiness_prefix->religion=terms;
    if(terms.peaceful_foreign_divisor>0){p->stage=BnwMajorTeamHasTechPhase::happiness_foreign_founder_pending;p->check(&supplied);return;}
    amount=terms.total;
   }
  }
  auto& raw=s->world.all_players->players.at(static_cast<std::size_t>(actor));auto& field=major_named(raw.core.fields,"m_iHappiness");
  const auto total=major_happiness_int(static_cast<std::int64_t>(field.values.at(0))+amount);
  const bool changed=field.values[0]!=total||s->cities.players.at(actor).excess_happiness.has_value();
  major_need(!changed||s->ui_source_revision<UINT64_MAX,"BNW religion happiness revision exhausted");
  p->happiness_prefix->stores.push_back({BnwHappinessNativeStore::Stage::religion,amount,total});
  field.values[0]=total;s->unhappiness.players.at(actor).happiness=total;s->cities.players.at(actor).excess_happiness.reset();
  p->happiness_stored=total;if(changed)++s->ui_source_revision;
  p->revision=s->ui_source_revision;p->happiness_prefix->after_revision=p->revision;
  p->happiness_prefix->religion_complete=true;p->stage=BnwMajorTeamHasTechPhase::happiness_suffix_pending;p->check(&supplied);
 }catch(...){p->retired.store(true);throw;}
}
}

namespace civ5::runtime {
std::vector<BnwMajorHappinessSuffixEffect> BnwMajorTeamHasTechPending::happiness_suffix_effects()const{
 Impl::Guard guard(impl_);return impl_->happiness_suffixes;
}
void BnwMajorTeamHasTechPending::advance_happiness_suffix(const ui::SelectedBnwPreviewHost& supplied)const{
 const auto p=impl_;Impl::Guard guard(p);
 try{
  const auto host=p->check(&supplied);const auto s=p->state.lock();
  major_need(s&&p->stage==BnwMajorTeamHasTechPhase::happiness_suffix_pending&&p->happiness_prefix&&
   p->happiness_prefix->religion_complete&&p->happiness_prefix->stores.size()==7,
   "BNW happiness suffix needs actual SAME-VM religion store");
  const auto actor=p->happiness_prefix->player;auto& raw=s->world.all_players->players.at(static_cast<std::size_t>(actor));
  const auto rules=BnwHappinessSuffixRules::load(host->database());
  major_need(rules.identity()==host->identity().rules_sha256&&s->ui_source_revision<=UINT64_MAX-7,
   "BNW happiness genuine suffix rules/revision unavailable");
  BnwMajorHappinessSuffixEffect effect;effect.player=actor;effect.before_revision=s->ui_source_revision;
  effect.connection_before=major_happiness_scalar(raw,"m_iCityConnectionHappiness");effect.stores.reserve(7);
  // Reserve every future reached member record and dirty slot before stores.
  p->happiness_suffixes.reserve(22);
  if(!p->happiness_publication){
   major_need(!s->setup.extensions.find<BnwMajorHappinessPublication>(),"BNW happiness dirty queue belongs to another operation");
   BnwMajorHappinessPublication publication;publication.events.resize(22);
   p->happiness_publication=&s->setup.extensions.emplace<BnwMajorHappinessPublication>(std::move(publication));
  }
  p->check(&supplied);p->stage=BnwMajorTeamHasTechPhase::uncertain;
  using Read=detail::BnwHappinessPostReligionRead;
  const auto read=[&](Read stage){return detail::bnw_happiness_post_religion_reached_read(*s,rules,host->identity(),actor,stage);};
  const auto add=[&](BnwHappinessSuffixStage stage,std::int32_t operand){
   auto& field=major_named(raw.core.fields,"m_iHappiness");
   const auto n=major_happiness_int(field.values.at(0)+static_cast<std::int64_t>(operand));
   const bool changed=field.values[0]!=n||s->cities.players.at(actor).excess_happiness.has_value();
   effect.stores.push_back({stage,operand,n});field.values[0]=n;s->unhappiness.players.at(actor).happiness=n;
   s->cities.players.at(actor).excess_happiness.reset();p->happiness_stored=n;p->happiness_expected_excess.reset();
   if(changed)++s->ui_source_revision;p->revision=s->ui_source_revision;p->check(&supplied);
  };
  add(BnwHappinessSuffixStage::wonders,read(Read::wonders).operand);
  add(BnwHappinessSuffixStage::minors,read(Read::minors).operand);
  add(BnwHappinessSuffixStage::leagues,read(Read::leagues).operand);
  // DoUpdateCityConnectionHappiness really recomputes from the current route
  // owner; the next GetHappinessFromTradeRoutes then reads THAT native cache.
  const auto connections=read(Read::connections_cache);major_need(connections.connections.has_value(),"BNW actual city connection quote absent");
  auto& connection=major_named(raw.core.fields,"m_iCityConnectionHappiness").values.at(0);
  effect.connection_after=connections.operand;
  effect.stores.push_back({BnwHappinessSuffixStage::connections_cache,connections.operand,*p->happiness_stored});
  if(connection!=connections.operand){connection=connections.operand;++s->ui_source_revision;}
  p->revision=s->ui_source_revision;p->check(&supplied);
  add(BnwHappinessSuffixStage::connections_add,read(Read::connections_add).operand);
  // Preserve isLocalPlayer() short-circuit: never invent an excess value or
  // call Unhappiness for a nonlocal member merely to fill a diagnostic DTO.
  const auto achievement=read(Read::achievement_condition);
  if(achievement.unhappiness){
   effect.excess=major_happiness_int(achievement.unhappiness->excess_happiness);
   s->cities.players.at(actor).excess_happiness=*effect.excess;p->happiness_expected_excess=effect.excess;
   effect.stores.push_back({BnwHappinessSuffixStage::excess,*effect.excess,*p->happiness_stored});
  }
  effect.achievement_requested=achievement.achievement_requested;
  // Achievement request is retained, not Steam execution (separate milestone).
  const auto occurrence=static_cast<std::uint64_t>(p->happiness_publication->written)+1;
  const BnwMajorHappinessDirtyEvent dirty{actor,*p->happiness_stored,effect.connection_after,effect.excess,p->generation,occurrence,
   BnwTeamTechResearchDirtyBit::game_data,true,effect.achievement_requested};
  p->happiness_publication->events.at(p->happiness_publication->written++)=dirty;
  ++s->ui_source_revision;p->revision=s->ui_source_revision;effect.after_revision=p->revision;
  p->happiness_suffixes.push_back(std::move(effect));
  p->next_happiness_owner=actor+1;
  std::int64_t next=-1;
  for(std::int64_t i=p->next_happiness_owner;i<22;++i){
   const auto& player=s->world.all_players->players.at(static_cast<std::size_t>(i));const auto& alive=major_named(player.core.fields,"m_bAlive").values;
   major_need(alive.size()==1&&(alive[0]==0||alive[0]==1)&&s->setup.players.at(i).alive==(alive[0]!=0)&&
    s->setup.players.at(i).team==p->pregame_teams.at(static_cast<std::size_t>(i)),"BNW happiness next living-major roster differs");
   if(alive[0]&&p->pregame_teams.at(static_cast<std::size_t>(i))==p->request.team){next=i;break;}
  }
  p->check(&supplied);
  if(next>=0){
   // Same native helper will execute the next actual member, not a supplied
   // 'all happiness done' proof. Completed dirty records guard past stores.
   p->happiness_prefix.reset();p->happiness_stored.reset();p->happiness_rules.reset();p->happiness_expected_excess.reset();
   p->stage=BnwMajorTeamHasTechPhase::major_happiness_pending;
  }else p->stage=BnwMajorTeamHasTechPhase::technology_ui_pending;
  p->check(&supplied);
 }catch(...){p->retired.store(true);throw;}
}
}

namespace civ5::runtime {
std::vector<BnwMajorTechUiTransportEvent> BnwMajorTeamHasTechPending::technology_transport_events()const{
 Impl::Guard guard(impl_);return impl_->ui_expected;
}
void BnwMajorTeamHasTechPending::advance_technology_ui_prefix(const ui::SelectedBnwPreviewHost& supplied)const{
 const auto p=impl_;Impl::Guard guard(p);
 try{
  const auto host=p->check(&supplied);const auto s=p->state.lock();
  major_need(s&&p->stage==BnwMajorTeamHasTechPhase::technology_ui_pending&&!p->ui_transport,
   "BNW technology UI requires actual completed living-member happiness loop");
  const auto initialized=major_named(s->world.game_state.flags,"final_initialized").value;
  major_need(initialized==0||initialized==1,"BNW actual final initialization encoding invalid");
  const auto active=detail::bnw_game_active_team_read(*s);
  std::vector<BnwMajorTechUiTransportEvent> batch;
  if(initialized&&p->request.team==active){
   for(const auto* name:{"MiscButtons_DIRTY_BIT","SelectionButtons_DIRTY_BIT","ResearchButtons_DIRTY_BIT"}){
    BnwMajorTechUiTransportEvent event;event.kind=BnwMajorTechUiTransportKind::dirty;
    event.native_name=name;event.dirty=true;batch.push_back(std::move(event));
   }
  }
  // SDK maximum: three dirty requests, 22 ordered notifications, engine event.
  p->ui_expected.reserve(26);BnwMajorTechUiTransportQueue queue;queue.events.resize(26);
  major_need(!s->setup.extensions.find<BnwMajorTechUiTransportQueue>(),"BNW tech transport already issued/foreign");
  p->check(&supplied);p->stage=BnwMajorTeamHasTechPhase::uncertain;
  p->ui_transport=&s->setup.extensions.emplace<BnwMajorTechUiTransportQueue>(std::move(queue));
  p->publish_ui(std::move(batch),*s);p->check(&supplied);
  p->ui_initialized=initialized!=0;
  p->stage=initialized?BnwMajorTeamHasTechPhase::reward_option_pending:BnwMajorTeamHasTechPhase::gameplay_tech_acquired_pending;
  p->check(&supplied);
 }catch(...){p->retired.store(true);throw;}
}
void BnwMajorTeamHasTechPending::advance_technology_reward_ui(const ui::SelectedBnwPreviewHost& supplied,
 const BnwSelectedUserOptionsOwner& options)const{
 const auto p=impl_;Impl::Guard guard(p);
 try{
  const auto host=p->check(&supplied);const auto s=p->state.lock();
  major_need(s&&p->stage==BnwMajorTeamHasTechPhase::reward_option_pending&&p->ui_initialized,
   "BNW reward option requires actual initialized UI prefix");
  // Reserve attempt before the genuine settings owner's source/parent work.
  p->stage=BnwMajorTeamHasTechPhase::uncertain;options.require_current();
  const bool no_popup=options.no_reward_popups();options.require_current();p->check(&supplied);
  std::vector<BnwMajorTechUiTransportEvent> batch;
  if(no_popup){
   const auto* tech=host->technologies().find(static_cast<std::uint64_t>(p->request.technology));
   major_need(tech&&!tech->base.text.key.empty(),"BNW tech award actual TextKey missing");
   for(std::int32_t actor=0;actor<22;++actor){
    const auto& raw=s->world.all_players->players.at(static_cast<std::size_t>(actor));
    const auto& alive=major_named(raw.core.fields,"m_bAlive").values;
    major_need(alive.size()==1&&(alive[0]==0||alive[0]==1)&&raw.saved_player_id==actor&&
     s->setup.players.at(actor).alive==(alive[0]!=0)&&s->setup.players.at(actor).team==p->pregame_teams.at(static_cast<std::size_t>(actor)),
     "BNW tech award true major roster differs");
    if(!alive[0]||p->pregame_teams.at(static_cast<std::size_t>(actor))!=p->request.team)continue;
    const auto& admin=raw.administration;
    major_need(admin.layout==major_bnw&&!admin.blocks.empty()&&admin.blocks[0].name&&std::string_view(admin.blocks[0].name)=="notifications",
     "BNW AddNotification actual owner block missing");
    const auto& ring=admin.blocks[0];const auto has=major_named(ring.fields,"has_notifications").value;
    major_need(has==0||has==1,"BNW notification pointer presence invalid");
    if(!has){major_need(ring.fields.size()==1&&ring.children.empty(),"BNW absent notification owner has saved slots");continue;}
    major_need(major_named(ring.fields,"version").value==2&&major_named(ring.fields,"m_ePlayer").value==actor&&ring.children.size()==100,
     "BNW AddNotification genuine owner identity/schema invalid");
    BnwMajorTechUiTransportEvent event;event.kind=BnwMajorTechUiTransportKind::notification_add;
    event.player=actor;event.native_name="NOTIFICATION_TECH_AWARD";
    event.localization_tag="TXT_KEY_MISC_YOU_DISCOVERED_TECH";event.localization_operands={tech->base.text.key};
    event.x=-1;event.y=-1;event.game_data_index=0;event.extra_game_data=p->request.technology;
    batch.push_back(std::move(event));
   }
  }else{
   // Preserve native short-circuit: the network source is not queried when
   // NoRewardPopups already chose notifications. This owner supports real SP.
   major_need(s->world.pregame&&s->world.pregame->archive.integers.at("game_type")==s->setup.game_type&&s->setup.game_type==0,
    "BNW actual technology award network mode not supported");
   if(p->request.team==detail::bnw_game_active_team_read(*s)){
    BnwMajorTechUiTransportEvent event;event.kind=BnwMajorTechUiTransportKind::popup_add;
    event.player=detail::bnw_game_active_player_read(*s);event.native_name="BUTTONPOPUP_TECH_AWARD";
    event.data1=event.player;event.data2=0;event.data3=p->request.technology;
    batch.push_back(std::move(event));
   }
  }
  options.require_current();p->check(&supplied);p->publish_ui(std::move(batch),*s);p->check(&supplied);
  p->stage=BnwMajorTeamHasTechPhase::gameplay_tech_acquired_pending;p->check(&supplied);
 }catch(...){p->retired.store(true);throw;}
}
void BnwMajorTeamHasTechPending::advance_gameplay_tech_acquired(const ui::SelectedBnwPreviewHost& supplied)const{
 const auto p=impl_;Impl::Guard guard(p);
 try{
  (void)p->check(&supplied);const auto s=p->state.lock();
  major_need(s&&p->stage==BnwMajorTeamHasTechPhase::gameplay_tech_acquired_pending,
   "BNW GameplayTechAcquired must follow actual reward branch");
  BnwMajorTechUiTransportEvent event;event.kind=BnwMajorTechUiTransportKind::gameplay_tech_acquired;
  event.native_name="GameplayTechAcquired"; // Engine interface, never Lua GameEvents.
  std::vector<BnwMajorTechUiTransportEvent> batch;batch.push_back(std::move(event));
  p->check(&supplied);p->stage=BnwMajorTeamHasTechPhase::uncertain;p->publish_ui(std::move(batch),*s);
  p->stage=BnwMajorTeamHasTechPhase::technology_transport_pending;p->check(&supplied);
 }catch(...){p->retired.store(true);throw;}
}
}
