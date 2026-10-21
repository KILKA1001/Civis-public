#include "civ5/runtime/bnw_city_specialist_counts.hpp"
#include "civ5/runtime/bnw_city_strategy_yields.hpp"
#include "civ5/saves/vp_city_management_profile.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <tuple>
#include <string>
#include <climits>
#include <map>
#include <thread>
#include <type_traits>
namespace civ5::runtime {
namespace {
void need(bool b,const char* m){if(!b)throw BnwCitySpecialistCountsUnavailable(m);}
std::int32_t native(std::int64_t n){need(n>=INT32_MIN&&n<=INT32_MAX,"Specialist native32 operand/intermediate unavailable");return static_cast<std::int32_t>(n);}
template<class F>auto typed(F&& f)->decltype(f()){try{return f();}catch(const BnwCitySpecialistCountsUnavailable&){throw;}catch(const std::exception& e){throw BnwCitySpecialistCountsUnavailable(e.what());}}
const auto& named(const auto& rows,std::string_view name){using T=typename std::remove_cvref_t<decltype(rows)>::value_type;const T* out=nullptr;for(const auto& r:rows)if(r.name&&std::string_view(r.name)==name){need(!out,"Specialist duplicate raw occurrence");out=&r;}need(out,"Specialist raw occurrence absent");return *out;}
std::int32_t scalar(const auto& c,std::string_view name){const auto& f=named(c.fields,name);need(f.encoding==saves::CityCoreEncoding::i32&&f.values.size()==1&&!f.wire_count&&f.bytes.empty()&&f.hashes.empty()&&f.row_keys.empty()&&f.row_counts.empty(),"Specialist raw core scalar shape unavailable");return native(f.values.front());}
using Seal=std::vector<std::int64_t>;
void span(Seal& out,std::uint64_t a,std::uint64_t b){need(a<=b&&b<=INT64_MAX,"Specialist source span unavailable");out.push_back(static_cast<std::int64_t>(a));out.push_back(static_cast<std::int64_t>(b));}
void text(Seal& out,std::string_view s){need(s.size()<=INT32_MAX,"Specialist metadata string exceeds native bounds");out.push_back(static_cast<std::int64_t>(s.size()));for(const unsigned char c:s)out.push_back(c);}
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
constexpr std::int32_t slot_budget=16*1024*1024;
struct Collection {std::int32_t span=0,default_id=-1;bool default_bound=false;content::InfoTypeCatalog types;std::map<std::int32_t,std::pair<std::string,std::uint32_t>> infos;Seal seal;};
struct Observed {detail::BnwSpecialistMathResult replay;Seal seal;};
}
std::vector<std::int32_t> detail::bnw_specialist_reset_read_dense_math(std::int32_t size,const std::vector<BnwSpecialistMathRow>& rows){return typed([&]{
 need(size>=0&&size<=slot_budget,"Specialist native span exceeds bounded host allocation budget");
 std::vector<std::int32_t> dense(static_cast<std::size_t>(size),0); // actual Reset includes null Info slots
 for(const auto& r:rows){
  if(!r.count){need(!r.mapped_id,"Specialist null hash cannot carry a remap without a value");continue;}
  if(!r.mapped_id||*r.mapped_id==-1)continue;
  need(*r.mapped_id>=0,"Specialist negative non-sentinel native remap unavailable");
  if(*r.mapped_id<size)dense.at(static_cast<std::size_t>(*r.mapped_id))=*r.count;
 }return dense;
});}
detail::BnwSpecialistMathResult detail::bnw_specialist_reset_read_math(std::int32_t size,std::int32_t default_id,const std::vector<BnwSpecialistMathRow>& rows){return typed([&]{
 BnwSpecialistMathResult result;result.dense=bnw_specialist_reset_read_dense_math(size,rows);
 for(std::int32_t i=0;i<size;++i)if(i!=default_id)result.total=native(std::int64_t(result.total)+result.dense.at(static_cast<std::size_t>(i)));
 return result;
});}
struct BnwCitySpecialistCounts::Impl {
 const std::thread::id thread=std::this_thread::get_id();BnwCityCurrentFoodRate parent;
 std::optional<Collection> collection,building_collection;std::map<std::tuple<std::int64_t,std::int64_t,std::string>,Observed> reached;
 explicit Impl(const BnwCityCurrentFoodRate& p):parent(p){}
};
BnwCitySpecialistCounts::BnwCitySpecialistCounts(std::shared_ptr<Impl> p)noexcept:impl_(std::move(p)){}
BnwCitySpecialistCounts BnwCitySpecialistCounts::from_same_rate(const BnwCityCurrentFoodRate& source){return typed([&]{
 source.require_current();return BnwCitySpecialistCounts(std::make_shared<Impl>(source));
});}
BnwCitySpecialistCounts BnwCitySpecialistCounts::from_same_host(std::shared_ptr<const ui::SelectedBnwPreviewHost> host,std::int64_t owner,std::int64_t city){return typed([&]{
 need(bool(host),"Specialist source actual selected Host absent");(void)host->session().bnw_game_debug_mode();
 auto session=std::shared_ptr<const LiveSession>(host,&host->session());auto db=std::shared_ptr<const content::RulesDatabase>(host,&host->database());
 auto rules=std::make_shared<const BnwCityFoodRateRules>(BnwCityFoodRateRules::load(db,host->identity().rules_sha256));
 // Genuine metadata-only parent: no capture_initial/value FOOD or excess
 // happiness read. Do not require the output currently being recomputed.
 auto metadata=BnwCityCurrentFoodRate::bind_specialist_source_only(std::move(session),std::move(rules),owner,city);
 metadata.require_metadata_current();
 auto result=BnwCitySpecialistCounts(std::make_shared<Impl>(metadata));
 result.require_current_source();return result;
});}
void BnwCitySpecialistCounts::require_current_source()const{typed([&]{const auto p=impl_;need(p&&p->thread==std::this_thread::get_id(),"Specialist source foreign thread/moved");p->parent.require_metadata_current();});}
void BnwCitySpecialistCounts::require_same_world_database(const saves::SavedSessionSnapshot& world,const content::RulesDatabase& db)const{typed([&]{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id(),"Specialist source foreign thread/moved");p->parent.require_metadata_current();
 need(&p->parent.current_source_state().world==&world&&p->parent.current_source_database_owner().get()==&db,"Specialist source requires SAME actual world and DB allocation");
});}
std::vector<std::int32_t> BnwCitySpecialistCounts::capture_dense_for_city(std::int64_t owner,std::int64_t city,bool total_requested,std::string_view source_array)const{return typed([&]{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id(),"Specialist source foreign thread/moved");p->parent.require_metadata_current();
 need(owner==p->parent.canonical_owner(),"Specialist source crossed actual player owner");
 const bool building=source_array=="m_aiNumSpecialistsInBuilding"||source_array=="m_aiNumForcedSpecialistsInBuilding";
 need(building||source_array=="m_aiSpecialistCounts"||source_array=="m_piBuildingGreatPeopleRateChanges","Specialist private source array unavailable");
 need(!building||!total_requested,"Building specialist count cannot request specialist total/default");
 const std::string_view table=building?"Buildings":"Specialists";
 auto& selected_collection=building?p->building_collection:p->collection;
 const auto db=p->parent.current_source_database_owner();
 if(!selected_collection){
  Collection collection;collection.types=p->parent.current_source_info_types();
  const auto& global=collection.types.report();need(global.all_succeeded()&&!global.name_overwrites&&!global.hash_collisions,"Specialist native global hash collision/overwrite provenance unavailable");
  if(total_requested){
  const auto& defines=p->parent.current_source_defines();const auto* d=defines.find("DEFAULT_SPECIALIST");
  need(d&&defines.profile()==content::GlobalDefineProfile::bnw&&d->lookup.found&&d->lookup.error.empty()&&std::holds_alternative<std::int64_t>(d->lookup.value),"Specialist actual DEFAULT_SPECIALIST unavailable");collection.default_id=native(std::get<std::int64_t>(d->lookup.value));
   collection.default_bound=true;
  }
  p->parent.require_metadata_current();const auto report=db->prefetch_info(std::string(table),[&](auto slot,auto authored,const auto& row){
   need(slot<=static_cast<std::uint64_t>(slot_budget)&&authored>=0&&authored==static_cast<std::int64_t>(slot),"Specialist native slot/authored ID provenance unavailable");
   const auto type=row.text("Type");need(type&&!type->empty()&&type->find('\0')==std::string::npos,"Specialist actual nonempty finite Type unavailable");
   const auto hash=content::saved_info_type_hash(*type);need(hash!=0,"Specialist nonnull native Type hash is zero");const auto* name=collection.types.find(*type);const auto* wire=collection.types.find_hash(hash);
   need(name&&wire&&name->table==table&&wire->table==table&&name->type==*type&&wire->type==*type&&name->id==authored&&wire->id==authored&&name->hash==hash&&wire->hash==hash,"Specialist actual global Type/hash/slot identity unavailable");
   const auto id=static_cast<std::int32_t>(slot);need(collection.infos.emplace(id,std::pair{*type,hash}).second,"Specialist duplicate actual Info slot");return true;
  });
  need(report.all_succeeded()&&!report.overlapping_ids&&report.slots<=static_cast<std::uint64_t>(slot_budget)&&report.rows==collection.infos.size()&&report.holes+report.rows==report.slots,"Specialist actual native Info span unavailable");collection.span=static_cast<std::int32_t>(report.slots);
  collection.seal={collection.span};if(collection.default_bound)collection.seal.push_back(collection.default_id);
  for(std::int32_t i=0;i<collection.span;++i){collection.seal.push_back(i);const auto it=collection.infos.find(i);collection.seal.push_back(it!=collection.infos.end());if(it!=collection.infos.end()){text(collection.seal,it->second.first);collection.seal.push_back(it->second.second);}}
  p->parent.require_metadata_current();selected_collection=std::move(collection);
 }
 if(total_requested&&!selected_collection->default_bound){
  auto& collection=*selected_collection;
  const auto& defines=p->parent.current_source_defines();const auto* d=defines.find("DEFAULT_SPECIALIST");
  need(d&&defines.profile()==content::GlobalDefineProfile::bnw&&d->lookup.found&&d->lookup.error.empty()&&std::holds_alternative<std::int64_t>(d->lookup.value),"Specialist actual DEFAULT_SPECIALIST unavailable");collection.default_id=native(std::get<std::int64_t>(d->lookup.value));
  collection.default_bound=true;collection.seal.insert(collection.seal.begin()+1,collection.default_id);p->parent.require_metadata_current();
 }
 const auto& collection=*selected_collection;const auto& state=p->parent.current_source_state();
 const auto* raw=resolve_bnw_current_city(state,native(owner),native(city));need(raw&&scalar(raw->core,"m_iID")==city&&scalar(raw->core,"m_eOwner")==owner,"Specialist canonical current city absent/stale");
 const auto& owned=state.cities.players.at(owner).cities.at(city);need(owned.owner==owner&&owned.id==city&&owned.source_core==std::optional{std::pair{raw->core.start_offset,raw->core.end_offset}}&&owned.x==scalar(raw->core,"m_iX")&&owned.y==scalar(raw->core,"m_iY"),"Specialist raw/owned canonical city metadata stale");
 const auto& management=raw->management;need(management.layout==bnw&&management.city_plot_slots==37&&management.context.bnw_projects&&*management.context.bnw_projects<=INT32_MAX&&!saves::vp_city_management_dimensions(management.context),"Specialist actual BNW Citizens schema unavailable");
 constexpr const char* block_names[]={"strategy","building_ai","unit_ai","project_ai","process_ai","citizens"};
 need(management.blocks.size()==6,"Specialist BNW management block count unavailable");auto cursor=management.start_offset;
 for(std::size_t i=0;i<6;++i){const auto& block=management.blocks.at(i);need(block.name&&std::string_view(block.name)==block_names[i]&&block.start_offset==cursor&&block.end_offset>=cursor&&block.end_offset<=management.end_offset,"Specialist BNW management block order/boundaries unavailable");cursor=block.end_offset;}
 need(cursor==management.end_offset,"Specialist BNW management terminal boundary unavailable");
 const auto& citizens=named(management.blocks,"citizens");const auto version=named(citizens.fields,"version").value;need(version>=0&&version<=1,"Specialist Citizens version unavailable");
 need(management.start_offset<=citizens.start_offset&&citizens.end_offset<=management.end_offset,"Specialist Citizens source boundaries unavailable");
 const auto& counts=named(citizens.hashes,source_array);need(counts.wire_count>=0&&counts.wire_count<=UINT32_MAX&&static_cast<std::uint64_t>(counts.wire_count)==counts.rows.size(),"Specialist unsigned native wire count differs");
 Observed now;now.seal={owner,city,scalar(raw->core,"m_iX"),scalar(raw->core,"m_iY"),version,counts.wire_count,static_cast<std::int64_t>(*management.context.bnw_projects)};span(now.seal,raw->core.start_offset,raw->core.end_offset);span(now.seal,management.start_offset,management.end_offset);span(now.seal,citizens.start_offset,citizens.end_offset);
 std::vector<detail::BnwSpecialistMathRow> rows;rows.reserve(counts.rows.size());
 for(const auto& row:counts.rows){
  need(row.type_hash<=UINT32_MAX,"Specialist raw hash exceeds native uint32");now.seal.push_back(static_cast<std::int64_t>(row.type_hash));
  if(!row.type_hash){need(row.values.empty(),"Specialist null hash carries a value");rows.push_back({{}, {}});continue;}
  need(row.values.size()==1,"Specialist nonnull raw row lacks exactly one value");const auto count=native(row.values.front());now.seal.push_back(count);
  const auto* remap=collection.types.find_hash(row.type_hash);if(!remap){rows.push_back({{},count});continue;}
  need(remap->table==table,"Specialist reached foreign-table global remap provenance unavailable");const auto mapped=native(remap->id);
  const auto* name=collection.types.find(remap->type);need(name&&name->id==remap->id&&name->table==remap->table&&name->hash==row.type_hash&&content::saved_info_type_hash(remap->type)==row.type_hash,"Specialist reached global remap identity unavailable");
  if(mapped>=0&&mapped<collection.span){const auto info=collection.infos.find(mapped);need(info!=collection.infos.end()&&info->second.first==remap->type&&info->second.second==row.type_hash,"Specialist reached canonical Info slot unavailable");}
  rows.push_back({mapped,count});
 }
 now.replay.dense=detail::bnw_specialist_reset_read_dense_math(collection.span,rows);p->parent.require_metadata_current();
 const auto key=std::tuple{owner,city,std::string(source_array)};const auto it=p->reached.find(key);
 if(it!=p->reached.end())need(it->second.seal==now.seal&&it->second.replay.dense==now.replay.dense,"Specialist retained ordered source rows stale");else p->reached.emplace(key,now);
 return now.replay.dense;
});}
std::int32_t BnwCitySpecialistCounts::total_for_city(std::int64_t owner,std::int64_t city)const{return typed([&]{
 const auto dense=capture_dense_for_city(owner,city,true,"m_aiSpecialistCounts");const auto p=impl_;std::int32_t total=0;
 for(std::size_t i=0;i<dense.size();++i)if(static_cast<std::int64_t>(i)!=p->collection->default_id)total=native(std::int64_t(total)+dense.at(i));
 p->parent.require_metadata_current();return total;
});}
std::int32_t BnwCitySpecialistCounts::count_for_city(std::int64_t owner,std::int64_t city,std::int32_t specialist)const{return typed([&]{
 require_current_source();need(specialist>=0,"Specialist reached negative per-index count");const auto dense=capture_dense_for_city(owner,city,false,"m_aiSpecialistCounts");need(static_cast<std::uint64_t>(specialist)<dense.size(),"Specialist reached per-index count outside native span");return dense.at(static_cast<std::size_t>(specialist));
});}
std::int32_t BnwCitySpecialistCounts::building_rate_for_city(std::int64_t owner,std::int64_t city,std::int32_t specialist)const{return typed([&]{
 require_current_source();need(specialist>=0,"Specialist reached negative per-index building rate");const auto dense=capture_dense_for_city(owner,city,false,"m_piBuildingGreatPeopleRateChanges");need(static_cast<std::uint64_t>(specialist)<dense.size(),"Specialist reached per-index building rate outside native span");return dense.at(static_cast<std::size_t>(specialist));
});}
std::int32_t BnwCitySpecialistCounts::building_specialists_for_city(std::int64_t owner,std::int64_t city,std::int32_t building)const{return typed([&]{
 require_current_source();need(building>=0,"Building specialist reached negative native index");
 const auto dense=capture_dense_for_city(owner,city,false,"m_aiNumSpecialistsInBuilding");
 need(static_cast<std::uint64_t>(building)<dense.size(),"Building specialist count outside actual native Buildings span");
 return dense.at(static_cast<std::size_t>(building));
});}
std::int32_t BnwCitySpecialistCounts::forced_building_specialists_for_city(std::int64_t owner,std::int64_t city,std::int32_t building)const{return typed([&]{
 require_current_source();need(building>=0,"Forced building specialist reached negative native index");
 const auto dense=capture_dense_for_city(owner,city,false,"m_aiNumForcedSpecialistsInBuilding");
 need(static_cast<std::uint64_t>(building)<dense.size(),"Forced building specialist count outside actual native Buildings span");
 return dense.at(static_cast<std::size_t>(building));
});}
std::vector<std::int64_t> BnwCitySpecialistCounts::reached_source_seal()const{return typed([&]{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id(),"Specialist source foreign thread/moved");p->parent.require_metadata_current();
 Seal out;if(!p->collection&&!p->building_collection)return out;
 if(p->collection)out=p->collection->seal;
 if(p->building_collection){out.push_back(-2);out.push_back(static_cast<std::int64_t>(p->building_collection->seal.size()));out.insert(out.end(),p->building_collection->seal.begin(),p->building_collection->seal.end());}
 for(const auto& [key,facts]:p->reached){(void)capture_dense_for_city(std::get<0>(key),std::get<1>(key),false,std::get<2>(key));need(facts.seal.size()<=INT64_MAX,"Specialist operand seal exceeds signed host size");out.push_back(static_cast<std::int64_t>(facts.seal.size()));out.insert(out.end(),facts.seal.begin(),facts.seal.end());}
 p->parent.require_metadata_current();return out;
});}
}
