#include "civ5/runtime/bnw_normal_plot_yield.hpp"
#include "civ5/runtime/bnw_lua_calculated_plot_yield.hpp"
#include "civ5/runtime/lua_map_services.hpp"
#include "civ5/runtime/bnw_happiness_policies.hpp"
#include "civ5/runtime/map_geometry.hpp"
#include "civ5/runtime/bnw_city_strategy_yields.hpp"
#include "civ5/runtime/bnw_city_registry.hpp"
#include "civ5/runtime/bnw_live_plot_impassability.hpp"
#include "civ5/runtime/improvement_yield_calculation.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <algorithm>
#include <climits>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
using namespace civ5;using namespace civ5::runtime;
namespace {
void need(bool b,const char* m){if(!b)throw std::runtime_error(m);}
template<class F>void refuses(F&& f){bool caught=false;try{f();}catch(const BnwNormalPlotYieldUnavailable&){caught=true;}need(caught,"Normal plot source accepted unsupported/stale operands");}
template<class F>std::string exact_normal_refusal(F&& f){try{f();}catch(const BnwNormalPlotYieldUnavailable& e){return e.what();}throw std::runtime_error("Expected exact normal source refusal absent");}
void paired_force(LiveSessionState& s,std::size_t plot,std::int64_t team,bool enabled);
template<class T>T& named(std::vector<T>& v,std::string_view name){for(auto& x:v)if(x.name&&std::string_view(x.name)==name)return x;throw std::runtime_error("Fixture named raw field absent");}
template<class T>const T& named(const std::vector<T>& v,std::string_view name){for(const auto& x:v)if(x.name&&std::string_view(x.name)==name)return x;throw std::runtime_error("Fixture named raw field absent");}
void paired_force(LiveSessionState& s,std::size_t plot,std::int64_t team,bool enabled){
 need(team>=0&&team<64,"Detached force team outside active native domain");
 auto& raw=named(s.world.map_plots->plots.at(plot).arrays,"resource_force_reveal");
 auto& projected=s.world.map_projection->cells.at(plot).force_resource_reveal;
 need(raw.columns==1&&raw.values.size()==80&&projected,"Detached force native storage80 absent");
 raw.values.at(static_cast<std::size_t>(team))=enabled?1:0;
 if(enabled)projected->insert(team);else projected->erase(team);
}
bool none(const std::optional<std::int64_t>& id,const std::optional<std::uint64_t>& hash){return (id&&*id==-1&&!hash)||(!id&&hash&&*hash==0);}
saves::TeamObservation& source_team(LiveSessionState& state,std::int64_t team){for(auto& raw:state.world.teams->teams)if(named(raw.fields,"id").value==team)return raw;throw std::runtime_error("Actual source team absent");}
TeamTechnologies& owned_team(LiveSessionState& state,std::int64_t team){for(auto& owned:state.technologies)if(owned.team()==team)return owned;throw std::runtime_error("Actual owned team absent");}
const content::InfoTypeEntry& info_id(const content::InfoTypeCatalog& types,std::string_view table,std::int64_t id){for(const auto& [name,t]:types.entries()){(void)name;if(t.table==table&&t.id==id)return t;}throw std::runtime_error("Actual selected Info ID absent");}
void paired_resource(LiveSessionState& state,std::size_t plot,std::uint32_t hash){named(state.world.map_plots->plots.at(plot).fields,"resource_type_hash").value=hash;state.world.map_projection->cells.at(plot).resource_type_hash=hash;}
void paired_tech(LiveSessionState& state,const content::InfoTypeCatalog& types,std::int64_t team,std::int64_t tech,bool value){
 if(tech==-1)return;auto& raw=source_team(state,team);const auto hash=info_id(types,"Technologies",tech).hash;const auto& hashes=named(raw.arrays,"tech_hashes").values;auto& flags=named(raw.arrays,"has_tech").values;bool found=false;
 for(std::size_t i=0;i<hashes.size();++i)if(hashes[i]==hash){flags.at(i)=value?1:0;found=true;}need(found,"Actual selected technology source row absent");owned_team(state,team).set_has_tech(tech,value);
}
void paired_policy(LiveSessionState& state,const content::InfoTypeCatalog& types,std::int64_t player,std::int64_t policy,bool value){
 auto& rows=named(named(state.world.all_players->players.at(static_cast<std::size_t>(player)).strategy.blocks,"policies").hashes,"has_policy");
 rows.rows.push_back({info_id(types,"Policies",policy).hash,{value?1:0}});++rows.wire_count;
 auto& owned=state.setup.players.at(player);need(owned.policies_resolved,"Fixture policy mirror must first come from actual restore");if(value)owned.policies.insert(policy);else owned.policies.erase(policy);
}
void paired_improvement(LiveSessionState& state,std::size_t plot,std::uint32_t hash,bool pillaged){
 auto& raw=state.world.map_plots->plots.at(plot);auto& owned=state.world.map_projection->cells.at(plot);
 named(raw.fields,"improvement_type_hash").value=hash;owned.improvement_type_hash=hash;
 named(raw.fields,"improvement_pillaged").value=pillaged?1:0;owned.improvement_pillaged=pillaged;
}
void paired_branch_blocked(LiveSessionState& state,const content::InfoTypeCatalog& types,std::int32_t player,std::int32_t branch,bool blocked){
 auto& rows=named(named(state.world.all_players->players.at(static_cast<std::size_t>(player)).strategy.blocks,"policies").hashes,"branch_blocked");rows.rows.push_back({info_id(types,"PolicyBranchTypes",branch).hash,{blocked?1:0}});++rows.wire_count;
}
template<class F>void exercise_improvement_culture(const ui::SelectedBnwPreviewHost& host,F&& calc){
 const auto original=host.session().state();const auto policy_binding=BnwPolicyHappinessRules::load(host.database());
 auto controlled=original;auto& owner=controlled.world.all_players->players.at(0);named(owner.core.fields,"m_iGoldenAgeTurns").values[0]=0;controlled.cities.players.at(0).golden_age=false;named(owner.core.fields,"m_aiExtraYieldThreshold").values[4]=0;
 need(controlled.setup.players.at(0).policies_resolved,"Actual source policy mirror not imported; culture must remain unavailable");
 const auto* holy=host.types().find("IMPROVEMENT_HOLY_SITE");const auto* piety=host.types().find("POLICY_PIETY_FINISHER");const auto* landmark=host.types().find("IMPROVEMENT_LANDMARK");const auto* deal=host.types().find("POLICY_NEW_DEAL");
 need(holy&&piety&&landmark&&deal&&policy_binding.policies().at(piety->id).blocked_branch==-1&&deal->id<=INT32_MAX,"Actual culture policy/improvement source fixture absent");
 const auto relation=[&](const content::InfoTypeEntry& policy,const content::InfoTypeEntry& improvement){std::int32_t amount=0;const auto rows=host.database().query_rows({"select CultureChange from Policy_ImprovementCultureChanges where PolicyType = ? and ImprovementType = ?",{policy.type,improvement.type}},[&](const auto& row){const auto value=row.integer_at(0);need(value>=INT32_MIN&&value<=INT32_MAX,"Actual culture metadata outside native width");amount=static_cast<std::int32_t>(value);});need(rows.all_succeeded(),"Actual culture relation query unavailable");return amount;};
 const auto holy_change=relation(*piety,*holy),landmark_change=relation(*deal,*landmark);need(holy_change>0&&landmark_change>0,"Actual culture policy terms not meaningful");
 paired_improvement(controlled,673,holy->hash,false);paired_policy(controlled,host.types(),0,piety->id,false);const auto before_holy=calc(controlled,673,4);
 paired_policy(controlled,host.types(),0,piety->id,true);need(calc(controlled,673,4)==before_holy+holy_change,"Owned policy per-improvement culture getter missing");
 const auto* piety_branch=host.types().find("POLICY_BRANCH_PIETY");need(piety_branch&&piety_branch->id>=0&&piety_branch->id<=INT32_MAX,"Actual Piety branch missing");paired_branch_blocked(controlled,host.types(),0,static_cast<std::int32_t>(piety_branch->id),true);
 need(calc(controlled,673,4)==before_holy+holy_change,"Finisher incorrectly inherited a free-opener blocked branch");
 auto landmark_case=controlled;paired_improvement(landmark_case,673,landmark->hash,false);paired_policy(landmark_case,host.types(),0,deal->id,false);const auto branch=policy_binding.policies().at(deal->id).blocked_branch;need(branch>=0&&branch<=INT32_MAX,"Actual New Deal blocked branch missing");
 paired_branch_blocked(landmark_case,host.types(),0,static_cast<std::int32_t>(branch),false);const auto before_deal=calc(landmark_case,673,4);paired_policy(landmark_case,host.types(),0,deal->id,true);
 need(calc(landmark_case,673,4)==before_deal+landmark_change,"Active New Deal culture missing");paired_branch_blocked(landmark_case,host.types(),0,static_cast<std::int32_t>(branch),true);need(calc(landmark_case,673,4)==before_deal,"Blocked policy still contributed improvement culture");
 const auto* ancient=host.types().find("ERA_ANCIENT");const auto* classical=host.types().find("ERA_CLASSICAL");need(ancient&&classical&&ancient->id==0&&classical->id==1,"Actual native Era fixture changed");
 auto era_case=landmark_case;const auto team=era_case.setup.players.at(0).team;named(source_team(era_case,team).fields,"era").value=ancient->id;era_case.maintenance.team_eras.at(team)=ancient->id;
 named(era_case.world.map_plots->plots.at(673).fields,"artifact_era").value=-1;era_case.world.map_projection->cells.at(673).artifact_era=-1;const auto no_era=calc(era_case,673,4);
 named(era_case.world.map_plots->plots.at(673).fields,"artifact_era").value=ancient->id;era_case.world.map_projection->cells.at(673).artifact_era=ancient->id;need(calc(era_case,673,4)==no_era,"NO_ERA native subtraction leaked into source era contribution");
 std::int32_t per_era=0;const auto era_rows=host.database().query_rows({"select Yield from Improvement_YieldPerEra where ImprovementType = ? and YieldType = 'YIELD_CULTURE'",{landmark->type}},[&](const auto& row){const auto value=row.integer_at(0);need(value>0&&value<=INT32_MAX,"Actual Landmark per-era source term invalid");per_era=static_cast<std::int32_t>(value);});need(era_rows.all_succeeded()&&per_era>0,"Actual Landmark per-era metadata absent");
 named(source_team(era_case,team).fields,"era").value=classical->id;era_case.maintenance.team_eras.at(team)=classical->id;need(calc(era_case,673,4)==no_era+per_era,"Actual current team/artifact era operands were ignored");
 auto stale_era=era_case;stale_era.maintenance.team_eras.at(team)=ancient->id;refuses([&]{(void)calc(stale_era,673,4);});
 auto invalid_era=era_case;named(invalid_era.world.map_plots->plots.at(673).fields,"artifact_era").value=-2;invalid_era.world.map_projection->cells.at(673).artifact_era=-2;refuses([&]{(void)calc(invalid_era,673,4);});
 auto culture_overflow=landmark_case;const auto culture_index=static_cast<std::size_t>(landmark->id)*6+4;named(named(culture_overflow.world.all_players->players.at(0).administration.blocks,"pre_city").arrays,"m_ppaaiImprovementYieldChange").values.at(culture_index)=INT32_MAX;named(culture_overflow.world.all_players->players.at(0).traits.fields,"m_ppaaiImprovementYieldChange").values.at(culture_index)=-INT32_MAX;refuses([&]{(void)calc(culture_overflow,673,4);});
 auto mirror=controlled;mirror.setup.players.at(0).policies.erase(piety->id);refuses([&]{(void)calc(mirror,673,4);});
 for(const auto invalid:{std::int64_t{2},std::int64_t{255}}){auto flags=controlled;auto& rows=named(named(flags.world.all_players->players.at(0).strategy.blocks,"policies").hashes,"branch_blocked");rows.rows.push_back({piety_branch->hash,{invalid}});++rows.wire_count;bool exact=false;try{(void)calc(flags,673,4);}catch(const BnwNormalPlotYieldUnavailable& e){exact=std::string_view(e.what())=="Normal improvement culture native policy boolean invalid";}need(exact,"Malformed blocked branch boolean was not rejected before common flags helper");}
 auto zero_hash=controlled;auto& null_rows=named(named(zero_hash.world.all_players->players.at(0).strategy.blocks,"policies").hashes,"has_policy");null_rows.rows.push_back({0,{1}});++null_rows.wire_count;refuses([&]{(void)calc(zero_hash,673,4);});
 auto wrong_hash=controlled;auto& wrong_rows=named(named(wrong_hash.world.all_players->players.at(0).strategy.blocks,"policies").hashes,"branch_blocked");wrong_rows.rows.push_back({holy->hash,{0}});++wrong_rows.wire_count;refuses([&]{(void)calc(wrong_hash,673,4);});
 // Report the actual global numeric culture metadata even if standard BNW
 // gives zero for every policy. No fabricated SQL modifier replaces it.
 std::size_t numeric_nonzero=0;const auto numeric=host.database().query_rows({"select CultureImprovementChange from Policies",{}},[&](const auto& row){if(row.integer_at(0)!=0)++numeric_nonzero;});need(numeric.all_succeeded(),"Actual global policy culture metadata query failed");std::cout<<"Actual policies with numeric improvement culture="<<numeric_nonzero<<"\n";
 const auto* moai=host.types().find("IMPROVEMENT_MOAI");
 if(moai){auto adjacency=controlled;paired_improvement(adjacency,673,moai->hash,false);std::int32_t bonus=0;const auto metadata=host.database().query_rows({"select CultureAdjacentSameType from Improvements where Type = ?",{moai->type}},[&](const auto& row){const auto value=row.integer_at(0);need(value>0&&value<=INT32_MAX,"Actual Moai adjacency source term invalid");bonus=static_cast<std::int32_t>(value);});need(metadata.all_succeeded()&&bonus>0,"Actual Moai adjacency metadata absent");
  const auto& map=*adjacency.world.map_projection;const auto& cell=map.cells.at(673);need(cell.x<=INT32_MAX&&cell.y<=INT32_MAX,"Actual adjacency coordinates exceed native width");const MapGeometry geometry{map.grid,map.wrap_x,map.wrap_y,saves::GameStateLayout::expansion2_version1};std::optional<std::int64_t> chosen;for(std::int32_t d=0;d<6&&!chosen;++d)chosen=geometry.direction(static_cast<std::int64_t>(cell.x),static_cast<std::int64_t>(cell.y),d);need(chosen.has_value(),"Actual culture neighbor absent");
  const auto baseline=calc(adjacency,673,4);std::int32_t slots=0;for(std::int32_t d=0;d<6;++d)if(geometry.direction(static_cast<std::int64_t>(cell.x),static_cast<std::int64_t>(cell.y),d)==chosen)++slots;
  // Neighbor owner remains genuine. Neighbor pillage is deliberately true:
  // native culture adjacency filters neither owner nor neighbor pillage.
  paired_improvement(adjacency,static_cast<std::size_t>(*chosen),moai->hash,true);const auto expected=std::int64_t(baseline)+std::int64_t(slots)*bonus;need(expected>=0&&expected<=INT32_MAX,"Actual adjacency fixture expected sum outside native width");need(calc(adjacency,673,4)==expected,"Same-type culture adjacency lost source slot multiplicity or filtered pillage");
 }else std::cout<<"Actual activated MOAI absent; adjacency runtime scene pending, no invented rule\n";
 std::cout<<"PASS CULTURE source math: real policy flags/blocked branches, finisher mapping, current mirror, native bool and same-type adjacency; no policy activation or construction command\n";
}
template<class F>void exercise_improvements(const ui::SelectedBnwPreviewHost& host,F&& calc){
 const auto original=host.session().state();const auto* farm=host.types().find("IMPROVEMENT_FARM");const auto* forest=host.types().find("FEATURE_FOREST");
 need(farm&&farm->table=="Improvements"&&farm->id>=0&&farm->id<=INT32_MAX&&forest&&forest->id>=0&&forest->id<=INT32_MAX&&original.world.map_projection->cells.size()<=INT32_MAX,"Actual improvement/forest Info missing");
 std::size_t actual_improved=0;for(const auto& p:original.world.map_projection->cells)if(!none(p.improvement_type,p.improvement_type_hash))++actual_improved;
 std::cout<<"Actual selected improved plots="<<actual_improved<<"; FARM cases below are detached read-only math variations, not constructed improvements\n";
 auto bare=original;auto& owner=bare.world.all_players->players.at(0);named(owner.core.fields,"m_iGoldenAgeTurns").values[0]=0;bare.cities.players.at(0).golden_age=false;named(owner.core.fields,"m_aiExtraYieldThreshold").values[0]=0;
 need(bare.world.map_projection->cells.at(673).owner==0&&none(bare.world.map_projection->cells.at(673).improvement_type,bare.world.map_projection->cells.at(673).improvement_type_hash),"Actual forest improvement fixture changed");
 auto pillaged=bare;paired_improvement(pillaged,673,farm->hash,true);
 std::array<std::int32_t,6> pillaged_yields{};for(std::int32_t y=0;y<6;++y)pillaged_yields.at(static_cast<std::size_t>(y))=calc(pillaged,673,y);
 // Presence suppresses the source unimproved-feature trait even if pillaged.
 auto bare_trait=bare;auto pillaged_trait=pillaged;
 const auto feature_index=static_cast<std::size_t>(forest->id)*6;
 named(bare_trait.world.all_players->players.at(0).traits.fields,"m_ppaaiUnimprovedFeatureYieldChange").values.at(feature_index)+=3;
 named(pillaged_trait.world.all_players->players.at(0).traits.fields,"m_ppaaiUnimprovedFeatureYieldChange").values.at(feature_index)+=3;
 need(calc(bare_trait,673,0)==calc(bare,673,0)+3,"Bare trait control did not reach actual source term");
 for(std::int32_t y=0;y<6;++y)need(calc(pillaged_trait,673,y)==pillaged_yields.at(static_cast<std::size_t>(y)),"Pillaged presence incorrectly restored unimproved-feature trait");
 auto improved=pillaged;paired_improvement(improved,673,farm->hash,false);
 std::array<std::int32_t,6> improved_yields{};for(std::int32_t y=0;y<6;++y)improved_yields.at(static_cast<std::size_t>(y))=calc(improved,673,y);
 need(improved_yields[0]>pillaged_yields[0],"Actual FARM helper FOOD contribution was not meaningful");
 need(improved_yields[4]>=0,"Owned FARM culture closure unavailable");
 // Source caches are facts already stored by player/team/trait processing;
 // independent increments must all reach the existing remapper and helper.
 auto counters=improved;const auto row_index=static_cast<std::size_t>(farm->id)*6;auto& raw_player=counters.world.all_players->players.at(0);
 named(named(raw_player.administration.blocks,"pre_city").arrays,"m_ppaaiImprovementYieldChange").values.at(row_index)+=5;
 named(raw_player.traits.fields,"m_ppaaiImprovementYieldChange").values.at(row_index)+=4;
 const auto team=counters.setup.players.at(0).team;auto& raw_team=source_team(counters,team);
 const auto append_delta=[&](const char* name,std::int64_t delta){auto& rows=named(raw_team.hashes,name);std::vector<std::int64_t> values(6,0);for(const auto& row:rows.rows)if(row.type_hash==farm->hash)values=row.values;need(values.size()==6,"Actual native team matrix row malformed");values[0]+=delta;rows.rows.push_back({farm->hash,std::move(values)});++rows.wire_count;};
 append_delta("improvement_yields",3);append_delta("improvement_dry_yields",2);append_delta("improvement_fresh_yields",2);
 need(calc(counters,673,0)==improved_yields[0]+14,"Current player/trait/team/fresh-or-dry matrix contributions were lost");
 auto overflow=counters;named(named(overflow.world.all_players->players.at(0).administration.blocks,"pre_city").arrays,"m_ppaaiImprovementYieldChange").values.at(row_index)=INT32_MAX;
 named(overflow.world.all_players->players.at(0).traits.fields,"m_ppaaiImprovementYieldChange").values.at(row_index)=-INT32_MAX;refuses([&]{(void)calc(overflow,673,0);});
 auto wrong_width=improved;named(named(wrong_width.world.all_players->players.at(0).administration.blocks,"pre_city").arrays,"m_ppaaiImprovementYieldChange").columns=5;refuses([&]{(void)calc(wrong_width,673,0);});
 auto wrong_team=improved;auto& rows=named(source_team(wrong_team,team).hashes,"improvement_yields");rows.rows.push_back({forest->hash,std::vector<std::int64_t>(6,0)});++rows.wire_count;refuses([&]{(void)calc(wrong_team,673,0);});
 auto stale_pillage=improved;stale_pillage.world.map_projection->cells.at(673).improvement_pillaged=true;refuses([&]{(void)calc(stale_pillage,673,0);});
 auto malformed_pillage=improved;named(malformed_pillage.world.map_plots->plots.at(673).fields,"improvement_pillaged").value=2;malformed_pillage.world.map_projection->cells.at(673).improvement_pillaged=true;refuses([&]{(void)calc(malformed_pillage,673,0);});
 auto stale_era=improved;stale_era.world.map_projection->cells.at(673).artifact_era=INT32_MAX;refuses([&]{(void)calc(stale_era,673,0);});
 auto wide_river=improved;named(wide_river.world.map_plots->plots.at(673).fields,"river_crossing_bits").value=128;wide_river.world.map_projection->cells.at(673).river_crossing_bits=128;refuses([&]{(void)calc(wide_river,673,0);});
 auto wrong_improvement=improved;paired_improvement(wrong_improvement,673,forest->hash,false);refuses([&]{(void)calc(wrong_improvement,673,0);});
 // Outer route pillage does not suppress the helper's route Info lookup.
 // A paired unknown route makes that distinction observable without forging
 // an improvement/route SQL bonus or any gameplay owner.
 auto helper_route=improved;named(helper_route.world.map_plots->plots.at(673).fields,"route_type").value=INT32_MAX;helper_route.world.map_projection->cells.at(673).route->type=INT32_MAX;
 named(helper_route.world.map_plots->plots.at(673).fields,"route_pillaged").value=1;helper_route.world.map_projection->cells.at(673).route->pillaged=true;
 bool route_exact=false;try{(void)calc(helper_route,673,0);}catch(const BnwNormalPlotYieldUnavailable& e){route_exact=std::string_view(e.what())=="Normal improvement reached Route Info unavailable";}need(route_exact,"Route pillage skipped improvement route lookup or stopped at unrelated frontier");
 paired_improvement(helper_route,673,farm->hash,true);for(std::int32_t y=0;y<6;++y)need(calc(helper_route,673,y)>=0,"Pillaged improvement fetched unreachable helper route");
 // Existing actual full/index city references keep the same current registry.
 const auto ref=*improved.world.map_projection->cells.at(673).yield_city;const auto* raw=resolve_bnw_current_city(improved,static_cast<std::int32_t>(ref.owner),static_cast<std::int32_t>(ref.id));need(raw,"Actual improvement working city absent");const auto full=named(raw->core.fields,"m_iID").values.at(0);
 for(const auto handle:{full,full&0x1fff}){auto alias=improved;named(alias.world.map_plots->plots.at(673).fields,"working_city_id").value=handle;alias.world.map_projection->cells.at(673).yield_city->id=handle;for(std::int32_t y=0;y<6;++y)need(calc(alias,673,y)==improved_yields.at(static_cast<std::size_t>(y)),"Improvement full/index working registry differs");}
 // Genuine neutral improvements remain UNOBSERVED until this test is run.
 // Bounded future observation: first real neutral improved candidate only,
 // six scalar queries. The scan itself never calls a yield producer.
 bool observed_neutral=false;for(std::size_t i=0;i<original.world.map_projection->cells.size();++i){const auto& p=original.world.map_projection->cells[i];if(p.owner!=-1||none(p.improvement_type,p.improvement_type_hash))continue;
  for(std::int32_t y=0;y<6;++y)need(calc(original,static_cast<std::int32_t>(i),y)>=0,"Actual neutral improvement failed source query");observed_neutral=true;std::cout<<"Observed genuine neutral improved plot="<<i<<"\n";break;
 }if(!observed_neutral)std::cout<<"Genuine neutral improved scene absent from selected save; detached FARM math follows, no construction\n";
 bool unowned_checked=false;for(std::size_t i=0;i<original.world.map_projection->cells.size();++i){const auto& p=original.world.map_projection->cells[i];if(p.owner!=-1||!p.potential_city_work||!*p.potential_city_work||p.plot_type==0||p.plot_type==3||bnw_live_plot_impassability(original).plots.at(i).impassable||!none(p.feature_type,p.feature_type_hash)||!none(p.resource_type,p.resource_type_hash)||!none(p.improvement_type,p.improvement_type_hash))continue;
  auto unowned=original;paired_improvement(unowned,i,farm->hash,false);auto neutral_pillaged=unowned;paired_improvement(neutral_pillaged,i,farm->hash,true);
  auto cities=unowned.cities;const auto rules=load_improvement_yield_rules(host.database(),content::TechnologyProfile::bnw);const auto religion=load_improvement_religion_rules(host.database());const auto state=restore_improvement_yield_state(unowned.world,rules,host.types(),cities);
  auto setup=unowned.setup;auto map=*unowned.world.map_projection;const auto water_state=restore_plot_water(unowned.world);const auto water_rules=load_plot_water_rules(host.database(),content::TechnologyProfile::bnw);const auto defines=host.database().cache_global_defines(content::GlobalDefineProfile::bnw);const auto& pass=bnw_live_plot_impassability(unowned);
  PlotWaterContext water{map,water_state,water_rules,host.types(),pass,setup,cities,defines};const auto visibility=load_resource_visibility_rules(host.database(),host.types(),content::TechnologyProfile::bnw);ResourceVisibilityState no_team;
  ResourceVisibilityContext resources{map,no_team,visibility,setup,unowned.player_research,unowned.technologies,host.types(),defines,false};ImprovementYieldContext context{rules,state,water,resources,unowned.maintenance.team_eras,&religion};context.current_city_registry=&unowned.world;
  const bool fresh=plot_is_fresh_water(water,static_cast<std::int32_t>(i));const auto& improvement_info=rules.improvements.at(farm->id);const auto get=[](const YieldValues& row,std::int32_t y){const auto it=row.find(y);return it==row.end()?std::int64_t{0}:it->second;};
  std::array<std::int32_t,6> neutral_values{};
  for(std::int32_t y=0;y<6;++y){std::int64_t tech=0,policies=0;const auto& wet=fresh?improvement_info.fresh_technologies:improvement_info.dry_technologies;
   for(std::uint64_t id=0;id<rules.technology_slots;++id){for(const auto* rows:{&improvement_info.technologies,&wet}){const auto row=rows->find(static_cast<std::int64_t>(id));if(row!=rows->end())tech+=get(row->second,y);need(tech>=INT32_MIN&&tech<=INT32_MAX,"Actual all-Tech expected native prefix overflow");}}
   for(const auto& [id,rows]:rules.policies){(void)id;const auto row=rows.find(farm->id);if(row!=rows.end())policies+=get(row->second,y);need(policies>=INT32_MIN&&policies<=INT32_MAX,"Actual all-Policy expected native prefix overflow");}
   const auto quote=bnw_improvement_yield(context,static_cast<std::int32_t>(i),farm->id,y,-1,false);need(quote.technologies==tech&&quote.policies==policies&&quote.era==0&&quote.player==0&&quote.traits==0&&quote.team==0&&quote.team_water==0&&quote.resource==0&&quote.fresh==0,"Neutral helper confused normal NO_PLAYER with optimal or owned scope");
   const auto expected=std::int64_t(calc(neutral_pillaged,static_cast<std::int32_t>(i),y))+quote.total();need(expected>=0&&expected<=INT32_MAX,"Neutral actual FARM expected outside unclamped native scope");neutral_values.at(static_cast<std::size_t>(y))=calc(unowned,static_cast<std::int32_t>(i),y);need(neutral_values.at(static_cast<std::size_t>(y))==expected,"Neutral normal did not reuse all-Tech/all-Policy helper");
   if(y==0)need(tech>0,"Actual FARM all-technology FOOD control is not meaningful");
  }
  // Unreachable owned matrices remain syntactically native, but must never
  // enter neutral prefix bounds or yields. No source owner is reassigned.
  auto ignored=unowned;for(auto& p_raw:ignored.world.all_players->players){auto& own=named(named(p_raw.administration.blocks,"pre_city").arrays,"m_ppaaiImprovementYieldChange").values;auto& trait=named(p_raw.traits.fields,"m_ppaaiImprovementYieldChange").values;for(std::int32_t y=0;y<6;++y){own.at(row_index+static_cast<std::size_t>(y))=INT32_MAX;trait.at(row_index+static_cast<std::size_t>(y))=INT32_MAX;}}
  for(std::int32_t y=0;y<6;++y)need(calc(ignored,static_cast<std::int32_t>(i),y)==neutral_values.at(static_cast<std::size_t>(y)),"Unreached owned cache bounded or contributed to neutral yield");
  // Landmark's positive per-era getter does not authorize a NO_PLAYER era.
  const auto* landmark=host.types().find("IMPROVEMENT_LANDMARK");need(landmark,"Actual neutral Landmark Info absent");auto landmark_case=unowned;paired_improvement(landmark_case,i,landmark->hash,false);const auto before=calc(landmark_case,static_cast<std::int32_t>(i),4);named(landmark_case.world.map_plots->plots.at(i).fields,"artifact_era").value=-2;landmark_case.world.map_projection->cells.at(i).artifact_era=-2;need(calc(landmark_case,static_cast<std::int32_t>(i),4)==before,"Neutral per-era incorrectly consumed artifact/team era");
  const auto* holy=host.types().find("IMPROVEMENT_HOLY_SITE");const auto* piety=host.types().find("POLICY_PIETY_FINISHER");need(holy&&piety,"Actual neutral culture policy control unavailable");auto holy_case=unowned;paired_improvement(holy_case,i,holy->hash,false);paired_policy(holy_case,host.types(),0,piety->id,false);const auto no_policy=calc(holy_case,static_cast<std::int32_t>(i),4);paired_policy(holy_case,host.types(),0,piety->id,true);need(calc(holy_case,static_cast<std::int32_t>(i),4)==no_policy,"Neutral owned-culture predicate substituted an active player");
  if(const auto* moai=host.types().find("IMPROVEMENT_MOAI")){auto adjacency=unowned;paired_improvement(adjacency,i,moai->hash,false);const auto& cell=adjacency.world.map_projection->cells.at(i);const MapGeometry geometry{map.grid,map.wrap_x,map.wrap_y,saves::GameStateLayout::expansion2_version1};std::optional<std::int64_t> selected;
   for(std::int32_t d=0;d<6&&!selected;++d)if(const auto n=geometry.direction(static_cast<std::int64_t>(cell.x),static_cast<std::int64_t>(cell.y),d))if(none(map.cells.at(static_cast<std::size_t>(*n)).improvement_type,map.cells.at(static_cast<std::size_t>(*n)).improvement_type_hash))selected=n;
   need(selected.has_value(),"Actual neutral Moai neighbor control unavailable");std::int32_t bonus=0;const auto query=host.database().query_rows({"select CultureAdjacentSameType from Improvements where Type = ?",{moai->type}},[&](const auto& row){const auto n=row.integer_at(0);need(n>0&&n<=INT32_MAX,"Actual Moai adjacency bonus unavailable");bonus=static_cast<std::int32_t>(n);});need(query.all_succeeded()&&bonus>0,"Actual neutral Moai source metadata absent");const auto without=calc(adjacency,static_cast<std::int32_t>(i),4);paired_improvement(adjacency,static_cast<std::size_t>(*selected),moai->hash,true);std::int32_t slots=0;for(std::int32_t d=0;d<6;++d)if(geometry.direction(static_cast<std::int64_t>(cell.x),static_cast<std::int64_t>(cell.y),d)==selected)++slots;const auto expected=std::int64_t(without)+std::int64_t(slots)*bonus;need(expected<=INT32_MAX&&calc(adjacency,static_cast<std::int32_t>(i),4)==expected,"Neutral CULTURE adjacency lost native slot multiplicity or filtered neighboring pillage");
  }else std::cout<<"Actual MOAI absent; neutral adjacency observation pending, no invented metadata\n";
  unowned_checked=true;break;
 }need(unowned_checked,"No actual bare neutral branch fixture; owner was never forged");
 need(!host.session().readiness().can_continue(),"Improvement math granted continuation");
 std::cout<<"PASS improvement math: all6 pillaged/owned/neutral; genuine scene status separate; detached neutral FARM adds all technology and generic policy math without grants\n";
}
template<class F>void exercise_resources(const ui::SelectedBnwPreviewHost& host,F&& calc,const BnwLuaCalculatedPlotYieldOwner& actual_normal){
 const auto original=host.session().state();const auto visibility=load_resource_visibility_rules(host.database(),host.types(),content::TechnologyProfile::bnw);
 need(original.world.map_plots->dimensions.teams==80&&original.world.teams->dimensions.teams==64,"Actual BNW force storage/active roster dimensions changed");
 const auto& real_force=named(original.world.map_plots->plots.at(593).arrays,"resource_force_reveal");
 need(real_force.columns==1&&real_force.values.size()==80,"Genuine 80-flag storage scene unavailable");
 const auto* horse=host.types().find("RESOURCE_HORSE");const auto* ivory=host.types().find("RESOURCE_IVORY");
 const auto* wheat=host.types().find("RESOURCE_WHEAT");const auto* husbandry=host.types().find("TECH_ANIMAL_HUSBANDRY");
 need(horse&&ivory&&wheat&&husbandry&&horse->table=="Resources"&&ivory->table=="Resources"&&wheat->table=="Resources"&&
  husbandry->table=="Technologies"&&original.world.map_projection->cells.at(593).resource_type_hash==horse->hash&&
  original.world.map_projection->cells.at(631).resource_type_hash==ivory->hash,"Actual HORSE593/IVORY631 Info identity changed");
 const auto horse_gate=visibility.resources.at(horse->id),ivory_gate=visibility.resources.at(ivory->id),wheat_gate=visibility.resources.at(wheat->id);
 need(horse_gate.technology==husbandry->id&&horse_gate.policy==-1&&ivory_gate.technology==-1&&ivory_gate.policy==-1&&
  wheat_gate.technology==-1&&wheat_gate.policy==-1,"Actual resource reveal metadata changed");
 const auto intrinsic=[&](const content::InfoTypeEntry& resource,const char* yield,std::int64_t expected){
  std::int64_t value=0;const auto rows=host.database().query_rows(
   {"SELECT Yield FROM Resource_YieldChanges WHERE ResourceType = ? AND YieldType = ?",{resource.type,std::string(yield)}},
   [&](const auto& row){value=row.integer("Yield");need(value>=INT32_MIN&&value<=INT32_MAX,"Actual resource intrinsic outside native width");});
  need(rows.all_succeeded()&&rows.rows==1&&value==expected,"Actual HORSE/IVORY intrinsic fixture metadata changed");
 };
 intrinsic(*horse,"YIELD_PRODUCTION",1);intrinsic(*ivory,"YIELD_GOLD",2);
 const auto owning_team=original.setup.players.at(0).team;
 const auto horse_tech=std::find_if(original.technologies.begin(),original.technologies.end(),[&](const auto& tech){return tech.team()==owning_team;});
 need(owning_team>=0&&owning_team<64&&horse_tech!=original.technologies.end(),"Actual HORSE team technology owner absent");
 auto witnessed_setup=original.setup;auto witnessed_visibility=restore_resource_visibility(original.world,witnessed_setup,visibility,host.types());
 ResourceVisibilityContext witness{*original.world.map_projection,witnessed_visibility,visibility,witnessed_setup,
  original.player_research,original.technologies,host.types(),host.defines(),false};
 need(!host.session().bnw_game_debug_mode()&&plot_resource_type(witness,593,owning_team)==-1&&
  !horse_tech->has_tech(horse_gate.technology)&&
  real_force.values.at(static_cast<std::size_t>(owning_team))==0&&
  !original.world.map_projection->cells.at(593).force_resource_reveal->contains(owning_team)&&
  witnessed_visibility.teams.at(owning_team).forced_resources.end()==std::find(
   witnessed_visibility.teams.at(owning_team).forced_resources.begin(),witnessed_visibility.teams.at(owning_team).forced_resources.end(),horse->id),
  "Actual hidden HORSE visibility facts changed");
 need(exact_normal_refusal([&]{(void)calc(original,593,0);})==
  "Normal plot yield hidden resource requires genuine debug owner join","Hidden HORSE detached refusal changed");
 auto no_horse=original;paired_resource(no_horse,593,0);
 for(std::int32_t y=0;y<6;++y){
  need(actual_normal.value(593,y,false)==calc(no_horse,593,y),"Actual false-debug hidden HORSE differs from NO_RESOURCE nature/suffix");
  need(actual_normal.value(631,y,false)==calc(original,631,y),"Actual visible IVORY differs from detached debug-invariant normal math");
 }
 std::cout<<"Actual HORSE593 hidden: owner team="<<owning_team<<" TechReveal="<<horse_gate.technology<<
  " HasTech=0 policy=-1 teamForce=0 plotForce=0; actual Game debug=false; genuine SAME VM normal all6 read\n";
 // Visible FOOD is a sanctioned paired detached WHEAT scenario, not the
 // unchanged HORSE/IVORY fixture and not a construction/debug/tech action.
 std::int64_t wheat_food=0;
 const auto food_rows=host.database().query_rows({"SELECT Yield FROM Resource_YieldChanges WHERE ResourceType = ? AND YieldType = 'YIELD_FOOD'",{wheat->type}},[&](const auto& row){
  const auto value=row.integer("Yield");need(value>0&&value<=INT32_MAX,"Actual WHEAT FOOD metadata not meaningful");wheat_food=value;
 });
 need(food_rows.all_succeeded()&&food_rows.rows==1&&wheat_food>0,"Actual WHEAT FOOD metadata absent");
 auto visible_reference=original;paired_resource(visible_reference,593,wheat->hash);
 need(calc(visible_reference,593,0)>0,"Detached actual WHEAT80/64 positive FOOD control unavailable");
 auto truncated=visible_reference;named(truncated.world.map_plots->plots.at(593).arrays,"resource_force_reveal").values.resize(64);auto& truncated_flags=*truncated.world.map_projection->cells.at(593).force_resource_reveal;for(auto it=truncated_flags.begin();it!=truncated_flags.end();)if(*it>=64)it=truncated_flags.erase(it);else ++it;
 bool shape_exact=false;try{(void)calc(truncated,593,0);}catch(const BnwNormalPlotYieldUnavailable& e){shape_exact=std::string_view(e.what())=="Normal plot yield current native resource force flags absent or wrong width";}need(shape_exact,"False storage64 passed or declined at unrelated source frontier");
 auto false_dimension=original;false_dimension.world.map_plots->dimensions.teams=64;refuses([&]{(void)calc(false_dimension,593,0);});
 auto false_roster=original;false_roster.world.teams->dimensions.teams=80;refuses([&]{(void)calc(false_roster,593,0);});
 auto unused_flag=original;named(unused_flag.world.map_plots->plots.at(593).arrays,"resource_force_reveal").values.at(79)=2;refuses([&]{(void)calc(unused_flag,593,0);}); // All80 raw storage flags are bool; slot79 is not an active team grant.
 for(const std::size_t plot:{std::size_t{593},std::size_t{631}}){const auto& cell=original.world.map_projection->cells.at(plot);need(cell.owner==0&&cell.resource_type_hash&&*cell.resource_type_hash&&none(cell.improvement_type,cell.improvement_type_hash),"Genuine selected resource plot changed");
  auto visible_original=original;paired_force(visible_original,plot,owning_team,true);
  for(std::int32_t y=0;y<6;++y)need(calc(visible_original,static_cast<std::int32_t>(plot),y)>=0,"Detached forced resource all6 did not reach source clamp");
  auto controlled=original;paired_resource(controlled,plot,wheat->hash);auto& player=controlled.world.all_players->players.at(0);named(player.core.fields,"m_iGoldenAgeTurns").values[0]=0;controlled.cities.players.at(0).golden_age=false;named(player.core.fields,"m_aiExtraYieldThreshold").values[0]=0;
  const auto with_resource=calc(controlled,static_cast<std::int32_t>(plot),0);paired_resource(controlled,plot,0);const auto without=calc(controlled,static_cast<std::int32_t>(plot),0);need(with_resource>without,"Detached real WHEAT had no meaningful FOOD contribution");
  std::cout<<"Detached actual WHEAT on original plot"<<plot<<" FOOD="<<with_resource<<" resource contribution="<<(with_resource-without)<<"\n";
 }
 // NO_TEAM in native nature is a skip, never a visibility grant. Pair the
 // genuine plot ownership fields in a detached variation; all six resource
 // results must equal the matching NO_RESOURCE branch for this scope.
 auto unowned=original;named(unowned.world.map_plots->plots.at(593).fields,"owner").value=-1;unowned.world.map_projection->cells.at(593).owner=-1;
 auto unowned_empty=unowned;paired_resource(unowned_empty,593,0);
 for(std::int32_t y=0;y<6;++y)need(calc(unowned,593,y)==calc(unowned_empty,593,y),"NO_TEAM incorrectly granted a resource nature/suffix term");
 auto force_mismatch=original;auto& projected=force_mismatch.world.map_projection->cells.at(593).force_resource_reveal;need(projected.has_value(),"Actual force flag projection absent");if(projected->contains(0))projected->erase(0);else projected->insert(0);refuses([&]{(void)calc(force_mismatch,593,0);});
 auto wide=original;named(wide.world.map_plots->plots.at(593).fields,"resource_type_hash").value=INT64_MAX;wide.world.map_projection->cells.at(593).resource_type_hash=INT64_MAX;refuses([&]{(void)calc(wide,593,0);});
 const auto* forest=host.types().find("FEATURE_FOREST");need(forest,"Actual forest Info absent");auto wrong=original;paired_resource(wrong,593,forest->hash);refuses([&]{(void)calc(wrong,593,0);});
 auto flag_width=original;named(flag_width.world.map_plots->plots.at(593).arrays,"resource_force_reveal").values.push_back(0);refuses([&]{(void)calc(flag_width,593,0);});
 const auto* actual_policy=host.types().find("POLICY_TRADITION");need(actual_policy&&actual_policy->table=="Policies","Actual native policy boolean fixture absent");
 for(const std::int64_t invalid:{std::int64_t{2},std::int64_t{255}}){auto malformed_policy=original;auto& rows=named(named(malformed_policy.world.all_players->players.at(0).strategy.blocks,"policies").hashes,"has_policy");rows.rows.push_back({actual_policy->hash,{invalid}});++rows.wire_count;
  bool exact=false;try{(void)calc(malformed_policy,593,0);}catch(const BnwNormalPlotYieldUnavailable& e){exact=std::string_view(e.what())=="Normal plot yield native policy boolean invalid";}need(exact,"Native policy byte accepted or rejected at an unrelated frontier");
 }
 // Actual strategic Info with a real reveal technology. Hypothetical paired
 // decoded variations retain the genuine world/FLTA/rules and execute no
 // construction, technology completion, policy effect or debug command.
 const auto* iron=host.types().find("RESOURCE_IRON");need(iron&&iron->table=="Resources","Actual IRON Info absent");const auto iron_visibility=visibility.resources.at(iron->id);need(iron_visibility.technology>=0&&iron_visibility.policy==-1,"Actual IRON reveal gates changed");
 auto strategic=original;paired_resource(strategic,593,iron->hash);const auto team=strategic.setup.players.at(0).team;paired_tech(strategic,host.types(),team,iron_visibility.technology,false);
 named(source_team(strategic,team).arrays,"revealed_resources").values.clear();named(source_team(strategic,team).fields,"revealed_resources_wire_count").value=0;
 named(strategic.world.map_plots->plots.at(593).arrays,"resource_force_reveal").values.at(static_cast<std::size_t>(team))=0;strategic.world.map_projection->cells.at(593).force_resource_reveal->erase(team);
 refuses([&]{(void)calc(strategic,593,0);}); // genuinely hidden: unknown debug owner
 auto stale_tech=strategic;owned_team(stale_tech,team).set_has_tech(iron_visibility.technology,true);refuses([&]{(void)calc(stale_tech,593,0);});
 auto& forced=named(source_team(strategic,team).arrays,"revealed_resources").values;forced.push_back(iron->hash);named(source_team(strategic,team).fields,"revealed_resources_wire_count").value=1;
 auto& owner=strategic.world.all_players->players.at(0);named(owner.core.fields,"m_iGoldenAgeTurns").values[0]=0;strategic.cities.players.at(0).golden_age=false;named(owner.core.fields,"m_aiExtraYieldThreshold").values[0]=0;
 const auto& cell=strategic.world.map_projection->cells.at(593);auto* city=const_cast<saves::SavedCityObservation*>(resolve_bnw_current_city(strategic,0,static_cast<std::int32_t>(cell.yield_city->id)));need(city,"Actual resource working city absent");
 auto& matrix=named(named(city->production.blocks,"before_strategy").children,"m_ppaiResourceYieldChange");saves::PlotArrayObservation* values=nullptr;
 for(auto& row:matrix.children)if(named(row.fields,"hash").value==iron->hash)values=&named(row.arrays,"values");need(values&&values->values.size()==6,"Actual city IRON resource matrix row absent");values->values[0]=5;
 named(owner.traits.fields,"m_iYieldChangeStrategicResources").values.at(0)=7;
 const auto forced_without_tech=calc(strategic,593,0);need(forced_without_tech>=0,"True team force reveal was not admitted");paired_tech(strategic,host.types(),team,iron_visibility.technology,true);
 need(calc(strategic,593,0)==forced_without_tech+12,"Forced visibility incorrectly bypassed city-extra/strategic reveal-tech gates");
 auto malformed_count=strategic;++named(source_team(malformed_count,team).fields,"revealed_resources_wire_count").value;refuses([&]{(void)calc(malformed_count,593,0);});
 auto trait_overflow=strategic;named(trait_overflow.world.all_players->players.at(0).traits.fields,"m_iYieldChangeStrategicResources").values.at(0)=INT32_MAX;refuses([&]{(void)calc(trait_overflow,593,0);});
 bool sea_checked=false;const auto* fish=host.types().find("RESOURCE_FISH");need(fish&&visibility.resources.at(fish->id).technology==-1&&visibility.resources.at(fish->id).policy==-1,"Actual FISH visibility gates changed");
 for(std::size_t plot=0;plot<original.world.map_projection->cells.size();++plot){const auto& water=original.world.map_projection->cells[plot];if(water.owner!=0||water.plot_type!=3||!water.yield_city||water.yield_city->owner!=0||!none(water.feature_type,water.feature_type_hash)||!none(water.improvement_type,water.improvement_type_hash))continue;
  auto sea=original;paired_resource(sea,plot,fish->hash);auto& sea_owner=sea.world.all_players->players.at(0);named(sea_owner.core.fields,"m_iGoldenAgeTurns").values[0]=0;sea.cities.players.at(0).golden_age=false;named(sea_owner.core.fields,"m_aiExtraYieldThreshold").values[0]=0;
  const auto before=calc(sea,static_cast<std::int32_t>(plot),0);auto* sea_city=const_cast<saves::SavedCityObservation*>(resolve_bnw_current_city(sea,0,static_cast<std::int32_t>(water.yield_city->id)));need(sea_city,"Actual water working city absent");named(sea_city->core.fields,"m_aiSeaResourceYield").values.at(0)+=4;
  need(calc(sea,static_cast<std::int32_t>(plot),0)==before+4,"Visible water resource skipped actual sea-resource city yield");sea_checked=true;break;
 }
 need(sea_checked,"Actual selected water city fixture absent");
 // Team policy can reveal a resource while the owning player still lacks
 // its separate SDK7435 policy gate. Derive BOTH policy mirrors from actual
 // raw rows first; then change paired low-level fields in a detached fixture.
 const content::InfoTypeEntry* policy_resource=nullptr;ResourceRevealInfo gates;
 for(const auto& [id,reveal]:visibility.resources)if(reveal.policy!=-1){policy_resource=&info_id(host.types(),"Resources",id);gates=reveal;break;}
 need(policy_resource,"Actual policy-reveal resource fixture absent");auto policy_case=original;paired_resource(policy_case,593,policy_resource->hash);
 (void)restore_resource_visibility(policy_case.world,policy_case.setup,visibility,host.types());paired_tech(policy_case,host.types(),team,gates.technology,true);
 auto& teammate=policy_case.world.all_players->players.at(1);named(teammate.core.fields,"m_bAlive").values[0]=1;policy_case.setup.players.at(1).alive=true;policy_case.world.pregame->archive.integer_arrays.at("team_types").at(1)=team;policy_case.setup.players.at(1).team=team;
 paired_policy(policy_case,host.types(),0,gates.policy,false);paired_policy(policy_case,host.types(),1,gates.policy,true);
 auto& policy_owner=policy_case.world.all_players->players.at(0);named(policy_owner.core.fields,"m_iGoldenAgeTurns").values[0]=0;policy_case.cities.players.at(0).golden_age=false;named(policy_owner.core.fields,"m_aiExtraYieldThreshold").values[0]=0;
 named(policy_owner.traits.fields,"m_iYieldChangeStrategicResources").values.at(0)=0;
 auto* policy_city=const_cast<saves::SavedCityObservation*>(resolve_bnw_current_city(policy_case,0,static_cast<std::int32_t>(cell.yield_city->id)));need(policy_city,"Actual policy-resource city absent");auto& policy_matrix=named(named(policy_city->production.blocks,"before_strategy").children,"m_ppaiResourceYieldChange");saves::PlotArrayObservation* policy_values=nullptr;
 for(auto& row:policy_matrix.children)if(named(row.fields,"hash").value==policy_resource->hash)policy_values=&named(row.arrays,"values");need(policy_values,"Actual policy-resource matrix row absent");policy_values->values.at(0)=5;
 const auto team_policy_only=calc(policy_case,593,0);paired_policy(policy_case,host.types(),0,gates.policy,true);need(calc(policy_case,593,0)==team_policy_only+5,"Team policy visibility incorrectly granted owning-player resource-extra policy");
 auto raw_policy_mismatch=policy_case;raw_policy_mismatch.setup.players.at(0).policies.erase(gates.policy);refuses([&]{(void)calc(raw_policy_mismatch,593,0);});
 std::cout<<"PASS source-bound resources: original hidden HORSE / visible IVORY actual normal all6, detached WHEAT FOOD, current HasTech/policy/force mirrors, distinct suffix gates; no debug/freshness grant\n";
}
void exercise(const std::shared_ptr<ui::SelectedBnwPreviewHost>& selected){
 const auto& host=*selected;
 auto actual_session=std::shared_ptr<const LiveSession>(selected,&selected->session());
 auto actual_rules=std::make_shared<const BnwNormalPlotYieldRules>(BnwNormalPlotYieldRules::load(host.database(),host.identity().rules_sha256));
 auto actual_normal=make_bnw_lua_calculated_plot_yield_owner(actual_session,actual_rules);
 // Exactly one genuine VM incarnation, retained throughout the test; no
 // provider replacement, debug toggle or detached-to-live authority join.
 LuaScriptHost actual_vm(ScriptBudget{},ScriptSourceProvider{},ScriptModdingProvider{},ScriptGameplayProvider{},
  ScriptSaveDataProvider{},ScriptPregameProvider{},bnw_lua_calculated_plot_services(actual_normal));
 auto state=host.session().state();const auto identity=host.identity();const auto rules=BnwNormalPlotYieldRules::load(host.database(),identity.rules_sha256);
 const auto calc=[&](const LiveSessionState& s,std::int32_t plot,std::int32_t y){return calculate_bnw_normal_plot_yield(s,host.session().source(),identity,rules,plot,y);};
 exercise_improvements(host,calc);
 exercise_improvement_culture(host,calc);
 exercise_resources(host,calc,*actual_normal);
 // Genuine decoded home632 with its original road0; no branch stripping.
 const auto& home_road=state.world.map_projection->cells.at(632);
 need(home_road.owner&&*home_road.owner==0&&home_road.plot_city&&home_road.plot_city->owner==0&&home_road.route&&home_road.route->type==0&&!home_road.route->pillaged,"Actual selected home632/road0 identity changed");
 std::array<std::int32_t,6> original_home_yields{};
 for(std::int32_t y=0;y<6;++y){original_home_yields.at(static_cast<std::size_t>(y))=calc(state,632,y);need(original_home_yields.at(static_cast<std::size_t>(y))>=0,"Actual road home did not reach source clamp");}
 need(original_home_yields[0]>0,"Actual original road home FOOD branch not meaningful");
 std::cout<<"Genuine original home632 road0 FOOD="<<original_home_yields[0]<<" PRODUCTION="<<original_home_yields[1]<<"\n";
 // Genuine ordinary forest673 is not replaced by a bare plot or proposed
 // improvement. Its working city uses the exact current FLTA registry.
 const auto* forest_info=host.types().find("FEATURE_FOREST");
 const auto& forest=state.world.map_projection->cells.at(673);
 need(forest_info&&forest_info->table=="Features"&&forest.feature_type_hash==forest_info->hash&&forest.yield_city&&forest.owner&&*forest.owner==0&&none(forest.resource_type,forest.resource_type_hash)&&none(forest.improvement_type,forest.improvement_type_hash),"Actual selected forest673 identity changed");
 std::array<std::int32_t,6> forest_yields{};
 for(std::int32_t y=0;y<6;++y){forest_yields.at(static_cast<std::size_t>(y))=calc(state,673,y);need(forest_yields.at(static_cast<std::size_t>(y))>=0,"Genuine forest did not reach source clamp");}
 need(forest_yields[0]>0&&forest_yields[1]>0,"Genuine forest nature branch was not meaningful");
 std::cout<<"Genuine forest673 FOOD="<<forest_yields[0]<<" PRODUCTION="<<forest_yields[1]<<"\n";
 need(!none(state.world.map_projection->cells.at(593).resource_type,state.world.map_projection->cells.at(593).resource_type_hash),"Actual selected resource frontier identity changed");
 need(exact_normal_refusal([&]{(void)calc(state,593,0);})=="Normal plot yield hidden resource requires genuine debug owner join",
  "Second original hidden-resource detached witness changed");
 for(std::int32_t y=0;y<6;++y)need(actual_normal->value(593,y,false)>=0,"Actual Game normal hidden-resource query did not reach clamp");
 auto stale_feature=state;stale_feature.world.map_projection->cells.at(673).feature_type_hash=0;
 refuses([&]{(void)calc(stale_feature,673,0);});
 const auto set_feature_hash=[&](LiveSessionState& changed,std::uint64_t hash){
  need(hash<=INT64_MAX,"Fixture wire value does not fit observation storage");named(changed.world.map_plots->plots.at(673).fields,"feature_type_hash").value=static_cast<std::int64_t>(hash);changed.world.map_projection->cells.at(673).feature_type_hash=hash;
 };
 auto wide_feature=state;set_feature_hash(wide_feature,INT64_MAX);refuses([&]{(void)calc(wide_feature,673,0);});
 const auto* grass=host.types().find("TERRAIN_GRASS");need(grass&&grass->table=="Terrains","Actual terrain wire identity absent");
 auto wrong_feature_table=state;set_feature_hash(wrong_feature_table,grass->hash);refuses([&]{(void)calc(wrong_feature_table,673,0);});
 auto unknown_feature=state;std::uint32_t unused_hash=UINT32_MAX;while(host.types().find_hash(unused_hash)&&unused_hash>0)--unused_hash;need(unused_hash>0,"No unused native feature hash fixture");set_feature_hash(unknown_feature,unused_hash);refuses([&]{(void)calc(unknown_feature,673,0);});
 // Use actual selected GEYSER Info, not an arbitrary feature ID or proposed
 // rules. FOOD is zero in its native base/river/hills and all belief terms;
 // current working-city selection still executes on this original plot.
 const auto* wonder_info=host.types().find("FEATURE_GEYSER");need(wonder_info&&wonder_info->table=="Features"&&wonder_info->id>=0,"Actual GEYSER Info fixture absent");
 const auto wonder_rows=host.database().query_rows({"select NaturalWonder, YieldNotAdditive, Impassable from Features where Type = ?",{wonder_info->type}},[&](const auto& row){need(row.integer_at(0)==1&&row.integer_at(1)==1&&row.integer_at(2)==1,"Actual GEYSER native flags changed");});
 need(wonder_rows.all_succeeded()&&wonder_rows.rows==1,"Actual GEYSER metadata absent");
 const auto food_rows=host.database().query_rows({
  "select Yield from Feature_YieldChanges where FeatureType = ? and YieldType = 'YIELD_FOOD' "
  "union all select Yield from Feature_RiverYieldChanges where FeatureType = ? and YieldType = 'YIELD_FOOD' "
  "union all select Yield from Feature_HillsYieldChanges where FeatureType = ? and YieldType = 'YIELD_FOOD' "
  "union all select Yield from Belief_FeatureYieldChanges where FeatureType = ? and YieldType = 'YIELD_FOOD' "
  "union all select Yield from Belief_YieldChangeNaturalWonder where YieldType = 'YIELD_FOOD' "
  "union all select Yield from Belief_YieldModifierNaturalWonder where YieldType = 'YIELD_FOOD'",
  {wonder_info->type,wonder_info->type,wonder_info->type,wonder_info->type}},[&](const auto& row){need(row.integer_at(0)==0,"Actual GEYSER/belief FOOD fixture changed");});
 need(food_rows.all_succeeded(),"Actual GEYSER/belief FOOD query failed");
 auto wonder=state;set_feature_hash(wonder,wonder_info->hash);
 // Changing a feature cannot claim that the old impassability cache is live.
 refuses([&]{(void)calc(wonder,673,0);});rebuild_bnw_live_plot_impassability(wonder,host.database(),host.types(),identity);
 need(bnw_live_plot_impassability(wonder).plots.at(673).impassable,"Actual GEYSER impassability not restored");
 for(std::int32_t y=0;y<6;++y)need(calc(wonder,673,y)>=0,"Actual natural-wonder all-six query did not reach native clamp");
 auto& wonder_owner=wonder.world.all_players->players.at(0);
 named(wonder_owner.core.fields,"m_iGoldenAgeTurns").values[0]=0;wonder.cities.players.at(0).golden_age=false;named(wonder_owner.core.fields,"m_aiExtraYieldThreshold").values[0]=0;
 auto& wonder_trait=named(wonder_owner.traits.fields,"m_ppaaiUnimprovedFeatureYieldChange");need(wonder_trait.columns==6,"Actual natural-wonder trait matrix width changed");wonder_trait.values.at(static_cast<std::size_t>(wonder_info->id)*6)=0;
 auto& wonder_food=named(wonder_owner.traits.fields,"m_iYieldChangeNaturalWonder").values;need(wonder_food.size()==6,"Actual natural-wonder trait vector width changed");
 auto& wonder_modifier=named(wonder_owner.traits.fields,"m_iNaturalWonderYieldModifier").values;need(wonder_modifier.size()==1,"Actual natural-wonder trait modifier shape changed");
 wonder_food[0]=0;wonder_modifier[0]=0;need(calc(wonder,673,0)==0,"Selected natural-wonder FOOD fixture retained an uncontrolled suffix");
 // Reuse a real saved league/member, never fabricate a League or append
 // records. CvGameLeagues visits IsMember on every non-CULTURE query; only
 // resolution arithmetic is gated. The detached mutation tests that exact
 // admission boundary without granting live world/cache readiness.
 std::size_t member_league=wonder.world.leagues.leagues.size();
 for(std::size_t i=0;i<wonder.world.leagues.leagues.size();++i)if(!wonder.world.leagues.leagues.at(i).members.empty()){member_league=i;break;}
 if(member_league<wonder.world.leagues.leagues.size()){
  need(forest.yield_city->owner>=0&&forest.yield_city->owner<64&&forest.yield_city->id>=INT32_MIN&&forest.yield_city->id<=INT32_MAX,"Actual natural-wonder current-city native reference differs");
  const auto* member_city=resolve_bnw_current_city(wonder,static_cast<std::int32_t>(forest.yield_city->owner),static_cast<std::int32_t>(forest.yield_city->id));need(member_city,"Actual natural-wonder member working city absent");
  const auto member_owner=named(member_city->core.fields,"m_eOwner").values.at(0);need(member_owner>=0&&member_owner<64,"Actual natural-wonder working-city owner outside native registry");
  const std::array<std::int32_t,5> nonculture{0,1,2,3,5};std::array<std::int32_t,5> nonculture_baseline{};
  for(std::size_t i=0;i<nonculture.size();++i)nonculture_baseline.at(i)=calc(wonder,673,nonculture.at(i));
  auto first_member=wonder;first_member.world.leagues.leagues.at(member_league).members.at(0).player=member_owner;
  for(std::size_t i=0;i<nonculture.size();++i)need(calc(first_member,673,nonculture.at(i))==nonculture_baseline.at(i),"Non-CULTURE member match read culture-only resolution values");
  auto wide_member=first_member;wide_member.world.leagues.leagues.at(member_league).members.at(0).player=INT64_MAX;
  for(const auto y:nonculture)need(exact_normal_refusal([&]{(void)calc(wide_member,673,y);})=="Normal plot yield native intermediate overflow","Non-CULTURE query bypassed the visited native PlayerTypes width");
  if(first_member.world.leagues.leagues.at(member_league).members.size()>1){
   first_member.world.leagues.leagues.at(member_league).members.at(1).player=INT64_MAX;
   for(std::size_t i=0;i<nonculture.size();++i)need(calc(first_member,673,nonculture.at(i))==nonculture_baseline.at(i),"Original IsMember first match read a later unvisited malformed member");
  }else std::cout<<"Natural-wonder later-member short-circuit witness pending: selected saved league has one member\n";
  std::cout<<"PASS natural-wonder reached member native width on all five non-CULTURE yields; saved member inventory only\n";
 }else std::cout<<"Natural-wonder non-CULTURE member witness pending: no selected saved league/member\n";
 // Original int32 multiply then divide golden cases: 3*150/100=4 and
 // 3*199/100=5; nonpositive modifiers skip both operations. 14316557*150
 // fits int32, while the next numerator overflows before a fitting quotient.
 for(const auto& golden:{std::array<std::int64_t,3>{3,0,3},std::array<std::int64_t,3>{3,-1,3},std::array<std::int64_t,3>{3,50,4},std::array<std::int64_t,3>{3,99,5},std::array<std::int64_t,3>{3,100,6},std::array<std::int64_t,3>{14316557,50,21474835},std::array<std::int64_t,3>{0,INT32_MAX-100,0}}){
  wonder_food[0]=golden[0];wonder_modifier[0]=golden[1];need(calc(wonder,673,0)==golden[2],"Natural-wonder native golden arithmetic changed");
 }
 wonder_food[0]=14316558;wonder_modifier[0]=50;
 need(exact_normal_refusal([&]{(void)calc(wonder,673,0);})=="Normal plot yield native intermediate overflow","Natural-wonder divided away an overflowing native product");
 wonder_food[0]=0;wonder_modifier[0]=INT32_MAX-99;
 need(exact_normal_refusal([&]{(void)calc(wonder,673,0);})=="Normal plot yield native intermediate overflow","Natural-wonder skipped an overflowing native 100+modifier");
 wonder_food[0]=3;wonder_modifier[0]=50;
 auto mountain_wonder=wonder;named(mountain_wonder.world.map_plots->plots.at(673).fields,"plot_type").value=0;mountain_wonder.world.map_projection->cells.at(673).plot_type=0;
 need(calc(mountain_wonder,673,0)==4,"Native mountain exception suppressed the natural-wonder yield");
 std::cout<<"PASS natural wonders: current selected Info/cache/owner, original int32 multiplier/division golden boundary, all-six and mountain exception; no turn/cache readiness grant\n";
 const auto* working_raw=resolve_bnw_current_city(state,static_cast<std::int32_t>(forest.yield_city->owner),static_cast<std::int32_t>(forest.yield_city->id));need(working_raw,"Genuine forest working-city registry absent");
 const auto full_id=named(working_raw->core.fields,"m_iID").values.at(0);need(full_id==8192,"Actual selected forest full working-city identity changed");
 for(const auto handle:{full_id,full_id&0x1fff}){auto alias=state;named(alias.world.map_plots->plots.at(673).fields,"working_city_id").value=handle;alias.world.map_projection->cells.at(673).yield_city->id=handle;for(std::int32_t y=0;y<6;++y)need(calc(alias,673,y)==forest_yields.at(static_cast<std::size_t>(y)),"Forest working-city full/index registry disagreed");}
 auto current_forest=state;auto& forest_owner=current_forest.world.all_players->players.at(0);named(forest_owner.core.fields,"m_iGoldenAgeTurns").values[0]=0;current_forest.cities.players.at(0).golden_age=false;named(forest_owner.core.fields,"m_aiExtraYieldThreshold").values[0]=0;
 auto& trait=named(forest_owner.traits.fields,"m_ppaaiUnimprovedFeatureYieldChange");need(trait.columns==6&&forest_info->id>=0,"Actual forest trait matrix width/slot differs");
 const auto trait_index=static_cast<std::size_t>(forest_info->id)*6;auto& trait_food=trait.values.at(trait_index);const auto prior_trait=trait_food;
 const auto forest_baseline=calc(current_forest,673,0);trait_food+=3;need(calc(current_forest,673,0)==forest_baseline+3,"Normal forest ignored current raw unimproved-feature trait");trait_food=prior_trait;
 auto* current_city=const_cast<saves::SavedCityObservation*>(resolve_bnw_current_city(current_forest,0,static_cast<std::int32_t>(forest.yield_city->id)));need(current_city,"Current forest city disappeared");
 auto& feature_matrix=named(named(current_city->production.blocks,"before_strategy").children,"m_ppaiFeatureYieldChange");
 saves::PlotArrayObservation* feature_values=nullptr;
 for(auto& row:feature_matrix.children){const auto hash=named(row.fields,"hash").value;if(hash==forest_info->hash)feature_values=&named(row.arrays,"values");}
 need(feature_values&&feature_values->columns==6&&feature_values->values.size()==6,"Actual decoded forest city feature matrix row absent");
 const auto prior_extra=feature_values->values.at(0);feature_values->values.at(0)+=5;need(calc(current_forest,673,0)==forest_baseline+5,"Normal forest ignored current raw city feature-extra matrix");feature_values->values.at(0)=prior_extra;
 // Later city-extra cancellation cannot justify overflowing the earlier
 // native nature feature accumulator (SDK6944 precedes SDK7415).
 trait_food=INT32_MAX;feature_values->values.at(0)=-INT32_MAX;refuses([&]{(void)calc(current_forest,673,0);});trait_food=prior_trait;feature_values->values.at(0)=prior_extra;
 auto feature_duplicate=current_forest;auto* duplicate_city=const_cast<saves::SavedCityObservation*>(resolve_bnw_current_city(feature_duplicate,0,static_cast<std::int32_t>(forest.yield_city->id)));auto& duplicate_matrix=named(named(duplicate_city->production.blocks,"before_strategy").children,"m_ppaiFeatureYieldChange");
 auto duplicate_row=duplicate_matrix.children.front();bool copied_forest=false;
 for(const auto& row:duplicate_matrix.children)if(named(row.fields,"hash").value==forest_info->hash){duplicate_row=row;copied_forest=true;}
 need(copied_forest,"Actual duplicate-row source absent");named(duplicate_row.arrays,"values").values.at(0)=prior_extra+4;duplicate_matrix.children.push_back(duplicate_row);++named(duplicate_matrix.fields,"wire_count").value;
 need(calc(feature_duplicate,673,0)==forest_baseline+4,"Native city feature hash remapping did not use final assignment");
 auto empty_features=current_forest;auto* empty_city=const_cast<saves::SavedCityObservation*>(resolve_bnw_current_city(empty_features,0,static_cast<std::int32_t>(forest.yield_city->id)));auto& empty_matrix=named(named(empty_city->production.blocks,"before_strategy").children,"m_ppaiFeatureYieldChange");empty_matrix.children.clear();named(empty_matrix.fields,"wire_count").value=0;
 need(calc(empty_features,673,0)==std::max<std::int32_t>(0,forest_baseline-static_cast<std::int32_t>(prior_extra)),"Native city feature matrix zero initialization differs");
 auto matrix_count=state;auto* bad_city=const_cast<saves::SavedCityObservation*>(resolve_bnw_current_city(matrix_count,0,static_cast<std::int32_t>(forest.yield_city->id)));auto& bad_matrix=named(named(bad_city->production.blocks,"before_strategy").children,"m_ppaiFeatureYieldChange");++named(bad_matrix.fields,"wire_count").value;refuses([&]{(void)calc(matrix_count,673,0);});
 auto stale_route=state;stale_route.world.map_projection->cells.at(632).route->pillaged=true;
 refuses([&]{(void)calc(stale_route,632,0);});
 auto malformed_flag=state;named(malformed_flag.world.map_plots->plots.at(632).fields,"route_pillaged").value=2;malformed_flag.world.map_projection->cells.at(632).route->pillaged=true;
 refuses([&]{(void)calc(malformed_flag,632,0);});
 auto bad_route=state;named(bad_route.world.map_plots->plots.at(632).fields,"route_type").value=INT64_MAX;bad_route.world.map_projection->cells.at(632).route->type=INT64_MAX;
 refuses([&]{(void)calc(bad_route,632,0);});
 auto unknown_route=state;named(unknown_route.world.map_plots->plots.at(632).fields,"route_type").value=INT32_MAX;unknown_route.world.map_projection->cells.at(632).route->type=INT32_MAX;
 refuses([&]{(void)calc(unknown_route,632,0);});
 // Source never fetches a RouteInfo on the pillaged branch. This controlled
 // detached variation authenticates BOTH current fields and tests that order.
 named(unknown_route.world.map_plots->plots.at(632).fields,"route_pillaged").value=1;unknown_route.world.map_projection->cells.at(632).route->pillaged=true;
 for(std::int32_t y=0;y<6;++y)need(calc(unknown_route,632,y)>=0,"Actual pillage prefix fetched an unreachable RouteInfo");
  // Preserve the independent original bare-branch scenario. The detached,
  // paired raw/projected route variation reaches
 // the home-city formula without replacing its actual FLTA, city/player,
 // rules, source or religion operands. It is not a command or fresh-yield proof.
 std::size_t varied_home_routes=0;
 for(std::size_t i=0;i<state.world.map_projection->cells.size();++i){
  auto& cell=state.world.map_projection->cells[i];
  if(!cell.owner||*cell.owner<0||!cell.plot_city||cell.plot_city->owner<0||
     !none(cell.feature_type,cell.feature_type_hash)||!none(cell.resource_type,cell.resource_type_hash)||
     !none(cell.improvement_type,cell.improvement_type_hash)||!cell.route)continue;
  auto& route=named(state.world.map_plots->plots[i].fields,"route_type").value;
  need(route==cell.route->type,"Actual home raw/projected route differs");
  if(route!=-1){route=-1;cell.route->type=-1;++varied_home_routes;}
 }
 need(varied_home_routes>0,"Selected source did not supply the expected road-bearing home variation");
 std::cout<<"Detached actual home NO_ROUTE variations="<<varied_home_routes<<"\n";
 std::size_t land=0,water=0,homes=0,owned=0;std::optional<std::int32_t> first;std::string first_error;
 for(std::size_t i=0;i<state.world.map_projection->cells.size();++i){const auto& c=state.world.map_projection->cells[i];if(!c.owner||*c.owner<0||!c.potential_city_work||!*c.potential_city_work||!c.terrain_type||*c.terrain_type<0||!none(c.feature_type,c.feature_type_hash)||!none(c.resource_type,c.resource_type_hash)||!none(c.improvement_type,c.improvement_type_hash)||!c.route||c.route->type!=-1)continue;
  const auto index=static_cast<std::int32_t>(i);std::int32_t food=0;
  try{food=calc(state,index,0);}catch(const BnwNormalPlotYieldUnavailable& e){if(first_error.empty())first_error=e.what();continue;}
  need(food>=0,"Normal native yield below final source clamp");if(!first)first=index;++owned;
  if(c.plot_type==3)++water;else ++land;
  if(c.plot_city&&c.plot_city->owner>=0)++homes;
  for(std::int32_t y=1;y<6;++y)need(calc(state,index,y)>=0,"All-six bare-plot branch did not clamp");
  // A saved cache is not the producer's answer: this keeps raw and owned
  // caches mutually inconsistent on purpose, with all reached inputs intact.
  auto cache_changed=state;cache_changed.world.map_projection->cells[i].cached_yields->at(0)=food+999;
  named(cache_changed.world.map_plots->plots[i].arrays,"yields").values[0]=-123;
  need(calc(cache_changed,index,0)==food,"Normal calculator read saved cache as its fresh answer");
  if(owned>=8&&land&&water&&homes)break;
 }
 need(first&&land&&water&&homes,("Actual selected fixture lacks supported owned land/water/home branches; first refusal: "+first_error).c_str());
 auto stale=state;stale.world.map_projection->cells[static_cast<std::size_t>(*first)].owner=63;refuses([&]{(void)calc(stale,*first,0);});
 refuses([&]{(void)calc(state,-1,0);});refuses([&]{(void)calc(state,*first,6);});
 auto wide=state;wide.world.map_header.grid.width=UINT64_MAX;wide.world.map_projection->grid.width=UINT64_MAX;refuses([&]{(void)calc(wide,*first,0);});
 auto wrong=identity;wrong.rules_sha256[0]=wrong.rules_sha256[0]=='0'?'1':'0';auto empty=state;empty.world.map_projection.reset();refuses([&]{(void)calculate_bnw_normal_plot_yield(empty,host.session().source(),wrong,rules,*first,0);});
 bool foreign_refused=false;std::thread worker([&]{try{(void)calc(state,*first,0);}catch(const BnwNormalPlotYieldUnavailable&){foreign_refused=true;}});worker.join();need(foreign_refused,"Foreign-thread read reached actual world");need(calc(state,*first,0)>=0,"Rejected foreign query poisoned original immutable rules");
 // Use the genuine home cell for a changed source player centiyield.
 bool current_mutation_checked=false;
 for(std::size_t i=0;i<state.world.map_projection->cells.size();++i){const auto& c=state.world.map_projection->cells[i];if(!c.owner||*c.owner<0||!c.plot_city||c.plot_city->owner<0||!none(c.feature_type,c.feature_type_hash)||!none(c.resource_type,c.resource_type_hash)||!none(c.improvement_type,c.improvement_type_hash)||!c.route||c.route->type!=-1)continue;
  const auto& source_player=state.world.all_players->players.at(static_cast<std::size_t>(*c.owner));const auto cap=named(source_player.core.fields,"m_iCapitalCityID").values[0];if(cap!=c.plot_city->id)continue;
  const auto index=static_cast<std::int32_t>(i);try{(void)calc(state,index,0);}catch(const BnwNormalPlotYieldUnavailable&){continue;}
  auto owner_mismatch=state;const auto other_owner=(*c.owner+1)%64;owner_mismatch.world.map_projection->cells[i].owner=other_owner;named(owner_mismatch.world.map_plots->plots[i].fields,"owner").value=other_owner;refuses([&]{(void)calc(owner_mismatch,index,0);});
  auto changed=state;auto& raw=changed.world.all_players->players.at(static_cast<std::size_t>(*c.owner));named(raw.core.fields,"m_iGoldenAgeTurns").values[0]=0;changed.cities.players.at(*c.owner).golden_age=false;named(raw.core.fields,"m_aiExtraYieldThreshold").values[0]=0;
  const auto baseline=calc(changed,index,0);auto& amount=named(raw.core.fields,"m_aiCityYieldChange").values[0];const auto prior=amount;amount+=700;need(calc(changed,index,0)==baseline+7,"Normal calculation used initial SavedSession player counters instead of current raw world");amount=INT32_MAX;
  auto& capital=named(raw.core.fields,"m_aiCapitalYieldChange").values[0];capital=100;refuses([&]{(void)calc(changed,index,0);});amount=prior;
  current_mutation_checked=true;break;
 }
 need(current_mutation_checked,"Actual selected fixture did not exercise current capital player counters");
 bool terrain_extra_checked=false;
 for(std::size_t i=0;i<state.world.map_projection->cells.size();++i){const auto& c=state.world.map_projection->cells[i];if(!c.owner||*c.owner<0||!c.yield_city||c.yield_city->owner<0||!c.plot_city||c.plot_city->owner!=-1||!c.terrain_type||!c.plot_type||*c.plot_type==0||!none(c.feature_type,c.feature_type_hash)||!none(c.resource_type,c.resource_type_hash)||!none(c.improvement_type,c.improvement_type_hash)||!c.route||c.route->type!=-1)continue;
  const auto index=static_cast<std::int32_t>(i);try{(void)calc(state,index,0);}catch(const BnwNormalPlotYieldUnavailable&){continue;}
  auto changed=state;auto& p=changed.world.all_players->players.at(static_cast<std::size_t>(*c.owner));named(p.core.fields,"m_iGoldenAgeTurns").values[0]=0;changed.cities.players.at(*c.owner).golden_age=false;named(p.core.fields,"m_aiExtraYieldThreshold").values[0]=0;
  const auto baseline=calc(changed,index,0);auto* raw=const_cast<saves::SavedCityObservation*>(resolve_bnw_current_city(changed,static_cast<std::int32_t>(c.yield_city->owner),static_cast<std::int32_t>(c.yield_city->id)));need(raw,"Actual working city disappeared");auto& matrix=named(named(raw->production.blocks,"before_strategy").children,"m_ppaiTerrainYieldChange");
  saves::PlotArrayObservation* values=nullptr;
  for(auto& row:matrix.children){const auto hash=named(row.fields,"hash").value;const auto* t=hash?host.types().find_hash(static_cast<std::uint64_t>(hash)):nullptr;if(t&&t->table=="Terrains"&&t->id==*c.terrain_type)values=&named(row.arrays,"values");}
  if(!values)continue;values->values.at(0)+=5;need(calc(changed,index,0)==baseline+5,"Normal calculation did not consume actual current city terrain-extra matrix");terrain_extra_checked=true;break;
 }
 need(terrain_extra_checked,"Actual selected fixture did not exercise decoded city terrain-extra matrix");
 need(!host.session().readiness().can_continue(),"Numeric plot getter authorized full game continuation");
 std::cout<<"PASS actual owned bare BNW plot producer: land="<<land<<" water="<<water<<" home="<<homes<<"; saved cache ignored; current raw mutations and native bounds checked; no freshness authority\n";
}
}
int main(int argc,char** argv){try{need(argc==3,"Supply actual selected BNW save and shared installation view");const auto before=identify_file(argv[1]).sha256;auto bootstrap=ui::bootstrap_selected_bnw_preview(argv[1],argv[2],0,true);if(bootstrap.preview.blocker)throw std::runtime_error(bootstrap.preview.blocker->reason);need(static_cast<bool>(bootstrap.preview.host),"Actual selected immutable host absent");exercise(std::shared_ptr<ui::SelectedBnwPreviewHost>(std::move(bootstrap.preview.host)));need(identify_file(argv[1]).sha256==before,"Original selected save changed");return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
