#include "civ5/runtime/bnw_city_current_strategy_eligibility.hpp"
#include "civ5/runtime/bnw_city_management_checkpoint.hpp"
#include "civ5/runtime/bnw_city_strategy_yields.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <atomic>
#include <climits>
#include <functional>
#include <set>
#include <string_view>
#include <thread>
#include <type_traits>
namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void need(bool b,const char* why){if(!b)throw BnwCityCurrentStrategyEligibilityUnavailable(why);}
std::int32_t native(std::int64_t n){need(n>=INT32_MIN&&n<=INT32_MAX,"Current city strategy reached native signed32 overflow");return static_cast<std::int32_t>(n);}
std::int32_t add(std::int32_t a,std::int32_t b){return native(std::int64_t(a)+b);}
std::int32_t sub(std::int32_t a,std::int32_t b){return native(std::int64_t(a)-b);}
template<class F>auto typed(F&& f)->decltype(f()){
 try{return f();}catch(const BnwCityCurrentStrategyEligibilityUnavailable&){throw;}
 catch(const std::exception& e){throw BnwCityCurrentStrategyEligibilityUnavailable(e.what());}
}
const auto& named(const auto& rows,std::string_view name){
 using T=typename std::remove_cvref_t<decltype(rows)>::value_type;const T* out=nullptr;
 for(const auto& r:rows)if(r.name&&std::string_view(r.name)==name){need(!out,"Current city strategy duplicate raw field");out=&r;}
 need(out,"Current city strategy raw field absent");return *out;
}
std::int32_t city_scalar(const saves::SavedCityObservation& c,const char* name){
 const auto& f=named(c.core.fields,name);need(c.core.layout==bnw&&f.encoding==saves::CityCoreEncoding::i32&&f.values.size()==1&&!f.wire_count&&f.bytes.empty()&&f.hashes.empty()&&f.row_keys.empty()&&f.row_counts.empty(),"Current city strategy native city scalar schema differs");return native(f.values.front());
}
// ONE source-order algorithm; private current readers and supplied arithmetic
// readers below share it. There are no public callbacks or completion flags.
template<class Reader>BnwCityStrategyTriggerObligation evaluate(Reader& r){
 if(r.minor()&&r.no_minor())return BnwCityStrategyTriggerObligation::skipped_for_minor;
 bool start=true;
 if(r.using_strategy())start=false;
 else{
  const auto prereq=r.prerequisite();
  if(prereq!=-1&&!r.has_tech(prereq))start=false;
  if(start){const auto obsolete=r.obsolete();if(obsolete!=-1&&r.has_tech(obsolete))start=false;}
 }
 bool end=false;
 if(r.using_strategy()&&!r.permanent()){
  const auto check=r.check_turn_count();
  if(check>0){const auto turn=r.game_turn();const auto adopted=r.turn_adopted();if(sub(turn,adopted)%check==0)end=true;}
  if(end){const auto minimum=r.minimum_turns();if(minimum>0){const auto turn=r.game_turn();const auto adopted=r.turn_adopted();if(turn<add(adopted,minimum))end=false;}}
 }
 return start?BnwCityStrategyTriggerObligation::start:end?BnwCityStrategyTriggerObligation::end:BnwCityStrategyTriggerObligation::none;
}
struct SuppliedReader {
 const BnwCityStrategyEligibilityMathInputs& p;
 bool minor()const{return p.minor;}bool no_minor()const{return p.no_minor;}bool using_strategy()const{return p.using_strategy;}bool permanent()const{return p.permanent;}
 auto prerequisite()const{return p.prerequisite;}auto obsolete()const{return p.obsolete;}
 auto check_turn_count()const{return p.check_turn_count;}auto minimum_turns()const{return p.minimum_turns;}
 auto game_turn()const{return p.game_turn;}auto turn_adopted()const{return p.turn_adopted;}
 bool has_tech(std::int32_t id)const{const auto it=p.has_tech.find(id);need(it!=p.has_tech.end(),"Supplied strategy arithmetic reached missing technology fact");return it->second;}
};
struct Rule {
 std::int32_t id=0;std::string type;bool no_minor=false,permanent=false;
 std::int32_t prereq=-1,obsolete=-1;std::int64_t check=0,minimum=0;
};
std::vector<std::optional<Rule>> load_rules(const ui::SelectedBnwPreviewHost& host){
 std::vector<std::optional<Rule>> result;
 const auto tech=[&](const std::optional<std::string>& text){
  if(!text||text->empty())return std::int32_t(-1);
  const auto* t=host.types().find(*text);if(!t)return std::int32_t(-1); // SDK getInfoTypeForString(...,true).
  need(t->table=="Technologies"&&t->id>=0&&t->id<=INT32_MAX&&host.technologies().find(static_cast<std::uint64_t>(t->id)),"Current strategy Tech reference has incompatible native Info mapping");return native(t->id);
 };
 const auto report=host.database().prefetch_info("AICityStrategies",[&](auto slot,auto authored,const auto& row){
  need(slot<INT32_MAX&&slot>=result.size()&&authored==static_cast<std::int64_t>(slot),"Current strategy native Info logical order unavailable");
  result.resize(static_cast<std::size_t>(slot)+1);
  const auto type=row.text("Type");need(type&&!type->empty(),"Current strategy native Info Type absent");
  const auto hash=content::saved_info_type_hash(*type);const auto* t=host.types().find(*type);const auto* h=host.types().find_hash(hash);
  need(t&&h&&t->table=="AICityStrategies"&&t->id==authored&&h->type==*type&&h->table==t->table&&h->id==authored,"Current strategy activated Type/hash/logical slot differs");
  result.back()=Rule{native(authored),*type,row.boolean("NoMinorCivs"),row.boolean("Permanent"),tech(row.text("TechPrereq")),tech(row.text("TechObsolete")),row.integer("CheckTriggerTurnCount"),row.integer("MinimumNumTurnsExecuted")};return true;
 });need(report.all_succeeded()&&!report.overlapping_ids&&report.slots<=INT32_MAX&&report.slots==result.size(),"Current strategy Info cache/slot mapping unavailable");return result;
}
struct CurrentReader {
 const LiveSessionState& s;const ui::SelectedBnwPreviewHost& host;const saves::SavedCityObservation& city;const Rule& rule;
 std::int32_t owner;std::uint64_t strategy_slots;std::optional<bool> used;
 bool minor()const{
  need(s.setup.profile==content::TechnologyProfile::bnw&&s.world.pregame&&s.world.pregame->layout==bnw&&s.world.all_players&&s.world.all_players->layout==bnw&&s.world.all_players->context.player_count==64&&s.world.all_players->context.strategy.civ_players==63&&s.world.all_players->players.size()==64,"Current strategy actual player/Pregame roster unavailable");
  const auto& p=s.world.all_players->players.at(static_cast<std::size_t>(owner));const auto& id=named(p.core.fields,"m_eID");
  need(p.index==static_cast<std::uint64_t>(owner)&&p.saved_player_id==owner&&p.core.layout==bnw&&id.encoding==saves::PlayerCoreEncoding::i32&&id.values.size()==1&&id.values[0]==owner&&!id.wire_count&&id.bytes.empty()&&id.hashes.empty()&&id.row_counts.empty(),"Current strategy actual player native identity differs");
  const auto& flags=s.world.pregame->archive.flag_arrays.at("minor_nation_civs");need(flags.size()==64,"Current strategy minor native64 dimensions unavailable");const bool value=flags[static_cast<std::size_t>(owner)];need(s.cities.players.at(owner).minor==value,"Current strategy raw/owned minor differs");return value;
 }
 bool no_minor()const{return rule.no_minor;}
 std::int64_t hash_value(const char* name)const{
  need(city.management.layout==bnw,"Current strategy native management profile differs");const auto& b=named(city.management.blocks,"strategy");need(named(b.fields,"version").value==1,"Current strategy Read schema version unavailable");
  const auto& a=named(b.hashes,name);need(a.wire_count>=0&&a.wire_count<=UINT32_MAX&&a.rows.size()<=UINT32_MAX&&a.wire_count==static_cast<std::int64_t>(a.rows.size()),"Current strategy hash wire count differs");std::optional<std::int64_t> value;
  for(const auto& row:a.rows){need(row.type_hash<=UINT32_MAX,"Current strategy hash native width differs");if(!row.type_hash){need(row.values.empty(),"Current strategy null hash consumed value");continue;}
   need(row.values.size()==1,"Current strategy hash payload scalar differs");
   if(std::string_view(name)=="m_pabUsingCityStrategy")need(row.values[0]==0||row.values[0]==1,"Current strategy native using bool is not0/1");
   else (void)native(row.values[0]);
   const auto* t=host.types().find_hash(row.type_hash);if(!t)continue; // source ignores unknown hashed row after consuming payload.
   need(t->table=="AICityStrategies"&&t->id>=0&&t->id<static_cast<std::int64_t>(strategy_slots),"Current strategy hash maps to incompatible Info span");
   if(t->id==rule.id)value=row.values[0]; // ReadHashedDataArray last-wins, including false/-1.
  }
  need(value.has_value(),"Current strategy missing reached hash slot requires actual native Reset/Read strategy array owner");return *value;
 }
 bool using_strategy(){const bool value=hash_value("m_pabUsingCityStrategy")!=0;used=value;return value;}
 bool permanent()const{return rule.permanent;}
 auto prerequisite()const{return rule.prereq;}auto obsolete()const{return rule.obsolete;}
 std::int32_t check_turn_count()const{return native(rule.check);}std::int32_t minimum_turns()const{return native(rule.minimum);}
 std::int32_t game_turn()const{need(s.world.pregame&&s.world.pregame->layout==bnw,"Current strategy GAME turn Pregame unavailable");const auto n=native(s.world.pregame->archive.integers.at("game_turn"));need(s.setup.game_turn==n,"Current strategy raw/owned GAME turn differs");return n;}
 std::int32_t turn_adopted()const{return native(hash_value("m_paiTurnCityStrategyAdopted"));}
 bool has_tech(std::int32_t tech)const{
  need(host.technologies().collection.slots<=INT32_MAX&&tech>=0&&static_cast<std::uint64_t>(tech)<host.technologies().collection.slots&&host.technologies().find(static_cast<std::uint64_t>(tech)),"Current strategy reached native technology slot/span unavailable");
  const auto& preg=s.world.pregame->archive.integer_arrays.at("team_types");need(preg.size()==64,"Current strategy Pregame team native64 slots unavailable");const auto team=native(preg.at(static_cast<std::size_t>(owner)));need(team>=0&&team<64&&s.setup.players.at(owner).team==team,"Current strategy raw/owned owner TEAM differs");
  need(s.world.teams&&s.world.teams->layout==bnw&&s.world.teams->dimensions.teams==64&&s.world.teams->dimensions.civ_teams==63&&s.world.teams->teams.size()==64,"Current strategy actual team native registry unavailable");const auto& raw=s.world.teams->teams.at(static_cast<std::size_t>(team));need(raw.index==static_cast<std::uint64_t>(team)&&native(named(raw.fields,"id").value)==team,"Current strategy native TEAM identity differs");
  const auto& hashes=named(raw.arrays,"tech_hashes");need(hashes.columns==1,"Current strategy native tech hash columns differ");bool covered=false;
  for(const auto h:hashes.values){need(h>=0&&h<=UINT32_MAX,"Current strategy native TEAM tech hash width differs");const auto* t=h?host.types().find_hash(static_cast<std::uint64_t>(h)):nullptr;if(!t)continue;need(t->table=="Technologies","Current strategy TEAM tech hash cross-table mapping unavailable");if(t->id==tech)covered=true;}
  need(covered,"Current strategy missing reached HasTech slot requires actual native TEAM Reset/Read owner");
  const auto decoded=restore_team_technologies(raw,host.technologies(),host.types());const TeamTechnologies* owned=nullptr;
  for(const auto& t:s.technologies)if(t.team()==team){need(!owned&&t.profile()==content::TechnologyProfile::bnw&&t.slots()==host.technologies().collection.slots,"Current strategy owned tech TEAM/slots ambiguous");owned=&t;}
  need(owned&&decoded.has_tech(tech)==owned->has_tech(tech),"Current strategy raw/owned current HasTech differs");return owned->has_tech(tech);
 }
};
}
BnwCityStrategyTriggerObligation bnw_city_strategy_eligibility_math(const BnwCityStrategyEligibilityMathInputs& p){SuppliedReader r{p};return evaluate(r);}
struct BnwCityCurrentStrategyEligibility::Impl {
 const std::thread::id thread=std::this_thread::get_id();std::shared_ptr<const ui::SelectedBnwPreviewHost> host;
 std::shared_ptr<const LiveSession> session;std::shared_ptr<const content::RulesDatabase> database;std::shared_ptr<const LiveSessionState> state;
 const saves::SavedSession* source=nullptr;std::uint64_t generation=0;SessionCheckpointIdentity identity;
 saves::CivisSaveSection management_beforeimage;
 std::function<bool()> rules_current;std::vector<std::optional<Rule>> rules;std::int32_t owner=-1,city=-1;mutable std::atomic<bool> retired{false};
};
BnwCityCurrentStrategyEligibility::BnwCityCurrentStrategyEligibility(std::shared_ptr<Impl> p)noexcept:impl_(std::move(p)){}
void BnwCityCurrentStrategyEligibility::require_current()const{
 const auto& p=impl_;need(p&&p->thread==std::this_thread::get_id(),"Current strategy foreign constructor thread/moved handle");
 try{
  need(!p->retired.load()&&p->host&&p->session&&p->database&&p->rules_current&&p->rules_current(),"Current strategy actual immutable host retired");
  need(p->database->gameplay_fingerprint_sha256()==p->identity.rules_sha256&&p->host->identity().rules_sha256==p->identity.rules_sha256&&p->host->identity().payload_sha256==p->identity.payload_sha256,"Current strategy actual host/rules identity differs");
  (void)p->session->bnw_game_debug_mode();
  need(&p->host->session()==p->session.get()&& &p->host->database()==p->database.get()&&p->session->read_state_lease()==p->state&&p->session->state_generation()==p->generation&& &p->session->source()==p->source,"Current strategy actual allocation/source/generation retired");
  const auto current=make_bnw_city_management_checkpoint(p->state->world,p->source->snapshot,p->identity,true);
  need(current.id==p->management_beforeimage.id&&current.flags==p->management_beforeimage.flags&&current.bytes==p->management_beforeimage.bytes,
   "Current strategy component revision/raw beforeimage changed after owner binding");
 }catch(...){p->retired.store(true);throw;}
}
std::vector<BnwCityStrategyEligibilityFact> BnwCityCurrentStrategyEligibility::records()const{return typed([&]{
 require_current();auto& p=*impl_;try{
  const auto* raw=resolve_bnw_current_city(*p.state,p.owner,p.city);need(raw&&city_scalar(*raw,"m_iID")==p.city&&city_scalar(*raw,"m_eOwner")==p.owner,"Current strategy canonical city disappeared");std::vector<BnwCityStrategyEligibilityFact> out;out.reserve(p.rules.size());
  for(const auto& slot:p.rules){need(slot.has_value(),"Current strategy reached native NULL StrategyInfo dereference");const auto& rule=*slot;CurrentReader reader{*p.state,*p.host,*raw,rule,p.owner,static_cast<std::uint64_t>(p.rules.size()),{}};const auto obligation=evaluate(reader);out.push_back({rule.id,rule.type,obligation,reader.used});}
  require_current();return out;
 }catch(...){p.retired.store(true);throw;}
});}
BnwCityCurrentStrategyEligibility BnwCityCurrentStrategyEligibility::prepare_ordered(
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host,std::int64_t owner,std::int64_t city){return typed([&]{
 need(bool(host),"Current strategy genuine selected host absent");(void)host->session().bnw_game_debug_mode();
 auto p=std::make_shared<BnwCityCurrentStrategyEligibility::Impl>();p->host=std::move(host);p->session=std::shared_ptr<const LiveSession>(p->host,&p->host->session());p->database=std::shared_ptr<const content::RulesDatabase>(p->host,&p->host->database());p->rules_current=p->database->immutable_preview_fingerprint_currentness();need(p->rules_current&&p->rules_current(),"Current strategy requires actual immutable activated DB");p->identity=p->host->identity();
 need(p->identity.payload_sha256.size()==64&&p->identity.rules_sha256.size()==64&&p->database->gameplay_fingerprint_sha256()==p->identity.rules_sha256&&p->host->types().report().all_succeeded()&&p->host->technologies().all_succeeded()&&p->host->technologies().profile==content::TechnologyProfile::bnw,"Current strategy genuine host identity/rules incomplete");
 p->state=p->session->read_state_lease();p->source=&p->session->source();p->generation=p->session->state_generation();need(p->state&&identify_bytes(p->source->source.bytes).sha256==p->identity.payload_sha256,"Current strategy actual original source identity differs");p->rules=load_rules(*p->host);p->owner=native(owner);const auto* raw=resolve_bnw_current_city(*p->state,p->owner,native(city));need(raw,"Current strategy actual city absent");p->city=city_scalar(*raw,"m_iID");
 p->management_beforeimage=make_bnw_city_management_checkpoint(p->state->world,p->source->snapshot,p->identity,true);
 BnwCityCurrentStrategyEligibility out(std::move(p));out.require_current();return out;
});}
BnwCityStrategyEligibilityFact BnwCityCurrentStrategyEligibility::record_at(std::int64_t id)const{return typed([&]{
 require_current();auto& p=*impl_;const auto n=native(id);need(n>=0&&static_cast<std::size_t>(n)<p.rules.size(),"Current strategy native selector outside collection");
 const auto& slot=p.rules.at(static_cast<std::size_t>(n));need(slot.has_value(),"Current strategy reached native NULL StrategyInfo dereference");
 const auto* raw=resolve_bnw_current_city(*p.state,p.owner,p.city);need(raw&&city_scalar(*raw,"m_iID")==p.city&&city_scalar(*raw,"m_eOwner")==p.owner,"Current strategy single-slot actual city absent");
 const auto& rule=*slot;CurrentReader reader{*p.state,*p.host,*raw,rule,p.owner,static_cast<std::uint64_t>(p.rules.size()),{}};const auto obligation=evaluate(reader);
 require_current();return BnwCityStrategyEligibilityFact{rule.id,rule.type,obligation,reader.used};
});}
bool BnwCityCurrentStrategyEligibility::obsolete_for_dispatch(std::int64_t id)const{return typed([&]{
 require_current();const auto& p=*impl_;const auto n=native(id);need(n>=0&&static_cast<std::size_t>(n)<p.rules.size(),"Current strategy dispatch selector outside collection");
 const auto& slot=p.rules.at(static_cast<std::size_t>(n));need(slot.has_value(),"Current strategy dispatch NULL StrategyInfo");
 const auto* raw=resolve_bnw_current_city(*p.state,p.owner,p.city);need(raw,"Current strategy dispatch actual city absent");
 CurrentReader reader{*p.state,*p.host,*raw,*slot,p.owner,static_cast<std::uint64_t>(p.rules.size()),{}};
 const auto tech=reader.obsolete();const bool result=tech!=-1&&reader.has_tech(tech);require_current();return result;
});}
std::int32_t BnwCityCurrentStrategyEligibility::native_slots()const{require_current();return native(static_cast<std::int64_t>(impl_->rules.size()));}
BnwCityCurrentStrategyEligibility inspect_bnw_city_current_strategy_eligibility(
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host,std::int64_t owner,std::int64_t city){
 auto out=BnwCityCurrentStrategyEligibility::prepare_ordered(std::move(host),owner,city);(void)out.records();return out;
}
}
