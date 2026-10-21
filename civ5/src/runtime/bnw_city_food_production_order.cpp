#include "civ5/runtime/bnw_city_food_production_order.hpp"
#include "civ5/runtime/bnw_city_strategy_yields.hpp"
#include <climits>
#include <map>
#include <thread>
#include <type_traits>
namespace civ5::runtime {
namespace {
void need(bool b,const char* m){if(!b)throw std::runtime_error(m);}
std::int32_t i32(std::int64_t n){need(n>=INT32_MIN&&n<=INT32_MAX,"Food production native int width unavailable");return static_cast<std::int32_t>(n);}
const auto& named(const auto& rows,std::string_view name){using T=typename std::remove_cvref_t<decltype(rows)>::value_type;const T* out=nullptr;for(const auto& r:rows)if(r.name&&std::string_view(r.name)==name){need(!out,"Food production duplicate source field");out=&r;}need(out,"Food production source field absent");return *out;}
std::int32_t core(const auto& c,std::string_view name){const auto& f=named(c.fields,name);need(f.values.size()==1,"Food production source scalar shape unavailable");return i32(f.values.front());}
struct Head {
 std::int64_t owner=-1;std::int32_t city=-1;
 std::pair<std::uint64_t,std::uint64_t> city_span,production_span,queue_span,head_span;
 std::uint64_t version=0,count=0;
 std::optional<CityYieldState::ProductionOrder> order;
 std::int64_t wire_item=0;
 bool operator==(const Head&)const=default;
};
Head head(const LiveSessionState& s,std::int64_t owner,std::int64_t id){
 const auto* c=resolve_bnw_current_city(s,i32(owner),i32(id));need(c,"Food production current city absent");
 Head h;h.owner=owner;h.city=core(c->core,"m_iID");h.city_span={c->core.start_offset,c->core.end_offset};
 need(c->production.layout==saves::GameStateLayout::expansion2_version1&&c->production.context.bnw_city_version,"Food production BNW queue schema unknown");
 h.version=*c->production.context.bnw_city_version;h.production_span={c->production.start_offset,c->production.end_offset};
 const auto& q=named(c->production.blocks,"queue");h.queue_span={q.start_offset,q.end_offset};
 const auto n=named(q.fields,"wire_count").value;need(n>=0&&n<=UINT32_MAX&&static_cast<std::uint64_t>(n)==q.children.size(),"Food production raw queue count differs");h.count=static_cast<std::uint64_t>(n);
 const auto& orders=s.cities.players.at(owner).cities.at(h.city).production_orders;
 need(orders&&orders->size()==q.children.size(),"Food production raw/owned queue count stale");
 if(!h.count)return h;
 const auto& r=q.children.front();h.head_span={r.start_offset,r.end_offset};h.order=orders->front();
 need(r.index==0&&h.order->type==i32(named(r.fields,"eOrderType").value)&&h.order->secondary==i32(named(r.fields,"iData2").value),"Food production raw/owned head fields stale");
 const auto saved=named(r.fields,"bSave").value,rush=named(r.fields,"bRush").value;
 need((saved==0||saved==1)&&(rush==0||rush==1)&&h.order->save==(saved!=0)&&h.order->rush==(rush!=0),"Food production raw/owned head flags stale");
 const bool hashed=h.version>=5&&h.order->type>=0&&h.order->type<=4;
 h.wire_item=named(r.fields,hashed?"iData1_hash":"iData1").value;
 if(hashed)need(h.wire_item>=0&&h.wire_item<=UINT32_MAX&&h.order->source_hash==std::optional{static_cast<std::uint32_t>(h.wire_item)},"Food production raw/owned head hash stale");
 else need(!h.order->source_hash&&h.order->item==i32(h.wire_item),"Food production raw/owned head ID stale");
 need(h.order->type>=0&&h.order->type<=4,"Food production unsupported native order kind");
 return h;
}
std::int32_t military(const LiveSessionState& s,std::int64_t owner){
 const auto& p=s.world.all_players->players.at(static_cast<std::size_t>(owner));
 need(p.saved_player_id==owner&&p.core.layout==saves::GameStateLayout::expansion2_version1,"Food production player binding stale");
 const auto& f=named(p.core.fields,"m_iMilitaryFoodProductionCount");
 need(f.encoding==saves::PlayerCoreEncoding::i32&&f.values.size()==1&&!f.wire_count&&f.bytes.empty()&&f.hashes.empty()&&f.row_counts.empty(),"Food production military counter shape unavailable");return i32(f.values.front());
}
}
struct BnwCityFoodProductionRules::Impl {
 struct Unit{std::optional<std::string> type;std::optional<std::uint32_t> hash;bool food=false,military=false;};
 std::thread::id thread=std::this_thread::get_id();std::function<bool()> current;
 std::string sha;std::uint64_t slots=0;content::InfoTypeCatalog types;std::map<std::int32_t,Unit> units;
};
BnwCityFoodProductionRules::BnwCityFoodProductionRules(std::unique_ptr<Impl> p)noexcept:impl_(std::move(p)){}
BnwCityFoodProductionRules::~BnwCityFoodProductionRules()=default;
void BnwCityFoodProductionRules::require_current(std::string_view sha)const{
 need(impl_&&impl_->thread==std::this_thread::get_id(),"Food production rules foreign thread/moved");
 need(impl_->current&&impl_->current(),"Food production SAME immutable rules scope stale");
 need(impl_->sha==sha,"Food production rules identity mismatch");
}
std::shared_ptr<const BnwCityFoodProductionRules> BnwCityFoodProductionRules::load(const content::RulesDatabase& db,std::string_view sha){
 auto p=std::make_unique<Impl>();p->current=db.immutable_preview_fingerprint_currentness();
 need(p->current&&p->current(),"Food production requires SAME immutable activated rules scope");
 p->sha=db.gameplay_fingerprint_sha256();need(p->sha==sha,"Food production rules identity mismatch");
 p->types=db.cache_info_types();need(p->types.report().all_succeeded(),"Food production Info type import unavailable");
 const auto report=db.prefetch_info("Units",[&](auto slot,auto authored,const auto& row){
  need(slot<=INT32_MAX,"Food production UnitInfo slot exceeds native width");
  Impl::Unit unit;unit.type=row.text("Type");if(unit.type)unit.hash=content::saved_info_type_hash(*unit.type);
  const auto* info=unit.type?p->types.find(*unit.type):nullptr;const auto* wire=unit.hash?p->types.find_hash(*unit.hash):nullptr;
  need(unit.type&&unit.hash&&authored==static_cast<std::int64_t>(slot)&&info&&info->table=="Units"&&info->id==static_cast<std::int64_t>(slot)&&wire&&wire->type==*unit.type&&wire->table=="Units"&&wire->id==info->id,"Food production actual UnitInfo slot/Type/hash identity unavailable");
  unit.food=row.boolean("Food");unit.military=row.boolean("MilitaryProduction");
  need(p->units.emplace(static_cast<std::int32_t>(slot),std::move(unit)).second,"Food production duplicate UnitInfo slot");return true;
 });
 need(report.all_succeeded()&&!report.overlapping_ids&&report.slots<=INT32_MAX,"Food production UnitInfo span unavailable");p->slots=report.slots;
 need(p->current(),"Food production rules source retired during import");
 return std::shared_ptr<const BnwCityFoodProductionRules>(new BnwCityFoodProductionRules(std::move(p)));
}
struct BnwCityFoodProductionOrder::Impl {
 std::thread::id thread=std::this_thread::get_id();Head head;
 std::shared_ptr<const BnwCityFoodProductionRules> rules;
 bool result=false,unit_read=false,counter_read=false,complete=false;std::int32_t counter=0;
};
BnwCityFoodProductionOrder::BnwCityFoodProductionOrder(std::shared_ptr<const Impl> p)noexcept:impl_(std::move(p)){}
void BnwCityFoodProductionOrder::check_thread()const{need(impl_&&impl_->thread==std::this_thread::get_id(),"Food production getter foreign thread/moved");}
bool BnwCityFoodProductionOrder::food_production()const{check_thread();need(impl_->complete,"Food production UnitInfo source not yet reached");return impl_->result;}
bool BnwCityFoodProductionOrder::read_unit_info()const{check_thread();return impl_->unit_read;}
bool BnwCityFoodProductionOrder::read_military_counter()const{check_thread();return impl_->counter_read;}
std::int64_t BnwCityFoodProductionOrder::owner()const{check_thread();return impl_->head.owner;}
std::int32_t BnwCityFoodProductionOrder::city()const{check_thread();return impl_->head.city;}
void BnwCityFoodProductionOrder::require_same_inputs(const LiveSessionState& s,std::string_view sha)const{
 need(impl_&&impl_->thread==std::this_thread::get_id(),"Food production getter foreign thread/moved");
 if(impl_->rules)impl_->rules->require_current(sha);
 need(head(s,impl_->head.owner,impl_->head.city)==impl_->head,"Food production retained head/queue stale");
 if(impl_->counter_read)need(military(s,impl_->head.owner)==impl_->counter,"Food production military counter stale");
}
BnwCityFoodProductionOrder inspect_bnw_city_food_production_order(
 const LiveSessionState& s,std::shared_ptr<const BnwCityFoodProductionRules> rules,std::string_view sha,std::int64_t owner,std::int64_t city){
 // A provided retained rules facade is checked BEFORE the first borrowed
 // head/queue/world read, including the native empty/non-TRAIN branch.
 if(rules)rules->require_current(sha);
 auto out=BnwCityFoodProductionOrder::inspect_head_only(s,owner,city);
 if(out.requires_unit_rules())out.complete_from_same_rules(s,std::move(rules),sha);
 else if(rules){
  // No UnitInfo/counter is read on this branch, but an actually supplied
  // retained rules scope must remain checked before future head borrowing.
  auto p=std::make_shared<BnwCityFoodProductionOrder::Impl>(*out.impl_);
  p->rules=std::move(rules);out.impl_=std::move(p);
 }
 return out;
}
BnwCityFoodProductionOrder BnwCityFoodProductionOrder::inspect_head_only(const LiveSessionState& s,std::int64_t owner,std::int64_t city){
 auto p=std::make_shared<BnwCityFoodProductionOrder::Impl>();p->head=head(s,owner,city);
 p->complete=!p->head.order||p->head.order->type!=0;return BnwCityFoodProductionOrder(std::move(p));
}
bool BnwCityFoodProductionOrder::requires_unit_rules()const{check_thread();return impl_->head.order&&impl_->head.order->type==0;}
void BnwCityFoodProductionOrder::complete_from_same_rules(const LiveSessionState& s,std::shared_ptr<const BnwCityFoodProductionRules> rules,std::string_view sha){
 check_thread();need(!impl_->complete&&requires_unit_rules(),"Food production completed/invalid head-stage replay");
 need(bool(rules),"Food production TRAIN requires immutable BNW UnitInfo owner");rules->require_current(sha);
 need(head(s,impl_->head.owner,impl_->head.city)==impl_->head,"Food production retained head/queue stale");
 auto p=std::make_shared<BnwCityFoodProductionOrder::Impl>(*impl_);p->complete=true;
 auto unit=i32(p->head.order->item);
 if(p->head.order->source_hash){const auto* info=rules->impl_->types.find_hash(*p->head.order->source_hash);const auto expected=info&&info->table=="Units"?i32(info->id):-1;need(unit==expected,"Food production head UnitInfo hash resolution stale");}
 p->unit_read=true;p->rules=std::move(rules);
 const auto it=p->rules->impl_->units.find(unit);
 // GC.getUnitInfo has an explicit bounds check returning NULL, then a nullable
 // initialized vector slot. NULL Info returns before the military counter.
 if(unit<0||static_cast<std::uint64_t>(unit)>=p->rules->impl_->slots||it==p->rules->impl_->units.end()){impl_=std::move(p);return;}
 if(it->second.food){p->result=true;impl_=std::move(p);return;}
 p->counter_read=true;p->counter=military(s,p->head.owner);
 if(p->counter>0)p->result=it->second.military;
 impl_=std::move(p);
}
}
