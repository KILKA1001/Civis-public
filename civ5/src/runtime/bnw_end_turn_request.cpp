#include "civ5/runtime/bnw_end_turn_request.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <limits>
#include <stdexcept>

namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void need(bool value,const char *message) {if(!value)throw std::runtime_error(message);}
std::int64_t &field(saves::PlayerCoreObservation &core,std::string_view name) {
    saves::PlayerCoreField *found=nullptr;
    for(auto &item:core.fields)if(item.name && std::string_view(item.name)==name) {
        need(!found,"Duplicate BNW turn-input scalar");found=&item;
    }
    need(found && found->values.size()==1,"Missing BNW turn-input scalar");
    need(found->encoding==saves::PlayerCoreEncoding::boolean ||
         found->encoding==saves::PlayerCoreEncoding::i32,"Wrong BNW turn-input encoding");
    return found->values[0];
}
std::int64_t field(const saves::PlayerCoreObservation &core,std::string_view name) {
    return field(const_cast<saves::PlayerCoreObservation&>(core),name);
}
void boolean(std::int64_t value) {need(value==0 || value==1,"BNW turn-input flag is not boolean");}
void validate(const LiveSessionState &state,const BnwGameActivationOwner &owner,
    const BnwEndTurnInputBinding &binding,const content::RulesDatabase &rules,std::int64_t player) {
    need(owner.identity.payload_sha256==binding.identity().payload_sha256 &&
         owner.identity.rules_sha256==binding.identity().rules_sha256 &&
         identify_bytes(rules.gameplay_fingerprint_data()).sha256==binding.identity().rules_sha256,
         "BNW turn-input source/rules owner differs");
    need(state.setup.profile==content::TechnologyProfile::bnw && state.world.game_state.layout==bnw &&
         state.world.pregame && state.world.pregame->layout==bnw && state.world.all_players &&
         state.world.all_players->layout==bnw && state.world.all_players->players.size()==64 &&
         state.world.all_players->context.player_count==64 && state.setup.players.size()==64 &&
         state.world.map_header.guid==binding.map_guid(),"BNW turn-input world/roster binding differs");
    const auto &pregame=state.world.pregame->archive.integers;
    need(pregame.at("game_type")==0 && state.setup.game_type==0,
         "BNW turn-input supports ordinary single-player only");
    auto setup=state.setup;
    need(setup.game_option_value(rules,"GAMEOPTION_DYNAMIC_TURNS")==0 &&
         setup.game_option_value(rules,"GAMEOPTION_SIMULTANEOUS_TURNS")==0 &&
         setup.game_option_value(rules,"GAMEOPTION_PITBOSS")==0 && state.setup.pitboss!=true,
         "BNW turn-input needs simultaneous/network/PitBoss owners");
    need(player>=0 && player<63 && pregame.at("active_player")==player &&
         pregame.at("game_turn")==state.setup.game_turn && state.setup.game_turn>=0 &&
         state.setup.game_turn<=std::numeric_limits<std::int32_t>::max(),
         "BNW turn-input player/clock differs");
    const auto &raw=state.world.all_players->players.at(player);
    const auto &owned=state.setup.players.at(player);
    need(raw.core.layout==bnw && raw.index==static_cast<std::uint64_t>(player) &&
         raw.saved_player_id==player && field(raw.core,"m_eID")==player &&
         field(raw.core,"m_bAlive")==1 && owned.alive && state.setup.is_human(owned) &&
         field(raw.core,"m_bTurnActive")==1,"BNW turn-input requires the alive active human");
    boolean(field(raw.core,"m_bEndTurn"));boolean(field(raw.core,"m_bAutoMoves"));
    need(owner.processed_auto_moves.size()==64 && owner.processed_auto_moves.contains(player),
         "BNW turn-input processed-auto-moves transient is unknown");
    for(std::int64_t id=0;id<64;++id)
        need(owner.processed_auto_moves.contains(id),"BNW turn-input transient roster differs");
    need(owner.pending.empty(),"BNW turn-input has unconsumed activation publications");
}
void append(std::vector<std::uint8_t>&bytes,std::uint64_t value) {
    for(unsigned i=0;i<8;++i)bytes.push_back(static_cast<std::uint8_t>(value>>(8*i)));
}
}
BnwEndTurnInputBinding bind_bnw_end_turn_import(
    const saves::SavedSession &source,BnwGameActivationOwner &owner) {
    need(source.source.container==saves::SaveContainerKind::civ5 &&
         identify_bytes(source.source.bytes).sha256==owner.identity.payload_sha256 &&
         owner.identity.rules_sha256.size()==64 && source.snapshot.all_players &&
         source.snapshot.all_players->layout==bnw && source.snapshot.all_players->players.size()==64 &&
         owner.processed_auto_moves.empty() && owner.pending.empty() && owner.completed.empty(),
         "BNW turn-input transient initialization needs a fresh original import owner");
    auto draft=owner;
    for(std::int64_t id=0;id<64;++id) {
        const auto &raw=source.snapshot.all_players->players.at(id);
        need(raw.core.layout==bnw && raw.saved_player_id==id && raw.index==static_cast<std::uint64_t>(id) &&
             field(raw.core,"m_eID")==id,"BNW turn-input import player identity differs");
        draft.processed_auto_moves.emplace(id,false); // actual reset/Read transient
    }
    BnwEndTurnInputBinding result(owner.identity,source.snapshot.map_header.guid);
    owner=std::move(draft);return result;
}
std::string bnw_end_turn_input_revision(const LiveSessionState &state,
    const BnwGameActivationOwner &owner,const BnwEndTurnInputBinding &binding,
    const content::RulesDatabase &rules,std::int64_t player) {
    validate(state,owner,binding,rules,player);
    std::vector<std::uint8_t> bytes;
    for(const auto &sha:{binding.identity().payload_sha256,binding.identity().rules_sha256})
        bytes.insert(bytes.end(),sha.begin(),sha.end());
    bytes.insert(bytes.end(),binding.map_guid().begin(),binding.map_guid().end());
    append(bytes,state.setup.game_turn);append(bytes,player);
    const auto &raw=state.world.all_players->players.at(player).core;
    append(bytes,field(raw,"m_bEndTurn"));append(bytes,field(raw,"m_bAutoMoves"));
    append(bytes,owner.processed_auto_moves.at(player));
    return identify_bytes(bytes).sha256;
}
BnwEndTurnRequestExecution execute_bnw_end_turn_request_draft(
    const LiveSessionState &source,const BnwGameActivationOwner &owner,
    const BnwEndTurnInputBinding &binding,const content::RulesDatabase &rules,
    const BnwEndTurnRequest &request) {
    BnwEndTurnRequestExecution result;
    try {
        const auto before=bnw_end_turn_input_revision(source,owner,binding,rules,request.player);
        need(request.expected_turn==source.setup.game_turn && request.expected_input_revision==before,
             "BNW end-turn request has a stale input revision");
        auto draft=std::make_unique<BnwEndTurnRequestDraft>(source,owner);
        auto &core=draft->state.world.all_players->players.at(request.player).core;
        auto &end=field(core,"m_bEndTurn");auto &automatic=field(core,"m_bAutoMoves");
        const auto set_auto=[&](bool value) {
            if(automatic!=value) {
                automatic=value;draft->owner.processed_auto_moves.at(request.player)=false;
            }
        };
        if(end!=request.end_turn) {end=request.end_turn;set_auto(request.end_turn);}
        else if(!request.end_turn && automatic)set_auto(false);
        draft->requires_auto_move_processing=automatic!=0 && !draft->owner.processed_auto_moves.at(request.player);
        draft->input_revision=bnw_end_turn_input_revision(draft->state,draft->owner,binding,rules,request.player);
        draft->flags_changed=draft->input_revision!=before;
        result.draft=std::move(draft);
    } catch(const std::exception &error) {result.blocker=error.what();}
    return result;
}
}
