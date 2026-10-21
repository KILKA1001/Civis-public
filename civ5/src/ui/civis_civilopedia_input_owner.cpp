#include "civ5/ui/civis_civilopedia_input_owner.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/content/game_data_xml.hpp"
#include <charconv>
#include <fstream>
#include <thread>

namespace civ5::ui {
namespace {
std::string pinned(const std::filesystem::path &path,const std::string &hash){
    const auto before=runtime::identify_file(path);
    if(before.sha256!=hash||before.bytes>64ULL*1024*1024)throw runtime::ScriptError("Civilopedia input source pin changed");
    std::ifstream file(path,std::ios::binary);
    if(!file)throw runtime::ScriptError("Civilopedia input source is missing");
    const std::string text{std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};
    const auto bytes=std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(text.data()),text.size());
    if(runtime::identify_bytes(bytes).sha256!=hash)throw runtime::ScriptError("Civilopedia input source pin changed during read");
    return text;
}
// Civis counts Unicode scalar values against authored MaxLength. Original
// EditBox UTF-16/codepoint counting equivalence has not been established.
std::size_t scalars(std::string_view text){
    std::size_t count=0;
    for(std::size_t i=0;i<text.size();++count){
        const auto first=static_cast<unsigned char>(text[i++]);std::uint32_t value=first;int extra=0;
        if(first==0)throw runtime::ScriptError("Civilopedia input contains NUL");
        if(first<0x80)continue;
        if(first>=0xc2&&first<=0xdf){value=first&0x1f;extra=1;}
        else if(first>=0xe0&&first<=0xef){value=first&0x0f;extra=2;}
        else if(first>=0xf0&&first<=0xf4){value=first&0x07;extra=3;}
        else throw runtime::ScriptError("Civilopedia input UTF-8 lead byte is invalid");
        for(int j=0;j<extra;++j){
            if(i==text.size())throw runtime::ScriptError("Civilopedia input UTF-8 is truncated");
            const auto next=static_cast<unsigned char>(text[i++]);
            if((next&0xc0)!=0x80)throw runtime::ScriptError("Civilopedia input UTF-8 continuation is invalid");
            value=(value<<6)|(next&0x3f);
        }
        if((extra==1&&value<0x80)||(extra==2&&value<0x800)||(extra==3&&value<0x10000)||
           (value>=0xd800&&value<=0xdfff)||value>0x10ffff)
            throw runtime::ScriptError("Civilopedia input UTF-8 scalar is invalid");
    }
    return count;
}
}
CivisCivilopediaInputOwner::CivisCivilopediaInputOwner(CivilopediaLayoutOwnerKey identity,
    std::shared_ptr<CivisCivilopediaCallbackOwner> owner,std::size_t limit,bool hidden):
    identity_(std::move(identity)),owner_(std::move(owner)),max_length_(limit),hidden_(hidden),thread_(std::this_thread::get_id()){}
std::shared_ptr<CivisCivilopediaInputOwner> CivisCivilopediaInputOwner::open(
    const std::filesystem::path &root,const SelectedBnwCivilopediaLayoutSources &sources,
    CivilopediaLayoutOwnerKey identity,std::shared_ptr<CivisCivilopediaCallbackOwner> owner,bool hidden){
    if(!sources.verified||!sources.control_tree.verified||!owner||!owner->current(identity)||
       identity.frame_number==0||identity.owner_token.empty()||identity.player_id<0||identity.game_turn<0||
       identity.payload_sha256.size()!=64||identity.rules_sha256.size()!=64||
       sources.civilopedia_sha256!=identity.civilopedia_sha256||sources.xml_sha256!=identity.xml_sha256||
       sources.styles_sha256!=identity.styles_sha256)throw runtime::ScriptError("Civilopedia input selected owner differs");
    const auto directory=root/"Assets/DLC/Expansion2/UI/Civilopedia";
    const auto source=pinned(directory/"CivilopediaScreen.lua",sources.civilopedia_sha256);
    const auto xml=content::read_xml_metadata(pinned(directory/"CivilopediaScreen.xml",sources.xml_sha256));
    if(source.find("function OnSearchTextEnter( dummyString, dummyControl )")==source.npos||
       source.find("Controls.SearchEditBox:RegisterCallback( OnSearchTextEnter );")==source.npos||
       source.find("local searchString = Controls.SearchEditBox:GetText();")==source.npos||
       source.find("ContextPtr:SetInputHandler( InputHandler );")==source.npos||
       source.find("ContextPtr:SetShowHideHandler( ShowHideHandler );")==source.npos)
        throw runtime::ScriptError("Civilopedia input callback source contract changed");
    const content::XmlNode *control=nullptr;std::size_t count=0;
    const std::function<void(const content::XmlNode &)> find=[&](const auto &node){
        if(node.attribute("ID")=="SearchEditBox"){control=&node;++count;}
        for(const auto &child:node.children)find(child);
    };find(xml);
    if(count!=1||!control||control->name!="EditBox"||!control->attribute("String").empty())
        throw runtime::ScriptError("Civilopedia selected EditBox XML identity changed");
    const auto value=control->attribute("MaxLength");std::size_t limit=0;
    const auto parsed=std::from_chars(value.data(),value.data()+value.size(),limit);
    if(parsed.ec!=std::errc{}||parsed.ptr!=value.data()+value.size()||limit==0||limit>4096)
        throw runtime::ScriptError("Civilopedia selected EditBox MaxLength is invalid");
    return std::shared_ptr<CivisCivilopediaInputOwner>(new CivisCivilopediaInputOwner(std::move(identity),std::move(owner),limit,hidden));
}
void CivisCivilopediaInputOwner::require_current()const{
    const auto owner=owner_.lock();
    if(std::this_thread::get_id()!=thread_||closed_||!owner||!owner->current(identity_))
        throw runtime::ScriptError("Civilopedia captured input owner is closed, stale, expired, or on another thread");
}
void CivisCivilopediaInputOwner::replace(std::string_view text){
    require_current();if(scalars(text)>max_length_)throw runtime::ScriptError("Civilopedia captured input exceeds authored MaxLength");text_=text;
}
std::string CivisCivilopediaInputOwner::text()const{require_current();return text_;}
bool CivisCivilopediaInputOwner::hidden()const{require_current();return hidden_;}
void CivisCivilopediaInputOwner::close(){
    if(std::this_thread::get_id()!=thread_)throw runtime::ScriptError("Civilopedia input close requires owner thread");
    closed_=true;text_.clear();
}
runtime::ScriptUiTextEntryProvider CivisCivilopediaInputOwner::text_provider(){
    const std::weak_ptr<CivisCivilopediaInputOwner> weak=shared_from_this();
    const auto lock=[weak]{auto owner=weak.lock();if(!owner)throw runtime::ScriptError("Civilopedia captured input owner expired");owner->require_current();return owner;};
    return {"CivilopediaScreen","SearchEditBox",[lock]{return lock()->text();},[lock](auto text){lock()->replace(text);}};
}
runtime::ScriptUiContextProvider CivisCivilopediaInputOwner::context_provider(){
    const std::weak_ptr<CivisCivilopediaInputOwner> weak=shared_from_this();
    const auto lock=[weak]{auto owner=weak.lock();if(!owner)throw runtime::ScriptError("Civilopedia captured input owner expired");owner->require_current();return owner;};
    return {"CivilopediaScreen",[lock]{return lock()->hidden();},[lock](bool hidden){lock()->hidden_=hidden;}};
}
} // namespace civ5::ui
