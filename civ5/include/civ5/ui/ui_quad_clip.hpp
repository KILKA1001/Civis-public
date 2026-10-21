#pragma once
#include "civ5/ui/world_view.hpp"
#include <algorithm>
#include <array>
#include <cmath>

namespace civ5::ui {
struct UiClippedQuad {
    WorldViewRect destination;
    std::array<float,4> uv;
};
// Crop a draw, retaining the original layout and texture mapping. The clip is
// independent of the layout rectangle: moving its edge must not move text or
// rescale a scroll row's artwork.
inline std::optional<UiClippedQuad> clip_ui_quad(
    const WorldViewRect &destination, std::array<float,4> uv,
    const WorldViewRect &clip) {
    if(destination.width<=0||destination.height<=0||clip.width<=0||clip.height<=0)
        return std::nullopt;
    for(const auto value:uv)if(!std::isfinite(value))return std::nullopt;
    const auto left=std::max<std::int64_t>(destination.x,clip.x);
    const auto top=std::max<std::int64_t>(destination.y,clip.y);
    const auto right=std::min(std::int64_t{destination.x}+destination.width,
                              std::int64_t{clip.x}+clip.width);
    const auto bottom=std::min(std::int64_t{destination.y}+destination.height,
                               std::int64_t{clip.y}+clip.height);
    if(right<=left||bottom<=top)return std::nullopt;
    const auto fraction=[](std::int64_t value,std::int32_t size) {
        return static_cast<float>(static_cast<double>(value)/size);
    };
    const std::array<float,4> cropped{
        uv[0]+uv[2]*fraction(left-destination.x,destination.width),
        uv[1]+uv[3]*fraction(top-destination.y,destination.height),
        uv[2]*fraction(right-left,destination.width),
        uv[3]*fraction(bottom-top,destination.height)};
    return UiClippedQuad{{static_cast<std::int32_t>(left),static_cast<std::int32_t>(top),
        static_cast<std::int32_t>(right-left),static_cast<std::int32_t>(bottom-top)},cropped};
}
} // namespace civ5::ui
