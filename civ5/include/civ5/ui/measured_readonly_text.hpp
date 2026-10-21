#pragma once
#include "civ5/ui/world_view.hpp"
#include <cstddef>
#include <functional>

namespace civ5::ui {
struct MeasuredReadonlyTextFrame {
    const void *frame_owner=nullptr;
    std::uint64_t frame_sequence=0,snapshot_frame=0;
    std::int32_t framebuffer_width=0,framebuffer_height=0;
    bool operator==(const MeasuredReadonlyTextFrame &) const = default;
};
struct MeasuredReadonlyTextRegion {
    std::size_t source_paint_index=0;
    WorldViewRect bounds;
    std::string context_id,control_id,tooltip;
};
struct MeasuredReadonlyTextReport {
    MeasuredReadonlyTextFrame frame;
    std::vector<MeasuredReadonlyTextRegion> regions;
};
// Renderer-produced visible glyph/effect quad bounds only. Source paint index
// supplies ordering/identity; this helper neither measures text nor grants
// original native TextButton auto-size/input geometry or gameplay authority.
// The synchronous current validator must check the retained actual UI/client
// owner fences, camera/viewport/DPI and any source leases used by this report.
inline std::optional<MeasuredReadonlyTextRegion> pick_measured_readonly_text(
    const MeasuredReadonlyTextReport &report,const MeasuredReadonlyTextFrame &current_frame,
    std::int32_t x,std::int32_t y,const std::optional<std::string> &focused_context,
    const std::function<bool()> &current) noexcept {
    try {
        if(focused_context || !current || !current_frame.frame_owner ||
            current_frame.framebuffer_width<=0 || current_frame.framebuffer_height<=0 ||
            report.frame!=current_frame || !current()) return std::nullopt;
        const MeasuredReadonlyTextRegion *selected=nullptr;
        for(auto it=report.regions.rbegin();it!=report.regions.rend();++it) {
            const auto &bounds=it->bounds;
            const auto right=std::int64_t(bounds.x)+bounds.width,bottom=std::int64_t(bounds.y)+bounds.height;
            if(bounds.x<0 || bounds.y<0 || bounds.width<=0 || bounds.height<=0 ||
                right>current_frame.framebuffer_width || bottom>current_frame.framebuffer_height ||
                it->context_id.empty() || it->control_id.empty() || it->tooltip.empty() ||
                x<bounds.x || y<bounds.y || std::int64_t(x)>=right || std::int64_t(y)>=bottom) continue;
            if(!selected || it->source_paint_index>selected->source_paint_index) selected=&*it;
        }
        // Copy before the second check. A validator can observe asynchronous
        // invalidation; no borrowed report region escapes this call.
        const auto result=selected ? std::optional<MeasuredReadonlyTextRegion>(*selected) : std::nullopt;
        if(!current()) return std::nullopt;
        return result;
    } catch (...) {return std::nullopt;}
}
} // namespace civ5::ui