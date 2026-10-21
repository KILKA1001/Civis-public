#pragma once
#include "civ5/runtime/bnw_stock_lua_client_extent.hpp"
#include "civ5/assets/resource_mounts.hpp"
#include "civ5/content/manifest.hpp"
#include "civ5/ui/glyphgen.hpp"
namespace civ5::ui {
// Authored descendants are inventory, not a native instantiated control tree.
// Missing Hidden remains unknown, including through an ancestor.
struct BnwCivilopediaContentDeclaration {
    std::string panel_id,path,element,id,anchor,anchor_side;
    std::string size,offset,style,font,text,stack_growth,padding;
    std::vector<std::pair<std::string,std::string>> source_attributes;
    std::optional<bool> explicit_hidden;
    bool explicitly_hidden_ancestor=false;
};
struct BnwCivilopediaLiteralLine {
    std::string id,text,image_path,font,font_style;
    std::int32_t image_width=0,image_height=0;
    // Renderer-neutral, image-local glyph advance box. This does NOT quote the
    // proprietary Label auto-size/padding or resolve its enclosing Stack.
    double line_x=0,line_y=0,baseline=0;
    GlyphGenLine glyph_line;
};
struct BnwCivilopediaInitialContentSnapshot {
    runtime::BnwStockLuaClientExtent client;
    std::uint64_t shell_revision=0;
    std::string xml_sha256,font_sha256,font_key;
    GlyphGenGeometry font_geometry;
    std::vector<std::string> font_style_layers;
    std::vector<BnwCivilopediaContentDeclaration> declared_descendants;
    std::vector<BnwCivilopediaLiteralLine> literal_lines;
    std::size_t explicit_hidden_count=0,unknown_hidden_count=0;
    bool native_internal_size_ready=false;
    std::string blocker;
    const BnwCivilopediaLiteralLine *line(std::string_view)const noexcept;
};
// Pure source inspection; fixtures do not grant native control or epoch readiness.
std::vector<BnwCivilopediaContentDeclaration> inspect_bnw_civilopedia_content_declarations(
    const content::XmlNode &);
std::vector<BnwCivilopediaLiteralLine> project_bnw_civilopedia_initial_literal_lines(
    const content::XmlNode &,const GlyphGenFont &,const GlyphGenGeometry &);
// The sealed production owner binds actual selected source/VFS bytes to the
// same attached client and retained shell revision. Renderer and line getter
// read the same immutable owned lines. It has no CalculateInternalSize success.
class BnwCivilopediaInitialContentOwner {
 public:
    static std::shared_ptr<const BnwCivilopediaInitialContentOwner> create(
        const std::filesystem::path &shared_asset_root,
        const assets::ResourceMountTable &,const runtime::BnwStockLuaClientExtent &,
        std::uint64_t shell_revision,std::string_view expected_xml_sha256);
    const BnwCivilopediaInitialContentSnapshot &render_snapshot()const noexcept{return snapshot_;}
    const BnwCivilopediaLiteralLine *line(const runtime::BnwStockLuaClientExtent &,
        std::uint64_t shell_revision,std::string_view id)const noexcept;
 private:
    explicit BnwCivilopediaInitialContentOwner(BnwCivilopediaInitialContentSnapshot snapshot):
        snapshot_(std::move(snapshot)){}
    BnwCivilopediaInitialContentSnapshot snapshot_;
};
}