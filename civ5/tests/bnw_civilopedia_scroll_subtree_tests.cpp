#include "civ5/ui/bnw_civilopedia_scroll_subtree.hpp"
#include <algorithm>
#include <array>
#include <memory>
#include <stdexcept>

namespace {
void check(bool value,const char *reason) {
    if(!value)throw std::runtime_error(reason);
}
}

int main(int argc,char **argv) {
    if(argc!=2)throw std::runtime_error("Expected installed game root");
    const auto root=std::filesystem::path(argv[1]);
    const auto sources=civ5::ui::inspect_bnw_civilopedia_layout_sources(root);
    check(sources.verified,"Selected Civilopedia layout sources unavailable");
    auto base=std::make_shared<civ5::assets::LooseResourceIndex>(root);
    base->add_file(root/"Assets/UI/Styles.xml");
    civ5::assets::ResourceMountTable resources(
        civ5::assets::resource_lookup(base),
        civ5::assets::resource_lookup_all(base));
    const civ5::runtime::SessionCheckpointIdentity identity{
        std::string(64,'a'),std::string(64,'b')};
    for(const auto [width,height]:std::array<std::pair<std::int32_t,std::int32_t>,2>{
            std::pair{1280,960},std::pair{1920,1080}}) {
        civ5::ui::CivilopediaMeasuredLayout measured;
        measured.frame_number=width==1280?31:32;
        measured.owner_token="CivilopediaScreen:subtree-frame-"+
            std::to_string(measured.frame_number);
        measured.player_id=0;
        measured.game_turn=123;
        measured.payload_sha256=identity.payload_sha256;
        measured.rules_sha256=identity.rules_sha256;
        measured.resolved_styles_sha256=sources.styles_sha256;
        measured.viewport_width=width;
        measured.viewport_height=height;
        measured.scrollbar_width=18;
        measured.scroll_panel_width=1002;
        measured.scroll_panel_height=980;
        measured.left_panel_width=294;
        measured.left_panel_height=980;
        const auto plan=civ5::ui::plan_bnw_civilopedia_native_layout(
            sources,identity,0,123,measured.frame_number,measured);
        check(plan.source_and_owner_verified && plan.instructions.size()==11,
              "Selected Civilopedia authored layout plan unavailable");
        std::string blocker;
        auto tree=civ5::ui::CivisCivilopediaControlTree::open(
            resources,sources,plan,measured,blocker);
        if(!tree)throw std::runtime_error(blocker);
        const auto prefix=tree->apply_ordered_plan(plan.owner);
        check(!prefix.applied && prefix.completed_operations==9,
              "Selected Civilopedia setter prefix changed");
        const auto subtree=civ5::ui::inspect_bnw_civilopedia_scroll_subtree(
            root,resources,sources,*tree,measured);
        check(subtree.source_and_owner_verified &&
              subtree.viewport_width==width && subtree.viewport_height==height &&
              subtree.main_panel_width==1002 &&
              subtree.main_panel_height==height-126 &&
              subtree.left_panel_width==294 &&
              subtree.left_panel_height==height-126 &&
              subtree.nodes.size()==303 && subtree.explicit_hidden==1 &&
              subtree.unresolved_rects>100 &&
              subtree.content_roots.size()==4 &&
              subtree.authored_extents.size()==3,
              "Selected Civilopedia XML subtree or measured panel size changed");
        const auto node=[&](std::string_view id) -> const civ5::ui::BnwCivilopediaScrollNode& {
            const auto found=std::find_if(subtree.nodes.begin(),subtree.nodes.end(),
                [&](const auto &candidate){return candidate.id==id;});
            if(found==subtree.nodes.end())throw std::runtime_error("XML node missing");
            return *found;
        };
        check(node("ScrollBar").style=="VertSlider" &&
              node("ScrollBar").anchor=="R,T" &&
              node("ScrollBar").frame_control_rect &&
              node("ScrollBar").frame_control_rect->width==18 &&
              node("ScrollBar").frame_control_rect->height==height-172 &&
              node("ListOfArticles").anchor=="L,T" &&
              node("ListOfArticles").authored_local_rect &&
              node("ListOfArticles").authored_local_rect->x==7 &&
              node("ListOfArticles").authored_local_rect->width==64,
              "Selected Civilopedia style/anchor/content Stack provenance changed");
        const auto extent=[&](std::string_view panel,std::string_view anchor) {
            const auto found=std::find_if(subtree.authored_extents.begin(),
                subtree.authored_extents.end(),[&](const auto &candidate) {
                    return candidate.panel_id==panel && candidate.anchor==anchor;
                });
            if(found==subtree.authored_extents.end())
                throw std::runtime_error("Authored content extent missing");
            return *found;
        };
        const auto right=extent("ScrollPanel","R,T");
        const auto left=extent("ScrollPanel","L,T");
        const auto articles=extent("LeftScrollPanel","L,T");
        check(right.min_x==38 && right.max_x==102 && right.min_y==0 &&
              right.max_y==64 && right.content_roots==1 &&
              left.min_x==304 && left.max_x==384 && left.min_y==0 &&
              left.max_y==324 && left.content_roots==2 &&
              articles.min_x==7 && articles.max_x==71 &&
              articles.min_y==0 && articles.max_y==64 &&
              articles.content_roots==1 &&
              subtree.blocker.find("independent receipt")!=std::string::npos,
              "Authored anchor-local content extents were treated as native internal size");
        ++measured.frame_number;
        check(!civ5::ui::inspect_bnw_civilopedia_scroll_subtree(
                  root,resources,sources,*tree,measured).source_and_owner_verified,
              "Stale Civilopedia frame reached scroll subtree");
    }
}
