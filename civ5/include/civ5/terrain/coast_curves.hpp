#pragma once
#include "coast_limits.hpp"
#include "native_math.hpp"
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace civ5::terrain {
struct CoastControlPoint {
    float x, y, transition;
    std::uint8_t style_a, style_b;
};
struct CoastProfileNode {
    float offset, opacity, height;
    std::int32_t character;
};
struct CoastProfileRecord {
    std::int32_t key;
    std::array<CoastProfileNode, 5> nodes;
    std::array<std::int32_t, 4> materials;
};
// Parsed authored fields, BEFORE native profile construction. Text/XML parsing
// and source selection are separate; no locale or malformed-native-file emulation.
struct CoastBeachSource {
    float height, offset;
    std::int32_t character, terrain_type;
};
struct CoastWaterSource {
    float height, offset;
    std::int32_t character;
};
struct CoastStyleSource {
    std::string type;
    float smoothing_high, smoothing_low;
    CoastBeachSource high, low;
    CoastWaterSource water;
};
std::int32_t coast_style_key(std::string_view name);
CoastProfileRecord compile_coast_profile(const CoastStyleSource &source);
class CoastProfiles {
  public:
    explicit CoastProfiles(std::span<const CoastProfileRecord> records);
    std::span<const CoastProfileRecord> records() const { return records_; }
    CoastProfileRecord resolve(std::int32_t key, std::span<const std::uint32_t> palette,
                               std::uint32_t family_prefix, std::uint32_t common_prefix) const;

  private:
    std::vector<CoastProfileRecord> records_;
};
struct CoastCharacterSourcePoint {
    float x, y, z;
};
struct CoastCharacterPoint {
    float tangent, normal;
};
using CoastCharacterTables = std::vector<std::vector<CoastCharacterPoint>>;
// Consumes exactly the first 98 parsed authored triples; Z is validated but not
// used, and later records are ignored. PC24/53 is explicit, not host x87 state.
std::vector<CoastCharacterPoint> compile_coast_character(std::span<const CoastCharacterSourcePoint> source,
                                                         X87Precision precision);

using CoastCurves = std::array<std::vector<CoastControlPoint>, 5>;
class CoastCurveSet {
  public:
    CoastCurveSet(CoastCurves curves, std::uint32_t owner_flag);
    const CoastCurves &curves() const { return curves_; }
    std::size_t control_count() const { return curves_[0].size(); }
    std::uint32_t owner_flag() const { return owner_flag_; }

  private:
    CoastCurves curves_;
    std::uint32_t owner_flag_;
};
// Literal B+A+C quadratic/derivative helper. Metadata comes from the first
// selected control; the terminal window backs up and forces t=1.
CoastControlPoint sample_coast_curve(std::span<const CoastControlPoint> points, float argument,
                                     bool derivative = false);
struct CoastCurveState {
    bool closed;
    Point2 wrap_offset;
    std::uint32_t owner_flag;
    std::span<const std::uint32_t> palette;
    std::uint32_t family_prefix, common_prefix;
    X87Precision precision;
};
// The input base contour is AFTER mandatory random perturbation. This stage
// neither skips perturbation nor invents its PRNG entry/cursor. Owns all output.
CoastCurveSet build_coast_curves(std::span<const CoastControlPoint> points, const CoastProfiles &profiles,
                                 const CoastCharacterTables &characters, const CoastCurveState &state);
} // namespace civ5::terrain
