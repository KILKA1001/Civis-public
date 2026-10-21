#include "civ5/runtime/bnw_grand_world_averages.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/bnw_city_registry.hpp"
#include "civ5/runtime/city_yields.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <atomic>
#include <functional>
#include <thread>
#include <algorithm>
#include <climits>
#include <stdexcept>
#include <type_traits>
namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void need(bool b,const char *why){if(!b)throw std::runtime_error(why);}
std::int32_t integer(std::int64_t v){need(v>=INT32_MIN&&v<=INT32_MAX,"Grand averages native int overflow/width");return static_cast<std::int32_t>(v);}
std::int32_t add(std::int32_t a,std::int32_t b){return integer(static_cast<std::int64_t>(a)+b);}
std::int32_t mul(std::int32_t a,std::int32_t b){return integer(static_cast<std::int64_t>(a)*b);}
auto &named(const auto &rows,std::string_view n){using R=typename std::remove_cvref_t<decltype(rows)>::value_type;const R *p=nullptr;for(const auto &r:rows)if(r.name&&n==r.name){need(!p,"Grand averages duplicate native field");p=&r;}need(p,"Grand averages required native field missing");return *p;}
std::int32_t scalar(const auto &rows,std::string_view n){const auto &f=named(rows,n);need(f.values.size()==1,"Grand averages scalar source shape unknown");
 if constexpr(std::is_same_v<std::remove_cvref_t<decltype(f)>,saves::PlayerCoreField>)
  need(f.encoding==(n=="m_bAlive"?saves::PlayerCoreEncoding::boolean:saves::PlayerCoreEncoding::i32),"Grand averages player scalar source encoding unknown");
 if constexpr(std::is_same_v<std::remove_cvref_t<decltype(f)>,saves::CityCoreField>)
  need(f.encoding==saves::CityCoreEncoding::i32,"Grand averages city scalar source encoding unknown");
 if constexpr(std::is_same_v<std::remove_cvref_t<decltype(f)>,saves::CityTailNode>)
  need(f.encoding&&std::string_view(f.encoding)==(n=="version"?"u32":"i32"),"Grand averages city religion source encoding unknown");
 if constexpr(std::is_same_v<std::remove_cvref_t<decltype(f)>,saves::PlayerTraitsField>)
  need(f.encoding==saves::PlayerTraitsEncoding::i32,"Grand averages trait scalar source encoding unknown");
return integer(f.values[0]);}
std::int32_t amount(const YieldValues &rows,std::int64_t id){const auto it=rows.find(id);return it==rows.end()?0:integer(it->second);}
const TeamTechnologies &owned_techs(const LiveSessionState &s,std::int64_t team){const TeamTechnologies *p=nullptr;for(const auto &t:s.technologies)if(t.team()==team){need(!p,"Grand averages duplicate current owned team technologies");p=&t;}need(p&&p->profile()==content::TechnologyProfile::bnw,"Grand averages current owned team technologies missing");return *p;}
void coherent_techs(const LiveSessionState &s,const BnwGrandWorldAverageRules &r,std::int64_t team){
 need(s.world.teams&&s.world.teams->layout==bnw&&s.world.teams->teams.size()==64&&team>=0&&team<64,"Grand averages actual team Read unknown");const auto &raw=s.world.teams->teams.at(team);need(raw.index==static_cast<std::uint64_t>(team)&&named(raw.fields,"id").value==team,"Grand averages current team identity differs");
 const auto expected=restore_team_technologies(raw,r.technologies(),r.types());const auto &live=owned_techs(s,team);
 need(expected.remap().all_mapped()&&expected.remap().holes==0&&live.slots()==r.technologies().collection.slots&&live.slots()==expected.slots()&&live.last_tech()==expected.last_tech()&&live.nondefault_values()==expected.nondefault_values(),"Grand averages current raw/owned HasTech registry or mapping stale");
}
const saves::CityProductionRecord &buildings(const saves::SavedCityObservation &c){need(c.production.layout==bnw,"Grand tourism native building Read profile unavailable");const auto &b=named(c.production.blocks,"buildings");need(named(b.fields,"version").value==1,"Grand tourism native building Read version unknown");return b;}
YieldValues counts(const saves::CityProductionRecord &b,const char *n,const BnwBuildingHappinessRules &r){const auto &a=named(b.children,n);need(named(a.fields,"wire_count").value==static_cast<std::int64_t>(a.children.size()),"Grand tourism native building count inventory unknown");YieldValues result;for(const auto &row:a.children){const auto h=named(row.fields,"hash").value;need(h>=0&&h<=UINT32_MAX,"Grand tourism building hash width invalid");if(!h){need(row.arrays.empty(),"Grand tourism null hash has unexpected payload");continue;}const auto *type=r.resolve(h);need(type&&type->table=="Buildings"&&r.building_modifiers().contains(type->id),"Grand tourism building hash requires actual type remapping");const auto &v=named(row.arrays,"values");need(v.columns==1&&v.values.size()==1&&v.values[0]>=0,"Grand tourism building count payload invalid");result[type->id]=integer(v.values[0]);}return result;}
// Private current-only proof parameter. Legacy detached grand queries keep
// their original formula; a public DTO cannot manufacture this proof object.
struct CurrentTourismProof {
 const LiveSessionState& s;const saves::SavedSession& source;
 const content::RulesDatabase& db;const content::InfoTypeCatalog& types;
 std::int32_t owner,city;std::function<void()> current;
 void building_facts(const saves::SavedCityObservation& raw,const CityYieldState& live,const BnwGrandWorldAverageRules& r)const{
  current();const saves::SavedCityObservation* original=nullptr;
  for(const auto& old:source.snapshot.all_players->players.at(static_cast<std::size_t>(owner)).cities.cities)if(scalar(old.core.fields,"m_iID")==city){need(!original,"Current BaseTourism duplicate original city");original=&old;}
  need(original&&raw.production.layout==bnw&&raw.production.context.bnw_city_version==raw.core.version&&raw.production.start_offset==original->production.start_offset&&raw.production.end_offset==original->production.end_offset,"Current BaseTourism actual Buildings source binding unavailable");
  const auto& b=buildings(raw);const auto& old=buildings(*original);need(b.start_offset==old.start_offset&&b.end_offset==old.end_offset&&b.start_offset<b.end_offset,"Current BaseTourism actual Buildings span differs");
  const auto report=db.prefetch_info("Buildings",[&](auto slot,auto authored,const auto& row){const auto name=row.text("Type");need(slot<=INT32_MAX&&authored==static_cast<std::int64_t>(slot)&&name,"Current BaseTourism native BuildingInfo slot unavailable");const auto* a=types.find(*name);const auto* hash=types.find_hash(content::saved_info_type_hash(*name));need(a&&hash&&a->table=="Buildings"&&hash->table=="Buildings"&&a->id==authored&&hash->id==authored&&a->type==*name&&hash->type==*name,"Current BaseTourism native BuildingInfo metadata identity differs");return true;});current();
  need(report.all_succeeded()&&!report.overlapping_ids&&report.slots<=INT32_MAX&&report.rows+report.holes==report.slots,"Current BaseTourism finite BuildingInfo span unavailable");
  need(live.building_counts_resolved,"Current BaseTourism actual Building import unresolved");const auto read=[&](const char* name,const YieldValues& owned){const auto& rows=named(b.children,name);need(rows.start_offset>=b.start_offset&&rows.end_offset<=b.end_offset,"Current BaseTourism Building rows outside actual Read span");const auto values=detail::bnw_native_building_counts_reset_read_math(rows,report.slots,types);current();
   for(const auto& [id,n]:owned){need(id>=0&&static_cast<std::uint64_t>(id)<report.slots,"Current BaseTourism owned Building outside native slots");(void)integer(n);}for(std::size_t i=0;i<values.size();++i){const auto it=owned.find(static_cast<std::int64_t>(i));need((it==owned.end()?0:integer(it->second))==values[i],"Current BaseTourism Reset/Read and actual owned Building count differ");}
  };read("m_paiNumRealBuilding",live.real_buildings);read("m_paiNumFreeBuilding",live.free_buildings);
  need(r.buildings().rules_sha256()==db.gameplay_fingerprint_sha256(),"Current BaseTourism actual Building rule identity differs");
 }
 bool flag(const saves::TeamHashedArray& a,std::string_view table,std::int64_t target)const{
  current();need(a.wire_count>=0&&static_cast<std::uint64_t>(a.wire_count)==a.rows.size(),"Current BaseTourism actual policy flag wire cardinality differs");std::optional<bool> value;
  for(const auto& row:a.rows){need(row.type_hash<=UINT32_MAX,"Current BaseTourism native policy hash width unavailable");if(!row.type_hash){need(row.values.empty(),"Current BaseTourism null policy hash payload differs");continue;}need(row.values.size()==1&&(row.values[0]==0||row.values[0]==1),"Current BaseTourism actual policy native flag not boolean");const auto* t=types.find_hash(row.type_hash);const auto* n=t?types.find(t->type):nullptr;need(t&&n&&t->table==table&&n->table==table&&t->id==n->id&&t->id>=0&&t->id<=INT32_MAX&&t->hash==n->hash,"Current BaseTourism actual policy native Info identity unavailable");if(t->id==target)value=row.values[0]!=0;}
  need(value.has_value(),"Current BaseTourism missing reached policy flag requires actual native Reset/Read owner");return *value;
 }
 void policy(std::int64_t id,bool decoded)const{current();const auto& b=named(s.world.all_players->players.at(static_cast<std::size_t>(owner)).strategy.blocks,"policies");const bool actual=flag(named(b.hashes,"has_policy"),"Policies",id);need(actual==decoded&&s.setup.players.at(owner).policies_resolved&&s.setup.players.at(owner).policies.contains(id)==actual,"Current BaseTourism actual raw/owned policy membership differs");}
 void blocked(std::int64_t id,bool decoded)const{current();const auto& b=named(s.world.all_players->players.at(static_cast<std::size_t>(owner)).strategy.blocks,"policies");need(flag(named(b.hashes,"branch_blocked"),"PolicyBranchTypes",id)==decoded,"Current BaseTourism actual blocked-branch flag differs");}
 void technology(std::int64_t team,std::int64_t tech,const BnwGrandWorldAverageRules& r)const{
  current();if(tech==-1)return;need(team>=0&&team<64&&s.world.teams&&s.world.teams->layout==bnw&&s.world.teams->dimensions.teams==64&&s.world.teams->teams.size()==64,"Current BaseTourism actual native Team registry unavailable");need(tech>=0&&static_cast<std::uint64_t>(tech)<r.technologies().collection.slots&&r.technologies().find(static_cast<std::uint64_t>(tech)),"Current BaseTourism reached native TechInfo unavailable");
  const auto& hashes=named(s.world.teams->teams.at(static_cast<std::size_t>(team)).arrays,"tech_hashes");need(hashes.columns==1,"Current BaseTourism HasTech hash shape unavailable");bool covered=false;
  for(auto hash:hashes.values){need(hash>=0&&hash<=UINT32_MAX,"Current BaseTourism native technology hash width unavailable");const auto* t=hash?types.find_hash(static_cast<std::uint64_t>(hash)):nullptr;if(!t)continue;need(t->table=="Technologies","Current BaseTourism native technology cross-table mapping unavailable");if(t->id==tech)covered=true;}
  need(covered,"Current BaseTourism missing reached HasTech slot requires actual native TEAM Reset/Read owner");
 }
};

std::int32_t majority(const saves::SavedCityObservation &c,const CityYieldState &live){
 need(c.tail.layout==bnw,"Grand tourism city religion Read missing");const auto &block=named(c.tail.blocks,"religions");const auto ver=scalar(block.children,"version");need(ver>=1&&ver<=3,"Grand tourism city religion version unknown");const auto &list=named(block.children,"m_ReligionStatus");need(list.wire_count&&*list.wire_count>=0&&static_cast<std::uint64_t>(*list.wire_count)==list.children.size()&&list.children.size()==live.religions.size(),"Grand tourism current religious status inventory differs");
 std::int32_t total=0,most=-1,pressure=0,religion=-1;for(std::size_t i=0;i<list.children.size();++i){const auto &row=list.children[i];const auto version=scalar(row.children,"version");need(version>=1&&version<=2,"Grand tourism religious status version unknown");const auto id=scalar(row.children,"m_eReligion"),f=scalar(row.children,"m_iFollowers"),p=scalar(row.children,"m_iPressure");const auto &v=live.religions.at(i);need(f>=0&&p>=0&&id==v.religion&&f==v.followers&&p==v.pressure,"Grand tourism current raw/owned religious followers stale");total=add(total,f);if(f>most||(f==most&&p>pressure)){most=f;pressure=p;religion=id;}}
 return mul(most,2)>=total?religion:-1;
}
std::vector<std::int64_t> active_policies(const LiveSessionState &s,const BnwGrandWorldAverageRules &r,std::int64_t player,const CurrentTourismProof *proof=nullptr){const auto &raw=s.world.all_players->players.at(player);need(raw.strategy.layout==bnw,"Grand tourism current policy Read unavailable");const auto &b=named(raw.strategy.blocks,"policies");const auto version=named(b.fields,"version").value;need(version==1||version==2,"Grand tourism policy Read version unknown");const auto &pr=r.buildings().policies();const auto has=pr.flags(named(b.hashes,"has_policy"),false),blocked=pr.flags(named(b.hashes,"branch_blocked"),true);std::set<std::int64_t> membership;std::vector<std::int64_t> active;
 for(std::size_t i=0;i<has.size();++i){if(proof)proof->policy(static_cast<std::int64_t>(i),has[i]);if(has[i]){membership.insert(i);const auto it=pr.policies().find(i);need(it!=pr.policies().end(),"Grand tourism owned policy native Info unavailable");if(it->second.blocked_branch>=0){if(proof)proof->blocked(it->second.blocked_branch,blocked.at(it->second.blocked_branch));if(blocked.at(it->second.blocked_branch))continue;}active.push_back(i);}}const auto &live=s.setup.players.at(player);need(live.policies_resolved&&live.policies==membership,"Grand tourism current shared policy owner stale");return active;
}
std::vector<const saves::SavedCityObservation *> city_roster(const LiveSessionState &s,std::int64_t player){const auto &c=s.world.all_players->players.at(player).cities;const auto &live=s.cities.players.at(player).cities;need(c.layout==bnw&&c.header.layout==bnw&&c.header.wire_city_count==static_cast<std::int64_t>(c.cities.size())&&c.cities.size()==live.size(),"Grand tourism actual current city roster missing");const auto last=integer(named(c.header.fields,"last_index").value),free=integer(named(c.header.fields,"free_list_count").value);need(last>=-1&&free>=0&&static_cast<std::int64_t>(last)-free+1==static_cast<std::int64_t>(live.size())&&named(c.header.fields,"num_slots").value==static_cast<std::int64_t>(c.header.source_slots),"Grand tourism native city FLTA inventory unknown");
 std::set<std::int64_t> slots;std::vector<const saves::SavedCityObservation *> result;for(const auto &raw:c.cities){const auto id=scalar(raw.core.fields,"m_iID");const auto &owned=live.at(id);const auto slot=id&8191;need(id>=0&&slot<=last&&static_cast<std::uint64_t>(slot)<c.header.source_slots&&slots.insert(slot).second&&raw.core.layout==bnw&&scalar(raw.core.fields,"m_eOwner")==player&&owned.id==id&&owned.owner==player&&owned.source_core==std::optional{std::pair{raw.core.start_offset,raw.core.end_offset}},"Grand tourism current city FLTA/source owner differs");result.push_back(&raw);}std::sort(result.begin(),result.end(),[](auto a,auto b){return (scalar(a->core.fields,"m_iID")&8191)<(scalar(b->core.fields,"m_iID")&8191);});return result;
}
std::int32_t league_query(const LiveSessionState &s){const auto &l=s.world.leagues;need(l.layout==bnw&&l.version&&*l.version>=1&&*l.version<=5&&l.start_offset<l.end_offset,"Grand tourism actual GameLeagues Read inventory unknown");need(l.leagues.empty(),"Grand tourism GetCityTourismModifier requires actual member/active-resolution holy-city owner");return 0;}
class CurrentCityBuildings {
 public:
 CurrentCityBuildings(const LiveSessionState &s,const BnwGrandWorldAverageRules &r,
  std::int64_t player,const saves::SavedCityObservation &raw,const CityYieldState &live,const CurrentTourismProof *proof=nullptr)
  :s_(s),r_(r),player_(player),raw_(raw),live_(live),proof_(proof){}
 std::int64_t mapped(std::int64_t cls){
  if(!mapping_){const auto &owner=s_.cities.players.at(player_);
   const auto &ids=s_.world.pregame->slots.integer_arrays.at("civilizations");const auto &keys=s_.world.pregame->slots.string_arrays.at("civilization_keys");
   need(ids.size()==64&&keys.size()==64&&ids.at(player_)==owner.civilization,"Grand tourism current civilization mirror stale");
   const auto key=keys.at(player_).substr(0,keys.at(player_).find('\0'));const auto *type=r_.types().find(key);
   need(type&&type->table=="Civilizations"&&type->id==owner.civilization&&r_.buildings().replacements().civilization_buildings.contains(owner.civilization),"Grand tourism actual civilization Info/map unavailable");
   mapping_=&r_.buildings().replacements().civilization_buildings.at(owner.civilization);
  }const auto it=mapping_->find(cls);return it==mapping_->end()?std::int64_t{-1}:it->second;
 }
 const saves::CityProductionRecord &current(){
  if(!record_){if(proof_)proof_->building_facts(raw_,live_,r_);record_=&buildings(raw_);real_=counts(*record_,"m_paiNumRealBuilding",r_.buildings());free_=counts(*record_,"m_paiNumFreeBuilding",r_.buildings());
   need(live_.building_counts_resolved,"Grand tourism current city building owner unresolved");
   for(const auto &[id,v]:real_)need(amount(live_.real_buildings,id)==v,"Grand tourism raw/owned real building count stale");
   for(const auto &[id,v]:free_)need(amount(live_.free_buildings,id)==v,"Grand tourism raw/owned free building count stale");
   for(const auto &[id,v]:live_.real_buildings)need(r_.entries().contains(id)&&amount(real_,id)==v,"Grand tourism current real building owner stale");
   for(const auto &[id,v]:live_.free_buildings)need(r_.entries().contains(id)&&amount(free_,id)==v,"Grand tourism current free building owner stale");
  }return *record_;
 }
 std::int32_t count(std::int64_t id){current();return r_.buildings().building_count(amount(real_,id),amount(free_,id));}
 std::int32_t theming(std::int32_t &visited){
  for(std::uint64_t cls=0;cls<r_.buildings().replacements().building_class_slots;++cls){++visited;const auto building=mapped(cls);if(building==-1)continue;need(r_.entries().contains(building),"Grand tourism mapped building Info missing");if(count(building)>0)need(!r_.entries().at(building).theming_possible,"Grand tourism GetThemingBonus requires actual theming index/great-work metadata/policy owner");}
  return 0;
 }
 private:
 const LiveSessionState &s_;const BnwGrandWorldAverageRules &r_;std::int64_t player_;
 const saves::SavedCityObservation &raw_;const CityYieldState &live_;const CurrentTourismProof *proof_=nullptr;
 const YieldValues *mapping_=nullptr;const saves::CityProductionRecord *record_=nullptr;
 YieldValues real_,free_;
};
BnwGrandCityTourism city_tourism(const LiveSessionState &s,const BnwGrandWorldAverageRules &r,std::int64_t player,const saves::SavedCityObservation &raw,const CurrentTourismProof *proof=nullptr){
 BnwGrandCityTourism q;q.city=scalar(raw.core.fields,"m_iID");const auto &live=s.cities.players.at(player).cities.at(q.city);const auto resistance=scalar(raw.core.fields,"m_iResistanceTurns"),razing=scalar(raw.core.fields,"m_iRazingTurns");need(live.resistance==(resistance>0)&&live.razing==(razing>0),"Grand tourism raw/owned resistance/razing stale");q.base_suppressed=resistance>0||razing>0;
 const auto &p=s.world.all_players->players.at(player);const auto &owner=s.cities.players.at(player);
 CurrentCityBuildings access(s,r,player,raw,live,proof);
 const auto mapped=[&](auto cls){return access.mapped(cls);};
 const auto current_buildings=[&]() -> const saves::CityProductionRecord & {return access.current();};
 const auto count=[&](auto id){return access.count(id);};
 std::optional<std::vector<std::int64_t>> active;
 if(!q.base_suppressed){
  const auto &b=current_buildings();
  const auto &works=named(b.arrays,"m_aBuildingGreatWork");const auto size=named(b.fields,"m_aBuildingGreatWork_wire_count").value;need(size>=0&&works.columns==3&&static_cast<std::uint64_t>(size)*3==works.values.size(),"Grand tourism actual great-work inventory unknown");q.great_works=integer(size);auto base=mul(q.great_works,r.base_tourism());base=add(base,mul(integer(named(b.fields,"m_iGreatWorksTourismModifier").value),base)/100);
  // Native GetThemingBonuses loops classes even for zero base/great works.
  base=add(base,access.theming(q.theming_classes_visited));
  const auto landmarks=integer(named(b.fields,"m_iLandmarksTourismPercent").value);need(landmarks<=0,"Grand tourism positive LandmarksTourismPercent requires actual GetCultureFromWonders, natural-wonder and improvement queries");
  q.majority=majority(raw,live);const auto religions=BnwFounderReligionReadModel::bind(s);const auto *religion=religions.religion(q.majority,player);q.get_religion_null=!religion;need(!religion,"Grand tourism GetReligion returned actual beliefs; FaithBuildingTourism/building-class tourism owner required");
  const auto team=s.setup.players.at(player).team;bool tech_checked=false;
  for(const auto cls:r.buildings().classes()){++q.enhanced_classes_visited;const auto building=mapped(cls);if(building==-1)continue;need(r.entries().contains(building),"Grand tourism enhanced building Info unavailable");const auto &entry=r.entries().at(building);if(count(building)>0&&entry.enhanced_tourism>0){if(!tech_checked){coherent_techs(s,r,team);tech_checked=true;}if(proof)proof->technology(team,entry.enhanced_tech,r);if(owned_techs(s,team).has_tech(entry.enhanced_tech))base=add(base,entry.enhanced_tourism);}}
  q.base_before_modifiers=base;
 }
 // Source GetBaseTourism continues these queries even when the base getter
 // early-returned zero for resistance/razing. Do not shortcut by base value.
 auto modifier=std::int32_t{0};const auto spread=scalar(p.core.fields,"m_iInfluenceSpreadModifier");if(spread>0)modifier=add(modifier,spread);if(scalar(p.core.fields,"m_iTourismBonusTurns")>0)modifier=add(modifier,r.temporary_boost());const auto league=league_query(s);q.league_query_executed=true;if(league>0)modifier=add(modifier,league);
 const bool golden=scalar(p.core.fields,"m_iGoldenAgeTurns")>0;need(owner.golden_age==golden,"Grand tourism current golden-age mirror stale");if(golden){need(p.traits.layout==bnw,"Grand tourism current trait Read unavailable");modifier=add(modifier,scalar(p.traits.fields,"m_iGoldenAgeTourismModifier"));}
 for(std::uint64_t cls=0;cls<r.buildings().replacements().building_class_slots;++cls){++q.policy_classes_visited;const auto building=mapped(cls);if(building==-1)continue;need(r.entries().contains(building),"Grand tourism policy building Info unavailable");if(!active)active=active_policies(s,r,player,proof);std::int32_t policy=0;for(auto id:*active)policy=add(policy,amount(r.policy_tourism().at(id),cls));if(policy!=0&&count(building)>0)modifier=add(modifier,policy);}
 q.modifier=modifier;q.tourism=modifier>0?mul(q.base_before_modifiers,add(100,modifier))/100:q.base_before_modifiers;return q;
}
void player_tourism(const LiveSessionState &s,const BnwGrandWorldAverageRules &r,std::int64_t player,std::vector<BnwGrandCityTourism> &cities,std::int32_t &total){
 for(const auto *city:city_roster(s,player)){cities.push_back(city_tourism(s,r,player,*city));total=add(total,cities.back().tourism);}
}
void influence_binding(const LiveSessionState &s,const BnwCultureInfluenceRules &r,const SessionCheckpointIdentity &id,std::int64_t player){
 need(id.payload_sha256.size()==64&&id.rules_sha256==r.identity(),"BNW influence source/rules identity differs");
 need(player>=0&&player<64&&s.setup.profile==content::TechnologyProfile::bnw&&s.player_research.profile==content::TechnologyProfile::bnw&&s.world.all_players&&s.world.all_players->layout==bnw&&s.world.all_players->players.size()==64&&s.setup.players.size()==64&&s.world.pregame&&s.world.pregame->layout==bnw,"BNW influence current native BNW roster unavailable");
 const auto &raw=s.world.all_players->players.at(player);need(raw.index==static_cast<std::uint64_t>(player)&&raw.saved_player_id==player&&raw.core.layout==bnw&&scalar(raw.core.fields,"m_eID")==player,"BNW influence actor identity differs");
}
std::int64_t influence_team(const LiveSessionState &s,std::int64_t p){const auto &ids=s.world.pregame->archive.integer_arrays.at("team_types");need(ids.size()==64&&ids.at(p)==s.setup.players.at(p).team&&ids.at(p)>=0&&ids.at(p)<64,"BNW influence current player-team mirror stale");return ids.at(p);}
BnwCultureInfluenceLevel influence_level(const LiveSessionState &s,const BnwCultureInfluenceRules &r,std::int64_t player,std::int64_t foreign){
 BnwCultureInfluenceLevel q;q.player=player;q.foreign=foreign;need(foreign>=0&&foreign<22,"BNW influence native major target unavailable");
 const auto &other=s.world.all_players->players.at(foreign);need(other.index==static_cast<std::uint64_t>(foreign)&&other.saved_player_id==foreign&&other.core.layout==bnw&&scalar(other.core.fields,"m_eID")==foreign,"BNW influence foreign identity differs");
 const auto own_team=influence_team(s,player),foreign_team=influence_team(s,foreign);need(s.world.teams&&s.world.teams->layout==bnw&&s.world.teams->teams.size()==64,"BNW influence native teams unavailable");const auto &team=s.world.teams->teams.at(foreign_team);need(team.index==static_cast<std::uint64_t>(foreign_team)&&named(team.fields,"id").value==foreign_team,"BNW influence foreign team identity differs");
 const auto &met=named(team.arrays,"has_met");need(met.columns==1&&met.values.size()==64&&(met.values.at(own_team)==0||met.values.at(own_team)==1),"BNW influence native foreign met64 malformed");q.foreign_met=met.values.at(own_team)!=0;need(s.player_research.teams.at(foreign_team).met.contains(own_team)==q.foreign_met,"BNW influence current foreign met mirror stale");
 if(!q.foreign_met){q.level=-1;q.calculated=true;return q;}
 const auto &raw=s.world.all_players->players.at(player).trade_culture;need(raw.layout==bnw&&raw.dimensions.major==22,"BNW influence actual own culture Read dimensions unknown");const auto &culture=named(raw.blocks,"player_culture");const auto version=named(culture.fields,"version").value;need(version>=0&&version<=6,"BNW influence native culture version unknown");const auto &influence=named(culture.arrays,"m_aiCulturalInfluence");need(named(culture.fields,"cultural_influence_wire_count").value==22&&influence.columns==1&&influence.values.size()==22,"BNW influence actual current influence22 shape unknown");
 q.influence=integer(influence.values.at(foreign));q.lifetime_culture=scalar(other.core.fields,"m_iJONSCultureEverGenerated");q.percent=*q.lifetime_culture>0?mul(*q.influence,100)/(*q.lifetime_culture):0;q.level=0;
 for(std::size_t i=0;i<r.thresholds().size();++i)if(*q.percent>=r.thresholds()[i]){q.level=5-static_cast<std::int32_t>(i);break;}q.calculated=true;return q;
}

}
struct BnwCityCurrentBaseTourism::Impl {
 const std::thread::id thread=std::this_thread::get_id();
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host;
 std::shared_ptr<const LiveSession> session;std::shared_ptr<const content::RulesDatabase> db;
 std::shared_ptr<const LiveSessionState> state;const saves::SavedSession* source=nullptr;
 SessionCheckpointIdentity identity;std::function<bool()> rules_current;std::uint64_t generation=0;
 std::int32_t owner=-1,city=-1;std::pair<std::uint64_t,std::uint64_t> core_span;
 mutable std::optional<BnwGrandWorldAverageRules> rules;mutable std::atomic<bool> retired{false};
};
BnwCityCurrentBaseTourism::BnwCityCurrentBaseTourism(std::shared_ptr<Impl> p)noexcept:impl_(std::move(p)){}
void BnwCityCurrentBaseTourism::require_current()const{
 const auto p=impl_;need(p&&p->thread==std::this_thread::get_id(),"Current BaseTourism foreign constructor thread/moved owner");
 try{need(!p->retired.load()&&p->host&&p->session&&p->db&&p->rules_current&&p->rules_current(),"Current BaseTourism actual parent/rules retired");(void)p->session->bnw_game_debug_mode();
  need(&p->host->session()==p->session.get()&&&p->host->database()==p->db.get()&&p->session->read_state_lease()==p->state&&p->session->state_generation()==p->generation&&&p->session->source()==p->source,"Current BaseTourism actual source/allocation/generation retired");
  need(p->db->gameplay_fingerprint_sha256()==p->identity.rules_sha256&&p->host->identity().rules_sha256==p->identity.rules_sha256&&p->host->identity().payload_sha256==p->identity.payload_sha256,"Current BaseTourism actual rules/payload identity differs");
 }catch(...){p->retired.store(true);throw;}
}
std::int32_t BnwCityCurrentBaseTourism::value()const{
 try{require_current();const auto p=impl_;const auto* raw=resolve_bnw_city_registry(p->state->world,p->state->cities,p->owner,p->city);
  need(raw&&scalar(raw->core.fields,"m_iID")==p->city&&scalar(raw->core.fields,"m_eOwner")==p->owner&&std::pair{raw->core.start_offset,raw->core.end_offset}==p->core_span,"Current BaseTourism canonical city source differs");
  const auto& live=p->state->cities.players.at(p->owner).cities.at(p->city);need(live.owner==p->owner&&live.id==p->city&&live.source_core==std::optional{p->core_span},"Current BaseTourism actual owned city source differs");
  if(!p->rules){p->rules=BnwGrandWorldAverageRules::load(*p->db);require_current();}
  need(p->rules->identity()==p->identity.rules_sha256,"Current BaseTourism actual loaded rule identity differs");
  CurrentTourismProof proof{*p->state,*p->source,*p->db,p->host->types(),p->owner,p->city,[&]{require_current();}};
  const auto q=city_tourism(*p->state,*p->rules,p->owner,*raw,&proof);require_current();return q.tourism;
 }catch(const BnwCityCurrentBaseTourismUnavailable&){throw;}catch(const std::exception& e){throw BnwCityCurrentBaseTourismUnavailable(e.what());}
}
BnwCityCurrentBaseTourism inspect_bnw_city_current_base_tourism(std::shared_ptr<const ui::SelectedBnwPreviewHost> host,std::int64_t owner,std::int64_t city){
 try{need(bool(host),"Current BaseTourism actual selected Host absent");(void)host->session().bnw_game_debug_mode();auto p=std::make_shared<BnwCityCurrentBaseTourism::Impl>();p->host=std::move(host);
  p->session=std::shared_ptr<const LiveSession>(p->host,&p->host->session());p->db=std::shared_ptr<const content::RulesDatabase>(p->host,&p->host->database());p->rules_current=p->db->immutable_preview_fingerprint_currentness();need(p->rules_current&&p->rules_current(),"Current BaseTourism genuine immutable DB scope absent");
  p->identity=p->host->identity();need(p->identity.payload_sha256.size()==64&&p->db->gameplay_fingerprint_sha256()==p->identity.rules_sha256,"Current BaseTourism actual source/rules identity differs");p->state=p->session->read_state_lease();need(bool(p->state),"Current BaseTourism actual state absent");p->generation=p->session->state_generation();p->source=&p->session->source();need(identify_bytes(p->source->source.bytes).sha256==p->identity.payload_sha256,"Current BaseTourism actual source payload differs");
  p->owner=integer(owner);need(p->owner>=0&&p->owner<64,"Current BaseTourism native owner outside BNW roster");const auto* raw=resolve_bnw_city_registry(p->state->world,p->state->cities,p->owner,integer(city));need(raw,"Current BaseTourism actual current city absent");p->city=scalar(raw->core.fields,"m_iID");need(scalar(raw->core.fields,"m_eOwner")==p->owner&&raw->core.start_offset<raw->core.end_offset,"Current BaseTourism canonical source city differs");p->core_span={raw->core.start_offset,raw->core.end_offset};
  BnwCityCurrentBaseTourism q{std::move(p)};q.require_current();return q;
 }catch(const BnwCityCurrentBaseTourismUnavailable&){throw;}catch(const std::exception& e){throw BnwCityCurrentBaseTourismUnavailable(e.what());}
}

BnwGrandWorldAverageRules BnwGrandWorldAverageRules::load(const content::RulesDatabase &db){BnwGrandWorldAverageRules r;r.buildings_=BnwBuildingHappinessRules::load(db);r.types_=db.cache_info_types();need(r.types_.report().all_succeeded(),"Grand averages actual Info catalog missing");r.tech_=content::load_technology_rules(db,r.types_,content::TechnologyProfile::bnw,5);need(r.tech_.all_succeeded(),"Grand averages current technology rules unavailable");
 const auto defines=db.cache_global_defines(content::GlobalDefineProfile::bnw);const auto d=[&](const char *n){const auto *v=defines.find(n);need(v&&v->lookup.found&&v->lookup.error.empty()&&std::holds_alternative<std::int64_t>(v->lookup.value),"Grand tourism required native define missing");return integer(std::get<std::int64_t>(v->lookup.value));};r.base_=d("BASE_TOURISM_PER_GREAT_WORK");r.temporary_=d("TEMPORARY_TOURISM_BOOST_MOD");
 const auto buildings=db.prefetch_info("Buildings",[&](auto slot,auto,const auto &row){need(slot<=INT32_MAX&&row.has_column("TechEnhancedTourism")&&row.has_column("EnhancedYieldTech")&&row.has_column("Type"),"Grand tourism required Building Info schema missing");Building b;b.enhanced_tourism=integer(row.integer("TechEnhancedTourism"));const auto key=row.text("EnhancedYieldTech").value_or("");if(!key.empty()){const auto *type=r.types_.find(key);need(type&&type->table=="Technologies","Grand tourism enhanced technology Info missing");b.enhanced_tech=integer(type->id);}const auto typekey=row.text("Type");need(typekey&&!typekey->empty(),"Grand tourism actual Building Info Type missing");std::int64_t n=0;const auto report=db.query_rows({"SELECT COUNT(*) FROM Building_ThemingBonuses WHERE BuildingType=?",{*typekey}},[&](const auto &v){n=v.integer_at(0);});need(report.all_succeeded()&&n>=0,"Grand tourism native theming Info unavailable");b.theming_possible=n>0;r.entries_[slot]=b;return true;});need(buildings.all_succeeded(),"Grand tourism building Info load failed");
 for(const auto &[id,entry]:r.buildings_.policies().policies()){auto &matrix=r.policy_[id];(void)entry;const auto report=db.query_rows({"SELECT BuildingClasses.ID, TourismModifier FROM Policy_BuildingClassTourismModifiers INNER JOIN BuildingClasses ON BuildingClasses.Type=BuildingClassType WHERE PolicyType=(SELECT Type FROM Policies WHERE ID=?)",{id}},[&](const auto &v){const auto cls=v.integer_at(0);need(r.buildings_.classes().contains(cls),"Grand tourism native policy class reference unknown");matrix[cls]=integer(v.integer_at(1));});need(report.all_succeeded(),"Grand tourism native policy tourism matrix unavailable");}
 return r;}
BnwGrandPlayerTourismQuery quote_bnw_player_tourism(const LiveSessionState &s,const BnwGrandWorldAverageRules &r,const SessionCheckpointIdentity &id,std::int64_t player){
 BnwGrandPlayerTourismQuery q;q.player=player;try{need(id.payload_sha256.size()==64&&id.rules_sha256==r.identity(),"BNW player tourism source/rules identity differs");need(player>=0&&player<64&&s.setup.profile==content::TechnologyProfile::bnw&&s.cities.profile==content::TechnologyProfile::bnw&&s.world.all_players&&s.world.all_players->layout==bnw&&s.world.all_players->players.size()==64&&s.setup.players.size()==64&&s.cities.players.size()==64&&s.world.pregame&&s.world.pregame->layout==bnw,"BNW player tourism actual current registries unavailable");const auto &raw=s.world.all_players->players.at(player);need(raw.index==static_cast<std::uint64_t>(player)&&raw.saved_player_id==player&&raw.core.layout==bnw&&scalar(raw.core.fields,"m_eID")==player,"BNW player tourism actor identity differs");std::int32_t total=0;player_tourism(s,r,player,q.cities,total);q.total=total;q.calculated=true;}catch(const std::exception &e){q.total.reset();q.blocker=e.what();}return q;
}
BnwCultureInfluenceRules BnwCultureInfluenceRules::load(const content::RulesDatabase &db){
 BnwCultureInfluenceRules r;r.identity_=identify_bytes(db.gameplay_fingerprint_data()).sha256;const auto d=db.cache_global_defines(content::GlobalDefineProfile::bnw);const std::array keys{"CULTURE_LEVEL_DOMINANT","CULTURE_LEVEL_INFLUENTIAL","CULTURE_LEVEL_POPULAR","CULTURE_LEVEL_FAMILIAR","CULTURE_LEVEL_EXOTIC"};for(std::size_t i=0;i<keys.size();++i){const auto *v=d.find(keys[i]);need(v&&v->lookup.found&&v->lookup.error.empty()&&!v->retained_previous&&std::holds_alternative<std::int64_t>(v->lookup.value),"BNW influence mandatory native culture define unavailable");r.thresholds_[i]=integer(std::get<std::int64_t>(v->lookup.value));}return r;
}
BnwCultureInfluenceLevel quote_bnw_culture_influence_level(const LiveSessionState &s,const BnwCultureInfluenceRules &r,const SessionCheckpointIdentity &id,std::int64_t player,std::int64_t foreign){
 BnwCultureInfluenceLevel q;q.player=player;q.foreign=foreign;try{influence_binding(s,r,id,player);q=influence_level(s,r,player,foreign);}catch(const std::exception &e){q.calculated=false;q.level.reset();q.blocker=e.what();}return q;
}
BnwCultureInfluentialCount quote_bnw_num_civs_influential_on(const LiveSessionState &s,const BnwCultureInfluenceRules &r,const SessionCheckpointIdentity &id,std::int64_t player){
 BnwCultureInfluentialCount q;q.player=player;try{influence_binding(s,r,id,player);std::int32_t count=0;const auto &minor=s.world.pregame->archive.flag_arrays.at("minor_nation_civs");need(minor.size()==64,"BNW influential count native minor64 unknown");for(std::int64_t p=0;p<22;++p){++q.slots_visited;if(p==player)continue;const auto &raw=s.world.all_players->players.at(p);const auto alive=scalar(raw.core.fields,"m_bAlive");need(raw.index==static_cast<std::uint64_t>(p)&&raw.saved_player_id==p&&raw.core.layout==bnw&&scalar(raw.core.fields,"m_eID")==p&&(alive==0||alive==1)&&s.setup.players.at(p).alive==(alive==1),"BNW influential count current foreign alive mirror stale");if(!alive)continue;need(s.player_research.players.at(p).pregame_minor==minor.at(p)&&s.cities.players.at(p).minor==minor.at(p),"BNW influential count current foreign minor mirror stale");if(minor.at(p))continue;q.foreigners.push_back(influence_level(s,r,player,p));if(*q.foreigners.back().level>=4)count=add(count,1);}q.count=count;q.calculated=true;}catch(const std::exception &e){q.count.reset();q.blocker=e.what();}return q;
}
BnwGrandWorldAverages quote_bnw_grand_world_averages(const LiveSessionState &s,const BnwGrandWorldAverageRules &r,const SessionCheckpointIdentity &id){BnwGrandWorldAverages q;try{
 need(id.payload_sha256.size()==64&&id.rules_sha256==r.identity(),"Grand averages selected rules identity differs");need(s.setup.profile==content::TechnologyProfile::bnw&&s.cities.profile==content::TechnologyProfile::bnw&&s.world.all_players&&s.world.all_players->layout==bnw&&s.world.all_players->players.size()==64&&s.setup.players.size()==64&&s.cities.players.size()==64&&s.world.pregame&&s.world.pregame->layout==bnw&&s.world.teams&&s.world.teams->layout==bnw&&s.world.teams->teams.size()==64,"Grand averages actual current BNW registries unavailable");const auto &teams=s.world.pregame->archive.integer_arrays.at("team_types");need(teams.size()==64,"Grand averages actual CvPreGame team mapping unknown");
 std::int32_t culture=0,tourism=0;for(std::int64_t player=0;player<22;++player){const auto &raw=s.world.all_players->players.at(player);const auto alive=scalar(raw.core.fields,"m_bAlive");need(raw.index==static_cast<std::uint64_t>(player)&&raw.saved_player_id==player&&raw.core.layout==bnw&&scalar(raw.core.fields,"m_eID")==player&&(alive==0||alive==1)&&s.setup.players.at(player).alive==(alive==1),"Grand averages actual major alive/identity mirrors differ");if(!alive)continue;need(teams.at(player)==s.setup.players.at(player).team,"Grand averages current player-team mirror stale");BnwGrandPlayerCultureTourism p;p.player=player;p.lifetime_culture=scalar(raw.core.fields,"m_iJONSCultureEverGenerated");culture=add(culture,p.lifetime_culture);player_tourism(s,r,player,p.cities,p.tourism);tourism=add(tourism,p.tourism);q.players.push_back(std::move(p));}
 need(!q.players.empty(),"Grand averages source culture/tourism divisor has zero actual alive majors");q.culture_average=culture/integer(q.players.size());q.tourism_average=tourism/integer(q.players.size());
 std::int32_t known=0;for(std::int64_t team=0;team<22;++team){const auto &raw=s.world.teams->teams.at(team);need(raw.index==static_cast<std::uint64_t>(team)&&named(raw.fields,"id").value==team,"Grand averages major team actual Read ID differs");const auto count=integer(named(raw.fields,"alive_count").value);std::int32_t actual=0;for(std::int64_t player=0;player<64;++player){const auto &p=s.world.all_players->players.at(player);const auto alive=scalar(p.core.fields,"m_bAlive");need(p.index==static_cast<std::uint64_t>(player)&&p.saved_player_id==player&&p.core.layout==bnw&&scalar(p.core.fields,"m_eID")==player&&(alive==0||alive==1)&&s.setup.players.at(player).alive==(alive==1)&&teams.at(player)==s.setup.players.at(player).team,"Grand averages team alive-count roster mirrors differ");if(alive&&teams.at(player)==team)actual=add(actual,1);}need(count==actual,"Grand averages current team alive count stale");if(count<=0)continue;coherent_techs(s,r,team);const auto value=integer(owned_techs(s,team).known_count());known=add(known,value);q.teams.push_back({team,value});}
 need(!q.teams.empty(),"Grand averages source tech divisor has zero actual alive major teams");q.tech_average=known/integer(q.teams.size());q.calculated=true;
 }catch(const std::exception &e){q.culture_average.reset();q.tourism_average.reset();q.tech_average.reset();q.blocker=e.what();}return q;}
BnwGrandPlayerCityCulture quote_bnw_jons_culture_from_cities(
 const LiveSessionState &s,const BnwGrandWorldAverageRules &r,
 const BnwHappinessSuffixRules &native,const SessionCheckpointIdentity &id,std::int64_t player){
 BnwGrandPlayerCityCulture q;
 try{
  need(id.payload_sha256.size()==64&&id.rules_sha256==r.identity()&&id.rules_sha256==native.identity(),"BNW JONS culture rules/source identity differs");
  need(s.setup.profile==content::TechnologyProfile::bnw&&s.cities.profile==content::TechnologyProfile::bnw&&s.world.all_players&&s.world.all_players->layout==bnw&&s.world.all_players->players.size()==64&&s.world.pregame&&s.world.pregame->layout==bnw&&player>=0&&player<64,"BNW JONS culture current registries unavailable");
  const auto &p=s.world.all_players->players.at(player);need(p.index==static_cast<std::uint64_t>(player)&&p.saved_player_id==player&&p.core.layout==bnw&&scalar(p.core.fields,"m_eID")==player,"BNW JONS culture current player Read identity differs");
  const auto define=[&](const char *name){const auto *v=native.defines().find(name);need(v&&v->lookup.found&&v->lookup.sqlite_code==0&&!v->retained_previous&&std::holds_alternative<std::int64_t>(v->lookup.value),"BNW JONS culture actual define missing");return integer(std::get<std::int64_t>(v->lookup.value));};
  std::int32_t total=0;
  for(const auto *raw:city_roster(s,player)){
   BnwGrandCityCulture c;c.city=scalar(raw->core.fields,"m_iID");const auto &live=s.cities.players.at(player).cities.at(c.city);
   const auto resistance=scalar(raw->core.fields,"m_iResistanceTurns"),razing=scalar(raw->core.fields,"m_iRazingTurns");
   need(live.resistance==(resistance>0)&&live.razing==(razing>0),"BNW JONS culture current resistance/razing mirror stale");
   c.suppressed=resistance>0||razing>0;
   if(!c.suppressed){
    c.buildings=scalar(raw->core.fields,"m_iJONSCulturePerTurnFromBuildings");c.base=add(c.base,c.buildings);
    c.policies=scalar(raw->core.fields,"m_iJONSCulturePerTurnFromPolicies");c.base=add(c.base,c.policies);
    c.specialists=scalar(raw->core.fields,"m_iJONSCulturePerTurnFromSpecialists");c.base=add(c.base,c.specialists);
    const auto *culture=native.types().find("YIELD_CULTURE");need(culture&&culture->table=="Yields"&&culture->id==4&&s.cities.yield_slots==6,"BNW JONS culture actual native YIELD_CULTURE/NUM_YIELD_TYPES profile unavailable");
    const auto &workchange=named(p.core.fields,"m_aiGreatWorkYieldChange");need(workchange.encoding==saves::PlayerCoreEncoding::vector_i32&&workchange.values.size()==s.cities.yield_slots,"BNW JONS culture native GreatWorkYieldChange array missing");
    const auto per_work=add(define("BASE_CULTURE_PER_GREAT_WORK"),integer(workchange.values.at(culture->id)));
    CurrentCityBuildings access(s,r,player,*raw,live);const auto &b=access.current();const auto &works=named(b.arrays,"m_aBuildingGreatWork");const auto n=named(b.fields,"m_aBuildingGreatWork_wire_count").value;
    need(n>=0&&n<=INT32_MAX&&works.columns==3&&static_cast<std::uint64_t>(n)*3==works.values.size(),"BNW JONS culture actual great-work inventory unknown");
    c.great_works=add(mul(per_work,integer(n)),access.theming(c.theming_classes_visited));c.base=add(c.base,c.great_works);
    const auto &terrain=named(raw->core.fields,"m_aiBaseYieldRateFromTerrain");need(terrain.encoding==saves::CityCoreEncoding::vector_i32&&terrain.values.size()==s.cities.yield_slots,"BNW JONS culture native terrain array missing");
    c.terrain=integer(terrain.values.at(culture->id));const auto found=live.yields.find(culture->id);need(c.terrain==(found==live.yields.end()?0:integer(found->second.terrain)),"BNW JONS culture current raw/owned terrain stale");c.base=add(c.base,c.terrain);
    need(p.traits.layout==bnw,"BNW JONS culture current trait Read missing");c.traits=scalar(p.traits.fields,"m_iCityCultureBonus");c.base=add(c.base,c.traits);
    c.religion=scalar(raw->core.fields,"m_iJONSCulturePerTurnFromReligion");c.base=add(c.base,c.religion);
    const auto wonders=scalar(raw->core.fields,"m_iNumWorldWonders");need(wonders>=0,"BNW JONS culture native world wonder counter invalid");
    const auto &leagues=s.world.leagues;need(leagues.layout==bnw&&leagues.version&&*leagues.version>=1&&*leagues.version<=5&&leagues.start_offset<leagues.end_offset,"BNW JONS culture actual league Read inventory unknown");
    need(leagues.leagues.empty()&&s.cities.leagues.empty(),"BNW JONS culture GetWorldWonderYieldChange requires actual member/resolution owner");c.league_query_executed=true;c.leagues=mul(wonders,0);c.base=add(c.base,c.leagues);
    c.modifier=add(100,scalar(raw->core.fields,"m_iCultureRateModifier"));c.modifier=add(c.modifier,scalar(p.core.fields,"m_iJONSCultureCityModifier"));
    if(wonders>0)c.modifier=add(c.modifier,scalar(p.core.fields,"m_iCultureWonderMultiplier"));
    const auto &puppet=named(raw->core.fields,"m_bPuppet");need(puppet.encoding==saves::CityCoreEncoding::boolean&&puppet.values.size()==1&&(puppet.values[0]==0||puppet.values[0]==1)&&live.puppet==(puppet.values[0]==1),"BNW JONS culture current puppet mirror stale");
    if(puppet.values[0])c.modifier=add(c.modifier,define("PUPPET_CULTURE_MODIFIER"));
    c.culture=mul(c.base,c.modifier)/100;
   }
   total=add(total,c.culture);q.cities.push_back(c);
  }
  q.total=total;q.calculated=true;
 }catch(const std::exception &e){q.total.reset();q.blocker=e.what();}
 return q;
}
}
