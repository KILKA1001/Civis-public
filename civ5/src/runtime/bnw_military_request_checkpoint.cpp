#include "civ5/runtime/bnw_military_request_checkpoint.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/runtime/bnw_player_strategy_checkpoint.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <array>
#include <span>
#include <stdexcept>
namespace civ5::runtime {
namespace {
constexpr std::array<std::uint8_t,8> magic{'C','I','V','B','M','R','Q','1'};
constexpr std::size_t header_size=8+4+4*64+4;
void need(bool ok,const char*why){if(!ok)throw std::runtime_error(why);}
void sha(std::string_view s){need(s.size()==64&&std::all_of(s.begin(),s.end(),[](char c){return (c>='0'&&c<='9')||(c>='a'&&c<='f');}),"Military checkpoint malformed SHA256");}
struct Writer{
 std::vector<std::uint8_t> b;
 void u32(std::uint32_t v){for(unsigned i=0;i<4;++i)b.push_back(static_cast<std::uint8_t>(v>>(8*i)));}
 void hash(std::string_view s){sha(s);b.insert(b.end(),s.begin(),s.end());}
};
struct Reader{
 std::span<const std::uint8_t>b;std::size_t at=8;
 std::uint32_t u32(){need(at<=b.size()&&b.size()-at>=4,"Military checkpoint truncated");std::uint32_t v=0;for(unsigned i=0;i<4;++i)v|=std::uint32_t(b[at++])<<(8*i);return v;}
 std::string hash(){need(at<=b.size()&&b.size()-at>=64,"Military checkpoint truncated hash");std::string s(reinterpret_cast<const char*>(b.data()+at),64);at+=64;sha(s);return s;}
};
struct Decoded{std::string payload,rules,source_strategy,current_strategy;std::vector<std::uint32_t>players;};
Decoded decode(const saves::CivisSaveSection&s){
 need(s.id==44&&s.flags==0&&s.bytes.size()>=header_size&&s.bytes.size()<=header_size+22*12&&std::equal(magic.begin(),magic.end(),s.bytes.begin()),"Military checkpoint unsupported section");
 Reader in{s.bytes};need(in.u32()==1,"Military checkpoint unsupported version");Decoded d{in.hash(),in.hash(),in.hash(),in.hash(),{}};
 const auto n=in.u32();need(n>0&&n<=22&&s.bytes.size()==header_size+static_cast<std::size_t>(n)*12,"Military checkpoint sparse count/extent invalid");
 for(std::uint32_t i=0;i<n;++i){const auto player=in.u32();need(player<22&&(d.players.empty()||player>d.players.back()),"Military checkpoint duplicate/foreign player");need(in.u32()==0&&in.u32()==0,"Military checkpoint unsupported nonzero request history");d.players.push_back(player);}
 need(in.at==s.bytes.size(),"Military checkpoint trailing bytes");return d;
}
std::string strategy_hash(const saves::SavedSessionSnapshot&w,const saves::SavedSessionSnapshot&s,const SessionCheckpointIdentity&id){return identify_bytes(make_bnw_player_strategy_checkpoint(w,s,id,true).bytes).sha256;}
void paired_army(const LiveSessionState&s,std::int64_t player){
 need(s.setup.profile==content::TechnologyProfile::bnw&&s.world.all_players&&s.world.all_players->context.strategy.major==22&&s.world.all_players->players.size()==64,"Military checkpoint requires complete BNW player roster");
 need(player>=0&&player<22,"Military checkpoint actor outside major roster");
 const auto&saved=s.world.all_players->players.at(static_cast<std::size_t>(player));need(saved.saved_player_id==player,"Military checkpoint physical actor alias");
 const auto owner=s.setup.players.find(player);need(owner!=s.setup.players.end()&&owner->second.alive,"Military checkpoint dead/unrestored actor");
 const saves::PlayerCoreField*alive=nullptr;for(const auto&f:saved.core.fields)if(f.name&&std::string_view(f.name)=="m_bAlive"){need(!alive,"Military checkpoint duplicate alive scalar");alive=&f;}
 need(alive&&alive->values.size()==1&&alive->values.front()==1,"Military checkpoint raw/owned alive mismatch");
 const saves::PlayerStrategyBlock*m=nullptr;for(const auto&b:saved.strategy.blocks)if(b.name&&std::string_view(b.name)=="military_ai"){need(!m,"Military checkpoint duplicate military block");m=&b;}need(m,"Military checkpoint missing military block");
 const saves::NamedSaveInteger*army=nullptr;for(const auto&f:m->fields)if(f.name&&std::string_view(f.name)=="army_type_building"){need(!army,"Military checkpoint duplicate army scalar");army=&f;}
 need(army&&army->value==-1,"Military checkpoint ID23 Reset army companion mismatch");
}
void boundary(const LiveSessionState&s){need(!s.player_early_publication&&!s.player_gold_publication&&!s.player_research_publication&&!s.game_clock.pending,"Military checkpoint pending player/clock effects cannot commit");}
}
void validate_bnw_military_request_checkpoint_shape(const saves::CivisSaveSection&s){(void)decode(s);}
void require_bnw_military_request_checkpoint_history_retained(const saves::CivisSaveSection&section,const LiveSessionState&s,const saves::SavedSessionSnapshot&source,const SessionCheckpointIdentity&id){
 const auto d=decode(section);sha(id.payload_sha256);sha(id.rules_sha256);
 need(d.payload==id.payload_sha256&&d.rules==id.rules_sha256&&d.source_strategy==strategy_hash(source,source,id),"Military checkpoint re-save history source mismatch");
 for(const auto player:d.players){const auto c=s.military_request_counters.find(static_cast<std::int64_t>(player));need(c!=s.military_request_counters.end()&&c->second.land==0&&c->second.naval==0,"Military checkpoint re-save discarded earned state");}
}
saves::CivisSaveSection make_bnw_military_request_checkpoint(const LiveSessionState&s,const saves::SavedSessionSnapshot&source,const SessionCheckpointIdentity&id){
 sha(id.payload_sha256);sha(id.rules_sha256);boundary(s);need(!s.military_request_counters.empty()&&s.military_request_counters.size()<=22,"Military checkpoint has no earned sparse state");
 Writer out;out.b.insert(out.b.end(),magic.begin(),magic.end());out.u32(1);out.hash(id.payload_sha256);out.hash(id.rules_sha256);out.hash(strategy_hash(source,source,id));out.hash(strategy_hash(s.world,source,id));out.u32(static_cast<std::uint32_t>(s.military_request_counters.size()));
 for(const auto&[player,c]:s.military_request_counters){paired_army(s,player);need(c.land==0&&c.naval==0,"Military checkpoint nonzero requests need another producer codec");out.u32(static_cast<std::uint32_t>(player));out.u32(0);out.u32(0);}
 saves::CivisSaveSection result{44,0,std::move(out.b)};validate_bnw_military_request_checkpoint_shape(result);return result;
}
void restore_bnw_military_request_checkpoint(const saves::CivisSaveSection&section,LiveSessionState&s,const saves::SavedSessionSnapshot&source,const SessionCheckpointIdentity&id){
 sha(id.payload_sha256);sha(id.rules_sha256);boundary(s);need(s.military_request_counters.empty(),"Military checkpoint destination already has earned state");const auto d=decode(section);
 need(d.payload==id.payload_sha256&&d.rules==id.rules_sha256,"Military checkpoint source/rules mismatch");need(d.source_strategy==strategy_hash(source,source,id)&&d.current_strategy==strategy_hash(s.world,source,id),"Military checkpoint immutable roster/current ID23 mismatch");
 decltype(s.military_request_counters) staged;for(const auto player:d.players){paired_army(s,player);staged.emplace(static_cast<std::int64_t>(player),LiveSessionState::BnwMilitaryRequestCounters{0,0});}
 s.military_request_counters.swap(staged); // All allocation/validation precedes publication.
}
} // namespace civ5::runtime
