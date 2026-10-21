#pragma once
#include "civ5/runtime/map_geometry.hpp"
#include <cstdint>
#include <array>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
namespace civ5::ui {class SelectedBnwPreviewHost;}
namespace civ5::saves {struct PregameOption;}
namespace civ5::runtime {
enum class BnwSupportCityStrategy {need_culture_building,need_tourism_building,good_airlift_city,key_science_city,good_gp_city,many_techs_stolen};
class BnwCitySupportStrategyUnavailable final:public std::runtime_error {public:using std::runtime_error::runtime_error;};
class BnwCityCurrentSupportStrategy final {
 public:bool value(BnwSupportCityStrategy)const;
 private:struct Impl;std::shared_ptr<Impl> impl_;
 explicit BnwCityCurrentSupportStrategy(std::shared_ptr<Impl>)noexcept;
 void require_current()const;
 friend BnwCityCurrentSupportStrategy inspect_bnw_city_current_support_strategy(std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int64_t,std::int64_t);
};
BnwCityCurrentSupportStrategy inspect_bnw_city_current_support_strategy(std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int64_t owner,std::int64_t city);
namespace detail {
// Detached numeric/reached-order witnesses only; cannot construct current owners.
// Native Read64/first-NUL/string-prefix first match only; absence means that
// a genuine Game/DB reader must reach the default query, never caller false.
std::optional<std::int32_t> bnw_culture_saved_no_espionage_option(std::span<const saves::PregameOption>);
bool bnw_need_tourism_four_terms_math(const std::array<std::optional<std::int64_t>,4>&);
struct BnwGpStrategyMathSlot {
 bool info_present=false;
 std::optional<std::int64_t> unit_class,rate,count,building_rate,city_modifier,player_modifier,class_kind,trait_scientist_modifier,class_modifier;
};
bool bnw_good_gp_strategy_math(std::span<const BnwGpStrategyMathSlot>);
struct BnwKeyScienceMathCity {bool is_self=false;std::optional<std::int64_t> science_times100;bool info_present=true;};
bool bnw_key_science_strategy_math(std::int64_t espionage_started,
 std::optional<std::int64_t> selected_science_times100,
 std::span<const BnwKeyScienceMathCity> native_city_slots,
 std::optional<std::int64_t> espionage_personality_flavor);
bool bnw_many_techs_stolen_math(std::int64_t game_turn,std::int64_t started,std::optional<std::int64_t> robbed);
bool bnw_good_airlift_strategy_math(const MapGeometry&,bool is_capital,
 std::optional<std::int64_t> city_area,std::optional<std::int64_t> capital_area,
 std::optional<std::int64_t> x,std::optional<std::int64_t> y,
 std::optional<std::int64_t> capital_x,std::optional<std::int64_t> capital_y);
}
}
