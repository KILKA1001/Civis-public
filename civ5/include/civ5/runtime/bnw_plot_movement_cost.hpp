#pragma once
#include "civ5/runtime/bnw_unit_order_owners.hpp"
#include "civ5/runtime/bnw_native_unit_allocator_core.hpp"
#include <map>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
namespace civ5::ui {class SelectedBnwPreviewHost;}
namespace civ5::runtime {
// Closed path-node adapter. Only Core can issue its authentic dispatch or
// invoke this consumer; ordinary detached arithmetic remains separate.
class BnwNativeUnitPathMovementCost final {
 friend class BnwNativeUnitAllocatorCore;
 static std::int32_t evaluate(const BnwNativeUnitAllocatorCore&,const BnwNativeUnitAllocatorCore::ExistingPathMovementCostDispatch&);
};
class BnwPlotMovementCostUnavailable final:public std::runtime_error {
 public:using std::runtime_error::runtime_error;
};
// Explicit detached arithmetic only. Missing reached operands refuse; this
// map cannot issue a query, movement command, phase or world owner.
std::int32_t bnw_plot_movement_cost_math(const std::map<std::string,std::int64_t,std::less<>>&);
class BnwPlotMovementCostQuery final {
 public:std::int32_t value()const;
 private:
  struct Impl;std::shared_ptr<Impl> impl_;
  explicit BnwPlotMovementCostQuery(std::shared_ptr<Impl>)noexcept;
  void require_current()const;
  friend BnwPlotMovementCostQuery bind_bnw_plot_movement_cost(
   std::shared_ptr<const ui::SelectedBnwPreviewHost>,BnwUnitOrderKey,std::uint64_t);
};
BnwPlotMovementCostQuery bind_bnw_plot_movement_cost(
 std::shared_ptr<const ui::SelectedBnwPreviewHost>,BnwUnitOrderKey,std::uint64_t target);
// Genuine current read; no move admission, command, path or renderer grant.
std::int32_t inspect_bnw_plot_movement_cost(
 std::shared_ptr<const ui::SelectedBnwPreviewHost>,BnwUnitOrderKey,std::uint64_t target);
}
