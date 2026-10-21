#pragma once
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
namespace civ5::ui {class SelectedBnwPreviewHost;}
namespace civ5::saves {struct PregameOption;}
namespace civ5::runtime {
class BnwCityCurrentGrowthThresholdUnavailable final:public std::runtime_error {
 public:using std::runtime_error::runtime_error;
};
namespace detail {
// Native CustomOption Read/string-first-match arithmetic only. Empty optional
// means no saved match; it grants no actual Game/DB/IsHuman authority.
std::optional<std::int32_t> bnw_growth_saved_pitboss_option(
 std::span<const saves::PregameOption>);
}
class BnwCityCurrentGrowthThreshold final {
 public:
  // Recompute current reached operands under genuine host/Game/rules guards.
  // This is a numeric read; it grants no growth/effect/cursor permission.
  std::int32_t value()const;
 private:
  struct Impl;std::shared_ptr<Impl> impl_;
  explicit BnwCityCurrentGrowthThreshold(std::shared_ptr<Impl>)noexcept;
  friend BnwCityCurrentGrowthThreshold inspect_bnw_city_current_growth_threshold(
   std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int64_t,std::int64_t);
  void require_current()const;
};
// Only a real selected host can join its actual session, DB and identity here.
// No caller predicates, source values, rules hash or currentness callback.
BnwCityCurrentGrowthThreshold inspect_bnw_city_current_growth_threshold(
 std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int64_t owner,std::int64_t city);
}
