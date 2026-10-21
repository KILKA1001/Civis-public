#include "civ5/terrain/scene_mask.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace civ5::terrain {
namespace {
void require(bool ok,const char *message) { if (!ok) throw std::invalid_argument(message); }
struct Arithmetic {
    X87Precision precision;
    double q(double value) const { return round_x87(value,precision); }
    double triangle(Point2 a,Point2 b,Point2 c) const {
        const auto first=q(q(double(c.y)-a.y)*q(double(b.x)-a.x));
        const auto second=q(q(double(b.y)-a.y)*q(double(c.x)-a.x));
        return q(q(first-second)*.5);
    }
};
bool contains(const std::array<Point2,3> &triangle,Point2 point) {
    // Removed vertices retain Y but mark X as quiet NaN in the original.
    if (std::isnan(point.x)) return false;
    unsigned crossings=0;
    auto previous=triangle.back();
    for (auto current:triangle) {
        if (std::min(previous.y,current.y)<point.y && point.y<=std::max(previous.y,current.y) &&
            point.x<=std::max(previous.x,current.x) && previous.y!=current.y &&
            (previous.x==current.x || point.x<=
                ((point.y-previous.y)*(current.x-previous.x))/(current.y-previous.y)+previous.x)) ++crossings;
        previous=current;
    }
    return crossings%2!=0;
}
std::optional<std::vector<std::uint16_t>> triangulate(std::span<const Point2> points,Arithmetic math) {
    if (points.size()<3) return std::nullopt;
    float sum=0;
    auto previous=points.back();
    for (auto current:points) {
        sum=(current.y-previous.y)*previous.x+sum;
        previous=current;
    }
    // This is the actual 895390 accumulator, not the shoelace formula. In
    // particular, reversing a right triangle can change success to failure.
    const float orientation=static_cast<float>(math.q(double(sum)*.5));
    std::vector<Point2> work(points.begin(),points.end());
    std::vector<std::uint16_t> indices;
    for (std::size_t remaining=points.size()-2;remaining;--remaining) {
        bool found=false;
        for (std::size_t a=0;a<work.size();++a) {
            if (std::isnan(work[a].x)) continue;
            auto b=(a+1)%work.size();while(std::isnan(work[b].x)) b=(b+1)%work.size();
            auto c=(b+1)%work.size();while(std::isnan(work[c].x)) c=(c+1)%work.size();
            const std::array<Point2,3> candidate{work[a],work[b],work[c]};
            bool occupied=false;
            for (std::size_t p=0;p<work.size();++p)
                if (p!=a && p!=b && p!=c && contains(candidate,work[p])) { occupied=true;break; }
            if (math.q(math.triangle(work[a],work[b],work[c])*orientation)>0 && !occupied) {
                work[b].x=std::bit_cast<float>(0x7fc00000u);
                for (auto index:{a,b,c}) indices.push_back(static_cast<std::uint16_t>(index));
                found=true;break;
            }
        }
        if (!found) return std::nullopt;
    }
    return indices;
}
Point2 centroid(std::span<const Point2> points) {
    Point2 result{0,0};float sum=0;auto previous=points.back();
    for (auto current:points) {
        const float cross=current.y*previous.x-current.x*previous.y;
        result.x=(current.x+previous.x)*cross+result.x;
        result.y=(previous.y+current.y)*cross+result.y;
        sum=cross+sum;previous=current;
    }
    const float inverse=1.f/(sum*3.f);
    return {inverse*result.x,inverse*result.y};
}
bool valid(Point2 point) {
    return std::isfinite(point.x) && std::isfinite(point.y) && std::abs(point.x)<=65536 && std::abs(point.y)<=65536;
}
} // namespace
std::optional<ScenePolygonShape> triangulate_scene_polygon(std::span<const Point2> points,
    CoastBoundsPrecision precision) {
    require(points.size()<=512,"Scene mask polygon exceeds bounded triangulation capacity");
    require(precision==CoastBoundsPrecision::single || precision==CoastBoundsPrecision::double_or_extended,
        "Unsupported polygon mask precision");
    for (auto point:points) require(valid(point),"Invalid polygon mask source point");
    const Arithmetic math{precision==CoastBoundsPrecision::single?X87Precision::single:X87Precision::double_precision};
    auto indices=triangulate(points,math);
    if (!indices) return std::nullopt;
    ScenePolygonShape result{centroid(points),std::move(*indices)};
    require(valid(result.centroid),"Invalid polygon mask centroid");
    return result;
}
std::optional<SceneMaskPolygon> build_scene_mask_polygon(std::span<const Point2> points,
    Point2 offset,CoastBoundsPrecision precision,const ReciprocalEstimate &reciprocal) {
    require(valid(offset),"Invalid polygon mask offset");
    auto shape=triangulate_scene_polygon(points,precision);
    if (!shape) return std::nullopt;
    require(bool(reciprocal),"Polygon mask requires explicit RCPSS estimate");
    const Arithmetic math{precision==CoastBoundsPrecision::single?X87Precision::single:X87Precision::double_precision};
    SceneMaskPolygon result{shape->centroid,{},{},std::move(shape->indices)};
    const auto convert=[&](Point2 point) {
        const float estimate=reciprocal(.866f);
        require(std::isfinite(estimate) && estimate>0,"Invalid polygon mask RCPSS estimate");
        const Point2 raster{point.x+offset.x,(point.y+offset.y)*estimate};
        require(valid(raster),"Polygon mask conversion exceeds coordinate range");
        return raster;
    };
    for (auto point:points) result.raster_points.push_back(convert(point));
    for (auto point:points) {
        const float dx=-result.centroid.x+point.x,dy=point.y+-result.centroid.y;
        // SSE products spill before the x87 sum/sqrt; the length spills again
        // before DIVSS. Keeping the whole expression in double changes bytes.
        const float xx=dx*dx,yy=dy*dy;
        const float length=static_cast<float>(math.q(std::sqrt(math.q(double(yy)+xx))));
        require(std::isfinite(length) && length>0,"Polygon vertex coincides with its centroid");
        const float inverse=1.f/length;
        const Point2 expanded{(inverse*dx)*5.f+point.x,point.y+(dy*inverse)*5.f};
        result.expanded_raster_points.push_back(convert(expanded));
    }
    return result;
}
} // namespace civ5::terrain
