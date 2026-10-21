#include "civ5/terrain/terrain_decal_updates.hpp"
#include <algorithm>
#include <bit>
#include <fstream>
#include <iostream>
#include <stdexcept>

using namespace civ5::terrain;
using Bytes = civ5::assets::Bytes;
namespace {
std::size_t checks = 0, table_bytes = 0, event_count = 0;
void check(bool ok, const char *message) {
    ++checks;
    if (!ok) throw std::runtime_error(message);
}
template<class F> void rejects(F f) {
    bool caught = false;
    try { f(); } catch (const std::exception &) { caught = true; }
    check(caught, "Invalid decal input accepted");
}
std::uint32_t word(std::istream &in, unsigned size = 4) {
    std::uint32_t value = 0;
    for (unsigned i = 0; i < size; ++i) {
        const auto c = in.get();
        check(c != std::char_traits<char>::eof(), "Truncated decal oracle");
        value |= std::uint32_t(static_cast<unsigned char>(c)) << (8*i);
    }
    return value;
}
Bytes blob(std::istream &in) {
    const auto size = word(in);
    check(size <= 180*94*24, "Oversized decal table");
    Bytes b(size);
    check(bool(in.read(reinterpret_cast<char *>(b.data()), size)), "Truncated decal table");
    return b;
}
std::vector<TerrainDecalCell> cells(std::span<const std::uint8_t> bytes) {
    check(bytes.size()%24 == 0, "Misaligned decal cells");
    std::vector<TerrainDecalCell> result;
    for (std::size_t at = 0; at < bytes.size(); at += 24) {
        const auto get = [&](unsigned offset) {
            std::uint32_t v = 0;
            for (unsigned j = 0; j < 4; ++j) v |= std::uint32_t(bytes[at+offset+j]) << (8*j);
            return v;
        };
        result.push_back({{get(0), get(4), get(8), get(12)}, get(16), bytes[at+20],
                          {bytes[at+21], bytes[at+22], bytes[at+23]}});
    }
    return result;
}
Bytes bytes(std::span<const TerrainDecalCell> cells) {
    Bytes result;
    const auto put = [&](std::uint32_t v) {
        for (unsigned i = 0; i < 4; ++i) result.push_back(static_cast<std::uint8_t>(v >> (8*i)));
    };
    for (const auto &cell : cells) {
        for (auto v : cell.geometry) put(v);
        put(cell.handle);
        result.push_back(cell.dirty);
        result.insert(result.end(), cell.reserved.begin(), cell.reserved.end());
    }
    return result;
}
void guards() {
    TerrainRandom random(1);
    rejects([&] { select_terrain_decal({4, 3}, random); });
    check(random.state() == 1 && random.calls() == 0, "Rejected range consumed randomness");
    check(select_terrain_decal({0xffffffffu, 0xffffffffu}, random) == 0xffffffffu &&
          random.calls() == 0, "Empty high range changed native behavior");
    check(select_terrain_decal({0xfffffffeu, 0xffffffffu}, random) == 0xfffffffeu &&
          random.calls() == 0, "Singleton consumed randomness");
    TerrainRandom reference(1);
    const auto raw = reference.next();
    check(select_terrain_decal({0, 0xffffffffu}, random) == raw &&
          random.state() == reference.state() && random.calls() == 1,
          "Decal selector used the multi-draw shuffled-index routine");
    std::vector<TerrainDecalCell> grid{{{1, 2, 3, 4}, 17, 255, {5, 6, 7}}};
    const auto before = bytes(grid);
    unsigned removed = 0, rebuilt = 0;
    const TerrainDecalRemove remove = [&](std::size_t i, std::uint32_t handle) {
        check(i == 0 && handle == 17 && grid[i].handle == 17 && grid[i].dirty == 255,
              "Removal callback observed the wrong prior state");
        ++removed;
    };
    const TerrainDecalRebuild rebuild = [&](std::size_t, const TerrainDecalCell &) {
        ++rebuilt; return 19;
    };
    rejects([&] { refresh_terrain_decals(-1, 1, grid, remove, rebuild); });
    rejects([&] { refresh_terrain_decals(1, -1, grid, remove, rebuild); });
    rejects([&] { refresh_terrain_decals(1, 2, grid, remove, rebuild); });
    rejects([&] { refresh_terrain_decals(0x7fffffff, 0x7fffffff, grid, remove, rebuild); });
    rejects([&] { refresh_terrain_decals(1, 1, grid, {}, rebuild); });
    rejects([&] { refresh_terrain_decals(1, 1, grid, remove, {}); });
    check(bytes(grid) == before && removed == 0 && rebuilt == 0, "Invalid refresh changed state");
    refresh_terrain_decals(0, 0x7fffffff, {}, remove, rebuild);
    refresh_terrain_decals(0x7fffffff, 0, {}, remove, rebuild);
    check(removed == 0 && rebuilt == 0, "Empty refresh invoked callbacks");
    rejects([&] {
        refresh_terrain_decals(1, 1, grid, [](auto, auto) { throw std::runtime_error("remove"); }, rebuild);
    });
    check(bytes(grid) == before && rebuilt == 0, "Failed removal lost the current handle");
    rejects([&] {
        refresh_terrain_decals(1, 1, grid, remove, [](auto, const auto &cell) -> std::uint32_t {
            check(cell.handle == absent_terrain_decal && cell.dirty == 255, "Rebuild state after removal");
            throw std::runtime_error("build");
        });
    });
    check(grid[0].handle == absent_terrain_decal && grid[0].dirty == 255 && removed == 1,
          "Failed rebuild lost pending state");
    refresh_terrain_decals(1, 1, grid, remove, rebuild);
    check(grid[0].handle == 19 && grid[0].dirty == 0 && removed == 1 && rebuilt == 1,
          "Retry removed an already absent handle");
    refresh_terrain_decals(1, 1, grid, remove, rebuild);
    check(removed == 1 && rebuilt == 1, "Clean cell was refreshed");
}
void oracle(const char *path) {
    std::ifstream in(path, std::ios::binary);
    char magic[8];
    check(bool(in.read(magic, 8)) && std::string_view(magic, 8) == "C5DECL01", "Invalid decal oracle");
    const auto selections = word(in);
    check(selections > 0 && selections <= 1000, "Invalid selection case count");
    for (unsigned n = 0; n < selections; ++n) {
        const TerrainDecalRange range{word(in), word(in)};
        TerrainRandom random(word(in));
        const auto family = word(in), repeats = word(in);
        check(family <= 16 && repeats <= 100, "Invalid selection controls");
        for (unsigned i = 0; i < repeats; ++i) {
            const auto selected = select_terrain_decal(range, random);
            check(selected == word(in), "Original selected decal index differs");
            check(random.state() == word(in), "Original selection random state differs");
            check(random.calls() == word(in), "Original selection random consumption differs");
        }
    }
    const auto refreshes = word(in);
    check(refreshes > 0 && refreshes <= 100, "Invalid refresh case count");
    for (unsigned n = 0; n < refreshes; ++n) {
        const auto width = word(in), height = word(in);
        check(width <= 180 && height <= 94, "Invalid refresh dimensions");
        const TerrainDecalRange range{word(in), word(in)};
        TerrainRandom random(word(in));
        auto grid = cells(blob(in));
        check(grid.size() == std::size_t(width)*height, "Refresh table extent differs");
        const auto expected = blob(in);
        const auto final_state = word(in), calls = word(in), events = word(in);
        check(events <= grid.size()*3, "Too many decal callback events");
        std::vector<std::array<std::uint32_t, 6>> actual;
        const auto event = [&](std::uint32_t kind, std::size_t index, std::uint32_t value) {
            actual.push_back({kind, static_cast<std::uint32_t>(index), value, grid[index].dirty,
                              random.state(), static_cast<std::uint32_t>(random.calls())});
        };
        const TerrainDecalRemove remove = [&](std::size_t index, std::uint32_t handle) {
            event(0, index, handle);
        };
        const TerrainDecalRebuild rebuild = [&](std::size_t index, const TerrainDecalCell &cell) {
            event(1, index, cell.handle);
            const auto selected = select_terrain_decal(range, random);
            event(2, index, selected);
            return index%7 == 0 ? absent_terrain_decal :
                0x60000000u | (static_cast<std::uint32_t>(index) << 8) | (selected & 255u);
        };
        refresh_terrain_decals(static_cast<std::int32_t>(width), static_cast<std::int32_t>(height),
                               grid, remove, rebuild);
        check(bytes(grid) == expected, "Original complete decal table differs");
        table_bytes += expected.size();
        check(random.state() == final_state && random.calls() == calls, "Original refresh cursor differs");
        check(actual.size() == events, "Original callback count differs");
        for (const auto &e : actual)
            for (const auto value : e) check(value == word(in), "Original callback order/state differs");
        event_count += actual.size();
        refresh_terrain_decals(static_cast<std::int32_t>(width), static_cast<std::int32_t>(height),
                               grid, remove, rebuild);
        check(bytes(grid) == expected && actual.size() == events && random.state() == final_state &&
              random.calls() == calls, "Second refresh was not inert");
    }
    check(in.peek() == std::char_traits<char>::eof(), "Trailing decal oracle bytes");
    std::cout << selections << " selection cases, " << refreshes << " refresh cases, ";
}
} // namespace
int main(int argc, char **argv) {
    try {
        if (argc > 2) throw std::runtime_error("Usage: civ5_terrain_decal_updates_tests [oracle.bin]");
        guards();
        if (argc == 2) oracle(argv[1]);
        std::cout << "PASS decal updates: " << checks << " checks, " << table_bytes
                  << " table bytes, " << event_count << " ordered callback events\n";
    } catch (const std::exception &e) {
        std::cerr << "FAIL decal updates: " << e.what() << '\n'; return 1;
    }
}
