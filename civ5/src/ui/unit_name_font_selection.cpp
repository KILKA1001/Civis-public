#include "civ5/ui/unit_name_font_selection.hpp"
#include <stdexcept>
#include <utility>
namespace civ5::ui {
bool unit_name_own_width_exceeds_button(std::int32_t width,std::int32_t button) {
    if(width<0||button<=0) throw std::invalid_argument("Own UnitName width/button is unavailable");
    return std::int64_t{width}>std::int64_t{button}-unit_name_source_width_margin;
}
UnitNameOwnFontSelection select_unit_name_own_glyphgen_font(
    const UnitNameOwnFontSelectionRequest &request, std::string_view text,
    std::string_view style, const std::array<const GlyphGenGeometry *,3> &geometries) {
    if (request.context_id!="UnitPanel" || request.control_id!="UnitName" ||
        request.source_xml_key.empty() || request.player_id<0 || request.selected_unit_id<0 ||
        request.text!=text || request.style!=style || style!="SoftShadow" ||
        text.empty() || text.find_first_of("\r\n[]")!=text.npos || request.button_width<=0)
        throw UnitNameOwnFontSelectionError({UnitNameOwnFontSelectionIssue::command_lineage,{}},
            "Current authored UnitName command lineage is unavailable/different");
    const auto measure=[&](std::size_t index) {
        if(!geometries[index] || geometries[index]->line_height<=0)
            throw UnitNameOwnFontSelectionError({UnitNameOwnFontSelectionIssue::missing_candidate,index},
                "Required UnitName candidate geometry is unavailable");
        GlyphGenLine line;
        try { line=project_glyphgen_line(*geometries[index],text); }
        catch(const std::runtime_error &error) {
            throw UnitNameOwnFontSelectionError({UnitNameOwnFontSelectionIssue::invalid_projection,index},error.what());
        }
        if(line.advance_width<0)
            throw UnitNameOwnFontSelectionError({UnitNameOwnFontSelectionIssue::invalid_projection,index},
                "Own UnitName width is negative");
        return line;
    };
    UnitNameOwnFontSelection out;
    auto first=measure(0);out.measured_indices[0]=0;out.measured_widths[0]=first.advance_width;
    if(unit_name_own_width_exceeds_button(first.advance_width,request.button_width))out.selected_index=1;
    // Preserve the second source read even when no initial font change occurs.
    auto second=measure(out.selected_index);
    out.measured_indices[1]=out.selected_index;out.measured_widths[1]=second.advance_width;
    if(unit_name_own_width_exceeds_button(second.advance_width,request.button_width))out.selected_index=2;
    // Font14 is not a third source GetSizeVal. Its projection supplies the
    // actual selected draw line; it may still exceed the threshold, as Lua
    // performs no further size fallback after SetFontByName14.
    out.draw_line=out.selected_index==2?measure(2):std::move(second);
    return out;
}
} // namespace civ5::ui
