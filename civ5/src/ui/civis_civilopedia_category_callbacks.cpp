#include "civ5/ui/civis_civilopedia_category_callbacks.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <charconv>
#include <fstream>
#include <map>
#include <sstream>

namespace civ5::ui {
namespace {
std::string pinned_source(const std::filesystem::path &path,std::string_view sha){
    const auto before=runtime::identify_file(path);
    if(before.sha256!=sha||before.bytes>64ULL*1024*1024)
        throw runtime::ScriptError("Civilopedia callback source identity changed");
    std::ifstream file(path,std::ios::binary);
    if(!file)throw runtime::ScriptError("Civilopedia callback source cannot be read");
    std::string text{std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};
    const auto bytes=std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(text.data()),text.size());
    if(runtime::identify_bytes(bytes).sha256!=sha)
        throw runtime::ScriptError("Civilopedia callback source changed during read");
    return text;
}
void collect_buttons(const content::XmlNode &node,std::map<std::string,std::string> &buttons){
    const auto id=node.attribute("ID");
    if(id.starts_with("CategoryButton")){
        if(node.name!="Button"||!buttons.emplace(std::string(id),node.name).second)
            throw runtime::ScriptError("Civilopedia category button is duplicated or has changed kind");
    }
    for(const auto &child:node.children)collect_buttons(child,buttons);
}
}
CivisCivilopediaCallbackOwner::CivisCivilopediaCallbackOwner(CivilopediaLayoutOwnerKey owner)
    :owner_(std::move(owner)){}
bool CivisCivilopediaCallbackOwner::current(const CivilopediaLayoutOwnerKey &owner)const{
    return valid_&&civis_same_ui_owner(owner_,owner);
}
void CivisCivilopediaCallbackOwner::observe(const CivilopediaLayoutOwnerKey &owner){
    if(!civis_same_ui_owner(owner_,owner))valid_=false;
}
void CivisCivilopediaCallbackOwner::close(){valid_=false;}
std::vector<std::string> bind_civis_civilopedia_category_callbacks(
    runtime::ScriptUiLayoutProvider &layout,const std::filesystem::path &game_root,
    const SelectedBnwCivilopediaLayoutSources &sources,const CivilopediaLayoutOwnerKey &identity,
    const std::shared_ptr<CivisCivilopediaCallbackOwner> &owner,std::int64_t left_click_event){
    if(layout.context_id!="CivilopediaScreen"||!sources.verified||!sources.control_tree.verified||
       sources.civilopedia_sha256!=identity.civilopedia_sha256||sources.xml_sha256!=identity.xml_sha256||
       sources.styles_sha256!=identity.styles_sha256||identity.frame_number==0||identity.owner_token.empty()||
       identity.player_id<0||identity.game_turn<0||identity.payload_sha256.size()!=64||identity.rules_sha256.size()!=64||
       !owner||!owner->current(identity)||!layout.callback_events.empty()||layout.callback_owner_current)
        throw runtime::ScriptError("Civilopedia callback provider has a different or stale selected owner");
    const auto directory=game_root/"Assets/DLC/Expansion2/UI/Civilopedia";
    const auto source=pinned_source(directory/"CivilopediaScreen.lua",sources.civilopedia_sha256);
    const auto xml=pinned_source(directory/"CivilopediaScreen.xml",sources.xml_sha256);
    constexpr std::string_view count_prefix="local numCategories = ";
    const auto start=source.find(count_prefix);
    if(start==std::string::npos||source.find(count_prefix,start+1)!=std::string::npos)
        throw runtime::ScriptError("Civilopedia category count declaration changed");
    const auto first=source.data()+start+count_prefix.size();
    int count=0;const auto parsed=std::from_chars(first,source.data()+source.size(),count);
    if(parsed.ec!=std::errc{}||parsed.ptr==source.data()+source.size()||*parsed.ptr!=';'||count<=0||count>1024||
       source.find("for i = 1, numCategories, 1 do")==std::string::npos||
       source.find("Controls[buttonName]:RegisterCallback( Mouse.eLClick, CivilopediaCategory[i].buttonClicked );")==std::string::npos)
        throw runtime::ScriptError("Civilopedia selected category registration loop changed");
    std::map<std::string,std::string> buttons;
    collect_buttons(content::read_xml_metadata(xml),buttons);
    if(buttons.size()!=static_cast<std::size_t>(count))
        throw runtime::ScriptError("Civilopedia source category count differs from its XML controls");
    std::vector<std::string> ids;
    auto declared=layout.controls;
    for(int index=1;index<=count;++index){
        const auto id="CategoryButton"+std::to_string(index);
        if(!buttons.contains(id))throw runtime::ScriptError("Civilopedia selected category button is missing");
        const auto existing=std::find_if(declared.begin(),declared.end(),[&](const auto &control){return control.id==id;});
        if(existing==declared.end())declared.push_back({id,"Button",false,true});
        else{
            if(existing->kind!="Button")throw runtime::ScriptError("Civilopedia declared category control kind changed");
            existing->has_callbacks=true;
        }
        ids.push_back(id);
    }
    const std::weak_ptr<CivisCivilopediaCallbackOwner> weak_owner=owner;
    layout.controls=std::move(declared);layout.callback_events={left_click_event};
    layout.callback_owner_current=[weak_owner,identity]{
        const auto locked=weak_owner.lock();return locked&&locked->current(identity);
    };
    return ids;
}
std::vector<std::string> bind_civis_civilopedia_top_level_button_callbacks(
    runtime::ScriptUiLayoutProvider &layout,const std::filesystem::path &game_root,
    const SelectedBnwCivilopediaLayoutSources &sources,const CivilopediaLayoutOwnerKey &identity,
    const std::shared_ptr<CivisCivilopediaCallbackOwner> &owner,std::int64_t left_click_event){
    if(layout.context_id!="CivilopediaScreen"||!sources.verified||!owner||!owner->current(identity)||
       sources.civilopedia_sha256!=identity.civilopedia_sha256||sources.xml_sha256!=identity.xml_sha256||
       layout.callback_events!=std::vector<std::int64_t>{left_click_event}||!layout.callback_owner_current||!layout.callback_owner_current())
        throw runtime::ScriptError("Civilopedia top-level callback extension differs from its declared category owner");
    const auto directory=game_root/"Assets/DLC/Expansion2/UI/Civilopedia";
    const auto source=pinned_source(directory/"CivilopediaScreen.lua",sources.civilopedia_sha256);
    const auto xml=content::read_xml_metadata(pinned_source(directory/"CivilopediaScreen.xml",sources.xml_sha256));
    auto declared=layout.controls;std::vector<std::string> ids;std::istringstream lines(source);std::string line;
    while(std::getline(lines,line)){
        constexpr std::string_view prefix="Controls.",operation=":RegisterCallback(";
        if(!line.starts_with(prefix))continue;
        const auto call=line.find(operation,prefix.size());if(call==line.npos)continue;
        const auto id=line.substr(prefix.size(),call-prefix.size());
        const content::XmlNode *control=nullptr;std::size_t count=0;
        const std::function<void(const content::XmlNode &)> find=[&](const auto &node){
            if(node.attribute("ID")==id){control=&node;++count;}
            for(const auto &child:node.children)find(child);
        };find(xml);
        if(count!=1||!control)throw runtime::ScriptError("Civilopedia top-level callback XML identity is missing or duplicated: "+id);
        const auto arguments=std::string_view(line).substr(call+operation.size());
        const auto first=arguments.find_first_not_of(" \t");
        const bool supported=(control->name=="Button"||control->name=="GridButton")&&
            first!=arguments.npos&&arguments.substr(first).starts_with("Mouse.eLClick,");
        const auto existing=std::find_if(declared.begin(),declared.end(),[&](const auto &entry){return entry.id==id;});
        if(existing==declared.end())declared.push_back({id,control->name,false,supported});
        else{
            if(existing->kind!=control->name)throw runtime::ScriptError("Civilopedia top-level callback control kind changed: "+id);
            if(supported)existing->has_callbacks=true;
        }
        if(supported)ids.push_back(id);
    }
    if(ids.empty())throw runtime::ScriptError("Civilopedia selected top-level Button click block is absent");
    layout.controls=std::move(declared);return ids;
}
} // namespace civ5::ui
