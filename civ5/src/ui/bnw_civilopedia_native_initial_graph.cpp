#include "civ5/ui/bnw_civilopedia_native_initial_graph.hpp"
#include "civ5/ui/detail/bnw_native_label_xml_prefix_guard.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/bnw_native_ui_color_owner.hpp"
#include <charconv>
#include <atomic>
#include <cmath>
#include <thread>
#include <set>
#include <map>
#include <algorithm>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
namespace civ5::ui {
namespace {
void need(bool v,const char*s){if(!v)throw std::runtime_error(s);}
bool same(const runtime::BnwStockLuaClientExtent&a,const runtime::BnwStockLuaClientExtent&b){return a.surface_owner==b.surface_owner&&a.revision==b.revision&&a.width==b.width&&a.height==b.height;}
std::string attr(const BnwCreatedRightControl&n,std::string_view key){for(const auto&[k,v]:n.source_attributes)if(k==key)return v;return {};}
float number(std::string_view s){float f=0;auto r=std::from_chars(s.data(),s.data()+s.size(),f);need(r.ec==std::errc{}&&r.ptr==s.data()+s.size()&&std::isfinite(f)&&std::abs(f)<32768,"Native direct-control scalar source unsupported");return f;}
std::array<float,2> pair(std::string_view s){auto comma=s.find(',');need(comma!=std::string_view::npos&&s.find(',',comma+1)==std::string_view::npos,"Native direct-control numeric pair unknown");return {number(s.substr(0,comma)),number(s.substr(comma+1))};}
const content::XmlNode& unique(const content::XmlNode&n,std::string_view id){
 std::vector<const content::XmlNode*>found;auto walk=[&](auto&&self,const content::XmlNode&v)->void{if(v.name=="Instance")return;if(v.attribute("ID")==id)found.push_back(&v);for(const auto&c:v.children)self(self,c);};walk(walk,n);need(found.size()==1,"Native special control missing/ambiguous");return *found.front();
}
const content::XmlNode& style(const content::XmlNode&n,std::string_view key){const content::XmlNode*found=nullptr;for(const auto&v:n.children)if(v.name==key){need(!found,"Native special style ambiguous");found=&v;}need(found,"Native special style absent");return *found;}
BnwNativeChildCache base_geometry(float w,float h,float x,float y){BnwNativeChildCache c;c.width=w;c.height=h;c.x=x;c.y=y;c.edge_x=w;c.edge_y=h;c.bounds={x,y,x+w,y+h};return c;}
BnwNativeSpecialButtonGeometry button_prior(const content::XmlNode&node,const content::XmlNode&skin,const assets::ResourceMountTable&resources){
 const bool up=node.name=="UpButton";need(up||node.name=="DownButton","Native selected special button class differs");
 const auto id=up?"UpButton":"DownButton",key=up?"ScrollBarUp":"ScrollBarDown";
 need(node.attribute("ID")==id&&node.attribute("Style")==key&&node.attribute("Anchor")=="R,T"&&node.attribute("Offset")=="0,0"&&node.children.empty(),"Native special button source differs");
 std::set<std::string> seen;for(const auto&[k,v]:node.attributes){(void)v;need(seen.insert(k).second&&(k=="ID"||k=="Style"||k=="Anchor"||k=="Offset"),"Unowned native special button XML attribute");}
 need(skin.name==key&&skin.children.empty()&&skin.attribute("Size")=="18,18"&&!skin.attribute("Texture").empty(),"Native special button style differs");seen.clear();for(const auto&[k,v]:skin.attributes){(void)v;need(seen.insert(k).second&&(k=="Size"||k=="Texture"),"Unowned native special button style attribute");}
 BnwNativeSpecialButtonGeometry out;out.id=id;out.style=key;out.cache.width=out.cache.height=18;out.texture=std::make_shared<const assets::UiTextureBinding>(assets::bind_ui_texture(resources,assets::resource_basename(skin.attribute("Texture"))));need(out.texture->width()&&out.texture->height(),"Native Button requires actual logical texture extent");out.constructor_parser_complete=true;return out;
}
BnwNativeRightDirectPrior construct(std::size_t index,const BnwCreatedRightControl&n,const content::XmlNode&skin){
 need(n.element=="Stack"||n.element=="Image"||n.element=="Grid"||n.element=="Box","Native direct-control class constructor outside proved profile");
 if(!n.style.empty()){
  need(n.element=="Grid"&&n.style=="GridBlackIndent8"&&skin.children.empty(),"Native direct style/factory dependency unknown");
  for(const auto key:{"Style","Hidden","Offset","Size","Anchor","AnchorSide"})need(skin.attribute(key).empty(),"Native direct style alters constructor geometry; source producer required");
 }
 need(attr(n,"AnchorSide").empty(),"Native outside-anchor parser needs its exact source producer");
 BnwNativeRightDirectPrior p;p.geometry_flags=8;p.control_index=index;p.path=n.path;p.element=n.element;p.id=n.id;
 const auto size=pair(n.size);need(size[0]>0&&size[1]>0,"Native selected direct constructor explicit size absent");p.cache.width=size[0];p.cache.height=size[1];
 if(!n.offset.empty()){auto o=pair(n.offset);p.offset_x=o[0];p.offset_y=o[1];}
 need(n.anchor=="L,T"||n.anchor=="R,T","Native direct anchor flags outside selected parser scope");if(n.anchor=="R,T")p.geometry_flags|=0x20;
 const auto hidden=attr(n,"Hidden");need(hidden.empty()||hidden=="0"||hidden=="1","Native direct Hidden parser source unknown");if(hidden=="1")p.geometry_flags|=2;p.cache.hidden=(p.geometry_flags&2)!=0;
 // 0x438170 directly parses Offset+70/74 and Size+30/34. It does
 // NOT invoke SetSize/initialize. Base ctor0x4F9470 initialized x/y,
 // cached edge+20/+24 and bounds+10..1C to zero; subclass constructors
 // in this exact profile leave these geometry fields/flag bits unchanged.
 p.geometry_constructor_parser_complete=true;return p;
}
}
struct BnwCivilopediaNativeInitialGraph::Impl {
 // One occurrence per actual public facade. Validation copies retain this
 // token without keeping the facade alive; fork publication gets a new token.
 std::shared_ptr<std::atomic<bool>> facade_alive=std::make_shared<std::atomic<bool>>(true);
 HWND hwnd=nullptr;std::thread::id thread=std::this_thread::get_id();
 runtime::BnwStockLuaClientExtentProvider client;std::filesystem::path root;
 std::shared_ptr<const BnwCivilopediaRightTreeOwner>right;
 std::shared_ptr<const BnwNativeSliderJournalOwner>slider;
 BnwNativeInitialGraphFrame frame;std::optional<assets::AssetHandle> styles_handle;
 // Shared field components across detached geometry forks; no second graph.
 std::shared_ptr<const content::XmlNode> source_xml;
 std::shared_ptr<detail::BnwNativeRootContextFields> root_context_fields;
 std::map<std::size_t,std::shared_ptr<detail::BnwNativeLabelFieldStorage>> label_fields;
 std::map<std::size_t,BnwNativeLabelXmlReadPrefixFrame> label_read_prefixes;
 std::shared_ptr<detail::BnwNativeLabelXmlPrefixGuard>label_prefix_guard=std::make_shared<detail::BnwNativeLabelXmlPrefixGuard>();
 std::shared_ptr<const BnwNativeUiColorOwner> color_owner;
 std::optional<BnwNativeUiColorQuote> outer_color_quote;
 std::shared_ptr<const std::uint8_t> panel_identity=std::make_shared<const std::uint8_t>(0);
 struct TextureGuard{assets::AssetHandle handle;std::vector<std::uint8_t> bytes;};
 std::vector<TextureGuard> texture_guards;
 void validate()const{
  need(std::this_thread::get_id()==thread,"Native initial graph foreign thread");DWORD process=0;RECT rc{};need(IsWindow(hwnd)&&GetWindowThreadProcessId(hwnd,&process)==GetCurrentThreadId()&&process==GetCurrentProcessId()&&GetClientRect(hwnd,&rc)&&rc.right-rc.left==frame.client.width&&rc.bottom-rc.top==frame.client.height,"Native initial graph requires SAME actual current-process HWND");
  need(label_prefix_guard->healthy(),"Native initial graph XML prefix authorization retired");
  const auto c=client.current();need(c&&same(*c,frame.client),"Stale native initial graph HWND epoch");
  const auto&tree=right->advance(*c,0);const auto&bar=slider->current();need(slider->retained_right_tree().get()==right.get()&&same(bar.client,*c)&&tree.xml_sha256==frame.xml_sha256&&tree.styles_sha256==frame.styles_sha256,"Native initial graph requires SAME retained tree/slider source epoch");
  need(runtime::identify_file(root/"Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.xml").sha256==frame.xml_sha256,"Native initial graph XML changed");
  for(const auto&g:texture_guards)need(g.handle.read_bytes(256ULL*1024*1024)==g.bytes,"Native special texture source changed");
  if(color_owner){need(outer_color_quote.has_value()&&same(color_owner->client_extent(),frame.client),"Native enclosing Color client binding differs");color_owner->validate_quote(*outer_color_quote);}
  if(styles_handle)need(runtime::identify_bytes(styles_handle->read_bytes(64ULL*1024*1024)).sha256==frame.styles_sha256,"Native initial graph stylesheet changed");
  need(label_prefix_guard->healthy(),"Native initial graph XML prefix changed during source callbacks");
 }
};
BnwCivilopediaNativeInitialGraph::BnwCivilopediaNativeInitialGraph(std::unique_ptr<Impl>p):impl_(std::move(p)){
 // Source-only leases may share the old token; they are not public facades.
 impl_->facade_alive=std::make_shared<std::atomic<bool>>(true);
}
BnwCivilopediaNativeInitialGraph::~BnwCivilopediaNativeInitialGraph(){impl_->facade_alive->store(false);}
std::shared_ptr<const BnwCivilopediaNativeInitialGraph>BnwCivilopediaNativeInitialGraph::create(std::uintptr_t hwnd,runtime::BnwStockLuaClientExtentProvider client,const std::filesystem::path&root,const assets::ResourceMountTable&resources,std::shared_ptr<const BnwCivilopediaRightTreeOwner>right,std::shared_ptr<const BnwNativeSliderJournalOwner>slider){
 need(hwnd&&client.current&&right&&slider,"Native initial graph actual retained owners missing");const auto c=client.current();need(c&&slider->current().control=="ScrollBar"&&!slider->current().current_fields&&slider->retained_right_tree().get()==right.get(),"Native initial graph requires SAME original RIGHT constructor journal");
 auto i=std::make_unique<Impl>();i->hwnd=reinterpret_cast<HWND>(hwnd);i->client=std::move(client);i->root=root;i->right=std::move(right);i->slider=std::move(slider);auto&f=i->frame;const auto&t=i->right->advance(*c,0);f.client=*c;f.xml_sha256=t.xml_sha256;f.styles_sha256=t.styles_sha256;i->validate();
 const auto resolved=resources.resolve(assets::AssetKey("Styles.xml"));const auto*h=std::get_if<assets::AssetHandle>(&resolved.asset);need(h,"Native initial graph stylesheet source unavailable");i->styles_handle=*h;const auto bytes=h->read_bytes(64ULL*1024*1024);need(runtime::identify_bytes(bytes).sha256==f.styles_sha256,"Native initial graph stylesheet changed");const auto styles=content::read_xml_metadata(std::string(bytes.begin(),bytes.end()));const content::XmlNode*skin=nullptr;for(const auto&n:styles.children)if(n.name=="GridBlackIndent8"){need(!skin,"Native direct Grid style ambiguous");skin=&n;}need(skin,"Native direct Grid style unavailable");
 i->source_xml=std::make_shared<const content::XmlNode>(content::read_xml(root/"Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.xml"));const auto&xml=*i->source_xml;const auto&panel=unique(xml,"ScrollPanel");need(panel.attribute("Size")=="1002,980"&&panel.attribute("Vertical")=="1"&&panel.attribute("AutoScrollBar")=="1","Native Right panel special geometry source differs");
 for(std::size_t b=0;b<2;++b){const auto key=b==0?"ScrollBarUp":"ScrollBarDown";const auto&node=unique(panel,b==0?"UpButton":"DownButton");auto prior=button_prior(node,style(styles,key),resources);
  const auto capture=[&](const assets::AssetCandidate&identity,const assets::DdsData&dds){const auto bound=resources.resolve(assets::AssetKey(assets::resource_basename(identity.original_name)));const auto*handle=std::get_if<assets::AssetHandle>(&bound.asset);need(handle&&handle->candidate().original_name==identity.original_name&&handle->candidate().provenance.source_id==identity.provenance.source_id,"Native button texture identity changed");auto data=handle->read_bytes(256ULL*1024*1024);need(data==dds.original_bytes(),"Native button texture changed during construction");i->texture_guards.push_back({*handle,std::move(data)});};
  capture(prior.texture->source_identity,prior.texture->source);if(prior.texture->index_identity)capture(*prior.texture->index_identity,*prior.texture->index);f.special_button_priors[b]=std::move(prior);
 }
 // Retain actual track/shuttle resources too. Immutable cache bytes are
 // not permission to keep using an asset handle whose payload changed.
 const auto capture_graphic=[&](const assets::UiTextureBinding&binding){
  const auto capture=[&](const assets::AssetCandidate&identity,const assets::DdsData&dds){const auto bound=resources.resolve(assets::AssetKey(assets::resource_basename(identity.original_name)));const auto*handle=std::get_if<assets::AssetHandle>(&bound.asset);need(handle&&handle->candidate().original_name==identity.original_name&&handle->candidate().provenance.source_id==identity.provenance.source_id,"Native embedded Graphic texture identity changed");auto data=handle->read_bytes(256ULL*1024*1024);need(data==dds.original_bytes(),"Native embedded Graphic texture changed during construction");i->texture_guards.push_back({*handle,std::move(data)});};
  capture(binding.source_identity,binding.source);if(binding.index_identity)capture(*binding.index_identity,*binding.index);
 };
 const auto&embedded=i->slider->current().embedded_graphics->snapshot();need(embedded.track.texture&&embedded.shuttle.texture,"Native actual embedded Graphic textures missing");capture_graphic(*embedded.track.texture);capture_graphic(*embedded.shuttle.texture);
 constexpr std::array<std::size_t,3>counts{15,2,59};
 for(std::size_t s=0;s<3;++s){const auto index=t.stacks[s];need(index<t.controls.size()&&t.controls[index].element=="Stack"&&t.controls[index].children.size()==counts[s],"Native direct child registration changed");f.stacks[s]=construct(index,t.controls[index],*skin);for(const auto child:t.controls[index].children){need(child<t.controls.size()&&t.controls[child].parent==index,"Native direct parent identity differs");auto prior=construct(child,t.controls[child],*skin);/* StackGrowth Bottom=0: native AddChild685AA0 clears Y-center/bottom anchors unless outsideY; these selected children have no outsideY. It does not update position/edges/bounds. */prior.geometry_flags&=0xfffffe7f;f.direct_children[s].push_back(std::move(prior));}}
 f.direct_geometry_constructor_parser_complete=true;
 // Panel0x514FD0 +84/+88 start ZERO. Init7ADD60 initializes special
 // buttons/Slider and registers callbacks BEFORE base5761E0 normal traversal.
 // Base anchor599810 also fans out to those specials. Never skip them or
 // substitute final1002 internal width for this initial native zero field.
 f.pending_source="0x7ADD60 special Up/Down/Slider.initialize ->0x599810 special anchor fanout";
 f.pending_dependency="Owned pre-ratio special control initialization/callback graph required before normal RightStack base init; then Label710270 context/TextUpdate6A2EA0 and ordered parent/cache callbacks";
 i->validate();return std::shared_ptr<const BnwCivilopediaNativeInitialGraph>(new BnwCivilopediaNativeInitialGraph(std::move(i)));
}
const BnwNativeInitialGraphFrame&BnwCivilopediaNativeInitialGraph::current()const{
 need(std::this_thread::get_id()==impl_->thread,"Native graph current foreign thread");
 auto lease=std::make_unique<Impl>(*impl_);const auto facade=lease->facade_alive;
 auto operation=lease->label_prefix_guard->enter();lease->validate();operation.check();
 // This API returns a borrowed frame. A surviving copied lease cannot make
 // a reference to a destroyed facade's frame valid: reject before touching
 // impl_ again. The normal caller must retain its facade while using the ref.
 need(facade->load(),"Native graph current facade retired during callback");
 return impl_->frame;
}
}

namespace civ5::ui {
std::shared_ptr<const BnwCivilopediaNativeInitialGraph>BnwCivilopediaNativeInitialGraph::fork_initialize_pre_ratio_special_geometry()const{
 need(std::this_thread::get_id()==impl_->thread,"Native graph fork foreign thread");
 auto i=std::make_unique<Impl>(*impl_);
 auto operation=i->label_prefix_guard->enter();i->validate();operation.check();
 const auto&prior=i->frame;need(!prior.pre_ratio_special_geometry&&prior.direct_geometry_constructor_parser_complete&&prior.geometry_revision==0&&prior.panel_internal_width==0&&prior.panel_internal_height==0,"Native special geometry transition cannot replay or use post-ratio extents");
 BnwNativePreRatioSpecialGeometry g;g.buttons=prior.special_button_priors;
 // Generic Button5AAA20 ->6F6C10 ->helper4535B0/5C7B50,
 // Button base71A000 ->Control5761E0 ->state0(6B3610). Both
 // selected styles have an actual texture, no nested Label/GridData,
 // TextureOffset/Increment/Size override, AutoSize or NoStateChange.
 for(std::size_t b=0;b<2;++b){auto&v=g.buttons[b];need(v.constructor_parser_complete&&v.texture&&v.flags==0x8028&&v.state==0&&v.cache.width==18&&v.cache.height==18,"Native special Button constructor prior differs");v.state_increment={0,18};v.texture_size=v.draw_size={18,18};v.flags|=1;v.cache=base_geometry(18,18,-18,0);v.uv_start={0,0};v.uv_span={18.0f/static_cast<float>(v.texture->width()),18.0f/static_cast<float>(v.texture->height())};v.geometry_prefix_complete=true;g.ordered_operations.push_back(v.id+".geometry0x6F6C10/0x71A000");g.callbacks[b]={b==0?BnwNativePanelSpecialCallback::scroll_up:BnwNativePanelSpecialCallback::scroll_down,b==0?0x6463a0U:0x6e9af0U,i->panel_identity,false};g.ordered_operations.push_back(v.id+".bind0x7ADD60");}
 const auto&bar=i->slider->current();need(bar.constructor_parser_complete&&bar.special_panel_registration&&bar.embedded_shuttle_parent_linked&&bar.normal_children==0&&!bar.current_fields&&bar.embedded_graphics&&bar.slider_flags==0x10008028&&bar.track_flags==0x10000000&&bar.shuttle_flags==0x10000008,"Native special Slider actual constructor graph unavailable");
 g.slider=bar.constructor_parser_fields;need(g.slider.vertical&&g.slider.width==18&&g.slider.height==800&&g.slider.shuttle_width==18&&g.slider.shuttle_height==18&&g.slider.gutter==0&&g.slider.value==-1,"Native Right pre-ratio Slider source fields differ");
 const auto&graphics=bar.embedded_graphics->snapshot();need(graphics.track.size.x==18&&graphics.track.size.y==800&&graphics.shuttle.size.x==18&&graphics.shuttle.size.y==18&&graphics.track.texture&&graphics.shuttle.texture&&graphics.track.dirty_cache_branch_evaluated&&graphics.shuttle.dirty_cache_branch_evaluated,"Native pre-ratio embedded Graphic geometry/UV dependencies absent");
 // Embedded track has no parent. Shuttle is manually parent-linked to
 // Slider's still-nominal18x800; the vertical center X is exactly0.
 // 6D3180 clears track anchors; base init dirties both graphics. Source
 // SetValue(-1) clamps0; SetOffset(0,0) observes existing identical offsets
 // and does NOT call anchor/bounds/fanout in that branch.
 g.embedded_graphics=bar.embedded_graphics;g.track_flags=(bar.track_flags&0xfffffc0f)|1;g.shuttle_flags=(bar.shuttle_flags&0xffffff9f)|0x10|1;g.slider_flags=bar.slider_flags|1;
 g.track_cache=base_geometry(18,800,0,0);g.shuttle_cache=base_geometry(18,18,0,0);const auto size=reduce_bnw_native_slider_size_prefix(g.slider,18,800);g.slider.travel=size.fields.travel;need(g.slider.offset_x==0&&g.slider.offset_y==0&&g.slider.travel==782,"Native source same-offset SetValue branch unavailable");g.slider.value=0;g.set_value_same_offset_branch=true;g.slider.x=-18;g.slider.y=0;g.slider.edge_x=18;g.slider.edge_y=800;g.slider_cache=base_geometry(18,800,-18,0);
 // Source Button current-state0 then ShuttleStateUV456090 keeps authored
 // texture offsets exactly unchanged; no shader or user event is invoked.
 g.slider_button_state=0;g.ordered_operations.push_back("ScrollBar.geometry0x6D3180/0x71A000");g.callbacks[2]={BnwNativePanelSpecialCallback::slider_value_changed,0x478630,i->panel_identity,false};g.ordered_operations.push_back("ScrollBar.bind0x7ADD60");g.geometry_prefix_complete=true;
 i->frame.pre_ratio_special_geometry=std::move(g);++i->frame.geometry_revision;i->frame.pending_source="0x5761E0 Panel base ->0x599810 special anchor/bounds fanout ->normal child traversal";i->frame.pending_dependency="Source enclosing Panel/parent cached bounds, RightStack/Label TextUpdate callbacks and full event/material owners required; pre-ratio special geometry is not full initialize";
 i->validate();operation.check();return std::shared_ptr<const BnwCivilopediaNativeInitialGraph>(new BnwCivilopediaNativeInitialGraph(std::move(i)));
}

std::shared_ptr<const BnwCivilopediaNativeInitialGraph>BnwCivilopediaNativeInitialGraph::fork_create_enclosing_box_constructors()const{
 need(std::this_thread::get_id()==impl_->thread,"Native graph fork foreign thread");
 auto i=std::make_unique<Impl>(*impl_);
 auto operation=i->label_prefix_guard->enter();i->validate();operation.check();
 const auto&prior=i->frame;need(!prior.enclosing_constructors&&prior.geometry_revision==0&&!prior.pre_ratio_special_geometry,"Native enclosing Box constructors require pre-initialize source stage and cannot replay");
 need(i->source_xml!=nullptr,"Native enclosing retained source XML unavailable");const auto&xml=*i->source_xml;
 std::vector<const content::XmlNode*>route;auto find=[&](auto&&self,const content::XmlNode&node)->bool{if(node.name=="Instance")return false;route.push_back(&node);if(node.attribute("ID")=="ScrollPanel")return true;for(const auto&child:node.children)if(self(self,child))return true;route.pop_back();return false;};
 need(find(find,xml)&&route.size()==4&&route[0]->name=="Context"&&route[1]->name=="Box"&&route[2]->name=="Box"&&route[3]->name=="ScrollPanel","Native actual enclosing Box factory route changed");
 const auto&outer=*route[1];const auto&inner=*route[2];const auto&panel=*route[3];
 need(outer.attribute("Anchor")=="C,C"&&outer.attribute("Size")=="1024,Full"&&outer.attribute("Color")=="0,0,0,0"&&outer.attributes.size()==3&&outer.children.size()==5,"Native outer Box source differs");
 need(inner.attribute("Anchor")=="C,T"&&inner.attribute("Size")=="1000,980"&&inner.attribute("Offset")=="0,126"&&inner.attribute("Color")=="255,255,255,0"&&inner.attributes.size()==4&&inner.children.size()==3,"Native immediate Box source differs");
 need(panel.attribute("ID")=="ScrollPanel"&&panel.attribute("Size")=="1002,980"&&panel.attribute("Anchor")=="C,T","Native SAME panel constructor source differs");
 const std::array<std::vector<std::string>,2>expected{{{"Image","Label","Grid","Image","Box"},{"AlphaAnim","ScrollPanel:ScrollPanel","ScrollPanel:LeftScrollPanel"}}};
 BnwNativeEnclosingConstructorGraph enclosing;enclosing.panel_identity=i->panel_identity;
 for(std::size_t n=0;n<2;++n){const auto&node=*route[n+1];auto&box=enclosing.boxes[n];box.path=n==0?"Context/Box[outer1024]":"Context/Box[outer1024]/Box[inner1000]";box.source_attributes=node.attributes;box.authored_nominal_width=n==0?1024.0f:1000.0f;if(n==1){box.authored_nominal_height=980;box.authored_offset={0,126};}else box.authored_full_height=true;box.identity=std::make_shared<const std::uint8_t>(0);
  for(const auto&child:node.children){need(child.name!="Instance","Unowned enclosing Box Instance factory");std::string item=child.name;const auto id=child.attribute("ID");if(!id.empty())item+=":"+std::string(id);box.source_normal_child_order.push_back(std::move(item));}
  need(box.source_normal_child_order==expected[n],"Native enclosing source normal-child order differs");
  // Actual Box58E740 calls4F9470. Position/edges/bounds/nominal dimensions,
  // offsets and flags are zero; branch alpha is1. The subtype stores only
  // four color1 scalars. No Read/style/color/child initialization is inferred.
  box.constructor_complete=true;
 }
 need(&outer.children[4]==&inner&& &inner.children[1]==&panel,"Native enclosing source registration ordinal differs");
 enclosing.bindings[0]={enclosing.boxes[0].identity,enclosing.boxes[1].identity,4,0x44bf40,false};
 enclosing.bindings[1]={enclosing.boxes[1].identity,i->panel_identity,1,0x44bf40,false};
 enclosing.constructor_instances_created=true;enclosing.pending_source="Box.Read0x6A3690 ->Color0x580780 ->providers0x55E2B0/0x5307B0; baseRead0x438170 follows";
 enclosing.pending_dependency="Actual selected Color-provider registry required before numeric fallback; outer Full also needs GUIglobal+0x648 and dynamic callback0x668750. Normal AddChild awaits preceding source sibling factories; parent bounds/clip remain unavailable";
 i->frame.enclosing_constructors=std::move(enclosing);i->validate();operation.check();return std::shared_ptr<const BnwCivilopediaNativeInitialGraph>(new BnwCivilopediaNativeInitialGraph(std::move(i)));
}
std::shared_ptr<const BnwCivilopediaNativeInitialGraph>BnwCivilopediaNativeInitialGraph::fork_read_outer_box_color_prefix(std::shared_ptr<const BnwNativeUiColorOwner>color)const{
 need(std::this_thread::get_id()==impl_->thread,"Native graph fork foreign thread");
 auto i=std::make_unique<Impl>(*impl_);
 auto operation=i->label_prefix_guard->enter();i->validate();operation.check();
 const auto&prior=i->frame;need(prior.enclosing_constructors&&prior.geometry_revision==0&&!prior.pre_ratio_special_geometry&&!prior.enclosing_constructors->boxes[0].subtype_color_read_complete&&color&&same(color->client_extent(),prior.client),"Native outer Box Color needs SAME actual client and original constructor source stage");
 i->color_owner=std::move(color);
 auto&enclosing=*i->frame.enclosing_constructors;auto&outer=enclosing.boxes[0];std::string declaration;for(const auto&[key,value]:outer.source_attributes)if(key=="Color")declaration=value;need(declaration=="0,0,0,0"&&outer.constructor_complete&&outer.flags==0,"Native source outer Box Color declaration/state changed");
 i->outer_color_quote=i->color_owner->quote(declaration);outer.color=i->outer_color_quote->rgba();outer.subtype_color_read_complete=true;
 // Subtype Read Color precedes base Size(1024,Full). Do not jump to inner
 // Box Read: its factory waits for outer Full/dynamic and preceding siblings.
 enclosing.pending_source="Box baseRead0x438170 ->Size0x76DC50 GUIglobal+0x648 ->dynamic registration0x668750/0x86AC20";enclosing.pending_dependency="Actual GUI global full-height producer/dynamic callback registry, then ordered outer sibling factories. Inner Box Color/nominal/parent append remain unexecuted";
 i->validate();operation.check();return std::shared_ptr<const BnwCivilopediaNativeInitialGraph>(new BnwCivilopediaNativeInitialGraph(std::move(i)));
}

std::shared_ptr<const BnwCivilopediaNativeInitialGraph>BnwCivilopediaNativeInitialGraph::fork_create_label_constructor_fields()const{
 need(std::this_thread::get_id()==impl_->thread,"Native graph fork foreign thread");
 auto i=std::make_unique<Impl>(*impl_);
 auto operation=i->label_prefix_guard->enter();i->validate();operation.check();
 const auto&prior=i->frame;need(i->label_fields.empty(),"Native Label field constructors cannot replay");
 const auto&tree=i->right->advance(prior.client,0);
 struct Pin{assets::AssetHandle handle;std::string sha256;};std::vector<Pin> texture_pins;
 for(const auto&g:i->texture_guards)texture_pins.push_back({g.handle,runtime::identify_bytes(g.bytes).sha256});
 // Retain source/current owners, not this borrowed Impl. Exact Label/parent
 // identity aliases the SAME already-retained source factory records. Those
 // records do not certify native ordered parent append or initialization.
 const auto source_guard=[hwnd=i->hwnd,thread=i->thread,client=i->client,actual=prior.client,right=i->right,slider=i->slider,root=i->root,xml=prior.xml_sha256,styles=prior.styles_sha256,style_handle=i->styles_handle,pins=std::move(texture_pins),color=i->color_owner,color_quote=i->outer_color_quote,prefix_guard=i->label_prefix_guard](){
  try{
   need(std::this_thread::get_id()==thread,"Native Label graph foreign thread");
   need(prefix_guard->healthy(),"Native Label XML prefix owner retired");DWORD process=0;RECT rc{};
   need(IsWindow(hwnd)&&GetWindowThreadProcessId(hwnd,&process)==GetCurrentThreadId()&&process==GetCurrentProcessId()&&GetClientRect(hwnd,&rc)&&rc.right-rc.left==actual.width&&rc.bottom-rc.top==actual.height,"Native Label SAME HWND changed");
   const auto c=client.current();need(c&&same(*c,actual),"Native Label client epoch changed");
   const auto&t=right->advance(*c,0);const auto&bar=slider->current();need(slider->retained_right_tree().get()==right.get()&&same(bar.client,*c)&&t.xml_sha256==xml&&t.styles_sha256==styles,"Native Label retained graph/source differs");
   need(runtime::identify_file(root/"Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.xml").sha256==xml,"Native Label source XML changed");
   if(style_handle)need(runtime::identify_bytes(style_handle->read_bytes(64ULL*1024*1024)).sha256==styles,"Native Label stylesheet changed");
   for(const auto&p:pins)need(runtime::identify_bytes(p.handle.read_bytes(256ULL*1024*1024)).sha256==p.sha256,"Native Label retained source texture changed");
   if(color){need(color_quote.has_value()&&same(color->client_extent(),actual),"Native Label retained Color source changed");color->validate_quote(*color_quote);}
   const auto after=client.current();need(after&&same(*after,actual)&&prefix_guard->healthy(),"Native Label client/prefix changed during source guard");
   return true;
  }catch(...){return false;}
 };
 for(std::size_t index=0;index<tree.controls.size();++index){
  const auto&n=tree.controls[index];if(n.element!="Label")continue;
  need(n.parent&&*n.parent<tree.controls.size(),"Native Label source parent record absent");
  const auto&parent=tree.controls[*n.parent];need(std::find(parent.children.begin(),parent.children.end(),index)!=parent.children.end(),"Native Label SAME parent source ordinal differs");
  detail::BnwLabelFieldBinding binding{i->panel_identity,std::shared_ptr<const void>(i->right,&n),std::shared_ptr<const void>(i->right,&parent),index,n.path,source_guard};
  i->label_fields.emplace(index,std::make_shared<detail::BnwNativeLabelFieldStorage>(std::move(binding)));
 }
 need(!i->label_fields.empty(),"Native Label selected source records absent");
 i->validate();operation.check();return std::shared_ptr<const BnwCivilopediaNativeInitialGraph>(new BnwCivilopediaNativeInitialGraph(std::move(i)));
}
std::vector<std::size_t>BnwCivilopediaNativeInitialGraph::staged_label_control_indices()const{
 need(std::this_thread::get_id()==impl_->thread,"Native Label indices foreign thread");
 auto i=std::make_unique<Impl>(*impl_);auto operation=i->label_prefix_guard->enter();
 i->validate();operation.check();std::vector<std::size_t>out;
 for(const auto&[index,fields]:i->label_fields){const auto receipt=fields->snapshot();need(receipt.current(),"Native Label indices field retired");out.push_back(index);}
 i->validate();operation.check();return out;
}
detail::BnwLabelFieldSnapshot BnwCivilopediaNativeInitialGraph::staged_label_fields(std::size_t index)const{
 need(std::this_thread::get_id()==impl_->thread,"Native Label field query foreign thread");
 auto i=std::make_unique<Impl>(*impl_);auto operation=i->label_prefix_guard->enter();
 i->validate();operation.check();const auto found=i->label_fields.find(index);
 need(found!=i->label_fields.end(),"Native Label constructor field instance absent");
 const auto receipt=found->second->snapshot();i->validate();
 need(receipt.current(),"Native Label query lost field/source");operation.check();return receipt;
}

std::shared_ptr<const BnwCivilopediaNativeInitialGraph>BnwCivilopediaNativeInitialGraph::fork_enter_label_xml_read_prefix(std::size_t index)const{
 need(std::this_thread::get_id()==impl_->thread,"Native Label XML prefix foreign thread");
 // Own all inputs before source callbacks, including a callback dropping the
 // last old graph owner. This candidate is a lease on the same staged cells.
 auto i=std::make_unique<Impl>();i->hwnd=impl_->hwnd;i->thread=impl_->thread;i->client=impl_->client;i->root=impl_->root;i->right=impl_->right;i->slider=impl_->slider;i->frame=impl_->frame;i->label_fields=impl_->label_fields;i->label_read_prefixes=impl_->label_read_prefixes;i->styles_handle=impl_->styles_handle;i->panel_identity=impl_->panel_identity;i->label_prefix_guard=impl_->label_prefix_guard;i->source_xml=impl_->source_xml;i->root_context_fields=impl_->root_context_fields;i->texture_guards=impl_->texture_guards;i->color_owner=impl_->color_owner;i->outer_color_quote=impl_->outer_color_quote;
 auto operation=i->label_prefix_guard->enter();
 const auto found=i->label_fields.find(index);need(found!=i->label_fields.end(),"Native Label XML prefix requires SAME constructed field receiver");
 const auto&tree=i->right->snapshot();need(index<tree.controls.size(),"Native Label source index outside SAME tree");
 const auto node=tree.controls[index]; // owned complete actual source attrs
 need(node.element=="Label"&&!i->label_read_prefixes.contains(index),"Native Label XML prefix wrong receiver/replay");
 //409390 queries presence, not nonempty value. FontStyle is a distinct key.
 for(const auto&[key,value]:node.source_attributes){(void)value;need(key!="Style","Native Label Style prefix needs actual registry/recursive read owner");}
 i->validate();operation.check();
 const auto before=found->second->snapshot();need(!before.fields().subtype_read_entered&&before.identity().control_index==index&&before.identity().control.get()==&tree.controls[index],"Native Label XML prefix stale/wrong SAME field occurrence");
 BnwNativeLabelXmlReadPrefixFrame receipt;receipt.control_index=index;receipt.source_path=node.path;receipt.xml_sha256=tree.xml_sha256;receipt.absent_style_branch_complete=true;
 receipt.pending_dependency="Actual supplied UI context+3C at5E3AD9 is unavailable; then SAME lazy font registry543DB0, current Color/style/String suppliers and438170 baseRead remain mandatory";
 // Local modeled source OR02000000 precedes the unavailable context write.
 // No parser, Font/Color/Text setter or initialization is skipped as complete.
 operation.check();
 const auto shared_cell=found->second; // survives candidate allocation/publication failure
 try{
  const auto transition=shared_cell->xml_read_entry();need(!transition.text_update_called&&!transition.native_vm_executed,"Native Label XML prefix invented TextUpdate computation");
  receipt.subtype_read_entry_modeled=true;receipt.fields=found->second->snapshot();
  i->label_read_prefixes.emplace(index,std::move(receipt));i->validate();
  need(i->label_read_prefixes.at(index).fields->current(),"Native Label XML prefix changed during final graph validation");
  operation.check();
  return std::shared_ptr<const BnwCivilopediaNativeInitialGraph>(new BnwCivilopediaNativeInitialGraph(std::move(i)));
 }catch(...){
  // The cell is shared with previous graph forks. Once its source revision
  // has changed, an unpublished or uncertain transition cannot stay current.
  shared_cell->close();throw;
 }
}
std::optional<BnwNativeLabelXmlReadPrefixFrame>BnwCivilopediaNativeInitialGraph::staged_label_xml_read_prefix(std::size_t index)const{
 need(std::this_thread::get_id()==impl_->thread,"Native Label XML prefix query foreign thread");
 // Keep an owned validation lease before callbacks; no borrowed Impl reads
 // after a callback can drop the last graph shared_ptr.
 auto i=std::make_unique<Impl>();i->hwnd=impl_->hwnd;i->thread=impl_->thread;i->client=impl_->client;i->root=impl_->root;i->right=impl_->right;i->slider=impl_->slider;i->frame=impl_->frame;i->label_fields=impl_->label_fields;i->label_read_prefixes=impl_->label_read_prefixes;i->styles_handle=impl_->styles_handle;i->panel_identity=impl_->panel_identity;i->label_prefix_guard=impl_->label_prefix_guard;i->source_xml=impl_->source_xml;i->root_context_fields=impl_->root_context_fields;i->texture_guards=impl_->texture_guards;i->color_owner=impl_->color_owner;i->outer_color_quote=impl_->outer_color_quote;
 auto operation=i->label_prefix_guard->enter();
 i->validate();operation.check();const auto found=i->label_read_prefixes.find(index);if(found==i->label_read_prefixes.end())return {};
 need(found->second.fields&&found->second.fields->current(),"Native Label XML prefix field/source revision retired");operation.check();return found->second;
}

BnwGraphicPair BnwCivilopediaNativeInitialGraph::enclosing_panel_anchor_nominal_input()const{
 need(std::this_thread::get_id()==impl_->thread,"Native graph dispatch/query foreign thread");
 auto i=std::make_unique<Impl>(*impl_);auto operation=i->label_prefix_guard->enter();
 i->validate();operation.check();
 const auto&frame=i->frame;need(frame.enclosing_constructors.has_value(),"Native enclosing Box constructors absent");throw std::runtime_error("Native Panel anchor input unavailable: actual Box Read/color providers and source parent registration not complete; authored1000x980 is not an applied nominal field");
}
void BnwCivilopediaNativeInitialGraph::dispatch_enclosing_parent_bounds()const{
 need(std::this_thread::get_id()==impl_->thread,"Native graph dispatch/query foreign thread");
 auto i=std::make_unique<Impl>(*impl_);auto operation=i->label_prefix_guard->enter();
 i->validate();operation.check();
 const auto&frame=i->frame;need(frame.enclosing_constructors.has_value(),"Native enclosing parent receiver absent");const auto&enclosing=*frame.enclosing_constructors;need(enclosing.bindings[1].child_identity.get()==i->panel_identity.get()&&enclosing.bindings[1].parent_identity.get()==enclosing.boxes[1].identity.get(),"Native enclosing parent receiver differs");throw std::runtime_error("Native parent bounds dispatch unavailable: AddChild source sibling vector and all child bounds required before6EBC30/ancestor fanout; no clip/ancestor initialization");
}
void BnwCivilopediaNativeInitialGraph::dispatch_special_callback(BnwNativePanelSpecialCallback kind)const{
 need(std::this_thread::get_id()==impl_->thread,"Native graph dispatch/query foreign thread");
 auto i=std::make_unique<Impl>(*impl_);auto operation=i->label_prefix_guard->enter();
 i->validate();operation.check();
 const auto&f=i->frame;need(f.pre_ratio_special_geometry.has_value(),"Native special callback slot not registered");const auto&g=*f.pre_ratio_special_geometry;const BnwNativeSpecialCallbackBinding*binding=nullptr;for(const auto&v:g.callbacks)if(v.kind==kind)binding=&v;need(binding&&binding->panel_identity.get()==i->panel_identity.get(),"Native special callback receiver identity differs");throw std::runtime_error("Native special callback dispatch unavailable: normal/scroll state and synchronous event dependencies not initialized");
}
}

namespace civ5::ui {
std::shared_ptr<const BnwCivilopediaNativeInitialGraph>
BnwCivilopediaNativeInitialGraph::fork_create_root_context_constructor_fields()const{
 need(std::this_thread::get_id()==impl_->thread,"Native ROOT Context constructor foreign thread");
 // Complete owned lease BEFORE the first external current/source callback.
 auto i=std::make_unique<Impl>(*impl_);
 auto operation=i->label_prefix_guard->enter();
 need(!i->root_context_fields&&i->source_xml&&i->source_xml->name=="Context","Native ROOT Context constructor missing/replayed");
 i->validate();operation.check();
 // Source-only validation lease must NOT retain this component (or any
 // mutable Label field cells) and therefore cannot form a PIMPL/cell cycle.
 auto sources=std::make_shared<Impl>(*i);
 sources->root_context_fields.reset();sources->label_fields.clear();sources->label_read_prefixes.clear();
 detail::BnwRootContextFieldBinding binding{i->source_xml,"Context",i->frame.xml_sha256,[sources]{
  try{sources->validate();const auto after=sources->client.current();
   return after&&same(*after,sources->frame.client)&&sources->label_prefix_guard->healthy();
  }catch(...){return false;}
 }};
 auto component=std::make_shared<detail::BnwNativeRootContextFields>(std::move(binding));
 try{
  i->root_context_fields=component;i->validate();
  need(component->snapshot().current(),"Native ROOT Context constructor lost source/owner");operation.check();
  return std::shared_ptr<const BnwCivilopediaNativeInitialGraph>(new BnwCivilopediaNativeInitialGraph(std::move(i)));
 }catch(...){component->close();throw;}
}
std::shared_ptr<const BnwCivilopediaNativeInitialGraph>
BnwCivilopediaNativeInitialGraph::fork_read_root_context_empty_declaration()const{
 need(std::this_thread::get_id()==impl_->thread,"Native ROOT Context read foreign thread");
 auto i=std::make_unique<Impl>(*impl_);
 auto operation=i->label_prefix_guard->enter();
 const auto component=i->root_context_fields;
 need(component!=nullptr,"Native ROOT Context SAME constructor component absent");
 i->validate();operation.check();
 const auto before=component->snapshot();
 need(!before.fields().empty_declaration_read_modeled,"Native ROOT Context empty read cannot replay");
 // All source declarations are tested by the existing retained XmlNode. The
 // narrow empty branch never delegates to another parser/registry/default.
 try{
  component->read_empty_declaration();const auto receipt=component->snapshot();
  i->validate();need(receipt.current(),"Native ROOT Context read lost final source/owner");operation.check();
  return std::shared_ptr<const BnwCivilopediaNativeInitialGraph>(new BnwCivilopediaNativeInitialGraph(std::move(i)));
 }catch(...){component->close();throw;}
}
std::optional<detail::BnwRootContextFieldSnapshot>
BnwCivilopediaNativeInitialGraph::staged_root_context_fields()const{
 need(std::this_thread::get_id()==impl_->thread,"Native ROOT Context query foreign thread");
 auto i=std::make_unique<Impl>(*impl_);
 auto operation=i->label_prefix_guard->enter();i->validate();operation.check();
 if(!i->root_context_fields)return {};
 const auto receipt=i->root_context_fields->snapshot();
 i->validate();need(receipt.current(),"Native ROOT Context query lost source/owner");operation.check();return receipt;
}
}

namespace civ5::ui {
std::shared_ptr<const BnwCivilopediaNativeInitialGraph>
BnwCivilopediaNativeInitialGraph::fork_register_root_instance_metadata_prefix()const{
 need(std::this_thread::get_id()==impl_->thread,"Native ROOT metadata foreign thread");
 auto i=std::make_unique<Impl>(*impl_);
 auto operation=i->label_prefix_guard->enter();
 const auto component=i->root_context_fields;
 need(component!=nullptr,"Native ROOT metadata SAME constructor component absent");
 i->validate();operation.check();
 const auto before=component->snapshot();
 need(before.fields().empty_declaration_read_modeled&&!before.fields().instance_metadata_prefix_modeled,
      "Native ROOT metadata source order/replay refused");
 try{
  component->register_instance_metadata_prefix();const auto receipt=component->snapshot();
  i->validate();need(receipt.current(),"Native ROOT metadata lost final source/owner");operation.check();
  return std::shared_ptr<const BnwCivilopediaNativeInitialGraph>(new BnwCivilopediaNativeInitialGraph(std::move(i)));
 }catch(...){component->close();throw;}
}
std::optional<detail::BnwRootContextInstanceTemplateQuote>
BnwCivilopediaNativeInitialGraph::staged_root_instance_template(std::string_view input)const{
 need(std::this_thread::get_id()==impl_->thread,"Native ROOT template query foreign thread");
 auto i=std::make_unique<Impl>(*impl_);const std::string name(input);
 auto operation=i->label_prefix_guard->enter();i->validate();operation.check();
 need(i->root_context_fields!=nullptr,"Native ROOT template SAME component absent");
 const auto receipt=i->root_context_fields->snapshot();
 auto result=receipt.instance_template(name);
 i->validate();need(receipt.current(),"Native ROOT template query lost source/owner");operation.check();
 return result;
}
}
