#include "civ5/ui/bnw_civilopedia_scroll_subtree.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <charconv>
#include <limits>
#include <stdexcept>

namespace civ5::ui {
namespace {

bool parse_pair(std::string_view source,std::int32_t &x,std::int32_t &y) {
    const auto comma=source.find(',');
    if(comma==std::string_view::npos || source.find(',',comma+1)!=source.npos)
        return false;
    const auto one=source.substr(0,comma),two=source.substr(comma+1);
    if(one.empty()||two.empty())return false;
    const auto first=std::from_chars(one.data(),one.data()+one.size(),x);
    const auto second=std::from_chars(two.data(),two.data()+two.size(),y);
    return first.ec==std::errc{} && second.ec==std::errc{} &&
           first.ptr==one.data()+one.size() &&
           second.ptr==two.data()+two.size();
}

void find_id(const content::XmlNode &node,std::string_view id,
             const content::XmlNode *&found,std::size_t &count) {
    if(node.attribute("ID")==id){found=&node;++count;}
    for(const auto &child:node.children)find_id(child,id,found,count);
}

void add_node(const content::XmlNode &node,std::string_view panel,
              std::string path,bool hidden_ancestor,
              const CivisCivilopediaControlTree &controls,
              const CivilopediaLayoutOwnerKey &owner,
              BnwCivilopediaScrollSubtree &result) {
    BnwCivilopediaScrollNode entry;
    entry.panel_id=panel;
    entry.path=std::move(path);
    entry.id=node.attribute("ID");
    entry.element=node.name;
    entry.anchor=node.attribute("Anchor");
    entry.anchor_side=node.attribute("AnchorSide");
    entry.size_expression=node.attribute("Size");
    entry.offset_expression=node.attribute("Offset");
    entry.style=node.attribute("Style");
    const auto hidden=node.attribute("Hidden");
    if(!hidden.empty()) {
        if(hidden!="0"&&hidden!="1")
            throw std::runtime_error("Selected Civilopedia Hidden token is not 0/1");
        entry.declared_hidden=hidden=="1";
        if(*entry.declared_hidden)++result.explicit_hidden;
    }
    entry.ancestor_declares_hidden=hidden_ancestor;
    std::int32_t x=0,y=0,width=0,height=0;
    if(parse_pair(entry.offset_expression,x,y) &&
       parse_pair(entry.size_expression,width,height) &&
       width>0 && height>0 &&
       std::int64_t(x)+width<=std::numeric_limits<std::int32_t>::max() &&
       std::int64_t(y)+height<=std::numeric_limits<std::int32_t>::max())
        entry.authored_local_rect=BnwCivilopediaDeclaredRect{x,y,width,height};
    else ++result.unresolved_rects;
    if(!entry.id.empty()) {
        const auto geometry=controls.get_geometry(owner,entry.id);
        if(geometry.available)
            entry.frame_control_rect=BnwCivilopediaDeclaredRect{
                geometry.geometry.offset_x,geometry.geometry.offset_y,
                geometry.geometry.width,geometry.geometry.height};
    }
    const auto parent_path=entry.path;
    result.nodes.push_back(std::move(entry));
    const bool descendants_hidden=hidden_ancestor || hidden=="1";
    for(std::size_t i=0;i<node.children.size();++i)
        add_node(node.children[i],panel,
            parent_path+"/"+std::to_string(i)+":"+
                node.children[i].name,
            descendants_hidden,controls,owner,result);
}

void add_content_roots(const content::XmlNode &panel,
                       BnwCivilopediaScrollSubtree &result) {
    const auto panel_id=panel.attribute("ID");
    for(const auto &child:panel.children) {
        if(child.name!="Stack")continue;
        std::int32_t x=0,y=0,width=0,height=0;
        if(child.attribute("ID").empty() || child.attribute("Anchor").empty() ||
           !parse_pair(child.attribute("Offset"),x,y) ||
           !parse_pair(child.attribute("Size"),width,height) ||
           width<=0 || height<=0)
            throw std::runtime_error("Selected Civilopedia content Stack has unresolved authored geometry");
        result.content_roots.push_back({panel_id,child.attribute("ID"),
            child.attribute("Anchor"),{x,y,width,height}});
        auto extent=std::find_if(result.authored_extents.begin(),
            result.authored_extents.end(),[&](const auto &value) {
                return value.panel_id==panel_id &&
                       value.anchor==child.attribute("Anchor");
            });
        if(extent==result.authored_extents.end())
            result.authored_extents.push_back({panel_id,child.attribute("Anchor"),
                x,y,x+width,y+height,1});
        else {
            extent->min_x=std::min(extent->min_x,x);
            extent->min_y=std::min(extent->min_y,y);
            extent->max_x=std::max(extent->max_x,x+width);
            extent->max_y=std::max(extent->max_y,y+height);
            ++extent->content_roots;
        }
    }
}

} // namespace

BnwCivilopediaScrollSubtree inspect_bnw_civilopedia_scroll_subtree(
    const std::filesystem::path &game_root,
    const assets::ResourceMountTable &resources,
    const SelectedBnwCivilopediaLayoutSources &sources,
    const CivisCivilopediaControlTree &controls,
    const CivilopediaMeasuredLayout &measured) {
    BnwCivilopediaScrollSubtree result;
    try {
        if(!sources.verified || controls.completed_operations()!=9 ||
           measured.frame_number!=controls.owner().frame_number ||
           measured.owner_token!=controls.owner().owner_token ||
           measured.payload_sha256!=controls.owner().payload_sha256 ||
           measured.rules_sha256!=controls.owner().rules_sha256 ||
           measured.viewport_width<=0 || measured.viewport_height<=0)
            throw std::runtime_error("Civilopedia scroll subtree frame/source boundary unavailable");
        const auto style=resolve_bnw_civilopedia_vert_slider_style(resources,sources);
        if(!style.selected_source_verified ||
           style.sha256!=measured.resolved_styles_sha256 ||
           style.sha256!=controls.owner().styles_sha256)
            throw std::runtime_error("Civilopedia active style changed before subtree read");
        const auto root=std::filesystem::canonical(game_root);
        const auto xml_path=root/"Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.xml";
        if(runtime::identify_file(xml_path).sha256!=sources.xml_sha256 ||
           sources.xml_sha256!=controls.owner().xml_sha256 ||
           sources.civilopedia_sha256!=controls.owner().civilopedia_sha256)
            throw std::runtime_error("Civilopedia selected XML/Lua source changed");
        const auto xml=content::read_xml(xml_path);
        if(runtime::identify_file(xml_path).sha256!=sources.xml_sha256)
            throw std::runtime_error("Civilopedia XML changed during subtree read");
        const content::XmlNode *main=nullptr,*left=nullptr;
        std::size_t main_count=0,left_count=0;
        find_id(xml,"ScrollPanel",main,main_count);
        find_id(xml,"LeftScrollPanel",left,left_count);
        if(main_count!=1||left_count!=1||!main||!left||
           !inspect_bnw_civilopedia_native_control_tree(xml).verified)
            throw std::runtime_error("Civilopedia scroll panel ownership changed");
        const auto main_geometry=controls.get_geometry(controls.owner(),"ScrollPanel");
        const auto left_geometry=controls.get_geometry(controls.owner(),"LeftScrollPanel");
        if(!main_geometry.available||!left_geometry.available ||
           main_geometry.geometry.height!=measured.viewport_height-126 ||
           left_geometry.geometry.height!=measured.viewport_height-126)
            throw std::runtime_error("Civilopedia measured panel height differs from Lua setters");
        result.xml_sha256=sources.xml_sha256;
        result.styles_sha256=style.sha256;
        result.viewport_width=measured.viewport_width;
        result.viewport_height=measured.viewport_height;
        result.main_panel_width=main_geometry.geometry.width;
        result.main_panel_height=main_geometry.geometry.height;
        result.left_panel_width=left_geometry.geometry.width;
        result.left_panel_height=left_geometry.geometry.height;
        add_node(*main,"ScrollPanel","ScrollPanel",false,
                 controls,controls.owner(),result);
        add_node(*left,"LeftScrollPanel","LeftScrollPanel",false,
                 controls,controls.owner(),result);
        add_content_roots(*main,result);
        add_content_roots(*left,result);
        if(result.content_roots.size()!=4 ||
           result.authored_extents.size()!=3)
            throw std::runtime_error("Civilopedia authored content root set changed");
        result.source_and_owner_verified=true;
        result.blocker="Source-local content extents are measured; native ScrollPanel child layout, visibility defaults and scroll state after CalculateInternalSize require an independent receipt";
    } catch(const std::exception &error) {
        result.blocker=error.what();
    }
    return result;
}

} // namespace civ5::ui
