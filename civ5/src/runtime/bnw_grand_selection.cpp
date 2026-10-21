#include "civ5/runtime/bnw_grand_selection.hpp"
#include "civ5/runtime/bnw_game_activation_execution.hpp"
#include <climits>
#include <type_traits>
namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void need(bool b,const char*s){if(!b)throw std::runtime_error(s);}
std::int32_t integer(std::int64_t v){need(v>=INT32_MIN&&v<=INT32_MAX,"BNW Grand selection native integer width invalid");return static_cast<std::int32_t>(v);}
auto&named(auto&rows,std::string_view name){using Row=typename std::remove_reference_t<decltype(rows)>::value_type;using Pointer=std::conditional_t<std::is_const_v<std::remove_reference_t<decltype(rows)>>,const Row*,Row*>;Pointer p=nullptr;for(auto&v:rows)if(v.name&&name==v.name){need(!p,"BNW Grand selection duplicate native field");p=&v;}need(p,"BNW Grand selection missing native field");return *p;}
auto raw_rng(const LiveSessionState&s){std::array<SimulationRandomState,2> r;for(std::size_t i=0;i<2;++i){const auto&v=s.world.progression.random[i];r[i]={v.seed,v.calls,v.resets};}return r;}
auto live_rng(const LiveSessionState&s){return std::array{s.random.map.state(),s.random.gameplay.state()};}
void binding(const BnwTeamTechLuaEpoch&e,const SessionCheckpointIdentity&id,const SavedContentProfile&profile,const BnwGrandWorldAverageRules&r,const BnwHappinessSuffixRules&n,const content::RulesDatabase&db){need(e.world&&e.source_world&&e.source_world!=e.world.get()&&e.host&&e.getter_world==e.world.get()&&e.identity.payload_sha256==id.payload_sha256&&e.identity.rules_sha256==id.rules_sha256&&id.payload_sha256.size()==64&&id.rules_sha256==r.identity()&&id.rules_sha256==n.identity()&&db.gameplay_fingerprint_sha256()==id.rules_sha256&&e.selected_dlc==profile.enabled_dlc&&e.selected_mods==profile.enabled_mods&&profile.enabled_mods.empty()&&profile.can_activate&&profile.all_requirements_located,"BNW Grand selection SAME epoch/source/rules binding stale");need(e.all_selected_contexts_loaded&&e.all_listeners_registered&&e.all_external_writes_escrowed&&e.host->loading_complete()&&e.host->source_errors().empty(),"BNW Grand selection retained VM/current escrow stale");}
bool minor(const LiveSessionState&s,std::int64_t p){need(s.world.pregame&&s.world.pregame->layout==bnw&&p>=0&&p<22,"BNW Grand selection native minor query owner missing");const auto&core=s.world.all_players->players.at(p).core;const auto&id=named(core.fields,"m_eID");need(id.encoding==saves::PlayerCoreEncoding::i32&&id.values.size()==1&&id.values[0]==p,"BNW Grand selection native GetID stale");const auto i=s.world.pregame->archive.flag_arrays.find("minor_nation_civs");need(i!=s.world.pregame->archive.flag_arrays.end()&&i->second.size()==64,"BNW Grand selection actual native minor_nation_civs64 missing");return i->second.at(static_cast<std::size_t>(p));}
bool boolean(const saves::NamedSaveInteger&v){need(v.value==0||v.value==1,"BNW Grand selection native specialization bool invalid");return v.value!=0;}
}
std::int32_t BnwGrandMinimumAgeOwner::require_current(const content::RulesDatabase&db)const{
 need(source_&&database_&&database_.get()==&db&&cached_value_&&actual_source_current_&&actual_source_current_()&&db.immutable_preview_fingerprints_current(),"BNW Grand selection genuine GC cached minimum-age source/lifecycle unavailable");return *cached_value_;
}
BnwGrandSelectionPending stage_bnw_grand_selection(const BnwGameActivationHappinessPending&enclosing,BnwTeamTechLuaEpoch&e,const BnwGrandWorldAverageRules&r,const BnwHappinessSuffixRules&n,const content::RulesDatabase&db,std::shared_ptr<const BnwGrandMinimumAgeOwner> minimum_age){
 need(enclosing.builder_complete()&&enclosing.diplomacy_begin_complete()&&enclosing.completed_prefix().size()==14&&enclosing.completed_prefix().back()==BnwGameActivationStep::diplomacy_requests_begin&&enclosing.happiness().native_update_complete()&&&enclosing.state()==e.world.get(),"BNW Grand selection requires genuine SAME incomplete activation prefix");
 need(!enclosing.happiness_->grand_selection_,"BNW Grand selection already retained; no decision/adoption replay");
 const auto*pressure=enclosing.happiness().grand_pressure();need(pressure,"BNW Grand selection actual pressure/age predecessor unavailable");const auto&prior=checked_bnw_grand_pressure(*pressure,e,r,n,db);binding(e,pressure->identity(),pressure->profile(),r,n,db);
 BnwGrandSelectionPending j;j.identity_=pressure->identity();j.profile_=pressure->profile();j.player_=pressure->player();j.world_=e.world.get();j.source_=e.source_world;j.host_=e.host.get();j.raw_=raw_rng(*e.world);j.prefix_=live_rng(*e.source_world);j.live_=live_rng(*e.world);j.roster_=bnw_grand_military_inputs_revision(*e.world);j.before_grand_=bnw_grand_native_component_revision(*e.world,j.player_);j.source_grand_=bnw_grand_native_component_revision(*e.source_world,j.player_);
 auto&q=j.progress_;q.active_before=prior.active_strategy;q.incremented_age=prior.age_after;q.priorities=prior.after_priorities;q.pressure_log_changes=prior.log_changes;
 // Source active==-1 short circuits the GC cached getter completely.
 if(q.active_before!=-1){need(minimum_age!=nullptr,"BNW Grand selection actual cached GC minimum-age owner unavailable; pressure prefix retained");q.minimum_age=minimum_age->require_current(db);j.minimum_age_=std::move(minimum_age);}
 q.decision=evaluate_bnw_grand_selection(q.priorities,q.active_before,q.incremented_age,q.minimum_age);
 auto&g=named(e.world->world.all_players->players.at(j.player_).strategy.blocks,"grand_strategy_ai");auto prospective_grand=g;auto&active=named(g.fields,"active_strategy");auto&age=named(g.fields,"turns_since_active_set");need(integer(active.value)==q.active_before&&integer(age.value)==q.incremented_age&&q.incremented_age>0,"BNW Grand selection pressure/native incremented age changed");
 if(q.decision.active_written){named(prospective_grand.fields,"active_strategy").value=q.decision.active_after;named(prospective_grand.fields,"turns_since_active_set").value=q.decision.age_after;}
 saves::NamedSaveInteger*dirty=nullptr;
 if(q.decision.specialization_called){q.minor=minor(*e.world,j.player_);if(!*q.minor){auto&spec=named(e.world->world.all_players->players.at(j.player_).strategy.blocks,"city_specialization_ai");q.dirty_before=boolean(named(spec.fields,"specializations_dirty"));q.interrupt_wonders=boolean(named(spec.fields,"interrupt_wonders"));q.dirty_after=true;q.dirty_written=true;dirty=&named(spec.fields,"specializations_dirty");j.before_specialization_=bnw_grand_native_component_revision(*e.world,j.player_,spec);auto prospective_spec=spec;named(prospective_spec.fields,"specializations_dirty").value=1;j.after_specialization_=bnw_grand_native_component_revision(*e.world,j.player_,prospective_spec);q.frontier=BnwGrandSelectionFrontier::city_specialization_log;}}
 j.after_grand_=bnw_grand_native_component_revision(*e.world,j.player_,prospective_grand);
 j.frontier_=q.frontier==BnwGrandSelectionFrontier::city_specialization_log?"Native Grand SetActive/age and specialization dirty retained; actual GC logging/AI logging and LogSpecializationUpdate required before reason switch and LogGrandStrategies; NO Grand receipt":"Native Grand best/min-age decision retained; actual GC logging/AI logging and LogGrandStrategies required; NO Grand receipt";
 static_assert(std::is_nothrow_move_constructible_v<BnwGrandSelectionPending>);
 // All checks, arithmetic, allocations and prospective guards above. Source
 // primitive stores are preserved at the first actual mandatory logger call.
 if(q.decision.active_written){active.value=q.decision.active_after;age.value=q.decision.age_after;}
 if(dirty)dirty->value=1;
 return j;
}
const BnwGrandSelectionProgress&checked_bnw_grand_selection(const BnwGrandSelectionPending&j,const BnwTeamTechLuaEpoch&e,const BnwGrandWorldAverageRules&r,const BnwHappinessSuffixRules&n,const content::RulesDatabase&db){
 binding(e,j.identity_,j.profile_,r,n,db);need(e.world.get()==j.world_&&e.source_world==j.source_&&e.host.get()==j.host_,"BNW Grand selection exact retained allocations changed");if(j.minimum_age_)(void)j.minimum_age_->require_current(db);
 need(raw_rng(*e.world)==j.raw_&&raw_rng(*e.source_world)==j.raw_&&live_rng(*e.source_world)==j.prefix_&&live_rng(*e.world)==j.live_&&e.world->random.map.algorithm()==RandomAlgorithm::bnw&&e.world->random.gameplay.algorithm()==RandomAlgorithm::bnw&&e.source_world->random.map.algorithm()==RandomAlgorithm::bnw&&e.source_world->random.gameplay.algorithm()==RandomAlgorithm::bnw,"BNW Grand selection retained RNG/source mirrors stale");
 need(bnw_grand_native_component_revision(*e.world,j.player_)==j.after_grand_&&bnw_grand_native_component_revision(*e.source_world,j.player_)==j.source_grand_&&bnw_grand_military_inputs_revision(*e.world)==j.roster_,"BNW Grand selection Grand/source/roster post seals stale");
 if(j.progress_.dirty_written){const auto&spec=named(e.world->world.all_players->players.at(j.player_).strategy.blocks,"city_specialization_ai");need(bnw_grand_native_component_revision(*e.world,j.player_,spec)==j.after_specialization_&&minor(*e.world,j.player_)==*j.progress_.minor,"BNW Grand selection specialization/minor post seal stale");}
 else if(j.progress_.minor)need(minor(*e.world,j.player_)==*j.progress_.minor,"BNW Grand selection observed minor branch changed");
 return j.progress_;
}
}
