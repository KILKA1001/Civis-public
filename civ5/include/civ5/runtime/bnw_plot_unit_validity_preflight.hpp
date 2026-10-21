#pragma once

#include "civ5/runtime/bnw_map_occupied_preflight.hpp"
#include "civ5/runtime/bnw_map_turn.hpp"
#include <string>
#include <vector>

namespace civ5::runtime {

enum class BnwPlotUnitCheck {
    saved_identity, delayed_death, at_plot, cargo, in_combat,
    stack_limit, valid_domain, can_enter_territory,
    same_owner_team, unit_team, plot_team, visible_enemy, invisible
};
enum class BnwPlotUnitEffectCause {
    stack_limit, invalid_domain_or_territory, visible_enemy
};
struct BnwPlotUnitStep {
    std::uint64_t plot=0;
    saves::SavedPlotUnitReference handle;
    unsigned pass=0;
    BnwPlotUnitCheck check=BnwPlotUnitCheck::saved_identity;
    bool resolved=false,live_proof=false;
    std::int64_t value=0;
};
struct BnwPlotUnitEffect {
    std::uint64_t plot=0;
    saves::SavedPlotUnitReference handle;
    unsigned pass=0;
    BnwPlotUnitEffectCause cause=BnwPlotUnitEffectCause::stack_limit;
    // SDK first calls jumpToNearestValidPlot and calls kill(false) only if
    // jump fails. A successful stack jump may still lead to a second jump.
    bool jump_required=true,kill_if_jump_fails=true;
};
struct BnwPlotUnitValidityPreflight {
    bool source_valid=false,complete_no_effect=false;
    std::string reason;
    std::vector<BnwPlotUnitStep> steps;
    std::vector<BnwPlotUnitEffect> effects;
};
struct BnwPlotUnitReadServices {
    // Revalidate load-time object/link predicates against the live registry.
    // Saved flags are still reported when a callback is absent, but cannot
    // authorize the executable no-effect branch.
    BnwUnitValidityProbe exists,delayed_death,at_plot,cargo,in_combat;
    BnwUnitValidityNumber friendly_units_of_type,unit_team,plot_team;
    BnwUnitValidityProbe valid_domain,can_enter_territory;
    BnwUnitValidityProbe visible_enemy,invisible;
};

// Read-only SDK-order simulation for one plot. Saved identity, position,
// delayed-death, transport and combat inputs are checked against the raw
// world. Other predicates must be supplied by the live GameCore; absent
// answers stop at the first unresolved SDK branch. When a jump is required,
// the plan records jump/kill and stops because later predicates depend on the
// post-jump world. This function never mutates units or publishes events.
BnwPlotUnitValidityPreflight preflight_bnw_verify_unit_valid_plot(
    const LiveSessionState &state,std::uint64_t plot_index,
    std::int64_t plot_unit_limit,const BnwPlotUnitReadServices &services);

// Executable no-effect branch. It rechecks the live state immediately before
// returning success and never performs jump/kill. Any unresolved predicate or
// planned effect rejects; the caller still handles world-anchor removal.
LiveActionResult run_bnw_verified_no_effect_plot_units(
    LiveSessionState &state,std::uint64_t plot_index,
    std::int64_t plot_unit_limit,const BnwPlotUnitReadServices &services);

} // namespace civ5::runtime
