#include "civ5/runtime/vp_city_yield_cache_profile.hpp"
#include <string_view>

namespace civ5::runtime {
void restore_vp_city_yield_cache(CityYieldContext &world, std::int64_t owner,
                                 std::int64_t city, const saves::CityCoreObservation &source) {
    if (world.profile != content::TechnologyProfile::vp542 ||
        source.layout != saves::GameStateLayout::vp542_corpus)
        throw CityYieldUnavailable("VP city yield cache needs matching GameCore profile");
    const saves::CityCoreField *field = nullptr;
    for (const auto &candidate : source.fields) {
        if (candidate.name && std::string_view(candidate.name) == "m_aiStaticCityYield") {
            if (field) throw CityYieldUnavailable("Duplicate VP city yield cache field");
            field = &candidate;
        }
    }
    if (!field || field->values.size() != world.yield_slots)
        throw CityYieldUnavailable("VP city yield cache wire span mismatch");
    YieldValues values;
    for (std::size_t index = 0; index < field->values.size(); ++index) {
        if (field->values[index]) {
            if (index > INT64_MAX) throw CityYieldUnavailable("VP city yield ID exceeds int64");
            values.emplace(static_cast<std::int64_t>(index), field->values[index]);
        }
    }
    vp_city_cached_yields(world, owner, city) = std::move(values);
}

CityYieldQuote vp_city_cached_yield_quote(const CityYieldContext &world, std::int64_t owner,
                                          std::int64_t city, std::int64_t yield,
                                          bool ignore_trade) {
    if (ignore_trade)
        throw CityYieldUnavailable("VP ignore-trade query requires city yield recomputation");
    CityYieldQuote quote;
    quote.cached = true;
    quote.total_times100 = vp_city_cached_yield(world, owner, city, yield);
    return quote;
}
} // namespace civ5::runtime
