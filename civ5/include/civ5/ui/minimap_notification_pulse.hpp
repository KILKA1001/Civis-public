#pragma once

#include "civ5/ui/minimap_hex_geometry.hpp"
#include "civ5/ui/minimap_visible_bounds.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <string_view>
#include <vector>

namespace civ5::ui {

// CvNotifications.cpp:804 passes lookup+1: marker zero belongs to the selected
// unit. CvPlayer.cpp:4864,4872 passes (-1,-1) for NOTIFICATION_TECH, so a
// successful callback need not create a visible dot.
enum class MinimapNotificationStatus {
    queued_visible,
    queued_outside_view,
    no_spatial_marker,
    unavailable,
    stale,
    invalid
};

struct MinimapNotificationResult {
    MinimapNotificationStatus status = MinimapNotificationStatus::unavailable;
    std::string_view reason;
    bool accepted() const noexcept {
        return status == MinimapNotificationStatus::queued_visible ||
               status == MinimapNotificationStatus::queued_outside_view ||
               status == MinimapNotificationStatus::no_spatial_marker;
    }
};

struct MinimapNotificationPulse {
    std::int32_t map_x = -1, map_y = -1;
    std::int64_t marker_id = -1, player_id = -1;
    std::int32_t turn = -1;
    std::uint64_t owner_sequence = 0;
    MinimapPoint2 world{}, texture_pixel{};
    bool inside_view = false;
    std::chrono::steady_clock::time_point started_at{}, expires_at{};
};

struct MinimapNotificationPulseContext {
    std::int32_t map_width = 0, map_height = 0;
    std::int64_t player_id = -1;
    std::int32_t turn = -1;
    MinimapWorldTransform transform;
    MinimapVisibleBounds visible;
    // The native flashing-dot timing is not in the public BNW SDK. Require an
    // explicit presentation policy rather than baking an unverified duration
    // into the map owner.
    std::chrono::milliseconds lifetime{0};
};

// The render target must be ready before an emitted spatial marker is accepted.
// The caller presents active() on each frame and retains this owner for the
// same player/turn. A new owner starts a fresh queue on player/turn change.
class MinimapNotificationPulseOwner {
  public:
    using Clock = std::function<std::chrono::steady_clock::time_point()>;
    using TargetReady = std::function<bool()>;

    explicit MinimapNotificationPulseOwner(MinimapNotificationPulseContext context,
                                           TargetReady target_ready = {},
                                           Clock clock = std::chrono::steady_clock::now)
        : context_(std::move(context)), target_ready_(std::move(target_ready)),
          clock_(std::move(clock)) {}

    MinimapNotificationResult emit_minimap_notification(
        std::int32_t map_x, std::int32_t map_y, std::int64_t lookup_plus_one,
        std::int64_t player_id, std::int32_t turn, std::uint64_t owner_sequence) {
        if (player_id != context_.player_id || turn != context_.turn ||
            owner_sequence == 0 || owner_sequence <= last_sequence_)
            return {MinimapNotificationStatus::stale,
                    "Minimap notification belongs to another owner or turn"};
        if (lookup_plus_one <= 0 || lookup_plus_one == std::numeric_limits<std::int64_t>::max())
            return {MinimapNotificationStatus::invalid,
                    "Minimap notification lookup is invalid"};
        if (map_x == -1 && map_y == -1) {
            last_sequence_ = owner_sequence;
            return {MinimapNotificationStatus::no_spatial_marker,
                    "BNW notification has no map coordinate"};
        }
        if (context_.map_width <= 0 || context_.map_height <= 0 ||
            context_.lifetime.count() <= 0 || context_.lifetime > std::chrono::seconds(60) ||
            !clock_ || !std::isfinite(context_.transform.scale_x) ||
            !std::isfinite(context_.transform.scale_y) ||
            context_.transform.scale_x == 0 || context_.transform.scale_y == 0 ||
            !std::isfinite(context_.transform.offset_x) ||
            !std::isfinite(context_.transform.offset_y) ||
            !std::all_of(context_.transform.cube_to_world.begin(),
                         context_.transform.cube_to_world.end(),
                         [](float value) { return std::isfinite(value); }))
            return {MinimapNotificationStatus::invalid,
                    "Minimap notification context or lookup is invalid"};
        if (map_x < 0 || map_y < 0 || map_x >= context_.map_width ||
            map_y >= context_.map_height)
            return {MinimapNotificationStatus::invalid,
                    "Minimap notification coordinate is outside the saved map"};
        if (!target_ready_ || !target_ready_())
            return {MinimapNotificationStatus::unavailable,
                    "Minimap notification render target is unavailable"};
        const auto &visible = context_.visible;
        if (visible.segment_count == 0 || visible.segment_count > 2 ||
            visible.texture.active_width == 0 ||
            visible.texture.padding_x + visible.texture.active_width > 320)
            return {MinimapNotificationStatus::invalid,
                    "Minimap notification visible bounds are invalid"};
        const auto now = clock_();
        prune(now);
        if (pulses_.size() >= 100 ||
            std::any_of(pulses_.begin(), pulses_.end(), [&](const auto &pulse) {
                return pulse.marker_id == lookup_plus_one;
            }))
            return {MinimapNotificationStatus::unavailable,
                    "Minimap notification marker slot is occupied"};
        const auto world = minimap_hex_geometry(map_x, map_y, context_.transform).center;
        if (!std::isfinite(world.x) || !std::isfinite(world.y))
            return {MinimapNotificationStatus::invalid,
                    "Minimap notification projection is non-finite"};
        MinimapNotificationPulse pulse;
        pulse.map_x = map_x;
        pulse.map_y = map_y;
        pulse.marker_id = lookup_plus_one;
        pulse.player_id = player_id;
        pulse.turn = turn;
        pulse.owner_sequence = owner_sequence;
        pulse.world = world;
        pulse.started_at = now;
        pulse.expires_at = now + context_.lifetime;
        for (std::uint32_t segment = 0; segment < visible.segment_count; ++segment) {
            const auto &box = visible.segments[segment];
            if (!std::isfinite(box.min_x) || !std::isfinite(box.min_y) ||
                !std::isfinite(box.max_x) || !std::isfinite(box.max_y) ||
                box.max_x <= box.min_x || box.max_y <= box.min_y)
                return {MinimapNotificationStatus::invalid,
                        "Minimap notification world bounds are invalid"};
            const auto x = static_cast<float>(visible.texture.padding_x) +
                (world.x - box.min_x) * visible.texture.active_width /
                    (box.max_x - box.min_x);
            const auto y = (box.max_y - world.y) * 190.0f /
                (box.max_y - box.min_y);
            if (segment == 0 || (x >= visible.texture.padding_x &&
                                 x < visible.texture.padding_x + visible.texture.active_width &&
                                 y >= 0.0f && y < 190.0f))
                pulse.texture_pixel = {x, y};
            if (x >= visible.texture.padding_x &&
                x < visible.texture.padding_x + visible.texture.active_width &&
                y >= 0.0f && y < 190.0f) {
                pulse.inside_view = true;
                break;
            }
        }
        pulses_.push_back(pulse);
        last_sequence_ = owner_sequence;
        return pulse.inside_view
            ? MinimapNotificationResult{MinimapNotificationStatus::queued_visible, {}}
            : MinimapNotificationResult{MinimapNotificationStatus::queued_outside_view,
                                        "Notification is outside the current minimap view"};
    }

    const std::vector<MinimapNotificationPulse> &active() {
        prune(clock_());
        return pulses_;
    }

    std::uint64_t last_sequence() const noexcept { return last_sequence_; }

  private:
    void prune(std::chrono::steady_clock::time_point now) {
        std::erase_if(pulses_, [now](const auto &pulse) {
            return pulse.expires_at <= now;
        });
    }
    MinimapNotificationPulseContext context_;
    TargetReady target_ready_;
    Clock clock_;
    std::uint64_t last_sequence_ = 0;
    std::vector<MinimapNotificationPulse> pulses_;
};

} // namespace civ5::ui
