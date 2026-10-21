#include "civ5/runtime/bnw_city_strategy_checkpoint_owner.hpp"
#include "civ5/runtime/bnw_city_registry.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <atomic>
#include <climits>
#include <functional>
#include <set>
#include <string_view>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void need(bool v,const char* why){if(!v)throw BnwCityStrategyCheckpointUnavailable(why);}
template<class F>auto typed(F&& f)->decltype(f()){
 try{return f();}catch(const BnwCityStrategyCheckpointUnavailable&){throw;}
 catch(const std::exception& e){throw BnwCityStrategyCheckpointUnavailable(e.what());}
}
const auto& named(const auto& rows,std::string_view name){
 using T=typename std::remove_cvref_t<decltype(rows)>::value_type;const T* out=nullptr;
 for(const auto& row:rows)if(row.name&&name==row.name){need(!out,"Strategy checkpoint duplicate named field");out=&row;}
 need(out,"Strategy checkpoint mandatory field absent");return *out;
}
std::int32_t native(std::int64_t value){need(value>=INT32_MIN&&value<=INT32_MAX,"Strategy checkpoint native int32 width differs");return static_cast<std::int32_t>(value);}
std::int32_t scalar(const saves::CityCoreObservation& c,const char* name){
 const auto& f=named(c.fields,name);
 need(c.layout==bnw&&f.encoding==saves::CityCoreEncoding::i32&&f.values.size()==1&&!f.wire_count&&f.bytes.empty()&&f.hashes.empty()&&f.row_keys.empty()&&f.row_counts.empty(),"Strategy checkpoint city native identity schema differs");return native(f.values.front());
}
using Key=std::tuple<std::int32_t,std::int32_t,std::uint64_t,std::uint64_t>;
std::vector<Key> roster(const LiveSessionState& state,const saves::SavedSessionSnapshot& source){
 const auto& world=state.world;
 need(world.all_players&&source.all_players&&world.all_players->layout==bnw&&source.all_players->layout==bnw&&world.all_players->players.size()==64&&source.all_players->players.size()==64&&world.all_players->context.player_count==64&&source.all_players->context.player_count==64&&world.all_players->context.strategy.civ_players==63&&source.all_players->context.strategy.civ_players==63,"Strategy checkpoint requires SAME complete native64 roster");
 std::vector<Key> keys;
 for(std::size_t p=0;p<64;++p){
  const auto owner=static_cast<std::int32_t>(p);const auto& current=world.all_players->players.at(p);const auto& original=source.all_players->players.at(p);
  need(current.index==p&&original.index==p&&current.saved_player_id==owner&&original.saved_player_id==owner&&current.cities.cities.size()==original.cities.cities.size(),"Strategy checkpoint source/player roster changed");
  std::set<std::int32_t> ids;
  for(std::size_t i=0;i<current.cities.cities.size();++i){
   const auto& city=current.cities.cities.at(i);const auto& old=original.cities.cities.at(i);
   const auto id=scalar(city.core,"m_iID");
   need(id>=0&&ids.insert(id).second&&city.index==i&&old.index==i&&scalar(city.core,"m_eOwner")==owner&&scalar(old.core,"m_eOwner")==owner&&scalar(old.core,"m_iID")==id&&city.core.start_offset==old.core.start_offset&&city.core.end_offset==old.core.end_offset&&city.core.start_offset<city.core.end_offset,"Strategy checkpoint canonical city fullID/coreSpan changed");
   need(resolve_bnw_city_registry(world,state.cities,owner,id)==&city&&resolve_bnw_city_registry(source,state.cities,owner,id)==&old,"Strategy checkpoint genuine current/source FLTA identity differs");
   keys.emplace_back(owner,id,city.core.start_offset,city.core.end_offset);
  }
 }
 return keys;
}
bool same_section(const saves::CivisSaveSection& a,const saves::CivisSaveSection& b){return a.id==b.id&&a.flags==b.flags&&a.bytes==b.bytes;}
void mapped_rows(const saves::SavedSessionSnapshot& world,const content::InfoTypeCatalog& types,std::uint64_t slots){
 for(const auto& p:world.all_players->players)for(const auto& c:p.cities.cities){
  const auto& b=named(c.management.blocks,"strategy");
  for(const auto& a:b.hashes)for(const auto& row:a.rows){
   const auto* info=row.type_hash?types.find_hash(row.type_hash):nullptr;
   if(!info)continue; // Exact unknown-row payload/order retained by ID27.
   need(info->table=="AICityStrategies"&&info->id>=0&&static_cast<std::uint64_t>(info->id)<slots,"Strategy checkpoint known hash maps outside actual strategy Info span");
  }
 }
}
}
void detail::require_bnw_city_strategy_checkpoint_arrays(const saves::SavedSessionSnapshot& world){
 need(world.game_state.layout==bnw&&world.all_players&&world.all_players->layout==bnw&&world.all_players->context.player_count==64&&world.all_players->players.size()==64,"Strategy checkpoint native64 BNW schema absent");
 for(const auto& p:world.all_players->players)for(const auto& c:p.cities.cities){
  need(c.management.layout==bnw,"Strategy checkpoint management native profile differs");
  const auto& b=named(c.management.blocks,"strategy");
  need(named(b.fields,"version").value==1&&b.hashes.size()==2,"Strategy checkpoint strategy native version/array count differs");
  for(std::size_t i=0;i<2;++i){
   const auto& a=b.hashes.at(i);const auto wanted=i==0?"m_pabUsingCityStrategy":"m_paiTurnCityStrategyAdopted";
   need(a.name&&std::string_view(a.name)==wanted&&a.wire_count>=0&&a.wire_count<=UINT32_MAX&&a.rows.size()<=UINT32_MAX&&a.wire_count==static_cast<std::int64_t>(a.rows.size()),"Strategy checkpoint native uint32 count/name/order differs");
   for(const auto& row:a.rows){
    need(row.type_hash<=UINT32_MAX,"Strategy checkpoint hash native uint32 width differs");
    if(!row.type_hash){need(row.values.empty(),"Strategy checkpoint null hash has a payload");continue;}
    need(row.values.size()==1,"Strategy checkpoint nonzero hash lacks exactly one payload");
    if(i==0)need(row.values.front()==0||row.values.front()==1,"Strategy checkpoint native using bool is not0/1");
    else (void)native(row.values.front());
   }
  }
 }
}
struct BnwCityStrategyCheckpointOwner::Impl {
 const std::thread::id thread=std::this_thread::get_id();
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host;
 std::shared_ptr<const LiveSession> session;
 std::shared_ptr<const content::RulesDatabase> database;
 std::shared_ptr<const LiveSessionState> state;
 const saves::SavedSession* source=nullptr;std::uint64_t generation=0,slots=0;
 SessionCheckpointIdentity identity;std::function<bool()> rules_current;
 std::vector<Key> keys;saves::CivisSaveSection section;
 mutable std::atomic<bool> retired{false},prepared{false};
};
struct BnwCityStrategyDetachedRestore::Impl {
 BnwCityStrategyCheckpointOwner parent;
 std::shared_ptr<const saves::SavedSessionSnapshot> world;
 Impl(BnwCityStrategyCheckpointOwner p,std::shared_ptr<const saves::SavedSessionSnapshot> s):parent(std::move(p)),world(std::move(s)){}
};
BnwCityStrategyCheckpointOwner::BnwCityStrategyCheckpointOwner(std::shared_ptr<Impl> p)noexcept:impl_(std::move(p)){}
BnwCityStrategyDetachedRestore::BnwCityStrategyDetachedRestore(std::shared_ptr<const Impl> p)noexcept:impl_(std::move(p)){}
void BnwCityStrategyCheckpointOwner::require_current()const{
 const auto& p=impl_;need(p&&p->thread==std::this_thread::get_id(),"Strategy checkpoint foreign constructor thread/moved owner");
 try{
  need(!p->retired.load()&&p->host&&p->session&&p->database&&p->rules_current&&p->rules_current(),"Strategy checkpoint genuine immutable owner retired");
  need(p->database->gameplay_fingerprint_sha256()==p->identity.rules_sha256&&p->host->identity().rules_sha256==p->identity.rules_sha256&&p->host->identity().payload_sha256==p->identity.payload_sha256,"Strategy checkpoint SAME rules/source identity changed");
  (void)p->session->bnw_game_debug_mode(); // Actual Game lease, never a supplied debug flag.
  need(&p->host->session()==p->session.get()&& &p->host->database()==p->database.get()&&p->session->read_state_lease()==p->state&&p->session->state_generation()==p->generation&& &p->session->source()==p->source,"Strategy checkpoint actual allocation/generation/source retired");
  need(identify_bytes(p->source->source.bytes).sha256==p->identity.payload_sha256,"Strategy checkpoint actual source bytes changed");
  detail::require_bnw_city_strategy_checkpoint_arrays(p->state->world);
  detail::require_bnw_city_strategy_checkpoint_arrays(p->source->snapshot);
  mapped_rows(p->state->world,p->host->types(),p->slots);
  mapped_rows(p->source->snapshot,p->host->types(),p->slots);
  need(roster(*p->state,p->source->snapshot)==p->keys,"Strategy checkpoint complete source roster/coreSpan changed");
  need(same_section(make_bnw_city_management_checkpoint(p->state->world,p->source->snapshot,p->identity,true),p->section),"Strategy checkpoint actual management beforeimage changed");
 }catch(...){p->retired.store(true);throw;}
}
saves::CivisSaveSection BnwCityStrategyCheckpointOwner::section()const{return typed([&]{require_current();auto result=impl_->section;require_current();return result;});}
std::shared_ptr<const saves::SavedSessionSnapshot> BnwCityStrategyDetachedRestore::read_world()const{return typed([&]{
 const auto p=impl_;need(bool(p),"Strategy detached restore moved handle");p->parent.require_current();return p->world;
});}
BnwCityStrategyDetachedRestore BnwCityStrategyCheckpointOwner::prepare_detached_restore()const{return typed([&]{
 require_current();const auto p=impl_;need(!p->prepared.load(),"Strategy checkpoint detached restore already prepared once");
 // Existing codec only writes into PRIVATE staging, never the const live lease.
 auto world=std::make_shared<saves::SavedSessionSnapshot>(p->source->snapshot);
 restore_bnw_city_management_checkpoint(p->section,*world,p->source->snapshot,p->identity);
 detail::require_bnw_city_strategy_checkpoint_arrays(*world);
 mapped_rows(*world,p->host->types(),p->slots);
 need(same_section(make_bnw_city_management_checkpoint(*world,p->source->snapshot,p->identity,true),p->section),"Strategy checkpoint detached restore did not reproduce exact captured ID27");
 auto stage=std::make_shared<BnwCityStrategyDetachedRestore::Impl>(*this,std::move(world));
 require_current();need(!p->prepared.exchange(true),"Strategy checkpoint detached restore once slot consumed");
 // No allocation/callback/source mutation follows once publication.
 return BnwCityStrategyDetachedRestore(std::move(stage));
});}
BnwCityStrategyCheckpointOwner capture_bnw_city_strategy_checkpoint(std::shared_ptr<const ui::SelectedBnwPreviewHost> host){return typed([&]{
 need(bool(host),"Strategy checkpoint actual selected Host absent");(void)host->session().bnw_game_debug_mode();
 auto p=std::make_shared<BnwCityStrategyCheckpointOwner::Impl>();p->host=std::move(host);
 p->session=std::shared_ptr<const LiveSession>(p->host,&p->host->session());p->database=std::shared_ptr<const content::RulesDatabase>(p->host,&p->host->database());
 p->rules_current=p->database->immutable_preview_fingerprint_currentness();need(p->rules_current&&p->rules_current(),"Strategy checkpoint requires actual immutable activated DB");p->identity=p->host->identity();
 need(p->identity.payload_sha256.size()==64&&p->identity.rules_sha256.size()==64&&p->database->gameplay_fingerprint_sha256()==p->identity.rules_sha256&&p->host->types().report().all_succeeded(),"Strategy checkpoint actual source/rules identity incomplete");
 p->state=p->session->read_state_lease();p->source=&p->session->source();p->generation=p->session->state_generation();need(bool(p->state)&&identify_bytes(p->source->source.bytes).sha256==p->identity.payload_sha256,"Strategy checkpoint actual source beforeimage identity differs");
 const auto report=p->database->prefetch_info("AICityStrategies",[&](auto slot,auto authored,const auto& row){
  need(slot<=INT32_MAX&&authored==static_cast<std::int64_t>(slot),"Strategy checkpoint actual native strategy Info mapping differs");const auto type=row.text("Type");const auto* info=type?p->host->types().find(*type):nullptr;
  need(info&&info->table=="AICityStrategies"&&info->id==authored,"Strategy checkpoint SAME actual Info Type/slot absent");return true;
 });need(report.all_succeeded()&&report.slots<=INT32_MAX&&!report.overlapping_ids,"Strategy checkpoint actual strategy Info span unavailable");p->slots=report.slots;
 detail::require_bnw_city_strategy_checkpoint_arrays(p->state->world);detail::require_bnw_city_strategy_checkpoint_arrays(p->source->snapshot);
 mapped_rows(p->state->world,p->host->types(),p->slots);mapped_rows(p->source->snapshot,p->host->types(),p->slots);
 p->keys=roster(*p->state,p->source->snapshot);p->section=make_bnw_city_management_checkpoint(p->state->world,p->source->snapshot,p->identity,true);
 BnwCityStrategyCheckpointOwner out(std::move(p));out.require_current();return out;
});}
}
