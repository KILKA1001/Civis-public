#include "civ5/runtime/bnw_player_do_turn_military.hpp"
#include "civ5/runtime/bnw_player_do_turn_revolts.hpp"
#include "civ5/runtime/bnw_player_grand_military_average.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <climits>
#include <stdexcept>
#include <type_traits>
namespace civ5::runtime {
namespace {
void need(bool b,const char *why){if(!b)throw std::runtime_error(why);}
auto &named(auto &rows,std::string_view name){using R=std::remove_reference_t<decltype(rows.front())>;R *found=nullptr;for(auto &r:rows)if(r.name&&name==r.name){need(!found,"Military duplicate source field/record");found=&r;}need(found,"Military current required source field/record missing");return *found;}
void i32(std::int64_t v){need(v>=INT32_MIN&&v<=INT32_MAX,"Military source int width invalid");}
void schema(const LiveSessionState &s,std::int64_t player){
 constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
 need(s.world.all_players&&s.world.all_players->layout==bnw&&s.world.all_players->players.size()==64&&player>=0&&player<22&&s.world.all_players->players.at(player).saved_player_id==player,"Military actual player Read roster incomplete");
 const auto &p=s.world.all_players->players.at(player).strategy;need(p.layout==bnw&&p.dimensions.major==22&&p.dimensions.civ_players==63&&p.dimensions.yields==6&&p.dimensions.grand==4&&p.dimensions.profile_extensions.values.empty()&&p.start_offset<p.end_offset,"Military original current BNW Read schema unknown");
 const auto &m=named(p.blocks,"military_ai");need(m.start_offset>=p.start_offset&&m.start_offset<m.end_offset&&m.end_offset<=p.end_offset&&m.fields.size()==5&&m.arrays.size()==6&&m.hashes.empty(),"Military source Read block/field inventory unknown");
 need(named(m.fields,"version").value==1,"Military current source version unsupported");for(auto n:{"total_threat_weight","army_type_building","ops_build_skips","strategy_wire_count"})i32(named(m.fields,n).value);
 const auto count=named(m.fields,"strategy_wire_count").value;need(count>=0,"Military source strategy count invalid");
 for(auto n:{"using_strategy","turn_strategy_adopted","last_turn_war_state","last_turn_military_threat","last_turn_military_strength","last_turn_target_value"}){const auto &a=named(m.arrays,n);need(a.columns==1&&a.values.size()==static_cast<std::uint64_t>(std::string_view(n)=="using_strategy"||std::string_view(n)=="turn_strategy_adopted"?count:63),"Military actual Read array width differs");for(auto v:a.values){i32(v);if(std::string_view(n)=="using_strategy")need(v==0||v==1,"Military strategy flag invalid");}}
}
}
std::string bnw_player_military_registry_revision(const LiveSessionState &s,std::int64_t player){
 need(s.world.all_players&&player>=0&&player<static_cast<std::int64_t>(s.world.all_players->players.size()),"Military registry digest missing actual player");
 std::vector<std::uint8_t> bytes;const auto num=[&](std::int64_t v){const auto u=static_cast<std::uint64_t>(v);for(unsigned i=0;i<8;++i)bytes.push_back(static_cast<std::uint8_t>(u>>(i*8)));};const auto text=[&](std::string_view v){num(v.size());bytes.insert(bytes.end(),v.begin(),v.end());};
 const auto &p=s.world.all_players->players.at(player);num(p.saved_player_id);const auto &st=p.strategy;num(static_cast<unsigned>(st.layout));num(st.start_offset);num(st.end_offset);num(st.dimensions.major);num(st.dimensions.civ_players);num(st.dimensions.yields);num(st.dimensions.grand);num(st.dimensions.profile_extensions.values.size());
 const auto &m=named(st.blocks,"military_ai");text(m.name);num(m.start_offset);num(m.end_offset);num(m.fields.size());for(const auto &f:m.fields){text(f.name?f.name:"");num(f.value);}num(m.arrays.size());for(const auto &a:m.arrays){text(a.name?a.name:"");num(a.columns);num(a.values.size());for(auto v:a.values)num(v);}num(m.hashes.size());
 const auto c=s.military_request_counters.find(player);num(c!=s.military_request_counters.end());if(c!=s.military_request_counters.end()){num(c->second.land);num(c->second.naval);}return identify_bytes(bytes).sha256;
}
BnwPlayerDoTurnMilitaryJournal stage_bnw_player_do_turn_military(LiveSessionState &state,const BnwPlayerDoTurnTradeJournal &trade,const BnwHappinessSuffixRules &native,const content::RulesDatabase &db,const SessionCheckpointIdentity &id,std::int64_t player){
 need(trade.native().player_id==player&&trade.after_world_revision()==bnw_player_do_turn_revolts_revision(state,native,db,id,player)&&trade.military_input_revision()==bnw_player_military_registry_revision(state,player),"Military Reset requires SAME unchanged accepted Trade/world/counter registry");schema(state,player);
 BnwPlayerDoTurnMilitaryJournal j;j.before_=bnw_player_military_registry_revision(state,player);if(const auto c=state.military_request_counters.find(player);c!=state.military_request_counters.end())j.before_counters_=c->second;
 auto trial=state;j.native_=apply_bnw_player_military_reset_counters(trial,player);need(trial.military_request_counters.at(player).land==0&&trial.military_request_counters.at(player).naval==0,"Military source Reset stores incomplete");j.after_=bnw_player_military_registry_revision(trial,player);j.grand_=bnw_grand_military_inputs_revision(trial);
 static_assert(std::is_nothrow_move_assignable_v<LiveSessionState>&&std::is_nothrow_move_constructible_v<BnwPlayerDoTurnMilitaryJournal>);state=std::move(trial);return j;
}
}
