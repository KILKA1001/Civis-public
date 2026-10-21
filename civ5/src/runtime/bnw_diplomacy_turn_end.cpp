#include "civ5/runtime/bnw_diplomacy_turn_end.hpp"
#include "civ5/runtime/bnw_unit_order_owners.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <climits>
#include <stdexcept>
namespace civ5::runtime {
namespace {
void need(bool v,const char* why){if(!v)throw std::runtime_error(why);}
void number(std::vector<std::uint8_t>& b,std::uint64_t v){for(int i=0;i<8;++i)b.push_back(v>>(8*i));}
void text(std::vector<std::uint8_t>& b,std::string_view v){number(b,v.size());b.insert(b.end(),v.begin(),v.end());}
bool same(const BnwDiplomacyRequestsAfterRead& a,const BnwDiplomacyRequestsAfterRead& b){return a.player==b.player && a.next_ai_player==b.next_ai_player && a.request_active==b.request_active && a.active_from_player==b.active_from_player && a.request_count==b.request_count;}
}
std::string bnw_diplomacy_turn_epoch(const LiveSessionState& s,const SessionCheckpointIdentity& identity,const content::RulesDatabase& rules){
 constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
 need(s.setup.profile==content::TechnologyProfile::bnw && s.world.game_state.layout==bnw && s.world.all_players && s.world.all_players->layout==bnw && s.world.all_players->players.size()==64 && s.world.all_players->context.player_count==64 && s.world.all_players->context.strategy.major==22 && s.world.pregame && s.world.pregame->layout==bnw && s.world.pregame->archive.integers.at("game_turn")==s.setup.game_turn && s.setup.game_turn>=0 && s.setup.game_turn<=INT32_MAX && identity.payload_sha256.size()==64 && identify_bytes(rules.gameplay_fingerprint_data()).sha256==identity.rules_sha256,"Diplomacy turn source/roster/rules/clock unknown");
 need(s.setup.players.size()==64,"Diplomacy owned native roster unknown");
 for(std::size_t id=0;id<64;++id){const auto& raw=s.world.all_players->players.at(id);const saves::PlayerCoreField* actual=nullptr;for(const auto& f:raw.core.fields)if(f.name && std::string_view(f.name)=="m_eID"){need(!actual,"Duplicate diplomacy source player ID");actual=&f;}need(raw.index==id && raw.saved_player_id==static_cast<std::int64_t>(id) && raw.core.layout==bnw && actual && actual->encoding==saves::PlayerCoreEncoding::i32 && actual->values==std::vector<std::int64_t>{static_cast<std::int64_t>(id)} && s.setup.players.contains(id),"Diplomacy native GetPlayer/GetID roster unknown");}
 const auto& tail=s.player_read_tail;need(tail.current && tail.requests.size()==22,"Diplomacy actual retained Init/Read lifecycle unknown");
 std::vector<std::uint8_t> b;text(b,identity.payload_sha256);text(b,identity.rules_sha256);b.insert(b.end(),s.world.map_header.guid.begin(),s.world.map_header.guid.end());number(b,s.setup.game_turn);
 for(std::int64_t id=0;id<22;++id){const auto& p=s.world.all_players->players.at(id);need(p.index==static_cast<std::uint64_t>(id) && p.saved_player_id==id && p.core.layout==bnw && tail.requests.contains(id),"Diplomacy GetPlayer/object roster differs");const auto& q=tail.requests.at(id);need(q.player==id && q.next_ai_player>=-1 && q.next_ai_player<64 && !q.request_active && q.active_from_player==-1 && q.request_count==0,"Diplomacy full request/active payload owner unavailable or invalid next player");number(b,id);number(b,q.next_ai_player);number(b,q.request_active);number(b,q.active_from_player);number(b,q.request_count);}
 return identify_bytes(b).sha256;
}
BnwDiplomacyReadReceipt attest_bnw_diplomacy_read_context(const LiveSessionState& s,const saves::SavedSession& source,const SessionCheckpointIdentity& identity,const content::RulesDatabase& rules){
 need(source.source.container==saves::SaveContainerKind::civ5 && identify_bytes(source.source.bytes).sha256==identity.payload_sha256 && source.snapshot.map_header.guid==s.world.map_header.guid,"Diplomacy actual import/Read receipt identity unknown");const auto expected=rebuild_bnw_player_read_tail_transients(s,source,identity);need(expected.current && s.player_read_tail.current && s.player_read_tail.requests.size()==expected.requests.size(),"Diplomacy actual source Read object roster missing");for(const auto& [id,q]:expected.requests)need(s.player_read_tail.requests.contains(id) && same(q,s.player_read_tail.requests.at(id)),"Diplomacy actual current Read Init/Uninit flags differ");return BnwDiplomacyReadReceipt(identity,s.world.map_header.guid,bnw_unit_order_world_revision(s,identity,rules),bnw_diplomacy_turn_epoch(s,identity,rules));
}
BnwDiplomacyTurnOwner bind_bnw_diplomacy_turn_read(const LiveSessionState& s,BnwDiplomacyReadReceipt&& receipt,const content::RulesDatabase& rules){need(!receipt.used_ && receipt.guid_==s.world.map_header.guid && receipt.world_==bnw_unit_order_world_revision(s,receipt.identity_,rules) && receipt.epoch_==bnw_diplomacy_turn_epoch(s,receipt.identity_,rules),"Diplomacy cold bind missing/used/stale actual Read receipt");receipt.used_=true;return BnwDiplomacyTurnOwner(receipt.identity_,receipt.guid_,receipt.epoch_);}
class BnwDiplomacyTurnOperations {
 public:
 static BnwDiplomacyTurnExecution assign(const LiveSessionState& s,const BnwDiplomacyTurnOwner& owner,const content::RulesDatabase& rules,std::int64_t player,std::int64_t value){BnwDiplomacyTurnExecution out;try{need(player>=0 && player<64 && owner.guid_==s.world.map_header.guid && owner.epoch_==bnw_diplomacy_turn_epoch(s,owner.identity_,rules),"Diplomacy turn owner stale or actor invalid");need(owner.operations_<UINT64_MAX,"Diplomacy operation count exhausted");auto d=std::make_unique<BnwDiplomacyTurnDraft>(BnwDiplomacyTurnDraft{s,owner,player,-1,-1,false});
 if(player<22){auto& actual=d->state.player_read_tail.requests.at(player);d->object_present=true;d->before_next_ai=actual.next_ai_player;actual.next_ai_player=value;d->after_next_ai=actual.next_ai_player;}
 // Source GetDiplomacyRequests pointer is null for the other native slots at
 // the original Read frontier. Presence comes from the sealed roster, not UI.
 ++d->owner.operations_;d->owner.epoch_=bnw_diplomacy_turn_epoch(d->state,owner.identity_,rules);out.draft=std::move(d);
 }catch(const std::exception& e){out.blocker=e.what();}return out;}
};
BnwDiplomacyTurnExecution execute_bnw_diplomacy_end_turn_draft(const LiveSessionState& s,const BnwDiplomacyTurnOwner& o,const content::RulesDatabase& r,std::int64_t p){return BnwDiplomacyTurnOperations::assign(s,o,r,p,-1);}
BnwDiplomacyTurnExecution execute_bnw_diplomacy_begin_turn_scalar_draft(const LiveSessionState& s,const BnwDiplomacyTurnOwner& o,const content::RulesDatabase& r,std::int64_t p){return BnwDiplomacyTurnOperations::assign(s,o,r,p,0);}
} // namespace civ5::runtime
