#include "civ5/ui/bnw_civilopedia_initial_content.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <set>
namespace civ5::ui {
namespace {
void need(bool ok,const char *why){if(!ok)throw std::runtime_error(why);}
bool same_frame(const runtime::BnwStockLuaClientExtent &a,const runtime::BnwStockLuaClientExtent &b)noexcept {
 return a.surface_owner==b.surface_owner&&a.revision==b.revision&&a.width==b.width&&a.height==b.height;
}
void find(const content::XmlNode &node,std::string_view id,std::vector<const content::XmlNode*> &found){
 if(node.name=="Instance")return;
 if(node.attribute("ID")==id)found.push_back(&node);
 for(const auto &child:node.children)find(child,id,found);
}
const content::XmlNode &unique(const content::XmlNode &root,std::string_view id){
 std::vector<const content::XmlNode*> found;find(root,id,found);
 need(found.size()==1,"Initial Civilopedia source control is absent or ambiguous");return *found[0];
}
void inventory(const content::XmlNode &node,std::string_view panel,std::string parent,bool hidden,
               std::vector<BnwCivilopediaContentDeclaration> &out){
 if(node.name=="Instance")return; // templates are not BuildInstance results
 const auto id=node.attribute("ID");
 const auto path=parent+"/"+node.name+(id.empty()?"":"#"+id);
 BnwCivilopediaContentDeclaration item;
 item.panel_id=panel;item.path=path;item.element=node.name;item.id=id;
 item.source_attributes=node.attributes;item.anchor=node.attribute("Anchor");item.anchor_side=node.attribute("AnchorSide");if(item.anchor_side.empty())item.anchor_side=node.attribute("Anchorside");
 item.size=node.attribute("Size");item.offset=node.attribute("Offset");
 item.style=node.attribute("Style");item.font=node.attribute("Font");item.text=node.attribute("String");
 item.stack_growth=node.attribute("StackGrowth");item.padding=node.attribute("Padding");
 const auto state=node.attribute("Hidden");
 if(!state.empty()){need(state=="0"||state=="1","Unknown authored Hidden value");item.explicit_hidden=state=="1";}
 item.explicitly_hidden_ancestor=hidden;
 out.push_back(std::move(item));hidden=hidden||state=="1";
 for(std::size_t i=0;i<node.children.size();++i)inventory(node.children[i],panel,path+"["+std::to_string(i)+"]",hidden,out);
}
void allowed(const content::XmlNode &node,std::initializer_list<std::string_view> names){
 std::set<std::string> seen;
 for(const auto &[key,value]:node.attributes){(void)value;need(seen.insert(key).second&&std::find(names.begin(),names.end(),key)!=names.end(),"Initial literal content has an unsupported attribute");}
}
}
const BnwCivilopediaLiteralLine *BnwCivilopediaInitialContentSnapshot::line(std::string_view id)const noexcept {
 if(id.empty())return nullptr;for(const auto &item:literal_lines)if(item.id==id)return &item;return nullptr;
}
std::vector<BnwCivilopediaContentDeclaration> inspect_bnw_civilopedia_content_declarations(const content::XmlNode &xml){
 need(xml.name=="Context","Civilopedia content root differs");
 const auto &right=unique(xml,"ScrollPanel");const auto &left=unique(xml,"LeftScrollPanel");
 need(right.name=="ScrollPanel"&&left.name=="ScrollPanel","Civilopedia content panel kind differs");
 std::vector<BnwCivilopediaContentDeclaration> result;
 inventory(right,"ScrollPanel","",false,result);inventory(left,"LeftScrollPanel","",false,result);
 return result;
}
std::vector<BnwCivilopediaLiteralLine> project_bnw_civilopedia_initial_literal_lines(
 const content::XmlNode &xml,const GlyphGenFont &font,const GlyphGenGeometry &geometry){
 const auto &stack=unique(xml,"WideStack");need(stack.name=="Stack"&&stack.attribute("Anchor")=="R,T"&&stack.attribute("Offset")=="38,0"&&stack.attribute("StackGrowth")=="Bottom"&&stack.attribute("Padding")=="12"&&stack.attribute("Size")=="64,64"&&stack.children.size()>=2,"Initial literal content source stack differs");
 const GlyphGenStyle *style=nullptr;for(const auto &entry:font.styles)if(entry.name=="SoftShadow"){need(!style,"Duplicate selected SoftShadow style");style=&entry;}
 need(style&&style->layers==std::vector<std::string>{"TwCenMT20Base","TwCenMT20Shadow","TwCenMT20Soft"},"Selected initial literal font style differs");
 need(geometry.line_height==20&&geometry.ascent==18&&geometry.descent==5,"Selected TwCenMT20 metric profile differs");
 std::vector<BnwCivilopediaLiteralLine> lines;
 for(std::size_t i=0;i<2;++i){
  const auto &image=stack.children[i];const auto image_id=i?"SubtitleID":"";
  allowed(image,{"Anchor","Size","Offset","Texture","ID"});
  need(image.name=="Image"&&image.attribute("ID")==image_id&&image.attribute("Anchor")=="R,T"&&image.attribute("Size")=="500,64"&&image.attribute("Offset")==(i?"-18,-30":"-18,0")&&image.attribute("Texture")=="CivilopediaTitle.dds"&&image.children.size()==1,"Initial literal title image source differs");
  const auto &label=image.children[0];allowed(label,{"Anchor","Offset","Font","FontStyle","Color0","Color1","String","ID"});
  need(label.name=="Label"&&label.attribute("Anchor")=="C,C"&&label.attribute("Offset")=="0,-2"&&label.attribute("Font")=="TwCenMT20"&&label.attribute("FontStyle")=="SoftShadow"&&label.attribute("Color0")=="Beige"&&label.attribute("Color1")=="0.0.0.160"&&label.attribute("ID")== (i?"SubtitleLabel":"ArticleID")&&label.attribute("String")==(i?"Subtitle goes here":"title goes here"),"Initial literal title label source differs");
  need(&unique(xml,label.attribute("ID"))==&label,"Initial literal source ID is ambiguous");
  BnwCivilopediaLiteralLine line;line.id=label.attribute("ID");line.text=label.attribute("String");line.font=label.attribute("Font");line.font_style=label.attribute("FontStyle");line.image_path="ScrollPanel/WideStack/Image["+std::to_string(i)+"]";line.image_width=500;line.image_height=64;
  line.glyph_line=project_glyphgen_line(geometry,line.text);
  need(line.glyph_line.advance_width>0&&line.glyph_line.advance_width<=500,"Initial literal glyph line exceeds its owned image-local scope");
  line.line_x=(line.image_width-line.glyph_line.advance_width)/2.0;
  line.line_y=(line.image_height-geometry.line_height)/2.0-2;
  line.baseline=line.line_y+geometry.ascent;lines.push_back(std::move(line));
 }
 return lines;
}
std::shared_ptr<const BnwCivilopediaInitialContentOwner> BnwCivilopediaInitialContentOwner::create(
 const std::filesystem::path &root,const assets::ResourceMountTable &resources,
 const runtime::BnwStockLuaClientExtent &client,std::uint64_t revision,std::string_view expected_xml){
 need(!client.surface_owner.empty()&&client.width>0&&client.height>0&&revision==9,"Initial content requires the actual initial stock shell revision/frame");
 const auto xml_path=root/"Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.xml";
 const auto xml_id=runtime::identify_file(xml_path);need(xml_id.sha256==expected_xml,"Initial content source changed since context staging");
 const auto xml=content::read_xml(xml_path);need(runtime::identify_file(xml_path).sha256==expected_xml,"Initial content XML changed during read");
 constexpr std::string_view key="Assets/UI/Fonts/Tw Cent MT/TwCenMT20.ggxml";
 const auto selected=resources.resolve(assets::AssetKey(std::string(key)));const auto *handle=std::get_if<assets::AssetHandle>(&selected.asset);
 need(handle,"Initial content selected font descriptor VFS binding unavailable");
 const auto bytes=handle->read_bytes(4ULL*1024*1024);const auto font_id=runtime::identify_bytes(bytes);
 need(runtime::identify_file(root/std::filesystem::path(key)).sha256==font_id.sha256,"Initial content selected font differs from bounded shared source");
 const auto font=GlyphGenFont::from_xml(content::read_xml_metadata(std::string(bytes.begin(),bytes.end())));
 BnwCivilopediaInitialContentSnapshot snapshot;snapshot.client=client;snapshot.shell_revision=revision;snapshot.xml_sha256=xml_id.sha256;snapshot.font_sha256=font_id.sha256;snapshot.font_key=key;
 snapshot.font_geometry=glyphgen_geometry(font);snapshot.literal_lines=project_bnw_civilopedia_initial_literal_lines(xml,font,snapshot.font_geometry);
 for(const auto &style:font.styles)if(style.name=="SoftShadow")snapshot.font_style_layers=style.layers;
 snapshot.declared_descendants=inspect_bnw_civilopedia_content_declarations(xml);
 for(const auto &item:snapshot.declared_descendants){if(!item.explicit_hidden)++snapshot.unknown_hidden_count;else if(*item.explicit_hidden)++snapshot.explicit_hidden_count;}
 // A descriptor/glyph line is not a realized Stack or a native scroll cache.
 snapshot.blocker="CalculateInternalSize unavailable: native child traversal, default Hidden, StackGrowth/padding and label auto-size/cache semantics lack source receipts; created image-local literal lines do not authorize panel bounds";
 need(runtime::identify_file(xml_path).sha256==xml_id.sha256&&runtime::identify_file(root/std::filesystem::path(key)).sha256==font_id.sha256,"Initial content sources changed before immutable owner creation");
 return std::shared_ptr<const BnwCivilopediaInitialContentOwner>(new BnwCivilopediaInitialContentOwner(std::move(snapshot)));
}
const BnwCivilopediaLiteralLine *BnwCivilopediaInitialContentOwner::line(
 const runtime::BnwStockLuaClientExtent &client,std::uint64_t revision,std::string_view id)const noexcept {
 if(!same_frame(snapshot_.client,client)||snapshot_.shell_revision!=revision)return nullptr;
 return snapshot_.line(id);
}
}