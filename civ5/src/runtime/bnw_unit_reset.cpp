#include "civ5/runtime/bnw_unit_reset.hpp"
#include "civ5/runtime/map_geometry.hpp"
#include <climits>
#include <stdexcept>
namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void need(bool b,const char *why){if(!b)throw std::runtime_error(why);}
std::int64_t &scalar(saves::UnitCoreObservation &core,std::string_view name) {
 saves::UnitCoreField *found=nullptr;
 for(auto &f:core.fields)if(f.name && std::string_view(f.name)==name){need(!found && f.values.size()==1,"Unknown unit reset scalar");found=&f;}
 need(found && (found->encoding==saves::UnitCoreEncoding::i32 || found->encoding==saves::UnitCoreEncoding::boolean),"Missing native unit reset scalar");
 need(found->values[0]>=INT32_MIN && found->values[0]<=INT32_MAX,"Unit reset operand exceeds native int");return found->values[0];
}
std::int64_t scalar(const saves::UnitCoreObservation &core,std::string_view n){return scalar(const_cast<saves::UnitCoreObservation&>(core),n);}
const saves::SavedUnitObservation &unit(const LiveSessionState &state,BnwUnitOrderKey key) {
 (void)bnw_unit_order_plot(state,key);const saves::SavedUnitObservation *found=nullptr;
 for(const auto &u:state.world.all_players->players.at(key.first).units.units)if(scalar(u.core,"m_iID")==key.second){need(!found,"Duplicate reset unit");found=&u;}
 need(found,"Reset unit missing");return *found;
}
std::int64_t field(const saves::PlotObservation &plot,std::string_view name) {
 const saves::NamedSaveInteger *found=nullptr;
 for(const auto &f:plot.fields)if(f.name && std::string_view(f.name)==name){need(!found,"Duplicate citadel plot field");found=&f;}
 need(found,"Missing citadel plot field");return found->value;
}
std::int64_t improvement(const saves::PlotObservation &raw,const saves::SavedMapCellProjection &cell,const content::InfoTypeCatalog &types) {
 need(raw.version && *raw.version>=1 && *raw.version<=7,"Unknown citadel plot version");
 if(*raw.version>=5) {
  const auto hash=field(raw,"improvement_type_hash");need(hash>=0 && hash<=UINT32_MAX && cell.improvement_type_hash && *cell.improvement_type_hash==static_cast<std::uint64_t>(hash),"Citadel raw/projected improvement hash differs");
  if(!hash){need(!cell.improvement_type || *cell.improvement_type==-1,"Citadel no-improvement projection differs");return -1;}
  const auto *info=types.find_hash(hash);need(info && info->table=="Improvements" && info->id>=0 && (!cell.improvement_type || *cell.improvement_type==info->id),"Citadel improvement hash/rules binding unknown");return info->id;
 }
 const auto id=field(raw,"improvement_type");need(id>=-1 && cell.improvement_type && *cell.improvement_type==id,"Citadel legacy improvement projection differs");return id;
}
std::int32_t nearby_damage(const content::RulesDatabase &rules,std::int64_t id) {
 std::optional<std::int64_t> value;const auto q=rules.query_rows({"SELECT NearbyEnemyDamage FROM Improvements WHERE ID=?",{id}},[&](const auto &row){need(!value,"Duplicate citadel improvement rules");value=row.integer_at(0);},2);
 need(q.all_succeeded() && value && *value>=INT32_MIN && *value<=INT32_MAX,"Citadel native NearbyEnemyDamage rules unavailable");return static_cast<std::int32_t>(*value);
}
std::int64_t &ignore_danger(saves::UnitTailObservation &tail) {
 saves::UnitTailNode *found=nullptr;
 const auto visit=[&](auto &&self,saves::UnitTailNode &node)->void{if(node.name && std::string_view(node.name)=="m_bIgnoreDangerWakeup"){need(!found && node.values.size()==1,"Unknown ignore-danger unit tail");found=&node;}for(auto &child:node.children)self(self,child);};
 for(auto &node:tail.blocks)visit(visit,node);
 need(found && found->encoding && std::string_view(found->encoding)=="bool" && (found->values[0]==0 || found->values[0]==1),"Missing native ignore-danger wakeup state");return found->values[0];
}
}
BnwEnemyCitadelQuote quote_bnw_near_enemy_citadel(const LiveSessionState &state,const content::RulesDatabase &rules,
 const content::InfoTypeCatalog &types,const SessionCheckpointIdentity &identity,BnwUnitOrderKey key) {
 BnwEnemyCitadelQuote out;
 try {
  (void)bnw_unit_order_world_revision(state,identity,rules);const auto &u=unit(state,key);
  need(types.report().all_succeeded() && state.world.map_plots && state.world.map_plots->layout==bnw,"Citadel current map/rules unknown");
  const auto &map=*state.world.map_projection;need(state.world.map_plots->plots.size()==map.cells.size(),"Citadel raw/projected map span differs");
  MapGeometry geometry{map.grid,map.wrap_x,map.wrap_y,bnw};geometry.validate();
  const auto x=scalar(u.core,"m_iX"),y=scalar(u.core,"m_iY");
  const auto half=[](std::int64_t v){return v>=0?v/2:(v-1)/2;};
  for(int dx=-1;dx<=1;++dx)for(int dy=-1;dy<=1;++dy) {
   const auto distance=(dx>=0)==(dy>=0)?std::abs(dx)+std::abs(dy):std::max(std::abs(dx),std::abs(dy));if(distance>1)continue;
   const auto py=y+dy;const auto px=x-half(y)+dx+half(py);const auto index=geometry.plot(px,py);if(!index)continue;
   out.visited_plots.push_back(*index);const auto &cell=map.cells.at(*index);const auto &raw=state.world.map_plots->plots.at(*index);
   need(cell.index==static_cast<std::uint64_t>(*index) && cell.x==static_cast<std::uint64_t>(*index)%map.grid.width && cell.y==static_cast<std::uint64_t>(*index)/map.grid.width && field(raw,"source_x")==static_cast<std::int64_t>(cell.x) && field(raw,"source_y")==static_cast<std::int64_t>(cell.y),"Citadel raw/projected source coordinates differ");
   const auto id=improvement(raw,cell,types);if(id==-1)continue;
   const auto pillaged=field(raw,"improvement_pillaged");need((pillaged==0 || pillaged==1) && cell.improvement_pillaged && *cell.improvement_pillaged==(pillaged!=0),"Citadel pillage state unknown or mismatched");if(pillaged)continue;
   const auto damage=nearby_damage(rules,id);if(!damage)continue;
   const auto owner=field(raw,"owner");need(owner>=-1 && owner<64 && cell.owner && *cell.owner==owner,"Citadel owner state unknown or mismatched");if(owner==-1)continue;
   const auto own_team=state.setup.players.at(key.first).team,their_team=state.setup.players.at(owner).team;
   need(own_team>=0 && own_team<64 && their_team>=0 && their_team<64 && state.world.teams && state.world.teams->layout==bnw && state.world.teams->teams.size()==64,"Citadel current player/team binding unknown");
   const saves::PlotArrayObservation *war=nullptr;for(const auto &a:state.world.teams->teams.at(own_team).arrays)if(a.name && std::string_view(a.name)=="at_war"){need(!war,"Duplicate citadel war array");war=&a;}
   need(war && war->columns==1 && war->values.size()==64 && (war->values.at(their_team)==0 || war->values.at(their_team)==1),"Citadel native current war state unknown");
   if(war->values.at(their_team)){out.near_enemy_citadel=true;out.damage=damage;out.damaging_plot=*index;return out;}
  }
  out.near_enemy_citadel=false;
 } catch(const std::exception &e){out.near_enemy_citadel.reset();out.damage.reset();out.damaging_plot.reset();out.blocker=e.what();}
 return out;
}
static BnwHumanUnitResetExecution reset_component(const BnwEndTurnRequestDraft *original_active,const BnwEndTurnRequestDraft &input,
 const BnwEndTurnInputBinding &binding,const BnwUnitPresentationOwner &client,const content::RulesDatabase &rules,
 const content::InfoTypeCatalog &types,std::int64_t player) {
 BnwHumanUnitResetExecution out;
 try {
  if(original_active) {
   (void)bnw_end_turn_input_revision(original_active->state,original_active->owner,binding,rules,player);
   auto expected=original_active->state;for(auto &f:expected.world.all_players->players.at(player).core.fields)if(f.name && std::string_view(f.name)=="m_bTurnActive")f.values.at(0)=0;
   need(input.owner.identity.payload_sha256==original_active->owner.identity.payload_sha256 && input.owner.identity.rules_sha256==original_active->owner.identity.rules_sha256 && input.owner.processed_auto_moves==original_active->owner.processed_auto_moves && input.owner.completed==original_active->owner.completed && input.owner.pending.empty(),"Unit reset deactivation retained owner differs");
   need(bnw_unit_order_world_revision(input.state,input.owner.identity,rules)==bnw_unit_order_world_revision(expected,original_active->owner.identity,rules),"Unit reset deactivation unit epoch changed before source entry");
   std::optional<std::int64_t> blocking_type,blocking_index;
   const auto inspect=[&](auto &&self,const saves::PlayerAdministrationRecord &r)->void{for(const auto &f:r.fields)if(f.name){const auto n=std::string_view(f.name);if(n=="m_eEndTurnBlockingType"){need(!blocking_type,"Duplicate reset blocking type");blocking_type=f.value;}if(n=="m_iEndTurnBlockingNotificationIndex"){need(!blocking_index,"Duplicate reset blocking index");blocking_index=f.value;}}for(const auto &c:r.children)self(self,c);};
   for(const auto &r:input.state.world.all_players->players.at(player).administration.blocks)inspect(inspect,r);
   need(blocking_type==-1 && blocking_index==-1,"Unit reset deactivation must follow actual serialized blocking clear");
  } else (void)bnw_end_turn_input_revision(input.state,input.owner,binding,rules,player);
  const auto core_value=[&](std::string_view n){std::optional<std::int64_t> v;for(const auto &f:input.state.world.all_players->players.at(player).core.fields)if(f.name && std::string_view(f.name)==n){need(!v && f.values.size()==1,"Unknown reset player scalar");v=f.values[0];}need(v.has_value(),"Missing reset player scalar");return *v;};
  need(core_value("m_bEndTurn")==1 && input.owner.processed_auto_moves.at(player),"DoUnitReset requires actual EndTurn and SAME processed automoves owner");
  need(client.world_revision()==bnw_unit_order_world_revision(input.state,input.owner.identity,rules),"Unit reset has stale actual client world");
  auto d=std::make_unique<BnwHumanUnitResetDraft>(BnwHumanUnitResetDraft{input,client,{},{}});auto &request=d->request;auto &batch=d->batch;
  batch.identity=input.owner.identity;batch.map_guid=input.state.world.map_header.guid;batch.before_world_revision=client.world_revision();batch.expected_client_revision=client.client_revision();
  for(auto &u:request.state.world.all_players->players.at(player).units.units) {
   const BnwUnitOrderKey key{player,scalar(u.core,"m_iID")};const auto movement=quote_bnw_unit_movement(request.state,rules,types,request.owner.identity,key);
   need(movement.max_moves.has_value(),movement.blocker.c_str());
   const auto embarked=scalar(u.core,"m_bEmbarked");need(embarked==0 || embarked==1,"Unit reset embark state unknown");
   if(!embarked){const bool heal=*movement.has_moved?scalar(u.core,"m_iAlwaysHealCount")>0:scalar(u.core,"m_iDamage")>0;need(!heal,"DoUnitReset requires actual doHeal world/presentation owner");}
   const auto citadel=quote_bnw_near_enemy_citadel(request.state,rules,types,request.owner.identity,key);need(citadel.near_enemy_citadel.has_value(),citadel.blocker.c_str());need(!*citadel.near_enemy_citadel,"DoUnitReset requires actual citadel changeDamage owner");
   const auto plot=bnw_unit_order_plot(request.state,key);auto &moves=scalar(u.core,"m_iMoves");
   if(moves!=*movement.max_moves) {
    moves=*movement.max_moves;batch.effects.push_back({BnwUnitPresentationEffectKind::should_dim_flag,key,moves<=0});
    if(client.selected_unit()==key){need(request.owner.pathfinder_cache.has_value(),"Selected unit reset requires actual common finder owner");force_reset_bnw_update_moves_common_cache(request.state,*request.owner.pathfinder_cache,player);batch.effects.push_back({BnwUnitPresentationEffectKind::dirty_unit_info,key,1});}
    if(client.selection_plot()==plot)batch.effects.push_back({BnwUnitPresentationEffectKind::dirty_plot_list_buttons,key,1});
   }
   // Movement query already excludes the source GreatGeneral branch.
   const auto ai=scalar(u.core,"m_eUnitAIType");const auto *archaeologist=types.find("UNITAI_ARCHAEOLOGIST");need(archaeologist && archaeologist->table=="UnitAIInfos" && archaeologist->id==39,"Unit reset archaeologist source enum unknown");
   need(ai!=archaeologist->id,"DoUnitReset requires actual culture HasDigCompleteHere owner");
   ignore_danger(u.tail)=0;scalar(u.core,"m_iAttacksMade")=0;scalar(u.core,"m_iMadeInterceptionCount")=0;d->reset_units.push_back(key);
  }
  batch.after_world_revision=bnw_unit_order_world_revision(request.state,request.owner.identity,rules);
  // Nonpresentation native stores still bind the client to the new epoch.
  if(batch.before_world_revision!=batch.after_world_revision || !batch.effects.empty())apply_bnw_unit_presentation_batch(request.state,d->presentation,rules,batch);
  if(original_active)request.input_revision.clear();
  else request.input_revision=bnw_end_turn_input_revision(request.state,request.owner,binding,rules,player);
  out.draft=std::move(d);
 } catch(const std::exception &e){out.blocker=e.what();}
 return out;
}
BnwHumanUnitResetExecution execute_bnw_human_unit_reset_draft(const BnwEndTurnRequestDraft &input,const BnwEndTurnInputBinding &binding,const BnwUnitPresentationOwner &client,const content::RulesDatabase &rules,const content::InfoTypeCatalog &types,std::int64_t player) {
 return reset_component(nullptr,input,binding,client,rules,types,player);
}
namespace bnw_unit_reset_detail {
BnwHumanUnitResetExecution execute_after_turn_flag_draft(const BnwEndTurnRequestDraft &before,const BnwEndTurnRequestDraft &input,const BnwEndTurnInputBinding &binding,const BnwUnitPresentationOwner &client,const content::RulesDatabase &rules,const content::InfoTypeCatalog &types,std::int64_t player) {
 return reset_component(&before,input,binding,client,rules,types,player);
}
}
} // namespace civ5::runtime
