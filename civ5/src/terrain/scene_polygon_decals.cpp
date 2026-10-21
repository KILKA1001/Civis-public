#include "civ5/terrain/scene_polygon_decals.hpp"
#include <cmath>
#include <stdexcept>

namespace civ5::terrain {
namespace {
void require(bool value,const char *message){if(!value)throw std::invalid_argument(message);}
}
ScenePolygonDecalUpdate update_scene_polygon_decals(std::span<const Point2> points,
    ScenePolygonDecalState &state,SceneMask &mask,Point2 world_offset,
    const SceneDirectionField &direction,const ScenePolygonDecalPalette &palette,
    TerrainRandom &random,CoastBoundsPrecision precision,Vec4Path transform,
    const ReciprocalEstimate &reciprocal,const ScenePolygonDecalSubmit &submit,
    const SceneMaskWriteObserver &mask_observer) {
    require(bool(submit),"Missing polygon decal submission");
    require(transform==Vec4Path::legacy_x87 || transform==Vec4Path::sse,"Unsupported polygon decal transform");
    for(const auto range:palette.ranges)
        require(range.first<=65535 && range.end>=range.first && range.end-range.first<=65535 &&
            range.end<=palette.records.size(),"Invalid polygon decal palette range");
    ScenePolygonDecalUpdate result;
    result.mask_updates=mask.polygon(points,world_offset,true,precision,reciprocal,mask_observer);
    if(result.mask_updates.empty())return result;
    auto shape=triangulate_scene_polygon(points,precision);
    require(bool(shape),"Polygon changed during admission");
    const auto pc=precision==CoastBoundsPrecision::single?X87Precision::single:X87Precision::double_precision;
    auto axis=direction.direction(shape->centroid,pc);
    const float length=std::sqrt(axis.x*axis.x+axis.y*axis.y);
    if(length==0)axis={0,0};
    else {const float inverse=1.f/length;axis={axis.x*inverse,axis.y*inverse};}
    float angle=static_cast<float>(-std::atan2(double(axis.y),double(axis.x)));
    if(random.next()%3==0)angle+=std::bit_cast<float>(0x3fc90fdbu);
    result.texture_angle=angle;
    const float cosine=static_cast<float>(std::cos(double(angle)));
    const float sine=static_cast<float>(std::sin(double(angle)));
    Matrix4 rotation{cosine,sine,0,0,-sine,cosine,0,0,0,0,1,0,0,0,0,1};
    TerrainDecalGeometry geometry;
    geometry.indices=std::move(shape->indices);
    geometry.vertices.reserve(points.size());
    for(const auto p:points){
        const float u=(-shape->centroid.x+p.x)*.0390625f+.5f;
        const float v=(p.y+-shape->centroid.y)*.0390625f+.5f;
        const auto uv=vector4_transform({u,v,0,1},rotation,pc,transform);
        geometry.vertices.push_back({world_offset.x+p.x,(p.y+world_offset.y)*std::bit_cast<float>(0x3f93ce56u),uv[0],uv[1]});
    }
    const auto kind=(state.flags>>4)&15;
    if(kind<1 || kind>4 || !state.enabled)return result;
    unsigned slot=kind==1?2:kind==2?3:kind==3?1:4;
    if(state.flags&0x200)slot=state.type==2?6:5;
    else if(state.type==2)slot=0;
    const auto primary=palette.ranges[slot];
    if(primary.first==primary.end)return result;
    const auto emit=[&](unsigned output,TerrainDecalRange range){
        require(range.first<range.end,"Empty secondary polygon decal range");
        const auto selected=range.first+random.next()%(range.end-range.first);
        const auto handle=submit(output,selected,palette.records[selected],geometry);
        state.handles[output]=handle;
        ++result.submitted;
    };
    emit(0,primary);
    if(state.type==8)emit(1,palette.ranges[7]);
    return result;
}
} // namespace civ5::terrain
