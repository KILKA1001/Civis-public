#pragma once
#include <cstdint>
#include <memory>
namespace civ5::ui {class SelectedBnwPreviewHost;class BnwClientSceneFrame;class BnwOwnedCameraGuiScreen;}
namespace civ5::graphics {class BnwUnitDamageCommandOwner;}
namespace civ5::runtime {
class BnwNativeUnitAllocatorCore;class BnwNativeAttritionPrefix;class BnwSelectedStockLuaSourceOwner;class BnwStockLuaAttemptLease;class BnwRetainedTurnCursor;struct LiveSessionState;
// Actual bounded Civis constructor/release representation of CvDllUnit(this).
// No original ICvUnit ABI, general QueryInterface or graphics-call capability.
class BnwAttritionDamageDllWrapperPrefix final {
 public:
 ~BnwAttritionDamageDllWrapperPrefix();
 BnwAttritionDamageDllWrapperPrefix(const BnwAttritionDamageDllWrapperPrefix&)=delete;
 BnwAttritionDamageDllWrapperPrefix&operator=(const BnwAttritionDamageDllWrapperPrefix&)=delete;
 private:
 friend class BnwNativeAttritionPrefix;friend class BnwRetainedTurnCursor;friend class BnwSelectedStockLuaSourceOwner;
 friend class graphics::BnwUnitDamageCommandOwner;
 friend class BnwUnitFlagLivingHealthRead;
 std::shared_ptr<const class BnwUnitFlagLivingHealthRead>capture_unitflag_health_for_actual_append_without_callbacks()const;
 struct Impl;std::unique_ptr<Impl>impl_;
 explicit BnwAttritionDamageDllWrapperPrefix(std::unique_ptr<Impl>);
 void require_alive_without_callbacks()const;
 void require_common_without_callbacks()const;
 void require_lease_without_callbacks(const BnwStockLuaAttemptLease&)const;

 void execute_owned_command_until_popup(const BnwStockLuaAttemptLease&,const std::shared_ptr<ui::BnwClientSceneFrame>&,const std::shared_ptr<const ui::BnwOwnedCameraGuiScreen>&)const;
 void require_command_binding_without_callbacks(const graphics::BnwUnitDamageCommandOwner&)const;
 void require_command_payload_without_callbacks(std::int32_t,std::int32_t,std::int32_t,std::int32_t,std::int32_t,const void*,const void*,std::uint64_t,std::uint64_t)const;
 void require_call_return_without_callbacks()const;
 void require_core_without_callbacks(const BnwNativeUnitAllocatorCore&)const;
 void cleanup_wrapper_without_callbacks()const noexcept;
 void require_earned_cleanup_without_callbacks()const;
 void fail_without_callbacks()const noexcept;
};
}
