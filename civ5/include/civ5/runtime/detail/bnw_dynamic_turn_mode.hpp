#pragma once
#include "civ5/runtime/live_session.hpp"
#include <array>
#include <climits>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <vector>
#include <utility>

namespace civ5::runtime::detail {
// Component operands, never a scheduler/VM return or a caller readiness grant.
inline void dynamic_need(bool value,const char* why){if(!value)throw std::runtime_error(why);}
inline std::int64_t dynamic_field(const auto& fields,std::string_view name){
 const saves::NamedSaveInteger* found=nullptr;
 for(const auto& f:fields)if(f.name&&std::string_view(f.name)==name){dynamic_need(!found,"Dynamic mode duplicate native field");found=&f;}
 dynamic_need(found&&found->value>=INT32_MIN&&found->value<=INT32_MAX,"Dynamic mode native i32 field absent");return found->value;
}
inline const saves::PlayerCoreField& dynamic_core(const saves::SavedPlayerObservation& p,std::string_view name){
 const saves::PlayerCoreField* found=nullptr;
 for(const auto& f:p.core.fields)if(f.name&&std::string_view(f.name)==name){dynamic_need(!found,"Dynamic mode duplicate player member");found=&f;}
 dynamic_need(found&&found->values.size()==1,"Dynamic mode scalar player member absent");return *found;
}
struct DynamicPlayerOperands {
 std::int64_t id=0,alive=0,team=0,status=0,claim=0,mode=0,notification=-1;
 const void* fields=nullptr;const void* mode_field=nullptr;const void* mode_values=nullptr;
 const void* administration=nullptr;const void* notification_fields=nullptr;
 std::size_t mode_index=0,field_count=0,administration_count=0;
 bool operator==(const DynamicPlayerOperands&)const=default;
};
struct DynamicTeamOperands {
 std::int64_t id=0,alive=0;
 std::array<std::int64_t,64> war{};
 const void* fields=nullptr;const void* arrays=nullptr;const void* war_values=nullptr;
 std::size_t field_count=0,array_count=0;
 bool operator==(const DynamicTeamOperands&)const=default;
};
struct DynamicModeOperands {
 std::array<DynamicPlayerOperands,64> players{};
 std::array<DynamicTeamOperands,64> teams{};
 const void* player_storage=nullptr;const void* team_storage=nullptr;
 bool operator==(const DynamicModeOperands&)const=default;
};
inline DynamicModeOperands capture_dynamic_mode_operands(const LiveSessionState& s){
 constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
 dynamic_need(s.setup.profile==content::TechnologyProfile::bnw&&s.setup.game_type==0&&s.world.pregame&&
  s.world.all_players&&s.world.all_players->layout==bnw&&s.world.all_players->players.size()==64&&s.setup.players.size()==64&&
  s.world.teams&&s.world.teams->layout==bnw&&s.world.teams->teams.size()==64&&s.world.teams->dimensions.teams==64,
  "Dynamic mode requires actual BNW64 Player/Team collections");
 const auto& pg=s.world.pregame->archive.integer_arrays;
 const auto& ts=pg.at("team_types");const auto& ss=pg.at("slot_status");const auto& sc=pg.at("slot_claims");
 dynamic_need(ts.size()==64&&ss.size()==64&&sc.size()==64,"Dynamic mode actual Pregame arrays incomplete");
 DynamicModeOperands out;out.player_storage=s.world.all_players->players.data();out.team_storage=s.world.teams->teams.data();
 for(std::size_t n=0;n<64;++n){
  const auto& raw=s.world.all_players->players[n];const auto at=s.setup.players.find(static_cast<std::int64_t>(n));
  const auto& id=dynamic_core(raw,"m_eID");const auto& alive=dynamic_core(raw,"m_bAlive");const auto& mode=dynamic_core(raw,"m_bDynamicTurnsSimultMode");
  dynamic_need(raw.index==n&&raw.core.layout==bnw&&raw.saved_player_id==id.values[0]&&
   (id.values[0]==static_cast<std::int64_t>(n)||id.values[0]==-1)&&id.encoding==saves::PlayerCoreEncoding::i32&&
   alive.encoding==saves::PlayerCoreEncoding::boolean&&mode.encoding==saves::PlayerCoreEncoding::boolean&&
   (alive.values[0]==0||alive.values[0]==1)&&(mode.values[0]==0||mode.values[0]==1)&&
   at!=s.setup.players.end()&&at->second.alive==(alive.values[0]!=0)&&at->second.team==ts[n]&&
   at->second.slot_status==ss[n]&&at->second.slot_claim==sc[n]&&
   (!alive.values[0]||(id.values[0]==static_cast<std::int64_t>(n)&&ts[n]>=0&&ts[n]<64)),
   "Dynamic mode actual raw/owned Player/Pregame operands differ");
  auto& p=out.players[n];p.id=id.values[0];p.alive=alive.values[0];p.team=ts[n];p.status=ss[n];p.claim=sc[n];p.mode=mode.values[0];
  p.fields=raw.core.fields.data();p.field_count=raw.core.fields.size();p.mode_field=&mode;p.mode_values=mode.values.data();
  p.mode_index=static_cast<std::size_t>(&mode-raw.core.fields.data());
  p.administration=raw.administration.blocks.data();p.administration_count=raw.administration.blocks.size();
  // Reading the saved pointer discriminator is not a notification dispatch.
  if(raw.administration.layout==bnw&&!raw.administration.blocks.empty()){
   const auto& ring=raw.administration.blocks.front();
   if(ring.name&&std::string_view(ring.name)=="notifications"){
    p.notification_fields=ring.fields.data();p.notification=dynamic_field(ring.fields,"has_notifications");
   }
  }
  const auto& team=s.world.teams->teams[n];auto& t=out.teams[n];
  t.id=dynamic_field(team.fields,"id");t.alive=dynamic_field(team.fields,"alive_count");
  dynamic_need(team.index==n&&t.alive>=0&&(t.id==static_cast<std::int64_t>(n)||(t.id==-1&&t.alive==0)),"Dynamic mode native Team ID/alive differs");
  t.fields=team.fields.data();t.arrays=team.arrays.data();t.field_count=team.fields.size();t.array_count=team.arrays.size();
  const saves::PlotArrayObservation* war=nullptr;
  for(const auto& a:team.arrays)if(a.name&&std::string_view(a.name)=="at_war"){dynamic_need(!war,"Dynamic mode duplicate war relation");war=&a;}
  dynamic_need(war&&war->columns==1&&war->values.size()==64,"Dynamic mode native64 war relation absent");t.war_values=war->values.data();
  for(std::size_t k=0;k<64;++k){dynamic_need(war->values[k]==0||war->values[k]==1,"Dynamic mode war boolean invalid");t.war[k]=war->values[k];}
 }
 return out;
}
inline bool dynamic_player_human(const DynamicPlayerOperands& p,std::int64_t pitboss){
 if(p.id==-1)return false; // CvPlayer::isHuman NO_PLAYER guard.
 // Ordinary gameType0: HotSeat/PBEM are false. PitBoss uses value!=0.
 return p.status==3||(p.status==0&&pitboss!=0)||(p.status==4&&p.claim==2);
}
struct DynamicModeCall {std::int32_t team=0,player=0;bool before=false,requested=false;bool notification_required=false;};
class DynamicModeComponentPlan final {
 public:
  static DynamicModeComponentPlan prepare(LiveSessionState& state,std::int64_t pitboss){
   DynamicModeComponentPlan p;p.world_=&state;p.before_=capture_dynamic_mode_operands(state);p.after_=p.before_;
   const auto team_human=[&](std::size_t team){
    for(const auto& player:p.before_.players)if(player.alive&&player.team==p.before_.teams[team].id){
     if(player.id!=-1&&player.status==0)p.pitboss_read_=true;
     if(dynamic_player_human(player,pitboss))return true;
    }
    return false;
   };
   for(std::size_t team=0;team<64;++team){
    bool simultaneous=true;
    if(team_human(team))for(std::size_t opponent=0;opponent<64;++opponent)
     if(p.before_.teams[opponent].alive>0&&p.before_.teams[team].war[opponent]&&team_human(opponent)){simultaneous=false;break;}
    // Native outer loop visits all teams, including dead/barbarian teams.
    ++p.visited_;
    for(std::size_t player=0;player<64;++player){
     const auto& old=p.before_.players[player];if(!old.alive||old.team!=p.before_.teams[team].id)continue;
     const bool changed=(old.mode!=0)!=simultaneous;
     // Read(false) does not clear a prior major-player notification pointer:
     // reset calls Uninit, not delete. Only the non-major constructor/Init
     // lineage (NULL; Init allocates for IDs<22 only) qualifies a saved NULL.
     const bool known_null=old.notification==0&&old.id>=22;
     const bool needs_owner=changed&&!known_null;
     p.calls_.push_back({static_cast<std::int32_t>(team),static_cast<std::int32_t>(player),old.mode!=0,simultaneous,needs_owner});
     if(needs_owner){p.blocked_=std::pair{static_cast<std::int32_t>(team),static_cast<std::int32_t>(player)};return p;}
     ++p.returned_;
     if(changed){p.stores_.push_back(player);p.after_.players[player].mode=simultaneous?1:0;}
    }
   }
   return p;
  }
  void require_current(const LiveSessionState& state,bool after)const{
   dynamic_need((after?applied_:!applied_)&& &state==world_&&capture_dynamic_mode_operands(state)==(after?after_:before_),"Dynamic mode actual physical source operands changed");
  }
  // Caller finishes all fallible/provider checks first. No allocations,
  // notification callback, Lua hook, RNG draw or raw storage replacement here.
  bool apply_prepared_without_callbacks(LiveSessionState& state)noexcept{
   if(applied_||&state!=world_)return false;applied_=true;
   for(auto player:stores_){const auto field=before_.players[player].mode_index;
    state.world.all_players->players[player].core.fields[field].values[0]=after_.players[player].mode;}
   return true;
  }
  const auto& calls()const noexcept{return calls_;}
  const auto& blocked()const noexcept{return blocked_;}
  std::uint64_t teams_visited()const noexcept{return visited_;}
  std::uint64_t returned_setters()const noexcept{return returned_;}
  std::uint64_t stores()const noexcept{return stores_.size();}
  bool applied()const noexcept{return applied_;}
  bool pitboss_read()const noexcept{return pitboss_read_;}
 private:
  DynamicModeComponentPlan()=default;
  LiveSessionState* world_=nullptr;DynamicModeOperands before_,after_;
  std::vector<DynamicModeCall> calls_;std::vector<std::size_t> stores_;
  std::optional<std::pair<std::int32_t,std::int32_t>> blocked_;
  std::uint64_t visited_=0,returned_=0;bool applied_=false,pitboss_read_=false;
};
} // namespace civ5::runtime::detail


namespace civ5::runtime::detail {
// Real component state/effects only; neither this component nor its observations
// issues a Source/VM/cursor predecessor. Production supplies the actual session.
class TurnShuffleComponent final {
 public:
  enum class ScanReturn { dead, human, equal_active, changed_active_required };
  static TurnShuffleComponent prepare(LiveSessionState& state){
   TurnShuffleComponent p;p.world_=&state;p.operands_=capture_dynamic_mode_operands(state);
   p.active_=capture_active(state);p.map_=state.random.map.state();p.before_=state.random.gameplay.state();p.after_=p.before_;
   dynamic_need(state.random.map.algorithm()==RandomAlgorithm::bnw&&state.random.gameplay.algorithm()==RandomAlgorithm::bnw&&
    p.before_.seed<=UINT32_MAX&&p.before_.calls<=UINT32_MAX-64ULL&&p.before_.resets<=UINT32_MAX,
    "Turn shuffle real BNW RNG64 counter capacity unavailable");
   for(std::size_t n=0;n<64;++n)p.permutation_[n]=static_cast<std::int32_t>(n);
   return p;
  }
  void require_current(const LiveSessionState& state,bool after)const{
   dynamic_need(world_==&state&&capture_dynamic_mode_operands(state)==operands_&&capture_active(state)==active_&&
    state.random.map.algorithm()==RandomAlgorithm::bnw&&state.random.gameplay.algorithm()==RandomAlgorithm::bnw&&
    state.random.map.state()==map_&&state.random.gameplay.state()==(after?after_:before_)&&
    (!after||(draws_==64&&attempted_&&after_.calls==before_.calls+64&&after_.resets==before_.resets)),
    "Turn shuffle physical roster/RNG source changed");
  }
  void draw_without_callbacks(LiveSessionState& state){
   require_current(state,false);dynamic_need(!attempted_&&draws_==0,"Turn shuffle real RNG draws repeated");attempted_=true;
   // SDK CvGameCoreUtils.cpp876-886 includes the last bound1 draw. All storage
   // and overflow preflight precedes the first mutation; no allocator/provider.
   for(std::size_t n=0;n<64;++n){
    const auto value=state.random.gameplay.draw(64ULL-static_cast<std::uint64_t>(n));
    values_[n]=value;const auto other=n+static_cast<std::size_t>(value);
    if(n!=other)std::swap(permutation_[n],permutation_[other]);
    after_=state.random.gameplay.state();++draws_;
   }
  }
  ScanReturn scan_one_without_callbacks(const LiveSessionState& state,std::int64_t actual_pitboss){
   require_current(state,true);dynamic_need(scanned_<64&&!pending_,"Turn shuffle scan repeated or changed setter pending");
   const auto index=static_cast<std::size_t>(permutation_[scanned_]);const auto& player=operands_.players[index];
   ++scanned_; // real alive query occurs before the human predicate
   if(!player.alive)return ScanReturn::dead;
   if(dynamic_player_human(player,actual_pitboss))return ScanReturn::human;
   if(active_.values[index]){++equal_returns_;return ScanReturn::equal_active;}
   pending_=static_cast<std::int32_t>(index);return ScanReturn::changed_active_required;
  }
  bool next_requires_pitboss()const{
   dynamic_need(draws_==64&&scanned_<64&&!pending_,"Turn shuffle human query outside current scan");
   const auto& p=operands_.players[static_cast<std::size_t>(permutation_[scanned_])];
   return p.alive&&p.id!=-1&&p.status==0;
  }
  std::uint64_t draws()const noexcept{return draws_;}
  std::uint64_t scanned()const noexcept{return scanned_;}
  std::uint64_t equal_returns()const noexcept{return equal_returns_;}
  bool attempted()const noexcept{return attempted_;}
  bool loop_returned()const noexcept{return draws_==64&&scanned_==64&&!pending_;}
  std::optional<std::int32_t> pending()const noexcept{return pending_;}
  const auto& permutation()const noexcept{return permutation_;}
  const auto& values()const noexcept{return values_;}
  const SimulationRandomState& before_rng()const noexcept{return before_;}
  const SimulationRandomState& after_rng()const noexcept{return after_;}
  const DynamicModeOperands& operands()const noexcept{return operands_;}
 private:
  struct Active {std::array<std::int64_t,64> values{};std::array<const void*,64> allocations{};bool operator==(const Active&)const=default;};
  static Active capture_active(const LiveSessionState& state){
   dynamic_need(state.world.all_players&&state.world.all_players->players.size()==64,"Turn shuffle actual Player64 absent");Active result;
   for(std::size_t n=0;n<64;++n){const auto& f=dynamic_core(state.world.all_players->players[n],"m_bTurnActive");
    dynamic_need(f.encoding==saves::PlayerCoreEncoding::boolean&&(f.values[0]==0||f.values[0]==1),"Turn shuffle actual TurnActive bool absent");
    result.values[n]=f.values[0];result.allocations[n]=f.values.data();}
   return result;
  }
  LiveSessionState* world_=nullptr;DynamicModeOperands operands_;Active active_;
  SimulationRandomState map_,before_,after_;
  std::array<std::int32_t,64> permutation_{};std::array<std::uint64_t,64> values_{};
  std::uint64_t draws_=0,scanned_=0,equal_returns_=0;bool attempted_=false;
  std::optional<std::int32_t> pending_;
};
} // namespace civ5::runtime::detail
