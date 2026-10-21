#include "civ5/runtime/bnw_religion_happiness_terms.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <array>
#include <cmath>
#include <limits>
#include <set>
#include <string_view>
#include <type_traits>
namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
constexpr std::size_t source_players=64,civ_players=63;
void need(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
std::int32_t integer(std::int64_t n){need(n>=INT32_MIN&&n<=INT32_MAX,"BNW religious happiness source int overflow");return static_cast<std::int32_t>(n);}
std::int32_t add(std::int32_t a,std::int32_t b){return integer(static_cast<std::int64_t>(a)+b);}
const auto &named(const auto &values,std::string_view name){
 const typename std::remove_reference_t<decltype(values)>::value_type *found=nullptr;
 for(const auto &v:values)if(v.name&&name==v.name){need(!found,"BNW religious happiness duplicate source field");found=&v;}
 need(found,"BNW religious happiness required source field is absent");return *found;
}
std::int32_t core(const auto &fields,std::string_view name){const auto &v=named(fields,name).values;need(v.size()==1,"BNW religious happiness malformed core scalar");return integer(v[0]);}
std::int32_t node(const auto &nodes,std::string_view name){const auto &v=named(nodes,name).values;need(v.size()==1,"BNW religious happiness malformed religion scalar");return integer(v[0]);}
std::int32_t majority(const std::vector<CityYieldReligion> &status){
 std::int32_t total=0,pressure=0,most=-1,religion=-1;
 for(const auto &r:status){const auto followers=integer(r.followers),p=integer(r.pressure);total=add(total,followers);if(followers>most||(followers==most&&p>pressure)){most=followers;pressure=p;religion=integer(r.religion);}}
 return integer(static_cast<std::int64_t>(most)*2)>=total?religion:-1;
}
std::int32_t followers(const std::vector<CityYieldReligion> &status,std::int32_t religion){for(const auto &r:status)if(r.religion==religion)return integer(r.followers);return 0;}
void reconciled_beliefs(const LiveSessionState &state,const BnwReligionHappinessRules &rules){
 need(state.cities.profile==content::TechnologyProfile::bnw&&state.cities.religions.size()==state.world.religion_culture.religions.size(),"BNW religion registry/runtime binding differs");
 for(std::size_t i=0;i<state.cities.religions.size();++i){const auto &raw=state.world.religion_culture.religions[i];const auto &live=state.cities.religions[i];std::vector<std::int64_t> expected;for(const auto id:rules.resolved_beliefs(raw))if(id>=0&&static_cast<std::uint64_t>(id)<rules.belief_slots())expected.push_back(id);need(raw.religion==live.religion&&raw.founder==live.founder&&live.beliefs==expected,"BNW founder beliefs raw/runtime hash binding differs");}
}
std::int32_t repeat_founder(BnwTeamTechLuaEpoch &epoch,std::int64_t player){
 need(epoch.world&&epoch.host&&epoch.getter_world==epoch.world.get()&&epoch.all_selected_contexts_loaded&&epoch.all_listeners_registered&&epoch.all_external_writes_escrowed&&epoch.host->loading_complete()&&epoch.host->source_errors().empty(),"BNW foreign followers requires the same verified active Lua epoch");
 const std::array<ScriptValue,1> args{player};const auto accumulated=epoch.host->accumulate_int("GetFounderBenefitsReligion",args,0);
 if(!accumulated.errors.empty())throw std::runtime_error("BNW nested founder callback failed: "+accumulated.errors.front());
 if(accumulated.value)return integer(*accumulated.value);
 const auto model=BnwFounderReligionReadModel::bind(*epoch.world);const auto created=model.religion_created_by_player(player);return model.eligible_for_founder_benefits(created,player)?created:-1;
}
}
BnwReligionHappinessRules BnwReligionHappinessRules::load(const content::RulesDatabase &database){
 BnwReligionHappinessRules rules;rules.rules_sha256_=identify_bytes(database.gameplay_fingerprint_data()).sha256;rules.types_=database.cache_info_types();need(rules.types_.report().all_succeeded(),"BNW belief InfoType catalog is incomplete");
 const auto report=database.prefetch_info("Beliefs",[&](auto slot,auto,const auto &row){Entry e;e.player_happiness=integer(row.integer("PlayerHappiness"));e.divisor=integer(row.integer("HappinessPerXPeacefulForeignFollowers"));e.following=static_cast<float>(row.real("HappinessPerFollowingCity"));need(std::isfinite(e.following),"BNW belief following-city float is nonfinite");e.requires_peace=row.boolean("RequiresPeace");need(slot<=INT32_MAX&&rules.entries_.emplace(static_cast<std::int64_t>(slot),e).second,"BNW belief collection slot is unavailable");return true;});
 need(report.all_succeeded()&&report.slots<=INT32_MAX,"BNW belief collection loading failed");rules.slots_=report.slots;return rules;
}
std::vector<std::int64_t> BnwReligionHappinessRules::resolved_beliefs(const saves::ReligionObservation &religion)const{
 need(religion.beliefs.version&&*religion.beliefs.version>=1&&*religion.beliefs.version<=2,"BNW founder belief saved version is unknown");std::vector<std::int64_t> ids;
 for(const auto hash:religion.beliefs.belief_hashes){need(hash<=UINT32_MAX,"BNW belief saved hash exceeds source width");const auto *entry=hash?types_.find_hash(hash):nullptr;ids.push_back(entry?integer(entry->id):-1);}return ids;
}
BnwBeliefHappinessTerms BnwReligionHappinessRules::terms(const saves::ReligionObservation &religion,bool peace)const{
 const auto ids=resolved_beliefs(religion);std::set<std::int64_t> membership(ids.begin(),ids.end());BnwBeliefHappinessTerms result;
 for(const auto id:membership)if(id>=0&&static_cast<std::uint64_t>(id)<slots_)need(entries_.contains(id),"BNW active belief refers to a null native collection slot");
 // Native helpers iterate the cached logical collection, not saved list order.
 for(const auto &[slot,entry]:entries_)if(membership.contains(slot)){
  if(peace||!entry.requires_peace)result.player_happiness=add(result.player_happiness,entry.player_happiness);
  result.per_following_city=static_cast<float>(result.per_following_city+entry.following);need(std::isfinite(result.per_following_city),"BNW belief float accumulation is nonfinite");
  if(!result.peaceful_foreign_divisor&&entry.divisor>0)result.peaceful_foreign_divisor=entry.divisor;
 }
 return result;
}
BnwReligionFollowersReadModel BnwReligionFollowersReadModel::bind(const LiveSessionState &state){
 BnwReligionFollowersReadModel result;const auto &world=state.world;
 need(state.setup.profile==content::TechnologyProfile::bnw&&state.cities.profile==content::TechnologyProfile::bnw&&world.pregame&&world.pregame->layout==bnw&&world.all_players&&world.all_players->layout==bnw&&world.teams&&world.teams->layout==bnw&&world.teams->dimensions.teams==source_players&&world.teams->teams.size()==source_players&&world.all_players->players.size()==source_players&&state.setup.players.size()==source_players&&state.cities.players.size()==source_players,"BNW religious followers requires the complete current BNW roster/cities/teams");
 const auto &team_types=world.pregame->slots.integer_arrays.at("team_types");need(team_types.size()>=source_players,"BNW religion player-team settings are incomplete");result.players_.resize(source_players);std::vector<std::int32_t> alive_counts(source_players);
 for(std::size_t p=0;p<source_players;++p){const auto &player=state.setup.players.at(p);const auto &raw=world.all_players->players[p];need(raw.index==p&&raw.saved_player_id==static_cast<std::int64_t>(p)&&raw.core.layout==bnw&&core(raw.core.fields,"m_eID")==static_cast<std::int64_t>(p)&&core(raw.core.fields,"m_bAlive")==static_cast<std::int32_t>(player.alive)&&player.team==team_types[p],"BNW religious followers player identity/alive/team binding differs");auto &owned=result.players_[p];owned.alive=player.alive;owned.team=player.team;
  if(!player.alive)continue;
  need(player.team>=0&&player.team<static_cast<std::int64_t>(source_players),"BNW live religion player has no owning team");++alive_counts[static_cast<std::size_t>(player.team)];const auto &cities=state.cities.players.at(p).cities;need(raw.cities.layout==bnw&&raw.cities.cities.size()==cities.size(),"BNW religious followers city registry is incomplete");std::set<std::int64_t> ids;
  for(const auto &city:raw.cities.cities){const auto id=core(city.core.fields,"m_iID");need(ids.insert(id).second&&city.core.layout==bnw&&city.tail.layout==bnw&&city.core.version&&city.tail.context.bnw_city_version==city.core.version&&core(city.core.fields,"m_eOwner")==static_cast<std::int64_t>(p),"BNW religious followers city identity differs");const auto &live=cities.at(id);need(live.owner==static_cast<std::int64_t>(p)&&live.id==id&&live.source_core==std::optional{std::pair{city.core.start_offset,city.core.end_offset}},"BNW religious followers city source binding differs");const auto &religions=named(city.tail.blocks,"religions");const auto version=node(religions.children,"version");need(version>=1&&version<=3,"BNW city religion class version is unknown");const auto &status=named(religions.children,"m_ReligionStatus");need(status.wire_count&&*status.wire_count>=0&&static_cast<std::uint64_t>(*status.wire_count)==status.children.size(),"BNW city religion status collection is unresolved");std::vector<CityYieldReligion> rows;
   for(const auto &record:status.children){const auto version=node(record.children,"version");need(version>=1&&version<=2,"BNW city religion record version is unknown");rows.push_back({node(record.children,"m_eReligion"),node(record.children,"m_iFollowers"),node(record.children,"m_iPressure")});}need(rows.size()==live.religions.size(),"BNW city religious followers raw/runtime count differs");for(std::size_t i=0;i<rows.size();++i)need(rows[i].religion==live.religions[i].religion&&rows[i].followers==live.religions[i].followers&&rows[i].pressure==live.religions[i].pressure,"BNW city religious followers raw/runtime values differ");owned.cities.push_back(std::move(rows));
  }
 }
 result.alive_teams_.resize(source_players);result.at_war_.resize(source_players);
 for(std::size_t t=0;t<source_players;++t){const auto &team=world.teams->teams[t];need(team.index==t&&named(team.fields,"id").value==static_cast<std::int64_t>(t)&&named(team.fields,"alive_count").value==alive_counts[t],"BNW religious happiness team alive count/identity differs");result.alive_teams_[t]=alive_counts[t]>0;const auto &war=named(team.arrays,"at_war");need(war.columns==1&&war.values.size()==source_players,"BNW religion at-war relation owner is incomplete");for(const auto value:war.values){need(value==0||value==1,"BNW religion at-war relation is not boolean");result.at_war_[t].push_back(value!=0);}need(!result.at_war_[t][t],"BNW religious happiness requires the applied CvTeam::Read self-war repair");
 }return result;
}
bool BnwReligionFollowersReadModel::at_war(std::int64_t team,std::int64_t other)const{need(team>=0&&other>=0&&team<static_cast<std::int64_t>(source_players)&&other<static_cast<std::int64_t>(source_players),"BNW religion at-war team query is unavailable");return at_war_[static_cast<std::size_t>(team)][static_cast<std::size_t>(other)];}
bool BnwReligionFollowersReadModel::at_peace(std::int64_t player)const{need(player>=0&&player<static_cast<std::int64_t>(source_players),"BNW religion peace player is unavailable");const auto team=players_[static_cast<std::size_t>(player)].team;for(std::size_t other=0;other<civ_players;++other)if(alive_teams_[other]&&at_war(team,other))return false;return true;}
std::int32_t BnwReligionFollowersReadModel::cities_following(std::int32_t religion)const{std::int32_t count=0;for(const auto &player:players_)if(player.alive)for(const auto &city:player.cities)if(majority(city)==religion)count=add(count,1);return count;}
std::int32_t BnwReligionFollowersReadModel::foreign_cities_following(std::int64_t player,std::int32_t religion)const{need(player>=0&&player<static_cast<std::int64_t>(source_players),"BNW foreign city player is unavailable");if(religion<=0)return 0;std::int32_t total=0;for(std::size_t p=0;p<civ_players;++p)if(p!=static_cast<std::size_t>(player)&&players_[p].alive)for(const auto &city:players_[p].cities)if(majority(city)==religion)total=add(total,1);return total;}
std::int32_t BnwReligionFollowersReadModel::foreign_followers(std::int64_t player,std::int32_t religion)const{need(player>=0&&player<static_cast<std::int64_t>(source_players),"BNW foreign follower player is unavailable");if(religion<=0)return 0;std::int32_t total=0;for(std::size_t p=0;p<civ_players;++p)if(p!=static_cast<std::size_t>(player)&&players_[p].alive)for(const auto &city:players_[p].cities)total=add(total,followers(city,religion));return total;}
std::int32_t BnwReligionFollowersReadModel::peaceful_foreign_followers(std::int64_t player,std::int32_t religion)const{need(player>=0&&player<static_cast<std::int64_t>(source_players),"BNW foreign follower player is unavailable");if(religion<=0)return 0;std::int32_t total=0;const auto team=players_[static_cast<std::size_t>(player)].team;for(std::size_t p=0;p<civ_players;++p)if(p!=static_cast<std::size_t>(player)&&players_[p].alive&&!at_war(team,players_[p].team))for(const auto &city:players_[p].cities)total=add(total,followers(city,religion));return total;}
BnwReligionHappinessTermsQuote quote_bnw_happiness_from_religion_terms(const LiveSessionState &state,const SavedContentProfile &selected,const SessionCheckpointIdentity &identity,BnwGameplayLuaEpochOwner &owner,const BnwReligionHappinessRules &rules,std::int64_t player){
 BnwReligionHappinessTermsQuote result;
 try{
  need(rules.rules_sha256()==identity.rules_sha256,"BNW religious happiness selected rules identity differs");result.query=quote_bnw_happiness_from_religion(state,selected,identity,owner,player);if(!result.query.founder_resolved||!result.query.epoch)return result;if(result.query.happiness_quoted)return result;
  auto &epoch=*result.query.epoch;auto model=BnwFounderReligionReadModel::bind(*epoch.world);const auto *religion=model.religion(*result.query.founder_religion,-1);need(religion,"BNW founded happiness has no matching current religion");reconciled_beliefs(*epoch.world,rules);const auto cities=BnwReligionFollowersReadModel::bind(*epoch.world);BnwReligionHappinessTerms terms;terms.at_peace=cities.at_peace(player);const auto beliefs=rules.terms(*religion,terms.at_peace);terms.player_happiness=beliefs.player_happiness;terms.per_following_city=beliefs.per_following_city;terms.peaceful_foreign_divisor=beliefs.peaceful_foreign_divisor;
  terms.following_cities=cities.cities_following(*result.query.founder_religion);const auto following=static_cast<float>(static_cast<float>(terms.following_cities)*terms.per_following_city);need(std::isfinite(following)&&static_cast<double>(following)>=INT32_MIN&&static_cast<double>(following)<=INT32_MAX,"BNW religious following-city conversion exceeds native int");terms.following_happiness=static_cast<std::int32_t>(following);terms.total=add(terms.player_happiness,terms.following_happiness);
  if(terms.peaceful_foreign_divisor>0){terms.founder_callback_calls=2;const auto foreign_religion=repeat_founder(epoch,player);terms.foreign_founder_religion=foreign_religion;if(foreign_religion>0){const auto current=BnwReligionFollowersReadModel::bind(*epoch.world);terms.peaceful_foreign_followers=current.peaceful_foreign_followers(player,foreign_religion);}terms.foreign_happiness=integer(static_cast<std::int64_t>(terms.peaceful_foreign_followers)/terms.peaceful_foreign_divisor);terms.total=add(terms.total,terms.foreign_happiness);}
  result.query.happiness=terms.total;result.query.happiness_quoted=true;result.query.blocker="BNW founded religious happiness terms quoted; aggregate happiness and atomic VM/world publication remain";result.terms=terms;
 }catch(const std::exception &error){result={};result.query.blocker=std::string("BNW religious happiness failed; detached epoch discarded: ")+error.what();}return result;
}
}
