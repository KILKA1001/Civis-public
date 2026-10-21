#include "civ5/runtime/bnw_city_current_growth_threshold.hpp"
#include "civ5/runtime/bnw_game_active_team_read.hpp"
#include "civ5/runtime/bnw_city_growth_threshold_math.hpp"
#include "civ5/runtime/bnw_city_strategy_yields.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <atomic>
#include <climits>
#include <cmath>
#include <map>
#include <string_view>
#include <thread>
#include <type_traits>
namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void need(bool b,const char* m){if(!b)throw BnwCityCurrentGrowthThresholdUnavailable(m);}
std::int32_t native(std::int64_t n){need(n>=INT32_MIN&&n<=INT32_MAX,"Current growth native signed32 operand unavailable");return static_cast<std::int32_t>(n);}
template<class F>auto typed(F&& f)->decltype(f()){
 try{return f();}catch(const BnwCityCurrentGrowthThresholdUnavailable&){throw;}
 catch(const std::exception& e){throw BnwCityCurrentGrowthThresholdUnavailable(e.what());}
}
const auto& named(const auto& rows,std::string_view name){
 using T=typename std::remove_cvref_t<decltype(rows)>::value_type;const T* out=nullptr;
 for(const auto& r:rows)if(r.name&&std::string_view(r.name)==name){need(!out,"Current growth duplicate raw field");out=&r;}
 need(out,"Current growth required raw field absent");return *out;
}
std::int32_t city_scalar(const saves::SavedCityObservation& c,std::string_view name){
 const auto& f=named(c.core.fields,name);
 need(c.core.layout==bnw&&f.encoding==saves::CityCoreEncoding::i32&&f.values.size()==1&&!f.wire_count&&
  f.bytes.empty()&&f.hashes.empty()&&f.row_keys.empty()&&f.row_counts.empty(),"Current growth native city scalar shape unavailable");
 return native(f.values.front());
}
const saves::SavedPlayerObservation& player(const LiveSessionState& s,std::int32_t id){
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
bool human(const LiveSessionState& s,std::int32_t id,const content::RulesDatabase& db){
 return detail::bnw_player_human_read(s,id,db);
}
bool alive(const LiveSessionState& s,std::int32_t id){
 const auto& f=named(player(s,id).core.fields,"m_bAlive");
 need(f.encoding==saves::PlayerCoreEncoding::boolean&&f.values.size()==1&&
  (f.values.front()==0||f.values.front()==1)&&s.setup.players.at(id).alive==(f.values.front()!=0),
  "Current growth raw/owned native alive differs");return f.values.front()!=0;
}
std::int32_t team(const LiveSessionState& s,std::int32_t id){
 (void)player(s,id);const auto result=preg_slot(pregame(s),"team_types",id);
 need(result>=0&&result<64&&s.setup.players.at(id).team==result,"Current growth raw/owned native team differs");return result;
}
bool ai_teammate(const LiveSessionState& s,std::int32_t actor,const content::RulesDatabase& db){
 for(std::int32_t id=0;id<64;++id)if(human(s,id,db)&&alive(s,id)){
  // SDK reads candidate getTeam, then actor getTeam, only on this branch.
  const auto candidate=team(s,id);if(candidate==team(s,actor))return true;
 }return false;
}
std::int32_t team_era(const LiveSessionState& s,std::int32_t actor){
 const auto id=team(s,actor);const auto& teams=s.world.teams;
 need(teams&&teams->layout==bnw&&teams->dimensions.teams==64&&teams->dimensions.civ_teams==63&&teams->teams.size()==64,
  "Current growth actual native team registry unavailable");
 const auto& row=teams->teams[static_cast<std::size_t>(id)];
 need(row.index==static_cast<std::uint64_t>(id)&&native(named(row.fields,"id").value)==id,
  "Current growth native team identity differs");return native(named(row.fields,"era").value);
}
struct Rules {
 std::int32_t base=0,minor_percent=0;float multiplier=0,exponent=0;
 struct Info {std::int64_t growth=0,per_era=0;};
 std::map<std::int32_t,Info> speeds,eras,handicaps;
};
Rules load_rules(const ui::SelectedBnwPreviewHost& host){
 const auto& db=host.database();Rules r;const auto defs=db.cache_global_defines(content::GlobalDefineProfile::bnw);
 const auto get=[&](const char* name,content::GlobalDefineKind kind)->const content::CachedGlobalDefine&{
  const auto* v=defs.find(name);need(v&&v->kind==kind&&v->lookup.found&&v->lookup.error.empty(),"Current growth present BNW Define source unavailable");return *v;};
 const auto integer=[&](const char* name){const auto& v=get(name,content::GlobalDefineKind::integer);
  need(std::holds_alternative<std::int64_t>(v.lookup.value),"Current growth native integer Define unavailable");return native(std::get<std::int64_t>(v.lookup.value));};
 const auto floating=[&](const char* name){const auto& v=get(name,content::GlobalDefineKind::source_float);
  need(std::holds_alternative<double>(v.lookup.value),"Current growth native source-float Define unavailable");const auto n=std::get<double>(v.lookup.value);
  const auto f=static_cast<float>(n);need(std::isnan(n)||static_cast<double>(f)==n,"Current growth cached Define is not source binary32");return f;};
 r.base=integer("BASE_CITY_GROWTH_THRESHOLD");r.minor_percent=integer("MINOR_CIV_GROWTH_PERCENT");
 r.multiplier=floating("CITY_GROWTH_MULTIPLIER");r.exponent=floating("CITY_GROWTH_EXPONENT");
 const auto table=[&](const char* name,auto& out){const auto report=db.prefetch_info(name,[&](auto slot,auto authored,const auto& row){
  need(slot<=INT32_MAX&&authored==static_cast<std::int64_t>(slot),"Current growth Info native logical slot unavailable");
  const auto type=row.text("Type");need(type&&!type->empty(),"Current growth Info Type unavailable");
  const auto hash=content::saved_info_type_hash(*type);const auto* info=host.types().find(*type);const auto* wire=host.types().find_hash(hash);
  need(info&&wire&&info->table==name&&info->id==authored&&wire->type==*type&&wire->table==name&&wire->id==authored,
   "Current growth activated Info slot/Type/hash differs");
  const bool handicap=std::string_view(name)=="HandicapInfos";
  Rules::Info value{row.integer(handicap?"AIGrowthPercent":"GrowthPercent"),
   handicap?row.integer("AIPerEraModifier"):0};
  need(out.emplace(static_cast<std::int32_t>(slot),value).second,"Current growth duplicate Info slot");return true;
 });need(report.all_succeeded()&&!report.holes&&!report.overlapping_ids,"Current growth immutable Info span unavailable");};
 table("GameSpeeds",r.speeds);table("Eras",r.eras);table("HandicapInfos",r.handicaps);return r;
}
}
std::optional<std::int32_t> detail::bnw_growth_saved_pitboss_option(
 std::span<const saves::PregameOption> options){
 constexpr std::string_view requested="GAMEOPTION_PITBOSS";
 // Native CustomOption::Read copies FString.c_str() into 64 bytes. Preserve
 // first-NUL truncation; an overlong prefix needs the unavailable CRT invalid
 // parameter branch, never an invented normalized/empty option. Validate the
 // complete reconstructed Read input before executing first-match lookup.
 for(const auto& option:options){
  const auto n=std::string_view(option.name).substr(0,option.name.find('\0')).size();
  need(n<64,"Current growth CustomOption native Read name exceeds 63 bytes");
  (void)native(option.value); // Native Read stores int32, including nonmatching rows.
 }
 for(const auto& option:options){
  const auto saved=std::string_view(option.name).substr(0,option.name.find('\0'));
  // strncmp(saved,requested,strlen(saved)): empty/prefix keys can match; first
  // vector match wins. Longer names mismatch at requested's terminating NUL.
  if(saved.size()<=requested.size()&&saved==requested.substr(0,saved.size()))return native(option.value);
 }
 return std::nullopt;
}
struct BnwCityCurrentGrowthThreshold::Impl {
 const std::thread::id thread=std::this_thread::get_id();
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host;
 std::shared_ptr<const LiveSession> session;
 std::shared_ptr<const content::RulesDatabase> database;
 std::shared_ptr<const LiveSessionState> state;
 const saves::SavedSession* source=nullptr;std::uint64_t generation=0;
 SessionCheckpointIdentity identity;std::function<bool()> rules_current;Rules rules;
 std::int32_t owner=-1,city=-1;mutable std::atomic<bool> retired{false};
};
BnwCityCurrentGrowthThreshold::BnwCityCurrentGrowthThreshold(std::shared_ptr<Impl> p)noexcept:impl_(std::move(p)){}
void BnwCityCurrentGrowthThreshold::require_current()const{
 const auto& p=impl_;need(p&&p->thread==std::this_thread::get_id(),"Current growth query foreign constructor thread/moved handle");
 try{
  need(!p->retired.load()&&p->host&&p->session&&p->database&&p->rules_current&&p->rules_current(),"Current growth genuine immutable rules/host retired");
  need(p->database->gameplay_fingerprint_sha256()==p->identity.rules_sha256&&
   p->host->identity().rules_sha256==p->identity.rules_sha256&&p->host->identity().payload_sha256==p->identity.payload_sha256,
   "Current growth actual host/DB identity differs");
  (void)p->session->bnw_game_debug_mode(); // Actual constructor-thread/Game guard BEFORE state/source.
  need(&p->host->session()==p->session.get() && &p->host->database()==p->database.get()&&
   p->session->read_state_lease()==p->state&&p->session->state_generation()==p->generation && &p->session->source()==p->source,
   "Current growth actual session allocation/source/generation retired");
 }catch(...){p->retired.store(true);throw;}
}
std::int32_t BnwCityCurrentGrowthThreshold::value()const{return typed([&]{
 require_current();auto& p=*impl_;try{
  const auto& s=*p.state;const auto* raw=resolve_bnw_current_city(s,p.owner,p.city);need(raw,"Current growth actual current city absent");
  need(city_scalar(*raw,"m_iID")==p.city&&city_scalar(*raw,"m_eOwner")==p.owner,"Current growth canonical current city identity changed");
  const auto population=city_scalar(*raw,"m_iPopulation");
  const auto& owned=s.cities.players.at(p.owner);need(owned.cities.at(p.city).population==population,"Current growth raw/owned population differs");
  (void)player(s,p.owner);const auto& preg=pregame(s);const auto& flags=preg.archive.flag_arrays.at("minor_nation_civs");
  need(flags.size()==64,"Current growth actual minor native64 flags unavailable");const bool minor=flags[static_cast<std::size_t>(p.owner)];
  need(owned.minor==minor,"Current growth raw/owned minor differs");
  const auto speed=preg_scalar(preg,"game_speed"),era=preg_scalar(preg,"era");
  need(s.setup.speed==speed&&s.setup.start_era==era,"Current growth raw/owned speed/START era differs");
  BnwCityGrowthThresholdMathInputs input{population,p.rules.base,p.rules.multiplier,p.rules.exponent,minor,p.rules.minor_percent,
   native(p.rules.speeds.at(speed).growth),native(p.rules.eras.at(era).growth),false,false,false,0,0,0};
  input.human=human(s,p.owner,*p.database);
  if(!input.human){input.ai_teammate_of_human=ai_teammate(s,p.owner,*p.database);
   if(!input.ai_teammate_of_human){input.barbarian=p.owner==63;
    if(!input.barbarian){const auto handicap=native(named(s.world.game_state.controls,"handicap").value);
     need(s.setup.game_handicap==handicap,"Current growth raw/owned GAME handicap differs");
     const auto& h=p.rules.handicaps.at(handicap);input.game_handicap_ai_growth_percent=native(h.growth);
     input.game_handicap_ai_per_era_modifier=native(h.per_era);input.player_current_era=team_era(s,p.owner);
   }
  }}
  const auto result=bnw_city_growth_threshold_math(input);require_current();return result;
 }catch(...){p.retired.store(true);throw;}
});}
BnwCityCurrentGrowthThreshold inspect_bnw_city_current_growth_threshold(
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host,std::int64_t owner,std::int64_t city){return typed([&]{
 need(static_cast<bool>(host),"Current growth actual selected host absent");
 (void)host->session().bnw_game_debug_mode(); // BEFORE any actual state/source borrow.
 auto p=std::make_shared<BnwCityCurrentGrowthThreshold::Impl>();p->host=std::move(host);
 p->session=std::shared_ptr<const LiveSession>(p->host,&p->host->session());
 p->database=std::shared_ptr<const content::RulesDatabase>(p->host,&p->host->database());
 p->rules_current=p->database->immutable_preview_fingerprint_currentness();
 need(p->rules_current&&p->rules_current(),"Current growth actual rules must be immutable activated scope");
 p->identity=p->host->identity();need(!p->identity.payload_sha256.empty()&&!p->identity.rules_sha256.empty()&&
  p->database->gameplay_fingerprint_sha256()==p->identity.rules_sha256,"Current growth actual host rules identity absent/mismatched");
 p->state=p->session->read_state_lease();p->source=&p->session->source();p->generation=p->session->state_generation();
 need(p->state&&identify_bytes(p->source->source.bytes).sha256==p->identity.payload_sha256,"Current growth actual source payload differs");
 p->rules=load_rules(*p->host);p->owner=native(owner);const auto* raw=resolve_bnw_current_city(*p->state,p->owner,native(city));
 need(raw,"Current growth actual current city absent");p->city=city_scalar(*raw,"m_iID");
 BnwCityCurrentGrowthThreshold result(std::move(p));(void)result.value();return result;
});}
}
