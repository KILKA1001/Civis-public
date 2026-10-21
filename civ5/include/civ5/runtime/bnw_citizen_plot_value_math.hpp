#pragma once
#include <array>
#include <cstdint>
namespace civ5::runtime::detail {
// Pure supplied operands only. Actual caller must execute prefix BEFORE FOOD
// rate/consumption calls, then source AvoidGrowth/Focus/population and suffix.
std::int32_t bnw_citizen_plot_value_term(std::int32_t,std::int16_t);
std::array<std::int32_t,6> bnw_citizen_plot_value_prefix(
 const std::array<std::int32_t,6>& defines,const std::array<std::int16_t,6>& cached);
std::array<std::int32_t,6> bnw_citizen_plot_value_focus_food(std::array<std::int32_t,6>,std::int32_t excess,bool avoid,std::int32_t focus);
// Pure SDK bUseAllowGrowthFlag and sentinel comparison, never issuing state.
std::array<std::int32_t,6> bnw_citizen_plot_value_focus_food(std::array<std::int32_t,6>,std::int32_t excess,bool avoid,std::int32_t focus,bool allow_growth);
bool bnw_citizen_plot_value_choose(std::int32_t best,std::int32_t candidate,bool want_best)noexcept;
std::int32_t bnw_citizen_plot_value_total(std::array<std::int32_t,6>,bool avoid,std::int32_t focus,std::int32_t population);
std::int32_t bnw_citizen_plot_value_suffix(std::array<std::int32_t,6> values,
 std::int32_t excess_food_times100,bool avoid_growth,std::int32_t focus,std::int32_t population);
}
