#include "civ5/runtime/bnw_city_strategy_obsolete_end.hpp"
#include "civ5/runtime/bnw_city_current_strategy_eligibility.hpp"
#include "civ5/runtime/bnw_city_strategy_checkpoint_owner.hpp"
#include "civ5/runtime/bnw_city_strategy_yields.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <algorithm>
#include <atomic>
#include <climits>
#include <functional>
#include <map>
#include <string_view>
#include <thread>
#include <type_traits>
namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void need(bool b,const char* why){if(!b)throw BnwCityStrategyObsoleteEndUnavailable(why);}
std::int32_t native(std::int64_t n){need(n>=INT32_MIN&&n<=INT32_MAX,"Strategy effect reached native32 overflow");return static_cast<std::int32_t>(n);}
template<class F>auto typed(F&& f)->decltype(f()){
 try{return f();}catch(const BnwCityStrategyObsoleteEndUnavailable&){throw;}
 catch(const std::exception& e){throw BnwCityStrategyObsoleteEndUnavailable(e.what());}
}
auto& named(auto& rows,std::string_view name){
 using T=std::remove_reference_t<decltype(rows.front())>;T* out=nullptr;
 for(auto& r:rows)if(r.name&&name==r.name){need(!out,"Strategy effect duplicate named raw field");out=&r;}
 need(out,"Strategy effect mandatory raw field absent");return *out;
}
bool same(const saves::CivisSaveSection& a,const saves::CivisSaveSection& b){return a.id==b.id&&a.flags==b.flags&&a.bytes==b.bytes;}
std::int32_t city_scalar(const saves::SavedCityObservation& c,const char* name){
 const auto& f=named(c.core.fields,name);need(c.core.layout==bnw&&f.encoding==saves::CityCoreEncoding::i32&&f.values.size()==1&&!f.wire_count&&f.bytes.empty()&&f.hashes.empty()&&f.row_keys.empty()&&f.row_counts.empty(),"Strategy effect city scalar schema differs");return native(f.values.front());
}
struct Info {std::string type;std::uint32_t hash=0;};
std::vector<Info> infos(const ui::SelectedBnwPreviewHost& h,const char* table,bool native_holes=false){
 std::vector<Info> out;std::uint64_t holes=0,occupied=0;
 const auto report=h.database().prefetch_info(table,[&](auto slot,auto authored,const auto& row){
  need(slot<=INT32_MAX&&slot>=out.size()&&authored==static_cast<std::int64_t>(slot),"Strategy effect native Info slot/order differs");
  need(native_holes||slot==out.size(),"Strategy effect requires exact dense native Info slots");
  // Actual PrefetchCollection inserts NULL for every skipped ID. Keep those
  // native positions; only a real cache row may supply Type/flavor operands.
  holes+=slot-out.size();out.resize(static_cast<std::size_t>(slot));
  const auto type=row.text("Type");need(type&&!type->empty(),"Strategy effect actual Info Type absent");
  const auto hash=content::saved_info_type_hash(*type);const auto* t=h.types().find(*type);const auto* v=h.types().find_hash(hash);
  need(hash!=0&&t&&v&&t->table==table&&t->id==authored&&v->table==table&&v->id==authored&&v->type==*type,"Strategy effect SAME Type/hash/slot differs");out.push_back({*type,hash});++occupied;return true;
 });need(report.all_succeeded()&&!report.overlapping_ids&&report.slots==out.size()&&report.rows==occupied&&report.holes==holes&&(native_holes||holes==0),"Strategy effect native Info span/cache failure unsupported");return out;
}
std::vector<std::int32_t> flavors(const ui::SelectedBnwPreviewHost& h,const char* table,const char* filter,const std::string& type,std::size_t count){
 // CvDatabaseUtility::PopulateArrayByValue: zero-initialized actual cache,
 // exact SQL join and source Step order, assignment (not sum), last row wins.
 std::vector<std::int32_t> out(count,0);
 const std::string sql=std::string("select Flavors.ID, Flavor from ")+table+" inner join Flavors on FlavorType = Flavors.Type where "+filter+" = ?";
 const auto report=h.database().query_rows({sql,{type}},[&](const auto& row){const auto id=native(row.integer_at(0));need(id>=0&&static_cast<std::size_t>(id)<count,"Strategy effect linked flavor index outside actual cache");out.at(static_cast<std::size_t>(id))=native(row.integer_at(1));});
 need(report.all_succeeded(),"Strategy effect SetFlavors actual SQL cache failed");return out;
}
std::int32_t define(const ui::SelectedBnwPreviewHost& h,const char* name){
 const auto* c=h.defines().find(name);const auto r=h.database().global_define(name,content::GlobalDefineKind::integer);
 need(h.defines().profile()==content::GlobalDefineProfile::bnw&&c&&c->kind==content::GlobalDefineKind::integer&&c->lookup.error.empty()&&c->lookup.sqlite_code==0&&r.error.empty()&&r.sqlite_code==0&&std::holds_alternative<std::int64_t>(c->lookup.value)&&std::holds_alternative<std::int64_t>(r.value)&&c->lookup.found==r.found&&std::get<std::int64_t>(c->lookup.value)==std::get<std::int64_t>(r.value),"Strategy effect native cached/DB Define differs");return native(std::get<std::int64_t>(r.value));
}
std::int32_t obsolete(const ui::SelectedBnwPreviewHost& h,std::int32_t id){
 std::optional<std::int32_t> result;
 const auto report=h.database().prefetch_info("AICityStrategies",[&](auto slot,auto authored,const auto& row){
  need(authored==static_cast<std::int64_t>(slot),"Strategy effect obsolete Info mapping differs");
  if(authored==id){const auto text=row.text("TechObsolete");const auto* t=text&&!text->empty()?h.types().find(*text):nullptr;
   if(!t)result=-1;else{need(t->table=="Technologies"&&t->id>=0&&t->id<=INT32_MAX&&h.technologies().find(static_cast<std::uint64_t>(t->id)),"Strategy effect obsolete Tech mapping differs");result=native(t->id);}}
  return true;
 });need(report.all_succeeded()&&!report.holes&&!report.overlapping_ids&&result.has_value(),"Strategy effect actual obsolete Info absent");return *result;
}
bool has_tech(const LiveSessionState& s,const ui::SelectedBnwPreviewHost& h,std::int32_t owner,std::int32_t tech){
 need(tech>=0&&h.technologies().find(static_cast<std::uint64_t>(tech)),"Strategy effect obsolete branch requires actual Tech Info");
 need(s.world.pregame&&s.world.pregame->layout==bnw,"Strategy effect current Pregame absent");
 const auto& ids=s.world.pregame->archive.integer_arrays.at("team_types");need(ids.size()==64,"Strategy effect TEAM native64 map differs");
 const auto team=native(ids.at(static_cast<std::size_t>(owner)));need(team>=0&&team<64&&s.setup.players.at(owner).team==team,"Strategy effect actual/owned TEAM differs");
 need(s.world.teams&&s.world.teams->layout==bnw&&s.world.teams->dimensions.teams==64&&s.world.teams->dimensions.civ_teams==63&&s.world.teams->teams.size()==64,"Strategy effect native TEAM roster absent");
 const auto& raw=s.world.teams->teams.at(static_cast<std::size_t>(team));need(raw.index==static_cast<std::uint64_t>(team)&&named(raw.fields,"id").value==team,"Strategy effect TEAM identity differs");
 const auto& hashes=named(raw.arrays,"tech_hashes");need(hashes.columns==1,"Strategy effect HasTech hash shape differs");bool covered=false;
 for(const auto hash:hashes.values){need(hash>=0&&hash<=UINT32_MAX,"Strategy effect HasTech native hash width differs");const auto* t=hash?h.types().find_hash(static_cast<std::uint64_t>(hash)):nullptr;if(!t)continue;need(t->table=="Technologies","Strategy effect known Tech hash cross-table");if(t->id==tech)covered=true;}
 need(covered,"Strategy effect reached HasTech requires actual Reset/Read owner for missing hash");
 const auto restored=restore_team_technologies(raw,h.technologies(),h.types());const TeamTechnologies* owned=nullptr;
 for(const auto& t:s.technologies)if(t.team()==team){need(!owned&&t.profile()==content::TechnologyProfile::bnw&&t.slots()==h.technologies().collection.slots,"Strategy effect owned TEAM techs ambiguous");owned=&t;}
 need(owned&&owned->has_tech(tech)==restored.has_tech(tech),"Strategy effect raw/owned HasTech differs");return owned->has_tech(tech);
}
void set_strategy(saves::TeamHashedArray& a,const Info& info,std::int32_t value){
 saves::TeamHashedValues* last=nullptr;for(auto& r:a.rows)if(r.type_hash==info.hash)last=&r;
 need(last&&last->values.size()==1,"Strategy effect native strategy slot has no explicit saved payload");last->values.front()=value; // strategy Read last-wins
}
void store_weights(saves::CityManagementBlock& b,const char* name,const char* table,const std::vector<Info>& rows,const std::vector<std::int32_t>& weights,const content::InfoTypeCatalog& types,bool process){
 need(named(b.fields,"version").value==1&&b.hashes.size()==1&&b.arrays.empty(),"Strategy effect native production schema differs");
 auto& a=named(b.hashes,name);need(a.wire_count>=0&&a.wire_count<=INT32_MAX&&a.rows.size()==static_cast<std::size_t>(a.wire_count)&&weights.size()==rows.size(),"Strategy effect production native count differs");
 std::vector<std::vector<saves::TeamHashedValues*>> positions(rows.size());
 for(auto& r:a.rows){need(r.type_hash<=UINT32_MAX,"Strategy effect production hash width differs");
  if(!r.type_hash){need(r.values.size()==(process?1u:0u),"Strategy effect production null payload differs");if(process)(void)native(r.values.front());continue;}
  need(r.values.size()==1,"Strategy effect production scalar payload absent");(void)native(r.values.front());const auto* t=types.find_hash(r.type_hash);if(!t)continue;
  need(t->table==table&&t->id>=0&&static_cast<std::size_t>(t->id)<rows.size()&&rows.at(static_cast<std::size_t>(t->id)).hash==r.type_hash,"Strategy effect production mapped slot differs");positions.at(static_cast<std::size_t>(t->id)).push_back(&r);
 }
 // Keep the ID27 skeleton/unknown rows. Building/unit Read SUMS duplicates;
 // process Read is last-wins. Zero preceding duplicates then put the actual
 // Reset+FlavorUpdate weight in the final row reproduces BOTH native readers.
 for(std::size_t i=0;i<rows.size();++i){if(rows[i].hash==0){need(rows[i].type.empty()&&weights[i]==0&&positions[i].empty(),"Strategy effect native NULL slot must remain zero/unmapped");continue;}need(!positions[i].empty(),"Strategy effect new weight cannot be represented by existing native hash skeleton");for(auto* r:positions[i])r->values.front()=0;positions[i].back()->values.front()=weights[i];}
}
}
namespace {
detail::BnwCityStrategyFlavorResult flavor_math(const detail::BnwCityStrategyFlavorMath& p,bool start){
 need(p.latest.size()==p.delta.size(),"Strategy flavor native array dimensions differ");detail::BnwCityStrategyFlavorResult result;result.latest=p.latest;
 for(std::size_t flavor=0;flavor<p.latest.size();++flavor)if(p.delta[flavor]!=0){auto& n=result.latest[flavor];n=start?native(std::int64_t(n)+p.delta[flavor]):native(std::int64_t(n)-p.delta[flavor]);if(n<p.minimum)n=p.minimum;else if(n>p.maximum)n=p.maximum;}
 // Reset all FOUR vectors BEFORE any coefficient is consumed, then preserve
 // SDK flavor-major/category-major/native-slot order and checked mul/add.
 for(std::size_t k=0;k<4;++k)result.weights[k].assign(p.coefficients[k].size(),0);
 for(std::size_t flavor=0;flavor<p.latest.size();++flavor){const auto value=std::max(result.latest[flavor],std::int32_t(0));for(std::size_t k=0;k<4;++k)for(std::size_t slot=0;slot<p.coefficients[k].size();++slot){const auto& row=p.coefficients[k][slot];if(!row)continue;need(row->size()==p.latest.size(),"Strategy production flavor array dimension differs");const auto product=native(std::int64_t(row->at(flavor))*value);result.weights[k][slot]=native(std::int64_t(result.weights[k][slot])+product);}}
 return result;
}
}
detail::BnwCityStrategyFlavorResult detail::bnw_city_strategy_end_flavor_math(const BnwCityStrategyFlavorMath& p){return flavor_math(p,false);}
detail::BnwCityStrategyFlavorResult detail::bnw_city_strategy_start_flavor_math(const BnwCityStrategyFlavorMath& p){return flavor_math(p,true);}
struct BnwCityStrategyObsoleteEndOwner::Impl {
 const std::thread::id thread=std::this_thread::get_id();std::shared_ptr<const ui::SelectedBnwPreviewHost> host;
 std::shared_ptr<const LiveSession> session;std::shared_ptr<const LiveSessionState> state;const saves::SavedSession* source=nullptr;
 std::uint64_t generation=0;std::int32_t owner=-1,city=-1;std::pair<std::uint64_t,std::uint64_t> span;SessionCheckpointIdentity identity;
 BnwCityStrategyCheckpointOwner checkpoint;BnwCityCurrentStrategyEligibility eligibility;
 std::vector<Info> strategies,flavor_types;std::array<std::vector<Info>,4> production;
 std::array<std::vector<std::optional<std::vector<std::int32_t>>>,4> coefficients;
 std::function<bool()> rules_current;mutable std::atomic<bool> retired{false},prepared{false};
 Impl(BnwCityStrategyCheckpointOwner c,BnwCityCurrentStrategyEligibility e):checkpoint(std::move(c)),eligibility(std::move(e)){}
};
struct BnwCityStrategyObsoleteEndStage::Impl {
 BnwCityStrategyObsoleteEndOwner parent;std::shared_ptr<const saves::SavedSessionSnapshot> world;saves::CivisSaveSection section;
 detail::BnwCityStrategyFlavorResult computed;
 Impl(BnwCityStrategyObsoleteEndOwner p,std::shared_ptr<const saves::SavedSessionSnapshot> w,saves::CivisSaveSection s,detail::BnwCityStrategyFlavorResult result):parent(std::move(p)),world(std::move(w)),section(std::move(s)),computed(std::move(result)){}
};
BnwCityStrategyObsoleteEndOwner::BnwCityStrategyObsoleteEndOwner(std::shared_ptr<Impl> p)noexcept:impl_(std::move(p)){}
BnwCityStrategyObsoleteEndStage::BnwCityStrategyObsoleteEndStage(std::shared_ptr<const Impl> p)noexcept:impl_(std::move(p)){}
void BnwCityStrategyObsoleteEndOwner::require_current()const{
 const auto& p=impl_;need(p&&p->thread==std::this_thread::get_id(),"Strategy effect foreign constructor thread/moved owner");
 try{need(!p->retired.load()&&p->rules_current&&p->rules_current(),"Strategy effect immutable rules owner retired");(void)p->checkpoint.section();
  (void)p->session->bnw_game_debug_mode();need(&p->host->session()==p->session.get()&&p->session->read_state_lease()==p->state&&p->session->state_generation()==p->generation&& &p->session->source()==p->source,"Strategy effect actual allocation/source/generation retired");
  const auto* raw=resolve_bnw_current_city(*p->state,p->owner,p->city);need(raw&&city_scalar(*raw,"m_eOwner")==p->owner&&city_scalar(*raw,"m_iID")==p->city&&std::pair{raw->core.start_offset,raw->core.end_offset}==p->span,"Strategy effect actual fullID/coreSpan changed");
 }catch(...){p->retired.store(true);throw;}
}
BnwCityStrategyObsoleteEndStage BnwCityStrategyObsoleteEndOwner::prepare(std::int64_t selector)const{return prepare_effect(selector,EffectPath::obsolete_end);}
BnwCityStrategyObsoleteEndStage BnwCityStrategyObsoleteEndOwner::prepare_false_end(std::int64_t selector)const{return prepare_effect(selector,EffectPath::ordered_false_end);}
BnwCityStrategyObsoleteEndStage BnwCityStrategyObsoleteEndOwner::prepare_start_candidate(std::int64_t selector)const{return prepare_effect(selector,EffectPath::ordered_start_candidate);}
BnwCityStrategyObsoleteEndStage BnwCityStrategyObsoleteEndOwner::prepare_effect(std::int64_t selector,EffectPath path)const{return typed([&]{
 require_current();const auto p=impl_;need(!p->prepared.load(),"Strategy effect already prepared once");const auto id=native(selector);need(id>=0&&static_cast<std::size_t>(id)<p->strategies.size(),"Strategy effect selector outside actual native strategy Info");
 const bool start=path==EffectPath::ordered_start_candidate;
 const auto record=p->eligibility.record_at(id);need(record.native_id==id&&record.type==p->strategies.at(static_cast<std::size_t>(id)).type&&record.obligation==(start?BnwCityStrategyTriggerObligation::start:BnwCityStrategyTriggerObligation::end)&&record.using_strategy==std::optional(!start),"Strategy effect requires actual scheduled matching using/obligation");
 // SDK SetUsing(true) reads GAME turn only when the current false flag changes.
 // Derive it from the genuine current raw archive, never a preview/caller turn.
 std::int32_t adopted=-1;
 if(start){need(p->state->world.pregame&&p->state->world.pregame->layout==bnw,"Strategy START actual GAME turn Pregame unavailable");adopted=native(p->state->world.pregame->archive.integers.at("game_turn"));need(p->state->setup.game_turn==adopted,"Strategy START raw/owned GAME turn differs");}
 if(path==EffectPath::obsolete_end){const auto tech=obsolete(*p->host,id);need(tech!=-1&&has_tech(*p->state,*p->host,p->owner,tech),"Strategy effect trigger/Lua result unavailable: obsolete bypass not reached");}
 const auto* current=resolve_bnw_current_city(*p->state,p->owner,p->city);need(current,"Strategy effect current city absent");
 detail::BnwCityStrategyFlavorMath math;const auto& strategy=named(current->management.blocks,"strategy");const auto& latest=named(strategy.arrays,"m_piLatestFlavorValues");
 need(latest.columns==1&&latest.values.size()==p->flavor_types.size()&&named(strategy.fields,"flavor_wire_count").value==static_cast<std::int64_t>(latest.values.size()),"Strategy effect actual latest flavor dimensions differ");for(const auto n:latest.values)math.latest.push_back(native(n));
 math.delta=flavors(*p->host,"AICityStrategy_Flavors","AICityStrategyType",p->strategies.at(static_cast<std::size_t>(id)).type,p->flavor_types.size());math.minimum=define(*p->host,"FLAVOR_MIN_VALUE");math.maximum=define(*p->host,"FLAVOR_MAX_VALUE");math.coefficients=p->coefficients;
 const auto result=start?detail::bnw_city_strategy_start_flavor_math(math):detail::bnw_city_strategy_end_flavor_math(math);auto world=std::make_shared<saves::SavedSessionSnapshot>(p->state->world);
 auto& c=world->all_players->players.at(static_cast<std::size_t>(p->owner)).cities.cities.at(static_cast<std::size_t>(current->index));need(city_scalar(c,"m_iID")==p->city&&city_scalar(c,"m_eOwner")==p->owner,"Strategy effect detached fullID differs");
 auto& b=named(c.management.blocks,"strategy");set_strategy(named(b.hashes,"m_pabUsingCityStrategy"),p->strategies.at(static_cast<std::size_t>(id)),start?1:0);set_strategy(named(b.hashes,"m_paiTurnCityStrategyAdopted"),p->strategies.at(static_cast<std::size_t>(id)),adopted);
 auto& values=named(b.arrays,"m_piLatestFlavorValues").values;values.assign(result.latest.begin(),result.latest.end());
 constexpr const char* block_names[]{"building_ai","unit_ai","project_ai","process_ai"};constexpr const char* arrays[]{"m_BuildingAIWeights","m_UnitAIWeights","m_ProjectAIWeights","m_ProcessAIWeights"};constexpr const char* tables[]{"Buildings","Units","Projects","Processes"};
 for(std::size_t k=0;k<4;++k){auto& prod=named(c.management.blocks,block_names[k]);if(k==2){need(named(prod.fields,"version").value==1&&prod.hashes.empty()&&prod.arrays.size()==1&&c.management.context.bnw_projects==std::optional<std::uint64_t>(p->production[k].size()),"Strategy effect project native fixed dimensions differ");auto& a=named(prod.arrays,arrays[k]);need(a.columns==1&&a.values.size()==result.weights[k].size(),"Strategy effect project native slot coverage differs");for(const auto n:a.values)(void)native(n);a.values.assign(result.weights[k].begin(),result.weights[k].end());}else store_weights(prod,arrays[k],tables[k],p->production[k],result.weights[k],p->host->types(),k==3);}
 detail::require_bnw_city_strategy_checkpoint_arrays(*world);const auto section=make_bnw_city_management_checkpoint(*world,p->source->snapshot,p->identity,true);
 // Existing ID27 roundtrip seals represented changes; no live stores occur.
 auto roundtrip=p->source->snapshot;restore_bnw_city_management_checkpoint(section,roundtrip,p->source->snapshot,p->identity);need(same(make_bnw_city_management_checkpoint(roundtrip,p->source->snapshot,p->identity,true),section),"Strategy effect detached ID27 did not roundtrip");
 auto stage=std::make_shared<BnwCityStrategyObsoleteEndStage::Impl>(*this,std::move(world),section,result);require_current();need(!p->prepared.exchange(true),"Strategy effect once slot consumed");return BnwCityStrategyObsoleteEndStage(std::move(stage));
});}
std::shared_ptr<const saves::SavedSessionSnapshot> BnwCityStrategyObsoleteEndStage::read_world()const{return typed([&]{const auto p=impl_;need(bool(p),"Strategy effect moved stage");p->parent.require_current();return p->world;});}
saves::CivisSaveSection BnwCityStrategyObsoleteEndStage::management_section()const{return typed([&]{const auto p=impl_;need(bool(p),"Strategy effect moved stage");p->parent.require_current();auto result=p->section;p->parent.require_current();return result;});}
detail::BnwCityStrategyFlavorResult BnwCityStrategyObsoleteEndStage::computed_flavor_result()const{return typed([&]{const auto p=impl_;need(bool(p),"Strategy effect moved stage");p->parent.require_current();auto result=p->computed;p->parent.require_current();return result;});}
BnwCityStrategyObsoleteEndOwner bind_bnw_city_strategy_obsolete_end(std::shared_ptr<const ui::SelectedBnwPreviewHost> host,std::int64_t owner,std::int64_t city){return typed([&]{
 need(bool(host),"Strategy effect genuine selected Host absent");(void)host->session().bnw_game_debug_mode();const auto native_owner=native(owner);need(native_owner>=0&&native_owner<64,"Strategy effect native owner outside roster");const auto handle=native(city);
 auto checkpoint=capture_bnw_city_strategy_checkpoint(host);auto eligibility=BnwCityCurrentStrategyEligibility::prepare_ordered(host,native_owner,handle);auto p=std::make_shared<BnwCityStrategyObsoleteEndOwner::Impl>(std::move(checkpoint),std::move(eligibility));p->host=std::move(host);p->session=std::shared_ptr<const LiveSession>(p->host,&p->host->session());p->state=p->session->read_state_lease();p->source=&p->session->source();p->generation=p->session->state_generation();p->identity=p->host->identity();p->rules_current=p->host->database().immutable_preview_fingerprint_currentness();need(p->rules_current&&p->rules_current(),"Strategy effect actual immutable DB absent");
 const auto* raw=resolve_bnw_current_city(*p->state,native_owner,handle);need(raw,"Strategy effect actual city absent");p->owner=native_owner;p->city=city_scalar(*raw,"m_iID");p->span={raw->core.start_offset,raw->core.end_offset};p->strategies=infos(*p->host,"AICityStrategies");p->flavor_types=infos(*p->host,"Flavors");
 // SetFlavors sizes by MAX(rowid)+1, not ID span. Prove all reached native
 // flavor accesses fit that source allocation without pretending a Reset.
 std::optional<std::int32_t> maxrow;const auto r=p->host->database().query_rows({"SELECT max(rowid) from Flavors",{}},[&](const auto& row){need(!maxrow,"Strategy effect MAX(rowid) result duplicated");maxrow=native(row.integer_at(0));});need(r.all_succeeded()&&maxrow&&*maxrow<INT32_MAX&&native(std::int64_t(*maxrow)+1)>=0&&static_cast<std::uint64_t>(native(std::int64_t(*maxrow)+1))>=p->flavor_types.size(),"Strategy effect source flavor cache allocation too small/overflowed");
 constexpr const char* tables[]{"Buildings","Units","Projects","Processes"};constexpr const char* flavor_tables[]{"Building_Flavors","Unit_Flavors","Project_Flavors","Process_Flavors"};constexpr const char* columns[]{"BuildingType","UnitType","ProjectType","ProcessType"};
 for(std::size_t k=0;k<4;++k){p->production[k]=infos(*p->host,tables[k],true);for(const auto& info:p->production[k]){if(info.hash==0){need(info.type.empty(),"Strategy effect native NULL Info marker differs");p->coefficients[k].push_back(std::nullopt);}else p->coefficients[k].push_back(flavors(*p->host,flavor_tables[k],columns[k],info.type,p->flavor_types.size()));}}
 BnwCityStrategyObsoleteEndOwner out(std::move(p));out.require_current();return out;
});}
}
