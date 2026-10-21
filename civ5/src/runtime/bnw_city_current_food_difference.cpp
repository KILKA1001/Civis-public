#include "civ5/runtime/bnw_city_current_food_difference.hpp"
#include "civ5/runtime/bnw_city_current_food_consumption.hpp"
#include "civ5/runtime/bnw_city_food_production_order.hpp"
#include "civ5/runtime/bnw_city_food_production_conversion.hpp"
#include "civ5/runtime/bnw_city_food_scalars.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/runtime/bnw_city_strategy_yields.hpp"
#include "civ5/runtime/bnw_city_registry.hpp"
#include "civ5/runtime/bnw_religion_happiness_terms.hpp"
#include "civ5/runtime/plot_religion.hpp"
#include <algorithm>
#include <climits>
#include <utility>
#include <set>
#include <type_traits>
namespace civ5::runtime {
namespace {
void need(bool b,const char* m){if(!b)throw BnwCityCurrentFoodDifferenceUnavailable(m);}
std::int32_t native(std::int64_t n){need(n>=INT32_MIN&&n<=INT32_MAX,"FOOD difference native intermediate overflow");return static_cast<std::int32_t>(n);}
std::int32_t add(std::int32_t a,std::int32_t b){return native(std::int64_t(a)+b);}
using Seal=std::vector<std::int64_t>;
const auto& named(const auto& rows,std::string_view name){
 using T=typename std::remove_cvref_t<decltype(rows)>::value_type;const T* out=nullptr;
 for(const auto& row:rows)if(row.name&&std::string_view(row.name)==name){need(!out,"FOOD growth duplicate source field");out=&row;}
 need(out,"FOOD growth required source field absent");return *out;
}
void count(Seal& out,std::size_t n){need(n<=INT32_MAX,"FOOD growth native collection width unavailable");out.push_back(static_cast<std::int64_t>(n));}
void span(Seal& out,std::uint64_t begin,std::uint64_t end){need(begin<=end&&end<=INT64_MAX,"FOOD growth source span unavailable");out.push_back(static_cast<std::int64_t>(begin));out.push_back(static_cast<std::int64_t>(end));}
std::int32_t player_scalar(const auto& player,std::string_view name,Seal& seal){
 const auto& f=named(player.core.fields,name);
 need(f.encoding==saves::PlayerCoreEncoding::i32&&!f.wire_count&&f.values.size()==1&&f.bytes.empty()&&f.hashes.empty()&&f.row_counts.empty(),"FOOD growth actual player scalar encoding unavailable");
 span(seal,f.start_offset,f.end_offset);const auto value=native(f.values.front());seal.push_back(value);return value;
}
std::int32_t node_scalar(const auto& nodes,std::string_view name,Seal& seal){
 const auto& f=named(nodes,name);need(f.encoding&&std::string_view(f.encoding)=="i32"&&!f.wire_count&&f.values.size()==1&&f.children.empty(),"FOOD growth religion scalar encoding unavailable");
 span(seal,f.start_offset,f.end_offset);const auto value=native(f.values.front());seal.push_back(value);return value;
}
template<class F>auto typed(F&& f)->decltype(f()){try{return f();}catch(const BnwCityCurrentFoodDifferenceUnavailable&){throw;}catch(const std::exception& e){throw BnwCityCurrentFoodDifferenceUnavailable(e.what());}}
}
std::int32_t detail::bnw_food_difference_unmodified_times100(bool production,std::int32_t rate,std::int32_t consumption,bool bottom,std::optional<std::int32_t> population,std::optional<std::int32_t> food){return typed([&]{
 std::int32_t value;
 if(production){const auto excess=native(std::int64_t(rate/100)-consumption);const auto converted=bnw_city_food_production_from_excess(excess);const auto product=native(std::int64_t(converted)*100);value=std::min(0,product);}
 else{const auto consumed=native(std::int64_t(consumption)*100);value=native(std::int64_t(rate)-consumed);}
 if(bottom){need(population.has_value(),"FOOD difference bottom population unavailable");if(*population==1){need(food.has_value(),"FOOD difference bottom stock unavailable");BnwCityFoodScalarMirror source;source.food_times100=*food;if(source.food()==0)value=std::max(0,value);}}
 return value;
});}
std::int32_t detail::bnw_food_growth_religion(std::span<const std::int32_t> majority,std::optional<std::int32_t> secondary){
 std::int32_t total=0;for(const auto n:majority)total=add(total,n);if(secondary)total=add(total,*secondary);return total;
}
std::int32_t detail::bnw_food_growth_total(std::optional<std::int32_t> capital,std::int32_t city,std::optional<std::int32_t> religion,std::optional<std::int32_t> penalty){
 std::int32_t total=100;if(capital&&*capital)total=add(total,*capital);if(city)total=add(total,city);if(religion)total=add(total,*religion);if(penalty)total=add(total,*penalty);return total;
}
std::int32_t detail::bnw_food_growth_apply(std::int32_t difference,std::int32_t total){need(difference>0,"FOOD growth requires positive unmodified pure input");return native(std::int64_t(difference)*total)/100;}
detail::BnwFoodGrowthUnhappy detail::bnw_food_growth_unhappy(bool disabled,std::optional<std::int32_t> excess,std::optional<std::int32_t> threshold){
 if(disabled)return BnwFoodGrowthUnhappy::none;need(excess&&threshold,"FOOD growth actual excess/very-unhappy threshold unavailable");
 if(*excess<=*threshold)return BnwFoodGrowthUnhappy::very_unhappy;if(*excess<0)return BnwFoodGrowthUnhappy::unhappy;return BnwFoodGrowthUnhappy::none;
}
struct BnwCityCurrentFoodDifference::Growth {bool reached=false;std::int32_t total=100;Seal seal;bool operator==(const Growth&)const=default;};
struct BnwCityCurrentFoodDifference::Impl {
 BnwCityCurrentFoodRate rate;std::shared_ptr<const content::RulesDatabase> database;std::string sha;
 std::optional<BnwCityFoodProductionOrder> order;std::optional<BnwCityCurrentFoodConsumption> consumption;
 std::int32_t owner=-1,city=-1,result=0;bool bottom=true;
 std::optional<std::int32_t> population,stock;
 std::optional<Growth> growth;
 explicit Impl(BnwCityCurrentFoodRate&& source):rate(std::move(source)){}
};
BnwCityCurrentFoodDifference::Growth BnwCityCurrentFoodDifference::capture_growth(const Impl& p){
 // Metadata-only guard: this leaf never calls rate numeric capture/value.
 p.rate.require_current();const auto& s=p.rate.current_source_state();
 const auto db=p.rate.current_source_database_owner();need(db&&db==p.database,"FOOD growth SAME actual database owner differs");
 const auto& types=p.rate.current_source_info_types();const auto& defines=p.rate.current_source_defines();
 need(types.report().all_succeeded()&&!types.report().name_overwrites&&!types.report().hash_collisions,"FOOD growth native global Info collision provenance unavailable");
 constexpr auto bnw=saves::GameStateLayout::expansion2_version1;Growth out;out.reached=true;auto& seal=out.seal;
 need(s.world.all_players&&s.world.all_players->layout==bnw&&s.world.all_players->players.size()==64&&p.owner>=0&&p.owner<64,"FOOD growth actual player roster unavailable");
 const auto& player=s.world.all_players->players.at(static_cast<std::size_t>(p.owner));
 need(player.index==static_cast<std::uint64_t>(p.owner)&&player.saved_player_id==p.owner&&player.core.layout==bnw,"FOOD growth source player identity stale");
 const auto* raw=resolve_bnw_current_city(s,p.owner,p.city);need(raw,"FOOD growth canonical current city absent");
 const auto& live=s.cities.players.at(p.owner).cities.at(p.city);const auto& owned=s.cities.players.at(p.owner);
 span(seal,raw->core.start_offset,raw->core.end_offset);span(seal,player.core.start_offset,player.core.end_offset);
 const auto capital_handle=player_scalar(player,"m_iCapitalCityID",seal);need(owned.capital==capital_handle,"FOOD growth capital raw/owned handle stale");
 const auto* capital=resolve_bnw_city_registry(s.world,s.cities,p.owner,capital_handle);seal.push_back(capital==raw);
 std::optional<std::int32_t> capital_mod;if(capital==raw)capital_mod=player_scalar(player,"m_iCapitalGrowthMod",seal);
 // Check the source prefix BEFORE reading its next leaf, not only after all
 // later values have been collected (which could conceal overflow).
 auto running_total=detail::bnw_food_growth_total(capital_mod,0,{},{});
 const auto city_mod=player_scalar(player,"m_iCityGrowthMod",seal);
 if(city_mod)running_total=add(running_total,city_mod);
 // Prove EVERY native majority prefix before reusing the shared wide selector.
 need(raw->tail.layout==bnw,"FOOD growth actual city religion layout unavailable");
 const auto& status=named(named(raw->tail.blocks,"religions").children,"m_ReligionStatus");
 need(status.wire_count&&status.encoding&&std::string_view(status.encoding)=="signed_records"&&static_cast<std::uint64_t>(std::max(0,native(*status.wire_count)))==status.children.size()&&status.children.size()==live.religions.size(),"FOOD growth actual religion status count stale");
 span(seal,status.start_offset,status.end_offset);seal.push_back(*status.wire_count);
 std::int32_t total_followers=0,most=-1,pressure=0,major=-1;
 for(std::size_t i=0;i<status.children.size();++i){const auto& fields=status.children[i].children;const auto r=node_scalar(fields,"m_eReligion",seal),followers=node_scalar(fields,"m_iFollowers",seal),current_pressure=node_scalar(fields,"m_iPressure",seal);const auto& mirror=live.religions[i];
  need(mirror.religion==r&&mirror.followers==followers&&mirror.pressure==current_pressure,"FOOD growth religious majority raw/owned operands stale");
  total_followers=add(total_followers,followers);if(followers>most||(followers==most&&current_pressure>pressure)){most=followers;pressure=current_pressure;major=r;}
 }
 if(native(std::int64_t(most)*2)<total_followers)major=-1;seal.push_back(major);
 std::optional<std::int32_t> religion_mod;
 if(major!=-1){
  need(s.world.religion_culture.layout==bnw&&s.world.religion_culture.religions_version&&*s.world.religion_culture.religions_version>=2&&*s.world.religion_culture.religions_version<=4&&s.world.religion_culture.start_offset<s.world.religion_culture.culture_offset&&s.world.religion_culture.culture_offset<=s.world.religion_culture.end_offset,"FOOD growth actual global religion list unavailable");
  span(seal,s.world.religion_culture.start_offset,s.world.religion_culture.culture_offset);count(seal,s.world.religion_culture.religions.size());
  const saves::ReligionObservation* selected=nullptr;std::size_t selected_index=0;
  for(std::size_t i=0;i<s.world.religion_culture.religions.size();++i){const auto& r=s.world.religion_culture.religions[i];seal.push_back(native(r.religion));seal.push_back(native(r.founder));if(!selected&&r.religion==major&&(major!=0||r.founder==p.owner)){selected=&r;selected_index=i;}}
  seal.push_back(selected!=nullptr);
  if(selected){
   // The actual source reads team peace BEFORE GetCityGrowthModifier.
   p.rate.require_current();const auto peace=BnwReligionFollowersReadModel::bind(s).at_peace(p.owner);seal.push_back(peace);
   need(s.world.teams&&s.world.teams->dimensions.teams==64&&s.world.teams->dimensions.civ_teams==63,"FOOD growth source team domains unavailable");
   span(seal,s.world.teams->start_offset,s.world.teams->end_offset);
   for(const auto& t:s.world.teams->teams){seal.push_back(native(named(t.fields,"id").value));seal.push_back(native(named(t.fields,"alive_count").value));const auto& war=named(t.arrays,"at_war");count(seal,war.values.size());for(const auto flag:war.values)seal.push_back(native(flag));}
   count(seal,s.setup.players.size());for(const auto& [id,setup]:s.setup.players){seal.push_back(native(id));seal.push_back(native(setup.team));seal.push_back(setup.alive);}
   struct Belief{std::int32_t growth=0;bool pantheon=false,requires_peace=false;};std::map<std::int64_t,Belief> infos;
   p.rate.require_current();const auto report=db->prefetch_info("Beliefs",[&](auto slot,auto authored,const auto& row){
    need(slot<=INT32_MAX,"FOOD growth native BeliefInfo span unavailable");const auto id=static_cast<std::int32_t>(slot);const auto type=row.text("Type");const auto* entry=type?types.find(*type):nullptr;const auto* wire=entry?types.find_hash(entry->hash):nullptr;
    need(type&&!type->empty()&&authored==id&&entry&&entry->table=="Beliefs"&&entry->id==id&&entry->hash==content::saved_info_type_hash(*type)&&entry->hash&&wire&&wire->table==entry->table&&wire->type==entry->type&&wire->id==entry->id,"FOOD growth actual BeliefInfo Type/hash/slot identity unavailable");
    const Belief info{native(row.integer("CityGrowthModifier")),row.boolean("Pantheon"),row.boolean("RequiresPeace")};need(infos.emplace(id,info).second,"FOOD growth duplicate BeliefInfo slot");seal.push_back(id);seal.push_back(entry->hash);seal.push_back(info.growth);seal.push_back(info.pantheon);seal.push_back(info.requires_peace);return true;
   });p.rate.require_current();need(report.all_succeeded()&&!report.overlapping_ids&&report.slots<=INT32_MAX&&report.holes<=report.slots&&infos.size()<=report.slots&&infos.size()+report.holes==report.slots,"FOOD growth native BeliefInfo collection incomplete");seal.push_back(static_cast<std::int64_t>(report.slots));seal.push_back(static_cast<std::int64_t>(report.holes));
   auto context=s.cities;need(context.pantheon==0&&context.religions.size()==s.world.religion_culture.religions.size(),"FOOD growth global religion raw/owned count stale");
   // Preserve source list order/duplicates, unknown->NO_BELIEF native remap,
   // and prove all retained cache IDs before shared selection.
   std::vector<std::vector<std::int32_t>> native_lists;native_lists.reserve(context.religions.size());
   for(std::size_t i=0;i<context.religions.size();++i){const auto& raw_religion=s.world.religion_culture.religions[i];const auto& mirror=context.religions[i];
    need(raw_religion.beliefs.version&&*raw_religion.beliefs.version>=1&&*raw_religion.beliefs.version<=2&&mirror.religion==raw_religion.religion&&mirror.founder==raw_religion.founder,"FOOD growth actual religion/hash Read version stale");
    count(seal,raw_religion.beliefs.belief_hashes.size());std::vector<std::int64_t> expected;std::vector<std::int32_t> source_ids;
    for(const auto h:raw_religion.beliefs.belief_hashes){need(h<=UINT32_MAX,"FOOD growth native belief hash outside width");seal.push_back(static_cast<std::int64_t>(h));const auto* t=h?types.find_hash(h):nullptr;need(!t||t->table=="Beliefs","FOOD growth reached cross-table belief hash unavailable");const auto id=t?native(t->id):-1;source_ids.push_back(id);if(id>=0&&static_cast<std::uint64_t>(id)<report.slots)expected.push_back(id);}
    need(expected==mirror.beliefs,"FOOD growth belief membership raw/owned stale");native_lists.push_back(std::move(source_ids));
   }
   const auto& membership=context.religions.at(selected_index).beliefs;std::set<std::int64_t> unique(membership.begin(),membership.end());
   for(const auto id:unique)need(infos.contains(id),"FOOD growth majority references a null native BeliefInfo slot");
   std::vector<std::int32_t> majority_values;majority_values.reserve(unique.size());
   for(const auto id:unique){const auto& entry=infos.at(id);if(peace||!entry.requires_peace)majority_values.push_back(entry.growth);}
   const auto majority_sum=detail::bnw_food_growth_religion(majority_values);
   // Native majority arithmetic completes BEFORE its secondary getter.
   const auto second=player_scalar(player,"m_iSecondReligionPantheonCount",seal);if(owned.second_pantheon_count)need(*owned.second_pantheon_count==second,"FOOD growth secondary pantheon raw/owned counter stale");context.players.at(p.owner).second_pantheon_count=second;
   // Secondary calls GetEntry directly, so -1/out-of-range BEFORE its first
   // pantheon match is a native unsupported operand, never silently skipped.
   if(second>0){std::int32_t sec=-1,best=-1,sec_pressure=-1;
    for(const auto& r:live.religions)if(r.religion!=major&&r.religion>0&&r.followers>0&&(r.followers>best||(r.followers==best&&r.pressure>sec_pressure))){sec=native(r.religion);best=native(r.followers);sec_pressure=native(r.pressure);}
    if(sec!=-1)for(std::size_t i=0;i<context.religions.size();++i)if(context.religions[i].religion==sec){for(const auto id:native_lists[i]){need(id>=0&&static_cast<std::uint64_t>(id)<report.slots,"FOOD growth reached secondary native BeliefInfo index unavailable");const auto entry=infos.find(id);if(entry!=infos.end()&&entry->second.pantheon)break;}break;}
   }
   const auto selected_beliefs=bnw_plot_beliefs(context,context.players.at(p.owner).cities.at(p.city),infos);
   need(selected_beliefs.size()>=unique.size()&&selected_beliefs.size()<=unique.size()+1,"FOOD growth shared majority/secondary boundary differs");
   std::size_t selected_index_in_prefix=0;for(const auto id:unique){need(selected_beliefs[selected_index_in_prefix]==&infos.at(id),"FOOD growth shared majority collection order differs");++selected_index_in_prefix;}
   const auto secondary=selected_beliefs.size()>unique.size()?std::optional{selected_beliefs.back()->growth}:std::nullopt;
   religion_mod=secondary?add(majority_sum,*secondary):majority_sum;seal.push_back(*religion_mod);running_total=add(running_total,*religion_mod);
  }
 }
 // Rate value was already revalidated before this branch, including the
 // actual NO_HAPPINESS setup and native unhappiness/excess mirror.
 const auto define=[&](const char* name){p.rate.require_current();const auto* d=defines.find(name);need(d&&d->kind==content::GlobalDefineKind::integer&&d->lookup.found&&d->lookup.error.empty()&&d->lookup.sqlite_code==0&&std::holds_alternative<std::int64_t>(d->lookup.value),"FOOD growth reached actual integer define unavailable");const auto value=native(std::get<std::int64_t>(d->lookup.value));seal.push_back(value);return value;};
 seal.push_back(s.cities.no_happiness);std::optional<std::int32_t> excess,threshold,penalty;
 if(!s.cities.no_happiness){need(owned.excess_happiness.has_value(),"FOOD growth SAME native excess happiness unavailable");excess=native(*owned.excess_happiness);seal.push_back(*excess);threshold=define("VERY_UNHAPPY_THRESHOLD");
  // Native GetExcessHappiness was proven by the immediately prior rate read.
  // No saved unhappiness/final growth cache is substituted.
 }
 const auto unhappy_kind=detail::bnw_food_growth_unhappy(s.cities.no_happiness,excess,threshold);
 if(unhappy_kind==detail::BnwFoodGrowthUnhappy::very_unhappy)penalty=define("VERY_UNHAPPY_GROWTH_PENALTY");
 else if(unhappy_kind==detail::BnwFoodGrowthUnhappy::unhappy)penalty=define("UNHAPPY_GROWTH_PENALTY");
 const auto unhappy=unhappy_kind!=detail::BnwFoodGrowthUnhappy::none;
 seal.push_back(unhappy);
 if(!unhappy){const auto wltkd=p.rate.canonical_core_scalar("m_iWeLoveTheKingDayCounter");seal.push_back(wltkd);if(live.we_love_king_turns)need(*live.we_love_king_turns==wltkd,"FOOD growth WLTKD raw/owned counter stale");const auto& f=named(raw->core.fields,"m_iWeLoveTheKingDayCounter");span(seal,f.start_offset,f.end_offset);if(wltkd>0)penalty=define("WLTKD_GROWTH_MULTIPLIER");}
 if(penalty)running_total=add(running_total,*penalty);out.total=running_total;seal.push_back(out.total);p.rate.require_current();return out;
}
BnwCityCurrentFoodDifference::BnwCityCurrentFoodDifference(std::shared_ptr<const Impl> p)noexcept:impl_(std::move(p)){}
std::int32_t BnwCityCurrentFoodDifference::evaluate(const Impl& p,Growth* initial){
 // The retained rate source's metadata/rules/thread/allocation guard precedes
 // EVERY borrowed state/head query. Actual head predicate precedes rate math.
 p.rate.require_current();need(p.order&&p.consumption,"FOOD difference incomplete source children");
 p.order->require_same_inputs(p.rate.current_source_state(),p.sha);const auto production=p.order->food_production();
 const auto rate=p.rate.value_times100();
 p.consumption->require_same_inputs(p.rate.current_source_state(),p.sha);const auto consumption=p.consumption->value();
 std::optional<std::int32_t> population,stock;
 if(p.bottom){population=p.rate.canonical_core_scalar("m_iPopulation");if(*population==1)stock=p.rate.canonical_core_scalar("m_iFood");}
 need(population==p.population&&stock==p.stock,"FOOD difference retained bottom operands stale");
 auto result=detail::bnw_food_difference_unmodified_times100(production,rate,consumption,p.bottom,population,stock);
 if(result>0){auto growth=capture_growth(p);if(initial)*initial=growth;else need(p.growth&&*p.growth==growth,"FOOD growth retained actual source operands stale");result=detail::bnw_food_growth_apply(result,growth.total);}
 else need(!p.growth,"FOOD growth reached branch changed");
 p.rate.require_current();return result;
}
BnwCityCurrentFoodDifference BnwCityCurrentFoodDifference::make(std::shared_ptr<const LiveSession> session,std::shared_ptr<const BnwCityFoodRateRules> rules,std::int64_t owner,std::int64_t city,bool bottom){return typed([&]{
 auto p=std::make_shared<Impl>(BnwCityCurrentFoodRate::bind_source_only(std::move(session),std::move(rules),owner,city));
 p->owner=p->rate.canonical_owner();p->city=p->rate.canonical_city();p->bottom=bottom;
 p->database=p->rate.current_source_database_owner();p->sha=p->rate.current_rules_sha();
 // First gameplay branch getter: head/queue only. Do NOT run eager rate capture
 // or load/read UnitInfo on an empty/non-TRAIN native branch.
 p->order=BnwCityFoodProductionOrder::inspect_head_only(p->rate.current_source_state(),p->owner,p->city);
 if(p->order->requires_unit_rules()){
  p->rate.require_current();auto unit_rules=BnwCityFoodProductionRules::load(*p->database,p->sha);
  p->rate.require_current();p->order->complete_from_same_rules(p->rate.current_source_state(),std::move(unit_rules),p->sha);
 }
 (void)p->order->food_production(); // Resolve the actual predicate BEFORE rate.
 p->rate.capture_initial();
 p->consumption=BnwCityCurrentFoodConsumption::from_same_rate(p->rate,p->owner,p->city,0);
 if(p->bottom){p->population=p->rate.canonical_core_scalar("m_iPopulation");if(*p->population==1)p->stock=p->rate.canonical_core_scalar("m_iFood");}
 Growth first;p->result=evaluate(*p,&first);if(first.reached)p->growth=std::move(first);return BnwCityCurrentFoodDifference(std::move(p));
});}
std::int32_t BnwCityCurrentFoodDifference::value_times100()const{return typed([&]{
 need(bool(impl_),"FOOD difference moved result");const auto p=impl_;const auto value=evaluate(*p);need(value==p->result,"FOOD difference retained numeric result stale");return value;
});}
BnwCityCurrentFoodDifference inspect_bnw_city_current_food_difference(std::shared_ptr<const LiveSession> session,std::shared_ptr<const BnwCityFoodRateRules> rules,std::int64_t owner,std::int64_t city,bool bottom){return BnwCityCurrentFoodDifference::make(std::move(session),std::move(rules),owner,city,bottom);}
}
