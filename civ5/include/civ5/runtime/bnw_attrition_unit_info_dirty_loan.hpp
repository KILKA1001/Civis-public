#pragma once
#include <cstdint>
#include <memory>
namespace civ5::ui {struct BnwClientUnitDirtyEvent;class BnwClientSelectionControl;class BnwEngineUiTurnOwner;class SelectedBnwPreviewHost;}
namespace civ5::runtime {
class BnwAttritionUnitSelectedRead;
class BnwAttritionSelectedUnitInfoDirtyLoan;
class BnwAttritionPlotListDirtyLoan;
class BnwNativeUnitAllocatorCore;class BnwNativeAttritionPrefix;class BnwStockLuaAttemptLease;class BnwRetainedTurnCursor;class BnwSelectedStockLuaSourceOwner;struct LiveSessionState;
// Genuine Core-only child. No caller-supplied flags, IDs or callbacks issue it.
class BnwAttritionUnitInfoDirtyLoan final {
 public:
 ~BnwAttritionUnitInfoDirtyLoan();
 BnwAttritionUnitInfoDirtyLoan(const BnwAttritionUnitInfoDirtyLoan&)=delete;
 BnwAttritionUnitInfoDirtyLoan&operator=(const BnwAttritionUnitInfoDirtyLoan&)=delete;
 private:
 friend class BnwNativeAttritionPrefix;
 friend class BnwAttritionUnitSelectedRead;
 friend class BnwAttritionSelectedUnitInfoDirtyLoan;
 friend class BnwAttritionPlotListDirtyLoan;
 friend class BnwRetainedTurnCursor;friend class BnwSelectedStockLuaSourceOwner;
 friend class ui::BnwEngineUiTurnOwner;
 friend class ui::BnwClientSelectionControl;
 void require_core_without_callbacks(const BnwNativeUnitAllocatorCore&)const;
 void require_lease_without_callbacks(const BnwStockLuaAttemptLease&)const;
 void require_common_without_callbacks()const;
 struct Impl;std::unique_ptr<Impl> impl_;
 explicit BnwAttritionUnitInfoDirtyLoan(std::unique_ptr<Impl>);
 struct Operands {
  std::uint64_t generation,world_revision,source_serial,client_before,ordinal_before;
  // Producer lineage, NOT arguments of SerialEventUnitInfoDirty (zero args). No move Pending is synthesized.
  std::int32_t unit_owner,unit_full_id;
 };
 void require_alive_without_callbacks()const;
 // Sticky unexpected-busy/duplicate entry; same original localized weak
 // Engine/control block and the same Selection owner are mandatory.
 void enter_engine_once_without_callbacks(const ui::BnwEngineUiTurnOwner&,
  const BnwStockLuaAttemptLease&)const;
 void require_engine_without_callbacks(const ui::BnwEngineUiTurnOwner&,
  const BnwStockLuaAttemptLease&)const;
 void require_setter_source(const ui::BnwClientSelectionControl&,
  const ui::SelectedBnwPreviewHost&,const LiveSessionState&,
  const BnwStockLuaAttemptLease&)const;
 Operands operands_without_callbacks()const;
 ui::BnwClientUnitDirtyEvent event_without_callbacks()const;
 // Final pure seal checks raw city/unit receiver spans, after-HP, registered
 // strength/archive node, canonical allocations, exact serial and client state.
 void require_final_setter_without_callbacks(const ui::BnwClientSelectionControl&,
  const ui::SelectedBnwPreviewHost&,const LiveSessionState&,
  const BnwStockLuaAttemptLease&,std::uint64_t,std::uint64_t)const;
 // Preallocated, callbackless, noexcept typed return + Source/Cursor adoption.
 // The genuine Cursor/Source return is preallocated before entering Engine.
 void record_setter_and_adopt_without_callbacks(const ui::BnwClientSelectionControl&,
  std::uint64_t,std::uint64_t)const noexcept;
 void require_return_without_callbacks(const ui::BnwClientSelectionControl&,
  const ui::SelectedBnwPreviewHost&,const LiveSessionState&,
  std::uint64_t,std::uint64_t)const;
 void fail_without_callbacks()const noexcept;
};
}
