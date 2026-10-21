#pragma once
#include "civ5/saves/city_core.hpp"
#include "civ5/saves/city_management.hpp"
#include "civ5/saves/city_production.hpp"
#include "civ5/saves/city_tail.hpp"

namespace civ5::saves {
struct CitiesContext {
    std::optional<std::uint64_t> bnw_projects;
    runtime::ProfileExtensions profile_extensions;
};
struct CitiesBudget {
    GameStateBudget per_block;
    // Resource budgets can be raised by the caller; these are not source game capacities.
    std::uint64_t retained_elements = 8 * 1024 * 1024;
    std::uint64_t retained_string_bytes = 64 * 1024 * 1024;
};
struct SavedCityObservation {
    std::uint64_t index = 0;
    CityCoreObservation core;
    CityProductionObservation production;
    CityManagementObservation management;
    CityTailObservation tail;
};
struct CitiesObservation {
    GameStateLayout layout;
    CitiesContext context;
    CityCollectionHeader header;
    std::uint64_t start_offset = 0, end_offset = 0, retained_elements = 0, retained_string_bytes = 0;
    std::vector<SavedCityObservation> cities;
};
// Reads the header and every complete city in source order, stopping before m_units.
// No ID masking, insertion into original fixed-capacity containers, or load effects.
// Empty collections need no content dimensions. Nonempty BNW cities each supply
// their own saved version to their production and tail readers.
CitiesObservation observe_cities(runtime::BinaryReader &input, GameStateLayout layout,
                                 CitiesContext context = {}, CitiesBudget budget = {});
} // namespace civ5::saves
