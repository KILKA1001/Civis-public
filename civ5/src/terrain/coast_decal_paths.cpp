#include "civ5/terrain/coast_decal_paths.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace civ5::terrain {
namespace {
void require(bool ok, const char *message) {
    if (!ok) throw std::invalid_argument(message);
}
std::vector<Point2> subdivide(std::span<const Point2> points) {
    std::vector<Point2> result;
    result.reserve(points.size()*2-2);
    for (std::size_t i = 0; i+1 < points.size(); ++i) {
        const auto a = points[i], b = points[i+1];
        result.emplace_back(b.x*.25f+a.x*.75f, b.y*.25f+a.y*.75f);
        result.emplace_back(b.x*.75f+a.x*.25f, b.y*.75f+a.y*.25f);
    }
    return result;
}
Point2 sample(std::span<const Point2> points, float argument, bool derivative) {
    auto index = static_cast<std::size_t>(argument);
    float t = argument-static_cast<float>(std::floor(double(argument)));
    if (index >= points.size()-2) { --index; t = 1; }
    const auto a = points[index], b = points[index+1], c = points[index+2];
    const float square = t*t;
    const float w0 = derivative ? 1.f*t+-1.f : (.5f*square+-1.f*t)+.5f;
    const float w1 = derivative ? 1.f+-2.f*t : (-1.f*square+1.f*t)+.5f;
    const float w2 = derivative ? 1.f*t+0.f : (.5f*square+0.f*t)+0.f;
    // 606130's X derivative is A+B+C; 4435F0 and both Y paths use B+A+C.
    const float x = derivative ? (a.x*w0+w1*b.x)+w2*c.x : (b.x*w1+a.x*w0)+c.x*w2;
    return {x, (b.y*w1+a.y*w0)+c.y*w2};
}
} // namespace
CoastDecalPaths build_coast_decal_paths(std::span<const CoastDecalSource> sources,
                                        Point2 center, X87Precision precision) {
    require(precision == X87Precision::single || precision == X87Precision::double_precision,
            "Unsupported coast decal precision");
    require(std::isfinite(center.x) && std::isfinite(center.y) && sources.size() <= max_coast_contours,
            "Invalid coast decal source extent/center");
    CoastDecalPaths result;
    std::size_t total = 0, paths = 0;
    for (const auto &source : sources) {
        if (source.owner_flag) continue;
        require(source.points.size() >= 4 && source.points.size() <= max_coast_contour_controls,
                "Unsupported coast decal contour size");
        total += source.points.size()*4-6;
        // Sum(4*n-6)=8*E+2*C <= 10*E for the original graph.
        require(total <= max_coast_graph_edges*10, "Coast decal paths exceed graph capacity");
        ++paths;
    }
    result.points.reserve(total); result.paths.reserve(paths);
    for (std::size_t s = 0; s < sources.size(); ++s) {
        const auto &source = sources[s];
        if (source.owner_flag) continue;
        std::vector<Point2> controls;
        for (auto p : source.points) controls.emplace_back(p.x, p.y);
        controls = subdivide(subdivide(controls));
        const auto count = static_cast<std::uint32_t>(controls.size());
        result.paths.push_back({static_cast<std::uint32_t>(result.points.size()), count,
                                static_cast<std::uint32_t>(s), source.closed});
        const float last = static_cast<float>(count-2), step = last/static_cast<float>(count-1);
        float argument = step+-step;
        for (std::uint32_t i = 0; i < count; ++i) {
            const auto tangent = sample(controls, argument, true), point = sample(controls, argument, false);
            const auto xx = round_x87(double(tangent.x)*tangent.x, precision);
            const auto yy = round_x87(double(tangent.y)*tangent.y, precision);
            const float length = static_cast<float>(round_x87(std::sqrt(round_x87(xx+yy, precision)), precision));
            require(length > 0 && std::isfinite(length), "Degenerate coast decal tangent");
            const float inverse = 1.f/length;
            const float x = point.x-(inverse*tangent.y)*5.f;
            const float y = (inverse*tangent.x)*5.f+point.y;
            result.points.emplace_back(x-center.x, y*.866f-center.y);
            argument = std::max(0.f, std::min(last, step+argument));
        }
    }
    return result;
}
} // namespace civ5::terrain
