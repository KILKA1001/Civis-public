#include "civ5/ui/civis_civilopedia_tooltip_owner.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <fstream>
#include <set>

namespace civ5::ui {
namespace {
std::string pinned(const std::filesystem::path &path,std::string_view hash){
    const auto before=runtime::identify_file(path);
    if(before.sha256!=hash||before.bytes>64ULL*1024*1024)throw runtime::ScriptError("Civilopedia tooltip source pin changed");
    std::ifstream file(path,std::ios::binary);if(!file)throw runtime::ScriptError("Civilopedia tooltip source is missing");
    const std::string text{std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};
    if(runtime::identify_bytes(std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(text.data()),text.size())).sha256!=hash)
        throw runtime::ScriptError("Civilopedia tooltip source changed during read");
    return text;
}
assets::AssetHandle retained(const assets::ResourceMountTable &resources,std::string_view key,const UiTextureAssetSource &source){
    const auto mounted=resources.resolve(assets::AssetKey(std::string(key)));
    const auto *handle=std::get_if<assets::AssetHandle>(&mounted.asset);
    if(!handle||mounted.mount_id!=source.mount_id||handle->candidate().normalized_name!=source.candidate.normalized_name||
       handle->candidate().provenance.source_id!=source.candidate.provenance.source_id||
       runtime::identify_bytes(handle->read_bytes(256ULL*1024*1024)).sha256!=source.sha256)
        throw runtime::ScriptError("Civilopedia tooltip selected texture changed before retention");
    return *handle;
}
}
CivisCivilopediaTooltipOwner::CivisCivilopediaTooltipOwner(CivilopediaLayoutOwnerKey identity,
    std::shared_ptr<CivisCivilopediaCallbackOwner> owner,const assets::ResourceMountTable *resources):
    identity_(std::move(identity)),owner_(std::move(owner)),resources_(resources),thread_(std::this_thread::get_id()){}
std::shared_ptr<CivisCivilopediaTooltipOwner> CivisCivilopediaTooltipOwner::open(
    const std::filesystem::path &root,const SelectedBnwCivilopediaLayoutSources &sources,CivilopediaLayoutOwnerKey identity,
    std::shared_ptr<CivisCivilopediaCallbackOwner> owner,const assets::ResourceMountTable *resources){
    if(!sources.verified||!sources.control_tree.verified||!owner||!owner->current(identity)||
       identity.frame_number==0||identity.owner_token.empty()||identity.player_id<0||identity.game_turn<0||
       identity.payload_sha256.size()!=64||identity.rules_sha256.size()!=64||sources.civilopedia_sha256!=identity.civilopedia_sha256||
       sources.xml_sha256!=identity.xml_sha256||sources.styles_sha256!=identity.styles_sha256)
        throw runtime::ScriptError("Civilopedia tooltip selected owner differs");
    const auto directory=root/"Assets/DLC/Expansion2/UI/Civilopedia";
    const auto source=pinned(directory/"CivilopediaScreen.lua",sources.civilopedia_sha256);
    const auto xml=content::read_xml_metadata(pinned(directory/"CivilopediaScreen.xml",sources.xml_sha256));
    constexpr std::string_view call="TTManager:GetTypeControlTable( \"";
    const auto begin=source.find(call);const auto end=source.find("\", tipControlTable );",begin);
    if(begin==source.npos||end==source.npos||source.find(call,begin+1)!=source.npos)
        throw runtime::ScriptError("Civilopedia selected tooltip registration source changed");
    auto result=std::shared_ptr<CivisCivilopediaTooltipOwner>(new CivisCivilopediaTooltipOwner(std::move(identity),std::move(owner),resources));
    result->type_name_=source.substr(begin+call.size(),end-begin-call.size());
    const content::XmlNode *type=nullptr;std::size_t count=0;
    const std::function<void(const content::XmlNode &)> find=[&](const auto &node){
        if(node.name=="ToolTipType"&&node.attribute("Name")==result->type_name_){type=&node;++count;}
        for(const auto &child:node.children)find(child);
    };find(xml);
    if(count!=1||!type)throw runtime::ScriptError("Civilopedia tooltip XML type is missing or duplicated");
    std::set<std::string> ids;
    const std::function<void(const content::XmlNode &)> collect=[&](const auto &node){
        const auto id=node.attribute("ID");
        if(!id.empty()){
            if(node.name!="Image"||!ids.insert(std::string(id)).second)
                throw runtime::ScriptError("Civilopedia tooltip control kind or identity differs");
            CivisCivilopediaTooltipControlState state;state.id=id;state.kind=node.name;state.texture=node.attribute("Texture");
            const auto hidden=node.attribute("Hidden");if(!hidden.empty()&&hidden!="0"&&hidden!="1")
                throw runtime::ScriptError("Civilopedia tooltip authored Hidden differs");
            state.hidden=hidden=="1";
            if(state.texture.empty()||!node.attribute("TextureOffset").empty())
                throw runtime::ScriptError("Civilopedia tooltip authored image/offset differs");
            state.blocker="Tooltip render receipt pending; authored texture has not been selected";
            result->controls_.push_back(std::move(state));
        }
        for(const auto &child:node.children)collect(child);
    };collect(*type);
    if(result->controls_.empty())throw runtime::ScriptError("Civilopedia tooltip type has no declared controls");
    return result;
}
void CivisCivilopediaTooltipOwner::require_current()const{
    const auto owner=owner_.lock();if(std::this_thread::get_id()!=thread_||closed_||!owner||!owner->current(identity_))
        throw runtime::ScriptError("Civilopedia tooltip owner is closed, stale, expired, or on another thread");
}
CivisCivilopediaTooltipControlState &CivisCivilopediaTooltipOwner::control(std::string_view id){
    require_current();const auto found=std::find_if(controls_.begin(),controls_.end(),[&](const auto &state){return state.id==id;});
    if(found==controls_.end())throw runtime::ScriptError("Civilopedia tooltip control differs from selected XML");return *found;
}
void CivisCivilopediaTooltipOwner::texture(std::string_view id,std::string_view key){
    auto &state=control(id);
    try{
        if(!resources_)throw runtime::ScriptError("Civilopedia tooltip texture ResourceMountTable is absent");
        if(key.empty()||key.find('\0')!=key.npos)throw runtime::ScriptError("Civilopedia tooltip texture key is invalid");
        auto asset=load_ui_texture_asset(*resources_,key);
        auto main=retained(*resources_,key,asset.source);std::optional<assets::AssetHandle> index;
        if(asset.index)index=retained(*resources_,asset.index->candidate.normalized_name,*asset.index);
        state.texture=key;state.selected_texture=std::move(asset);state.retained_source=std::move(main);state.retained_index=std::move(index);
        state.blocker="Tooltip render receipt pending; selected texture payload retained";
    }catch(const std::exception &error){state.blocker=error.what();throw;}
}
void CivisCivilopediaTooltipOwner::offset(std::string_view id,std::int64_t x,std::int64_t y){
    auto &state=control(id);
    if(x<0||y<0||x>INT32_MAX||y>INT32_MAX)throw runtime::ScriptError("Civilopedia tooltip texture offset is outside Civis scalar range");
    if(!state.selected_texture){state.blocker="Civilopedia tooltip texture offset requires a retained selected texture";throw runtime::ScriptError(state.blocker);}
    state.offset_x=x;state.offset_y=y;
}
std::vector<CivisCivilopediaTooltipControlState> CivisCivilopediaTooltipOwner::snapshot()const{
    require_current();
    for(const auto &state:controls_)if(state.selected_texture){
        if(!resources_)throw runtime::ScriptError("Civilopedia tooltip texture resource owner disappeared");
        (void)retained(*resources_,state.texture,state.selected_texture->source);
        if(state.selected_texture->index)(void)retained(*resources_,state.selected_texture->index->candidate.normalized_name,*state.selected_texture->index);
        if(!state.retained_source||runtime::identify_bytes(state.retained_source->read_bytes(256ULL*1024*1024)).sha256!=state.selected_texture->source.sha256||
           (state.selected_texture->index&&(!state.retained_index||runtime::identify_bytes(state.retained_index->read_bytes(256ULL*1024*1024)).sha256!=state.selected_texture->index->sha256)))
            throw runtime::ScriptError("Civilopedia tooltip retained texture payload changed");
    }
    return controls_;
}
void CivisCivilopediaTooltipOwner::close(){
    if(std::this_thread::get_id()!=thread_)throw runtime::ScriptError("Civilopedia tooltip close requires owner thread");
    closed_=true;controls_.clear();
}
runtime::ScriptUiTooltipProvider CivisCivilopediaTooltipOwner::provider(){
    require_current();const std::weak_ptr<CivisCivilopediaTooltipOwner> weak=shared_from_this();
    const auto lock=[weak]{auto owner=weak.lock();if(!owner)throw runtime::ScriptError("Civilopedia tooltip owner expired");owner->require_current();return owner;};
    std::vector<runtime::ScriptUiTooltipControl> controls;for(const auto &state:controls_)controls.push_back({state.id,state.kind});
    return {"CivilopediaScreen",type_name_,std::move(controls),[lock]{++lock()->registrations_;},
        [lock](auto id,auto key){lock()->texture(id,key);},[lock](auto id,auto x,auto y){lock()->offset(id,x,y);},
        [lock](auto id,bool hidden){lock()->control(id).hidden=hidden;}};
}
} // namespace civ5::ui
