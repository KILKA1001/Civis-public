#include "civ5/ui/bnw_native_slider_setter.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <cmath>
#include <thread>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
namespace civ5::ui {
namespace {
void need(bool v,const char*s){if(!v)throw std::runtime_error(s);}
bool same(const runtime::BnwStockLuaClientExtent&a,const runtime::BnwStockLuaClientExtent&b){return a.surface_owner==b.surface_owner&&a.revision==b.revision&&a.width==b.width&&a.height==b.height;}
const content::XmlNode&unique(const content::XmlNode&n,std::string_view id){std::vector<const content::XmlNode*>found;auto walk=[&](auto&&self,const content::XmlNode&v)->void{if(v.name=="Instance")return;if(v.attribute("ID")==id)found.push_back(&v);for(const auto&c:v.children)self(self,c);};walk(walk,n);need(found.size()==1,"Native Slider source control ambiguous/missing");return *found.front();}
}
BnwNativeSliderSizePrefix reduce_bnw_native_slider_size_prefix(const BnwNativeSliderFields&prior,float w,float h){
 for(float f:{prior.width,prior.height,prior.shuttle_width,prior.shuttle_height,prior.offset_x,prior.offset_y,w,h})need(std::isfinite(f),"Native Slider size/travel input unknown");
 BnwNativeSliderSizePrefix out;out.fields=prior;auto&v=out.fields;const auto length=prior.vertical?h:w;const auto shuttle=prior.vertical?prior.shuttle_height:prior.shuttle_width;
 // Native CVTTSS2SI, then stores LOW16. Avoid C++ out-of-range signed casts.
 const float remaining=(length-shuttle)-static_cast<float>(static_cast<std::int32_t>(prior.gutter)*2);need(remaining>=-2147483648.0f&&static_cast<double>(remaining)<2147483648.0,"Native Slider travel outside bounded CVTT range");const auto truncated=static_cast<std::int32_t>(remaining);const auto low=static_cast<std::uint16_t>(static_cast<std::uint32_t>(truncated));v.travel=low<0x8000?static_cast<std::int16_t>(low):static_cast<std::int16_t>(static_cast<std::int32_t>(low)-65536);
 // Unlike generic SetSize, only the PRIMARY extent changes. Width request is
 // ignored for a vertical slider; shuttle extents and value remain untouched.
 if(v.vertical)v.height=h;else v.width=w;out.track_size={v.width,v.height};v.edge_x=std::abs(v.offset_x)+v.width;v.edge_y=std::abs(v.offset_y)+v.height;return out;
}
struct BnwNativeSliderJournalOwner::Impl {
 HWND hwnd=nullptr;runtime::BnwStockLuaClientExtentProvider client;
 std::shared_ptr<BnwCivilopediaRightTreeOwner>right;
 std::filesystem::path root;
 BnwNativeSliderJournalFrame frame;std::thread::id thread=std::this_thread::get_id();
 void validate()const{need(std::this_thread::get_id()==thread,"Native Slider journal used on foreign thread");DWORD process=0;need(IsWindow(hwnd)&&GetWindowThreadProcessId(hwnd,&process)==GetCurrentThreadId()&&process==GetCurrentProcessId(),"Native Slider requires SAME actual current-process HWND");RECT r{};need(GetClientRect(hwnd,&r)&&r.right-r.left==frame.client.width&&r.bottom-r.top==frame.client.height,"Native Slider actual window extent changed");const auto c=client.current();need(c&&same(*c,frame.client),"Stale native Slider frame epoch");const auto&tree=right->advance(*c,0);need(tree.xml_sha256==frame.xml_sha256&&tree.styles_sha256==frame.styles_sha256,"Native Slider right-tree source epoch changed");}
};
BnwNativeSliderJournalOwner::BnwNativeSliderJournalOwner(std::unique_ptr<Impl>i):impl_(std::move(i)){}
BnwNativeSliderJournalOwner::~BnwNativeSliderJournalOwner()=default;
std::shared_ptr<BnwNativeSliderJournalOwner>BnwNativeSliderJournalOwner::create(std::uintptr_t hwnd,runtime::BnwStockLuaClientExtentProvider client,const std::filesystem::path&root,const assets::ResourceMountTable&resources,const BnwCivilopediaScrollBarOwner&shell,std::shared_ptr<BnwCivilopediaRightTreeOwner>right,std::string_view control){
 need(hwnd&&client.current&&right&&(control=="ScrollBar"||control=="LeftScrollBar"),"Native Slider actual HWND/shell/right-tree inputs absent");const auto c=client.current();const auto&s=shell.render_snapshot();need(c&&same(*c,s.client)&&s.control_revision==0,"Native Slider journal requires SAME original stock shell");auto i=std::make_unique<Impl>();i->hwnd=reinterpret_cast<HWND>(hwnd);i->client=std::move(client);i->right=std::move(right);i->root=root;auto&f=i->frame;f.client=*c;f.control=control;f.xml_sha256=s.xml_sha256;f.styles_sha256=s.styles_sha256;i->validate();
 const auto xmlpath=root/"Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.xml";need(runtime::identify_file(xmlpath).sha256==f.xml_sha256,"Native Slider source changed");const auto xml=content::read_xml(xmlpath);const auto&bar=unique(xml,control);need(bar.name=="ScrollBar"&&bar.children.empty()&&bar.attribute("Style")=="VertSlider"&&bar.attribute("Anchor")=="R,T"&&bar.attribute("Offset")=="0,0"&&bar.attribute("Length")== (control=="ScrollBar"?"800":"930"),"Native special Slider constructor/parser scope differs");
 // 0x5D0C00/0x4F9470 zeros normal childvector, offsets, gutter, travel;
 // Value begins -1. 0x6FCF90 recursive style parse binds Shuttle manually at
 // embedded+0x12C, then its parent pointer at Slider+0x164. It is not AddChild.
 const auto resolved=resources.resolve(assets::AssetKey("Styles.xml"));const auto*handle=std::get_if<assets::AssetHandle>(&resolved.asset);need(handle,"Native Slider selected stylesheet missing");const auto bytes=handle->read_bytes(64ULL*1024*1024);need(runtime::identify_bytes(bytes).sha256==f.styles_sha256,"Native Slider stylesheet epoch changed");const auto styles=content::read_xml_metadata(std::string(bytes.begin(),bytes.end()));const content::XmlNode*skin=nullptr;for(const auto&n:styles.children)if(n.name=="VertSlider"){need(!skin,"Native Slider duplicate style");skin=&n;}need(skin&&skin->attribute("Size")=="18,18"&&skin->attribute("Vertical")=="1"&&skin->attribute("Gutter").empty()&&skin->attribute("Value").empty()&&skin->children.size()==1&&skin->children[0].name=="Shuttle","Native Slider initial style/childvector dependency differs");
 f.embedded_graphics=std::shared_ptr<const BnwSliderGraphicCacheOwner>(BnwSliderGraphicCacheOwner::create(root,resources,s,control));auto&v=f.constructor_parser_fields;v.width=18;v.height=control=="ScrollBar"?800:930;v.shuttle_width=v.shuttle_height=18;v.vertical=true;v.value=-1;
 // Base parser4387C1 ORs8 for the Slider and parsed Shuttle. The embedded
 // track uses72A7A0 directly, so it never executes the base parser.
 f.slider_flags=0x10008028;f.track_flags=0x10000000;f.shuttle_flags=0x10000008;f.special_panel_registration=true;f.embedded_shuttle_parent_linked=true;f.constructor_parser_complete=true;
 // This actual object is instantiated ONCE, but has not run Panel initialize.
 // 0x7ADD60 ->0x62E250 ->0x58EE20 overwrites shuttle height before Lua; the
 // stylesheet18 is explicitly NOT a current-native ratio/SetSize authority.
 f.pending_source="0x7ADD60 ->0x62E250 ->0x58EE20";f.pending_dependency="Initial ScrollPanel direct-child cached edge_y/hidden/flags and source ratio producer required before current embedded Shuttle.height can authorize Slider.SetSize";need(runtime::identify_file(xmlpath).sha256==f.xml_sha256,"Native Slider source changed during instantiation");i->validate();return std::shared_ptr<BnwNativeSliderJournalOwner>(new BnwNativeSliderJournalOwner(std::move(i)));
}
const BnwNativeSliderJournalFrame&BnwNativeSliderJournalOwner::current()const{impl_->validate();return impl_->frame;}
std::shared_ptr<const BnwCivilopediaRightTreeOwner> BnwNativeSliderJournalOwner::retained_right_tree()const{impl_->validate();return impl_->right;}
BnwGraphicPair BnwNativeSliderJournalOwner::get_native_size()const{const auto&f=current();need(f.current_fields.has_value(),f.pending_dependency.c_str());return {f.current_fields->width,f.current_fields->height};}
CivisControlSizeResult BnwNativeSliderJournalOwner::get_size()const{const auto&f=current();if(!f.current_fields)return {false,0,0,f.pending_dependency};const auto&v=*f.current_fields;if(std::trunc(v.width)!=v.width||std::trunc(v.height)!=v.height)return {false,0,0,"Native fractional size requires exact float Lua/layout seam; no integer truncation success"};need(v.width>=0&&v.height>=0&&v.width<2147483648.0f&&v.height<2147483648.0f,"Native Slider getter outside integer binding scope");return {true,static_cast<std::int32_t>(v.width),static_cast<std::int32_t>(v.height),{}};}
BnwNativeSliderSizePrefix BnwNativeSliderJournalOwner::stage_set_size(float w,float h)const{const auto&f=current();need(f.current_fields.has_value()&&f.initial_shuttle_ratio_known,f.pending_dependency.c_str());return reduce_bnw_native_slider_size_prefix(*f.current_fields,w,h);}
std::shared_ptr<BnwNativeSliderJournalOwner>BnwNativeSliderJournalOwner::fork_initialize_empty_left()const{
 const auto&prior=current();need(prior.control=="LeftScrollBar"&&!prior.current_fields,"Only actual selected empty LeftStack initial source branch is owned");const auto xmlpath=impl_->root/"Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.xml";need(runtime::identify_file(xmlpath).sha256==prior.xml_sha256,"Native LeftStack source changed before initialization");const auto xml=content::read_xml(xmlpath);const auto&panel=unique(xml,"LeftScrollPanel");const auto&stack=unique(xml,"ListOfArticles");need(panel.children.size()==4&& &stack==&panel.children[3]&&panel.attribute("Size")=="294,980"&&panel.attribute("Vertical")=="1"&&panel.attribute("AutoScrollBar").empty()&&stack.name=="Stack"&&stack.children.empty()&&stack.attribute("StackGrowth")=="Bottom"&&stack.attribute("Padding")=="0"&&stack.attribute("Size")=="64,64"&&stack.attribute("Anchor")=="L,T"&&stack.attribute("Offset")=="7,0"&&stack.attribute("Hidden").empty()&&stack.attribute("Style").empty(),"Native LeftStack initial child producer source differs");
 // Native empty Stack CalculateSize resets primary extents, irrespective of
 // authored64x64. Actual panel has only this normal child; special controls
 // are separate registrations. No hidden/text/style child is substituted.
 const auto empty=reduce_bnw_bottom_stack_cache({},0,7,0,7,0);need(empty.width==0&&empty.height==0,"Native empty Stack reducer differs");auto i=std::make_unique<Impl>();i->hwnd=impl_->hwnd;i->client=impl_->client;i->right=impl_->right;i->root=impl_->root;i->frame=prior;auto&f=i->frame;auto v=f.constructor_parser_fields;
 // 0x62E250 starts edge_y at panel.height=980; empty Stack does not increase
 // it. Ratio980/980=1; 0x58EE20 changes CURRENT shuttle930*1 to930; value is
 // source clamped0 and derived travel0. Shader draw is independent/unready.
 f.initial_panel_content_height=980;f.initial_ratio=980.0f/f.initial_panel_content_height;v.shuttle_height=v.height*f.initial_ratio;v.value=0;v.travel=0;v.x=294-v.width;v.y=0;v.edge_x=v.width;v.edge_y=v.height;f.current_fields=v;f.control_bounds=update_bnw_native_base_bounds(v.x,v.y,v.width,v.height,{});f.parent_panel_bounds=update_bnw_native_base_bounds(0,0,294,980,{{7,0,7,0}});f.embedded_graphics=std::shared_ptr<const BnwSliderGraphicCacheOwner>(prior.embedded_graphics->fork_shuttle_cache(f.client,0,18,v.shuttle_height));// Source0x6D3180 clears track anchor bits, centers the vertical shuttle;
 // geometry base0x5761E0 ORs dirty bit1. These are owned concrete flags;
 // no renderer/material readiness flag is inferred from them.
 f.track_flags=(f.track_flags&0xfffffc0f)|1;f.shuttle_flags=(f.shuttle_flags&0xffffff9f)|0x10|1;f.slider_flags|=1;f.initial_shuttle_ratio_known=true;f.size_geometry_dependencies_complete=true;f.pending_source="FontShader draw/full UI registry";f.pending_dependency="Native selected empty-left geometry is initialized; shader draw/full UI/Lua initialization remains unavailable";++f.native_revision;need(runtime::identify_file(xmlpath).sha256==prior.xml_sha256,"Native LeftStack source changed before geometry publication");i->validate();return std::shared_ptr<BnwNativeSliderJournalOwner>(new BnwNativeSliderJournalOwner(std::move(i)));
}
std::shared_ptr<BnwNativeSliderJournalOwner>BnwNativeSliderJournalOwner::fork_set_size(float w,float h)const{
 const auto&prior=current();const auto prefix=stage_set_size(w,h);need(prior.control=="LeftScrollBar"&&prior.size_geometry_dependencies_complete&&(prior.track_flags&1)!=0&&(prior.slider_flags&0x10000000)!=0&&w==18&&h>0&&h<=16777216&&prior.native_revision!=UINT64_MAX,"Native Slider setter requires exact initialized selected left geometry");auto i=std::make_unique<Impl>();i->hwnd=impl_->hwnd;i->client=impl_->client;i->right=impl_->right;i->root=impl_->root;i->frame=prior;auto&f=i->frame;auto v=prefix.fields;
 // Source track+0x3A8 dirty cache, then nominal edge caches, then actual R,T
 // parent-local anchor. Vertical internal parent width is exactly294.
 f.embedded_graphics=std::shared_ptr<const BnwSliderGraphicCacheOwner>(prior.embedded_graphics->fork_track_native_cache(f.client,0,18,h));v.x=294-v.width;v.y=v.offset_y;f.control_bounds=update_bnw_native_base_bounds(v.x,v.y,v.width,v.height,{});
 // Normal slider childvector is empty. Changed slider bounds call parent
 // +0x94, whose normal vector is the independently initialized empty Stack.
 // Slider is a SPECIAL registration, so this union is independent of it;
 // unchanged actual parent bounds stop the recursion before outer Box.
 const auto parent=update_bnw_native_base_bounds(0,0,294,980,{{7,0,7,0}});need(parent==prior.parent_panel_bounds,"Native Slider needs actual enclosing parent bounds producer");f.parent_panel_bounds=parent;f.current_fields=v;f.native_setter_complete=true;++f.native_revision;i->validate();return std::shared_ptr<BnwNativeSliderJournalOwner>(new BnwNativeSliderJournalOwner(std::move(i)));
}
}
