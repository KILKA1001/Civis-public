#include "civ5/runtime/bnw_city_production_turn.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/runtime/bnw_city_registry.hpp"
#include "civ5/runtime/bnw_city_strategy_yields.hpp"
#include "civ5/runtime/bnw_city_food_production_order.hpp"
#include "civ5/runtime/bnw_city_current_science_rate.hpp"
#include "civ5/runtime/bnw_game_active_team_read.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <algorithm>
#include <atomic>
#include <climits>
#include <functional>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <vector>
#include <variant>
namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void need(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
std::int32_t native(std::int64_t n){need(n>=INT32_MIN&&n<=INT32_MAX,"Production turn native int32 overflow");return static_cast<std::int32_t>(n);}
std::int32_t add(std::int32_t a,std::int32_t b){return native(static_cast<std::int64_t>(a)+b);}
std::int32_t player_project_modifier(std::int32_t supply,std::int32_t space){return add(supply,space);}
std::int32_t city_project_prefix(std::int32_t general,std::int32_t player){return add(general,player);}
std::int32_t sub(std::int32_t a,std::int32_t b){return native(static_cast<std::int64_t>(a)-b);}
std::int32_t mul(std::int32_t a,std::int32_t b){return native(static_cast<std::int64_t>(a)*b);}
std::int32_t free_supply(std::int32_t base,std::int32_t per_city,std::int32_t cities,std::int32_t percent,std::int32_t population){
 auto free=add(base,mul(per_city,cities));return add(free,mul(population,percent)/100);
}
std::int32_t supply_penalty(std::int32_t paid,std::int32_t cap){return native(-static_cast<std::int64_t>(std::min(cap,mul(paid,10))));}
// Preserve the existing const observation path. Mutable source records are
// selected only from non-const native storage for the actual TRAIN stores.
template<class Rows>requires(!std::is_const_v<Rows>)auto& named(Rows& rows,std::string_view name){
 using T=typename Rows::value_type;T* out=nullptr;
 for(auto& f:rows)if(f.name&&name==f.name){need(!out,"Production turn duplicate source field");out=&f;}
 need(out,"Production turn missing source field");return *out;
}
template<class Rows>const auto& named(const Rows& rows,std::string_view name){
 using T=typename Rows::value_type;const T* out=nullptr;
 for(const auto& f:rows)if(f.name&&name==f.name){need(!out,"Production turn duplicate source field");out=&f;}
 need(out,"Production turn missing source field");return *out;
}
const saves::CityCoreField& cfield(const saves::SavedCityObservation& c,std::string_view name,saves::CityCoreEncoding e){
 const auto& f=named(c.core.fields,name);need(c.core.layout==bnw&&f.encoding==e&&f.values.size()==1&&!f.wire_count&&f.row_keys.empty()&&f.row_counts.empty()&&f.bytes.empty()&&f.hashes.empty(),"Production turn native city scalar shape unavailable");(void)native(f.values.front());return f;
}
std::int32_t scalar(const saves::SavedCityObservation& c,std::string_view name){return native(cfield(c,name,saves::CityCoreEncoding::i32).values.front());}
bool flag(const saves::SavedCityObservation& c,std::string_view name){const auto n=cfield(c,name,saves::CityCoreEncoding::boolean).values.front();need(n==0||n==1,"Production turn malformed native city boolean");return n==1;}
std::int32_t pscalar(const saves::SavedPlayerObservation& p,std::string_view name){
 const auto& f=named(p.core.fields,name);need(p.core.layout==bnw&&f.encoding==saves::PlayerCoreEncoding::i32&&f.values.size()==1&&!f.wire_count&&f.bytes.empty()&&f.hashes.empty()&&f.row_counts.empty(),"Production turn native player scalar shape unavailable");return native(f.values.front());
}
struct Rules {
 struct Unit {std::int32_t cost=0,unit_class=-1,domain=-1,combat=-1,special=-1,spaceship=-1;bool military=false,found=false,food=false;std::map<std::int32_t,std::int32_t> building_modifiers;};
 std::map<std::int32_t,Unit> units;std::map<std::int32_t,std::int32_t> unit_classes;std::set<std::int32_t> nonzero_traits;
 std::uint64_t unit_class_slots=0,combat_slots=0;
 struct Handicap{std::int32_t base=0,city=0,population=0;};
 std::string sha;std::function<bool()> current;content::InfoTypeCatalog types;
 std::map<std::int32_t,Handicap> handicaps;std::map<std::int32_t,bool> trade;
 struct Project{std::int32_t cost=0;bool spaceship=false;};
 std::uint64_t project_slots=0;std::map<std::int32_t,Project> projects;
 std::map<std::int32_t,std::int32_t> speeds,eras;
 std::set<std::int32_t> processes;std::vector<std::int32_t> league_processes;
};
Rules load(const ui::SelectedBnwPreviewHost& host,bool project,bool train=false){
 const auto& db=host.database();Rules r;r.current=db.immutable_preview_fingerprint_currentness();
 need(r.current&&r.current(),"Production turn SAME immutable rules unavailable");r.sha=db.gameplay_fingerprint_sha256();
 need(r.sha==host.identity().rules_sha256,"Production turn actual rules identity differs");r.types=db.cache_info_types();need(r.types.report().all_succeeded(),"Production turn native Info catalog unavailable");
 auto table=[&](const char* name,auto use){
  const auto report=db.prefetch_info(name,[&](auto slot,auto authored,const auto& row){
   need(slot<=INT32_MAX,"Production turn native Info span overflow");const auto id=static_cast<std::int32_t>(slot);const auto type=row.text("Type");
   const auto* info=type?r.types.find(*type):nullptr;const auto* wire=info?r.types.find_hash(info->hash):nullptr;
   need(type&&info&&authored==id&&info->table==name&&info->id==id&&info->hash==content::saved_info_type_hash(*type)&&wire&&wire->table==info->table&&wire->type==*type&&wire->id==id,"Production turn actual Info slot/Type/hash differs");
   use(id,row);return true;
  });need(report.all_succeeded()&&!report.overlapping_ids&&report.slots<=INT32_MAX,"Production turn Info collection unavailable");return report.slots;
 };
 table("HandicapInfos",[&](auto id,const auto& row){r.handicaps.emplace(id,Rules::Handicap{native(row.integer("ProductionFreeUnits")),native(row.integer("ProductionFreeUnitsPerCity")),native(row.integer("ProductionFreeUnitsPopulationPercent"))});});
 table("Units",[&](auto id,const auto& row){(void)native(row.integer("Trade"));r.trade.emplace(id,row.boolean("Trade"));});
 table("Processes",[&](auto id,const auto&){r.processes.insert(id);});
 table("LeagueProjects",[&](auto,const auto& row){const auto text=row.text("Process");const auto* process=text?r.types.find(*text):nullptr;
  if(process)need(process->table=="Processes"&&r.processes.contains(native(process->id)),"Production turn LeagueProject Process remap outside current Processes");
  r.league_processes.push_back(process?native(process->id):-1);
 });
 if(project){
  r.project_slots=table("Projects",[&](auto id,const auto& row){r.projects.emplace(id,Rules::Project{native(row.integer("Cost")),row.boolean("Spaceship")});});
  table("GameSpeeds",[&](auto id,const auto& row){r.speeds.emplace(id,native(row.integer("CreatePercent")));});
  table("Eras",[&](auto id,const auto& row){r.eras.emplace(id,native(row.integer("CreatePercent")));});
 }
 if(train){
  const auto ref=[&](const std::optional<std::string>& key,std::string_view table){if(!key||key->empty())return std::int32_t{-1};const auto* t=r.types.find(*key);need(t&&t->table==table&&t->id>=0,"Production TRAIN Info relation unavailable");return native(t->id);};
  table("Units",[&](auto id,const auto& row){Rules::Unit u;u.cost=native(row.integer("Cost"));u.unit_class=ref(row.text("Class"),"UnitClasses");u.domain=ref(row.text("Domain"),"Domains");u.combat=ref(row.text("CombatClass"),"UnitCombatInfos");u.special=ref(row.text("Special"),"SpecialUnits");u.spaceship=ref(row.text("SpaceshipProject"),"Projects");u.military=row.boolean("MilitaryProduction");u.found=row.boolean("Found");u.food=row.boolean("Food");r.units.emplace(id,std::move(u));});
  r.unit_class_slots=table("UnitClasses",[&](auto id,const auto& row){r.unit_classes.emplace(id,native(row.integer("InstanceCostModifier")));});
  r.combat_slots=table("UnitCombatInfos",[&](auto,const auto&){});
  table("GameSpeeds",[&](auto id,const auto& row){r.speeds.emplace(id,native(row.integer("TrainPercent")));});
  table("Eras",[&](auto id,const auto& row){r.eras.emplace(id,native(row.integer("TrainPercent")));});
  auto rows=db.query_rows({"SELECT Units.ID, Trait FROM Unit_ProductionTraits INNER JOIN Units ON UnitType=Units.Type INNER JOIN Traits ON TraitType=Traits.Type",{}},[&](const auto& row){const auto id=native(row.integer_at(0));need(r.units.contains(id),"Production TRAIN trait UnitInfo outside active slots");if(native(row.integer("Trait"))!=0)r.nonzero_traits.insert(id);});need(rows.all_succeeded(),"Production TRAIN actual trait modifiers query unavailable");
  rows=db.query_rows({"SELECT Units.ID AS UnitID, Buildings.ID AS BuildingID, ProductionModifier FROM Unit_ProductionModifierBuildings INNER JOIN Units ON UnitType=Units.Type INNER JOIN Buildings ON BuildingType=Buildings.Type",{}},[&](const auto& row){const auto u=native(row.integer("UnitID")),b=native(row.integer("BuildingID"));need(r.units.contains(u)&&host.production_rules().buildings.contains(b),"Production TRAIN actual Building modifier remap outside active slots");r.units.at(u).building_modifiers[b]=native(row.integer("ProductionModifier"));});need(rows.all_succeeded(),"Production TRAIN actual Building modifiers query unavailable");
 }
 need(r.current(),"Production turn rules retired during metadata load");return r;
}
std::int32_t define(const ui::SelectedBnwPreviewHost& host,std::string_view name){
 const auto* f=host.defines().find(name);need(f&&f->kind==content::GlobalDefineKind::integer&&f->lookup.found&&f->lookup.error.empty(),"Production turn reached Define unavailable");
 const auto* value=std::get_if<std::int64_t>(&f->lookup.value);need(value,"Production turn reached Define type unavailable");return native(*value);
}
using Seal=std::vector<std::int64_t>;
struct Facts {BnwCityProductionTurnKey key{};std::int32_t process=-1,overflow=0,feature=0,needed=INT32_MAX,stored=0,modifier=0;bool project=false,train=false,suppressed=false;std::map<std::int64_t,std::int64_t> unit_values;Seal seal;};
Facts capture(const LiveSessionState& s,const saves::SavedSession& source,
 const ui::SelectedBnwPreviewHost& host,const Rules& rules,std::int32_t owner,std::int32_t handle,bool project,bool train=false){
 need(s.setup.profile==content::TechnologyProfile::bnw&&s.cities.profile==content::TechnologyProfile::bnw&&s.world.game_state.layout==bnw&&s.world.pregame&&s.world.pregame->layout==bnw,"Production turn requires actual BNW world");
 const auto turn=native(s.world.pregame->archive.integers.at("game_turn"));need(turn>=0&&s.setup.game_turn==turn,"Production turn raw/owned turn differs");
 const auto* c=resolve_bnw_current_city(s,owner,handle);need(c,"Production turn actual city absent");const auto id=scalar(*c,"m_iID");
 const auto* original=resolve_bnw_city_registry(source.snapshot,s.cities,owner,id);
 need(original&&original->core.version==c->core.version&&original->core.start_offset==c->core.start_offset&&original->core.end_offset==c->core.end_offset&&original->production.start_offset==c->production.start_offset&&original->production.end_offset==c->production.end_offset,"Production turn original city schema/identity differs");
 auto& live=s.cities.players.at(owner).cities.at(id);
 need(live.owner==owner&&live.id==id&&live.production_orders&&live.production_progress&&live.production_progress->source_profile==content::TechnologyProfile::bnw&&live.production_progress->fully_resolved,"Production turn owned production unresolved");
 Facts f;f.project=project;f.train=train;f.key={turn,owner,id,c->core.start_offset,c->core.end_offset};f.seal={turn,owner,id};
 const auto rawbool=[&](std::string_view n){const bool value=flag(*c,n);f.seal.push_back(value);return value;};
 const auto raw=[&](std::string_view n){const auto value=scalar(*c,n);f.seal.push_back(value);return value;};
 // CvCity13184: Human FIRST, ProductionAutomated only if Human.
 need(detail::bnw_player_human_read(s,owner,host.database()),"Production turn requires actual AI_chooseProduction owner for nonhuman");f.seal.push_back(1);
 need(!rawbool("m_bProductionAutomated"),"Production turn requires actual AI_chooseProduction owner for automated city");
 need(!live.production_orders->empty(),"Production turn empty queue requires doCheckProduction allow-no-production lineage");
 const auto& q=live.production_orders->front();need(q.type==(train?0:project?2:4),"Production turn queued item cost/modifier/wonder/spawn producer unavailable");
 auto head=inspect_bnw_city_food_production_order(s,train?BnwCityFoodProductionRules::load(host.database(),rules.sha):std::shared_ptr<const BnwCityFoodProductionRules>{},rules.sha,owner,id);head.require_same_inputs(s,rules.sha);if(train)need(!head.food_production(),"Production TRAIN actual food-conversion path requires food producer");
 f.process=native(q.item);need(train?rules.units.contains(f.process):project?rules.projects.contains(f.process):rules.processes.contains(f.process),"Production turn native queued Info unavailable");
 if(q.source_hash){const auto* info=rules.types.find_hash(*q.source_hash);need(info&&info->table==(train?"Units":project?"Projects":"Processes")&&info->id==f.process,"Production turn source Process hash remap differs");}
 f.seal.insert(f.seal.end(),{q.type,q.item,q.secondary,q.save,q.rush,q.source_hash?static_cast<std::int64_t>(*q.source_hash):-1});
 if(project){
  const auto& minors=s.world.pregame->archive.flag_arrays.at("minor_nation_civs");need(owner>=0&&owner<64&&minors.size()==64&&minors[static_cast<std::size_t>(owner)]==s.cities.players.at(owner).minor,"Production turn project Minor source/mirror differs");
  auto cost=rules.projects.at(f.process).cost;f.seal.push_back(cost);
  if(minors[static_cast<std::size_t>(owner)]){const auto percent=define(host,"MINOR_CIV_PRODUCTION_PERCENT");cost=mul(cost,percent)/100;f.seal.push_back(percent);}
  const auto percent=define(host,"PROJECT_PRODUCTION_PERCENT");cost=mul(cost,percent)/100;f.seal.push_back(percent);
  const auto speed=native(s.world.pregame->archive.integers.at("game_speed"));need(s.setup.speed==speed&&rules.speeds.contains(speed),"Production turn SAME native SpeedInfo unavailable");cost=mul(cost,rules.speeds.at(speed))/100;
  const auto era=native(s.world.pregame->archive.integers.at("era"));need(s.setup.start_era==era&&rules.eras.contains(era),"Production turn SAME native start EraInfo unavailable");cost=mul(cost,rules.eras.at(era))/100;
  f.needed=std::max(1,cost);f.seal.insert(f.seal.end(),{speed,rules.speeds.at(speed),era,rules.eras.at(era),f.needed});
  const auto& values=named(c->core.fields,"m_paiProjectProduction");need(values.encoding==saves::CityCoreEncoding::vector_i32&&values.wire_count==rules.project_slots&&values.values.size()==rules.project_slots&&values.hashes.empty()&&values.bytes.empty()&&values.row_counts.empty()&&values.row_keys.empty()&&f.process>=0&&static_cast<std::uint64_t>(f.process)<rules.project_slots,"Production turn native Project Reset/plain-Read vector unavailable");
  const auto& original_values=named(original->core.fields,"m_paiProjectProduction");need(values.start_offset==original_values.start_offset&&values.end_offset==original_values.end_offset,"Production turn native Project Read source span differs");
  for(std::size_t i=0;i<values.values.size();++i){const auto v=native(values.values[i]);const auto it=live.production_progress->projects.find(static_cast<std::int64_t>(i));need((it==live.production_progress->projects.end()?0:it->second)==v,"Production turn Project raw/owned vector differs");}
  f.stored=native(values.values.at(static_cast<std::size_t>(f.process)));
 }
 if(train){
  const auto u=rules.units.find(f.process);need(u!=rules.units.end(),"Production TRAIN native UnitInfo unavailable");const auto& info=u->second;
  need(!info.found&&!info.food&&info.domain==2&&info.special==-1&&info.spaceship==-1,
       "Production TRAIN reached settler/food/special/space/domain owner unavailable");
  need(!rules.nonzero_traits.contains(f.process),"Production TRAIN reached nonzero active-trait production owner unavailable");
  const auto& owner_player=s.world.all_players->players.at(static_cast<std::size_t>(owner));
  const auto& minors=s.world.pregame->archive.flag_arrays.at("minor_nation_civs");
  need(minors.size()==64&&!minors.at(static_cast<std::size_t>(owner))&&!s.cities.players.at(owner).minor,"Production TRAIN cost finite major Human branch required");
  const auto cls=rules.unit_classes.find(info.unit_class);need(cls!=rules.unit_classes.end()&&cls->second==0,"Production TRAIN instance-cost owner unavailable");
  const auto& count=named(owner_player.core.fields,"m_paiUnitClassCount");
  need(count.encoding==saves::PlayerCoreEncoding::vector_i32&&count.wire_count==rules.unit_class_slots&&count.values.size()==rules.unit_class_slots&&info.unit_class>=0&&static_cast<std::uint64_t>(info.unit_class)<rules.unit_class_slots,
       "Production TRAIN actual class count vector unavailable");
  const auto class_count=native(count.values.at(static_cast<std::size_t>(info.unit_class)));need(class_count>=0,"Production TRAIN negative class count");f.seal.push_back(class_count);
  auto cost=mul(info.cost,100)/100; // exact native instance-modifier0 operation
  cost=mul(cost,define(host,"UNIT_PRODUCTION_PERCENT"))/100;
  const auto speed=native(s.world.pregame->archive.integers.at("game_speed")),era=native(s.world.pregame->archive.integers.at("era"));
  need(speed==s.setup.speed&&era==s.setup.start_era&&rules.speeds.contains(speed)&&rules.eras.contains(era),"Production TRAIN actual speed/start era differs");
  cost=mul(cost,rules.speeds.at(speed))/100;cost=mul(cost,rules.eras.at(era))/100;
  const auto& extra=named(owner_player.tail.fields,"m_aUnitExtraCosts");
  need(extra.encoding&&std::string_view(extra.encoding)=="stl_pairs"&&extra.wire_count==0&&extra.values.empty()&&extra.children.empty()&&extra.bytes.empty(),"Production TRAIN reached extra cost owner unavailable");
  f.needed=std::max(1,cost);f.seal.insert(f.seal.end(),{speed,era,f.needed});
  need(c->production.context.bnw_city_version&&*c->production.context.bnw_city_version>=5,"Production TRAIN native hashed progress version unavailable");
  const auto& block=named(named(c->production.blocks,"before_queue").children,"m_paiUnitProduction");
  need(block.fields.size()==1&&named(block.fields,"wire_count").value>=0&&static_cast<std::uint64_t>(named(block.fields,"wire_count").value)==block.children.size()&&block.arrays.empty()&&block.blobs.empty(),"Production TRAIN hashed progress count/shape differs");
  for(const auto& row:block.children){
   need(row.fields.size()==1&&row.children.empty()&&row.blobs.empty(),"Production TRAIN progress row shape differs");const auto hash=named(row.fields,"hash").value;need(hash>=0&&hash<=UINT32_MAX,"Production TRAIN progress hash width differs");
   if(!hash){need(row.arrays.empty(),"Production TRAIN null hash has payload");continue;}
   const auto* type=rules.types.find_hash(static_cast<std::uint32_t>(hash));need(type&&type->table=="Units"&&rules.units.contains(native(type->id))&&row.arrays.size()==1,"Production TRAIN progress hash unresolved");
   const auto& values=named(row.arrays,"values");need(values.columns==1&&values.values.size()==1,"Production TRAIN progress native value shape differs");
   f.unit_values[type->id]=native(values.values.front());
  }
  need(f.unit_values==live.production_progress->units,"Production TRAIN actual raw/owned progress differs");
  const auto stored=f.unit_values.find(f.process);f.stored=stored==f.unit_values.end()?0:native(stored->second);
 }
 // The actual MAINTAIN getters yield needed=INT_MAX, stored production=0,
 // food=false. Modifier still reaches city route and player UNIT SUPPLY.
 const auto industrial=rawbool("m_bIndustrialRouteToCapital");
 std::int32_t modifier=industrial?define(host,"INDUSTRIAL_ROUTE_PRODUCTION_MOD"):0;f.seal.push_back(modifier);
 need(owner>=0&&owner<64&&s.world.all_players&&s.world.all_players->layout==bnw&&s.world.all_players->context.player_count==64&&s.world.all_players->players.size()==64,"Production turn native roster unavailable");
 const auto& p=s.world.all_players->players[static_cast<std::size_t>(owner)];need(p.index==static_cast<std::uint64_t>(owner)&&p.saved_player_id==owner&&pscalar(p,"m_eID")==owner,"Production turn actual player ID differs");
 const auto& handicaps=s.world.pregame->archive.integer_arrays.at("handicaps");need(handicaps.size()==64,"Production turn native Handicap roster unavailable");
 const auto handicap=native(handicaps[static_cast<std::size_t>(owner)]);need(s.setup.players.at(owner).handicap==handicap&&rules.handicaps.contains(handicap),"Production turn raw/owned Handicap differs");const auto h=rules.handicaps.at(handicap);
 need(p.cities.layout==bnw&&p.cities.header.wire_city_count>=0&&static_cast<std::uint64_t>(p.cities.header.wire_city_count)==p.cities.cities.size()&&p.cities.cities.size()==s.cities.players.at(owner).cities.size(),"Production turn native city count differs");
 const auto count=native(p.cities.header.wire_city_count),pop=pscalar(p,"m_iTotalPopulation");need(s.cities.players.at(owner).total_population==pop,"Production turn population raw/owned mirror differs");
 const auto free=free_supply(h.base,h.city,count,h.population,pop); // native prefix BEFORE Minor/Unit getters
 const auto& minors=s.world.pregame->archive.flag_arrays.at("minor_nation_civs");need(minors.size()==64&&minors[static_cast<std::size_t>(owner)]==s.cities.players.at(owner).minor,"Production turn raw/owned Minor predicate differs");
 // actual Human above means the AI multiplier is not reached, regardless Minor.
 need(p.units.layout==bnw&&p.units.header.wire_unit_count>=0&&static_cast<std::uint64_t>(p.units.header.wire_unit_count)==p.units.units.size()&&p.units.units.size()==s.maintenance.players.at(owner).units.size(),"Production turn native unit count differs");
 std::int32_t trade=0;std::set<std::int32_t> ids;
 for(const auto& unit:p.units.units){
  need(unit.core.layout==bnw,"Production turn native Unit layout differs");
  const auto uscalar=[&](std::string_view n){const auto& v=named(unit.core.fields,n);need(v.values.size()==1&&!v.wire_count&&v.bytes.empty()&&(v.encoding==saves::UnitCoreEncoding::i32||v.encoding==saves::UnitCoreEncoding::hash32),"Production turn Unit scalar shape unavailable");return v.values.front();};
  const auto uid=native(uscalar("m_iID"));need(ids.insert(uid).second&&uscalar("m_eOwner")==owner,"Production turn actual Unit identity differs");
  const auto hash=uscalar("idUnitType");need(hash>0&&hash<=UINT32_MAX,"Production turn native Unit type hash unavailable");const auto* type=rules.types.find_hash(static_cast<std::uint32_t>(hash));
  need(type&&type->table=="Units"&&rules.trade.contains(native(type->id)),"Production turn actual UnitInfo unavailable");
  const auto& u=s.maintenance.players.at(owner).units.at(uid);need(u.type==type->id&&u.source_core==std::optional{std::pair{unit.core.start_offset,unit.core.end_offset}},"Production turn raw/owned Unit type/span differs");
  const auto istrade=rules.trade.at(native(type->id));if(istrade)trade=add(trade,1);f.seal.insert(f.seal.end(),{uid,hash,istrade});
 }
 const auto units=native(p.units.header.wire_unit_count);
 const auto paid=std::max(0,sub(sub(units,trade),free));
 const auto cap=define(host,"MAX_UNIT_SUPPLY_PRODMOD");
 const auto supply=supply_penalty(paid,cap);auto player_modifier=supply;
 f.seal.insert(f.seal.end(),{handicap,h.base,h.city,h.population,count,pop,units,trade,cap,supply});
 if(project&&rules.projects.at(f.process).spaceship){
  const auto player_space=pscalar(p,"m_iSpaceProductionModifier");player_modifier=player_project_modifier(supply,player_space);f.seal.push_back(player_space);
 }
 if(train){
  const auto& info=rules.units.at(f.process);
  // Complete native player Unit overload precedes City's outer addition.
  if(info.military){const auto v=pscalar(p,"m_iMilitaryProductionModifier");player_modifier=add(player_modifier,v);f.seal.push_back(v);}
  if(info.combat!=-1){
   const auto& v=named(p.core.fields,"m_paiUnitCombatProductionModifiers");
   need(v.encoding==saves::PlayerCoreEncoding::vector_i32&&v.wire_count==rules.combat_slots&&v.values.size()==rules.combat_slots&&info.combat>=0&&static_cast<std::uint64_t>(info.combat)<rules.combat_slots,
        "Production TRAIN native player combat modifiers unavailable");
   const auto value=native(v.values.at(static_cast<std::size_t>(info.combat)));player_modifier=add(player_modifier,value);f.seal.push_back(value);
  }
  // HasTrait values cannot affect this bounded interval: every actual joined
  // Unit_ProductionTraits value is zero; positive rows are strict barriers.
  modifier=add(modifier,player_modifier);
  const auto& domains=named(c->core.fields,"m_aiDomainProductionModifier");
  need(domains.encoding==saves::CityCoreEncoding::vector_i32&&domains.wire_count==4&&domains.values.size()==4,"Production TRAIN actual domain modifier vector unavailable");
  const auto domain=native(domains.values.at(static_cast<std::size_t>(info.domain)));modifier=add(modifier,domain);f.seal.push_back(domain);
  if(info.combat!=-1){
   const auto& combat=named(named(c->production.blocks,"before_queue").children,"m_paiUnitCombatProductionModifier");const auto& values=named(combat.arrays,"values");
   need(combat.fields.size()==1&&named(combat.fields,"wire_count").value>=0&&static_cast<std::uint64_t>(named(combat.fields,"wire_count").value)==rules.combat_slots&&values.columns==1&&values.values.size()==rules.combat_slots,
        "Production TRAIN actual city combat modifier vector unavailable");
   const auto value=native(values.values.at(static_cast<std::size_t>(info.combat)));modifier=add(modifier,value);f.seal.push_back(value);
  }
  if(info.military){const auto value=raw("m_iMilitaryProductionModifier");modifier=add(modifier,value);}
  // Native CvCity loops actual BuildingInfo slots in ascending order and sums
  // only owned nonzero buildings; uses the same genuine Reset/Read count helper.
  const auto& buildings=named(c->production.blocks,"buildings");
  const auto real=detail::bnw_native_building_counts_reset_read_math(named(buildings.children,"m_paiNumRealBuilding"),host.production_rules().building_slots,rules.types);
  const auto free=detail::bnw_native_building_counts_reset_read_math(named(buildings.children,"m_paiNumFreeBuilding"),host.production_rules().building_slots,rules.types);
  need(live.building_counts_resolved&&real.size()==free.size(),"Production TRAIN building counts unresolved");
  const auto city_max=define(host,"CITY_MAX_NUM_BUILDINGS");std::int32_t building_modifier=0;
  for(std::size_t i=0;i<real.size();++i){const auto id=static_cast<std::int64_t>(i);const auto r=live.real_buildings.find(id),b=live.free_buildings.find(id);
   need(real[i]>=0&&free[i]>=0&&(r==live.real_buildings.end()?0:r->second)==real[i]&&(b==live.free_buildings.end()?0:b->second)==free[i],"Production TRAIN raw/owned building count differs");
   const auto count=city_max<=1?std::max(real[i],free[i]):add(real[i],free[i]);f.seal.insert(f.seal.end(),{id,real[i],free[i]});
   if(!host.production_rules().buildings.contains(id)||count<=0)continue;
   const auto found=info.building_modifiers.find(native(id));const auto amount=found==info.building_modifiers.end()?0:found->second;
   if(amount!=0)building_modifier=add(building_modifier,amount);
  }
  if(building_modifier!=0)modifier=add(modifier,building_modifier);
 }
 // SDK Player8042 returns its nested sum BEFORE City5183 outer add.
 if(!train)modifier=city_project_prefix(modifier,player_modifier);f.seal.push_back(modifier);
 if(project&&rules.projects.at(f.process).spaceship){
  const auto city_space=raw("m_iSpaceProductionModifier");modifier=add(modifier,city_space);
 }
 f.modifier=modifier;
 // Suppression short-circuit BEFORE FOOD/base/rate/overflow reads.
 const auto anarchy=pscalar(p,"m_iAnarchyNumTurns");need((anarchy>0)==s.treasury.players.at(owner).anarchy,"Production turn anarchy raw/owned mirror differs");f.seal.push_back(anarchy);
 f.suppressed=anarchy>0;
 if(!f.suppressed){const auto resistance=raw("m_iResistanceTurns");need((resistance>0)==live.resistance,"Production turn resistance raw/owned mirror differs");f.suppressed=resistance>0;if(!f.suppressed){const auto razing=raw("m_iRazingTurns");need((razing>0)==live.razing,"Production turn razing raw/owned mirror differs");f.suppressed=razing>0;}}
 if(!project&&!train){need(f.suppressed,"Production turn actual getCurrentProductionDifference unsuppressed producer unavailable");
  for(const auto process:rules.league_processes){f.seal.push_back(process);need(process!=f.process,"Production turn matching LeagueProject contribution owner unavailable");}
 }
 // Only after the genuine doProcess branch finishes are setter operands bound.
 f.overflow=scalar(*c,"m_iOverflowProduction");f.feature=scalar(*c,"m_iFeatureProduction");need(live.production_progress->overflow_times100==f.overflow&&live.production_progress->feature_times100==f.feature,"Production turn raw/owned overflow/feature differs");
 for(auto n:{"m_iOverflowProduction","m_iFeatureProduction"}){const auto& a=cfield(*c,n,saves::CityCoreEncoding::i32);const auto& b=cfield(*original,n,saves::CityCoreEncoding::i32);need(a.start_offset==b.start_offset&&a.end_offset==b.end_offset,"Production turn store source span differs");}
 return f;
}
saves::SavedCityObservation& mutable_city(LiveSessionState& s,std::int32_t owner,std::int32_t id){
 for(auto& p:s.world.all_players->players)if(p.saved_player_id==owner)for(auto& c:p.cities.cities)if(scalar(c,"m_iID")==id)return c;
 throw std::runtime_error("Production turn actual mutable city missing");
}
}
std::int32_t detail::bnw_production_supply_math(std::int32_t base,std::int32_t per_city,std::int32_t cities,std::int32_t percent,std::int32_t population,std::int32_t units,std::int32_t trade,std::int32_t cap){
 const auto free=free_supply(base,per_city,cities,percent,population);
 const auto paid=std::max(0,sub(sub(units,trade),free));
 return supply_penalty(paid,cap);
}
std::int32_t detail::bnw_project_modifier_math(std::int32_t industrial,std::int32_t supply,std::int32_t player_space,std::int32_t city_space){
 return add(city_project_prefix(industrial,player_project_modifier(supply,player_space)),city_space);
}
struct BnwCityProductionTurnPending::Impl {
 const std::thread::id thread=std::this_thread::get_id();std::atomic<bool> retired{false};
 const LiveSession* session=nullptr;std::weak_ptr<const ui::SelectedBnwPreviewHost> host;
 std::shared_ptr<LiveSessionState> state;std::shared_ptr<const saves::SavedSession> source;
 SessionCheckpointIdentity identity;std::uint64_t generation=0,revision=0;
 Rules rules;Facts facts;bool ui_started=false;std::uint64_t ui_occurrence=0;BnwCityProductionTurnProgress progress;std::map<std::int64_t,std::int64_t>::node_type project_node;std::map<std::int64_t,std::int64_t> after_units,expected_units;std::vector<saves::CityProductionRecord> after_unit_rows;
};
struct BnwCityProductionTurnCheckpoint::Impl {BnwCityProductionTurnKey key;BnwCityProductionTurnProgress progress;};
BnwCityProductionTurnPending::BnwCityProductionTurnPending(std::unique_ptr<Impl> p)noexcept:impl_(std::move(p)){}
BnwCityProductionTurnPending::~BnwCityProductionTurnPending()=default;
BnwCityProductionTurnCheckpoint::BnwCityProductionTurnCheckpoint(std::shared_ptr<const Impl> p)noexcept:impl_(std::move(p)){}
void BnwCityProductionTurnPending::invalidate()noexcept{impl_->retired.store(true);}
BnwCityProductionTurnProgress BnwCityProductionTurnPending::progress()const{need(impl_&&impl_->thread==std::this_thread::get_id(),"Production turn foreign thread/moved cursor");return impl_->progress;}
BnwCityProductionTurnKey BnwCityProductionTurnPending::source_key()const{(void)progress();return impl_->facts.key;}
std::int32_t BnwCityProductionTurnPending::private_current_difference()const{
 const auto& p=*impl_;need(p.thread==std::this_thread::get_id()&&!p.retired.load(),"Production turn difference owner foreign/retired");
 const auto host=p.host.lock();need(host&&p.rules.current&&p.rules.current(),"Production turn difference actual Host/rules retired");
 const auto f=capture(*p.state,*p.source,*host,p.rules,p.progress.owner,p.progress.city,p.facts.project,p.facts.train);
 if(f.suppressed)return 0;
 need(f.project||f.train,"Production turn unsuppressed nonproject difference producer unavailable");
 // SDK overflow sum/feature product BEFORE base/modifier operands.
 const auto overflow=add(f.overflow,mul(f.feature,100));
 auto child=BnwCityCurrentScienceRate::bind_current_difference_source(host,p.progress.owner,p.progress.city,f.modifier,overflow);
 const auto value=child.value_times100();need(p.rules.current(),"Production turn difference rules retired after child");return value;
}
void BnwCityProductionTurnPending::require_current()const{
 const auto& p=*impl_;need(p.thread==std::this_thread::get_id(),"Production turn foreign thread");need(!p.retired.load()&&p.progress.phase!=BnwCityProductionTurnPhase::quarantined,"Production turn retired/quarantined history");
 const auto host=p.host.lock();need(bool(host)&&p.rules.current&&p.rules.current(),"Production turn actual Host/rules retired");
 need(&host->session()==p.session&&p.session->state_==p.state&&p.session->source_==p.source&&p.session->state_generation_==p.generation&&p.session->bnw_game_debug_owner_current(),"Production turn actual session/source/generation retired");
 need(host->identity().payload_sha256==p.identity.payload_sha256&&host->identity().rules_sha256==p.identity.rules_sha256&&p.state->ui_source_revision==p.revision,"Production turn identity/UI source revision differs");
 (void)p.session->bnw_game_debug_mode();const auto now=capture(*p.state,*p.source,*host,p.rules,p.progress.owner,p.progress.city,p.facts.project,p.facts.train);
 need(now.key==p.facts.key&&now.process==p.facts.process&&now.seal==p.facts.seal&&now.overflow==p.facts.overflow&&now.feature==p.facts.feature&&now.stored==p.facts.stored&&now.needed==p.facts.needed&&now.unit_values==p.facts.unit_values,"Production turn actual source operands changed");if((p.facts.project&&p.progress.phase==BnwCityProductionTurnPhase::project_store_pending)||(p.facts.train&&p.progress.phase==BnwCityProductionTurnPhase::unit_store_pending))need(private_current_difference()==p.progress.difference_times100,"Production turn current difference changed before store");need(p.rules.current(),"Production turn rules retired after source read");
}
std::shared_ptr<BnwCityProductionTurnPending> BnwCityProductionTurnPending::prepare(LiveSession& session,std::shared_ptr<const ui::SelectedBnwPreviewHost> host,std::int64_t owner,std::int64_t city){return prepare_mode(session,std::move(host),owner,city,false);}
std::shared_ptr<BnwCityProductionTurnPending> BnwCityProductionTurnPending::prepare_project(const LiveSession& session,std::shared_ptr<const ui::SelectedBnwPreviewHost> host,std::int64_t owner,std::int64_t city){return prepare_mode(session,std::move(host),owner,city,true);}
std::shared_ptr<BnwCityProductionTurnPending> BnwCityProductionTurnPending::prepare_project_read(const LiveSession& session,std::shared_ptr<const ui::SelectedBnwPreviewHost> host,std::int64_t owner,std::int64_t city){return prepare_mode(session,std::move(host),owner,city,true,true);}
std::shared_ptr<BnwCityProductionTurnPending> BnwCityProductionTurnPending::prepare_mode(const LiveSession& session,std::shared_ptr<const ui::SelectedBnwPreviewHost> host,std::int64_t owner,std::int64_t city,bool project,bool read_only,bool train){
 need(session.plot_yield_refresh_thread_==std::this_thread::get_id(),"Production turn session foreign thread");
 need(host&&&host->session()==&session&&session.state_&&session.source_&&session.bnw_game_debug_owner_current(),"Production turn requires genuine SAME imported Host/session");
 (void)session.bnw_game_debug_mode();auto p=std::make_unique<Impl>();p->session=&session;p->host=host;p->state=session.state_;p->source=session.source_;p->generation=session.state_generation_;p->revision=p->state->ui_source_revision;p->identity=session.debug_->identity;
 need(p->identity.payload_sha256==host->identity().payload_sha256&&p->identity.rules_sha256==host->identity().rules_sha256,"Production turn actual Host/import identity differs");
 p->rules=load(*host,project,train);p->facts=capture(*p->state,*p->source,*host,p->rules,native(owner),native(city),project,train);
 const auto [turn,id,full,begin,end]=p->facts.key;(void)begin;(void)end;p->progress.turn=turn;p->progress.owner=id;p->progress.city=full;p->progress.process=p->facts.process;p->progress.overflow_before=p->facts.overflow;p->progress.feature_before=p->facts.feature;
 // Real zero-difference -> changeProductionTimes100 -> unmatched doProcess
 // call. Registered session slot is published before this count is advanced.
 p->progress.native_calls=0;
 auto out=std::shared_ptr<BnwCityProductionTurnPending>(new BnwCityProductionTurnPending(std::move(p)));
 if(project){auto& x=*out->impl_;x.progress.phase=BnwCityProductionTurnPhase::project_store_pending;x.progress.difference_times100=out->private_current_difference();x.progress.project_before=x.facts.stored;x.progress.production_needed=x.facts.needed;
  if(!read_only){const auto target=add(x.facts.stored,x.progress.difference_times100);need(target>=0,"Production turn native project production negative assertion boundary");
   std::map<std::int64_t,std::int64_t> node;node.emplace(x.facts.process,target);x.project_node=node.extract(x.facts.process);
  }
 }
 if(train){auto& x=*out->impl_;x.progress.phase=BnwCityProductionTurnPhase::unit_store_pending;x.progress.difference_times100=out->private_current_difference();x.progress.project_before=x.facts.stored;x.progress.production_needed=x.facts.needed;
  const auto target=add(x.facts.stored,x.progress.difference_times100);need(target>=0,"Production TRAIN negative native production assertion");x.after_units=x.facts.unit_values;x.after_units[x.facts.process]=target;x.expected_units=x.after_units;
  const auto& raw=mutable_city(*x.state,x.progress.owner,x.progress.city);const auto& block=named(named(raw.production.blocks,"before_queue").children,"m_paiUnitProduction");x.after_unit_rows=block.children;
  // Select by queued ID from actual catalog, never a hardcoded unit name.
  const content::InfoTypeEntry* unit_info=nullptr;for(const auto& [key,e]:x.rules.types.entries())if(e.table=="Units"&&e.id==x.facts.process){(void)key;need(!unit_info,"Production TRAIN ambiguous queued UnitInfo");unit_info=&e;}need(unit_info,"Production TRAIN queued UnitInfo missing");
  saves::CityProductionRecord* last=nullptr;for(auto& row:x.after_unit_rows)if(named(row.fields,"hash").value==static_cast<std::int64_t>(unit_info->hash))last=&row;
  if(last)named(last->arrays,"values").values.front()=target;
  else {need(x.after_unit_rows.size()<UINT32_MAX,"Production TRAIN raw hash count exhausted");saves::CityProductionRecord row{"row",static_cast<std::uint64_t>(x.after_unit_rows.size()),0,0,{{"hash",unit_info->hash}},{{"values",1,{target}}},{},{}};x.after_unit_rows.push_back(std::move(row));}
 }
 out->require_current();return out;
}
std::shared_ptr<BnwCityProductionTurnPending> BnwCityProductionTurnPending::prepare_train(const LiveSession& session,std::shared_ptr<const ui::SelectedBnwPreviewHost> host,std::int64_t owner,std::int64_t city){return prepare_mode(session,std::move(host),owner,city,false,false,true);}
void BnwCityProductionTurnPending::advance_one(){
 require_current();auto& p=*impl_;need(p.progress.phase!=BnwCityProductionTurnPhase::complete_component,"Production turn completed component cannot replay");
 if(p.progress.phase==BnwCityProductionTurnPhase::city_screen_predicate_pending)throw std::runtime_error("Production turn actual CityScreen selection/banner UI owner unavailable after project store");
 if(p.progress.phase==BnwCityProductionTurnPhase::city_screen_dirty_pending)throw std::runtime_error("Production turn genuine CityScreen publication adapter required");
 if(p.progress.phase==BnwCityProductionTurnPhase::banner_event_pending)throw std::runtime_error("Production turn actual SAME stock/world SpecificCityInfoDirty BANNER return unavailable");
 if(p.progress.phase==BnwCityProductionTurnPhase::pop_order_pending)throw std::runtime_error("Production turn actual popOrder completion/spawn owner unavailable");
 try{
  if(p.progress.phase==BnwCityProductionTurnPhase::unit_store_pending){
   const auto target=add(p.facts.stored,p.progress.difference_times100);need(target>=0,"Production TRAIN negative native production assertion");const bool changed=target!=p.facts.stored;
   auto& raw=mutable_city(*p.state,p.progress.owner,p.progress.city);auto& block=named(named(raw.production.blocks,"before_queue").children,"m_paiUnitProduction");auto& count=named(block.fields,"wire_count");
   auto& live=*p.state->cities.players.at(p.progress.owner).cities.at(p.progress.city).production_progress;
   if(changed)need(p.revision<UINT64_MAX,"Production TRAIN revision exhausted before store");
   require_current();static_assert(noexcept(block.children.swap(p.after_unit_rows)));static_assert(noexcept(live.units.swap(p.after_units)));static_assert(noexcept(p.facts.unit_values.swap(p.expected_units)));
   ++p.progress.native_calls;
   if(changed){block.children.swap(p.after_unit_rows);count.value=static_cast<std::int64_t>(block.children.size());live.units.swap(p.after_units);p.facts.unit_values.swap(p.expected_units);p.facts.stored=target;
    ++p.progress.raw_writes;++p.state->ui_source_revision;p.revision=p.state->ui_source_revision;p.progress.phase=BnwCityProductionTurnPhase::city_screen_predicate_pending;
   }else p.progress.phase=BnwCityProductionTurnPhase::overflow_pending;
   require_current();return;
  }
  if(p.progress.phase==BnwCityProductionTurnPhase::project_store_pending){
   const auto target=add(p.facts.stored,p.progress.difference_times100);need(target>=0,"Production turn native project negative assertion boundary");
   auto& raw=mutable_city(*p.state,p.progress.owner,p.progress.city);auto& progress=*p.state->cities.players.at(p.progress.owner).cities.at(p.progress.city).production_progress;
   auto& fields=raw.core.fields;const auto found=std::find_if(fields.begin(),fields.end(),[](const auto& f){return f.name&&std::string_view(f.name)=="m_paiProjectProduction";});need(found!=fields.end()&&static_cast<std::size_t>(p.facts.process)<found->values.size(),"Production turn project store preflight failed");
   const auto old=progress.projects.find(p.facts.process);const bool changed=target!=p.facts.stored;
   if(changed)need(p.revision<UINT64_MAX,"Production turn UI revision exhausted before project store");
   ++p.progress.native_calls; // genuine changeProjectProductionTimes100 call, before conditional setter
   if(changed){
    if(old==progress.projects.end()){auto inserted=progress.projects.insert(std::move(p.project_node));need(inserted.inserted,"Production turn preallocated Project mirror insertion failed");}else old->second=target;
    found->values.at(static_cast<std::size_t>(p.facts.process))=target;p.facts.stored=target;
    ++p.progress.raw_writes;++p.state->ui_source_revision;p.revision=p.state->ui_source_revision;
    p.progress.phase=BnwCityProductionTurnPhase::city_screen_predicate_pending;
   }else p.progress.phase=BnwCityProductionTurnPhase::overflow_pending;
   require_current();return;
  }
  if(!p.facts.project&&!p.facts.train&&p.progress.native_calls==0){p.progress.native_calls=1;return;} // actual doProcess no matching league contribution
  need(p.revision<UINT64_MAX,"Production turn UI revision exhausted before actual store");
  const bool overflow=p.progress.phase==BnwCityProductionTurnPhase::overflow_pending;
  auto& raw=mutable_city(*p.state,p.progress.owner,p.progress.city);auto& live=*p.state->cities.players.at(p.progress.owner).cities.at(p.progress.city).production_progress;
  auto& fields=raw.core.fields;auto found=std::find_if(fields.begin(),fields.end(),[&](const auto& f){return f.name&&std::string_view(f.name)==(overflow?"m_iOverflowProduction":"m_iFeatureProduction");});need(found!=fields.end()&&found->values.size()==1,"Production turn preflight raw store unavailable");
  ++p.progress.native_calls; // once marker BEFORE unconditional native setter
  found->values.front()=0;
  if(overflow){live.overflow_times100=0;p.facts.overflow=0;}else{live.feature_times100=0;p.facts.feature=0;}
  ++p.progress.raw_writes;++p.state->ui_source_revision;p.revision=p.state->ui_source_revision;
  p.progress.phase=overflow?BnwCityProductionTurnPhase::feature_pending:(((p.facts.project||p.facts.train)&&p.facts.stored/100>=p.facts.needed)?BnwCityProductionTurnPhase::pop_order_pending:BnwCityProductionTurnPhase::complete_component);
  // MAINTAIN getProduction()==0; needed==INT_MAX, so no completion/popOrder.
  require_current();
 }catch(...){p.progress.phase=BnwCityProductionTurnPhase::quarantined;p.retired.store(true);throw;}
}
void BnwCityProductionTurnPending::begin_ui_effect(){
 require_current();auto& p=*impl_;need(!p.ui_started&&p.progress.native_calls<UINT8_MAX,"Production UI effect attempted twice/exhausted");
 need(p.progress.phase==BnwCityProductionTurnPhase::city_screen_predicate_pending||p.progress.phase==BnwCityProductionTurnPhase::city_screen_dirty_pending,"Production UI effect source phase unavailable");
 if(p.progress.phase==BnwCityProductionTurnPhase::city_screen_dirty_pending){
  need(p.state->city_dirty_next_occurrence<UINT64_MAX&&p.revision<UINT64_MAX,"Production CityScreen occurrence/revision exhausted");
  p.ui_occurrence=p.state->city_dirty_next_occurrence;
 }
 ++p.progress.native_calls;p.ui_started=true; // attempt marker BEFORE source getter/publication
}
void BnwCityProductionTurnPending::accept_city_screen_predicate(bool dirty){
 require_current();auto& p=*impl_;need(p.ui_started&&p.progress.phase==BnwCityProductionTurnPhase::city_screen_predicate_pending,"Production UI predicate has no private actual attempt");
 p.ui_started=false;p.progress.phase=dirty?BnwCityProductionTurnPhase::city_screen_dirty_pending:BnwCityProductionTurnPhase::banner_event_pending;
}
void BnwCityProductionTurnPending::accept_city_screen_publication(){
 auto& p=*impl_;need(p.thread==std::this_thread::get_id()&&!p.retired.load()&&p.ui_started&&p.progress.phase==BnwCityProductionTurnPhase::city_screen_dirty_pending,"Production CityScreen private publication attempt absent");
 const auto host=p.host.lock();need(host&&p.rules.current&&p.rules.current(),"Production CityScreen actual Host/rules retired before queue read");
 need(&host->session()==p.session&&p.session->state_==p.state&&p.session->source_==p.source&&p.session->state_generation_==p.generation&&p.session->bnw_game_debug_owner_current(),"Production CityScreen actual source/allocation retired before queue read");
 need(host->identity().payload_sha256==p.identity.payload_sha256&&host->identity().rules_sha256==p.identity.rules_sha256,"Production CityScreen actual identity changed");
 (void)p.session->bnw_game_debug_mode();
 // No caller ack: inspect the actual just-published SAME session queue.
 need(p.revision<UINT64_MAX&&p.state->ui_source_revision==p.revision+1&&p.ui_occurrence<UINT64_MAX&&p.state->city_dirty_next_occurrence==p.ui_occurrence+1&&!p.state->city_dirty_events.empty(),"Production CityScreen actual queue/revision differs");
 const auto& event=p.state->city_dirty_events.back();need(event.occurrence==p.ui_occurrence&&event.generation==p.generation&&event.owner==p.progress.owner&&event.city==p.progress.city&&event.kind==BnwCityDirtyKind::city_screen&&event.value&&event.plot==-1&&event.yield==-1,"Production CityScreen actual notification identity differs");
 p.revision=p.state->ui_source_revision;require_current();p.ui_started=false;p.progress.phase=BnwCityProductionTurnPhase::banner_event_pending;
}
void BnwCityProductionTurnPending::fail_ui_effect()noexcept{
 if(impl_){impl_->progress.phase=BnwCityProductionTurnPhase::quarantined;impl_->retired.store(true);}
}
std::shared_ptr<const BnwCityProductionTurnCheckpoint> BnwCityProductionTurnPending::checkpoint(bool history)const{
 (void)progress();if(!history&&!impl_->retired.load()&&impl_->progress.phase!=BnwCityProductionTurnPhase::quarantined)require_current();
 auto p=std::make_shared<BnwCityProductionTurnCheckpoint::Impl>();p->key=impl_->facts.key;p->progress=impl_->progress;
 return std::shared_ptr<const BnwCityProductionTurnCheckpoint>(new BnwCityProductionTurnCheckpoint(std::move(p)));
}
std::shared_ptr<BnwCityProductionTurnPending> BnwCityProductionTurnPending::restore_quarantined(const BnwCityProductionTurnCheckpoint& saved){
 need(bool(saved.impl_),"Production turn missing checkpoint metadata");auto p=std::make_unique<Impl>();p->facts.key=saved.impl_->key;p->progress=saved.impl_->progress;p->progress.phase=BnwCityProductionTurnPhase::quarantined;p->retired.store(true);
 return std::shared_ptr<BnwCityProductionTurnPending>(new BnwCityProductionTurnPending(std::move(p)));
}
}

namespace civ5::runtime {
void BnwCityProductionTurnPending::begin_train_banner(){
 require_current();auto& p=*impl_;need(p.facts.train&&!p.ui_started&&p.progress.phase==BnwCityProductionTurnPhase::banner_event_pending&&p.progress.native_calls<UINT8_MAX&&p.revision<UINT64_MAX&&p.state->city_dirty_next_occurrence<UINT64_MAX,"Production TRAIN banner source phase/occurrence unavailable");p.ui_occurrence=p.state->city_dirty_next_occurrence;p.ui_started=true;++p.progress.native_calls;
}
void BnwCityProductionTurnPending::accept_train_banner(){
 auto& p=*impl_;need(p.thread==std::this_thread::get_id()&&!p.retired.load()&&p.facts.train&&p.ui_started&&p.progress.phase==BnwCityProductionTurnPhase::banner_event_pending,"Production TRAIN banner actual attempt absent");
 need(p.revision<UINT64_MAX&&p.state->ui_source_revision==p.revision+1&&p.ui_occurrence<UINT64_MAX&&p.state->city_dirty_next_occurrence==p.ui_occurrence+1&&!p.state->specific_city_info_events.empty(),"Production TRAIN actual BANNER queue/revision absent");
 const BnwSpecificCityInfoEvent expected{p.ui_occurrence,p.generation,p.progress.owner,p.progress.city,BnwSpecificCityInfoKind::banner};need(p.state->specific_city_info_events.back()==expected,"Production TRAIN actual BANNER source identity differs");
 p.revision=p.state->ui_source_revision;require_current();p.ui_started=false;p.progress.phase=BnwCityProductionTurnPhase::overflow_pending;
}
}

namespace civ5::runtime {
bool BnwCityProductionTurnPending::private_train_mode()const{need(impl_&&impl_->thread==std::this_thread::get_id(),"Production TRAIN mode foreign/moved cursor");return impl_->facts.train;}
}

namespace civ5::runtime {
BnwCityProductionTurnPending::TrainCompletionRead BnwCityProductionTurnPending::read_train_completion_without_overflow(
 const std::shared_ptr<const ui::SelectedBnwPreviewHost>& host,std::int32_t owner,std::int32_t city){
 need(host!=nullptr,"TRAIN completion actual Host absent");const auto& session=host->session();
 need(session.plot_yield_refresh_thread_==std::this_thread::get_id()&&session.state_&&session.source_&&session.bnw_game_debug_owner_current(),"TRAIN completion actual source lease retired");
 auto rules=load(*host,false,true);const auto facts=capture(*session.state_,*session.source_,*host,rules,owner,city,false,true);
 TrainCompletionRead out;out.needed_times100=mul(facts.needed,100);out.stored_times100=facts.stored;
 if(!facts.suppressed){auto actual=BnwCityCurrentScienceRate::bind_current_difference_source(host,owner,city,facts.modifier,0);out.difference_without_overflow=actual.value_times100();}
 need(rules.current(),"TRAIN completion rules changed after genuine current difference");return out;
}
}
