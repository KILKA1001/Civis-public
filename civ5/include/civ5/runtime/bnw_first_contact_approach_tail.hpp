#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace civ5::runtime {
enum class BnwApproachTailGate : std::uint8_t {
    input_unavailable, native_arithmetic_unavailable,
    native_sort_tie_unverified, quoted,
};
struct BnwApproachTailInput {
    std::array<std::int64_t,7> weights{};
    bool target_human=false;
    std::int64_t human_war_modifier=100;
    std::int64_t existing_war_face=-1;
    bool denounced_target=false;
};
struct BnwApproachTailQuote {
    BnwApproachTailGate gate=BnwApproachTailGate::input_unavailable;
    std::array<std::int64_t,7> weights{};
    std::optional<std::size_t> approach;
    std::optional<std::int64_t> war_face;
    std::int64_t highest_weight=0;
    bool war_face_write_required=false;
    // All candidates tied for the native sort's first slot, or for its
    // second slot when first is WAR and no WarFace exists yet.
    std::vector<std::size_t> ambiguous_candidates;
    std::string reason;
    bool can_commit() const noexcept {return false;}
};
// Expansion2 CvDiplomacyAI.cpp:3449-3527. Uses native int multiplication
// and truncating division, then CvWeightedVector's descending weight
// comparator. SortItems is std::sort without a tie key; differing observable
// outcomes from tied elements stay blocked instead of inventing a rule.
BnwApproachTailQuote quote_bnw_first_contact_approach_tail(
    const BnwApproachTailInput &input);
} // namespace civ5::runtime