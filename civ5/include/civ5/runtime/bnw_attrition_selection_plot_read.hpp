#pragma once
#include <cstdint>
#include <memory>
#include <optional>
namespace civ5::ui {class BnwClientSelectionControl;class BnwEngineUiTurnOwner;class SelectedBnwPreviewHost;}
namespace civ5::runtime {
class BnwAttritionPlotListDirtyLoan;
class BnwNativeUnitAllocatorCore;class BnwNativeAttritionPrefix;class BnwStockLuaAttemptLease;class BnwRetainedTurnCursor;class BnwSelectedStockLuaSourceOwner;struct LiveSessionState;
// Actual getSelectionPlot nullable input -> GetPlotIndex -> native unit plot comparison.
// No caller input, save selection guess, public factory or supplied result.
class BnwAttritionSelectionPlotRead final {
 public:
 ~BnwAttritionSelectionPlotRead();
 BnwAttritionSelectionPlotRead(const BnwAttritionSelectionPlotRead&)=delete;
 BnwAttritionSelectionPlotRead&operator=(const BnwAttritionSelectionPlotRead&)=delete;
 private:
 friend class BnwNativeAttritionPrefix;friend class BnwRetainedTurnCursor;friend class BnwSelectedStockLuaSourceOwner;
 friend class BnwAttritionPlotListDirtyLoan;
 friend class ui::BnwEngineUiTurnOwner;friend class ui::BnwClientSelectionControl;
 struct Impl;std::unique_ptr<Impl>impl_;
 explicit BnwAttritionSelectionPlotRead(std::unique_ptr<Impl>);
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
 std::int32_t require_final_query_without_callbacks(const ui::BnwClientSelectionControl&,const ui::SelectedBnwPreviewHost&,const LiveSessionState&,const BnwStockLuaAttemptLease&,std::optional<std::int32_t>)const;
 void record_query_and_adopt_without_callbacks(const ui::BnwClientSelectionControl&,std::optional<std::int32_t>,std::int32_t)const noexcept;
 void require_return_without_callbacks(const ui::BnwClientSelectionControl&,const ui::SelectedBnwPreviewHost&,const LiveSessionState&,std::optional<std::int32_t>)const;
 std::optional<std::int32_t> result_without_callbacks()const;
 bool plot_list_required_without_callbacks()const;
};
}
