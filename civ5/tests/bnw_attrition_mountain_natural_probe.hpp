#pragma once
#include "civ5/runtime/bnw_retained_turn_cursor.hpp"
#include <memory>
namespace civ5::ui {class SelectedBnwPreviewHost;}
namespace civ5::runtime {class BnwSelectedStockLuaSourceOwner;class BnwNativeUnitAllocatorCore;}
namespace civ5::tests {
runtime::BnwRetainedTurnView probe_bnw_attrition_mountain_natural(
 const std::shared_ptr<const ui::SelectedBnwPreviewHost>&,
 const std::shared_ptr<runtime::BnwSelectedStockLuaSourceOwner>&,
 const std::shared_ptr<runtime::BnwRetainedTurnCursor>&,
 const std::weak_ptr<const runtime::BnwNativeUnitAllocatorCore>&);
}
