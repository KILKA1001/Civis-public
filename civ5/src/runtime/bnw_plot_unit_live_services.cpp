#include "civ5/runtime/bnw_plot_unit_live_services.hpp"
#include "civ5/runtime/bnw_native_unit_allocator_core.hpp"
#include "civ5/runtime/bnw_map_occupied_preflight.hpp"
#include <optional>
#include <utility>

namespace civ5::runtime {
namespace {
using Member=bool BnwOccupiedPlotUnit::*;

std::optional<bool> current_unit_field(
    const LiveSessionState &state,std::uint64_t plot,
    saves::SavedPlotUnitReference handle,Member field) {
    // This is the mutable session's owned CvUnit collection, crosschecked
    // against both current raw and projected plot rosters. A stale detached
    // map, duplicate handle, or missing unit never becomes an affirmative
    // GameCore answer.
    const auto roster=preflight_bnw_map_occupied(state.world);
    if(!roster.shape_valid) return std::nullopt;
    for(const auto &unit:roster.units)
        if(unit.plot==plot && unit.handle==handle)
            return unit.*field;
    return std::nullopt;
}

BnwUnitValidityProbe from_member(Member field) {
    return [field](const LiveSessionState &state,std::uint64_t plot,
                   saves::SavedPlotUnitReference handle) {
        return current_unit_field(state,plot,handle,field);
    };
}
} // namespace

BnwPlotUnitReadServices make_bnw_plot_unit_no_effect_services(
    BnwPlotDomainRules domain_rules,
    const content::RulesDatabase *active_rules) {
    BnwPlotUnitReadServices services;
    services.exists=from_member(&BnwOccupiedPlotUnit::saved_unit_found);
    services.delayed_death=from_member(&BnwOccupiedPlotUnit::delayed_death);
    services.at_plot=from_member(&BnwOccupiedPlotUnit::at_saved_plot);
    services.cargo=from_member(&BnwOccupiedPlotUnit::transport_link);
    services.in_combat=from_member(&BnwOccupiedPlotUnit::combat_link);
    services.valid_domain=bind_bnw_plot_valid_domain(std::move(domain_rules));
    services.can_enter_territory=
        make_bnw_can_enter_territory_probe(active_rules);
    services.visible_enemy=make_bnw_visible_enemy_probe();
    services.invisible=make_bnw_unit_invisible_probe();
    return services;
}
BnwPlotUnitReadServices make_bnw_plot_unit_no_effect_services(
    BnwPlotDomainRules domain_rules,const content::RulesDatabase*active_rules,
    const std::shared_ptr<const BnwNativeUnitAllocatorCore>&allocator){
    if(!allocator)throw BnwNativeUnitAllocatorUnavailable("Native plot read binder requires a canonical actual Core owner");
    return allocator->bind_native_existing_plot_unit_reads(make_bnw_plot_unit_no_effect_services(std::move(domain_rules),active_rules));
}
} // namespace civ5::runtime
