#include "civ5/terrain/scene_mask.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace civ5::terrain {
namespace {
void require(bool ok,const char *message) { if (!ok) throw std::invalid_argument(message); }
std::int32_t dimension(std::int32_t value) {
    require(value>=2 && value<=65536,"Invalid scene mask raster dimensions");
    return ((value/2+15)/16)*16;
}
SceneMaskUpdate validate(std::span<const Point2> points,std::uint8_t bits,
                         CoastBoundsPrecision precision,std::int32_t width) {
    require(precision==CoastBoundsPrecision::single || precision==CoastBoundsPrecision::double_or_extended,
            "Unsupported scene mask bounds precision");
    const auto high=std::numeric_limits<float>::max();
    SceneMaskUpdate update{{high,high,-high,-high},bits};
    for (auto p:points) {
        require(std::isfinite(p.x) && std::isfinite(p.y) && std::abs(p.x)<=65536 && std::abs(p.y)<=65536 &&
                p.x>=-198.f*float(width),"Invalid scene mask point/wrap range");
        // Match original comparisons, including which signed zero wins a tie.
        if (!(update.bounds[0]<=p.x)) update.bounds[0]=p.x;
        if (!(update.bounds[1]<=p.y)) update.bounds[1]=p.y;
        if (!(p.x<=update.bounds[2])) update.bounds[2]=p.x;
        if (!(p.y<=update.bounds[3])) update.bounds[3]=p.y;
    }
    return update;
}
void work_bound(Point2 a,Point2 b,Point2 c) {
    const double x=std::max({a.x,b.x,c.x})-double(std::min({a.x,b.x,c.x}));
    const double y=std::max({a.y,b.y,c.y})-double(std::min({a.y,b.y,c.y}));
    require((x*.5+3)*(y*.5+3)<=16777216,"Scene mask triangle exceeds bounded work");
}
} // namespace
SceneMask::SceneMask(std::int32_t raster_width,std::int32_t raster_height)
    :width_(dimension(raster_width)),height_(dimension(raster_height)) {
    require(std::uint64_t(width_)*std::uint64_t(height_)<=16777216,"Scene mask exceeds field capacity");
    flags_.resize(std::size_t(width_)*std::size_t(height_));
}
void SceneMask::raster(Point2 a,Point2 b,Point2 c,std::uint8_t bits,bool setting,
                       CoastBoundsPrecision precision,const SceneMaskWriteObserver &observer) {
    a={a.x*.5f,a.y*.5f}; b={b.x*.5f,b.y*.5f}; c={c.x*.5f,c.y*.5f};
    const auto lower=[&](float v) {
        const double sum=double(v)+.5;
        return float(std::floor(precision==CoastBoundsPrecision::single?double(float(sum)):sum));
    };
    const float sx=lower(std::min({a.x,b.x,c.x})),sy=lower(std::min({a.y,b.y,c.y}));
    const auto x0=static_cast<std::int32_t>(sx),y0=static_cast<std::int32_t>(sy);
    const auto x1=static_cast<std::int32_t>(std::max({a.x,b.x,c.x}))+1;
    const auto y1=static_cast<std::int32_t>(std::max({a.y,b.y,c.y}))+1;
    const float ax=a.x-sx,ay=a.y-sy,bx=b.x-sx,by=b.y-sy,cx=c.x-sx,cy=c.y-sy;
    const float ex=by-cy,ey=cx-bx,ec=cy*bx-cx*by;
    const float fx=cy-ay,fy=ax-cx,fc=cx*ay-cy*ax;
    const float gx=ay-by,gy=bx-ax,gc=by*ax-bx*ay;
    const float determinant=ey*ay+ex*ax+ec;
    if (std::abs(double(determinant))<1e-5) return;
    const float inverse=1.f/determinant;
    float dy=0;
    for (auto y=y0;y<y1;++y) {
        float dx=0;
        const float row_a=ec*inverse+ey*dy,row_b=fy*dy,row_c=gy*dy;
        const float base_b=fc*inverse,base_c=gc*inverse;
        for (auto x=x0;x<x1;++x) {
            // The third edge adds row_c BEFORE base_c in these mask bodies.
            if (ex*dx+row_a>=0 && fx*dx+base_b+row_b>=0 && gx*dx+row_c+base_c>=0 && y>=0 && y<height_) {
                const auto wrapped=(width_*100+x)%width_;
                const auto at=std::uint32_t(y)*std::uint32_t(width_)+std::uint32_t(wrapped);
                auto &value=flags_[at];
                value=static_cast<std::uint8_t>((value&~bits)|(setting?bits:0));
                if (observer) observer(at,value);
            }
            dx+=inverse;
        }
        dy+=inverse;
    }
}
SceneMaskUpdate SceneMask::indexed(std::span<const Point2> points,std::span<const std::uint16_t> indices,
    std::uint8_t bits,bool setting,CoastBoundsPrecision precision,const SceneMaskWriteObserver &observer) {
    const auto update=validate(points,bits,precision,width_);
    require(indices.size()%3==0,"Incomplete scene mask triangles");
    for (auto i:indices) require(i<points.size(),"Scene mask index outside points");
    for (std::size_t i=0;i<indices.size();i+=3) work_bound(points[indices[i]],points[indices[i+1]],points[indices[i+2]]);
    for (std::size_t i=0;i<indices.size();i+=3) raster(points[indices[i]],points[indices[i+1]],points[indices[i+2]],bits,setting,precision,observer);
    return update;
}
SceneMaskUpdate SceneMask::triangles(std::span<const Point2> points,std::uint8_t bits,bool setting,
    CoastBoundsPrecision precision,const SceneMaskWriteObserver &observer) {
    const auto update=validate(points,bits,precision,width_);
    // Original 7331A0 truncates the count/3; leftover points still affect bounds.
    for (std::size_t i=0;i+2<points.size();i+=3) work_bound(points[i],points[i+1],points[i+2]);
    for (std::size_t i=0;i+2<points.size();i+=3) raster(points[i],points[i+1],points[i+2],bits,setting,precision,observer);
    return update;
}
std::vector<SceneMaskUpdate> SceneMask::polygon(std::span<const Point2> points,Point2 offset,bool setting,
    CoastBoundsPrecision precision,const ReciprocalEstimate &reciprocal,const SceneMaskWriteObserver &observer) {
    const auto mesh=build_scene_mask_polygon(points,offset,precision,reciprocal);
    if (!mesh) return {};
    // Preflight both packets so malformed caller input cannot partially update
    // the owned field. Original callers only provide valid native polygons.
    for (const auto *vertices:{&mesh->raster_points,&mesh->expanded_raster_points}) {
        validate(*vertices,0,precision,width_);
        for (std::size_t i=0;i<mesh->indices.size();i+=3)
            work_bound((*vertices)[mesh->indices[i]],(*vertices)[mesh->indices[i+1]],(*vertices)[mesh->indices[i+2]]);
    }
    std::vector<SceneMaskUpdate> updates;
    updates.push_back(indexed(mesh->raster_points,mesh->indices,1,true,precision,observer));
    updates.push_back(indexed(mesh->expanded_raster_points,mesh->indices,16,setting,precision,observer));
    return updates;
}
CoastMaskMesh build_coast_mask_mesh(const CoastCurveSet &set) {
    const auto count=static_cast<std::uint32_t>(set.control_count());
    require(count>=4 && count<=max_coast_curve_controls,"Unsupported coast mask curve count");
    CoastMaskMesh mesh;
    mesh.points.reserve((count-1)*2);
    for (std::uint32_t i=0;i<count-1;++i) {
        const auto a=sample_coast_curve(set.curves()[1],static_cast<float>(i));
        const auto b=sample_coast_curve(set.curves()[4],static_cast<float>(i));
        mesh.points.push_back({a.x,a.y});mesh.points.push_back({b.x,b.y});
    }
    const auto triangles=(count-2)*2;
    std::uint32_t limit=64;
    for (;;) {
        const auto chunks=(triangles-1)/limit+1,remainder=triangles%limit;
        if (!(chunks>1 && chunks%2!=0) && !(remainder>=1 && remainder<=3)) break;
        limit+=2;
    }
    mesh.indices.reserve(triangles*3);
    for (std::uint32_t row=0;row<count-2;) {
        const auto connections=std::min(limit/2,count-2-row);
        require((connections+1)*2<=65536,"Coast mask chunk exceeds U16 index capacity");
        mesh.chunks.push_back({row*2,(connections+1)*2,row*2,connections*2,9});
        for (std::uint32_t i=0;i<connections;++i) {
            const auto base=i*2;
            for (auto v:{base,base+1,base+2,base+1,base+3,base+2}) mesh.indices.push_back(static_cast<std::uint16_t>(v));
        }
        row+=connections;
    }
    return mesh;
}
SceneMask build_coast_scene_mask(std::span<const CoastCurveSet> curves,std::int32_t raster_width,
    std::int32_t raster_height,CoastBoundsPrecision precision) {
    require(precision==CoastBoundsPrecision::single || precision==CoastBoundsPrecision::double_or_extended,
            "Unsupported coast mask bounds precision");
    SceneMask mask(raster_width,raster_height);
    for (const auto &set:curves) {
        if (set.owner_flag()!=0) continue;
        const auto mesh=build_coast_mask_mesh(set);
        for (auto chunk:mesh.chunks)
            mask.indexed(std::span(mesh.points).subspan(chunk.first_vertex,chunk.vertex_count),
                         std::span(mesh.indices).subspan(chunk.first_triangle*3,chunk.triangle_count*3),
                         chunk.bits,true,precision);
    }
    return mask;
}
} // namespace civ5::terrain
