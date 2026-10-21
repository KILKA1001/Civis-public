#pragma once
#include "civ5/runtime/maintenance.hpp"

namespace civ5::runtime {
// VP-only saved expenses and contract state stay outside the common player.
struct VpMaintenancePlayerProfile {
    std::int64_t vassal_modifier=0, vassal_taxes_times100=0, outgoing_espionage_gold=0;
    YieldValues extra_building_percent;
    std::set<std::int64_t> active_contracts;
    std::set<std::int64_t> contract_units;
};
struct VpMaintenanceRulesProfile {
    std::set<std::int64_t> contracts;
};
struct VpMaintenanceContextProfile {
    std::vector<saves::ContractObservation> contracts;
};
struct VpMaintenanceQuote {
    std::int64_t vassals=0,taxes=0,contracts=0,espionage=0,diplomacy=0;
};
inline VpMaintenanceQuote &ensure_vp_maintenance_quote(MaintenanceQuote &quote) {
    if(auto *state=quote.profile_extensions.find<VpMaintenanceQuote>())return *state;
    return quote.profile_extensions.emplace<VpMaintenanceQuote>();
}
inline const VpMaintenanceQuote &vp_maintenance_quote(const MaintenanceQuote &quote) {
    const auto *state=quote.profile_extensions.find<VpMaintenanceQuote>();
    if(!state)throw MaintenanceUnavailable("Missing VP maintenance quote");
    return *state;
}
inline VpMaintenanceRulesProfile &ensure_vp_maintenance_rules_profile(MaintenanceRules &rules) {
    if(rules.profile!=content::TechnologyProfile::vp542)
        throw MaintenanceUnavailable("VP contract rules require VP GameCore profile");
    if(auto *state=rules.extensions.find<VpMaintenanceRulesProfile>())return *state;
    return rules.extensions.emplace<VpMaintenanceRulesProfile>();
}
inline const VpMaintenanceRulesProfile &vp_maintenance_rules_profile(const MaintenanceRules &rules) {
    if(rules.profile!=content::TechnologyProfile::vp542)
        throw MaintenanceUnavailable("VP contract rules require VP GameCore profile");
    const auto *state=rules.extensions.find<VpMaintenanceRulesProfile>();
    if(!state)throw MaintenanceUnavailable("Missing VP maintenance rules profile");
    return *state;
}
inline VpMaintenanceContextProfile &ensure_vp_maintenance_context_profile(MaintenanceContext &world) {
    if(world.profile!=content::TechnologyProfile::vp542)
        throw MaintenanceUnavailable("VP contracts require VP GameCore profile");
    if(auto *state=world.extensions.find<VpMaintenanceContextProfile>())return *state;
    return world.extensions.emplace<VpMaintenanceContextProfile>();
}
inline const VpMaintenanceContextProfile &vp_maintenance_context_profile(const MaintenanceContext &world) {
    if(world.profile!=content::TechnologyProfile::vp542)
        throw MaintenanceUnavailable("VP contracts require VP GameCore profile");
    const auto *state=world.extensions.find<VpMaintenanceContextProfile>();
    if(!state)throw MaintenanceUnavailable("Missing VP maintenance context profile");
    return *state;
}
inline VpMaintenancePlayerProfile &ensure_vp_maintenance_profile(
    MaintenancePlayer &player, content::TechnologyProfile profile) {
    if(profile!=content::TechnologyProfile::vp542)
        throw MaintenanceUnavailable("VP maintenance state requires VP GameCore profile");
    if(auto *state=player.profile_extensions.find<VpMaintenancePlayerProfile>())return *state;
    return player.profile_extensions.emplace<VpMaintenancePlayerProfile>();
}
inline const VpMaintenancePlayerProfile &vp_maintenance_profile(const MaintenancePlayer &player) {
    const auto *state=player.profile_extensions.find<VpMaintenancePlayerProfile>();
    if(!state)throw MaintenanceUnavailable("Missing VP maintenance profile");
    return *state;
}
inline VpMaintenancePlayerProfile &vp_maintenance_profile(MaintenancePlayer &player) {
    auto *state=player.profile_extensions.find<VpMaintenancePlayerProfile>();
    if(!state)throw MaintenanceUnavailable("Missing VP maintenance profile");
    return *state;
}
} // namespace civ5::runtime
