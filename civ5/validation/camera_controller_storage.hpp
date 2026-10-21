#pragma once
#include "civ5/terrain/camera_controller.hpp"
#include <cstring>

// Validation-only original object representation. Runtime structures contain
// no original vtables, pointers, offsets or opaque original object storage.
namespace camera_controller_storage {
using Bytes = std::array<std::uint8_t, 0x164>;
inline civ5::terrain::CameraControllerState prior(const Bytes &bytes) {
    civ5::terrain::CameraControllerState result{};
    std::memcpy(&result.wrap_orientation, bytes.data() + 0x10c, 4);
    return result;
}
inline Bytes encode(const civ5::terrain::CameraControllerState &state, Bytes bytes) {
    const auto put = [&](unsigned offset, const auto &value) {
        std::memcpy(bytes.data() + offset, &value, sizeof(value));
    };
    put(0xc, state.camera.field_of_view);
    put(0x10, state.camera.aspect_ratio);
    put(0x14, state.pending_motion);
    put(0x34, state.camera.world);
    put(0xb4, state.viewport);
    put(0xd4, state.camera.near_plane);
    put(0xd8, state.camera.far_plane);
    put(0xdc, state.target);
    put(0xe8, state.pan_speed);
    put(0xec, state.zoom_speed);
    put(0xf0, state.minimum_height);
    put(0xf4, state.minimum_zoom);
    put(0xf8, state.maximum_zoom);
    put(0xfc, state.zoom);
    put(0x100, state.time);
    put(0x104, state.flags);
    put(0x10c, state.wrap_orientation);
    put(0x110, state.movement_bounds);
    put(0x120, state.history);
    put(0x140, state.velocity);
    put(0x14c, state.last_position);
    put(0x158, state.inertia_time);
    put(0x15c, state.inertia_duration);
    put(0x160, state.history_index);
    return bytes;
}
} // namespace camera_controller_storage
