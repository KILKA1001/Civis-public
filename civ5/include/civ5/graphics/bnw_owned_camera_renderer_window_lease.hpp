#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <thread>
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;}
namespace civ5::ui {class Win32ViewportTransaction;class BnwOwnedCameraGuiScreen;class UiScreenGlRenderer;class BnwWorldViewWindowInputLoan;}
namespace civ5::ui::detail {class BnwOriginalNonFontDeviceInitializer;}
namespace civ5::graphics {
class GlContext;
namespace detail {class BnwRendererContextPin;}
class BnwUnitSetupGraphicsState;
// Initial physical renderer/window ownership only. No GPU/reset, native UI,
// loading, VM, audio or accepted frame completion is issued by this owner.
class BnwOwnedCameraRendererWindowLease final {
  public:
    class Loan final {
        friend class BnwOwnedCameraRendererWindowLease;
        friend class ::civ5::runtime::BnwWorldViewSourceOperationLoan;
        friend class ::civ5::ui::BnwOwnedCameraGuiScreen;
        friend class ::civ5::ui::BnwWorldViewWindowInputLoan;
        const BnwOwnedCameraRendererWindowLease& owner_;
        explicit Loan(const BnwOwnedCameraRendererWindowLease& owner):owner_(owner){}
        friend class ::civ5::ui::detail::BnwOriginalNonFontDeviceInitializer;
        friend class ::civ5::ui::UiScreenGlRenderer;
        std::shared_ptr<const detail::BnwRendererContextPin> retain_context_pin_owned()const;
        void require_context_gui_client_profile_owned()const;
      public:
        Loan(const Loan&)=delete;
        Loan& operator=(const Loan&)=delete;
        void require_current()const;
        // Only a genuine active loan may retain the producer-created control
        // block. Consumer never accepts caller shared-pointer alias ownership.
        std::shared_ptr<const BnwOwnedCameraRendererWindowLease> retain_owner()const;
        // Actual producer control comparison inside a genuine current loan.
        // Weak input does not add an uncounted facade pin across callbacks.
        void require_context_control_owned(const std::weak_ptr<GlContext>&)const;
        std::uintptr_t window()const;
        std::uintptr_t dc()const;
        std::uintptr_t wgl_context()const;
        std::uint32_t process()const;
        std::uint32_t ui_thread()const;
        std::uint64_t viewport_revision()const;
        std::uintptr_t window_occurrence()const;
    };
    ~BnwOwnedCameraRendererWindowLease();
    BnwOwnedCameraRendererWindowLease(const BnwOwnedCameraRendererWindowLease&)=delete;
    BnwOwnedCameraRendererWindowLease& operator=(const BnwOwnedCameraRendererWindowLease&)=delete;
    void require_current()const;
    // Stack Loan remains callable only during this actual guarded operation.
    void with_actual_window_loan(const std::function<void(const Loan&)>&)const;
  private:
    friend class BnwUnitSetupGraphicsState;
    friend class ::civ5::ui::Win32ViewportTransaction;
    friend class ::civ5::runtime::BnwWorldViewSourceOperationLoan;
    friend class ::civ5::ui::BnwOwnedCameraGuiScreen;
    friend class ::civ5::ui::BnwWorldViewWindowInputLoan;
    friend class ::civ5::ui::UiScreenGlRenderer;
    friend class ::civ5::ui::detail::BnwOriginalNonFontDeviceInitializer;
    // Only the actual renderer may mint this counted lender over its retained
    // canonical slot. No numeric outside-owner budget or public pin API.
    class RendererPin final {
        friend class ::civ5::ui::UiScreenGlRenderer;
        const std::shared_ptr<const BnwOwnedCameraRendererWindowLease>& owner_;
        explicit RendererPin(const std::shared_ptr<const BnwOwnedCameraRendererWindowLease>&);
      public:
        ~RendererPin();
        RendererPin(const RendererPin&)=delete;
        RendererPin& operator=(const RendererPin&)=delete;
        void require_current_without_native_callbacks()const;
    };
    static void require_retained_context_pin_origin_without_callbacks(
        const std::weak_ptr<const BnwOwnedCameraRendererWindowLease>&,
        const std::shared_ptr<const detail::BnwRendererContextPin>&);
    void require_origin_lifetime_without_native_callbacks()const;
    std::shared_ptr<const detail::BnwRendererContextPin> retain_context_pin_without_native_callbacks()const;
    struct Impl;
    class Pin;
    const std::shared_ptr<Impl> impl_;
    const std::thread::id thread_=std::this_thread::get_id();
    std::weak_ptr<const BnwOwnedCameraRendererWindowLease> self_;
    mutable std::size_t self_pins_=0;
    explicit BnwOwnedCameraRendererWindowLease(std::shared_ptr<Impl>);
    static std::shared_ptr<const BnwOwnedCameraRendererWindowLease> capture_from_actual_transaction(
        const std::shared_ptr<GlContext>&,const ::civ5::ui::Win32ViewportTransaction&);
    static void drain_retired_creator_storage()noexcept;
};
}
