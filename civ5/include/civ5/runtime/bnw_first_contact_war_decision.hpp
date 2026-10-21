#pragma once
#include "civ5/runtime/bnw_first_contact_rng_stage.hpp"
#include "civ5/runtime/bnw_first_contact_approach_war_gate.hpp"
#include "civ5/runtime/bnw_detached_gameplay_lua.hpp"
#include "civ5/runtime/bnw_first_contact_approach_tail.hpp"

namespace civ5::runtime {

enum class BnwFirstContactWarDecisionGate : std::uint8_t {
    earlier_rng_or_team_gate_unavailable,
    fixture_source_unavailable,
    lua_callback_failed,
    selected_gameplay_lua_order_unverified,
};
enum class BnwFirstContactAfterFixtureGate : std::uint8_t {
    human_handicap_rule_unavailable,
    native_sort_tie_unverified,
    war_face_unimplemented,
    approach_log_unimplemented,
    approach_computed_private,
};
struct BnwFirstContactApproachReceipt {
    std::int64_t recipient=-1,target=-1;
    std::array<std::int64_t,7> before_tail{},after_tail{};
    bool target_human=false;
    std::int64_t human_war_modifier=100,approach=-1,war_face=-1;
    bool war_face_write_required=false;
    std::string rng_checkpoint_sha256,ordered_lua_sha256,sha256;
};
struct BnwFirstContactWarDecisionQuote {
    BnwFirstContactWarDecisionGate gate=
        BnwFirstContactWarDecisionGate::earlier_rng_or_team_gate_unavailable;
    BnwFirstContactAfterFixtureGate next_after_fixture=
        BnwFirstContactAfterFixtureGate::human_handicap_rule_unavailable;
    std::optional<BnwCanDeclareWarLuaResult> lua_decision;
    std::array<std::int64_t,7> weights{};
    std::optional<std::size_t> unique_best_approach;
    std::int64_t highest_weight=0;
    std::optional<BnwApproachTailQuote> tail;
    std::optional<BnwFirstContactApproachReceipt> receipt;
    std::string reason;
    bool can_commit() const noexcept {return false;}
};

// Uses the real CallTestAll listener bridge in a disposable VM, after the
// attested RNG receipt and native canDeclareWar prefix. Quotes the veto's WAR
// weight and the human handicap, native sort and WarFace tail. Attests the local approach
// candidate. A fixture is never evidence of selected stock Lua entrypoints.
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
    std::int64_t recipient_player,std::int64_t target_player);
// Reexecutes the same private source-prefix/Lua/tail quote and checks its
// receipt, including ID13, chunk order, human modifier and proposed writes.
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
    std::int64_t recipient_player,std::int64_t target_player);
} // namespace civ5::runtime
