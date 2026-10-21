#include "civ5/runtime/bnw_city_current_simple_strategy.hpp"
#include "civ5/runtime/bnw_city_registry.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <atomic>
#include <climits>
#include <functional>
#include <string_view>
#include <thread>
#include <type_traits>
#include <variant>
namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void need(bool b,const char* m){if(!b)throw BnwCitySimpleStrategyUnavailable(m);}
std::int32_t native(std::int64_t n){need(n>=INT32_MIN&&n<=INT32_MAX,"Simple city strategy reached signed32 operand unavailable");return static_cast<std::int32_t>(n);}
template<class F>auto typed(F&& f)->decltype(f()){
 try{return f();}catch(const BnwCitySimpleStrategyUnavailable&){throw;}
 catch(const std::exception& e){throw BnwCitySimpleStrategyUnavailable(e.what());}
}
const auto& named(const auto& rows,std::string_view name){
 using T=typename std::remove_cvref_t<decltype(rows)>::value_type;const T* out=nullptr;
 for(const auto& r:rows)if(r.name&&std::string_view(r.name)==name){need(!out,"Simple city strategy duplicate source field");out=&r;}
 need(out,"Simple city strategy required source field absent");return *out;
}
std::int32_t city_scalar(const saves::SavedCityObservation& c,std::string_view name){
 const auto& f=named(c.core.fields,name);
 need(c.core.layout==bnw&&f.encoding==saves::CityCoreEncoding::i32&&f.values.size()==1&&!f.wire_count&&
  f.bytes.empty()&&f.hashes.empty()&&f.row_keys.empty()&&f.row_counts.empty(),"Simple city strategy city native scalar shape unavailable");
 return native(f.values.front());
}
bool city_puppet(const saves::SavedCityObservation& c){
 const auto& f=named(c.core.fields,"m_bPuppet");
 need(c.core.layout==bnw&&f.encoding==saves::CityCoreEncoding::boolean&&f.values.size()==1&&!f.wire_count&&
  f.bytes.empty()&&f.hashes.empty()&&f.row_keys.empty()&&f.row_counts.empty()&&
  (f.values.front()==0||f.values.front()==1),"Simple city strategy native puppet bool unavailable");
 return f.values.front()!=0;
}
template<class Pop,class Threshold,class Capital,class Puppet>
bool evaluate(BnwSimpleCityStrategy trigger,Pop&& pop,Threshold&& threshold,Capital&& capital,Puppet&& puppet){
 // These are closed local readers; no caller callbacks are retained or used
 // to authenticate a genuine owner. C++ short circuits are the native order.
 switch(trigger){
 case BnwSimpleCityStrategy::tiny:return pop()<threshold(0);
 case BnwSimpleCityStrategy::small:return pop()>=threshold(0)&&pop()<threshold(1);
 case BnwSimpleCityStrategy::medium:
  if(capital())return false;
  return pop()>=threshold(1)&&pop()<threshold(2);
 case BnwSimpleCityStrategy::large:return pop()>=threshold(2);
 case BnwSimpleCityStrategy::is_puppet:return puppet();
 }
 throw BnwCitySimpleStrategyUnavailable("Simple city strategy trigger outside five-source scope");
}
}
bool bnw_city_simple_strategy_math(BnwSimpleCityStrategy trigger,const BnwSimpleCityStrategyMathInputs& i){
 const auto read=[](const std::optional<std::int64_t>& v){need(v.has_value(),"Simple city strategy reached pure operand absent");return native(*v);};
 const auto bit=[&](const std::optional<std::int64_t>& v){const auto n=read(v);need(n==0||n==1,"Simple city strategy reached pure bool invalid");return n!=0;};
 return evaluate(trigger,[&]{return read(i.population);},[&](int k){return read(k==0?i.small_threshold:k==1?i.medium_threshold:i.large_threshold);},
  [&]{return bit(i.capital);},[&]{return bit(i.puppet);});
}
struct BnwCityCurrentSimpleStrategy::Impl {
 const std::thread::id thread=std::this_thread::get_id();
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host;
 std::shared_ptr<const LiveSession> session;
 std::shared_ptr<const content::RulesDatabase> database;
 std::shared_ptr<const LiveSessionState> state;
 const saves::SavedSession* source=nullptr;std::uint64_t generation=0;
 SessionCheckpointIdentity identity;std::function<bool()> rules_current;
 std::int32_t owner=-1,city=-1;
 std::pair<std::uint64_t,std::uint64_t> city_span;
 mutable std::atomic<bool> retired{false};
};
BnwCityCurrentSimpleStrategy::BnwCityCurrentSimpleStrategy(std::shared_ptr<Impl> p)noexcept:impl_(std::move(p)){}
void BnwCityCurrentSimpleStrategy::require_current()const{
 const auto& p=impl_;need(p&&p->thread==std::this_thread::get_id(),"Simple city strategy foreign constructor thread/moved query");
 try{
  need(!p->retired.load()&&p->host&&p->session&&p->database&&p->rules_current&&p->rules_current(),"Simple city strategy genuine rules/host retired");
  need(p->database->gameplay_fingerprint_sha256()==p->identity.rules_sha256&&
   p->host->identity().rules_sha256==p->identity.rules_sha256&&p->host->identity().payload_sha256==p->identity.payload_sha256,
   "Simple city strategy actual rules/source identity differs");
  (void)p->session->bnw_game_debug_mode(); // actual owning thread/lifecycle BEFORE facade state/source
  need(&p->host->session()==p->session.get() && &p->host->database()==p->database.get()&&
   p->session->read_state_lease()==p->state&&p->session->state_generation()==p->generation && &p->session->source()==p->source,
   "Simple city strategy actual allocation/source/generation retired");
 }catch(...){p->retired.store(true);throw;}
}
bool BnwCityCurrentSimpleStrategy::value(BnwSimpleCityStrategy trigger)const{return typed([&]{
 require_current();const auto& p=*impl_;
 try{
  const auto& s=*p.state;const auto* raw=resolve_bnw_city_registry(s.world,s.cities,p.owner,p.city);
  need(raw&&city_scalar(*raw,"m_iID")==p.city&&city_scalar(*raw,"m_eOwner")==p.owner&&
   std::pair{raw->core.start_offset,raw->core.end_offset}==p.city_span,"Simple city strategy canonical city/source span differs");
  const auto& owned=s.cities.players.at(p.owner).cities.at(p.city);
  const auto pop=[&]{require_current();const auto n=city_scalar(*raw,"m_iPopulation");need(owned.population==n,"Simple city strategy raw/owned population differs");return n;};
  const auto threshold=[&](int k){
   require_current();const char* name=k==0?"AI_CITYSTRATEGY_SMALL_CITY_POP_THRESHOLD":k==1?"AI_CITYSTRATEGY_MEDIUM_CITY_POP_THRESHOLD":"AI_CITYSTRATEGY_LARGE_CITY_POP_THRESHOLD";
   const auto* cache=p.host->defines().find(name);
   need(p.host->defines().profile()==content::GlobalDefineProfile::bnw&&cache&&cache->kind==content::GlobalDefineKind::integer&&
    cache->lookup.found&&cache->lookup.error.empty()&&cache->lookup.sqlite_code==0&&std::holds_alternative<std::int64_t>(cache->lookup.value),
    "Simple city strategy reached source Define cache unavailable");
   const auto lookup=p.database->global_define(name,content::GlobalDefineKind::integer);
   require_current();need(lookup.found&&lookup.error.empty()&&lookup.sqlite_code==0&&std::holds_alternative<std::int64_t>(lookup.value)&&
    std::get<std::int64_t>(lookup.value)==std::get<std::int64_t>(cache->lookup.value),"Simple city strategy reached actual cached/DB Define differs");
   return native(std::get<std::int64_t>(cache->lookup.value));
  };
  const auto capital=[&]{
   require_current();need(s.world.all_players&&s.world.all_players->layout==bnw&&s.world.all_players->context.player_count==64&&
    s.world.all_players->players.size()==64,"Simple city strategy capital native player roster unavailable");
   const auto& player=s.world.all_players->players.at(static_cast<std::size_t>(p.owner));
   const auto scalar=[&](const char* name){const auto& f=named(player.core.fields,name);need(player.core.layout==bnw&&f.encoding==saves::PlayerCoreEncoding::i32&&f.values.size()==1&&!f.wire_count&&
    f.bytes.empty()&&f.hashes.empty()&&f.row_counts.empty(),"Simple city strategy capital native player scalar unavailable");return native(f.values.front());};
   need(player.index==static_cast<std::uint64_t>(p.owner)&&player.saved_player_id==p.owner&&scalar("m_eID")==p.owner,"Simple city strategy capital actual player differs");
   const auto id=scalar("m_iCapitalCityID");need(s.cities.players.at(p.owner).capital==id,"Simple city strategy raw/owned capital handle differs");
   return resolve_bnw_city_registry(s.world,s.cities,p.owner,id)==raw; // native getCapitalCity pointer identity, not an index/flag guess
  };
  const auto puppet=[&]{require_current();const bool b=city_puppet(*raw);need(owned.puppet==b,"Simple city strategy raw/owned puppet differs");return b;};
  const auto result=evaluate(trigger,pop,threshold,capital,puppet);require_current();return result;
 }catch(...){p.retired.store(true);throw;}
});}
BnwCityCurrentSimpleStrategy inspect_bnw_city_current_simple_strategy(
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host,std::int64_t owner,std::int64_t city){return typed([&]{
 need(bool(host),"Simple city strategy actual selected host absent");
 (void)host->session().bnw_game_debug_mode(); // actual thread/owner BEFORE DB or world
 auto p=std::make_shared<BnwCityCurrentSimpleStrategy::Impl>();p->host=std::move(host);
 p->session=std::shared_ptr<const LiveSession>(p->host,&p->host->session());
 p->database=std::shared_ptr<const content::RulesDatabase>(p->host,&p->host->database());
 p->rules_current=p->database->immutable_preview_fingerprint_currentness();
 need(p->rules_current&&p->rules_current(),"Simple city strategy actual immutable rules scope absent");
 p->identity=p->host->identity();need(!p->identity.payload_sha256.empty()&&!p->identity.rules_sha256.empty()&&
  p->database->gameplay_fingerprint_sha256()==p->identity.rules_sha256,"Simple city strategy actual host rules identity differs");
 p->state=p->session->read_state_lease();need(bool(p->state),"Simple city strategy actual state absent");
 p->source=&p->session->source();p->generation=p->session->state_generation();
 need(identify_bytes(p->source->source.bytes).sha256==p->identity.payload_sha256,"Simple city strategy actual source payload differs");
 p->owner=native(owner);need(p->owner>=0&&p->owner<64,"Simple city strategy native owner outside BNW roster");
 const auto* raw=resolve_bnw_city_registry(p->state->world,p->state->cities,p->owner,native(city));
 need(raw,"Simple city strategy actual current city absent");p->city=city_scalar(*raw,"m_iID");
 need(city_scalar(*raw,"m_eOwner")==p->owner&&raw->core.start_offset<raw->core.end_offset,"Simple city strategy canonical owner/source span differs");
 p->city_span={raw->core.start_offset,raw->core.end_offset};
 BnwCityCurrentSimpleStrategy result(std::move(p));result.require_current();return result;
});}
}
