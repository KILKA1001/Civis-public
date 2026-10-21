#include "civ5/runtime/bnw_player_grand_military_average.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/bnw_player_do_turn_revolts.hpp"
#include <climits>
#include <stdexcept>
#include <type_traits>
#include <variant>
namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void need(bool b,const char *why){if(!b)throw std::runtime_error(why);}
auto &named(const auto &rows,std::string_view name){using R=typename std::remove_cvref_t<decltype(rows)>::value_type;const R *found=nullptr;for(const auto &r:rows)if(r.name&&name==r.name){need(!found,"Grand military duplicate native query field");found=&r;}need(found,"Grand military required native query field unavailable");return *found;}
std::int64_t i32(std::int64_t v){need(v>=INT32_MIN&&v<=INT32_MAX,"Grand military native int width invalid");return v;}
std::int64_t scalar(const saves::SavedPlayerObservation &p,std::string_view name){const auto &f=named(p.core.fields,name);need(f.values.size()==1&&(f.encoding==saves::PlayerCoreEncoding::i32||(name=="m_bAlive"&&f.encoding==saves::PlayerCoreEncoding::boolean)),"Grand military native player scalar encoding unknown");return i32(f.values[0]);}
const auto &team(const LiveSessionState &s,std::int64_t id){need(id>=0&&id<64,"Grand military native team ID invalid");const auto &t=s.world.teams->teams.at(id);need(t.index==static_cast<std::uint64_t>(id)&&named(t.fields,"id").value==id,"Grand military actual team Read identity differs");return t;}
struct Digest {
 std::vector<std::uint8_t> bytes;
 void n(std::int64_t v){const auto u=static_cast<std::uint64_t>(v);for(unsigned i=0;i<8;++i)bytes.push_back(static_cast<std::uint8_t>(u>>(i*8)));}
 void t(std::string_view v){n(v.size());bytes.insert(bytes.end(),v.begin(),v.end());}
};
}
std::string bnw_grand_military_inputs_revision(const LiveSessionState &s){
 Digest h;h.n(static_cast<unsigned>(s.setup.profile));h.n(static_cast<unsigned>(s.world.game_state.layout));h.n(s.world.game_state.prefix.index());
 if(const auto *prefix=std::get_if<saves::Expansion2PrefixObservation>(&s.world.game_state.prefix)){h.n(prefix->scalars.size());for(auto v:prefix->scalars)h.n(v);}
 h.n(static_cast<unsigned>(s.maintenance.profile));h.n(s.maintenance.elapsed_turns);
 h.n(s.world.all_players.has_value());if(s.world.all_players){h.n(static_cast<unsigned>(s.world.all_players->layout));h.n(s.world.all_players->players.size());for(const auto &p:s.world.all_players->players){h.n(p.index);h.n(p.saved_player_id);h.n(static_cast<unsigned>(p.core.layout));h.n(p.core.fields.size());for(const auto &f:p.core.fields){h.t(f.name?f.name:"");h.n(static_cast<unsigned>(f.encoding));h.n(f.values.size());for(auto v:f.values)h.n(v);}}}
 h.n(s.world.pregame.has_value());if(s.world.pregame){h.n(static_cast<unsigned>(s.world.pregame->layout));const auto it=s.world.pregame->archive.integer_arrays.find("team_types");h.n(it!=s.world.pregame->archive.integer_arrays.end());if(it!=s.world.pregame->archive.integer_arrays.end()){h.n(it->second.size());for(auto v:it->second)h.n(v);}}
 h.n(s.setup.players.size());for(const auto &[id,p]:s.setup.players){h.n(id);h.n(p.alive);h.n(p.team);}
 h.n(s.world.teams.has_value());if(s.world.teams){const auto &ts=*s.world.teams;h.n(static_cast<unsigned>(ts.layout));h.n(ts.start_offset);h.n(ts.end_offset);h.n(ts.dimensions.teams);h.n(ts.dimensions.players);h.n(ts.dimensions.major);h.n(ts.dimensions.civ_teams);h.n(ts.dimensions.profile_extensions.values.size());h.n(ts.teams.size());for(const auto &t:ts.teams){h.n(t.index);h.n(t.start_offset);h.n(t.end_offset);h.n(t.fields.size());for(const auto &f:t.fields){h.t(f.name?f.name:"");h.n(f.value);}h.n(t.arrays.size());for(const auto &a:t.arrays){h.t(a.name?a.name:"");h.n(a.columns);h.n(a.values.size());for(auto v:a.values)h.n(v);}}}
 return identify_bytes(h.bytes).sha256;
}
BnwGrandMilitaryAverageJournal query_bnw_grand_current_military_average(const LiveSessionState &s,const BnwHappinessSuffixRules &native,const content::RulesDatabase &db,const SessionCheckpointIdentity &id,std::int64_t player){
 need(id.payload_sha256.size()==64&&id.rules_sha256==native.identity()&&identify_bytes(db.gameplay_fingerprint_data()).sha256==id.rules_sha256,"Grand current military actual source/rules identity differs");
 need(s.setup.profile==content::TechnologyProfile::bnw&&s.world.all_players&&s.world.all_players->layout==bnw&&s.world.all_players->players.size()==64&&player>=0&&player<22,"Grand military actual BNW/major Read roster unavailable");
 need(s.world.game_state.layout==bnw&&s.maintenance.profile==content::TechnologyProfile::bnw&&std::holds_alternative<saves::Expansion2PrefixObservation>(s.world.game_state.prefix),"Grand military native elapsed clock profile unavailable");
 const auto &clock=std::get<saves::Expansion2PrefixObservation>(s.world.game_state.prefix);need(clock.scalars.size()>1&&clock.scalars[1]>=0&&clock.scalars[1]<=INT32_MAX&&s.maintenance.elapsed_turns==clock.scalars[1],"Grand military native elapsed clock/owned mirror differs");
 need(s.world.teams&&s.world.teams->layout==bnw&&s.world.teams->teams.size()==64&&s.world.teams->dimensions.teams==64&&s.world.teams->dimensions.players==64&&s.world.teams->dimensions.major==22&&s.world.teams->dimensions.civ_teams==63&&s.world.teams->dimensions.profile_extensions.values.empty(),"Grand military complete actual team Read roster unavailable");
 need(s.world.pregame&&s.world.pregame->layout==bnw,"Grand military actual CvPreGame player-team owner unavailable");const auto mapping=s.world.pregame->archive.integer_arrays.find("team_types");need(mapping!=s.world.pregame->archive.integer_arrays.end()&&mapping->second.size()==64,"Grand military native CvPreGame team_types64 inventory unknown");const auto &player_teams=mapping->second;
 const auto input=bnw_grand_military_inputs_revision(s);
 const auto &own=s.world.all_players->players.at(player);const auto own_team=i32(player_teams.at(player));need(own.index==static_cast<std::uint64_t>(player)&&own.saved_player_id==player&&own.core.layout==bnw&&scalar(own,"m_bAlive")==1&&s.setup.players.at(player).alive&&own_team==s.setup.players.at(player).team,"Grand military actual alive player/team mirrors differ");
 const auto &met=named(team(s,own_team).arrays,"has_met");need(met.columns==1&&met.values.size()==64,"Grand military native hasMet64 inventory unknown");for(auto v:met.values)need(v==0||v==1,"Grand military native hasMet flag invalid");
 BnwGrandMilitaryAverageJournal j;j.player_=player;j.elapsed_=clock.scalars[1];j.input_=input;
 for(std::int64_t p=0;p<22;++p){
  const auto &raw=s.world.all_players->players.at(p);need(raw.index==static_cast<std::uint64_t>(p)&&raw.saved_player_id==p&&raw.core.layout==bnw,"Grand military native major slot identity differs");const auto alive=scalar(raw,"m_bAlive");need((alive==0||alive==1)&&s.setup.players.at(p).alive==(alive==1),"Grand military actual major alive mirrors differ");
  if(!alive)continue;
  const auto target_team=i32(player_teams.at(p));need(target_team==s.setup.players.at(p).team,"Grand military actual alive major team mirrors differ");(void)team(s,target_team);
  // includeMe=true does NOT bypass isHasMet, including the actual self team.
  if(!met.values.at(target_team))continue;
  const auto turn=scalar(raw,"m_iTurnMightRecomputed");
  if(turn<j.elapsed_)throw std::runtime_error("Grand military GetMilitaryMight player="+std::to_string(p)+" requires actual calculateMilitaryMight AND calculateEconomicMight; source cache stale");
  const auto might=scalar(raw,"m_iMilitaryMight");
  const auto total=j.sum_+might;i32(total);j.sum_=total;j.participants_.push_back({p,target_team,might,turn});
 }
 if(!j.participants_.empty())j.average_=j.sum_/static_cast<std::int64_t>(j.participants_.size());
 static_assert(std::is_nothrow_move_constructible_v<BnwGrandMilitaryAverageJournal>);return j;
}
BnwGrandMilitaryAverageJournal stage_bnw_grand_military_average(const LiveSessionState &s,const BnwPlayerDoTurnTradeJournal &trade,const BnwPlayerDoTurnMilitaryJournal &military,const BnwHappinessSuffixRules &native,const content::RulesDatabase &db,const SessionCheckpointIdentity &id,std::int64_t player){
 need(s.setup.profile==content::TechnologyProfile::bnw&&s.world.all_players&&s.world.all_players->layout==bnw&&s.world.all_players->players.size()==64&&player>=0&&player<22,"Grand military actual BNW/major Read roster unavailable");
 need(s.world.game_state.layout==bnw&&s.maintenance.profile==content::TechnologyProfile::bnw&&std::holds_alternative<saves::Expansion2PrefixObservation>(s.world.game_state.prefix),"Grand military native elapsed clock profile unavailable");
 const auto &clock=std::get<saves::Expansion2PrefixObservation>(s.world.game_state.prefix);need(clock.scalars.size()>1&&clock.scalars[1]>=0&&clock.scalars[1]<=INT32_MAX&&s.maintenance.elapsed_turns==clock.scalars[1],"Grand military native elapsed clock/owned mirror differs");
 need(s.world.teams&&s.world.teams->layout==bnw&&s.world.teams->teams.size()==64&&s.world.teams->dimensions.teams==64&&s.world.teams->dimensions.players==64&&s.world.teams->dimensions.major==22&&s.world.teams->dimensions.civ_teams==63&&s.world.teams->dimensions.profile_extensions.values.empty(),"Grand military complete actual team Read roster unavailable");
 need(s.world.pregame&&s.world.pregame->layout==bnw,"Grand military actual CvPreGame player-team owner unavailable");const auto mapping=s.world.pregame->archive.integer_arrays.find("team_types");need(mapping!=s.world.pregame->archive.integer_arrays.end()&&mapping->second.size()==64,"Grand military native CvPreGame team_types64 inventory unknown");const auto &player_teams=mapping->second;
 need(trade.native().player_id==player&&military.native().player_id==player&&trade.after_world_revision()==bnw_player_do_turn_revolts_revision(s,native,db,id,player)&&military.after_world_revision()==bnw_player_military_registry_revision(s,player),"Grand military requires SAME accepted Trade full-world and Military component seals");
 const auto input=bnw_grand_military_inputs_revision(s);need(military.grand_military_input_revision()==input,"Grand military current elapsed/team/query inputs stale after accepted Reset");
 return query_bnw_grand_current_military_average(s,native,db,id,player);
}
}
