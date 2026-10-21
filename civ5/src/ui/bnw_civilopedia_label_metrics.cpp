#include "civ5/ui/bnw_civilopedia_label_metrics.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <map>
#include <set>
namespace civ5::ui {
namespace {
void need(bool v,const char*s){if(!v)throw std::runtime_error(s);}
bool same(const runtime::BnwStockLuaClientExtent&a,const runtime::BnwStockLuaClientExtent&b){return a.surface_owner==b.surface_owner&&a.revision==b.revision&&a.width==b.width&&a.height==b.height;}
int scalar(std::string_view s){int v=0;auto q=std::from_chars(s.data(),s.data()+s.size(),v);need(q.ec==std::errc{}&&q.ptr==s.data()+s.size()&&v>-32768&&v<32768,"Unsupported native Label scalar");return v;}
int integer(std::string_view s){int v=0;auto q=std::from_chars(s.data(),s.data()+s.size(),v);need(q.ec==std::errc{}&&q.ptr==s.data()+s.size()&&v>0&&v<32768,"Unsupported native Label wrap source");return v;}
void collect(const content::XmlNode&n,std::string path,std::map<std::string,const content::XmlNode*>&out){need(out.emplace(path,&n).second,"Duplicate Label source path");for(std::size_t i=0;i<n.children.size();++i){const auto&c=n.children[i];if(c.name=="GridData")continue;if(c.name=="ShowOnMouseOver"){for(std::size_t j=0;j<c.children.size();++j)collect(c.children[j],path+"/"+std::to_string(i)+":ShowOnMouseOver/"+std::to_string(j)+":"+c.children[j].name,out);}else collect(c,path+"/"+std::to_string(i)+":"+c.name,out);}}
void stacks(const content::XmlNode&n,std::map<std::string,const content::XmlNode*>&out){if(n.name=="Instance")return;const auto id=n.attribute("ID");if(id=="WideStack"||id=="SuperWideStack"||id=="NarrowStack"){collect(n,"ScrollPanel/"+id,out);return;}for(const auto&c:n.children)stacks(c,out);}
struct Font {std::string key,sha;GlyphGenGeometry geometry;std::set<std::string>styles;};
}
std::shared_ptr<const BnwCivilopediaLabelMetricsOwner>BnwCivilopediaLabelMetricsOwner::create(
 const std::filesystem::path&root,const assets::ResourceMountTable&resources,const BnwRightTreeFrame&tree){
 need(tree.shell_revision==0&&!tree.client.surface_owner.empty()&&!tree.xml_sha256.empty(),"Label metrics require retained initial actual tree");
 const auto source=root/"Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.xml";
 need(runtime::identify_file(source).sha256==tree.xml_sha256,"Label source changed since retained tree");
 const auto xml=content::read_xml(source);std::map<std::string,const content::XmlNode*>authored;stacks(xml,authored);need(authored.size()==tree.controls.size(),"Owned Label tree differs from selected XML factory set");for(const auto&n:tree.controls){const auto found=authored.find(n.path);need(found!=authored.end()&&found->second->name==n.element&&found->second->attributes==n.source_attributes&&found->second->attribute("String")==n.text&&found->second->attribute("Font")==n.font&&found->second->attribute("Style")==n.style,"Owned Label declaration differs from selected source");}
 BnwNativePlainLabelFrame f;f.client=tree.client;f.shell_revision=tree.shell_revision;f.xml_sha256=tree.xml_sha256;
 std::map<std::string,Font>fonts;std::set<std::string>missing;
 for(std::size_t i=0;i<tree.controls.size();++i){const auto&n=tree.controls[i];if(n.element!="Label"||n.text.empty())continue;
  if(n.text.starts_with("TXT_KEY_")||n.text.find('{')!=std::string::npos){f.localized_paths.push_back(n.path);continue;}
  need(n.style.empty()&&(n.font=="TwCenMT16"||n.font=="TwCenMT18"||n.font=="TwCenMT20"),"Unowned source plain Label font/style");
  for(unsigned char cp:n.text)need(cp>=32&&cp<127&&cp!='['&&cp!=']'&&cp!='}',"Source literal needs rich text/UTF8 owner");
  std::string font_style;int leading=0;for(const auto&[k,v]:n.source_attributes){if(k=="TruncateWidth")need(v=="0","Source literal requires truncation owner");if(k=="LeadingOffset")leading=scalar(v);if(k=="FontStyle")font_style=v;}
  const std::string key="Assets/UI/Fonts/Tw Cent MT/"+n.font+".ggxml";
  auto font=fonts.find(key);if(font==fonts.end()){
   const auto selected=resources.resolve(assets::AssetKey(key));const auto*handle=std::get_if<assets::AssetHandle>(&selected.asset);
   if(!handle){need(std::holds_alternative<std::monostate>(selected.asset),"Ambiguous selected Label font");missing.insert(key);continue;}
   const auto bytes=handle->read_bytes(4ULL*1024*1024);const auto identity=runtime::identify_bytes(bytes);
   need(runtime::identify_file(root/std::filesystem::path(key)).sha256==identity.sha256,"Selected Label font differs from shared source");
   const auto descriptor=GlyphGenFont::from_xml(content::read_xml_metadata(std::string(bytes.begin(),bytes.end())));
   Font value{key,identity.sha256,glyphgen_geometry(descriptor),{}};for(const auto&style:descriptor.styles)value.styles.insert(style.name);
   const auto&g=value.geometry;need(g.advance_adjustment==0&&g.ascent-g.ascent_adjustment>0,"Unknown native Label adjusted ascent");
   font=fonts.emplace(key,std::move(value)).first;
  }
  need(!font_style.empty()&&font->second.styles.contains(font_style),"Native Label source font style absent");
  const auto&g=font->second.geometry;std::int64_t width=0;
  // 0x5077F0 casts each positive integer native glyph+20 advance separately.
  for(unsigned char cp:n.text){const auto glyph=std::find_if(g.glyphs.begin(),g.glyphs.end(),[&](const auto&v){return v.code_point==cp;});need(glyph!=g.glyphs.end()&&glyph->advance()>=0,"Literal native glyph/fallback unsupported");width+=glyph->advance();need(width<32768,"Literal native width overflow");}
  int wrap=-1;for(const auto&[k,v]:n.source_attributes)if(k=="WrapWidth")wrap=integer(v);
  // 0x6204A0: no current glyph or remaining word can cross wrap when every
  // nonnegative advance's full sum fits. No line is silently discarded.
  need(wrap<0||width<=wrap,"Literal requires native word wrapping");
  BnwNativePlainLabelMetric m;m.control_index=i;m.path=n.path;m.text=n.text;m.font_key=key;m.font_sha256=font->second.sha;m.width=static_cast<int>(width);m.height=g.ascent-g.ascent_adjustment;m.wrap_width=wrap;m.font_style=font_style;m.leading_offset=leading;for(const auto&[k,v]:n.source_attributes){const bool b=!v.empty()&&(v[0]=='t'||v[0]=='T'||v[0]=='1');if(k=="ForceIME")m.force_ime=b;if(k=="ForceNonIME")m.force_non_ime=b;}m.glyph_line=project_glyphgen_line(g,n.text);need(m.glyph_line.advance_width==m.width,"Native Label and shared renderer glyph advances differ");f.staged_metrics.push_back(std::move(m));
 }
 for(const auto&[key,font]:fonts)need(runtime::identify_file(root/std::filesystem::path(key)).sha256==font.sha,"Label font changed before publication");
 need(runtime::identify_file(source).sha256==f.xml_sha256,"Label source changed before publication");
 f.missing_font_keys.assign(missing.begin(),missing.end());
 return std::shared_ptr<const BnwCivilopediaLabelMetricsOwner>(new BnwCivilopediaLabelMetricsOwner(std::move(f)));
}
const BnwNativePlainLabelMetric*BnwCivilopediaLabelMetricsOwner::metric(const runtime::BnwStockLuaClientExtent&c,std::uint64_t r,std::size_t i)const noexcept{if(!same(c,frame_.client)||r!=frame_.shell_revision)return nullptr;for(const auto&m:frame_.staged_metrics)if(m.control_index==i)return &m;return nullptr;}
BnwLiteralImageChainResult stage_bnw_literal_image_parent_chain(const BnwLiteralImageChainInput&in,int w,int h,float ox,float oy){
 need(w>=0&&h>0&&in.image_index<in.stack_children.size(),"Invalid literal parent chain input");need(std::isfinite(ox)&&std::isfinite(oy),"Unknown Label authored offset");
 BnwLiteralImageChainResult r;r.label=in.label;r.image=in.image;r.calls.push_back("TextControl.size_writer");r.label.width=static_cast<float>(w);r.label.height=static_cast<float>(h);r.label.edge_x=std::abs(ox)+r.label.width;r.label.edge_y=std::abs(oy)+r.label.height;
 r.calls.push_back("Label.ControlBase.bounds");r.label.bounds=update_bnw_native_base_bounds(r.label.x,r.label.y,r.label.width,r.label.height,{});
 if(r.label.bounds==in.label.bounds)return r;
 r.calls.push_back("Image.ControlBase.bounds");r.image.bounds=update_bnw_native_base_bounds(r.image.x,r.image.y,r.image.width,r.image.height,{r.label.bounds});
 if(r.image.bounds==in.image.bounds)return r;
 auto children=in.stack_children;children[in.image_index]=r.image;
 r.calls.push_back("Stack.CalculateSize");r.stack=reduce_bnw_bottom_stack_cache(children,in.padding,in.stack_offset_x,in.stack_offset_y,in.stack_x,in.stack_y);
 // 0x4D5110 invokes parent BEFORE 0x76E380 can reach base bounds. Retain
 // suspension at that nested callback, not after fictitious completion.
 if(r.stack->bounds!=in.previous_stack_bounds){r.calls.push_back("ScrollPanel.ControlBase.bounds REQUIRED");r.mandatory_outer_callback=true;r.size_writer_suspended=true;return r;}
 r.calls.push_back("Stack.ControlBase.bounds");std::vector<std::array<float,4>>bounds;for(const auto&child:children)bounds.push_back(child.bounds);const auto b=update_bnw_native_base_bounds(in.stack_x,in.stack_y,r.stack->width,r.stack->height,bounds);if(b!=r.stack->bounds){r.stack->bounds=b;r.calls.push_back("ScrollPanel.ControlBase.bounds REQUIRED");r.mandatory_outer_callback=true;r.size_writer_suspended=true;}
 return r;
}
}
