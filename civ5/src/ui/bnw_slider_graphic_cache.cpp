#include "civ5/ui/bnw_slider_graphic_cache.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <set>
namespace civ5::ui {
namespace {
void need(bool v,const char *why){if(!v)throw std::runtime_error(why);}
float scalar(std::string_view s){float v=0;const auto r=std::from_chars(s.data(),s.data()+s.size(),v);need(r.ec==std::errc{}&&r.ptr==s.data()+s.size()&&std::isfinite(v)&&std::abs(v)<=16777216,"Graphic numeric source unavailable");return v;}
BnwGraphicPair pair(std::string_view s){const auto comma=s.find(',');need(comma!=std::string_view::npos&&s.find(',',comma+1)==std::string_view::npos,"Graphic source pair unavailable");return {scalar(s.substr(0,comma)),scalar(s.substr(comma+1))};}
const content::XmlNode &style(const content::XmlNode &styles,std::string_view key){const content::XmlNode *found=nullptr;for(const auto &n:styles.children)if(n.name==key){need(!found,"Duplicate selected graphic style");found=&n;}need(found,"Selected graphic style unavailable");return *found;}
bool same(const runtime::BnwStockLuaClientExtent &a,const runtime::BnwStockLuaClientExtent &b){return a.surface_owner==b.surface_owner&&a.revision==b.revision&&a.width==b.width&&a.height==b.height;}
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
BnwGraphicNineSlice skin(const content::XmlNode &node,const assets::ResourceMountTable &resources,BnwGraphicPair size,bool track){
 BnwGraphicNineSlice g;g.style=node.name;g.texture_key=node.attribute("Texture");g.size=size;
 need(node.name==(track?"VertSlider":"VertShuttle")&&node.attribute("Vertical")=="1"&&!g.texture_key.empty(),"Selected vertical graphic instance differs");
 constexpr std::array<std::string_view,9> slots{"C","UC","LC","L","R","UL","UR","LL","LR"};
 std::set<std::string> allowed{"Texture","Size","Vertical","StateOffsetIncrement"};
 for(const auto slot:slots){allowed.insert(std::string(slot)+"Size");allowed.insert(std::string(slot)+"TexStart");}
 // The actual stock stylesheet typo is ignored by native 0x0072A7A0.
 if(track)allowed.insert("ULCexStart");
 std::set<std::string> seen;for(const auto &[name,value]:node.attributes){(void)value;need(seen.insert(name).second&&allowed.contains(name),"Unowned graphic style attribute");}
 need(node.attribute("StateOffsetIncrement")== (track?"0,0":"0,18"),"Selected initial graphic state offset differs");
 if(track){need(node.children.size()==1&&node.children[0].name=="Shuttle"&&node.children[0].attribute("Style")=="VertShuttle"&&node.children[0].attribute("Size")=="18,18"&&node.children[0].children.empty(),"Embedded shuttle source binding differs");}
 else need(node.children.empty(),"Graphic has unowned native children");
 for(std::size_t i=0;i<slots.size();++i){auto text=node.attribute(std::string(slots[i])+"Size");if(!text.empty())g.authored_sizes[i]=pair(text);text=node.attribute(std::string(slots[i])+"TexStart");if(!text.empty())g.source_starts[i]=pair(text);}
 g.texture=std::make_shared<const assets::UiTextureBinding>(assets::bind_ui_texture(resources,assets::resource_basename(g.texture_key)));
 need(g.texture->width()>0&&g.texture->height()>0,"Graphic logical texture extent unavailable");
 // 0x005D33C0 zeros arrays. The cache portion of 0x0070F8E0
 // records source ends BEFORE replacing authored sizes with stretch sizes.
 // Base anchors/children/flags are NOT declared initialized by this scope.
 g.extents=g.authored_sizes;for(std::size_t i=0;i<9;++i)g.source_ends[i]={g.source_starts[i].x+g.authored_sizes[i].x,g.source_starts[i].y+g.authored_sizes[i].y};
 recache(g);g.dirty_cache_branch_evaluated=true;return g;
}
}
std::unique_ptr<BnwSliderGraphicCacheOwner> BnwSliderGraphicCacheOwner::create(const std::filesystem::path &root,const assets::ResourceMountTable &resources,const BnwScrollBarLayoutFrame &shell,std::string_view control){
 need(shell.control_revision==0&&!shell.styles_sha256.empty()&&!shell.client.surface_owner.empty(),"Graphic creation requires original parsed shell/source frame");
 const auto *bar=shell.control(control);need(bar&&(control=="ScrollBar"||control=="LeftScrollBar")&&bar->rectangle.width==18&&bar->rectangle.height>0&&bar->rectangle.height<=16777216,"Selected slider actual shell instance unavailable");
 const auto resolved=resources.resolve(assets::AssetKey("Styles.xml"));const auto *handle=std::get_if<assets::AssetHandle>(&resolved.asset);need(handle,"Selected graphic stylesheet VFS missing");const auto bytes=handle->read_bytes(64ULL*1024*1024);const auto id=runtime::identify_bytes(bytes);need(id.sha256==shell.styles_sha256&&runtime::identify_file(root/"Assets/UI/Styles.xml").sha256==id.sha256,"Graphic source changed since shell creation");
 const auto styles=content::read_xml_metadata(std::string(bytes.begin(),bytes.end()));need(styles.name=="StyleSheet","Graphic style root differs");
 BnwSliderGraphicCacheFrame frame;frame.client=shell.client;frame.shell_revision=shell.control_revision;frame.styles_sha256=id.sha256;
 frame.track=skin(style(styles,"VertSlider"),resources,{18,static_cast<float>(bar->rectangle.height)},true);
 const auto &shuttle=style(styles,"VertShuttle");need(shuttle.attribute("Size")=="18,18","Actual embedded shuttle cross/primary extent differs");frame.shuttle=skin(shuttle,resources,{18,18},false);
 return std::unique_ptr<BnwSliderGraphicCacheOwner>(new BnwSliderGraphicCacheOwner(std::move(frame)));
}
const BnwGraphicNineSlice *BnwSliderGraphicCacheOwner::graphic(const runtime::BnwStockLuaClientExtent &client,std::uint64_t revision,std::string_view key)const noexcept{if(!same(client,frame_.client)||revision!=frame_.shell_revision)return nullptr;if(key=="track")return &frame_.track;if(key=="shuttle")return &frame_.shuttle;return nullptr;}
std::unique_ptr<BnwSliderGraphicCacheOwner> BnwSliderGraphicCacheOwner::fork_track_cache(const runtime::BnwStockLuaClientExtent &client,std::uint64_t revision,std::int64_t width,std::int64_t height)const{
 need(graphic(client,revision,"track"),"Stale graphic frame cannot be mutated");need(width==18&&height>0&&height<=16777216&&frame_.graphic_revision!=UINT64_MAX,"Graphic update outside exact positive vertical slider branch");
 auto next=frame_;next.track.size={static_cast<float>(width),static_cast<float>(height)};recache(next.track);++next.graphic_revision;
 return std::unique_ptr<BnwSliderGraphicCacheOwner>(new BnwSliderGraphicCacheOwner(std::move(next)));
}
std::unique_ptr<BnwSliderGraphicCacheOwner>BnwSliderGraphicCacheOwner::fork_track_native_cache(const runtime::BnwStockLuaClientExtent&client,std::uint64_t revision,float w,float h)const{
 need(graphic(client,revision,"track"),"Stale native track cache");need(w==18&&std::isfinite(h)&&h>0&&h<=16777216&&frame_.graphic_revision!=UINT64_MAX,"Native track extent outside owned positive vertical scope");auto next=frame_;next.track.size={w,h};recache(next.track);++next.graphic_revision;return std::unique_ptr<BnwSliderGraphicCacheOwner>(new BnwSliderGraphicCacheOwner(std::move(next)));
}
std::unique_ptr<BnwSliderGraphicCacheOwner>BnwSliderGraphicCacheOwner::fork_shuttle_cache(const runtime::BnwStockLuaClientExtent&client,std::uint64_t revision,float w,float h)const{
 need(graphic(client,revision,"shuttle"),"Stale native shuttle cache");need(w==18&&std::isfinite(h)&&h>0&&h<=16777216&&frame_.graphic_revision!=UINT64_MAX,"Native shuttle extent outside owned positive vertical scope");auto next=frame_;next.shuttle.size={w,h};recache(next.shuttle);++next.graphic_revision;return std::unique_ptr<BnwSliderGraphicCacheOwner>(new BnwSliderGraphicCacheOwner(std::move(next)));
}

}
