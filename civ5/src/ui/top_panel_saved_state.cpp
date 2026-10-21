#include "civ5/ui/top_panel_saved_state.hpp"
#include <limits>
#include <stdexcept>
#include <string_view>

namespace civ5::ui {
namespace {
using saves::NamedSaveInteger;

std::optional<std::int64_t> integer(const std::vector<NamedSaveInteger> &fields, std::string_view name) {
    for (const auto &field : fields)
        if (field.name && field.name == name)
            return field.value;
    return std::nullopt;
}

const saves::PlayerAdministrationRecord *block(const saves::PlayerAdministrationObservation &observation,
                                               std::string_view name) {
    for (const auto &candidate : observation.blocks)
        if (candidate.name && candidate.name == name)
            return &candidate;
    return nullptr;
}

std::int32_t narrow(std::int64_t value, std::string_view name) {
    if (value < std::numeric_limits<std::int32_t>::min() ||
        value > std::numeric_limits<std::int32_t>::max())
        throw std::runtime_error("Saved TopPanel value is outside int32 range: " + std::string(name));
    return static_cast<std::int32_t>(value);
}

std::optional<std::int32_t> narrow_optional(const std::optional<std::int64_t> &value, std::string_view name) {
    return value ? std::optional<std::int32_t>(narrow(*value, name)) : std::nullopt;
}

std::optional<TopPanelSavedScaledValue> scaled(const std::vector<saves::PlayerCoreField> &fields,
                                               std::string_view name, std::int32_t scale) {
    for (const auto &field : fields) {
        if (!field.name || field.name != name)
            continue;
        if (field.values.size() != 1)
            throw std::runtime_error("Saved TopPanel scalar has an unexpected shape: " + std::string(name));
        return TopPanelSavedScaledValue{field.values.front(), scale};
    }
    return std::nullopt;
}

std::optional<std::int32_t> core_integer(const saves::PlayerCoreObservation &core, std::string_view name) {
    for (const auto &field : core.fields) {
        if (!field.name || field.name != name)
            continue;
        if (field.values.size() != 1)
            throw std::runtime_error("Saved TopPanel scalar has an unexpected shape: " + std::string(name));
        return narrow(field.values.front(), name);
    }
    return std::nullopt;
}
} // namespace

TopPanelSavedProjection project_top_panel_saved_player(const saves::SavedPlayerObservation &observation) {
    TopPanelSavedProjection result;
    if (observation.cities.cities.size() > static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max()))
        throw std::runtime_error("Saved TopPanel city count is outside int32 range");
    result.city_count = static_cast<std::int32_t>(observation.cities.cities.size());

    if (const auto *treasury = block(observation.administration, "treasury")) {
        result.gold = narrow_optional(integer(treasury->fields, "m_iGold"), "m_iGold");
        result.gold_per_turn_from_diplomacy =
            narrow_optional(integer(treasury->fields, "m_iGoldPerTurnFromDiplomacy"),
                            "m_iGoldPerTurnFromDiplomacy");
        result.city_connection_gold_times100 =
            narrow_optional(integer(treasury->fields, "m_iCityConnectionGoldTimes100"),
                            "m_iCityConnectionGoldTimes100");
        result.unit_cost = narrow_optional(integer(treasury->fields, "m_iExpensePerTurnUnitMaintenance"),
                                           "m_iExpensePerTurnUnitMaintenance");
        result.unit_supply = narrow_optional(integer(treasury->fields, "m_iExpensePerTurnUnitSupply"),
                                             "m_iExpensePerTurnUnitSupply");
        result.building_maintenance =
            narrow_optional(integer(treasury->fields, "m_iBaseBuildingGoldMaintenance"),
                            "m_iBaseBuildingGoldMaintenance");
        result.improvement_maintenance =
            narrow_optional(integer(treasury->fields, "m_iBaseImprovementGoldMaintenance"),
                            "m_iBaseImprovementGoldMaintenance");
    }

    const bool vp = observation.core.layout == saves::GameStateLayout::vp542_corpus;
    result.happiness_from_buildings = core_integer(observation.core, "m_iHappinessFromBuildings");
    result.extra_happiness_per_luxury = core_integer(observation.core, "m_iExtraHappinessPerLuxury");
    result.happiness_per_city = core_integer(observation.core, "m_iHappinessPerCity");
    result.happiness_from_minors = core_integer(observation.core, "m_iHappinessFromMinorCivs");
    result.total_happiness = core_integer(observation.core, "m_iHappinessTotal");
    result.total_unhappiness = core_integer(observation.core, "m_iUnhappiness");
    result.golden_age_progress =
        scaled(observation.core.fields, vp ? "m_iGoldenAgeProgressMeterTimes100" : "m_iGoldenAgeProgressMeter",
               vp ? 100 : 1);
    result.golden_age_turns = core_integer(observation.core, "m_iGoldenAgeTurns");
    result.culture = scaled(observation.core.fields, vp ? "m_iJONSCultureTimes100" : "m_iJONSCulture",
                            vp ? 100 : 1);
    result.faith = scaled(observation.core.fields, vp ? "m_iFaithTimes100" : "m_iFaith",
                          vp ? 100 : 1);
    result.next_policy_cost = core_integer(observation.core, "m_iCostNextPolicy");
    result.unit_production_maintenance_mod =
        core_integer(observation.core, "m_iUnitProductionMaintenanceMod");
    return result;
}

void apply_top_panel_saved_projection(TopPanelPlayerState &state, const TopPanelSavedProjection &projection) {
    if (projection.city_count)
        state.city_count = *projection.city_count;
    if (projection.gold)
        state.gold = *projection.gold;
    if (projection.gold_per_turn_from_diplomacy)
        state.gold_per_turn_from_diplomacy = *projection.gold_per_turn_from_diplomacy;
    if (projection.city_connection_gold_times100)
        state.city_connection_gold_times100 = *projection.city_connection_gold_times100;
    if (projection.unit_cost)
        state.unit_cost = *projection.unit_cost;
    if (projection.unit_supply)
        state.unit_supply = *projection.unit_supply;
    if (projection.building_maintenance)
        state.building_maintenance = *projection.building_maintenance;
    if (projection.improvement_maintenance)
        state.improvement_maintenance = *projection.improvement_maintenance;
    if (projection.happiness_from_buildings)
        state.happiness_from_buildings = *projection.happiness_from_buildings;
    if (projection.extra_happiness_per_luxury)
        state.extra_happiness_per_luxury = *projection.extra_happiness_per_luxury;
    if (projection.happiness_per_city)
        state.extra_happiness_per_city = *projection.happiness_per_city;
    if (projection.happiness_from_minors)
        state.happiness_from_minors = *projection.happiness_from_minors;
    if (projection.total_happiness)
        state.total_happiness = *projection.total_happiness;
    if (projection.total_unhappiness)
        state.total_unhappiness = *projection.total_unhappiness;
    if (projection.golden_age_progress && projection.golden_age_progress->scale == 1)
        state.golden_age_progress = narrow(projection.golden_age_progress->raw, "golden_age_progress");
    if (projection.golden_age_turns)
        state.golden_age_turns = *projection.golden_age_turns;
    if (projection.culture && projection.culture->scale == 1)
        state.culture = narrow(projection.culture->raw, "culture");
    if (projection.faith && projection.faith->scale == 1)
        state.faith = narrow(projection.faith->raw, "faith");
    if (projection.next_policy_cost)
        state.next_policy_cost = *projection.next_policy_cost;
    if (projection.unit_production_maintenance_mod)
        state.unit_production_maintenance_mod = *projection.unit_production_maintenance_mod;
}

TopPanelSavedProjection project_top_panel_saved_player(const saves::SavedSessionSnapshot &snapshot,
                                                       std::int64_t saved_player_id) {
    if (!snapshot.all_players)
        throw std::runtime_error("Saved session has no decoded players");
    const saves::SavedPlayerObservation *selected = nullptr;
    for (const auto &player : snapshot.all_players->players)
        if (player.saved_player_id == saved_player_id) {
            if (selected)
                throw std::runtime_error("Ambiguous saved TopPanel player identity");
            selected = &player;
        }
    if (!selected)
        throw std::runtime_error("Saved TopPanel player identity is absent from the decoded session");
    return project_top_panel_saved_player(*selected);
}
} // namespace civ5::ui
