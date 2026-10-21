#pragma once
#include "civ5/content/custom_mod_options.hpp"
#include "civ5/runtime/vp_policy_cost.hpp"
#include <string>

namespace civ5::runtime {
struct VpPolicyPhaseQuote {
    bool outer_guard=false,human_policy_choice=false,human_diplomacy=false;
    bool vassal=false,affordable=false,safe_no_effect=false;
    std::int64_t ideology_branch=-1,culture_balance_times100=0,next_policy_cost=0;
    std::string unresolved;
};
// Prove the source's policy/ideology step has no effect before research. This
// recognizes a strict human path; other paths require policy AI, notification
// or ideology hosts and are reported unresolved rather than silently skipped.
VpPolicyPhaseQuote vp_policy_phase_without_host(
    const ResearchContext &world,const PlayerResearchContext &players,
    const content::RulesDatabase &database,const content::CustomModOptions &options,
    std::int64_t player);
} // namespace civ5::runtime
