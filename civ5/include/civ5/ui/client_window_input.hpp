#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace civ5::ui {

struct ClientPoint {
    std::int32_t x = 0, y = 0;
    bool operator==(const ClientPoint &) const = default;
};
struct ClientRect {
    std::int32_t x = 0, y = 0, width = 0, height = 0;
};

// The native client dimensions are already in the process's coordinate space.
// The framebuffer ratio is the only pointer conversion; applying DPI again
// would double-scale physical Win32 messages in a per-monitor aware process.
struct ClientViewport {
    std::int32_t client_width = 0, client_height = 0;
    std::int32_t framebuffer_width = 0, framebuffer_height = 0;
    std::uint32_t dpi = 96;
    bool drawable() const noexcept;
    std::optional<ClientPoint> framebuffer_point(ClientPoint point) const;
    std::optional<ClientPoint> dip_point(ClientPoint point) const;
    bool operator==(const ClientViewport &) const = default;
};

enum class ClientInputOwner { none, camera, ui };
enum class ClientPointerKind { move, left_down, left_up, wheel };
struct ClientPointerRoute {
    ClientInputOwner owner = ClientInputOwner::none;
    std::optional<ClientPoint> framebuffer;
    bool capture = false, release = false, cancel_camera = false;
};

// This owner runs on the native UI thread. A gesture retains its press owner
// across controls. Opening a modal, resizing, or losing focus cancels a camera
// gesture and requires a fresh press before dragging can resume.
class ClientWindowInput {
  public:
    bool set_viewport(ClientViewport viewport);
    bool set_ui(std::span<const ClientRect> measured_framebuffer_hits,
                bool focused_modal, bool text_focus = false);
    ClientPointerRoute pointer(ClientPointerKind kind, ClientPoint client_point);
    ClientInputOwner keyboard_owner() const noexcept;
    bool cancel() noexcept;
    const ClientViewport &viewport() const noexcept { return viewport_; }
    bool modal() const noexcept { return modal_; }
    ClientInputOwner gesture_owner() const noexcept { return gesture_; }

  private:
    ClientInputOwner hit(ClientPoint framebuffer) const noexcept;
    ClientViewport viewport_;
    std::vector<ClientRect> hits_;
    ClientInputOwner gesture_ = ClientInputOwner::none;
    bool modal_ = false, text_focus_ = false, cancel_camera_ = false;
    bool measured_ = false;
};

} // namespace civ5::ui
