#include "civ5/terrain/camera_input.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <type_traits>

namespace civ5::terrain {
namespace {
void require(bool ok, const char *why) {
    if (!ok)
        throw std::invalid_argument(why);
}
Vector3 normalize_basis(const Matrix4 &m, unsigned row, X87Precision pc) {
    const auto q = [pc](double value) { return round_x87(value, pc); };
    const double x = q(double(m[row]) * m[row]);
    const double y = q(double(m[row + 1]) * m[row + 1]);
    const double z = q(double(m[row + 2]) * m[row + 2]);
    // 4A05B0 normalizes the right axis in Z,X,Y order and the other two
    // axes in Z,Y,X order, stores the length to float, then uses scalar SSE.
    const double squared = row == 0 ? q(q(z + x) + y) : q(q(z + y) + x);
    const float length = static_cast<float>(q(std::sqrt(squared)));
    const float reciprocal = 1.f / length;
    return {m[row] * reciprocal, m[row + 1] * reciprocal, m[row + 2] * reciprocal};
}
bool accepts_initial_move(const WorldLayout &world, std::int32_t mode, const CameraControllerState &state,
                          const Vector3 &position) {
    // Complete 4EC860 ordering: project Y to the world ground plane, test
    // global bounds, wrap a local X, then test the configured movement bounds.
    const float slope = (-1.f / state.camera.world[10]) * state.camera.world[9];
    const float offset = (position[2] - world.bounds[2]) * slope;
    if (position[1] > world.bounds[4] - offset || world.bounds[1] - offset > position[1])
        return false;
    auto x = position[0];
    const float y = position[1] + offset;
    if (mode == 1) {
        while (x < world.bounds[0]) {
            const float next = world.bounds[6] + x;
            require(next > x, "Camera X wrap cannot advance");
            x = next;
        }
        while (x >= world.bounds[3]) {
            const float next = x - world.bounds[6];
            require(next < x, "Camera X wrap cannot advance");
            x = next;
        }
    }
    return !(state.movement_bounds[0] > x || x > state.movement_bounds[1] || state.movement_bounds[2] > y ||
             y > state.movement_bounds[3]);
}
bool finite_values(const auto &values) {
    return std::all_of(values.begin(), values.end(), [](float f) { return std::isfinite(f); });
}
void require_camera_input(const WorldLayout &world, std::int32_t mode,
                          const CameraControllerState &state, X87Precision precision) {
    require(precision == X87Precision::single || precision == X87Precision::double_precision,
            "Invalid camera input precision");
    require(mode >= 0 && mode <= 2, "Invalid camera input world mode");
    require(finite_values(world.bounds) && world.bounds[0] < world.bounds[3] &&
                world.bounds[1] < world.bounds[4] && world.bounds[6] > 0 &&
                finite_values(state.camera.world) && state.camera.world[10] != 0 &&
                finite_values(state.target) && finite_values(state.movement_bounds) &&
                state.movement_bounds[0] <= state.movement_bounds[1] &&
                state.movement_bounds[2] <= state.movement_bounds[3], "Invalid camera input state");
}
Vector3 unproject(const Vector3 &point, const std::array<Matrix4, 2> &matrices,
                  X87Precision precision, const ReciprocalEstimate &estimate) {
    // Original D3DX baseline Unproject, with NULL viewport and world. It
    // recomputes the product and inverse for each call, including both rays.
    const auto inverse = inverse_sse(matrix_multiply(matrices[0], matrices[1], precision, MatrixPath::sse), estimate);
    Vector4 transformed;
    for (unsigned i = 0; i < 4; ++i)
        transformed[i] = (point[1] * inverse[4 + i] + point[0] * inverse[i]) +
                         (point[2] * inverse[8 + i] + inverse[12 + i]);
    // Vec3TransformCoord uses RCPPS. Only its W lane reaches the result.
    const float seed = estimate(transformed[3]);
    const float refined = (seed + seed) - ((seed * transformed[3]) * seed);
    Vector3 result;
    for (unsigned i = 0; i < 3; ++i) result[i] = refined * transformed[i];
    require(finite_values(result), "Camera unprojection overflow");
    return result;
}
Vector3 plane_intersection(const Vector4 &plane, const Vector3 &a, const Vector3 &b,
                           const ReciprocalEstimate &estimate) {
    const auto dot = [&](const Vector3 &v) { return (plane[2] * v[2] + plane[0] * v[0]) + plane[1] * v[1]; };
    const float da = dot(a), denominator = da - dot(b);
    if (denominator == 0) return {}; // Original caller initializes output to zero.
    const float seed = estimate(denominator);
    const float refined = (seed + seed) - ((denominator * seed) * seed);
    const float t = (da + plane[3]) * refined;
    Vector3 result;
    for (unsigned i = 0; i < 3; ++i) result[i] = a[i] + t * (b[i] - a[i]);
    require(finite_values(result), "Camera ray intersection overflow");
    return result;
}
} // namespace

CameraPointerResult camera_pointer_event(const CameraPointerState &prior, const CameraPointerEvent &event) {
    require(prior.width > 0 && prior.height > 0 && finite_values(prior.position) &&
                finite_values(prior.drag_origin) && finite_values(prior.pixel_delta) &&
                finite_values(prior.ndc) && finite_values(prior.ndc_delta), "Invalid camera pointer state");
    require(event.kind == CameraPointerEventKind::move || event.kind == CameraPointerEventKind::left_down ||
                event.kind == CameraPointerEventKind::left_up, "Invalid camera pointer event");
    CameraPointerResult result{prior, {}, {}};
    auto &s = result.pointer;
    s.buttons = event.buttons;
    if (event.kind == CameraPointerEventKind::move) {
        const float x = event.x, y = event.y, width = static_cast<float>(s.width), height = static_cast<float>(s.height);
        s.packed_position = std::uint16_t(event.x) | (std::uint32_t(std::uint16_t(event.y)) << 16);
        s.pixel_delta = {s.position[0] - x, s.position[1] - y};
        s.position = {x, y};
        s.ndc = {(x * 2.f) / width - 1.f, 1.f - (y * 2.f) / height};
        s.ndc_delta = {(s.pixel_delta[0] * -2.f) / width, (s.pixel_delta[1] * 2.f) / height};
        if (s.flags & 4) {
            if (!(s.flags & 8) && (std::abs(double(s.drag_origin[0]) - x) > 4 ||
                                   std::abs(double(s.drag_origin[1]) - y) > 4)) {
                s.flags |= 8;
                result.capture = CameraCaptureEvent::capture;
            }
            if (s.flags & 8) {
                s.flags |= 64;
                result.drag = Vector4{s.ndc[0] + -s.ndc_delta[0], s.ndc[1] + -s.ndc_delta[1], s.ndc[0], s.ndc[1]};
            }
        }
    } else if (event.kind == CameraPointerEventKind::left_up) {
        if (s.flags & 4) {
            s.flags &= ~4u;
            if (s.flags & 8) {
                result.capture = CameraCaptureEvent::release;
                s.flags &= ~8u;
            }
        }
    } else {
        // Unconsumed WM_LBUTTONDOWN branch 44D40B: captures the most recent
        // mouse-move coordinates, not the button message's own lParam.
        s.flags |= 4;
        s.drag_origin = s.position;
    }
    return result;
}
CameraPointerState finish_camera_pointer_frame(const CameraPointerState &prior) {
    auto result = prior;
    result.flags &= ~64u;
    return result;
}
CameraMotionInput camera_pointer_motion(const CameraPointerState &pointer, float delta_seconds,
                                       std::uint8_t commands, bool inertia_enabled, std::uint32_t input_mode) {
    return {delta_seconds, commands, bool(pointer.flags & 64), inertia_enabled, bool(pointer.buttons & 1), input_mode};
}
CameraWheelResult camera_wheel_event(const CameraPointerState &prior, const CameraWheelEvent &event,
                                     bool straight_zoom) {
    require(prior.width > 0 && prior.height > 0 && finite_values(prior.ndc), "Invalid camera wheel pointer");
    auto pointer = prior;
    pointer.buttons = event.buttons | (std::uint32_t(std::uint16_t(event.delta)) << 16);
    return {pointer, {pointer.ndc, 1.f, event.delta >= 0, straight_zoom}};
}

CameraControllerState drag_camera(const WorldLayout &world, std::int32_t mode,
                                  const CameraControllerState &prior, const Vector4 &points,
                                  const CameraDragSettings &settings, X87Precision precision,
                                  const ReciprocalEstimate &estimate) {
    require_camera_input(world, mode, prior, precision);
    require(finite_values(points) && std::isfinite(settings.sensitivity) &&
                std::isfinite(settings.ground_plane_units) && bool(estimate), "Invalid camera drag input");
    auto result = prior;
    Vector3 candidate{};
    if (result.flags[4]) {
        const auto matrices = camera_view_projection(result.camera, precision, estimate);
        const Vector4 plane{0, 0, 1, settings.ground_plane_units * -60.f};
        std::array<Vector3, 2> hits;
        for (unsigned i = 0; i < 2; ++i) {
            const unsigned point = i == 0 ? 2 : 0; // Current ray, then previous.
            const auto far = unproject({points[point], points[point + 1], 1}, matrices, precision, estimate);
            const auto near = unproject({points[point], points[point + 1], 0}, matrices, precision, estimate);
            hits[i] = plane_intersection(plane, far, near, estimate);
        }
        for (unsigned i = 0; i < 2; ++i)
            candidate[i] = (hits[1][i] + -hits[0][i]) * settings.sensitivity + result.camera.world[12 + i];
        candidate[2] = result.camera.world[14];
    }
    const auto forward = normalize_basis(result.camera.world, 8, precision);
    const float slope = forward[2] != 0 ? (-1.f / forward[2]) * forward[1] : 0;
    const float offset = (candidate[2] - world.bounds[2]) * slope;
    constexpr float epsilon = std::bit_cast<float>(std::uint32_t{0x3a83126f});
    if (candidate[1] > world.bounds[4] - offset)
        candidate[1] = (world.bounds[4] - offset) - epsilon;
    else if (world.bounds[1] - offset > candidate[1])
        candidate[1] = (world.bounds[1] - offset) + epsilon;
    const float projected_y = offset + candidate[1];
    float x = candidate[0];
    require(finite_values(candidate) && std::isfinite(projected_y), "Camera drag overflow");
    if (mode == 1) {
        while (x < world.bounds[0]) {
            const float next = x + world.bounds[6];
            require(next > x, "Camera drag X wrap cannot advance");
            x = next;
        }
        while (x >= world.bounds[3]) {
            const float next = x - world.bounds[6];
            require(next < x, "Camera drag X wrap cannot advance");
            x = next;
        }
    }
    if (result.movement_bounds[0] > x) candidate[0] = result.movement_bounds[0] + epsilon;
    else if (x > result.movement_bounds[1]) candidate[0] = result.movement_bounds[1] - epsilon;
    if (result.movement_bounds[2] > projected_y) candidate[1] = (result.movement_bounds[2] + epsilon) - offset;
    else if (projected_y > result.movement_bounds[3]) candidate[1] = (result.movement_bounds[3] - epsilon) - offset;
    if (accepts_initial_move(world, mode, result, candidate)) {
        result.target = candidate;
        result.camera.world[12] = candidate[0];
        result.camera.world[13] = candidate[1];
    }
    result.flags[1] &= 0xfe;
    result.flags[3] = 1;
    return result;
}

CameraControllerState zoom_camera(const WorldLayout &world, std::int32_t mode,
                                  const CameraControllerState &prior, const CameraZoomInput &input,
                                  X87Precision precision, const ReciprocalEstimate &estimate) {
    require_camera_input(world, mode, prior, precision);
    require(finite_values(input.ndc) && std::isfinite(input.amount) && input.amount >= 0 &&
                finite_values(std::array{prior.zoom, prior.zoom_speed, prior.minimum_height,
                                         prior.minimum_zoom, prior.maximum_zoom}) && prior.minimum_height > 0 &&
                prior.minimum_zoom <= prior.maximum_zoom && bool(estimate), "Invalid camera zoom input");
    auto result = prior;
    const auto q = [precision](double v) { return round_x87(v, precision); };
    const bool cursor_ray = input.inward || !input.straight_zoom;
    Vector3 direction;
    if (cursor_ray) {
        const auto matrices = camera_view_projection(result.camera, precision, estimate);
        const auto near = unproject({input.ndc[0], input.ndc[1], 0}, matrices, precision, estimate);
        const auto far = unproject({input.ndc[0], input.ndc[1], 1}, matrices, precision, estimate);
        for (unsigned i = 0; i < 3; ++i) direction[i] = near[i] + -far[i];
        const auto square = [&](float v) { return q(double(v) * v); };
        const float length = static_cast<float>(q(std::sqrt(q(q(square(direction[2]) + square(direction[1])) + square(direction[0])))));
        require(std::isfinite(length) && length > 0, "Degenerate camera zoom ray");
        const float inverse = 1.f / length;
        for (auto &v : direction) v = inverse * v;
    } else direction = normalize_basis(result.camera.world, 8, precision);
    const float increment = input.inward ? -input.amount : input.amount;
    const auto move = [&](const Vector3 &axis) {
        require(axis[2] != 0, "Camera zoom ray parallel to ground");
        const float zoom = result.zoom_speed * increment + result.zoom;
        result.zoom = std::clamp(zoom, result.minimum_zoom, result.maximum_zoom);
        require(std::isfinite(result.zoom), "Camera zoom overflow");
        constexpr float exponent_scale = std::bit_cast<float>(std::uint32_t{0x3e19999a});
        constexpr double base = std::bit_cast<double>(std::uint64_t{0x4005bf0a80000000});
        const double exponent = q(double(result.zoom) * exponent_scale);
        const double power = std::pow(base, exponent);
        const float dz = static_cast<float>(q(q(power * result.minimum_height) - result.target[2]));
        const float inverse_z = 1.f / axis[2];
        const Vector3 delta{(axis[0] * inverse_z) * dz, (axis[1] * inverse_z) * dz, dz};
        Vector3 candidate;
        for (unsigned i = 0; i < 3; ++i) candidate[i] = result.target[i] + delta[i] * 1.f;
        require(finite_values(candidate), "Camera zoom position overflow");
        const bool accepted = accepts_initial_move(world, mode, result, candidate);
        if (accepted) result.target = candidate;
        constexpr float log_scale = std::bit_cast<float>(std::uint32_t{0x40d55555});
        result.zoom = static_cast<float>(q(std::log(q(double(result.target[2]) / result.minimum_height)) * log_scale));
        require(std::isfinite(result.zoom), "Camera zoom height overflow");
        return accepted;
    };
    if (!move(direction) && cursor_ray) move(normalize_basis(result.camera.world, 8, precision));
    return result;
}

CameraInputResult advance_camera_input(const WorldLayout &world, std::int32_t mode,
                                       const CameraControllerState &prior, const CameraPointerState &pointer_prior,
                                       const CameraInputFrame &input, X87Precision precision,
                                       const ReciprocalEstimate &estimate) {
    require_camera_input(world, mode, prior, precision);
    require(pointer_prior.width > 0 && pointer_prior.height > 0 && finite_values(pointer_prior.position) &&
                finite_values(pointer_prior.drag_origin) && finite_values(pointer_prior.pixel_delta) &&
                finite_values(pointer_prior.ndc) && finite_values(pointer_prior.ndc_delta) &&
                std::isfinite(input.settings.drag.sensitivity) && std::isfinite(input.settings.drag.ground_plane_units),
            "Invalid camera input frame settings");
    auto controller = prior;
    auto pointer = pointer_prior;
    std::vector<CameraCaptureEvent> captures;
    for (const auto &event : input.events) {
        std::visit([&](const auto &value) {
            using Event = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<Event, CameraPointerEvent>) {
                const auto produced = camera_pointer_event(pointer, value);
                pointer = produced.pointer;
                if (produced.capture) captures.push_back(*produced.capture);
                if (produced.drag)
                    controller = drag_camera(world, mode, controller, *produced.drag, input.settings.drag, precision, estimate);
            } else if constexpr (std::is_same_v<Event, CameraWheelEvent>) {
                const auto produced = camera_wheel_event(pointer, value, input.settings.straight_zoom);
                pointer = produced.pointer;
                controller = zoom_camera(world, mode, controller, produced.zoom, precision, estimate);
            } else {
                controller = zoom_camera(world, mode, controller, value, precision, estimate);
            }
        }, event);
    }
    auto motion = advance_camera(world, mode, controller,
        camera_pointer_motion(pointer, input.delta_seconds, input.commands, input.settings.inertia_enabled, input.input_mode),
        precision, estimate);
    return {std::move(motion), finish_camera_pointer_frame(pointer), std::move(captures)};
}

CameraControllerState initialize_camera(const WorldLayout &world, std::int32_t world_mode,
                                        const CameraStartupSettings &settings,
                                        const CameraControllerState &prior, X87Precision precision) {
    require(precision == X87Precision::single || precision == X87Precision::double_precision,
            "Invalid camera startup precision");
    require(world_mode >= 0 && world_mode <= 2, "Invalid camera startup world mode");
    require(settings.screen_width > 0 && settings.screen_height > 0 && std::isfinite(settings.minimum_zoom),
            "Invalid camera startup settings");
    require(
        std::all_of(world.bounds.begin(), world.bounds.end(), [](float f) { return std::isfinite(f); }) &&
            world.bounds[0] < world.bounds[3] && world.bounds[1] < world.bounds[4] && world.bounds[6] > 0 &&
            world.bounds[7] > 0,
        "Invalid camera startup world bounds");
    const auto q = [precision](double value) { return round_x87(value, precision); };
    CameraControllerState result{};
    result.tilt_degrees = 45;
    result.wrap_orientation = prior.wrap_orientation;
    result.viewport = {0, 1, 1, 0};
    result.pan_speed = 1;
    result.zoom_speed = 2;
    result.minimum_height = 50;
    result.minimum_zoom = 11;
    result.maximum_zoom = 20.5f;
    result.flags[4] = 1;
    result.movement_bounds = {world.bounds[0], world.bounds[3], world.bounds[1], world.bounds[4]};
    // Original float angle and D3DX scalar FSINCOS, then the original SSE
    // matrix multiply. Transcendental instructions do not use x87 PC rounding.
    constexpr float angle = std::bit_cast<float>(std::uint32_t{0x3f490fdb});
    const float cosine = static_cast<float>(std::cos(double(angle)));
    const float sine = static_cast<float>(std::sin(double(angle)));
    const Matrix4 rotation{1, 0, 0, 0, 0, cosine, sine, 0, 0, -sine, cosine, 0, 0, 0, 0, 1};
    result.camera.world = matrix_multiply(rotation, identity_matrix(), precision, MatrixPath::sse);
    const auto right = normalize_basis(result.camera.world, 0, precision);
    const auto up = normalize_basis(result.camera.world, 4, precision);
    const auto forward = normalize_basis(result.camera.world, 8, precision);
    Vector3 position;
    for (unsigned i = 0; i < 3; ++i)
        position[i] = ((right[i] * 0.f + result.camera.world[12 + i]) - forward[i] * -600.f) + up[i] * 0.f;
    if (accepts_initial_move(world, world_mode, result, position)) {
        result.target = position;
        for (unsigned i = 0; i < 3; ++i)
            result.camera.world[12 + i] = position[i];
    }
    // 4A0830: rounded height division, FLDLN2/FYL2X, then rounded FMUL.
    // The multiplier is the original 9F6B38 float, not the real-valued 20/3.
    constexpr float log_scale = std::bit_cast<float>(std::uint32_t{0x40d55555});
    result.zoom =
        static_cast<float>(q(std::log(q(double(result.target[2]) / result.minimum_height)) * log_scale));
    // The complete original encoded getters return 60 and 4000. Width and
    // height are unsigned; negative FILD inputs receive an original 2^32 add.
    const auto unsigned_dimension = [&](std::uint32_t v) {
        const auto signed_value = std::bit_cast<std::int32_t>(v);
        return signed_value < 0 ? q(double(signed_value) + 4294967296.0) : double(signed_value);
    };
    const double aspect =
        q(unsigned_dimension(settings.screen_width) / unsigned_dimension(settings.screen_height));
    const double diagonal_squared = q(60.0 * 60.0);
    const double denominator = q(q(aspect * aspect) + 1.0);
    const double fov = q(std::sqrt(q(diagonal_squared / denominator)));
    result.camera.field_of_view = static_cast<float>(std::max(fov, 26.5));
    result.camera.aspect_ratio = static_cast<float>(aspect);
    result.camera.near_plane = 50;
    result.camera.far_plane = 4000;
    if (settings.minimum_zoom > 0)
        result.minimum_zoom = settings.minimum_zoom;
    return result;
}

CameraControllerState set_camera_position(const WorldLayout &world, std::int32_t mode,
                                          const CameraControllerState &prior, const Vector3 &position,
                                          X87Precision precision) {
    require(precision == X87Precision::single || precision == X87Precision::double_precision,
            "Invalid camera position precision");
    require(mode >= 0 && mode <= 2, "Invalid camera position world mode");
    const auto finite = [](const auto &values) {
        return std::all_of(values.begin(), values.end(), [](float f) { return std::isfinite(f); });
    };
    require(finite(position) && finite(prior.camera.world) && finite(prior.target) &&
                finite(prior.movement_bounds) && finite(world.bounds) &&
                std::isfinite(prior.minimum_height) && prior.minimum_height > 0 &&
                prior.camera.world[10] != 0 && world.bounds[0] < world.bounds[3] &&
                world.bounds[1] < world.bounds[4] && world.bounds[6] > 0,
            "Invalid camera position input");
    auto result = prior;
    if (accepts_initial_move(world, mode, result, position)) {
        result.target = position;
        std::copy(position.begin(), position.end(), result.camera.world.begin() + 12);
    }
    constexpr float scale = std::bit_cast<float>(std::uint32_t{0x40d55555});
    const auto q = [precision](double v) { return round_x87(v, precision); };
    result.zoom = static_cast<float>(q(std::log(q(double(result.target[2]) / result.minimum_height)) * scale));
    require(std::isfinite(result.zoom), "Camera position zoom overflow");
    return result;
}

CameraMotionResult advance_camera(const WorldLayout &world, std::int32_t mode,
                                   const CameraControllerState &prior, const CameraMotionInput &input,
                                   X87Precision precision, const ReciprocalEstimate &estimate) {
    require(precision == X87Precision::single || precision == X87Precision::double_precision,
            "Invalid camera motion precision");
    require(mode >= 0 && mode <= 2, "Invalid camera motion world mode");
    require(std::isfinite(input.delta_seconds) && input.delta_seconds >= 0,
            "Invalid camera motion delta time");
    require(prior.wrap_orientation == 1 || prior.wrap_orientation == -1,
            "Unverified camera wrap orientation");
    require(prior.history_index < 2, "Invalid camera motion history index");
    const auto finite = [](const auto &values) {
        return std::all_of(values.begin(), values.end(), [](float f) { return std::isfinite(f); });
    };
    require(finite(prior.camera.world) && finite(prior.target) && finite(prior.movement_bounds) &&
                finite(prior.velocity) && finite(prior.last_position) && finite(prior.history[0]) &&
                finite(prior.history[1]) && finite(std::array{prior.pan_speed, prior.minimum_height,
                    prior.minimum_zoom, prior.maximum_zoom, prior.zoom, prior.time,
                    prior.inertia_time, prior.inertia_duration}) && prior.minimum_height > 0 &&
                prior.camera.world[10] != 0 && prior.minimum_zoom != prior.maximum_zoom,
            "Invalid entering camera motion state");
    require(std::all_of(world.bounds.begin(), world.bounds.end(), [](float f) { return std::isfinite(f); }) &&
                world.bounds[0] < world.bounds[3] && world.bounds[1] < world.bounds[4] && world.bounds[6] > 0,
            "Invalid camera motion world bounds");
    CameraMotionResult result{prior, {}, {}};
    auto &s = result.controller;
    auto &m = s.camera.world;
    const auto q = [precision](double v) { return round_x87(v, precision); };
    const auto square = [&](float v) { return q(double(v) * v); };
    const auto length_xzy = [&](const Vector3 &v) {
        return q(std::sqrt(q(q(square(v[0]) + square(v[2])) + square(v[1]))));
    };
    const auto length_zyx = [&](const Vector3 &v) {
        return q(std::sqrt(q(q(square(v[2]) + square(v[1])) + square(v[0]))));
    };
    const auto update_zoom = [&] {
        constexpr float scale = std::bit_cast<float>(std::uint32_t{0x40d55555});
        s.zoom = static_cast<float>(q(std::log(q(double(s.target[2]) / s.minimum_height)) * scale));
    };
    const auto clamp_target = [&] {
        const float slope = (-1.f / m[10]) * m[9];
        const float offset = (s.target[2] - world.bounds[2]) * slope;
        const float projected_y = s.target[1] + offset;
        auto x = s.target[0];
        if (s.target[1] > world.bounds[4] - offset)
            s.target[1] = world.bounds[4] - offset;
        if (world.bounds[1] - offset > s.target[1])
            s.target[1] = world.bounds[1] - offset;
        if (mode == 1) {
            while (x < world.bounds[0]) {
                const float next = world.bounds[6] + x;
                require(next > x, "Camera X clamp cannot advance");
                x = next;
            }
            while (x >= world.bounds[3]) {
                const float next = x - world.bounds[6];
                require(next < x, "Camera X clamp cannot advance");
                x = next;
            }
        }
        if (s.movement_bounds[0] > x)
            s.target[0] = s.movement_bounds[0];
        if (x > s.movement_bounds[1])
            s.target[0] = s.movement_bounds[1];
        if (s.movement_bounds[2] > projected_y)
            s.target[1] = s.movement_bounds[2] - offset;
        if (projected_y > s.movement_bounds[3])
            s.target[1] = s.movement_bounds[3] - offset;
    };
    const auto pan = [&](float x, float y) {
        const float right_length = static_cast<float>(q(std::sqrt(q(square(m[0]) + square(m[1])))));
        const float forward_length = static_cast<float>(q(std::sqrt(q(square(m[9]) + square(m[8])))));
        require(std::isfinite(right_length) && right_length > 0 && std::isfinite(forward_length) &&
                    forward_length > 0, "Degenerate camera pan basis");
        const float right_inverse = 1.f / right_length, forward_inverse = 1.f / forward_length;
        const Vector3 right{right_inverse * m[0], right_inverse * m[1], right_inverse * 0.f};
        const Vector3 forward{forward_inverse * m[8], forward_inverse * m[9], forward_inverse * 0.f};
        Vector3 candidate;
        for (unsigned i = 0; i < 3; ++i)
            candidate[i] = (right[i] * x + s.target[i]) - forward[i] * y;
        require(finite(candidate), "Camera pan overflow");
        if (accepts_initial_move(world, mode, s, candidate))
            s.target = candidate;
        update_zoom();
    };
    const auto rotate = [&](float degrees) {
        const float inverse = -1.f / m[10];
        const float x = (inverse * m[8]) * m[14] + m[12];
        const float y = (inverse * m[9]) * m[14] + m[13];
        m[12] = m[12] - x;
        m[13] = m[13] - y;
        s.target[0] = s.target[0] - x;
        s.target[1] = s.target[1] - y;
        constexpr double radians = std::bit_cast<double>(std::uint64_t{0x3f91df46a2529d39});
        const float angle = static_cast<float>(q(double(degrees) * radians));
        const float cosine = static_cast<float>(std::cos(double(angle)));
        const float sine = static_cast<float>(std::sin(double(angle)));
        const Matrix4 rotation{cosine, sine, 0, 0, -sine, cosine, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
        m = matrix_multiply(m, rotation, precision, MatrixPath::sse);
        Vector3 transformed;
        // D3DXVec3Transform SSE groups Y+X and Z+translation, unlike Vec4.
        for (unsigned i = 0; i < 3; ++i)
            transformed[i] = (s.target[1] * rotation[4 + i] + s.target[0] * rotation[i]) +
                             (s.target[2] * rotation[8 + i] + rotation[12 + i]);
        s.target = transformed;
        m[12] = m[12] + x;
        m[13] = m[13] + y;
        s.target[0] = s.target[0] + x;
        s.target[1] = s.target[1] + y;
    };
    const auto wrap = [&] {
        if (mode != 1)
            return;
        const auto emit = [&](std::uint32_t direction) {
            const float orientation = static_cast<float>(s.wrap_orientation);
            for (unsigned row : {4u, 0u})
                for (unsigned i = 0; i < 3; ++i)
                    m[row + i] = orientation * m[row + i];
            s.wrap_orientation = std::abs(s.wrap_orientation);
            result.events.push_back({CameraMotionEventKind::wrapped, direction});
        };
        const float period = world.bounds[3] - world.bounds[0];
        // Original 423060 shifts one period per branch, with strict bounds.
        if (m[12] > world.bounds[3]) {
            m[12] = m[12] - period;
            s.target[0] = s.target[0] - period;
            emit(1);
        }
        if (world.bounds[0] > m[12]) {
            m[12] = m[12] + period;
            s.target[0] = s.target[0] + period;
            emit(0);
        }
    };
    const float dt = input.delta_seconds;
    s.flags[0] = input.commands;
    s.time = dt + s.time;
    require(std::isfinite(s.time), "Camera motion time overflow");
    bool moving = false;
    if (input.sample_inertia && input.inertia_enabled) {
        if (input.dragging) {
            s.flags[1] &= 0xfe;
            auto &sample = s.history[s.history_index];
            sample[3] = dt;
            for (unsigned i = 0; i < 3; ++i)
                sample[i] = m[12 + i] - s.last_position[i];
            if (++s.history_index == 2)
                s.history_index = 0;
        } else {
            s.velocity = {};
            float duration = 0;
            for (const auto &sample : s.history) {
                if (sample[3] != 0) {
                    for (unsigned i = 0; i < 3; ++i)
                        s.velocity[i] = sample[i] + s.velocity[i];
                    duration = sample[3] + duration;
                }
            }
            if (duration > 0) {
                const float inverse = 1.f / duration;
                for (auto &v : s.velocity)
                    v = (v * inverse) * .9f;
                s.flags[1] |= 1;
                s.inertia_time = s.time;
                const double duration_value = q(q(q(length_zyx(s.velocity) * .5) / m[14]) + .125);
                s.inertia_duration = (duration_value < 2 ? static_cast<float>(duration_value) : 2.f) * .5f;
            }
            s.history = {};
            s.history_index = 0;
        }
    }
    if (s.flags[1] & 1) {
        const float phase = (s.time - s.inertia_time) / s.inertia_duration;
        require(std::isfinite(phase), "Invalid camera inertia interval");
        if (phase > 1) {
            s.flags[1] &= 0xfe;
        } else {
            constexpr double pi = std::bit_cast<double>(std::uint64_t{0x400921fb54442d18});
            constexpr double half_pi = std::bit_cast<double>(std::uint64_t{0x3ff921fb54442d18});
            const double angle = q(q(double(phase) * pi) + half_pi);
            const float scale = static_cast<float>(q(q(q(std::sin(angle) * .5) + .5) * dt));
            for (unsigned i = 0; i < 3; ++i)
                s.target[i] = s.target[i] + s.velocity[i] * scale;
            require(finite(s.target), "Camera inertia overflow");
            clamp_target();
            for (unsigned i = 0; i < 3; ++i)
                m[12 + i] = s.target[i];
            update_zoom();
            moving = true;
        }
    }
    for (unsigned command : {1u, 2u, 4u, 8u}) {
        if (input.commands & command) {
            const float distance = static_cast<float>(q(q(std::abs(double(m[14])) * s.pan_speed) * dt));
            if (command == 1) pan(-distance, 0);
            if (command == 2) pan(distance, 0);
            if (command == 4) pan(0, distance);
            if (command == 8) pan(0, -distance);
            moving = true;
        }
    }
    if (input.commands & 16) { rotate(dt * 40.f); moving = true; }
    if (input.commands & 32) { rotate(dt * -40.f); moving = true; }
    if (input.commands & 0xc0) moving = true;
    wrap();
    auto target = s.target;
    Vector3 difference;
    const auto difference_from = [&](const Vector3 &v) {
        return Vector3{m[12] + -v[0], m[13] + -v[1], m[14] + -v[2]};
    };
    difference = difference_from(target);
    const double initial_distance = length_xzy(difference);
    if (initial_distance > 1) {
        if (mode == 1) {
            const float period = world.bounds[3] - world.bounds[0];
            auto plus = target, minus = target;
            plus[0] = plus[0] + period;
            minus[0] = minus[0] - period;
            const auto plus_difference = difference_from(plus), minus_difference = difference_from(minus);
            if (initial_distance > length_zyx(plus_difference)) {
                target = plus;
                difference = plus_difference;
            } else if (initial_distance > length_zyx(minus_difference)) {
                target = minus;
                difference = minus_difference;
            }
        }
        const float distance = static_cast<float>(length_xzy(difference));
        if (distance > 1) {
            constexpr float attenuation = std::bit_cast<float>(std::uint32_t{0x3f79c190});
            const double power = std::pow(q(double(distance) * attenuation), .25);
            const double value = q(q(power * attenuation) - dt);
            const float factor = value < 0 ? 0.f : static_cast<float>(value);
            const float remaining = ((factor * factor) * factor) * factor * 1.025f;
            const float inverse = 1.f / distance;
            for (unsigned i = 0; i < 3; ++i)
                m[12 + i] = target[i] + (inverse * difference[i]) * remaining;
            wrap();
            moving = true;
        }
    }
    if (input.input_mode == 2 || s.flags[3] != 0)
        moving = true;
    s.flags[3] = 0;
    if (moving) {
        const auto matrices = camera_view_projection(s.camera, precision, estimate);
        result.events.push_back({CameraMotionEventKind::camera_changed, 0, matrices[0], matrices[1]});
    }
    result.zoom = {(s.zoom - s.minimum_zoom) / (s.maximum_zoom - s.minimum_zoom), s.zoom,
                   s.minimum_zoom, s.maximum_zoom};
    require(finite(m) && finite(s.target) && finite(result.zoom), "Camera motion output overflow");
    if (moving != s.flags[2]) {
        result.events.push_back({moving ? CameraMotionEventKind::started : CameraMotionEventKind::stopped});
        s.flags[2] = static_cast<std::uint8_t>(moving);
    }
    for (unsigned i = 0; i < 3; ++i)
        s.last_position[i] = m[12 + i];
    return result;
}
} // namespace civ5::terrain
