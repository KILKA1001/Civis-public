#pragma once
#include "civ5/runtime/map_geometry.hpp"
#include <memory>
#include <stdexcept>
namespace civ5::ui {class SelectedBnwPreviewHost;}
namespace civ5::runtime {
enum class BnwLandImprovementCityStrategy {need_tile_improvers,want_tile_improvers,enough_tile_improvers,need_improvement_food,need_improvement_production};
class BnwCityLandImprovementUnavailable final:public std::runtime_error {
 public:using std::runtime_error::runtime_error;
};
namespace detail {
// Pure native operands only, never source/currentness/trigger grants.
std::int32_t bnw_citizen_working_plot_index(const MapGeometry&,
 std::int64_t city_x,std::int64_t city_y,std::int64_t plot_x,std::int64_t plot_y);
bool bnw_city_deficient_yield_math(double desired,std::int64_t amount,std::int64_t tiles);
bool bnw_need_land_worker_pressure_math(std::int64_t workers,std::int64_t weight,
 std::int64_t cities,std::int64_t feature_cities,std::int64_t elapsed,
 std::int64_t desperate,std::int64_t train_percent);
}
class BnwCityCurrentLandImprovementStrategy final {
 public:
  bool value(BnwLandImprovementCityStrategy)const;
  std::int32_t deficient_yield()const;
 private:
  struct Impl;std::shared_ptr<Impl> impl_;
  explicit BnwCityCurrentLandImprovementStrategy(std::shared_ptr<Impl>)noexcept;
  void require_current()const;
  friend BnwCityCurrentLandImprovementStrategy inspect_bnw_city_current_land_improvement_strategy(
   std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int64_t,std::int64_t);
};
BnwCityCurrentLandImprovementStrategy inspect_bnw_city_current_land_improvement_strategy(
 std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int64_t owner,std::int64_t city);
}
