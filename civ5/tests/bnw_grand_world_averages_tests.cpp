#include "civ5/runtime/bnw_grand_world_averages.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <algorithm>
#include <iostream>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
using namespace civ5;
using namespace civ5::runtime;
namespace {
void check(bool value,const char *why){if(!value)throw std::runtime_error(why);}
auto &named(auto &rows,std::string_view name){for(auto &r:rows)if(r.name&&name==r.name)return r;throw std::runtime_error("Grand averages fixture native field absent");}
auto &buildings(auto &world,std::int64_t p){return named(world.world.all_players->players.at(p).cities.cities.at(0).production.blocks,"buildings");}
const auto &city_quote(const BnwGrandWorldAverages &q,std::int64_t p){for(const auto &r:q.players)if(r.player==p){check(r.cities.size()==1,"Selected actual city quote inventory differs");return r.cities[0];}throw std::runtime_error("Selected actual major quote absent");}
auto &owned_techs(auto &w,std::int64_t team){for(auto &t:w.technologies)if(t.team()==team)return t;throw std::runtime_error("Fixture owned team absent");}
void block(const BnwGrandWorldAverages &q,std::string_view reason){check(!q.calculated&&!q.culture_average&&!q.tourism_average&&!q.tech_average,"Incomplete query supplied a successful average");check(q.blocker.find(reason)!=std::string::npos,q.blocker.c_str());}
}
int main(int argc,char **argv)try{
#ifdef _WIN32
 ::SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX|SEM_NOOPENFILEERRORBOX);
#endif
 check(argc==3||argc==4,"Expected selected save installation [SDK root]");
 if(argc==4)for(const auto &source:kBnwGrandWorldAverageSources)check(identify_file(std::filesystem::path(argv[3])/source.path).sha256==source.sha256,"Grand averages native source pin differs");
 auto selected=ui::bootstrap_selected_bnw_preview(argv[1],argv[2],0);
 check(static_cast<bool>(selected.preview),"Selected preview bootstrap missing");
 auto &host=*selected.preview.host;const auto lease=host.session().read_state_lease();const auto &original=*lease;
 const auto generation=host.session().state_generation();const auto rules=BnwGrandWorldAverageRules::load(host.database());const auto identity=host.identity();
 const auto p=host.active_player_id();check(p==0&&original.cities.players.at(p).cities.size()==1,"This acceptance requires actual selected Gandhi city, not empty roster");
 const auto id=original.cities.players.at(p).cities.begin()->first;check(id==8192,"Selected actual city identity differs");
 const auto actual=quote_bnw_grand_world_averages(original,rules,identity);check(actual.calculated,actual.blocker.c_str());
 const auto &city=city_quote(actual,p);check(city.city==id&&!city.base_suppressed&&city.great_works==0&&city.base_before_modifiers==0&&city.tourism==0&&city.get_religion_null&&city.majority==-1&&city.league_query_executed&&city.theming_classes_visited==static_cast<std::int32_t>(rules.buildings().replacements().building_class_slots)&&city.enhanced_classes_visited==static_cast<std::int32_t>(rules.buildings().classes().size())&&city.policy_classes_visited==static_cast<std::int32_t>(rules.buildings().replacements().building_class_slots),"Selected zero tourism omitted native city/class/religion/league/policy query");
 check(actual.players.size()==2&&actual.teams.size()==2&&actual.culture_average==0&&actual.tourism_average==0&&actual.tech_average==1,"Selected source alive-major/alive-team averages differ");
 for(const auto &v:actual.teams)check(v.known==1,"Selected team tech count is not HasTech count");
 const auto repeated=quote_bnw_grand_world_averages(original,rules,identity);check(repeated.calculated&&repeated.culture_average==actual.culture_average&&repeated.tourism_average==actual.tourism_average&&repeated.tech_average==actual.tech_average&&host.session().state_generation()==generation,"Read-only query mutated selected world");
 std::cout<<"selected actual city8192 tourism0; culture0 tourism0 tech1, alive majors2/teams2 (query only)\n";
 // Actual City::getJONSCulturePerTurn is separate from lifetime culture and
 // generic city yields. Reuse the same source building/theming owners.
 const auto native=BnwHappinessSuffixRules::load(host.database());
 const auto actual_culture=quote_bnw_jons_culture_from_cities(original,rules,native,identity,p);
 check(actual_culture.calculated&&actual_culture.total&&actual_culture.cities.size()==1,actual_culture.blocker.c_str());
 const auto &cc=actual_culture.cities[0];check(cc.city==id&&!cc.suppressed&&cc.league_query_executed&&cc.theming_classes_visited==static_cast<std::int32_t>(rules.buildings().replacements().building_class_slots)&&cc.culture==*actual_culture.total,"Selected city culture skipped mandatory theming/league queries");
 std::cout<<"selected actual city JONS culture="<<*actual_culture.total<<" base="<<cc.base<<" modifier="<<cc.modifier<<"\n";
 const auto d=[&](const char *key){const auto *v=native.defines().find(key);check(v&&v->lookup.found&&std::holds_alternative<std::int64_t>(v->lookup.value),"Culture fixture actual native define absent");return std::get<std::int64_t>(v->lookup.value);};
 for(int scenario=0;scenario<16;++scenario){
  auto w=original;auto &pr=w.world.all_players->players.at(p);auto &raw=pr.cities.cities[0];auto &live=w.cities.players.at(p).cities.at(id);auto &b=buildings(w,p);
  named(raw.core.fields,"m_iJONSCulturePerTurnFromBuildings").values[0]=3;
  named(raw.core.fields,"m_iJONSCulturePerTurnFromPolicies").values[0]=2;
  named(raw.core.fields,"m_iJONSCulturePerTurnFromSpecialists").values[0]=4;
  named(raw.core.fields,"m_iJONSCulturePerTurnFromReligion").values[0]=5;
  named(raw.core.fields,"m_aiBaseYieldRateFromTerrain").values[4]=7;live.yields[4].terrain=7;
  named(pr.traits.fields,"m_iCityCultureBonus").values[0]=6;
  named(pr.core.fields,"m_aiGreatWorkYieldChange").values[4]=1;
  named(b.arrays,"m_aBuildingGreatWork").values={0,0,1,0,1,2,0,2,3};named(b.fields,"m_aBuildingGreatWork_wire_count").value=3;
  named(raw.core.fields,"m_iCultureRateModifier").values[0]=25;named(pr.core.fields,"m_iJONSCultureCityModifier").values[0]=15;
  named(raw.core.fields,"m_iNumWorldWonders").values[0]=1;named(pr.core.fields,"m_iCultureWonderMultiplier").values[0]=10;
  named(raw.core.fields,"m_bPuppet").values[0]=1;live.puppet=true;
  if(scenario==1)named(raw.core.fields,"m_iCultureRateModifier").values[0]=-250;
  if(scenario==2||scenario==3){named(raw.core.fields,scenario==2?"m_iResistanceTurns":"m_iRazingTurns").values[0]=1;if(scenario==2)live.resistance=true;else live.razing=true;std::erase_if(b.arrays,[](const auto &v){return v.name&&std::string_view(v.name)=="m_aBuildingGreatWork";});w.world.leagues.version.reset();}
  if(scenario==4)++named(raw.core.fields,"m_aiBaseYieldRateFromTerrain").values[4];
  if(scenario==5)named(b.fields,"m_aBuildingGreatWork_wire_count").value=4;
  if(scenario==6){named(raw.core.fields,"m_iNumWorldWonders").values[0]=0;w.world.leagues.version.reset();}
  if(scenario==7)w.world.leagues.leagues.emplace_back();
  if(scenario==8)live.puppet=false;
  if(scenario==9)pr.traits.layout=saves::GameStateLayout::vp542_corpus;
  if(scenario==10)named(pr.core.fields,"m_aiGreatWorkYieldChange").values.pop_back();
  if(scenario==11)named(raw.core.fields,"m_iNumWorldWonders").values[0]=-1;
  if(scenario==12)named(raw.core.fields,"m_iJONSCulturePerTurnFromBuildings").values[0]=INT32_MAX;
  if(scenario==13)++named(pr.cities.header.fields,"last_index").value;
  if(scenario==14){auto &rows=named(b.children,"m_paiNumRealBuilding").children;bool changed=false;for(auto &row:rows)if(named(row.fields,"hash").value&&named(row.arrays,"values").values[0]>0){++named(row.arrays,"values").values[0];changed=true;break;}check(changed,"Actual culture Palace fixture absent");}
  if(scenario==15)raw.core.fields.push_back(named(raw.core.fields,"m_iJONSCulturePerTurnFromBuildings"));
  const auto rng=w.random.gameplay.state();const auto q=quote_bnw_jons_culture_from_cities(w,rules,native,identity,p);
  if(scenario<4){check(q.calculated&&q.total&&q.cities.size()==1,q.blocker.c_str());const auto &c=q.cities[0];
   if(scenario<2){const auto base=3+2+4+3*(d("BASE_CULTURE_PER_GREAT_WORK")+1)+7+6+5;const auto mod=100+(scenario==1?-250:25)+15+10+d("PUPPET_CULTURE_MODIFIER");check(c.base==base&&c.modifier==mod&&c.culture==base*mod/100&&*q.total==c.culture&&c.league_query_executed&&c.theming_classes_visited==static_cast<std::int32_t>(rules.buildings().replacements().building_class_slots),"Positive JONS caches/work/terrain/trait/wonder/puppet native query wrong");}
   else check(c.suppressed&&q.total==0&&!c.league_query_executed&&!c.theming_classes_visited,"City resistance/razing native early return queried later dependencies");
  }else{check(!q.calculated&&!q.total,"Malformed mandatory city culture input computed a value");const char *reason=scenario==4?"terrain stale":scenario==5?"great-work inventory":scenario==6?"league Read inventory":scenario==7?"member/resolution owner":scenario==8?"puppet mirror stale":scenario==9?"trait Read missing":scenario==10?"GreatWorkYieldChange array":scenario==11?"wonder counter":scenario==12?"overflow/width":scenario==13?"city FLTA inventory":scenario==14?"building count stale":"duplicate native field";check(q.blocker.find(reason)!=std::string::npos,q.blocker.c_str());}
  check(w.random.gameplay.state()==rng&&host.session().state_generation()==generation,"Culture city read query consumed RNG/published world");
 }
 // A positive theming-capable actual Palace must require the full native
 // theming producer even when the great-work size/cache yield is zero.
 {auto themed_db=host.database().clone_gameplay();themed_db->apply_sql("INSERT INTO Building_ThemingBonuses (BuildingType,Bonus) VALUES ('BUILDING_PALACE',3);");
 const auto themed=BnwGrandWorldAverageRules::load(*themed_db);const auto themed_native=BnwHappinessSuffixRules::load(*themed_db);auto themed_id=identity;themed_id.rules_sha256=themed.identity();
 const auto q=quote_bnw_jons_culture_from_cities(original,themed,themed_native,themed_id,p);check(!q.calculated&&!q.total&&q.blocker.find("GetThemingBonus")!=std::string::npos,q.blocker.c_str());}
 // Isolated current-state fixtures, not selected stock Lua/turn completion.
 // Their changes are test operands; none is a gameplay publication/receipt.
 for(int scenario=0;scenario<17;++scenario){
  auto w=original;auto &raw=w.world.all_players->players.at(p);auto &c=raw.cities.cities.at(0);auto &live=w.cities.players.at(p).cities.at(id);auto &b=buildings(w,p);
  auto &works=named(b.arrays,"m_aBuildingGreatWork");works.values={0,0,1,0,1,2,0,2,3};named(b.fields,"m_aBuildingGreatWork_wire_count").value=3;named(b.fields,"m_iGreatWorksTourismModifier").value=50;
  named(raw.core.fields,"m_iJONSCultureEverGenerated").values[0]=5;
  named(w.world.all_players->players.at(1).core.fields,"m_iJONSCultureEverGenerated").values[0]=8;
  const auto base=3*rules.base_tourism()+3*rules.base_tourism()*50/100;
  if(scenario==1){named(raw.core.fields,"m_iInfluenceSpreadModifier").values[0]=25;named(raw.core.fields,"m_iTourismBonusTurns").values[0]=1;named(raw.core.fields,"m_iGoldenAgeTurns").values[0]=1;w.cities.players.at(p).golden_age=true;named(raw.traits.fields,"m_iGoldenAgeTourismModifier").values[0]=25;}
  if(scenario==2){named(raw.core.fields,"m_iInfluenceSpreadModifier").values[0]=-25;named(raw.core.fields,"m_iGoldenAgeTurns").values[0]=1;w.cities.players.at(p).golden_age=true;named(raw.traits.fields,"m_iGoldenAgeTourismModifier").values[0]=-200;}
  if(scenario==3){named(c.core.fields,"m_iResistanceTurns").values[0]=1;live.resistance=true;std::erase_if(b.arrays,[](const auto &a){return a.name&&std::string_view(a.name)=="m_aBuildingGreatWork";});}
  if(scenario==4){named(c.core.fields,"m_iRazingTurns").values[0]=1;live.razing=true;w.world.leagues.version.reset();}
  if(scenario==5)named(b.fields,"m_aBuildingGreatWork_wire_count").value=4;
  if(scenario==6){auto &rows=named(b.children,"m_paiNumRealBuilding").children;bool changed=false;for(auto &row:rows)if(named(row.fields,"hash").value&&named(row.arrays,"values").values[0]>0){++named(row.arrays,"values").values[0];changed=true;break;}check(changed,"Actual positive Palace count fixture absent");}
  if(scenario==7)named(b.fields,"m_iLandmarksTourismPercent").value=1;
  if(scenario==8){auto &status=named(named(c.tail.blocks,"religions").children,"m_ReligionStatus");named(status.children[0].children,"m_eReligion").values[0]=1;live.religions[0].religion=1;saves::ReligionObservation entry;entry.version=3;entry.religion=1;entry.founder=p;w.world.religion_culture.religions.push_back(entry);}
  if(scenario==9){named(c.core.fields,"m_iResistanceTurns").values[0]=1;live.resistance=true;w.setup.players.at(p).policies_resolved=false;}
  if(scenario==10)++named(w.world.teams->teams.at(0).fields,"alive_count").value;
  if(scenario==11)owned_techs(w,0).increment_count(0);
  if(scenario==12){named(raw.core.fields,"m_iGoldenAgeTurns").values[0]=1;w.cities.players.at(p).golden_age=true;std::erase_if(raw.traits.fields,[](const auto &f){return f.name&&std::string_view(f.name)=="m_iGoldenAgeTourismModifier";});}
  if(scenario==13)++w.cities.players.at(p).civilization;
  if(scenario==14)named(raw.core.fields,"m_iJONSCultureEverGenerated").encoding=saves::PlayerCoreEncoding::boolean;
  if(scenario==15){named(raw.core.fields,"m_iJONSCultureEverGenerated").values[0]=INT32_MAX;named(w.world.all_players->players.at(1).core.fields,"m_iJONSCultureEverGenerated").values[0]=INT32_MAX;}
  if(scenario==16)w.world.leagues.leagues.emplace_back();
  const auto game_rng=w.random.gameplay.state(),map_rng=w.random.map.state();
  const auto q=quote_bnw_grand_world_averages(w,rules,identity);
  if(scenario<=3){check(q.calculated,q.blocker.c_str());const auto &v=city_quote(q,p);const auto expected=scenario==3?0:scenario==1?base*(150+rules.temporary_boost())/100:base;check(v.tourism==expected&&q.culture_average==6&&q.tourism_average==expected/2&&q.tech_average==1,"Positive native GreatWorks/modifier/truncation or suppressed base branch wrong");if(scenario==3)check(v.base_suppressed&&v.league_query_executed&&v.policy_classes_visited==static_cast<std::int32_t>(rules.buildings().replacements().building_class_slots),"Suppressed base skipped later source getter");}
  else if(scenario==4)block(q,"GameLeagues Read");
  else if(scenario==5)block(q,"great-work inventory");
  else if(scenario==6)block(q,"building count stale");
  else if(scenario==7)block(q,"LandmarksTourismPercent");
  else if(scenario==8)block(q,"actual beliefs");
  else if(scenario==9)block(q,"shared policy owner stale");
  else if(scenario==10)block(q,"alive count stale");
  else if(scenario==11)block(q,"HasTech registry");
  else if(scenario==12)block(q,"required native field missing");
  else if(scenario==13)block(q,"civilization mirror stale");
  else if(scenario==14)block(q,"source encoding");
  else if(scenario==15)block(q,"overflow/width");
  else if(scenario==16)block(q,"active-resolution holy-city owner");
  check(w.random.gameplay.state()==game_rng&&w.random.map.state()==map_rng&&host.session().state_generation()==generation,"Query mutated RNG/live public session");
 }

 // Isolated rules fixture exercises actual native Info/matrix loading and the
 // positive EnhancedYieldTech -> current HasTech path, not a formula callback.
 auto db=host.database().clone_gameplay();db->apply_sql(
  "UPDATE Buildings SET TechEnhancedTourism=7,EnhancedYieldTech='TECH_AGRICULTURE' WHERE Type='BUILDING_PALACE';"
  "DELETE FROM Policy_BuildingClassTourismModifiers;"
  "INSERT INTO Policy_BuildingClassTourismModifiers (PolicyType,BuildingClassType,TourismModifier) VALUES ('POLICY_TRADITION','BUILDINGCLASS_PALACE',25);");
 const auto positive=BnwGrandWorldAverageRules::load(*db);auto positive_identity=identity;positive_identity.rules_sha256=positive.identity();
 auto w=original;auto &pr=w.world.all_players->players.at(p);auto &policies=named(pr.strategy.blocks,"policies");
 const auto *opener=positive.types().find("POLICY_TRADITION");const auto *branch=positive.types().find("POLICY_BRANCH_TRADITION");const auto *tech=positive.types().find("TECH_AGRICULTURE");check(opener&&branch&&tech,"Positive source rules fixture types missing");
 auto &flags=named(policies.hashes,"has_policy");flags.rows={{opener->hash,{1}}};flags.wire_count=1;
 auto &blocked=named(policies.hashes,"branch_blocked");blocked.rows={{branch->hash,{0}}};blocked.wire_count=1;
 w.setup.players.at(p).policies={opener->id};w.setup.players.at(p).policies_resolved=true;
 auto &team=w.world.teams->teams.at(w.setup.players.at(p).team);auto &hashes=named(team.arrays,"tech_hashes").values;auto &has=named(team.arrays,"has_tech").values;auto &counts=named(team.arrays,"tech_count").values;
 const auto found=std::find(hashes.begin(),hashes.end(),static_cast<std::int64_t>(tech->hash));check(found!=hashes.end(),"Positive current team Read technology absent");const auto slot=static_cast<std::size_t>(found-hashes.begin());has.at(slot)=1;counts.at(slot)=9;
 owned_techs(w,w.setup.players.at(p).team)=restore_team_technologies(team,positive.technologies(),positive.types());
 const auto known=owned_techs(w,w.setup.players.at(p).team).known_count();check(known==1,"Repeat count changed native HasTech known count");
 auto q=quote_bnw_grand_world_averages(w,positive,positive_identity);check(q.calculated,q.blocker.c_str());check(city_quote(q,p).base_before_modifiers==7&&city_quote(q,p).modifier==25&&city_quote(q,p).tourism==8&&q.tourism_average==4&&q.tech_average==1,"Positive source tech-enhancement/policy-matrix/known-count queries wrong");
 blocked.rows[0].values[0]=1;q=quote_bnw_grand_world_averages(w,positive,positive_identity);check(q.calculated,q.blocker.c_str());check(city_quote(q,p).modifier==0&&city_quote(q,p).tourism==7,"Blocked free opener remained active in native tourism matrix");
 has.at(slot)=0;owned_techs(w,w.setup.players.at(p).team)=restore_team_technologies(team,positive.technologies(),positive.types());q=quote_bnw_grand_world_averages(w,positive,positive_identity);check(q.calculated,q.blocker.c_str());check(city_quote(q,p).base_before_modifiers==0&&q.tech_average==0,"Repeat counter substituted for source HasTech false");
 std::cout<<"Grand culture/tourism/tech positive/current-owner/suppressed-base/source-order guards PASS; no turn/readiness grant\n";
}catch(const std::exception &e){std::cerr<<"Grand averages FAIL: "<<e.what()<<'\n';return 1;}
