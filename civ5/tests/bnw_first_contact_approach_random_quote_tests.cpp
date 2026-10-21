#include "civ5/runtime/bnw_first_contact_approach_random_quote.hpp"
#include <array>
#include <climits>
#include <iostream>
#include <stdexcept>

namespace {
void check(bool yes,const char *message) {
    if(!yes) throw std::runtime_error(message);
}
}

int main() {
    using namespace civ5::runtime;
    try {
        // Expansion2 CvDiplomacyAI.cpp:3407-3420 enumerates seven approach
        // slots. CvGame.cpp:9015 uses m_jonRand; CvRandom.cpp:115 applies
        // the 32-bit LCG. Seed 2 gives samples 33676, then 22817.
        const SimulationRandom gameplay(RandomAlgorithm::bnw,{2,0,0});
        const auto before=gameplay.state();
        const auto quote=quote_bnw_approach_random_weights(
            std::array<std::int64_t,7>{1,0,-1,2,0,0,0},15,gameplay);
        check(quote.gate==BnwApproachRandomGate::next_can_declare_war_unimplemented &&
              !quote.can_commit() && quote.weights==
                  (std::array<std::int64_t,7>{107,0,-100,210,0,0,0}) &&
              quote.draws.size()==2 &&
              quote.draws[0].approach==0 && quote.draws[0].native_bound==15 &&
              quote.draws[0].value==7 &&
              quote.draws[1].approach==3 && quote.draws[1].native_bound==30 &&
              quote.draws[1].value==10 &&
              quote.rng_before==before && quote.rng_after.seed==1495354192 &&
              quote.rng_after.calls==2 && gameplay.state()==before,
              "BNW approach JonRand quote changed source or sequence");

        // The native int bound narrows to unsigned short before CvRandom.
        // A positive 65536 becomes zero, yet get(0) advances the stream.
        const auto zero=quote_bnw_approach_random_weights(
            std::array<std::int64_t,7>{65536,0,0,0,0,0,0},1,gameplay);
        check(zero.gate==BnwApproachRandomGate::next_can_declare_war_unimplemented &&
              zero.draws.size()==1 && zero.draws[0].native_bound==0 &&
              zero.draws[0].value==0 && zero.rng_after.calls==1 &&
              zero.rng_after.seed==2207042835 && gameplay.state()==before,
              "BNW native zero-bound draw was omitted");

        const auto overflow=quote_bnw_approach_random_weights(
            std::array<std::int64_t,7>{INT32_MAX,0,0,0,0,0,0},15,gameplay);
        check(overflow.gate==BnwApproachRandomGate::native_arithmetic_unavailable &&
              !overflow.can_commit() && gameplay.state()==before,
              "BNW native overflow published RNG state");
        const SimulationRandom foreign(RandomAlgorithm::vp_reference,{2,0,0});
        check(quote_bnw_approach_random_weights(
                  std::array<std::int64_t,7>{},15,foreign).gate==
                  BnwApproachRandomGate::random_rule_unavailable,
              "BNW quote accepted non-BNW RNG");
        std::cout << "BNW detached first-contact JonRand quote PASS\n";
        return 0;
    } catch(const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
