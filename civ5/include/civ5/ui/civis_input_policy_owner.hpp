#pragma once
#include "civ5/assets/civis_resource_bank_owner.hpp"
#include "civ5/ui/client_window_input.hpp"
#include <memory>
#include <optional>
#include <string>
#include <vector>
namespace civ5::graphics {class BnwOwnedCameraRendererWindowLease;}
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;class BnwSelectedStockLuaSourceOwner;
class BnwSelectedStockLuaBootstrap;class LuaScriptHost;struct LiveSessionState;}
namespace civ5::ui {
class SelectedBnwPreviewHost;
// Copied facts, never an issuer. Only NativeLoan's closed helper can supply
// these facts to the private selected-session controller initializer.
struct CivisInputWindowOrigin {
 std::weak_ptr<const graphics::BnwOwnedCameraRendererWindowLease> window;
 std::uintptr_t hwnd=0,occurrence=0;
 std::uint32_t process=0,ui_thread=0;
 std::uint64_t viewport_revision=0;
 ClientViewport viewport;
};
struct CivisInputDeviceFacts {
 std::int32_t mouse_present=0,mouse_buttons=0,digitizer_flags=0,keyboard_type=0;
 // Diagnostics only. SM_DIGITIZER never issues Original touch-enabled state.
};
struct CivisInputPolicySource {
 std::string requested_key;
 assets::AssetCandidate candidate;
 runtime::FileIdentity payload_identity;
};
struct CivisInterfaceModeChange {
 std::int32_t previous=0,current=0;
 std::uint64_t revision=0;
 bool changed=false; // Actual owned store fact, never event delivery/ACK.
};
class CivisInputPolicyOwner final {
 public:
 ~CivisInputPolicyOwner();
 CivisInputPolicyOwner(const CivisInputPolicyOwner&)=delete;
 CivisInputPolicyOwner&operator=(const CivisInputPolicyOwner&)=delete;
 void retire()noexcept;
 void require_for_source(const runtime::BnwWorldViewSourceOperationLoan&)const;
 bool is_touch_enabled(const runtime::BnwWorldViewSourceOperationLoan&)const;
 std::int32_t interface_mode(const runtime::BnwWorldViewSourceOperationLoan&)const;
 CivisInputDeviceFacts device_facts(const runtime::BnwWorldViewSourceOperationLoan&)const;
 std::vector<CivisInputPolicySource> source_records(const runtime::BnwWorldViewSourceOperationLoan&)const;
 private:
 friend class runtime::BnwSelectedStockLuaSourceOwner;
 friend class runtime::BnwSelectedStockLuaBootstrap;
 friend class runtime::LuaScriptHost;
 // Explicit own client controller initialization, not a missing-state false
 // or SELECTION fallback. Actual binding dispatches mouse/key, not touch.
 static std::shared_ptr<CivisInputPolicyOwner> initialize_mouse_keyboard(
  const std::weak_ptr<const SelectedBnwPreviewHost>&,
  const std::weak_ptr<assets::CivisResourceBankOwner>&,
  const runtime::BnwWorldViewSourceOperationLoan&);
 CivisInterfaceModeChange set_interface_mode_owned(
  const runtime::BnwWorldViewSourceOperationLoan&,std::int32_t);
 struct Impl;const std::shared_ptr<Impl>impl_;
 explicit CivisInputPolicyOwner(std::shared_ptr<Impl>);
};
}
