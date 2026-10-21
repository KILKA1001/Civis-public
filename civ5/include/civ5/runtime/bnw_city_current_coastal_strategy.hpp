#pragma once
#include <cstdint>
#include <memory>
#include <stdexcept>
namespace civ5::saves {struct SavedSessionSnapshot;struct MapRegionRecord;}
namespace civ5::ui {class SelectedBnwPreviewHost;}
namespace civ5::runtime {
namespace detail {
// Pure native borrowed lookup; does not issue a current owner or receipt.
const saves::MapRegionRecord* bnw_coastal_landmass_lookup(
 const saves::SavedSessionSnapshot&,std::int64_t handle);
}
enum class BnwCoastalCityStrategy {coast,landlocked};
class BnwCityCoastalStrategyUnavailable final:public std::runtime_error {
 public:using std::runtime_error::runtime_error;
};
class BnwCityCurrentCoastalStrategy final {
 public:bool value(BnwCoastalCityStrategy)const;
 private:
  struct Impl;std::shared_ptr<Impl> impl_;
  explicit BnwCityCurrentCoastalStrategy(std::shared_ptr<Impl>)noexcept;
  void require_current()const;
  friend BnwCityCurrentCoastalStrategy inspect_bnw_city_current_coastal_strategy(
   std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int64_t,std::int64_t);
};
// Current SAME selected host/session/immutable DB, not caller water contexts,
// DTO counters, cache flags, area aliases or strategy completion receipts.
BnwCityCurrentCoastalStrategy inspect_bnw_city_current_coastal_strategy(
 std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int64_t owner,std::int64_t city);
}
