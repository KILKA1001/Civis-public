#include "civ5/graphics/gl_bnw_world_anchor_scene.hpp"
#include <climits>
#include <stdexcept>
#include <utility>

namespace civ5::graphics {
namespace {
void require(bool condition, const char *reason) {
    if (!condition) throw std::runtime_error(reason);
}
} // namespace

GlBnwWorldAnchorScene::GlBnwWorldAnchorScene(
    const runtime::LiveSessionState &state,
    std::shared_ptr<GlContext> context,
    std::shared_ptr<GlBnwWorldAnchorDrawBackend> backend)
    : context_(std::move(context)), backend_(std::move(backend)),
      render_thread_(std::this_thread::get_id()) {
    const auto &world=state.world;
    const auto &map=world.map_header;
    require(world.game_state.layout==saves::GameStateLayout::expansion2_version1 &&
            map.layout==saves::GameStateLayout::expansion2_version1 &&
            map.grid.width>0 && map.grid.height>0 &&
            map.grid.width<=INT32_MAX && map.grid.height<=INT32_MAX &&
            world.map_plots && world.map_projection &&
            world.map_plots->plots.size()==map.grid.size() &&
            world.map_projection->cells.size()==map.grid.size(),
            "BNW GL anchor scene requires a complete BNW map");
    identity_={map.guid,map.grid.width,map.grid.height};
    for(std::uint64_t plot=0;plot<map.grid.size();++plot) {
        const auto plan=runtime::plan_bnw_world_anchor_clear(state,plot);
        require(plan.valid,"BNW GL anchor scene has an invalid source plot");
        if(plan.removal) {
            const auto &source=*plan.removal;
            require(source.x<=INT32_MAX && source.y<=INT32_MAX &&
                    source.old_anchor>=0 && source.old_anchor<=127,
                    "BNW GL anchor scene source exceeds graphics range");
            require(anchors_.emplace(plot,Anchor{
                static_cast<std::int32_t>(source.old_anchor),
                static_cast<std::int32_t>(source.x),
                static_cast<std::int32_t>(source.y)}).second,
                "Duplicate BNW GL anchor scene plot");
        }
    }
}

runtime::BnwWorldAnchorGraphicsIdentity
GlBnwWorldAnchorScene::current_world() const { return identity_; }

bool GlBnwWorldAnchorScene::publication_context_current() const {
    if(!context_ || render_thread_!=std::this_thread::get_id()) return false;
    try { context_->check("BNW world anchor scene publication"); }
    catch(const std::exception &) { return false; }
    return true;
}

bool GlBnwWorldAnchorScene::supports_gameplay_world_anchor() const {
    if(uncertain_ || !backend_ || !context_ ||
       backend_->context()!=context_.get() ||
       render_thread_!=std::this_thread::get_id()) return false;
    try {
        if(backend_->world_identity()!=identity_ ||
           !publication_context_current()) return false;
        for(const auto &[plot,anchor]:anchors_)
            if(!backend_->has_drawable(plot,anchor.type,anchor.x,anchor.y))
                return false;
        return true;
    } catch(const std::exception &) { return false; }
}

void GlBnwWorldAnchorScene::gameplay_world_anchor(
    std::int32_t anchor,bool add,std::int32_t x,std::int32_t y,
    std::int32_t data) {
    require(!add && data==-1 && x>=0 && y>=0 &&
            static_cast<std::uint64_t>(x)<identity_.width &&
            static_cast<std::uint64_t>(y)<identity_.height,
            "BNW GL anchor scene supports exact SDK removal only");
    const auto plot=static_cast<std::uint64_t>(y)*identity_.width+
                    static_cast<std::uint64_t>(x);
    const auto found=anchors_.find(plot);
    require(found!=anchors_.end() && found->second.type==anchor &&
            found->second.x==x && found->second.y==y,
            "BNW GL anchor removal disagrees with retained source object");
    require(supports_gameplay_world_anchor(),
            "BNW GL anchor scene has no complete live drawable backend");
    // The backend may have removed the drawable before throwing. Keep this
    // owner unavailable until an explicit new scene is built and reconciled.
    uncertain_=true;
    backend_->remove_drawable(plot,anchor,x,y,data);
    anchors_.erase(found);
    uncertain_=false;
}

} // namespace civ5::graphics
