#include "civ5/terrain/coast_curves.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace civ5::terrain {
namespace {
void require(bool condition, const char *message) {
    if (!condition)
        throw std::invalid_argument(message);
}
void validate_precision(X87Precision p) {
    require(p == X87Precision::single || p == X87Precision::double_precision,
            "Unsupported coast curve precision");
}
void validate(CoastControlPoint p) {
    require(std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.transition),
            "Nonfinite coast control point");
}
void validate(const CoastProfileRecord &r) {
    for (auto n : r.nodes)
        require(std::isfinite(n.offset) && std::isfinite(n.opacity) && std::isfinite(n.height),
                "Nonfinite coast style node");
}
std::int32_t sentinel(std::int32_t v) { return v == 65535 ? -1 : v; }
std::vector<CoastControlPoint> subdivide(std::span<const CoastControlPoint> points) {
    std::vector<CoastControlPoint> result;
    result.reserve(points.size() * 2 - 2);
    for (std::size_t i = 0; i + 1 < points.size(); ++i) {
        const auto a = points[i], b = points[i + 1];
        for (auto weight : {.25f, .75f}) {
            const auto other = 1.f - weight;
            result.push_back({b.x * weight + a.x * other, b.y * weight + a.y * other,
                              b.transition * weight + a.transition * other, b.style_a, b.style_b});
        }
    }
    return result;
}
} // namespace
std::int32_t coast_style_key(std::string_view name) {
    constexpr std::array<std::string_view, 13> names{
        "Default", "Grass",      "GrassHill", "Plains",     "PlainsHill", "Snow",           "SnowHill",
        "Desert",  "DesertHill", "Tundra",    "TundraHill", "Mountain",   "DefaultShallows"};
    const auto found = std::find(names.begin(), names.end(), name);
    return found == names.end() ? -1 : static_cast<std::int32_t>(found - names.begin());
}
CoastProfileRecord compile_coast_profile(const CoastStyleSource &s) {
    const CoastProfileRecord result{coast_style_key(s.type),
                                    {{{-s.smoothing_high, 0.f, s.high.height, -1},
                                      {-s.high.offset, 1.f, s.high.height, sentinel(s.high.character)},
                                      {s.water.offset, 1.f, s.water.height, sentinel(s.water.character)},
                                      {s.low.offset, 1.f, s.low.height, sentinel(s.low.character)},
                                      {s.smoothing_low, 0.f, s.low.height, -1}}},
                                    {-1, sentinel(s.high.terrain_type), sentinel(s.low.terrain_type), -1}};
    validate(result);
    return result;
}
CoastProfiles::CoastProfiles(std::span<const CoastProfileRecord> records) {
    require(records.size() <= 4096, "Coast style table capacity exceeded");
    for (const auto &r : records)
        validate(r);
    records_.assign(records.begin(), records.end());
}
CoastProfileRecord CoastProfiles::resolve(std::int32_t key, std::span<const std::uint32_t> palette,
                                          std::uint32_t family, std::uint32_t common) const {
    require(palette.size() <= 65536, "Coast palette capacity exceeded");
    CoastProfileRecord result{}; // Missing key: actual zero record, including character0.
    for (const auto &r : records_)
        if (r.key == key)
            result = r; // LAST duplicate wins.
    const auto index = [&](std::uint32_t value) {
        const auto p = std::find(palette.begin(), palette.end(), value);
        return p == palette.end() ? 255 : static_cast<int>((p - palette.begin()) & 255);
    };
    for (auto &material : result.materials)
        if (material != -1) {
            const auto value = std::bit_cast<std::uint32_t>(material);
            const auto selected = index(family | value);
            material = selected == 255 ? index(common | value) : selected;
        }
    return result;
}

std::vector<CoastCharacterPoint> compile_coast_character(std::span<const CoastCharacterSourcePoint> source,
                                                         X87Precision pc) {
    validate_precision(pc);
    require(source.size() >= 98, "Coast Character needs 98 source records");
    struct XY {
        float x, y;
    };
    std::vector<XY> original;
    for (auto s : source.first(98)) {
        require(std::isfinite(s.x) && std::isfinite(s.y) && std::isfinite(s.z), "Nonfinite Character source");
        original.push_back({s.x, s.y});
    }
    const auto reduce = [](const auto &p) {
        std::vector<XY> result;
        result.push_back({p[0].x * 1.5f + p[1].x * -.5f, p[0].y * 1.5f + p[1].y * -.5f});
        for (std::size_t i = 0; i + 3 < p.size(); i += 2) {
            const auto a = p[i], b = p[i + 1], c = p[i + 2], d = p[i + 3];
            result.push_back({((a.x * -.25f + b.x * .75f) + c.x * .75f) + d.x * -.25f,
                              ((a.y * -.25f + b.y * .75f) + c.y * .75f) + d.y * -.25f});
        }
        const auto a = p[p.size() - 2], b = p.back();
        result.push_back({a.x * -.5f + b.x * 1.5f, a.y * -.5f + b.y * 1.5f});
        return result;
    };
    const auto expand = [](const auto &p) {
        std::vector<XY> result;
        for (std::size_t i = 0; i + 1 < p.size(); ++i) {
            const auto a = p[i], b = p[i + 1];
            result.push_back({b.x * .25f + a.x * .75f, b.y * .25f + a.y * .75f});
            result.push_back({b.x * .75f + a.x * .25f, b.y * .75f + a.y * .25f});
        }
        return result;
    };
    const auto baseline = expand(expand(reduce(reduce(original))));
    const auto dx = baseline[1].x - baseline[0].x;
    std::vector<XY> padded{{baseline[0].x - dx, baseline.back().y}};
    padded.insert(padded.end(), baseline.begin(), baseline.end());
    padded.push_back({baseline.back().x + dx, baseline.back().y});
    std::vector<CoastCharacterPoint> result;
    for (std::size_t i = 0; i < 98; ++i) {
        const auto a = padded[i], b = padded[i + 1], c = padded[i + 2];
        const float tx0 = (a.x * -.5f + 0.f * b.x) + .5f * c.x;
        const float ty0 = (b.y * 0.f + a.y * -.5f) + c.y * .5f;
        const auto xx = round_x87(double(tx0) * tx0, pc), yy = round_x87(double(ty0) * ty0, pc);
        const auto length = static_cast<float>(round_x87(std::sqrt(round_x87(xx + yy, pc)), pc));
        require(length > 0.f && std::isfinite(length), "Degenerate coast Character tangent");
        // This producer spills LENGTH, unlike the mixed reciprocal spill in curves.
        const auto inverse = 1.f / length, tx = inverse * tx0, ty = inverse * ty0;
        const auto rx = -original[i].x + baseline[i].x, ry = -original[i].y + baseline[i].y;
        const CoastCharacterPoint p{rx * tx + ry * ty, -ty * rx + ry * tx};
        require(std::isfinite(p.tangent) && std::isfinite(p.normal), "Nonfinite compiled Character");
        result.push_back(p);
    }
    return result;
}

CoastCurveSet::CoastCurveSet(CoastCurves curves, std::uint32_t owner_flag)
    : curves_(std::move(curves)), owner_flag_(owner_flag) {
    const auto n = curves_[0].size();
    require(n >= 3 && n <= max_coast_curve_controls, "Unsupported coast curve size");
    for (const auto &curve : curves_) {
        require(curve.size() == n, "Coast curves must have corresponding controls");
        for (auto p : curve)
            validate(p);
    }
}
CoastControlPoint sample_coast_curve(std::span<const CoastControlPoint> p, float argument, bool derivative) {
    require(p.size() >= 3 && p.size() <= max_coast_curve_controls && std::isfinite(argument) && argument >= 0.f &&
                argument < static_cast<float>(p.size()) - 1.f,
            "Unsupported coast sampling argument");
    auto index = static_cast<std::size_t>(argument);
    float t = argument - static_cast<float>(std::floor(double(argument)));
    if (index >= p.size() - 2) {
        --index;
        t = 1.f;
    }
    const auto a = p[index], b = p[index + 1], c = p[index + 2];
    validate(a);
    validate(b);
    validate(c);
    const auto square = t * t;
    const auto w0 = derivative ? 1.f * t + -1.f : (.5f * square + -1.f * t) + .5f;
    const auto w1 = derivative ? -2.f * t + 1.f : (-1.f * square + 1.f * t) + .5f;
    const auto w2 = derivative ? 1.f * t + 0.f : (.5f * square + 0.f * t) + 0.f;
    const CoastControlPoint result{(b.x * w1 + a.x * w0) + c.x * w2, (b.y * w1 + a.y * w0) + c.y * w2,
                                   (b.transition * w1 + a.transition * w0) + c.transition * w2, a.style_a,
                                   a.style_b};
    validate(result);
    return result;
}

CoastCurveSet build_coast_curves(std::span<const CoastControlPoint> points, const CoastProfiles &profiles,
                                 const CoastCharacterTables &characters, const CoastCurveState &s) {
    validate_precision(s.precision);
    require(points.size() >= 4 && points.size() <= max_coast_contour_controls, "Unsupported base coast contour size");
    require(std::isfinite(s.wrap_offset.x) && std::isfinite(s.wrap_offset.y), "Nonfinite coast wrap offset");
    require(s.palette.size() <= 65536 && characters.size() <= 4096, "Coast source table capacity exceeded");
    std::uint64_t character_points = 0;
    for (const auto &table : characters) {
        character_points += table.size();
        require(character_points <= 1048576, "Coast Character point capacity exceeded");
        for (auto p : table)
            require(std::isfinite(p.tangent) && std::isfinite(p.normal), "Nonfinite Character table");
    }
    for (auto p : points)
        validate(p);
    CoastCurves curves;
    for (auto &curve : curves)
        curve.resize(points.size(), CoastControlPoint{});
    struct Cache {
        int previous = -1;
        bool reverse = true;
        CoastProfileRecord first{}, second{};
    };
    const auto update = [&](Cache &cache, CoastControlPoint p) {
        if (p.style_a == cache.previous)
            return;
        cache.first = profiles.resolve(p.style_a, s.palette, s.family_prefix, s.common_prefix);
        cache.second = profiles.resolve(p.style_b, s.palette, s.family_prefix, s.common_prefix);
        cache.previous = p.style_a;
        cache.reverse = !cache.reverse;
    };
    const auto normalized = [&](float x, float y, bool sse_length) {
        const auto xx = round_x87(double(x) * x, s.precision), yy = round_x87(double(y) * y, s.precision);
        const auto squared = sse_length ? double(x * x + y * y) : round_x87(yy + xx, s.precision);
        require(squared > 0.0 && std::isfinite(squared), "Degenerate coast curve tangent");
        const auto inverse = round_x87(1.0 / round_x87(std::sqrt(squared), s.precision), s.precision);
        return Point2(static_cast<float>(round_x87(inverse * x, s.precision)),
                      static_cast<float>(inverse) * y);
    };
    const auto wrap = [&](auto &curve) {
        const auto before_last = curve[curve.size() - 2], second = curve[1];
        // XYZ only; original endpoint metadata is not repaired.
        curve[0].x = before_last.x - s.wrap_offset.x;
        curve[0].y = before_last.y - s.wrap_offset.y;
        curve[0].transition = before_last.transition;
        curve.back().x = second.x + s.wrap_offset.x;
        curve.back().y = second.y + s.wrap_offset.y;
        curve.back().transition = second.transition;
    };
    Cache offset_cache;
    for (std::size_t i = s.closed ? 1 : 0; i < points.size(); ++i) {
        const auto argument = s.closed                 ? static_cast<float>(i) - .5f
                              : i == 0                 ? 0.f
                              : i == points.size() - 1 ? static_cast<float>(points.size() - 2) - .01f
                                                       : static_cast<float>(i) - .5f;
        const auto derivative = sample_coast_curve(points, argument, true),
                   sampled = sample_coast_curve(points, argument);
        const auto normal = normalized(derivative.y, -derivative.x, true);
        update(offset_cache, sampled);
        const auto amount = offset_cache.reverse ? 1.f - sampled.transition : sampled.transition,
                   complement = 1.f - amount;
        for (std::size_t band = 0; band < 5; ++band) {
            const auto offset = offset_cache.first.nodes[band].offset * amount +
                                offset_cache.second.nodes[band].offset * complement;
            auto p = points[i];
            p.x = offset * normal.x + p.x;
            p.y = offset * normal.y + p.y;
            curves[band][i] = p;
        }
    }
    for (auto &curve : curves) {
        if (s.closed)
            wrap(curve);
        curve = subdivide(subdivide(curve));
    }
    Cache character_cache; // Retained across all FIVE curves, not reset per band.
    for (std::size_t band = 0; band < 5; ++band) {
        auto &curve = curves[band];
        for (std::size_t i = 0; i + 1 < curve.size(); ++i) {
            const auto p = curve[i];
            // Read the mutable curve: the last backed-up derivative observes an
            // earlier displaced point. Do not precompute tangents or positions.
            const auto derivative = sample_coast_curve(curve, static_cast<float>(i) + .5f, true);
            const auto tangent = normalized(derivative.x, derivative.y, false);
            update(character_cache, p);
            if (i == 0)
                continue;
            const auto displacement = [&](std::int32_t character) {
                if (character == -1)
                    return Point2(0.f, 0.f);
                require(character >= 0 && std::size_t(character) < characters.size() &&
                            !characters[character].empty(),
                        "Missing compiled coast Character table");
                const auto &table = characters[character];
                const auto value = table[i % table.size()];
                return Point2(value.tangent * tangent.x + value.normal * -tangent.y,
                              value.normal * tangent.x + tangent.y * value.tangent);
            };
            const auto a = displacement(character_cache.first.nodes[band].character);
            const auto b = displacement(character_cache.second.nodes[band].character);
            const auto amount = character_cache.reverse ? 1.f - p.transition : p.transition,
                       complement = 1.f - amount;
            curve[i].x = (a.x * amount + complement * b.x) + p.x;
            curve[i].y = p.y + (a.y * amount + complement * b.y);
        }
        if (s.closed)
            wrap(curve);
    }
    for (auto &curve : curves)
        curve = subdivide(curve);
    return CoastCurveSet(std::move(curves), s.owner_flag);
}
} // namespace civ5::terrain
