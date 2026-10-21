#include "civ5/terrain/scene_polygon_owner.hpp"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace civ5::terrain {
namespace {
std::array<std::int32_t,3> transform(const std::array<std::int32_t,16> &matrix,
    const std::array<std::int32_t,3> &position){
    std::array<std::int32_t,3> out{};
    for(unsigned column=0;column<3;++column){
        std::uint32_t value=std::bit_cast<std::uint32_t>(matrix[12+column]);
        for(unsigned row=0;row<3;++row)
            value+=std::bit_cast<std::uint32_t>(matrix[row*4+column])*
                std::bit_cast<std::uint32_t>(position[row]);
        out[column]=std::bit_cast<std::int32_t>(value);
    }
    return out;
}
std::int64_t column_of(const std::array<std::int32_t,3> &position){
    // Arithmetic right shift in 408F30/7A0C10 rounds negative Y down.
    const auto half=position[1]>=0?position[1]/2:
        (static_cast<std::int64_t>(position[1])-1)/2;
    return static_cast<std::int64_t>(position[0])+half;
}
}
ScenePolygonOwner::ScenePolygonOwner(ScenePolygonOwnerGrid grid):grid_(grid){
    if(grid.width<=0 || grid.width>256 || grid.height<=0 || grid.height>256 ||
       grid.tile_ids.size()<static_cast<std::size_t>(grid.height)*256)
        throw std::invalid_argument("Invalid scene polygon source grid");
    if(grid.plot_store){
        for(std::int32_t y=0;y<grid.height;++y)
            for(std::int32_t x=0;x<grid.width;++x){
                const auto tile=grid.tile_ids[static_cast<std::size_t>(y)*256+x];
                if(tile!=0xffffu && tile>=grid.plot_store->slots().size())
                    throw std::invalid_argument("Scene polygon tile lacks a plot slot");
            }
    }
}
std::span<const std::uint16_t> ScenePolygonOwner::plot(std::uint16_t tile_id) const{
    auto it=plots_.find(tile_id);
    return it==plots_.end()?std::span<const std::uint16_t>{}:
        std::span<const std::uint16_t>(it->second);
}
std::uint16_t ScenePolygonOwner::tile_at(std::size_t offset) const{
    const auto it=tile_overrides_.find(offset);
    return it==tile_overrides_.end()?grid_.tile_ids[offset]:it->second;
}
std::vector<ScenePolygonNeighborReapply> ScenePolygonOwner::neighbor_reapplications(
    const std::array<std::int32_t,3> &position) const{
    const auto tile_at=[&](const std::array<std::int32_t,3> &cube)
        -> std::uint16_t{
        const auto mapped=transform(grid_.tile_matrix,cube);
        const auto column=column_of(mapped);
        if(mapped[1]<0 || mapped[1]>=grid_.height || column<0 || column>=grid_.width)
            return 0xffffu;
        return this->tile_at(static_cast<std::size_t>(mapped[1])*256+
            static_cast<std::size_t>(column));
    };
    if(tile_at(position)==0xffffu)return {};
    constexpr std::array<std::array<std::int32_t,3>,6> offsets{{
        {0,1,-1},{-1,0,1},{1,-1,0},{0,-1,1},{1,0,-1},{-1,1,0}}};
    std::vector<ScenePolygonNeighborReapply> result;
    for(const auto &offset:offsets){
        std::array<std::int32_t,3> neighbor{};
        for(unsigned i=0;i<3;++i)
            neighbor[i]=std::bit_cast<std::int32_t>(
                std::bit_cast<std::uint32_t>(position[i])+
                std::bit_cast<std::uint32_t>(offset[i]));
        const auto tile_id=tile_at(neighbor);
        if(tile_id==0xffffu)continue;
        const auto records=plot(tile_id);
        for(std::uint16_t slot=0;slot<records.size();++slot)
            result.push_back({tile_id,records[slot],slot});
    }
    return result;
}
std::array<float,4> ScenePolygonOwner::dirty_region(
    const std::array<std::int32_t,3> &position,
    const ScenePolygonDirtyProjection &projection) const{
    if(!std::all_of(projection.world_matrix.begin(),projection.world_matrix.end(),
            [](float value){return std::isfinite(value);}) ||
       !std::isfinite(projection.world_offset[0]) ||
       !std::isfinite(projection.world_offset[1]))
        throw std::invalid_argument("Nonfinite scene polygon dirty projection");
    constexpr float extent=64.f;
    constexpr float vertical_scale=std::bit_cast<float>(0x3f5db22du);
    const auto square=[&](const std::array<std::int32_t,3> &cube){
        auto translation=identity_matrix();
        for(unsigned axis=0;axis<3;++axis)
            translation[12+axis]=static_cast<float>(cube[axis]);
        const auto matrix=matrix_multiply(translation,projection.world_matrix,
            X87Precision::double_precision,MatrixPath::sse);
        const auto center=vector4_transform({0.f,0.f,-0.f,1.f},matrix,
            X87Precision::double_precision,Vec4Path::sse);
        const float left=center[0]-extent,top=center[1]-extent;
        const float right=center[0]+extent,bottom=center[1]+extent;
        return std::array<float,4>{projection.world_offset[0]+left,
            top*vertical_scale+projection.world_offset[1],
            projection.world_offset[0]+right,
            bottom*vertical_scale+projection.world_offset[1]};
    };
    const auto plot_x=static_cast<std::int32_t>(projection.plot_coordinates[0]);
    const auto plot_y=static_cast<std::int32_t>(projection.plot_coordinates[1]);
    std::array<float,4> bounds=square({plot_x,plot_y,wrap_neg(wrap_add(plot_x,plot_y))});
    constexpr std::array<std::array<std::int32_t,3>,6> offsets{{
        {0,1,-1},{-1,0,1},{1,-1,0},{0,-1,1},{1,0,-1},{-1,1,0}}};
    for(const auto &offset:offsets){
        std::array<std::int32_t,3> neighbor{};
        for(unsigned axis=0;axis<3;++axis)
            neighbor[axis]=wrap_add(position[axis],offset[axis]);
        const auto mapped=transform(grid_.tile_matrix,neighbor);
        const auto column=column_of(mapped);
        if(mapped[1]<0 || mapped[1]>=grid_.height || column<0 || column>=grid_.width)
            continue;
        const auto region=square(neighbor);
        if(!(region[0]>bounds[0]))bounds[0]=region[0];
        if(!(region[1]>bounds[1]))bounds[1]=region[1];
        if(!(bounds[2]>region[2]))bounds[2]=region[2];
        if(!(bounds[3]>region[3]))bounds[3]=region[3];
    }
    return bounds;
}
void ScenePolygonOwner::set_record_handles(std::uint32_t record_index,
    std::array<std::uint32_t,2> handles){
    if(record_index>=records_.size() || !records_[record_index].active)
        throw std::invalid_argument("Polygon handle record is not active");
    records_[record_index].handles=handles;
}
ScenePolygonRemovalResult ScenePolygonOwner::remove_cell(
    const std::array<std::int32_t,3> &position,
    const ScenePolygonDirtyProjection &projection,
    const ScenePolygonHandleRemove &remove_handle,
    const ScenePolygonMaskApply &apply_mask,
    const ScenePolygonDirty &dirty){
    const auto mapped=transform(grid_.tile_matrix,position);
    const auto column=column_of(mapped);
    if(mapped[1]<0 || mapped[1]>=grid_.height || column<0 || column>=grid_.width)
        return {};
    const auto cell=static_cast<std::size_t>(mapped[1])*256+
        static_cast<std::size_t>(column);
    const auto tile_id=tile_at(cell);
    if(tile_id==0xffffu)return {};
    if(grid_.plot_store && projection.plot_coordinates!=
        grid_.plot_store->plot_coordinates(tile_id))
        throw std::invalid_argument("Scene polygon projection differs from source plot");
    if(!remove_handle || !apply_mask || !dirty)
        throw std::invalid_argument("Polygon removal requires scene callbacks");
    const auto neighbors=neighbor_reapplications(position);
    ScenePolygonRemovalResult result{true,tile_id};
    if(const auto it=plots_.find(tile_id);it!=plots_.end()){
        for(std::uint32_t slot=0;slot<it->second.size();++slot){
            const auto index=it->second[slot];
            auto &record=records_[index];
            if(!record.active)throw std::logic_error("Polygon plot contains a freed record");
            for(auto &handle:record.handles){
                if(handle!=0xffffffffu){
                    remove_handle(handle);
                    handle=0xffffffffu;
                }
            }
            apply_mask(tile_id,slot,index,false,record.assembly.points());
            record.active=false;
            free_indices_.insert(std::lower_bound(free_indices_.begin(),free_indices_.end(),index),index);
            ++result.removed;
        }
    }
    const auto region=dirty_region(position,projection);
    for(const auto [neighbor_tile,index,slot]:neighbors){
        apply_mask(neighbor_tile,slot,index,true,records_[index].assembly.points());
        ++result.reapplied;
    }
    dirty(region);
    tile_overrides_[cell]=0xffffu;
    if(grid_.plot_store)grid_.plot_store->release(tile_id);
    plots_.erase(tile_id);
    return result;
}
ScenePolygonRemovalResult ScenePolygonOwner::remove_cell_using_plot(
    const std::array<std::int32_t,3> &position,const Matrix4 &world_matrix,
    std::array<float,2> world_offset,const ScenePolygonHandleRemove &remove_handle,
    const ScenePolygonMaskApply &apply_mask,const ScenePolygonDirty &dirty){
    if(!grid_.plot_store)
        throw std::invalid_argument("Scene polygon source has no plot records");
    const auto mapped=transform(grid_.tile_matrix,position);
    const auto column=column_of(mapped);
    if(mapped[1]<0 || mapped[1]>=grid_.height || column<0 || column>=grid_.width)
        return {};
    const auto cell=static_cast<std::size_t>(mapped[1])*256+
        static_cast<std::size_t>(column);
    const auto tile_id=tile_at(cell);
    if(tile_id==0xffffu)return {};
    return remove_cell(position,{grid_.plot_store->plot_coordinates(tile_id),
        world_matrix,world_offset},remove_handle,apply_mask,dirty);
}
std::uint16_t ScenePolygonOwner::insert_cell(
    const std::array<std::int32_t,3> &position,const ScenePolygonPlotBytes &source){
    if(!grid_.plot_store)
        throw std::invalid_argument("Scene polygon source has no plot records");
    // 5415F0 addresses the tile grid through owner +700, the tile matrix.
    const auto mapped=transform(grid_.tile_matrix,position);
    const auto column=column_of(mapped);
    if(mapped[1]<0 || mapped[1]>=grid_.height || column<0 || column>=grid_.width)
        throw std::invalid_argument("Scene polygon insertion outside source grid");
    const auto cell=static_cast<std::size_t>(mapped[1])*256+
        static_cast<std::size_t>(column);
    if(tile_at(cell)!=0xffffu)
        throw std::invalid_argument("Scene polygon plot cell is occupied");
    const auto tile_id=grid_.plot_store->insert(source);
    tile_overrides_[cell]=static_cast<std::uint16_t>(tile_id);
    return static_cast<std::uint16_t>(tile_id);
}
ScenePolygonPlotEventResult ScenePolygonOwner::apply_plot_source_event(
    const ScenePolygonPlotSourceEvent &event,std::uint32_t scene_identity,
    std::uint32_t scene_gate,std::uint32_t variation_count,
    const ScenePolygonRandomDraw &random,
    const ScenePolygonMaterialResolve &resolve_material,
    const Matrix4 &world_matrix,std::array<float,2> world_offset,
    const ScenePolygonHandleRemove &remove_handle,
    const ScenePolygonMaskApply &apply_mask,const ScenePolygonDirty &dirty,
    const ScenePolygonCellNotify &notify){
    if(!grid_.plot_store || !notify)
        throw std::invalid_argument("Scene polygon event requires plot storage and notification");
    const std::array<std::int32_t,3> position{event.x,event.y,
        -static_cast<std::int32_t>(event.x)-event.y};
    const auto mapped=transform(grid_.tile_matrix,position);
    const auto column=column_of(mapped);
    if(mapped[1]<0 || mapped[1]>=grid_.height || column<0 || column>=grid_.width)
        return {};
    const auto cell=static_cast<std::size_t>(mapped[1])*256+
        static_cast<std::size_t>(column);
    const auto old_tile=tile_at(cell);
    if(old_tile==0xffffu && event.gate!=scene_gate)return {};
    if(old_tile!=0xffffu){
        const auto removal=remove_cell_using_plot(position,world_matrix,world_offset,
            remove_handle,apply_mask,dirty);
        if(!removal.found)
            throw std::logic_error("Occupied scene polygon tile was not removed");
        if(event.gate!=scene_gate)
            return {ScenePolygonPlotEventAction::removed,0xffffu};
    }
    const auto record=make_scene_polygon_plot_record(event,scene_identity,
        variation_count,random,resolve_material);
    const auto tile=insert_cell(position,record);
    notify(tile);
    return {old_tile==0xffffu?ScenePolygonPlotEventAction::inserted:
        ScenePolygonPlotEventAction::replaced,tile};
}
void ScenePolygonOwner::finish(std::uint16_t tile_id,std::uint32_t record_index,
    const ScenePolygonOwnerSubmit &submit){
    auto &entries=plots_[tile_id];
    if(entries.size()<50){
        const auto slot=static_cast<std::uint32_t>(entries.size());
        entries.push_back(static_cast<std::uint16_t>(record_index));
        if(submit)submit(tile_id,slot,record_index,records_[record_index].assembly.points());
    }
    ++completed_count_;
}
ScenePolygonEventResult ScenePolygonOwner::accept(const ScenePolygonEvent &event,
    const ScenePolygonOwnerSubmit &submit,bool defer){
    const auto position=transform(grid_.bounds_matrix,event.position);
    const auto column=column_of(position);
    // 5750F0 calls the same 408F30 gate twice before looking up a tile.
    if(column<0 || column>=grid_.width || position[1]<0 || position[1]>=grid_.height)
        return ScenePolygonEventResult::rejected;
    const auto tile_position=transform(grid_.tile_matrix,event.position);
    const auto tile_column=column_of(tile_position);
    if(tile_position[1]<0 || tile_position[1]>=grid_.height ||
       tile_column<0 || tile_column>=256)
        throw std::invalid_argument("Tile matrix outside bounded source grid");
    const auto tile_id=tile_at(static_cast<std::size_t>(tile_position[1])*256+
        static_cast<std::size_t>(tile_column));
    if(tile_id==0xffffu)return ScenePolygonEventResult::rejected;
    if(defer){deferred_.push_back(event);return ScenePolygonEventResult::deferred;}
    for(auto it=pending_.begin();it!=pending_.end();++it){
        auto &record=records_[*it];
        if(record.polygon_id!=event.polygon_id)continue;
        if(record.assembly.append(event.chunk)){
            const auto index=*it;
            finish(tile_id,index,submit);
            pending_.erase(it);
            return ScenePolygonEventResult::completed;
        }
        return ScenePolygonEventResult::pending;
    }
    ScenePolygonPointAssembly assembly;
    const bool complete=assembly.append(event.chunk);
    std::uint32_t index;
    if(!free_indices_.empty()){
        index=free_indices_.front();
        free_indices_.erase(free_indices_.begin());
        records_[index]={event.polygon_id,tile_id,std::move(assembly)};
    }else{
        if(records_.size()>=65536)throw std::length_error("Polygon plot record index exceeds U16");
        index=static_cast<std::uint32_t>(records_.size());
        records_.push_back({event.polygon_id,tile_id,std::move(assembly)});
    }
    if(complete){
        finish(tile_id,index,submit);
        return ScenePolygonEventResult::completed;
    }
    pending_.push_back(index);
    return ScenePolygonEventResult::pending;
}
std::size_t ScenePolygonOwner::drain_deferred(const ScenePolygonOwnerSubmit &submit){
    std::vector<ScenePolygonEvent> queued;
    queued.swap(deferred_);
    for(const auto &event:queued)accept(event,submit,false);
    return queued.size();
}
} // namespace civ5::terrain
