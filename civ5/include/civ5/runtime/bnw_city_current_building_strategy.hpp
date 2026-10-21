#pragma once
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
namespace civ5::ui {class SelectedBnwPreviewHost;}
namespace civ5::runtime {
enum class BnwBuildingCityStrategy {original_capital,medium_high_difficulty,training_facility,first_culture,first_culture_emergency,first_science,first_gold,first_faith,first_production};
class BnwCityBuildingStrategyUnavailable final:public std::runtime_error {public:using std::runtime_error::runtime_error;};
// Optional detached math facts are never an import/strategy authority.
struct BnwBuildingCityStrategyMathInputs {
 std::optional<std::int64_t> population,culture_buildings,capital,science_per_pop,gold_modifier,production_buildings,production_modifier,faith_buildings,religion_flavor,pantheon,land_experience,game_handicap,city_x,city_y,original_capital_x,original_capital_y;
};
bool bnw_city_building_strategy_math(BnwBuildingCityStrategy,const BnwBuildingCityStrategyMathInputs&);
class BnwCityCurrentBuildingStrategy final {
 public:bool value(BnwBuildingCityStrategy)const;
 private:struct Impl;std::shared_ptr<Impl> impl_;
 explicit BnwCityCurrentBuildingStrategy(std::shared_ptr<Impl>)noexcept;
 void require_current()const;
 friend BnwCityCurrentBuildingStrategy inspect_bnw_city_current_building_strategy(std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int64_t,std::int64_t);
};
BnwCityCurrentBuildingStrategy inspect_bnw_city_current_building_strategy(std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int64_t owner,std::int64_t city);
}
