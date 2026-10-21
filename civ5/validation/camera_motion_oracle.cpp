#include "camera_controller_storage.hpp"
#include "civ5/materials/water_reflection.hpp"
#include "civ5/materials/material_cache_scheduler.hpp"
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
};
} // namespace
int main(int argc, char **argv) {
    try {
        require(argc == 3 || argc == 5,
                "Usage: camera_motion_oracle primitive-input fresh-output [controls-input fresh-controls-output]");
        std::vector<MaterialCacheStartup> controls;
        Writer control_output;
        if (argc == 5) {
            require(!std::filesystem::exists(argv[4]), "Fresh material camera output required");
            std::ifstream stream(argv[3], std::ios::binary | std::ios::ate);
            require(bool(stream), "Open material camera controls");
            const auto length = stream.tellg();
            require(length >= 12 && length <= 12 + 256 * 16, "Bounded material camera controls");
            Reader settings{std::vector<std::uint8_t>(static_cast<std::size_t>(length))};
            stream.seekg(0);
            stream.read(reinterpret_cast<char *>(settings.bytes.data()), length);
            require(bool(stream) && std::memcmp(settings.bytes.data(), "C5MCIN01", 8) == 0,
                    "Material camera controls schema");
            const auto profiles = settings.word();
            require(profiles > 0 && profiles <= 256, "Material camera controls count");
            for (unsigned i = 0; i < profiles; ++i) {
                MaterialCacheStartup profile;
                profile.moving_budget = settings.word();
                profile.still_budget = settings.word();
                const auto fast = settings.word(), forced = settings.word();
                require(fast <= 1 && forced <= 1, "Material camera control flags");
                profile.fast_when_still = fast != 0;
                profile.force_fast = forced != 0;
                controls.push_back(profile);
            }
            require(settings.at == settings.bytes.size(), "Trailing material camera controls");
            control_output.bytes = {'C', '5', 'M', 'C', 'O', 'U', '0', '1'};
        }
        std::ifstream input(argv[1], std::ios::binary | std::ios::ate);
        require(bool(input), "Open motion input");
        const auto size = input.tellg();
        require(size >= 12 && size <= 64 * 1024 * 1024, "Bounded motion input");
        Reader r{std::vector<std::uint8_t>(static_cast<std::size_t>(size))};
        input.seekg(0);
        input.read(reinterpret_cast<char *>(r.bytes.data()), size);
        require(bool(input) && std::memcmp(r.bytes.data(), "C5CAMM03", 8) == 0, "Motion schema");
        const auto count = r.word();
        require(count > 0 && count <= 2048, "Motion sequence count");
        Writer out;
        out.value(count);
        if (!controls.empty()) control_output.value(count);
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
            const auto steps = r.word();
            require(steps > 0 && steps <= 16384, "Motion step count");
            out.value(steps);
            MaterialCacheCameraState material_camera;
            if (!controls.empty()) {
                control_output.value(steps);
                control_output.value(static_cast<std::uint32_t>(controls.size()));
            }
            for (unsigned step = 0; step < steps; ++step) {
                const float dt = r.number();
                const auto commands = r.word(), inertia = r.word(), event = r.word(), drag = r.word(), force = r.word();
                require(commands <= 255 && inertia <= 1 && event <= 1 && drag <= 1, "Motion input bits");
                const CameraMotionInput command{dt, static_cast<std::uint8_t>(commands), bool(inertia), bool(event), bool(drag), force};
                const auto set_position = r.word();
                require(set_position <= 1, "Motion position presence");
                if (set_position) {
                    Vector3 position;
                    r.floats(position);
                    controller = set_camera_position(world, mode, controller, position, precision);
                }
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
                material_camera.consume(result.events);
                for (const auto &profile : controls) {
                    MaterialCacheSchedulerSettings schedule;
                    material_cache_camera_controls(material_camera, profile, schedule);
                    for (auto value : {std::uint32_t(material_camera.moving),
                                       std::uint32_t(schedule.fast), std::uint32_t(schedule.force_fast),
                                       schedule.attempt_budget, schedule.fast_selection_budget})
                        control_output.value(value);
                }
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
            }
        }
        require(r.at == r.bytes.size(), "Trailing motion input");
        require(!std::filesystem::exists(argv[2]), "Fresh motion output required");
        std::ofstream output(argv[2], std::ios::binary);
        output.write(reinterpret_cast<const char *>(out.bytes.data()), out.bytes.size());
        output.close();
        require(bool(output), "Complete motion output");
        if (!controls.empty()) {
            std::ofstream stream(argv[4], std::ios::binary);
            stream.write(reinterpret_cast<const char *>(control_output.bytes.data()), control_output.bytes.size());
            stream.close();
            require(bool(stream), "Complete material camera output");
        }
        std::cout << "PASS " << count << " camera motion sequences\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
