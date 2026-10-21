#pragma once
#include "civ5/runtime/production_progress.hpp"
#include "civ5/saves/civis_save.hpp"

namespace civ5::runtime {
struct ProductionCheckpointBudget {
    std::uint64_t players=1'000'000,cities=10'000'000,entries=100'000'000;
    std::uint64_t bytes=1024ULL*1024*1024;
};
// Complete common city-production state, independent of any VP-only modifier.
// The active Info mapping and original city offsets are bound into the section.
saves::CivisSaveSection make_production_checkpoint(const CityYieldContext&,
    const ProductionProgressRules&,const content::InfoTypeCatalog&,ProductionCheckpointBudget={});
// Parse and validate the whole section before changing a city. Missing mod
// types, a different profile, changed Info IDs or a different city roster reject.
void restore_production_checkpoint(const saves::CivisSaveSection&,
    const ProductionProgressRules&,const content::InfoTypeCatalog&,CityYieldContext&,
    ProductionCheckpointBudget={});
} // namespace civ5::runtime
