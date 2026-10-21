#include "civ5/runtime/bnw_unit_order_owners.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <stdexcept>
namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void need(bool value,const char *why){if(!value)throw std::runtime_error(why);}
template<class Core>std::int64_t scalar(const Core &core,std::string_view name) {
    std::optional<std::int64_t> result;
    for(const auto &field:core.fields)if(field.name && std::string_view(field.name)==name) {
        need(!result && field.values.size()==1,"Unknown/duplicate unit-command scalar");result=field.values[0];
    }
    need(result.has_value(),"Missing unit-command scalar");return *result;
}
bool same(const SessionCheckpointIdentity &a,const SessionCheckpointIdentity &b) {
    return a.payload_sha256==b.payload_sha256 && a.rules_sha256==b.rules_sha256;
}
void number(std::vector<std::uint8_t> &bytes,std::uint64_t value){for(unsigned n=0;n<8;++n)bytes.push_back(static_cast<std::uint8_t>(value>>(n*8)));}
void text(std::vector<std::uint8_t> &bytes,std::string_view value){number(bytes,value.size());bytes.insert(bytes.end(),value.begin(),value.end());}
template<class Field>void hashed_field(std::vector<std::uint8_t>&bytes,const Field &field,std::optional<std::int32_t>projection=std::nullopt) {
    text(bytes,field.name?field.name:"");number(bytes,field.wire_count.has_value());if(field.wire_count)number(bytes,*field.wire_count);
    number(bytes,field.values.size());if(projection){need(field.values.size()==1,"Flags revision projection requires one actual scalar");number(bytes,*projection);}else for(auto value:field.values)number(bytes,value);
    number(bytes,field.bytes.size());bytes.insert(bytes.end(),field.bytes.begin(),field.bytes.end());
}
void hashed_tail(std::vector<std::uint8_t>&bytes,const saves::UnitTailNode &node) {
    hashed_field(bytes,node);text(bytes,node.encoding?node.encoding:"");number(bytes,node.children.size());for(const auto &child:node.children)hashed_tail(bytes,child);
}
const saves::SavedUnitObservation &find_unit(const LiveSessionState &state,BnwUnitOrderKey key) {
    need(state.world.all_players && key.first>=0 && key.first<64 && key.second>=0,"Unknown unit-command roster/key");
    const auto &player=state.world.all_players->players.at(key.first);need(player.saved_player_id==key.first,"Unit-command player index differs");
    const saves::SavedUnitObservation *found=nullptr;
    for(const auto &unit:player.units.units)if(scalar(unit.core,"m_iID")==key.second){need(!found,"Duplicate unit-command ID");found=&unit;}
    need(found && found->core.layout==bnw && found->tail.layout==bnw && scalar(found->core,"m_eOwner")==key.first,"Unit-command actual unit is absent");
    return *found;
}
void binding(const LiveSessionState &state,const SessionCheckpointIdentity &identity,const content::RulesDatabase &rules) {
    need(state.setup.profile==content::TechnologyProfile::bnw && state.world.game_state.layout==bnw && state.world.all_players &&
         state.world.all_players->layout==bnw && state.world.all_players->players.size()==64 && state.world.all_players->context.player_count==64 &&
         state.world.pregame && state.world.pregame->layout==bnw && state.world.map_projection &&
         identity.payload_sha256.size()==64 && identity.rules_sha256==rules.gameplay_fingerprint_sha256(),
         "Unit-command source/rules/world binding differs");
    need(state.world.pregame->archive.integers.at("game_turn")==state.setup.game_turn,"Unit-command clock differs");
}
}
void BnwUnitPathOwner::clear(BnwUnitOrderKey key) {
    auto &path=paths_.at(key);need(path.clear_operations<std::numeric_limits<std::uint64_t>::max(),"Unit path-clear receipt exhausted");
    path.nodes.clear();path.destination=std::numeric_limits<std::uint32_t>::max();++path.clear_operations;
}
BnwUnitPathOwner bind_bnw_unit_paths_original_import(const saves::SavedSession &source,const SessionCheckpointIdentity &identity) {
    // Original payload is retained by both actual file readers. Native Reset
    // recreates unsaved empty paths; this never recovers a pending mission or
    // cached finder permission from a detached scalar overlay.
    need((source.source.container==saves::SaveContainerKind::civ5||source.source.container==saves::SaveContainerKind::civis) && identify_bytes(source.source.bytes).sha256==identity.payload_sha256 &&
         source.snapshot.all_players && source.snapshot.all_players->layout==bnw && source.snapshot.all_players->players.size()==64,
         "BNW unit path reset needs original import identity");
    if(source.source.container==saves::SaveContainerKind::civis)
        need(source.source.source_sha256&&source.source.source_sha256->size()==64&&std::all_of(source.source.source_sha256->begin(),source.source.source_sha256->end(),[](char x){return (x>='0'&&x<='9')||(x>='a'&&x<='f');})&&source.source.source_prefix&&!source.source.source_prefix->empty()&&source.source.inflate.stream_end&&source.source.inflate.available_bytes==source.source.bytes.size(),"BNW original-payload path reset Civis source provenance unavailable");
    BnwUnitPathOwner owner(identity,source.snapshot.map_header.guid);
    for(const auto &player:source.snapshot.all_players->players)for(const auto &unit:player.units.units) {
        need(unit.core.layout==bnw && unit.tail.layout==bnw && scalar(unit.core,"m_eOwner")==player.saved_player_id,
             "BNW original unit path reset roster differs");
        need(owner.paths_.emplace(BnwUnitOrderKey{player.saved_player_id,scalar(unit.core,"m_iID")},BnwUnitRetainedPath{}).second,
             "BNW original unit path reset has duplicate ID");
    }
    return owner;
}
namespace {
std::string order_revision(const LiveSessionState &state,const SessionCheckpointIdentity &identity,const content::RulesDatabase &rules,std::optional<std::pair<BnwUnitOrderKey,std::int32_t>>projection) {
    std::size_t projected=0;
    binding(state,identity,rules);std::vector<std::uint8_t> bytes;
    text(bytes,identity.payload_sha256);text(bytes,identity.rules_sha256);bytes.insert(bytes.end(),state.world.map_header.guid.begin(),state.world.map_header.guid.end());
    number(bytes,state.setup.game_turn);number(bytes,state.world.pregame->archive.integers.at("active_player"));
    for(const auto &player:state.world.all_players->players) {
        number(bytes,player.index);number(bytes,player.saved_player_id);number(bytes,scalar(player.core,"m_bAlive"));number(bytes,scalar(player.core,"m_bTurnActive"));
        number(bytes,player.units.units.size());
        for(const auto &unit:player.units.units){number(bytes,unit.index);number(bytes,unit.core.fields.size());
            for(const auto &field:unit.core.fields){std::optional<std::int32_t>value;
                if(projection&&player.saved_player_id==projection->first.first&&scalar(unit.core,"m_iID")==projection->first.second&&field.name&&std::string_view(field.name)=="m_iFlags"){
                    need(field.encoding==saves::UnitCoreEncoding::i32&&field.values.size()==1&&field.bytes.empty()&&++projected==1,"Flags revision projection actual field unavailable/duplicate");value=projection->second;
                }
                hashed_field(bytes,field,value);number(bytes,static_cast<unsigned>(field.encoding));}
            number(bytes,unit.tail.blocks.size());for(const auto &node:unit.tail.blocks)hashed_tail(bytes,node);}
    }
    const auto &map=*state.world.map_projection;number(bytes,map.grid.width);number(bytes,map.grid.height);number(bytes,map.cells.size());
    for(const auto &cell:map.cells){number(bytes,cell.units.has_value());if(cell.units){number(bytes,cell.units->size());for(const auto &unit:*cell.units){number(bytes,unit.owner);number(bytes,unit.id);}}}
    need(!projection||projected==1,"Flags revision projection actual unit absent");return identify_bytes(bytes).sha256;
}
}
std::string bnw_unit_order_world_revision(const LiveSessionState&state,const SessionCheckpointIdentity&identity,const content::RulesDatabase&rules){return order_revision(state,identity,rules,std::nullopt);}
std::string BnwUnitPathOwner::project_unit_flags_revision(const LiveSessionState&state,const SessionCheckpointIdentity&identity,const content::RulesDatabase&rules,BnwUnitOrderKey key,std::int32_t value){return order_revision(state,identity,rules,std::pair{key,value});}
std::uint64_t bnw_unit_order_plot(const LiveSessionState &state,BnwUnitOrderKey key) {
    const auto &unit=find_unit(state,key);need(state.world.map_projection.has_value(),"Unit-command map is unknown");
    const auto &map=*state.world.map_projection;const auto x=scalar(unit.core,"m_iX"),y=scalar(unit.core,"m_iY");
    need(x>=0 && y>=0 && static_cast<std::uint64_t>(x)<map.grid.width && static_cast<std::uint64_t>(y)<map.grid.height,"Unit-command plot is invalid");
    const auto index=map.grid.index(x,y);need(index<map.cells.size() && map.cells[index].units &&
        std::count_if(map.cells[index].units->begin(),map.cells[index].units->end(),[&](const auto &ref){return ref.owner==key.first && ref.id==key.second;})==1,
        "Unit-command map and player unit reference differ");return index;
}
BnwUnitPresentationOwner start_bnw_unit_presentation_client(const LiveSessionState &state,const SessionCheckpointIdentity &identity,const content::RulesDatabase &rules) {
    BnwUnitPresentationOwner owner(identity,state.world.map_header.guid,bnw_unit_order_world_revision(state,identity,rules));
    for(const auto &player:state.world.all_players->players)for(const auto &unit:player.units.units) {
        const BnwUnitOrderKey key{player.saved_player_id,scalar(unit.core,"m_iID")};(void)bnw_unit_order_plot(state,key);
        need(owner.dim_flags_.emplace(key,scalar(unit.core,"m_iMoves")<=0).second,"Duplicate new-client unit flag");
    }
    return owner;
}
void select_bnw_unit_presentation(const LiveSessionState &state,BnwUnitPresentationOwner &owner,const content::RulesDatabase &rules,BnwUnitOrderKey key,std::uint64_t expected) {
    need(expected==owner.client_revision_ && expected<std::numeric_limits<std::uint64_t>::max() &&
         owner.world_revision_==bnw_unit_order_world_revision(state,owner.identity_,rules) && owner.map_guid_==state.world.map_header.guid,
         "Unit selection has stale client/world revision");
    const auto &unit=find_unit(state,key);const auto &player=state.world.all_players->players.at(key.first);
    need(state.world.pregame->archive.integers.at("active_player")==key.first && scalar(player.core,"m_bAlive")==1 &&
         state.setup.is_human(state.setup.players.at(key.first)) && scalar(unit.core,"m_bDeathDelay")==0,
         "New-client selection requires the actual live local-human unit");
    const auto plot=bnw_unit_order_plot(state,key);auto draft=owner;
    draft.selected_unit_=key;draft.selection_plot_=plot;draft.unit_info_dirty_=true;draft.plot_list_dirty_=true;draft.selection_buttons_dirty_=true;
    ++draft.client_revision_;owner=std::move(draft);
}
void apply_bnw_unit_presentation_batch(const LiveSessionState &state,BnwUnitPresentationOwner &owner,const content::RulesDatabase &rules,const BnwUnitPresentationBatch &batch) {
    need(same(owner.identity_,batch.identity) && owner.map_guid_==batch.map_guid && batch.map_guid==state.world.map_header.guid &&
         owner.world_revision_==batch.before_world_revision && owner.client_revision_==batch.expected_client_revision &&
         owner.client_revision_<std::numeric_limits<std::uint64_t>::max() && batch.after_world_revision==bnw_unit_order_world_revision(state,owner.identity_,rules),
         "Unit presentation feed has stale client/world/source binding");
    auto draft=owner;
    for(const auto &effect:batch.effects) {
        need(draft.dim_flags_.contains(effect.unit),"Unit presentation effect refers to an unknown flag");
        switch(effect.kind) {
          case BnwUnitPresentationEffectKind::should_dim_flag:
            need(effect.value==0 || effect.value==1,"Unit dim effect is not boolean");
            need((effect.value!=0)==(scalar(find_unit(state,effect.unit).core,"m_iMoves")<=0),"Unit setMoves/load dim effect differs from actual after-world movement");
            draft.dim_flags_.at(effect.unit)=effect.value!=0;break;
          case BnwUnitPresentationEffectKind::dirty_unit_info:need(effect.value==1,"UnitInfo dirty effect must be true");draft.unit_info_dirty_=true;break;
          case BnwUnitPresentationEffectKind::dirty_plot_list_buttons:need(effect.value==1,"PlotList dirty effect must be true");draft.plot_list_dirty_=true;break;
          case BnwUnitPresentationEffectKind::dirty_selection_buttons:need(effect.value==1,"Selection dirty effect must be true");draft.selection_buttons_dirty_=true;break;
          case BnwUnitPresentationEffectKind::dirty_waypoints:need(effect.value==1,"Waypoints dirty effect must be true");draft.waypoints_dirty_=true;break;
          case BnwUnitPresentationEffectKind::cycle_selection_delta:
            need(effect.value>=0 && draft.cycle_selection_counter_<=std::numeric_limits<std::int64_t>::max()-effect.value,"Unit selection counter overflow");
            draft.cycle_selection_counter_+=effect.value;break;
          case BnwUnitPresentationEffectKind::has_moved_unit:need(effect.value==1,"HasMoved unit effect must be true");draft.has_moved_unit_=true;break;
          default:throw std::runtime_error("Unknown unit presentation effect");
        }
        draft.received_effects_.push_back(effect);
    }
    draft.world_revision_=batch.after_world_revision;++draft.client_revision_;owner=std::move(draft);
}
} // namespace civ5::runtime
