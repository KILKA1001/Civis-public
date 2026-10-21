#include "civ5/runtime/bnw_update_moves_prefix.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <limits>
#include <set>
#include <stdexcept>

namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void need(bool value,const char *why) {if(!value)throw std::runtime_error(why);}
template<class Core>std::int64_t scalar(const Core &core,std::string_view name) {
    const typename std::decay_t<decltype(core.fields)>::value_type *found=nullptr;
    for(const auto &field:core.fields)if(field.name && std::string_view(field.name)==name) {
        need(!found,"Duplicate BNW updateMoves scalar");found=&field;
    }
    need(found && found->values.size()==1,"Missing BNW updateMoves scalar");
    return found->values[0];
}
void boolean(std::int64_t value) {need(value==0 || value==1,"Nonboolean BNW updateMoves flag");}
void bound_world(const LiveSessionState &state,const BnwUpdateMovesImportOwner &owner) {
    need(owner.identity().payload_sha256.size()==64 && owner.identity().rules_sha256.size()==64 &&
         state.setup.profile==content::TechnologyProfile::bnw && state.world.game_state.layout==bnw &&
         state.world.map_header.layout==bnw && state.world.map_header.guid==owner.map_guid() &&
         state.world.all_players && state.world.all_players->layout==bnw &&
         state.world.all_players->context.player_count==64 && state.world.all_players->players.size()==64 &&
         state.setup.players.size()==64,"BNW updateMoves original world binding differs");
}
}
BnwUpdateMovesImportOwner bind_bnw_update_moves_original_import(
    const saves::SavedSession &source,const SessionCheckpointIdentity &identity) {
    need(source.source.container==saves::SaveContainerKind::civ5 &&
         identify_bytes(source.source.bytes).sha256==identity.payload_sha256 && identity.rules_sha256.size()==64 &&
         source.snapshot.game_state.layout==bnw && source.snapshot.map_header.layout==bnw &&
         source.snapshot.all_players && source.snapshot.all_players->layout==bnw &&
         source.snapshot.all_players->context.player_count==64 && source.snapshot.all_players->players.size()==64,
         "BNW updateMoves reset receipt needs the original complete import; continuation is unknown");
    return BnwUpdateMovesImportOwner(identity,source.snapshot.map_header.guid);
}
std::vector<BnwUpdateMovesUnitEvidence> observe_bnw_update_moves_units(
    const LiveSessionState &state,const BnwUpdateMovesImportOwner &owner,std::int64_t player) {
    bound_world(state,owner);need(player>=0 && player<64,"Invalid BNW updateMoves unit owner");
    const auto &raw=state.world.all_players->players.at(player);
    need(raw.core.layout==bnw && raw.index==static_cast<std::uint64_t>(player) &&
         raw.saved_player_id==player && scalar(raw.core,"m_eID")==player && raw.units.layout==bnw &&
         raw.units.header.layout==bnw && raw.units.header.wire_unit_count>=0 &&
         static_cast<std::uint64_t>(raw.units.header.wire_unit_count)==raw.units.units.size(),
         "BNW updateMoves complete unit collection binding differs");
    std::vector<BnwUpdateMovesUnitEvidence> result;std::set<std::int64_t> ids;
    for(const auto &unit:raw.units.units) {
        need(unit.core.layout==bnw && unit.tail.layout==bnw && unit.core.version &&
             unit.tail.context.bnw_unit_version==unit.core.version &&
             unit.core.start_offset<unit.core.end_offset && unit.tail.start_offset<unit.tail.end_offset &&
             scalar(unit.core,"m_eOwner")==player,"BNW updateMoves unit wire identity differs");
        BnwUpdateMovesUnitEvidence item;
        item.id=scalar(unit.core,"m_iID");
        need(item.id>=0 && item.id<=INT32_MAX && ids.insert(item.id).second,
             "BNW updateMoves unit ID is invalid or duplicated");
        item.moves=scalar(unit.core,"m_iMoves");item.activity=scalar(unit.core,"m_eActivityType");
        item.automate=scalar(unit.core,"m_eAutomateType");item.unit_ai=scalar(unit.core,"m_eUnitAIType");
        const auto processed=scalar(unit.core,"m_bAITurnProcessed");boolean(processed);
        item.turn_processed=processed;item.can_move=item.moves>0;
        item.automated=item.automate!=-1; // NO_AUTOMATE, CvEnums.h:1594
        item.needs_ai_update=!item.turn_processed && item.automated && item.unit_ai!=0 && item.can_move;
        const saves::UnitTailNode *queue=nullptr;
        for(const auto &node:unit.tail.blocks)if(node.name && std::string_view(node.name)=="m_missionQueue") {
            need(!queue,"Duplicate BNW updateMoves mission queue");queue=&node;
        }
        need(queue && std::string_view(queue->encoding)=="counted_missions" && queue->wire_count &&
             *queue->wire_count>=0 && static_cast<std::uint64_t>(*queue->wire_count)<=
                 std::numeric_limits<std::uint64_t>::max()/5 &&
             static_cast<std::uint64_t>(*queue->wire_count)*5==queue->values.size(),
             "BNW updateMoves mission queue is unknown or incomplete");
        item.mission_count=static_cast<std::uint64_t>(*queue->wire_count);result.push_back(item);
    }
    return result;
}
BnwUpdateMovesPrefixGate gate_bnw_update_moves_original_draft(
    const LiveSessionState &state,const BnwGameActivationOwner &activation,
    const BnwEndTurnInputBinding &binding,const BnwUpdateMovesImportOwner &owner,
    const content::RulesDatabase &rules,std::int64_t player,std::string_view expected_input_revision) {
    BnwUpdateMovesPrefixGate result;
    try {
        bound_world(state,owner);
        need(owner.identity().payload_sha256==activation.identity.payload_sha256 &&
             owner.identity().rules_sha256==activation.identity.rules_sha256 &&
             binding.map_guid()==owner.map_guid(),"BNW updateMoves source/input owners differ");
        need(bnw_end_turn_input_revision(state,activation,binding,rules,player)==expected_input_revision,
             "BNW updateMoves end-turn request revision is stale");
        const auto &requested=state.world.all_players->players.at(player).core;
        need(scalar(requested,"m_bEndTurn")==1 && scalar(requested,"m_bAutoMoves")==1,
             "BNW updateMoves bounded completion requires an actual end-turn/automoves request");
        result.last_ai_civs_processed=owner.last_ai_civs_processed();result.attempted_turn=state.setup.game_turn;
        // Source CvGame.cpp:8080-8091, first active alive AI only. Do not
        // select any human until the subsequent engine all-AI barrier succeeds.
        for(std::int64_t id=0;id<64;++id) {
            const auto &raw=state.world.all_players->players.at(id);
            const auto &owned=state.setup.players.at(id);
            need(raw.index==static_cast<std::uint64_t>(id) && raw.saved_player_id==id &&
                 raw.core.layout==bnw && scalar(raw.core,"m_eID")==id,
                 "BNW updateMoves source player identity differs");
            const auto alive=scalar(raw.core,"m_bAlive"),active=scalar(raw.core,"m_bTurnActive");
            boolean(alive);boolean(active);need(bool(alive)==owned.alive,"BNW updateMoves raw/owned alive flag differs");
            if(alive && active && !state.setup.is_human(owned)) {
                result.source_players_to_process.push_back(id);break;
            }
        }
        result.source_ai_scan_completed=true;
        if(!result.source_players_to_process.empty()) {
            result.reason=BnwUpdateMovesPrefixBlocker::active_ai_branch;
            result.blocker="BNW updateMoves selected the first active AI; its native update owner is unavailable";
            return result;
        }
        // Fresh Read reset requires Send before the native field assignment.
        // A repeated pass requires the same retained delivered barrier instead.
        // Never write current turn or resend to simulate a lost receipt.
        need((owner.last_ai_civs_processed()==-1 || owner.last_ai_civs_processed()==state.setup.game_turn) && state.setup.game_turn>=0,
             "BNW updateMoves original Read reset receipt differs");
        if(owner.last_ai_civs_processed()==state.setup.game_turn) {
            result.reason=BnwUpdateMovesPrefixBlocker::all_ai_civs_processed_owner_missing;
            result.blocker="BNW updateMoves requires the retained delivered all-AI receipt for this already-marked turn";
            return result;
        }
        result.reason=BnwUpdateMovesPrefixBlocker::send_ai_civs_processed_owner_missing;
        result.blocker="BNW updateMoves blocked at SendAICivsProcessed: no source-bound standalone local engine delivery owner; last-AI-turn write, allAICivsProcessedThisTurn, human selection, ForceReset, AutoMission and deactivation were not executed";
    } catch(const std::exception &error) {result.blocker=error.what();}
    return result;
}
} // namespace civ5::runtime
