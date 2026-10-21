#include "civ5/terrain/terrain_auxiliary_owners.hpp"
#include <bit>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
std::vector<std::uint8_t> read_file(const char *path) {
    std::ifstream input(path, std::ios::binary | std::ios::ate);
    if (!input) throw std::runtime_error("Cannot open ice graph fixture");
    const auto size = input.tellg();
    if (size < 0) throw std::runtime_error("Cannot size ice graph fixture");
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
    input.seekg(0);
    if (!input.read(reinterpret_cast<char *>(bytes.data()), size))
        throw std::runtime_error("Cannot read ice graph fixture");
    return bytes;
}
std::uint32_t word(const std::vector<std::uint8_t> &bytes, std::size_t at) {
    if (at + 4 > bytes.size()) throw std::runtime_error("Truncated ice graph fixture");
    std::uint32_t value = 0;
    for (unsigned i = 0; i < 4; ++i) value |= std::uint32_t(bytes[at+i]) << (8*i);
    return value;
}
void put(std::vector<std::uint8_t> &bytes, std::size_t at, std::uint32_t value) {
    if (at + 4 > bytes.size()) throw std::runtime_error("Truncated ice graph fixture");
    for (unsigned i = 0; i < 4; ++i) bytes[at+i] = static_cast<std::uint8_t>(value >> (8*i));
}
int controlled(const char *path) {
    const auto fixture = read_file(path);
    if (fixture.size() < 76 || std::memcmp(fixture.data(), "C5IWGT01", 8))
        throw std::runtime_error("Wrong controlled ice-weight fixture tag");
    const auto cases = word(fixture, 72);
    std::size_t at = 76, differences = 0;
    for (std::uint32_t c = 0; c < cases; ++c) {
        const auto name_length = word(fixture, at);
        const auto profile_length = word(fixture, at+4);
        const auto precision = word(fixture, at+8);
        const auto point_count = word(fixture, at+12);
        const auto edge_count = word(fixture, at+16);
        const civ5::terrain::TerrainIceSourceWeightSettings settings{
            std::bit_cast<float>(word(fixture, at+20)),
            std::bit_cast<float>(word(fixture, at+24)),
            std::bit_cast<float>(word(fixture, at+28)),
            std::bit_cast<float>(word(fixture, at+32)), 0.0f};
        at += 36;
        if (at + name_length + profile_length > fixture.size())
            throw std::runtime_error("Truncated controlled ice-weight fixture label");
        const std::string name(reinterpret_cast<const char *>(fixture.data()+at), name_length);
        at += name_length + profile_length;
        const auto point_bytes = 24ull*point_count, edge_bytes = 36ull*edge_count;
        if (at + 2*(point_bytes+edge_bytes) > fixture.size())
            throw std::runtime_error("Truncated controlled ice-weight fixture records");
        std::vector<std::uint8_t> actual_points(fixture.begin()+at,
                                                fixture.begin()+at+point_bytes);
        at += point_bytes;
        std::vector<std::uint8_t> actual_edges(fixture.begin()+at,
                                               fixture.begin()+at+edge_bytes);
        at += edge_bytes;
        const auto expected_points = at;
        const auto expected_edges = expected_points + point_bytes;
        at += point_bytes + edge_bytes;
        std::vector<civ5::terrain::TerrainIceSourceWeightPoint> points(point_count);
        std::vector<civ5::terrain::TerrainIceSourceWeightEdge> edges(edge_count);
        for (std::size_t i = 0; i < point_count; ++i) {
            const auto p = 24*i;
            points[i].xy = {std::bit_cast<float>(word(actual_points, p)),
                            std::bit_cast<float>(word(actual_points, p+4))};
            points[i].active = actual_points[p+8];
        }
        for (std::size_t i = 0; i < edge_count; ++i) {
            const auto e = 36*i;
            edges[i].first = word(actual_edges, e);
            edges[i].second = word(actual_edges, e+4);
            edges[i].boundary = actual_edges[e+8];
        }
        civ5::terrain::weight_terrain_ice_source_graph(points, edges, {}, settings);
        for (std::size_t i = 0; i < point_count; ++i) {
            const auto p = 24*i;
            actual_points[p+8] = points[i].active;
            put(actual_points, p+12, std::bit_cast<std::uint32_t>(points[i].potential));
        }
        for (std::size_t i = 0; i < edge_count; ++i) {
            const auto e = 36*i;
            for (unsigned slot = 0; slot < 2; ++slot) {
                put(actual_edges, e+12+4*slot,
                    std::bit_cast<std::uint32_t>(edges[i].weights[slot]));
                put(actual_edges, e+20+4*slot,
                    std::bit_cast<std::uint32_t>(edges[i].propagation[slot]));
                put(actual_edges, e+28+4*slot,
                    std::bit_cast<std::uint32_t>(edges[i].pending[slot]));
            }
        }
        for (std::size_t i = 0; i < point_bytes; ++i)
            if (actual_points[i] != fixture[expected_points+i]) {
                if (differences < 12)
                    std::cout << "DIFF case=" << name << " precision=" << precision
                              << " point_offset=" << i << '\n';
                ++differences;
            }
        for (std::size_t i = 0; i < edge_bytes; ++i)
            if (actual_edges[i] != fixture[expected_edges+i]) {
                if (differences < 12)
                    std::cout << "DIFF case=" << name << " precision=" << precision
                              << " edge_offset=" << i << '\n';
                ++differences;
            }
    }
    if (at != fixture.size()) throw std::runtime_error("Trailing controlled ice-weight fixture bytes");
    std::cout << "controlled ice source-weight comparison cases=" << cases
              << " different_bytes=" << differences << '\n';
    return differences ? 1 : 0;
}
}
int main(int argc, char **argv) {
    try {
        if (argc != 3) throw std::runtime_error("Usage: ice_source_weight_port_probe CLIPPED WEIGHTED");
        if (std::string_view(argv[1]) == "--controlled") return controlled(argv[2]);
        auto clipped = read_file(argv[1]);
        const auto weighted = read_file(argv[2]);
        if (clipped.size() < 112 || weighted.size() < 128 ||
            std::memcmp(clipped.data(), "C5ICLP01", 8) ||
            std::memcmp(weighted.data(), "C5IWGS01", 8))
            throw std::runtime_error("Wrong selected ice graph fixture tags");
        const auto point_count = word(clipped, 104), edge_count = word(clipped, 108);
        if (point_count != word(weighted, 120) || edge_count != word(weighted, 124) ||
            112ull + 24ull*point_count + 36ull*edge_count != clipped.size() ||
            128ull + 24ull*point_count + 36ull*edge_count != weighted.size())
            throw std::runtime_error("Selected ice graph fixture extent changed");
        std::vector<civ5::terrain::TerrainIceSourceWeightPoint> points(point_count);
        std::vector<civ5::terrain::TerrainIceSourceWeightEdge> edges(edge_count);
        std::vector<std::array<std::uint32_t, 2>> endpoints(edge_count);
        const auto point_base = std::size_t{112};
        const auto edge_base = point_base + 24ull*point_count;
        for (std::size_t i = 0; i < point_count; ++i) {
            const auto at = point_base + 24*i;
            points[i].xy = {std::bit_cast<float>(word(clipped, at)),
                            std::bit_cast<float>(word(clipped, at+4))};
            points[i].active = clipped[at+8];
            points[i].potential = std::bit_cast<float>(word(clipped, at+12));
        }
        for (std::size_t i = 0; i < edge_count; ++i) {
            const auto at = edge_base + 36*i;
            auto &edge = edges[i];
            edge.first = word(clipped, at);
            edge.second = word(clipped, at+4);
            edge.boundary = clipped[at+8];
            endpoints[i] = {edge.first, edge.second};
        }
        auto adjacency = civ5::terrain::build_terrain_ice_source_adjacency(
            point_count, endpoints);
        const civ5::terrain::TerrainIceSourceWeightSettings settings{
            std::bit_cast<float>(word(weighted, 104)),
            std::bit_cast<float>(word(weighted, 108)),
            std::bit_cast<float>(word(weighted, 112)),
            std::bit_cast<float>(word(weighted, 116)),
            std::bit_cast<float>(0x41bfff7bu)};
        civ5::terrain::weight_terrain_ice_source_graph(points, edges, adjacency, settings);
        for (std::size_t i = 0; i < point_count; ++i) {
            const auto at = point_base + 24*i;
            clipped[at+8] = points[i].active;
            put(clipped, at+12, std::bit_cast<std::uint32_t>(points[i].potential));
        }
        for (std::size_t i = 0; i < edge_count; ++i) {
            const auto at = edge_base + 36*i;
            const auto &edge = edges[i];
            for (unsigned slot = 0; slot < 2; ++slot) {
                put(clipped, at+12+4*slot,
                    std::bit_cast<std::uint32_t>(edge.weights[slot]));
                put(clipped, at+20+4*slot,
                    std::bit_cast<std::uint32_t>(edge.propagation[slot]));
                put(clipped, at+28+4*slot,
                    std::bit_cast<std::uint32_t>(edge.pending[slot]));
            }
        }
        std::size_t differences = 0;
        for (std::size_t i = 0; i < clipped.size() - 112; ++i) {
            if (clipped[112+i] != weighted[128+i]) {
                if (differences < 12)
                    std::cout << "DIFF offset=" << i << " actual="
                              << unsigned(clipped[112+i]) << " original="
                              << unsigned(weighted[128+i]) << '\n';
                ++differences;
            }
        }
        std::cout << "ice source-weight comparison points=" << point_count
                  << " edges=" << edge_count << " different_bytes="
                  << differences << '\n';
        return differences ? 1 : 0;
    } catch (const std::exception &error) {
        std::cerr << "ice source-weight probe: " << error.what() << '\n';
        return 2;
    }
}
