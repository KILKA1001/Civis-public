#pragma once
#include "civ5/ui/glyphgen.hpp"
#include <array>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

namespace civ5::ui {
// Command lineage from SAME current authored WorldView title, not a native
// control/font/metric authority. Button width is the selected skin's geometry.
struct UnitNameOwnFontSelectionRequest {
    std::string source_xml_key, context_id, control_id, text, style;
    std::int64_t player_id = -1, selected_unit_id = -1;
    std::uint64_t snapshot_frame = 0;
    std::optional<std::int32_t> turn;
    std::int32_t button_width = 0;
};
inline constexpr std::array<std::string_view,3> unit_name_source_fonts{
    "TwCenMT24", "TwCenMT20", "TwCenMT14"};
inline constexpr std::int32_t unit_name_source_width_margin = 50;
enum class UnitNameOwnFontSelectionIssue { command_lineage, missing_candidate, invalid_projection };
struct UnitNameOwnFontSelectionRefusal {
    UnitNameOwnFontSelectionIssue issue = UnitNameOwnFontSelectionIssue::command_lineage;
    std::optional<std::size_t> candidate_index;
};
class UnitNameOwnFontSelectionError : public std::runtime_error {
  public:
    UnitNameOwnFontSelectionRefusal refusal;
    UnitNameOwnFontSelectionError(UnitNameOwnFontSelectionRefusal r,const std::string &m)
        : std::runtime_error(m),refusal(r) {}
};
struct UnitNameOwnFontSelection {
    std::size_t selected_index = 0;
    GlyphGenLine draw_line;
    // Exact two source GetSizeVal positions, measured here only by own C++
    // GlyphGen. Position 1 remeasures 24 when the first comparison is false.
    std::array<std::size_t,2> measured_indices{};
    std::array<std::int32_t,2> measured_widths{};
    static constexpr bool native_get_size_val = false;
    static constexpr bool native_label_ready = false;
};
// Shared own-C++ comparison policy only. It does not measure a font or
// attest a native GetSizeVal receiver; both actual geometry consumers keep
// their own measurement/currentness and the two original read positions.
bool unit_name_own_width_exceeds_button(std::int32_t measured_width,std::int32_t button_width);

// No callbacks, no layout/cache/font owner. The caller retains the same
// actual resolver geometries through measurement and draw. Missing geometry,
// invalid UTF8, multiline, missing glyph or overflow refuse before any draw.
UnitNameOwnFontSelection select_unit_name_own_glyphgen_font(
    const UnitNameOwnFontSelectionRequest &, std::string_view current_text,
    std::string_view current_style,
    const std::array<const GlyphGenGeometry *,3> &current_geometries);
} // namespace civ5::ui
