#pragma once
#include "civ5/ui/bnw_civilopedia_scrollbar_owner.hpp"
namespace civ5::ui {
struct BnwEmptyArticleStackFrame {
    runtime::BnwStockLuaClientExtent client;
    std::string xml_sha256,id="ListOfArticles",parent="LeftScrollPanel";
    std::uint64_t shell_revision=0;
    // Native coordinates are local to its parent; screen transform is separate.
    BnwScrollBarLayoutRect local_rectangle;
    float cached_edge_x=0,cached_edge_y=0;
    std::uint32_t source_child_count=0,created_child_count=0;
    bool hidden=false,own_base_init_complete=false,parent_scroll_init_complete=false;
};
// Source fixture projection cannot manufacture the sealed production owner.
BnwEmptyArticleStackFrame inspect_bnw_empty_article_stack_init(const content::XmlNode &);
// Actual created empty leaf Stack has no font/texture/localization/child work.
// Its own init does NOT initialize enclosing ScrollPanel/other UI controls.
class BnwEmptyArticleStackOwner {
 public:
    static std::unique_ptr<BnwEmptyArticleStackOwner> create(
        const std::filesystem::path &shared_root,const BnwScrollBarLayoutFrame &shell);
    const BnwEmptyArticleStackFrame &render_local_snapshot()const noexcept{return frame_;}
    const BnwScrollBarLayoutRect *local_rectangle(
        const runtime::BnwStockLuaClientExtent &,std::uint64_t shell_revision)const noexcept;
 private:
    explicit BnwEmptyArticleStackOwner(BnwEmptyArticleStackFrame f):frame_(std::move(f)){}
    BnwEmptyArticleStackFrame frame_;
};
}
