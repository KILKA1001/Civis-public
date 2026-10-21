#include "civ5/materials/fog_of_war.hpp"
#include <bit>
#include <cstring>
#include <fstream>
#include <iostream>

using namespace civ5::materials;
using civ5::assets::Bytes;
namespace fs = std::filesystem;
namespace {
template <typename T> T read(std::span<const std::uint8_t> bytes, std::size_t offset) {
    if (offset > bytes.size() || sizeof(T) > bytes.size() - offset)
        throw std::invalid_argument("Truncated FOW primitive input");
    T value;
    std::memcpy(&value, bytes.data() + offset, sizeof(T));
    return value;
}
template <typename T> void append(Bytes &bytes, const T &value) {
    const auto data = std::bit_cast<std::array<std::uint8_t, sizeof(T)>>(value);
    bytes.insert(bytes.end(), data.begin(), data.end());
}
void snapshot(Bytes &bytes, const FogOfWarState &state) {
    append(bytes, std::array<std::uint32_t, 4>{static_cast<std::uint32_t>(state.width()),
                                               static_cast<std::uint32_t>(state.height()), 1,
                                               static_cast<std::uint32_t>(state.dirty())});
    append(bytes, state.elapsed());
    append(bytes, state.normalized_scale());
    append(bytes, state.frame_scale());
    for (const auto *values : {&state.visibility(), &state.visibility_override(), &state.culling()})
        for (auto value : *values)
            append(bytes, value);
}
} // namespace
int wmain(int argc, wchar_t **argv) {
    try {
        if (argc != 3)
            throw std::invalid_argument("Expected primitive input and fresh output directory");
        const fs::path input(argv[1]), output(argv[2]);
        if (fs::exists(output) || fs::file_size(input) < 48 || fs::file_size(input) > 1048576)
            throw std::invalid_argument("Bounded FOW primitive file and fresh output required");
        std::ifstream file(input, std::ios::binary);
        const Bytes bytes{std::istreambuf_iterator<char>(file), {}};
        if (std::memcmp(bytes.data(), "C5FOWST1", 8) != 0)
            throw std::invalid_argument("FOW primitive schema");
        const auto width = read<std::int32_t>(bytes, 8), height = read<std::int32_t>(bytes, 12);
        const auto bounds = read<std::array<float, 4>>(bytes, 16);
        const auto offset = read<std::array<std::int32_t, 3>>(bytes, 32);
        const auto count = read<std::uint32_t>(bytes, 44);
        if (bytes.size() != 48ull + 24ull * count || count > 4096)
            throw std::invalid_argument("FOW primitive operation extent");
        FogOfWarState state(width, height, offset, bounds[0], bounds[2]);
        Bytes result;
        snapshot(result, state);
        for (unsigned i = 0; i < count; ++i) {
            const auto cursor = 48 + std::size_t(i) * 24;
            const auto kind = read<std::uint32_t>(bytes, cursor);
            const auto code = read<std::uint32_t>(bytes, cursor + 4);
            if (kind == 0)
                state.tick(read<float>(bytes, cursor + 20));
            else if (kind == 1 || kind == 2) {
                if (code > 255)
                    throw std::invalid_argument("FOW native visibility type must be a byte");
                if (kind == 1)
                    state.visibility_event(read<std::array<std::int32_t, 3>>(bytes, cursor + 8),
                                           static_cast<std::uint8_t>(code));
                else
                    state.visibility_event_all(static_cast<std::uint8_t>(code));
            } else if (kind == 3)
                state.restart(width, height, offset, bounds[0], bounds[2]);
            else
                throw std::invalid_argument("Unknown FOW operation");
            snapshot(result, state);
        }
        fs::create_directories(output);
        std::ofstream out(output / "states.bin", std::ios::binary);
        out.write(reinterpret_cast<const char *>(result.data()), static_cast<std::streamsize>(result.size()));
        if (!out)
            throw std::runtime_error("FOW snapshot write failed");
        std::cout << "PASS C++ FOW state: " << count + 1 << " snapshots\n";
    } catch (const std::exception &e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
