#include "civ5/runtime/bnw_first_contact_war_decision.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <climits>
#include <sstream>
#include <locale>
#include <stdexcept>

namespace civ5::runtime {
namespace {
void require(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
std::string digest(std::string_view value) {
    return identify_bytes(std::span(
        reinterpret_cast<const std::uint8_t*>(value.data()),value.size())).sha256;
}
std::string receipt_sha(const BnwFirstContactApproachReceipt &receipt,
    const SessionCheckpointIdentity &identity) {
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out<<"BNW-first-contact-approach-v1|"<<identity.payload_sha256<<'|'
       <<identity.rules_sha256<<'|'<<receipt.recipient<<'|'<<receipt.target;
    for(auto value:receipt.before_tail) out<<'|'<<value;
    for(auto value:receipt.after_tail) out<<'|'<<value;
    out<<'|'<<receipt.target_human<<'|'<<receipt.human_war_modifier<<'|'
       <<receipt.approach<<'|'<<receipt.war_face<<'|'
       <<receipt.war_face_write_required<<'|'<<receipt.rng_checkpoint_sha256
       <<'|'<<receipt.ordered_lua_sha256;
    return digest(out.str());
}
std::string chunks_sha(std::span<const BnwDetachedGameplayLuaChunk> chunks) {
    std::ostringstream out;
    out.imbue(std::locale::classic());
    for(const auto &chunk:chunks)
        out<<chunk.context.size()<<':'<<chunk.context<<chunk.name.size()<<':'
           <<chunk.name<<chunk.sha256.size()<<':'<<chunk.sha256;
    return digest(out.str());
}
std::int64_t diplomacy_value(const saves::SavedSession &source,
    const LiveSessionState &state,std::int64_t player,std::int64_t target,
    std::string_view name,saves::DiplomacyEncoding encoding,std::size_t width) {
    require(source.snapshot.all_players && state.world.all_players &&
            player>=0 && player<63 && target>=0 && target<22 &&
            source.snapshot.all_players->players.size()==64 &&
            state.world.all_players->players.size()==64,
            "BNW approach tail diplomacy player roster unavailable");
    const auto find=[&](const auto &snapshot) {
        const auto &entry=snapshot.all_players->players.at(
            static_cast<std::size_t>(player));
        require(entry.saved_player_id==player && entry.diplomacy.layout==
                saves::GameStateLayout::expansion2_version1,
                "BNW approach tail diplomacy player/profile differs");
        const saves::PlayerDiplomacyField *found=nullptr;
        for(const auto &field:entry.diplomacy.fields) if(field.name && name==field.name) {
            require(!found,"BNW approach tail duplicate diplomacy row");
            found=&field;
        }
        require(found && found->encoding==encoding && found->rows==1 &&
                found->columns==width && found->values.size()==width,
                "BNW approach tail diplomacy row shape unavailable");
        return found;
    };
    const auto *saved=find(source.snapshot),*owned=find(state.world);
    require(saved->values==owned->values,
            "BNW approach tail source/owned diplomacy row differs");
    return owned->values.at(static_cast<std::size_t>(target));
}
std::int64_t human_modifier(const content::RulesDatabase &database,
    std::int64_t handicap) {
    require(handicap>=INT32_MIN && handicap<=INT32_MAX,
            "BNW approach handicap exceeds native enum");
    if(handicap<0) return 100; // getHandicapInfo returns NULL.
    std::optional<std::int64_t> modifier;
    const auto report=database.query_rows({
        "SELECT AIDeclareWarProb FROM HandicapInfos WHERE ID=?",{handicap}},
        [&](const auto &row) {
            require(!modifier,"BNW approach duplicate handicap info ID");
            modifier=row.integer_at(0);
        },2);
    require(report.all_succeeded(),"BNW approach handicap rule lookup failed");
    return modifier.value_or(100); // Native missing-info branch.
}
} // namespace

BnwFirstContactWarDecisionQuote quote_bnw_first_contact_war_decision_fixture(
    const saves::SavedSession &source,const LiveSessionState &state,
    const BnwSetXYRequest &request,
    const BnwFirstContactRngStage &rng_stage,
    const BnwApproachWarGateQuote &native_gate,
    const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,
    const BnwDiplomacyScratchpadLoad &scratchpad,
    const SessionCheckpointIdentity &identity,
    std::span<const BnwDetachedGameplayLuaChunk> ordered_chunks,
    std::int64_t recipient_player,std::int64_t target_player) {
    BnwFirstContactWarDecisionQuote result;
    try {
        require(identity.payload_sha256.size()==64 &&
                identify_bytes(source.source.bytes).sha256==identity.payload_sha256 &&
                verify_bnw_first_contact_rng_receipt(
                    rng_stage,state.random,identity) &&
                rng_stage.receipt &&
                native_gate.gate==
                    BnwApproachWarGate::lua_can_declare_war_unavailable &&
                native_gate.can_declare_without_lua && !native_gate.at_war &&
                native_gate.rng_before==rng_stage.receipt->gameplay_before &&
                native_gate.rng_after==rng_stage.receipt->gameplay_after &&
                native_gate.weights==rng_stage.receipt->local_weights &&
                recipient_player>=0 && recipient_player<63 &&
                target_player>=0 && target_player<22 &&
                recipient_player!=target_player &&
                state.setup.profile==content::TechnologyProfile::bnw &&
                state.setup.players.contains(recipient_player) &&
                state.setup.players.contains(target_player) &&
                state.setup.players.at(recipient_player).alive &&
                state.setup.players.at(target_player).alive &&
                state.setup.players.at(recipient_player).team==
                    native_gate.recipient_team &&
                state.setup.players.at(target_player).team==
                    native_gate.target_team,
                "BNW first-contact Lua branch lacks matching source, RNG or player/team state");
        // ID13 binds random state, not AI weights or team identities. Requote
        // the SDK prefix for this exact request rather than trusting a
        // caller-populated gate or a mutable local-weight receipt.
        const auto expected=quote_bnw_first_contact_approach_war_gate(
            source,state,request,database,types,defines,identity,scratchpad);
        const auto pair=preflight_bnw_first_contact_approach(
            source,state,request,database,types,defines,identity);
        require(expected.gate==BnwApproachWarGate::lua_can_declare_war_unavailable &&
                expected.recipient_team==native_gate.recipient_team &&
                expected.target_team==native_gate.target_team &&
                expected.weights==native_gate.weights &&
                expected.rng_before==native_gate.rng_before &&
                expected.rng_after==native_gate.rng_after &&
                expected.can_declare_without_lua==native_gate.can_declare_without_lua &&
                expected.at_war==native_gate.at_war &&
                pair.recipient==recipient_player && pair.target==target_player,
                "BNW first-contact Lua prefix differs from current source request");
        const auto lua=quote_bnw_can_declare_war_detached_fixture(
            database,types,state.setup,state.technologies,
            ordered_chunks,native_gate.recipient_team,native_gate.target_team);
        if(lua.gate!=
               BnwDetachedGameplayLuaGate::fixture_quoted_stock_context_unverified ||
           !lua.decision) {
            result.gate=lua.gate==BnwDetachedGameplayLuaGate::source_plan_unavailable
                ? BnwFirstContactWarDecisionGate::fixture_source_unavailable
                : BnwFirstContactWarDecisionGate::lua_callback_failed;
            result.reason=lua.reason;
            return result;
        }
        result.lua_decision=lua.decision;
        result.weights=native_gate.weights;
        // CvDiplomacyAI.cpp:3421-3447: a veto clears WAR because this
        // selected pair is not already at war. A missing listener allows it.
        if(lua.decision->decision==BnwCanDeclareWarLuaDecision::veto)
            result.weights[0]=0;
        // The same-team branch cannot fire for this attested contact pair.
        // CvPlayer::getHandicapType reads CvPreGame::handicap, so verify the
        // saved slot and handicap mirrors before evaluating the human branch.
        const auto saved_setup=restore_research_context(source.snapshot,database);
        const auto owned_world_setup=restore_research_context(state.world,database);
        const auto &world_target=owned_world_setup.players.at(target_player);
        const auto &saved_target=saved_setup.players.at(target_player);
        const auto &owned_target=state.setup.players.at(target_player);
        require(saved_setup.game_type==state.setup.game_type &&
                owned_world_setup.game_type==state.setup.game_type &&
                world_target.slot_status==owned_target.slot_status &&
                world_target.slot_claim==owned_target.slot_claim &&
                world_target.handicap==owned_target.handicap &&
                owned_world_setup.is_human(world_target)==state.setup.is_human(owned_target) &&
                saved_target.slot_status==owned_target.slot_status &&
                saved_target.slot_claim==owned_target.slot_claim &&
                saved_target.handicap==owned_target.handicap &&
                saved_setup.is_human(saved_target)==state.setup.is_human(owned_target),
                "BNW approach target human/handicap setup differs from source");
        BnwApproachTailInput tail_input;
        tail_input.weights=result.weights;
        tail_input.target_human=state.setup.is_human(owned_target);
        if(tail_input.target_human)
            tail_input.human_war_modifier=human_modifier(database,owned_target.handicap);
        auto tail=quote_bnw_first_contact_approach_tail(tail_input);
        if(tail.approach && *tail.approach==0) {
            tail_input.existing_war_face=diplomacy_value(source,state,
                recipient_player,target_player,"m_paeWarFace",
                saves::DiplomacyEncoding::i8,63);
            require(tail_input.existing_war_face>=-1 &&
                    tail_input.existing_war_face<=2,
                    "BNW approach saved WarFace is outside source enum");
            if(tail_input.existing_war_face==-1) {
                const auto denounced=diplomacy_value(source,state,
                    recipient_player,target_player,"m_pabDenouncedPlayer",
                    saves::DiplomacyEncoding::boolean,22);
                require(denounced==0 || denounced==1,
                        "BNW approach denounced flag is not boolean");
                tail_input.denounced_target=denounced==1;
            }
            tail=quote_bnw_first_contact_approach_tail(tail_input);
        }
        result.tail=tail;
        result.weights=tail.weights;
        result.unique_best_approach=tail.approach;
        result.highest_weight=tail.highest_weight;
        if(tail.gate==BnwApproachTailGate::quoted) {
            BnwFirstContactApproachReceipt receipt;
            receipt.recipient=recipient_player;receipt.target=target_player;
            receipt.before_tail=tail_input.weights;receipt.after_tail=tail.weights;
            receipt.target_human=tail_input.target_human;
            receipt.human_war_modifier=tail_input.human_war_modifier;
            receipt.approach=static_cast<std::int64_t>(*tail.approach);
            receipt.war_face=*tail.war_face;
            receipt.war_face_write_required=tail.war_face_write_required;
            receipt.rng_checkpoint_sha256=rng_stage.receipt->checkpoint_sha256;
            receipt.ordered_lua_sha256=chunks_sha(ordered_chunks);
            receipt.sha256=receipt_sha(receipt,identity);
            result.receipt=std::move(receipt);
            result.next_after_fixture=
                BnwFirstContactAfterFixtureGate::approach_computed_private;
        } else if(tail.gate==BnwApproachTailGate::native_sort_tie_unverified)
            result.next_after_fixture=
                BnwFirstContactAfterFixtureGate::native_sort_tie_unverified;
        else {
            result.reason=tail.reason;
            return result;
        }
        result.gate=
            BnwFirstContactWarDecisionGate::selected_gameplay_lua_order_unverified;
        result.reason="BNW CallTestAll and approach tail quoted privately; selected stock listener order and later contact effects remain unverified";
    } catch(const std::exception &error) {
        result.reason=error.what();
    }
    return result;
}
bool verify_bnw_first_contact_approach_receipt(
    const BnwFirstContactWarDecisionQuote &quote,
    const saves::SavedSession &source,const LiveSessionState &state,
    const BnwSetXYRequest &request,
    const BnwFirstContactRngStage &rng_stage,
    const BnwApproachWarGateQuote &native_gate,
    const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,
    const BnwDiplomacyScratchpadLoad &scratchpad,
    const SessionCheckpointIdentity &identity,
    std::span<const BnwDetachedGameplayLuaChunk> ordered_chunks,
    std::int64_t recipient_player,std::int64_t target_player) {
    if(quote.gate!=BnwFirstContactWarDecisionGate::selected_gameplay_lua_order_unverified ||
       !quote.receipt || !quote.tail ||
       quote.tail->gate!=BnwApproachTailGate::quoted ||
       quote.receipt->sha256!=receipt_sha(*quote.receipt,identity) ||
       quote.receipt->after_tail!=quote.weights ||
       quote.tail->weights!=quote.weights ||
       quote.tail->approach!=quote.unique_best_approach ||
       quote.tail->war_face!=quote.receipt->war_face ||
       quote.tail->war_face_write_required!=quote.receipt->war_face_write_required ||
       quote.tail->highest_weight!=quote.highest_weight ||
       quote.highest_weight!=*std::max_element(quote.weights.begin(),quote.weights.end()) ||
       !quote.unique_best_approach ||
       quote.receipt->approach!=static_cast<std::int64_t>(*quote.unique_best_approach))
        return false;
    const auto replay=quote_bnw_first_contact_war_decision_fixture(
        source,state,request,rng_stage,native_gate,database,types,defines,
        scratchpad,identity,ordered_chunks,recipient_player,target_player);
    return replay.receipt && replay.receipt->sha256==quote.receipt->sha256 &&
        replay.lua_decision && quote.lua_decision &&
        replay.lua_decision->decision==quote.lua_decision->decision &&
        replay.lua_decision->handled==quote.lua_decision->handled;
}
} // namespace civ5::runtime
