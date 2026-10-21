#include "civ5/ui/bnw_tech_award_context_layout.hpp"
#include "civ5/ui/bnw_tech_award_context_owner.hpp"
#include "civ5/ui/bnw_civilopedia_grid_graphic_cache.hpp"
#include "civ5/content/manifest.hpp"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <map>
#include <set>
#include <stdexcept>
namespace civ5::ui {
namespace {
void need(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
float scalar(std::string_view s){float f=0;const auto p=std::from_chars(s.data(),s.data()+s.size(),f);need(p.ec==std::errc{}&&p.ptr==s.data()+s.size()&&std::isfinite(f)&&std::abs(f)<32768,"TechAward selected layout scalar unavailable");return f;}
std::array<std::string,2> tokens(std::string_view s){const auto split=s.find_first_of(",.");need(split!=s.npos&&s.find_first_of(",.",split+1)==s.npos,"TechAward selected layout pair unavailable");return {std::string(s.substr(0,split)),std::string(s.substr(split+1))};}
std::array<float,2> pair(std::string_view s){const auto t=tokens(s);return {scalar(t[0]),scalar(t[1])};}
std::map<std::string,std::string> attributes(const content::XmlNode& n){std::map<std::string,std::string> result;for(const auto& [key,value]:n.attributes)need(result.emplace(key,value).second,"TechAward duplicate style attribute");return result;}
bool equivalent(const content::XmlNode& a,const content::XmlNode& b){if(a.name!=b.name||a.text!=b.text||attributes(a)!=attributes(b)||a.children.size()!=b.children.size())return false;for(std::size_t i=0;i<a.children.size();++i)if(!equivalent(a.children[i],b.children[i]))return false;return true;}
const content::XmlNode& style(const content::XmlNode& styles,const std::string& key){const content::XmlNode* result=nullptr;for(const auto& n:styles.children)if(n.name==key){need(!result||equivalent(*result,n),"TechAward ambiguous selected style");result=&n;}need(result,"TechAward selected style missing");return *result;}
content::XmlNode expanded(const content::XmlNode& n,const content::XmlNode& styles){auto result=n;const auto key=n.attribute("Style");if(key.empty())return result;const auto& inherited=style(styles,key);auto attrs=attributes(inherited);for(const auto& [k,v]:n.attributes)attrs.insert_or_assign(k,v);result.attributes.assign(attrs.begin(),attrs.end());result.children=n.children;result.children.insert(result.children.end(),inherited.children.begin(),inherited.children.end());return result;}
float anchor(float origin,float parent,float size,char a,char side,float offset){
 need(side=='I'||side=='O',"TechAward anchor side unknown");
 if(a=='C')return origin+(parent-size)*0.5f+offset;
 if(a=='L'||a=='T')return origin+(side=='O'?-size:0.0f)+offset;
 if(a=='R'||a=='B')return origin+parent+(side=='O'?offset:-size-offset);
 throw std::runtime_error("TechAward selected anchor unknown");
}
}
BnwTechAwardContextLayout project_bnw_tech_award_context_layout(const content::XmlNode& context,const content::XmlNode& styles,const BnwTechAwardContextObservation& observed,ClientViewport viewport){
 need(context.name=="Context"&&styles.name=="StyleSheet"&&viewport.drawable()&&observed.measured_grid_width&&observed.measured_grid_height,"TechAward layout needs actual context/style/viewport/nominal dimensions");
 BnwTechAwardContextLayout out;out.viewport=viewport;
 const auto size=[&](const auto& visit,const content::XmlNode& authored,const std::string& path,std::array<float,2> parent)->std::array<float,2>{
  const auto n=expanded(authored,styles);const auto id=n.attribute("ID");
  if(id=="OuterGrid")return {static_cast<float>(*observed.measured_grid_width),static_cast<float>(*observed.measured_grid_height)};
  if(id=="BonusStack")return {static_cast<float>(observed.bonus_width),static_cast<float>(observed.bonus_height)};
  if(n.name=="Label") {for(const auto& label:observed.measured_labels)if(label.path==path)return {static_cast<float>(label.layout.width),static_cast<float>(label.layout.height)};throw std::runtime_error("TechAward actual measured Label absent in recursive layout");}
  if(n.name=="Stack") {const auto direction=n.attribute("StackGrowth");need(direction=="Bottom","TechAward unsupported dynamic stack direction");const auto padding=n.attribute("Padding");const float gap=padding.empty()?0:scalar(padding);float w=0,h=0;bool first=true;
   for(std::size_t i=0;i<n.children.size();++i){const auto& child=n.children[i];const auto child_id=child.attribute("ID");bool hidden=child.attribute("Hidden")=="1";for(const auto& v:observed.controls)if(v.id==child_id&&!child_id.empty())hidden=v.hidden;const auto s=visit(visit,child,path+"/"+std::to_string(i)+":"+child.name,parent);if(hidden||s[0]==0||s[1]==0)continue;w=std::max(w,s[0]);h+=s[1]+(first?0:gap);first=false;}return {w,h};
  }
  if(n.name=="GridData"||(n.name=="ShowOnMouseOver"||n.name=="HideOnMouseOver"))return parent;
  const auto declared=n.attribute("Size");if(declared.empty())return {0,0};const auto t=tokens(declared);std::array<float,2> result{};
  for(std::size_t i=0;i<2;++i)result[i]=t[i]=="Full"?(i==0?static_cast<float>(viewport.client_width):static_cast<float>(viewport.client_height)):scalar(t[i]);
  need(result[0]>=0&&result[1]>=0,"TechAward negative selected layout extent");return result;
 };
 const auto walk=[&](const auto& visit,const content::XmlNode& authored,const std::string& path,std::array<float,4> parent,bool inherited_hidden,bool hover,std::optional<std::array<float,2>> forced)->std::array<float,4>{
  const auto n=expanded(authored,styles);const auto extent=size(size,authored,path,{parent[2],parent[3]});const auto id=n.attribute("ID");
  BnwTechAwardLayoutNode value;value.path=path;value.id=id;value.kind=n.name;value.style=n.attribute("Style");value.texture=n.attribute("Texture");value.color=n.attribute("Color");value.color_set=n.attribute("ColorSet");
  for(std::size_t color_index=0;color_index<3;++color_index)value.color_groups[color_index]=n.attribute("Color"+std::to_string(color_index));
  if(const auto authored_crop=n.attribute("TextureOffset");!authored_crop.empty()) {
   const auto crop=pair(authored_crop);need(crop[0]>=0&&crop[1]>=0&&std::trunc(crop[0])==crop[0]&&std::trunc(crop[1])==crop[1],"TechAward actual authored crop is not bounded integer");
   value.texture_offset={static_cast<std::int32_t>(crop[0]),static_cast<std::int32_t>(crop[1])};
  }value.hidden=inherited_hidden||n.attribute("Hidden")=="1";value.mouse_over_only=hover||n.name=="ShowOnMouseOver";
  for(const auto& c:observed.controls)if(c.id==id&&!id.empty()){value.hidden=inherited_hidden||c.hidden;value.texture=c.texture;value.texture_offset={c.texture_x,c.texture_y};}
  const auto declared_anchor=n.attribute("Anchor"),declared_side=n.attribute("AnchorSide"),declared_offset=n.attribute("Offset");const auto a=declared_anchor.empty()?std::array<std::string,2>{"L","T"}:tokens(declared_anchor);const auto side=declared_side.empty()?std::array<std::string,2>{"I","I"}:tokens(declared_side);need(a[0].size()==1&&a[1].size()==1&&side[0].size()==1&&side[1].size()==1,"TechAward malformed selected anchors");const auto offset=declared_offset.empty()?std::array<float,2>{}:pair(declared_offset);
  value.rect={anchor(parent[0],parent[2],extent[0],a[0][0],side[0][0],offset[0]),anchor(parent[1],parent[3],extent[1],a[1][0],side[1][0],offset[1]),extent[0],extent[1]};
  if(forced){value.rect[0]=parent[0]+(*forced)[0];value.rect[1]=parent[1]+(*forced)[1];}
  value.alpha_start=n.attribute("AlphaStart");value.alpha_end=n.attribute("AlphaEnd");value.cycle=n.attribute("Cycle");value.speed=n.attribute("Speed");
  if(n.name=="AlphaAnim"){const auto actual=attributes(n);value.animation_profile.assign(actual.begin(),actual.end());}
  if(n.name=="Label")for(std::size_t i=0;i<observed.measured_labels.size();++i)if(observed.measured_labels[i].path==path)value.measured_label=i;
  need(out.nodes.size()<1024,"TechAward layout recursion limit");const auto index=out.nodes.size();out.nodes.push_back(value);std::vector<std::array<float,4>> child_bounds;float cursor=0;
  for(std::size_t i=0;i<n.children.size();++i){const auto& child=n.children[i];const auto child_path=path+"/"+std::to_string(i)+":"+child.name;std::optional<std::array<float,2>> child_position;
   if(n.name=="Stack"){const auto child_size=size(size,child,child_path,{value.rect[2],value.rect[3]});bool hidden=child.attribute("Hidden")=="1";for(const auto& c:observed.controls)if(c.id==child.attribute("ID")&&!c.id.empty())hidden=c.hidden;
    if(n.attribute("StackGrowth")=="Bottom")child_position=std::array<float,2>{(value.rect[2]-child_size[0])*0.5f,cursor};
    else if(id=="BonusStack")child_position=std::array<float,2>{cursor,(value.rect[3]-child_size[1])*0.5f};else throw std::runtime_error("TechAward stack positioning unsupported");
    if(hidden)child_position.reset();
    if(!hidden&&child_size[0]>0&&child_size[1]>0)cursor+=(n.attribute("StackGrowth")=="Bottom"?child_size[1]:child_size[0])+scalar(n.attribute("Padding"));
   }
   child_bounds.push_back(visit(visit,child,child_path,value.rect,value.hidden,value.mouse_over_only,child_position));
  }
  // Seal the actual authored wrapper and its attached parent. Nested wrappers
  // are explicitly unsupported rather than inventing a composed flag issuer.
  if(n.name=="ShowOnMouseOver"||n.name=="HideOnMouseOver") {
   const auto slash=path.find_last_of('/');need(slash!=std::string::npos,"TechAward hover wrapper has no parent");
   const auto parent_path=path.substr(0,slash);
   for(std::size_t at=index;at<out.nodes.size();++at) {
    auto& member=out.nodes[at];
    need(member.mouse_wrapper_path.empty(),"TechAward nested hover wrappers unsupported");
    member.mouse_wrapper_path=path;member.mouse_parent_path=parent_path;
    member.mouse_hide_only=n.name=="HideOnMouseOver";member.mouse_over_only=true;
   }
  }
  const std::set<std::string> unsupported{"Disabled","ConsumeMouse","ConsumeMouseOver","ClipChildren","MouseEnter","MouseExit"};
  for(const auto& key:unsupported)if(!n.attribute(key).empty()&&n.attribute(key)!="0")out.nodes[index].mouse_input_unsupported=true;
  // Genuine port of +0x94: all children contribute, including hidden/outside.
  const auto bounds=update_bnw_native_base_bounds(value.rect[0],value.rect[1],value.rect[2],value.rect[3],child_bounds);out.nodes[index].bounds=bounds;return bounds;
 };
 for(std::size_t i=0;i<context.children.size();++i)walk(walk,context.children[i],"Context/"+std::to_string(i)+":"+context.children[i].name,{0,0,static_cast<float>(viewport.client_width),static_cast<float>(viewport.client_height)},false,false,std::nullopt);
 return out;
}
BnwTechAwardGridSlices project_bnw_tech_award_grid_slices(const content::XmlNode& skin,const BnwTechAwardLayoutNode& node,std::uint32_t width,std::uint32_t height){
 need(skin.attribute("Padding")=="0,0","TechAward selected Grid kerning/padding profile changed");
 need(width&&height&&node.rect[2]>0&&node.rect[3]>0,"TechAward nine-slice selected texture/extent absent");
 constexpr std::array<const char*,9> slots{"C","UC","LC","L","R","UL","UR","LL","LR"};std::array<std::array<float,2>,9>s{},start{},authored{};
 for(std::size_t i=0;i<9;++i){s[i]=pair(skin.attribute(std::string(slots[i])+"Size"));start[i]=pair(skin.attribute(std::string(slots[i])+"TexStart"));authored[i]=s[i];need(s[i][0]>=0&&s[i][1]>=0&&start[i][0]>=0&&start[i][1]>=0&&start[i][0]+s[i][0]<=static_cast<float>(width)&&start[i][1]+s[i][1]<=static_cast<float>(height),"TechAward nine-slice source crop exceeds selected DDS");}
 const float w=node.rect[2],h=node.rect[3];
 // 0x47CC00 exact float order; selected styles have actual zero kerning.
 s[1][0]=(((w-s[6][0])+0.0f)-s[5][0])+0.0f;s[3][1]=(((h-s[5][1])+0.0f)-s[7][1])+0.0f;
 s[0][0]=((w-s[3][0])-s[4][0])+0.0f+0.0f;s[0][1]=((h-s[1][1])-s[2][1])+0.0f+0.0f;
 s[4][1]=(((h-s[6][1])+0.0f)-s[8][1])+0.0f;s[2][0]=(((w-s[8][0])+0.0f)-s[7][0])+0.0f;
 const std::array<std::array<float,2>,9> positions{{{s[3][0],s[1][1]},{s[5][0],0},{s[7][0],h-s[2][1]},{0,s[5][1]},{w-s[4][0],s[6][1]},{0,0},{w-s[6][0],0},{0,h-s[7][1]},{w-s[8][0],h-s[8][1]}}};
 BnwTechAwardGridSlices result;result.path=node.path;result.texture=skin.attribute("Texture");const float iw=1.0f/static_cast<float>(width),ih=1.0f/static_cast<float>(height);
 for(std::size_t i=0;i<9;++i){need(s[i][0]>=0&&s[i][1]>=0,"TechAward nine-slice negative dynamic center");result.rects[i]={node.rect[0]+positions[i][0],node.rect[1]+positions[i][1],s[i][0],s[i][1]};result.uvs[i]={start[i][0]*iw,start[i][1]*ih,authored[i][0]*iw,authored[i][1]*ih};}
 return result;
}
}
