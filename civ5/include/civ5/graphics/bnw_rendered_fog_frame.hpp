#pragma once
#include "civ5/graphics/bnw_deferred_fog_client.hpp"
#include "civ5/runtime/bnw_game_activation_execution.hpp"
#include "civ5/graphics/bnw_scene_restore_receipt.hpp"
#include "civ5/graphics/gl_fog_of_war_map.hpp"
#include "civ5/ui/bnw_client_frame.hpp"
#include "civ5/materials/fog_of_war_frame.hpp"
#include <bit>
#include <stdexcept>

namespace civ5::graphics {
namespace bnw_rendered_fog_detail {
inline void append(std::vector<std::uint8_t> &bytes, std::uint64_t value) {
    for (unsigned n=0; n<8; ++n) bytes.push_back(static_cast<std::uint8_t>(value>>(n*8)));
}
inline std::string native_digest(const materials::FogOfWarState &state) {
    std::vector<std::uint8_t> bytes;
    append(bytes,static_cast<std::uint64_t>(state.width())); append(bytes,static_cast<std::uint64_t>(state.height()));
    for (const auto offset:state.grid_offset()) append(bytes,static_cast<std::uint64_t>(offset));
    append(bytes,state.dirty());
    append(bytes,std::bit_cast<std::uint32_t>(state.elapsed()));
    append(bytes,std::bit_cast<std::uint32_t>(state.normalized_scale()));
    for (const auto v:state.frame_scale()) append(bytes,std::bit_cast<std::uint32_t>(v));
    for (const auto *values:{&state.visibility(),&state.visibility_override(),&state.culling()}) {
        append(bytes,values->size()); for (const auto v:*values) append(bytes,v);
    }
    return runtime::identify_bytes(bytes).sha256;
}
inline bool camera_matches(const terrain::PerspectiveCamera &a,const terrain::PerspectiveCamera &b) {
    return a.world==b.world && a.field_of_view==b.field_of_view &&
        a.aspect_ratio==b.aspect_ratio && a.near_plane==b.near_plane && a.far_plane==b.far_plane;
}
inline bool views_match(const terrain::CameraFrameState &a,const terrain::CameraFrameState &b) {
    if(a.view_count!=b.view_count || a.view_count>4 || a.projection!=b.projection) return false;
    for(unsigned view=0;view<a.view_count;++view)
        if(a.views[view]!=b.views[view] || a.inverse_views[view]!=b.inverse_views[view] ||
           a.relative_offsets[view]!=b.relative_offsets[view] || a.frusta[view]!=b.frusta[view]) return false;
    return true;
}
inline bool bounds_match(const terrain::ViewBounds &a,const terrain::ViewBounds &b) {
    return a.clipped_low==b.clipped_low && a.clipped_high==b.clipped_high &&
        a.raw_low==b.raw_low && a.raw_high==b.raw_high &&
        a.clipped_tile_low==b.clipped_tile_low && a.clipped_tile_high==b.clipped_tile_high &&
        a.raw_tile_low==b.raw_tile_low && a.raw_tile_high==b.raw_tile_high;
}
inline std::string prepared_digest(const materials::FogOfWarFrame &frame) {
    std::vector<std::uint8_t> bytes;
    append(bytes,frame.passes.size()); append(bytes,frame.draws.size());
    for(const auto &pass:frame.passes) {
        append(bytes,static_cast<std::uint32_t>(pass.kind)); append(bytes,pass.begin); append(bytes,pass.end);
        for(float v:pass.clear_color) append(bytes,std::bit_cast<std::uint32_t>(v));
    }
    for(const auto &draw:frame.draws) {
        append(bytes,draw.geometry); append(bytes,draw.selector); append(bytes,draw.constants.size());
        for(const auto &v:draw.constants)
            for(float component:v) append(bytes,std::bit_cast<std::uint32_t>(component));
    }
    return runtime::identify_bytes(bytes).sha256;
}
} // namespace bnw_rendered_fog_detail

// A borrowed production binding. The receipt can only come from the native
// baseline/event consumer. This checks the actual owner, source and CPU state;
// it does not acknowledge a GPU draw (see the private rendered receipt below).
struct BnwNativeFogFrameBinding {
    const runtime::BnwDeferredFogOwner *owner=nullptr;
    const materials::FogOfWarState *native=nullptr;
    const GlFogOfWarMap *map=nullptr;
    runtime::SessionCheckpointIdentity identity;
    const runtime::BnwGameActivationOwner *activation=nullptr;
    bool *handoff_lost=nullptr;

    bool same_outer_owner() const noexcept {
        if(handoff_lost && *handoff_lost) return false;
        const bool same=!activation || (activation->identity.payload_sha256==identity.payload_sha256 &&
            activation->identity.rules_sha256==identity.rules_sha256 && activation->deferred_fog &&
            &*activation->deferred_fog==owner);
        if(!same && handoff_lost) *handoff_lost=true;
        return same;
    }
    bool matches(const runtime::LiveSessionState &live,const BnwRendererSceneReceipt &scene,
                 const ui::BnwClientFrame &frame) const {
        try {
            if(!same_outer_owner() || !owner || !native || !map || !owner->queue_known || !owner->published ||
                owner->pending || !owner->queued_plots.empty() || native->dirty()) return false;
            const auto &receipt=*owner->published;
            const auto &snapshot=receipt.snapshot(); const auto &binding=snapshot.binding;
            if(identity.payload_sha256!=scene.restored_payload_sha256 ||
                binding.identity.payload_sha256!=identity.payload_sha256 ||
                binding.identity.rules_sha256!=identity.rules_sha256 ||
                binding.active_player!=frame.stamp.identity.player ||
                binding.active_team!=frame.stamp.identity.team || binding.game_turn!=frame.stamp.identity.turn ||
                binding.map_guid!=scene.map_guid || binding.grid.width!=scene.width ||
                binding.grid.height!=scene.height || receipt.grid_offset()!=native->grid_offset() ||
                binding.grid.width!=static_cast<std::uint64_t>(native->width()) ||
                binding.grid.height!=static_cast<std::uint64_t>(native->height()) ||
                snapshot.modes.size()!=frame.visibility.plots.size() ||
                snapshot.modes.size()!=native->visibility().size()) return false;
            const auto current=runtime::capture_bnw_fog_snapshot(live,identity,binding.client_id);
            if(!runtime::same_bnw_fog_client(binding,current.binding) ||
                current.binding.revision!=binding.revision || current.modes!=snapshot.modes) return false;
            for(std::uint64_t plot=0;plot<binding.grid.size();++plot) {
                const auto xy=binding.grid.coordinates(plot); const auto mode=snapshot.modes.at(plot);
                if(mode>2 || native->visibility().at(xy[0]*binding.grid.height+xy[1])!=2-mode ||
                    frame.visibility.plots.at(plot)!=static_cast<ui::WorldViewPlotFog>(mode)) return false;
            }
            return true;
        } catch(const std::exception &) { return false; }
    }
};

class BnwFogFrameRenderer;
// Issued only after the real same-state prepare + GlFogOfWarMap::render call.
// Borrows owners for the synchronous frame/capture lifetime. It acknowledges
// those actual passes and targets; pixel parity remains a separate comparison.
class BnwFogRenderedFrameReceipt {
  public:
    bool matches(const ui::BnwClientFrame &frame,const BnwRendererSceneReceipt &scene,
                 const runtime::LiveSessionState &live) const {
        try {
            if(&live!=live_ || !binding_.matches(live,scene,frame) ||
                frame.stamp.identity!=stamp_.identity || frame.stamp.selection!=stamp_.selection ||
                frame.stamp.viewport!=stamp_.viewport ||
                binding_.owner->published->sequence()!=applied_.sequence() ||
                binding_.owner->published->batch_digest()!=applied_.batch_digest() ||
                binding_.owner->published->snapshot().binding.revision!=applied_.snapshot().binding.revision ||
                bnw_rendered_fog_detail::native_digest(*binding_.native)!=native_digest_ ||
                !bnw_rendered_fog_detail::camera_matches(controller_->camera,camera_) ||
                !bnw_rendered_fog_detail::camera_matches(frame.camera.camera,camera_) ||
                !bnw_rendered_fog_detail::views_match(*views_,rendered_views_) ||
                !bnw_rendered_fog_detail::bounds_match(*bounds_,rendered_bounds_) ||
                binding_.map->render_revision()!=render_revision_ ||
                &binding_.map->mask()!=mask_ || mask_->id()!=mask_id_ ||
                color_->id()!=color_id_ || depth_->id()!=depth_id_ || color_->levels().empty() ||
                color_->levels()[0].width!=width_ || color_->levels()[0].height!=height_ ||
                depth_->levels().empty() || depth_->levels()[0].width!=width_ ||
                depth_->levels()[0].height!=height_ ||
                color_->plan().internal_format!=color_format_ || depth_->plan().internal_format!=depth_format_) return false;
            return matches_bnw_renderer_scene_receipt(scene,frame.world,frame.stamp.identity.turn,
                frame.stamp.identity.player) && matches_bnw_renderer_scene_receipt(scene,live.world,
                live.setup.game_turn,frame.stamp.identity.player);
        } catch(const std::exception &) { return false; }
    }
    bool belongs_to(const BnwNativeFogFrameBinding &binding) const {
        return binding_.owner==binding.owner && binding_.native==binding.native &&
            binding_.map==binding.map && binding_.activation==binding.activation &&
            binding_.handoff_lost==binding.handoff_lost && binding_.identity.payload_sha256==binding.identity.payload_sha256 &&
            binding_.identity.rules_sha256==binding.identity.rules_sha256;
    }
    std::uint64_t sequence() const { return applied_.sequence(); }
    std::uint64_t render_revision() const { return render_revision_; }
    std::string_view logical_revision() const { return applied_.snapshot().binding.revision; }
    std::string_view native_state_digest() const { return native_digest_; }
    std::string_view prepared_frame_digest() const { return prepared_digest_; }
    std::uint32_t mask_texture() const { return mask_id_; }
    std::array<std::uint32_t,2> render_extent() const { return {width_,height_}; }
    // Camera preview renders LAND/FOW into an internal target then actually
    // composites it into this stamped client extent. These are distinct sizes.
    std::array<std::uint32_t,2> presentation_extent() const {
        return {static_cast<std::uint32_t>(stamp_.viewport.framebuffer_width),
                static_cast<std::uint32_t>(stamp_.viewport.framebuffer_height)};
    }
    bool internal_scene_target() const { return render_extent()!=presentation_extent(); }
    const runtime::BnwFogClientReceipt &applied_client() const { return applied_; }
  private:
    friend class BnwFogFrameRenderer;
    BnwFogRenderedFrameReceipt(BnwNativeFogFrameBinding binding,const runtime::LiveSessionState &live,
        const ui::BnwClientFrame &frame,const terrain::CameraControllerState &controller,
        const terrain::CameraFrameState &views,const terrain::ViewBounds &bounds,
        const GlTexture &color,const GlTexture &depth,
        std::string prepared)
        :binding_(std::move(binding)),live_(&live),stamp_(frame.stamp),applied_(*binding_.owner->published),
         controller_(&controller),views_(&views),bounds_(&bounds),camera_(controller.camera),
         rendered_views_(views),rendered_bounds_(bounds),
         mask_(&binding_.map->mask()),color_(&color),depth_(&depth),mask_id_(mask_->id()),
         color_id_(color.id()),depth_id_(depth.id()),color_format_(color.plan().internal_format),
         depth_format_(depth.plan().internal_format),width_(color.levels().at(0).width),
         height_(color.levels().at(0).height),
         native_digest_(bnw_rendered_fog_detail::native_digest(*binding_.native)),
         prepared_digest_(std::move(prepared)),render_revision_(binding_.map->render_revision()) {}
    BnwNativeFogFrameBinding binding_;
    const runtime::LiveSessionState *live_;
    ui::BnwClientFrameStamp stamp_;
    runtime::BnwFogClientReceipt applied_;
    const terrain::CameraControllerState *controller_;
    const terrain::CameraFrameState *views_;
    const terrain::ViewBounds *bounds_;
    terrain::PerspectiveCamera camera_;
    terrain::CameraFrameState rendered_views_;
    terrain::ViewBounds rendered_bounds_;
    const GlTexture *mask_,*color_,*depth_;
    std::uint32_t mask_id_,color_id_,depth_id_;
    GLenum color_format_,depth_format_;
    std::uint32_t width_,height_;
    std::string native_digest_,prepared_digest_;
    std::uint64_t render_revision_;
};

class BnwFogFrameRenderer {
    static void require(bool value,const char *reason) {
        if(!value) throw std::runtime_error(reason);
    }
  public:
    static BnwFogRenderedFrameReceipt render(
        BnwNativeFogFrameBinding binding,const runtime::LiveSessionState &live,
        const BnwRendererSceneReceipt &scene_receipt,const ui::BnwClientFrame &client_frame,
        const terrain::CameraControllerState &controller,const terrain::CameraFrameState &views,
        const terrain::WorldLayout &world,const terrain::HeightOwnerParameters &height,
        const terrain::ViewBounds &bounds,const terrain::OverlayMatrices &prior_overlays,
        std::uint32_t quality,materials::FogOfWarSceneFrame &prepared,
        GlFogOfWarMap &map,const GlTexture &color,const GlTexture &depth,
        const std::function<void(const materials::FogOfWarSceneFrame &)> &prepare_scene={},
        const std::function<void(const GlTexture &)> &draw_scene={}) {
        require(binding.map==&map,"FOW GPU map differs from the bound native owner");
        require(binding.matches(live,scene_receipt,client_frame),
            "FOW native owner is not applied to this live frame");
        require(matches_bnw_renderer_scene_receipt(scene_receipt,live.world,live.setup.game_turn,
                    client_frame.stamp.identity.player) &&
                matches_bnw_renderer_scene_receipt(scene_receipt,client_frame.world,
                    client_frame.stamp.identity.turn,client_frame.stamp.identity.player),
            "FOW frame does not match the prepared immutable scene");
        require(bnw_rendered_fog_detail::camera_matches(controller.camera,client_frame.camera.camera),
            "FOW renderer and latched controller cameras differ");
        require(world.source_grid.width==binding.native->width() &&
                world.source_grid.height==binding.native->height(),"FOW renderer grid differs");
        const auto native_before=bnw_rendered_fog_detail::native_digest(*binding.native);
        const auto camera_before=controller.camera; const auto views_before=views;
        const auto bounds_before=bounds;
        const auto applied_before=*binding.owner->published;
        auto generated=materials::fog_of_war_scene_frame(world,height,*binding.native,views,bounds,
            prior_overlays,quality,true,false,false,terrain::X87Precision::double_precision);
        // LAND constants may be prepared from this exact scene frame. Its actual
        // GL commands remain privately copied, even if the callback changes an output.
        const auto commands=generated.commands;
        prepared=std::move(generated);
        if(prepare_scene) prepare_scene(prepared);
        const auto unchanged=[&] {
            return binding.matches(live,scene_receipt,client_frame) &&
                bnw_rendered_fog_detail::native_digest(*binding.native)==native_before &&
                bnw_rendered_fog_detail::camera_matches(controller.camera,camera_before) &&
                bnw_rendered_fog_detail::views_match(views,views_before) &&
                bnw_rendered_fog_detail::bounds_match(bounds,bounds_before) &&
                binding.owner->published->sequence()==applied_before.sequence() &&
                binding.owner->published->batch_digest()==applied_before.batch_digest() &&
                binding.owner->published->snapshot().binding.revision==applied_before.snapshot().binding.revision;
        };
        require(unchanged(),"FOW state or camera changed during preparation");
        const auto revision_before=binding.map->render_revision();
        map.render(commands,color,depth,draw_scene);
        require(binding.map->render_revision()==revision_before+1 && unchanged(),
            "FOW owner, applied journal, phase or camera changed during the GPU pass");
        BnwFogRenderedFrameReceipt receipt(binding,live,client_frame,controller,views,bounds,color,depth,
            bnw_rendered_fog_detail::prepared_digest(commands));
        require(receipt.matches(client_frame,scene_receipt,live),
            "Rendered FOW receipt became stale before publication");
        return receipt;
    }
};
} // namespace civ5::graphics
