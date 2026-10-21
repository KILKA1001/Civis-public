#include "civ5/terrain/terrain_auxiliary_owners.hpp"
#include "civ5/terrain/height_owner.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string_view>

using namespace civ5::terrain;
namespace {
std::size_t checks = 0, table_bytes = 0;
void check(bool value, const char *message) {
    ++checks;
    if (!value) throw std::runtime_error(message);
}
template <class F> void rejects(F f) {
    bool rejected = false;
    try { f(); } catch (const std::exception &) { rejected = true; }
    check(rejected, "Invalid auxiliary owner input was accepted");
}
std::uint32_t word(std::istream &in, unsigned bytes = 4) {
    std::uint32_t result = 0;
    for (unsigned i = 0; i < bytes; ++i) {
        const auto b = in.get();
        if (b < 0) throw std::runtime_error("Truncated auxiliary owner oracle");
        result |= std::uint32_t(b) << (i*8);
    }
    return result;
}
template <std::size_t N> std::array<std::uint8_t, N> fixed_bytes(std::istream &in) {
    std::array<std::uint8_t, N> result{};
    for (auto &byte : result) byte = static_cast<std::uint8_t>(word(in, 1));
    return result;
}
std::int32_t integer(std::istream &in) { return std::bit_cast<std::int32_t>(word(in)); }
WorldGrid grid(std::istream &in) {
    WorldGrid result;
    for (auto &v : result.first_matrix) v = integer(in);
    for (auto &v : result.second_matrix) v = integer(in);
    for (auto &v : result.descriptor) v = integer(in);
    result.width = integer(in); result.height = integer(in);
    return result;
}
bool same(const WorldGrid &a, const WorldGrid &b) {
    return a.width == b.width && a.height == b.height && a.descriptor == b.descriptor &&
        a.first_matrix == b.first_matrix && a.second_matrix == b.second_matrix;
}
std::vector<std::uint8_t> prior_bytes(std::size_t count, unsigned seed) {
    std::vector<std::uint8_t> result(count);
    for (std::size_t i = 0; i < count; ++i) result[i] = static_cast<std::uint8_t>(i*37+(i >> 8)*19+seed);
    return result;
}
template <class Grid> void storage(Grid &g, unsigned seed) {
    const auto raw = prior_bytes(Grid::capacity*Grid::record_bytes+g.fallback.size(), seed);
    g.cells.resize(Grid::capacity);
    for (std::size_t i = 0; i < Grid::capacity; ++i)
        for (std::size_t b = 0; b < Grid::record_bytes; ++b) g.cells[i][b] = raw[i*Grid::record_bytes+b];
    for (std::size_t b = 0; b < g.fallback.size(); ++b) g.fallback[b] = raw[Grid::capacity*Grid::record_bytes+b];
}
template <class Grid> void input(std::istream &in, Grid &g, unsigned seed) {
    g.coordinates = grid(in); g.wrap_x = word(in) != 0; storage(g, seed);
}
template <class Grid> std::vector<std::uint8_t> bytes(const Grid &g) {
    std::vector<std::uint8_t> result;
    result.reserve(Grid::capacity*Grid::record_bytes+g.fallback.size());
    for (auto cell : g.cells) result.insert(result.end(), cell.begin(), cell.end());
    result.insert(result.end(), g.fallback.begin(), g.fallback.end());
    return result;
}
template <class Grid> void compare(std::istream &in, const Grid &g, unsigned seed, unsigned test_case, const char *name) {
    const auto actual = bytes(g);
    auto expected = prior_bytes(actual.size(), seed);
    const auto changes = word(in);
    check(changes <= expected.size(), "Invalid native change count");
    std::uint32_t previous = 0;
    for (unsigned i = 0; i < changes; ++i) {
        const auto index = word(in), value = word(in, 1);
        check(index < expected.size() && (i == 0 || index > previous), "Invalid native change address");
        expected[index] = static_cast<std::uint8_t>(value); previous = index;
    }
    for (std::size_t i = 0; i < actual.size(); ++i)
        if (actual[i] != expected[i])
            throw std::runtime_error("Original auxiliary owner case " + std::to_string(test_case) + " " + name +
                " byte " + std::to_string(i) + ": expected " + std::to_string(expected[i]) +
                ", actual " + std::to_string(actual[i]));
    ++checks;
    table_bytes += actual.size();
}
using MeshRuns = std::vector<std::vector<std::pair<std::uint8_t, std::uint32_t>>>;
void mesh_flags(const TerrainIceContours &contours, std::uint32_t lattice,
                const MeshRuns &expected) {
    const auto plan = plan_terrain_ice_mesh_flags(contours, lattice);
    check(plan.segments.size() == expected.size(), "Original ice mesh segment count differs");
    std::size_t cursor = 0;
    for (std::size_t i = 0; i < expected.size(); ++i) {
        const auto first = cursor;
        for (const auto [value, count] : expected[i]) {
            for (std::uint32_t j = 0; j < count; ++j) {
                check(cursor < plan.flags.size() && plan.flags[cursor] == value,
                      "Original ice mesh vertex flag or order differs");
                ++cursor;
            }
        }
        check(plan.segments[i].first_vertex == first &&
                  plan.segments[i].vertex_count == cursor - first,
              "Original ice mesh vertex segment span differs");
    }
    check(plan.flags.size() == cursor, "Original ice mesh has trailing vertex flags");
}
void ice_curve_settings_fixture(const char *path, std::uint32_t lattice) {
    std::ifstream in(path, std::ios::binary);
    char magic[8], exe_hash[32], grid_hash[32];
    const auto hex = [](const char *bytes) {
        std::string result;
        static constexpr char digits[] = "0123456789abcdef";
        for (unsigned i = 0; i < 32; ++i) {
            const auto byte = static_cast<std::uint8_t>(bytes[i]);
            result.push_back(digits[byte >> 4]);
            result.push_back(digits[byte & 15]);
        }
        return result;
    };
    check(bool(in.read(magic, 8)) && std::string_view(magic, 8) == "C5ICST01" &&
              bool(in.read(exe_hash, 32)) && bool(in.read(grid_hash, 32)) &&
              word(in) == lattice,
          "Invalid original ice curve settings fixture");
    check(hex(exe_hash) == "7c9f93d512c49a7ab2bc70f51c6caa10a82ddd697ac11d8a24ff1bb7ce7c3686" &&
              hex(grid_hash) == "b2146b5a3f134ea07dd5ea4e83dfcb6f8a5cdf5c7f70bf3cb9a74bd164dae015",
          "Original ice curve fixture source identity differs");
    TerrainIceProjection projection;
    projection.world_matrix = {
        63.998600006103516f, 0.0009384156437590718f, 0, 0,
        32, 64.00188446044922f, 0, 0,
        0, 0, 0, 0,
        32.00070571899414f, 128.0032958984375f, 0, 1};
    projection.scale_x = 1.0f;
    projection.scale_y = 0.8660254f;
    const auto world = build_world_grid_layout(40, 24, 1, {}).source_grid;
    const auto curve = build_terrain_ice_curve_parameters(world, projection, lattice);
    const auto actual = std::bit_cast<std::array<std::uint8_t, 152>>(curve);
    for (const auto byte : actual)
        check(word(in, 1) == byte, "Original ice curve parameter byte differs");
    check(in.peek() == std::char_traits<char>::eof(),
          "Trailing original ice curve setting bytes");
}
TerrainIceProjection native_ice_projection() {
    TerrainIceProjection projection;
    projection.world_matrix = {
        63.998600006103516f, 0.0009384156437590718f, 0, 0,
        32, 64.00188446044922f, 0, 0,
        0, 0, 0, 0,
        32.00070571899414f, 128.0032958984375f, 0, 1};
    projection.scale_y = 0.8660254f;
    return projection;
}
TerrainIceProjection selected_save_ice_projection() {
    // A 40x24 mode-1 source uses a 40x28 worker grid of 64-unit tiles.
    const auto world = build_world_layout(40, 24, 1, {},
        [](float) { return std::bit_cast<float>(0x45800001u); });
    const auto owner = HeightOwnerParameters::from_world(world);
    const auto projection = terrain_ice_projection(world, owner);
    check(owner.width == 40 * 64 && owner.height == 28 * 64 &&
              projection.world_matrix == native_ice_projection().world_matrix,
          "Selected-save ice world matrix or extent differs");
    check(std::bit_cast<std::uint32_t>(projection.scale_y) == 0x3f5db22du &&
              std::bit_cast<std::uint32_t>(projection.center_x) == 0x44a00000u &&
              std::bit_cast<std::uint32_t>(projection.center_y) == 0x4441fbe7u,
          "Selected-save ice owner projection differs");
    return projection;
}
TerrainIceContourEdge control_edge(std::istream &in) {
    TerrainIceContourEdge edge{{integer(in), integer(in), integer(in)},
                               static_cast<std::uint8_t>(word(in, 1))};
    in.ignore(3);
    check(in.gcount() == 3 && edge.side < 6,
          "Invalid original ice control-point edge");
    return edge;
}
TerrainIceRandomState control_random(std::istream &in) {
    TerrainIceRandomState result;
    result.value = word(in); result.draws = word(in);
    result.initial_value = word(in); result.modulus = word(in);
    const std::uint64_t low = word(in), high = word(in);
    result.inverse_modulus = std::bit_cast<double>(low | (high << 32));
    return result;
}
void ice_control_fixture(const char *path, std::uint32_t expected_count,
                         std::string_view expected_input_sha,
                         std::uint32_t expected_precision) {
    std::ifstream in(path, std::ios::binary);
    char magic[8], exe_hash[32], input_hash[32], settings_hash[32];
    check(bool(in.read(magic, 8)) && std::string_view(magic, 8) == "C5ICCP03" &&
              bool(in.read(exe_hash, 32)) && bool(in.read(input_hash, 32)) &&
              bool(in.read(settings_hash, 32)),
          "Invalid original ice control-point fixture header");
    const auto hex = [](const char *bytes) {
        std::string result;
        static constexpr char digits[] = "0123456789abcdef";
        for (unsigned i = 0; i < 32; ++i) {
            const auto byte = static_cast<std::uint8_t>(bytes[i]);
            result.push_back(digits[byte >> 4]);
            result.push_back(digits[byte & 15]);
        }
        return result;
    };
    check(hex(exe_hash) == "7c9f93d512c49a7ab2bc70f51c6caa10a82ddd697ac11d8a24ff1bb7ce7c3686" &&
              hex(input_hash) == expected_input_sha &&
              hex(settings_hash) == "c658114c59fd8099f897ac2e174bb0534927d5716294772dcda3771375728774" &&
              word(in) == 12 && word(in) == 12345 && word(in) == expected_count &&
              word(in) == expected_precision,
          "Original ice control-point fixture identity differs");
    const auto projection = native_ice_projection();
    const auto world = build_world_grid_layout(40, 24, 1, {}).source_grid;
    const auto curve = build_terrain_ice_curve_parameters(world, projection, 12);
    for (std::uint32_t record = 0; record < expected_count; ++record) {
        const auto previous = control_edge(in), center = control_edge(in),
                   following = control_edge(in);
        auto random = control_random(in);
        std::array<std::uint32_t, 4> expected_base{}, expected{};
        for (auto &value : expected_base) value = word(in);
        for (auto &value : expected) value = word(in);
        const auto expected_random = control_random(in);
        const auto actual = build_terrain_ice_control_point(previous, center,
                                                            following, curve, projection, random,
                                                            expected_precision);
        for (std::size_t i = 0; i < 4; ++i) {
            const auto bits = std::bit_cast<std::uint32_t>(actual.base[i]);
            if (bits != expected_base[i])
                throw std::runtime_error("Original ice control-point record " +
                    std::to_string(record) + " base float " + std::to_string(i) +
                    ": expected bits " + std::to_string(expected_base[i]) +
                    ", actual " + std::to_string(bits));
            ++checks;
        }
        for (std::size_t i = 0; i < 4; ++i) {
            const auto bits = std::bit_cast<std::uint32_t>(actual.displaced[i]);
            if (bits != expected[i])
                throw std::runtime_error("Original ice control-point record " +
                    std::to_string(record) + " float " + std::to_string(i) +
                    ": expected bits " + std::to_string(expected[i]) +
                    ", actual " + std::to_string(bits));
            ++checks;
        }
        check(random.value == expected_random.value &&
                  random.draws == expected_random.draws &&
                  random.initial_value == expected_random.initial_value &&
                  random.modulus == expected_random.modulus &&
                  std::bit_cast<std::uint64_t>(random.inverse_modulus) ==
                      std::bit_cast<std::uint64_t>(expected_random.inverse_modulus),
              "Original ice control-point random state differs");
    }
    check(in.peek() == std::char_traits<char>::eof(),
          "Trailing original ice control-point fixture bytes");
}
void ice_spline_fixture(const char *path, std::uint32_t expected_count,
                        std::string_view expected_input_sha,
                        std::uint32_t expected_precision) {
    std::ifstream in(path, std::ios::binary);
    char magic[8], exe_hash[32], input_hash[32], settings_hash[32];
    check(bool(in.read(magic, 8)) && std::string_view(magic, 8) == "C5ISPL02" &&
              bool(in.read(exe_hash, 32)) && bool(in.read(input_hash, 32)) &&
              bool(in.read(settings_hash, 32)),
          "Invalid original ice spline fixture header");
    const auto hex = [](const char *bytes) {
        std::string result;
        static constexpr char digits[] = "0123456789abcdef";
        for (unsigned i = 0; i < 32; ++i) {
            const auto byte = static_cast<std::uint8_t>(bytes[i]);
            result.push_back(digits[byte >> 4]);
            result.push_back(digits[byte & 15]);
        }
        return result;
    };
    check(hex(exe_hash) == "7c9f93d512c49a7ab2bc70f51c6caa10a82ddd697ac11d8a24ff1bb7ce7c3686" &&
              hex(input_hash) == expected_input_sha &&
              hex(settings_hash) == "c658114c59fd8099f897ac2e174bb0534927d5716294772dcda3771375728774" &&
              word(in) == 12 && word(in) == 12345 && word(in) == expected_count &&
              word(in) == expected_precision,
          "Original ice spline fixture identity differs");
    for (std::uint32_t record = 0; record < expected_count; ++record) {
        const auto count = word(in), t_bits = word(in), boundary = word(in);
        check(count >= 3 && boundary == 0 &&
                  std::isfinite(std::bit_cast<float>(t_bits)),
              "Original ice spline call arguments differ");
        const auto expected_first = word(in);
        std::array<float, 6> points{};
        for (auto &point : points) point = std::bit_cast<float>(word(in));
        const auto local_t = std::bit_cast<float>(word(in));
        const auto expected_x = word(in), expected_y = word(in);
        const auto selection = select_terrain_ice_spline_triplet(
            count, std::bit_cast<float>(t_bits), static_cast<std::int32_t>(boundary));
        check(selection.first_point == expected_first &&
                  std::bit_cast<std::uint32_t>(selection.local_t) ==
                      std::bit_cast<std::uint32_t>(local_t),
              "Original ice spline selection or local parameter differs");
        const auto actual = interpolate_terrain_ice_spline_triplet(points, local_t);
        if (std::bit_cast<std::uint32_t>(actual[0]) != expected_x ||
            std::bit_cast<std::uint32_t>(actual[1]) != expected_y)
            throw std::runtime_error("Original ice spline record " +
                std::to_string(record) + " differs: expected " +
                std::to_string(expected_x) + "," + std::to_string(expected_y) +
                ", actual " + std::to_string(std::bit_cast<std::uint32_t>(actual[0])) +
                "," + std::to_string(std::bit_cast<std::uint32_t>(actual[1])));
        checks += 2;
    }
    check(in.peek() == std::char_traits<char>::eof(),
          "Trailing original ice spline fixture bytes");
}
TerrainIceContours read_ice_contour_geometry(const char *path) {
    std::ifstream in(path, std::ios::binary);
    char magic[8], identity[32];
    check(bool(in.read(magic, 8)) && std::string_view(magic, 8) == "C5ICCT01" &&
              bool(in.read(identity, 32)), "Invalid ice contour geometry fixture");
    const auto mode = word(in), groups = word(in), vertices = word(in),
               segments = word(in), edges = word(in);
    check(mode == 1 && groups > 0 && groups <= vertices &&
              vertices > 0 && segments > 0 && segments <= edges && edges <= vertices * 6,
          "Invalid ice contour geometry extents");
    TerrainIceContours result;
    for (std::uint32_t i = 0; i < groups; ++i)
        result.groups.push_back({word(in), word(in), word(in), word(in)});
    for (std::uint32_t i = 0; i < vertices; ++i)
        result.vertices.push_back({integer(in), integer(in), integer(in)});
    for (std::uint32_t i = 0; i < vertices; ++i)
        result.boundary_flags.push_back(static_cast<std::uint8_t>(word(in, 1)));
    result.grid.cells.resize(TerrainIceGrid::capacity);
    result.grid = initialize_terrain_ice_grid(result.grid, 40, 24, {});
    result.grid.wrap_x = true;
    for (std::uint32_t i = 0; i < segments; ++i)
        result.segments.push_back({word(in), word(in)});
    for (std::uint32_t i = 0; i < edges; ++i)
        result.edges.push_back(control_edge(in));
    for (std::int32_t y = 0; y < 24; ++y)
        for (std::int32_t x = 0; x < 40; ++x)
            result.grid.cells[y * TerrainIceGrid::stride + x][0] =
                static_cast<std::uint8_t>(word(in, 1));
    check(in.peek() == std::char_traits<char>::eof(),
          "Trailing ice contour geometry fixture bytes");
    return result;
}
void ice_adjacency_fixture(const char *path) {
    std::ifstream in(path, std::ios::binary);
    char magic[8];
    check(bool(in.read(magic, 8)) && std::string_view(magic, 8) == "C5IADJ01",
          "Invalid original ice adjacency fixture");
    const auto exe_hash = fixed_bytes<32>(in), parent_hash = fixed_bytes<32>(in);
    const auto hex = [](const auto &bytes) {
        std::string result;
        static constexpr char digits[] = "0123456789abcdef";
        for (const auto byte : bytes) {
            result.push_back(digits[byte >> 4]);
            result.push_back(digits[byte & 15]);
        }
        return result;
    };
    check(hex(exe_hash) == "7c9f93d512c49a7ab2bc70f51c6caa10a82ddd697ac11d8a24ff1bb7ce7c3686" &&
              hex(parent_hash) == "aa59efeb85d6d7661ad0e5f4da733422cd174820442ee7cb055306a7be6091fa" &&
              word(in) == 6, "Original ice adjacency source identity differs");
    const auto read_blob = [&](std::size_t size) {
        std::vector<std::uint8_t> raw(size);
        check(bool(in.read(reinterpret_cast<char *>(raw.data()),
                           static_cast<std::streamsize>(size))),
              "Truncated original ice adjacency fixture");
        return raw;
    };
    const auto load32 = [](const std::vector<std::uint8_t> &raw, std::size_t offset) {
        std::uint32_t value = 0;
        for (unsigned i = 0; i < 4; ++i)
            value |= std::uint32_t(raw[offset + i]) << (8 * i);
        return value;
    };
    const auto write32 = [](std::vector<std::uint8_t> &raw,
                            std::vector<std::uint8_t> &mask,
                            std::size_t offset, std::uint32_t value) {
        for (unsigned i = 0; i < 4; ++i) {
            raw[offset + i] = static_cast<std::uint8_t>(value >> (8 * i));
            mask[offset + i] = 1;
        }
    };
    for (unsigned test_case = 0; test_case < 6; ++test_case) {
        const auto n20 = word(in), n12 = word(in);
        check(n20 <= 100 && n12 <= 100, "Invalid original ice adjacency extent");
        const auto input20 = read_blob(20 * n20), input12 = read_blob(12 * n12);
        const auto original20 = read_blob(20 * n20), original_mask20 = read_blob(20 * n20);
        const auto original_adj = read_blob(8 * n12), original_mask_adj = read_blob(8 * n12);
        TerrainIceIntermediateFill fill;
        fill.points.resize(n20);
        fill.links.resize(n12);
        for (std::size_t i = 0; i < n20; ++i) {
            fill.points[i].xy = {std::bit_cast<float>(load32(input20, 20 * i)),
                                 std::bit_cast<float>(load32(input20, 20 * i + 4))};
            fill.points[i].link_count = load32(input20, 20 * i + 16);
        }
        for (std::size_t i = 0; i < n12; ++i)
            fill.links[i] = {load32(input12, 12 * i), load32(input12, 12 * i + 4),
                             input12[12 * i + 8]};
        build_terrain_ice_intermediate_adjacency(fill);
        check(fill.adjacency_link_indices.size() == 2 * n12,
              "Original ice adjacency list extent differs");
        auto actual20 = input20;
        std::vector<std::uint8_t> mask20(20 * n20);
        for (std::size_t i = 0; i < n20; ++i) {
            write32(actual20, mask20, 20 * i + 8,
                    0x20900000u + 4 * fill.points[i].adjacency_first);
            if (fill.points[i].link_count)
                write32(actual20, mask20, 20 * i + 16, fill.points[i].link_count);
        }
        std::vector<std::uint8_t> actual_adj(8 * n12, 0xcd), mask_adj(8 * n12);
        for (std::size_t i = 0; i < fill.adjacency_link_indices.size(); ++i)
            write32(actual_adj, mask_adj, 4 * i,
                    0x20400000u + 12 * fill.adjacency_link_indices[i]);
        check(actual20 == original20 && mask20 == original_mask20,
              "Original ice adjacency point pointer or degree differs");
        check(actual_adj == original_adj && mask_adj == original_mask_adj,
              "Original ice adjacency link order differs");
    }
    check(in.peek() == std::char_traits<char>::eof(),
          "Trailing original ice adjacency fixture bytes");
}
void ice_adjacency_sort_fixture(const char *path) {
    std::ifstream in(path, std::ios::binary);
    char magic[8];
    check(bool(in.read(magic, 8)) && std::string_view(magic, 8) == "C5ISRT01",
          "Invalid original ice adjacency sort fixture");
    const auto exe_hash = fixed_bytes<32>(in), parent_hash = fixed_bytes<32>(in);
    const auto hex = [](const auto &bytes) {
        std::string result;
        static constexpr char digits[] = "0123456789abcdef";
        for (const auto byte : bytes) {
            result.push_back(digits[byte >> 4]);
            result.push_back(digits[byte & 15]);
        }
        return result;
    };
    check(hex(exe_hash) == "7c9f93d512c49a7ab2bc70f51c6caa10a82ddd697ac11d8a24ff1bb7ce7c3686" &&
              hex(parent_hash) == "aa59efeb85d6d7661ad0e5f4da733422cd174820442ee7cb055306a7be6091fa" &&
              word(in) == 26, "Original ice adjacency sort source identity differs");
    const auto read_blob = [&](std::size_t size) {
        std::vector<std::uint8_t> raw(size);
        check(bool(in.read(reinterpret_cast<char *>(raw.data()),
                           static_cast<std::streamsize>(size))),
              "Truncated original ice adjacency sort fixture");
        return raw;
    };
    const auto load32 = [](const std::vector<std::uint8_t> &raw, std::size_t offset) {
        std::uint32_t value = 0;
        for (unsigned i = 0; i < 4; ++i)
            value |= std::uint32_t(raw[offset + i]) << (8 * i);
        return value;
    };
    for (unsigned test_case = 0; test_case < 26; ++test_case) {
        const auto name_size = word(in), n20 = word(in), n12 = word(in);
        check(name_size <= 32 && n20 <= 64 && n12 <= 64,
              "Invalid original ice adjacency sort extent");
        const auto name = read_blob(name_size);
        const auto precision = word(in);
        check(!name.empty() && (precision == 24 || precision == 53),
              "Invalid original ice adjacency sort case identity");
        const auto input20 = read_blob(20 * n20), input12 = read_blob(12 * n12);
        const auto before = read_blob(8 * n12), original = read_blob(8 * n12);
        const auto mask = read_blob(8 * n12);
        TerrainIceIntermediateFill fill;
        fill.points.resize(n20);
        fill.links.resize(n12);
        for (std::size_t i = 0; i < n20; ++i)
            fill.points[i].xy = {std::bit_cast<float>(load32(input20, 20 * i)),
                                 std::bit_cast<float>(load32(input20, 20 * i + 4))};
        for (std::size_t i = 0; i < n12; ++i)
            fill.links[i] = {load32(input12, 12 * i), load32(input12, 12 * i + 4),
                             input12[12 * i + 8]};
        const auto pointers = [&]() {
            std::vector<std::uint8_t> bytes(8 * n12);
            for (std::size_t i = 0; i < fill.adjacency_link_indices.size(); ++i) {
                const auto value = 0x20400000u + 12 * fill.adjacency_link_indices[i];
                for (unsigned byte = 0; byte < 4; ++byte)
                    bytes[4 * i + byte] = static_cast<std::uint8_t>(value >> (8 * byte));
            }
            return bytes;
        };
        build_terrain_ice_intermediate_adjacency(fill);
        check(pointers() == before, "Original ice adjacency before sort differs");
        sort_terrain_ice_intermediate_adjacency(fill);
        check(pointers() == original, "Original ice adjacency angular order differs");
        for (std::size_t i = 0; i < mask.size(); ++i)
            check(mask[i] <= 1 && (before[i] == original[i] || mask[i]),
                  "Original ice adjacency sort write mask is inconsistent");
    }
    check(in.peek() == std::char_traits<char>::eof(),
          "Trailing original ice adjacency sort fixture bytes");
}
void ice_closed_walk_fixture(const char *path) {
    std::ifstream in(path, std::ios::binary);
    char magic[8];
    check(bool(in.read(magic, 8)) && std::string_view(magic, 8) == "C5IWAL01",
          "Invalid original ice closed-walk fixture");
    const auto exe_hash = fixed_bytes<32>(in), function_hash = fixed_bytes<32>(in);
    const auto parent_hash = fixed_bytes<32>(in);
    const auto hex = [](const auto &bytes) {
        std::string result;
        static constexpr char digits[] = "0123456789abcdef";
        for (const auto byte : bytes) {
            result.push_back(digits[byte >> 4]);
            result.push_back(digits[byte & 15]);
        }
        return result;
    };
    check(hex(exe_hash) == "7c9f93d512c49a7ab2bc70f51c6caa10a82ddd697ac11d8a24ff1bb7ce7c3686" &&
              hex(function_hash) == "6efdd67c048a9f4391b2175dab70f0d4c47aea15869a531a8074861fcc3c4b88" &&
              hex(parent_hash) == "aa59efeb85d6d7661ad0e5f4da733422cd174820442ee7cb055306a7be6091fa" &&
              word(in) == 20, "Original ice closed-walk source identity differs");
    const auto read_blob = [&](std::size_t size) {
        std::vector<std::uint8_t> raw(size);
        check(bool(in.read(reinterpret_cast<char *>(raw.data()),
                           static_cast<std::streamsize>(size))),
              "Truncated original ice closed-walk fixture");
        return raw;
    };
    const auto load32 = [](const std::vector<std::uint8_t> &raw, std::size_t offset) {
        std::uint32_t value = 0;
        for (unsigned i = 0; i < 4; ++i)
            value |= std::uint32_t(raw[offset + i]) << (8 * i);
        return value;
    };
    for (unsigned test_case = 0; test_case < 20; ++test_case) {
        const auto name_size = word(in), precision = word(in);
        const auto n20 = word(in), n12 = word(in);
        check(name_size <= 32 && (precision == 24 || precision == 53) &&
                  n20 <= 64 && n12 <= 64,
              "Invalid original ice closed-walk extent");
        const auto name = read_blob(name_size);
        check(!name.empty(), "Invalid original ice closed-walk case name");
        const auto input20 = read_blob(20 * n20), input12 = read_blob(12 * n12);
        const auto original_adj = read_blob(8 * n12);
        const auto original_flags = read_blob(2 * n12), write_mask = read_blob(2 * n12);
        TerrainIceIntermediateFill fill;
        fill.points.resize(n20);
        fill.links.resize(n12);
        for (std::size_t i = 0; i < n20; ++i)
            fill.points[i].xy = {std::bit_cast<float>(load32(input20, 20 * i)),
                                 std::bit_cast<float>(load32(input20, 20 * i + 4))};
        for (std::size_t i = 0; i < n12; ++i)
            fill.links[i] = {load32(input12, 12 * i), load32(input12, 12 * i + 4),
                             input12[12 * i + 8]};
        build_terrain_ice_intermediate_adjacency(fill);
        sort_terrain_ice_intermediate_adjacency(fill);
        std::vector<std::uint8_t> actual_adj(8 * n12);
        for (std::size_t i = 0; i < fill.adjacency_link_indices.size(); ++i) {
            const auto value = 0x20400000u + 12 * fill.adjacency_link_indices[i];
            for (unsigned byte = 0; byte < 4; ++byte)
                actual_adj[4 * i + byte] = static_cast<std::uint8_t>(value >> (8 * byte));
        }
        check(actual_adj == original_adj, "Original closed-walk sorted adjacency differs");
        mark_terrain_ice_closed_walks(fill);
        check(fill.closed_walk_flags == original_flags,
              "Original ice closed-walk flags differ");
        check(original_flags == write_mask,
              "Original closed-walk flags and writes disagree");
    }
    check(in.peek() == std::char_traits<char>::eof(),
          "Trailing original ice closed-walk fixture bytes");
}
void ice_region_walk_fixture(const char *path) {
    std::ifstream in(path, std::ios::binary);
    char magic[8];
    check(bool(in.read(magic, 8)) && std::string_view(magic, 8) == "C5IRWK01",
          "Invalid original ice region-walk fixture");
    const auto exe_hash = fixed_bytes<32>(in), function_hash = fixed_bytes<32>(in);
    const auto parent_hash = fixed_bytes<32>(in);
    const auto hex = [](const auto &bytes) {
        std::string result;
        static constexpr char digits[] = "0123456789abcdef";
        for (const auto byte : bytes) {
            result.push_back(digits[byte >> 4]);
            result.push_back(digits[byte & 15]);
        }
        return result;
    };
    check(hex(exe_hash) == "7c9f93d512c49a7ab2bc70f51c6caa10a82ddd697ac11d8a24ff1bb7ce7c3686" &&
              hex(function_hash) == "9ec4a25ce54ad3ebb19ad7c5a03e6d960ba4919b6b4ba38d1a4142277ea45eb3" &&
              hex(parent_hash) == "a944d3ab7958f446c72876c667f707dc54ce181e30858714c43988be9ee1c4b7" &&
              word(in) == 22, "Original ice region-walk source identity differs");
    const auto read_blob = [&](std::size_t size) {
        std::vector<std::uint8_t> raw(size);
        check(bool(in.read(reinterpret_cast<char *>(raw.data()),
                           static_cast<std::streamsize>(size))),
              "Truncated original ice region-walk fixture");
        return raw;
    };
    const auto load32 = [](const std::vector<std::uint8_t> &raw, std::size_t offset) {
        std::uint32_t value = 0;
        for (unsigned i = 0; i < 4; ++i)
            value |= std::uint32_t(raw[offset + i]) << (8 * i);
        return value;
    };
    for (unsigned test_case = 0; test_case < 22; ++test_case) {
        const auto name_size = word(in), precision = word(in);
        const auto n20 = word(in), n12 = word(in), region_count = word(in);
        check(name_size <= 32 && (precision == 24 || precision == 53) &&
                  n20 <= 64 && n12 <= 64 && region_count <= 16,
              "Invalid original ice region-walk extent");
        const auto name = read_blob(name_size);
        check(!name.empty(), "Invalid original ice region-walk case name");
        const auto input20 = read_blob(20 * n20), input12 = read_blob(12 * n12);
        const auto original_adj = read_blob(8 * n12), initial_flags = read_blob(2 * n12);
        const auto output_flags = read_blob(2 * n12), write_mask = read_blob(2 * n12);
        std::vector<std::vector<std::array<std::uint32_t, 2>>> expected_regions;
        for (std::size_t region = 0; region < region_count; ++region) {
            const auto pair_count = word(in);
            check(pair_count <= 128, "Invalid original ice region-walk pair extent");
            auto &pairs = expected_regions.emplace_back();
            for (std::size_t pair = 0; pair < pair_count; ++pair)
                pairs.push_back({word(in), word(in)});
        }
        TerrainIceIntermediateFill fill;
        fill.points.resize(n20);
        fill.links.resize(n12);
        for (std::size_t i = 0; i < n20; ++i)
            fill.points[i].xy = {std::bit_cast<float>(load32(input20, 20 * i)),
                                 std::bit_cast<float>(load32(input20, 20 * i + 4))};
        for (std::size_t i = 0; i < n12; ++i)
            fill.links[i] = {load32(input12, 12 * i), load32(input12, 12 * i + 4),
                             input12[12 * i + 8]};
        build_terrain_ice_intermediate_adjacency(fill);
        sort_terrain_ice_intermediate_adjacency(fill);
        std::vector<std::uint8_t> actual_adj(8 * n12);
        for (std::size_t i = 0; i < fill.adjacency_link_indices.size(); ++i) {
            const auto value = 0x20400000u + 12 * fill.adjacency_link_indices[i];
            for (unsigned byte = 0; byte < 4; ++byte)
                actual_adj[4 * i + byte] = static_cast<std::uint8_t>(value >> (8 * byte));
        }
        check(actual_adj == original_adj, "Original ice region-walk input order differs");
        mark_terrain_ice_closed_walks(fill);
        check(fill.closed_walk_flags == initial_flags,
              "Original ice region-walk initial flags differ");
        const auto actual_regions = trace_terrain_ice_intermediate_regions(fill);
        check(actual_regions.size() == expected_regions.size(),
              "Original ice region-walk count differs");
        for (std::size_t i = 0; i < expected_regions.size(); ++i)
            check(actual_regions[i].directed_edges == expected_regions[i],
                  "Original ice region-walk edge order differs");
        check(fill.closed_walk_flags == output_flags,
              "Original ice region-walk final flags differ");
        for (std::size_t i = 0; i < write_mask.size(); ++i)
            check(write_mask[i] <= 1 && (initial_flags[i] == output_flags[i] || write_mask[i]) &&
                      (write_mask[i] == 0 || output_flags[i] == 1),
                  "Original ice region-walk flag writes are inconsistent");
    }
    check(in.peek() == std::char_traits<char>::eof(),
          "Trailing original ice region-walk fixture bytes");
}
void ice_region_processing_fixture(const char *path) {
    std::ifstream in(path, std::ios::binary);
    char magic[8];
    check(bool(in.read(magic, 8)) && std::string_view(magic, 8) == "C5IPRC01",
          "Invalid original ice region-processing fixture");
    const auto exe_hash = fixed_bytes<32>(in), parent_hash = fixed_bytes<32>(in);
    const auto settings_hash = fixed_bytes<32>(in);
    const auto hex = [](const auto &bytes) {
        std::string result;
        static constexpr char digits[] = "0123456789abcdef";
        for (const auto byte : bytes) {
            result.push_back(digits[byte >> 4]);
            result.push_back(digits[byte & 15]);
        }
        return result;
    };
    check(hex(exe_hash) == "7c9f93d512c49a7ab2bc70f51c6caa10a82ddd697ac11d8a24ff1bb7ce7c3686" &&
              hex(parent_hash) == "9a7f4f1e9dd8a2bbfce59fe5f1f5c5a51cc3fa0f4da61287b9f77b7d76e9ec38" &&
              hex(settings_hash) == "ab594d596e9dc386ca2cabcd952799a97190093a70cbc3b8486c7b2593318f1b" &&
              word(in) == 90, "Original ice region-processing source identity differs");
    const auto read_blob = [&](std::size_t size) {
        std::vector<std::uint8_t> raw(size);
        check(bool(in.read(reinterpret_cast<char *>(raw.data()),
                           static_cast<std::streamsize>(size))),
              "Truncated original ice region-processing fixture");
        return raw;
    };
    const auto load32 = [](const auto &raw, std::size_t offset) {
        std::uint32_t value = 0;
        for (unsigned byte = 0; byte < 4; ++byte)
            value |= std::uint32_t(raw[offset + byte]) << (8 * byte);
        return value;
    };
    for (unsigned test_case = 0; test_case < 90; ++test_case) {
        const auto name_size = word(in), profile_size = word(in);
        const auto precision = word(in), nregion = word(in), nvertex = word(in);
        const auto nindex = word(in), seed = word(in), final_seed = word(in);
        const auto shrink_bits = word(in), boundary_rate_bits = word(in);
        const auto interior_rate_bits = word(in);
        check(name_size <= 32 && profile_size <= 16 &&
                  (precision == 24 || precision == 53) &&
                  nregion <= 64 && nvertex <= 256 && nindex <= 256 &&
                  seed == 0 && shrink_bits == 0x3eb33333u,
              "Invalid original ice region-processing extent or settings");
        const auto name = read_blob(name_size), profile = read_blob(profile_size);
        check(!name.empty() && !profile.empty(),
              "Invalid original ice region-processing case name");
        const auto raw_regions = read_blob(16 * nregion);
        const auto raw_vertices = read_blob(16 * nvertex);
        const auto raw_indices = read_blob(12 * nindex);
        const auto expected_regions = read_blob(16 * nregion);
        const auto expected_xy = read_blob(8 * nvertex);
        const auto expected_indices = read_blob(12 * nindex);
        TerrainIceIntermediateRegionGeometry geometry;
        geometry.regions.reserve(nregion);
        geometry.vertices.reserve(nvertex);
        geometry.indices.reserve(nindex);
        geometry.incidence.resize(nvertex);
        for (std::size_t i = 0; i < nregion; ++i)
            geometry.regions.push_back({load32(raw_regions, 16 * i),
                                        load32(raw_regions, 16 * i + 4),
                                        load32(raw_regions, 16 * i + 8),
                                        raw_regions[16 * i + 12]});
        for (std::size_t i = 0; i < nvertex; ++i)
            geometry.vertices.push_back({
                std::bit_cast<float>(load32(raw_vertices, 16 * i)),
                std::bit_cast<float>(load32(raw_vertices, 16 * i + 4))});
        for (std::size_t i = 0; i < nindex; ++i) {
            const auto edge = std::array<std::uint32_t, 3>{
                load32(raw_indices, 12 * i),
                load32(raw_indices, 12 * i + 4),
                load32(raw_indices, 12 * i + 8)};
            check(edge[0] < nvertex && edge[1] < nvertex,
                  "Invalid original ice region-processing edge");
            geometry.indices.push_back(edge);
            geometry.incidence[edge[0]].push_back(static_cast<std::uint32_t>(i));
            geometry.incidence[edge[1]].push_back(static_cast<std::uint32_t>(i));
        }
        auto random = initial_terrain_ice_random_state();
        random.value = seed;
        random.initial_value = seed;
        process_terrain_ice_intermediate_regions(
            geometry, random, std::bit_cast<float>(boundary_rate_bits),
            std::bit_cast<float>(interior_rate_bits),
            std::bit_cast<float>(shrink_bits), precision);
        check(random.value == final_seed, "Original ice region-processing RNG differs");
        for (std::size_t i = 0; i < nregion; ++i) {
            const auto &record = geometry.regions[i];
            check(record.first_vertex == load32(expected_regions, 16 * i) &&
                      record.first_index == load32(expected_regions, 16 * i + 4) &&
                      record.vertex_count == load32(expected_regions, 16 * i + 8) &&
                      record.boundary == expected_regions[16 * i + 12],
                  "Original ice region-processing record differs");
        }
        for (std::size_t i = 0; i < nvertex; ++i)
            for (unsigned component = 0; component < 2; ++component)
                check(std::bit_cast<std::uint32_t>(geometry.vertices[i][component]) ==
                          load32(expected_xy, 8 * i + 4 * component),
                      "Original ice region-processing vertex differs");
        for (std::size_t i = 0; i < nindex; ++i)
            for (unsigned field = 0; field < 3; ++field)
                check(geometry.indices[i][field] ==
                          load32(expected_indices, 12 * i + 4 * field),
                      "Original ice region-processing edge differs");
        for (std::size_t vertex = 0; vertex < nvertex; ++vertex) {
            const auto count = word(in);
            check(count <= 2 * nindex &&
                      count == geometry.incidence[vertex].size(),
                  "Original ice region-processing degree differs");
            for (std::size_t i = 0; i < count; ++i)
                check(word(in) == geometry.incidence[vertex][i],
                      "Original ice region-processing incidence differs");
        }
    }
    check(in.peek() == std::char_traits<char>::eof(),
          "Trailing original ice region-processing fixture bytes");
}
void ice_saved_source_adjacency_fixture(const char *clipped_path,
                                        const char *adjacency_path) {
    char magic[8];
    std::ifstream source(clipped_path, std::ios::binary);
    check(bool(source.read(magic, 8)) &&
              std::string_view(magic, 8) == "C5ICLP01",
          "Invalid selected-save clipped ice graph");
    const auto exe = fixed_bytes<32>(source);
    source.ignore(64); // Original precursor graph and spatial-tree identities.
    const auto points = word(source), edges = word(source);
    check((points == 230661 && edges == 345189) ||
              (points == 9951 && edges == 13991),
          "Selected-save clipped ice extent changed");
    std::vector<std::array<std::uint32_t, 2>> source_ranges(points);
    for (auto &range : source_ranges) {
        source.ignore(16); // XY, active flag and initial potential.
        range = {word(source), word(source)};
    }
    std::vector<std::array<std::uint32_t, 2>> source_edges(edges);
    for (auto &edge : source_edges) {
        edge = {word(source), word(source)};
        source.ignore(28); // Identity flag and weight-stage scratch.
    }
    check(source.peek() == std::char_traits<char>::eof(),
          "Trailing selected-save clipped ice graph bytes");
    const auto native = build_terrain_ice_source_adjacency(points, source_edges);
    std::ifstream original(adjacency_path, std::ios::binary);
    check(bool(original.read(magic, 8)) &&
              std::string_view(magic, 8) == "C5IADJ01" &&
              fixed_bytes<32>(original) == exe,
          "Invalid original source adjacency identity");
    fixed_bytes<32>(original); // Hash of the exact source point and edge bytes.
    check(word(original) == points && word(original) == edges &&
              word(original) == 2 * edges,
          "Original source adjacency extent changed");
    check(native.ranges.size() == points &&
              native.edge_indices.size() == 2ull * edges,
          "Native source adjacency extent differs");
    const auto base = points ? source_ranges.front()[0] : 0;
    for (std::size_t i = 0; i < native.ranges.size(); ++i) {
        const auto &range = native.ranges[i];
        check(source_ranges[i][0] == base + 4 * range[0] &&
                  source_ranges[i][1] == range[1],
              "Native source adjacency point allocation differs");
        check(range[0] == word(original) && range[1] == word(original),
              "Native source adjacency range differs");
    }
    for (const auto edge : native.edge_indices)
        check(edge == word(original),
              "Native source adjacency edge order differs");
    check(original.peek() == std::char_traits<char>::eof(),
          "Trailing original source adjacency bytes");
}
void ice_saved_region_geometry_fixture(const char *weighted_path,
                                       const char *regions_path,
                                       const char *topology_path,
                                       const char *geometry_path,
                                       const char *postprocessed_path = nullptr) {
    char magic[8];
    std::ifstream weighted(weighted_path, std::ios::binary);
    check(bool(weighted.read(magic, 8)) &&
              std::string_view(magic, 8) == "C5IWGS01",
          "Invalid selected-save weighted ice graph");
    const auto exe = fixed_bytes<32>(weighted);
    const auto xml = fixed_bytes<32>(weighted);
    weighted.ignore(48); // Original source graph and parent identities.
    const auto source_points = word(weighted), source_edges = word(weighted);
    const bool full = source_points == 230661 && source_edges == 345189;
    const bool prefix = source_points == 9951 && source_edges == 13991;
    check(full || prefix, "Selected-save weighted ice extent changed");
    const auto hex = [](const auto &bytes) {
        std::string result;
        static constexpr char digits[] = "0123456789abcdef";
        for (const auto byte : bytes) {
            result.push_back(digits[byte >> 4]);
            result.push_back(digits[byte & 15]);
        }
        return result;
    };
    check(hex(exe) == "7c9f93d512c49a7ab2bc70f51c6caa10a82ddd697ac11d8a24ff1bb7ce7c3686" &&
              hex(xml) == "ab594d596e9dc386ca2cabcd952799a97190093a70cbc3b8486c7b2593318f1b",
          "Selected-save weighted ice executable or settings changed");
    std::vector<TerrainIceIntermediateSourcePoint> points(source_points);
    for (auto &point : points) {
        point.xy = {std::bit_cast<float>(word(weighted)),
                    std::bit_cast<float>(word(weighted))};
        point.active = static_cast<std::uint8_t>(word(weighted, 1));
        weighted.ignore(15);
    }
    std::vector<TerrainIceIntermediateSourceEdge> edges(source_edges);
    for (auto &edge : edges) {
        edge.first = word(weighted); edge.second = word(weighted);
        edge.boundary = static_cast<std::uint8_t>(word(weighted, 1));
        weighted.ignore(3);
        edge.weights = {std::bit_cast<float>(word(weighted)),
                        std::bit_cast<float>(word(weighted))};
        weighted.ignore(16);
    }
    check(weighted.peek() == std::char_traits<char>::eof(),
          "Trailing selected-save weighted ice bytes");
    auto fill = build_terrain_ice_intermediate_regions(points, edges);

    std::ifstream original(regions_path, std::ios::binary);
    check(bool(original.read(magic, 8)) &&
              std::string_view(magic, 8) == "C5IRSV01" &&
              fixed_bytes<32>(original) == exe,
          "Invalid selected-save original ice fill identity");
    const auto weighted_hash = fixed_bytes<32>(original);
    check(hex(weighted_hash) == (full
              ? "a4b6550933d6727ef5c4b47d4e70930195ef6c6eeebf8071cca718bf10ef2e3f"
              : "bae2d5cec926b5f3e93a3315df59fe5cfe279c0eb3afa35f666cc3462fe5e7bd"),
          "Selected-save ice weighted source identity differs");
    const auto out_points = word(original), out_edges = word(original);
    const auto active = word(original), n20 = word(original), n12 = word(original);
    check(out_points == source_points && out_edges == source_edges &&
              active == (full ? 18978u : 2601u) &&
              fill.points.size() == n20 && fill.links.size() == n12 &&
              n12 == (full ? 19345u : 2609u),
          "Selected-save ice fill counts differ");
    for (const auto mapped : fill.source24_to_active)
        check(mapped == word(original),
              "Selected-save ice source-point map differs");
    for (const auto &point : fill.points) {
        check(std::bit_cast<std::uint32_t>(point.xy[0]) == word(original) &&
                  std::bit_cast<std::uint32_t>(point.xy[1]) == word(original),
              "Selected-save ice filled point differs");
        word(original); word(original);
        check(point.link_count == word(original),
              "Selected-save ice filled point degree differs");
    }
    for (const auto &link : fill.links) {
        check(link.first == word(original) && link.second == word(original) &&
                  link.boundary == word(original, 1),
              "Selected-save ice filled link differs");
        original.ignore(3);
    }
    check(original.peek() == std::char_traits<char>::eof(),
          "Trailing selected-save ice region fill bytes");
    build_terrain_ice_intermediate_adjacency(fill);
    sort_terrain_ice_intermediate_adjacency(fill);

    std::ifstream topology(topology_path, std::ios::binary);
    check(bool(topology.read(magic, 8)) &&
              std::string_view(magic, 8) == "C5IWKS01" &&
              fixed_bytes<32>(topology) == exe &&
              fixed_bytes<32>(topology) == weighted_hash &&
              word(topology) == n20 && word(topology) == n12,
          "Invalid selected-save original ice topology identity");
    for (const auto &point : fill.points)
        check(point.adjacency_first == word(topology) &&
                  point.link_count == word(topology),
              "Selected-save ice adjacency range differs");
    for (const auto link : fill.adjacency_link_indices)
        check(link == word(topology),
              "Selected-save ice sorted adjacency differs");
    mark_terrain_ice_closed_walks(fill);
    for (const auto flag : fill.closed_walk_flags)
        check(flag == word(topology, 1),
              "Selected-save ice closed-walk flag differs");
    check(topology.peek() == std::char_traits<char>::eof(),
          "Trailing selected-save ice topology bytes");
    const auto walks = trace_terrain_ice_intermediate_regions(fill);
    const auto geometry = build_terrain_ice_intermediate_region_geometry(
        fill, walks, 0.25f, 53);

    std::ifstream output(geometry_path, std::ios::binary);
    check(bool(output.read(magic, 8)) &&
              std::string_view(magic, 8) == "C5IRGM01" &&
              fixed_bytes<32>(output) == exe,
          "Invalid selected-save original ice geometry identity");
    const auto regions_hash = fixed_bytes<32>(output);
    const auto topology_hash = fixed_bytes<32>(output);
    check(hex(regions_hash) == (full
              ? "4f550d7da1fd32205700352c416ea7824bcee371d740609f3673ab20819d4cd4"
              : "9501019a7e4ef911dee510c3592ee72cb53fd6dffb4eb4e6e0d6e73344254240") &&
              hex(topology_hash) == (full
              ? "fb017169f94501fab147c2ab8981be7f5429c1b74f34546524d9dc86261691b8"
              : "32d8de0a59c06d6fb97d55db342601508462b80de7248e0f1a4c28464034d671"),
          "Selected-save ice geometry source identity differs");
    check(word(output) == 0x3e800000u &&
              word(output) == geometry.regions.size() &&
              word(output) == geometry.vertices.size() &&
              word(output) == geometry.indices.size() &&
              geometry.regions.size() == (full ? 376u : 9u),
          "Selected-save ice geometry counts differ");
    for (const auto flag : fill.closed_walk_flags)
        check(flag == word(output, 1),
              "Selected-save ice final walk flag differs");
    for (const auto &region : geometry.regions)
        check(region.first_vertex == word(output) &&
                  region.first_index == word(output) &&
                  region.vertex_count == word(output) &&
                  region.boundary == word(output),
              "Selected-save ice geometry region differs");
    for (const auto &point : geometry.vertices) {
        check(std::bit_cast<std::uint32_t>(point[0]) == word(output) &&
                  std::bit_cast<std::uint32_t>(point[1]) == word(output),
              "Selected-save ice geometry vertex differs");
        word(output); word(output);
    }
    for (const auto &edge : geometry.indices)
        check(edge[0] == word(output) && edge[1] == word(output) &&
                  edge[2] == word(output),
              "Selected-save ice geometry edge differs");
    check(output.peek() == std::char_traits<char>::eof(),
          "Trailing selected-save ice geometry bytes");
    if (!postprocessed_path) return;
    std::ifstream postprocessed(postprocessed_path, std::ios::binary);
    check(bool(postprocessed.read(magic, 8)) &&
              std::string_view(magic, 8) == "C5IPST01" &&
              fixed_bytes<32>(postprocessed) == exe &&
              fixed_bytes<32>(postprocessed) == xml,
          "Invalid selected-save ice postprocessing identity");
    const auto geometry_hash = fixed_bytes<32>(postprocessed);
    check(hex(geometry_hash) == (full
              ? "8891d9668759f34bb7fb534d3299a45fe9ff10b51c1c7f91221ff21483d13e63"
              : "b96dd14273e6b976a07ca205c9e8f70b548756eb965dbec9a3f8efb54ae64ae0") &&
              word(postprocessed) == geometry.regions.size(),
          "Selected-save ice component source identity differs");
    for (unsigned i = 0; i < 6; ++i) word(postprocessed);
    const auto box = terrain_ice_region_component_box(geometry);
    for (const float bound : box)
        check(std::bit_cast<std::uint32_t>(bound) == word(postprocessed),
              "Selected-save ice component bound differs");
}
void ice_saved_region_processing_fixture(const char *geometry_path,
                                         const char *processed_path,
                                         const char *finalized_path = nullptr,
                                         const char *postprocessed_path = nullptr) {
    std::ifstream source(geometry_path, std::ios::binary);
    char magic[8];
    check(bool(source.read(magic, 8)) &&
              std::string_view(magic, 8) == "C5IRGM01",
          "Invalid selected-save ice region geometry");
    const auto exe = fixed_bytes<32>(source);
    fixed_bytes<32>(source); fixed_bytes<32>(source);
    const auto edge_pull = word(source), regions = word(source);
    const auto vertices = word(source), edges = word(source);
    const bool full = regions == 376 && vertices == 27955 && edges == 27955;
    const bool prefix = regions == 9 && vertices == 2060 && edges == 2060;
    check((full || prefix) && edge_pull == 0x3e800000u,
          "Selected-save ice region geometry dimensions changed");
    const auto links = full ? 19345u : 2609u;
    source.ignore(2 * links);
    check(source.gcount() == 2 * links,
          "Truncated selected-save ice region flags");
    TerrainIceIntermediateRegionGeometry geometry;
    geometry.regions.resize(regions);
    for (auto &region : geometry.regions) {
        region.first_vertex = word(source);
        region.first_index = word(source);
        region.vertex_count = word(source);
        const auto boundary = word(source);
        check(boundary <= 1, "Invalid selected-save ice region boundary");
        region.boundary = static_cast<std::uint8_t>(boundary);
    }
    geometry.vertices.resize(vertices);
    for (auto &point : geometry.vertices) {
        point = {std::bit_cast<float>(word(source)),
                 std::bit_cast<float>(word(source))};
        word(source); word(source);
    }
    geometry.indices.resize(edges);
    geometry.incidence.resize(vertices);
    for (std::uint32_t i = 0; i < edges; ++i) {
        auto &edge = geometry.indices[i];
        edge = {word(source), word(source), word(source)};
        check(edge[0] < vertices && edge[1] < vertices && edge[2] < regions,
              "Invalid selected-save ice region edge");
        geometry.incidence[edge[0]].push_back(i);
        geometry.incidence[edge[1]].push_back(i);
    }
    check(source.peek() == std::char_traits<char>::eof(),
          "Trailing selected-save ice region geometry bytes");
    auto random = initial_terrain_ice_random_state();
    process_terrain_ice_intermediate_regions(
        geometry, random, 0.6f, 0.05f, 0.35f, 53);

    std::ifstream original(processed_path, std::ios::binary);
    check(bool(original.read(magic, 8)) &&
              std::string_view(magic, 8) == "C5IPRS01" &&
              fixed_bytes<32>(original) == exe,
          "Invalid selected-save original ice processing identity");
    fixed_bytes<32>(original); fixed_bytes<32>(original);
    const auto out_regions = word(original), out_vertices = word(original);
    const auto out_edges = word(original), retained = word(original);
    const auto seed = word(original);
    check(out_regions == regions && out_vertices == vertices &&
              out_edges == edges && random.value == seed &&
              retained == (full ? 164u : 3u),
          "Selected-save ice processing dimensions or RNG differ");
    for (const auto &region : geometry.regions) {
        check(region.first_vertex == word(original) &&
                  region.first_index == word(original) &&
                  region.vertex_count == word(original) &&
                  region.boundary == (word(original) & 0xffu),
              "Selected-save ice processed region differs");
    }
    std::vector<std::array<std::uint32_t, 2>> original_incidence(vertices);
    for (std::uint32_t i = 0; i < vertices; ++i) {
        const auto &point = geometry.vertices[i];
        check(std::bit_cast<std::uint32_t>(point[0]) == word(original) &&
                  std::bit_cast<std::uint32_t>(point[1]) == word(original),
              "Selected-save ice processed vertex differs");
        original_incidence[i] = {word(original), word(original)};
    }
    for (const auto &edge : geometry.indices)
        for (const auto value : edge)
            check(value == word(original),
                  "Selected-save ice processed edge differs");
    std::vector<std::uint32_t> original_links(2 * edges);
    for (auto &link : original_links) link = word(original);
    check(original.peek() == std::char_traits<char>::eof(),
          "Trailing selected-save ice processed bytes");
    constexpr std::uint32_t adjacency_base = 0x60500000u;
    for (std::uint32_t i = 0; i < vertices; ++i) {
        const auto [address, degree] = original_incidence[i];
        check(address >= adjacency_base &&
                  (address - adjacency_base) % 4 == 0 &&
                  (address - adjacency_base) / 4 <= original_links.size() &&
                  degree <= original_links.size() - (address - adjacency_base) / 4 &&
                  degree == geometry.incidence[i].size(),
              "Selected-save ice processed adjacency span differs");
        for (std::uint32_t j = 0; j < degree; ++j)
            check(geometry.incidence[i][j] ==
                      original_links[(address - adjacency_base) / 4 + j],
                  "Selected-save ice processed adjacency link differs");
    }
    if (!finalized_path) return;
    const auto finalized = finalize_terrain_ice_intermediate_regions(geometry);
    std::ifstream target(finalized_path, std::ios::binary);
    check(bool(target.read(magic, 8)) &&
              std::string_view(magic, 8) == "C5IFIN01" &&
              fixed_bytes<32>(target) == exe,
          "Invalid selected-save original ice finalization identity");
    fixed_bytes<32>(target); fixed_bytes<32>(target);
    const auto final_regions = word(target), final_vertices = word(target);
    const auto final_edges = word(target), final_links = word(target);
    check(final_regions == finalized.regions.size() &&
              final_vertices == finalized.vertices.size() &&
              final_edges == finalized.indices.size() &&
              final_links == 2 * final_edges &&
              final_vertices == final_edges &&
              final_vertices == (full ? 27213u : 2042u) &&
              word(target) == retained && word(target) == random.value,
          "Selected-save ice finalization dimensions or RNG differ");
    for (const auto &region : finalized.regions)
        check(region.first_vertex == word(target) &&
                  region.first_index == word(target) &&
                  region.vertex_count == word(target) &&
                  region.boundary == (word(target) & 0xffu),
              "Selected-save ice finalized region differs");
    std::uint32_t link_cursor = 0;
    for (std::size_t i = 0; i < finalized.vertices.size(); ++i) {
        const auto &point = finalized.vertices[i];
        check(std::bit_cast<std::uint32_t>(point[0]) == word(target) &&
                  std::bit_cast<std::uint32_t>(point[1]) == word(target),
              "Selected-save ice finalized vertex differs");
        const auto address = word(target), degree = word(target);
        check(address == 0x60600000u + 4 * link_cursor &&
                  degree == finalized.incidence[i].size(),
              "Selected-save ice finalized adjacency span differs");
        link_cursor += degree;
    }
    check(link_cursor == final_links,
          "Selected-save ice finalized adjacency length differs");
    for (const auto &edge : finalized.indices)
        for (const auto value : edge)
            check(value == word(target),
                  "Selected-save ice finalized edge differs");
    for (const auto &list : finalized.incidence)
        for (const auto edge : list)
            check(edge == word(target),
                  "Selected-save ice finalized adjacency link differs");
    check(target.peek() == std::char_traits<char>::eof(),
          "Trailing selected-save ice finalized bytes");
    if (!postprocessed_path) return;
    const auto crossings = find_terrain_ice_edge_intersections(finalized);
    auto crossing_pruned = finalized;
    if (full) {
        constexpr std::array<std::array<std::uint32_t, 2>, 10> original_crossings{{
            {14745, 14747}, {14793, 14795}, {16431, 16433},
            {16431, 16434}, {17697, 17700}, {17697, 17701},
            {17698, 17700}, {17698, 17701}, {21305, 21308},
            {21306, 21308}}};
        check(crossings.size() == original_crossings.size(),
              "Selected-save ice crossing count differs");
        for (std::size_t i = 0; i < crossings.size(); ++i)
            check(crossings[i] == original_crossings[i],
                  "Selected-save ice crossing edge pair differs");
        constexpr std::array<std::array<std::uint32_t, 3>, 20> original_directed{{
            {14745, 14747, 0x3f55e340u}, {14747, 14745, 0x3f4213fcu},
            {14793, 14795, 0x3f57ad30u}, {14795, 14793, 0x3f19f962u},
            {16431, 16434, 0x3e17ef5au}, {16431, 16433, 0x3e3a38b3u},
            {16433, 16431, 0x3f6605adu}, {16434, 16431, 0x3d8f84f2u},
            {17697, 17700, 0x3f7bb10bu}, {17697, 17701, 0x3f7bc0b9u},
            {17698, 17701, 0x3d073dfbu}, {17698, 17700, 0x3d2b716eu},
            {17700, 17698, 0x3f79c9b0u}, {17700, 17697, 0x3f7faa4bu},
            {17701, 17697, 0x3a9c6bfeu}, {17701, 17698, 0x3c912b1fu},
            {21305, 21308, 0x3f6621e7u}, {21306, 21308, 0x3e184182u},
            {21308, 21306, 0x3f30886eu}, {21308, 21305, 0x3f3ddbdau}}};
        const auto directed = build_terrain_ice_directed_intersections(finalized);
        check(directed.size() == original_directed.size(),
              "Selected-save ice directed crossing count differs");
        for (std::size_t i = 0; i < directed.size(); ++i)
            check(directed[i].edge == original_directed[i][0] &&
                      directed[i].other == original_directed[i][1] &&
                      std::bit_cast<std::uint32_t>(directed[i].position) ==
                          original_directed[i][2],
                  "Selected-save ice directed crossing position differs");
        constexpr std::array<std::array<std::uint32_t, 2>, 10> original_xy{{
            {3256593874u, 3290411603u}, {3283464709u, 3290395834u},
            {3286900375u, 1141176492u}, {3286900128u, 1141176245u},
            {3289973541u, 1141094107u}, {3289973540u, 1141094108u},
            {3289973413u, 1141094083u}, {3289973402u, 1141094051u},
            {1144584584u, 1142385529u}, {1144584222u, 1142385576u}}};
        constexpr std::array<std::array<std::uint32_t, 2>, 10> original_vertex_edges{{
            {14745, 14747}, {14793, 14795}, {16431, 16434}, {16431, 16433},
            {17697, 17700}, {17697, 17701}, {17698, 17701}, {17698, 17700},
            {21305, 21308}, {21306, 21308}}};
        const auto generated = build_terrain_ice_crossing_vertices(finalized, directed);
        check(generated.size() == original_xy.size(),
              "Selected-save ice crossing vertex count differs");
        for (std::size_t i = 0; i < generated.size(); ++i)
            check(generated[i].first_edge == original_vertex_edges[i][0] &&
                      generated[i].second_edge == original_vertex_edges[i][1] &&
                      std::bit_cast<std::uint32_t>(generated[i].xy[0]) ==
                          original_xy[i][0] &&
                      std::bit_cast<std::uint32_t>(generated[i].xy[1]) ==
                          original_xy[i][1],
                  "Selected-save ice crossing vertex differs");
        const auto split = split_terrain_ice_intersecting_edges(finalized);
        constexpr std::array<std::uint32_t, 14> replaced_edges{{
            14745, 14747, 14793, 14795, 16431, 16433, 16434,
            17697, 17698, 17700, 17701, 21305, 21306, 21308}};
        constexpr std::array<std::array<std::uint32_t, 3>, 34> appended_edges{{
            {14646, 27213, 49}, {27213, 14647, 49},
            {14648, 27213, 49}, {27213, 14645, 49},
            {14672, 27214, 63}, {27214, 14673, 63},
            {14674, 27214, 63}, {27214, 14671, 63},
            {15969, 27215, 212}, {27215, 27216, 212},
            {27216, 15970, 212}, {15971, 27216, 212},
            {27216, 15972, 212}, {15972, 27215, 212},
            {27215, 15973, 212}, {17231, 27217, 214},
            {27217, 27218, 214}, {27218, 17232, 214},
            {17232, 27219, 214}, {27219, 27220, 214},
            {27220, 17233, 214}, {17234, 27220, 214},
            {27220, 27217, 214}, {27217, 17235, 214},
            {17235, 27218, 214}, {27218, 27219, 214},
            {27219, 17236, 214}, {20795, 27221, 245},
            {27221, 20796, 245}, {20796, 27222, 245},
            {27222, 20797, 245}, {20798, 27222, 245},
            {27222, 27221, 245}, {27221, 20799, 245}}};
        check(split.vertices.size() == finalized.vertices.size() + original_xy.size() &&
                  split.indices.size() == finalized.indices.size() + appended_edges.size() &&
                  split.incidence.empty(),
              "Selected-save ice split graph extent differs");
        for (std::size_t i = 0; i < generated.size(); ++i)
            check(split.vertices[finalized.vertices.size() + i] == generated[i].xy,
                  "Selected-save ice split vertex differs");
        for (std::size_t i = 0; i < finalized.indices.size(); ++i) {
            const auto expected = std::binary_search(replaced_edges.begin(),
                                                     replaced_edges.end(), i) ?
                std::array<std::uint32_t, 3>{UINT32_MAX, UINT32_MAX, UINT32_MAX} :
                finalized.indices[i];
            check(split.indices[i] == expected,
                  "Selected-save ice original split edge differs");
        }
        for (std::size_t i = 0; i < appended_edges.size(); ++i)
            check(split.indices[finalized.indices.size() + i] == appended_edges[i],
                  "Selected-save ice appended split edge differs");
        const auto compact_split = finalize_terrain_ice_intermediate_regions(split);
        check(compact_split.vertices.size() == 27223 &&
                  compact_split.indices.size() == 27233,
              "Selected-save ice compact split extent differs");
        std::uint64_t xy_hash = 0xcbf29ce484222325ull;
        std::uint64_t edge_hash = xy_hash;
        const auto update_hash = [](std::uint64_t &hash, std::uint32_t value) {
            for (unsigned byte = 0; byte < 4; ++byte) {
                hash ^= (value >> (8 * byte)) & 0xffu;
                hash *= 0x100000001b3ull;
            }
        };
        for (const auto &point : compact_split.vertices)
            for (float value : point)
                update_hash(xy_hash, std::bit_cast<std::uint32_t>(value));
        for (const auto &edge : compact_split.indices)
            for (const auto value : edge) update_hash(edge_hash, value);
        check(xy_hash == 0x25a27e1957954ffbull &&
                  edge_hash == 0x3262b3ca73d4020full,
              "Selected-save ice compact split differs from original trace");
        const auto rebuilt = rebuild_terrain_ice_crossing_faces(finalized);
        check(rebuilt.regions.size() == 389 &&
                  rebuilt.vertices == compact_split.vertices &&
                  rebuilt.indices.size() == compact_split.indices.size(),
              "Selected-save ice rebuilt face graph extent differs");
        std::uint64_t rebuilt_hash = 0xcbf29ce484222325ull;
        std::size_t relabeled = 0;
        for (std::size_t i = 0; i < rebuilt.indices.size(); ++i) {
            const auto &before = compact_split.indices[i];
            const auto &after = rebuilt.indices[i];
            check(before[0] == after[0] && before[1] == after[1],
                  "Selected-save ice face rebuild moved an edge endpoint");
            relabeled += before[2] != after[2];
            for (const auto value : after) update_hash(rebuilt_hash, value);
        }
        check(relabeled == 5141 && rebuilt_hash == 0x28fe658be56a73daull,
              "Selected-save ice face labels differ from original trace");
        crossing_pruned = rebuilt;
        prune_terrain_ice_crossing_faces(
            crossing_pruned, static_cast<std::uint32_t>(finalized.vertices.size()),
            static_cast<std::uint32_t>(finalized.regions.size()));
        constexpr std::array<std::uint32_t, 19> original_deleted{{
            14744, 14747, 14792, 16431, 16433, 17698, 17699,
            21305, 21306, 21308, 27213, 27216, 27217, 27220,
            27222, 27225, 27226, 27229, 27232}};
        check(crossing_pruned.indices.size() == rebuilt.indices.size() &&
                  crossing_pruned.incidence.empty(),
              "Selected-save ice pruned crossing graph extent differs");
        std::uint64_t pruned_hash = 0xcbf29ce484222325ull;
        for (std::size_t i = 0; i < crossing_pruned.indices.size(); ++i) {
            const auto expected = std::binary_search(original_deleted.begin(),
                                                     original_deleted.end(), i) ?
                std::array<std::uint32_t, 3>{UINT32_MAX, UINT32_MAX, UINT32_MAX} :
                rebuilt.indices[i];
            check(crossing_pruned.indices[i] == expected,
                  "Selected-save ice crossing prune edge differs");
            for (const auto value : crossing_pruned.indices[i])
                update_hash(pruned_hash, value);
        }
        check(pruned_hash == 0xbc7d94b5600fcd24ull,
              "Selected-save ice crossing prune stream differs");
    } else {
        check(crossings.empty(),
              "Selected-save ice prefix gained an original-absent crossing");
        const auto directed = build_terrain_ice_directed_intersections(finalized);
        check(directed.empty(),
              "Selected-save ice prefix gained a directed crossing");
        check(build_terrain_ice_crossing_vertices(finalized, directed).empty(),
              "Selected-save ice prefix gained a crossing vertex");
        const auto split = split_terrain_ice_intersecting_edges(finalized);
        check(split.vertices == finalized.vertices &&
                  split.indices == finalized.indices &&
                  split.incidence == finalized.incidence,
              "Selected-save ice prefix changed during crossing split");
        const auto rebuilt = rebuild_terrain_ice_crossing_faces(finalized);
        check(rebuilt.regions.size() == finalized.regions.size() &&
                  rebuilt.vertices == finalized.vertices &&
                  rebuilt.indices == finalized.indices &&
                  rebuilt.incidence == finalized.incidence,
              "Selected-save ice prefix changed during face rebuild");
        crossing_pruned = rebuilt;
        prune_terrain_ice_crossing_faces(
            crossing_pruned, static_cast<std::uint32_t>(finalized.vertices.size()),
            static_cast<std::uint32_t>(finalized.regions.size()));
        check(crossing_pruned.indices == finalized.indices &&
                  crossing_pruned.incidence == finalized.incidence,
              "Selected-save ice prefix changed during crossing prune");
    }
    std::ifstream postprocessed(postprocessed_path, std::ios::binary);
    check(bool(postprocessed.read(magic, 8)) &&
              std::string_view(magic, 8) == "C5IPST01" &&
              fixed_bytes<32>(postprocessed) == exe,
          "Invalid selected-save ice postprocessing identity");
    fixed_bytes<32>(postprocessed); fixed_bytes<32>(postprocessed);
    const auto initial_faces = word(postprocessed), face_count = word(postprocessed);
    const auto point_count = word(postprocessed), edge_count = word(postprocessed);
    const auto link_count = word(postprocessed), occupied = word(postprocessed);
    const auto post_seed = word(postprocessed);
    check(initial_faces == regions && face_count == (full ? 389u : 9u) &&
              point_count == (full ? 27214u : 2042u) &&
              edge_count == point_count && link_count == 2 * edge_count &&
              occupied == (full ? 166u : 3u) && post_seed == random.value,
          "Selected-save ice postprocessing extent differs");
    for (unsigned i = 0; i < 4; ++i) word(postprocessed); // Pre-prune box.
    std::vector<std::array<std::uint32_t, 4>> original_faces(face_count);
    for (auto &face : original_faces)
        for (auto &field : face) field = word(postprocessed);
    auto recounted = finalized;
    recounted.vertices.resize(point_count);
    std::vector<std::array<std::uint32_t, 2>> post_incidence(point_count);
    for (std::size_t i = 0; i < point_count; ++i) {
        auto &point = recounted.vertices[i];
        point = {std::bit_cast<float>(word(postprocessed)),
                 std::bit_cast<float>(word(postprocessed))};
        post_incidence[i] = {word(postprocessed), word(postprocessed)};
    }
    recounted.indices.resize(edge_count);
    for (auto &edge : recounted.indices)
        edge = {word(postprocessed), word(postprocessed), word(postprocessed)};
    std::vector<std::uint32_t> post_links(link_count);
    for (auto &link : post_links) link = word(postprocessed);
    check(postprocessed.peek() == std::char_traits<char>::eof(),
          "Trailing selected-save ice postprocessing bytes");
    auto native_postprocessed = postprocess_terrain_ice_crossings(finalized);
    check(native_postprocessed.vertices.size() == point_count &&
              native_postprocessed.indices.size() == edge_count &&
              native_postprocessed.incidence.size() == point_count,
          "Selected-save native ice postprocess extent differs");
    std::uint32_t native_link_cursor = 0;
    for (std::size_t i = 0; i < point_count; ++i) {
        const auto &actual = native_postprocessed.vertices[i];
        const auto &expected = recounted.vertices[i];
        check(std::bit_cast<std::uint32_t>(actual[0]) ==
                      std::bit_cast<std::uint32_t>(expected[0]) &&
                  std::bit_cast<std::uint32_t>(actual[1]) ==
                      std::bit_cast<std::uint32_t>(expected[1]),
              "Selected-save native ice postprocess vertex differs");
        const auto [address, degree] = post_incidence[i];
        check(address == 0x60600000u + 4 * native_link_cursor &&
                  degree == native_postprocessed.incidence[i].size(),
              "Selected-save native ice postprocess adjacency span differs");
        for (const auto edge : native_postprocessed.incidence[i])
            check(edge == post_links[native_link_cursor++],
                  "Selected-save native ice postprocess adjacency link differs");
    }
    check(native_link_cursor == link_count &&
              native_postprocessed.indices == recounted.indices,
          "Selected-save native ice postprocess edges differ");
    for (std::size_t i = 0; i < face_count; ++i) {
        const auto &face = native_postprocessed.regions[i];
        check(face.first_vertex == original_faces[i][0] &&
                  face.first_index == original_faces[i][1] &&
                  face.vertex_count == original_faces[i][2] &&
                  face.boundary == original_faces[i][3],
              "Selected-save native ice postprocess face differs");
    }
    if (prefix) {
        check(recounted.vertices == finalized.vertices &&
                  recounted.indices == finalized.indices,
              "Selected-save prefix changed graph during crossing repair");
        std::uint32_t offset = 0;
        for (std::size_t i = 0; i < point_count; ++i) {
            const auto [address, degree] = post_incidence[i];
            check(address == 0x60600000u + 4 * offset &&
                      degree == finalized.incidence[i].size(),
                  "Selected-save prefix postprocessed adjacency span differs");
            for (const auto link : finalized.incidence[i])
                check(link == post_links[offset++],
                      "Selected-save prefix postprocessed adjacency link differs");
        }
        check(offset == link_count,
              "Selected-save prefix postprocessed adjacency length differs");
    }
    recount_terrain_ice_region_faces(recounted, face_count);
    for (std::size_t i = 0; i < face_count; ++i) {
        const auto &face = recounted.regions[i];
        check(face.first_vertex == original_faces[i][0] &&
                  face.first_index == original_faces[i][1] &&
                  face.vertex_count == original_faces[i][2] &&
                  face.boundary == original_faces[i][3],
              "Selected-save ice postprocessed face differs");
    }
}
void ice_region_prune_mutation_fixture(const char *path) {
    std::ifstream in(path, std::ios::binary);
    char magic[8];
    check(bool(in.read(magic, 8)) && std::string_view(magic, 8) == "C5IPRM01",
          "Invalid original ice region-prune mutation fixture");
    const auto exe_hash = fixed_bytes<32>(in), parent_hash = fixed_bytes<32>(in);
    const auto hex = [](const auto &bytes) {
        std::string result;
        static constexpr char digits[] = "0123456789abcdef";
        for (const auto byte : bytes) {
            result.push_back(digits[byte >> 4]);
            result.push_back(digits[byte & 15]);
        }
        return result;
    };
    check(hex(exe_hash) == "7c9f93d512c49a7ab2bc70f51c6caa10a82ddd697ac11d8a24ff1bb7ce7c3686" &&
              hex(parent_hash) == "9a7f4f1e9dd8a2bbfce59fe5f1f5c5a51cc3fa0f4da61287b9f77b7d76e9ec38" &&
              word(in) == 32, "Original ice region-prune mutation identity differs");
    const auto read_blob = [&](std::size_t size) {
        std::vector<std::uint8_t> raw(size);
        check(bool(in.read(reinterpret_cast<char *>(raw.data()),
                           static_cast<std::streamsize>(size))),
              "Truncated original ice region-prune mutation fixture");
        return raw;
    };
    const auto load32 = [](const auto &raw, std::size_t offset) {
        std::uint32_t value = 0;
        for (unsigned byte = 0; byte < 4; ++byte)
            value |= std::uint32_t(raw[offset + byte]) << (8 * byte);
        return value;
    };
    for (unsigned test_case = 0; test_case < 32; ++test_case) {
        const auto name_size = word(in), precision = word(in);
        const auto nregion = word(in), nvertex = word(in), nindex = word(in);
        const auto nselected = word(in);
        check(name_size <= 32 && (precision == 24 || precision == 53) &&
                  nregion <= 64 && nvertex <= 256 && nindex <= 256 &&
                  nselected <= nregion,
              "Invalid original ice region-prune mutation extent");
        const auto name = read_blob(name_size);
        check(!name.empty(), "Invalid original ice region-prune mutation case name");
        std::vector<std::uint32_t> selected(nselected);
        for (auto &region : selected) {
            region = word(in);
            check(region < nregion, "Invalid selected ice region-prune index");
        }
        const auto raw_regions = read_blob(16 * nregion);
        const auto raw_vertices = read_blob(16 * nvertex);
        const auto raw_indices = read_blob(12 * nindex);
        TerrainIceIntermediateRegionGeometry geometry;
        geometry.regions.reserve(nregion);
        geometry.vertices.reserve(nvertex);
        geometry.indices.reserve(nindex);
        geometry.incidence.resize(nvertex);
        for (std::size_t i = 0; i < nregion; ++i)
            geometry.regions.push_back({load32(raw_regions, 16 * i),
                                        load32(raw_regions, 16 * i + 4),
                                        load32(raw_regions, 16 * i + 8),
                                        raw_regions[16 * i + 12]});
        for (std::size_t i = 0; i < nvertex; ++i)
            geometry.vertices.push_back({
                std::bit_cast<float>(load32(raw_vertices, 16 * i)),
                std::bit_cast<float>(load32(raw_vertices, 16 * i + 4))});
        for (std::size_t i = 0; i < nindex; ++i) {
            const auto edge = std::array<std::uint32_t, 3>{
                load32(raw_indices, 12 * i),
                load32(raw_indices, 12 * i + 4),
                load32(raw_indices, 12 * i + 8)};
            check(edge[0] < nvertex && edge[1] < nvertex,
                  "Invalid original ice region-prune edge");
            geometry.indices.push_back(edge);
            geometry.incidence[edge[0]].push_back(static_cast<std::uint32_t>(i));
            geometry.incidence[edge[1]].push_back(static_cast<std::uint32_t>(i));
        }
        for (const auto region : selected) {
            prune_terrain_ice_intermediate_region(geometry, region);
            check(geometry.regions[region].vertex_count == 0,
                  "Original ice caller did not clear pruned region count");
            const auto expected_indices = read_blob(12 * nindex);
            for (std::size_t i = 0; i < nindex; ++i)
                for (unsigned field = 0; field < 3; ++field)
                    check(geometry.indices[i][field] ==
                              load32(expected_indices, 12 * i + 4 * field),
                          "Original ice pruned graph edge differs");
            for (std::size_t vertex = 0; vertex < nvertex; ++vertex) {
                const auto count = word(in);
                check(count <= 2 * nindex &&
                          count == geometry.incidence[vertex].size(),
                      "Original ice pruned graph degree differs");
                for (std::size_t i = 0; i < count; ++i)
                    check(word(in) == geometry.incidence[vertex][i],
                          "Original ice pruned graph incidence order differs");
            }
        }
    }
    check(in.peek() == std::char_traits<char>::eof(),
          "Trailing original ice region-prune mutation fixture bytes");
}
void ice_region_shrink_fixture(const char *path) {
    std::ifstream in(path, std::ios::binary);
    char magic[8];
    check(bool(in.read(magic, 8)) && std::string_view(magic, 8) == "C5ISHR01",
          "Invalid original ice region-shrink fixture");
    const auto exe_hash = fixed_bytes<32>(in), parent_hash = fixed_bytes<32>(in);
    const auto settings_hash = fixed_bytes<32>(in);
    const auto hex = [](const auto &bytes) {
        std::string result;
        static constexpr char digits[] = "0123456789abcdef";
        for (const auto byte : bytes) {
            result.push_back(digits[byte >> 4]);
            result.push_back(digits[byte & 15]);
        }
        return result;
    };
    check(hex(exe_hash) == "7c9f93d512c49a7ab2bc70f51c6caa10a82ddd697ac11d8a24ff1bb7ce7c3686" &&
              hex(parent_hash) == "9a7f4f1e9dd8a2bbfce59fe5f1f5c5a51cc3fa0f4da61287b9f77b7d76e9ec38" &&
              hex(settings_hash) == "ab594d596e9dc386ca2cabcd952799a97190093a70cbc3b8486c7b2593318f1b" &&
              word(in) == 32, "Original ice region-shrink source identity differs");
    const auto read_blob = [&](std::size_t size) {
        std::vector<std::uint8_t> raw(size);
        check(bool(in.read(reinterpret_cast<char *>(raw.data()),
                           static_cast<std::streamsize>(size))),
              "Truncated original ice region-shrink fixture");
        return raw;
    };
    const auto load32 = [](const auto &raw, std::size_t offset) {
        std::uint32_t value = 0;
        for (unsigned byte = 0; byte < 4; ++byte)
            value |= std::uint32_t(raw[offset + byte]) << (8 * byte);
        return value;
    };
    for (unsigned test_case = 0; test_case < 32; ++test_case) {
        const auto name_size = word(in), precision = word(in);
        const auto nregion = word(in), nvertex = word(in), nindex = word(in);
        const auto shrink_bits = word(in);
        check(name_size <= 32 && (precision == 24 || precision == 53) &&
                  nregion <= 64 && nvertex <= 256 && nindex <= 256 &&
                  shrink_bits == 0x3eb33333u,
              "Invalid original ice region-shrink extent or settings");
        const auto name = read_blob(name_size);
        check(!name.empty(), "Invalid original ice region-shrink case name");
        const auto raw_regions = read_blob(16 * nregion);
        const auto raw_vertices = read_blob(16 * nvertex);
        const auto raw_indices = read_blob(12 * nindex);
        const auto expected_xy = read_blob(8 * nvertex);
        TerrainIceIntermediateRegionGeometry geometry;
        geometry.regions.reserve(nregion);
        geometry.vertices.reserve(nvertex);
        geometry.indices.reserve(nindex);
        for (std::size_t i = 0; i < nregion; ++i)
            geometry.regions.push_back({load32(raw_regions, 16 * i),
                                        load32(raw_regions, 16 * i + 4),
                                        load32(raw_regions, 16 * i + 8),
                                        raw_regions[16 * i + 12]});
        for (std::size_t i = 0; i < nvertex; ++i)
            geometry.vertices.push_back({
                std::bit_cast<float>(load32(raw_vertices, 16 * i)),
                std::bit_cast<float>(load32(raw_vertices, 16 * i + 4))});
        for (std::size_t i = 0; i < nindex; ++i)
            geometry.indices.push_back({load32(raw_indices, 12 * i),
                                        load32(raw_indices, 12 * i + 4),
                                        load32(raw_indices, 12 * i + 8)});
        for (std::size_t i = 0; i < nregion; ++i)
            if (geometry.regions[i].vertex_count <= 16)
                shrink_terrain_ice_intermediate_region(
                    geometry, i, std::bit_cast<float>(shrink_bits), precision);
        for (std::size_t i = 0; i < nvertex; ++i)
            for (unsigned component = 0; component < 2; ++component)
                check(std::bit_cast<std::uint32_t>(geometry.vertices[i][component]) ==
                          load32(expected_xy, 8 * i + 4 * component),
                      "Original ice region-shrink vertex differs");
    }
    check(in.peek() == std::char_traits<char>::eof(),
          "Trailing original ice region-shrink fixture bytes");
}
void ice_region_geometry_fixture(const char *path) {
    std::ifstream in(path, std::ios::binary);
    char magic[8];
    check(bool(in.read(magic, 8)) && std::string_view(magic, 8) == "C5IGEO01",
          "Invalid original ice region-geometry fixture");
    const auto exe_hash = fixed_bytes<32>(in), function_hash = fixed_bytes<32>(in);
    const auto parent_hash = fixed_bytes<32>(in);
    const auto hex = [](const auto &bytes) {
        std::string result;
        static constexpr char digits[] = "0123456789abcdef";
        for (const auto byte : bytes) {
            result.push_back(digits[byte >> 4]);
            result.push_back(digits[byte & 15]);
        }
        return result;
    };
    check(hex(exe_hash) == "7c9f93d512c49a7ab2bc70f51c6caa10a82ddd697ac11d8a24ff1bb7ce7c3686" &&
              hex(function_hash) == "9ec4a25ce54ad3ebb19ad7c5a03e6d960ba4919b6b4ba38d1a4142277ea45eb3" &&
              hex(parent_hash) == "5882ee07b64129129bf90a66c8803f64d3b668d1883f81123ade136b8a4b6b63" &&
              word(in) == 26, "Original ice region-geometry source identity differs");
    const auto read_blob = [&](std::size_t size) {
        std::vector<std::uint8_t> raw(size);
        check(bool(in.read(reinterpret_cast<char *>(raw.data()),
                           static_cast<std::streamsize>(size))),
              "Truncated original ice region-geometry fixture");
        return raw;
    };
    const auto load32 = [](const std::vector<std::uint8_t> &raw, std::size_t offset) {
        std::uint32_t value = 0;
        for (unsigned i = 0; i < 4; ++i)
            value |= std::uint32_t(raw[offset + i]) << (8 * i);
        return value;
    };
    for (unsigned test_case = 0; test_case < 26; ++test_case) {
        const auto name_size = word(in), precision = word(in);
        const auto n20 = word(in), n12 = word(in), nregion = word(in);
        const auto nvertex = word(in), nindex = word(in);
        const auto edge_pull = std::bit_cast<float>(word(in));
        check(name_size <= 32 && (precision == 24 || precision == 53) &&
                  n20 <= 64 && n12 <= 64 && nregion <= 64 &&
                  nvertex <= 256 && nindex <= 256,
              "Invalid original ice region-geometry extent");
        const auto name = read_blob(name_size);
        check(!name.empty(), "Invalid original ice region-geometry case name");
        const auto input20 = read_blob(20 * n20), input12 = read_blob(12 * n12);
        const auto original_adj = read_blob(8 * n12), initial_flags = read_blob(2 * n12);
        const auto output_flags = read_blob(2 * n12), flag_mask = read_blob(2 * n12);
        const auto original_regions = read_blob(16 * nregion);
        const auto region_mask = read_blob(16 * nregion);
        const auto original_vertices = read_blob(16 * nvertex);
        const auto vertex_mask = read_blob(16 * nvertex);
        const auto original_indices = read_blob(12 * nindex);
        const auto index_mask = read_blob(12 * nindex);
        TerrainIceIntermediateFill fill;
        fill.points.resize(n20);
        fill.links.resize(n12);
        for (std::size_t i = 0; i < n20; ++i)
            fill.points[i].xy = {std::bit_cast<float>(load32(input20, 20 * i)),
                                 std::bit_cast<float>(load32(input20, 20 * i + 4))};
        for (std::size_t i = 0; i < n12; ++i)
            fill.links[i] = {load32(input12, 12 * i), load32(input12, 12 * i + 4),
                             input12[12 * i + 8]};
        build_terrain_ice_intermediate_adjacency(fill);
        sort_terrain_ice_intermediate_adjacency(fill);
        std::vector<std::uint8_t> actual_adj(8 * n12);
        for (std::size_t i = 0; i < fill.adjacency_link_indices.size(); ++i) {
            const auto value = 0x20400000u + 12 * fill.adjacency_link_indices[i];
            for (unsigned byte = 0; byte < 4; ++byte)
                actual_adj[4 * i + byte] = static_cast<std::uint8_t>(value >> (8 * byte));
        }
        check(actual_adj == original_adj, "Original ice region geometry input order differs");
        mark_terrain_ice_closed_walks(fill);
        check(fill.closed_walk_flags == initial_flags,
              "Original ice region geometry input flags differ");
        const auto regions = trace_terrain_ice_intermediate_regions(fill);
        check(fill.closed_walk_flags == output_flags && regions.size() == nregion,
              "Original ice region geometry walks differ");
        const auto geometry = build_terrain_ice_intermediate_region_geometry(
            fill, regions, edge_pull, precision);
        check(geometry.regions.size() == nregion && geometry.vertices.size() == nvertex &&
                  geometry.indices.size() == nindex,
              "Original ice region geometry counts differ");
        for (std::size_t i = 0; i < nregion; ++i) {
            const auto &record = geometry.regions[i];
            check(record.first_vertex == load32(original_regions, 16 * i) &&
                      record.first_index == load32(original_regions, 16 * i + 4) &&
                      record.vertex_count == load32(original_regions, 16 * i + 8) &&
                      record.boundary == original_regions[16 * i + 12],
                  "Original ice region record differs");
            for (unsigned byte = 0; byte < 13; ++byte)
                check(region_mask[16 * i + byte] == 1,
                      "Original ice region record byte was not written");
        }
        for (std::size_t i = 0; i < nvertex; ++i) {
            for (unsigned component = 0; component < 2; ++component) {
                const auto actual = std::bit_cast<std::uint32_t>(geometry.vertices[i][component]);
                const auto original = load32(original_vertices, 16 * i + 4 * component);
                if (actual != original)
                    throw std::runtime_error("Original ice smoothed vertex XY differs in case " +
                                             std::to_string(test_case) + " vertex " +
                                             std::to_string(i) + " component " +
                                             std::to_string(component) + ": " +
                                             std::to_string(actual) + " vs " +
                                             std::to_string(original));
                check(true, "Original ice smoothed vertex XY differs");
            }
            for (unsigned byte = 0; byte < 8; ++byte)
                check(vertex_mask[16 * i + byte] == 1,
                      "Original ice vertex XY byte was not written");
        }
        for (std::size_t i = 0; i < nindex; ++i) {
            for (unsigned component = 0; component < 3; ++component)
                check(geometry.indices[i][component] ==
                          load32(original_indices, 12 * i + 4 * component),
                      "Original ice region index differs");
            for (unsigned byte = 0; byte < 12; ++byte)
                check(index_mask[12 * i + byte] == 1,
                      "Original ice index byte was not written");
        }
        for (std::size_t i = 0; i < flag_mask.size(); ++i)
            check(flag_mask[i] <= 1 && (initial_flags[i] == output_flags[i] || flag_mask[i]),
                  "Original ice region geometry flag writes are inconsistent");
    }
    check(in.peek() == std::char_traits<char>::eof(),
          "Trailing original ice region-geometry fixture bytes");
}
void ice_region_fill_fixture(const char *path) {
    std::ifstream in(path, std::ios::binary);
    char magic[8];
    check(bool(in.read(magic, 8)) && std::string_view(magic, 8) == "C5IRFL01",
          "Invalid original ice intermediate-region fill fixture");
    const auto exe_hash = fixed_bytes<32>(in);
    std::string hash;
    static constexpr char digits[] = "0123456789abcdef";
    for (const auto byte : exe_hash) {
        hash.push_back(digits[byte >> 4]);
        hash.push_back(digits[byte & 15]);
    }
    check(hash == "7c9f93d512c49a7ab2bc70f51c6caa10a82ddd697ac11d8a24ff1bb7ce7c3686" &&
              word(in) == 6, "Original ice intermediate-region fill identity differs");
    const auto read_blob = [&](std::size_t size) {
        std::vector<std::uint8_t> raw(size);
        check(bool(in.read(reinterpret_cast<char *>(raw.data()),
                           static_cast<std::streamsize>(size))),
              "Truncated original ice intermediate-region fill fixture");
        return raw;
    };
    const auto load32 = [&](const std::vector<std::uint8_t> &raw, std::size_t offset) {
        check(offset + 4 <= raw.size(), "Invalid original ice fill field offset");
        std::uint32_t value = 0;
        for (unsigned i = 0; i < 4; ++i)
            value |= std::uint32_t(raw[offset + i]) << (8 * i);
        return value;
    };
    const auto write32 = [](std::vector<std::uint8_t> &raw,
                            std::vector<std::uint8_t> &mask,
                            std::size_t offset, std::uint32_t value) {
        for (unsigned i = 0; i < 4; ++i) {
            raw[offset + i] = static_cast<std::uint8_t>(value >> (8 * i));
            mask[offset + i] = 1;
        }
    };
    for (unsigned test_case = 0; test_case < 6; ++test_case) {
        const auto n24 = word(in), n36 = word(in), active = word(in);
        const auto count20 = word(in), count12 = word(in);
        check(n24 <= 100 && n36 <= 100 && count20 <= 200 && count12 <= 200,
              "Invalid original ice intermediate-region fill extent");
        const auto raw24 = read_blob(24 * n24), raw36 = read_blob(36 * n36);
        const auto source_map = read_blob(4 * n24);
        const auto original20 = read_blob(20 * count20);
        const auto original_mask20 = read_blob(20 * count20);
        const auto original12 = read_blob(12 * count12);
        const auto original_mask12 = read_blob(12 * count12);
        std::vector<TerrainIceIntermediateSourcePoint> points(n24);
        for (std::size_t i = 0; i < n24; ++i)
            points[i] = {{std::bit_cast<float>(load32(raw24, 24 * i)),
                          std::bit_cast<float>(load32(raw24, 24 * i + 4))},
                         raw24[24 * i + 8]};
        std::vector<TerrainIceIntermediateSourceEdge> edges(n36);
        for (std::size_t i = 0; i < n36; ++i)
            edges[i] = {load32(raw36, 36 * i), load32(raw36, 36 * i + 4),
                        raw36[36 * i + 8],
                        {std::bit_cast<float>(load32(raw36, 36 * i + 12)),
                         std::bit_cast<float>(load32(raw36, 36 * i + 16))}};
        const auto result = build_terrain_ice_intermediate_regions(points, edges);
        check(result.points.size() == count20 && result.links.size() == count12 &&
                  result.source24_to_active.size() == n24 && active <= count20,
              "Original ice intermediate-region fill counts differ");
        for (std::size_t i = 0; i < n24; ++i)
            check(result.source24_to_active[i] == load32(source_map, 4 * i),
                  "Original ice intermediate-region source index differs");
        std::vector<std::uint8_t> actual20(20 * count20, 0xcd), mask20(20 * count20);
        std::vector<std::uint8_t> actual12(12 * count12, 0xcd), mask12(12 * count12);
        for (std::size_t i = 0; i < result.points.size(); ++i) {
            write32(actual20, mask20, 20 * i,
                    std::bit_cast<std::uint32_t>(result.points[i].xy[0]));
            write32(actual20, mask20, 20 * i + 4,
                    std::bit_cast<std::uint32_t>(result.points[i].xy[1]));
            write32(actual20, mask20, 20 * i + 16, result.points[i].link_count);
        }
        for (std::size_t i = 0; i < result.links.size(); ++i) {
            write32(actual12, mask12, 12 * i, result.links[i].first);
            write32(actual12, mask12, 12 * i + 4, result.links[i].second);
            actual12[12 * i + 8] = result.links[i].boundary;
            mask12[12 * i + 8] = 1;
        }
        check(actual20 == original20 && mask20 == original_mask20,
              "Original ice intermediate 20-byte records differ");
        check(actual12 == original12 && mask12 == original_mask12,
              "Original ice intermediate 12-byte links differ");
    }
    check(in.peek() == std::char_traits<char>::eof(),
          "Trailing original ice intermediate-region fill bytes");
}
void ice_region_count_fixture(const char *path) {
    std::ifstream in(path, std::ios::binary);
    char magic[8];
    check(bool(in.read(magic, 8)) && std::string_view(magic, 8) == "C5IRCT01",
          "Invalid original ice intermediate-region count fixture");
    const auto exe_hash = fixed_bytes<32>(in);
    std::string hash;
    static constexpr char digits[] = "0123456789abcdef";
    for (const auto byte : exe_hash) {
        hash.push_back(digits[byte >> 4]);
        hash.push_back(digits[byte & 15]);
    }
    check(hash == "7c9f93d512c49a7ab2bc70f51c6caa10a82ddd697ac11d8a24ff1bb7ce7c3686" &&
              word(in) == 6, "Original ice intermediate-region count identity differs");
    for (unsigned test_case = 0; test_case < 6; ++test_case) {
        const auto n24 = word(in), n36 = word(in);
        const auto expected_active = word(in), expected20 = word(in), expected12 = word(in);
        check(n24 <= 100 && n36 <= 100, "Invalid original ice region count extent");
        std::vector<std::uint8_t> flags(n24);
        for (auto &flag : flags) flag = static_cast<std::uint8_t>(word(in, 1));
        std::vector<std::array<float, 2>> weights(n36);
        for (auto &pair : weights)
            pair = {std::bit_cast<float>(word(in)), std::bit_cast<float>(word(in))};
        const auto result = count_terrain_ice_intermediate_regions(flags, weights);
        check(result.active_source24 == expected_active &&
                  result.records20 == expected20 && result.links12 == expected12 &&
                  result.source24_to_active.size() == n24,
              "Original ice intermediate-region allocation counts differ");
        for (const auto mapping : result.source24_to_active)
            check(mapping == word(in), "Original ice source24 index map differs");
    }
    check(in.peek() == std::char_traits<char>::eof(),
          "Trailing original ice intermediate-region count bytes");
}
void ice_region_prune_rng_fixture(const char *path) {
    std::ifstream in(path, std::ios::binary);
    char magic[8];
    check(bool(in.read(magic, 8)) && std::string_view(magic, 8) == "C5IRNG01",
          "Invalid original ice region-pruning RNG fixture");
    const auto exe_hash = fixed_bytes<32>(in), settings_hash = fixed_bytes<32>(in);
    const auto hex = [](const auto &bytes) {
        std::string result;
        static constexpr char digits[] = "0123456789abcdef";
        for (const auto byte : bytes) {
            result.push_back(digits[byte >> 4]);
            result.push_back(digits[byte & 15]);
        }
        return result;
    };
    check(hex(exe_hash) == "7c9f93d512c49a7ab2bc70f51c6caa10a82ddd697ac11d8a24ff1bb7ce7c3686" &&
              hex(settings_hash) == "ab594d596e9dc386ca2cabcd952799a97190093a70cbc3b8486c7b2593318f1b" &&
              word(in) == 128, "Original ice region-pruning RNG identity differs");
    auto random = initial_terrain_ice_random_state();
    for (std::uint32_t index = 0; index < 128; ++index) {
        const auto precision = word(in), boundary = word(in);
        const auto before = word(in), after = word(in);
        const auto draw_bits = word(in), killed = word(in);
        if (index == 64) random = initial_terrain_ice_random_state();
        check(precision == (index < 64 ? 24u : 53u) && boundary == (index % 2 == 0) &&
                  killed <= 1 && random.value == before,
              "Original ice region-pruning draw order differs");
        auto decision_random = random;
        const auto draw = draw_terrain_ice_region_prune_random(random, precision);
        check(random.value == after && std::bit_cast<std::uint32_t>(draw) == draw_bits,
              "Original ice region-pruning random Float32 differs");
        const auto prune = should_prune_terrain_ice_region_record(decision_random,
            16, boundary != 0, 0.6f, 0.05f, precision);
        check(prune == (killed != 0) && decision_random.value == after &&
                  decision_random.draws == random.draws,
              "Original ice region-pruning kill decision differs");
    }
    check(in.peek() == std::char_traits<char>::eof() &&
              random.value == 2219737664u && random.draws == 64,
          "Original ice region-pruning RNG tail differs");
    const auto before_skip = random;
    check(!should_prune_terrain_ice_region_record(random, 17, true, 0.6f, 0.05f) &&
              random.value == before_skip.value && random.draws == before_skip.draws,
          "Original large ice region consumed a prune draw");
    rejects([&] { auto r = initial_terrain_ice_random_state();
                  draw_terrain_ice_region_prune_random(r, 64); });
    rejects([&] { auto r = initial_terrain_ice_random_state(); r.modulus = 0;
                  draw_terrain_ice_region_prune_random(r, 53); });
}
void ice_saved_projection_fixture(const char *contour_path, const char *vertex_path,
                                  std::uint32_t precision, std::uint32_t seed,
                                  std::uint32_t expected_rng_after,
                                  std::uint32_t expected_vertices = 1922,
                                  std::uint32_t expected_rng_draws = 636,
                                  std::string_view expected_contour_sha =
                                      "862f218e8d7a1152076f02b063e3216c826f1458f3afce5c68579e40636e6a56",
                                  std::array<std::uint32_t, 4> expected_bounds =
                                      {3298033536u, 3292496278u, 1157627318u, 1144105818u}) {
    const auto contours = read_ice_contour_geometry(contour_path);
    std::ifstream in(vertex_path, std::ios::binary);
    char magic[8], exe_hash[32], contour_hash[32], settings_hash[32];
    check(bool(in.read(magic, 8)) && std::string_view(magic, 8) == "C5IVTX01" &&
              bool(in.read(exe_hash, 32)) && bool(in.read(contour_hash, 32)) &&
              bool(in.read(settings_hash, 32)),
          "Invalid selected-projection ice mesh fixture");
    const auto hex = [](const char *bytes) {
        std::string result;
        static constexpr char digits[] = "0123456789abcdef";
        for (unsigned i = 0; i < 32; ++i) {
            const auto byte = static_cast<std::uint8_t>(bytes[i]);
            result.push_back(digits[byte >> 4]);
            result.push_back(digits[byte & 15]);
        }
        return result;
    };
    check(hex(exe_hash) == "7c9f93d512c49a7ab2bc70f51c6caa10a82ddd697ac11d8a24ff1bb7ce7c3686" &&
              hex(contour_hash) == expected_contour_sha &&
              hex(settings_hash) == "fb38d6facb2c7feacff64f31ad50d9aebd5ae77dd6bcaa9b2154530e7ead2ba9" &&
              word(in) == 12 && word(in) == seed && word(in) == expected_vertices &&
              word(in) == precision,
          "Selected-projection ice mesh identity differs");
    std::vector<std::array<std::uint32_t, 2>> original(expected_vertices);
    for (auto &point : original) point = {word(in), word(in)};
    const auto world = build_world_grid_layout(40, 24, 1, {}).source_grid;
    const auto projection = selected_save_ice_projection();
    const auto curve = build_terrain_ice_curve_parameters(world, projection, 12);
    TerrainIceRandomState random = seed == 0 ? initial_terrain_ice_random_state() :
        TerrainIceRandomState{seed, 0, seed, 0xffffcfefu, 2.3283131071116735e-10};
    check(random.value == seed && random.draws == 0 &&
              random.initial_value == seed && random.modulus == 0xffffcfefu &&
              std::bit_cast<std::uint64_t>(random.inverse_modulus) ==
                  std::bit_cast<std::uint64_t>(2.3283131071116735e-10),
          "Selected-projection initial ice RNG differs");
    const auto mesh = build_terrain_ice_mesh_geometry(
        contours, curve, projection, random, precision);
    check(mesh.vertices.size() == original.size() && mesh.flags.size() == original.size(),
          "Selected-projection ice mesh extent differs");
    for (std::size_t i = 0; i < original.size(); ++i)
        for (unsigned coordinate = 0; coordinate < 2; ++coordinate)
            check(std::bit_cast<std::uint32_t>(mesh.vertices[i][coordinate]) ==
                      original[i][coordinate],
                  "Selected-projection ice mesh Float32 coordinate differs");
    for (const auto flag : mesh.flags)
        check(flag == word(in, 1), "Selected-projection ice mesh flag differs");
    check(in.peek() == std::char_traits<char>::eof() &&
              random.value == expected_rng_after && random.draws == expected_rng_draws,
          "Selected-projection ice mesh tail or RNG differs");
    const auto bounds = bound_terrain_ice_mesh_geometry(mesh.vertices);
    for (std::size_t i = 0; i < bounds.size(); ++i)
        check(std::bit_cast<std::uint32_t>(bounds[i]) == expected_bounds[i],
              "Selected-projection ice bounds differ");
}
void ice_group_projection_fixture(const char *contour_path, const char *projected_path,
                                  std::uint32_t expected_groups,
                                  std::uint32_t expected_vertices,
                                  std::string_view expected_contour_sha,
                                  const TerrainIceProjection &projection) {
    const auto contours = read_ice_contour_geometry(contour_path);
    std::ifstream in(projected_path, std::ios::binary);
    char magic[8], exe_hash[32], contour_hash[32];
    check(bool(in.read(magic, 8)) && std::string_view(magic, 8) == "C5IGPX01" &&
              bool(in.read(exe_hash, 32)) && bool(in.read(contour_hash, 32)),
          "Invalid original ice group projection fixture");
    const auto hex = [](const char *bytes) {
        std::string result;
        static constexpr char digits[] = "0123456789abcdef";
        for (unsigned i = 0; i < 32; ++i) {
            const auto byte = static_cast<std::uint8_t>(bytes[i]);
            result.push_back(digits[byte >> 4]);
            result.push_back(digits[byte & 15]);
        }
        return result;
    };
    check(hex(exe_hash) == "7c9f93d512c49a7ab2bc70f51c6caa10a82ddd697ac11d8a24ff1bb7ce7c3686" &&
              hex(contour_hash) == expected_contour_sha,
          "Original ice group projection identity differs");
    check(word(in) == expected_groups && word(in) == expected_vertices &&
              contours.groups.size() == expected_groups &&
              contours.vertices.size() == expected_vertices,
          "Original ice group projection extent differs");
    const auto projected = project_terrain_ice_group_vertices(
        contours, projection);
    check(projected.size() == expected_vertices,
          "Projected ice group vertex count differs");
    for (std::size_t i = 0; i < projected.size(); ++i)
        for (unsigned coordinate = 0; coordinate < 2; ++coordinate)
            check(std::bit_cast<std::uint32_t>(projected[i][coordinate]) == word(in),
                  "Original ice group projection coordinate differs");
    check(in.peek() == std::char_traits<char>::eof(),
          "Trailing original ice group projection bytes");
}
void ice_exterior_fixture(const char *contour_path, const char *vertex_path,
                          const char *exterior_path, std::uint32_t expected_count,
                          std::uint32_t expected_exterior_count,
                          std::uint32_t expected_precision,
                          std::uint32_t expected_rng_value,
                          std::uint32_t expected_rng_draws,
                          std::array<std::uint32_t, 4> expected_bounds,
                          std::string_view expected_contour_sha) {
    const auto contours = read_ice_contour_geometry(contour_path);
    std::ifstream in(vertex_path, std::ios::binary);
    char magic[8], exe_hash[32], contour_hash[32], settings_hash[32];
    check(bool(in.read(magic, 8)) && std::string_view(magic, 8) == "C5IVTX01" &&
              bool(in.read(exe_hash, 32)) && bool(in.read(contour_hash, 32)) &&
              bool(in.read(settings_hash, 32)),
          "Invalid original ice vertex fixture header");
    const auto hex = [](const char *bytes) {
        std::string result;
        static constexpr char digits[] = "0123456789abcdef";
        for (unsigned i = 0; i < 32; ++i) {
            const auto byte = static_cast<std::uint8_t>(bytes[i]);
            result.push_back(digits[byte >> 4]);
            result.push_back(digits[byte & 15]);
        }
        return result;
    };
    check(hex(exe_hash) == "7c9f93d512c49a7ab2bc70f51c6caa10a82ddd697ac11d8a24ff1bb7ce7c3686" &&
              hex(contour_hash) == expected_contour_sha &&
              hex(settings_hash) == "c658114c59fd8099f897ac2e174bb0534927d5716294772dcda3771375728774" &&
              word(in) == 12 && word(in) == 12345 && word(in) == expected_count &&
              word(in) == expected_precision,
          "Original ice vertex fixture identity differs");
    std::vector<std::array<std::uint32_t, 2>> original(expected_count);
    for (auto &vertex : original) vertex = {word(in), word(in)};
    const auto plan = plan_terrain_ice_mesh_flags(contours, 12);
    check(plan.flags.size() == expected_count,
          "Ice exterior geometry and original vertex count differ");
    for (const auto flag : plan.flags)
        check(word(in, 1) == flag, "Original ice vertex flag differs");
    check(in.peek() == std::char_traits<char>::eof(),
          "Trailing original ice vertex fixture bytes");
    const auto projection = native_ice_projection();
    const auto world = build_world_grid_layout(40, 24, 1, {}).source_grid;
    const auto curve = build_terrain_ice_curve_parameters(world, projection, 12);
    const auto exterior = build_terrain_ice_exterior_vertices(contours, curve, projection);
    check(exterior.size() == static_cast<std::size_t>(
              std::count(plan.flags.begin(), plan.flags.end(), 0)),
          "Ice exterior vertex count differs from original flags");
    for (const auto &vertex : exterior) {
        check(vertex.index < expected_count && plan.flags[vertex.index] == 0,
              "Ice exterior vertex index differs");
        for (unsigned coordinate = 0; coordinate < 2; ++coordinate) {
            const auto actual = std::bit_cast<std::uint32_t>(vertex.position[coordinate]);
            if (actual != original[vertex.index][coordinate])
                throw std::runtime_error("Original ice exterior vertex " +
                    std::to_string(vertex.index) + " float " + std::to_string(coordinate) +
                    ": expected bits " + std::to_string(original[vertex.index][coordinate]) +
                    ", actual " + std::to_string(actual));
            ++checks;
        }
    }
    std::ifstream exterior_in(exterior_path, std::ios::binary);
    check(bool(exterior_in.read(magic, 8)) &&
              std::string_view(magic, 8) == "C5IEXT02" &&
              bool(exterior_in.read(exe_hash, 32)) &&
              bool(exterior_in.read(contour_hash, 32)) &&
              bool(exterior_in.read(settings_hash, 32)),
          "Invalid original ice exterior-edge fixture header");
    check(hex(exe_hash) == "7c9f93d512c49a7ab2bc70f51c6caa10a82ddd697ac11d8a24ff1bb7ce7c3686" &&
              hex(contour_hash) == expected_contour_sha &&
              hex(settings_hash) == "c658114c59fd8099f897ac2e174bb0534927d5716294772dcda3771375728774" &&
              word(exterior_in) == 12 && word(exterior_in) == 12345 &&
              word(exterior_in) == expected_exterior_count &&
              word(exterior_in) == 53 && exterior.size() == expected_exterior_count,
          "Original ice exterior-edge fixture identity differs");
    static constexpr std::array<std::uint32_t, 6> native_corners{2, 4, 0, 5, 1, 3};
    for (const auto &vertex : exterior) {
        const auto edge_index = word(exterior_in), vertex_index = word(exterior_in);
        const auto edge = control_edge(exterior_in);
        const auto corner = word(exterior_in);
        for (unsigned coordinate = 0; coordinate < 3; ++coordinate) (void)word(exterior_in);
        const auto x = word(exterior_in), y = word(exterior_in);
        check(edge_index == vertex.source_edge && vertex_index == vertex.index &&
                  edge_index < contours.edges.size() &&
                  edge.cube.x == contours.edges[edge_index].cube.x &&
                  edge.cube.y == contours.edges[edge_index].cube.y &&
                  edge.cube.z == contours.edges[edge_index].cube.z &&
                  edge.side == contours.edges[edge_index].side &&
                  corner == native_corners[edge.side] &&
                  x == std::bit_cast<std::uint32_t>(vertex.position[0]) &&
                  y == std::bit_cast<std::uint32_t>(vertex.position[1]),
              "Original ice exterior edge-to-vertex mapping differs");
    }
    check(exterior_in.peek() == std::char_traits<char>::eof(),
          "Trailing original ice exterior-edge fixture bytes");
    TerrainIceRandomState random{12345, 0, 12345, 0xffffcfefu,
                                 2.3283131071116735e-10};
    const auto mesh = build_terrain_ice_mesh_geometry(
        contours, curve, projection, random, expected_precision);
    check(mesh.vertices.size() == expected_count && mesh.flags == plan.flags &&
              mesh.segments.size() == plan.segments.size(),
          "Complete ice mesh layout differs from original");
    for (std::size_t i = 0; i < plan.segments.size(); ++i)
        check(mesh.segments[i].first_vertex == plan.segments[i].first_vertex &&
                  mesh.segments[i].vertex_count == plan.segments[i].vertex_count,
              "Complete ice mesh segment differs from original");
    for (std::size_t i = 0; i < expected_count; ++i)
        for (unsigned coordinate = 0; coordinate < 2; ++coordinate) {
            const auto actual = std::bit_cast<std::uint32_t>(mesh.vertices[i][coordinate]);
            if (actual != original[i][coordinate])
                throw std::runtime_error("Original complete ice mesh vertex " +
                    std::to_string(i) + " float " + std::to_string(coordinate) +
                    ": expected bits " + std::to_string(original[i][coordinate]) +
                    ", actual " + std::to_string(actual));
            ++checks;
        }
    const auto bounds = bound_terrain_ice_mesh_geometry(mesh.vertices);
    for (std::size_t i = 0; i < bounds.size(); ++i)
        check(std::bit_cast<std::uint32_t>(bounds[i]) == expected_bounds[i],
              "Original complete ice mesh bounds differ");
    const auto empty_bounds = bound_terrain_ice_mesh_geometry({});
    check(std::bit_cast<std::uint32_t>(empty_bounds[0]) == 0x7f7fffffu &&
              std::bit_cast<std::uint32_t>(empty_bounds[1]) == 0x7f7fffffu &&
              std::bit_cast<std::uint32_t>(empty_bounds[2]) == 0xff7fffffu &&
              std::bit_cast<std::uint32_t>(empty_bounds[3]) == 0xff7fffffu,
          "Original empty ice mesh bounds differ");
    check(random.value == expected_rng_value && random.draws == expected_rng_draws &&
              random.initial_value == 12345 && random.modulus == 0xffffcfefu &&
              std::bit_cast<std::uint64_t>(random.inverse_modulus) ==
                  std::bit_cast<std::uint64_t>(2.3283131071116735e-10),
          "Complete ice mesh random state differs from original");
}
TerrainAuxiliaryOwnerState prior() {
    TerrainAuxiliaryOwnerState p{};
    p.relief_enabled = p.water_enabled = p.flood_state_flag = true;
    const auto source = build_world_grid_layout(3, 5, 1, {}).source_grid;
    p.relief.coordinates = p.adjacency.coordinates = p.water.coordinates = p.ice.coordinates = p.flood.coordinates = source;
    storage(p.relief, 11); storage(p.adjacency, 29); storage(p.water, 47); storage(p.ice, 65); storage(p.flood, 83);
    return p;
}
void guards() {
    auto p = prior();
    TerrainAuxiliarySources sources;
    const auto before = bytes(p.ice);
    sources.feature0.push_back({0, -1, 1});
    rejects([&] { apply_terrain_auxiliary_flags(p, sources); });
    check(bytes(p.ice) == before, "Rejected update mutated prior owner");
    sources = {};
    sources.feature4.push_back({-1, 0, 1});
    const auto changed = apply_terrain_auxiliary_flags(p, sources);
    check(!changed.flood_state_flag && bytes(changed.flood) == bytes(p.flood),
          "Rejected flood position must still clear the owner flag");
    auto disabled = prior();
    disabled.relief_enabled = disabled.water_enabled = false;
    disabled.relief.coordinates = disabled.adjacency.coordinates = disabled.water.coordinates = {};
    TerrainAuxiliarySources ignored;
    ignored.relief.push_back({-32768, -32768, 0});
    const auto untouched = apply_terrain_auxiliary_flags(disabled, ignored);
    check(bytes(untouched.relief) == bytes(disabled.relief) && bytes(untouched.water) == bytes(disabled.water),
          "Disabled handlers read uninitialized coordinate headers");
    disabled.water_enabled = true;
    ignored.relief.front().kind = 2;
    check(bytes(apply_terrain_auxiliary_flags(disabled, ignored).water) == bytes(disabled.water),
          "Non-water records must not access the water grid header");
    p.relief.cells.pop_back();
    rejects([&] { apply_terrain_auxiliary_flags(p, {}); });
    rejects([&] { initialize_terrain_ice_grid(p.ice, 256, 1, {}); });
    rejects([&] { initialize_terrain_ice_grid(p.ice, 1, 1, {1, 0, 0, 0, 0}); });
    p.ice.coordinates = {};
    const auto initialized = initialize_terrain_ice_grid(p.ice, 3, 5, {});
    check(initialized.coordinates.width == 3 && initialized.coordinates.height == 5 &&
              initialized.cells[0][0] == 0 && initialized.cells.back() == p.ice.cells.back(),
          "Fresh ice initialization must replace the old header and retain inactive storage");
    p.ice.cells.pop_back();
    rejects([&] { initialize_terrain_ice_grid(p.ice, 3, 5, {}); });
    auto adjacency = prior().adjacency;
    adjacency.cells.pop_back();
    rejects([&] { finalize_terrain_auxiliary_adjacency(adjacency); });
    TerrainAuxiliaryTwoByteGrid two_byte{};
    two_byte.cells.resize(TerrainAuxiliaryTwoByteGrid::capacity);
    adjacency.cells.resize(TerrainAdjacencyGrid::capacity);
    two_byte.cells.pop_back();
    rejects([&] { initialize_terrain_auxiliary_tables(two_byte, adjacency, {}); });
}
void ice_input() {
    auto state = prior();
    state.ice = initialize_terrain_ice_grid(state.ice, 3, 5, {});
    state.ice.wrap_x = false;
    TerrainAuxiliarySources adjacent;
    adjacent.feature0 = {{0, 0, 0}, {1, 0, -1}};
    const auto marked = apply_terrain_auxiliary_flags(state, adjacent);
    const auto before = bytes(marked.ice);
    const auto built = build_terrain_ice_input(marked.ice);
    check(built.ice_cells == 2 && built.exposed_sides == 10 &&
              built.grid.cells[0][0] == 0xef && built.grid.cells[1][0] == 0xfd,
          "Native ice input did not mark adjacent hex boundaries");
    check(bytes(marked.ice) == before,
          "Native ice input preparation mutated prior occupancy");
    const auto grouped = group_terrain_ice_input(built);
    check(grouped.groups.size() == 1 && grouped.groups[0].first_vertex == 0 &&
              grouped.groups[0].vertex_count == 2 && grouped.vertices.size() == 2 &&
              grouped.boundary_flags.size() == 2 &&
              grouped.vertices[0].x == 0 && grouped.vertices[0].y == 0 &&
              grouped.vertices[1].x == 1 && grouped.vertices[1].y == 0 &&
              (grouped.grid.cells[0][0] & 0x40u) == 0 &&
              (grouped.grid.cells[1][0] & 0x40u) == 0,
          "Native ice grouping lost adjacent hex order or marks");
    check((built.grid.cells[0][0] & 0x40u) != 0,
          "Native ice grouping mutated its input");

    state.ice.wrap_x = true;
    TerrainAuxiliarySources across_wrap;
    across_wrap.feature0 = {{0, 0, 0}, {2, 0, -2}};
    const auto wrapped = build_terrain_ice_input(
        apply_terrain_auxiliary_flags(state, across_wrap).ice);
    check(wrapped.ice_cells == 2 && wrapped.exposed_sides == 10 &&
              wrapped.grid.cells[0][0] == 0xfd && wrapped.grid.cells[2][0] == 0xef,
          "Native ice input did not wrap opposite map edges");
    const auto wrapped_groups = group_terrain_ice_input(wrapped);
    check(wrapped_groups.groups.size() == 1 && wrapped_groups.groups[0].vertex_count == 2,
          "Native ice grouping did not cross the horizontal wrap seam");
    const auto wrapped_contours = build_terrain_ice_contours_mode1(wrapped_groups);
    check(wrapped_contours.groups.size() == 1 && !wrapped_contours.segments.empty() &&
              wrapped_contours.edges.size() == wrapped.exposed_sides,
          "Native ice contour traversal lost the horizontal seam");
    auto broken_contours = wrapped_contours;
    broken_contours.segments.front().first_edge = 1;
    rejects([&] { count_terrain_ice_allocation(broken_contours, 12); });
    rejects([&] { plan_terrain_ice_mesh_flags(wrapped_contours, 0); });
    state.ice.cells.pop_back();
    rejects([&] { build_terrain_ice_input(state.ice); });
}
void synthetic_wrap_row_fixture(const char *path) {
    auto state = prior();
    state.ice = initialize_terrain_ice_grid(state.ice, 40, 24, {});
    state.ice.wrap_x = true;
    TerrainAuxiliarySources sources;
    for (std::int32_t x = 0; x < 40; ++x) sources.feature0.push_back({x, 0, -x});
    const auto marked = apply_terrain_auxiliary_flags(state, sources);
    const auto built = build_terrain_ice_input(marked.ice);
    check(built.ice_cells == 40 && built.exposed_sides == 160,
          "Synthetic wrap-row ice input differs");
    const auto grouped = group_terrain_ice_input(built);
    check(grouped.groups.size() == 1 && grouped.groups[0].vertex_count == 40,
          "Synthetic wrap-row grouping differs");
    for (std::int32_t x = 0; x < 40; ++x) {
        const auto cube_x = x == 39 ? -1 : x;
        check(grouped.vertices[x].x == cube_x && grouped.vertices[x].y == 0 &&
                  grouped.vertices[x].z == -cube_x && grouped.boundary_flags[x] == 1,
              "Synthetic wrap-row grouping order differs");
    }
    const auto contours = build_terrain_ice_contours_mode1(grouped);
    const auto allocation8 = count_terrain_ice_allocation(contours, 8);
    const auto allocation12 = count_terrain_ice_allocation(contours, 12);
    check(allocation8.in_bounds_edges == 82 && allocation8.vertices == 736 &&
              allocation12.in_bounds_edges == 82 && allocation12.vertices == 1064,
          "Synthetic wrap-row ice allocation differs from original 87AAC0");
    mesh_flags(contours, 8, {{{1, 656}, {0, 80}}});
    mesh_flags(contours, 12, {{{1, 984}, {0, 80}}});
    std::ifstream in(path, std::ios::binary);
    char magic[8];
    check(bool(in.read(magic, 8)) && std::string_view(magic, 8) == "C5ICCT01",
          "Invalid synthetic native ice contour fixture");
    char identity[32];
    check(bool(in.read(identity, 32)), "Truncated synthetic ice identity");
    const auto mode = word(in), group_count = word(in), vertex_count = word(in);
    const auto segment_count = word(in), edge_count = word(in);
    check(mode == 1 && group_count == 1 && vertex_count == 40 &&
              segment_count == 1 && edge_count == 162 &&
              contours.groups.size() == group_count &&
              contours.vertices.size() == vertex_count &&
              contours.segments.size() == segment_count &&
              contours.edges.size() == edge_count,
          "Synthetic native seam reconstruction counts differ");
    for (const auto &group : contours.groups)
        check(word(in) == group.first_segment && word(in) == group.first_vertex &&
                  word(in) == group.segment_count && word(in) == group.vertex_count,
              "Synthetic native ice contour group differs");
    for (const auto &p : contours.vertices)
        check(integer(in) == p.x && integer(in) == p.y && integer(in) == p.z,
              "Synthetic native ice contour vertex differs");
    for (const auto flag : contours.boundary_flags)
        check(word(in, 1) == flag, "Synthetic native ice boundary flag differs");
    for (const auto &segment : contours.segments)
        check(word(in) == segment.first_edge && word(in) == segment.edge_count,
              "Synthetic native ice contour segment differs");
    for (const auto &edge : contours.edges) {
        check(integer(in) == edge.cube.x && integer(in) == edge.cube.y &&
                  integer(in) == edge.cube.z && word(in, 1) == edge.side,
              "Synthetic native ice ordered edge differs");
        in.ignore(3);
        check(in.gcount() == 3, "Truncated synthetic native ice edge padding");
    }
    for (std::int32_t y = 0; y < 24; ++y)
        for (std::int32_t x = 0; x < 40; ++x)
            check(word(in, 1) == contours.grid.cells[y * TerrainIceGrid::stride + x][0],
                  "Synthetic native ice contour owner byte differs");
    check(in.peek() == std::char_traits<char>::eof(),
          "Trailing synthetic native ice contour fixture bytes");
    std::cout << "Synthetic wrap-row ice contours=" << segment_count << " segment, "
              << edge_count << " edges\n";
}
void synthetic_ring_fixture(const char *path) {
    auto state = prior();
    state.ice = initialize_terrain_ice_grid(state.ice, 40, 24, {});
    state.ice.wrap_x = true;
    TerrainAuxiliarySources sources;
    static constexpr LogicalHex directions[]{
        {0, 1, -1}, {-1, 0, 1}, {1, -1, 0},
        {0, -1, 1}, {1, 0, -1}, {-1, 1, 0}};
    for (const auto d : directions)
        sources.feature0.push_back({15 + d.x, 10 + d.y, -25 + d.z});
    const auto marked = apply_terrain_auxiliary_flags(state, sources);
    const auto built = build_terrain_ice_input(marked.ice);
    check(built.ice_cells == 6 && built.exposed_sides == 24,
          "Synthetic ice-ring input differs");
    const auto grouped = group_terrain_ice_input(built);
    check(grouped.groups.size() == 1 && grouped.groups[0].vertex_count == 6,
          "Synthetic ice-ring grouping differs");
    const auto contours = build_terrain_ice_contours_mode1(grouped);
    const auto allocation8 = count_terrain_ice_allocation(contours, 8);
    const auto allocation12 = count_terrain_ice_allocation(contours, 12);
    check(allocation8.in_bounds_edges == 24 && allocation8.vertices == 192 &&
              allocation12.in_bounds_edges == 24 && allocation12.vertices == 288,
          "Synthetic ice-ring allocation differs from original 87AAC0");
    mesh_flags(contours, 8, {{{1, 48}}, {{1, 144}}});
    mesh_flags(contours, 12, {{{1, 72}}, {{1, 216}}});
    std::ifstream in(path, std::ios::binary);
    char magic[8], identity[32];
    check(bool(in.read(magic, 8)) && std::string_view(magic, 8) == "C5ICCT01" &&
              bool(in.read(identity, 32)), "Invalid synthetic ice-ring contour fixture");
    const auto mode = word(in), group_count = word(in), vertex_count = word(in);
    const auto segment_count = word(in), edge_count = word(in);
    check(mode == 1 && group_count == 1 && vertex_count == 6 &&
              segment_count == 2 && edge_count == 24 &&
              contours.groups.size() == group_count &&
              contours.vertices.size() == vertex_count &&
              contours.segments.size() == segment_count &&
              contours.edges.size() == edge_count,
          "Synthetic native ice-ring contour counts differ");
    for (const auto &group : contours.groups)
        check(word(in) == group.first_segment && word(in) == group.first_vertex &&
                  word(in) == group.segment_count && word(in) == group.vertex_count,
              "Synthetic native ice-ring group differs");
    for (const auto &p : contours.vertices)
        check(integer(in) == p.x && integer(in) == p.y && integer(in) == p.z,
              "Synthetic native ice-ring vertex differs");
    for (const auto flag : contours.boundary_flags)
        check(word(in, 1) == flag, "Synthetic native ice-ring flag differs");
    for (const auto &segment : contours.segments)
        check(word(in) == segment.first_edge && word(in) == segment.edge_count,
              "Synthetic native ice-ring segment differs");
    for (const auto &edge : contours.edges) {
        check(integer(in) == edge.cube.x && integer(in) == edge.cube.y &&
                  integer(in) == edge.cube.z && word(in, 1) == edge.side,
              "Synthetic native ice-ring edge differs");
        in.ignore(3);
        check(in.gcount() == 3, "Truncated synthetic native ice-ring edge padding");
    }
    for (std::int32_t y = 0; y < 24; ++y)
        for (std::int32_t x = 0; x < 40; ++x)
            check(word(in, 1) == contours.grid.cells[y * TerrainIceGrid::stride + x][0],
                  "Synthetic native ice-ring owner byte differs");
    check(in.peek() == std::char_traits<char>::eof(),
          "Trailing synthetic native ice-ring fixture bytes");
    std::cout << "Synthetic ice-ring contours=" << segment_count << " segments, "
              << edge_count << " edges\n";
}
void selected_ice_fixture(const char *path, const char *group_path = nullptr,
                          const char *contour_path = nullptr,
                          std::uint32_t expected_inside_edges = 150,
                          std::uint32_t expected_vertices8 = 1322,
                          std::uint32_t expected_vertices12 = 1922,
                          bool check_legacy_flag_runs = true) {
    std::ifstream in(path, std::ios::binary);
    char magic[8];
    check(bool(in.read(magic, 8)) && std::string_view(magic, 8) == "C5ICIN01",
          "Invalid selected-save ice input fixture");
    char save_hash[32];
    check(bool(in.read(save_hash, 32)), "Truncated selected-save ice identity");
    const auto width = word(in, 2), height = word(in, 2), wrap = word(in, 1);
    in.ignore(3);
    check(in.gcount() == 3, "Truncated selected-save ice fixture header");
    const auto count = word(in);
    check(width > 0 && width <= 255 && height > 0 && height <= 255 && wrap <= 1 &&
              count > 0 && count <= width * height,
          "Invalid selected-save ice input extent");
    std::vector<std::uint8_t> expected_bytes(width * height, 0);
    std::vector<TerrainAuxiliaryPlot> plots(width * height, {1, 1, 0, -1});
    std::uint32_t previous = 0, exposed = 0;
    for (std::uint32_t index = 0; index < count; ++index) {
        const auto x = word(in, 2), y = word(in, 2), mask = word(in, 1);
        in.ignore(3);
        check(in.gcount() == 3, "Truncated selected-save ice input cell");
        const auto position = y * width + x;
        check(x < width && y < height && mask <= 0x3f &&
                  (index == 0 || position > previous),
              "Invalid selected-save ice input cell order");
        previous = position;
        exposed += std::popcount(mask);
        expected_bytes[position] = static_cast<std::uint8_t>(0xc0u | mask);
        plots[x * height + y].feature = 0;
    }
    check(in.peek() == std::char_traits<char>::eof(),
          "Trailing selected-save ice input bytes");

    const auto source_grid = build_world_grid_layout(width, height, 1, {}).source_grid;
    const auto sources = build_terrain_auxiliary_sources(source_grid, plots);
    check(sources.feature0.size() == count,
          "Selected-save ice plots did not enter the native feature-zero stream");
    auto state = prior();
    state.ice = initialize_terrain_ice_grid(state.ice, width, height, {});
    state.ice.wrap_x = wrap != 0;
    const auto marked = apply_terrain_auxiliary_flags(state, sources);
    const auto built = build_terrain_ice_input(marked.ice);
    check(built.ice_cells == count && built.exposed_sides == exposed,
          "Selected-save native ice input counts differ from independent topology");
    for (std::uint32_t y = 0; y < height; ++y)
        for (std::uint32_t x = 0; x < width; ++x)
            check(built.grid.cells[y * TerrainIceGrid::stride + x][0] ==
                      expected_bytes[y * width + x],
                  "Selected-save native ice input byte differs");
    if (group_path) {
        std::ifstream group_in(group_path, std::ios::binary);
        char group_magic[8], group_hash[32];
        check(bool(group_in.read(group_magic, 8)) &&
                  std::string_view(group_magic, 8) == "C5ICGR01" &&
                  bool(group_in.read(group_hash, 32)) &&
                  std::string_view(group_hash, 32) == std::string_view(save_hash, 32),
              "Invalid selected-save ice group fixture or identity");
        const auto group_count = word(group_in), vertex_count = word(group_in);
        check(group_count <= count && vertex_count == count,
              "Invalid selected-save ice group counts");
        const auto grouped = group_terrain_ice_input(built);
        check(grouped.groups.size() == group_count && grouped.vertices.size() == count &&
                  grouped.boundary_flags.size() == count,
              "Selected-save native ice grouping counts differ");
        for (const auto &group : grouped.groups)
            check(word(group_in) == group.first_vertex &&
                      word(group_in) == group.vertex_count,
                  "Selected-save native ice group span differs");
        for (std::uint32_t i = 0; i < count; ++i) {
            const auto x = integer(group_in), y = integer(group_in), z = integer(group_in);
            const auto flag = word(group_in, 1);
            check(grouped.vertices[i].x == x && grouped.vertices[i].y == y &&
                      grouped.vertices[i].z == z && grouped.boundary_flags[i] == flag,
                  "Selected-save native ice DFS vertex or boundary flag differs");
        }
        check(group_in.peek() == std::char_traits<char>::eof(),
              "Trailing selected-save ice group bytes");
        for (std::uint32_t y = 0; y < height; ++y)
            for (std::uint32_t x = 0; x < width; ++x)
                check((grouped.grid.cells[y * TerrainIceGrid::stride + x][0] & 0x40u) == 0,
                      "Selected-save grouping retained a discovery mark");
        std::cout << "Selected-save ice groups=" << group_count << "\n";
        if (contour_path) {
            std::ifstream contour_in(contour_path, std::ios::binary);
            char contour_magic[8], contour_hash[32];
            check(bool(contour_in.read(contour_magic, 8)) &&
                      std::string_view(contour_magic, 8) == "C5ICCT01" &&
                      bool(contour_in.read(contour_hash, 32)) &&
                      std::string_view(contour_hash, 32) == std::string_view(save_hash, 32),
                  "Invalid original ice contour fixture identity");
            const auto mode = word(contour_in), oracle_groups = word(contour_in);
            const auto oracle_vertices = word(contour_in), segment_count = word(contour_in);
            const auto edge_count = word(contour_in);
            check(mode == 1 && oracle_groups == group_count && oracle_vertices == count &&
                      segment_count <= edge_count && edge_count <= 6 * count,
                  "Invalid original ice contour fixture header");
            const auto contours = build_terrain_ice_contours_mode1(grouped);
            const auto allocation8 = count_terrain_ice_allocation(contours, 8);
            const auto allocation12 = count_terrain_ice_allocation(contours, 12);
            check(allocation8.in_bounds_edges == expected_inside_edges &&
                      allocation8.vertices == expected_vertices8 &&
                      allocation12.in_bounds_edges == expected_inside_edges &&
                      allocation12.vertices == expected_vertices12,
                  "Selected-save ice allocation differs from original 87AAC0");
            if (check_legacy_flag_runs) {
                mesh_flags(contours, 8, {
                    {{1, 640}, {0, 78}}, {{1, 48}}, {{1, 160}, {0, 16}}, {{1, 48}},
                    {{0, 2}, {1, 32}}, {{0, 4}, {1, 48}}, {{0, 6}, {1, 64}},
                    {{0, 2}, {1, 32}}, {{0, 14}, {1, 128}}});
                mesh_flags(contours, 12, {
                    {{1, 960}, {0, 78}}, {{1, 72}}, {{1, 240}, {0, 16}}, {{1, 72}},
                    {{0, 2}, {1, 48}}, {{0, 4}, {1, 72}}, {{0, 6}, {1, 96}},
                    {{0, 2}, {1, 48}}, {{0, 14}, {1, 192}}});
            }
            check(contours.groups.size() == oracle_groups &&
                      contours.vertices.size() == oracle_vertices &&
                      contours.boundary_flags.size() == count &&
                      contours.segments.size() == segment_count &&
                      contours.edges.size() == edge_count,
                  "Original ice contour output counts differ");
            for (const auto &group : contours.groups)
                check(word(contour_in) == group.first_segment &&
                          word(contour_in) == group.first_vertex &&
                          word(contour_in) == group.segment_count &&
                          word(contour_in) == group.vertex_count,
                      "Original ice contour group record differs");
            for (const auto &p : contours.vertices)
                check(integer(contour_in) == p.x && integer(contour_in) == p.y &&
                          integer(contour_in) == p.z,
                      "Original ice contour vertex differs");
            for (const auto flag : contours.boundary_flags)
                check(word(contour_in, 1) == flag,
                      "Original ice contour boundary flag differs");
            for (const auto &segment : contours.segments)
                check(word(contour_in) == segment.first_edge &&
                          word(contour_in) == segment.edge_count,
                      "Original ice contour segment differs");
            for (const auto &edge : contours.edges) {
                check(integer(contour_in) == edge.cube.x &&
                          integer(contour_in) == edge.cube.y &&
                          integer(contour_in) == edge.cube.z &&
                          word(contour_in, 1) == edge.side,
                      "Original ice contour ordered edge differs");
                contour_in.ignore(3);
                check(contour_in.gcount() == 3, "Truncated original ice contour edge padding");
            }
            for (std::uint32_t y = 0; y < height; ++y)
                for (std::uint32_t x = 0; x < width; ++x)
                    check(word(contour_in, 1) ==
                              contours.grid.cells[y * TerrainIceGrid::stride + x][0],
                          "Original ice contour owner byte differs");
            check(contour_in.peek() == std::char_traits<char>::eof(),
                  "Trailing original ice contour fixture bytes");
            std::cout << "Selected-save ice contours=" << segment_count << " segments, "
                      << edge_count << " edges\n";
        }
    }
    std::cout << "Selected-save ice input=" << count << " cells, " << exposed
              << " exposed sides\n";
}
std::uint32_t oracle(const char *path) {
    std::ifstream in(path, std::ios::binary);
    char magic[8];
    check(bool(in.read(magic, 8)) && std::string_view(magic, 8) == "C5AOWN01", "Invalid owner oracle");
    const auto cases = word(in);
    check(cases > 0 && cases <= 100, "Invalid owner case count");
    for (unsigned n = 0; n < cases; ++n) {
        const auto flags = word(in), seed = word(in), initialize = word(in);
        const auto source = grid(in);
        check(source.width > 0 && source.height > 0 && source.width <= 255 && source.height <= 255,
              "Invalid source dimensions");
        std::vector<TerrainAuxiliaryPlot> plots(std::size_t(source.width)*source.height);
        for (auto &p : plots) p = {integer(in), integer(in), integer(in), integer(in)};
        auto records = build_terrain_auxiliary_sources(source, plots);
        auto count = word(in);
        check(count < 100, "Too many extra relief controls");
        for (unsigned i = 0; i < count; ++i) {
            const auto x = std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(word(in, 2)));
            const auto y = std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(word(in, 2)));
            records.relief.push_back({x, y, static_cast<std::uint8_t>(word(in, 1))});
        }
        for (auto stream : {&records.feature0, &records.feature4}) {
            count = word(in); check(count < 100, "Too many extra feature controls");
            for (unsigned i = 0; i < count; ++i) stream->push_back({integer(in), integer(in), integer(in)});
        }
        TerrainAuxiliaryOwnerState state{};
        state.relief_enabled = flags & 1; state.water_enabled = flags & 2; state.flood_state_flag = flags & 4;
        input(in, state.relief, seed); input(in, state.adjacency, seed+17); input(in, state.water, seed+34);
        input(in, state.ice, seed+51); input(in, state.flood, seed+68);
        const auto expected_ice_grid = grid(in);
        const auto expected_flag = word(in);
        const auto initial_ice_wrap = state.ice.wrap_x;
        if (initialize) state.ice = initialize_terrain_ice_grid(state.ice, source.width, source.height, source.descriptor);
        const auto result = apply_terrain_auxiliary_flags(state, records);
        check(same(result.ice.coordinates, expected_ice_grid), "Original ice initialization header differs");
        check(result.flood_state_flag == bool(expected_flag), "Original flood owner flag differs");
        check(result.ice.wrap_x == initial_ice_wrap && result.relief_enabled == state.relief_enabled &&
                  result.water_enabled == state.water_enabled, "Unmodified owner flags changed");
        compare(in, result.relief, seed, n, "relief"); compare(in, result.adjacency, seed+17, n, "adjacency");
        compare(in, result.water, seed+34, n, "water"); compare(in, result.ice, seed+51, n, "ice");
        compare(in, result.flood, seed+68, n, "flood");
    }
    check(in.peek() == std::char_traits<char>::eof(), "Trailing owner oracle bytes");
    return cases;
}
std::uint32_t finalize_oracle(const char *path) {
    std::ifstream in(path, std::ios::binary);
    char magic[8];
    check(bool(in.read(magic, 8)) && std::string_view(magic, 8) == "C5AJFN01",
          "Invalid adjacency finalization oracle");
    const auto cases = word(in);
    check(cases > 0 && cases <= 100, "Invalid adjacency finalization case count");
    for (unsigned n = 0; n < cases; ++n) {
        const auto seed = word(in);
        TerrainAdjacencyGrid state{};
        input(in, state, seed+17);
        const auto before = bytes(state);
        const auto result = finalize_terrain_auxiliary_adjacency(state);
        check(bytes(state) == before, "Adjacency finalization mutated prior table");
        compare(in, result, seed+17, n, "adjacency finalization");
    }
    check(in.peek() == std::char_traits<char>::eof(), "Trailing adjacency finalization oracle bytes");
    return cases;
}
std::uint32_t initialization_oracle(const char *path) {
    std::ifstream in(path, std::ios::binary);
    char magic[8];
    check(bool(in.read(magic, 8)) && std::string_view(magic, 8) == "C5AINT01",
          "Invalid auxiliary initialization oracle");
    const auto cases = word(in);
    check(cases > 0 && cases <= 100, "Invalid auxiliary initialization case count");
    for (unsigned n = 0; n < cases; ++n) {
        const auto seed = word(in);
        const auto scratch = fixed_bytes<12>(in);
        TerrainAuxiliaryTwoByteGrid two_byte{};
        TerrainAdjacencyGrid adjacency{};
        input(in, two_byte, seed+31);
        input(in, adjacency, seed+17);
        const auto two_byte_before = bytes(two_byte), adjacency_before = bytes(adjacency);
        const auto expected_mask = fixed_bytes<2>(in);
        const auto expected_mask_fallback = fixed_bytes<2>(in);
        const auto expected_adjacency = fixed_bytes<12>(in);
        const auto result = initialize_terrain_auxiliary_tables(two_byte, adjacency, scratch);
        check(bytes(two_byte) == two_byte_before && bytes(adjacency) == adjacency_before,
              "Auxiliary initialization mutated prior tables");
        auto expected_two_byte_header = two_byte.coordinates;
        auto expected_adjacency_header = adjacency.coordinates;
        expected_two_byte_header.width = expected_two_byte_header.height = 255;
        expected_adjacency_header.width = expected_adjacency_header.height = 255;
        check(same(result.two_byte.coordinates, expected_two_byte_header) &&
              same(result.adjacency.coordinates, expected_adjacency_header) &&
              result.two_byte.wrap_x == two_byte.wrap_x && result.adjacency.wrap_x == adjacency.wrap_x,
              "Original auxiliary initialization changed other header fields");
        for (const auto &cell : result.two_byte.cells)
            if (cell != expected_mask) throw std::runtime_error("Native two-byte initializer cell mismatch");
        for (const auto &cell : result.adjacency.cells)
            if (cell != expected_adjacency) throw std::runtime_error("Native adjacency initializer cell mismatch");
        check(result.two_byte.fallback == expected_mask_fallback &&
              result.adjacency.fallback == expected_adjacency,
              "Native auxiliary initializer fallback mismatch");
        table_bytes += TerrainAuxiliaryTwoByteGrid::capacity*2 + TerrainAdjacencyGrid::capacity*12 + 14;
    }
    check(in.peek() == std::char_traits<char>::eof(), "Trailing auxiliary initialization oracle bytes");
    return cases;
}
void ice_vertex_packing_fixture(const char *path) {
    std::ifstream in(path, std::ios::binary);
    char magic[8];
    check(bool(in.read(magic, 8)) && std::string_view(magic, 8) == "C5IVPK01",
          "Invalid original ice vertex pack fixture");
    const auto exe_hash = fixed_bytes<32>(in);
    static constexpr std::array<std::uint8_t, 32> expected_hash{
        0x7c, 0x9f, 0x93, 0xd5, 0x12, 0xc4, 0x9a, 0x7a,
        0xb2, 0xbc, 0x70, 0xf5, 0x1c, 0x6c, 0xaa, 0x10,
        0xa8, 0x2d, 0xdd, 0x69, 0x7a, 0xc1, 0x1d, 0x8a,
        0x24, 0xff, 0x1b, 0xb7, 0xce, 0x7c, 0x36, 0x86};
    check(exe_hash == expected_hash && word(in) == 42,
          "Original ice vertex pack source identity changed");
    for (unsigned i = 0; i < 42; ++i) {
        std::array<float, 4> box{};
        for (auto &value : box) value = std::bit_cast<float>(word(in));
        const auto source = fixed_bytes<16>(in);
        const auto scale = std::bit_cast<float>(word(in));
        const auto offset = std::bit_cast<float>(word(in));
        const auto expected = fixed_bytes<8>(in);
        const auto actual = pack_terrain_ice_vertex(box, source, scale, offset);
        for (unsigned lane = 0; lane < actual.size(); ++lane)
            check(actual[lane] == expected[lane],
                  "Original ice packed vertex byte differs");
        if (i == 0) {
            auto wrong = box;
            wrong[2] = wrong[0];
            rejects([&] { pack_terrain_ice_vertex(wrong, source, scale, offset); });
            check(pack_terrain_ice_vertex(box, source, 0.0f, 0.0f)[7] == 0,
                  "Original zero-height conversion differs");
        }
    }
    check(in.peek() == std::char_traits<char>::eof(),
          "Trailing original ice vertex pack bytes");
}
void ice_partition_fixture(const char *mesh_path, const char *records_path,
                           bool combined_source = false,
                           bool authored_settings = false) {
    std::ifstream mesh(mesh_path, std::ios::binary);
    std::ifstream records(records_path, std::ios::binary);
    char magic[8];
    check(bool(mesh.read(magic, 8)) &&
          std::string_view(magic, 8) == (combined_source ? "C5IMCB01" : "C5IPRP01"),
          "Invalid original ice mesh source");
    const auto exe_hash = fixed_bytes<32>(mesh);
    const auto xml_hash = fixed_bytes<32>(mesh);
    const auto graph_hash = fixed_bytes<32>(mesh);
    std::uint32_t source_vertices = 0, source_indices = 0;
    float height_scale = 0, height_offset = 0;
    std::array<float, 4> box{};
    if (combined_source) {
        const auto scale_bits = word(mesh);
        source_indices = word(mesh);
        source_vertices = word(mesh);
        const auto seed = word(mesh);
        const auto precision = word(mesh);
        check(scale_bits == 0x41bfff7bu && seed == 0x270ecbf1u &&
              precision == 53, "Original combined ice identity changed");
        if (authored_settings) {
            height_scale = 25.0f;
            height_offset = 8.0f;
        }
    } else {
        source_vertices = word(mesh);
        source_indices = word(mesh);
        for (auto &value : box) value = std::bit_cast<float>(word(mesh));
        height_scale = std::bit_cast<float>(word(mesh));
        height_offset = std::bit_cast<float>(word(mesh));
    }
    check(source_vertices == 184103 && source_indices == 994311,
          "Original ice mesh source dimensions changed");
    std::vector<std::uint32_t> source_index(source_indices);
    for (auto &value : source_index) value = word(mesh);
    std::vector<std::array<std::uint8_t, 16>> source_vertex(source_vertices);
    for (auto &value : source_vertex) value = fixed_bytes<16>(mesh);
    check(mesh.peek() == std::char_traits<char>::eof(),
          "Trailing original ice mesh source bytes");

    check(bool(records.read(magic, 8)) && std::string_view(magic, 8) == "C5IRTB02",
          "Invalid original packed ice regions");
    check(fixed_bytes<32>(records) == exe_hash &&
          fixed_bytes<32>(records) == xml_hash &&
          fixed_bytes<32>(records) == graph_hash,
          "Original prepared and packed ice sources differ");
    const auto region_count = word(records);
    const auto packed_vertices = word(records);
    const auto packed_indices = word(records);
    const auto seed = word(records);
    check(region_count == 4 && packed_vertices == 184419 &&
          packed_indices == source_indices && seed == 0x270ecbf1u,
          "Original packed ice region dimensions changed");
    for (auto &value : box) {
        const auto bits = word(records);
        if (combined_source) value = std::bit_cast<float>(bits);
        else check(bits == std::bit_cast<std::uint32_t>(value),
                   "Original packed ice component box differs");
    }
    struct ExpectedRegion {
        std::array<std::uint32_t, 4> counts;
        std::array<std::uint32_t, 4> bounds;
    };
    std::vector<ExpectedRegion> expected(region_count);
    for (auto &region : expected) {
        for (auto &value : region.counts) value = word(records);
        for (auto &value : region.bounds) value = word(records);
    }
    const auto actual = combined_source
        ? build_terrain_ice_gpu_regions(box, source_vertex, source_index,
                                        height_scale, height_offset)
        : partition_terrain_ice_mesh(box, box, source_vertex, source_index,
                                     height_scale, height_offset);
    check(actual.size() == region_count, "Original ice partition count differs");
    std::uint32_t next_vertex = 0, next_index = 0;
    for (std::size_t region = 0; region < region_count; ++region) {
        const auto &got = actual[region];
        const auto &want = expected[region];
        check(want.counts[0] == next_vertex && want.counts[1] == next_index &&
              want.counts[2] == got.vertices.size() &&
              want.counts[3] == got.indices.size(),
              "Original ice partition span differs");
        for (unsigned lane = 0; lane < 4; ++lane)
            check(want.bounds[lane] == std::bit_cast<std::uint32_t>(got.bounds[lane]),
                  "Original ice partition bound differs");
        next_vertex += static_cast<std::uint32_t>(got.vertices.size());
        next_index += static_cast<std::uint32_t>(got.indices.size());
    }
    check(next_vertex == packed_vertices && next_index == packed_indices,
          "Original ice partition totals differ");
    for (const auto &region : actual)
        for (const auto &vertex : region.vertices)
            for (auto byte : vertex)
                check(byte == word(records, 1), "Original ice packed vertex differs");
    for (const auto &region : actual)
        for (auto index : region.indices)
            check(index == word(records, 2), "Original ice local index differs");
    check(records.peek() == std::char_traits<char>::eof(),
          "Trailing original packed ice region bytes");
    auto bad = source_index;
    bad[0] = source_vertices;
    rejects([&] { partition_terrain_ice_mesh(
        box, box, source_vertex, bad, height_scale, height_offset); });
}
void ice_normal_fixture(const char *combined_path, const char *prepared_path) {
    std::ifstream combined(combined_path, std::ios::binary);
    std::ifstream prepared(prepared_path, std::ios::binary);
    char magic[8];
    check(bool(combined.read(magic, 8)) && std::string_view(magic, 8) == "C5IMCB01",
          "Invalid original combined ice mesh");
    const auto exe_hash = fixed_bytes<32>(combined);
    const auto xml_hash = fixed_bytes<32>(combined);
    const auto graph_hash = fixed_bytes<32>(combined);
    const auto scale_bits = word(combined);
    const auto index_count = word(combined);
    const auto vertex_count = word(combined);
    const auto seed = word(combined);
    const auto precision = word(combined);
    check(scale_bits == 0x41bfff7bu && index_count == 994311 &&
          vertex_count == 184103 && seed == 0x270ecbf1u && precision == 53,
          "Original combined ice mesh dimensions changed");
    std::vector<std::uint32_t> indices(index_count);
    for (auto &value : indices) value = word(combined);
    std::vector<std::array<std::uint8_t, 16>> vertices(vertex_count);
    for (auto &value : vertices) value = fixed_bytes<16>(combined);
    check(combined.peek() == std::char_traits<char>::eof(),
          "Trailing original combined ice mesh bytes");

    check(bool(prepared.read(magic, 8)) && std::string_view(magic, 8) == "C5IPRP01",
          "Invalid original prepared ice mesh");
    check(fixed_bytes<32>(prepared) == exe_hash &&
          fixed_bytes<32>(prepared) == xml_hash &&
          fixed_bytes<32>(prepared) == graph_hash &&
          word(prepared) == vertex_count && word(prepared) == index_count,
          "Original combined and prepared ice identities differ");
    for (unsigned i = 0; i < 6; ++i) word(prepared);
    for (auto index : indices)
        check(word(prepared) == index, "Original ice preparation changed indices");
    prepare_terrain_ice_vertex_normals(vertices, indices);
    std::array<std::size_t, 16> lane_differences{};
    std::size_t first = vertex_count;
    std::size_t reported = 0;
    for (std::size_t i = 0; i < vertices.size(); ++i) {
        const auto expected = fixed_bytes<16>(prepared);
        bool differs = false;
        for (unsigned lane = 0; lane < 16; ++lane)
            if (vertices[i][lane] != expected[lane]) {
                ++lane_differences[lane];
                if (first == vertex_count) first = i;
                differs = true;
            }
        if (differs && reported++ < 12) {
            const auto position = std::bit_cast<std::array<float, 4>>(vertices[i]);
            std::cerr << "Ice normal mismatch vertex " << i << " at ("
                      << position[0] << ',' << position[1] << ',' << position[2]
                      << ") actual=";
            for (unsigned lane = 12; lane < 15; ++lane)
                std::cerr << ' ' << static_cast<unsigned>(vertices[i][lane]);
            std::cerr << " expected=";
            for (unsigned lane = 12; lane < 15; ++lane)
                std::cerr << ' ' << static_cast<unsigned>(expected[lane]);
            std::cerr << '\n';
        }
    }
    checks += vertex_count * 16;
    if (first != vertex_count) {
        std::cerr << "First ice normal mismatch at vertex " << first << "; lanes:";
        for (unsigned lane = 0; lane < 16; ++lane)
            if (lane_differences[lane])
                std::cerr << ' ' << lane << '=' << lane_differences[lane];
        std::cerr << '\n';
    }
    check(first == vertex_count, "Original ice vertex normals differ");
    check(prepared.peek() == std::char_traits<char>::eof(),
          "Trailing original prepared ice mesh bytes");
}
void ice_small_mesh_fixture(const char *graph_path, const char *mesh_path) {
    std::ifstream graph_file(graph_path, std::ios::binary);
    std::ifstream mesh_file(mesh_path, std::ios::binary);
    char magic[8];
    check(bool(graph_file.read(magic, 8)) &&
          std::string_view(magic, 8) == "C5IPST01",
          "Invalid original postprocessed ice graph");
    const auto exe_hash = fixed_bytes<32>(graph_file);
    const auto xml_hash = fixed_bytes<32>(graph_file);
    fixed_bytes<32>(graph_file);
    const auto initial = word(graph_file);
    const auto face_count = word(graph_file);
    const auto node_count = word(graph_file);
    const auto edge_count = word(graph_file);
    const auto link_count = word(graph_file);
    const auto retained = word(graph_file);
    const auto seed = word(graph_file);
    const bool full = initial == 376 && face_count == 389 &&
        node_count == 27214 && edge_count == 27214 && link_count == 54428 &&
        retained == 166 && seed == 0x270ecbf1u;
    const bool prefix = initial == 9 && face_count == 9 &&
        node_count == 2042 && edge_count == 2042 && link_count == 4084 &&
        retained == 3 && seed == 2115878600u;
    check(full || prefix, "Original postprocessed ice graph changed");
    for (unsigned lane = 0; lane < 4; ++lane) word(graph_file);
    TerrainIceIntermediateRegionGeometry graph;
    graph.regions.resize(face_count);
    for (auto &region : graph.regions) {
        region.first_vertex = word(graph_file);
        region.first_index = word(graph_file);
        region.vertex_count = word(graph_file);
        const auto boundary = word(graph_file);
        check(boundary <= 1, "Original small ice boundary flag changed");
        region.boundary = static_cast<std::uint8_t>(boundary);
    }
    graph.vertices.resize(node_count);
    for (auto &vertex : graph.vertices) {
        for (auto &coordinate : vertex)
            coordinate = std::bit_cast<float>(word(graph_file));
        word(graph_file); word(graph_file);
    }
    graph.indices.resize(edge_count);
    for (auto &edge : graph.indices)
        for (auto &value : edge) value = word(graph_file);
    for (std::uint32_t link = 0; link < link_count; ++link) word(graph_file);
    check(graph_file.peek() == std::char_traits<char>::eof(),
          "Trailing original postprocessed ice graph bytes");

    check(bool(mesh_file.read(magic, 8)) && std::string_view(magic, 8) == "C5IMSH01",
          "Invalid original small ice mesh");
    check(fixed_bytes<32>(mesh_file) == exe_hash &&
          fixed_bytes<32>(mesh_file) == xml_hash,
          "Original small ice mesh source identity differs");
    fixed_bytes<32>(mesh_file);
    const auto scale_bits = word(mesh_file);
    const auto index_count = word(mesh_file);
    const auto vertex_count = word(mesh_file);
    const auto mesh_seed = word(mesh_file);
    const auto precision = word(mesh_file);
    check(scale_bits == 0x41bfff7bu &&
          index_count == (full ? 4053u : 51u) &&
          vertex_count == (full ? 1110u : 14u) &&
          mesh_seed == seed && precision == 53,
          "Original small ice mesh dimensions changed");
    TerrainIceSmallMeshSettings settings;
    settings.scale = std::bit_cast<float>(scale_bits);
    settings.height_min = 0.1f; settings.height_max = 3.6f;
    settings.depth_min = 6.1f; settings.depth_max = 6.6f;
    settings.skirt_slope = 0.0015625f;
    const auto actual = build_terrain_ice_small_mesh(graph, settings, seed);
    check(actual.vertices.size() == vertex_count &&
          actual.indices.size() == index_count,
          "Original small ice output count differs");
    for (auto index : actual.indices)
        check(index == word(mesh_file), "Original small ice triangle index differs");
    for (const auto &vertex : actual.vertices)
        for (auto byte : vertex)
            check(byte == word(mesh_file, 1),
                  "Original small ice vertex byte differs");
    check(mesh_file.peek() == std::char_traits<char>::eof(),
          "Trailing original small ice mesh bytes");
}
void ice_refinement_construction_fixture(const char *path) {
    std::ifstream in(path, std::ios::binary);
    char magic[8];
    check(bool(in.read(magic, 8)) &&
          std::string_view(magic, 8) == "C5IRCS01",
          "Invalid original ice construction fixture");
    const auto hex = [](const std::array<std::uint8_t, 32> &bytes) {
        std::string result;
        static constexpr char digits[] = "0123456789abcdef";
        for (const auto byte : bytes) {
            result.push_back(digits[byte >> 4]);
            result.push_back(digits[byte & 15]);
        }
        return result;
    };
    const auto exe = hex(fixed_bytes<32>(in));
    const auto xml = hex(fixed_bytes<32>(in));
    const auto source = hex(fixed_bytes<32>(in));
    const bool full = source ==
        "fb9fbd46a0e81afb5a9e5e8643a569d2367e4c8494a64c42c5387ba7214e37fc";
    const bool prefix = source ==
        "bd27329ec3ceb221cf4da2c20cad1afdb53fe496ce452efd32054ba176cdc4af";
    check(exe == "7c9f93d512c49a7ab2bc70f51c6caa10a82ddd697ac11d8a24ff1bb7ce7c3686" &&
          xml == "ab594d596e9dc386ca2cabcd952799a97190093a70cbc3b8486c7b2593318f1b" &&
          (full || prefix), "Original ice construction source identity differs");
    struct Stage {
        std::vector<std::array<std::uint32_t, 2>> points;
        std::vector<std::array<std::uint32_t, 3>> edges;
    };
    std::array<Stage, 3> stages;
    const std::array<std::array<std::uint32_t, 2>, 3> expected = full ?
        std::array<std::array<std::uint32_t, 2>, 3>{
            {{142913, 113742}, {147111, 227072}, {147111, 262954}}} :
        std::array<std::array<std::uint32_t, 2>, 3>{
            {{73351, 70047}, {74503, 139401}, {74503, 145126}}};
    for (std::size_t stage = 0; stage < stages.size(); ++stage) {
        const auto point_count = word(in), edge_count = word(in);
        check(point_count == expected[stage][0] &&
              edge_count == expected[stage][1],
              "Original ice construction stage dimensions differ");
        auto &data = stages[stage];
        data.points.resize(point_count);
        data.edges.resize(edge_count);
        for (auto &point : data.points) point = {word(in), word(in)};
        for (auto &edge : data.edges) edge = {word(in), word(in), word(in)};
    }
    check(in.peek() == std::char_traits<char>::eof(),
          "Trailing original ice construction bytes");
    const auto &inserted = stages[0], &sampled = stages[1], &stitched = stages[2];
    check(std::equal(inserted.points.begin(), inserted.points.end(),
                     sampled.points.begin()) &&
          std::equal(inserted.edges.begin(), inserted.edges.end(),
                     sampled.edges.begin()) &&
          sampled.points == stitched.points &&
          std::equal(sampled.edges.begin(), sampled.edges.end(),
                     stitched.edges.begin()),
          "Original ice construction mutated a prior record");
    std::vector<TerrainIceTessellationEdge> sampled_edges;
    sampled_edges.reserve(sampled.edges.size());
    for (const auto &edge : sampled.edges) {
        check((edge[2] & 0xffu) == 0,
              "Original sampled ice edge was active");
        sampled_edges.push_back({edge[0], edge[1], 0});
    }
    const auto selected = select_terrain_ice_refinement_boundary_points(
        static_cast<std::uint32_t>(sampled.points.size()), sampled_edges);
    std::vector<std::uint8_t> incoming(selected.size()), outgoing(selected.size());
    for (std::size_t i = sampled.edges.size(); i < stitched.edges.size(); ++i) {
        const auto &edge = stitched.edges[i];
        check(edge[0] < selected.size() && edge[1] < selected.size() &&
              edge[0] != edge[1] && (edge[2] & 0xffu) == 1,
              "Original stitched ice edge is invalid");
        ++outgoing[edge[0]];
        ++incoming[edge[1]];
    }
    std::size_t selected_count = 0;
    for (std::size_t i = 0; i < selected.size(); ++i) {
        check(incoming[i] == selected[i] && outgoing[i] == selected[i],
              "Original ice boundary point selection differs");
        selected_count += selected[i];
    }
    check(selected_count == (full ? 35882u : 5725u),
          "Original ice boundary point count differs");
}
void ice_refinement_stitch_fixture(const char *construction_path,
                                   const char *input_path) {
    std::ifstream input(input_path, std::ios::binary);
    char magic[8];
    check(bool(input.read(magic, 8)) &&
          std::string_view(magic, 8) == "C5IRST01",
          "Invalid original ice stitching input");
    const auto exe = fixed_bytes<32>(input);
    const auto xml = fixed_bytes<32>(input);
    const auto source = fixed_bytes<32>(input);
    const auto source_count = word(input), frontier_count = word(input);
    check(source_count <= 1'000'000 && frontier_count <= 1'000'000,
          "Original ice stitching input extent changed");
    std::vector<std::array<std::uint32_t, 2>> source_edges;
    source_edges.reserve(source_count);
    for (std::uint32_t i = 0; i < source_count; ++i) {
        const auto first = word(input), second = word(input);
        word(input); // The source graph's third word is a face identifier.
        source_edges.push_back({first, second});
    }
    std::vector<TerrainIceRefinementFrontierPoint> frontier;
    frontier.reserve(frontier_count);
    for (std::uint32_t i = 0; i < frontier_count; ++i)
        frontier.push_back({word(input), word(input), std::bit_cast<float>(word(input))});
    check(input.peek() == std::char_traits<char>::eof(),
          "Trailing original ice stitching input bytes");

    std::ifstream construction(construction_path, std::ios::binary);
    check(bool(construction.read(magic, 8)) &&
          std::string_view(magic, 8) == "C5IRCS01",
          "Invalid original ice construction for stitching");
    check(fixed_bytes<32>(construction) == exe &&
          fixed_bytes<32>(construction) == xml &&
          fixed_bytes<32>(construction) == source,
          "Original ice stitching fixture identity differs");
    std::uint32_t sampled_edges = 0, stitched_points = 0, stitched_edges = 0;
    for (unsigned stage = 0; stage < 3; ++stage) {
        const auto points = word(construction), edges = word(construction);
        check(points <= 1'000'000 && edges <= 2'000'000,
              "Original ice construction extent changed");
        if (stage == 1) sampled_edges = edges;
        if (stage == 2) {
            stitched_points = points;
            stitched_edges = edges;
        }
        construction.seekg(std::streamoff(8) * points +
                           (stage == 2 ? std::streamoff(12) * sampled_edges :
                                         std::streamoff(12) * edges), std::ios::cur);
        check(bool(construction), "Truncated original ice construction stage");
    }
    check(stitched_edges >= sampled_edges &&
          source_count + frontier_count == stitched_edges - sampled_edges,
          "Original ice stitching output extent differs");
    const auto actual = stitch_terrain_ice_refinement_boundary(
        stitched_points, source_edges, frontier);
    check(actual.size() == stitched_edges - sampled_edges,
          "Ported ice stitching edge count differs");
    for (const auto &edge : actual) {
        const auto first = word(construction), second = word(construction);
        const auto flags = word(construction);
        check(edge.first_point == first && edge.second_point == second &&
              (flags & 0xffu) == edge.active,
              "Ported ice stitching edge differs");
    }
    check(construction.peek() == std::char_traits<char>::eof(),
          "Trailing original ice construction after stitching");
}
TerrainIceLargeSourceGraph ice_large_source_fixture(
    const char *postprocessed_path, const char *selected_path) {
    std::ifstream input(postprocessed_path, std::ios::binary);
    char magic[8];
    check(bool(input.read(magic, 8)) &&
          std::string_view(magic, 8) == "C5IPST01",
          "Invalid postprocessed ice source graph");
    const auto exe = fixed_bytes<32>(input);
    const auto xml = fixed_bytes<32>(input);
    fixed_bytes<32>(input);
    const auto initial = word(input), face_count = word(input);
    const auto node_count = word(input), edge_count = word(input);
    const auto link_count = word(input), retained = word(input), seed = word(input);
    const bool full = initial == 376 && face_count == 389 &&
        node_count == 27214 && edge_count == 27214 && link_count == 54428 &&
        retained == 166 && seed == 0x270ecbf1u;
    const bool prefix = initial == 9 && face_count == 9 &&
        node_count == 2042 && edge_count == 2042 && link_count == 4084 &&
        retained == 3 && seed == 2115878600u;
    const bool current = initial == 394 && face_count == 400 &&
        node_count == 29385 && edge_count == 29385 && link_count == 58770 &&
        retained == 173 && seed == 1216466304u;
    check(full || prefix || current,
          "Original postprocessed ice graph dimensions changed");
    for (unsigned i = 0; i < 4; ++i) word(input);
    TerrainIceIntermediateRegionGeometry graph;
    graph.regions.resize(face_count);
    for (auto &region : graph.regions) {
        region.first_vertex = word(input);
        region.first_index = word(input);
        region.vertex_count = word(input);
        const auto boundary = word(input);
        check(boundary <= 1, "Original ice source region flag changed");
        region.boundary = static_cast<std::uint8_t>(boundary);
    }
    graph.vertices.resize(node_count);
    for (auto &point : graph.vertices) {
        point = {std::bit_cast<float>(word(input)),
                 std::bit_cast<float>(word(input))};
        word(input); word(input);
    }
    graph.indices.resize(edge_count);
    for (auto &edge : graph.indices)
        edge = {word(input), word(input), word(input)};
    for (std::uint32_t i = 0; i < link_count; ++i) word(input);
    check(input.peek() == std::char_traits<char>::eof(),
          "Trailing postprocessed ice source graph bytes");

    std::ifstream selected(selected_path, std::ios::binary);
    check(bool(selected.read(magic, 8)) &&
          std::string_view(magic, 8) == "C5ISLC01",
          "Invalid original selected ice source graph");
    check(fixed_bytes<32>(selected) == exe &&
          fixed_bytes<32>(selected) == xml,
          "Original selected ice source identity differs");
    const auto digest = fixed_bytes<32>(selected);
    const auto expected_digest = current ?
        "e13a1762c23d739af5e5f69ce5bcd6310aac83eef557841a3ec110cf9b580add" :
        full ? "fb9fbd46a0e81afb5a9e5e8643a569d2367e4c8494a64c42c5387ba7214e37fc" :
               "bd27329ec3ceb221cf4da2c20cad1afdb53fe496ce452efd32054ba176cdc4af";
    static constexpr char digits[] = "0123456789abcdef";
    std::string digest_hex;
    for (auto byte : digest) {
        digest_hex.push_back(digits[byte >> 4]);
        digest_hex.push_back(digits[byte & 15]);
    }
    check(digest_hex == expected_digest,
          "Original selected ice source input hash differs");
    const auto expected_points = word(selected), expected_edges = word(selected);
    check(expected_points == (current ? 28819u : full ? 26659u : 2035u) &&
          expected_edges == expected_points,
          "Original selected ice source dimensions changed");
    const auto actual = select_terrain_ice_large_source_graph(graph, 16);
    check(actual.points.size() == expected_points &&
          actual.edges.size() == expected_edges,
          "Ported selected ice source dimensions differ");
    for (const auto &point : actual.points) {
        const auto x = word(selected), y = word(selected);
        word(selected);
        const auto degree = word(selected);
        check(std::bit_cast<std::uint32_t>(point[0]) == x &&
              std::bit_cast<std::uint32_t>(point[1]) == y && degree == 2,
              "Ported selected ice source point differs");
    }
    for (const auto &edge : actual.edges)
        check(edge == std::array<std::uint32_t, 3>{
                  word(selected), word(selected), word(selected)},
              "Ported selected ice source edge differs");
    check(selected.peek() == std::char_traits<char>::eof(),
          "Trailing original selected ice source bytes");
    return actual;
}
void ice_vertical_insertion_fixture(const char *postprocessed_path,
                                    const char *selected_path,
                                    const char *events_path,
                                    const char *construction_path) {
    const auto source = ice_large_source_fixture(postprocessed_path, selected_path);
    std::ifstream events(events_path, std::ios::binary);
    char magic[8];
    check(bool(events.read(magic, 8)) &&
          std::string_view(magic, 8) == "C5IVLN01",
          "Invalid original vertical ice events");
    const auto exe = fixed_bytes<32>(events);
    const auto xml = fixed_bytes<32>(events);
    const auto graph_hash = fixed_bytes<32>(events);
    const auto line_count = word(events);
    check(line_count <= 10'000, "Original vertical ice line count changed");
    std::vector<TerrainIceVerticalSweepLine> lines(line_count);
    for (auto &line : lines) {
        line.x = std::bit_cast<float>(word(events));
        const auto count = word(events);
        check(count <= 1'000'000, "Original vertical ice event count changed");
        line.events.resize(count);
        for (auto &event : line.events)
            event = {std::bit_cast<float>(word(events)),
                     word(events), word(events)};
    }
    check(events.peek() == std::char_traits<char>::eof(),
          "Trailing original vertical ice events");
    std::ifstream construction(construction_path, std::ios::binary);
    check(bool(construction.read(magic, 8)) &&
          std::string_view(magic, 8) == "C5IRCS01",
          "Invalid original vertical ice construction");
    check(fixed_bytes<32>(construction) == exe &&
          fixed_bytes<32>(construction) == xml &&
          fixed_bytes<32>(construction) == graph_hash,
          "Original vertical ice fixture identity differs");
    const auto points = word(construction), edges = word(construction);
    const bool full = source.points.size() == 26659;
    check(points == (full ? 142913u : 73351u) &&
          edges == (full ? 113742u : 70047u),
          "Original vertical ice construction dimensions changed");
    const float scale = std::bit_cast<float>(0x41bfff7bu);
    const auto actual = insert_terrain_ice_vertical_lattice(
        source, lines, scale / 12.0f);
    check(actual.points.size() == points && actual.edges.size() == edges,
          "Ported vertical ice lattice dimensions differ");
    for (const auto &point : actual.points)
        check(std::bit_cast<std::uint32_t>(point[0]) == word(construction) &&
              std::bit_cast<std::uint32_t>(point[1]) == word(construction),
              "Ported vertical ice point differs");
    for (const auto &edge : actual.edges) {
        const auto first = word(construction), second = word(construction);
        const auto flags = word(construction);
        check(edge.first_point == first && edge.second_point == second &&
              edge.active == (flags & 0xffu),
              "Ported vertical ice edge differs");
    }
}
void ice_contour_intersections_fixture(const char *postprocessed_path,
                                       const char *selected_path,
                                       const char *vertical_path,
                                       const char *horizontal_path) {
    const auto source = ice_large_source_fixture(postprocessed_path, selected_path);
    std::array<std::uint8_t, 32> identity_exe{}, identity_xml{}, identity_graph{};
    for (std::uint32_t axis = 0; axis < 2; ++axis) {
        std::ifstream in(axis ? horizontal_path : vertical_path, std::ios::binary);
        char magic[8];
        check(bool(in.read(magic, 8)) &&
              std::string_view(magic, 8) == (axis ? "C5IHLN01" : "C5IVLN01"),
              "Invalid original ice contour intersections");
        const auto exe = fixed_bytes<32>(in);
        const auto xml = fixed_bytes<32>(in);
        const auto graph_hash = fixed_bytes<32>(in);
        if (axis) {
            check(exe == identity_exe && xml == identity_xml &&
                  graph_hash == identity_graph,
                  "Original ice contour fixture identities differ");
        } else {
            identity_exe = exe;
            identity_xml = xml;
            identity_graph = graph_hash;
        }
        const auto line_count = word(in);
        const bool full = source.points.size() == 26659;
        check(line_count == (axis ? (full ? 185u : 67u) :
                                   (full ? 1613u : 1246u)),
              "Original ice contour line count changed");
        for (std::uint32_t line = 0; line < line_count; ++line) {
            const auto coordinate = std::bit_cast<float>(word(in));
            const auto count = word(in);
            check(count <= 1'000'000, "Original ice contour event count changed");
            std::vector<TerrainIceContourIntersection> expected(count);
            for (auto &event : expected)
                event = {std::bit_cast<float>(word(in)), word(in), word(in)};
            std::uint32_t next_point = static_cast<std::uint32_t>(source.points.size());
            for (const auto &event : expected)
                if (event.point >= source.points.size()) {
                    next_point = event.point;
                    break;
                }
            const auto actual = produce_terrain_ice_contour_intersections(
                source, axis, coordinate, next_point);
            check(actual.size() == expected.size(),
                  "Ported ice contour event count differs");
            for (std::size_t i = 0; i < expected.size(); ++i)
                check(std::bit_cast<std::uint32_t>(actual[i].position) ==
                          std::bit_cast<std::uint32_t>(expected[i].position) &&
                      actual[i].point == expected[i].point &&
                      actual[i].state == expected[i].state,
                      "Ported ice contour intersection differs");
        }
        check(in.peek() == std::char_traits<char>::eof(),
              "Trailing original ice contour events");
    }
}
void ice_current_vertical_intersections_fixture(const char *postprocessed_path,
                                                const char *selected_path,
                                                const char *vertical_path) {
    const auto selected_graph = ice_large_source_fixture(
        postprocessed_path, selected_path);
    std::ifstream postprocessed(postprocessed_path, std::ios::binary);
    postprocessed.seekg(132);
    const std::array<float, 4> component_box{
        std::bit_cast<float>(word(postprocessed)),
        std::bit_cast<float>(word(postprocessed)),
        std::bit_cast<float>(word(postprocessed)),
        std::bit_cast<float>(word(postprocessed))};
    std::ifstream selected(selected_path, std::ios::binary);
    char magic[8];
    check(bool(selected.read(magic, 8)) &&
          std::string_view(magic, 8) == "C5ISLC01",
          "Invalid current selected ice source");
    const auto exe = fixed_bytes<32>(selected);
    const auto xml = fixed_bytes<32>(selected);
    const auto graph_hash = fixed_bytes<32>(selected);
    const auto point_count = word(selected), edge_count = word(selected);
    check(point_count > 0 && point_count <= 1'000'000 &&
          edge_count > 0 && edge_count <= 1'000'000,
          "Invalid current selected ice source dimensions");
    TerrainIceLargeSourceGraph source;
    source.points.resize(point_count);
    source.edges.resize(edge_count);
    for (auto &point : source.points) {
        point = {std::bit_cast<float>(word(selected)),
                 std::bit_cast<float>(word(selected))};
        word(selected); word(selected);
    }
    for (auto &edge : source.edges)
        edge = {word(selected), word(selected), word(selected)};
    check(source.points == selected_graph.points &&
          source.edges == selected_graph.edges,
          "Current selected ice source differs from original");
    check(selected.peek() == std::char_traits<char>::eof(),
          "Trailing current selected ice source bytes");

    std::ifstream vertical(vertical_path, std::ios::binary);
    check(bool(vertical.read(magic, 8)) &&
          std::string_view(magic, 8) == "C5IVLN01" &&
          fixed_bytes<32>(vertical) == exe &&
          fixed_bytes<32>(vertical) == xml &&
          fixed_bytes<32>(vertical) == graph_hash,
          "Current native ice vertical events have different provenance");
    const auto line_count = word(vertical);
    check(line_count > 0 && line_count <= 10'000,
          "Invalid current native ice vertical line count");
    const float spacing = std::bit_cast<float>(0x41bfff7bu) / 12.0f;
    float next_x = component_box[0] + spacing;
    for (std::uint32_t line = 0; line < line_count; ++line) {
        const auto x = std::bit_cast<float>(word(vertical));
        check(std::bit_cast<std::uint32_t>(x) ==
              std::bit_cast<std::uint32_t>(next_x),
              "Current native ice vertical line coordinate differs");
        next_x += spacing;
        const auto count = word(vertical);
        check(count <= 1'000'000,
              "Invalid current native ice vertical event count");
        std::vector<TerrainIceContourIntersection> expected(count);
        for (auto &event : expected)
            event = {std::bit_cast<float>(word(vertical)),
                     word(vertical), word(vertical)};
        std::uint32_t next_point = point_count;
        for (const auto &event : expected)
            if (event.point >= point_count) {
                next_point = event.point;
                break;
            }
        const auto actual = produce_terrain_ice_contour_intersections(
            source, 0, x, next_point);
        check(actual.size() == expected.size(),
              "Current vertical ice event count differs from original");
        for (std::size_t i = 0; i < expected.size(); ++i)
            check(std::bit_cast<std::uint32_t>(actual[i].position) ==
                      std::bit_cast<std::uint32_t>(expected[i].position) &&
                  actual[i].point == expected[i].point &&
                  actual[i].state == expected[i].state,
                  "Current vertical ice intersection differs from original");
    }
    check(next_x >= component_box[2],
          "Current native ice vertical line sequence ends early");
    check(vertical.peek() == std::char_traits<char>::eof(),
          "Trailing current native ice vertical events");
}
void ice_generated_lattice_fixture(const char *postprocessed_path,
                                   const char *selected_path,
                                   const char *construction_path) {
    const auto source = ice_large_source_fixture(postprocessed_path, selected_path);
    std::ifstream in(construction_path, std::ios::binary);
    char magic[8];
    check(bool(in.read(magic, 8)) &&
          std::string_view(magic, 8) == "C5IRCS01",
          "Invalid original generated ice construction");
    fixed_bytes<32>(in); // Executable, settings, and source hashes.
    fixed_bytes<32>(in);
    fixed_bytes<32>(in);
    const bool full = source.points.size() == 26659;
    const float spacing = std::bit_cast<float>(0x41bfff7bu) / 12.0f;
    auto graph = insert_terrain_ice_vertical_lattice(source, spacing);
    const auto compare_stage = [&](const TerrainIceVerticalInsertionGraph &actual,
                                   std::uint32_t expected_points,
                                   std::uint32_t expected_edges) {
        const auto points = word(in), edges = word(in);
        check(points == expected_points && edges == expected_edges &&
              actual.points.size() == points && actual.edges.size() == edges,
              "Generated ice lattice dimensions differ");
        for (const auto &point : actual.points)
            check(std::bit_cast<std::uint32_t>(point[0]) == word(in) &&
                  std::bit_cast<std::uint32_t>(point[1]) == word(in),
                  "Generated ice lattice point differs");
        for (const auto &edge : actual.edges) {
            const auto first = word(in), second = word(in), flags = word(in);
            check(edge.first_point == first && edge.second_point == second &&
                  edge.active == (flags & 0xffu),
                  "Generated ice lattice edge differs");
        }
    };
    compare_stage(graph, full ? 142913u : 73351u,
                  full ? 113742u : 70047u);
    graph = sample_terrain_ice_horizontal_lattice(std::move(graph), source, spacing);
    compare_stage(graph, full ? 147111u : 74503u,
                  full ? 227072u : 139401u);
    const auto refined = construct_terrain_ice_refined_graph(source, spacing);
    const auto points = word(in), edges = word(in);
    check(points == (full ? 147111u : 74503u) &&
          edges == (full ? 262954u : 145126u) &&
          refined.points.size() == points && refined.edges.size() == edges,
          "Generated stitched ice graph dimensions differ");
    for (const auto &point : refined.points)
        check(std::bit_cast<std::uint32_t>(point.position[0]) == word(in) &&
              std::bit_cast<std::uint32_t>(point.position[1]) == word(in),
              "Generated stitched ice point differs");
    for (const auto &edge : refined.edges) {
        const auto first = word(in), second = word(in), flags = word(in);
        check(edge.first_point == first && edge.second_point == second &&
              edge.active == (flags & 0xffu),
              "Generated stitched ice edge differs");
    }
    check(in.peek() == std::char_traits<char>::eof(),
          "Trailing original generated ice construction bytes");
}
void ice_complete_mesh_fixture(const char *postprocessed_path,
                               const char *combined_path,
                               const char *pixels_path,
                               const char *records_path,
                               const char *output_path) {
    std::ifstream source(postprocessed_path, std::ios::binary);
    char magic[8];
    check(bool(source.read(magic, 8)) &&
          std::string_view(magic, 8) == "C5IPST01",
          "Invalid original complete ice source");
    const auto exe = fixed_bytes<32>(source);
    const auto xml = fixed_bytes<32>(source);
    fixed_bytes<32>(source);
    const auto initial = word(source), faces = word(source);
    const auto nodes = word(source), edge_count = word(source);
    const auto links = word(source), retained = word(source), seed = word(source);
    const bool full = initial == 376 && faces == 389 && nodes == 27214 &&
        edge_count == 27214 && links == 54428 && retained == 166 &&
        seed == 0x270ecbf1u;
    const bool prefix = initial == 9 && faces == 9 && nodes == 2042 &&
        edge_count == 2042 && links == 4084 && retained == 3 &&
        seed == 0x7e1dbec8u;
    check(full || prefix, "Original complete ice source dimensions changed");
    std::array<std::uint32_t, 4> source_box{};
    for (auto &value : source_box) value = word(source);
    std::array<float, 4> component_box{};
    for (unsigned lane = 0; lane < 4; ++lane)
        component_box[lane] = std::bit_cast<float>(source_box[lane]);
    TerrainIceIntermediateRegionGeometry geometry;
    geometry.regions.resize(faces);
    for (auto &region : geometry.regions) {
        region.first_vertex = word(source);
        region.first_index = word(source);
        region.vertex_count = word(source);
        const auto boundary = word(source);
        check(boundary <= 1, "Original complete ice region flag changed");
        region.boundary = static_cast<std::uint8_t>(boundary);
    }
    geometry.vertices.resize(nodes);
    for (auto &point : geometry.vertices) {
        point = {std::bit_cast<float>(word(source)),
                 std::bit_cast<float>(word(source))};
        word(source); word(source);
    }
    geometry.indices.resize(edge_count);
    for (auto &edge : geometry.indices)
        edge = {word(source), word(source), word(source)};
    for (std::uint32_t i = 0; i < links; ++i) word(source);
    check(source.peek() == std::char_traits<char>::eof(),
          "Trailing original complete ice source bytes");
    std::ifstream pixel_file(pixels_path, std::ios::binary | std::ios::ate);
    check(bool(pixel_file) && pixel_file.tellg() == 87381,
          "Original ice displacement pixel extent changed");
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(pixel_file.tellg()));
    pixel_file.seekg(0);
    pixel_file.read(reinterpret_cast<char *>(pixels.data()),
                    static_cast<std::streamsize>(pixels.size()));
    check(bool(pixel_file), "Original ice displacement pixels are missing");
    TerrainIceSmallMeshSettings small;
    small.scale = std::bit_cast<float>(0x41bfff7bu);
    small.height_min = 0.1f; small.height_max = 3.6f;
    small.depth_min = 6.1f; small.depth_max = 6.6f;
    small.skirt_slope = 0.0015625f;
    TerrainIceLargeTopSettings large;
    large.scale = small.scale;
    large.displacement_frequency = 0.1f;
    large.displacement_min = -8.0f; large.displacement_max = 15.0f;
    large.height = 10.0f; large.height_ceiling = 25.0f;
    auto invalid_box = component_box;
    invalid_box[2] = invalid_box[0];
    rejects([&] { build_terrain_ice_complete_mesh(
        geometry, invalid_box, small, large, pixels, 256, 256, 256,
        0.0078125f, 8.0f, seed); });
    auto actual = build_terrain_ice_complete_mesh(
        geometry, component_box, small, large, pixels, 256, 256, 256,
        0.0078125f, 8.0f, seed);
    auto wider_box = component_box;
    wider_box[0] -= 32.0f;
    wider_box[2] += 32.0f;
    const auto wider = build_terrain_ice_complete_mesh(
        geometry, wider_box, small, large, pixels, 256, 256, 256,
        0.0078125f, 8.0f, seed);
    check(wider.component_box == wider_box &&
          wider.combined.vertices == actual.combined.vertices &&
          wider.combined.indices == actual.combined.indices,
          "Pre-prune ice component box was replaced by selected mesh bounds");
    for (unsigned lane = 0; lane < 4; ++lane)
        check(std::bit_cast<std::uint32_t>(actual.component_box[lane]) ==
                  source_box[lane],
              "Generated ice component box differs");

    std::ifstream combined(combined_path, std::ios::binary);
    check(bool(combined.read(magic, 8)) &&
          std::string_view(magic, 8) == "C5IMCB01",
          "Invalid original complete ice mesh");
    check(fixed_bytes<32>(combined) == exe &&
          fixed_bytes<32>(combined) == xml,
          "Original complete ice mesh identity differs");
    const auto graph_hash = fixed_bytes<32>(combined);
    const auto scale = word(combined), indices = word(combined);
    const auto vertices = word(combined), mesh_seed = word(combined);
    const auto precision = word(combined);
    check(scale == 0x41bfff7bu && mesh_seed == seed && precision == 53 &&
          indices == (full ? 994311u : 464238u) &&
          vertices == (full ? 184103u : 80242u) &&
          actual.combined.indices.size() == indices &&
          actual.combined.vertices.size() == vertices,
          "Generated ice combined mesh dimensions differ");
    for (auto index : actual.combined.indices)
        check(index == word(combined), "Generated ice combined index differs");
    for (const auto &vertex : actual.combined.vertices)
        for (auto byte : vertex)
            check(byte == word(combined, 1), "Generated ice combined vertex differs");
    check(combined.peek() == std::char_traits<char>::eof(),
          "Trailing original complete ice mesh bytes");
    if (!records_path) return;
    std::ifstream records(records_path, std::ios::binary);
    check(bool(records.read(magic, 8)) &&
          std::string_view(magic, 8) == "C5IRTB02",
          "Invalid original complete ice GPU records");
    check(fixed_bytes<32>(records) == exe &&
          fixed_bytes<32>(records) == xml &&
          fixed_bytes<32>(records) == graph_hash,
          "Original complete ice GPU identity differs");
    const auto regions = word(records), packed_vertices = word(records);
    const auto packed_indices = word(records), record_seed = word(records);
    check(regions == 4 && packed_vertices == 184419 &&
          packed_indices == indices && record_seed == seed,
          "Original complete ice GPU dimensions changed");
    for (unsigned lane = 0; lane < 4; ++lane)
        check(word(records) ==
                  std::bit_cast<std::uint32_t>(actual.component_box[lane]),
              "Generated ice GPU component box differs");
    std::vector<std::array<std::uint32_t, 8>> expected(regions);
    for (auto &region : expected)
        for (auto &value : region) value = word(records);
    const auto packed = build_terrain_ice_gpu_regions(
        actual.component_box, actual.combined.vertices,
        actual.combined.indices, 25.0f, 8.0f);
    check(packed.size() == regions, "Generated ice GPU region count differs");
    std::uint32_t first_vertex = 0, first_index = 0;
    for (std::size_t i = 0; i < packed.size(); ++i) {
        const auto &region = packed[i];
        const auto &want = expected[i];
        check(want[0] == first_vertex && want[1] == first_index &&
              want[2] == region.vertices.size() &&
              want[3] == region.indices.size(),
              "Generated ice GPU region span differs");
        for (unsigned lane = 0; lane < 4; ++lane)
            check(want[4 + lane] ==
                      std::bit_cast<std::uint32_t>(region.bounds[lane]),
                  "Generated ice GPU region bound differs");
        first_vertex += static_cast<std::uint32_t>(region.vertices.size());
        first_index += static_cast<std::uint32_t>(region.indices.size());
    }
    check(first_vertex == packed_vertices && first_index == packed_indices,
          "Generated ice GPU packed totals differ");
    for (const auto &region : packed)
        for (const auto &vertex : region.vertices)
            for (auto byte : vertex)
                check(byte == word(records, 1),
                      "Generated ice GPU packed vertex differs");
    for (const auto &region : packed)
        for (auto index : region.indices)
            check(index == word(records, 2),
                  "Generated ice GPU local index differs");
    check(records.peek() == std::char_traits<char>::eof(),
          "Trailing original complete ice GPU bytes");
    if (!output_path) return;
    check(!std::filesystem::exists(output_path),
          "Generated ice GPU output already exists");
    std::ofstream output(output_path, std::ios::binary);
    const auto write_word = [&](std::uint32_t value, unsigned bytes = 4) {
        for (unsigned i = 0; i < bytes; ++i)
            output.put(static_cast<char>(value >> (8 * i)));
    };
    output.write("C5IRTB02", 8);
    output.write(reinterpret_cast<const char *>(exe.data()), exe.size());
    output.write(reinterpret_cast<const char *>(xml.data()), xml.size());
    output.write(reinterpret_cast<const char *>(graph_hash.data()),
                 graph_hash.size());
    write_word(static_cast<std::uint32_t>(packed.size()));
    write_word(packed_vertices);
    write_word(packed_indices);
    write_word(seed);
    for (auto value : actual.component_box)
        write_word(std::bit_cast<std::uint32_t>(value));
    first_vertex = first_index = 0;
    for (const auto &region : packed) {
        write_word(first_vertex);
        write_word(first_index);
        write_word(static_cast<std::uint32_t>(region.vertices.size()));
        write_word(static_cast<std::uint32_t>(region.indices.size()));
        for (auto value : region.bounds)
            write_word(std::bit_cast<std::uint32_t>(value));
        first_vertex += static_cast<std::uint32_t>(region.vertices.size());
        first_index += static_cast<std::uint32_t>(region.indices.size());
    }
    for (const auto &region : packed)
        for (const auto &vertex : region.vertices)
            output.write(reinterpret_cast<const char *>(vertex.data()),
                         vertex.size());
    for (const auto &region : packed)
        for (auto index : region.indices) write_word(index, 2);
    check(bool(output), "Generated ice GPU output failed");
}
void ice_complete_mesh_small_only() {
    TerrainIceIntermediateRegionGeometry geometry;
    geometry.regions.push_back({0, 0, 3, 0});
    geometry.vertices = {{{0.0f, 0.0f}, {1.0f, 0.0f}, {0.0f, 1.0f}}};
    geometry.indices = {{{0, 1, 0}, {1, 2, 0}, {2, 0, 0}}};
    TerrainIceSmallMeshSettings small;
    small.scale = 12.0f;
    small.height_min = 0.1f; small.height_max = 3.6f;
    small.depth_min = 6.1f; small.depth_max = 6.6f;
    small.skirt_slope = 0.0015625f;
    TerrainIceLargeTopSettings large;
    large.scale = small.scale;
    const std::array<float, 4> box{-2.0f, -2.0f, 2.0f, 2.0f};
    auto mesh = build_terrain_ice_complete_mesh(
        geometry, box, small, large, {}, 0, 0, 0, 0.0078125f, 8.0f, 0);
    check(mesh.component_box == box && mesh.combined.vertices.size() == 6 &&
          mesh.combined.indices.size() == 21,
          "Small-only ice contour did not produce a complete mesh");
    const auto gpu = build_terrain_ice_gpu_regions(
        mesh.component_box, mesh.combined.vertices, mesh.combined.indices,
        25.0f, 8.0f);
    check(gpu.size() == 1 && !gpu[0].vertices.empty() &&
          gpu[0].indices.size() == 21,
          "Small-only ice contour did not reach a GPU region");
}
void ice_horizontal_sampling_fixture(const char *events_path,
                                     const char *construction_path) {
    std::ifstream events(events_path, std::ios::binary);
    char magic[8];
    check(bool(events.read(magic, 8)) &&
          std::string_view(magic, 8) == "C5IHLN01",
          "Invalid original horizontal ice events");
    const auto exe = fixed_bytes<32>(events);
    const auto xml = fixed_bytes<32>(events);
    const auto graph_hash = fixed_bytes<32>(events);
    const auto line_count = word(events);
    check(line_count <= 10'000, "Original horizontal ice line count changed");
    std::vector<TerrainIceHorizontalSweepLine> lines(line_count);
    for (auto &line : lines) {
        line.y = std::bit_cast<float>(word(events));
        const auto count = word(events);
        check(count <= 1'000'000, "Original horizontal ice event count changed");
        line.events.resize(count);
        for (auto &event : line.events)
            event = {std::bit_cast<float>(word(events)),
                     word(events), word(events)};
    }
    check(events.peek() == std::char_traits<char>::eof(),
          "Trailing original horizontal ice events");
    std::ifstream construction(construction_path, std::ios::binary);
    check(bool(construction.read(magic, 8)) &&
          std::string_view(magic, 8) == "C5IRCS01",
          "Invalid original horizontal ice construction");
    check(fixed_bytes<32>(construction) == exe &&
          fixed_bytes<32>(construction) == xml &&
          fixed_bytes<32>(construction) == graph_hash,
          "Original horizontal ice fixture identity differs");
    TerrainIceVerticalInsertionGraph source;
    const auto source_points = word(construction), source_edges = word(construction);
    const bool full = source_points == 142913;
    check(source_points == (full ? 142913u : 73351u) &&
          source_edges == (full ? 113742u : 70047u),
          "Original horizontal ice source dimensions changed");
    source.points.resize(source_points);
    for (auto &point : source.points)
        point = {std::bit_cast<float>(word(construction)),
                 std::bit_cast<float>(word(construction))};
    source.edges.resize(source_edges);
    for (auto &edge : source.edges) {
        edge.first_point = word(construction);
        edge.second_point = word(construction);
        const auto flags = word(construction);
        check((flags & 0xffu) <= 1,
              "Original horizontal ice source edge flag changed");
        edge.active = static_cast<std::uint8_t>(flags & 0xffu);
    }
    const auto actual = sample_terrain_ice_horizontal_lattice(std::move(source), lines);
    const auto points = word(construction), edges = word(construction);
    check(points == (full ? 147111u : 74503u) &&
          edges == (full ? 227072u : 139401u) &&
          actual.points.size() == points && actual.edges.size() == edges,
          "Ported horizontal ice lattice dimensions differ");
    for (const auto &point : actual.points)
        check(std::bit_cast<std::uint32_t>(point[0]) == word(construction) &&
              std::bit_cast<std::uint32_t>(point[1]) == word(construction),
              "Ported horizontal ice point differs");
    for (const auto &edge : actual.edges) {
        const auto first = word(construction), second = word(construction);
        const auto flags = word(construction);
        check(edge.first_point == first && edge.second_point == second &&
              edge.active == (flags & 0xffu),
              "Ported horizontal ice edge differs");
    }
}
void ice_refinement_adjacency_fixture(const char *path) {
    std::ifstream in(path, std::ios::binary);
    char magic[8];
    check(bool(in.read(magic, 8)) &&
          std::string_view(magic, 8) == "C5IRFN01",
          "Invalid original ice refinement fixture");
    const auto exe = fixed_bytes<32>(in);
    const auto xml = fixed_bytes<32>(in);
    const auto source = fixed_bytes<32>(in);
    const auto points = word(in), edges = word(in), slots = word(in);
    check(slots == 2 * edges &&
          ((points == 74503 && edges == 145126) ||
           (points == 147111 && edges == 262954)),
          "Original ice refinement dimensions changed");
    const auto digest = [](const std::array<std::uint8_t, 32> &bytes) {
        std::string result;
        static constexpr char digits[] = "0123456789abcdef";
        for (const auto byte : bytes) {
            result.push_back(digits[byte >> 4]);
            result.push_back(digits[byte & 15]);
        }
        return result;
    };
    check(digest(exe) == "7c9f93d512c49a7ab2bc70f51c6caa10a82ddd697ac11d8a24ff1bb7ce7c3686" &&
          digest(xml) == "ab594d596e9dc386ca2cabcd952799a97190093a70cbc3b8486c7b2593318f1b" &&
          digest(source) == (points == 74503 ?
              "bd27329ec3ceb221cf4da2c20cad1afdb53fe496ce452efd32054ba176cdc4af" :
              "fb9fbd46a0e81afb5a9e5e8643a569d2367e4c8494a64c42c5387ba7214e37fc"),
          "Original ice refinement source identity differs");
    std::vector<std::array<float, 2>> positions(points);
    std::vector<std::array<std::uint32_t, 2>> spans(points);
    for (std::size_t i = 0; i < points; ++i) {
        positions[i] = {std::bit_cast<float>(word(in)),
                        std::bit_cast<float>(word(in))};
        spans[i] = {word(in), word(in)};
    }
    std::vector<TerrainIceTessellationEdge> graph_edges(edges);
    for (auto &edge : graph_edges) {
        edge.first_point = word(in);
        edge.second_point = word(in);
        const auto native_flags = word(in);
        check((native_flags & 0xffu) <= 1,
              "Original ice refinement edge flag changed");
        edge.active = static_cast<std::uint8_t>(native_flags & 0xffu);
    }
    const auto graph = assemble_terrain_ice_refined_graph(positions, graph_edges);
    check(graph.points.size() == points && graph.edges.size() == edges &&
          graph.halfedge_edges.size() == slots,
          "Ice refinement graph extent differs");
    std::vector<std::uint32_t> incident(slots);
    std::vector<std::uint32_t> remaining(points);
    for (std::size_t i = 0; i < points; ++i) {
        check(graph.points[i].halfedge_count == spans[i][0] &&
              graph.points[i].first_halfedge == spans[i][1],
              "Original ice refinement point span differs");
        remaining[i] = spans[i][0];
    }
    for (std::uint32_t edge_id = 0; edge_id < edges; ++edge_id) {
        const auto &edge = graph_edges[edge_id];
        for (const auto point : {edge.first_point, edge.second_point})
            incident[spans[point][1] + --remaining[point]] = edge_id;
    }
    for (std::size_t i = 0; i < slots; ++i)
        check(incident[i] == word(in),
              "Original ice initial incidence differs");
    for (std::size_t i = 0; i < slots; ++i)
        check(incident[i] == word(in),
              "Original ice pre-orientation incidence differs");
    for (std::size_t i = 0; i < slots; ++i)
        check(graph.halfedge_edges[i] == word(in),
              "Original ice oriented incidence differs");
    check(in.peek() == std::char_traits<char>::eof(),
          "Trailing original ice refinement bytes");
}
void ice_exterior_walk_fixture(const char *path, bool topology = false,
                               const char *combined_mesh_path = nullptr,
                               const char *displacement_dds_path = nullptr) {
    std::ifstream in(path, std::ios::binary);
    char magic[8];
    check(bool(in.read(magic, 8)) && std::string_view(magic, 8) ==
          (topology ? "C5ITBW02" : "C5ITBW01"),
          "Invalid original ice boundary-walk fixture");
    const auto hex = [](const std::array<std::uint8_t, 32> &bytes) {
        std::string result;
        static constexpr char digits[] = "0123456789abcdef";
        for (const auto byte : bytes) {
            result.push_back(digits[byte >> 4]);
            result.push_back(digits[byte & 15]);
        }
        return result;
    };
    const auto exe_hash = hex(fixed_bytes<32>(in));
    const auto xml_hash = hex(fixed_bytes<32>(in));
    const auto graph_hash = hex(fixed_bytes<32>(in));
    const auto point_count = word(in), edge_count = word(in), halfedge_count = word(in);
    const bool full = point_count == 147111 && edge_count == 262954 &&
        halfedge_count == 525908 &&
        graph_hash == "fb9fbd46a0e81afb5a9e5e8643a569d2367e4c8494a64c42c5387ba7214e37fc";
    const bool prefix = point_count == 74503 && edge_count == 145126 &&
        halfedge_count == 290252 &&
        graph_hash == "bd27329ec3ceb221cf4da2c20cad1afdb53fe496ce452efd32054ba176cdc4af";
    check(exe_hash == "7c9f93d512c49a7ab2bc70f51c6caa10a82ddd697ac11d8a24ff1bb7ce7c3686" &&
          xml_hash == "ab594d596e9dc386ca2cabcd952799a97190093a70cbc3b8486c7b2593318f1b" &&
          (full || prefix), "Original ice boundary-walk source identity differs");
    TerrainIceTessellationGraph graph;
    graph.points.resize(point_count);
    for (auto &point : graph.points) {
        for (auto &coordinate : point.position)
            coordinate = std::bit_cast<float>(word(in));
        point.halfedge_count = word(in);
        point.first_halfedge = word(in);
    }
    graph.edges.resize(edge_count);
    for (auto &edge : graph.edges) {
        edge.first_point = word(in);
        edge.second_point = word(in);
        const auto active = word(in);
        check(active <= 1, "Original ice boundary edge flag changed");
        edge.active = static_cast<std::uint8_t>(active);
    }
    graph.halfedge_edges.resize(halfedge_count);
    for (auto &edge : graph.halfedge_edges) edge = word(in);
    const auto actual = select_terrain_ice_exterior_halfedges(graph);
    check(actual.size() == halfedge_count, "Ice exterior walk extent differs");
    std::uint32_t selected_count = 0;
    for (std::size_t i = 0; i < actual.size(); ++i) {
        const auto expected = word(in, 1);
        if (actual[i] != expected)
            throw std::runtime_error("Original ice boundary side differs at halfedge " +
                                     std::to_string(i) + ": expected " +
                                     std::to_string(expected) + ", actual " +
                                     std::to_string(actual[i]));
        selected_count += actual[i];
        ++checks;
    }
    check(selected_count == (full ? 35882u : 5725u),
          "Original ice exterior boundary count differs");
    if (topology) {
        const auto expected_max = word(in), expected_indices = word(in);
        const auto prepared = prepare_terrain_ice_tessellation(graph);
        check(prepared.max_polygon_vertices == expected_max &&
              prepared.polygon_indices == expected_indices,
              "Original ice polygon reservation differs");
        check(prepared.flags.size() == halfedge_count,
              "Original ice polygon flag extent differs");
        for (std::size_t i = 0; i < halfedge_count; ++i)
            check(prepared.flags[i] == word(in, 1),
                  "Original ice propagated side differs");
    }
    if (combined_mesh_path) {
        std::ifstream mesh(combined_mesh_path, std::ios::binary);
        char mesh_magic[8];
        check(bool(mesh.read(mesh_magic, 8)) &&
              std::string_view(mesh_magic, 8) == "C5IMCB01",
              "Invalid original combined ice mesh");
        check(hex(fixed_bytes<32>(mesh)) == exe_hash &&
              hex(fixed_bytes<32>(mesh)) == xml_hash &&
              hex(fixed_bytes<32>(mesh)) == graph_hash,
              "Original large ice cap source identity differs");
        const auto scale = word(mesh), index_count = word(mesh);
        const auto vertex_count = word(mesh), seed = word(mesh);
        const auto precision = word(mesh);
        check(scale == 0x41bfff7bu && precision == 53 &&
              seed == (full ? 0x270ecbf1u : 0x7e1dbec8u) &&
              index_count == (full ? 994311u : 464238u) &&
              vertex_count == (full ? 184103u : 80242u),
              "Original combined ice mesh dimensions changed");
        const auto small_indices = full ? 4053u : 51u;
        const auto small_vertices = full ? 1110u : 14u;
        for (std::uint32_t i = 0; i < small_indices; ++i) word(mesh);
        const auto large = build_terrain_ice_large_indices(graph, small_vertices);
        check(large.size() == index_count - small_indices,
              "Original large ice index extent differs");
        for (std::size_t i = 0; i < large.size(); ++i)
            check(large[i] == word(mesh), "Original large ice index differs");
        mesh.seekg(static_cast<std::streamoff>(16u * small_vertices), std::ios::cur);
        check(bool(mesh), "Original small ice vertex prefix is missing");
        std::vector<std::array<float, 4>> top;
        if (displacement_dds_path) {
            std::ifstream dds(displacement_dds_path, std::ios::binary | std::ios::ate);
            check(bool(dds) && dds.tellg() == 87509,
                  "Original ice displacement DDS extent differs");
            std::vector<std::uint8_t> bytes(static_cast<std::size_t>(dds.tellg()));
            dds.seekg(0);
            dds.read(reinterpret_cast<char *>(bytes.data()),
                     static_cast<std::streamsize>(bytes.size()));
            check(bool(dds) && std::string_view(
                  reinterpret_cast<const char *>(bytes.data()), 4) == "DDS ",
                  "Original ice displacement DDS is invalid");
            TerrainIceLargeTopSettings settings;
            settings.scale = std::bit_cast<float>(scale);
            settings.displacement_frequency = 0.1f;
            settings.displacement_min = -8.0f;
            settings.displacement_max = 15.0f;
            settings.height = 10.0f;
            settings.height_ceiling = 25.0f;
            top = build_terrain_ice_large_top_vertices(
                graph, std::span(bytes).subspan(128), 256, 256, 256, settings);
            check(top.size() == graph.points.size(),
                  "Original large ice top vertex extent differs");
        }
        for (std::size_t i = 0; i < graph.points.size(); ++i) {
            if (displacement_dds_path) {
                for (const float coordinate : top[i])
                    check(std::bit_cast<std::uint32_t>(coordinate) == word(mesh),
                          "Original large ice top vertex byte differs");
            } else {
                const auto &point = graph.points[i];
                check(std::bit_cast<std::uint32_t>(point.position[0]) == word(mesh) &&
                      std::bit_cast<std::uint32_t>(point.position[1]) == word(mesh),
                      "Original large ice top XY differs");
                word(mesh); word(mesh);
            }
        }
        const auto border = build_terrain_ice_large_border_vertices(
            graph, std::bit_cast<float>(scale), 0.0078125f, 8.0f);
        check(border.size() == selected_count,
              "Original large ice border vertex extent differs");
        for (const auto &vertex : border)
            for (const float coordinate : vertex)
                check(std::bit_cast<std::uint32_t>(coordinate) == word(mesh),
                      "Original large ice border vertex byte differs");
        check(mesh.peek() == std::char_traits<char>::eof(),
              "Trailing original combined ice vertex bytes");
    }
    check(in.peek() == std::char_traits<char>::eof(),
          "Trailing original ice boundary-walk fixture bytes");
}
void ice_triangulation_examples() {
    const auto compare = [](std::initializer_list<std::array<float, 2>> points,
                            std::initializer_list<std::uint32_t> native_cap) {
        TerrainIceIntermediateRegionGeometry graph;
        graph.vertices.assign(points);
        const auto count = static_cast<std::uint32_t>(graph.vertices.size());
        graph.regions.push_back({0, 0, count, 0});
        for (std::uint32_t i = 0; i < count; ++i)
            graph.indices.push_back({i, (i + 1) % count, 0});
        TerrainIceSmallMeshSettings settings;
        settings.scale = std::bit_cast<float>(0x41bfff7bu);
        settings.height_min = 0.1f; settings.height_max = 3.6f;
        settings.depth_min = 6.1f; settings.depth_max = 6.6f;
        settings.skirt_slope = 0.0015625f;
        const auto result = build_terrain_ice_small_mesh(graph, settings, 0x270ecbf1u);
        check(result.vertices.size() == 2 * count,
              "Synthetic native ice polygon vertex count differs");
        check(result.indices.size() == native_cap.size() + 6 * count,
              "Synthetic native ice polygon triangle count differs");
        auto expected = native_cap.begin();
        for (std::size_t i = 0; i < native_cap.size(); ++i)
            check(result.indices[i] == *expected++,
                  "Synthetic native ice cap triangle differs");
    };
    // Expected cap indices were captured directly from retail 0x6BA100.
    compare({{0,0},{2,0},{3,1},{1.5f,2},{0,1}},
            {0,1,2, 0,2,3, 0,3,4});
    compare({{0,0},{2,0},{3,1},{2,2},{1.5f,0.7f},{0,1}},
            {0,1,2, 2,3,4, 4,5,0, 4,0,2});
    compare({{0,1},{1.5f,0.7f},{2,2},{3,1},{2,0},{0,0}},
            {1,2,3, 1,3,4, 1,4,5, 1,5,0});
    compare({{0,0},{2,0},{3,1},{2,2},{3,3},{1,4},{-1,3},{0,2}},
            {0,1,2, 0,2,3, 0,3,4, 0,4,5, 5,6,7, 5,7,0});
    compare({{10,0},{14,1},{17,3},{19,6},{20,10},{19,14},{17,17},{14,19},
             {10,20},{6,19},{3,17},{1,14},{0,10},{1,6},{3,3},{6,1}},
            {0,1,2, 0,2,3, 0,3,4, 0,4,5, 0,5,6, 0,6,7, 0,7,8,
             0,8,9, 0,9,10, 0,10,11, 0,11,12, 0,12,13, 0,13,14, 0,14,15});
    // An original large-ice face where a newly wound ear still contains
    // another point. Retail 0x6BA100 rotates and checks the interior again.
    compare({{-281.1351318359375f,-633.4782104492188f},
             {-281.66668701171875f,-633.3519897460938f},
             {-281.9009704589844f,-633.8516845703125f},
             {-281.8175964355469f,-633.2451171875f},
             {-282.0735778808594f,-633.16064453125f},
             {-282.7485656738281f,-633.6015625f},
             {-282.2005615234375f,-633.1043090820312f},
             {-282.52386474609375f,-633.0154418945312f},
             {-282.960205078125f,-633.5492553710938f},
             {-282.7242126464844f,-632.97998046875f},
             {-283.127197265625f,-632.8291015625f},
             {-283.127197265625f,-634.0054931640625f},
             {-281.1272277832031f,-634.0054931640625f},
             {-281.1272277832031f,-633.481201171875f}},
            {0,1,2, 2,3,4, 2,4,5, 5,6,7, 5,7,8, 8,9,10,
             8,10,11, 12,13,0, 12,0,2, 2,5,8, 2,8,11, 2,11,12});
}
} // namespace
int main(int argc, char **argv) {
    try {
        if ((argc == 5 || argc == 6 || argc == 7) &&
            std::string_view(argv[1]) == "--ice-complete-mesh-fixture") {
            ice_complete_mesh_fixture(argv[2], argv[3], argv[4],
                                      argc >= 6 ? argv[5] : nullptr,
                                      argc == 7 ? argv[6] : nullptr);
            std::cout << "PASS generated complete ice mesh: "
                      << checks << " checks\n";
            return 0;
        }
        if (argc == 5 && std::string_view(argv[1]) == "--ice-generated-lattice-fixture") {
            ice_generated_lattice_fixture(argv[2], argv[3], argv[4]);
            std::cout << "PASS generated original ice lattice: "
                      << checks << " checks\n";
            return 0;
        }
        if (argc == 6 && std::string_view(argv[1]) == "--ice-contour-intersections-fixture") {
            ice_contour_intersections_fixture(argv[2], argv[3], argv[4], argv[5]);
            std::cout << "PASS original ice contour intersections: "
                      << checks << " checks\n";
            return 0;
        }
        if (argc == 5 && std::string_view(argv[1]) == "--ice-current-vertical-intersections-fixture") {
            ice_current_vertical_intersections_fixture(argv[2], argv[3], argv[4]);
            std::cout << "PASS current native vertical ice intersections: "
                      << checks << " checks\n";
            return 0;
        }
        if (argc == 4 && std::string_view(argv[1]) == "--ice-horizontal-sampling-fixture") {
            ice_horizontal_sampling_fixture(argv[2], argv[3]);
            std::cout << "PASS original horizontal ice sampling: "
                      << checks << " checks\n";
            return 0;
        }
        if (argc == 6 && std::string_view(argv[1]) == "--ice-vertical-insertion-fixture") {
            ice_vertical_insertion_fixture(argv[2], argv[3], argv[4], argv[5]);
            std::cout << "PASS original vertical ice insertion: "
                      << checks << " checks\n";
            return 0;
        }
        if (argc == 4 && std::string_view(argv[1]) == "--ice-large-source-fixture") {
            ice_large_source_fixture(argv[2], argv[3]);
            std::cout << "PASS original selected large ice source graph: "
                      << checks << " checks\n";
            return 0;
        }
        if (argc == 4 && std::string_view(argv[1]) == "--ice-refinement-stitch-fixture") {
            ice_refinement_stitch_fixture(argv[2], argv[3]);
            std::cout << "PASS original ice refinement boundary stitching: "
                      << checks << " checks\n";
            return 0;
        }
        if (argc == 3 && std::string_view(argv[1]) == "--ice-refinement-construction-fixture") {
            ice_refinement_construction_fixture(argv[2]);
            std::cout << "PASS original ice refinement boundary points: "
                      << checks << " checks\n";
            return 0;
        }
        if (argc == 3 && std::string_view(argv[1]) == "--ice-refinement-adjacency-fixture") {
            ice_refinement_adjacency_fixture(argv[2]);
            std::cout << "PASS original ice refinement adjacency: "
                      << checks << " checks\n";
            return 0;
        }
        if (argc == 3 && std::string_view(argv[1]) == "--ice-exterior-walk-fixture") {
            ice_exterior_walk_fixture(argv[2]);
            std::cout << "PASS original ice exterior boundary walk: " << checks << " checks\n";
            return 0;
        }
        if (argc == 3 && std::string_view(argv[1]) == "--ice-topology-fixture") {
            ice_exterior_walk_fixture(argv[2], true);
            std::cout << "PASS original ice polygon preparation: " << checks << " checks\n";
            return 0;
        }
        if (argc == 4 && std::string_view(argv[1]) == "--ice-large-indices-fixture") {
            ice_exterior_walk_fixture(argv[2], false, argv[3]);
            std::cout << "PASS original large ice indices and border vertices: "
                      << checks << " checks\n";
            return 0;
        }
        if (argc == 5 && std::string_view(argv[1]) == "--ice-large-vertices-fixture") {
            ice_exterior_walk_fixture(argv[2], false, argv[3], argv[4]);
            std::cout << "PASS original large ice mesh indices and all vertices: "
                      << checks << " checks\n";
            return 0;
        }
        if (argc == 4 && std::string_view(argv[1]) == "--ice-gpu-regions-authored-fixture") {
            ice_partition_fixture(argv[2], argv[3], true, true);
            std::cout << "PASS authored ice combined-to-GPU regions: " << checks << " checks\n";
            return 0;
        }
        if (argc == 2 && std::string_view(argv[1]) == "--ice-triangulation-examples") {
            ice_triangulation_examples();
            std::cout << "PASS original ice polygon triangulation: " << checks << " checks\n";
            return 0;
        }
        if (argc == 4 && std::string_view(argv[1]) == "--ice-small-mesh-fixture") {
            ice_small_mesh_fixture(argv[2], argv[3]);
            std::cout << "PASS original small ice mesh: " << checks << " checks\n";
            return 0;
        }
        if (argc == 4 && std::string_view(argv[1]) == "--ice-gpu-regions-fixture") {
            ice_partition_fixture(argv[2], argv[3], true);
            std::cout << "PASS original ice combined-to-GPU regions: " << checks << " checks\n";
            return 0;
        }
        if (argc == 4 && std::string_view(argv[1]) == "--ice-normal-fixture") {
            ice_normal_fixture(argv[2], argv[3]);
            std::cout << "PASS original ice normal preparation: " << checks << " checks\n";
            return 0;
        }
        if (argc == 4 && std::string_view(argv[1]) == "--ice-partition-fixture") {
            ice_partition_fixture(argv[2], argv[3]);
            std::cout << "PASS original ice GPU partitions: " << checks << " checks\n";
            return 0;
        }
        if (argc == 3 && std::string_view(argv[1]) == "--ice-vertex-packing-fixture") {
            ice_vertex_packing_fixture(argv[2]);
            std::cout << "PASS original ice vertex packing: " << checks << " checks\n";
            return 0;
        }
        if (argc == 3 && std::string_view(argv[1]) == "--ice-region-processing-fixture") {
            ice_region_processing_fixture(argv[2]);
            std::cout << "PASS original ice region processing: " << checks << " checks\n";
            return 0;
        }
        if (argc == 4 && std::string_view(argv[1]) == "--ice-saved-region-processing-fixture") {
            ice_saved_region_processing_fixture(argv[2], argv[3]);
            std::cout << "PASS selected-save ice region processing: " << checks << " checks\n";
            return 0;
        }
        if ((argc == 6 || argc == 7) && std::string_view(argv[1]) == "--ice-saved-region-geometry-fixture") {
            ice_saved_region_geometry_fixture(argv[2], argv[3], argv[4], argv[5],
                                             argc == 7 ? argv[6] : nullptr);
            std::cout << "PASS selected-save ice region geometry: " << checks << " checks\n";
            return 0;
        }
        if (argc == 4 && std::string_view(argv[1]) == "--ice-source-adjacency-fixture") {
            ice_saved_source_adjacency_fixture(argv[2], argv[3]);
            std::cout << "PASS selected-save ice source adjacency: " << checks << " checks\n";
            return 0;
        }
        if (argc == 5 && std::string_view(argv[1]) == "--ice-saved-region-finalization-fixture") {
            ice_saved_region_processing_fixture(argv[2], argv[3], argv[4]);
            std::cout << "PASS selected-save ice region finalization: " << checks << " checks\n";
            return 0;
        }
        if (argc == 6 && std::string_view(argv[1]) == "--ice-saved-postprocess-face-recount-fixture") {
            ice_saved_region_processing_fixture(argv[2], argv[3], argv[4], argv[5]);
            std::cout << "PASS selected-save ice postprocess face recount: " << checks << " checks\n";
            return 0;
        }
        if (argc == 3 && std::string_view(argv[1]) == "--ice-region-prune-mutation-fixture") {
            ice_region_prune_mutation_fixture(argv[2]);
            std::cout << "PASS original ice region prune mutation: " << checks << " checks\n";
            return 0;
        }
        if (argc == 3 && std::string_view(argv[1]) == "--ice-region-shrink-fixture") {
            ice_region_shrink_fixture(argv[2]);
            std::cout << "PASS original ice region shrink: " << checks << " checks\n";
            return 0;
        }
        if (argc == 3 && std::string_view(argv[1]) == "--ice-region-geometry-fixture") {
            ice_region_geometry_fixture(argv[2]);
            std::cout << "PASS original ice region geometry: " << checks << " checks\n";
            return 0;
        }
        if (argc == 3 && std::string_view(argv[1]) == "--ice-region-walk-fixture") {
            ice_region_walk_fixture(argv[2]);
            std::cout << "PASS original ice region walks: " << checks << " checks\n";
            return 0;
        }
        if (argc == 3 && std::string_view(argv[1]) == "--ice-closed-walk-fixture") {
            ice_closed_walk_fixture(argv[2]);
            std::cout << "PASS original ice closed walks: " << checks << " checks\n";
            return 0;
        }
        if (argc == 3 && std::string_view(argv[1]) == "--ice-adjacency-sort-fixture") {
            ice_adjacency_sort_fixture(argv[2]);
            std::cout << "PASS original ice adjacency sort: " << checks << " checks\n";
            return 0;
        }
        if (argc == 3 && std::string_view(argv[1]) == "--ice-adjacency-fixture") {
            ice_adjacency_fixture(argv[2]);
            std::cout << "PASS original ice intermediate adjacency: " << checks << " checks\n";
            return 0;
        }
        if (argc == 3 && std::string_view(argv[1]) == "--ice-region-fill-fixture") {
            ice_region_fill_fixture(argv[2]);
            std::cout << "PASS original ice intermediate-region fill: " << checks << " checks\n";
            return 0;
        }
        if (argc == 3 && std::string_view(argv[1]) == "--ice-region-count-fixture") {
            ice_region_count_fixture(argv[2]);
            std::cout << "PASS original ice intermediate-region counts: " << checks << " checks\n";
            return 0;
        }
        if (argc == 3 && std::string_view(argv[1]) == "--ice-region-prune-rng-fixture") {
            ice_region_prune_rng_fixture(argv[2]);
            std::cout << "PASS original ice region-pruning RNG: " << checks << " checks\n";
            return 0;
        }
        if (argc == 4 && std::string_view(argv[1]) == "--ice-saved-group-projection-fixture") {
            ice_group_projection_fixture(argv[2], argv[3], 9, 65,
                "862f218e8d7a1152076f02b063e3216c826f1458f3afce5c68579e40636e6a56",
                selected_save_ice_projection());
            std::cout << "PASS selected-save ice group projection: " << checks << " checks\n";
            return 0;
        }
        if (argc == 5 && std::string_view(argv[1]) == "--ice-saved-projection-fixtures") {
            ice_saved_projection_fixture(argv[2], argv[3], 24, 12345, 0xb3bf0865u);
            ice_saved_projection_fixture(argv[2], argv[4], 53, 12345, 0xb3bf0865u);
            std::cout << "PASS selected-save ice projection meshes: " << checks << " checks\n";
            return 0;
        }
        if (argc == 5 && std::string_view(argv[1]) == "--ice-saved-original-seed-fixtures") {
            ice_saved_projection_fixture(argv[2], argv[3], 24, 0, 0x5b5346fcu);
            ice_saved_projection_fixture(argv[2], argv[4], 53, 0, 0x5b5346fcu);
            std::cout << "PASS selected-save native-seed ice meshes: " << checks << " checks\n";
            return 0;
        }
        if (argc == 4 && std::string_view(argv[1]) == "--ice-current-original-seed-fixture") {
            ice_saved_projection_fixture(argv[2], argv[3], 53, 0, 0x59e23a30u,
                2066, 688,
                "fc8f977a3799d2215d45dd5a3ffcbec60650bfaec6261f2604c17b2f3b6fef0b");
            std::cout << "PASS current-save native-seed ice mesh: " << checks << " checks\n";
            return 0;
        }
        if (argc == 8 && std::string_view(argv[1]) == "--ice-group-projection-fixtures") {
            ice_group_projection_fixture(argv[2], argv[3], 9, 65,
                "862f218e8d7a1152076f02b063e3216c826f1458f3afce5c68579e40636e6a56",
                native_ice_projection());
            ice_group_projection_fixture(argv[4], argv[5], 1, 40,
                "26477e179f3cc52aef2bd8a316d8f536407e59c6c36626346b3639d721aa4f17",
                native_ice_projection());
            ice_group_projection_fixture(argv[6], argv[7], 1, 6,
                "f27fa7b421f79ee54a6b77ac628b995eaaf479e48a1ba72d79745bf2641227a9",
                native_ice_projection());
            std::cout << "PASS native ice group projection: " << checks << " checks\n";
            return 0;
        }
        if (argc == 12 && std::string_view(argv[1]) == "--ice-mesh-fixtures") {
            const auto precision = static_cast<std::uint32_t>(std::stoul(argv[11]));
            check(precision == 24 || precision == 53, "Invalid ice mesh x87 precision");
            ice_exterior_fixture(argv[2], argv[3], argv[4], 1922, 122,
                precision, 0xb3bf0865u, 636,
                {1119881220u, 1092523936u, 1162870491u, 1153029377u},
                "862f218e8d7a1152076f02b063e3216c826f1458f3afce5c68579e40636e6a56");
            ice_exterior_fixture(argv[5], argv[6], argv[7], 1064, 80,
                precision, 0xb2a506f5u, 332,
                {1084775267u, 1092512856u, 1159724832u, 1125248000u},
                "26477e179f3cc52aef2bd8a316d8f536407e59c6c36626346b3639d721aa4f17");
            ice_exterior_fixture(argv[8], argv[9], argv[10], 288, 0,
                precision, 0xcc372a81u, 104,
                {1150844872u, 1141888521u, 1152353613u, 1144826428u},
                "f27fa7b421f79ee54a6b77ac628b995eaaf479e48a1ba72d79745bf2641227a9");
            std::cout << "PASS native ice complete meshes: " << checks << " checks\n";
            return 0;
        }
        if (argc == 6 && std::string_view(argv[1]) == "--ice-spline-fixtures") {
            const auto precision = static_cast<std::uint32_t>(std::stoul(argv[5]));
            check(precision == 24 || precision == 53, "Invalid ice spline x87 precision");
            ice_spline_fixture(argv[2], 1800,
                "862f218e8d7a1152076f02b063e3216c826f1458f3afce5c68579e40636e6a56", precision);
            ice_spline_fixture(argv[3], 984,
                "26477e179f3cc52aef2bd8a316d8f536407e59c6c36626346b3639d721aa4f17", precision);
            ice_spline_fixture(argv[4], 288,
                "f27fa7b421f79ee54a6b77ac628b995eaaf479e48a1ba72d79745bf2641227a9", precision);
            std::cout << "PASS native ice spline triplets: " << checks << " checks\n";
            return 0;
        }
        if (argc == 6 && std::string_view(argv[1]) == "--ice-control-fixtures") {
            const auto precision = static_cast<std::uint32_t>(std::stoul(argv[5]));
            check(precision == 24 || precision == 53,
                  "Invalid native ice x87 precision");
            ice_control_fixture(argv[2], 159,
                "862f218e8d7a1152076f02b063e3216c826f1458f3afce5c68579e40636e6a56", precision);
            ice_control_fixture(argv[3], 83,
                "26477e179f3cc52aef2bd8a316d8f536407e59c6c36626346b3639d721aa4f17", precision);
            ice_control_fixture(argv[4], 26,
                "f27fa7b421f79ee54a6b77ac628b995eaaf479e48a1ba72d79745bf2641227a9", precision);
            std::cout << "PASS native ice control points: " << checks << " checks\n";
            return 0;
        }
        if (argc == 4 && std::string_view(argv[1]) == "--ice-curve-fixtures") {
            ice_curve_settings_fixture(argv[2], 8);
            ice_curve_settings_fixture(argv[3], 12);
            std::cout << "PASS native ice curve settings: " << checks << " checks\n";
            return 0;
        }
        if (argc == 3 && std::string_view(argv[1]) == "--synthetic-wrap-row-fixture") {
            guards(); ice_input(); synthetic_wrap_row_fixture(argv[2]);
            std::cout << "PASS synthetic wrap-row ice owner: " << checks << " checks\n";
            return 0;
        }
        if (argc == 3 && std::string_view(argv[1]) == "--synthetic-ring-fixture") {
            guards(); ice_input(); synthetic_ring_fixture(argv[2]);
            std::cout << "PASS synthetic ice-ring owner: " << checks << " checks\n";
            return 0;
        }
        if ((argc >= 3 && argc <= 5) && std::string_view(argv[1]) == "--selected-ice-fixture") {
            guards(); ice_input(); selected_ice_fixture(argv[2], argc >= 4 ? argv[3] : nullptr,
                                                       argc == 5 ? argv[4] : nullptr);
            std::cout << "PASS selected-save ice owner: " << checks << " checks\n";
            return 0;
        }
        if (argc == 8 && std::string_view(argv[1]) == "--selected-ice-fixture") {
            guards(); ice_input();
            selected_ice_fixture(argv[2], argv[3], argv[4],
                static_cast<std::uint32_t>(std::stoul(argv[5])),
                static_cast<std::uint32_t>(std::stoul(argv[6])),
                static_cast<std::uint32_t>(std::stoul(argv[7])), false);
            std::cout << "PASS selected-save ice owner and original allocation: "
                      << checks << " checks\n";
            return 0;
        }
        if (argc > 4) throw std::runtime_error("Usage: civ5_terrain_auxiliary_owners_tests [owners.bin [finalize.bin [initialize.bin]]]");
        guards();
        ice_input();
        ice_complete_mesh_small_only();
        const auto full_cases = argc >= 2 ? oracle(argv[1]) : 0;
        const auto finalize_cases = argc >= 3 ? finalize_oracle(argv[2]) : 0;
        const auto initialization_cases = argc == 4 ? initialization_oracle(argv[3]) : 0;
        std::cout << "PASS auxiliary owners: " << checks << " checks, " << full_cases << " owner cases, "
                  << finalize_cases << " finalization cases, " << initialization_cases << " initialization cases, "
                  << table_bytes << " table bytes\n";
    } catch (const std::exception &e) {
        std::cerr << "FAIL auxiliary owners: " << e.what() << '\n'; return 1;
    }
}
