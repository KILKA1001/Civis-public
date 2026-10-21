#pragma once
#include "camera_controller_storage.hpp"
#include <type_traits>

// Validation-only scene inputs. These are client pixel/button/wheel events;
// no world-space camera position or expected intermediate state is supplied.
namespace {
struct CameraSceneInputStep {
    float dt = 1.f / 60;
    std::uint8_t commands = 0;
    std::uint32_t input_mode = 0;
    std::vector<CameraInputEvent> events;
};
std::vector<CameraSceneInputStep> camera_scene_input_steps() {
    std::vector<CameraSceneInputStep> result;
    for (unsigned i = 0; i < 250; ++i)
        result.push_back({1.f / 60, 2, 0, {}});
    for (unsigned i = 0; i < 12; ++i)
        result.push_back({1.f / 60, 16, 0, {}});
    result.push_back({1.f / 60,
                      0,
                      0,
                      {CameraPointerEvent{CameraPointerEventKind::move, 640, 480, 0},
                       CameraPointerEvent{CameraPointerEventKind::left_down, 640, 480, 1}}});
    for (int i = 1; i <= 28; ++i)
        result.push_back(
            {1.f / 60,
             0,
             0,
             {CameraPointerEvent{CameraPointerEventKind::move, static_cast<std::int16_t>(640 - 12 * i),
                                 static_cast<std::int16_t>(480 + 4 * i), 1}}});
    result.push_back({1.f / 60,
                      0,
                      0,
                      {CameraPointerEvent{CameraPointerEventKind::move, 292, 596, 1},
                       CameraPointerEvent{CameraPointerEventKind::left_up, 292, 596, 0}}});
    for (unsigned i = 0; i < 31; ++i) {
        CameraSceneInputStep step;
        if (i == 0)
            step.events.push_back(CameraWheelEvent{-120, 0});
        if (i == 8)
            step.events.push_back(CameraWheelEvent{120, 0});
        result.push_back(std::move(step));
    }
    return result;
}
void pointer_bytes(Writer &out, const CameraPointerState &s) {
    for (auto v : {std::bit_cast<std::uint32_t>(s.width), std::bit_cast<std::uint32_t>(s.height),
                   s.packed_position, s.buttons})
        out.word(v);
    for (const auto &values : {s.position, s.drag_origin, s.pixel_delta, s.ndc, s.ndc_delta})
        for (float v : values)
            out.word(std::bit_cast<std::uint32_t>(v));
    out.word(s.flags);
}
CameraControllerState run_camera_input_scene(
    GlWaterWorldRenderer &water, CameraControllerState controller,
    const camera_controller_storage::Bytes &prior, const WaterReflectionFrameSettings &frame_settings,
    const ReciprocalEstimate &rcp, const ReciprocalSqrtEstimate &rsqrt, const fs::path &output,
    float *normalized_zoom = nullptr, const std::function<void(unsigned, float)> &checkpoint = {},
    const std::function<void(std::span<const CameraMotionEvent>)> &motion_events = {}) {
    const CameraInputSettings settings{{1, 0}, false, true};
    CameraPointerState pointer{1280, 960};
    Writer commands, events, captures, pointers, configuration;
    configuration.word(std::bit_cast<std::uint32_t>(settings.drag.sensitivity));
    configuration.word(std::bit_cast<std::uint32_t>(settings.drag.ground_plane_units));
    configuration.word(settings.straight_zoom);
    configuration.word(settings.inertia_enabled);
    commands.bytes = {'C', '5', 'I', 'N', 'P', '0', '0', '1'};
    const auto steps = camera_scene_input_steps();
    check(steps.size() == 323, "Complete scene camera input sequence");
    commands.word(static_cast<std::uint32_t>(steps.size()));
    std::vector<camera_controller_storage::Bytes> controllers;
    std::vector<WaterCameraFrameState> frames;
    std::vector<Vector4> zoom;
    for (const auto &step : steps) {
        commands.word(std::bit_cast<std::uint32_t>(step.dt));
        commands.word(step.commands);
        commands.word(step.input_mode);
        commands.word(static_cast<std::uint32_t>(step.events.size()));
        for (const auto &event : step.events) {
            std::visit(
                [&](const auto &value) {
                    using Event = std::decay_t<decltype(value)>;
                    if constexpr (std::is_same_v<Event, CameraPointerEvent>) {
                        commands.word(static_cast<std::uint32_t>(value.kind));
                        commands.word(std::bit_cast<std::uint32_t>(static_cast<std::int32_t>(value.x)));
                        commands.word(std::bit_cast<std::uint32_t>(static_cast<std::int32_t>(value.y)));
                        commands.word(value.buttons);
                    } else if constexpr (std::is_same_v<Event, CameraWheelEvent>) {
                        commands.word(5);
                        commands.word(std::bit_cast<std::uint32_t>(static_cast<std::int32_t>(value.delta)));
                        commands.word(value.buttons);
                    } else {
                        throw std::invalid_argument("Scene camera must use pixel and wheel events");
                    }
                },
                event);
        }
        const auto result = water.prepare_camera_input(
            controller, pointer, {step.dt, step.commands, step.input_mode, settings, step.events},
            frame_settings, rcp, rsqrt);
        if (motion_events)
            motion_events(result.motion.events);
        controller = result.motion.controller;
        pointer = result.pointer;
        controllers.push_back(camera_controller_storage::encode(controller, prior));
        frames.push_back({water.camera_state(), water.frame_state()});
        zoom.push_back(result.motion.zoom);
        pointer_bytes(pointers, pointer);
        captures.word(static_cast<std::uint32_t>(result.capture_events.size()));
        for (auto event : result.capture_events)
            captures.word(static_cast<std::uint32_t>(event));
        events.word(static_cast<std::uint32_t>(result.motion.events.size()));
        for (const auto &event : result.motion.events) {
            events.word(static_cast<std::uint32_t>(event.kind));
            if (event.kind == CameraMotionEventKind::wrapped)
                events.word(event.wrap_direction);
            if (event.kind == CameraMotionEventKind::camera_changed) {
                for (float f : event.view)
                    events.word(std::bit_cast<std::uint32_t>(f));
                for (float f : event.projection)
                    events.word(std::bit_cast<std::uint32_t>(f));
            }
        }
        if (checkpoint)
            checkpoint(static_cast<unsigned>(frames.size() - 1), result.motion.zoom[0]);
    }
    save(output / "camera-inputs.bin", commands.bytes);
    save(output / "input-settings.bin", configuration.bytes);
    save(output / "input-events.bin", events.bytes);
    save(output / "input-capture.bin", captures.bytes);
    save(output / "input-pointer.bin", pointers.bytes);
    water_validation::save(output / "input-controllers.bin", controllers);
    water_validation::save(output / "input-frames.bin", frames);
    water_validation::save(output / "input-zoom.bin", zoom);
    if (normalized_zoom)
        *normalized_zoom = zoom.back()[0];
    return controller;
}
} // namespace
