#include "civ5/terrain/coast_strip.hpp"
#include "civ5/terrain/terrain_occurrences.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace civ5::terrain {
namespace {
void require(bool condition, const char *message) {
    if (!condition)
        throw std::invalid_argument(message);
}
} // namespace
std::array<float, 3> CoastStrip::coefficients(std::size_t triangle) {
    constexpr std::array<std::array<float, 3>, 8> values{
        {{0, 0, 0}, {1, 0, 0}, {1, 1, 1}, {1, 1, 1}, {1, 1, 1}, {1, 1, 1}, {0, 0, 1}, {0, 0, 0}}};
    return values[triangle % 8];
}
CoastRasterJob CoastStrip::raster_job(std::size_t index, std::array<std::uint8_t, 4> from,
                                      std::array<std::uint8_t, 4> to) const {
    require(index < chunks_.size(), "Coast strip chunk outside range");
    const auto &chunk = chunks_[index];
    std::vector<CoastTriangle> triangles;
    triangles.reserve(chunk.triangle_count);
    for (std::size_t i = 0; i < chunk.triangle_count; ++i) {
        const auto first = (chunk.first_triangle + i) * 3;
        // Native dispatcher forwards coefficient base zero. Each chunk starts
        // on an eight-triangle boundary, so the periodic triples coincide.
        const auto edge = coefficients(i);
        triangles.push_back({vertices_[chunk.first_vertex + indices_[first]],
                             vertices_[chunk.first_vertex + indices_[first + 1]],
                             vertices_[chunk.first_vertex + indices_[first + 2]], edge[0], edge[1], edge[2]});
    }
    return CoastRasterJob(triangles, from, to);
}
CoastStrip build_coast_strip(const CoastCurveSet &set, const CoastProfiles &profiles,
                             std::span<const std::uint32_t> palette, std::uint32_t family,
                             std::uint32_t common) {
    const auto count = static_cast<std::uint32_t>(set.control_count());
    require(count >= 3 && count <= max_coast_curve_controls, "Coast strip exceeds curve control capacity");
    const auto &curves = set.curves();
    CoastStrip result;
    result.vertices_.reserve((count - 1) * 8);
    int previous = -1;
    bool reverse = true;
    CoastProfileRecord first{}, second{};
    for (std::uint32_t sample = 0; sample < count - 1; ++sample) {
        for (unsigned band = 0; band < 4; ++band) {
            const auto a = sample_coast_curve(curves[band], static_cast<float>(sample));
            if (a.style_a != previous) {
                first = profiles.resolve(a.style_a, palette, family, common);
                second = profiles.resolve(a.style_b, palette, family, common);
                previous = a.style_a;
                reverse = !reverse;
            }
            const float amount = reverse ? 1.f - a.transition : a.transition;
            const float complement = 1.f - amount;
            const auto material =
                static_cast<std::uint8_t>(amount <= .5f ? second.materials[band] : first.materials[band]);
            const auto vertex = [&](CoastControlPoint p, unsigned node) {
                const float height =
                    second.nodes[node].height * complement + first.nodes[node].height * amount;
                const float opacity =
                    second.nodes[node].opacity * complement + first.nodes[node].opacity * amount;
                require(std::isfinite(height) && std::isfinite(opacity), "Nonfinite coast strip blend");
                result.vertices_.push_back({p.x, p.y, material, height, opacity});
            };
            vertex(a, band);
            vertex(sample_coast_curve(curves[band + 1], static_cast<float>(sample)), band + 1);
        }
    }
    const auto total = (count - 2) * 8;
    std::uint32_t limit = 64;
    for (;;) {
        const auto chunks = (total - 1) / limit + 1, remainder = total % limit;
        if (!(chunks > 1 && chunks % 2 != 0) && !(remainder >= 1 && remainder <= 7))
            break;
        limit += 8;
    }
    result.indices_.reserve(total * 3);
    for (std::uint32_t first_connection = 0; first_connection < count - 2;) {
        const auto connections = std::min(limit / 8, count - 2 - first_connection);
        require((connections + 1) * 8 <= 65536, "Coast chunk exceeds local U16 range");
        result.chunks_.push_back(
            {first_connection * 8, (connections + 1) * 8, first_connection * 8, connections * 8});
        for (std::uint32_t row = 0; row < connections; ++row)
            for (unsigned band = 0; band < 4; ++band) {
                const auto s = row * 8 + band * 2;
                for (auto index : {s, s + 1, s + 8, s + 1, s + 9, s + 8})
                    result.indices_.push_back(static_cast<std::uint16_t>(index));
            }
        first_connection += connections;
    }
    return result;
}
CoastDispatchPlan build_coast_dispatch(std::span<const CoastStrip> strips, std::uint16_t gate,
                                       std::span<const std::uint32_t> palette,
                                       std::span<const std::uint32_t> randoms, std::uint32_t helper) {
    CoastDispatchPlan result{gate, {}};
    std::uint64_t jobs = 0;
    for (const auto &strip : strips)
        jobs += strip.chunks().size();
    require(jobs <= (0x7fffffffULL - 4) / 76, "Coast dispatch allocation count exceeded");
    if (strips.empty())
        return result;
    require(randoms.size() >= 2 && randoms[0] <= 32767 && randoms[1] <= 32767,
            "Coast dispatch requires two explicit CRT random values");
    const auto selected_strip = randoms[0] % strips.size();
    const auto half = strips[selected_strip].chunks().size() / 2;
    require(half != 0, "Selected native coast strip has no chunk pair");
    const auto selected_chunk = (randoms[1] % half) * 2;
    result.direct_random_calls = 2;
    for (unsigned family = 0; family < 4; ++family) {
        const auto prefix = (0x11u + family) << 16;
        const auto lookup = [&](std::uint32_t kind) -> std::uint8_t {
            const auto found = std::find(palette.begin(), palette.end(), prefix | kind);
            return found == palette.end() ? 255 : static_cast<std::uint8_t>(found - palette.begin());
        };
        result.remap_from[family] = lookup(0x35);
        result.remap_to[family] = lookup(0x10);
        if (result.remap_to[family] == 255)
            result.remap_from[family] = 255;
    }
    for (unsigned phase = 0; phase < 2; ++phase)
        for (std::size_t strip = 0; strip < strips.size(); ++strip)
            for (std::size_t chunk = phase; chunk < strips[strip].chunks().size(); chunk += 2)
                result.phases[phase].push_back(
                    {strip, chunk, strip == selected_strip && chunk == selected_chunk ? helper - 255u : 0u});
    return result;
}
CoastDispatchPlan build_coast_dispatch(std::span<const CoastStrip> strips, std::uint16_t gate,
                                       std::span<const std::uint32_t> palette, TerrainRandom &random,
                                       std::uint32_t helper) {
    // Validate using a local stream so rejected input does not consume caller
    // state. Empty strips deliberately draw nothing.
    auto next = random;
    std::array<std::uint32_t, 2> values{};
    if (!strips.empty()) {
        values[0] = next.next();
        values[1] = next.next();
    }
    auto result = build_coast_dispatch(strips, gate, palette, values, helper);
    random = next;
    return result;
}
} // namespace civ5::terrain
