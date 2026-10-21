#include "civ5/runtime/bnw_activation_happiness.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <stdexcept>
namespace civ5::runtime {
namespace {
void need(bool b,const char *why){if(!b)throw std::runtime_error(why);}
std::int64_t scalar(const auto &fields,std::string_view name){std::optional<std::int64_t> value;for(const auto &f:fields)if(f.name&&name==f.name){need(!value&&f.values.size()==1,"Activation happiness scalar malformed");value=f.values[0];}need(value.has_value(),"Activation happiness scalar absent");return *value;}
struct RetainedFork final:BnwGameplayLuaEpochOwner{
 std::unique_ptr<BnwTeamTechLuaEpoch> epoch;const LiveSessionState *input;
 BnwGameplayLuaForkResult fork_at(const LiveSessionState &s)override{if(&s!=input||!epoch)return {{},"Activation happiness attempted another Lua fork"};return {std::move(epoch),{}};}
 BnwGameplayLuaPublishResult publish(std::unique_ptr<BnwTeamTechLuaEpoch>,LiveSessionState &)override{return {false,false,"Activation happiness never independently publishes"};}
};
}
const LiveSessionState &BnwActivationHappinessOwner::state()const{
 if(grand_culture_priority_&&grand_culture_priority_->status()==BnwGrandCulturePriorityStatus::discarded)
  throw std::runtime_error("Discarded Culture-priority world requires explicit recovery; older prefix is not current state");
 if(grand_culture_priority_&&grand_culture_priority_->epoch())return *grand_culture_priority_->epoch()->world;
 if(total_culture_&&total_culture_->status()==BnwTotalJonsCultureStatus::discarded)
  throw std::runtime_error("Discarded total-culture world requires explicit recovery; older prefix is not current state");
 if(total_culture_&&total_culture_->epoch())return *total_culture_->epoch()->world;
 return suffix_&&suffix_->continuation.religion.query.epoch?*suffix_->continuation.religion.query.epoch->world:prefix_->state();
}
struct BnwActivationHappinessAccess{
 static BnwActivationHappinessStart start(const LiveSessionState &s,const BnwActivationConnectionsOwner &connections,const content::RulesDatabase &db,const SessionCheckpointIdentity &id,std::int64_t p,std::span<const BnwGameActivationStep> done){
  BnwActivationHappinessStart out;try{
   using S=BnwGameActivationStep;constexpr S order[]={S::set_turn_active,S::publish_end_turn_dirty,S::clear_end_turn,S::unit_attrition,S::active_player_deferred_fog,S::active_turn_count_log,S::publish_player_turn_start,S::all_units_unprocessed,S::pathfinder_cache_enter,S::city_connections,S::city_connection_gold};
   need(done.size()==std::size(order)&&std::equal(done.begin(),done.end(),std::begin(order)),"Activation happiness requires exact completed source prefix; no imported active-turn shortcut");
   need(s.world.pregame&&s.world.all_players&&p==s.world.pregame->archive.integers.at("active_player")&&s.setup.is_human(s.setup.players.at(p))&&scalar(s.world.all_players->players.at(p).core.fields,"m_bTurnActive")==1&&scalar(s.world.all_players->players.at(p).core.fields,"m_bEndTurn")==0&&s.player_read_tail.active_player_ids==std::vector<std::int64_t>{p},"Activation happiness current active-human/clock prefix stale");
   const auto first=std::find_if(s.setup.players.begin(),s.setup.players.end(),[](const auto &r){return r.second.alive;});need(first!=s.setup.players.end()&&first->first==p,"Activation happiness sequential first alive slot differs");
   need(connections.identity.payload_sha256==id.payload_sha256&&connections.identity.rules_sha256==id.rules_sha256&&connections.map_guid==s.world.map_header.guid&&connections.players.at(p).updated_turn==s.setup.game_turn,"Activation happiness requires SAME source current CityConnections owner");
   auto pr=BnwHappinessPrefixRules::load(db);auto sr=BnwHappinessSuffixRules::load(db);auto rr=BnwReligionHappinessRules::load(db);auto client=start_bnw_happiness_game_data_client(s,pr,id,p);auto token=attest_bnw_happiness_prefix_input(s,pr,id,p);auto prefix=execute_bnw_happiness_native_prefix(s,token,pr);
   need(static_cast<bool>(prefix.prefix),prefix.frontier.c_str());out.owner=std::unique_ptr<BnwActivationHappinessOwner>(new BnwActivationHappinessOwner(std::move(prefix.prefix),std::move(pr),std::move(sr),std::move(rr),std::move(client)));out.owner->frontier_="Native source happiness prefix retained once; mandatory selected religion Lua epoch required";
  }catch(const std::exception &e){out.blocker=e.what();}return out;
 }
 static LiveActionResult resume(BnwActivationHappinessOwner &o,const SavedContentProfile &profile,BnwGameplayLuaEpochOwner &lua,const content::RulesDatabase &db){
  if(o.stage_!=BnwActivationHappinessStage::native_prefix&&o.stage_!=BnwActivationHappinessStage::awaiting_selected_lua)return {LiveActionOutcome::rejected,"Activation happiness already dispatched or completed; no repeat DoUpdate"};
  try{
   need(o.prefix_->identity().rules_sha256==identify_bytes(db.gameplay_fingerprint_data()).sha256,"Activation happiness retained rules epoch stale");
   auto fork=lua.fork_at(o.prefix_->state());
   if(!fork.epoch){o.stage_=BnwActivationHappinessStage::awaiting_selected_lua;o.frontier_=fork.blocker;return {LiveActionOutcome::rejected,o.frontier_};}
   const auto &e=*fork.epoch;
   if(!e.world||!e.host||e.source_world!=&o.prefix_->state()||e.getter_world!=e.world.get()||!e.all_selected_contexts_loaded||!e.all_listeners_registered||!e.all_external_writes_escrowed||!e.host->loading_complete()||e.selected_dlc!=profile.enabled_dlc||e.selected_mods!=profile.enabled_mods||e.identity.payload_sha256!=o.prefix_->identity().payload_sha256||e.identity.rules_sha256!=o.prefix_->identity().rules_sha256){o.stage_=BnwActivationHappinessStage::awaiting_selected_lua;o.frontier_="Actual selected Lua epoch incomplete/stale; sealed native prefix retained without dispatch";return {LiveActionOutcome::rejected,o.frontier_};}
   RetainedFork retained;retained.input=&o.prefix_->state();retained.epoch=std::move(fork.epoch);++o.religion_dispatches_;
   auto suffix=execute_bnw_happiness_remaining_suffix(*o.prefix_,o.prefix_rules_,profile,retained,o.religion_rules_,o.suffix_rules_,o.client_,db);
   if(!suffix.native_cache_suffix_complete){o.stage_=BnwActivationHappinessStage::failed;o.frontier_=suffix.frontier;return {LiveActionOutcome::rejected,o.frontier_};}
   o.suffix_=std::make_unique<BnwHappinessSuffixExecution>(std::move(suffix));o.stage_=BnwActivationHappinessStage::native_suffix_dirty_pending;o.frontier_=o.suffix_->frontier;
   return {LiveActionOutcome::committed,"Complete native happiness stores retained on sole private activation Lua/world epoch; deferred UI and outer publication pending"};
  }catch(const std::exception &e){o.stage_=BnwActivationHappinessStage::failed;o.frontier_=e.what();return {LiveActionOutcome::rejected,o.frontier_};}
 }
 static LiveActionResult consume(BnwActivationHappinessOwner &o,const content::RulesDatabase &db){
  if(o.grand_first_roll_)return {LiveActionOutcome::rejected,"Happiness UI cannot consume the retained epoch before remaining Grand source steps"};
  if(o.grand_culture_priority_&&(o.grand_culture_priority_->status()!=BnwGrandCulturePriorityStatus::complete||o.grand_culture_priority_->epoch()))return {LiveActionOutcome::rejected,"Happiness UI cannot consume a pending/discarded Culture-priority epoch"};
  if(o.total_culture_&&(o.total_culture_->status()!=BnwTotalJonsCultureStatus::complete||o.total_culture_->epoch()))return {LiveActionOutcome::rejected,"Happiness UI cannot consume a pending/discarded total-culture epoch"};
  if(o.stage_!=BnwActivationHappinessStage::native_suffix_dirty_pending||!o.suffix_)return {LiveActionOutcome::rejected,"Activation happiness has no pending UI packet or it was already consumed"};
  auto pumped=pump_bnw_happiness_game_data(std::move(*o.suffix_),db);o.suffix_.reset();o.frontier_=std::move(pumped.frontier);
  if(pumped.pending){o.suffix_=std::move(pumped.pending);return {LiveActionOutcome::rejected,o.frontier_};}
  if(!pumped.staged){o.stage_=BnwActivationHappinessStage::failed;return {LiveActionOutcome::rejected,o.frontier_};}
  o.suffix_=std::move(pumped.staged);o.stage_=BnwActivationHappinessStage::ui_consumed;return {LiveActionOutcome::committed,o.frontier_};
 }
};
BnwActivationHappinessStart stage_bnw_activation_happiness(const LiveSessionState &s,const BnwActivationConnectionsOwner &c,const content::RulesDatabase &db,const SessionCheckpointIdentity &id,std::int64_t p,std::span<const BnwGameActivationStep> done){return BnwActivationHappinessAccess::start(s,c,db,id,p,done);}
LiveActionResult continue_bnw_activation_happiness(BnwActivationHappinessOwner &o,const SavedContentProfile &p,BnwGameplayLuaEpochOwner &lua,const content::RulesDatabase &db){return BnwActivationHappinessAccess::resume(o,p,lua,db);}
LiveActionResult consume_bnw_activation_happiness_ui(BnwActivationHappinessOwner &o,const content::RulesDatabase &db){return BnwActivationHappinessAccess::consume(o,db);}
}
