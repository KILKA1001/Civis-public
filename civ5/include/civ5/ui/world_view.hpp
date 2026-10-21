#pragma once

#include "civ5/ui/ui_runtime.hpp"
#include "civ5/ui/unit_name_font_selection.hpp"
#include "civ5/ui/bnw_unit_panel_stat_rows_types.hpp"
#include "civ5/ui/ui_audio_events.hpp"
#include "civ5/ui/bnw_city_banner_appearance_types.hpp"
#include <cstdint>
#include <cstddef>
#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <memory>
namespace civ5::ui {
namespace detail {class BnwNormalCaptionFp32RectLoan;}

struct WorldViewPoint { std::int32_t x = 0, y = 0; };
struct WorldViewRect { std::int32_t x = 0, y = 0, width = 0, height = 0; };

// Screen positions are supplied by the live camera projection. They are never
// reconstructed from a fixture save or invented by the UI layer.
struct WorldViewIcon {
    std::int64_t id = 0;
    WorldViewPoint screen;
    std::string label;
    std::string texture;
    WorldViewRect atlas;
    bool visible = true;
};
struct WorldViewActionItem {
    std::int64_t id = 0;
    std::string label;
    std::string tooltip;
    std::string target_screen;
    bool available = false;
};
struct WorldViewNotification {
    std::int64_t id = 0;
    std::string text;
    std::string icon_texture;
    bool requires_decision = false;
    // Set only from the notification owner's authored audio transition.
    // A persistent notification may retain the cue; the bridge deduplicates
    // its owner sequence across frames.
    std::optional<UiAuthoredAudioCue> audio_cue;
};
// Supplied by the selected-unit owner after resolving a concrete UI atlas.
// An absent source leaves the portrait unpainted; the planner never infers one
// from the unit's name or saved type.
struct WorldViewUnitPortrait {
    std::string source_key;
    WorldViewRect crop;
};
// Visibility of the authored UnitPanel frames, supplied by the current
// selected-unit owner. This carries no movement, strength or action value.
struct WorldViewUnitStatBoxes {
    bool xp = false;
    bool movement = false;
    bool strength = false;
    bool ranged = false;
};
struct WorldViewUnit {
    std::int64_t id = 0;
    std::string name;
    std::string movement;
    std::string strength;
    std::vector<WorldViewActionItem> actions;
    std::optional<WorldViewUnitPortrait> portrait = std::nullopt;
    std::optional<WorldViewUnitStatBoxes> stat_boxes = std::nullopt;
    std::optional<BnwUnitPanelStatRowsRead> stat_rows = std::nullopt;
};
struct WorldViewCityBanner : WorldViewIcon {
    std::int32_t population = 0;
    bool owned_by_active_player = false;
    std::optional<BnwCityBannerAppearance> appearance = std::nullopt;
};
// UIPathFinderUpdate nodes, in path order. The owner projects GridToWorld
// through the current camera and supplies screen only when that projection is known.
struct WorldViewPathNode {
    std::int32_t plot_x = -1, plot_y = -1, turn = -1;
    std::optional<WorldViewPoint> screen;
};
enum class WorldViewPlotFog : std::uint8_t { hidden, explored, visible };
struct WorldViewMapVisibility {
    std::int64_t player_id = -1, team_id = -1;
    std::uint64_t width = 0, height = 0;
    // Row-major logical plots. No screen projection or raw wire counts are
    // exposed to the UI. Hidden plots have no revealed art or live visibility.
    std::vector<WorldViewPlotFog> plots;
};
struct WorldViewState {
    std::int64_t player_id = -1;
    std::optional<std::string> end_turn_label;
    bool end_turn_available = false;
    std::string end_turn_unavailable_reason;
    std::optional<std::string> new_turn_text;
    std::optional<std::string> turn_processing_text;
    std::optional<std::string> plot_help;
    std::vector<WorldViewPathNode> path_nodes;
    bool path_indicators_visible = false;
    std::optional<WorldViewUnit> selected_unit;
    std::optional<WorldViewUnit> enemy_unit;
    std::vector<WorldViewNotification> notifications;
    std::vector<WorldViewActionItem> info_corner;
    // The host exposes the authored CityList source only with a matching live
    // player frame; an empty city list remains a valid navigation state.
    bool city_list_launcher = false;
    std::vector<WorldViewCityBanner> city_banners;
    std::vector<WorldViewIcon> unit_flags;
    std::vector<WorldViewIcon> resource_icons;
    std::vector<WorldViewIcon> yield_icons;
    std::optional<WorldViewMapVisibility> map_visibility;
};

enum class WorldViewActionKind {
    end_turn, notification, unit_action, select_unit, open_city, info_corner
};
struct WorldViewAction {
    WorldViewActionKind kind = WorldViewActionKind::end_turn;
    std::int64_t id = 0;
    std::int64_t related_id = 0;
};

// A paint op is a 2D rectangle, an authored DDS crop, or glyph text. The
// renderer consumes these in order over the world framebuffer.
enum class WorldViewPaintKind { rectangle, texture, text };
struct UiFontStylePaint {
    std::string font, style;
    // Layer0 is the foreground. Absent colors retain an explicit unresolved
    // owner dependency; they never select a guessed effect color.
    std::array<std::optional<std::uint32_t>, 3> colors;
    std::optional<std::array<UiFloatColor,2>> float_colors = std::nullopt;
};
// Data only: the actual closed ControlQuote publisher must supply all four
// independent already-composed/clipped vertices, selected raw resource keys,
// native CTAB registers and LOD state. No rect/UV/clock default is an issuer.
struct WorldViewMaskedVertex {
    std::array<float,2> pixel_position, base_uv, mask_uv;
    UiFloatColor color0;
};
struct WorldViewMaskedPaint {
    // Genuine primitive ordering for the dedicated four-vertex triangle strip.
    std::array<WorldViewMaskedVertex,4> vertices;
    // BaseIndex, BaseBlock, MaskIndex, MaskBlock: selected RAW storage keys.
    std::array<std::string,4> resource_keys;
    // c1..c8: BlockSize, IndexScale, IndexTextureSize, BlockTextureSize,
    // MaskBlockSize, MaskIndexScale, MaskIndexTextureSize, MaskBlockTextureSize.
    // Values must be actual native recipe inputs; no DDS-size reconstruction.
    std::array<std::array<float,4>,8> native_registers;
    // Each actual sampler's MIN_LOD, MAX_LOD, LOD_BIAS; never guessed defaults.
    std::array<std::array<float,3>,4> sampler_lod;
};
struct WorldViewPaint {
    WorldViewPaintKind kind = WorldViewPaintKind::rectangle;
    WorldViewRect destination;
    std::string source_key;
    WorldViewRect source;
    std::string text;
    std::string style;
    std::uint32_t rgba = 0xffffffff;
    // Independent inherited viewport; destination retains layout coordinates.
    std::optional<WorldViewRect> clip = std::nullopt;
    std::optional<UiFontStylePaint> font_style = std::nullopt;
    std::optional<UiFloatColor> tint = std::nullopt;
    // Only current authored UnitName commands request own C++ font selection.
    std::optional<UnitNameOwnFontSelectionRequest> unit_name_font_selection = std::nullopt;
    // Optional data carrier only. Root must join a genuine masked publisher
    // and raw-resource tuple route before inserting it into any paint stream.
    std::optional<WorldViewMaskedPaint> masked = std::nullopt;
    // Closed SAME original caption owner; last field preserves positional legacy aggregates.
    std::shared_ptr<const detail::BnwNormalCaptionFp32RectLoan> normal_caption_fp32_rect{};
};
struct WorldViewHit {
    WorldViewRect bounds;
    UiCommand command;
    bool enabled = false;
    std::string unavailable_reason;
};
// Original autosized TextButton hover. Right/center is the authored control
// origin, not an invented hit rectangle. The renderer measures this exact
// painted text with its actual glyph owner before testing pointer containment.
struct WorldViewReadonlyTextTooltip {
    std::string context_id, control_id;
    WorldViewPaint text;
    std::string tooltip;
    std::string font_blocker = {};
    // Exact text paint in the containing plan/composed stream. Compositor
    // remaps by append offset; no text equality lookup supplies identity.
    std::optional<std::size_t> source_paint_index = std::nullopt;
};
// Observational ranges in this producer's ordered paint stream. These do not
// grant source readiness, input authority, or ownership of other contexts.
struct WorldViewOwnedPaintSpan {
    std::string context_id;
    std::size_t begin = 0, end = 0;
};
struct WorldViewPlan {
    std::vector<WorldViewPaint> paints;
    std::vector<WorldViewOwnedPaintSpan> owned_paint_spans;
    std::vector<WorldViewHit> hits;
    std::vector<WorldViewReadonlyTextTooltip> readonly_tooltips;
    // The viewport and minimap presenters consume this same live fog source.
    // Projection into screen pixels remains the renderer owner's task.
    std::optional<WorldViewMapVisibility> map_visibility;
    const WorldViewHit *hit(std::int32_t x, std::int32_t y) const;
};

struct WorldViewWidget {
    std::string context_id, id, element, texture, style, text;
    std::string anchor, anchor_side, size_spec;
    WorldViewPoint offset, texture_offset;
    WorldViewPoint size;
    bool hidden = false;
    std::string font;
    WorldViewPoint padding;
    std::string font_style = {}, color0 = {}, color1 = {};
    std::array<std::optional<std::uint32_t>, 3> font_colors = {};
    std::string color_set = {};
    std::string color2 = {};
    bool color2_declared = false;
    bool color3_declared = false;
};

struct WorldViewUnitStatLayout {
    WorldViewWidget background, parent_box, promotions;
};

// Load every U4 context from the active resource mounts, including the nested
// NotificationPanel under ActionInfoPanel. Missing authored XML is an error.
class WorldViewSkin {
  public:
    static WorldViewSkin from_resources(const assets::ResourceMountTable &resources,
                                        const UiContextRegistry &registry);
    const WorldViewWidget *find(std::string_view context, std::string_view id) const;
    const std::vector<WorldViewWidget> &widgets() const { return widgets_; }
    const std::vector<std::string> &source_keys() const { return source_keys_; }
    const std::optional<WorldViewUnitStatLayout> &unit_stat_layout() const { return unit_stat_layout_; }
    const std::string &generic_notification_icon() const {
        return generic_notification_icon_;
    }
    const std::string &resource_icon_background() const {
        return resource_icon_background_;
    }

  private:
    std::vector<WorldViewWidget> widgets_;
    std::optional<WorldViewUnitStatLayout> unit_stat_layout_;
    std::vector<std::string> source_keys_;
    std::string generic_notification_icon_;
    std::string resource_icon_background_;
};

// The frame must contain a WorldViewState for the active player. With no
// payload the builder returns no data-dependent elements or clickable actions.
WorldViewPlan plan_world_view(const WorldViewSkin &skin, const UiContextRegistry &registry,
                              const UiFrameSnapshot &frame, std::int32_t viewport_width,
                              std::int32_t viewport_height,
                              const UiGameplayBackend *backend = nullptr);

} // namespace civ5::ui

