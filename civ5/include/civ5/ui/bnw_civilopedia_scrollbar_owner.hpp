#pragma once
#include "civ5/runtime/bnw_stock_lua_client_extent.hpp"
#include "civ5/assets/resource_mounts.hpp"
#include "civ5/content/manifest.hpp"
#include "civ5/ui/civis_civilopedia_geometry.hpp"
#include "civ5/ui/bnw_civilopedia_initial_content.hpp"
namespace civ5::ui {
struct BnwScrollBarLayoutRect {double x=0,y=0;std::int64_t width=0,height=0;};
struct BnwScrollBarLayoutNode {
    std::string element,id,anchor,source_path;
    BnwScrollBarLayoutRect rectangle;
    std::optional<std::size_t> parent_index;
    std::int64_t offset_x=0,offset_y=0;
};
struct BnwScrollBarLayoutFrame {
    runtime::BnwStockLuaClientExtent client;
    std::string xml_sha256,styles_sha256;
    std::uint64_t control_revision=0;
    std::vector<BnwScrollBarLayoutNode> nodes;
    std::shared_ptr<const BnwCivilopediaInitialContentOwner> initial_content;
    const BnwScrollBarLayoutNode *control(std::string_view)const noexcept;
};
// Bounded creation of the exact selected ancestor/layout chain. Source fixtures
// may inspect it, but cannot manufacture the sealed production owner below.
BnwScrollBarLayoutFrame inspect_bnw_scrollbar_layout(
    const content::XmlNode &,const content::XmlNode &,
    const runtime::BnwStockLuaClientExtent &);
BnwScrollBarLayoutFrame inspect_bnw_civilopedia_scroll_layout(
    const content::XmlNode &,const content::XmlNode &,
    const runtime::BnwStockLuaClientExtent &);
// Owns created parsed control rectangles. API size and renderer snapshots read
// the SAME rectangle, not a separate style-width quote or a supplied measurement.
// Covers both scroll panels/bars and parsed up/down controls; content caches are unavailable.
class BnwCivilopediaScrollBarOwner {
 public:
    static std::unique_ptr<BnwCivilopediaScrollBarOwner> create(
        const std::filesystem::path &shared_asset_root,
        const assets::ResourceMountTable &selected_ui_resources,
        const runtime::BnwStockLuaClientExtent &,std::string_view expected_xml_sha256);
    CivisControlSizeResult get_size(const runtime::BnwStockLuaClientExtent &,
        std::string_view control="ScrollBar") const;
    std::unique_ptr<BnwCivilopediaScrollBarOwner> fork_for_frame(
        const runtime::BnwStockLuaClientExtent &)const;
    void set_scrollbar_size(const runtime::BnwStockLuaClientExtent &,
        std::int64_t width,std::int64_t height);
    void set_size(const runtime::BnwStockLuaClientExtent &,std::string_view control,
        std::int64_t width,std::int64_t height);
    void set_offset(const runtime::BnwStockLuaClientExtent &,std::string_view control,
        std::int64_t x,std::int64_t y);
    void create_initial_content(const std::filesystem::path &shared_asset_root,
        const assets::ResourceMountTable &,const runtime::BnwStockLuaClientExtent &);
    const BnwCivilopediaLiteralLine *initial_literal_line(
        const runtime::BnwStockLuaClientExtent &,std::string_view id)const noexcept;
    const BnwScrollBarLayoutFrame &render_snapshot()const noexcept{return frame_;}
 private:
    explicit BnwCivilopediaScrollBarOwner(BnwScrollBarLayoutFrame frame):frame_(std::move(frame)){}
    BnwScrollBarLayoutFrame frame_;
};
}
