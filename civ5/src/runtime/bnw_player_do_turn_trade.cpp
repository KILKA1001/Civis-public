#include "civ5/runtime/bnw_player_do_turn_trade.hpp"
#include "civ5/runtime/bnw_player_do_turn_revolts.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/bnw_player_do_turn_military.hpp"
#include <algorithm>
#include <climits>
#include <stdexcept>
#include <type_traits>
namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void need(bool b,const char *why){if(!b)throw std::runtime_error(why);}
auto &named(auto &rows,std::string_view name){using R=std::remove_reference_t<decltype(rows.front())>;R *found=nullptr;for(auto &r:rows)if(r.name&&name==r.name){need(!found,"Trade duplicate source field/record");found=&r;}need(found,"Trade required source field/record unknown");return *found;}
void i32(std::int64_t v){need(v>=INT32_MIN&&v<=INT32_MAX,"Trade serialized source int exceeds native width");}
void exact(const saves::PlayerTradeCultureRecord &r,std::initializer_list<std::string_view> names){need(r.fields.size()==names.size()&&r.arrays.empty()&&r.blobs.empty(),"Trade current source record schema incomplete/unknown");for(auto n:names)i32(named(r.fields,n).value);}
void history(const saves::PlayerTradeCultureRecord &r,bool plundered){
 need(r.children.empty()&&r.arrays.empty()&&r.blobs.empty()&&r.fields.size()==(plundered?14:15),"Trade full native history route schema incomplete");
 if(!plundered)i32(named(r.fields,"m_iID").value);
 for(auto n:{"m_iOriginX","m_iOriginY","m_iDestX","m_iDestY","m_eOriginOwner","m_eDestOwner","m_eDomain","m_eConnectionType","m_iTradeUnitLocationIndex","m_bTradeUnitMovingForward","m_unitID","m_iCircuitsCompleted","m_iCircuitsToComplete","m_iTurnRouteComplete"})i32(named(r.fields,n).value);
 const auto flag=named(r.fields,"m_bTradeUnitMovingForward").value;need(flag==0||flag==1,"Trade history moving-forward flag malformed");
}
void complete(const LiveSessionState &s,std::int64_t player){
 need(s.setup.profile==content::TechnologyProfile::bnw&&s.world.all_players&&s.world.all_players->layout==bnw&&s.world.all_players->players.size()==64&&s.world.pregame&&s.world.pregame->layout==bnw&&s.world.pregame->archive.integers.at("game_turn")==s.setup.game_turn,"Trade current complete registry/profile/clock missing");
 need(player>=0&&player<22&&s.setup.players.at(player).alive,"Trade living major source player missing");
 const auto &raw=s.world.all_players->players.at(player);need(raw.saved_player_id==player,"Trade source player ID differs");
 const auto &alive=named(raw.core.fields,"m_bAlive");need(alive.values.size()==1&&alive.values[0]==1,"Trade raw/shared alive flags differ");
 const auto &minors=s.world.pregame->archive.flag_arrays.at("minor_nation_civs");need(minors.size()==64&&!minors.at(player),"Trade source minor flag excludes native major branch");
 const auto &g=s.world.trade;
 need(g.layout==bnw&&g.dimensions.major_slots==22&&g.dimensions.yield_slots==6&&g.version&&*g.version<=3&&g.profile_extensions.values.empty(),"Trade global full BNW Read schema unknown; no default-empty inference");
 need(g.start_offset<g.routes_end_offset&&g.routes_end_offset==g.matrices_offset&&g.matrices_offset<g.end_offset,"Trade global full Read collection metadata unknown");
 need(g.version.value()>=3?g.technology_difference&&g.technology_difference->size()==484:!g.technology_difference,"Trade source matrix version/width unknown");
 if(g.technology_difference)for(auto v:*g.technology_difference)i32(v);i32(g.next_id);
 for(const auto &route:g.routes){
  need(route.profile_extensions.values.empty()&&route.origin_yields.size()==6&&route.destination_yields.size()==6&&route.id.has_value()==(*g.version>=1),"Trade complete global route/yield schema unknown");if(route.id)i32(*route.id);
  for(auto v:{route.origin_x,route.origin_y,route.destination_x,route.destination_y,route.origin_owner,route.destination_owner,route.domain,route.connection_type,route.unit_location_index,route.unit_id,route.circuits_completed,route.circuits_to_complete,route.turn_route_complete})i32(v);
  need(*g.version>=2||route.turn_route_complete==0,"Trade v0/1 source Read turn-complete default differs");
  for(auto v:route.origin_yields)i32(v);for(auto v:route.destination_yields)i32(v);for(const auto &p:route.plots){i32(p.x);i32(p.y);}
 }
 const auto &culture=raw.trade_culture;need(culture.layout==bnw&&culture.dimensions.major==22&&culture.dimensions.yields==6&&culture.start_offset<culture.end_offset,"Trade player full Read metadata unknown");
 const auto &t=named(culture.blocks,"player_trade");need(t.start_offset>=culture.start_offset&&t.start_offset<t.end_offset&&t.end_offset<=culture.end_offset,"Trade player Read block boundary unknown");
 const auto v=named(t.fields,"version").value;need(v>=0&&v<=2,"Trade player version unknown");
 if(v==0){exact(t,{"version","legacy_empty_value"});need(t.children.empty(),"Trade Reset/Read v0 history differs");}
 else if(v==1)exact(t,{"version","m_aRecentlyExpiredConnections_wire_count"});
 else exact(t,{"version","m_aRecentlyExpiredConnections_wire_count","m_aTradeConnectionWasPlundered_wire_count"});
 std::int64_t expired=0,plundered=0;
 for(const auto &r:t.children){
  if(r.name&&std::string_view(r.name)=="expired_route"){need(v>=1,"Trade v0 expired roster unknown");history(r,false);++expired;}
  else{need(v>=2&&r.name&&std::string_view(r.name)=="plundered_route","Trade unknown current history child");exact(r,{"m_iTurnPlundered"});need(r.children.size()==1&&r.children[0].name&&std::string_view(r.children[0].name)=="m_kTradeConnection","Trade actual plunder route payload unknown");history(r.children[0],true);const auto turn=named(r.fields,"m_iTurnPlundered").value;need(turn>=-1&&turn<=INT32_MAX-30,"Trade source plunder age addition overflow/invalid");++plundered;}
 }
 if(v>=1)need(named(t.fields,"m_aRecentlyExpiredConnections_wire_count").value==expired,"Trade complete expired Read roster/count differs");
 if(v>=2)need(named(t.fields,"m_aTradeConnectionWasPlundered_wire_count").value==plundered,"Trade complete plunder Read roster/count differs");
}
struct Hash {
 std::vector<std::uint8_t> b;
 void n(std::int64_t v){const auto u=static_cast<std::uint64_t>(v);for(unsigned i=0;i<8;++i)b.push_back(static_cast<std::uint8_t>(u>>(8*i)));}
 void t(std::string_view v){n(v.size());b.insert(b.end(),v.begin(),v.end());}
 void r(const saves::PlayerTradeCultureRecord &v){t(v.name?v.name:"");n(v.index);n(v.start_offset);n(v.end_offset);n(v.fields.size());for(const auto &f:v.fields){t(f.name?f.name:"");n(f.value);}n(v.arrays.size());for(const auto &a:v.arrays){t(a.name?a.name:"");n(a.columns);n(a.values.size());for(auto x:a.values)n(x);}n(v.blobs.size());for(const auto &a:v.blobs){t(a.name?a.name:"");n(a.bytes.size());b.insert(b.end(),a.bytes.begin(),a.bytes.end());}n(v.children.size());for(const auto &c:v.children)r(c);}
};
}
std::string bnw_player_trade_registry_revision(const LiveSessionState &s,std::int64_t player){
 need(s.world.all_players&&player>=0&&player<static_cast<std::int64_t>(s.world.all_players->players.size()),"Trade digest actual player registry missing");Hash h;const auto &g=s.world.trade;
 h.n(static_cast<unsigned>(g.layout));h.n(g.dimensions.major_slots);h.n(g.dimensions.yield_slots);h.n(g.version.has_value());if(g.version)h.n(*g.version);h.n(g.start_offset);h.n(g.routes_end_offset);h.n(g.matrices_offset);h.n(g.end_offset);h.n(g.next_id);h.n(g.profile_extensions.values.size());h.n(g.routes.size());
 for(const auto &r:g.routes){h.n(r.id.has_value());if(r.id)h.n(*r.id);for(auto v:{r.origin_x,r.origin_y,r.destination_x,r.destination_y,r.origin_owner,r.destination_owner,r.domain,r.connection_type,r.unit_location_index,r.unit_id,r.circuits_completed,r.circuits_to_complete,r.turn_route_complete})h.n(v);h.n(r.moving_forward);h.n(r.plots.size());for(const auto &p:r.plots){h.n(p.x);h.n(p.y);}h.n(r.origin_yields.size());for(auto v:r.origin_yields)h.n(v);h.n(r.destination_yields.size());for(auto v:r.destination_yields)h.n(v);h.n(r.profile_extensions.values.size());}
 h.n(g.technology_difference.has_value());if(g.technology_difference){h.n(g.technology_difference->size());for(auto v:*g.technology_difference)h.n(v);}
 const auto &p=s.world.all_players->players.at(player);h.n(p.saved_player_id);h.n(s.world.pregame.has_value());if(s.world.pregame){const auto minor=s.world.pregame->archive.flag_arrays.find("minor_nation_civs");h.n(minor!=s.world.pregame->archive.flag_arrays.end());if(minor!=s.world.pregame->archive.flag_arrays.end()){h.n(minor->second.size());for(auto flag:minor->second)h.n(flag);}}const auto &c=p.trade_culture;h.n(static_cast<unsigned>(c.layout));h.n(c.dimensions.major);h.n(c.dimensions.yields);h.n(c.start_offset);h.n(c.end_offset);h.r(named(c.blocks,"player_trade"));return identify_bytes(h.b).sha256;
}
BnwPlayerDoTurnTradeJournal stage_bnw_player_do_turn_trade(LiveSessionState &state,const BnwPlayerDoTurnRevoltsJournal &prior,const BnwHappinessSuffixRules &native,const content::RulesDatabase &db,const SessionCheckpointIdentity &id,std::int64_t player){
 need(prior.trade_required_next()&&prior.after_world_revision()==bnw_player_do_turn_revolts_revision(state,native,db,id,player),"Trade requires SAME unchanged accepted revolt/native rules/RNG registry");complete(state,player);
 // Source UpdateValues skips empty coords. MoveUnits does NOT: own-origin
 // always calls MoveUnit including TradeVisuals_UpdateRouteDirection.
 for(const auto &r:state.world.trade.routes){need(r.origin_owner!=player,"Trade mandatory MoveUnit/visual/expiry owner unavailable (including empty own-origin)");const bool empty=r.origin_x==-1&&r.origin_y==-1&&r.destination_x==-1&&r.destination_y==-1;need(empty||r.destination_owner!=player,"Trade mandatory destination yield query owner unavailable");}
 BnwPlayerDoTurnTradeJournal j;j.before_=bnw_player_do_turn_revolts_revision(state,native,db,id,player);j.before_rng_=state.random.gameplay.state();auto trial=state;
 j.native_=apply_bnw_player_trade_turn_without_player_routes(trial,player);
 using S=BnwPlayerDoTurnTradeStep;j.steps_={S::clear_expired,S::update_connection_values,S::age_plundered,S::move_units};
 j.after_=bnw_player_do_turn_revolts_revision(trial,native,db,id,player);j.after_rng_=trial.random.gameplay.state();j.military_=bnw_player_military_registry_revision(trial,player);need(j.after_rng_==j.before_rng_,"Trade supported branch unexpectedly consumed gameplay RNG");
 static_assert(std::is_nothrow_move_assignable_v<LiveSessionState>&&std::is_nothrow_move_constructible_v<BnwPlayerDoTurnTradeJournal>);state=std::move(trial);return j;
}
}
