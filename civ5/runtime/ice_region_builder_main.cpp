#include "civ5/assets/asset_source.hpp"
#include "civ5/assets/dds.hpp"
#include "civ5/content/manifest.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/terrain/terrain_auxiliary_owners.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
namespace fs = std::filesystem;
using namespace civ5::terrain;

void require(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}
std::vector<std::uint8_t> read_bounded(const fs::path &path,
                                       std::uint64_t maximum) {
    const auto size = fs::file_size(path);
    require(size <= maximum, "Ice input exceeds its size limit");
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
    std::ifstream input(path, std::ios::binary);
    require(bool(input.read(reinterpret_cast<char *>(bytes.data()),
                            static_cast<std::streamsize>(bytes.size()))),
            "Cannot read complete ice input");
    return bytes;
}
struct Cursor {
    const std::vector<std::uint8_t> &bytes;
    std::size_t position = 0;
    std::uint32_t word() {
        require(position <= bytes.size() && bytes.size() - position >= 4,
                "Truncated ice contour input");
        std::uint32_t result = 0;
        for (unsigned i = 0; i < 4; ++i)
            result |= std::uint32_t(bytes[position++]) << (8 * i);
        return result;
    }
    std::array<std::uint8_t, 32> digest() {
        require(position <= bytes.size() && bytes.size() - position >= 32,
                "Truncated ice contour identity");
        std::array<std::uint8_t, 32> result{};
        std::copy_n(bytes.begin() + position, result.size(), result.begin());
        position += result.size();
        return result;
    }
};
std::string hex(std::span<const std::uint8_t> bytes) {
    static constexpr char digits[] = "0123456789abcdef";
    std::string result;
    result.reserve(2 * bytes.size());
    for (const auto byte : bytes) {
        result.push_back(digits[byte >> 4]);
        result.push_back(digits[byte & 15]);
    }
    return result;
}
std::array<std::uint8_t, 32> digest_bytes(std::string_view text) {
    require(text.size() == 64, "Invalid SHA-256 identity");
    std::array<std::uint8_t, 32> result{};
    for (std::size_t i = 0; i < result.size(); ++i) {
        unsigned value = 0;
        const auto first = text.data() + 2 * i;
        const auto parsed = std::from_chars(first, first + 2, value, 16);
        require(parsed.ec == std::errc{} && parsed.ptr == first + 2,
                "Invalid SHA-256 identity");
        result[i] = static_cast<std::uint8_t>(value);
    }
    return result;
}
struct PostprocessedIce {
    std::array<std::uint8_t, 32> exe{}, xml{}, postprocessed{};
    std::uint32_t seed = 0;
    std::array<float, 4> box{};
    TerrainIceIntermediateRegionGeometry geometry;
    std::vector<std::uint8_t> reconstructed_bytes;
};
PostprocessedIce read_postprocessed(const fs::path &path) {
    const auto bytes = read_bounded(path, 128 * 1024 * 1024);
    require(bytes.size() >= 8 &&
                std::memcmp(bytes.data(), "C5IPST01", 8) == 0,
            "Expected original postprocessed ice contour capture");
    Cursor input{bytes, 8};
    PostprocessedIce result;
    result.exe = input.digest();
    result.xml = input.digest();
    input.digest(); // Upstream geometry identity retained by the capture.
    result.postprocessed = digest_bytes(civ5::runtime::identify_bytes(bytes).sha256);
    const auto initial = input.word();
    const auto faces = input.word();
    const auto points = input.word();
    const auto edges = input.word();
    const auto links = input.word();
    const auto retained = input.word();
    result.seed = input.word();
    require(initial <= faces && retained <= faces && faces <= 1'000'000 &&
                points <= 2'000'000 && edges <= 2'000'000 &&
                links == 2ull * edges &&
                8ull + 96 + 28 + 16 + 16ull * faces + 16ull * points +
                    12ull * edges + 4ull * links == bytes.size(),
            "Invalid postprocessed ice contour extent");
    for (auto &value : result.box)
        value = std::bit_cast<float>(input.word());
    auto &geometry = result.geometry;
    geometry.regions.resize(faces);
    for (auto &region : geometry.regions) {
        region.first_vertex = input.word();
        region.first_index = input.word();
        region.vertex_count = input.word();
        const auto boundary = input.word();
        // Retail retains allocation cursors in these records, including for
        // empty regions. The mesh builders validate spans they actually use.
        require(boundary <= 1 && region.vertex_count <= points,
                "Invalid postprocessed ice region");
        region.boundary = static_cast<std::uint8_t>(boundary);
    }
    geometry.vertices.resize(points);
    for (auto &vertex : geometry.vertices) {
        vertex = {std::bit_cast<float>(input.word()),
                  std::bit_cast<float>(input.word())};
        input.word(); input.word();
    }
    geometry.indices.resize(edges);
    for (auto &edge : geometry.indices) {
        edge = {input.word(), input.word(), input.word()};
        require(edge[0] < points && edge[1] < points && edge[2] < faces,
                "Invalid postprocessed ice edge");
    }
    for (std::uint32_t i = 0; i < links; ++i)
        require(input.word() < edges, "Invalid postprocessed ice incidence");
    require(input.position == bytes.size(), "Trailing ice contour bytes");
    return result;
}
float setting(const civ5::content::XmlNode &root, const char *name) {
    const auto text = root.attribute(name);
    require(!text.empty(), "Missing authored ice setting");
    std::size_t consumed = 0;
    const auto value = std::stof(text, &consumed);
    require(consumed == text.size() && std::isfinite(value),
            "Invalid authored ice setting");
    return value;
}
PostprocessedIce postprocess_geometry_bytes(
    const std::vector<std::uint8_t> &bytes,
    const civ5::content::XmlNode &settings,
    const std::array<std::uint8_t, 32> &settings_digest) {
    require(bytes.size() >= 120 &&
                (std::memcmp(bytes.data(), "C5IRGM01", 8) == 0 ||
                 std::memcmp(bytes.data(), "C5IGEN01", 8) == 0),
            "Expected captured or generated intermediate ice geometry");
    Cursor input{bytes, 8};
    PostprocessedIce result;
    result.exe = input.digest();
    input.digest(); input.digest(); // Upstream source identities.
    result.xml = settings_digest;
    const auto geometry_hash =
        digest_bytes(civ5::runtime::identify_bytes(bytes).sha256);
    const auto edge_pull = input.word();
    const auto region_count = input.word();
    const auto point_count = input.word();
    const auto edge_count = input.word();
    require(edge_pull == std::bit_cast<std::uint32_t>(
                setting(settings, "EdgePullAmount")) &&
                region_count <= 1'000'000 && point_count <= 2'000'000 &&
                edge_count <= 2'000'000,
            "Invalid intermediate ice geometry settings or extent");
    const std::uint64_t records = 120ull + 16ull * region_count +
                                  16ull * point_count + 12ull * edge_count;
    require(records <= bytes.size() && (bytes.size() - records) % 2 == 0,
            "Invalid intermediate ice geometry flag extent");
    const auto link_count = (bytes.size() - records) / 2;
    require(link_count <= 4'000'000 &&
                input.position + 2 * link_count <= bytes.size(),
            "Oversized intermediate ice geometry flags");
    input.position += static_cast<std::size_t>(2 * link_count);
    auto &geometry = result.geometry;
    geometry.regions.resize(region_count);
    for (auto &region : geometry.regions) {
        region.first_vertex = input.word();
        region.first_index = input.word();
        region.vertex_count = input.word();
        const auto boundary = input.word();
        require(boundary <= 1 && region.vertex_count <= point_count,
                "Invalid intermediate ice region");
        region.boundary = static_cast<std::uint8_t>(boundary);
    }
    geometry.vertices.resize(point_count);
    for (auto &point : geometry.vertices) {
        point = {std::bit_cast<float>(input.word()),
                 std::bit_cast<float>(input.word())};
        input.word(); input.word(); // Original adjacency allocation fields.
    }
    geometry.indices.resize(edge_count);
    geometry.incidence.resize(point_count);
    for (std::uint32_t i = 0; i < edge_count; ++i) {
        auto &edge = geometry.indices[i];
        edge = {input.word(), input.word(), input.word()};
        require(edge[0] < point_count && edge[1] < point_count &&
                    edge[2] < region_count,
                "Invalid intermediate ice geometry edge");
        geometry.incidence[edge[0]].push_back(i);
        geometry.incidence[edge[1]].push_back(i);
    }
    require(input.position == bytes.size(), "Trailing intermediate ice geometry bytes");
    result.box = terrain_ice_region_component_box(geometry);
    auto random = initial_terrain_ice_random_state();
    process_terrain_ice_intermediate_regions(
        geometry, random, setting(settings, "BoundaryKillRate"),
        setting(settings, "InteriorKillRate"),
        setting(settings, "SmallPolyShrinkAmount"), 53);
    result.geometry = postprocess_terrain_ice_crossings(
        finalize_terrain_ice_intermediate_regions(geometry));
    result.seed = random.value;
    const auto &finished = result.geometry;
    std::uint64_t incidences = 0;
    for (const auto &list : finished.incidence) incidences += list.size();
    require(finished.regions.size() <= UINT32_MAX &&
                finished.vertices.size() <= UINT32_MAX &&
                finished.indices.size() <= UINT32_MAX &&
                incidences == 2ull * finished.indices.size(),
            "Invalid native postprocessed ice graph extent");
    auto &output = result.reconstructed_bytes;
    output.reserve(static_cast<std::size_t>(8ull + 96 + 28 + 16 +
        16ull * finished.regions.size() + 16ull * finished.vertices.size() +
        12ull * finished.indices.size() + 4ull * incidences));
    output.insert(output.end(), {'C','5','I','P','S','T','0','1'});
    const auto append_word = [&output](std::uint32_t value) {
        for (unsigned byte = 0; byte < 4; ++byte)
            output.push_back(static_cast<std::uint8_t>(value >> (8 * byte)));
    };
    for (const auto &identity : {result.exe, result.xml, geometry_hash})
        output.insert(output.end(), identity.begin(), identity.end());
    append_word(region_count);
    append_word(static_cast<std::uint32_t>(finished.regions.size()));
    append_word(static_cast<std::uint32_t>(finished.vertices.size()));
    append_word(static_cast<std::uint32_t>(finished.indices.size()));
    append_word(static_cast<std::uint32_t>(incidences));
    append_word(static_cast<std::uint32_t>(std::count_if(
        finished.regions.begin(), finished.regions.end(),
        [](const auto &face) { return face.vertex_count != 0; })));
    append_word(result.seed);
    for (const auto bound : result.box)
        append_word(std::bit_cast<std::uint32_t>(bound));
    for (const auto &face : finished.regions) {
        append_word(face.first_vertex);
        append_word(face.first_index);
        append_word(face.vertex_count);
        append_word(face.boundary);
    }
    std::uint32_t link_cursor = 0;
    for (std::size_t i = 0; i < finished.vertices.size(); ++i) {
        const auto &point = finished.vertices[i];
        const auto &links = finished.incidence[i];
        append_word(std::bit_cast<std::uint32_t>(point[0]));
        append_word(std::bit_cast<std::uint32_t>(point[1]));
        append_word(0x60600000u + 4 * link_cursor);
        append_word(static_cast<std::uint32_t>(links.size()));
        link_cursor += static_cast<std::uint32_t>(links.size());
    }
    for (const auto &edge : finished.indices)
        for (const auto value : edge) append_word(value);
    for (const auto &links : finished.incidence)
        for (const auto edge : links) append_word(edge);
    result.postprocessed =
        digest_bytes(civ5::runtime::identify_bytes(output).sha256);
    return result;
}
PostprocessedIce read_geometry_and_postprocess(
    const fs::path &path, const civ5::content::XmlNode &settings,
    const std::array<std::uint8_t, 32> &settings_digest) {
    return postprocess_geometry_bytes(
        read_bounded(path, 128 * 1024 * 1024), settings, settings_digest);
}
PostprocessedIce postprocess_weighted_bytes(
    const std::vector<std::uint8_t> &bytes,
    const civ5::content::XmlNode &settings,
    const std::array<std::uint8_t, 32> &settings_digest) {
    require(bytes.size() >= 128 &&
                std::memcmp(bytes.data(), "C5IWGS01", 8) == 0,
            "Expected weighted ice source graph");
    Cursor input{bytes, 8};
    const auto exe = input.digest();
    require(input.digest() == settings_digest,
            "Authored IceSettings.xml differs from weighted graph capture");
    input.position += 48; // Original source graph and parent identities.
    const auto source_points = input.word();
    const auto source_edges = input.word();
    require(source_points <= 1'000'000 && source_edges <= 2'000'000 &&
                128ull + 24ull * source_points + 36ull * source_edges == bytes.size(),
            "Invalid weighted ice graph extent");
    std::vector<TerrainIceIntermediateSourcePoint> points(source_points);
    for (auto &point : points) {
        point.xy = {std::bit_cast<float>(input.word()),
                    std::bit_cast<float>(input.word())};
        point.active = bytes[input.position];
        input.position += 16;
        require(point.active <= 1 && std::isfinite(point.xy[0]) &&
                    std::isfinite(point.xy[1]),
                "Invalid weighted ice source point");
    }
    std::vector<TerrainIceIntermediateSourceEdge> edges(source_edges);
    for (auto &edge : edges) {
        edge.first = input.word();
        edge.second = input.word();
        edge.boundary = bytes[input.position];
        input.position += 4;
        edge.weights = {std::bit_cast<float>(input.word()),
                        std::bit_cast<float>(input.word())};
        input.position += 16;
        require(edge.first < source_points && edge.second < source_points &&
                    edge.boundary <= 1 && std::isfinite(edge.weights[0]) &&
                    std::isfinite(edge.weights[1]),
                "Invalid weighted ice source edge");
    }
    require(input.position == bytes.size(), "Trailing weighted ice graph bytes");
    auto fill = build_terrain_ice_intermediate_regions(points, edges);
    build_terrain_ice_intermediate_adjacency(fill);
    sort_terrain_ice_intermediate_adjacency(fill);
    mark_terrain_ice_closed_walks(fill);
    const auto walks = trace_terrain_ice_intermediate_regions(fill);
    const float edge_pull = setting(settings, "EdgePullAmount");
    const auto geometry = build_terrain_ice_intermediate_region_geometry(
        fill, walks, edge_pull, 53);
    require(geometry.regions.size() <= 1'000'000 &&
                geometry.vertices.size() <= 2'000'000 &&
                geometry.indices.size() <= 2'000'000 &&
                fill.closed_walk_flags.size() == 2ull * fill.links.size(),
            "Invalid generated intermediate ice geometry extent");

    // This is a canonical native-generated graph, not an original-EXE memory
    // capture. Its own tag and semantic records give the downstream contour
    // an auditable source identity without inventing retail padding bytes.
    std::vector<std::uint8_t> generated;
    generated.reserve(static_cast<std::size_t>(120ull +
        fill.closed_walk_flags.size() + 16ull * geometry.regions.size() +
        16ull * geometry.vertices.size() + 12ull * geometry.indices.size()));
    generated.insert(generated.end(), {'C','5','I','G','E','N','0','1'});
    const auto weighted_hash =
        digest_bytes(civ5::runtime::identify_bytes(bytes).sha256);
    for (const auto &identity : {exe, weighted_hash, settings_digest})
        generated.insert(generated.end(), identity.begin(), identity.end());
    const auto append_word = [&generated](std::uint32_t value) {
        for (unsigned byte = 0; byte < 4; ++byte)
            generated.push_back(static_cast<std::uint8_t>(value >> (8 * byte)));
    };
    append_word(std::bit_cast<std::uint32_t>(edge_pull));
    append_word(static_cast<std::uint32_t>(geometry.regions.size()));
    append_word(static_cast<std::uint32_t>(geometry.vertices.size()));
    append_word(static_cast<std::uint32_t>(geometry.indices.size()));
    generated.insert(generated.end(), fill.closed_walk_flags.begin(),
                     fill.closed_walk_flags.end());
    for (const auto &region : geometry.regions) {
        append_word(region.first_vertex);
        append_word(region.first_index);
        append_word(region.vertex_count);
        append_word(region.boundary);
    }
    for (const auto &point : geometry.vertices) {
        append_word(std::bit_cast<std::uint32_t>(point[0]));
        append_word(std::bit_cast<std::uint32_t>(point[1]));
        append_word(0);
        append_word(0);
    }
    for (const auto &edge : geometry.indices)
        for (const auto value : edge) append_word(value);
    return postprocess_geometry_bytes(generated, settings, settings_digest);
}
PostprocessedIce read_weighted_and_postprocess(
    const fs::path &path, const civ5::content::XmlNode &settings,
    const std::array<std::uint8_t, 32> &settings_digest) {
    return postprocess_weighted_bytes(
        read_bounded(path, 128 * 1024 * 1024), settings, settings_digest);
}
PostprocessedIce read_clipped_and_postprocess(
    const fs::path &path, const civ5::content::XmlNode &settings,
    const std::array<std::uint8_t, 32> &settings_digest,
    std::uint32_t scale_bits, const fs::path *spatial_snapshot_path) {
    const auto clipped = read_bounded(path, 128 * 1024 * 1024);
    require(clipped.size() >= 112 &&
                std::memcmp(clipped.data(), "C5ICLP01", 8) == 0,
            "Expected clipped ice source graph");
    Cursor input{clipped, 8};
    const auto exe = input.digest();
    input.digest(); // Original precursor graph identity.
    const auto spatial = input.digest();
    if (spatial_snapshot_path) {
        const auto snapshot = read_bounded(*spatial_snapshot_path, 8 * 1024 * 1024);
        std::uint32_t heap_used = 0;
        if (snapshot.size() >= 76)
            std::memcpy(&heap_used, snapshot.data() + 72, sizeof(heap_used));
        require(snapshot.size() >= 76 + 0x60 + 68 &&
                    snapshot.size() == 76ull + 0x60 + 68 + heap_used &&
                    std::memcmp(snapshot.data(), "C5ISPA01", 8) == 0 &&
                    std::equal(exe.begin(), exe.end(), snapshot.begin() + 8) &&
                    digest_bytes(civ5::runtime::identify_bytes(snapshot).sha256) == spatial,
                "Clipped ice graph differs from its original spatial snapshot");
    } else {
        require(hex(spatial) ==
                    "2d7d98efa87aa6d58718a2eb2c51044e387687dd11f40e1598061739c7b55d2f",
                "Pinned Gandhi clipped graph spatial identity differs");
    }
    require(scale_bits == 0x41bfff7bu,
            "Selected ice-weight caller scale differs");
    const auto source_points = input.word();
    const auto source_edges = input.word();
    require(source_points <= 1'000'000 && source_edges <= 2'000'000 &&
                112ull + 24ull * source_points + 36ull * source_edges == clipped.size(),
            "Invalid clipped ice source graph extent");
    std::vector<TerrainIceSourceWeightPoint> points(source_points);
    std::vector<TerrainIceSourceWeightEdge> edges(source_edges);
    std::vector<std::array<std::uint32_t, 2>> endpoints(source_edges);
    for (auto &point : points) {
        point.xy = {std::bit_cast<float>(input.word()),
                    std::bit_cast<float>(input.word())};
        point.active = clipped[input.position];
        input.position += 16;
        require(point.active <= 1, "Invalid clipped ice source point");
    }
    for (std::size_t i = 0; i < edges.size(); ++i) {
        auto &edge = edges[i];
        edge.first = input.word();
        edge.second = input.word();
        edge.boundary = clipped[input.position];
        input.position += 28;
        require(edge.boundary <= 1, "Invalid clipped ice source edge");
        endpoints[i] = {edge.first, edge.second};
    }
    require(input.position == clipped.size(), "Trailing clipped ice source bytes");
    const auto adjacency = build_terrain_ice_source_adjacency(source_points, endpoints);
    const TerrainIceSourceWeightSettings weight_settings{
        setting(settings, "CrackCost"), setting(settings, "InitialPotential"),
        setting(settings, "CrackMinClip"), setting(settings, "CrackMaxClip"),
        std::bit_cast<float>(scale_bits)};
    weight_terrain_ice_source_graph(points, edges, adjacency, weight_settings);
    std::vector<std::uint8_t> weighted;
    weighted.reserve(128ull + 24ull * source_points + 36ull * source_edges);
    weighted.insert(weighted.end(), {'C','5','I','W','G','S','0','1'});
    for (const auto &identity : {exe, settings_digest,
            digest_bytes(civ5::runtime::identify_bytes(
                std::span<const std::uint8_t>(clipped).subspan(112)).sha256)})
        weighted.insert(weighted.end(), identity.begin(), identity.end());
    const auto append_word = [&weighted](std::uint32_t value) {
        for (unsigned byte = 0; byte < 4; ++byte)
            weighted.push_back(static_cast<std::uint8_t>(value >> (8 * byte)));
    };
    for (const auto value : {weight_settings.crack_cost,
                             weight_settings.initial_potential,
                             weight_settings.min_clip, weight_settings.max_clip})
        append_word(std::bit_cast<std::uint32_t>(value));
    append_word(source_points);
    append_word(source_edges);
    weighted.insert(weighted.end(), clipped.begin() + 112, clipped.end());
    const auto put_word = [&weighted](std::size_t offset, std::uint32_t value) {
        for (unsigned byte = 0; byte < 4; ++byte)
            weighted[offset + byte] = static_cast<std::uint8_t>(value >> (8 * byte));
    };
    for (std::size_t i = 0; i < points.size(); ++i) {
        const auto offset = 128 + 24 * i;
        weighted[offset + 8] = points[i].active;
        put_word(offset + 12, std::bit_cast<std::uint32_t>(points[i].potential));
    }
    const auto edge_base = 128ull + 24ull * source_points;
    for (std::size_t i = 0; i < edges.size(); ++i) {
        const auto offset = edge_base + 36 * i;
        for (unsigned slot = 0; slot < 2; ++slot) {
            put_word(offset + 12 + 4 * slot,
                     std::bit_cast<std::uint32_t>(edges[i].weights[slot]));
            put_word(offset + 20 + 4 * slot,
                     std::bit_cast<std::uint32_t>(edges[i].propagation[slot]));
            put_word(offset + 28 + 4 * slot,
                     std::bit_cast<std::uint32_t>(edges[i].pending[slot]));
        }
    }
    return postprocess_weighted_bytes(weighted, settings, settings_digest);
}
void write_word(std::ofstream &output, std::uint32_t value,
                unsigned length = 4) {
    for (unsigned i = 0; i < length; ++i)
        output.put(static_cast<char>(value >> (8 * i)));
}
void write_records(const fs::path &path, const PostprocessedIce &source,
                   const TerrainIceCompleteMesh &mesh,
                   const std::vector<TerrainIcePackedRegion> &regions) {
    require(!fs::exists(path), "Ice output already exists");
    std::uint64_t vertices = 0, indices = 0;
    for (const auto &region : regions) {
        vertices += region.vertices.size();
        indices += region.indices.size();
    }
    require(!regions.empty() && regions.size() <= 64 &&
                vertices <= UINT32_MAX && indices <= UINT32_MAX,
            "Invalid packed ice region extent");
    std::ofstream output(path, std::ios::binary);
    require(bool(output), "Cannot create ice record output");
    output.write("C5IRTB02", 8);
    for (const auto &identity : {source.exe, source.xml, source.postprocessed})
        output.write(reinterpret_cast<const char *>(identity.data()),
                     identity.size());
    write_word(output, static_cast<std::uint32_t>(regions.size()));
    write_word(output, static_cast<std::uint32_t>(vertices));
    write_word(output, static_cast<std::uint32_t>(indices));
    write_word(output, source.seed);
    for (const auto value : mesh.component_box)
        write_word(output, std::bit_cast<std::uint32_t>(value));
    std::uint32_t first_vertex = 0, first_index = 0;
    for (const auto &region : regions) {
        write_word(output, first_vertex);
        write_word(output, first_index);
        write_word(output, static_cast<std::uint32_t>(region.vertices.size()));
        write_word(output, static_cast<std::uint32_t>(region.indices.size()));
        for (const auto value : region.bounds)
            write_word(output, std::bit_cast<std::uint32_t>(value));
        first_vertex += static_cast<std::uint32_t>(region.vertices.size());
        first_index += static_cast<std::uint32_t>(region.indices.size());
    }
    for (const auto &region : regions)
        for (const auto &vertex : region.vertices)
            output.write(reinterpret_cast<const char *>(vertex.data()),
                         vertex.size());
    for (const auto &region : regions)
        for (const auto index : region.indices) write_word(output, index, 2);
    output.close();
    require(bool(output), "Cannot finish ice record output");
}
} // namespace

int main(int argc, char **argv) {
    try {
        if (argc != 6 && argc != 8 && argc != 10)
            throw std::runtime_error("Usage: civis_ice_region_builder SOURCE ICE_SETTINGS_XML GAME_ROOT SCALE_BITS_HEX OUTPUT [--postprocessed-output PATH] [--spatial-snapshot PATH]");
        const char *postprocessed_file = nullptr, *spatial_file = nullptr;
        for (int i = 6; i < argc; i += 2) {
            const std::string_view flag(argv[i]);
            if (flag == "--postprocessed-output" && !postprocessed_file)
                postprocessed_file = argv[i + 1];
            else if (flag == "--spatial-snapshot" && !spatial_file)
                spatial_file = argv[i + 1];
            else
                throw std::runtime_error("Duplicate or unknown ice builder option");
        }
        const fs::path settings_path(argv[2]);
        const auto settings = civ5::content::read_xml(settings_path, 1 << 20);
        require(settings.name == "IceSettings", "Invalid ice settings root");
        const auto settings_hash =
            civ5::runtime::identify_file(settings_path).sha256;
        const std::string_view scale_text(argv[4]);
        std::uint32_t scale_bits = 0;
        const auto parsed = std::from_chars(scale_text.data(),
                                            scale_text.data() + scale_text.size(),
                                            scale_bits, 16);
        require(scale_text.size() == 8 && parsed.ec == std::errc{} &&
                    parsed.ptr == scale_text.data() + scale_text.size(),
                "Ice scale must be eight hexadecimal Float32 bits");
        const float scale = std::bit_cast<float>(scale_bits);
        std::ifstream source_input(argv[1], std::ios::binary);
        char source_magic[8]{};
        require(bool(source_input.read(source_magic, 8)),
                "Cannot read ice source identity");
        const auto source_type = std::string_view(source_magic, 8);
        require(!spatial_file || source_type == "C5ICLP01",
                "Spatial snapshot requires a clipped ice source graph");
        const fs::path spatial_path = spatial_file ? fs::path(spatial_file) : fs::path{};
        const auto source = source_type == "C5ICLP01" ?
            read_clipped_and_postprocess(argv[1], settings,
                                         digest_bytes(settings_hash), scale_bits,
                                         spatial_file ? &spatial_path : nullptr) :
            source_type == "C5IWGS01" ?
            read_weighted_and_postprocess(argv[1], settings,
                                          digest_bytes(settings_hash)) :
            source_type == "C5IRGM01" ?
            read_geometry_and_postprocess(argv[1], settings,
                                          digest_bytes(settings_hash)) :
            read_postprocessed(argv[1]);
        require(settings_hash == hex(source.xml),
                "Authored IceSettings.xml differs from contour capture");
        if (postprocessed_file) {
            require(!source.reconstructed_bytes.empty(),
                    "A postprocessed output requires intermediate ice geometry or weighted graph");
            require(!fs::exists(postprocessed_file), "Postprocessed ice output already exists");
            require(fs::path(argv[5]) != fs::path(postprocessed_file),
                    "Ice record and postprocessed outputs must differ");
        }
        const fs::path game_root(argv[3]);
        require(civ5::runtime::identify_file(game_root / "CivilizationV.exe").sha256 ==
                    hex(source.exe),
                "Original CivilizationV.exe differs from contour capture");
        if (postprocessed_file) {
            std::ofstream reconstructed(postprocessed_file, std::ios::binary);
            require(bool(reconstructed), "Cannot create postprocessed ice output");
            reconstructed.write(
                reinterpret_cast<const char *>(source.reconstructed_bytes.data()),
                static_cast<std::streamsize>(source.reconstructed_bytes.size()));
            reconstructed.close();
            require(bool(reconstructed), "Cannot finish postprocessed ice output");
        }
        const civ5::assets::AssetEnvironment assets(game_root);
        const auto archive = assets.open_fpk_archive(
            "Resource/DX9/TerrainTextures.fpk");
        const auto *terrain = std::get_if<std::shared_ptr<civ5::assets::FpkSource>>(
            &archive);
        require(terrain && *terrain, "Original terrain texture archive unavailable");
        const auto candidate = (*terrain)->find(
            civ5::assets::AssetKey("IceDisplacement.dds"));
        const auto *handle = std::get_if<civ5::assets::AssetHandle>(&candidate);
        require(handle, "Original IceDisplacement.dds unavailable");
        const auto displacement = civ5::assets::DdsData::parse(handle->read_bytes());
        require(displacement.width() == 256 && displacement.height() == 256 &&
                    displacement.format() == civ5::assets::DdsFormat::luminance &&
                    displacement.bits_per_pixel() == 8 &&
                    displacement.levels()[0].row_pitch == 256,
                "Unsupported original ice displacement layout");
        TerrainIceSmallMeshSettings small;
        small.scale = scale;
        small.height_min = setting(settings, "SmallPolyHeightMin");
        small.height_max = setting(settings, "SmallPolyHeightMax");
        small.depth_min = setting(settings, "SmallPolyDepthMin");
        small.depth_max = setting(settings, "SmallPolyDepthMax");
        small.skirt_slope = setting(settings, "SkirtSlopeSmall");
        TerrainIceLargeTopSettings large;
        large.scale = scale;
        large.displacement_frequency = setting(settings, "DisplaceFrequency");
        large.displacement_min = setting(settings, "DisplaceMin");
        large.displacement_max = setting(settings, "DisplaceMax");
        large.height = setting(settings, "LargePolyHeight");
        large.height_ceiling = large.displacement_max + large.height;
        const float large_depth = std::max({setting(settings, "LargePolyDepth"),
                                            small.depth_min, small.depth_max});
        const auto &dds_bytes = displacement.original_bytes();
        const auto pixels = std::span<const std::uint8_t>(dds_bytes).subspan(128);
        auto mesh = build_terrain_ice_complete_mesh(
            source.geometry, source.box, small, large, pixels,
            displacement.width(), displacement.height(), 256,
            setting(settings, "SkirtSlopeLarge"), large_depth, source.seed);
        auto regions = build_terrain_ice_gpu_regions(
            mesh.component_box, mesh.combined.vertices, mesh.combined.indices,
            large.height_ceiling, large_depth);
        write_records(argv[5], source, mesh, regions);
        std::cout << "ICE_GPU regions=" << regions.size()
                  << " source_vertices=" << mesh.combined.vertices.size()
                  << " source_indices=" << mesh.combined.indices.size()
                  << " output=" << argv[5] << '\n';
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "ICE_GPU_ERROR " << error.what() << '\n';
        return 1;
    }
}
