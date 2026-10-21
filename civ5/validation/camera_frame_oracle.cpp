#include "camera_controller_storage.hpp"
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
};
} // namespace
int main(int argc, char **argv) {
    try {
        static_assert(std::endian::native == std::endian::little);
        static_assert(sizeof(PerspectiveCamera) == 80 && sizeof(CameraFrameState) == 1012 &&
                      sizeof(WaterCameraFrameState) == 2188);
        require(argc == 3, "Usage: camera_frame_oracle primitive-input fresh-output");
        std::ifstream input(argv[1], std::ios::binary | std::ios::ate);
        require(bool(input), "Open camera input");
        const auto size = input.tellg();
        require(size >= 12 && size <= 16 * 1024 * 1024, "Bounded camera input");
        Reader r{std::vector<std::uint8_t>(static_cast<std::size_t>(size))};
        input.seekg(0);
        input.read(reinterpret_cast<char *>(r.bytes.data()), size);
        const bool startup = std::memcmp(r.bytes.data(), "C5CAMS01", 8) == 0;
        require(bool(input) && (startup || std::memcmp(r.bytes.data(), "C5CAMF01", 8) == 0),
                "Camera input schema");
        const auto count = r.word();
        require(count > 0 && count <= 2048, "Camera case count");
        Writer output;
        output.value(count);
        for (unsigned i = 0; i < count; ++i) {
            const auto width = r.integer(), height = r.integer(), mode = r.integer();
            const auto pc = r.word();
            require(pc == 24 || pc == 53, "Camera precision");
            const auto precision = pc == 24 ? X87Precision::single : X87Precision::double_precision;
            PerspectiveCamera camera{};
            CameraStartupSettings startup_settings{};
            camera_controller_storage::Bytes controller_prior{};
            if (startup) {
                startup_settings = {r.word(), r.word(), r.number()};
                r.copy(controller_prior);
            } else {
                camera = {r.number(), r.number(), r.number(), r.number(), {}};
                r.floats(camera.world);
            }
            WaterCameraFrameState prior{};
            r.copy(prior.camera);
            WaterReflectionFrameSettings settings{};
            r.floats(settings.source_scales);
            r.copy(prior.water.bounds);
            settings.angles = {r.number(), r.number(), r.number(), r.number()};
            r.floats(settings.shadow_direction);
            r.copy(prior.water.matrices);
            const auto inverse_count = r.word();
            require(inverse_count >= 5 && inverse_count <= 7, "Camera inverse count");
            std::vector<std::array<std::uint32_t, 2>> inverses(inverse_count);
            for (auto &v : inverses)
                for (auto &word : v)
                    word = r.word();
            std::array<std::array<std::uint32_t, 2>, 4> normalizations;
            for (auto &v : normalizations)
                for (auto &word : v)
                    word = r.word();
            unsigned inverse_cursor = 0;
            const auto reciprocal = [&](float det) {
                require(inverse_cursor < inverses.size() &&
                            std::bit_cast<std::uint32_t>(det) == inverses[inverse_cursor][0],
                        "Original camera determinant mismatch");
                return std::bit_cast<float>(inverses[inverse_cursor++][1]);
            };
            const auto world = build_world_layout(width, height, mode, {}, reciprocal);
            require(inverse_cursor == 2, "Complete camera world inverse chain");
            if (startup) {
                const auto controller =
                    initialize_camera(world, mode, startup_settings,
                                      camera_controller_storage::prior(controller_prior), precision);
                camera = controller.camera;
                output.value(camera_controller_storage::encode(controller, controller_prior));
                output.value(controller.tilt_degrees);
            }
            unsigned normal_cursor = 0;
            const auto result = water_camera_frame(
                world, mode, camera, settings, prior, precision, reciprocal, [&](float squared) {
                    require(normal_cursor < normalizations.size() &&
                                std::bit_cast<std::uint32_t>(squared) == normalizations[normal_cursor][0],
                            "Original camera reflection RSQRT input mismatch");
                    return std::bit_cast<float>(normalizations[normal_cursor++][1]);
                });
            require(inverse_cursor == inverses.size() && normal_cursor == 4, "Complete camera math chain");
            output.value(world.transforms);
            output.value(world.bounds);
            output.grid(world.source_grid);
            output.grid(world.worker_grid);
            output.value(result);
        }
        require(r.at == r.bytes.size(), "Trailing camera input");
        require(!std::filesystem::exists(argv[2]), "Fresh camera output required");
        std::ofstream out(argv[2], std::ios::binary);
        out.write(reinterpret_cast<const char *>(output.bytes.data()), output.bytes.size());
        out.close();
        require(bool(out), "Complete camera output");
        std::cout << "PASS " << count << " original connected camera-frame cases\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
