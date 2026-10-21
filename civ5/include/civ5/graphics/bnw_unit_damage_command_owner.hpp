#pragma once
#include <array>
#include <functional>
#include "civ5/runtime/bnw_unitflag_health_fifo_loan.hpp"
#include <cstdint>
#include <memory>
#include <thread>
namespace civ5::ui {class BnwClientSceneFrame;class BnwOwnedCameraGuiScreen;class SelectedBnwPreviewHost;}
namespace civ5::runtime {class BnwOwnedGameplayEventPumpLoan;class BnwStockLuaAttemptLease;class BnwSelectedStockLuaBootstrap;class BnwUnitFlagLivingHealthRead;class BnwAttritionDamageDllWrapperPrefix;class BnwNativeUnitAllocatorCore;class BnwSelectedStockLuaSourceOwner;struct LiveSessionState;}
namespace civ5::graphics {
// Actual Civis owned command queue. No public issuer, drain, actor/model or frame ACK.
class BnwUnitDamageCommandOwner final {
 public:~BnwUnitDamageCommandOwner();
 BnwUnitDamageCommandOwner(const BnwUnitDamageCommandOwner&)=delete;
 BnwUnitDamageCommandOwner&operator=(const BnwUnitDamageCommandOwner&)=delete;
 private:
 friend class runtime::BnwAttritionDamageDllWrapperPrefix;
 friend class runtime::BnwUnitFlagHealthFifoLoan;
 friend class runtime::BnwSelectedStockLuaBootstrap;
 struct Binding {
  std::weak_ptr<ui::BnwClientSceneFrame>scene;std::weak_ptr<const ui::BnwOwnedCameraGuiScreen>gui;
  std::weak_ptr<const ui::SelectedBnwPreviewHost>host;std::weak_ptr<const runtime::LiveSessionState>world;
  std::weak_ptr<const runtime::BnwNativeUnitAllocatorCore>core;
  std::weak_ptr<const runtime::BnwSelectedStockLuaSourceOwner>source;std::weak_ptr<const void>control;
  std::uint64_t generation=0;
 };
 struct Payload {
  std::int32_t owner=-1,id=-1,new_damage=0,previous_damage=0,max_hit_points=0;
  // Raw native producer header tags; not a claim of complete original wire/context layout.
  std::uint32_t header_first=0x80,header_second=0x48;
 };
 struct Receipt {
  std::weak_ptr<const BnwUnitDamageCommandOwner>owner;
  std::weak_ptr<const runtime::BnwAttritionDamageDllWrapperPrefix>issuer;
  Payload payload;const void*native_unit=nullptr;const void*wrapper=nullptr;const void*node=nullptr;
  std::shared_ptr<const runtime::BnwUnitFlagLivingHealthRead>health_source;
  std::uint64_t operation=0,revision=0,generation=0,occurrence=0;
  std::uint32_t sequence=0;bool appended=false,lock_released=false;mutable bool source_adopted=false;
 };
 struct Impl;struct HealthStorage;std::shared_ptr<Impl>impl_;const Binding binding_;
 const std::thread::id creator_=std::this_thread::get_id();std::weak_ptr<BnwUnitDamageCommandOwner>self_;
 explicit BnwUnitDamageCommandOwner(Binding);
 static std::shared_ptr<BnwUnitDamageCommandOwner>bind_owned(const Binding&);
 void require_origin_without_callbacks()const;
 static void require_health_fifo_without_callbacks(const std::shared_ptr<runtime::BnwUnitFlagHealthFifoLoan::State>&);
 static void require_health_fifo_current(const std::shared_ptr<runtime::BnwUnitFlagHealthFifoLoan::State>&);
 static void fail_health_fifo_without_callbacks(const std::shared_ptr<runtime::BnwUnitFlagHealthFifoLoan::State>&)noexcept;
 static std::shared_ptr<const runtime::BnwUnitFlagLivingHealthRead>health_source_from_fifo_without_callbacks(const std::shared_ptr<runtime::BnwUnitFlagHealthFifoLoan::State>&);
 static std::pair<std::int32_t,std::int32_t>health_full_id_from_fifo_without_callbacks(const std::shared_ptr<runtime::BnwUnitFlagHealthFifoLoan::State>&);
 static std::pair<std::int32_t,std::int32_t>health_damage_pair_from_fifo_without_callbacks(const std::shared_ptr<runtime::BnwUnitFlagHealthFifoLoan::State>&);
 bool has_pending_health_owned()const;
 void with_health_fifo_owned(const runtime::BnwOwnedGameplayEventPumpLoan&,const runtime::BnwStockLuaAttemptLease&,
  const runtime::BnwSelectedStockLuaSourceOwner&,const runtime::BnwSelectedStockLuaBootstrap&,
  const std::function<runtime::BnwUnitFlagHealthFifoLoan::DeliveryReturn(const runtime::BnwUnitFlagHealthFifoLoan&)>&);
 std::shared_ptr<const Receipt>append_owned(const std::shared_ptr<const runtime::BnwAttritionDamageDllWrapperPrefix>&,
  const Payload&,const void*,const void*,std::uint64_t,std::uint64_t);
 void require_return_without_callbacks(const Receipt&)const;
 void record_source_adoption_without_callbacks(const Receipt&,const runtime::BnwAttritionDamageDllWrapperPrefix&)const noexcept;
};
}
