#include "bnw_major_team_process_fixture.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/runtime/normalised_bnw_state.hpp"
#include <string_view>
#include <type_traits>
namespace civ5::runtime::tests {
namespace {
void check(bool value,const char* why){if(!value)throw std::runtime_error(why);}
const auto& named(const auto& rows,std::string_view name){
 const typename std::remove_reference_t<decltype(rows)>::value_type* found=nullptr;
 for(const auto& row:rows){if(row.name&&std::string_view(row.name)==name){check(!found,"Process fixture duplicate field");found=&row;}}
 check(found,"Process fixture missing genuine field");return *found;
}
std::uint64_t count(const content::RulesDatabase& db,const std::string& sql,const std::string& technology){
 std::int64_t value=-1;
 const auto report=db.query_rows({sql,{technology}},[&](const auto& row){value=row.integer_at(0);},1);
 check(report.all_succeeded()&&report.rows==1&&value>=0,"Process fixture complete SQL count unavailable");
 return static_cast<std::uint64_t>(value);
}
std::int32_t first_member(const LiveSessionState& state,std::int64_t team){
 check(state.world.pregame&&state.world.all_players,"Process fixture actual roster unavailable");
 const auto& teams=state.world.pregame->archive.integer_arrays.at("team_types");
 check(teams.size()==64&&state.world.all_players->players.size()==64,"Process fixture roster incomplete");
 for(std::int32_t player=0;player<64;++player){
  const auto& saved=state.world.all_players->players.at(static_cast<std::size_t>(player));
  const auto& ever=named(saved.core.fields,"m_bEverAlive").values;
  check(ever.size()==1&&(ever[0]==0||ever[0]==1),"Process fixture ever-alive source malformed");
  if(teams.at(static_cast<std::size_t>(player))==team&&ever[0])return player;
 }return -1;
}
}
struct BnwMajorProcessFixtureCapture::Impl {
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host;
 std::shared_ptr<const LiveSessionState> state;
 NormalisedBnwState before;
 std::uint64_t generation=0,revision=0,dirty_occurrence=0;
 std::size_t dirty_events=0;
 std::int64_t team=-1,technology=-1;
 std::string technology_type,rules_identity;
 std::uint64_t reveals=0,trades=0;
 std::int32_t member=-1;
};
BnwMajorProcessFixtureCapture::BnwMajorProcessFixtureCapture(std::unique_ptr<Impl> p):impl_(std::move(p)){}
BnwMajorProcessFixtureCapture::~BnwMajorProcessFixtureCapture()=default;
std::shared_ptr<const BnwMajorProcessFixtureCapture> BnwMajorProcessFixtureCapture::capture_before(
 const ui::SelectedBnwReadOnlyPreview& preview,const BnwMajorTeamHasTechPending& pending){
 auto p=std::make_unique<Impl>();
 preview.with_owned_host_camera_input([&](auto host,const auto*){
  pending.require_stored_current(*host);
  check(pending.phase()==BnwMajorTeamHasTechPhase::resource_reveal_pending&&!pending.process_prefix_observation(),
   "UNAVAILABLE actual SAME-VM hook/city frontier not reached or process already attempted");
  p->host=host;p->state=host->session().read_state_lease();p->generation=host->session().state_generation();
  p->revision=p->state->ui_source_revision;p->dirty_events=p->state->city_dirty_events.size();
  p->dirty_occurrence=p->state->city_dirty_next_occurrence;p->team=pending.command().team;p->technology=pending.command().technology;
  const auto* tech=host->technologies().find(static_cast<std::uint64_t>(p->technology));
  check(tech&&!tech->base.type.empty(),"Process fixture genuine selected technology missing");
  p->technology_type=tech->base.type;p->rules_identity=host->identity().rules_sha256;
  p->reveals=count(host->database(),"SELECT count(*) FROM Resources WHERE TechReveal=?",p->technology_type);
  p->trades=count(host->database(),"SELECT count(*) FROM Resources WHERE TechCityTrade=?",p->technology_type);
  p->member=first_member(*p->state,p->team);
  check(p->member>=0,"UNAVAILABLE genuine native ever-alive member absent");
  p->before=normalise_bnw_state(host->session());
  pending.require_stored_current(*host);
 });
 return std::shared_ptr<const BnwMajorProcessFixtureCapture>(new BnwMajorProcessFixtureCapture(std::move(p)));
}
void BnwMajorProcessFixtureCapture::verify_after(const ui::SelectedBnwReadOnlyPreview& preview,
 const BnwMajorTeamHasTechPending& pending)const{
 const auto& p=*impl_;
 preview.with_owned_host_camera_input([&](auto host,const auto*){
  check(host.get()==p.host.get()&&!host.owner_before(p.host)&&!p.host.owner_before(host),"Process fixture canonical Host substituted");
  pending.require_stored_current(*host);
  const auto state=host->session().read_state_lease();
  check(state.get()==p.state.get()&&!state.owner_before(p.state)&&!p.state.owner_before(state)&&
   host->session().state_generation()==p.generation&&host->identity().rules_sha256==p.rules_identity&&
   pending.command().team==p.team&&pending.command().technology==p.technology,"Process fixture actual source/rules/world replaced");
  check(pending.phase()==BnwMajorTeamHasTechPhase::player_traits_pending,"Process fixture actual Traits frontier missing");
  const auto observed=pending.process_prefix_observation();
  check(observed&&state->world.map_plots&&observed->map_plots==state->world.map_header.grid.size()&&
   observed->map_plots==state->world.map_plots->plots.size()&&observed->reveal_rules==p.reveals&&
   observed->trade_rules==p.trades&&observed->first_traits_player==p.member&&first_member(*state,p.team)==p.member,
   "Process fixture full actual map/catalogue/SDK first-player traversal differs");
  check(state->ui_source_revision==p.revision&&state->city_dirty_next_occurrence==p.dirty_occurrence&&
   state->city_dirty_events.size()==p.dirty_events,"Zero process calls unexpectedly published dirty effects");
  const auto after=normalise_bnw_state(host->session());const auto difference=diff_normalised_bnw_state(p.before,after);
  check(difference.known_values_equal&&p.before.unresolved==after.unresolved,
   "Zero process prefix unexpectedly mutated normalized raw/owned map, flags, players, clock, RNG or events");
  check(!host->session().readiness().can_continue(),"Process prefix falsely granted full BNW continuation");
  pending.require_stored_current(*host);
 });
}
}

namespace civ5::runtime::tests {
namespace {
bool same_trait_field(const saves::PlayerTraitsField& a,const saves::PlayerTraitsField& b){
 if(std::string_view(a.name?a.name:"")!=std::string_view(b.name?b.name:"")||a.encoding!=b.encoding||
  a.columns!=b.columns||a.wire_count!=b.wire_count||a.values!=b.values||a.hashes!=b.hashes||a.rows.size()!=b.rows.size())return false;
 for(std::size_t i=0;i<a.rows.size();++i){if(a.rows[i].key!=b.rows[i].key||a.rows[i].wire_count!=b.rows[i].wire_count||a.rows[i].values!=b.rows[i].values)return false;}
 return true;
}
}
struct BnwMajorTraitsFixtureCapture::Impl {
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host;
 std::shared_ptr<const LiveSessionState> state;
 NormalisedBnwState before;
 std::uint64_t revision=0,generation=0,dirty_occurrence=0;
 std::size_t dirty_events=0;
 std::int32_t actor=-1,wounded=0;
 std::vector<saves::PlayerTraitsField> history;
};
BnwMajorTraitsFixtureCapture::BnwMajorTraitsFixtureCapture(std::unique_ptr<Impl> p):impl_(std::move(p)){}
BnwMajorTraitsFixtureCapture::~BnwMajorTraitsFixtureCapture()=default;
std::shared_ptr<const BnwMajorTraitsFixtureCapture> BnwMajorTraitsFixtureCapture::capture_before(
 const ui::SelectedBnwReadOnlyPreview& preview,const BnwMajorTeamHasTechPending& pending){
 auto p=std::make_unique<Impl>();
 preview.with_owned_host_camera_input([&](auto host,const auto*){
  pending.require_stored_current(*host);const auto observation=pending.process_prefix_observation();
  check(pending.phase()==BnwMajorTeamHasTechPhase::player_traits_pending&&observation&&!pending.traits_reset_effect(),
   "UNAVAILABLE real process prefix has not reached first Traits or has been attempted");
  p->host=host;p->state=host->session().read_state_lease();p->generation=host->session().state_generation();
  p->revision=p->state->ui_source_revision;p->actor=observation->first_traits_player;
  p->dirty_events=p->state->city_dirty_events.size();p->dirty_occurrence=p->state->city_dirty_next_occurrence;
  const auto& raw=p->state->world.all_players->players.at(static_cast<std::size_t>(p->actor));
  const auto& wound=named(raw.core.fields,"m_iWoundedUnitDamageMod").values;
  check(wound.size()==1&&wound[0]>=INT32_MIN&&wound[0]<=INT32_MAX,"Traits fixture actual wounded operand unavailable");
  p->wounded=static_cast<std::int32_t>(wound[0]);
  for(const auto* field:{"m_iBaktunPreviousTurn","m_aMayaBonusChoices","m_iUniqueLuxuryCitiesPlaced","m_aUniqueLuxuryAreas"}){
   p->history.push_back(named(raw.traits.fields,field));
  }
  p->before=normalise_bnw_state(host->session());pending.require_stored_current(*host);
 });return std::shared_ptr<const BnwMajorTraitsFixtureCapture>(new BnwMajorTraitsFixtureCapture(std::move(p)));
}
void BnwMajorTraitsFixtureCapture::verify_after(const ui::SelectedBnwReadOnlyPreview& preview,
 const BnwMajorTeamHasTechPending& pending,const BnwMajorPlayerTraitsOwner& actual_owner)const{
 const auto& p=*impl_;
 preview.with_owned_host_camera_input([&](auto host,const auto*){
  check(host.get()==p.host.get()&&!host.owner_before(p.host)&&!p.host.owner_before(host),"Traits fixture canonical Host replaced");
  pending.require_stored_current(*host);
  const auto state=host->session().read_state_lease();const auto effect=pending.traits_reset_effect();
  check(state==p.state&&host->session().state_generation()==p.generation&&
   pending.phase()==BnwMajorTeamHasTechPhase::policy_cost_pending&&effect&&effect->player==p.actor&&
   effect->wounded_before==p.wounded&&effect->technology==pending.command().technology&&effect->change==1,
   "Traits fixture missing actual same-world Reset/Init effect");
  actual_owner.require_current();const auto cache=actual_owner.snapshot();
  const auto& raw=state->world.all_players->players.at(static_cast<std::size_t>(p.actor));
  check(cache.player==p.actor&&cache.team==pending.command().team&&cache.cache.size()==raw.traits.fields.size(),
   "Traits fixture actual player/native cache ownership differs");
  for(std::size_t i=0;i<cache.cache.size();++i){check(same_trait_field(cache.cache[i],raw.traits.fields[i]),"Traits fixture retained raw/runtime cache stores differ");}
  for(const auto& old:p.history){check(same_trait_field(old,named(raw.traits.fields,old.name)),"Traits Reset incorrectly cleared historical Maya/luxury state");}
  check(named(raw.core.fields,"m_iWoundedUnitDamageMod").values==std::vector<std::int64_t>{effect->wounded_after}&&
   state->ui_source_revision==p.revision+1&&pending.after_revision()==p.revision+1&&
   state->city_dirty_events.size()==p.dirty_events&&state->city_dirty_next_occurrence==p.dirty_occurrence,
   "Traits fixture native wounded/revision/dirty effect differs");
  const auto after=normalise_bnw_state(host->session());const auto diff=diff_normalised_bnw_state(p.before,after);
  const auto wound_path="/raw/players/"+std::to_string(p.actor)+"/core/m_iWoundedUnitDamageMod/values";
  for(const auto& change:diff.values){check(change.path==wound_path,"Traits Reset changed unrelated known map, flags, clock, RNG, city or player state");}
  check(p.before.unresolved==after.unresolved&&!host->session().readiness().can_continue(),"Traits effect falsely granted complete gameplay/parity");
  pending.require_stored_current(*host);
 });
}
}

namespace civ5::runtime::tests {
struct BnwMajorPolicyFixtureCapture::Impl {
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host;
 std::shared_ptr<const LiveSessionState> state;
 NormalisedBnwState before;
 std::uint64_t generation=0,revision=0,dirty_occurrence=0;
 std::size_t dirty_events=0;
 std::int32_t actor=-1,old_cost=0;
};
BnwMajorPolicyFixtureCapture::BnwMajorPolicyFixtureCapture(std::unique_ptr<Impl> p):impl_(std::move(p)){}
BnwMajorPolicyFixtureCapture::~BnwMajorPolicyFixtureCapture()=default;
std::shared_ptr<const BnwMajorPolicyFixtureCapture> BnwMajorPolicyFixtureCapture::capture_before(
 const ui::SelectedBnwReadOnlyPreview& preview,const BnwMajorTeamHasTechPending& pending){
 auto p=std::make_unique<Impl>();
 preview.with_owned_host_camera_input([&](auto host,const auto*){
  pending.require_stored_current(*host);const auto traits=pending.traits_reset_effect();
  check(pending.phase()==BnwMajorTeamHasTechPhase::policy_cost_pending&&traits&&!pending.policy_free_unit_effect(),
   "UNAVAILABLE genuine Traits Reset has not reached policy recompute or prefix consumed");
  p->host=host;p->state=host->session().read_state_lease();p->generation=host->session().state_generation();
  p->revision=p->state->ui_source_revision;p->actor=traits->player;
  p->dirty_events=p->state->city_dirty_events.size();p->dirty_occurrence=p->state->city_dirty_next_occurrence;
  const auto& value=named(p->state->world.all_players->players.at(static_cast<std::size_t>(p->actor)).core.fields,"m_iPolicyCostModifier").values;
  check(value.size()==1&&value[0]>=INT32_MIN&&value[0]<=INT32_MAX,"Policy fixture source cached modifier malformed");
  p->old_cost=static_cast<std::int32_t>(value[0]);p->before=normalise_bnw_state(host->session());pending.require_stored_current(*host);
 });return std::shared_ptr<const BnwMajorPolicyFixtureCapture>(new BnwMajorPolicyFixtureCapture(std::move(p)));
}
void BnwMajorPolicyFixtureCapture::verify_after(const ui::SelectedBnwReadOnlyPreview& preview,
 const BnwMajorTeamHasTechPending& pending,const BnwMajorPlayerTraitsOwner& actual_owner)const{
 const auto& p=*impl_;
 preview.with_owned_host_camera_input([&](auto host,const auto*){
  check(host.get()==p.host.get()&&!host.owner_before(p.host)&&!p.host.owner_before(host),"Policy fixture canonical Host replaced");
  pending.require_stored_current(*host);const auto state=host->session().read_state_lease();const auto effect=pending.policy_free_unit_effect();
  const auto phase=pending.phase();
  check(state==p.state&&host->session().state_generation()==p.generation&&effect&&effect->player==p.actor&&
   effect->technology==pending.command().technology&&effect->before==p.old_cost&&
   (phase==BnwMajorTeamHasTechPhase::first_trait_unit_pending||phase==BnwMajorTeamHasTechPhase::remaining_process_players_pending||
    phase==BnwMajorTeamHasTechPhase::resource_plot_yields_pending||phase==BnwMajorTeamHasTechPhase::city_routes_pending||
    phase==BnwMajorTeamHasTechPhase::team_tech_researched_pending),"Policy fixture missing actual policy/free-unit prefix frontier");
  actual_owner.require_current();const auto stored=actual_owner.policy_free_unit_effect();
  check(stored&&*stored==*effect,"Policy fixture genuine owned native iterator/cost storage differs");
  const auto& raw=state->world.all_players->players.at(static_cast<std::size_t>(p.actor));
  check(named(raw.core.fields,"m_iPolicyCostModifier").values==std::vector<std::int64_t>{effect->after}&&
   state->ui_source_revision==p.revision+1&&pending.after_revision()==p.revision+1&&
   state->city_dirty_events.size()==p.dirty_events&&state->city_dirty_next_occurrence==p.dirty_occurrence,
   "Policy fixture actual scalar/revision/dirty effects differ");
  const auto& units=named(raw.traits.fields,"m_aFreeTraitUnits").values;
  check(units.size()%2==0&&effect->free_unit_index<=units.size()/2,"Policy fixture actual free-unit iterator out of range");
  if(effect->first_free_unit_class!=-1){
   const auto i=static_cast<std::size_t>(effect->free_unit_index)*2;
   check(i+1<units.size()&&units[i]==effect->first_free_unit_class&&units[i+1]==effect->technology&&
    phase==BnwMajorTeamHasTechPhase::first_trait_unit_pending,"Policy fixture skipped reached unit allocation");
  }else{check(effect->free_unit_index==units.size()/2,"Policy fixture did not complete native no-unit scan");}
  const auto after=normalise_bnw_state(host->session());const auto diff=diff_normalised_bnw_state(p.before,after);
  const auto cost_path="/raw/players/"+std::to_string(p.actor)+"/core/m_iPolicyCostModifier/values";
  for(const auto& change:diff.values){check(change.path==cost_path,"Policy prefix changed unrelated known RNG/turn/map/flags/cities/players");}
  check(p.before.unresolved==after.unresolved&&!host->session().readiness().can_continue(),"Policy prefix falsely granted whole technology/turn parity");
  pending.require_stored_current(*host);
 });
}
}

namespace civ5::runtime::tests {
struct BnwMajorResearchedFixtureCapture::Impl {
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host;
 std::shared_ptr<const LiveSessionState> state;
 NormalisedBnwState before;
 std::uint64_t generation=0,revision=0;
 bool ends_game=false;
 std::int64_t team=-1,technology=-1,best_route=-1;
};
BnwMajorResearchedFixtureCapture::BnwMajorResearchedFixtureCapture(std::unique_ptr<Impl> p):impl_(std::move(p)){}
BnwMajorResearchedFixtureCapture::~BnwMajorResearchedFixtureCapture()=default;
std::shared_ptr<const BnwMajorResearchedFixtureCapture> BnwMajorResearchedFixtureCapture::capture_before(
 const ui::SelectedBnwReadOnlyPreview& preview,const BnwMajorTeamHasTechPending& pending){
 auto p=std::make_unique<Impl>();
 preview.with_owned_host_camera_input([&](auto host,const auto*){
  pending.require_stored_current(*host);
  check(pending.phase()==BnwMajorTeamHasTechPhase::team_tech_researched_pending&&
   !pending.team_tech_researched_completed()&&!pending.after_research_effect(),
   "UNAVAILABLE genuine complete map loops/TeamTechResearched frontier missing or consumed");
  p->host=host;p->state=host->session().read_state_lease();p->generation=host->session().state_generation();
  p->revision=p->state->ui_source_revision;p->team=pending.command().team;p->technology=pending.command().technology;
  p->ends_game=named(p->state->world.game_state.flags,"end_game_tech_researched").value;
  for(const auto& team:p->state->world.teams->teams)if(named(team.fields,"id").value==p->team)
   p->best_route=named(team.fields,"best_route").value;
  p->before=normalise_bnw_state(host->session());pending.require_stored_current(*host);
 });return std::shared_ptr<const BnwMajorResearchedFixtureCapture>(new BnwMajorResearchedFixtureCapture(std::move(p)));
}
void BnwMajorResearchedFixtureCapture::verify_after(const ui::SelectedBnwReadOnlyPreview& preview,
 const BnwMajorTeamHasTechPending& pending)const{
 const auto& p=*impl_;
 preview.with_owned_host_camera_input([&](auto host,const auto*){
  check(host.get()==p.host.get()&&!host.owner_before(p.host)&&!p.host.owner_before(host),"Researched fixture canonical Host substituted");
  pending.require_stored_current(*host);const auto state=host->session().read_state_lease();
  const auto observed=pending.after_research_effect();
  check(state.get()==p.state.get()&&!state.owner_before(p.state)&&!p.state.owner_before(state)&&
   host->session().state_generation()==p.generation&&pending.command().team==p.team&&pending.command().technology==p.technology&&
   pending.team_tech_researched_completed()&&observed&&observed->ends_game_before==p.ends_game,
   "Researched fixture genuine dispatch/world/observed scalar missing");
  const auto* tech=host->technologies().find(static_cast<std::uint64_t>(p.technology));
  check(tech&&observed->ends_game_after==(p.ends_game||tech->booleans.at("EndsGame"))&&
   named(state->world.game_state.flags,"end_game_tech_researched").value==observed->ends_game_after,
   "Researched fixture actual CvGame EndsGame scalar differs");
  const auto phase=pending.phase();
  check(phase==BnwMajorTeamHasTechPhase::artifact_notifications_pending||phase==BnwMajorTeamHasTechPhase::era_change_pending||
   phase==BnwMajorTeamHasTechPhase::map_reveal_pending||phase==BnwMajorTeamHasTechPhase::player_research_pending,
   "Researched fixture skipped actual suffix frontier");
  std::uint64_t expected_revision=p.revision+static_cast<std::uint64_t>(observed->ends_game_before!=observed->ends_game_after);
  if(observed->best_route_stored){
   check(phase==BnwMajorTeamHasTechPhase::player_research_pending&&observed->best_route_before==p.best_route&&
    observed->current_era==observed->requested_era,"Researched fixture skipped mandatory era/map calls");
   const saves::TeamObservation* raw=nullptr;
   for(const auto& team:state->world.teams->teams)if(named(team.fields,"id").value==p.team)raw=&team;
   check(raw&&named(raw->fields,"best_route").value==observed->best_route_after,
    "Researched fixture actual retained best route differs");
   const TeamTechnologies* owned=nullptr;
   for(const auto& team:state->technologies)if(team.team()==p.team){check(!owned,"Fixture duplicate owned team");owned=&team;}
   check(owned,"Fixture actual owned team absent");
   std::int64_t winner=-1,best=-1,value=0;std::uint64_t builds=0;
   const auto report=host->database().query_rows({"SELECT ID,RouteType,PrereqTech FROM Builds ORDER BY ID",{}},[&](const auto& row){
    const auto id=row.integer("ID");++builds;
    const auto route=row.text("RouteType");if(!route||route->empty())return;
    const auto* route_info=host->types().find(*route);check(route_info&&route_info->table=="Routes","Fixture actual route type unavailable");
    std::int64_t candidate=-1;const auto prereq=row.text("PrereqTech");
    if(!prereq||prereq->empty())candidate=1;
    else{const auto* type=host->types().find(*prereq);check(type&&type->table=="Technologies","Fixture actual route prerequisite absent");
     const auto* rule=host->technologies().find(static_cast<std::uint64_t>(type->id));check(rule,"Fixture actual route Tech Info absent");
     if(owned->has_tech(type->id))candidate=rule->integers.at("Cost");
    }
    if(candidate>=value){value=candidate;winner=id;best=route_info->id;}
   });
   check(report.all_succeeded()&&builds==observed->builds&&winner==observed->winning_build&&best==observed->best_route_after&&
    value==observed->route_value,"Researched fixture actual SQL route winner/order differs");
   expected_revision+=static_cast<std::uint64_t>(observed->best_route_before!=observed->best_route_after);
  }
  check(state->ui_source_revision==expected_revision&&pending.after_revision()==expected_revision,
   "Researched fixture callback or scalar mutation revised unexpected world");
  const auto after=normalise_bnw_state(host->session());const auto diff=diff_normalised_bnw_state(p.before,after);
  std::uint64_t raw_index=0;
  for(const auto& team:state->world.teams->teams)if(named(team.fields,"id").value==p.team)raw_index=team.index;
  for(const auto& change:diff.values){
   const auto route="/raw/teams/"+std::to_string(raw_index)+"/fields/best_route";
   check(change.path=="/raw/game/flags/end_game_tech_researched"||(observed->best_route_stored&&change.path==route),
    "Researched fixture changed unrelated known map/clock/RNG/players/cities/research");
  }
  check(p.before.unresolved==after.unresolved&&!host->session().readiness().can_continue(),"Researched suffix falsely granted whole technology/turn/parity");
  pending.require_stored_current(*host);
 });
}
}

#include "civ5/runtime/bnw_research_queue_checkpoint.hpp"
#include "civ5/runtime/bnw_city_production_checkpoint.hpp"
#include "civ5/runtime/bnw_game_active_team_read.hpp"
#include <algorithm>
namespace civ5::runtime::tests {
struct BnwMajorPlayerResearchFixtureCapture::Impl {
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host;
 std::shared_ptr<const LiveSessionState> state;
 saves::SavedSessionSnapshot before_world;
 std::map<std::int64_t,std::vector<std::int64_t>> queues;
 std::map<std::int64_t,std::set<std::int64_t>> flags;
 std::uint64_t revision=0,generation=0;
 NormalisedBnwState before;
};
BnwMajorPlayerResearchFixtureCapture::BnwMajorPlayerResearchFixtureCapture(std::unique_ptr<Impl> p):impl_(std::move(p)){}
BnwMajorPlayerResearchFixtureCapture::~BnwMajorPlayerResearchFixtureCapture()=default;
std::shared_ptr<const BnwMajorPlayerResearchFixtureCapture> BnwMajorPlayerResearchFixtureCapture::capture_before(
 const ui::SelectedBnwReadOnlyPreview& preview,const BnwMajorTeamHasTechPending& pending){
 auto p=std::make_unique<Impl>();
 preview.with_owned_host_camera_input([&](auto host,const auto*){
  pending.require_stored_current(*host);const auto suffix=pending.after_research_effect();
  check(pending.phase()==BnwMajorTeamHasTechPhase::player_research_pending&&pending.team_tech_researched_completed()&&
   suffix&&suffix->best_route_stored&&pending.player_research_effects().empty(),"UNAVAILABLE real popResearch frontier missing or consumed");
  p->host=host;p->state=host->session().read_state_lease();p->generation=host->session().state_generation();
  p->revision=p->state->ui_source_revision;p->before_world=p->state->world;p->before=normalise_bnw_state(host->session());
  for(const auto& player:p->state->player_research.players)p->queues.emplace(player.first,player.second.queue);
  for(const auto& player:p->state->research_turn.players)p->flags.emplace(player.first,player.second.researching);
  pending.require_stored_current(*host);
 });return std::shared_ptr<const BnwMajorPlayerResearchFixtureCapture>(new BnwMajorPlayerResearchFixtureCapture(std::move(p)));
}
void BnwMajorPlayerResearchFixtureCapture::verify_after(const ui::SelectedBnwReadOnlyPreview& preview,
 const BnwMajorTeamHasTechPending& pending)const{
 const auto& p=*impl_;
 preview.with_owned_host_camera_input([&](auto host,const auto*){
  check(host.get()==p.host.get()&&!host.owner_before(p.host)&&!p.host.owner_before(host),"Player research fixture canonical Host replaced");
  pending.require_stored_current(*host);const auto state=host->session().read_state_lease();
  check(state.get()==p.state.get()&&!state.owner_before(p.state)&&!p.state.owner_before(state)&&host->session().state_generation()==p.generation,
   "Player research fixture actual world/generation replaced");
  const auto phase=pending.phase();
  check(phase==BnwMajorTeamHasTechPhase::first_research_reward_pending||phase==BnwMajorTeamHasTechPhase::technology_announcement_pending||
   phase==BnwMajorTeamHasTechPhase::technology_share_pending,"Player research fixture skipped actual reward/announcement/share boundary");
  const auto effects=pending.player_research_effects();
  const auto* publication=state->setup.extensions.template find<BnwTeamTechResearchPublication>();
  check(publication&&publication->written==publication->events.size(),"Player research fixture actual dirty queue incomplete");
  std::size_t cursor=0;std::uint64_t revised=0;bool queue_changed=false,rank_changed=false;
  std::set<std::int64_t> affected;
  for(const auto& effect:effects){
   check(affected.insert(effect.player).second&&state->setup.players.at(effect.player).alive&&
    state->setup.players.at(effect.player).team==pending.command().team,"Player research fixture native alive-team roster differs");
   auto queue=p.queues.at(effect.player);auto flags=p.flags.at(effect.player);
   check(effect.popped==flags.contains(pending.command().technology),"Player research fixture actual IsResearchingTech branch differs");
   if(effect.popped){const auto it=std::find(queue.begin(),queue.end(),pending.command().technology);if(it!=queue.end())queue.erase(it);flags.erase(pending.command().technology);queue_changed=true;}
   check(state->player_research.players.at(effect.player).queue==queue&&state->research_turn.players.at(effect.player).researching==flags,
    "Player research fixture paired queue/flags lost a different queued technology");
   const auto& raw=state->world.all_players->players.at(static_cast<std::size_t>(effect.player));
   const auto& old=p.before_world.all_players->players.at(static_cast<std::size_t>(effect.player));
   check(raw.cities.cities.size()==effect.cities_invalidated&&raw.cities.cities.size()==old.cities.cities.size(),"Player research fixture skipped a genuine city");
   bool player_rank_changed=false;
   for(std::size_t i=0;i<raw.cities.cities.size();++i){
    const auto& before=named(old.cities.cities[i].production.blocks,"before_strategy");
    const auto& after=named(raw.cities.cities[i].production.blocks,"before_strategy");
    for(const auto* valid:{"m_abBaseYieldRankValid","m_abYieldRankValid"}){
     check(named(after.arrays,valid).values==std::vector<std::int64_t>(6,0),"Player research fixture city valid flag still true");
     const auto& prior=named(before.arrays,valid).values;
     player_rank_changed|=std::any_of(prior.begin(),prior.end(),[](auto v){return v!=0;});
    }
    for(const auto* rank:{"m_aiBaseYieldRank","m_aiYieldRank"})check(named(before.arrays,rank).values==named(after.arrays,rank).values,
     "Player research fixture incorrectly zeroed numeric rank values");
   }
   check(effect.rank_flags_changed==player_rank_changed,"Player research fixture native rank mutation report differs");rank_changed|=player_rank_changed;
   std::vector<BnwTeamTechResearchDirtyBit> expected_dirty;
   if(effect.popped){
    if(effect.player==detail::bnw_game_active_player_read(*state))expected_dirty.push_back(BnwTeamTechResearchDirtyBit::popup);
    if(pending.command().team==detail::bnw_game_active_team_read(*state))expected_dirty.insert(expected_dirty.end(),
     {BnwTeamTechResearchDirtyBit::research_buttons,BnwTeamTechResearchDirtyBit::game_data,BnwTeamTechResearchDirtyBit::score});
   }
   check(effect.dirty_order==expected_dirty,"Player research fixture SDK active-player/team dirty branches differ");
   for(const auto kind:expected_dirty){check(cursor<publication->written&&publication->events[cursor++]==
    BnwTeamTechResearchDirtyEvent{effect.player,pending.command().team,kind,p.generation},"Player research fixture dirty order/repetitions differ");}
   if(effect.popped||effect.rank_flags_changed||!effect.dirty_order.empty())++revised;
  }
  check(cursor==publication->written&&state->ui_source_revision==p.revision+revised&&pending.after_revision()==p.revision+revised,
   "Player research fixture actual source revision/dirty count differs");
  for(const auto& player:state->setup.players)check(affected.contains(player.first)==(player.second.alive&&player.second.team==pending.command().team),
   "Player research fixture missed an actual living team member");
  for(const auto& old:p.queues)if(!affected.contains(old.first))check(state->player_research.players.at(old.first).queue==old.second&&
   state->research_turn.players.at(old.first).researching==p.flags.at(old.first),"Player research fixture changed another team's queue");
  // Actual native mutation is the positive producer; copies are independent
  // codec roundtrip checks, not a synthetic input or a gameplay clone/swap.
  if(queue_changed){const auto section=make_bnw_research_queue_checkpoint(state->world,p.before_world,host->identity());
   auto restored=p.before_world;restore_bnw_research_queue_checkpoint(section,restored,p.before_world,host->identity());
   for(const auto& effect:effects){const auto& raw=state->world.all_players->players.at(static_cast<std::size_t>(effect.player));
    const auto& saved=restored.all_players->players.at(static_cast<std::size_t>(effect.player));
    check(named(named(raw.administration.blocks,"pre_city").arrays,"m_researchQueue").values==
     named(named(saved.administration.blocks,"pre_city").arrays,"m_researchQueue").values,
     "Actual popResearch queue cannot roundtrip existing ID33 codec");
    const auto& live_flags=named(named(raw.knowledge.blocks,"player_techs").hashes,"researching_tech");
    const auto& loaded_flags=named(named(saved.knowledge.blocks,"player_techs").hashes,"researching_tech");
    check(live_flags.wire_count==loaded_flags.wire_count&&live_flags.rows.size()==loaded_flags.rows.size()&&
     std::equal(live_flags.rows.begin(),live_flags.rows.end(),loaded_flags.rows.begin(),[](const auto& a,const auto& b){
      return a.type_hash==b.type_hash&&a.values==b.values;}),"Actual popResearch full raw flags cannot roundtrip existing ID33 codec");
   }
  }
  if(rank_changed){const auto section=make_bnw_city_production_checkpoint(state->world,p.before_world,host->identity());
   auto restored=p.before_world;restore_bnw_city_production_checkpoint(section,restored,p.before_world,host->identity());
   for(const auto& effect:effects)for(std::size_t i=0;i<state->world.all_players->players.at(static_cast<std::size_t>(effect.player)).cities.cities.size();++i){
    const auto& saved=restored.all_players->players.at(static_cast<std::size_t>(effect.player)).cities.cities[i];
    for(const auto* valid:{"m_abBaseYieldRankValid","m_abYieldRankValid"})
     check(named(named(saved.production.blocks,"before_strategy").arrays,valid).values==std::vector<std::int64_t>(6,0),
      "Actual city rank invalidation cannot roundtrip existing ID26 codec");
   }
  }
  const auto after=normalise_bnw_state(host->session());const auto diff=diff_normalised_bnw_state(p.before,after);
  for(const auto& change:diff.values){
   bool allowed=false;
   for(const auto actor:affected){const auto owner=std::to_string(actor);
    allowed|=change.path.starts_with("/owned/research/players/"+owner+"/queue/")||
     (change.path.starts_with("/raw/players/"+owner+"/")&&(change.path.find("/m_researchQueue")!=std::string::npos||
      change.path.find("/researching_tech/")!=std::string::npos||change.path.find("/m_abBaseYieldRankValid/")!=std::string::npos||
      change.path.find("/m_abYieldRankValid/")!=std::string::npos));
   }
   check(allowed,"Player research changed unrelated known map/clock/RNG/numeric rank/city/player field");
  }
  check(after.unresolved==p.before.unresolved&&!host->session().readiness().can_continue(),"Player research falsely granted whole technology/turn/parity");
  pending.require_stored_current(*host);
 });
}
}

namespace civ5::runtime::tests {
struct BnwMajorAnnouncementFixtureCapture::Impl {
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host;
 std::shared_ptr<const LiveSessionState> state;
 NormalisedBnwState before;
 std::uint64_t generation=0,revision=0;
};
BnwMajorAnnouncementFixtureCapture::BnwMajorAnnouncementFixtureCapture(std::unique_ptr<Impl> p):impl_(std::move(p)){}
BnwMajorAnnouncementFixtureCapture::~BnwMajorAnnouncementFixtureCapture()=default;
std::shared_ptr<const BnwMajorAnnouncementFixtureCapture> BnwMajorAnnouncementFixtureCapture::capture_before(
 const ui::SelectedBnwReadOnlyPreview& preview,const BnwMajorTeamHasTechPending& pending){
 auto p=std::make_unique<Impl>();
 preview.with_owned_host_camera_input([&](auto host,const auto*){
  pending.require_stored_current(*host);
  check(pending.phase()==BnwMajorTeamHasTechPhase::technology_announcement_pending||
   pending.phase()==BnwMajorTeamHasTechPhase::technology_share_pending,
   "UNAVAILABLE genuine announcement/share frontier not reached");
  check(!pending.announcement_share_effect(),"Announcement fixture already consumed");
  p->host=host;p->state=host->session().read_state_lease();p->generation=host->session().state_generation();
  p->revision=p->state->ui_source_revision;p->before=normalise_bnw_state(host->session());
  pending.require_stored_current(*host);
 });
 return std::shared_ptr<const BnwMajorAnnouncementFixtureCapture>(new BnwMajorAnnouncementFixtureCapture(std::move(p)));
}
void BnwMajorAnnouncementFixtureCapture::verify_after(const ui::SelectedBnwReadOnlyPreview& preview,
 const BnwMajorTeamHasTechPending& pending)const{
 const auto& p=*impl_;
 preview.with_owned_host_camera_input([&](auto host,const auto*){
  check(host.get()==p.host.get()&&!host.owner_before(p.host)&&!p.host.owner_before(host),"Announcement fixture Host replaced");
  pending.require_stored_current(*host);const auto s=host->session().read_state_lease();
  check(s.get()==p.state.get()&&!s.owner_before(p.state)&&!p.state.owner_before(s)&&
   host->session().state_generation()==p.generation,"Announcement fixture world/generation changed");
  const auto observed=pending.announcement_share_effect();
  check(observed.has_value(),"Announcement genuine source was not executed");
  const auto* queue=s->setup.extensions.template find<BnwMajorTechMessageQueue>();
  check(queue&&queue->written==queue->messages.size()&&queue->messages==observed->messages,
   "Announcement actual retained AddMessage publication incomplete");
  const auto& command=pending.command();std::vector<BnwMajorTechMessage> expected;
  if(command.announce&&named(s->world.game_state.flags,"final_initialized").value){
   const auto actor=detail::bnw_game_active_player_read(*s);
   if(s->setup.players.at(actor).alive&&s->setup.players.at(actor).team==command.team){
    const auto* tech=host->technologies().find(static_cast<std::uint64_t>(command.technology));
    check(tech&&s->setup.game_type==0,"Announcement SP/technology fixture unresolved");
    expected.push_back({0,actor,command.team,command.technology,-1,false,s->setup.game_turn,p.generation,
     "TXT_KEY_MISC_YOU_DISCOVERED_TECH",{tech->base.text.key}});
   }
  }
  check(observed->messages==expected&&observed->before_revision==p.revision&&
   observed->after_revision==p.revision+(expected.empty()?0u:1u)&&pending.after_revision()==observed->after_revision&&
   s->ui_source_revision==observed->after_revision,"Announcement native parameters/ordered operands/revision differ");
  std::int64_t previous=-1;
  for(const auto& entry:observed->sharing){
   check(entry.recipient>previous&&entry.recipient!=command.team,"TechShare source order/recipient differs");previous=entry.recipient;
   const auto& raw=s->world.teams->teams.at(static_cast<std::size_t>(entry.recipient));
   check(named(raw.fields,"alive_count").value>0,"TechShare processed dead recipient");
   const auto own=std::find_if(s->technologies.begin(),s->technologies.end(),[&](const auto& t){return t.team()==entry.recipient;});
   check(own!=s->technologies.end()&&entry.already_owned==own->has_tech(command.technology),"TechShare actual HasTech differs");
   std::int64_t threshold=-1,count=0;
   if(!entry.already_owned){
    const auto& share=named(raw.arrays,"tech_share").values;
    for(std::size_t i=0;i<share.size();++i)if(share[i]>0){threshold=static_cast<std::int64_t>(i+1);break;}
    if(threshold>0)for(std::size_t other=0;other<63;++other){
     const auto& candidate=s->world.teams->teams.at(other);
     const auto id=named(candidate.fields,"id").value;
     const auto techs=std::find_if(s->technologies.begin(),s->technologies.end(),[&](const auto& t){return t.team()==id;});
     check(techs!=s->technologies.end(),"TechShare fixture team missing");
     if(named(candidate.fields,"alive_count").value>0&&techs->has_tech(command.technology)&&named(raw.arrays,"has_met").values.at(other)==1)++count;
    }
   }
   check(entry.best_share==threshold&&entry.met_owner_count==count&&entry.would_grant==(threshold>0&&count>=threshold),
    "TechShare native threshold/met/owner math differs");
  }
  if(observed->sharing_complete){
   std::size_t expected_count=0;
   for(const auto& raw:s->world.teams->teams)if(named(raw.fields,"alive_count").value>0&&named(raw.fields,"id").value!=command.team)++expected_count;
   check(observed->sharing.size()==expected_count&&observed->recursive_team==-1,"TechShare skipped reachable team/grant");
  }else if(observed->announcement_complete){
   check(!observed->sharing.empty()&&observed->sharing.back().would_grant&&
    observed->recursive_team==observed->sharing.back().recipient&&pending.phase()==BnwMajorTeamHasTechPhase::technology_share_pending,
    "TechShare recursive acquisition falsely completed");
  }
  const auto after=normalise_bnw_state(host->session());
  check(diff_normalised_bnw_state(p.before,after).values.empty()&&after.unresolved==p.before.unresolved,
   "Announcement/share changed map/RNG/clock/teams/queue/city instead of only owned message transport");
  check(!host->session().readiness().can_continue(),"Announcement falsely granted whole-turn continuation");
  pending.require_stored_current(*host);
 });
}
}

namespace civ5::runtime::tests {
namespace {
auto& esp_fixture_named(auto& rows,std::string_view key){
 auto* found=static_cast<std::remove_reference_t<decltype(*rows.begin())>*>(nullptr);
 for(auto& row:rows)if(row.name&&std::string_view(row.name)==key){check(!found,"Espionage fixture duplicate named source field");found=&row;}
 check(found,"Espionage fixture missing source field");return *found;
}
bool same_esp_fixture_record(const saves::PlayerEspionageRecord& a,const saves::PlayerEspionageRecord& b){
 if(a.name!=b.name||a.index!=b.index||a.start_offset!=b.start_offset||a.end_offset!=b.end_offset||
  a.fields.size()!=b.fields.size()||a.arrays.size()!=b.arrays.size()||a.blobs.size()!=b.blobs.size()||a.children.size()!=b.children.size())return false;
 for(std::size_t i=0;i<a.fields.size();++i)if(a.fields[i].name!=b.fields[i].name||a.fields[i].value!=b.fields[i].value)return false;
 for(std::size_t i=0;i<a.arrays.size();++i)if(a.arrays[i].name!=b.arrays[i].name||a.arrays[i].columns!=b.arrays[i].columns||a.arrays[i].values!=b.arrays[i].values)return false;
 for(std::size_t i=0;i<a.blobs.size();++i)if(a.blobs[i].name!=b.blobs[i].name||a.blobs[i].bytes!=b.blobs[i].bytes)return false;
 for(std::size_t i=0;i<a.children.size();++i)if(!same_esp_fixture_record(a.children[i],b.children[i]))return false;
 return true;
}
}
struct BnwMajorEspionageFixtureCapture::Impl {
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host;
 std::shared_ptr<const LiveSessionState> state;
 saves::PlayerEspionageRecord before;
 NormalisedBnwState normalized;
 std::uint64_t generation=0,revision=0;
};
BnwMajorEspionageFixtureCapture::BnwMajorEspionageFixtureCapture(std::unique_ptr<Impl> p):impl_(std::move(p)){}
BnwMajorEspionageFixtureCapture::~BnwMajorEspionageFixtureCapture()=default;
std::shared_ptr<const BnwMajorEspionageFixtureCapture> BnwMajorEspionageFixtureCapture::capture_before(
 const ui::SelectedBnwReadOnlyPreview& preview,const BnwMajorTeamHasTechPending& pending){
 auto p=std::make_unique<Impl>();preview.with_owned_host_camera_input([&](auto host,const auto*){
  pending.require_stored_current(*host);
  const auto prior=pending.announcement_share_effect();
  check(pending.phase()==BnwMajorTeamHasTechPhase::espionage_research_pending&&prior&&prior->culture_complete&&
   !pending.espionage_prefix_effect(),"UNAVAILABLE genuine espionage frontier missing/consumed");
  p->host=host;p->state=host->session().read_state_lease();p->generation=host->session().state_generation();p->revision=p->state->ui_source_revision;
  p->before=named(p->state->world.all_players->players.at(static_cast<std::size_t>(pending.command().player)).espionage.blocks,"player_espionage");
  p->normalized=normalise_bnw_state(host->session());pending.require_stored_current(*host);
 });return std::shared_ptr<const BnwMajorEspionageFixtureCapture>(new BnwMajorEspionageFixtureCapture(std::move(p)));
}
void BnwMajorEspionageFixtureCapture::verify_after(const ui::SelectedBnwReadOnlyPreview& preview,
 const BnwMajorTeamHasTechPending& pending)const{
 const auto& p=*impl_;preview.with_owned_host_camera_input([&](auto host,const auto*){
  pending.require_stored_current(*host);const auto s=host->session().read_state_lease();
  check(host.get()==p.host.get()&&!host.owner_before(p.host)&&!p.host.owner_before(host)&&s.get()==p.state.get()&&
   !s.owner_before(p.state)&&!p.state.owner_before(s)&&host->session().state_generation()==p.generation,"Espionage fixture canonical owner/world changed");
  const auto effect=pending.espionage_prefix_effect();
  check(effect&&effect->player==pending.command().player&&effect->targets_cleared<=22&&effect->targets_completed<=effect->targets_cleared,
   "Espionage actual prefix missing/source spans differ");
  check(effect->complete?(effect->targets_cleared==22&&effect->targets_completed==22&&
   pending.phase()==BnwMajorTeamHasTechPhase::major_happiness_pending):
   (effect->targets_cleared==effect->targets_completed+1&&effect->first_hook_target==static_cast<std::int64_t>(effect->targets_completed)&&
    effect->first_hook_technology>=0&&pending.phase()==BnwMajorTeamHasTechPhase::espionage_research_pending),
   "Espionage native next-hook frontier/complete-target accounting differs");
  auto expected=p.before;std::uint64_t changed=0;
  for(auto& row:expected.children)if(row.name&&std::string_view(row.name)=="stealable_techs"&&row.index<effect->targets_cleared){
   auto& values=esp_fixture_named(row.arrays,"techs").values;
   if(!values.empty())++changed;
   values.clear();esp_fixture_named(row.fields,"techs_wire_count").value=0;
  }
  auto& costs=esp_fixture_named(expected.arrays,"m_aiMaxTechCost").values;
  auto& number=esp_fixture_named(expected.arrays,"m_aiNumTechsToStealList").values;
  for(std::size_t target=0;target<effect->targets_completed;++target){
   const auto& raw=s->world.all_players->players.at(target);const auto alive=named(raw.core.fields,"m_bAlive").values.at(0);
   if(alive&&costs.at(target)!=-1){costs[target]=-1;++changed;}
   if(number.at(target)!=0){number[target]=0;++changed;}
  }
  const auto& raw=named(s->world.all_players->players.at(static_cast<std::size_t>(effect->player)).espionage.blocks,"player_espionage");
  check(same_esp_fixture_record(raw,expected),"Espionage prefix changed spies/names/intrigue/heists/uncleared-target/unknown native fields");
  const auto* cache=s->setup.extensions.template find<BnwMajorTechEspionageCache>();
  check(cache&&cache->player==effect->player&&cache->generation==p.generation&&cache->maximum_cost==costs&&cache->number_to_steal==number,
   "Espionage raw/native-owned counters do not match actual transformed source");
  for(const auto& row:raw.children)if(row.name&&std::string_view(row.name)=="stealable_techs")
   check(named(row.arrays,"techs").values==cache->lists.at(static_cast<std::size_t>(row.index)),"Espionage raw/native-owned list differs");
  check(effect->before_revision==p.revision&&effect->after_revision==p.revision+changed&&pending.after_revision()==p.revision+changed&&
   s->ui_source_revision==p.revision+changed,"Espionage native actual changed-store revision differs");
  const auto after=normalise_bnw_state(host->session());
  for(const auto& change:diff_normalised_bnw_state(p.normalized,after).values)
   check(change.path.find("/espionage/")!=std::string::npos,"Espionage changed known map/RNG/turn/research/city instead of the native lists/counters");
  check(after.unresolved==p.normalized.unresolved&&!host->session().readiness().can_continue(),"Espionage falsely granted full game checkpoint/turn readiness");
  pending.require_stored_current(*host);
 });
}
}

namespace civ5::runtime::tests {
struct BnwMajorHappinessFixtureCapture::Impl {
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host;
 std::shared_ptr<const LiveSessionState> state;
 std::vector<saves::PlayerCoreObservation> before;
 NormalisedBnwState normalized;
 std::uint64_t generation=0,revision=0;
};
BnwMajorHappinessFixtureCapture::BnwMajorHappinessFixtureCapture(std::unique_ptr<Impl> p):impl_(std::move(p)){}
BnwMajorHappinessFixtureCapture::~BnwMajorHappinessFixtureCapture()=default;
std::shared_ptr<const BnwMajorHappinessFixtureCapture> BnwMajorHappinessFixtureCapture::capture_before(
 const ui::SelectedBnwReadOnlyPreview& preview,const BnwMajorTeamHasTechPending& pending){
 auto p=std::make_unique<Impl>();preview.with_owned_host_camera_input([&](auto host,const auto*){
  pending.require_stored_current(*host);const auto esp=pending.espionage_prefix_effect();
  check(pending.phase()==BnwMajorTeamHasTechPhase::major_happiness_pending&&esp&&esp->complete&&
   !pending.happiness_prefix_effect(),"UNAVAILABLE genuine living-major happiness frontier absent/consumed");
  p->host=host;p->state=host->session().read_state_lease();p->generation=host->session().state_generation();p->revision=p->state->ui_source_revision;
  for(const auto& raw:p->state->world.all_players->players)p->before.push_back(raw.core);
  p->normalized=normalise_bnw_state(host->session());pending.require_stored_current(*host);
 });return std::shared_ptr<const BnwMajorHappinessFixtureCapture>(new BnwMajorHappinessFixtureCapture(std::move(p)));
}
void BnwMajorHappinessFixtureCapture::verify_after(const ui::SelectedBnwReadOnlyPreview& preview,
 const BnwMajorTeamHasTechPending& pending)const{
 const auto& p=*impl_;preview.with_owned_host_camera_input([&](auto host,const auto*){
  pending.require_stored_current(*host);const auto s=host->session().read_state_lease();const auto effect=pending.happiness_prefix_effect();
  check(host.get()==p.host.get()&&!host.owner_before(p.host)&&!p.host.owner_before(host)&&s.get()==p.state.get()&&
   !s.owner_before(p.state)&&!p.state.owner_before(s)&&host->session().state_generation()==p.generation,
   "Happiness fixture canonical Host/world/generation changed");
  check(effect&&effect->player>=0&&effect->player<22&&effect->before_revision==p.revision&&
   (effect->stores.size()==6||effect->stores.size()==7),"Happiness native genuine six stores missing");
  const auto actor=effect->player;
  for(std::int64_t i=0;i<actor;++i)check(!s->setup.players.at(i).alive||s->setup.players.at(i).team!=pending.command().team,
   "Happiness native first living same-team order skipped a member");
  check(s->setup.players.at(actor).alive&&s->setup.players.at(actor).team==pending.command().team,
   "Happiness native selected living-major relation differs");
  auto expected=named(p.before.at(static_cast<std::size_t>(actor)).fields,"m_iHappiness").values.at(0);
  check(effect->initial_happiness==expected,"Happiness actual initial scalar not measured");
  std::uint64_t minimum_changes=0;
  const auto rules=BnwHappinessPrefixRules::load(host->database());
  for(std::size_t i=0;i<effect->stores.size();++i){const auto& store=effect->stores[i];
   check(static_cast<std::size_t>(store.stage)==i,"Happiness native order differs");
   if(i<6){const auto operand=detail::bnw_happiness_prefix_reached_stage_math(*s,rules,host->identity(),actor,store.stage).operand;
    check(store.operand==operand,"Happiness arithmetic does not use actual reached rules/world");}
   const auto next=i==0?static_cast<std::int64_t>(store.operand):expected+store.operand;
   check(next>=INT32_MIN&&next<=INT32_MAX&&store.happiness==next,"Happiness native recurrence/int width differs");
   if(next!=expected)++minimum_changes;expected=next;
  }
  for(std::size_t i=0;i<64;++i){const auto& before=p.before.at(i);const auto& now=s->world.all_players->players.at(i).core;
   check(before.fields.size()==now.fields.size()&&before.layout==now.layout&&before.start_offset==now.start_offset&&before.end_offset==now.end_offset,
    "Happiness changed core archive layout/identity");
   for(std::size_t j=0;j<before.fields.size();++j){const auto& a=before.fields[j];const auto& b=now.fields[j];
    check(a.name==b.name&&a.encoding==b.encoding&&a.wire_count==b.wire_count&&a.hashes==b.hashes&&a.start_offset==b.start_offset&&a.end_offset==b.end_offset,
     "Happiness modified unrelated core field metadata");
    if(i==static_cast<std::size_t>(actor)&&a.name&&std::string_view(a.name)=="m_iHappiness")check(b.values==std::vector<std::int64_t>{expected},"Happiness raw scalar wrong");
    else check(a.values==b.values,"Happiness modified unrelated player core scalar/array");
   }
  }
  check(s->unhappiness.players.at(actor).happiness==expected&&!s->cities.players.at(actor).excess_happiness&&
   effect->after_revision==s->ui_source_revision&&pending.after_revision()==s->ui_source_revision&&
   s->ui_source_revision>=p.revision+minimum_changes&&s->ui_source_revision<=p.revision+minimum_changes+1,
   "Happiness paired owned scalar/derived invalidation/revision differs");
  if(effect->religion_complete)check(effect->stores.size()==7&&effect->founder_callbacks>=1&&effect->founder_callbacks<=2&&
   pending.phase()==BnwMajorTeamHasTechPhase::happiness_suffix_pending,"Happiness false religion completion");
  else check(effect->stores.size()==6&&(pending.phase()==BnwMajorTeamHasTechPhase::happiness_founder_pending||
   pending.phase()==BnwMajorTeamHasTechPhase::happiness_foreign_founder_pending),"Happiness missing mandatory actual founder query");
  const auto after=normalise_bnw_state(host->session());
  check(diff_normalised_bnw_state(p.normalized,after).values.empty()&&after.unresolved==p.normalized.unresolved&&
   !host->session().readiness().can_continue(),"Happiness changed unrelated known world/RNG/clock or granted full continuation");
  pending.require_stored_current(*host);
 });
}
}
#include <array>
namespace civ5::runtime::tests {
struct BnwMajorHappinessSuffixFixtureCapture::Impl {
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host;
 std::shared_ptr<const LiveSessionState> state;
 saves::PlayerCoreObservation before;
 NormalisedBnwState normalized;
 std::vector<BnwMajorHappinessDirtyEvent> earlier;
 std::int64_t player=-1;
 std::uint64_t generation=0,revision=0;
};
BnwMajorHappinessSuffixFixtureCapture::BnwMajorHappinessSuffixFixtureCapture(std::unique_ptr<Impl> p):impl_(std::move(p)){}
BnwMajorHappinessSuffixFixtureCapture::~BnwMajorHappinessSuffixFixtureCapture()=default;
std::shared_ptr<const BnwMajorHappinessSuffixFixtureCapture> BnwMajorHappinessSuffixFixtureCapture::capture_before(
 const ui::SelectedBnwReadOnlyPreview& preview,const BnwMajorTeamHasTechPending& pending){
 auto p=std::make_unique<Impl>();preview.with_owned_host_camera_input([&](auto host,const auto*){
  pending.require_stored_current(*host);const auto prefix=pending.happiness_prefix_effect();
  check(pending.phase()==BnwMajorTeamHasTechPhase::happiness_suffix_pending&&prefix&&prefix->religion_complete&&prefix->stores.size()==7,
   "UNAVAILABLE genuine completed religion frontier required for suffix fixture");
  p->host=host;p->state=host->session().read_state_lease();p->generation=host->session().state_generation();p->revision=p->state->ui_source_revision;
  p->player=prefix->player;p->before=p->state->world.all_players->players.at(static_cast<std::size_t>(p->player)).core;
  if(const auto* queue=p->state->setup.extensions.template find<BnwMajorHappinessPublication>())
   p->earlier.assign(queue->events.begin(),queue->events.begin()+static_cast<std::ptrdiff_t>(queue->written));
  p->normalized=normalise_bnw_state(host->session());pending.require_stored_current(*host);
 });return std::shared_ptr<const BnwMajorHappinessSuffixFixtureCapture>(new BnwMajorHappinessSuffixFixtureCapture(std::move(p)));
}
void BnwMajorHappinessSuffixFixtureCapture::verify_after(const ui::SelectedBnwReadOnlyPreview& preview,
 const BnwMajorTeamHasTechPending& pending)const{
 const auto& p=*impl_;preview.with_owned_host_camera_input([&](auto host,const auto*){
  pending.require_stored_current(*host);const auto s=host->session().read_state_lease();const auto effects=pending.happiness_suffix_effects();
  check(host.get()==p.host.get()&&!host.owner_before(p.host)&&!p.host.owner_before(host)&&s.get()==p.state.get()&&
   !s.owner_before(p.state)&&!p.state.owner_before(s)&&host->session().state_generation()==p.generation,
   "Happiness suffix fixture canonical owner/world/generation changed");
  check(effects.size()==p.earlier.size()+1&&effects.back().player==p.player,"Happiness suffix skipped/repeated actual team member");
  const auto& effect=effects.back();const auto* queue=s->setup.extensions.template find<BnwMajorHappinessPublication>();
  check(queue&&queue->written==effects.size()&&queue->events.size()==22&&queue->pending_ui_consumption,"Happiness actual dirty queue absent/false delivery");
  for(std::size_t i=0;i<p.earlier.size();++i)check(queue->events.at(i)==p.earlier[i],"Happiness earlier member dirty event changed");
  const auto rules=BnwHappinessSuffixRules::load(host->database());using Read=detail::BnwHappinessPostReligionRead;
  const auto read=[&](Read r){return detail::bnw_happiness_post_religion_reached_read(*s,rules,host->identity(),p.player,r);};
  check(effect.stores.size()==5||effect.stores.size()==6,"Happiness actual suffix native store count differs");
  auto value=named(p.before.fields,"m_iHappiness").values.at(0);std::uint64_t changes=1; // mandatory actual GameData publication
  const std::array<std::pair<BnwHappinessSuffixStage,Read>,3> first{{
   {BnwHappinessSuffixStage::wonders,Read::wonders},{BnwHappinessSuffixStage::minors,Read::minors},{BnwHappinessSuffixStage::leagues,Read::leagues}}};
  for(std::size_t i=0;i<first.size();++i){const auto operand=read(first[i].second).operand;const auto& store=effect.stores[i];
   check(store.stage==first[i].first&&store.operand==operand&&store.happiness==value+operand,"Happiness native wonders/minors/leagues recurrence differs");
   if(operand)++changes;value+=operand;
  }
  const auto connections=read(Read::connections_cache);check(connections.connections.has_value(),"Happiness actual recomputed connection quote absent");
  check(effect.connection_before==named(p.before.fields,"m_iCityConnectionHappiness").values.at(0)&&effect.connection_after==connections.operand&&
   effect.stores[3].stage==BnwHappinessSuffixStage::connections_cache&&effect.stores[3].happiness==value&&
   effect.stores[4].stage==BnwHappinessSuffixStage::connections_add&&effect.stores[4].operand==effect.connection_after&&effect.stores[4].happiness==value+effect.connection_after,
   "Happiness cache recompute must precede native trade-route getter");
  if(effect.connection_before!=effect.connection_after)++changes;
  if(effect.connection_after)++changes;value+=effect.connection_after;
  const auto local=read(Read::achievement_condition);
  check(effect.excess==(local.unhappiness?std::optional{static_cast<std::int32_t>(local.unhappiness->excess_happiness)}:std::nullopt)&&
   effect.achievement_requested==local.achievement_requested&&effect.stores.size()==(local.unhappiness?6:5),
   "Happiness local-player short-circuit/excess/achievement differs");
  const auto& now=s->world.all_players->players.at(static_cast<std::size_t>(p.player)).core;
  for(std::size_t i=0;i<p.before.fields.size();++i){const auto& a=p.before.fields[i];const auto& b=now.fields.at(i);
   check(a.name==b.name&&a.encoding==b.encoding&&a.start_offset==b.start_offset&&a.end_offset==b.end_offset&&a.hashes==b.hashes&&
    a.row_counts==b.row_counts&&a.bytes==b.bytes&&a.wire_count==b.wire_count,"Happiness suffix altered unrelated raw metadata");
   if(a.name&&std::string_view(a.name)=="m_iHappiness")check(b.values==std::vector<std::int64_t>{value},"Happiness suffix raw final value wrong");
   else if(a.name&&std::string_view(a.name)=="m_iCityConnectionHappiness")check(b.values==std::vector<std::int64_t>{effect.connection_after},"Happiness suffix raw connection cache wrong");
   else check(a.values==b.values,"Happiness suffix changed unrelated player fields");
  }
  const BnwMajorHappinessDirtyEvent expected{p.player,static_cast<std::int32_t>(value),effect.connection_after,effect.excess,p.generation,
   static_cast<std::uint64_t>(effects.size()),BnwTeamTechResearchDirtyBit::game_data,true,effect.achievement_requested};
  check(queue->events.at(queue->written-1)==expected&&s->unhappiness.players.at(p.player).happiness==value&&
   s->cities.players.at(p.player).excess_happiness==effect.excess&&effect.before_revision==p.revision&&
   effect.after_revision==p.revision+changes&&s->ui_source_revision==p.revision+changes,"Happiness suffix paired stores/dirty native args/revision differs");
  std::int64_t next=-1;for(auto i=p.player+1;i<22;++i)if(s->setup.players.at(i).alive&&s->setup.players.at(i).team==pending.command().team){next=i;break;}
  check(pending.phase()==(next<0?BnwMajorTeamHasTechPhase::technology_ui_pending:BnwMajorTeamHasTechPhase::major_happiness_pending),
   "Happiness source next live member/UI frontier differs");
  check(diff_normalised_bnw_state(p.normalized,normalise_bnw_state(host->session())).values.empty()&&!host->session().readiness().can_continue(),
   "Happiness suffix changed unrelated known world/RNG/clock or claimed full continuation");
  pending.require_stored_current(*host);
 });
}
}
