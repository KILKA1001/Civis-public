#include "civ5/runtime/bnw_world_anchor_graphics_adapter.hpp"
#include <climits>
#include <stdexcept>
#include <utility>

namespace civ5::runtime {

BnwWorldAnchorGraphicsBinding bind_bnw_world_anchor_graphics(
    const LiveSessionState &state,
    std::shared_ptr<BnwWorldAnchorGraphicsOwner> owner) {
    const auto &map=state.world.map_header;
    if(state.world.game_state.layout!=saves::GameStateLayout::expansion2_version1 ||
       map.layout!=saves::GameStateLayout::expansion2_version1 ||
       !map.grid.width || !map.grid.height ||
       map.grid.width>INT32_MAX || map.grid.height>INT32_MAX)
        return {{},"BNW world-anchor graphics binding requires a valid BNW map"};
    if(!owner || !owner->supports_gameplay_world_anchor())
        return {{},"BNW graphics owner has no GameplayWorldAnchor capability"};
    const BnwWorldAnchorGraphicsIdentity identity{
        map.guid,map.grid.width,map.grid.height};
    try {
        if(owner->current_world()!=identity)
            return {{},"BNW graphics owner is attached to another map"};
        if(!owner->publication_context_current())
            return {{},"BNW graphics publication context is unavailable"};
    } catch(const std::exception &error) {
        return {{},error.what()};
    }
    return {[owner=std::move(owner),identity](
        const BnwWorldAnchorRemovalEvent &event) {
        if(event.map_guid!=identity.map_guid ||
           event.map_width!=identity.width ||
           event.map_height!=identity.height ||
           event.add || event.data!=-1 || event.old_anchor<0 ||
           event.old_anchor>127 || event.x<0 || event.y<0 ||
           event.x>=static_cast<std::int64_t>(identity.width) ||
           event.y>=static_cast<std::int64_t>(identity.height) ||
           event.plot!=static_cast<std::uint64_t>(event.y)*identity.width+
                       static_cast<std::uint64_t>(event.x))
            throw std::runtime_error("BNW GameplayWorldAnchor event identity or arguments invalid");
        if(!owner->supports_gameplay_world_anchor() ||
           owner->current_world()!=identity ||
           !owner->publication_context_current())
            throw std::runtime_error("BNW GameplayWorldAnchor owner changed before publication");
        owner->gameplay_world_anchor(
            static_cast<std::int32_t>(event.old_anchor),false,
            static_cast<std::int32_t>(event.x),
            static_cast<std::int32_t>(event.y),-1);
    },{}};
}

} // namespace civ5::runtime
