#include "civ5/runtime/bnw_minor_city_strength.hpp"
#include "civ5/runtime/bnw_city_registry.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
namespace {
using namespace civ5;using namespace civ5::runtime;
void check(bool b,const char* s){if(!b)throw std::runtime_error(s);}
const auto& field(const auto& rows,std::string_view name){
 const auto found=std::find_if(rows.begin(),rows.end(),[&](const auto& r){return r.name&&std::string_view(r.name)==name;});
 check(found!=rows.end(),"Natural city oracle required field absent");return *found;
}
std::int64_t scalar(const auto& rows,std::string_view name){const auto& f=field(rows,name);check(f.values.size()==1,"Natural city oracle scalar shape differs");return f.values[0];}
std::vector<std::int64_t> technology_seal(const LiveSessionState& state){
 std::vector<std::int64_t> result;for(const auto& t:state.technologies){result.push_back(t.last_tech());for(std::uint64_t slot=0;slot<t.slots();++slot){const auto v=t.at(static_cast<std::int64_t>(slot));result.insert(result.end(),{v.has_tech,v.no_trade,v.progress_times100,v.count});}}return result;
}
std::int64_t unit_value(const saves::SavedUnitObservation& u,std::string_view key){const auto& f=field(u.core.fields,key);check(f.values.size()==1,"Natural defender oracle required scalar absent");return f.values[0];}
const saves::SavedUnitObservation* defender_oracle(const LiveSessionState& state,const ui::SelectedBnwPreviewHost& host,std::int32_t owner,const saves::SavedCityObservation& city){
 const auto x=scalar(city.core.fields,"m_iX"),y=scalar(city.core.fields,"m_iY");const auto index=state.world.map_header.grid.index(static_cast<std::uint64_t>(x),static_cast<std::uint64_t>(y));
 const auto& plot=state.world.map_plots->plots.at(static_cast<std::size_t>(index));const auto& refs=field(plot.arrays,"units").values;const saves::SavedUnitObservation* best=nullptr;
 for(std::size_t i=0;i<refs.size();i+=2){
  if(refs[i]<0||refs[i]>=64||refs[i+1]==-1)continue;
  const auto& units=state.world.all_players->players.at(static_cast<std::size_t>(refs[i])).units.units;
  const auto found=std::find_if(units.begin(),units.end(),[&](const auto& u){const auto id=unit_value(u,"m_iID");return (id&8191)==(refs[i+1]&8191)&&((refs[i+1]&~8191)==0||id==refs[i+1]);});
  if(found==units.end())continue;
  check(unit_value(*found,"m_iX")==x&&unit_value(*found,"m_iY")==y,"Natural defender oracle unit/plot differs");
  if(unit_value(*found,"m_bEmbarked"))throw BnwMinorCityStrengthUnavailable("Natural defender oracle GetEmbarkedUnitDefense unsupported");
  if(unit_value(*found,"m_iBaseCombat")==0||unit_value(*found,"m_bDeathDelay"))continue;
  const auto* info=host.types().find_hash(static_cast<std::uint64_t>(unit_value(*found,"idUnitType")));check(info&&info->table=="Units"&&info->id==unit_value(*found,"m_eUnitType"),"Natural defender oracle active Info differs");
  std::string domain;std::size_t rows=0;
  const auto q=host.database().query_rows({"SELECT Type,Domain FROM Units WHERE ID=?",{info->id}},[&](const content::RulesRowView& row){check(row.text("Type")==info->type&&++rows==1,"Natural defender oracle SQL differs");const auto d=row.text("Domain");check(d.has_value(),"Natural defender oracle Domain absent");domain=*d;},2);
  check(q.all_succeeded()&&rows==1,"Natural defender oracle SQL failed");
  const bool water=field(plot.fields,"plot_type").value==3;bool valid=false;
  if(domain=="DOMAIN_AIR")valid=false;
  else if(domain=="DOMAIN_SEA"||(domain=="DOMAIN_HOVER"&&water))valid=water||unit_value(*found,"m_iCanMoveAllTerrainCount")>0;
  else if(domain=="DOMAIN_LAND"||domain=="DOMAIN_IMMOBILE"||domain=="DOMAIN_HOVER"){if(water)throw BnwMinorCityStrengthUnavailable("Natural defender oracle water-land unsupported");valid=true;}
  else throw BnwMinorCityStrengthUnavailable("Natural defender oracle unsupported Domain");
  if(!valid||refs[i]!=owner)continue;
  if(best)throw BnwMinorCityStrengthUnavailable("Natural defender oracle competing eligible units unsupported");best=&*found;
 }
 return best;
}
std::int64_t defender_strength_oracle(const LiveSessionState& state,const ui::SelectedBnwPreviewHost& host,std::int32_t owner,const saves::SavedCityObservation& city){
 const auto* u=defender_oracle(state,host,owner,city);if(!u)return 0;
 const auto hp=std::get<std::int64_t>(host.defines().find("MAX_HIT_POINTS")->lookup.value);check(hp>0,"Natural defender oracle health divisor absent");
 return unit_value(*u,"m_iBaseCombat")*100*(hp-unit_value(*u,"m_iDamage"))/hp;
}

// Independent source-formula oracle using actual imported operands only.
// Saved final strength is observational: an imported cache can be stale and
// its equality alone does not establish original-runtime parity.
std::int64_t oracle(const LiveSessionState& state,const ui::SelectedBnwPreviewHost& host,std::int32_t owner,const saves::SavedCityObservation& city){
 const auto integer=[&](const char* name){return std::get<std::int64_t>(host.defines().find(name)->lookup.value);};
 const auto source_float=[&](const char* name){return std::get<double>(host.defines().find(name)->lookup.value);};
 const auto& buildings=field(city.production.blocks,"buildings");
 auto defense=field(buildings.fields,"m_iBuildingDefense").value;
 defense*=100+field(buildings.fields,"m_iBuildingDefenseMod").value;defense/=100;
 auto answer=integer("CITY_STRENGTH_DEFAULT")+scalar(city.core.fields,"m_iPopulation")*integer("CITY_STRENGTH_POPULATION_CHANGE")+defense;
 // Source still reaches this division in the empty-defender branch.
 answer+=defender_strength_oracle(state,host,owner,city)*100/integer("CITY_STRENGTH_UNIT_DIVISOR");
 const auto team=state.world.pregame->archive.integer_arrays.at("team_types").at(static_cast<std::size_t>(owner));
 std::int64_t known=0;for(std::uint64_t tech=0;tech<host.technologies().collection.slots;++tech)known+=state.technologies.at(static_cast<std::size_t>(team)).has_tech(static_cast<std::int64_t>(tech));
 const auto percent=known*100/static_cast<std::int64_t>(host.technologies().collection.slots);
 const double progress=static_cast<double>(percent)/100.0*source_float("CITY_STRENGTH_TECH_BASE");
 double tech=std::pow(progress,source_float("CITY_STRENGTH_TECH_EXPONENT"));tech*=static_cast<double>(integer("CITY_STRENGTH_TECH_MULTIPLIER"));tech*=100;
 answer+=static_cast<std::int32_t>(tech+0.005);
 answer*=100+scalar(state.world.all_players->players.at(static_cast<std::size_t>(owner)).core.fields,"m_iCityStrengthMod");answer/=100;
 const auto index=state.world.map_header.grid.index(static_cast<std::uint64_t>(scalar(city.core.fields,"m_iX")),static_cast<std::uint64_t>(scalar(city.core.fields,"m_iY")));
 if(field(state.world.map_plots->plots.at(static_cast<std::size_t>(index)).fields,"plot_type").value==1)answer+=integer("CITY_STRENGTH_HILL_CHANGE");
 return answer;
}

struct Fixture {
 std::unique_ptr<ui::SelectedBnwReadOnlyPreview> preview;
 std::shared_ptr<const ui::SelectedBnwPreviewHost> host;
 LiveSession* actual_session=nullptr;
};
Fixture fixture(const std::filesystem::path& save,const std::filesystem::path& game){
 auto bootstrap=ui::bootstrap_selected_bnw_preview(save,game,0);
 if(bootstrap.preview.blocker)throw std::runtime_error(bootstrap.preview.blocker->reason);
 check(static_cast<bool>(bootstrap.preview.host),"Natural city selected host absent");
 Fixture f;f.actual_session=&bootstrap.preview.host->session();
 f.preview=std::make_unique<ui::SelectedBnwReadOnlyPreview>(std::move(bootstrap.preview));
 f.preview->with_owned_host_camera_input([&](std::shared_ptr<const ui::SelectedBnwPreviewHost> host,const ui::LiveBnwPreviewInput*){f.host=std::move(host);});
 return f;
}
void natural(const Fixture& f){
 const auto& host=*f.host;const auto& state=host.session().state();
 const auto original_tech=technology_seal(state);const auto source_events=state.city_dirty_events;const auto source_revision=state.ui_source_revision;
 std::size_t completed=0,occupied_completed=0,nonempty_refused=0,saved_equal=0,saved_different=0;
 const auto& minors=state.world.pregame->archive.flag_arrays.at("minor_nation_civs");
 // One owner per actual city: an unsupported branch retires that owner; it
 // cannot make a later city appear successfully refreshed in the same epoch.
 for(std::int32_t owner=22;owner<63;++owner){
  if(!state.setup.players.at(owner).alive||!minors.at(static_cast<std::size_t>(owner)))continue;
  for(const auto& city:state.world.all_players->players.at(static_cast<std::size_t>(owner)).cities.cities){
   auto scratch=BnwMinorCityStrengthScratch::from_selected_preview(*f.preview);
   const auto id=static_cast<std::int32_t>(scalar(city.core.fields,"m_iID"));
   const auto before=scratch->inspect(owner,id);const auto events=before.events;const auto next=before.next_occurrence,revision=before.revision;
   const auto index=state.world.map_header.grid.index(static_cast<std::uint64_t>(scalar(city.core.fields,"m_iX")),static_cast<std::uint64_t>(scalar(city.core.fields,"m_iY")));
   const bool empty=field(state.world.map_plots->plots.at(static_cast<std::size_t>(index)).arrays,"units").values.empty();
   // Oracle availability is diagnostic only. Every reached natural city
   // invokes the actual effect producer, including competing/embarked cases.
   std::optional<std::int64_t> expected;const saves::SavedUnitObservation* selected_defender=nullptr;std::string oracle_unavailable;
   try{expected=oracle(state,host,owner,city);selected_defender=defender_oracle(state,host,owner,city);}
   catch(const BnwMinorCityStrengthUnavailable& error){expected.reset();oracle_unavailable=error.what();}
   bool actual_refused=false;
   try{
    const auto result=scratch->refresh(owner,id);const auto after=scratch->inspect(owner,id);
    check(expected.has_value(),"Actual producer completed an oracle-unsupported mandatory branch");
    check(!selected_defender||!empty,"Natural selected defender not present in actual ordered stack");
    check(result.owner==owner&&result.city==id&&result.before==before.strength&&result.after==*expected&&result.plot==static_cast<std::int64_t>(index),"Natural city strength source formula/identity differs");
    check(after.strength==result.after&&after.next_occurrence==next+1&&after.revision==revision+1&&after.events.size()==events.size()+1&&std::equal(events.begin(),events.end(),after.events.begin()),"City strength cache/ordered dirty publication differs");
    const auto& event=after.events.back();check(event==BnwCityDirtyEvent{next,host.session().state_generation(),owner,id,-1,-1,BnwCityDirtyKind::city_info,true},"City strength emitted wrong source CityInfo event");
    check(after.technologies==before.technologies&&after.rng==before.rng&&after.map_rng==before.map_rng,"City strength changed technology acquisition or RNG");
    const auto repeated=scratch->refresh(owner,id);const auto again=scratch->inspect(owner,id);
    check(repeated.before==result.after&&repeated.after==result.after&&repeated.dirty_occurrence==next+1&&again.events.size()==events.size()+2&&again.next_occurrence==next+2&&again.revision==revision+2,"Repeated source refresh failed unconditional ordered dirty event");
    ++completed;if(selected_defender)++occupied_completed;if(before.strength==result.after)++saved_equal;else ++saved_different;
   }catch(const BnwMinorCityStrengthUnavailable& error){
    actual_refused=true;
    check(scratch->inspect(owner,id)==before,"Refused city refresh left partial cache/event/world writes");
    if(!empty&&std::string_view(error.what()).find("getBestDefender")!=std::string_view::npos)++nonempty_refused;
    else std::cout<<"Natural city effect subset unavailable: "<<error.what()<<'\n';
    if(!expected)std::cout<<"Actual producer refusal reached; separate oracle limitation: "<<oracle_unavailable<<'\n';
   }
   if(!expected)check(actual_refused&&scratch->inspect(owner,id)==before,"Oracle-unsupported natural case skipped actual producer refusal/atomicity");
  }
 }
 check(technology_seal(host.session().state())==original_tech&&host.session().state().city_dirty_events==source_events&&host.session().state().ui_source_revision==source_revision,"Detached city executor changed original session");
 check(!host.session().readiness().can_continue(),"City strength subset granted continuation readiness");
 if(!completed)std::cout<<"Positive empty-stack minor city fixture UNAVAILABLE; no world/HasTech/alive/Pregame fields fabricated\n";
 if(!nonempty_refused)std::cout<<"Actual competing/embarked defender refusal fixture UNAVAILABLE\n";
 if(!occupied_completed)std::cout<<"Positive actual occupied eligible defender fixture UNAVAILABLE; no unit/plot/HasTech fields fabricated\n";
 std::cout<<"Natural completed="<<completed<<" occupied-defender="<<occupied_completed<<" defender-refused="<<nonempty_refused<<" saved-cache equal="<<saved_equal<<" differs="<<saved_different<<"; saved equality is not original runtime parity\n";
}
std::optional<std::pair<std::int32_t,std::int32_t>> first_natural_city(const Fixture& f){
 const auto& s=f.host->session().state();const auto& minors=s.world.pregame->archive.flag_arrays.at("minor_nation_civs");
 for(std::int32_t owner=22;owner<63;++owner)if(s.setup.players.at(owner).alive&&minors.at(static_cast<std::size_t>(owner)))for(const auto& city:s.world.all_players->players.at(static_cast<std::size_t>(owner)).cities.cities)
  return std::pair{owner,static_cast<std::int32_t>(scalar(city.core.fields,"m_iID"))};
 return std::nullopt;
}
void actual_two_host_and_stale(const std::filesystem::path& save,const std::filesystem::path& game){
 auto a=fixture(save,game),b=fixture(save,game);
 check(a.host.get()!=b.host.get()&&a.actual_session!=b.actual_session,"Two-host test did not construct independent genuine factory hosts");
 auto scratch_a=BnwMinorCityStrengthScratch::from_selected_preview(*a.preview);
 auto scratch_b=BnwMinorCityStrengthScratch::from_selected_preview(*b.preview);
 static_assert(!std::is_constructible_v<BnwMinorCityStrengthScratch,LiveSessionState&,const ui::SelectedBnwPreviewHost&>);
 static_assert(!std::is_copy_constructible_v<BnwMinorCityStrengthScratch>);
 const auto city_a=first_natural_city(a),city_b=first_natural_city(b);
 if(!city_a||!city_b){std::cout<<"Natural two-host stale-city fixture UNAVAILABLE\n";return;}
 const auto [owner_a,id_a]=*city_a;const auto [owner_b,id_b]=*city_b;
 const auto before_a=scratch_a->inspect(owner_a,id_a),before_b=scratch_b->inspect(owner_b,id_b);
 const auto lease_a=a.actual_session->read_state_lease();const auto generation_a=a.actual_session->state_generation();
 // Existing real mutable_state API, real imported city cache. No HasTech,
 // alive/Pregame/unit fixture fields and no external receipt are fabricated.
 auto& mutable_state=a.actual_session->mutable_state();
 auto& cities=mutable_state.world.all_players->players.at(static_cast<std::size_t>(owner_a)).cities.cities;
 auto found=std::find_if(cities.begin(),cities.end(),[&](const auto& city){return scalar(city.core.fields,"m_iID")==id_a;});
 check(found!=cities.end(),"Natural mutation city disappeared");
 auto strength=std::find_if(found->core.fields.begin(),found->core.fields.end(),[](const auto& row){return row.name&&std::string_view(row.name)=="m_iStrengthValue";});
 check(strength!=found->core.fields.end()&&strength->values.size()==1,"Natural mutation strength absent");
 check(strength->values[0]<INT32_MAX,"Natural stale-cache mutation unsupported boundary");++strength->values[0];
 check(a.actual_session->read_state_lease()==lease_a&&a.actual_session->state_generation()==generation_a,"Semantic stale fixture changed pointer/generation rather than in-place source");
 bool refused=false;try{(void)scratch_a->refresh(owner_a,id_a);}catch(const BnwMinorCityStrengthUnavailable& error){refused=std::string_view(error.what()).find("in-place source")!=std::string_view::npos;}
 check(refused&&scratch_a->inspect(owner_a,id_a)==before_a,"Actual same-pointer/generation stale source was accepted or partially written");
 check(scratch_b->inspect(owner_b,id_b)==before_b,"Host A mutation changed genuine Host B private scratch");
 // B remains associated solely with its genuine source. Natural occupied B
 // remains explicitly unsupported rather than forced into a positive case.
 try{const auto result=scratch_b->refresh(owner_b,id_b);check(result.before==before_b.strength,"Host B scratch was stamped from another source");}
 catch(const BnwMinorCityStrengthUnavailable& error){check(scratch_b->inspect(owner_b,id_b)==before_b,"Host B unsupported effect wrote partial state");std::cout<<"Natural Host B positive effect subset UNAVAILABLE: "<<error.what()<<'\n';}
 scratch_a.reset();scratch_b.reset();
 // Repeat on a new owner from the now-current actual A state. Changing actual
 // revision via the existing mutation API must be semantic, not a false ready.
 auto revision_owner=BnwMinorCityStrengthScratch::from_selected_preview(*a.preview);
 const auto revision_before=revision_owner->inspect(owner_a,id_a);const auto revision_lease=a.actual_session->read_state_lease();const auto revision_generation=a.actual_session->state_generation();
 auto& revised=a.actual_session->mutable_state();check(revised.ui_source_revision<UINT64_MAX,"Natural stale revision boundary unsupported");++revised.ui_source_revision;
 check(a.actual_session->read_state_lease()==revision_lease&&a.actual_session->state_generation()==revision_generation,"In-place revision fixture unexpectedly replaced state");
 refused=false;try{(void)revision_owner->refresh(owner_a,id_a);}catch(const BnwMinorCityStrengthUnavailable& error){refused=std::string_view(error.what()).find("in-place source")!=std::string_view::npos;}
 check(refused&&revision_owner->inspect(owner_a,id_a)==revision_before,"Actual in-place event/revision semantic baseline was bypassed");
 if(revised.city_dirty_events.empty())std::cout<<"Naturally queued source-event mutation fixture UNAVAILABLE; no fake CityInfo receipt appended\n";
}
void actual_occupied_unit_stale(const std::filesystem::path& save,const std::filesystem::path& game){
 auto f=fixture(save,game);const auto& state=f.host->session().state();const auto& minors=state.world.pregame->archive.flag_arrays.at("minor_nation_civs");
 for(std::int32_t owner=22;owner<63;++owner){
  if(!state.setup.players.at(owner).alive||!minors.at(static_cast<std::size_t>(owner)))continue;
  for(const auto& city:state.world.all_players->players.at(static_cast<std::size_t>(owner)).cities.cities){
   const saves::SavedUnitObservation* selected=nullptr;
   try{selected=defender_oracle(state,*f.host,owner,city);}catch(const BnwMinorCityStrengthUnavailable&){continue;}
   if(!selected)continue;
   const auto uid=unit_value(*selected,"m_iID");const auto city_id=static_cast<std::int32_t>(scalar(city.core.fields,"m_iID"));
   const auto original_damage=unit_value(*selected,"m_iDamage");if(original_damage>=INT32_MAX)continue;
   auto scratch=BnwMinorCityStrengthScratch::from_selected_preview(*f.preview);const auto before=scratch->inspect(owner,city_id);
   const auto lease=f.actual_session->read_state_lease();const auto generation=f.actual_session->state_generation();
   auto& changed=f.actual_session->mutable_state();auto& units=changed.world.all_players->players.at(static_cast<std::size_t>(owner)).units.units;
   auto u=std::find_if(units.begin(),units.end(),[&](const auto& candidate){return unit_value(candidate,"m_iID")==uid;});check(u!=units.end(),"Natural selected defender disappeared");
   auto damage=std::find_if(u->core.fields.begin(),u->core.fields.end(),[](const auto& field){return field.name&&std::string_view(field.name)=="m_iDamage";});check(damage!=u->core.fields.end()&&damage->values.size()==1,"Natural selected defender damage absent");
   // Real existing mutation API on an already selected natural defender. This
   // makes currentness stale; it never fabricates a positive unit/plot branch.
   ++damage->values[0];
   check(f.actual_session->read_state_lease()==lease&&f.actual_session->state_generation()==generation,"Natural defender stale mutation changed lease/generation");
   bool refused=false;try{(void)scratch->refresh(owner,city_id);}catch(const BnwMinorCityStrengthUnavailable& error){refused=std::string_view(error.what()).find("in-place source")!=std::string_view::npos;}
   check(refused&&scratch->inspect(owner,city_id)==before,"Actual same-pointer defender damage changed without stale refusal or left partial effects");
   std::cout<<"Natural occupied defender in-place lifetime/currentness negative reached\n";return;
  }
 }
 std::cout<<"Natural occupied eligible defender stale fixture UNAVAILABLE; no unit/plot/HasTech fixture fabricated\n";
}

void actual_first_player_scalar_prefix(const std::filesystem::path& save,const std::filesystem::path& game){
 auto f=fixture(save,game);const auto& host=*f.host;const auto& source=host.session().state();
 constexpr std::array<std::string_view,6> columns{"FeatureProductionModifier","UnitFortificationModifier","UnitBaseHealModifier","WorkerSpeedModifier","InfluenceSpreadModifier","ExtraVotesPerDiplomat"};
 constexpr std::array<std::string_view,6> members{"m_iFeatureProductionModifier","m_iUnitFortificationModifier","m_iUnitBaseHealModifier","m_iWorkerSpeedModifier","m_iInfluenceSpreadModifier","m_iExtraVotesPerDiplomat"};
 std::optional<std::int32_t> technology;
 for(const auto& [slot,info]:host.technologies().entries){bool nonzero=false,complete=true;for(const auto column:columns){const auto v=info.integers.find(column);if(v==info.integers.end()){complete=false;break;}nonzero=nonzero||v->second!=0;}
  if(complete&&nonzero&&slot<=static_cast<std::uint64_t>(INT32_MAX)){technology=static_cast<std::int32_t>(slot);break;}
 }
 if(!technology){std::cout<<"Natural nonzero technology player-modifier rule UNAVAILABLE\n";return;}
 const auto& pregame=source.world.pregame->archive.integer_arrays.at("team_types");std::optional<std::int32_t> selected_team,actor;
 for(std::int32_t candidate=0;candidate<64&&!selected_team;++candidate){
  for(std::int32_t slot=0;slot<64;++slot){const auto& p=source.world.all_players->players.at(static_cast<std::size_t>(slot));
   if(pregame.at(static_cast<std::size_t>(slot))==candidate&&scalar(p.core.fields,"m_bEverAlive")){selected_team=candidate;actor=slot;break;}
  }
 }
 if(!selected_team){std::cout<<"Natural EVER-alive first-player modifier actor UNAVAILABLE\n";return;}
 const auto city=first_natural_city(f);auto scratch=BnwMinorCityStrengthScratch::from_selected_preview(*f.preview);
 const auto before=scratch->inspect_player_scalar_prefix(*actor);
 std::array<std::array<std::int32_t,6>,64> all_before{};for(std::int32_t slot=0;slot<64;++slot)all_before[static_cast<std::size_t>(slot)]=scratch->inspect_player_scalar_prefix(slot);
 const auto city_before=city?std::optional{scratch->inspect(city->first,city->second)}:std::nullopt;
 const auto source_tech=technology_seal(source);const auto source_revision=source.ui_source_revision;
 const auto& info=*host.technologies().find(static_cast<std::uint64_t>(*technology));
 const auto result=scratch->first_player_scalar_prefix(*selected_team,*technology,1);
 check(result&&result->team==*selected_team&&result->technology==*technology&&result->player==*actor&&result->change==1&&result->before==before,"Natural first-player scalar prefix selected wrong SDK actor/arguments");
 std::array<std::int32_t,6> expected{};for(std::size_t i=0;i<expected.size();++i){const auto rule=info.integers.find(columns[i]);check(rule!=info.integers.end(),"Natural first-player oracle rule absent");const auto value=rule->second;const auto after=std::int64_t(before[i])+value;check(after>=INT32_MIN&&after<=INT32_MAX,"Natural first-player modifier oracle native boundary unsupported");expected[i]=static_cast<std::int32_t>(after);check(result->delta[i]==value,"Natural first-player scalar delta differs from actual rule");}
 check(result->after==expected&&scratch->inspect_player_scalar_prefix(*actor)==expected,"Natural first-player scalar prefix didn't write six exact native effects");
 for(std::int32_t slot=0;slot<64;++slot)if(slot!=*actor)check(scratch->inspect_player_scalar_prefix(slot)==all_before[static_cast<std::size_t>(slot)],"First-player scalar prefix skipped mandatory traits by advancing another actor");
 if(city_before){const auto after=scratch->inspect(city->first,city->second);check(after.events==city_before->events&&after.next_occurrence==city_before->next_occurrence&&after.strength==city_before->strength&&after.technologies==city_before->technologies&&after.rng==city_before->rng&&after.map_rng==city_before->map_rng&&after.revision==city_before->revision+1,"Player scalar slice changed city/Lua/HasTech/RNG or wrong private revision");}
 const auto reverse=scratch->first_player_scalar_prefix(*selected_team,*technology,-1);
 check(reverse&&reverse->before==expected&&reverse->after==before&&scratch->inspect_player_scalar_prefix(*actor)==before,"Natural minus-one player scalar prefix failed exact SDK reversal");
 check(technology_seal(host.session().state())==source_tech&&host.session().state().ui_source_revision==source_revision&&!host.session().readiness().can_continue(),"Player scalar slice wrote actual source or granted technology/continuation");
 std::optional<std::int32_t> empty_team;for(std::int32_t candidate=0;candidate<64;++candidate){bool has_actor=false;for(std::int32_t slot=0;slot<64;++slot)has_actor=has_actor||(pregame.at(static_cast<std::size_t>(slot))==candidate&&scalar(source.world.all_players->players.at(static_cast<std::size_t>(slot)).core.fields,"m_bEverAlive")!=0);if(!has_actor){empty_team=candidate;break;}}
 if(empty_team){const auto unchanged=scratch->inspect_player_scalar_prefix(*actor);const auto empty=scratch->first_player_scalar_prefix(*empty_team,*technology,1);check(!empty&&scratch->inspect_player_scalar_prefix(*actor)==unchanged,"No-recipient scalar interval forged a prefix or changed world");}
 else std::cout<<"Naturally empty EVER-alive team fixture UNAVAILABLE\n";
 // Real source mutation through the existing API: not a modifier-rule,
 // HasTech, roster or callback witness manufactured to reach a positive case.
 scratch.reset();
 auto stale=BnwMinorCityStrengthScratch::from_selected_preview(*f.preview);const auto stale_before=stale->inspect_player_scalar_prefix(*actor);
 const auto lease=f.actual_session->read_state_lease();const auto generation=f.actual_session->state_generation();
 auto& changed=f.actual_session->mutable_state();auto& player=changed.world.all_players->players.at(static_cast<std::size_t>(*actor));
 auto row=std::find_if(player.core.fields.begin(),player.core.fields.end(),[&](const auto& field){return field.name&&std::string_view(field.name)==members[3];});check(row!=player.core.fields.end()&&row->values.size()==1&&row->values[0]<INT32_MAX,"Natural first-player stale field unavailable");++row->values[0];
 check(f.actual_session->read_state_lease()==lease&&f.actual_session->state_generation()==generation,"Natural scalar stale fixture replaced source instead of in-place mutation");
 bool refused=false;try{(void)stale->first_player_scalar_prefix(*selected_team,*technology,1);}catch(const BnwMinorCityStrengthUnavailable& error){refused=std::string_view(error.what()).find("in-place source")!=std::string_view::npos;}
 check(refused&&stale->inspect_player_scalar_prefix(*actor)==stale_before,"Natural stale scalar prefix wrote partial effects or bypassed source currentness");
 std::cout<<"Natural first-player six scalar effects +/-1, mandatory next-player barrier and stale source reached; no tech grant\n";
}

void selected(const std::filesystem::path& save,const std::filesystem::path& game){{auto f=fixture(save,game);natural(f);}actual_two_host_and_stale(save,game);actual_occupied_unit_stale(save,game);actual_first_player_scalar_prefix(save,game);}
}
int main(int argc,char** argv){check(argc==3,"Expected real selected BNW save and installed game root");selected(argv[1],argv[2]);}
