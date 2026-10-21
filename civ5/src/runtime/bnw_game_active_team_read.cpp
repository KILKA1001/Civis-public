#include "civ5/runtime/bnw_game_active_team_read.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/runtime/bnw_city_current_growth_threshold.hpp"
#include <type_traits>
#include <climits>
#include <stdexcept>
#include <string_view>
namespace civ5::runtime::detail {
namespace {
void need(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
std::int32_t native(std::int64_t n){need(n>=INT32_MIN&&n<=INT32_MAX,"Game active-team native index unavailable");return static_cast<std::int32_t>(n);}
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
const auto& named(const auto& rows,std::string_view name){
 using T=typename std::remove_cvref_t<decltype(rows)>::value_type;const T* out=nullptr;
 for(const auto& r:rows)if(r.name&&std::string_view(r.name)==name){need(!out,"Game human duplicate raw field");out=&r;}
 need(out,"Game human raw field absent");return *out;
}
const saves::SavedPlayerObservation& human_player(const LiveSessionState& s,std::int32_t id){
 need(id>=0&&id<64&&s.world.all_players&&s.world.all_players->layout==bnw&&
  s.world.all_players->context.player_count==64&&s.world.all_players->context.strategy.civ_players==63&&
  s.world.all_players->players.size()==64,"Current growth actual native player roster unavailable");
 const auto slot=static_cast<std::size_t>(id);const auto& p=s.world.all_players->players[slot];
 const auto& f=named(p.core.fields,"m_eID");
 need(p.index==slot&&p.saved_player_id==id&&p.core.layout==bnw&&f.encoding==saves::PlayerCoreEncoding::i32&&
  f.values.size()==1&&f.values.front()==id&&!f.wire_count&&f.bytes.empty()&&f.hashes.empty()&&f.row_counts.empty(),
  "Current growth actual player ID differs");return p;
}
const saves::PregameObservation& pregame(const LiveSessionState& s){
 need(s.setup.profile==content::TechnologyProfile::bnw&&s.world.game_state.layout==bnw&&s.world.pregame&&
  s.world.pregame->layout==bnw&&s.world.pregame->slots_version==3&&s.world.pregame->archive_version==6,
  "Current growth genuine BNW Pregame unavailable");return *s.world.pregame;
}
std::int32_t preg_scalar(const saves::PregameObservation& p,const char* name){return native(p.archive.integers.at(name));}
std::int32_t preg_slot(const saves::PregameObservation& p,const char* name,std::int32_t id){
 const auto& v=p.archive.integer_arrays.at(name);need(v.size()==64&&id>=0&&id<64,"Current growth Pregame native64 slots unavailable");
 return native(v[static_cast<std::size_t>(id)]);
}
bool pitboss(const LiveSessionState& s,const content::RulesDatabase& db){
 const auto& p=pregame(s);
 if(const auto saved=detail::bnw_growth_saved_pitboss_option(p.game_options))return *saved!=0;
 // GAMEOPTION_PITBOSS is a string #define, not an enum/cache lookup.
 // Native GetGameOption(const char*,int&) reads this DB directly after first
 // saved CustomOption match; it neither inserts a default nor runs Enum Sync.
 std::optional<std::int64_t> value;
 const auto report=db.query_rows({"Select \"Default\" from GameOptions where Type = ? LIMIT 1",
  {std::string("GAMEOPTION_PITBOSS")}},[&](const auto& row){value=row.integer_at(0);},1);
 need(report.all_succeeded(),"Current growth actual string PitBoss default query unavailable");
 // Native Execute/Step absent default returns false from isPitBoss. Query
 // errors conservatively refuse rather than publishing a false gameplay fact.
 return value&&native(*value)!=0;
}

}
bool bnw_player_human_read(const LiveSessionState& s,std::int32_t id,const content::RulesDatabase& db){
 (void)human_player(s,id);const auto& p=pregame(s);const auto status=preg_slot(p,"slot_status",id);
 need(status>=0&&status<=4&&s.setup.players.at(id).slot_status==status,"Current growth raw/owned slot status differs");
 if(status==3)return true;
 if(status==1||status==2)return false;
 if(status==4){const auto claim=preg_slot(p,"slot_claims",id);
  need(claim>=0&&claim<=2&&s.setup.players.at(id).slot_claim==claim,"Current growth raw/owned observer claim differs");return claim==2;}
 const auto type=preg_scalar(p,"game_type");need(type>=0&&type<=3&&s.setup.game_type==type,"Current growth raw/owned Game type differs");
 if(type==2)return true; // Hotseat short-circuits PitBoss.
 return pitboss(s,db)||type==3; // Native PitBoss read precedes play-by-email.
}

std::int32_t bnw_game_active_player_read(const LiveSessionState& state){
 const auto& world=state.world;
 constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
 need(world.game_state.layout==bnw&&world.pregame&&world.pregame->layout==bnw,
  "Lua active team genuine current BNW Pregame absent");
 const auto active=world.pregame->archive.integers.find("active_player");
 need(active!=world.pregame->archive.integers.end(),"Lua active team actual active player absent");
 const auto id=native(active->second);need(id>=-1&&id<64,"Lua active team native player domain unavailable");
 return id;
}
std::int32_t bnw_player_team_read(const LiveSessionState& state,std::int32_t id){
 const auto& world=state.world;
 constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
 need(id>=0&&id<64&&world.pregame&&world.pregame->layout==bnw,
  "Game player-team native Pregame/player unavailable");
 need(world.all_players&&world.all_players->layout==bnw&&world.all_players->context.player_count==64&&
  world.all_players->context.strategy.civ_players==63&&world.all_players->players.size()==64&&
  world.game_state.roster&&world.game_state.roster->dimensions.player_slots==64&&
  world.game_state.roster->dimensions.team_slots==64&&state.setup.profile==content::TechnologyProfile::bnw,
  "Lua active team native player/team source roster unavailable");
 const auto slot=static_cast<std::size_t>(id);const auto& player=world.all_players->players[slot];
 need(player.index==slot&&player.saved_player_id==id&&player.core.layout==bnw,
  "Lua active team actual player identity differs");
 const saves::PlayerCoreField* field=nullptr;
 for(const auto& f:player.core.fields)if(f.name&&std::string_view(f.name)=="m_eID"){
  need(field==nullptr,"Lua active team duplicate native player identity");field=&f;
 }
 need(field&&field->encoding==saves::PlayerCoreEncoding::i32&&field->values.size()==1&&field->values[0]==id,
  "Lua active team actual CvPlayer ID unavailable");
 // CvPlayer::getTeam() is CvPreGame::teamType(m_eID), NOT saved m_eTeam.
 const auto teams=world.pregame->archive.integer_arrays.find("team_types");
 need(teams!=world.pregame->archive.integer_arrays.end()&&teams->second.size()==64,
  "Lua active team native Pregame team assignments unavailable");
 const auto team=native(teams->second[slot]);need(team>=-1&&team<64,"Lua active team native team domain unavailable");
 const auto owned=state.setup.players.find(id);
 need(owned!=state.setup.players.end()&&owned->second.team==team,
  "Lua active team current raw/owned team assignment differs");
 return team;
}
std::int32_t bnw_game_active_team_read(const LiveSessionState& state){
 const auto id=bnw_game_active_player_read(state);
 if(id==-1)return -1; // SDK NO_PLAYER before GET_PLAYER and teamType.
 return bnw_player_team_read(state,id);
}
}
