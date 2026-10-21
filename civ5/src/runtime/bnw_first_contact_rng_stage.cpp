#include "civ5/runtime/bnw_first_contact_rng_stage.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <limits>
#include <stdexcept>

namespace civ5::runtime {
namespace {
void require(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
SessionRandomStreams replay(
    const SessionRandomStreams &source,
    const BnwFirstContactRngReceipt &receipt) {
    require(source.map.algorithm()==RandomAlgorithm::bnw &&
            source.gameplay.algorithm()==RandomAlgorithm::bnw &&
            source.map.state()==receipt.map_before &&
            source.gameplay.state()==receipt.gameplay_before &&
            receipt.draws.size()<=7,
            "BNW first-contact RNG source, algorithm or draw count changed");
    auto copy=source;
    std::size_t previous=0;
    for(std::size_t i=0;i<receipt.draws.size();++i) {
        const auto &draw=receipt.draws[i];
        require(draw.approach<7 && (i==0 || draw.approach>previous) &&
                draw.value<=std::numeric_limits<std::uint16_t>::max(),
                "BNW first-contact RNG draw order or value changed");
        previous=draw.approach;
        require(copy.gameplay.draw(draw.native_bound)==draw.value,
                "BNW first-contact RNG draw differs from SDK stream");
    }
    require(receipt.gameplay_before.calls<=
                std::numeric_limits<std::uint64_t>::max()-receipt.draws.size(),
            "BNW first-contact RNG call counter overflows");
    require(copy.map.state()==receipt.map_before &&
            copy.gameplay.state()==receipt.gameplay_after &&
            copy.gameplay.state().calls==
                receipt.gameplay_before.calls+receipt.draws.size(),
            "BNW first-contact RNG target differs from draw sequence");
    return copy;
}
void check_checkpoint(const saves::CivisSaveSection &section,
                      const SessionRandomStreams &source,
                      const SessionRandomStreams &target,
                      const SessionCheckpointIdentity &identity) {
    auto restored=source;
    restore_simulation_random_checkpoint(section,restored,identity);
    require(restored.map.state()==target.map.state() &&
            restored.gameplay.state()==target.gameplay.state() &&
            restored.map.algorithm()==RandomAlgorithm::bnw &&
            restored.gameplay.algorithm()==RandomAlgorithm::bnw,
            "BNW first-contact ID13 private restore changed RNG state");
    const auto roundtrip=make_simulation_random_checkpoint(restored,identity);
    require(roundtrip.id==section.id && roundtrip.flags==section.flags &&
            roundtrip.bytes==section.bytes,
            "BNW first-contact ID13 re-encoding changed");
}
} // namespace

BnwFirstContactRngStage stage_bnw_first_contact_approach_rng(
    const saves::SavedSession &source,const LiveSessionState &state,
    const BnwSetXYRequest &request,const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,
    const SessionCheckpointIdentity &identity,
    const BnwDiplomacyScratchpadLoad &scratchpad) {
    BnwFirstContactRngStage stage;
    try {
        const auto quote=quote_bnw_first_contact_approach_random(
            source,state,request,database,types,defines,identity,scratchpad);
        require(quote.gate==
                    BnwApproachRandomGate::next_can_declare_war_unimplemented,
                quote.reason.empty()
                    ? "BNW first-contact source-order random prefix unavailable"
                    : quote.reason.c_str());
        BnwFirstContactRngReceipt receipt;
        receipt.map_before=state.random.map.state();
        receipt.gameplay_before=quote.rng_before;
        receipt.gameplay_after=quote.rng_after;
        receipt.draws=quote.draws;
        receipt.local_weights=quote.weights;
        const auto target=replay(state.random,receipt);
        auto section=make_simulation_random_checkpoint(target,identity);
        check_checkpoint(section,state.random,target,identity);
        receipt.checkpoint_sha256=identify_bytes(section.bytes).sha256;
        stage.checkpoint=std::move(section);
        stage.receipt=std::move(receipt);
        stage.staged=true;
        stage.reason="BNW first-contact JonRand prefix staged with ID13; CanDeclareWar Lua and later approach effects remain";
    } catch(const std::exception &error) {
        stage.staged=false;
        stage.checkpoint.reset();stage.receipt.reset();
        stage.reason=error.what();
    }
    return stage;
}

bool verify_bnw_first_contact_rng_receipt(
    const BnwFirstContactRngStage &stage,
    const SessionRandomStreams &source_streams,
    const SessionCheckpointIdentity &identity) {
    try {
        require(stage.staged && stage.checkpoint && stage.receipt,
                "BNW first-contact RNG receipt or ID13 missing");
        require(identify_bytes(stage.checkpoint->bytes).sha256==
                    stage.receipt->checkpoint_sha256,
                "BNW first-contact RNG receipt SHA-256 changed");
        const auto target=replay(source_streams,*stage.receipt);
        check_checkpoint(*stage.checkpoint,source_streams,target,identity);
        return true;
    } catch(const std::exception &) {return false;}
}
} // namespace civ5::runtime
