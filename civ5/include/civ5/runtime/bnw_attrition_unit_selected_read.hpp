#pragma once
#include <cstdint>
#include <memory>
namespace civ5::ui {class BnwClientSelectionControl;class BnwEngineUiTurnOwner;class SelectedBnwPreviewHost;}
namespace civ5::runtime {
class BnwNativeUnitAllocatorCore;class BnwNativeAttritionPrefix;class BnwStockLuaAttemptLease;class BnwRetainedTurnCursor;class BnwSelectedStockLuaSourceOwner;struct LiveSessionState;
// Actual CvUnit::IsSelected -> DLLUI IsUnitSelected(owner, fullID) child.
// No caller input, save selection guess, public factory or supplied result.
class BnwAttritionUnitSelectedRead final {
 public:
 ~BnwAttritionUnitSelectedRead();
 BnwAttritionUnitSelectedRead(const BnwAttritionUnitSelectedRead&)=delete;
 BnwAttritionUnitSelectedRead&operator=(const BnwAttritionUnitSelectedRead&)=delete;
 private:
 friend class BnwNativeAttritionPrefix;friend class BnwRetainedTurnCursor;friend class BnwSelectedStockLuaSourceOwner;
 friend class ui::BnwEngineUiTurnOwner;friend class ui::BnwClientSelectionControl;
 struct Impl;std::unique_ptr<Impl>impl_;
 explicit BnwAttritionUnitSelectedRead(std::unique_ptr<Impl>);
 struct Operands {std::uint64_t generation,world_revision,source_serial;std::int32_t unit_owner,unit_full_id;};
 void require_alive_without_callbacks()const;
 void fail_without_callbacks()const noexcept;
 void require_common_without_callbacks()const;
 void require_core_without_callbacks(const BnwNativeUnitAllocatorCore&)const;
 void require_lease_without_callbacks(const BnwStockLuaAttemptLease&)const;
 void enter_engine_once_without_callbacks(const ui::BnwEngineUiTurnOwner&,const BnwStockLuaAttemptLease&)const;
 void require_engine_without_callbacks(const ui::BnwEngineUiTurnOwner&,const BnwStockLuaAttemptLease&)const;
 void require_query_source(const ui::BnwClientSelectionControl&,const ui::SelectedBnwPreviewHost&,const LiveSessionState&,const BnwStockLuaAttemptLease&)const;
 Operands operands_without_callbacks()const;
 void require_final_query_without_callbacks(const ui::BnwClientSelectionControl&,const ui::SelectedBnwPreviewHost&,const LiveSessionState&,const BnwStockLuaAttemptLease&,bool)const;
 void record_query_and_adopt_without_callbacks(const ui::BnwClientSelectionControl&,bool)const noexcept;
 void require_return_without_callbacks(const ui::BnwClientSelectionControl&,const ui::SelectedBnwPreviewHost&,const LiveSessionState&,bool)const;
 bool result_without_callbacks()const;
};
}
