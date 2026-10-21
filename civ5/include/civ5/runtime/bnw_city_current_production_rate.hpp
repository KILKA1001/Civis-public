#pragma once
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>
namespace civ5::ui {class SelectedBnwPreviewHost;}
namespace civ5::runtime {
class BnwCityCurrentScienceRate;
class BnwCityCurrentProductionRateUnavailable final:public std::runtime_error {public:using std::runtime_error::runtime_error;};
// Genuine native getYieldRateTimes100(PRODUCTION,false), not a production order.
// No resistance/raze/anarchy suppression; recursive self-process is unavailable.
class BnwCityCurrentProductionRate final {
 public:std::int32_t value_times100()const;
 private:struct Impl;std::shared_ptr<Impl> impl_;
 explicit BnwCityCurrentProductionRate(std::shared_ptr<Impl>)noexcept;
 std::vector<std::int64_t> private_source_seal()const;
 friend class BnwCityCurrentScienceRate;
 friend BnwCityCurrentProductionRate inspect_bnw_city_current_production_rate(
  std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int64_t,std::int64_t);
};
BnwCityCurrentProductionRate inspect_bnw_city_current_production_rate(
 std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int64_t owner,std::int64_t city);
}
