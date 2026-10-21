#pragma once
#include "civ5/runtime/live_session.hpp"
#include "civ5/runtime/session_checkpoint_identity.hpp"
namespace civ5::runtime {
// Borrowed, non-authorizing lookup over the existing CURRENT raw/owned city
// registry binding. Retain the input state across the query and pointer use.
// No Game lifecycle, layout event, placement or renderer proof is minted.
// Missing/malformed current registry inputs throw; native absent handles/holes
// return nullptr. Supports native FLTA index-only and full-ID semantics.
const saves::SavedCityObservation *resolve_bnw_current_city(
    const LiveSessionState &,std::int32_t owner,std::int32_t id);
class BnwCityStrategyYieldRules {
 public:
    struct Specialization {std::string type;bool is_default=false;std::int32_t yield=-1;std::array<std::int32_t,6> targets{};};
    static BnwCityStrategyYieldRules load(const content::RulesDatabase &);
    const auto &identity()const noexcept{return sha256_;}
    const auto &specializations()const noexcept{return specializations_;}
    const auto &buildings()const noexcept{return building_food_;}
    const auto &types()const noexcept{return types_;}
    std::int32_t min_city_food()const noexcept{return min_food_;}
    std::int32_t building_count(std::int32_t real,std::int32_t free)const;
 private:
    std::string sha256_;content::InfoTypeCatalog types_;
    std::map<std::int32_t,Specialization> specializations_;
    std::map<std::int32_t,std::int32_t> building_food_;
    std::int32_t min_food_=0,max_buildings_=0;
};
struct BnwCityStrategyPlotSample {
    std::uint32_t city_slot=0;std::uint64_t plot=0;
    std::array<std::int16_t,6> native_yields{};
    std::array<std::uint8_t,6> narrowed_yields{};
};
struct BnwCityStrategyYieldQuote {
    std::int64_t owner=-1,city=-1;
    std::int32_t population=0,evaluated=0,slots=0,food_extra=0,specialization=-1,focus_yield=-1;
    std::array<std::uint16_t,6> average{};
    // Absent on the source early return. It is never inferred zero/reset.
    std::optional<std::array<std::int16_t,6>> delta;
    std::vector<BnwCityStrategyPlotSample> samples;
};
// CvCityStrategyAI::UpdateBestYields is a CURRENT cache query: CvPlot::getYield
// directly reads native short m_aiYield. It is not calculateYield/CityYieldRate.
// Real working-city refs/FLTA, source37plot order/wrapping, uchar narrowing,
// stable descending sort, food-building single presence, specialization fallback
// and ushort/short stores are all evaluated. Missing current operands reject.
BnwCityStrategyYieldQuote quote_bnw_city_strategy_best_yields(
    const LiveSessionState &,const BnwCityStrategyYieldRules &,
    const SessionCheckpointIdentity &,std::int64_t owner,std::int64_t city);
struct BnwCityStrategyRetainedYield {
    std::int32_t focus_yield=-1;
    std::optional<std::array<std::uint16_t,6>> average;
    std::optional<std::array<std::int16_t,6>> delta;
    std::uint64_t updates=0;
};
class BnwCityStrategyYieldOwner {
 public:
    const auto &cities()const noexcept{return cities_;}
 private:
    friend BnwCityStrategyYieldOwner bind_bnw_city_strategy_yields_original_import(const saves::SavedSession &,const SessionCheckpointIdentity &);
    friend void update_bnw_city_strategy_best_yields(const LiveSessionState &,BnwCityStrategyYieldOwner &,const BnwCityStrategyYieldRules &,std::int64_t,std::int64_t);
    SessionCheckpointIdentity identity_;std::array<std::uint8_t,16> guid_{};
    std::map<std::pair<std::int64_t,std::int64_t>,BnwCityStrategyRetainedYield> cities_;
};
// Actual Read resets only focus=-1. Average/delta are NOT serialized/reset by
// Read, so they start unknown until actual UpdateBestYields writes each one.
BnwCityStrategyYieldOwner bind_bnw_city_strategy_yields_original_import(
    const saves::SavedSession &,const SessionCheckpointIdentity &);
void update_bnw_city_strategy_best_yields(const LiveSessionState &,BnwCityStrategyYieldOwner &,
    const BnwCityStrategyYieldRules &,std::int64_t owner,std::int64_t city);
}
