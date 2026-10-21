#include "civ5/runtime/bnw_first_contact_approach_tail.hpp"
#include <algorithm>
#include <climits>
#include <stdexcept>

namespace civ5::runtime {
namespace {
std::int64_t inferred_face(std::size_t approach,bool denounced) {
    // HOSTILE=1 -> HOSTILE=0; DECEPTIVE=2, AFRAID=4, FRIENDLY=5
    // -> FRIENDLY=2 unless denounced; remaining approaches -> NEUTRAL=1.
    if(approach==1) return 0;
    if(approach==2 || approach==4 || approach==5) return denounced?1:2;
    return 1;
}
} // namespace
BnwApproachTailQuote quote_bnw_first_contact_approach_tail(
    const BnwApproachTailInput &input) {
    BnwApproachTailQuote out;
    out.weights=input.weights;
    if(input.existing_war_face < -1 || input.existing_war_face>2 ||
       input.human_war_modifier<INT32_MIN || input.human_war_modifier>INT32_MAX ||
       std::any_of(out.weights.begin(),out.weights.end(),[](auto value) {
           return value<INT32_MIN || value>INT32_MAX;
       })) {
        out.reason="BNW approach tail has an invalid native weight, modifier or WarFace";
        return out;
    }
    if(input.target_human) {
        // The multiplication happens before /100 in the SDK. Widen only to
        // detect overflow; never silently accept a value native int cannot hold.
        for(const auto index:{std::size_t(0),std::size_t(2)}) {
            const auto product=out.weights[index]*input.human_war_modifier;
            if(product<INT32_MIN || product>INT32_MAX) {
                out.gate=BnwApproachTailGate::native_arithmetic_unavailable;
                out.reason="BNW human handicap approach multiplication overflows native int";
                return out;
            }
            out.weights[index]=product/100;
        }
    }
    out.highest_weight=*std::max_element(out.weights.begin(),out.weights.end());
    for(std::size_t index=0;index<7;++index)
        if(out.weights[index]==out.highest_weight)
            out.ambiguous_candidates.push_back(index);
    if(out.ambiguous_candidates.size()!=1) {
        out.gate=BnwApproachTailGate::native_sort_tie_unverified;
        out.reason="BNW native SortItems has multiple best approaches without an attested retail tie order";
        return out;
    }
    out.approach=out.ambiguous_candidates[0];
    out.ambiguous_candidates.clear();
    if(*out.approach!=0) out.war_face=-1;
    else if(input.existing_war_face!=-1) out.war_face=input.existing_war_face;
    else {
        const auto second=*std::max_element(out.weights.begin()+1,out.weights.end());
        std::optional<std::int64_t> face;
        bool differs=false;
        for(std::size_t index=1;index<7;++index) {
            if(out.weights[index]!=second) continue;
            out.ambiguous_candidates.push_back(index);
            const auto candidate=inferred_face(index,input.denounced_target);
            if(face && *face!=candidate) differs=true;
            else face=candidate;
        }
        if(differs) {
            out.gate=BnwApproachTailGate::native_sort_tie_unverified;
            out.reason="BNW native second-place tie changes WarFace without an attested retail tie order";
            return out;
        }
        // A tie whose possible second elements all give the same WarFace
        // is observable-equivalent and needs no invented sorting tie key.
        out.war_face=face;
        out.war_face_write_required=true;
        out.ambiguous_candidates.clear();
    }
    out.gate=BnwApproachTailGate::quoted;
    out.reason="BNW human handicap, best approach and WarFace computed privately; stock Lua publication boundary remains";
    return out;
}
} // namespace civ5::runtime