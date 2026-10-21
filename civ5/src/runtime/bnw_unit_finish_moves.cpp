#include "civ5/runtime/bnw_unit_finish_moves.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <limits>
#include <stdexcept>
namespace civ5::runtime {
namespace {
void need(bool value,const char *why){if(!value)throw std::runtime_error(why);}
std::int64_t &scalar(saves::UnitCoreObservation &core,std::string_view name) {
    saves::UnitCoreField *found=nullptr;
    for(auto &field:core.fields)if(field.name && std::string_view(field.name)==name){need(!found && field.values.size()==1,"Unknown FinishMoves scalar");found=&field;}
    need(found && (found->encoding==saves::UnitCoreEncoding::i32 || found->encoding==saves::UnitCoreEncoding::boolean),"Missing/wrong FinishMoves scalar");
    return found->values.front();
}
std::int64_t scalar(const saves::UnitCoreObservation &core,std::string_view name){return scalar(const_cast<saves::UnitCoreObservation&>(core),name);}
saves::SavedUnitObservation &unit(LiveSessionState &state,BnwUnitOrderKey key) {
    (void)bnw_unit_order_plot(state,key);saves::SavedUnitObservation *found=nullptr;
    for(auto &candidate:state.world.all_players->players.at(key.first).units.units)if(scalar(candidate.core,"m_iID")==key.second){need(!found,"Duplicate FinishMoves unit");found=&candidate;}
    need(found,"FinishMoves unit absent");return *found;
}
const saves::SavedUnitObservation &unit(const LiveSessionState &state,BnwUnitOrderKey key){return unit(const_cast<LiveSessionState&>(state),key);}
void append(std::vector<std::uint8_t>&bytes,std::uint64_t value){for(unsigned i=0;i<8;++i)bytes.push_back(static_cast<std::uint8_t>(value>>(i*8)));}
}
std::string bnw_finish_moves_input_revision(const BnwEndTurnRequestDraft &request,const BnwEndTurnInputBinding &binding,
    const BnwUnitPresentationOwner &client,const content::RulesDatabase &rules,BnwUnitOrderKey key) {
    const auto root_revision=bnw_end_turn_input_revision(request.state,request.owner,binding,rules,key.first);
    const auto world_revision=bnw_unit_order_world_revision(request.state,request.owner.identity,rules);
    need(client.identity().payload_sha256==request.owner.identity.payload_sha256 && client.identity().rules_sha256==request.owner.identity.rules_sha256 &&
         client.map_guid()==request.state.world.map_header.guid && client.world_revision()==world_revision && client.dim_flags().contains(key),
         "FinishMoves actual presentation owner has stale world/source inputs");
    const auto &actual=unit(request.state,key);
    need(scalar(actual.core,"m_eOwner")==key.first && scalar(actual.core,"m_bDeathDelay")==0,
         "FinishMoves command requires an actual live owned unit");
    const auto moves=scalar(actual.core,"m_iMoves");need(moves>=INT32_MIN && moves<=INT32_MAX,"FinishMoves moves exceed native int");
    std::vector<std::uint8_t> bytes(root_revision.begin(),root_revision.end());bytes.insert(bytes.end(),world_revision.begin(),world_revision.end());
    append(bytes,key.first);append(bytes,key.second);append(bytes,client.client_revision());
    append(bytes,client.selected_unit().has_value());if(client.selected_unit()){append(bytes,client.selected_unit()->first);append(bytes,client.selected_unit()->second);}
    append(bytes,client.selection_plot().has_value());if(client.selection_plot())append(bytes,*client.selection_plot());
    return identify_bytes(bytes).sha256;
}
BnwFinishMovesExecution execute_bnw_finish_moves_draft(const BnwEndTurnRequestDraft &input,const BnwEndTurnInputBinding &binding,
    const BnwUnitPresentationOwner &client,const content::RulesDatabase &rules,const BnwFinishMovesCommand &command) {
    BnwFinishMovesExecution result;
    try {
        need(command.expected_turn==input.state.setup.game_turn && command.expected_client_revision==client.client_revision() &&
             command.expected_input_revision==bnw_finish_moves_input_revision(input,binding,client,rules,command.unit),"Stale FinishMoves typed command");
        auto draft=std::make_unique<BnwFinishMovesDraft>(BnwFinishMovesDraft{input,client,{}});
        auto &request=draft->request;auto &batch=draft->batch;
        batch.identity=request.owner.identity;batch.map_guid=request.state.world.map_header.guid;batch.before_world_revision=client.world_revision();batch.expected_client_revision=client.client_revision();
        const auto plot=bnw_unit_order_plot(request.state,command.unit);auto &core=unit(request.state,command.unit).core;
        auto &moves=scalar(core,"m_iMoves");
        if(moves!=0) {
            moves=0;draft->movement_changed=true;draft->completed_steps.push_back(BnwFinishMovesStep::moves_store);
            batch.effects.push_back({BnwUnitPresentationEffectKind::should_dim_flag,command.unit,1});draft->completed_steps.push_back(BnwFinishMovesStep::dim_flag);
            if(client.selected_unit()==command.unit) {
                need(request.owner.pathfinder_cache.has_value(),"Selected FinishMoves requires actual common finder owner");
                force_reset_bnw_update_moves_common_cache(request.state,*request.owner.pathfinder_cache,command.unit.first);
                draft->completed_steps.push_back(BnwFinishMovesStep::common_finder_force_reset);
                batch.effects.push_back({BnwUnitPresentationEffectKind::dirty_unit_info,command.unit,1});draft->completed_steps.push_back(BnwFinishMovesStep::unit_info_dirty);
            }
            if(client.selection_plot()==plot) {
                batch.effects.push_back({BnwUnitPresentationEffectKind::dirty_plot_list_buttons,command.unit,1});draft->completed_steps.push_back(BnwFinishMovesStep::plot_list_dirty);
            }
        }
        batch.after_world_revision=bnw_unit_order_world_revision(request.state,request.owner.identity,rules);
        if(!batch.effects.empty())apply_bnw_unit_presentation_batch(request.state,draft->presentation,rules,batch);
        request.input_revision=bnw_end_turn_input_revision(request.state,request.owner,binding,rules,command.unit.first);
        result.draft=std::move(draft);
    } catch(const std::exception &error){result.blocker=error.what();}
    return result;
}
} // namespace civ5::runtime
