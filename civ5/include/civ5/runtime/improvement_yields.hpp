#pragma once
#include "civ5/runtime/city_yields.hpp"
#include "civ5/runtime/map_geometry.hpp"
#include "civ5/runtime/yield_fraction.hpp"
#include <array>

namespace civ5::runtime {
enum class ImprovementYieldKind : std::size_t {
    base, per_era, adjacent_city, adjacent_mountain, coastal, fresh_water, hills,
    river, wltkd, golden_age, count
};
using ImprovementYieldMatrix=std::map<std::int64_t,YieldValues>;
using ImprovementFractionMatrix=std::map<std::int64_t,std::map<std::int64_t,YieldFraction>>;
struct ImprovementYieldInfo {
    std::int64_t authored_id=-1;
    std::array<YieldValues,static_cast<std::size_t>(ImprovementYieldKind::count)> yields;
    ImprovementYieldMatrix resources, routes, technologies, fresh_technologies, dry_technologies;
    ImprovementYieldMatrix features, adjacent_features, adjacent_resources, adjacent_terrains;
    // Source orientation: yield -> adjacent type. Zero entries remain present.
    ImprovementFractionMatrix fractional_improvements,fractional_terrains;
};
struct ImprovementYieldRules {
    content::TechnologyProfile profile=content::TechnologyProfile::bnw;
    std::map<std::int64_t,ImprovementYieldInfo> improvements;
    std::map<std::int64_t,ImprovementYieldMatrix> policies;
    std::map<std::int64_t,YieldValues> routes;
    std::vector<std::int64_t> yields;
    std::uint64_t improvement_slots=0,technology_slots=0,route_slots=0,policy_slots=0,relation_rows=0;
};
ImprovementYieldRules load_improvement_yield_rules(const content::RulesDatabase&,content::TechnologyProfile);
struct ImprovementAdjacencyYield {
    YieldFraction improvements,terrains;
    // The source combines both fractions BEFORE its one truncation.
    std::int64_t combined() const;
};
ImprovementAdjacencyYield improvement_fractional_adjacency(const saves::SavedMapProjection&,
    const ImprovementYieldRules&,const content::InfoTypeCatalog&,std::int64_t plot,
    std::int64_t improvement,std::int64_t yield);
} // namespace civ5::runtime
