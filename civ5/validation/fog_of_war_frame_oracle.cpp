#include "civ5/materials/fog_of_war_frame.hpp"
#include <cstring>
#include <fstream>
#include <iostream>

using namespace civ5::materials;
using namespace civ5::terrain;
using civ5::assets::Bytes;
namespace fs = std::filesystem;
namespace {
struct Reader {
    Bytes bytes;
    std::size_t at = 8;
    template <class T> T read() {
        if (at > bytes.size() || bytes.size() - at < sizeof(T))
            throw std::invalid_argument("Truncated FOW draw input");
        T value;
        std::memcpy(&value, bytes.data() + at, sizeof(T));
        at += sizeof(T);
        return value;
    }
};
void append(Bytes &bytes, const auto &value) {
    const auto p = reinterpret_cast<const std::uint8_t *>(&value);
    bytes.insert(bytes.end(), p, p + sizeof(value));
}
void save(const fs::path &path, const Bytes &bytes) {
    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    if (!file)
        throw std::runtime_error("FOW draw output write failed");
}
} // namespace
int wmain(int argc, wchar_t **argv) {
    try {
        if (argc != 3)
            throw std::invalid_argument("Expected primitive input and fresh output directory");
        const fs::path input(argv[1]), output(argv[2]);
        if (fs::exists(output) || fs::file_size(input) < 144 || fs::file_size(input) > 1048576)
            throw std::invalid_argument("Bounded FOW draw input and fresh output required");
        std::ifstream file(input, std::ios::binary);
        Reader reader{Bytes{std::istreambuf_iterator<char>(file), {}}};
        if (std::memcmp(reader.bytes.data(), "C5FOWDR1", 8) != 0)
            throw std::invalid_argument("FOW draw schema");
        const auto width = reader.read<std::int32_t>(), height = reader.read<std::int32_t>(),
                   mode = reader.read<std::int32_t>();
        const auto precision = reader.read<std::uint32_t>(), quality = reader.read<std::uint32_t>();
        const bool enabled = reader.read<std::uint32_t>() != 0, clear = reader.read<std::uint32_t>() != 0,
                   overrides = reader.read<std::uint32_t>() != 0;
        const auto delta = reader.read<float>();
        const auto estimates = reader.read<std::array<std::uint32_t, 4>>();
        if (precision != 24 && precision != 53)
            throw std::invalid_argument("FOW x87 precision");
        std::size_t estimate_index = 0;
        const auto world = build_world_layout(width, height, mode, {}, [&](float determinant) {
            if (estimate_index >= 2 ||
                std::bit_cast<std::uint32_t>(determinant) != estimates[estimate_index * 2])
                throw std::invalid_argument("Original world reciprocal operand differs");
            return std::bit_cast<float>(estimates[estimate_index++ * 2 + 1]);
        });
        if (estimate_index != 2)
            throw std::invalid_argument("Original world reciprocal count");
        const FogOfWarCamera camera{reader.read<std::array<float, 2>>(), reader.read<std::array<float, 2>>(),
                                    reader.read<Matrix4>()};
        const auto count = reader.read<std::uint32_t>();
        if (count > 4)
            throw std::invalid_argument("FOW view capacity");
        std::vector<FogOfWarView> views;
        for (unsigned i = 0; i < count; ++i)
            views.push_back({reader.read<Matrix4>(), reader.read<Matrix4>(),
                             reader.read<std::array<std::int32_t, 4>>(),
                             reader.read<std::array<float, 4>>()});
        FogOfWarState state(width, height, {}, world.bounds[0], world.bounds[3]);
        for (int x = 0; x < width; ++x)
            for (int y = 0; y < height; ++y) {
                const auto visibility = reader.read<std::uint32_t>();
                if (visibility > 2)
                    throw std::invalid_argument("FOW visibility domain");
                const auto a = x - (y >> 1);
                state.visibility_event({a, y, -a - y}, static_cast<std::uint8_t>(2 - visibility));
            }
        if (reader.at != reader.bytes.size())
            throw std::invalid_argument("FOW trailing primitive bytes");
        state.tick(delta);
        const auto frame =
            fog_of_war_frame(world, state, camera, views, quality, enabled, clear, overrides,
                             precision == 24 ? X87Precision::single : X87Precision::double_precision);
        Bytes constants, commands;
        append(commands, static_cast<std::uint32_t>(frame.passes.size()));
        append(commands, static_cast<std::uint32_t>(frame.draws.size()));
        for (const auto &pass : frame.passes) {
            append(commands, pass.kind);
            append(commands, pass.begin);
            append(commands, pass.end);
            append(commands, pass.clear_color);
        }
        for (const auto &draw : frame.draws) {
            append(commands, draw.geometry);
            append(commands, draw.selector);
            append(commands, static_cast<std::uint32_t>(draw.constants.size()));
            for (const auto &v : draw.constants)
                append(constants, v);
        }
        fs::create_directories(output);
        save(output / "constants.bin", constants);
        save(output / "commands.bin", commands);
        std::cout << "PASS C++ FOW draws: " << frame.passes.size() << " passes, " << frame.draws.size()
                  << " draws\n";
    } catch (const std::exception &error) {
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
}
