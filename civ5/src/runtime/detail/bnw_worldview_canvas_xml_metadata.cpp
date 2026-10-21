#include "civ5/runtime/detail/bnw_worldview_canvas_xml_metadata.hpp"
#include "civ5/runtime/lua_script_host.hpp"
#include <set>
#include <utility>
namespace civ5::runtime::detail {
namespace {
void need(bool v,const char* why){if(!v)throw ScriptError(why);}
constexpr std::uint64_t maximum_bytes=64ull*1024*1024;
thread_local bool capturing=false;
bool same_xml(const content::XmlNode&a,const content::XmlNode&b){
    if(a.name!=b.name||a.text!=b.text||a.attributes!=b.attributes||a.children.size()!=b.children.size())return false;
    for(std::size_t i=0;i<a.children.size();++i)if(!same_xml(a.children[i],b.children[i]))return false;
    return true;
}
const content::XmlNode& at(const content::XmlNode& root,const std::vector<std::size_t>& path){
    const auto* node=&root;for(const auto index:path){need(index<node->children.size(),"Canvas metadata path substituted");node=&node->children[index];}return *node;
}
}
BnwWorldViewCanvasXmlMetadata::BnwWorldViewCanvasXmlMetadata(std::vector<Document> docs,std::vector<Entry> entries):
    documents_(std::move(docs)),ordered_entries_(std::move(entries)){}
std::shared_ptr<const BnwWorldViewCanvasXmlMetadata> BnwWorldViewCanvasXmlMetadata::capture_owned(
    const content::XmlNode& actual_ingame,std::span<const std::uint8_t> actual_ingame_bytes,
    const std::array<Seed,3>& actual_selected_children,const assets::ResourceMountTable& mounts,
    const BnwStockLuaAttemptLease& lease){
    const auto serial=lease.operation_serial();
    if(capturing){lease.invalidate();throw ScriptError("Canvas XML capture reentered");}
    capturing=true;struct End{~End(){capturing=false;}}end;
    try{
        // Copy authentic unpublished source inputs before the first callback.
        const auto root=actual_ingame;const auto root_identity=identify_bytes(actual_ingame_bytes);
        const auto seeds=actual_selected_children;
        need(root.name=="Context"&&root.attribute("Name")=="InGame","Canvas requires authentic retained InGame root");
        need(seeds[0].context=="WorldView"&&seeds[1].context=="ActionInfoPanel"&&seeds[2].context=="NotificationPanel",
            "Canvas selected closed path changed");
        std::vector<Document> docs;docs.reserve(4);std::vector<Entry> entries;
        std::set<std::string> loading,loaded;
        const auto load=[&](std::string context,std::string stem,const std::string& expected){
            assets::AssetKey key(stem+".xml");need(key.safe_relative_path(),"Canvas selected XML key unsafe");
            const auto name=key.normalized_name();need(!loading.contains(name)&&!loaded.contains(name),"Canvas XML cycle or repeated document");
            loading.insert(name);lease.require_operation(serial);
            const auto hit=mounts.resolve(key);lease.require_operation(serial);
            const auto* selected=std::get_if<assets::AssetHandle>(&hit.asset);
            need(selected,"Canvas XML winner missing or ambiguous");
            const auto handle=*selected;lease.require_operation(serial);
            auto staged=handle.read_bytes(maximum_bytes);lease.require_operation(serial);
            const auto identity=identify_bytes(staged);
            need(identity.bytes==handle.candidate().payload_size&&identity.sha256==expected,
                "Canvas XML winner differs from genuine retained selected source");
            auto layout=content::read_xml_metadata(std::string(staged.begin(),staged.end()));
            need(layout.name=="Context","Canvas external XML root is not Context");
            auto bytes=std::make_shared<const assets::Bytes>(std::move(staged));
            docs.push_back(Document{std::move(context),handle,hit.mount_id,std::move(bytes),identity,std::move(layout)});
            loading.erase(name);loaded.insert(name);lease.require_operation(serial);
            return docs.size()-1;
        };
        const auto root_index=load("InGame","Assets/UI/InGame/InGame",root_identity.sha256);
        need(root_index==0&&same_xml(docs[0].layout,root),"Canvas InGame VFS winner/layout is not retained declaration");
        std::array<std::size_t,3> selected_counts{};
        const auto walk=[&](auto&& self,std::size_t document,std::vector<std::size_t> path,bool in_template,std::size_t depth)->void{
            need(depth<=128&&entries.size()<65536,"Canvas XML metadata depth/node budget exhausted");
            // docs has fixed reserved capacity; no borrowed node survives a load.
            const auto node=at(docs[document].layout,path);
            const auto entry=entries.size();entries.push_back(Entry{document,path,in_template,std::nullopt});
            const bool templated=in_template||node.name=="Instance"||node.name=="InstanceData";
            if(node.name=="LuaContext"){
                need(!node.attribute("FileName").empty(),"Canvas LuaContext lacks actual source declaration");
                int next=-1;
                if(docs[document].context=="InGame"&&node.attribute("ID")=="WorldView")next=0;
                else if(docs[document].context=="WorldView"&&node.attribute("ID")=="ActionInfoPanel")next=1;
                else if(docs[document].context=="ActionInfoPanel"){
                    need(node.attribute("ID")=="NotificationPanel","Canvas ActionInfoPanel has unsupported added/conditional child context");next=2;
                }else if(docs[document].context=="NotificationPanel")
                    need(false,"Canvas Notification source has unsupported added child context");
                if(next>=0){
                    need(!templated&&++selected_counts[static_cast<std::size_t>(next)]==1,"Canvas selected declaration ambiguous/template-dependent");
                    if(next==0){
                        need(path.size()==2,"Canvas WorldView is not direct BulkUI child");
                        const auto& parent=docs[document].layout.children[path[0]];
                        need(parent.name=="Container"&&parent.attribute("ID")=="BulkUI","Canvas WorldView parent differs");
                    }else need(path.size()==1,"Canvas selected child is not direct actual Context child");
                    need(node.children.empty(),"Canvas selected LuaContext embedded merge needs genuine parser supplier");
                    for(const auto& attr:node.attributes)
                        need(attr.first=="FileName"||attr.first=="ID"||attr.first=="Hidden",
                            "Canvas selected context conditional/parser attributes unsupported");
                    const auto& seed=seeds[static_cast<std::size_t>(next)];
                    need(node.attribute("FileName")==seed.stem,"Canvas declaration references different selected source");
                    const auto child=load(seed.context,seed.stem,seed.payload_sha256);
                    entries[entry].expanded_document=child;
                    self(self,child,{},false,depth+1);
                }
            }
            for(std::size_t i=0;i<node.children.size();++i){auto child=path;child.push_back(i);self(self,document,std::move(child),templated,depth+1);}
        };
        walk(walk,0,{},false,0);
        need(docs.size()==4&&selected_counts==std::array<std::size_t,3>{1,1,1},"Canvas required source path incomplete");
        lease.require_operation(serial);
        return std::shared_ptr<const BnwWorldViewCanvasXmlMetadata>(new BnwWorldViewCanvasXmlMetadata(std::move(docs),std::move(entries)));
    }catch(...){lease.invalidate();throw;}
}
const BnwWorldViewCanvasXmlMetadata::Document& BnwWorldViewCanvasXmlMetadata::document_owned(std::string_view name)const{
    for(const auto& doc:documents_)if(doc.context==name)return doc;
    throw ScriptError("Canvas requested layout remains unresolved declaration metadata");
}
const content::XmlNode& BnwWorldViewCanvasXmlMetadata::declaration_owned(const Entry& entry)const{
    need(entry.document<documents_.size(),"Canvas metadata source index invalid");return at(documents_[entry.document].layout,entry.child_path);
}
void BnwWorldViewCanvasXmlMetadata::require_all_declarations_expanded_owned()const{
    for(const auto& entry:ordered_entries_)
        if(declaration_owned(entry).name=="LuaContext"&&!entry.expanded_document)
            throw ScriptError("Canvas wider/conditional child needs genuine selected XML/parser/attachment supplier");
}
}
