#include "camera_controller_storage.hpp"
#include "civ5/terrain/camera_input.hpp"
#include "civ5/materials/water_reflection.hpp"
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace civ5::terrain;
using namespace civ5::materials;
namespace {
void require(bool ok, const char *why) {
    if (!ok)
        throw std::invalid_argument(why);
}
struct Reader {
    std::vector<std::uint8_t> bytes;
    std::size_t at = 8;
    std::uint32_t word() {
        require(at <= bytes.size() && bytes.size() - at >= 4, "Truncated camera primitive");
        std::uint32_t result = 0;
        for (unsigned i = 0; i < 4; ++i)
            result |= std::uint32_t(bytes[at++]) << (8 * i);
        return result;
    }
    std::int32_t integer() { return std::bit_cast<std::int32_t>(word()); }
    float number() { return std::bit_cast<float>(word()); }
    void floats(auto &values) {
        for (auto &v : values)
            v = number();
    }
    void copy(auto &value) {
        require(at <= bytes.size() && bytes.size() - at >= sizeof(value), "Truncated prior camera storage");
        std::memcpy(&value, bytes.data() + at, sizeof(value));
        at += sizeof(value);
    }
};
struct Writer {
    std::vector<std::uint8_t> bytes;
    void value(const auto &v) {
        const auto *p = reinterpret_cast<const std::uint8_t *>(&v);
        bytes.insert(bytes.end(), p, p + sizeof(v));
    }
    void grid(const WorldGrid &g) {
        value(g.first_matrix);
        value(g.second_matrix);
        value(g.descriptor);
        value(g.width);
        value(g.height);
    }
    void pointer(const CameraPointerState &s) {
        value(s.width); value(s.height); value(s.packed_position); value(s.buttons);
        value(s.position); value(s.drag_origin); value(s.pixel_delta); value(s.ndc); value(s.ndc_delta);
        value(s.flags);
    }
};
} // namespace
int main(int argc, char **argv) {
    try {
        require(argc == 3, "Usage: camera_input_oracle primitive-input fresh-output");
        std::ifstream input(argv[1], std::ios::binary | std::ios::ate);
        require(bool(input), "Open motion input");
        const auto size = input.tellg();
        require(size >= 12 && size <= 64 * 1024 * 1024, "Bounded motion input");
        Reader r{std::vector<std::uint8_t>(static_cast<std::size_t>(size))};
        input.seekg(0);
        input.read(reinterpret_cast<char *>(r.bytes.data()), size);
        require(bool(input) && std::memcmp(r.bytes.data(), "C5CAMI01", 8) == 0, "Camera input schema");
        const auto count = r.word();
        require(count > 0 && count <= 2048, "Motion sequence count");
        Writer out;
        out.value(count);
        for (unsigned i = 0; i < count; ++i) {
            const auto width = r.integer(), height = r.integer(), mode = r.integer();
            const auto pc = r.word();
            require(pc == 24 || pc == 53, "Motion precision");
            const auto precision = pc == 24 ? X87Precision::single : X87Precision::double_precision;
            const CameraStartupSettings settings{r.word(), r.word(), r.number()};
            camera_controller_storage::Bytes prior;
            r.copy(prior);
            std::array<std::array<std::uint32_t, 2>, 2> world_inverses;
            for (auto &pair : world_inverses) for (auto &v : pair) v = r.word();
            unsigned cursor = 0;
            const auto world = build_world_layout(width, height, mode, {}, [&](float det) {
                require(cursor < 2 && std::bit_cast<std::uint32_t>(det) == world_inverses[cursor][0],
                        "Motion world determinant mismatch");
                return std::bit_cast<float>(world_inverses[cursor++][1]);
            });
            require(cursor == 2, "Motion world inverse count");
            auto controller = initialize_camera(world, mode, settings, camera_controller_storage::prior(prior), precision);
            out.value(camera_controller_storage::encode(controller, prior));
            out.value(controller.tilt_degrees);
            out.value(world.transforms);
            out.value(world.bounds);
            out.grid(world.source_grid);
            out.grid(world.worker_grid);
            WaterCameraFrameState frame{};
            r.copy(frame.camera);
            r.copy(frame.water.bounds);
            r.copy(frame.water.matrices);
            WaterReflectionFrameSettings lighting{};
            r.floats(lighting.source_scales);
            lighting.angles = {r.number(), r.number(), r.number(), r.number()};
            r.floats(lighting.shadow_direction);
            const CameraDragSettings drag_settings{r.number(), r.number()};
            const auto straight = r.word(), inertia_enabled = r.word();
            require(straight <= 1 && inertia_enabled <= 1 && settings.screen_width <= 0x7fffffff &&
                        settings.screen_height <= 0x7fffffff, "Camera pointer configuration");
            CameraPointerState pointer{static_cast<std::int32_t>(settings.screen_width), static_cast<std::int32_t>(settings.screen_height)};
            out.pointer(pointer);
            const auto steps = r.word();
            require(steps > 0 && steps <= 16384, "Motion step count");
            out.value(steps);
            for (unsigned step = 0; step < steps; ++step) {
                const float dt = r.number();
                const auto commands = r.word(), force = r.word(), event_count = r.word();
                require(commands <= 255 && event_count <= 4096, "Camera input commands/event count");
                out.value(event_count);
                for (unsigned event = 0; event < event_count; ++event) {
                    const auto kind = r.word();
                    require(kind <= 5, "Camera input event kind");
                    std::optional<Vector4> drag;
                    float amount = 0;
                    bool inward = kind == 3;
                    if (kind <= 2) {
                        const auto x = r.integer(), y = r.integer();
                        const auto buttons = r.word();
                        require(x >= -32768 && x <= 32767 && y >= -32768 && y <= 32767, "Camera pointer coordinate range");
                        const auto result = camera_pointer_event(pointer,
                            {static_cast<CameraPointerEventKind>(kind), static_cast<std::int16_t>(x), static_cast<std::int16_t>(y), buttons});
                        pointer = result.pointer;
                        out.pointer(pointer);
                        out.value(result.capture ? 1u + static_cast<std::uint32_t>(*result.capture) : 0u);
                        out.value(static_cast<std::uint32_t>(result.drag.has_value()));
                        drag = result.drag;
                        if (drag) out.value(*drag);
                    } else if (kind == 5) {
                        const auto delta = r.integer();
                        const auto buttons = r.word();
                        require(delta >= -32768 && delta <= 32767 && buttons <= 65535, "Camera wheel input range");
                        const auto wheel = camera_wheel_event(pointer,
                            {static_cast<std::int16_t>(delta), static_cast<std::uint16_t>(buttons)}, bool(straight));
                        pointer = wheel.pointer;
                        amount = wheel.zoom.amount;
                        inward = wheel.zoom.inward;
                        out.pointer(pointer);
                        out.value(inward ? 0u : 1u);
                        out.value(Vector4{0, wheel.zoom.ndc[0], wheel.zoom.ndc[1], amount});
                        out.value(pointer.ndc);
                    } else amount = r.number();
                    const auto estimate_count = r.word();
                    require(estimate_count <= 32, "Camera input estimate count");
                    std::vector<std::array<std::uint32_t, 2>> estimates(estimate_count);
                    for (auto &pair : estimates) for (auto &v : pair) v = r.word();
                    unsigned estimate_cursor = 0;
                    const auto estimate = [&](float value) {
                        if (!(estimate_cursor < estimates.size() && std::bit_cast<std::uint32_t>(value) == estimates[estimate_cursor][0]))
                            throw std::invalid_argument("Camera input RCP mismatch at sequence " + std::to_string(i) +
                                " step " + std::to_string(step) + " event " + std::to_string(event) +
                                " operation " + std::to_string(estimate_cursor) + " value " + std::to_string(std::bit_cast<std::uint32_t>(value)));
                        return std::bit_cast<float>(estimates[estimate_cursor++][1]);
                    };
                    if (drag) controller = drag_camera(world, mode, controller, *drag, drag_settings, precision, estimate);
                    else if (kind >= 3) controller = zoom_camera(world, mode, controller, {pointer.ndc, amount, inward, bool(straight)}, precision, estimate);
                    require(estimate_cursor == estimates.size(), "Complete camera input estimate consumption");
                    if (drag || kind >= 3) out.value(camera_controller_storage::encode(controller, prior));
                }
                out.pointer(pointer);
                const auto command = camera_pointer_motion(pointer, dt, static_cast<std::uint8_t>(commands), bool(inertia_enabled), force);
                const auto inverse_count = r.word();
                require(inverse_count <= 1, "Motion callback inverse count");
                std::array<std::uint32_t, 2> inverse{};
                if (inverse_count) for (auto &v : inverse) v = r.word();
                unsigned used = 0;
                const auto result = advance_camera(world, mode, controller, command, precision, [&](float det) {
                    if (!(used++ < inverse_count && std::bit_cast<std::uint32_t>(det) == inverse[0]))
                        throw std::invalid_argument("Motion determinant mismatch at step " + std::to_string(step));
                    return std::bit_cast<float>(inverse[1]);
                });
                require(used == inverse_count, "Motion inverse consumption");
                controller = result.controller;
                out.value(camera_controller_storage::encode(controller, prior));
                out.value(result.zoom);
                out.value(static_cast<std::uint32_t>(result.events.size()));
                for (const auto &event : result.events) {
                    out.value(static_cast<std::uint32_t>(event.kind));
                    if (event.kind == CameraMotionEventKind::wrapped) out.value(event.wrap_direction);
                    if (event.kind == CameraMotionEventKind::camera_changed) { out.value(event.view); out.value(event.projection); }
                }
                const auto frame_inverse_count = r.word();
                require(frame_inverse_count >= 3 && frame_inverse_count <= 5, "Motion frame inverse count");
                std::vector<std::array<std::uint32_t, 2>> frame_inverses(frame_inverse_count);
                for (auto &pair : frame_inverses) for (auto &v : pair) v = r.word();
                std::array<std::array<std::uint32_t, 2>, 4> normals;
                for (auto &pair : normals) for (auto &v : pair) v = r.word();
                unsigned frame_cursor = 0, normal_cursor = 0;
                frame = water_camera_frame(world, mode, controller.camera, lighting, frame, precision,
                    [&](float det) {
                        if (!(frame_cursor < frame_inverses.size() &&
                              std::bit_cast<std::uint32_t>(det) == frame_inverses[frame_cursor][0]))
                            throw std::invalid_argument("Motion frame determinant mismatch at step " + std::to_string(step));
                        return std::bit_cast<float>(frame_inverses[frame_cursor++][1]);
                    }, [&](float squared) {
                        if (!(normal_cursor < 4 && std::bit_cast<std::uint32_t>(squared) == normals[normal_cursor][0]))
                            throw std::invalid_argument("Motion frame RSQRT mismatch at step " + std::to_string(step));
                        return std::bit_cast<float>(normals[normal_cursor++][1]);
                    });
                require(frame_cursor == frame_inverses.size() && normal_cursor == 4,
                        "Complete motion frame estimate consumption");
                out.value(frame);
                pointer = finish_camera_pointer_frame(pointer);
                out.pointer(pointer);
            }
        }
        require(r.at == r.bytes.size(), "Trailing motion input");
        require(!std::filesystem::exists(argv[2]), "Fresh motion output required");
        std::ofstream output(argv[2], std::ios::binary);
        output.write(reinterpret_cast<const char *>(out.bytes.data()), out.bytes.size());
        output.close();
        require(bool(output), "Complete motion output");
        std::cout << "PASS " << count << " camera input sequences\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
