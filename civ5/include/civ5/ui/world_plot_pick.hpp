#pragma once
#include "civ5/ui/bnw_client_frame.hpp"
#include "civ5/terrain/height_owner.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace civ5::ui {
// Convert an actual rendered depth sample, never a guessed ground-plane ray.
// Authentication and GPU read ownership belong to the native caller.
inline std::optional<std::int64_t> world_plot_from_rendered_depth(
    const BnwClientFrame &frame, const terrain::WorldLayout &layout,
    const terrain::HeightOwnerParameters &height, const terrain::CameraFrameState &camera,
    std::int32_t pixel_x, std::int32_t pixel_y, std::int32_t width, std::int32_t extent_y,
    float depth, const terrain::ReciprocalEstimate &reciprocal) {
    using namespace terrain;
    if(!frame.world.map_projection) throw std::invalid_argument("Picked frame has no map");
    const auto &map=*frame.world.map_projection;
    if(width<=0 || extent_y<=0 || layout.source_grid.width<=0 || layout.source_grid.height<=0 ||
       map.grid.width!=static_cast<std::uint64_t>(layout.source_grid.width) ||
       map.grid.height!=static_cast<std::uint64_t>(layout.source_grid.height) ||
       map.cells.size()!=map.grid.size() || map.grid.size()>INT32_MAX || map.wrap_y ||
       !(height.scale_x>0) || !(height.scale_y>0) || !std::isfinite(height.scale_x) ||
       !std::isfinite(height.scale_y) || camera.view_count==0 || camera.view_count>camera.views.size())
        throw std::invalid_argument("Picked render/map geometry differs");
    if(pixel_x<0 || pixel_y<0 || pixel_x>=width || pixel_y>=extent_y) return {};
    if(!std::isfinite(depth) || depth<0 || depth>1) throw std::invalid_argument("Invalid GPU depth");
    if(depth==1) return {}; // The actual world target's untouched clear value.
    // NativeClipScope uses ZERO_TO_ONE and a +0.5 viewport offset. Scene
    // texture rows retain D3D top-to-bottom order, including integer centers.
    const Vector4 ndc{2.f*pixel_x/width-1.f,1.f-2.f*pixel_y/extent_y,depth,1};
    Matrix4 projection;std::copy_n(layout.transforms.begin(),16,projection.begin());
    std::optional<std::int64_t> result;
    for(std::uint32_t view=0;view<camera.view_count;++view) {
        const auto vp=matrix_multiply(camera.views[view],camera.projection,
            X87Precision::double_precision,MatrixPath::sse);
        const auto inverse=inverse_sse(vp,reciprocal);
        auto ground=vector4_transform(ndc,inverse,X87Precision::double_precision,Vec4Path::sse);
        if(!std::all_of(ground.begin(),ground.end(),[](float v){return std::isfinite(v);}) || ground[3]==0)
            throw std::invalid_argument("Picked camera cannot unproject");
        const Vector4 scaled{(ground[0]/ground[3]+height.center_x)/height.scale_x,
            (ground[1]/ground[3]+height.center_y)/height.scale_y,0,1};
        const auto cube=vector4_transform(scaled,projection,X87Precision::double_precision,Vec4Path::sse);
        Vector3 rounded;
        for(unsigned axis=0;axis<3;++axis) {
            if(!std::isfinite(cube[axis]) || std::abs(double(cube[axis]))>INT32_MAX/16.)
                throw std::invalid_argument("Picked cube coordinate overflow");
            const auto value=round_x87(double(cube[axis])+(cube[axis]>=0?.5:-.5),X87Precision::double_precision);
            rounded[axis]=static_cast<float>(cube[axis]>=0?std::floor(value):std::ceil(value));
        }
        const auto sum=rounded[0]+(rounded[1]+rounded[2]);
        if(sum!=0) {
            Vector3 delta;for(unsigned axis=0;axis<3;++axis)
                delta[axis]=sum>0?cube[axis]-rounded[axis]:rounded[axis]-cube[axis];
            const auto minimum=std::min(std::min(delta[0],delta[1]),delta[2]);
            if(minimum==delta[0])rounded[0]=-(rounded[1]+rounded[2]);
            else if(minimum==delta[1])rounded[1]=-(rounded[0]+rounded[2]);
            else rounded[2]=-(rounded[0]+rounded[1]);
        }
        const auto &m=layout.source_grid.first_matrix;
        const auto component=[&](unsigned axis) {
            return std::int64_t(m[axis])*static_cast<std::int32_t>(rounded[0])+
                std::int64_t(m[4+axis])*static_cast<std::int32_t>(rounded[1])+
                std::int64_t(m[8+axis])*static_cast<std::int32_t>(rounded[2])+m[12+axis];
        };
        const auto y=component(1);auto x=component(0)+(y>>1);
        if(map.wrap_x)x=(x%layout.source_grid.width+layout.source_grid.width)%layout.source_grid.width;
        if(x<0 || y<0 || x>=layout.source_grid.width || y>=layout.source_grid.height) continue;
        const auto plot=y*layout.source_grid.width+x;
        if(result && *result!=plot)throw std::invalid_argument("Wrapped camera hit is ambiguous");
        result=plot;
    }
    return result; // SavedMapProjection's canonical row-major SDK plot index.
}
} // namespace civ5::ui
