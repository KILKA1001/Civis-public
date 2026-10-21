#include "civ5/runtime/bnw_game_deals_turn_prefix.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/bnw_game_deals_checkpoint.hpp"
#include "civ5/runtime/bnw_player_core_checkpoint.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <climits>
#include <functional>
#include <map>
#include <string_view>
#include <thread>
#include <type_traits>
#include <utility>
namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
[[noreturn]] void refuse(BnwDealPrefixError c,const char* why){throw BnwDealPrefixRefusal(c,why);}
void need(bool b,const char* why){if(!b)refuse(BnwDealPrefixError::invalid_source,why);}
std::int32_t native(std::int64_t n){if(n<INT32_MIN||n>INT32_MAX)refuse(BnwDealPrefixError::native_overflow,"Deal reached native int32 overflow");return static_cast<std::int32_t>(n);}
auto& named(auto& rows,std::string_view name){
 using T=std::remove_reference_t<decltype(rows.front())>;T* found=nullptr;
 for(auto& x:rows)if(x.name&&name==x.name){need(!found,"Deal duplicate raw field");found=&x;}
 need(found,"Deal reached raw field absent");return *found;
}
std::int32_t core(const saves::SavedPlayerObservation& p,const char* name){
 const auto& f=named(p.core.fields,name);
 need(p.core.layout==bnw&&f.encoding==saves::PlayerCoreEncoding::i32&&f.values.size()==1&&!f.wire_count&&f.hashes.empty()&&f.bytes.empty()&&f.row_counts.empty(),"Deal player scalar schema differs");return native(f.values.front());
}
auto& player(LiveSessionState& s,std::int32_t id){
 need(id>=0&&id<64&&s.world.all_players&&s.world.all_players->layout==bnw&&s.world.all_players->context.player_count==64&&s.world.all_players->players.size()==64,"Deal native64 player roster unavailable");
 auto& p=s.world.all_players->players.at(static_cast<std::size_t>(id));
 need(p.index==static_cast<std::uint64_t>(id)&&p.saved_player_id==id&&core(p,"m_eID")==id,"Deal actual player identity differs");return p;
}
void team(LiveSessionState& s,std::int32_t id){
 (void)player(s,id);
 // CvPlayer::getTeam is current CvPreGame::teamType, not a PlayerCore field.
 need(s.world.pregame&&s.world.pregame->layout==bnw,"Deal actual Pregame team source absent");
 const auto raw=s.world.pregame->archive.integer_arrays.find("team_types");
 need(raw!=s.world.pregame->archive.integer_arrays.end()&&raw->second.size()==64,"Deal native Pregame team vector absent");
 const auto t=native(raw->second.at(static_cast<std::size_t>(id)));
 const auto it=s.setup.players.find(id);
 need(t>=0&&t<64&&it!=s.setup.players.end()&&it->second.team==t,"Deal raw/owned actual player team differs");
}
std::int32_t turn(const LiveSessionState& s){
 need(s.world.pregame&&s.world.pregame->layout==bnw,"Deal actual GAME turn source unavailable");
 const auto it=s.world.pregame->archive.integers.find("game_turn");
 need(it!=s.world.pregame->archive.integers.end(),"Deal raw GAME turn absent");
 const auto n=native(it->second);need(s.setup.game_turn==n,"Deal raw/owned GAME turn differs");return n;
}
void frontier(BnwDealPrefixObservation& r,BnwDealPrefixBoundary b,const char* effect){r.boundary=b;r.required_effect=effect;}
void diplomacy(LiveSessionState& s,BnwDealPrefixObservation& r,std::int32_t id,std::int32_t change){
 auto& p=player(s,id);need(p.administration.layout==bnw,"Deal Treasury native layout absent");
 auto& tr=named(p.administration.blocks,"treasury");
 auto& raw=named(tr.fields,"m_iGoldPerTurnFromDiplomacy");
 const auto version=named(tr.fields,"version").value;
 need(version>=0&&version<=1,"Deal reached Treasury version unsupported");
 auto it=s.maintenance.players.find(id);need(s.maintenance.profile==content::TechnologyProfile::bnw&&it!=s.maintenance.players.end(),"Deal owned diplomacy scalar absent");
 const auto before=native(raw.value);need(it->second.diplomacy_gold==before,"Deal raw/owned diplomacy scalar differs");
 const auto after=native(std::int64_t(before)+change);
 r.stores.push_back({BnwDealPrefixStoreKind::diplomacy_gold,r.deal_index,r.item_index,id,-1,before,after});
 // Real CvTreasury Get + native ADD + Set. The owned economy consumer must
 // see the same value; no gold-balance/DoGold/notification effect occurs here.
 raw.value=after;it->second.diplomacy_gold=after;
}
void resource_export(LiveSessionState& s,const content::RulesDatabase& database,const content::InfoTypeCatalog& types,
 BnwDealPrefixObservation& r,std::int32_t id,std::int32_t resource,std::int32_t change){
 bool mapped=false;
 const auto collection=database.prefetch_info("Resources",[&](auto slot,auto authored,const auto& row){
  need(authored==static_cast<std::int64_t>(slot)&&slot<=INT32_MAX,"Deal native Resources slot/ID mismatch");
  if(authored==resource){const auto type=row.text("Type");need(type&&!type->empty(),"Deal actual Resource Info Type absent");const auto* info=types.find(*type);need(info&&info->table=="Resources"&&info->id==resource,"Deal selected native Resource row/catalog differs");mapped=true;}return true;
 });
 need(collection.all_succeeded()&&!collection.overlapping_ids&&mapped&&resource>=0&&static_cast<std::uint64_t>(resource)<collection.slots,"Deal reached native Resource allocation/Info unavailable");
 auto& p=player(s,id);auto& f=named(p.core.fields,"m_paiResourceExport");
 need(resource>=0&&f.encoding==saves::PlayerCoreEncoding::hashed_i32&&f.wire_count&&*f.wire_count==f.hashes.size()&&f.bytes.empty()&&f.row_counts.empty(),"Deal reached export hash schema differs");
 // Native Read consumes each nonzero payload, ignores unknown Type hashes,
 // and SETs known duplicates in serialized order. Require explicit coverage
 // for the reached slot: no absent-hash zero/Reset authority is inferred.
 std::size_t at=0;std::int64_t* last=nullptr;
 for(const auto h:f.hashes){need(h<=UINT32_MAX,"Deal resource hash width differs");if(!h)continue;need(at<f.values.size(),"Deal resource hash lacks payload");
  auto& value=f.values.at(at++);(void)native(value);const auto* info=types.find_hash(h);if(!info)continue;
  need(info->table=="Resources"&&info->id>=0,"Deal known resource hash maps to foreign Info");
  const auto* byname=types.find(info->type);
  need(byname&&byname->table==info->table&&byname->id==info->id&&byname->type==info->type&&content::saved_info_type_hash(info->type)==h,"Deal actual resource Type/hash mapping differs");
  if(info->id==resource)last=&value;
 }
 need(at==f.values.size()&&last,"Deal reached resource export slot not restored explicitly");
 const auto before=native(*last),after=native(std::int64_t(before)+change);
 r.stores.push_back({BnwDealPrefixStoreKind::resource_export,r.deal_index,r.item_index,id,resource,before,after});
 *last=after; // Last-wins skeleton preserved; unknown/null/earlier rows intact.
 // This reached raw slot is the owned detached export storage. Other derived
 // resource/happiness consumers remain invalid until DoUpdateHappiness joins.
}
bool end_item(LiveSessionState& s,const content::RulesDatabase& database,const content::InfoTypeCatalog& types,
 saves::DealObservation& d,saves::TradedItemObservation& i,BnwDealPrefixObservation& r){
 const auto from=native(i.from_player);need(from==d.from_player||from==d.to_player,"Deal expired item is not in its actual deal");
 const auto to=native(d.from_player!=from?d.from_player:d.to_player);
 r.last_from=from;r.last_to=to;
 // SDK resolves BOTH players/teams BEFORE toRenewed=false/fromRenewed.
 team(s,from);team(s,to);
 need(i.to_renewed.has_value()&&i.from_renewed.has_value(),"Deal renewal Read fields unavailable");
 r.stores.push_back({BnwDealPrefixStoreKind::to_renewed_false,r.deal_index,r.item_index,from,-1,*i.to_renewed?1:0,0});
 i.to_renewed=false;
 if(*i.from_renewed)return true;
 const auto type=native(i.type);
 if(type==1){
  const auto amount=native(i.data[0]);diplomacy(s,r,from,amount);
  // Unary native - is reached AFTER the FROM treasury store.
  diplomacy(s,r,to,native(-std::int64_t(amount)));
  frontier(r,BnwDealPrefixBoundary::notifications_required,"GPT from GetNotifications -> localized message/summary -> Add; then to notifications");return false;
 }
 if(type==3){
  const auto resource=native(i.data[0]);const auto quantity=native(i.data[1]);
  const auto change=native(-std::int64_t(quantity));
  if(change!=0){resource_export(s,database,types,r,from,resource,change);frontier(r,BnwDealPrefixBoundary::resource_happiness_required,"FROM changeResourceExport -> DoUpdateHappiness BEFORE TO changeResourceImport");return false;}
  // Both native resource setters are actual conditional no-ops at change0.
  frontier(r,BnwDealPrefixBoundary::resource_description_notifications_required,"GC.ResourceInfo DescriptionKey then actual from/to notifications");return false;
 }
 if(type==6||type==7||type==8||type==9||type==13||type==14){
  frontier(r,BnwDealPrefixBoundary::team_effect_required,"Actual open-borders/pact/research/trade/force-peace setter and all subsequent effects required");return false;
 }
 if(type==19){frontier(r,BnwDealPrefixBoundary::league_vote_effect_required,"FROM LeagueAI.CancelVoteCommitmentsToPlayer then TO");return false;}
 // EXACT absence of an ending branch in SDK for all other integer types.
 // Not a invented generic skip: enum/unknown values match no else-if body.
 return true;
}
bool section_equal(const saves::CivisSaveSection& a,const saves::CivisSaveSection& b){return a.id==b.id&&a.flags==b.flags&&a.bytes==b.bytes;}
std::string_view text_name(const char* s){return s?std::string_view(s):std::string_view();}
bool same_record(const saves::PlayerAdministrationRecord& a,const saves::PlayerAdministrationRecord& b){
 if(text_name(a.name)!=text_name(b.name)||a.index!=b.index||a.start_offset!=b.start_offset||a.end_offset!=b.end_offset||a.fields.size()!=b.fields.size()||a.arrays.size()!=b.arrays.size()||a.blobs.size()!=b.blobs.size()||a.children.size()!=b.children.size())return false;
 for(std::size_t n=0;n<a.fields.size();++n)if(text_name(a.fields[n].name)!=text_name(b.fields[n].name)||a.fields[n].value!=b.fields[n].value)return false;
 for(std::size_t n=0;n<a.arrays.size();++n)if(text_name(a.arrays[n].name)!=text_name(b.arrays[n].name)||a.arrays[n].columns!=b.arrays[n].columns||a.arrays[n].values!=b.arrays[n].values)return false;
 for(std::size_t n=0;n<a.blobs.size();++n)if(text_name(a.blobs[n].name)!=text_name(b.blobs[n].name)||a.blobs[n].bytes!=b.blobs[n].bytes)return false;
 for(std::size_t n=0;n<a.children.size();++n)if(!same_record(a.children[n],b.children[n]))return false;
 return true;
}
bool same_administration(const saves::PlayerAdministrationObservation& a,const saves::PlayerAdministrationObservation& b){
 if(a.layout!=b.layout||a.start_offset!=b.start_offset||a.end_offset!=b.end_offset||a.blocks.size()!=b.blocks.size())return false;
 for(std::size_t n=0;n<a.blocks.size();++n)if(!same_record(a.blocks[n],b.blocks[n]))return false;
 return true;
}
}
BnwDealPrefixRefusal::BnwDealPrefixRefusal(BnwDealPrefixError c,const std::string& s):std::runtime_error(s),code_(c){}
void detail::run_bnw_deals_detached_prefix(LiveSessionState& s,const content::RulesDatabase& database,const content::InfoTypeCatalog& types,BnwDealPrefixObservation& r){
 need(!r.attempted,"Detached deal prefix cannot replay its existing journal");r.attempted=true;
 need(s.setup.profile==content::TechnologyProfile::bnw&&s.world.game_state.layout==bnw&&s.world.history.layout==bnw&&s.world.history.deals.version==1&&!s.world.history.deals.next_deal_id,"Deal BNW actual history shape differs");
 auto& current=s.world.history.deals.groups[1];
 if(current.empty()){r.boundary=BnwDealPrefixBoundary::empty_returned;return;}
 const auto game_turn=turn(s);r.boundary=BnwDealPrefixBoundary::expiry_scan;
 for(std::size_t d=0;d<current.size();++d)for(std::size_t i=0;i<current[d].items.size();++i){
  r.deal_index=d;r.item_index=i;auto& item=current[d].items[i];const auto final=native(item.final_turn);
  if(final>-1&&final==game_turn){r.something_changed=true;if(!end_item(s,database,types,current[d],item,r))return;}
 }
 r.boundary=BnwDealPrefixBoundary::validity_scan;
 for(std::size_t d=0;d<current.size();++d)for(std::size_t i=0;i<current[d].items.size();++i){
  r.deal_index=d;r.item_index=i;auto& item=current[d].items[i];const auto final=native(item.final_turn);
  if(final>-1&&final!=game_turn){
   r.last_from=native(item.from_player);
   if(item.type==3){frontier(r,BnwDealPrefixBoundary::resource_total_required,"Actual FROM getNumResourceTotal (strategic/mod/import/minor/siphon/export ordered getters) >=0 before cancellation");return;}
   if(item.type==13)break; // Native unbreakable peace skips later resource checks.
  }
 }
 if(r.something_changed){frontier(r,BnwDealPrefixBoundary::active_player_ui_required,"Actual GAME active player getter/predicate then optional GameData dirty publication BEFORE list migration");return;}
 // DoUpdateCurrentDealsList: prepare full copies before private replacement.
 auto next=s.world.history.deals;next.groups[1].clear();next.groups[1].reserve(current.size());
 for(const auto& d:current){if(native(d.final_turn)<=game_turn){next.groups[2].push_back(d);++r.migrated;}else next.groups[1].push_back(d);}
 s.world.history.deals=std::move(next);r.boundary=BnwDealPrefixBoundary::list_migration_returned;
}
struct BnwGameDealsTurnPrefix::Impl {
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host;
 std::shared_ptr<const LiveSessionState> state;
 const saves::SavedSession* source=nullptr;
 SessionCheckpointIdentity identity;std::uint64_t generation=0;
 std::thread::id thread=std::this_thread::get_id();std::function<bool()> rules_current;
 saves::CivisSaveSection deals,core;
 // ID36 export deliberately rejects unchanged trees. Retain ordinary deep
 // observations for beforeimage equality, never fabricate a changed codec tree.
 std::vector<saves::PlayerAdministrationObservation> administration;
 std::map<std::string,std::int64_t> pregame_integers;
 std::map<std::string,std::vector<std::int64_t>> pregame_arrays;
 std::int64_t setup_turn=0;
 std::map<std::int64_t,std::int64_t> diplomacy_before,teams_before;
 std::unique_ptr<LiveSessionState> detached;BnwDealPrefixObservation result;
 bool retired=false;
};
BnwGameDealsTurnPrefix::BnwGameDealsTurnPrefix(std::unique_ptr<Impl> p):impl_(std::move(p)){}
BnwGameDealsTurnPrefix::~BnwGameDealsTurnPrefix()=default;
void BnwGameDealsTurnPrefix::require_current()const{
 const auto& p=impl_;if(!p||!p->host)refuse(BnwDealPrefixError::missing_parent,"Deal actual selected parent missing");
 if(p->thread!=std::this_thread::get_id())refuse(BnwDealPrefixError::wrong_thread,"Deal owner thread fence");
 if(p->retired)refuse(BnwDealPrefixError::stale_source,"Deal owner retired");
 try{
  const auto& session=p->host->session();
  need(session.read_state_lease()==p->state&&session.state_generation()==p->generation&& &session.source()==p->source,"Deal actual state/generation/source retired");
  (void)session.bnw_game_debug_mode();
  need(p->rules_current&&p->rules_current()&&p->host->database().gameplay_fingerprint_sha256()==p->identity.rules_sha256&&p->host->identity().payload_sha256==p->identity.payload_sha256,"Deal actual selected rules/identity retired");
  need(identify_bytes(p->source->source.bytes).sha256==p->identity.payload_sha256,"Deal original source bytes changed");
  need(section_equal(p->deals,make_bnw_game_deals_checkpoint(p->state->world,p->source->snapshot,p->identity))&&section_equal(p->core,make_bnw_player_core_checkpoint(p->state->world,p->source->snapshot,p->identity,true)),"Deal actual whole beforeimage changed in same generation");
  need(p->state->world.pregame&&p->state->world.pregame->archive.integers==p->pregame_integers&&p->state->world.pregame->archive.integer_arrays==p->pregame_arrays&&p->state->setup.game_turn==p->setup_turn,"Deal actual Pregame/owned clock or team source changed");
  need(p->state->world.all_players->players.size()==p->administration.size(),"Deal administration roster changed");
  for(std::size_t i=0;i<p->administration.size();++i)need(same_administration(p->administration[i],p->state->world.all_players->players[i].administration),"Deal actual administration beforeimage changed");
  need(p->state->setup.profile==content::TechnologyProfile::bnw&&p->state->maintenance.profile==content::TechnologyProfile::bnw&&p->state->maintenance.players.size()==p->diplomacy_before.size()&&p->state->setup.players.size()==p->teams_before.size(),"Deal actual owned profile/roster changed");
  for(const auto& [id,before]:p->teams_before)need(p->state->setup.players.at(id).team==before,"Deal owned team mirror beforeimage changed");
  for(const auto& [id,before]:p->diplomacy_before)need(p->state->maintenance.players.at(id).diplomacy_gold==before,"Deal owned treasury mirror beforeimage changed");
 }catch(...){p->retired=true;throw BnwDealPrefixRefusal(BnwDealPrefixError::stale_source,"Deal current source/owned beforeimage retired");}
}
std::shared_ptr<BnwGameDealsTurnPrefix> BnwGameDealsTurnPrefix::from_selected_host(std::shared_ptr<const ui::SelectedBnwPreviewHost> host){
 if(!host)refuse(BnwDealPrefixError::missing_parent,"Deal requires actual selected Host");
 (void)host->session().bnw_game_debug_mode();
 auto p=std::make_unique<Impl>();p->host=std::move(host);const auto& session=p->host->session();
 p->state=session.read_state_lease();p->generation=session.state_generation();p->source=&session.source();p->identity=p->host->identity();
 p->rules_current=p->host->database().immutable_preview_fingerprint_currentness();
 need(p->state&&p->rules_current&&p->rules_current()&&p->host->types().report().all_succeeded(),"Deal genuine immutable rules/state unavailable");
 p->deals=make_bnw_game_deals_checkpoint(p->state->world,p->source->snapshot,p->identity);
 p->core=make_bnw_player_core_checkpoint(p->state->world,p->source->snapshot,p->identity,true);
 need(p->state->world.pregame&&p->state->world.pregame->layout==bnw,"Deal genuine Pregame source absent");
 for(const auto& player:p->state->world.all_players->players)p->administration.push_back(player.administration);
 p->pregame_integers=p->state->world.pregame->archive.integers;p->pregame_arrays=p->state->world.pregame->archive.integer_arrays;p->setup_turn=p->state->setup.game_turn;
 for(const auto& [id,value]:p->state->maintenance.players)p->diplomacy_before.emplace(id,value.diplomacy_gold);
 for(const auto& [id,value]:p->state->setup.players)p->teams_before.emplace(id,value.team);
 auto owner=std::shared_ptr<BnwGameDealsTurnPrefix>(new BnwGameDealsTurnPrefix(std::move(p)));owner->require_current();return owner;
}
void BnwGameDealsTurnPrefix::advance(){
 require_current();auto& p=*impl_;if(p.result.attempted)refuse(BnwDealPrefixError::already_attempted,"Deal actual detached operation already attempted");
 p.detached=std::make_unique<LiveSessionState>(*p.state);require_current();
 try{detail::run_bnw_deals_detached_prefix(*p.detached,p.host->database(),p.host->types(),p.result);require_current();}
 catch(...){p.result.boundary=BnwDealPrefixBoundary::failed_source;throw;}
}
BnwDealPrefixObservation BnwGameDealsTurnPrefix::observe()const{require_current();return impl_->result;}
std::shared_ptr<const LiveSessionState> BnwGameDealsTurnPrefix::read_detached_state()const{
 require_current();need(bool(impl_->detached),"Deal detached copy not prepared");
 auto copy=std::make_shared<const LiveSessionState>(*impl_->detached);require_current();return copy;
}
}
