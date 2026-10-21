#include "civ5/runtime/bnw_city_current_building_strategy.hpp"
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
void need(bool b,const char* m){if(!b)throw BnwCityBuildingStrategyUnavailable(m);}
std::int32_t native(std::int64_t n){need(n>=INT32_MIN&&n<=INT32_MAX,"Building city strategy reached signed32 operand unavailable");return static_cast<std::int32_t>(n);}
template<class F>auto typed(F&& f)->decltype(f()){
 try{return f();}catch(const BnwCityBuildingStrategyUnavailable&){throw;}
 catch(const std::exception& e){throw BnwCityBuildingStrategyUnavailable(e.what());}
}
const auto& named(const auto& rows,std::string_view name){
 using T=typename std::remove_cvref_t<decltype(rows)>::value_type;const T* out=nullptr;
 for(const auto& r:rows)if(r.name&&std::string_view(r.name)==name){need(!out,"Building city strategy duplicate source field");out=&r;}
 need(out,"Building city strategy required source field absent");return *out;
}
bool same_info(const content::InfoTypeEntry* a,const content::InfoTypeEntry* b){return a&&b&&a->table==b->table&&a->type==b->type&&a->id==b->id&&a->hash==b->hash;}
std::int32_t city_scalar(const saves::SavedCityObservation& c,std::string_view name){
 const auto& f=named(c.core.fields,name);
 need(c.core.layout==bnw&&f.encoding==saves::CityCoreEncoding::i32&&f.values.size()==1&&!f.wire_count&&
  f.bytes.empty()&&f.hashes.empty()&&f.row_keys.empty()&&f.row_counts.empty(),"Building city strategy city native scalar shape unavailable");
 return native(f.values.front());
}
template<class Pop,class Culture,class Capital,class Science,class Gold,class Prod,class ProdMod,class Faith,class Flavor,class Pantheon,class Training,class Handicap,class Original>
bool evaluate(BnwBuildingCityStrategy t,Pop&& pop,Culture&& culture,Capital&& capital,Science&& science,Gold&& gold,Prod&& prod,ProdMod&& prodmod,Faith&& faith,Flavor&& flavor,Pantheon&& pantheon,Training&& training,Handicap&& handicap,Original&& original){
 switch(t){
 case BnwBuildingCityStrategy::original_capital:return original();
 case BnwBuildingCityStrategy::medium_high_difficulty:return pop()>=5&&handicap()>4;
 case BnwBuildingCityStrategy::training_facility:return training()>0;
 case BnwBuildingCityStrategy::first_culture:if(culture()<=0)return true;return capital()&&culture()<=1;
 case BnwBuildingCityStrategy::first_culture_emergency:return pop()>=5&&culture()<=0;
 case BnwBuildingCityStrategy::first_science:return pop()>=4&&science()==0;
 case BnwBuildingCityStrategy::first_gold:return pop()>=4&&gold()==0;
 case BnwBuildingCityStrategy::first_production:return pop()>=4&&prod()==0&&prodmod()==0;
 case BnwBuildingCityStrategy::first_faith:{const auto f=flavor();if(faith()>0)return false;if(pop()<2)return false;return (f>4&&pantheon())||f>6;}
 }
 throw BnwCityBuildingStrategyUnavailable("Building city strategy outside nine-source scope");
}
}
bool bnw_city_building_strategy_math(BnwBuildingCityStrategy t,const BnwBuildingCityStrategyMathInputs& i){
 const auto read=[](const std::optional<std::int64_t>& v){need(v.has_value(),"Building city strategy reached pure operand absent");return native(*v);};
 const auto bit=[&](const auto& v){const auto n=read(v);need(n==0||n==1,"Building city strategy pure bool invalid");return n!=0;};
 return evaluate(t,[&]{return read(i.population);},[&]{return read(i.culture_buildings);},[&]{return bit(i.capital);},[&]{return read(i.science_per_pop);},[&]{return read(i.gold_modifier);},[&]{return read(i.production_buildings);},[&]{return read(i.production_modifier);},[&]{return read(i.faith_buildings);},[&]{return read(i.religion_flavor);},[&]{return bit(i.pantheon);},[&]{return read(i.land_experience);},[&]{return read(i.game_handicap);},[&]{return read(i.city_x)==read(i.original_capital_x)&&read(i.city_y)==read(i.original_capital_y);});
}
struct BnwCityCurrentBuildingStrategy::Impl {
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
BnwCityCurrentBuildingStrategy::BnwCityCurrentBuildingStrategy(std::shared_ptr<Impl> p)noexcept:impl_(std::move(p)){}
void BnwCityCurrentBuildingStrategy::require_current()const{
 const auto& p=impl_;need(p&&p->thread==std::this_thread::get_id(),"Building city strategy foreign constructor thread/moved query");
 try{
  need(!p->retired.load()&&p->host&&p->session&&p->database&&p->rules_current&&p->rules_current(),"Building city strategy genuine rules/host retired");
  need(p->database->gameplay_fingerprint_sha256()==p->identity.rules_sha256&&
   p->host->identity().rules_sha256==p->identity.rules_sha256&&p->host->identity().payload_sha256==p->identity.payload_sha256,
   "Building city strategy actual rules/source identity differs");
  (void)p->session->bnw_game_debug_mode(); // actual owning thread/lifecycle BEFORE facade state/source
  need(&p->host->session()==p->session.get() && &p->host->database()==p->database.get()&&
   p->session->read_state_lease()==p->state&&p->session->state_generation()==p->generation && &p->session->source()==p->source,
   "Building city strategy actual allocation/source/generation retired");
 }catch(...){p->retired.store(true);throw;}
}
bool BnwCityCurrentBuildingStrategy::value(BnwBuildingCityStrategy trigger)const{return typed([&]{
 require_current();need(trigger>=BnwBuildingCityStrategy::original_capital&&trigger<=BnwBuildingCityStrategy::first_production,"Building city strategy outside nine-source scope");const auto& p=*impl_;
 try{
 const auto& s=*p.state;const auto* raw=resolve_bnw_city_registry(s.world,s.cities,p.owner,p.city);need(raw&&city_scalar(*raw,"m_iID")==p.city&&city_scalar(*raw,"m_eOwner")==p.owner&&std::pair{raw->core.start_offset,raw->core.end_offset}==p.city_span,"Building city strategy canonical city/span differs");const auto& owned=s.cities.players.at(p.owner).cities.at(p.city);
 const auto pop=[&]{require_current();const auto n=city_scalar(*raw,"m_iPopulation");need(owned.population==n,"Building city strategy raw/owned population differs");return n;};
 const auto player=[&](std::int32_t owner)->const saves::SavedPlayerObservation&{require_current();need(owner>=0&&owner<64&&s.world.all_players&&s.world.all_players->layout==bnw&&s.world.all_players->context.player_count==64&&s.world.all_players->players.size()==64,"Building city strategy actual player roster absent");const auto& q=s.world.all_players->players.at(static_cast<std::size_t>(owner));need(q.index==static_cast<std::uint64_t>(owner)&&q.saved_player_id==owner&&q.core.layout==bnw,"Building city strategy actual player identity differs");const auto& id=named(q.core.fields,"m_eID");need(id.encoding==saves::PlayerCoreEncoding::i32&&id.values.size()==1&&native(id.values.front())==owner,"Building city strategy native player ID differs");return q;};
 const auto player_scalar=[&](const saves::SavedPlayerObservation& q,const char* name){require_current();const auto& f=named(q.core.fields,name);need(f.encoding==saves::PlayerCoreEncoding::i32&&f.values.size()==1&&!f.wire_count&&f.bytes.empty()&&f.hashes.empty()&&f.row_counts.empty(),"Building city strategy native player scalar absent");return native(f.values.front());};
 const auto capital=[&]{const auto& q=player(p.owner);const auto id=player_scalar(q,"m_iCapitalCityID");need(s.cities.players.at(p.owner).capital==id,"Building city strategy raw/owned capital handle differs");return resolve_bnw_city_registry(s.world,s.cities,p.owner,id)==raw;};
 const auto original=[&]{require_current();const auto& q=player(city_scalar(*raw,"m_eOriginalOwner"));const auto coord=[&](const char* field,std::int64_t mirror){const auto n=city_scalar(*raw,field);need(n==mirror,"Building city strategy raw/owned coordinate differs");return n;};return coord("m_iX",owned.x)==player_scalar(q,"m_iOriginalCapitalX")&&coord("m_iY",owned.y)==player_scalar(q,"m_iOriginalCapitalY");};
 const auto info=[&](const char* name,const char* table,std::int32_t expected){require_current();const auto* i=p.host->types().find(name);need(p.host->types().report().all_succeeded()&&i&&i->table==table&&i->id==expected&&same_info(p.host->types().find_hash(content::saved_info_type_hash(name)),i),"Building city strategy reached native enum/Info differs");};
 const auto vector=[&](const char* name,std::int32_t index,std::size_t size){require_current();const auto& f=named(raw->core.fields,name);need(f.encoding==saves::CityCoreEncoding::vector_i32&&f.wire_count==size&&f.values.size()==size&&f.bytes.empty()&&f.hashes.empty()&&f.row_keys.empty()&&f.row_counts.empty()&&index>=0&&static_cast<std::size_t>(index)<size,"Building city strategy native vector shape absent");return native(f.values.at(static_cast<std::size_t>(index)));};
 const auto yield=[&](const char* field,const char* name,std::int32_t index,auto member){info(name,"Yields",index);need(s.cities.profile==content::TechnologyProfile::bnw&&s.cities.yield_slots==6,"Building city strategy owned yield span differs");const auto n=vector(field,index,6);const auto it=owned.yields.find(index);const auto value=it==owned.yields.end()?0:it->second.*member;need(value==n,"Building city strategy reached raw/owned yield component differs");return n;};
 const auto handicap=[&]{require_current();need(s.world.game_state.layout==bnw&&s.setup.profile==content::TechnologyProfile::bnw,"Building city strategy native game setup absent");const auto id=native(named(s.world.game_state.controls,"handicap").value);need(s.setup.game_handicap==id&&id>=0,"Building city strategy raw/owned Game handicap differs");const content::InfoTypeEntry* found=nullptr;for(const auto& [key,i]:p.host->types().entries()){(void)key;if(i.table=="HandicapInfos"&&i.id==id){need(!found,"Building city strategy native HandicapInfo alias ambiguous");found=&i;}}need(found&&same_info(p.host->types().find_hash(found->hash),found)&&p.host->types().report().all_succeeded(),"Building city strategy reached HandicapInfo absent");bool matched=false;const auto report=p.database->query_rows({"SELECT ID,Type FROM HandicapInfos WHERE ID=?",{id}},[&](const auto& row){need(!matched&&row.has_column("ID")&&row.has_column("Type")&&row.integer("ID")==id&&row.text("Type")==found->type,"Building city strategy actual HandicapInfo metadata differs");matched=true;},2);require_current();need(report.all_succeeded()&&matched&&report.rows==1,"Building city strategy actual HandicapInfo query unavailable");return id;};
 const auto flavor=[&]{const auto& q=player(p.owner);const auto& values=checked_bnw_personality_flavors(q,q,*p.database);require_current();const auto* f=p.host->types().find("FLAVOR_RELIGION");need(f&&f->table=="Flavors"&&f->id>=0&&static_cast<std::uint64_t>(f->id)<values.size()&&same_info(p.host->types().find_hash(content::saved_info_type_hash("FLAVOR_RELIGION")),f),"Building city strategy actual Religion flavor Info absent");return native(values.at(static_cast<std::size_t>(f->id)));};
 const auto pantheon=[&]{require_current();need(s.world.religion_culture.layout==bnw&&p.source->snapshot.religion_culture.layout==bnw&&s.world.religion_culture.start_offset==p.source->snapshot.religion_culture.start_offset&&s.world.religion_culture.end_offset==p.source->snapshot.religion_culture.end_offset,"Building city strategy actual Game religion source absent");for(const auto& r:s.world.religion_culture.religions)if(native(r.founder)==p.owner&&r.pantheon)return true;return false;};
 const auto result=evaluate(trigger,pop,[&]{require_current();return city_scalar(*raw,"m_iJONSCulturePerTurnFromBuildings");},capital,
 [&]{return yield("m_aiYieldPerPop","YIELD_SCIENCE",3,&CityYieldComponents::per_population);},[&]{return yield("m_aiYieldRateModifier","YIELD_GOLD",2,&CityYieldComponents::rate_modifier);},[&]{return yield("m_aiBaseYieldRateFromBuildings","YIELD_PRODUCTION",1,&CityYieldComponents::buildings);},[&]{return yield("m_aiYieldRateModifier","YIELD_PRODUCTION",1,&CityYieldComponents::rate_modifier);},
 [&]{require_current();return city_scalar(*raw,"m_iFaithPerTurnFromBuildings");},flavor,pantheon,[&]{info("DOMAIN_LAND","Domains",2);return vector("m_aiDomainFreeExperience",2,5);},handicap,original);
 require_current();return result;
 }catch(...){p.retired.store(true);throw;}
});}
BnwCityCurrentBuildingStrategy inspect_bnw_city_current_building_strategy(
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host,std::int64_t owner,std::int64_t city){return typed([&]{
 need(bool(host),"Building city strategy actual selected host absent");
 (void)host->session().bnw_game_debug_mode(); // actual thread/owner BEFORE DB or world
 auto p=std::make_shared<BnwCityCurrentBuildingStrategy::Impl>();p->host=std::move(host);
 p->session=std::shared_ptr<const LiveSession>(p->host,&p->host->session());
 p->database=std::shared_ptr<const content::RulesDatabase>(p->host,&p->host->database());
 p->rules_current=p->database->immutable_preview_fingerprint_currentness();
 need(p->rules_current&&p->rules_current(),"Building city strategy actual immutable rules scope absent");
 p->identity=p->host->identity();need(!p->identity.payload_sha256.empty()&&!p->identity.rules_sha256.empty()&&
  p->database->gameplay_fingerprint_sha256()==p->identity.rules_sha256,"Building city strategy actual host rules identity differs");
 p->state=p->session->read_state_lease();need(bool(p->state),"Building city strategy actual state absent");
 p->source=&p->session->source();p->generation=p->session->state_generation();
 need(identify_bytes(p->source->source.bytes).sha256==p->identity.payload_sha256,"Building city strategy actual source payload differs");
 p->owner=native(owner);need(p->owner>=0&&p->owner<64,"Building city strategy native owner outside BNW roster");
 const auto* raw=resolve_bnw_city_registry(p->state->world,p->state->cities,p->owner,native(city));
 need(raw,"Building city strategy actual current city absent");p->city=city_scalar(*raw,"m_iID");
 need(city_scalar(*raw,"m_eOwner")==p->owner&&raw->core.start_offset<raw->core.end_offset,"Building city strategy canonical owner/source span differs");
 p->city_span={raw->core.start_offset,raw->core.end_offset};
 BnwCityCurrentBuildingStrategy result(std::move(p));result.require_current();return result;
});}
}
