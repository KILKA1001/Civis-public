#pragma once
#include <cstdint>
#include <memory>
namespace civ5::ui {class SelectedBnwPreviewHost;}
namespace civ5::runtime {
class BnwCityProductionTurnPending;
class BnwCityCurrentProjectProductionDifference final {
 public:std::int32_t value_times100()const;
 private:struct Impl;std::shared_ptr<Impl> impl_;
 explicit BnwCityCurrentProjectProductionDifference(std::shared_ptr<Impl>);
 static BnwCityCurrentProjectProductionDifference bind(std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int64_t,std::int64_t);
 friend BnwCityCurrentProjectProductionDifference inspect_bnw_city_current_project_production_difference(std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int64_t,std::int64_t);
};
BnwCityCurrentProjectProductionDifference inspect_bnw_city_current_project_production_difference(std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int64_t,std::int64_t);
}
