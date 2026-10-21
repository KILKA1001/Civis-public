#pragma once
#include "civ5/runtime/map_geometry.hpp"
#include <array>
#include <memory>
#include <span>
#include <stdexcept>
namespace civ5::ui {class SelectedBnwPreviewHost;}
namespace civ5::content {struct InfoTypeEntry;}
namespace civ5::saves {struct SavedSessionSnapshot;struct SavedUnitObservation;struct SavedMapProjection;}
namespace civ5::runtime {
// Fixed source selectors: all include current and head-TRAIN units.
// Workers/settlers exclude water; sea_workers and settlers_including_water
// include water. SDK GetNumUnitsWithUnitAI defaults includeWater=true.
enum class BnwCityNativeUnitCount {workers,sea_workers,settlers,settlers_including_water};
enum class BnwNavalCityStrategy {under_blockade,need_naval_growth,need_naval_tile_improvement,enough_naval_tile_improvement};
class BnwCityNavalStrategyUnavailable final:public std::runtime_error {
 public:using std::runtime_error::runtime_error;
};
namespace detail {
// Value identity only: name/hash indexes own separate copies. No source owner.
bool bnw_naval_info_same_identity(const content::InfoTypeEntry*,
 const content::InfoTypeEntry*)noexcept;
// Supplied geometry/container/math only; none of these functions issues a
// genuine session, a trigger-completion receipt or a gameplay effect.
// Native plotCityXY inverse; shared with live plot-yield refresh.
std::int32_t bnw_citizen_plot_slot(const saves::SavedMapProjection&,
 std::int32_t cx,std::int32_t cy,std::int32_t px,std::int32_t py);
std::array<std::optional<std::int64_t>,37> bnw_citizen_plot_positions(
 const MapGeometry&,std::int64_t x,std::int64_t y);
const saves::SavedUnitObservation* bnw_naval_unit_lookup(
 const saves::SavedSessionSnapshot&,std::int64_t owner,std::int64_t handle);
bool bnw_naval_growth_threshold_math(std::int64_t ocean,std::int64_t total,
 std::span<const std::int64_t> personality,std::span<const std::int64_t> modifiers,
 std::int64_t threshold);
}
class BnwCityCurrentNavalStrategy final {
 public:
  bool value(BnwNavalCityStrategy)const;
  std::int32_t unit_count(BnwCityNativeUnitCount)const;
  bool plot_blockaded(std::uint64_t plot)const;
  bool can_work(std::uint64_t plot)const;
 private:
  struct Impl;std::shared_ptr<Impl> impl_;
  explicit BnwCityCurrentNavalStrategy(std::shared_ptr<Impl>)noexcept;
  void require_current()const;
  friend BnwCityCurrentNavalStrategy inspect_bnw_city_current_naval_strategy(
   std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int64_t,std::int64_t);
};
// SAME genuine SelectedHost/session/immutable DB. This is a current read,
// never SetUsing/Lua/flavor publication, city phase completion or fresh food.
BnwCityCurrentNavalStrategy inspect_bnw_city_current_naval_strategy(
 std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int64_t owner,std::int64_t city);
}
