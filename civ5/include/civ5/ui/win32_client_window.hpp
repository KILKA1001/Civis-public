#pragma once
#include "civ5/ui/client_window_input.hpp"
#include "civ5/ui/bnw_audio_window_activation_loan.hpp"
#include "civ5/ui/bnw_tech_award_window_pointer_loan.hpp"
#include "civ5/ui/bnw_worldview_window_input_loan.hpp"
#include "civ5/ui/bnw_worldview_window_mouse_cache_loan.hpp"
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <functional>

namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;}
namespace civ5::ui {
namespace detail {class CivisFrontendPartyRoutingOwner;class CivisFrontendPartyRendererBackend;}
enum class ClientWindowEventKind { pointer, key_down, key_up, character,
                                   resize, dpi_changed, cancelled, close, activation, native_input };
struct ClientWindowEvent {
    ClientWindowEventKind kind;
    ClientInputOwner owner = ClientInputOwner::none;
    ClientPointerKind pointer_kind = ClientPointerKind::move;
    ClientPointerRoute pointer{};
    std::int32_t wheel_delta = 0;
    std::uint32_t native_flags = 0, key = 0;
    std::shared_ptr<const BnwAudioWindowActivationLoan> activation;
    std::shared_ptr<const BnwTechAwardWindowPointerLoan> pointer_move{};
    std::shared_ptr<const BnwWorldViewWindowInputLoan> worldview_input{};
};

// Borrows the renderer's HWND and preserves its procedure and GWLP_USERDATA.
// Attach on the window's UI thread; detach before destroying that window.
// The sink processes production input or queues it for the next frame.
class Win32ClientWindowBinding {
    struct Impl;
    friend class detail::CivisFrontendPartyRoutingOwner;
    friend class Win32ViewportTransaction;
  public:
    class ExistingInputLoan final {
        friend class Win32ClientWindowBinding;
        const DWORD creator_;
        const std::weak_ptr<Impl> operation_;
        std::shared_ptr<Impl> retain_current()const;
        friend class detail::CivisFrontendPartyRoutingOwner;
        friend class detail::CivisFrontendPartyRendererBackend;
        void claim_sink_event_owned(const ClientWindowEvent&)const;
        void require_retirement_idle_owned(const ClientWindowInput&)const;
        explicit ExistingInputLoan(std::shared_ptr<Impl>);
    public:
        void require_current()const;
        void require_idle()const;
        void require_same_input(const ClientWindowInput&)const;
        HWND window()const;
        const Win32ClientWindowBinding* binding()const;
    };

    using Sink = std::function<void(const ClientWindowEvent &)>;
    Win32ClientWindowBinding(HWND window, ClientWindowInput &input, Sink sink);
    ~Win32ClientWindowBinding();
    Win32ClientWindowBinding(const Win32ClientWindowBinding &) = delete;
    Win32ClientWindowBinding &operator=(const Win32ClientWindowBinding &) = delete;
    void set_framebuffer_size(std::int32_t width, std::int32_t height);
    bool failed() const noexcept;

  private:
    friend class ::civ5::runtime::BnwWorldViewSourceOperationLoan;
    static std::shared_ptr<const BnwWorldViewWindowMouseCacheLoan> borrow_mouse_cache_owned(
        const graphics::BnwOwnedCameraRendererWindowLease::Loan&);
    static LRESULT CALLBACK procedure(HWND, UINT, WPARAM, LPARAM);
    std::shared_ptr<const ExistingInputLoan> retain_existing_input_owned()const;
    const std::shared_ptr<Impl> impl_;
};
} // namespace civ5::ui
#endif
