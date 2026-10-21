#pragma once
#include "civ5/saves/city_management.hpp"
#include "civ5/content/info_types.hpp"
#include <array>
#include <cstdint>
#include <memory>
#include <vector>
namespace civ5::ui {class SelectedBnwPreviewHost;}
namespace civ5::runtime {
struct LiveSessionState;class BnwCityEntryPending;
// Values only, not an apply/AI/epoch permission. Production category order is
// building, unit, project, process; vector position is the native Info slot.
struct BnwCityStrategyRuntimeValues {
 std::vector<std::uint8_t> using_strategy;
 std::vector<std::int32_t> adopted,latest;
 std::array<std::vector<std::int32_t>,4> weights;
 bool operator==(const BnwCityStrategyRuntimeValues&)const=default;
};
namespace detail {
// Replays the actual source Init/Reset then Read arithmetic. Caller-supplied
// shape/catalog alone never construct the private genuine runtime owner.
struct BnwCityStrategyRuntimeReadShape {
 std::uint32_t strategies=0,flavors=0;
 std::array<std::uint32_t,4> production{};
};
BnwCityStrategyRuntimeValues decode_bnw_city_strategy_runtime_read(
 const saves::CityManagementObservation&,const content::InfoTypeCatalog&,
 const BnwCityStrategyRuntimeReadShape&);
}
class BnwCityStrategyRuntime final {
 public: ~BnwCityStrategyRuntime();
 private:
 friend class BnwCityEntryPending;
 struct Impl;std::shared_ptr<const Impl> impl_;
 explicit BnwCityStrategyRuntime(std::shared_ptr<const Impl>)noexcept;
 static std::shared_ptr<const BnwCityStrategyRuntime> read_current(
  const ui::SelectedBnwPreviewHost&,const LiveSessionState&,
  const saves::CityManagementObservation&,std::int32_t,std::int32_t,
  std::uint64_t,std::uint64_t,std::uint64_t revision);
 void require_binding(const LiveSessionState&,std::int32_t,std::int32_t,
  std::uint64_t,std::uint64_t,std::uint64_t)const;
 BnwCityStrategyRuntimeValues values()const;
 std::uint64_t revision()const;
};
}
