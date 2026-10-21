#pragma once
#include "civ5/ui/world_view.hpp"
#include <algorithm>
#include <limits>
#include <span>
#include <stdexcept>

namespace civ5::ui {
struct AnchoredTextMeasurement {
    std::int32_t origin_x=0, baseline_y=0;
    // Exact source parent-center coordinate, in half pixels. The integer
    // painter's compatibility floor below must not be called native rounding.
    std::int64_t center_y_twice=0;
    std::int32_t compatibility_line_top=0;
    // Bounding union of visible glyph/effect QUADS, not exact alpha ink or
    // proof of the original TextButton auto-size/input rectangle.
    std::optional<WorldViewRect> clipped_glyph_bounds;
};
// Caller provides already projected actual glyph/icon/effect quads relative to
// line origin X and baseline Y. Layout and paint must use these same metrics.
// No text estimation, default font, locale shaping, or original hitbox proof.
inline AnchoredTextMeasurement measure_right_center_text_quads(
    const WorldViewRect &request,std::int32_t advance_width,std::int32_t line_height,
    std::int32_t baseline_from_line_top,std::span<const WorldViewRect> relative_quads,
    const WorldViewRect &clip) {
    if(request.width!=0 || request.height<=0 || advance_width<0 || line_height<=0 ||
        clip.width<=0 || clip.height<=0) throw std::invalid_argument("Unsupported anchored text metrics");
    const auto checked=[](std::int64_t value) {
        if(value<std::numeric_limits<std::int32_t>::min() || value>std::numeric_limits<std::int32_t>::max())
            throw std::overflow_error("Anchored text coordinate exceeds int32");
        return static_cast<std::int32_t>(value);
    };
    const auto floor_half=[](std::int64_t value) {return value/2-(value<0 && value%2!=0 ? 1 : 0);};
    AnchoredTextMeasurement result;
    result.center_y_twice=std::int64_t(request.y)*2+request.height;
    result.compatibility_line_top=checked(floor_half(result.center_y_twice-line_height));
    result.origin_x=checked(std::int64_t(request.x)-advance_width);
    result.baseline_y=checked(std::int64_t(result.compatibility_line_top)+baseline_from_line_top);
    const auto clip_right=std::int64_t(clip.x)+clip.width,clip_bottom=std::int64_t(clip.y)+clip.height;
    checked(clip_right);checked(clip_bottom);
    std::int64_t left=0,top=0,right=0,bottom=0;
    bool visible=false;
    for(const auto &quad : relative_quads) {
        if(quad.width<0 || quad.height<0) throw std::invalid_argument("Negative glyph quad extent");
        if(!quad.width || !quad.height) continue;
        const auto qleft=std::int64_t(result.origin_x)+quad.x,qtop=std::int64_t(result.baseline_y)+quad.y;
        const auto qright=qleft+quad.width,qbottom=qtop+quad.height;
        checked(qleft);checked(qtop);checked(qright);checked(qbottom);
        const auto l=std::max(qleft,std::int64_t(clip.x)),t=std::max(qtop,std::int64_t(clip.y));
        const auto r=std::min(qright,clip_right),b=std::min(qbottom,clip_bottom);
        if(l>=r || t>=b) continue;
        if(!visible) {left=l;top=t;right=r;bottom=b;visible=true;}
        else {left=std::min(left,l);top=std::min(top,t);right=std::max(right,r);bottom=std::max(bottom,b);}
    }
    if(visible) result.clipped_glyph_bounds=WorldViewRect{checked(left),checked(top),checked(right-left),checked(bottom-top)};
    return result;
}
} // namespace civ5::ui