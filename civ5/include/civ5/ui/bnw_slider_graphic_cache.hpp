#pragma once
#include "civ5/ui/bnw_civilopedia_scrollbar_owner.hpp"
#include "civ5/assets/ui_texture_binding.hpp"
#include <array>
namespace civ5::ui {
struct BnwGraphicPair { float x=0,y=0; };
// Original native order: C, UC, LC, L, R, UL, UR, LL, LR.
struct BnwGraphicNineSlice {
    std::string style,texture_key;
    std::shared_ptr<const assets::UiTextureBinding> texture;
    std::array<BnwGraphicPair,9> authored_sizes,source_starts,source_ends;
    std::array<BnwGraphicPair,9> positions,extents,uv_starts,uv_spans;
    BnwGraphicPair size,hkern,vkern;
    bool dirty_cache_branch_evaluated=false;
    bool native_base_initialization_complete=false;
};
struct BnwSliderGraphicCacheFrame {
    runtime::BnwStockLuaClientExtent client;
    std::uint64_t shell_revision=0,graphic_revision=0;
    std::string styles_sha256;
    BnwGraphicNineSlice track,shuttle;
    // This scoped owner closes Graphic geometry/UV only. It never authorizes
    // native Slider.SetSize, parent bounds/focus, Lua loading or gameplay.
    bool native_slider_setter_complete=false;
};
class BnwSliderGraphicCacheOwner {
 public:
    static std::unique_ptr<BnwSliderGraphicCacheOwner> create(
        const std::filesystem::path &shared_root,const assets::ResourceMountTable &,
        const BnwScrollBarLayoutFrame &actual_shell,std::string_view control="ScrollBar");
    const BnwSliderGraphicCacheFrame &snapshot()const noexcept{return frame_;}
    const BnwGraphicNineSlice *graphic(const runtime::BnwStockLuaClientExtent &,
        std::uint64_t shell_revision,std::string_view embedded)const noexcept;
    // Pure detached Graphic cache update. Caller must still own the enclosing
    // Slider/bounds effects and validate current HWND before noexcept publication.
    std::unique_ptr<BnwSliderGraphicCacheOwner> fork_track_cache(
        const runtime::BnwStockLuaClientExtent &,std::uint64_t shell_revision,
        std::int64_t width,std::int64_t height)const;
    std::unique_ptr<BnwSliderGraphicCacheOwner> fork_track_native_cache(
        const runtime::BnwStockLuaClientExtent &,std::uint64_t shell_revision,
        float width,float height)const;
    std::unique_ptr<BnwSliderGraphicCacheOwner> fork_shuttle_cache(
        const runtime::BnwStockLuaClientExtent &,std::uint64_t shell_revision,
        float width,float height)const;
 private:
    explicit BnwSliderGraphicCacheOwner(BnwSliderGraphicCacheFrame frame):frame_(std::move(frame)){}
    BnwSliderGraphicCacheFrame frame_;
};
}
