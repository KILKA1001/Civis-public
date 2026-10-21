#include "civ5/ui/client_window_input.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace civ5::ui {
namespace {
std::optional<std::int32_t> scale(std::int32_t value, std::int64_t numerator,
                                std::int64_t denominator) {
    if (denominator <= 0 || numerator <= 0) return std::nullopt;
    const auto product = std::int64_t(value) * numerator;
    // Floor preserves the outside-window status of a negative pointer.
    auto result = product / denominator;
    if (product < 0 && product % denominator) --result;
    if (result < std::numeric_limits<std::int32_t>::min() ||
        result > std::numeric_limits<std::int32_t>::max()) return std::nullopt;
    return static_cast<std::int32_t>(result);
}
} // namespace

bool ClientViewport::drawable() const noexcept {
    return client_width > 0 && client_height > 0 && framebuffer_width > 0 &&
           framebuffer_height > 0 && dpi > 0;
}
std::optional<ClientPoint> ClientViewport::framebuffer_point(ClientPoint point) const {
    if (!drawable()) return std::nullopt;
    const auto x = scale(point.x, framebuffer_width, client_width);
    const auto y = scale(point.y, framebuffer_height, client_height);
    if (!x || !y) return std::nullopt;
    return ClientPoint{*x, *y};
}
std::optional<ClientPoint> ClientViewport::dip_point(ClientPoint point) const {
    const auto framebuffer = framebuffer_point(point);
    if (!framebuffer) return std::nullopt;
    const auto x = scale(framebuffer->x, 96, dpi);
    const auto y = scale(framebuffer->y, 96, dpi);
    if (!x || !y) return std::nullopt;
    return ClientPoint{*x, *y};
}

bool ClientWindowInput::set_viewport(ClientViewport viewport) {
    if (viewport.client_width < 0 || viewport.client_height < 0 ||
        viewport.framebuffer_width < 0 || viewport.framebuffer_height < 0 || !viewport.dpi)
        throw std::invalid_argument("Invalid client viewport");
    if (viewport_ == viewport) return false;
    const bool cancelled = cancel();
    viewport_ = viewport;
    // Measurements belong to the previous framebuffer. Even without a modal
    // they must never hit-test the resized image until presentation replaces them.
    hits_.clear();
    measured_ = false;
    return cancelled;
}
bool ClientWindowInput::set_ui(std::span<const ClientRect> hits,
                              bool focused_modal, bool text_focus) {
    std::vector<ClientRect> replacement(hits.begin(), hits.end());
    const bool cancelled = focused_modal && !modal_ && gesture_ == ClientInputOwner::camera;
    if (cancelled) {
        gesture_ = ClientInputOwner::none;
        cancel_camera_ = true;
    }
    hits_ = std::move(replacement);
    measured_ = true;
    modal_ = focused_modal;
    text_focus_ = text_focus;
    return cancelled;
}
ClientInputOwner ClientWindowInput::hit(ClientPoint p) const noexcept {
    if (modal_) return ClientInputOwner::ui;
    if (!measured_) return ClientInputOwner::none;
    if (p.x < 0 || p.y < 0 || p.x >= viewport_.framebuffer_width ||
        p.y >= viewport_.framebuffer_height) return ClientInputOwner::none;
    for (const auto &r : hits_)
        if (r.width > 0 && r.height > 0 && p.x >= r.x && p.y >= r.y &&
            std::int64_t(p.x) < std::int64_t(r.x) + r.width &&
            std::int64_t(p.y) < std::int64_t(r.y) + r.height)
            return ClientInputOwner::ui;
    return ClientInputOwner::camera;
}
ClientPointerRoute ClientWindowInput::pointer(ClientPointerKind kind, ClientPoint client) {
    ClientPointerRoute result;
    result.cancel_camera = std::exchange(cancel_camera_, false);
    result.release = result.cancel_camera;
    result.framebuffer = viewport_.framebuffer_point(client);
    if (!result.framebuffer) {
        result.release |= gesture_ != ClientInputOwner::none;
        result.cancel_camera |= cancel();
        return result;
    }
    const auto target = hit(*result.framebuffer);
    if (kind == ClientPointerKind::left_down) {
        if (gesture_ != ClientInputOwner::none) {
            result.owner = gesture_;
            return result; // Repeated press is not a fresh gesture.
        }
        gesture_ = target;
        result.owner = target;
        result.capture = target != ClientInputOwner::none;
    } else if (kind == ClientPointerKind::left_up) {
        result.owner = gesture_;
        result.release |= gesture_ != ClientInputOwner::none;
        gesture_ = ClientInputOwner::none;
    } else {
        result.owner = modal_ ? ClientInputOwner::ui :
            gesture_ != ClientInputOwner::none ? gesture_ : target;
    }
    return result;
}
ClientInputOwner ClientWindowInput::keyboard_owner() const noexcept {
    if (!viewport_.drawable() || !measured_) return ClientInputOwner::none;
    return modal_ || text_focus_ ? ClientInputOwner::ui : ClientInputOwner::camera;
}
bool ClientWindowInput::cancel() noexcept {
    const bool camera = gesture_ == ClientInputOwner::camera;
    gesture_ = ClientInputOwner::none;
    text_focus_ = false;
    cancel_camera_ |= camera;
    return camera;
}
} // namespace civ5::ui
