#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <vector>
namespace civ5::ui {class SelectedBnwPreviewHost;}
namespace civ5::runtime {
enum class BnwCapitalCityStrategy {under_threat,need_settler};
struct BnwCityCapitalStrategyUnavailable : std::runtime_error {using std::runtime_error::runtime_error;};
// Supplied values are math only, never a current source/city/AI capability.
struct BnwCapitalThreatCityMath {
 std::optional<std::int64_t> threat,population,capital;
};
struct BnwCapitalCityStrategyMathInputs {
 std::optional<std::int64_t> capital,war_count,minor,city_count,settlers;
 std::vector<BnwCapitalThreatCityMath> threat_cities;
 std::optional<std::size_t> selected_city;
 std::optional<std::int64_t> capital_threat_percent;
 std::optional<std::int64_t> under_threat_type,game_turn,under_threat_using;
 std::optional<std::int64_t> mobilization_type,mobilization_using,weight_threshold;
 std::optional<std::vector<std::int64_t>> personality,threshold_modifiers;
};
bool bnw_capital_city_strategy_math(BnwCapitalCityStrategy,const BnwCapitalCityStrategyMathInputs&);
class BnwCityCurrentCapitalStrategy final {
 public: bool value(BnwCapitalCityStrategy)const;
 private:
 struct Impl;std::shared_ptr<Impl> impl_;
 explicit BnwCityCurrentCapitalStrategy(std::shared_ptr<Impl>)noexcept;
 void require_current()const;
 friend BnwCityCurrentCapitalStrategy inspect_bnw_city_current_capital_strategy(
  std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int64_t,std::int64_t);
};
BnwCityCurrentCapitalStrategy inspect_bnw_city_current_capital_strategy(
 std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int64_t owner,std::int64_t city);
}
