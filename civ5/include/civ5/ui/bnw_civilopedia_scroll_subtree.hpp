#pragma once

#include "civ5/ui/civis_civilopedia_geometry.hpp"
#include <filesystem>
#include <optional>

namespace civ5::ui {

struct BnwCivilopediaDeclaredRect {
    std::int32_t x=0,y=0,width=0,height=0;
};

struct BnwCivilopediaScrollNode {
    std::string panel_id,path,id,element,anchor,anchor_side;
    std::string size_expression,offset_expression,style;
    std::optional<bool> declared_hidden;
    bool ancestor_declares_hidden=false;
    std::optional<BnwCivilopediaDeclaredRect> authored_local_rect;
    std::optional<BnwCivilopediaDeclaredRect> frame_control_rect;
};

struct BnwCivilopediaContentRoot {
    std::string panel_id,id,anchor;
    BnwCivilopediaDeclaredRect authored_local_rect;
};

struct BnwCivilopediaDeclaredExtent {
    std::string panel_id,anchor;
    std::int32_t min_x=0,min_y=0,max_x=0,max_y=0;
    std::size_t content_roots=0;
};

struct BnwCivilopediaScrollSubtree {
    bool source_and_owner_verified=false;
    std::string xml_sha256,styles_sha256;
    std::int32_t viewport_width=0,viewport_height=0;
    std::int32_t main_panel_width=0,main_panel_height=0;
    std::int32_t left_panel_width=0,left_panel_height=0;
    std::vector<BnwCivilopediaScrollNode> nodes;
    std::vector<BnwCivilopediaContentRoot> content_roots;
    std::vector<BnwCivilopediaDeclaredExtent> authored_extents;
    std::size_t explicit_hidden=0,unresolved_rects=0;
    std::string blocker;
};

// Captures the selected XML descendant tree and frame-owned outer panel sizes.
// Extents are grouped by authored anchor and use only literal numeric XML
// offsets/sizes. They are not native ScrollPanel internal bounds.
BnwCivilopediaScrollSubtree inspect_bnw_civilopedia_scroll_subtree(
    const std::filesystem::path &game_root,
    const assets::ResourceMountTable &selected_ui_resources,
    const SelectedBnwCivilopediaLayoutSources &sources,
    const CivisCivilopediaControlTree &controls,
    const CivilopediaMeasuredLayout &measured);

} // namespace civ5::ui
