#include "civ5/runtime/bnw_city_current_capital_strategy.hpp"
#include "civ5/runtime/bnw_city_current_naval_strategy.hpp"
#include "civ5/runtime/bnw_city_registry.hpp"
#include "civ5/runtime/bnw_personality_flavors.hpp"
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
void need(bool b,const char* m){if(!b)throw BnwCityCapitalStrategyUnavailable(m);}
std::int32_t native(std::int64_t n){need(n>=INT32_MIN&&n<=INT32_MAX,"Capital strategy reached signed32 operand unavailable");return static_cast<std::int32_t>(n);}
std::int32_t add(std::int32_t a,std::int32_t b){return native(static_cast<std::int64_t>(a)+b);}
std::int32_t mul(std::int32_t a,std::int32_t b){return native(static_cast<std::int64_t>(a)*b);}
bool bit(std::int64_t n){need(n==0||n==1,"Capital strategy reached native bool invalid");return n!=0;}
template<class F>auto typed(F&& f)->decltype(f()){
 try{return f();}catch(const BnwCityCapitalStrategyUnavailable&){throw;}
 catch(const std::exception& e){throw BnwCityCapitalStrategyUnavailable(e.what());}
}
const auto& named(const auto& rows,std::string_view name){
 using T=typename std::remove_cvref_t<decltype(rows)>::value_type;const T* out=nullptr;
 for(const auto& r:rows)if(r.name&&std::string_view(r.name)==name){need(!out,"Capital strategy duplicate source field");out=&r;}
 need(out,"Capital strategy required source field absent");return *out;
}
std::int32_t city_scalar(const saves::SavedCityObservation& c,std::string_view name){
 const auto& f=named(c.core.fields,name);
 need(c.core.layout==bnw&&f.encoding==saves::CityCoreEncoding::i32&&f.values.size()==1&&!f.wire_count&&f.bytes.empty()&&f.hashes.empty()&&f.row_keys.empty()&&f.row_counts.empty(),"Capital strategy native city scalar shape unavailable");return native(f.values.front());
}
std::int32_t player_scalar(const saves::SavedPlayerObservation& p,std::string_view name){
 const auto& f=named(p.core.fields,name);
 need(p.core.layout==bnw&&f.encoding==saves::PlayerCoreEncoding::i32&&f.values.size()==1&&!f.wire_count&&f.bytes.empty()&&f.hashes.empty()&&f.row_counts.empty(),"Capital strategy native player scalar shape unavailable");return native(f.values.front());
}
const saves::SavedPlayerObservation& player(const LiveSessionState& s,std::int32_t owner){
 need(owner>=0&&owner<64&&s.world.all_players&&s.world.all_players->layout==bnw&&s.world.all_players->context.player_count==64&&s.world.all_players->players.size()==64,"Capital strategy native player registry unavailable");
 const auto& p=s.world.all_players->players.at(static_cast<std::size_t>(owner));need(p.index==static_cast<std::uint64_t>(owner)&&p.saved_player_id==owner&&player_scalar(p,"m_eID")==owner,"Capital strategy actual player identity differs");return p;
}
bool same_info(const content::InfoTypeEntry* a,const content::InfoTypeEntry* b){return a&&b&&a->table==b->table&&a->type==b->type&&a->id==b->id&&a->hash==b->hash;}
template<class Threat,class Population,class Capital,class Percent>
std::optional<std::size_t> most_threatened(std::size_t n,Threat&& threat,Population&& pop,Capital&& capital,Percent&& percent){
 std::int32_t highest=0;std::optional<std::size_t> best;
 for(std::size_t i=0;i<n;++i){const auto original=threat(i);const auto population=pop(i);auto score=mul(original,population);if(capital(i))score=mul(score,percent())/100;if(score>highest){highest=score;best=i;}}
 return best;
}
template<class R>bool evaluate(BnwCapitalCityStrategy t,R& r){
 need(t==BnwCapitalCityStrategy::under_threat||t==BnwCapitalCityStrategy::need_settler,"Capital strategy outside two-source scope");
 if(!r.capital())return false;
 if(t==BnwCapitalCityStrategy::under_threat){if(r.war_count()==0)return false;if(r.minor())return false;return r.most_threatened_is_selected()&&r.threat()>200;}
 if(r.minor())return false;
 const auto cities=r.city_count();const auto settlers=r.settlers();const auto sum=add(cities,settlers);if(sum>=3)return false;
 if(r.under_threat_type()!=-1&&r.game_turn()>50&&r.under_threat_using())return false;
 if(r.mobilization_type()!=-1&&r.mobilization_using())return false;
 const auto modifier=r.threshold_modifier();const auto threshold=add(r.weight_threshold(),modifier);const auto turn=r.game_turn();
 return (sum==1&&mul(turn,4)>threshold)||(sum==2&&mul(turn,2)>threshold)||(sum==3&&turn>threshold);
}
struct SuppliedReader {
 const BnwCapitalCityStrategyMathInputs& p;
 static std::int32_t get(const std::optional<std::int64_t>& v){need(v.has_value(),"Capital strategy reached pure operand absent");return native(*v);}
 bool capital()const{return bit(get(p.capital));}bool minor()const{return bit(get(p.minor));}
 std::int32_t war_count()const{return get(p.war_count);}std::int32_t city_count()const{return get(p.city_count);}std::int32_t settlers()const{return get(p.settlers);}
 std::int32_t under_threat_type()const{return get(p.under_threat_type);}std::int32_t game_turn()const{return get(p.game_turn);}bool under_threat_using()const{return bit(get(p.under_threat_using));}
 std::int32_t mobilization_type()const{return get(p.mobilization_type);}bool mobilization_using()const{return bit(get(p.mobilization_using));}
 std::int32_t weight_threshold()const{return get(p.weight_threshold);}
 std::int32_t threshold_modifier()const{
  need(p.personality&&p.threshold_modifiers&&p.personality->size()==p.threshold_modifiers->size()&&p.personality->size()<=INT32_MAX,"Capital strategy reached pure flavor span absent");std::int32_t sum=0;
  for(std::size_t i=0;i<p.personality->size();++i){const auto personality=native(p.personality->at(i));const auto mod=native(p.threshold_modifiers->at(i));sum=add(sum,mul(personality,mod));}return sum;
 }
 bool most_threatened_is_selected()const{
  need(p.selected_city&&*p.selected_city<p.threat_cities.size(),"Capital strategy reached pure selected city absent");
  const auto result=most_threatened(p.threat_cities.size(),[&](auto i){return get(p.threat_cities.at(i).threat);},[&](auto i){return get(p.threat_cities.at(i).population);},[&](auto i){return bit(get(p.threat_cities.at(i).capital));},[&]{return get(p.capital_threat_percent);});return result==p.selected_city;
 }
 std::int32_t threat()const{need(p.selected_city&&*p.selected_city<p.threat_cities.size(),"Capital strategy selected city absent");return get(p.threat_cities.at(*p.selected_city).threat);}
};
}
bool bnw_capital_city_strategy_math(BnwCapitalCityStrategy t,const BnwCapitalCityStrategyMathInputs& p){SuppliedReader r{p};return evaluate(t,r);}
struct BnwCityCurrentCapitalStrategy::Impl {
 const std::thread::id thread=std::this_thread::get_id();std::shared_ptr<const ui::SelectedBnwPreviewHost> host;
 std::shared_ptr<const LiveSession> session;std::shared_ptr<const content::RulesDatabase> database;std::shared_ptr<const LiveSessionState> state;
 const saves::SavedSession* source=nullptr;std::uint64_t generation=0;SessionCheckpointIdentity identity;std::function<bool()> rules_current;
 std::int32_t owner=-1,city=-1;std::pair<std::uint64_t,std::uint64_t> city_span;mutable std::atomic<bool> retired{false};
};
BnwCityCurrentCapitalStrategy::BnwCityCurrentCapitalStrategy(std::shared_ptr<Impl> p)noexcept:impl_(std::move(p)){}
void BnwCityCurrentCapitalStrategy::require_current()const{
 const auto& p=impl_;need(p&&p->thread==std::this_thread::get_id(),"Capital strategy foreign constructor thread/moved query");
 try{
  need(!p->retired.load()&&p->host&&p->session&&p->database&&p->rules_current&&p->rules_current(),"Capital strategy genuine rules/host retired");
  need(p->database->gameplay_fingerprint_sha256()==p->identity.rules_sha256&&p->host->identity().rules_sha256==p->identity.rules_sha256&&p->host->identity().payload_sha256==p->identity.payload_sha256,"Capital strategy actual rules/source identity differs");
  (void)p->session->bnw_game_debug_mode();
  need(&p->host->session()==p->session.get()&& &p->host->database()==p->database.get()&&p->session->read_state_lease()==p->state&&p->session->state_generation()==p->generation&& &p->session->source()==p->source,"Capital strategy actual allocation/source/generation retired");
 }catch(...){p->retired.store(true);throw;}
}
bool BnwCityCurrentCapitalStrategy::value(BnwCapitalCityStrategy trigger)const{return typed([&]{
 require_current();const auto& p=*impl_;try{
 const auto* raw=resolve_bnw_city_registry(p.state->world,p.state->cities,p.owner,p.city);
 need(raw&&city_scalar(*raw,"m_iID")==p.city&&city_scalar(*raw,"m_eOwner")==p.owner&&std::pair{raw->core.start_offset,raw->core.end_offset}==p.city_span,"Capital strategy canonical city/span differs");
 struct Reader {
  const Impl& p;const saves::SavedCityObservation& city;std::function<void()> guard;
  const LiveSessionState& state()const{guard();return *p.state;}
  const saves::SavedPlayerObservation& owner()const{return player(state(),p.owner);}
  const content::InfoTypeEntry* info(const char* type,const char* table)const{
   guard();const auto* i=p.host->types().find(type);if(!i)return nullptr;need(i->table==table&&i->id>=0&&i->id<=INT32_MAX&&same_info(p.host->types().find_hash(content::saved_info_type_hash(type)),i),"Capital strategy reached actual Info identity differs");return i;
  }
  std::int32_t capital_handle()const{const auto id=player_scalar(owner(),"m_iCapitalCityID");need(state().cities.players.at(p.owner).capital==id,"Capital strategy raw/owned capital differs");return id;}
  bool capital()const{return resolve_bnw_city_registry(state().world,state().cities,p.owner,capital_handle())==&city;}
  bool minor()const{
   const auto& s=state();need(s.world.pregame&&s.world.pregame->layout==bnw,"Capital strategy actual Pregame minor absent");const auto& a=s.world.pregame->archive.flag_arrays.at("minor_nation_civs");need(a.size()==64,"Capital strategy minor native64 flags absent");const bool value=a.at(static_cast<std::size_t>(p.owner));need(s.cities.players.at(p.owner).minor==value,"Capital strategy raw/owned minor differs");return value;
  }
  const saves::TeamObservation& team(std::int32_t id)const{
   const auto& s=state();need(id>=0&&id<64&&s.world.teams&&s.world.teams->layout==bnw&&s.world.teams->dimensions.teams==64&&s.world.teams->dimensions.civ_teams==63&&s.world.teams->teams.size()==64,"Capital strategy actual TEAM registry unavailable");const auto& t=s.world.teams->teams.at(static_cast<std::size_t>(id));need(t.index==static_cast<std::uint64_t>(id)&&native(named(t.fields,"id").value)==id,"Capital strategy actual TEAM identity differs");need(p.source->snapshot.teams&&p.source->snapshot.teams->teams.at(static_cast<std::size_t>(id)).start_offset==t.start_offset&&p.source->snapshot.teams->teams.at(static_cast<std::size_t>(id)).end_offset==t.end_offset,"Capital strategy TEAM source span differs");return t;
  }
  std::int32_t war_count()const{
   const auto& s=state();(void)owner();need(s.world.pregame&&s.world.pregame->layout==bnw,"Capital strategy TEAM Pregame absent");const auto& a=s.world.pregame->archive.integer_arrays.at("team_types");need(a.size()==64,"Capital strategy native Pregame TEAM dimension differs");const auto own=native(a.at(static_cast<std::size_t>(p.owner)));need(own>=0&&own<64&&s.setup.profile==content::TechnologyProfile::bnw&&s.setup.players.at(p.owner).team==own,"Capital strategy native/raw/owned owner TEAM differs");
   std::int32_t count=0;for(std::int32_t i=0;i<63;++i){if(native(named(team(i).fields,"alive_count").value)<=0)continue;const auto& flags=named(team(own).arrays,"at_war");need(flags.columns==1&&flags.values.size()==64,"Capital strategy at-war native64 span unavailable");const bool war=bit(flags.values.at(static_cast<std::size_t>(i)));need(i!=own||!war,"Capital strategy self-war requires real TEAM post-Read repair owner");if(war)count=add(count,1);}return count;
  }
  std::vector<const saves::SavedCityObservation*> cities()const{
   const auto& s=state();const auto& rows=owner().cities.cities;std::vector<const saves::SavedCityObservation*> out;out.reserve(rows.size());std::int32_t previous=-1;
   for(const auto& c:rows){const auto id=city_scalar(c,"m_iID");need((id&8191)>previous&&resolve_bnw_city_registry(s.world,s.cities,p.owner,id)==&c,"Capital strategy native city FLTA order/binding differs");previous=id&8191;out.push_back(&c);}return out;
  }
  std::int32_t city_count()const{
   const auto rows=cities();const auto& h=owner().cities.header;const auto last=native(named(h.fields,"last_index").value),free=native(named(h.fields,"free_list_count").value);need(last>=-1&&last<8192&&free>=0&&free<=8192,"Capital strategy native GetCount operands invalid");const auto n=add(add(last,-free),1);need(n>=0&&static_cast<std::uint64_t>(n)==rows.size(),"Capital strategy native GetCount differs");return n;
  }
  std::int32_t define(const char* name)const{
   guard();const auto* c=p.host->defines().find(name);need(p.host->defines().profile()==content::GlobalDefineProfile::bnw&&c&&c->kind==content::GlobalDefineKind::integer&&c->lookup.found&&c->lookup.error.empty()&&c->lookup.sqlite_code==0&&std::holds_alternative<std::int64_t>(c->lookup.value),"Capital strategy actual native cached Define absent");const auto q=p.database->global_define(name,content::GlobalDefineKind::integer);guard();need(q.found&&q.error.empty()&&q.sqlite_code==0&&q.value==c->lookup.value,"Capital strategy cached/native DB Define differs");return native(std::get<std::int64_t>(c->lookup.value));
  }
  bool most_threatened_is_selected()const{
   const auto rows=cities();const auto& s=state();const auto cap=capital_handle();
   const auto best=most_threatened(rows.size(),[&](auto i){guard();return city_scalar(*rows.at(i),"m_iThreatValue");},[&](auto i){guard();const auto& c=*rows.at(i);const auto n=city_scalar(c,"m_iPopulation");need(s.cities.players.at(p.owner).cities.at(city_scalar(c,"m_iID")).population==n,"Capital strategy raw/owned population differs");return n;},[&](auto i){return resolve_bnw_city_registry(state().world,state().cities,p.owner,cap)==rows.at(i);},[&]{return define("AI_MILITARY_CITY_THREAT_WEIGHT_CAPITAL");});return best&&rows.at(*best)==&city;
  }
  std::int32_t threat()const{guard();return city_scalar(city,"m_iThreatValue");}
  std::int32_t settlers()const{
   guard();auto counter=inspect_bnw_city_current_naval_strategy(p.host,p.owner,p.city);guard();
   const auto count=counter.unit_count(BnwCityNativeUnitCount::settlers_including_water);guard();return count;
  }
  std::int32_t under_threat_type()const{const auto* i=info("AICITYSTRATEGY_CAPITAL_UNDER_THREAT","AICityStrategies");return i?native(i->id):-1;}
  std::int32_t game_turn()const{const auto& s=state();need(s.world.pregame&&s.world.pregame->layout==bnw,"Capital strategy GAME turn unavailable");const auto n=native(s.world.pregame->archive.integers.at("game_turn"));need(s.setup.game_turn==n,"Capital strategy raw/owned GAME turn differs");return n;}
  bool under_threat_using()const{
   const auto id=under_threat_type();need(id>=0,"Capital strategy reached UnderThreat Type absent");guard();need(city.management.layout==bnw,"Capital strategy actual city strategy Read schema absent");const auto& b=named(city.management.blocks,"strategy");need(named(b.fields,"version").value==1,"Capital strategy city strategy native version unavailable");const auto& a=named(b.hashes,"m_pabUsingCityStrategy");need(a.wire_count>=0&&a.wire_count<=UINT32_MAX&&static_cast<std::uint64_t>(a.wire_count)==a.rows.size(),"Capital strategy city using hash shape differs");std::optional<bool> result;
   for(const auto& row:a.rows){need(row.type_hash<=UINT32_MAX,"Capital strategy city using hash width differs");if(!row.type_hash){need(row.values.empty(),"Capital strategy null city using hash payload invalid");continue;}need(row.values.size()==1,"Capital strategy city using scalar shape differs");const bool value=bit(row.values.front());const auto* t=p.host->types().find_hash(row.type_hash);if(!t)continue;need(t->table=="AICityStrategies"&&t->id>=0&&t->id<=INT32_MAX,"Capital strategy city using cross-table remap unavailable");if(t->id==id)result=value;}
   need(result.has_value(),"Capital strategy omitted reached using slot requires actual native Reset/Read owner");return *result;
  }
  std::int32_t mobilization_type()const{const auto* i=info("MILITARYAISTRATEGY_WAR_MOBILIZATION","AIMilitaryStrategies");return i?native(i->id):-1;}
  bool mobilization_using()const{
   const auto id=mobilization_type();need(id>=0,"Capital strategy reached Military Type absent");
   std::int32_t slots=0;const auto catalog=p.database->query_rows({"SELECT ID,Type FROM AIMilitaryStrategies ORDER BY ID",{}},[&](const auto& row){need(native(row.integer("ID"))==slots,"Capital strategy finite native Military Info allocation has unsupported hole/order");const auto type=row.text("Type");need(type.has_value(),"Capital strategy native Military Info Type absent");const auto* t=info(type->c_str(),"AIMilitaryStrategies");need(t&&t->id==slots,"Capital strategy native Military Info catalog differs");slots=add(slots,1);},1024*1024);guard();need(catalog.all_succeeded()&&slots>id&&catalog.rows==static_cast<std::uint64_t>(slots),"Capital strategy native Military allocation unavailable");
   const auto& obs=owner().strategy;need(obs.layout==bnw,"Capital strategy actual MilitaryAI Read schema unavailable");const auto& b=named(obs.blocks,"military_ai");need(named(b.fields,"version").value==1,"Capital strategy MilitaryAI native version unavailable");const auto count=native(named(b.fields,"strategy_wire_count").value);need(count>=0&&count<=slots&&id<count,"Capital strategy missing/unsafe Military using prefix requires native Reset/Read owner");const auto& a=named(b.arrays,"using_strategy");need(a.columns==1&&a.values.size()==static_cast<std::size_t>(count),"Capital strategy native indexed Military using shape differs");return bit(a.values.at(static_cast<std::size_t>(id)));
  }
  const content::InfoTypeEntry& settler_strategy()const{const auto* i=info("AICITYSTRATEGY_CAPITAL_NEED_SETTLER","AICityStrategies");need(i,"Capital strategy reached Settler Info absent");return *i;}
  std::int32_t threshold_modifier()const{
   const auto& entry=settler_strategy();const auto& q=owner();const auto& personality=checked_bnw_personality_flavors(q,q,*p.database);guard();std::vector<std::int32_t> mods(personality.size(),0);
   const auto report=p.database->query_rows({"SELECT Flavors.ID AS FlavorID, Flavor FROM AICityStrategy_PersonalityFlavorThresholdMods INNER JOIN Flavors ON Flavors.Type=FlavorType WHERE AICityStrategyType=?",{entry.type}},[&](const auto& row){const auto slot=native(row.integer("FlavorID"));need(slot>=0&&static_cast<std::size_t>(slot)<mods.size(),"Capital strategy threshold flavor slot unavailable");mods.at(static_cast<std::size_t>(slot))=native(row.integer("Flavor"));},1024*1024);guard();need(report.all_succeeded(),"Capital strategy actual threshold modifier query failed");std::int32_t sum=0;for(std::size_t i=0;i<mods.size();++i)sum=add(sum,mul(native(personality.at(i)),mods.at(i)));return sum;
  }
  std::int32_t weight_threshold()const{
   const auto& entry=settler_strategy();std::optional<std::int32_t> value;const auto report=p.database->query_rows({"SELECT ID,Type,WeightThreshold FROM AICityStrategies WHERE ID=?",{entry.id}},[&](const auto& row){need(!value&&row.integer("ID")==entry.id&&row.text("Type")==entry.type,"Capital strategy actual Settler Info row differs");value=native(row.integer("WeightThreshold"));},2);guard();need(report.all_succeeded()&&report.rows==1&&value,"Capital strategy actual threshold query unavailable");return *value;
  }
 } reader{p,*raw,[&]{require_current();}};
 const auto result=evaluate(trigger,reader);require_current();return result;
 }catch(...){p.retired.store(true);throw;}
});}
BnwCityCurrentCapitalStrategy inspect_bnw_city_current_capital_strategy(
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host,std::int64_t owner,std::int64_t city){return typed([&]{
 need(bool(host),"Capital strategy genuine selected host absent");(void)host->session().bnw_game_debug_mode();auto p=std::make_shared<BnwCityCurrentCapitalStrategy::Impl>();p->host=std::move(host);
 p->session=std::shared_ptr<const LiveSession>(p->host,&p->host->session());p->database=std::shared_ptr<const content::RulesDatabase>(p->host,&p->host->database());p->rules_current=p->database->immutable_preview_fingerprint_currentness();need(p->rules_current&&p->rules_current(),"Capital strategy genuine immutable DB absent");p->identity=p->host->identity();need(p->database->gameplay_fingerprint_sha256()==p->identity.rules_sha256&&p->identity.rules_sha256.size()==64&&p->identity.payload_sha256.size()==64&&p->host->types().report().all_succeeded(),"Capital strategy actual host identity/rules incomplete");
 p->state=p->session->read_state_lease();p->source=&p->session->source();p->generation=p->session->state_generation();need(p->state&&identify_bytes(p->source->source.bytes).sha256==p->identity.payload_sha256,"Capital strategy actual source payload differs");p->owner=native(owner);need(p->owner>=0&&p->owner<64,"Capital strategy owner outside native roster");const auto* c=resolve_bnw_city_registry(p->state->world,p->state->cities,p->owner,native(city));need(c,"Capital strategy actual selected city absent");p->city=city_scalar(*c,"m_iID");need(city_scalar(*c,"m_eOwner")==p->owner&&c->core.start_offset<c->core.end_offset,"Capital strategy canonical owner/core span differs");p->city_span={c->core.start_offset,c->core.end_offset};BnwCityCurrentCapitalStrategy result(std::move(p));result.require_current();return result;
});}
}
