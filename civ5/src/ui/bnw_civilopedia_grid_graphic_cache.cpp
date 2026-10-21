#include "civ5/ui/bnw_civilopedia_grid_graphic_cache.hpp"
#include "civ5/ui/bnw_civilopedia_right_tree_owner.hpp"
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
float number(std::string_view s){float v=0;const auto r=std::from_chars(s.data(),s.data()+s.size(),v);need(r.ec==std::errc{}&&r.ptr==s.data()+s.size()&&std::isfinite(v)&&std::abs(v)<=16777216,"Grid numeric source unavailable");return v;}
BnwGraphicPair pair(std::string_view s){const auto c=s.find(',');need(c!=std::string_view::npos&&s.find(',',c+1)==std::string_view::npos,"Grid source pair unavailable");return {number(s.substr(0,c)),number(s.substr(c+1))};}
const content::XmlNode&style(const content::XmlNode&xml,std::string_view key){const content::XmlNode*n=nullptr;for(const auto&s:xml.children)if(s.name==key){need(!n,"Duplicate selected Grid style");n=&s;}need(n,"Selected Grid style missing");return *n;}
void recache(BnwGraphicNineSlice &g){
 need(static_cast<bool>(g.texture),"Graphic cache requires actual selected skin/texture owner");
 auto &s=g.extents;const float w=g.size.x,h=g.size.y,l=g.hkern.x,r=g.hkern.y,t=g.vkern.x,b=g.vkern.y;
 // 0x0047CC00, retaining the original float32 expression order.
 s[1].x=(((w-s[6].x)+l)-s[5].x)+r;
 s[3].y=(((h-s[5].y)+t)-s[7].y)+b;
 s[0].x=((w-s[3].x)-s[4].x)+r+l;
 s[0].y=((h-s[1].y)-s[2].y)+b+t;
 s[4].y=(((h-s[6].y)+t)-s[8].y)+b;
 s[2].x=(((w-s[8].x)+l)-s[7].x)+r;
 g.positions={{{s[3].x-l,s[1].y-t},{s[5].x-l,0},{s[7].x-l,h-s[2].y},
              {0,s[5].y-t},{w-s[4].x,s[6].y-t},{0,0},{w-s[6].x,0},
              {0,h-s[7].y},{w-s[8].x,h-s[8].y}}};
 // 0x004B0E90. Native texture+0x118 is logical width, +0x114 height.
 const float inv_w=1.0f/static_cast<float>(g.texture->width());
 const float inv_h=1.0f/static_cast<float>(g.texture->height());
 for(std::size_t i=0;i<9;++i){const auto p=g.source_starts[i],end=g.source_ends[i];
   g.uv_starts[i]={p.x*inv_w,p.y*inv_h};
   g.uv_spans[i]={(-p.x+end.x)*inv_w,(end.y+-p.y)*inv_h};
 }
}
BnwGraphicNineSlice skin(const content::XmlNode&style,const assets::ResourceMountTable&resources){
 need(style.name=="GridBlackIndent8"||style.name=="Grid9Frame"||style.name=="Grid9Science8","Unsupported selected Grid skin");need(style.children.empty()&&style.attribute("Padding")=="0,0","Unowned style child/padding effect");
 constexpr std::array<std::string_view,9>slots{"C","UC","LC","L","R","UL","UR","LL","LR"};std::set<std::string>allowed{"Texture","Padding"},seen;
 for(auto slot:slots){allowed.insert(std::string(slot)+"Size");allowed.insert(std::string(slot)+"TexStart");}
 for(const auto&[key,value]:style.attributes){(void)value;need(allowed.contains(key)&&seen.insert(key).second,"Unowned Grid style state/autosize/kerning attribute");}
 BnwGraphicNineSlice g;g.style=style.name;g.texture_key=style.attribute("Texture");need(!g.texture_key.empty(),"Selected Grid texture token unavailable");
 g.texture=std::make_shared<const assets::UiTextureBinding>(assets::bind_ui_texture(resources,g.texture_key));
 for(std::size_t i=0;i<slots.size();++i){g.authored_sizes[i]=pair(style.attribute(std::string(slots[i])+"Size"));g.source_starts[i]=pair(style.attribute(std::string(slots[i])+"TexStart"));g.source_ends[i]={g.source_starts[i].x+g.authored_sizes[i].x,g.source_starts[i].y+g.authored_sizes[i].y};}
 g.extents=g.authored_sizes;return g;
}
}
std::array<float,4>update_bnw_native_base_bounds(float x,float y,float w,float h,const std::vector<std::array<float,4>>&children){
 for(float v:{x,y,w,h})need(std::isfinite(v),"Unknown actual native base extent");std::array<float,4>r{x,y,x+w,y+h};
 for(const auto&child:children){for(float v:child)need(std::isfinite(v),"Unknown actual native child bounds");r[0]=std::min(r[0],child[0]);r[1]=std::min(r[1],child[1]);r[2]=std::max(r[2],child[2]);r[3]=std::max(r[3],child[3]);}
 return r;
}
std::shared_ptr<const BnwCivilopediaGridGraphicCacheOwner>BnwCivilopediaGridGraphicCacheOwner::create(const std::filesystem::path&root,const assets::ResourceMountTable&resources,const BnwRightTreeFrame&tree){
 need(tree.shell_revision==0&&!tree.client.surface_owner.empty()&&!tree.styles_sha256.empty()&&!tree.xml_sha256.empty(),"Grid cache requires actual initial retained tree");
 const auto selected=resources.resolve(assets::AssetKey("Styles.xml"));const auto*handle=std::get_if<assets::AssetHandle>(&selected.asset);need(handle,"Selected Grid stylesheet VFS absent");const auto bytes=handle->read_bytes(64ULL*1024*1024);const auto id=runtime::identify_bytes(bytes);
 need(id.sha256==tree.styles_sha256&&runtime::identify_file(root/"Assets/UI/Styles.xml").sha256==id.sha256,"Grid stylesheet changed since actual shell");
 const auto xml=content::read_xml_metadata(std::string(bytes.begin(),bytes.end()));need(xml.name=="StyleSheet","Selected Grid stylesheet root differs");
 BnwGridGraphicCacheFrame f;f.client=tree.client;f.shell_revision=tree.shell_revision;f.xml_sha256=tree.xml_sha256;f.styles_sha256=id.sha256;
 std::map<std::string,BnwGraphicNineSlice>skins;
 for(std::size_t i=0;i<tree.controls.size();++i){const auto&n=tree.controls[i];if(n.element!="Grid")continue;
  if(n.style=="Grid9Science8"&&n.size.empty()){f.pending_size_controls.push_back(i);continue;}
  need(n.style=="GridBlackIndent8"||n.style=="Grid9Frame"||n.style=="Grid9Science8","Unsupported actual initial Grid style");need(!n.size.empty(),"Grid nominal source size unavailable");const auto extent=pair(n.size);need(extent.x>0&&extent.y>0,"Grid source extent outside positive selected branch");
  auto s=skins.find(n.style);if(s==skins.end())s=skins.emplace(n.style,skin(style(xml,n.style),resources)).first;
  BnwGridGraphicBinding binding;binding.control_index=i;binding.path=n.path;binding.id=n.id;binding.graphic=s->second;binding.graphic.size=extent;recache(binding.graphic);binding.graphic.dirty_cache_branch_evaluated=true;
  // Data is staged. Graphic.base initialized flag never manufactured.
  f.staged_graphics.push_back(std::move(binding));
 }
 need(runtime::identify_file(root/"Assets/UI/Styles.xml").sha256==f.styles_sha256&&runtime::identify_file(root/"Assets/DLC/Expansion2/UI/Civilopedia/CivilopediaScreen.xml").sha256==tree.xml_sha256,"Grid source changed before immutable cache staging");
 return std::shared_ptr<const BnwCivilopediaGridGraphicCacheOwner>(new BnwCivilopediaGridGraphicCacheOwner(std::move(f)));
}
const BnwGridGraphicBinding*BnwCivilopediaGridGraphicCacheOwner::graphic(const runtime::BnwStockLuaClientExtent&c,std::uint64_t r,std::size_t index)const noexcept{if(!same(c,frame_.client)||r!=frame_.shell_revision)return nullptr;for(const auto&g:frame_.staged_graphics)if(g.control_index==index)return &g;return nullptr;}
}
