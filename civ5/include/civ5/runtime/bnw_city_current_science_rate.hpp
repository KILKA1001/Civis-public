#pragma once
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <vector>
#include <utility>
namespace civ5::ui {class SelectedBnwPreviewHost;class BnwEngineUiTurnOwner;}
namespace civ5::runtime {
class BnwCityCurrentProductionRate;
class BnwCityCurrentScienceRateUnavailable final:public std::runtime_error {public:using std::runtime_error::runtime_error;};
// Genuine current getYieldRateTimes100(SCIENCE,false), with SAME-host Production child.
// No city-refresh, research-credit, turn-phase or continuation capability.
class BnwCityCurrentScienceRate final {
 public:std::int32_t value_times100()const;
 private:
 friend class BnwCityProductionTurnPending;
 friend class ui::BnwEngineUiTurnOwner;
 enum class SourceKind {science,production,production_difference,gold};
 static BnwCityCurrentScienceRate bind_current_difference_source(std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int64_t,std::int64_t,std::int32_t,std::int32_t);
 static BnwCityCurrentScienceRate bind_current_difference_inputs(std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int64_t,std::int64_t,SourceKind,std::int32_t,std::int32_t);
 static BnwCityCurrentScienceRate bind_current_source(std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int64_t,std::int64_t,SourceKind);
 std::vector<std::int64_t> current_source_seal()const;
 friend class BnwCityCurrentProductionRate;
 friend BnwCityCurrentProductionRate inspect_bnw_city_current_production_rate(std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int64_t,std::int64_t);
 struct Impl;std::shared_ptr<Impl> impl_;
 explicit BnwCityCurrentScienceRate(std::shared_ptr<Impl>)noexcept;
 void require_current()const;
 std::pair<std::vector<std::int64_t>,std::int32_t> capture()const;
 friend BnwCityCurrentScienceRate inspect_bnw_city_current_science_rate(
  std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int64_t,std::int64_t);
};
BnwCityCurrentScienceRate inspect_bnw_city_current_science_rate(
 std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int64_t owner,std::int64_t city);
}
