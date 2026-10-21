#include "civ5/ui/bnw_civilopedia_right_tree_owner.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/bnw_civilopedia_label_metrics.hpp"
#include <algorithm>
#include <cmath>
#include <set>
namespace civ5::ui {
namespace {
void need(bool v,const char *s){if(!v)throw std::runtime_error(s);}
bool same(const runtime::BnwStockLuaClientExtent&a,const runtime::BnwStockLuaClientExtent&b){return a.surface_owner==b.surface_owner&&a.revision==b.revision&&a.width==b.width&&a.height==b.height;}
void unite(std::array<float,4>&a,const std::array<float,4>&b,float x,float y){a[0]=std::min(a[0],b[0]+x);a[1]=std::min(a[1],b[1]+y);a[2]=std::max(a[2],b[2]+x);a[3]=std::max(a[3],b[3]+y);}
void find(const content::XmlNode&n,std::string_view id,std::vector<const content::XmlNode*>&out){if(n.name=="Instance")return;if(n.attribute("ID")==id)out.push_back(&n);for(const auto&c:n.children)find(c,id,out);}
const content::XmlNode&unique(const content::XmlNode&xml,std::string_view id){std::vector<const content::XmlNode*>v;find(xml,id,v);need(v.size()==1,"Selected right control source absent/ambiguous");return *v[0];}
std::size_t create_nodes(const content::XmlNode &n,std::optional<std::size_t>parent,std::string path,BnwRightTreeFrame&f,std::set<std::string>&ids,bool hover=false){
 need(n.name!="Instance","Template is not an instantiated right child");
 const auto index=f.controls.size();BnwCreatedRightControl v;v.path=std::move(path);v.source_attributes=n.attributes;v.parent=parent;v.element=n.name;v.id=n.attribute("ID");v.style=n.attribute("Style");v.font=n.attribute("Font");v.text=n.attribute("String");v.anchor=n.attribute("Anchor");v.offset=n.attribute("Offset");v.size=n.attribute("Size");
 need(v.id.empty()||ids.insert(v.id).second,"Duplicate actual right control ID");
 auto hidden=n.attribute("Hidden");need(hidden.empty()||hidden=="0"||hidden=="1","Unknown right child Hidden source");
 v.show_on_mouse_over=hover;v.hidden=hidden=="1";v.has_explicit_hidden=!hidden.empty();v.hidden_resolved=v.has_explicit_hidden||v.style.empty(); // native ctor zero + parser only true sets bit2
 if(v.text.starts_with("TXT_KEY_"))f.pending_locale_keys.push_back(v.text);
 f.controls.push_back(std::move(v));
 // Native0x438170 processes hover group contents first, adding to +0x78
 // and hover vector+0x7C, flag0x2000. The wrapper is NOT a control.
 for(std::size_t i=0;i<n.children.size();++i){const auto&g=n.children[i];if(g.name=="ShowOnMouseOver"){
  f.controls[index].special_source_groups.push_back(g);++f.metadata_group_count;
  for(std::size_t j=0;j<g.children.size();++j){auto child=create_nodes(g.children[j],index,f.controls[index].path+"/"+std::to_string(i)+":ShowOnMouseOver/"+std::to_string(j)+":"+g.children[j].name,f,ids,true);f.controls[index].children.push_back(child);}
 }}
 for(std::size_t i=0;i<n.children.size();++i){const auto&c=n.children[i];if(c.name=="ShowOnMouseOver")continue;if(c.name=="GridData"){need(c.children.empty(),"Embedded GridData children need their separate native owner");f.controls[index].special_source_groups.push_back(c);++f.metadata_group_count;continue;}
  constexpr std::array<std::string_view,7>classes{"Stack","Image","Label","Grid","Box","GridButton","AlphaAnim"};need(std::find(classes.begin(),classes.end(),c.name)!=classes.end(),"Unsupported native source factory class");
  auto child=create_nodes(c,index,f.controls[index].path+"/"+std::to_string(i)+":"+c.name,f,ids);f.controls[index].children.push_back(child);
 }
 return index;
}
float rounded(float v){return v<0?std::ceil(v-0.5f):std::floor(v+0.5f);}
}
BnwBottomStackCache reduce_bnw_bottom_stack_cache(const std::vector<BnwNativeChildCache>&children,float padding,float ox,float oy,float px,float py){
 need(std::isfinite(padding)&&std::isfinite(ox)&&std::isfinite(oy)&&std::isfinite(px)&&std::isfinite(py),"Invalid source Stack scalar");BnwBottomStackCache r;r.bounds={std::abs(px),std::abs(py),std::abs(px),std::abs(py)};float cursor=0;
 for(const auto&c:children){for(float v:{c.width,c.height,c.x,c.y,c.edge_x,c.edge_y,c.bounds[0],c.bounds[1],c.bounds[2],c.bounds[3]})need(std::isfinite(v),"Unknown child cache");
  // 0x004AE8D0: skipped children return zero translation and do not advance.
  float dy=0;if(!c.hidden&&!c.outside_x&&!c.outside_y&&c.width!=0&&c.height!=0){dy=cursor;const auto step=c.height+c.y+padding;cursor+=std::abs(step);if(step<0)dy=cursor;}
  r.translations.push_back({0,dy});
  // 0x004D5110: outside children contribute visible bounds but no extent.
  if(!c.hidden&&c.width!=0&&c.height!=0){if(!c.outside_x&&!c.outside_y){r.width=std::max(r.width,c.edge_x);r.height=std::max(r.height,dy+c.edge_y);}unite(r.bounds,c.bounds,0,dy);}
 }
 r.edge_x=std::abs(ox)+r.width;r.edge_y=std::abs(oy)+r.height;
 // Native publishes zero-origin union span (not min corner).
 r.bounds={0,0,std::abs(r.bounds[2]-r.bounds[0]),std::abs(r.bounds[3]-r.bounds[1])};return r;
}
std::array<std::int32_t,2> inspect_bnw_initial_literal_native_size(const GlyphGenFont&font,std::string_view text){
 need(text=="title goes here"||text=="Subtitle goes here","Unknown native literal shaping source");
 const auto geometry=glyphgen_geometry(font); // requires explicit selected ascadj
 need(geometry.advance_adjustment==0&&geometry.ascent==18&&geometry.ascent_adjustment==5,"Native literal font profile unknown");
 std::int32_t width=0;for(unsigned char cp:text){auto g=std::find_if(geometry.glyphs.begin(),geometry.glyphs.end(),[&](const auto&v){return v.code_point==cp;});need(g!=geometry.glyphs.end(),"Native literal glyph absent");width+=static_cast<std::int32_t>(g->advance());}
 return {width,geometry.ascent-geometry.ascent_adjustment};
}
std::shared_ptr<BnwCivilopediaRightTreeOwner>BnwCivilopediaRightTreeOwner::create(const std::filesystem::path&root,const assets::ResourceMountTable&resources,const BnwScrollBarLayoutFrame&shell){
 need(shell.control_revision==0&&!shell.client.surface_owner.empty()&&shell.client.width>0&&shell.client.height>0&&shell.control("ScrollPanel"),"Right tree requires original retained actual shell");
 const auto path=root/"Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.xml";auto before=runtime::identify_file(path);need(before.sha256==shell.xml_sha256,"Right tree source changed since shell");auto xml=content::read_xml(path);need(runtime::identify_file(path).sha256==before.sha256,"Right tree source changed during read");
 BnwRightTreeFrame f;f.client=shell.client;f.shell_revision=shell.control_revision;f.xml_sha256=before.sha256;f.styles_sha256=shell.styles_sha256;std::set<std::string>ids;
 constexpr std::array<std::string_view,3>names{"WideStack","SuperWideStack","NarrowStack"};
 const auto&panel=unique(xml,"ScrollPanel");need(panel.children.size()==6&&panel.children[0].name=="ScrollBar"&&panel.children[1].name=="UpButton"&&panel.children[2].name=="DownButton","Actual right ScrollPanel direct child set differs");
 for(std::size_t i=0;i<3;++i){const auto&n=unique(xml,names[i]);need(&n==&panel.children[i+3]&&n.name=="Stack"&&n.attribute("StackGrowth")=="Bottom"&&n.attribute("Padding")=="12"&&n.attribute("Size")=="64,64"&&n.attribute("WrapWidth").empty(),"Selected initial right Stack shape differs");f.stacks[i]=create_nodes(n,std::nullopt,"ScrollPanel/"+std::string(names[i]),f,ids);}
 f.source_node_count=f.controls.size()+f.metadata_group_count;
 constexpr std::string_view key="Assets/UI/Fonts/Tw Cent MT/TwCenMT20.ggxml";
 const auto selected=resources.resolve(assets::AssetKey(std::string(key)));const auto*handle=std::get_if<assets::AssetHandle>(&selected.asset);need(handle,"Initial right font selected VFS unavailable");const auto bytes=handle->read_bytes(4ULL*1024*1024);f.font_sha256=runtime::identify_bytes(bytes).sha256;f.font_key=key;
 need(runtime::identify_file(root/std::filesystem::path(key)).sha256==f.font_sha256,"Selected right font differs from shared descriptor");
 const auto font=GlyphGenFont::from_xml(content::read_xml_metadata(std::string(bytes.begin(),bytes.end())));f.font_geometry=glyphgen_geometry(font);
 auto lines=project_bnw_civilopedia_initial_literal_lines(xml,font,f.font_geometry);
 // Native 0x00672FE0 ascent-ascadj; 0x007AC170 returns it; 0x005077F0
 // writes this to TextControl.height for a plain single line, NOT line_height.
 need(f.font_geometry.advance_adjustment==0&&f.font_geometry.ascent_adjustment==5&&f.font_geometry.ascent==18,"Actual native literal font profile differs");const int height=f.font_geometry.ascent-f.font_geometry.ascent_adjustment;f.native_font_ascent=height;
 auto title=std::make_shared<const assets::UiTextureBinding>(assets::bind_ui_texture(resources,"CivilopediaTitle.dds"));
 for(std::size_t i=0;i<2;++i){auto image=f.controls[f.stacks[0]].children[i];need(f.controls[image].children.size()==1,"Literal image actual child count differs");auto label=f.controls[image].children[0];auto&n=f.controls[label];need(lines[i].id==n.id,"Literal metric receipt does not match owned child");
  // Native font glyph+0x20 is a+b+c (0x005F4B00). No wrapping,
  // truncation, markup or localized key exists in these two source literals.
  const auto native_size=inspect_bnw_initial_literal_native_size(font,n.text);const auto advance=native_size[0];
  need(advance==lines[i].glyph_line.advance_width&&advance>0&&advance<500&&native_size[1]==height,"Native literal advance receipt differs");
  n.parent_local_rectangle={rounded((500.0f-static_cast<float>(advance))*0.5f),rounded((64.0f-height)*0.5f)-2,advance,height};n.cached_edge_x=static_cast<float>(advance);n.cached_edge_y=2.0f+height;n.glyph_line=std::move(lines[i].glyph_line);n.literal_metrics_complete=true;
  f.literal_images.push_back({image,label,title});++f.completed_literal_labels;
 }
 f.grid_graphics=BnwCivilopediaGridGraphicCacheOwner::create(root,resources,f);
 f.label_metrics=BnwCivilopediaLabelMetricsOwner::create(root,resources,f);
 const auto pending=f.controls[f.stacks[0]].children[2];f.pending_child_path=f.controls[pending].path;f.pending_path=f.controls[f.literal_images.front().label].path;
 f.pending_dependency="TextControl size writer virtual+0x94 Image/WideStack parent bounds propagation; GridBlackIndent8/Grid9Frame child initialization and localized shaping before Stack cached bounds; literal metric quotes do not skip callbacks";
 need(runtime::identify_file(path).sha256==f.xml_sha256&&runtime::identify_file(root/std::filesystem::path(key)).sha256==f.font_sha256,"Right tree dependencies changed before creation");
 return std::shared_ptr<BnwCivilopediaRightTreeOwner>(new BnwCivilopediaRightTreeOwner(std::move(f)));
}
const BnwRightTreeFrame&BnwCivilopediaRightTreeOwner::advance(const runtime::BnwStockLuaClientExtent&c,std::uint64_t r)const{need(same(c,frame_.client)&&r==frame_.shell_revision,"Stale client/right-tree epoch");return frame_;}
const BnwCreatedRightControl*BnwCivilopediaRightTreeOwner::control(const runtime::BnwStockLuaClientExtent&c,std::uint64_t r,std::string_view id)const noexcept{if(!same(c,frame_.client)||r!=frame_.shell_revision||id.empty())return nullptr;for(const auto&n:frame_.controls)if(n.id==id)return &n;return nullptr;}
} // namespace civ5::ui
