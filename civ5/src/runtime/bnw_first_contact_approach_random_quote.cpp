#include "civ5/runtime/bnw_first_contact_approach_random_quote.hpp"
#include <climits>
#include <stdexcept>
#include <variant>

namespace civ5::runtime {
namespace {
bool same(const saves::RandomStateObservation &source,
    const SimulationRandomState &owned) {
    return source.seed==owned.seed && source.calls==owned.calls &&
        source.resets==owned.resets;
}
bool same(const saves::RandomStateObservation &a,
    const saves::RandomStateObservation &b) {
    return a.seed==b.seed && a.calls==b.calls && a.resets==b.resets;
}
} // namespace

BnwApproachRandomQuote quote_bnw_approach_random_weights(
    const std::array<std::int64_t,7> &deterministic_weights,
    std::int64_t random_percent,const SimulationRandom &gameplay) {
    BnwApproachRandomQuote quote;
    quote.rng_before=gameplay.state();
    quote.rng_after=gameplay.state();
    quote.weights=deterministic_weights;
    if(gameplay.algorithm()!=RandomAlgorithm::bnw ||
       random_percent<INT32_MIN || random_percent>INT32_MAX) {
        quote.gate=BnwApproachRandomGate::random_rule_unavailable;
        quote.reason="BNW random rule or gameplay RNG algorithm unavailable";
        return quote;
    }
    auto detached=gameplay;
    try {
        // Expansion2 CvDiplomacyAI.cpp:3407-3420, CvGame.cpp:9015 and
        // CvRandom.cpp:115. Iteration is approach enum order, not weight
        // ranking. The bound can narrow to zero at the unsigned-short call;
        // get(0) still advances m_jonRand once.
        for(std::size_t approach=0;approach<quote.weights.size();++approach) {
            const auto scaled=quote.weights[approach]*100;
            if(scaled<INT32_MIN || scaled>INT32_MAX)
                throw std::overflow_error("BNW approach weight * 100 overflows native int");
            quote.weights[approach]=scaled;
            const auto product=scaled*random_percent;
            if(product<INT32_MIN || product>INT32_MAX)
                throw std::overflow_error("BNW approach random bound overflows native int");
            if(product<=0) continue;
            const auto divided=product/100;
            // CvGame::getJonRandNumVA(int) passes this to
            // CvRandom::get(unsigned short), truncating to 16 bits.
            const auto bound=static_cast<std::uint16_t>(divided);
            const auto draw=detached.draw(bound);
            if(quote.weights[approach]+static_cast<std::int64_t>(draw)>INT32_MAX)
                throw std::overflow_error("BNW randomized approach weight overflows native int");
            quote.weights[approach]+=static_cast<std::int64_t>(draw);
            quote.draws.push_back({approach,bound,draw});
        }
        quote.rng_after=detached.state();
        quote.gate=BnwApproachRandomGate::next_can_declare_war_unimplemented;
        quote.reason="BNW JonRand loop quoted on detached gameplay RNG; canDeclareWar and later effects remain unavailable";
    } catch(const std::exception &error) {
        quote.gate=BnwApproachRandomGate::native_arithmetic_unavailable;
        quote.reason=error.what();
    }
    return quote;
}

BnwApproachRandomQuote quote_bnw_first_contact_approach_random(
    const saves::SavedSession &source,const LiveSessionState &state,
    const BnwSetXYRequest &request,const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,
    const SessionCheckpointIdentity &identity,
    const BnwDiplomacyScratchpadLoad &scratchpad) {
    BnwApproachRandomQuote quote;
    const auto prefix=preflight_bnw_first_contact_approach_after_load(
        source,state,request,database,types,defines,identity,scratchpad);
    if(prefix.gate!=BnwFirstContactApproachAfterLoadGate::random_factor_unimplemented) {
        quote.reason="BNW deterministic approach prefix unavailable: "+prefix.reason;
        return quote;
    }
    constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
    const auto &a=source.snapshot.progression;
    const auto &b=state.world.progression;
    if(a.layout!=bnw || b.layout!=bnw ||
       state.random.map.algorithm()!=RandomAlgorithm::bnw ||
       state.random.gameplay.algorithm()!=RandomAlgorithm::bnw ||
       !same(a.random[0],b.random[0]) ||
       !same(a.random[1],b.random[1]) ||
       !same(a.random[0],state.random.map.state()) ||
       !same(a.random[1],state.random.gameplay.state())) {
        quote.gate=BnwApproachRandomGate::source_rng_unavailable;
        quote.reason="BNW source/owned map and gameplay RNG states differ before first-contact quote";
        return quote;
    }
    const auto *entry=defines.find("APPROACH_RANDOM_PERCENT");
    if(defines.profile()!=content::GlobalDefineProfile::bnw || !entry ||
       entry->lookup.sqlite_code!=0 ||
       !std::holds_alternative<std::int64_t>(entry->lookup.value)) {
        quote.gate=BnwApproachRandomGate::random_rule_unavailable;
        quote.reason="BNW active APPROACH_RANDOM_PERCENT rule unavailable";
        return quote;
    }
    return quote_bnw_approach_random_weights(prefix.weights,
        std::get<std::int64_t>(entry->lookup.value),state.random.gameplay);
}

} // namespace civ5::runtime
