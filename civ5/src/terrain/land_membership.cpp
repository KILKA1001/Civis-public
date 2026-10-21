#include "civ5/terrain/land_membership.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace civ5::terrain {
namespace {
void require(bool condition, const char *message) {
    if (!condition)
        throw std::invalid_argument(message);
}
bool segments_intersect(Point2 a, Point2 b, Point2 c, Point2 d) {
    const float dx = d.x - c.x, ax = b.x - a.x;
    const float determinant = (d.y - c.y) * ax - (b.y - a.y) * dx;
    require(std::isfinite(determinant), "Exceptional SSE geometry outside recovered range");
    if (std::abs(determinant) < 0.0001f)
        return false;
    const float reciprocal = 1.0f / determinant, offset = a.x - c.x;
    const float first = ((a.y - c.y) * ax - (b.y - a.y) * offset) * reciprocal;
    require(std::isfinite(first), "Exceptional SSE intersection parameter");
    if (first < 0 || first > 1)
        return false;
    const float second = ((a.y - c.y) * dx - (d.y - c.y) * offset) * reciprocal;
    require(std::isfinite(second), "Exceptional SSE intersection parameter");
    return second >= 0 && second <= 1;
}
std::int32_t candidate(float value, int count, float extent, X87Precision pc) {
    const float scaled =
        static_cast<float>(round_x87(round_x87(static_cast<double>(count) / extent, pc) * value, pc));
    require(std::isfinite(scaled) && static_cast<double>(scaled) >= -2147483648.0 &&
                static_cast<double>(scaled) < 2147483648.0,
            "Unsupported candidate CVTT input");
    return static_cast<std::int32_t>(scaled);
}
} // namespace
std::int32_t unsigned_wrapped_candidate(std::int32_t value, std::int32_t count) {
    require(count > 0, "Candidate period must be positive");
    return static_cast<std::int32_t>(
        (std::bit_cast<std::uint32_t>(value) + static_cast<std::uint32_t>(count)) %
        static_cast<std::uint32_t>(count));
}
bool edge_intersects(const std::array<float, 6> &bounds, Point2 a, Point2 b) {
    require(std::all_of(bounds.begin(), bounds.end(), [](float x) { return std::isfinite(x); }) &&
                bounds[0] <= bounds[3] && bounds[1] <= bounds[4],
            "Invalid edge bounds");
    require(std::isfinite(a.x) && std::isfinite(a.y) && std::isfinite(b.x) && std::isfinite(b.y),
            "Non-finite edge point");
    auto inside = [&](Point2 p) {
        return p.x >= bounds[0] && p.x <= bounds[3] && p.y >= bounds[1] && p.y <= bounds[4];
    };
    if (inside(a) || inside(b))
        return true;
    const std::array<Point2, 4> corners{Point2{bounds[0], bounds[1]}, Point2{bounds[3], bounds[1]},
                                        Point2{bounds[3], bounds[4]}, Point2{bounds[0], bounds[4]}};
    for (int i = 0; i < 4; ++i)
        if (segments_intersect(a, b, corners[i], corners[(i + 1) % 4]))
            return true;
    return false;
}
LandMembership build_land_membership(const LandLayout &layout, std::int32_t map_width,
                                     std::int32_t map_height, const IntMatrix4 &map, const Matrix4 &world,
                                     Point2 owner, X87Precision pc, MatrixPath path) {
    require(map_width >= 0 && map_width <= 255 && map_height >= 0 && map_height <= 255,
            "Unsupported renderer traversal extent");
    require(std::all_of(world.begin(), world.end(), [](float x) { return std::isfinite(x); }),
            "Non-finite world matrix");
    std::vector<std::array<float, 6>> bounds;
    for (int i = 0; i < layout.columns() * layout.rows(); ++i)
        bounds.push_back(layout.patch(i / layout.rows(), i % layout.rows()).culling_bounds());
    LandMembership result{{}, std::vector<std::vector<LogicalHex>>(bounds.size())};
    auto scale = identity_matrix();
    scale[5] = .866f;
    const auto scaling = matrix_multiply(identity_matrix(), scale, pc, path);
    auto transform = [&](float x, float y) {
        auto translation = identity_matrix();
        translation[12] = x;
        translation[13] = y;
        translation[14] = -x - y;
        const auto transformed =
            matrix_multiply(matrix_multiply(translation, world, pc, path), scaling, pc, path);
        return Point2{transformed[12], transformed[13]};
    };
    constexpr std::array<std::array<float, 2>, 6> offsets{{{2.0f / 3, -1.0f / 3},
                                                           {1.0f / 3, 1.0f / 3},
                                                           {-1.0f / 3, 2.0f / 3},
                                                           {-2.0f / 3, 1.0f / 3},
                                                           {-1.0f / 3, -1.0f / 3},
                                                           {1.0f / 3, -2.0f / 3}}};
    for (int x = 0; x < map_width; ++x)
        for (int y = 0; y < map_height; ++y) {
            const int a = x - (y >> 1), c = -(a + y);
            auto component = [&](int lane) {
                return wrap_add(
                    wrap_add(wrap_add(wrap_mul(y, map[4 + lane]), wrap_mul(a, map[lane])), map[12 + lane]),
                    wrap_mul(c, map[8 + lane]));
            };
            const int lx = component(0), ly = component(1);
            const LogicalHex logical{lx, ly, wrap_neg(wrap_add(lx, ly))};
            const auto center = transform(static_cast<float>(lx), static_cast<float>(ly));
            auto corner = [&](int i) {
                const auto p =
                    transform(static_cast<float>(lx) + offsets[i][0], static_cast<float>(ly) + offsets[i][1]);
                return Point2{p.x - owner.x, p.y - owner.y};
            };
            const std::array<Point2, 6> corners{corner(0), corner(1), corner(2),
                                                corner(3), corner(4), corner(5)};
            const int ix =
                candidate(center.x, layout.columns(), static_cast<float>(layout.domain().width()), pc);
            const int iy =
                candidate(center.y, layout.rows(), static_cast<float>(layout.domain().height()) * .866f, pc);
            require(ix > std::numeric_limits<std::int32_t>::min() &&
                        ix < std::numeric_limits<std::int32_t>::max() &&
                        iy > std::numeric_limits<std::int32_t>::min() &&
                        iy < std::numeric_limits<std::int32_t>::max(),
                    "Candidate loop overflow");
            std::array<LandCandidate, 9> candidates;
            // Offsets avoid an overflow in a C++ inclusive loop's final increment.
            int at = 0;
            for (int dx = -1; dx <= 1; ++dx)
                for (int dy = -1; dy <= 1; ++dy) {
                    const int px = unsigned_wrapped_candidate(ix + dx, layout.columns());
                    const int py = unsigned_wrapped_candidate(iy + dy, layout.rows());
                    const int index = px * layout.rows() + py;
                    bool intersects = false;
                    for (int edge = 0; edge < 6; ++edge)
                        if (edge_intersects(bounds[index], corners[edge], corners[(edge + 1) % 6]))
                            intersects = true;
                    candidates[at++] = {index, intersects};
                    if (intersects) {
                        require(result.patches[index].size() < 16,
                                "Original fixed 16-hex LAND membership capacity exceeded");
                        result.patches[index].push_back(logical);
                    }
                }
            result.hexes.push_back({x, y, logical, center, corners, ix, iy, candidates});
        }
    return result;
}
} // namespace civ5::terrain
