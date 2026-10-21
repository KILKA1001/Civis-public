#pragma once
#include <cstdint>
#include <memory>
namespace civ5::ui {class BnwScrollAnimationUpdateLoan;}
namespace civ5::runtime {
class BnwSelectedStockLuaSourceOwner;
class BnwWorldViewSourceOperationLoan;
class BnwStockLuaAttemptLease;
class BnwLocalClientScalarOwner;
class BnwSpClientPregameStartup;
class BnwImportedPregameReadCache;
class BnwImportedPregameLocaleOwner;
class BnwPregameNativeBootEventQueue;
// Source-owner-issued client child of the actual selected renderer update.
// Scroll supplies platform elapsed time only. Canonical Bootstrap/control/VM/
// restored Game plus SAME local client provide all client update authority.
class BnwSpClientUpdateLoan final {
 friend class BnwSelectedStockLuaSourceOwner;
 friend class BnwLocalClientScalarOwner;
 friend class BnwSpClientPregameStartup;
 friend class BnwImportedPregameReadCache;
 friend class BnwImportedPregameLocaleOwner;
 friend class BnwPregameNativeBootEventQueue;
 struct Impl;
 const std::unique_ptr<Impl> impl_;
 BnwSpClientUpdateLoan(const BnwWorldViewSourceOperationLoan&,
  const ui::BnwScrollAnimationUpdateLoan&,const BnwStockLuaAttemptLease&,
  std::weak_ptr<BnwLocalClientScalarOwner>);
 ~BnwSpClientUpdateLoan();
 void require_current_without_callbacks()const;
 void require_imported_cache_owned(const BnwImportedPregameReadCache&)const;
 float delta_seconds_without_callbacks()const;
 std::uint64_t tick_sequence_without_callbacks()const;
 void execute_owned();
 void control_registration_callback_owned(std::int32_t)const;
 void control_proxy_optional_ui_owned()const;
 void client155_registration_callback_owned()const;
 BnwSpClientUpdateLoan(const BnwSpClientUpdateLoan&)=delete;
 BnwSpClientUpdateLoan& operator=(const BnwSpClientUpdateLoan&)=delete;
 BnwSpClientUpdateLoan(BnwSpClientUpdateLoan&&)=delete;
 BnwSpClientUpdateLoan& operator=(BnwSpClientUpdateLoan&&)=delete;
};
}
