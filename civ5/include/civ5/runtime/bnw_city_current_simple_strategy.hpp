#pragma once
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
namespace civ5::ui {class SelectedBnwPreviewHost;}
namespace civ5::runtime {
enum class BnwSimpleCityStrategy {tiny,small,medium,large,is_puppet};
class BnwCitySimpleStrategyUnavailable final:public std::runtime_error {
 public:using std::runtime_error::runtime_error;
};
// Supplied facts are pure comparison math, never a current-source owner or
// strategy eligibility/activation/phase-completion capability.
struct BnwSimpleCityStrategyMathInputs {
 std::optional<std::int64_t> population,small_threshold,medium_threshold,large_threshold;
 std::optional<std::int64_t> capital,puppet;
};
bool bnw_city_simple_strategy_math(BnwSimpleCityStrategy,const BnwSimpleCityStrategyMathInputs&);
class BnwCityCurrentSimpleStrategy final {
 public:
  bool value(BnwSimpleCityStrategy)const;
 private:
  struct Impl;std::shared_ptr<Impl> impl_;
  explicit BnwCityCurrentSimpleStrategy(std::shared_ptr<Impl>)noexcept;
  void require_current()const;
  friend BnwCityCurrentSimpleStrategy inspect_bnw_city_current_simple_strategy(
   std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int64_t,std::int64_t);
};
// SAME actual selected host/session/immutable DB; no supplied operand DTO,
// rules hash, currentness callback, strategy result or completion receipt.
BnwCityCurrentSimpleStrategy inspect_bnw_city_current_simple_strategy(
 std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int64_t owner,std::int64_t city);
}
