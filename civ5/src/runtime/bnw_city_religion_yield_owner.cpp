#include "civ5/runtime/bnw_city_religion_yield_owner.hpp"
#include "civ5/runtime/bnw_city_entry.hpp"
#include "civ5/runtime/bnw_city_food_growth_prefix.hpp"
#include "civ5/runtime/bnw_city_strategy_yields.hpp"
#include "civ5/runtime/bnw_city_specialist_counts.hpp"
#include "civ5/runtime/bnw_happiness_buildings.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <algorithm>
#include <atomic>
#include <climits>
#include <functional>
#include <map>
#include <set>
#include <thread>
#include <type_traits>
namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void need(bool b,const char* why){if(!b)throw BnwCityReligionYieldUnavailable(why);}
std::int32_t native(std::int64_t v){need(v>=INT32_MIN&&v<=INT32_MAX,"City religion reached native int32 overflow");return static_cast<std::int32_t>(v);}
std::int32_t add(std::int32_t a,std::int32_t b){return native(std::int64_t(a)+b);}
const auto& named(const auto& rows,std::string_view name){
 using T=typename std::remove_cvref_t<decltype(rows)>::value_type;const T* out=nullptr;
 for(const auto& r:rows)if(r.name&&std::string_view(r.name)==name){need(!out,"City religion duplicate source field");out=&r;}
 need(out,"City religion required source field absent");return *out;
}
std::int32_t scalar(const saves::SavedCityObservation& city,std::string_view name){
 const auto& f=named(city.core.fields,name);
 need(city.core.layout==bnw&&f.encoding==saves::CityCoreEncoding::i32&&f.values.size()==1&&!f.wire_count&&
  f.hashes.empty()&&f.bytes.empty()&&f.row_keys.empty()&&f.row_counts.empty(),"City religion current scalar shape unavailable");return native(f.values.front());
}
const auto& vector(const saves::SavedCityObservation& city,std::string_view name){
 const auto& f=named(city.core.fields,name);
 need(city.core.layout==bnw&&f.encoding==saves::CityCoreEncoding::vector_i32&&f.values.size()==6&&f.wire_count==6&&
  f.hashes.empty()&&f.bytes.empty()&&f.row_keys.empty()&&f.row_counts.empty(),"City religion current six-yield shape unavailable");return f.values;
}
std::int32_t record_scalar(const auto& rows,std::string_view name){
 const auto& f=named(rows,name);
 need(f.encoding&&std::string_view(f.encoding)=="i32"&&f.values.size()==1&&!f.wire_count&&f.children.empty(),
  "City religion status scalar shape unavailable");return native(f.values.front());
}
std::vector<CityYieldReligion> status(const saves::SavedCityObservation& raw,const CityYieldState& owned){
 need(raw.tail.layout==bnw,"City religion status layout unavailable");
 const auto& rows=named(named(raw.tail.blocks,"religions").children,"m_ReligionStatus");
 need(rows.wire_count&&rows.encoding&&std::string_view(rows.encoding)=="signed_records"&&
  static_cast<std::uint64_t>(std::max(0,native(*rows.wire_count)))==rows.children.size()&&rows.children.size()==owned.religions.size(),
  "City religion current raw/owned status count differs");
 std::vector<CityYieldReligion> result;result.reserve(rows.children.size());
 for(std::size_t i=0;i<rows.children.size();++i){const auto& fields=rows.children[i].children;
  CityYieldReligion r{record_scalar(fields,"m_eReligion"),record_scalar(fields,"m_iFollowers"),record_scalar(fields,"m_iPressure")};
  const auto& o=owned.religions[i];need(r.religion==o.religion&&r.followers==o.followers&&r.pressure==o.pressure,
   "City religion current status mirror stale");result.push_back(r);
 }return result;
}
std::int32_t majority(std::span<const CityYieldReligion> rows){
 std::int32_t total=0,most=-1,pressure=0,id=-1;
 for(const auto& r:rows){const auto f=native(r.followers),p=native(r.pressure);total=add(total,f);
  if(f>most||(f==most&&p>pressure)){most=f;pressure=p;id=native(r.religion);}}
 return native(std::int64_t(most)*2)>=total?id:-1;
}
bool equal_status(std::span<const CityYieldReligion> a,std::span<const CityYieldReligion> b){
 if(a.size()!=b.size())return false;
 for(std::size_t i=0;i<a.size();++i)if(a[i].religion!=b[i].religion||a[i].followers!=b[i].followers||a[i].pressure!=b[i].pressure)return false;
 return true;
}
struct Belief {
 bool pantheon=false;std::int32_t population=0,followers=0;
 std::array<std::int32_t,6> city{},route{},specialist{},wonder{};
 std::map<std::int32_t,std::array<std::int32_t,6>> building;
};
struct Rules {
 std::map<std::int32_t,Belief> beliefs;
 std::map<std::int32_t,bool> classes;
 std::uint64_t belief_slots=0,class_slots=0,building_slots=0;
 CityYieldRules replacements;BnwBuildingHappinessRules counts;
};
Rules load_rules(const ui::SelectedBnwPreviewHost& host){
 const auto& db=host.database();Rules r;
 const auto info=[&](const char* table,auto&& visit){const auto report=db.prefetch_info(table,[&](auto slot,auto authored,const auto& row){
  need(slot<=INT32_MAX&&authored==static_cast<std::int64_t>(slot),"City religion native Info slot unavailable");
  const auto type=row.text("Type");const auto* by_type=type?host.types().find(*type):nullptr;
  const auto hash=type?content::saved_info_type_hash(*type):0;const auto* by_hash=host.types().find_hash(hash);
   need(type&&!type->empty()&&type->find('\0')==std::string::npos&&hash&&by_type&&by_hash&&by_type->table==table&&by_type->type==*type&&by_type->id==authored&&
    by_hash->table==table&&by_hash->type==*type&&by_hash->id==authored&&by_type->hash==hash&&by_hash->hash==hash,
   "City religion actual native Info Type/hash/slot differs");return visit(static_cast<std::int32_t>(slot),*type,row);
 });need(report.all_succeeded()&&!report.overlapping_ids&&report.slots<=INT32_MAX&&report.rows+report.holes==report.slots,
  "City religion native Info collection unavailable");return report.slots;};
 const auto yields=info("Yields",[](auto slot,auto,const auto&){need(slot>=0&&slot<6,"City religion native Yield enum differs");return true;});
 need(yields==6,"City religion actual six Yield slots unavailable");
 r.class_slots=info("BuildingClasses",[&](auto id,auto,const auto& row){r.classes.emplace(id,native(row.integer("MaxGlobalInstances"))!=-1);return true;});
 r.building_slots=info("Buildings",[](auto,auto,const auto&){return true;});
  (void)info("Civilizations",[](auto,auto,const auto&){return true;});
 // Existing native civilization DEFAULT then override SET implementation.
 r.replacements.profile=content::TechnologyProfile::bnw;r.replacements.building_slots=r.building_slots;r.replacements.building_class_slots=r.class_slots;
 load_city_building_replacements(db,r.replacements);r.counts=BnwBuildingHappinessRules::load(db);
 const auto query=[&](content::DatabaseStatement statement,auto&& visit){const auto report=db.query_rows(statement,visit);
  need(report.all_succeeded(),"City religion native authored array query failed");};
 const auto max_rows=[&](const char* table,std::uint64_t expected){std::optional<std::int32_t> n;
  query({std::string("SELECT max(rowid) from ")+table,{}},[&](const auto& row){n=add(native(row.integer_at(0)),1);});
  need(n&&*n>=0&&static_cast<std::uint64_t>(*n)==expected,"City religion native MaxRows/cache span mismatch");};
 max_rows("Yields",6);max_rows("BuildingClasses",r.class_slots);
 r.belief_slots=info("Beliefs",[&](auto id,const auto& type,const auto& row){
  Belief b;b.pantheon=row.boolean("Pantheon");b.population=native(row.integer("MinPopulation"));b.followers=native(row.integer("MinFollowers"));
  const auto array=[&](const char* table,auto& target){
   // Native SetYields: InitializeArray(0), then SET each returned row, in DB order.
   query({std::string("select Yields.ID, Yield from ")+table+" inner join Yields on YieldType = Yields.Type where BeliefType = ?",{type}},[&](const auto& value){
    const auto yi=native(value.integer_at(0));need(yi>=0&&yi<6,"City religion authored Yield native index unavailable");target[static_cast<std::size_t>(yi)]=native(value.integer_at(1));});
  };
  array("Belief_CityYieldChanges",b.city);array("Belief_YieldChangeTradeRoute",b.route);
  array("Belief_YieldChangeAnySpecialist",b.specialist);array("Belief_YieldChangeWorldWonder",b.wonder);
  query({"select BuildingClasses.ID as BuildingClassID, Yields.ID as YieldID, YieldChange from Belief_BuildingClassYieldChanges inner join BuildingClasses on BuildingClasses.Type = BuildingClassType inner join Yields on Yields.Type = YieldType where BeliefType = ?",{type}},[&](const auto& value){
   const auto klass=native(value.integer_at(0)),yi=native(value.integer_at(1));
   need(klass>=0&&static_cast<std::uint64_t>(klass)<r.class_slots&&yi>=0&&yi<6,"City religion native BuildingClass/Yield array index unavailable");
   b.building[klass][static_cast<std::size_t>(yi)]=native(value.integer_at(2));});
  need(r.beliefs.emplace(id,std::move(b)).second,"City religion duplicate Belief native slot");return true;
 });return r;
}
const saves::ReligionObservation* religion(const LiveSessionState& s,std::int32_t id,std::int32_t owner){
 const auto& g=s.world.religion_culture;
 need(g.layout==bnw&&g.religions_version&&*g.religions_version>=2&&*g.religions_version<=4&&g.start_offset<g.culture_offset&&
  g.culture_offset<=g.end_offset&&s.cities.religions.size()==g.religions.size(),"City religion actual global registry unavailable");
 for(std::size_t i=0;i<g.religions.size();++i){const auto& r=g.religions[i];const auto& o=s.cities.religions[i];
  need(o.religion==native(r.religion),"City religion global ID mirror stale");
  if(r.religion==id){if(id==0){need(o.founder==native(r.founder),"City religion pantheon founder mirror stale");if(r.founder!=owner)continue;}return &r;}
 }return nullptr;
}
std::vector<std::int32_t> belief_read(const saves::ReligionObservation& raw,const LiveSessionState& s,
 const content::InfoTypeCatalog& types,const Rules& rules){
 need(raw.beliefs.version&&*raw.beliefs.version>=1&&*raw.beliefs.version<=2,"City religion hashed belief Read version unavailable");
 std::vector<std::int32_t> values;std::vector<std::int64_t> expected;
 for(const auto h:raw.beliefs.belief_hashes){need(h<=UINT32_MAX,"City religion belief hash exceeds native width");const auto* t=h?types.find_hash(h):nullptr;
  need(!t||t->table=="Beliefs","City religion native cross-table belief remap unavailable");const auto id=t?native(t->id):-1;
  if(t){const auto* actual=types.find(t->type);need(actual&&actual->table==t->table&&actual->id==t->id&&actual->hash==h&&content::saved_info_type_hash(t->type)==h,
   "City religion global belief hash collision unavailable");}
  values.push_back(id);if(id>=0&&static_cast<std::uint64_t>(id)<rules.belief_slots)expected.push_back(id);
 }
 const auto index=static_cast<std::size_t>(&raw-s.world.religion_culture.religions.data());
 need(expected==s.cities.religions.at(index).beliefs,"City religion membership raw/owned mirror stale");return values;
}
std::int32_t player_scalar(const LiveSessionState& s,std::int32_t owner,std::string_view name){
 need(owner>=0&&owner<64&&s.world.all_players&&s.world.all_players->layout==bnw&&s.world.all_players->players.size()==64,
  "City religion actual native player roster unavailable");const auto& p=s.world.all_players->players[static_cast<std::size_t>(owner)];
 need(p.index==static_cast<std::uint64_t>(owner)&&p.saved_player_id==owner&&p.core.layout==bnw,"City religion actual player identity unavailable");
 const auto& f=named(p.core.fields,name);need(f.encoding==saves::PlayerCoreEncoding::i32&&f.values.size()==1&&!f.wire_count&&f.hashes.empty()&&f.bytes.empty()&&f.row_counts.empty(),
  "City religion reached player scalar shape unavailable");return native(f.values.front());
}
std::int32_t secondary(const LiveSessionState& s,std::int32_t owner,std::span<const CityYieldReligion> rows,
 std::int32_t major,const ui::SelectedBnwPreviewHost& host,const Rules& rules){
 const auto count=player_scalar(s,owner,"m_iSecondReligionPantheonCount");const auto& owned=s.cities.players.at(owner).second_pantheon_count;
 if(owned)need(*owned==count,"City religion secondary pantheon counter mirror stale");
 if(count<=0)return -1;
 std::int32_t id=-1,most=-1,pressure=-1;
 if(major!=-1)for(const auto& r:rows)if(r.religion!=major&&r.religion>0&&r.followers>0){const auto f=native(r.followers),p=native(r.pressure);
  if(f>most||(f==most&&p>pressure)){id=native(r.religion);most=f;pressure=p;}}
 if(id==-1)return -1;
 const auto* selected=religion(s,id,owner);if(!selected)return -1;
 for(const auto belief:belief_read(*selected,s,host.types(),rules)){
  need(belief>=0&&static_cast<std::uint64_t>(belief)<rules.belief_slots,"City religion secondary reached native unsafe Belief index");
  const auto p=rules.beliefs.find(belief);if(p!=rules.beliefs.end()&&p->second.pantheon)return belief;
 }return -1;
}
const auto& building_rows(const saves::SavedCityObservation& c,std::string_view name){
 need(c.production.layout==bnw,"City religion Buildings block layout unavailable");return named(named(c.production.blocks,"buildings").children,name);
}
std::int32_t building_count(const saves::SavedCityObservation& raw,const CityYieldState& city,
 const Rules& rules,const content::InfoTypeCatalog& types,std::int32_t id){
 need(id>=0&&static_cast<std::uint64_t>(id)<rules.building_slots,"City religion civilization Building native index unavailable");
 const auto real=detail::bnw_native_building_counts_reset_read_math(building_rows(raw,"m_paiNumRealBuilding"),rules.building_slots,types);
 const auto free=detail::bnw_native_building_counts_reset_read_math(building_rows(raw,"m_paiNumFreeBuilding"),rules.building_slots,types);
 need(city.building_counts_resolved,"City religion current owned Building counts unresolved");
 const auto mirror=[&](const YieldValues& values,const auto& dense){for(std::size_t i=0;i<dense.size();++i){const auto p=values.find(static_cast<std::int64_t>(i));
   need((p==values.end()?0:p->second)==dense[i],"City religion current Building count mirror stale");}};
 mirror(city.real_buildings,real);mirror(city.free_buildings,free);
 return rules.counts.building_count(real.at(static_cast<std::size_t>(id)),free.at(static_cast<std::size_t>(id)));
}
}
std::int32_t detail::bnw_city_religion_sum_math(std::span<const std::int32_t> values){std::int32_t n=0;for(const auto value:values)n=add(n,value);return n;}
std::int32_t detail::bnw_city_religion_centi_math(std::span<const CityYieldReligion> rows,std::int32_t centi){
 if(centi<=0)return 0;
 std::int32_t count=0;
 for(const auto& row:rows)if(native(row.followers)>0&&native(row.religion)>0)count=add(count,1);
 return native(std::int64_t(count)*centi)/100;
}
struct BnwCityReligionYieldOwner::Impl {
 std::thread::id thread=std::this_thread::get_id();std::weak_ptr<const ui::SelectedBnwPreviewHost> host;
 std::shared_ptr<const LiveSessionState> state;std::shared_ptr<const saves::SavedSession> source_owner;const saves::SavedSession* source=nullptr;
 const BnwCityEntryPending* parent=nullptr;const BnwCityFoodGrowthPending* population_parent=nullptr;std::uint64_t generation=0,serial=0;std::int32_t owner=-1,city=-1,majority=-1;
 SessionCheckpointIdentity identity;std::function<bool()> rules_current;Rules rules;Cursor cursor;
 std::vector<CityYieldReligion> entry_status;std::atomic<bool> retired{false};
};
BnwCityReligionYieldOwner::BnwCityReligionYieldOwner(std::unique_ptr<Impl> p)noexcept:impl_(std::move(p)){}
BnwCityReligionYieldOwner::~BnwCityReligionYieldOwner()=default;
void BnwCityReligionYieldOwner::retire()noexcept{if(impl_)impl_->retired.store(true);}
template<class Parent>void BnwCityReligionYieldOwner::require_current_bound(const Parent& parent,
 const std::shared_ptr<const ui::SelectedBnwPreviewHost>& host)const{
 need(impl_&&impl_->thread==std::this_thread::get_id(),"City religion foreign owner thread/moved handle");auto& p=*impl_;
 try{need(!p.retired.load()&&host&&p.host.lock()==host&&((std::is_same_v<Parent,BnwCityEntryPending>&&static_cast<const void*>(p.parent)==&parent)||
    (std::is_same_v<Parent,BnwCityFoodGrowthPending>&&static_cast<const void*>(p.population_parent)==&parent))&&p.rules_current&&p.rules_current(),
   "City religion actual parent/immutable RulesImpl retired");
  need(host->identity().rules_sha256==p.identity.rules_sha256&&host->identity().payload_sha256==p.identity.payload_sha256&&
   host->database().gameplay_fingerprint_sha256()==p.identity.rules_sha256,"City religion SAME Host rules/source identity differs");
  (void)host->session().bnw_game_debug_mode();
  need(host->session().read_state_lease()==p.state&&host->session().state_generation()==p.generation&&&host->session().source()==p.source,
   "City religion actual source allocation/generation retired");parent.require_current();
  const auto* raw=resolve_bnw_current_city(*p.state,p.owner,p.city);need(raw,"City religion current canonical city absent");
  const auto& owned=p.state->cities.players.at(p.owner).cities.at(p.city);
  need(scalar(*raw,"m_iID")==p.city&&scalar(*raw,"m_eOwner")==p.owner&&equal_status(status(*raw,owned),p.entry_status),
   "City religion original majority argument/status source changed");
  const auto& base=vector(*raw,"m_aiBaseYieldRateFromReligion");for(std::size_t yi=0;yi<6;++yi){const auto o=owned.yields.find(static_cast<std::int64_t>(yi));
   need(native(base[yi])==(o==owned.yields.end()?0:o->second.religion),"City religion current raw/owned base mirror stale");}
 }catch(...){p.retired.store(true);throw;}
}
std::int32_t BnwCityReligionYieldOwner::capture_majority_for_entry(const BnwCityEntryPending& parent,
 const std::shared_ptr<const ui::SelectedBnwPreviewHost>& host,
 const std::shared_ptr<const LiveSessionState>& state,
 const std::shared_ptr<const saves::SavedSession>& source){
 need(bool(host),"City religion majority needs actual Host");
 (void)host->session().bnw_game_debug_mode();
 const auto guard=host->database().immutable_preview_fingerprint_currentness();
 need(guard&&guard(),"City religion majority rules retired before source borrow");
 parent.require_current();
 need(state&&source&&host->session().read_state_lease()==state&&&host->session().source()==source.get(),
  "City religion majority needs SAME actual parent allocation");
 const auto progress=parent.progress();const auto* raw=resolve_bnw_current_city(*state,progress.owner,progress.city);
 need(raw,"City religion majority actual city absent");
 const auto result=majority(status(*raw,state->cities.players.at(progress.owner).cities.at(progress.city)));
 need(guard(),"City religion majority rules retired after native calculation");parent.require_current();return result;
}
std::unique_ptr<BnwCityReligionYieldOwner> BnwCityReligionYieldOwner::after_city_yield_child(
 const BnwCityEntryPending& parent,std::shared_ptr<const ui::SelectedBnwPreviewHost> host,
 const std::shared_ptr<const LiveSessionState>& parent_state,
 const std::shared_ptr<const saves::SavedSession>& parent_source,std::int32_t retained_majority){
 need(bool(host),"City religion actual Host absent");
 (void)host->session().bnw_game_debug_mode(); // Actual constructor thread; no world borrow.
 auto p=std::make_unique<Impl>();p->rules_current=host->database().immutable_preview_fingerprint_currentness();
 need(p->rules_current&&p->rules_current(),"City religion actual immutable rules required before source borrow");
 (void)host->session().bnw_game_debug_mode();parent.require_current();const auto progress=parent.progress();
 p->host=host;p->parent=&parent;p->identity=host->identity();p->owner=progress.owner;p->city=progress.city;
 need(parent_state&&parent_source&&host->session().read_state_lease()==parent_state&&&host->session().source()==parent_source.get(),
  "City religion actual parent source allocation differs from Host");
 p->state=parent_state;p->source_owner=parent_source;p->source=parent_source.get();p->generation=host->session().state_generation();
 need(p->state&&p->state->setup.profile==content::TechnologyProfile::bnw,"City religion current BNW state unavailable");
 const auto* raw=resolve_bnw_current_city(*p->state,p->owner,p->city);need(raw,"City religion current canonical city absent");
 p->entry_status=status(*raw,p->state->cities.players.at(p->owner).cities.at(p->city));p->majority=retained_majority;
 need(majority(p->entry_status)==retained_majority,"City religion pre-updateYield argument/source seal differs");
 p->rules=load_rules(*host);auto result=std::unique_ptr<BnwCityReligionYieldOwner>(new BnwCityReligionYieldOwner(std::move(p)));
 result->require_current(parent,host);return result;
}
std::int32_t BnwCityReligionYieldOwner::capture_majority_for_population(const BnwCityFoodGrowthPending& parent,
 const std::shared_ptr<const ui::SelectedBnwPreviewHost>& host,
 const std::shared_ptr<const LiveSessionState>& state,
 const std::shared_ptr<const saves::SavedSession>& source){
 need(bool(host),"City religion majority needs actual Host");
 (void)host->session().bnw_game_debug_mode();
 const auto guard=host->database().immutable_preview_fingerprint_currentness();
 need(guard&&guard(),"City religion majority rules retired before source borrow");
 parent.require_current();
 need(state&&source&&host->session().read_state_lease()==state&&&host->session().source()==source.get(),
  "City religion majority needs SAME actual parent allocation");
 const auto progress=parent.progress();const auto* raw=resolve_bnw_current_city(*state,progress.owner,progress.city);
 need(raw,"City religion majority actual city absent");
 const auto result=majority(status(*raw,state->cities.players.at(progress.owner).cities.at(progress.city)));
 need(guard(),"City religion majority rules retired after native calculation");parent.require_current();return result;
}
std::unique_ptr<BnwCityReligionYieldOwner> BnwCityReligionYieldOwner::after_population_city_yield_child(
 const BnwCityFoodGrowthPending& parent,std::shared_ptr<const ui::SelectedBnwPreviewHost> host,
 const std::shared_ptr<const LiveSessionState>& parent_state,
 const std::shared_ptr<const saves::SavedSession>& parent_source,std::int32_t retained_majority){
 parent.require_population_religion_return();
 need(bool(host),"City religion actual Host absent");
 (void)host->session().bnw_game_debug_mode(); // Actual constructor thread; no world borrow.
 auto p=std::make_unique<Impl>();p->rules_current=host->database().immutable_preview_fingerprint_currentness();
 need(p->rules_current&&p->rules_current(),"City religion actual immutable rules required before source borrow");
 (void)host->session().bnw_game_debug_mode();parent.require_current();const auto progress=parent.progress();
 p->host=host;p->population_parent=&parent;p->identity=host->identity();p->owner=progress.owner;p->city=progress.city;
 need(parent_state&&parent_source&&host->session().read_state_lease()==parent_state&&&host->session().source()==parent_source.get(),
  "City religion actual parent source allocation differs from Host");
 p->state=parent_state;p->source_owner=parent_source;p->source=parent_source.get();p->generation=host->session().state_generation();
 need(p->state&&p->state->setup.profile==content::TechnologyProfile::bnw,"City religion current BNW state unavailable");
 const auto* raw=resolve_bnw_current_city(*p->state,p->owner,p->city);need(raw,"City religion current canonical city absent");
 p->entry_status=status(*raw,p->state->cities.players.at(p->owner).cities.at(p->city));p->majority=retained_majority;
 need(majority(p->entry_status)==retained_majority,"City religion pre-updateYield argument/source seal differs");
 p->rules=load_rules(*host);auto result=std::unique_ptr<BnwCityReligionYieldOwner>(new BnwCityReligionYieldOwner(std::move(p)));
 result->require_current(parent,host);return result;
}
bool BnwCityReligionYieldOwner::player_happiness_tail_reached()const noexcept {
 return impl_&&impl_->cursor.yield==6&&impl_->cursor.phase==6;
}
void BnwCityReligionYieldOwner::require_player_happiness_tail(const BnwCityEntryPending& parent,
 const std::shared_ptr<const ui::SelectedBnwPreviewHost>& host)const {
 require_current(parent,host);
 need(player_happiness_tail_reached(),"Player happiness handoff lacks actual six-yield religion predecessor");
}
void BnwCityReligionYieldOwner::require_player_happiness_tail(const BnwCityFoodGrowthPending& parent,
 const std::shared_ptr<const ui::SelectedBnwPreviewHost>& host)const {
 require_current(parent,host);
 need(player_happiness_tail_reached(),"Player happiness handoff lacks actual six-yield religion predecessor");
}
bool BnwCityReligionYieldOwner::needs_specialist_total()const noexcept{return impl_&&impl_->cursor.phase==9;}
template<class Parent>BnwCityReligionYieldOwner::Step BnwCityReligionYieldOwner::prepare_next_bound(const Parent& parent,
 const std::shared_ptr<const ui::SelectedBnwPreviewHost>& host,const BnwCitySpecialistCounts* counts)const{
 require_current(parent,host);const auto& p=*impl_;need(p.serial<UINT64_MAX,"City religion step ledger exhausted");
 const auto& s=*p.state;const auto* raw=resolve_bnw_current_city(s,p.owner,p.city);need(raw,"City religion current canonical city absent");
 const auto& city=s.cities.players.at(p.owner).cities.at(p.city);auto c=p.cursor;
 const auto step=[&](std::string_view field,std::int8_t yi,std::int32_t delta,bool reset,Cursor after){
  const auto old=yi<0?scalar(*raw,field):native(vector(*raw,field)[static_cast<std::size_t>(yi)]);
  const auto next=reset?0:add(old,delta);require_current(parent,host);
  return Step{impl_.get(),p.serial,after,field,yi,old,next,reset||delta!=0,!reset&&yi>=0&&delta!=0};
 };
 if(c.phase==0){c.phase=1;return step("m_iJONSCulturePerTurnFromReligion",-1,0,true,c);}
 if(c.phase==1){c.phase=2;return step("m_iFaithPerTurnFromReligion",-1,0,true,c);}
 if(c.phase>=2&&c.phase<=5){const auto yi=static_cast<std::int8_t>(c.phase-2);++c.phase;return step("m_aiBaseYieldRateFromReligion",yi,0,true,c);}
 const auto skip=[&](Cursor after){require_current(parent,host);return Step{impl_.get(),p.serial,after,{},-1,0,0,false,false};};
 const auto change=[&](std::int32_t delta,Cursor after){const auto yi=static_cast<std::int8_t>(c.yield);
  if(yi==4)return step("m_iJONSCulturePerTurnFromReligion",-1,delta,false,after);
  if(yi==5)return step("m_iFaithPerTurnFromReligion",-1,delta,false,after);
  return step("m_aiBaseYieldRateFromReligion",yi,delta,false,after);
 };
 while(c.yield<6){
  const auto yi=static_cast<std::size_t>(c.yield);
  if(c.phase==6){const auto per=native(vector(*raw,"m_aiYieldPerReligion")[yi]);const auto it=city.yields.find(c.yield);
   need(per==(it==city.yields.end()?0:it->second.per_religion),"City religion perReligion centi mirror stale");c.phase=7;
   if(per>0)return change(detail::bnw_city_religion_centi_math(p.entry_status,per),c);
   return skip(c);
  }
  if(p.majority==-1){++c.yield;c.phase=6;return skip(c);}
  const auto* selected=religion(s,p.majority,p.owner);if(!selected){++c.yield;c.phase=6;return skip(c);}
  const auto native_beliefs=belief_read(*selected,s,host->types(),p.rules);
  const std::set<std::int32_t> membership(native_beliefs.begin(),native_beliefs.end());
  const auto sum=[&](auto&& value){std::int32_t n=0;for(const auto id:membership){if(id<0||static_cast<std::uint64_t>(id)>=p.rules.belief_slots)continue;
    const auto entry=p.rules.beliefs.find(id);need(entry!=p.rules.beliefs.end(),"City religion majority dereferences null native BeliefInfo");n=add(n,value(entry->second));}return n;};
  if(c.phase==7){c.followers=0;
   for(const auto& row:p.entry_status)if(row.religion==p.majority){c.followers=native(row.followers);break;}
   const auto pop=scalar(*raw,"m_iPopulation");need(pop==city.population,"City religion population mirror stale");
   const auto value=sum([&](const Belief& b){return pop>=b.population?b.city[yi]:0;});
   const auto sec=secondary(s,p.owner,p.entry_status,p.majority,*host,p.rules);c.secondary=sec;auto n=value;
   if(sec!=-1){const auto& b=p.rules.beliefs.at(sec);if(pop>=b.population)n=add(n,b.city[yi]);}
   c.phase=8;return change(n,c);
  }
  if(c.phase==8){need(raw->production.layout==bnw,"City religion route production layout unavailable");
   const auto route=native(named(named(raw->production.blocks,"before_strategy").fields,"m_bRouteToCapitalConnectedThisTurn").value);
   need(route==0||route==1,"City religion native route bool unavailable");c.phase=9;
   if(route){auto n=sum([&](const Belief& b){return b.route[yi];});const auto sec=c.secondary;
    if(sec!=-1)n=add(n,p.rules.beliefs.at(sec).route[yi]);
    return change(n,c);}
   return skip(c);
  }
  if(c.phase==9){need(counts,"City religion requires actual closed specialist total owner");counts->require_current_source();counts->require_same_world_database(s.world,host->database());
   const auto n=counts->total_for_city(p.owner,p.city);c.phase=10;c.building_class=0;
   if(n>0)return change(sum([&](const Belief& b){return b.specialist[yi];}),c);
   return skip(c);
  }
  need(c.phase==10,"City religion native source cursor invalid");
  while(static_cast<std::uint64_t>(c.building_class)<p.rules.class_slots){const auto klass=c.building_class++;const auto info=p.rules.classes.find(klass);
   if(info==p.rules.classes.end())continue;
   need(s.world.pregame&&s.world.pregame->layout==bnw,"City religion actual civilization source unavailable");
   const auto& slots=s.world.pregame->slots.integer_arrays.at("civilizations");need(slots.size()==64&&p.owner>=0&&p.owner<64,"City religion native civilization slots unavailable");
   const auto civ=native(slots.at(static_cast<std::size_t>(p.owner)));need(civ==s.cities.players.at(p.owner).civilization,"City religion civilization mirror stale");
   const auto civ_info=p.rules.replacements.civilization_buildings.find(civ);need(civ_info!=p.rules.replacements.civilization_buildings.end(),"City religion actual civilization Info null");
   const auto replacement=civ_info->second.find(klass);const auto building=replacement==civ_info->second.end()?-1:native(replacement->second);
   if(building==-1)continue;
   if(building_count(*raw,city,p.rules,host->types(),building)<=0)continue;
   auto n=sum([&](const Belief& b){const auto row=b.building.find(klass);return c.followers>=b.followers&&row!=b.building.end()?row->second[yi]:0;});
   if(info->second)n=add(n,sum([&](const Belief& b){return b.wonder[yi];}));
   return change(n,c);
  }
  ++c.yield;c.phase=6;return skip(c);
 }
 // This is ALWAYS required; NO_RELIGION does not bypass player happiness.
 throw BnwCityReligionYieldUnavailable("City religion Player::UpdateReligion requires actual live DoUpdateHappiness/selected Lua epoch publication");
}
template<class Parent>void BnwCityReligionYieldOwner::validate_before_publication_bound(const Step& step,const Parent& parent,
 const std::shared_ptr<const ui::SelectedBnwPreviewHost>& host)const{
 require_current(parent,host);need(step.owner==impl_.get()&&step.serial==impl_->serial,"City religion prepared setter replay/cross-owner refusal");
 if(step.field.empty())return;
 const auto* raw=resolve_bnw_current_city(*impl_->state,impl_->owner,impl_->city);need(raw,"City religion current canonical city absent");
 const auto old=step.vector_yield<0?scalar(*raw,step.field):native(vector(*raw,step.field)[static_cast<std::size_t>(step.vector_yield)]);
 need(old==step.before,"City religion prepared setter beforeimage stale");
}
void BnwCityReligionYieldOwner::require_current(const BnwCityEntryPending& p,const std::shared_ptr<const ui::SelectedBnwPreviewHost>& h)const{require_current_bound(p,h);}
BnwCityReligionYieldOwner::Step BnwCityReligionYieldOwner::prepare_next(const BnwCityEntryPending& p,const std::shared_ptr<const ui::SelectedBnwPreviewHost>& h,const BnwCitySpecialistCounts* c)const{return prepare_next_bound(p,h,c);}
void BnwCityReligionYieldOwner::validate_before_publication(const Step& s,const BnwCityEntryPending& p,const std::shared_ptr<const ui::SelectedBnwPreviewHost>& h)const{validate_before_publication_bound(s,p,h);}
void BnwCityReligionYieldOwner::require_current(const BnwCityFoodGrowthPending& p,const std::shared_ptr<const ui::SelectedBnwPreviewHost>& h)const{require_current_bound(p,h);}
BnwCityReligionYieldOwner::Step BnwCityReligionYieldOwner::prepare_next(const BnwCityFoodGrowthPending& p,const std::shared_ptr<const ui::SelectedBnwPreviewHost>& h,const BnwCitySpecialistCounts* c)const{return prepare_next_bound(p,h,c);}
void BnwCityReligionYieldOwner::validate_before_publication(const Step& s,const BnwCityFoodGrowthPending& p,const std::shared_ptr<const ui::SelectedBnwPreviewHost>& h)const{validate_before_publication_bound(s,p,h);}
void BnwCityReligionYieldOwner::commit_published(const Step& step)noexcept{impl_->cursor=step.after;++impl_->serial;}
}
