#pragma once
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
namespace civ5::ui {class SelectedBnwPreviewHost;}
namespace civ5::runtime {
struct MapGeometry;
enum class BnwGeographicCityStrategy {river,mountain,hill,forest,jungle,coast,landlocked};
class BnwCityGeographicStrategyUnavailable final:public std::runtime_error {
 public:using std::runtime_error::runtime_error;
};
// Pure comparison operands; no source owner/freshness/phase authority.
struct BnwGeographicPlotMath {
 std::optional<std::int64_t> plot_type,owner,feature,river_crossing_count;
};
bool bnw_city_geographic_strategy_math(BnwGeographicCityStrategy,const MapGeometry&,
 std::int64_t x,std::int64_t y,std::span<const BnwGeographicPlotMath>);
class BnwCityCurrentGeographicStrategy final {
 public:bool value(BnwGeographicCityStrategy)const;
 private:
  struct Impl;std::shared_ptr<Impl> impl_;
  explicit BnwCityCurrentGeographicStrategy(std::shared_ptr<Impl>)noexcept;
  void require_current()const;
  friend BnwCityCurrentGeographicStrategy inspect_bnw_city_current_geographic_strategy(
   std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int64_t,std::int64_t);
};
BnwCityCurrentGeographicStrategy inspect_bnw_city_current_geographic_strategy(
 std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int64_t owner,std::int64_t city);
}
