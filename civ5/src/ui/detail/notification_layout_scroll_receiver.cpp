#include "civ5/ui/detail/notification_layout_scroll_receiver.hpp"
#include "civ5/ui/detail/notification_layout_runtime_dimensions.hpp"
#include <algorithm>
#include <bit>
#include <charconv>
#include <cmath>
#include <limits>
#include <set>
#include <stdexcept>
#include <string_view>
namespace civ5::ui::detail {
namespace {
void need(bool value,const char* why){if(!value)throw std::runtime_error(why);}
float add(float a,float b){volatile float r=a+b;return r;}
float sub(float a,float b){volatile float r=a-b;return r;}
float mul(float a,float b){volatile float r=a*b;return r;}
float div(float a,float b){volatile float r=a/b;return r;}
float scalar(std::string_view s){
 float value=0;const auto p=std::from_chars(s.data(),s.data()+s.size(),value);
 need(p.ec==std::errc{}&&p.ptr==s.data()+s.size()&&std::isfinite(value),"Actual slider parser finite decimal domain unsupported");return value;
}
std::array<float,2> pair(std::string_view s){
 const auto delimiter=[](char c){return std::string_view(" .,x*|").find(c)!=std::string_view::npos;};
 std::array<float,2> result;std::size_t pos=0;
 for(auto& value:result){while(pos<s.size()&&delimiter(s[pos]))++pos;const auto start=pos;
  while(pos<s.size()&&!delimiter(s[pos]))++pos;need(pos>start,"Slider native two-token parser missing token");value=scalar(s.substr(start,pos-start));}
 while(pos<s.size()&&delimiter(s[pos]))++pos;need(pos==s.size(),"Slider native selected pair has extra tokens");return result;
}
bool affirmative(std::string_view s){return !s.empty()&&(s.front()=='1'||s.front()=='t'||s.front()=='T');}
std::int16_t travel(float value){
 // CVTTSS2SI followed by MOV word preserves the native low16 bits, including
 // the hardware integer-indefinite result for an out-of-range conversion.
 const double v=std::trunc(static_cast<double>(value));
 const std::int32_t i=std::isfinite(v)&&v>=-2147483648.0&&v<2147483648.0?static_cast<std::int32_t>(v):INT32_MIN;
 return std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(static_cast<std::uint32_t>(i)));
}
float extent(float margin,float size){return static_cast<float>(std::fabs(static_cast<long double>(margin))+static_cast<long double>(size));}
}
std::shared_ptr<NotificationLayoutButtonState> NotificationLayoutButtonState::clone_owned(
 const std::shared_ptr<NotificationLayoutButtonState>& actual){
 need(!actual||(!actual->backing_118_&&!actual->pending_label_178_&&!actual->label_178_&&!actual->label_retirement_),
  "Button child storage clone requires a genuine child-state supplier");
 return actual?std::make_shared<NotificationLayoutButtonState>(*actual):nullptr;
}
std::shared_ptr<NotificationLayoutButtonState> NotificationLayoutButtonState::capture_owned(
 NotificationLayoutBaseline& owner,const std::vector<content::XmlNode>& layers,
 const std::map<std::string,content::XmlNode,std::less<>>&){
 need(owner.kind_==NotificationLayoutBaseline::Kind::button&&(owner.actual_type_=="UpButton"||owner.actual_type_=="DownButton"||owner.actual_type_=="Button"),"Button capture needs actual5AAA20 receiver");
 // Family operands reach ONLY a guarded failing String prefix. They never
 // admit a successful text/Label profile or broaden the ordinary noString path.
 const bool selected_string_prefix=owner.actual_type_=="Button"&&
  std::any_of(layers.begin(),layers.end(),[](const auto& layer){
   return std::any_of(layer.attributes.begin(),layer.attributes.end(),[](const auto& e){return e.first=="String";});});
 if(owner.actual_type_=="Button")for(const auto& layer:layers)for(const auto& entry:layer.attributes){
  const auto& key=entry.first;
  // Ordinary Button shares the literal5AAA20 ctor/543910 parser. Selected
  // no-text profile rejects unknown/Style/String branches BEFORE earning
  // constructorNULL178. Existing ABSENT text guard below remains literal.
  if(!(key=="Style"||key=="ID"||key=="Anchor"||key=="AnchorSide"||key=="Size"||key=="Offset"||key=="Hidden"||
   key=="Texture"||key=="TextureOffset"||key=="StateOffsetIncrement"||key=="NoStateChange"||
   key=="Void1"||key=="Void2"||key=="NoDefaultSound"||key=="Disabled"||key=="ConsumeMouseOver"||key=="ConsumeMouse"||key=="Color"||key=="ToolTip"||key=="ToolTipType"||
   (selected_string_prefix&&(key=="String"||key=="Font"||key=="FontStyle"||key=="ColorSet"||
    key=="Color0"||key=="Color1"||key=="Color2"||key=="Color3"||key=="WrapWidth"||
    key=="TruncateWidth"||key=="LeadingOffset"||key=="ForceIME"||key=="ForceNonIME"||key=="ReduceOnTruncate")))){
   // Diagnostics only for the already-refused actual operand. No caller data
   // substitutes native String/Style construction or constructorNULL178.
   const auto& authored=layers.back();
   auto why=std::string("Ordinary Button selected constructorNULL178 profile unsupported authored/style operand")+
    " [actual_type="+owner.actual_type_+" authored_tag="+authored.name+" authored_id="+authored.attribute("ID")+
    " authored_style="+authored.attribute("Style")+" layer="+layer.name+" layer_id="+layer.attribute("ID")+
    " layer_style="+layer.attribute("Style")+" key="+key+" value="+entry.second+" layers={";
   std::size_t index=0;
   for(const auto& actual_layer:layers){
    why+="L"+std::to_string(index++)+":"+actual_layer.name+"{";
    for(const auto& actual_entry:actual_layer.attributes)why+=actual_entry.first+"="+actual_entry.second+";";
    why+="}";
   }
   why+="}]";
   need(false,why.c_str()); // Same failure; existing outer catches quarantine.
  }
  if(key=="Texture")need(!entry.second.empty()&&entry.second.find('\0')==std::string::npos,
   "Ordinary Button PRESENT Texture cannot substitute constructorNULL");
  if(key=="TextureOffset"||key=="StateOffsetIncrement")need(!entry.second.empty(),
   "Ordinary Button PRESENT pair needs genuine finite parser operands");
 }

 need(!owner.button_,"Button actual embedded constructor replay");
 std::shared_ptr<NotificationLayoutButtonState> out;
 if(owner.actual_type_=="Button"){
  need(owner.parser_runtime_dimensions_,"Ordinary Button5AAA20 requires canonical actual Runtime");
  owner.parser_runtime_dimensions_->construct_button_receiver_owned();
  out=owner.button_;need(bool(out),"Ordinary Button actual constructor did not retain its allocation");
 }else{
  owner.base_.flags_40|=0x8000; //Existing UpButton/DownButton constructor route.
  out=std::shared_ptr<NotificationLayoutButtonState>(new NotificationLayoutButtonState);
  out->child_178_observation=NotificationLayoutButtonState::Child178Observation::constructor_null;
  owner.button_=out;
 }
 const auto parse_button_layer=[&](auto&& self,content::XmlNode layer,std::size_t layer_index)->void{
  if(owner.actual_type_=="Button"){
   need(owner.parser_runtime_dimensions_,"Ordinary Style prefix requires actual canonical parser borrower");
   if(const auto style=owner.parser_runtime_dimensions_->run_button_style_prefix_owned(layer,layer_index)){
    // SAME Button vslot44, SAME second Context argument; one constructor only.
    self(self,owner.parser_layers_.at(*style),*style);
   }
  }
  // Each actually reached definition is checked; no stylesheet attributes are skipped.
  if(owner.actual_type_=="Button")for(const auto& entry:layer.attributes){const auto& key=entry.first;
   const bool text=std::any_of(layer.attributes.begin(),layer.attributes.end(),[](const auto& a){return a.first=="String";});
   need(key=="Style"||key=="ID"||key=="Anchor"||key=="AnchorSide"||key=="Size"||key=="Offset"||key=="Hidden"||
    key=="Texture"||key=="TextureOffset"||key=="StateOffsetIncrement"||key=="NoStateChange"||key=="NoDefaultSound"||
    key=="Disabled"||key=="ConsumeMouseOver"||key=="ConsumeMouse"||key=="Color"||key=="ToolTip"||key=="ToolTipType"||
    (text&&(key=="String"||key=="Font"||key=="FontStyle"||key=="ColorSet")),"Reached Button style parser attribute is unsupported");
  }
  // attribute().empty() conflates absent and present-empty. Native54395B
  // branches on lookup success before178 allocation, so this supported
  // constructorNULL178 profile accepts ONLY absent String/text properties.
  const auto absent=[&](const char* key){
   return std::none_of(layer.attributes.begin(),layer.attributes.end(),
    [&](const auto& entry){return entry.first==key;});
  };
  if(!absent("String")){
   need(owner.actual_type_=="Button"&&owner.parser_runtime_dimensions_,
    "Button String requires actual canonical per-layer parser borrower");
   // Exact54395B PRESENT occurrence; direct closed implementation only.
   owner.parser_runtime_dimensions_->run_button_string_failed_prefix_owned(layer,layer_index);
   need(out->child_178_observation==NotificationLayoutButtonState::Child178Observation::factory_child&&
    out->label_178_&&out->label_178_factory_return_,"Button actual full5E return lost published child");
  }
  need(absent("TextAnchor")&&absent("TextOffset"),
   "Selected button constructorNULL178 profile requires absent String/text properties");
  if(const auto key=layer.attribute("Texture");!key.empty()){
   if(owner.actual_type_=="Button")owner.parser_runtime_dimensions_->bind_button_texture_before_common_owned(layer,layer_index);
   else{out->texture_key=key;out->texture_extent_15c.reset();out->texture_binding_earned=false;}
  }
  if(const auto text=layer.attribute("TextureOffset");!text.empty())out->authored_108_10c=pair(text);
  //543A75..543A90 Color is a direct580780 decode into embedded12C..138,
  // AFTER TextureOffset and BEFORE StateOffsetIncrement. It never allocates178.
  for(const auto& entry:layer.attributes)if(entry.first=="Color"){
   need(owner.parser_runtime_dimensions_,"Button Color lacks actual canonical parser borrower");
   if(owner.actual_type_=="Button")BnwNormalButtonSubobject118Backing::invalidate_direct580780_color_before_store_owned(*out);
   out->color_12c_138=owner.parser_runtime_dimensions_->image_color_owned(entry.second);
  }
  if(const auto text=layer.attribute("StateOffsetIncrement");!text.empty()){out->increment_110_114=pair(text);owner.base_.flags_40|=0x20000000;}
  if(affirmative(layer.attribute("NoStateChange")))owner.base_.flags_40|=0x80000000;
  if(owner.actual_type_!="Button"){
   if(affirmative(layer.attribute("NoDefaultSound")))owner.base_.flags_40|=0x2000000;
   if(affirmative(layer.attribute("Disabled")))owner.base_.flags_40|=0x20000;
  } //Ordinary Button6CAEB0 owns these stores AFTER actual Void1/2 readers.
  if(owner.actual_type_=="Button")owner.parser_runtime_dimensions_->run_button_common_returning_layer_owned(layer,layer_index);
  else owner.parse_layer_owned(layer); //Existing UpButton/DownButton common route.
 };
 // Canonical ordinary Button has one authored entry; resolved definitions are
 // retained by the closed prefix borrower and recursively consumed above.
 const auto authored_layers=layers;
 for(std::size_t i=0;i<authored_layers.size();++i)parse_button_layer(parse_button_layer,authored_layers[i],i);
   //String/text admission remains strict above until a genuine Label178
   //factory/parser/initializer is supplied. Do not overwrite the actual
   //constructor observation at the end of parser layers: a future earned
   //nonNULL transition must remain its own retained allocation/state.
 if(owner.actual_type_=="Button"){
  owner.parser_runtime_dimensions_->require_button_common_return_from_parser_owned();
  need(owner.button_.get()==out.get()&&!owner.button_.owner_before(out)&&!out.owner_before(owner.button_),
   "Ordinary Button parser RETURN replaced its genuine physical allocation");
 }
 return out;
}
void NotificationLayoutButtonState::coordinates_owned(NotificationLayoutButtonState& b){
 if(b.backing_118_){BnwNormalButtonSubobject118Backing::execute_coordinates5c7b50_owned(b);return;}
 need(b.texture_binding_earned,"Button parsed Texture has no actual selected raw binding");if(!b.texture_extent_15c)return;
 if(b.backing_118_)BnwNormalButtonSubobject118Backing::invalidate_coordinates_before_store_owned(b);
 const auto d=*b.texture_extent_15c;need(d[0]&&d[1],"Button actual texture extent zero");
 // Literal 5C7B50 x87 FDIVR source field order; the raw field names avoid
 // assigning guessed x/y meaning to the embedded image layout.
 b.uv_170_174={static_cast<float>(static_cast<long double>(b.image_14c_150[0])/d[1]),static_cast<float>(static_cast<long double>(b.image_14c_150[1])/d[0])};
 b.uv_168_16c={static_cast<float>(static_cast<long double>(b.image_144_148[0])/d[1]),static_cast<float>(static_cast<long double>(b.image_144_148[1])/d[0])};
}
void NotificationLayoutButtonState::visual_state_owned(NotificationLayoutBaseline& owner,std::uint32_t state){
 need(bool(owner.button_),"Button genuine per-type state absent");auto& b=*owner.button_;
 if(b.backing_118_){BnwNormalButtonSubobject118Backing::execute_visual_state6b3610_owned(b,owner,state);return;}
 b.state_104=state; //Existing UpButton/DownButton route.
 if(owner.base_.flags_40&0x80000000)return;
 if(b.backing_118_)BnwNormalButtonSubobject118Backing::invalidate_geometry_before_store_owned(b);
 const auto f=static_cast<float>(state);
 b.image_14c_150={add(b.authored_108_10c[0],mul(b.increment_110_114[0],f)),add(mul(b.increment_110_114[1],f),b.authored_108_10c[1])};
 coordinates_owned(b);
}
NotificationLayoutSliderState::NineSlice::NineSlice(std::string type)
 :baseline(NotificationLayoutBaseline::Kind::base,std::move(type)){}
NotificationLayoutSliderState::NotificationLayoutSliderState()
 :shuttle_("Shuttle"),track_("Track"),fraction_128(-1.0f),steps_628(15.0f){}
std::shared_ptr<NotificationLayoutSliderState> NotificationLayoutSliderState::clone_owned(
 const std::shared_ptr<NotificationLayoutSliderState>& value){
 return value?std::shared_ptr<NotificationLayoutSliderState>(new NotificationLayoutSliderState(*value)):nullptr;
}
void NotificationLayoutSliderState::parse_nine_slice_layer_owned(NineSlice& out,const content::XmlNode& xml){
 need(xml.attribute("Color").empty()&&xml.attribute("Alpha").empty(),"NineSlice authored colour parser is outside selected constructor-colour profile");
 out.baseline.parse_layer_owned(xml);
 const auto assign=[&](const char* key,auto& destination){const auto s=xml.attribute(key);if(!s.empty())destination=pair(s);};
 const auto hk=xml.attribute("HKerning"),vk=xml.attribute("VKerning");
 if(!hk.empty()){const auto v=pair(hk);out.kerning_a0_ac[0]=v[0];out.kerning_a0_ac[1]=v[1];}
 if(!vk.empty()){const auto v=pair(vk);out.kerning_a0_ac[2]=v[0];out.kerning_a0_ac[3]=v[1];}
 assign("TextureOffset",out.authored_b8_bc);assign("StateOffsetIncrement",out.increment_b0_b4);
 constexpr std::array<std::string_view,9> names{"C","UC","LC","L","R","UL","UR","LL","LR"};
 for(std::size_t n=0;n<names.size();++n){
  const auto size=xml.attribute(std::string(names[n])+"Size");
  if(!size.empty()){const auto v=pair(size);out.size_110_154[2*n]=v[0];out.size_110_154[2*n+1]=v[1];}
  const auto start=xml.attribute(std::string(names[n])+"TexStart");
  if(!start.empty()){const auto v=pair(start);out.texture_start_158_19c[2*n]=v[0];out.texture_start_158_19c[2*n+1]=v[1];}
 }
 if(affirmative(xml.attribute("NoStateChange")))out.baseline.base_.flags_40|=0x800000;
 if(affirmative(xml.attribute("AutoSize")))out.baseline.base_.flags_40|=0x400000;
 const auto texture=xml.attribute("Texture");if(!texture.empty()){
  out.texture_key=texture;out.texture_binding_earned=false;out.texture_extent_278.reset();
 }
}
void NotificationLayoutSliderState::parse_layer_owned(NotificationLayoutBaseline& owner,const content::XmlNode& xml){
 if(const auto s=xml.attribute("WheelSteps");!s.empty())steps_628=scalar(s);
 if(const auto s=xml.attribute("Gutter");!s.empty()){
  std::int32_t value=0;const auto p=std::from_chars(s.data(),s.data()+s.size(),value);
  need(p.ec==std::errc{}&&p.ptr==s.data()+s.size()&&value>=INT16_MIN&&value<=INT16_MAX,"Slider selected signed-short Gutter parser domain unsupported");gutter_62c=static_cast<std::int16_t>(value);
 }
 parse_nine_slice_layer_owned(track_,xml);
}
std::shared_ptr<NotificationLayoutSliderState> NotificationLayoutSliderState::capture_owned(
 NotificationLayoutBaseline& owner,const std::vector<content::XmlNode>& layers,
 const std::map<std::string,content::XmlNode,std::less<>>& styles){
 need(owner.kind_==NotificationLayoutBaseline::Kind::slider,"Slider state needs actual typed baseline");
 auto result=std::shared_ptr<NotificationLayoutSliderState>(new NotificationLayoutSliderState);
 owner.base_.flags_40|=0x8000; // Actual5D0C00 ->548490 constructor.
 for(const auto& layer:layers){
  // Native style recursion reaches each genuine layer, including its actual
  // optional Shuttle descriptor, before the enclosing authored layer.
  result->parse_layer_owned(owner,layer);
  for(const auto& child:layer.children)if(child.name=="Shuttle"){
   std::vector<const content::XmlNode*> chain;std::set<std::string> seen;const auto* node=&child;
   while(!node->attribute("Style").empty()){
    const auto key=node->attribute("Style");need(seen.insert(key).second,"Slider actual Shuttle style cycle");
    const auto found=styles.find(key);need(found!=styles.end(),"Slider actual retained Shuttle style absent");node=&found->second;chain.push_back(node);
   }
   for(auto it=chain.rbegin();it!=chain.rend();++it)parse_nine_slice_layer_owned(result->shuttle_,**it);
   parse_nine_slice_layer_owned(result->shuttle_,child);
   result->shuttle_parent_earned_=true; // Genuine6FD03B embedded parent38 store.
  }
  if(affirmative(layer.attribute("Vertical"))){owner.base_.flags_40|=0x10000000;result->shuttle_.baseline.base_.flags_40|=0x10000000;result->track_.baseline.base_.flags_40|=0x10000000;}
  if(affirmative(layer.attribute("NoDefaultSound")))owner.base_.flags_40|=0x2000000;
  if(affirmative(layer.attribute("Disabled")))owner.base_.flags_40|=0x20000;
  owner.parse_layer_owned(layer); // Actual 6CAEB0 ->438170 common-base position.
  if(const auto s=layer.attribute("Length");!s.empty())owner.base_.size_30_34[(owner.base_.flags_40&0x10000000)?1:0]=scalar(s);
  result->track_.baseline.base_.size_30_34=owner.base_.size_30_34;
  if(const auto s=layer.attribute("Value");!s.empty())result->fraction_128=scalar(s);
 }
 // Parser completion is not native initialization, resource readiness or a
 // receiver return. The actual texture owner must still perform initialization.
 return result;
}
void NotificationLayoutSliderState::texture_coordinates_owned(NineSlice& s){
 need(s.texture_binding_earned,"NineSlice parsed Texture has no actual selected resource binding");
 if(!s.texture_extent_278)return;const auto dims=*s.texture_extent_278;
 need(dims[0]&&dims[1],"NineSlice actual texture extent zero");
 const float reciprocal_x=div(1.0f,static_cast<float>(dims[0])),reciprocal_y=div(1.0f,static_cast<float>(dims[1]));
 for(std::size_t n=0;n<18;++n){const float reciprocal=n%2?reciprocal_y:reciprocal_x;
  s.uv_1e8_22c[n]=mul(add(s.texture_start_158_19c[n],s.current_c0_c4[n%2]),reciprocal);
  s.uv_230_274[n]=mul(add(-s.texture_start_158_19c[n],s.texture_end_1a0_1e4[n]),reciprocal);
 }
}
void NotificationLayoutSliderState::recache_owned(NineSlice& s,bool update_coordinates){
 if(!(s.baseline.base_.flags_40&1))return;
 auto& a=s.size_110_154;auto& p=s.position_c8_10c;const auto size=s.baseline.base_.size_30_34;const auto k=s.kerning_a0_ac;
 a[2]=add(sub(add(sub(size[0],a[12]),k[0]),a[10]),k[1]);
 a[7]=add(sub(add(sub(size[1],a[11]),k[2]),a[15]),k[3]);
 a[0]=add(add(sub(sub(size[0],a[6]),a[8]),k[1]),k[0]);
 a[1]=add(add(sub(sub(size[1],a[3]),a[5]),k[3]),k[2]);
 a[9]=add(sub(add(sub(size[1],a[13]),k[2]),a[17]),k[3]);
 a[4]=add(sub(add(sub(size[0],a[16]),k[0]),a[14]),k[1]);
 p[10]=0;p[11]=0;p[2]=sub(a[10],k[0]);p[3]=0;p[12]=sub(size[0],a[12]);p[13]=0;
 p[6]=0;p[7]=sub(a[11],k[2]);p[0]=sub(a[6],k[0]);p[1]=sub(a[3],k[2]);
 p[8]=sub(size[0],a[8]);p[9]=sub(a[13],k[2]);p[14]=0;p[15]=sub(size[1],a[15]);
 p[4]=sub(a[14],k[0]);p[5]=sub(size[1],a[5]);p[16]=sub(size[0],a[16]);p[17]=sub(size[1],a[17]);
 if(update_coordinates)texture_coordinates_owned(s);
}
void NotificationLayoutSliderState::set_visual_state_owned(NineSlice& s,std::uint32_t state){
 if(s.baseline.base_.flags_40&0x800000)return;
 const auto f=static_cast<float>(state);
 s.current_c0_c4[0]=add(s.authored_b8_bc[0],mul(s.increment_b0_b4[0],f));
 s.current_c0_c4[1]=add(mul(s.increment_b0_b4[1],f),s.authored_b8_bc[1]);texture_coordinates_owned(s);
}
void NotificationLayoutSliderState::initialize_nine_slice_owned(NineSlice& s,const NotificationLayoutBaseline* parent){
 need(s.texture_binding_earned,"NineSlice actual texture parser owner has not returned");
 s.baseline.base_.flags_40|=1; // 5761E0 prefix precedes base4C/extent/bounds.
 s.baseline.base_.offset_28_2c=NotificationLayoutParentReceiver::base_anchor_coordinates(s.baseline,parent);
 s.baseline.base_.extent_20_24={extent(s.baseline.base_.margin_70_74[0],s.baseline.base_.size_30_34[0]),extent(s.baseline.base_.margin_70_74[1],s.baseline.base_.size_30_34[1])};
 s.baseline.base_.bounds_10_1c=NotificationLayoutParentReceiver::base_extent_bounds(s.baseline,{});
 need(!(s.baseline.base_.flags_40&0x400000),"NineSlice reached563190 AutoSize supplier outside selected no-AutoSize profile");
 for(std::size_t n=0;n<18;++n){s.texture_start_158_19c[n]=add(s.authored_b8_bc[n%2],s.texture_start_158_19c[n]);s.texture_end_1a0_1e4[n]=add(s.texture_start_158_19c[n],s.size_110_154[n]);}
 recache_owned(s,false); // Actual70F8E0 has inline stores; UV call comes last.
 // Actual vslot58 colour receiver is not a client callback. Selected literal
 // ctor RGBA1 is retained; unsupported authored colour is rejected by parser.
 if(s.baseline.base_.flags_40&0x800000)s.current_c0_c4=s.authored_b8_bc;
 texture_coordinates_owned(s);
}

NotificationLayoutScrollReceiver::InitializationReturn NotificationLayoutScrollReceiver::initialize_slider_owned(View& view,std::uint64_t id){
 view.require_pure();view.require_full();view.require_pure();
 view.initialize_slider_textures_owned(id);view.require_pure();
 auto& owner=*Parent::node(view,id).state;
 need(owner.kind_==NotificationLayoutBaseline::Kind::slider&&owner.slider_,"Slider init requires actual parsed typed receiver");auto& s=*owner.slider_;
 need(Parent::children(view,id).empty(),"Slider selected initializer requires actual empty ordinary-child list, excluding embedded Track/Shuttle");
 need(!s.initialization_entered_&&!s.initialization_returned_,"Slider native initialization repeated");
 need(s.shuttle_parent_earned_,"Selected actual Shuttle descriptor/parent38 has not been observed");
 view.preflight_store_owned({0,id});
 owner.record_owned(NotificationLayoutBaseline::Mutation::receiver_prefix);s.initialization_entered_=true;
 s.track_.baseline.base_.flags_40&=0xfffffc0f;Slider::initialize_nine_slice_owned(s.track_,nullptr); // Genuine Track ctorNULL, no parser parent store.
 const auto axis=(owner.base_.flags_40&0x10000000)?1u:0u;
 if(axis)s.shuttle_.baseline.base_.flags_40=(s.shuttle_.baseline.base_.flags_40&0xffffff9f)|0x10;
 else s.shuttle_.baseline.base_.flags_40=(s.shuttle_.baseline.base_.flags_40&0xfffffcff)|0x80;
 Slider::initialize_nine_slice_owned(s.shuttle_,&owner);
 s.travel_626=travel(sub(sub(owner.base_.size_30_34[axis],s.shuttle_.baseline.base_.size_30_34[axis]),static_cast<float>(std::int32_t(s.gutter_62c)*2)));
 view.record_store_owned({0,id},Parent::Store::Kind::slider);
 const auto initial_fraction=s.fraction_128;slider_set_fraction(view,id,initial_fraction);view.require_pure();
 auto& current=*Parent::node(view,id).state;current.base_.flags_40|=1;
 Parent::base_anchor(view,id);view.require_pure();
 auto& anchored=*Parent::node(view,id).state;
 anchored.base_.extent_20_24={extent(anchored.base_.margin_70_74[0],anchored.base_.size_30_34[0]),extent(anchored.base_.margin_70_74[1],anchored.base_.size_30_34[1])};
 Parent::slot94(view,id);view.require_pure();
 auto& coloured=*Parent::node(view,id).state;Slider::set_visual_state_owned(coloured.slider_->shuttle_,coloured.slider_->state_104);
 view.require_full();view.require_pure();auto& returned=*Parent::node(view,id).state;
 // Actual initializer BODY completed. The enclosing scope target has NOT
 // been destroyed yet; entered remains sticky and initialized stays false.
 return InitializationReturn(view.binding_,{0,id},returned.slider_,returned.sequence_);
}

void NotificationLayoutScrollReceiver::slot4c(View& view,std::uint64_t id){
 view.require_pure();view.require_full();view.require_pure();Parent::base_anchor(view,id);
 view.require_pure();const auto& n=Parent::node(view,id);auto& state=*n.state;
 need(state.scroll_&&state.scroll_->handles_c8_cc_d0,"Scroll slot4C actual bindings absent");
 const auto handles=*state.scroll_->handles_c8_cc_d0;
 for(const auto handle:handles)if(handle){Parent::reprocess_anchoring(view,handle);view.require_pure();}
 view.require_full();view.require_pure();
 view.record_four_c_return_owned({0,id}); // Full599810, not merely79D300 base prefix.
}
void NotificationLayoutScrollReceiver::set_size_y(View& view,std::uint64_t id,float value){
 view.require_pure();view.require_full();view.require_pure();auto& owner=*Parent::node(view,id).state;
 need(owner.kind_==NotificationLayoutBaseline::Kind::scroll_panel&&owner.scroll_&&std::isfinite(value),"Scroll40D560 requires genuine finite ScrollPanel state");
 view.preflight_store_owned({0,id});
 owner.size_y_prefix_owned(value); // Real30/34 store FIRST; no lower clamp.
 view.record_store_owned({0,id},Parent::Store::Kind::scroll_size);
 need(owner.scroll_->handles_c8_cc_d0.has_value(),"Scroll size prefix retained; actual handle bindings missing");
 const auto handles=*owner.scroll_->handles_c8_cc_d0;const auto axis=(owner.base_.flags_40&0x400000)?1u:0u;
 if(handles[2]){
  const float first=handles[0]?Parent::node(view,handles[0]).state->base_.size_30_34[axis]:0.0f;
  const float second=handles[1]?Parent::node(view,handles[1]).state->base_.size_30_34[axis]:0.0f;
  const auto length=sub(sub(owner.base_.size_30_34[axis],first),second);
  slider_set_length(view,handles[2],length);view.require_pure();
 }
 auto& current=*Parent::node(view,id).state;
 current.base_.extent_20_24={extent(current.base_.margin_70_74[0],current.base_.size_30_34[0]),extent(current.base_.margin_70_74[1],current.base_.size_30_34[1])};
 view.record_store_owned({0,id},Parent::Store::Kind::scroll_size);
 slot4c(view,id);view.require_pure();Parent::slot94(view,id);view.require_pure();view.require_full();view.require_pure();
 Parent::node(view,id).state->record_owned(NotificationLayoutBaseline::Mutation::receiver_return);
}
void NotificationLayoutScrollReceiver::internal_size(View& view,std::uint64_t id){
 view.require_pure();view.require_full();view.require_pure();auto& owner=*Parent::node(view,id).state;
 need(owner.kind_==NotificationLayoutBaseline::Kind::scroll_panel&&owner.scroll_,"Scroll62E250 requires actual typed state");
 view.preflight_store_owned({0,id});
 owner.record_owned(NotificationLayoutBaseline::Mutation::receiver_prefix);auto& s=*owner.scroll_;
 s.internal_8c_98={0.0f,owner.base_.size_30_34[0],0.0f,owner.base_.size_30_34[1]};
 // Handle absence is not an empty ordinary-child grant. Parent::children
 // verifies earned bindings and excludes only real native special handles.
 const auto ordered=Parent::children(view,id);
 for(const auto key:ordered){const auto& child=*Parent::node(view,key).state;
  if(child.base_.flags_40&2)continue;
  if(child.base_.flags_40&0x240){owner.base_.flags_40|=0x2000000;continue;}
  if(child.base_.extent_20_24[0]>s.internal_8c_98[1])s.internal_8c_98[1]=child.base_.extent_20_24[0];
  if(child.base_.extent_20_24[1]>s.internal_8c_98[3])s.internal_8c_98[3]=child.base_.extent_20_24[1];
 }
 s.ratio_a8=1.0f;const auto axis=(owner.base_.flags_40&0x400000)?1u:0u;
 const auto start=s.internal_8c_98[2*axis],end=s.internal_8c_98[2*axis+1],available=owner.base_.size_30_34[axis];
 const auto content=sub(end,start);s.range_ac=start;s.range_b0=sub(available,end);
 s.calculated_84_88=axis?std::array<float,2>{owner.base_.size_30_34[0],content}:std::array<float,2>{content,owner.base_.size_30_34[1]};
 s.ratio_a8=content==0.0f?available:div(available,content);
 need(std::isfinite(content)&&std::isfinite(s.range_b0)&&std::isfinite(s.ratio_a8),"Scroll actual FP exceptional environment required after axis/A8 prefix");
 if(s.ratio_a8<0.699999988079071f){need(content!=0,"Scroll B4 division by zero actual FP environment unavailable");s.step_b4=div(mul(available,0.10000000149011612f),content);}else s.step_b4=0.5f;
 need(s.handles_c8_cc_d0.has_value(),"Scroll actual c8/cc/d0 binding missing after step prefix");
 const auto handles=*s.handles_c8_cc_d0;const auto ratio=s.ratio_a8,step=s.step_b4;
 view.record_store_owned({0,id},Parent::Store::Kind::scroll_internal);
 if(handles[2]){slider_set_ratio(view,handles[2],ratio);view.require_pure();reciprocal_steps(view,handles[2],step);view.require_pure();}
 const bool one=ratio>=1.0f||ratio==0.0f;
 if(Parent::node(view,id).state->base_.flags_40&0x800000){
  for(const auto handle:handles)if(handle){slot88(view,handle,one);view.require_pure();}
 }else{
  auto& flags=Parent::node(view,id).state->base_.flags_40;
  if(one)flags|=0x1000000;else flags&=0xfeffffff;
 }
 const auto supplied=Parent::node(view,id).state->scroll_->fraction_a4;
 need(std::isfinite(supplied),"Scroll5BE050 nonfinite native comparison domain unavailable");
 if(supplied<0.0f||supplied>1.0f){view.scroll_fraction_diagnostic_owned(id,supplied);view.require_pure();}
 auto& fraction=*Parent::node(view,id).state->scroll_;
 fraction.fraction_a4=supplied<0.0f?0.0f:(supplied>1.0f?1.0f:supplied);
 need(fraction.range_ac==0.0f,"Scroll x87 fraction nonzero origin outside earned62E250 arithmetic domain");
 volatile double product=double(fraction.range_b0)*double(fraction.fraction_a4); // Earned AC is literal XORPS zero.
 volatile double expression=product+double(fraction.range_ac);
 fraction.offset_9c_a0[axis]=static_cast<float>(std::floor(expression));
 need(std::isfinite(fraction.offset_9c_a0[axis]),"Scroll native floor FP environment unavailable after offset store");
 view.record_store_owned({0,id},Parent::Store::Kind::scroll_internal);
 if(handles[2]){slider_set_fraction(view,handles[2],supplied);view.require_pure();}
 view.require_full();view.require_pure();Parent::node(view,id).state->record_owned(NotificationLayoutBaseline::Mutation::receiver_return);
}
void NotificationLayoutScrollReceiver::slider_set_length(View& view,std::uint64_t id,float value){
 view.require_pure();auto& owner=*Parent::node(view,id).state;
 need(owner.kind_==NotificationLayoutBaseline::Kind::slider&&owner.slider_&&owner.slider_->initialization_returned_,"Slider505ED0 needs earned actual initialized slider");
 view.preflight_store_owned({0,id});
 auto& s=*owner.slider_;owner.record_owned(NotificationLayoutBaseline::Mutation::receiver_prefix);
 const auto axis=(owner.base_.flags_40&0x10000000)?1u:0u;
 owner.base_.size_30_34[axis]=value;
 s.travel_626=travel(sub(sub(value,s.shuttle_.baseline.base_.size_30_34[axis]),static_cast<float>(std::int32_t(s.gutter_62c)*2)));
 s.track_.baseline.base_.size_30_34=owner.base_.size_30_34;Slider::recache_owned(s.track_);
 owner.base_.extent_20_24={extent(owner.base_.margin_70_74[0],owner.base_.size_30_34[0]),extent(owner.base_.margin_70_74[1],owner.base_.size_30_34[1])};
 view.record_store_owned({0,id},Parent::Store::Kind::slider);
 Parent::base_anchor(view,id);view.require_pure();Parent::slot94(view,id);view.require_pure();view.require_full();view.require_pure();
 Parent::node(view,id).state->record_owned(NotificationLayoutBaseline::Mutation::receiver_return);
}
void NotificationLayoutScrollReceiver::slider_set_fraction(View& view,std::uint64_t id,float supplied){
 view.require_pure();auto& owner=*Parent::node(view,id).state;
 need(owner.slider_&&(owner.slider_->initialization_returned_||owner.slider_->initialization_entered_)&&std::isfinite(supplied),"Slider42F430 actual initialization scope finite state absent");
 view.preflight_store_owned({0,id});
 auto& s=*owner.slider_;owner.record_owned(NotificationLayoutBaseline::Mutation::receiver_prefix);
 const float clamped=supplied<0.0f?0.0f:(supplied>1.0f?1.0f:supplied);s.fraction_128=clamped;
 volatile double expression=double(s.travel_626)*double(clamped)+double(s.gutter_62c);
 const float coordinate=static_cast<float>(std::floor(expression));const auto axis=(owner.base_.flags_40&0x10000000)?1u:0u;
 const std::array<float,2> position=axis?std::array<float,2>{0,coordinate}:std::array<float,2>{coordinate,0};
 auto& shuttle=s.shuttle_.baseline;
 // 69CD90 compares requested floats BEFORE floor/stores and skips ONLY actual
 // equality. The embedded subcontrol remains owned by this genuine slider.
 if(shuttle.base_.margin_70_74!=position){
  shuttle.base_.margin_70_74={static_cast<float>(std::floor(double(position[0]))),static_cast<float>(std::floor(double(position[1])))};
  shuttle.base_.extent_20_24={extent(shuttle.base_.margin_70_74[0],shuttle.base_.size_30_34[0]),extent(shuttle.base_.margin_70_74[1],shuttle.base_.size_30_34[1])};
  shuttle.base_.offset_28_2c=Parent::base_anchor_coordinates(shuttle,&owner);
  shuttle.base_.bounds_10_1c=Parent::base_extent_bounds(shuttle,{});
 }
 view.record_store_owned({0,id},Parent::Store::Kind::slider);
 view.require_full();view.require_pure();Parent::node(view,id).state->record_owned(NotificationLayoutBaseline::Mutation::receiver_return);
}
void NotificationLayoutScrollReceiver::slider_set_ratio(View& view,std::uint64_t id,float ratio){
 view.require_pure();auto& owner=*Parent::node(view,id).state;
 need(owner.slider_&&owner.slider_->initialization_returned_&&std::isfinite(ratio),"Slider58EE20 actual initialized ratio supplier absent");
 view.preflight_store_owned({0,id});
 owner.record_owned(NotificationLayoutBaseline::Mutation::receiver_prefix);auto& s=*owner.slider_;
 const auto axis=(owner.base_.flags_40&0x10000000)?1u:0u;
 // Actual receiver is the embedded Shuttle +12C, not the slider or Track.
 s.shuttle_.baseline.base_.size_30_34[axis]=mul(owner.base_.size_30_34[axis],ratio);Slider::recache_owned(s.shuttle_);
 s.travel_626=travel(sub(sub(owner.base_.size_30_34[axis],s.shuttle_.baseline.base_.size_30_34[axis]),static_cast<float>(std::int32_t(s.gutter_62c)*2)));
 view.record_store_owned({0,id},Parent::Store::Kind::slider);
 const auto fraction=s.fraction_128;slider_set_fraction(view,id,fraction);view.require_pure();view.require_full();view.require_pure();
 Parent::node(view,id).state->record_owned(NotificationLayoutBaseline::Mutation::receiver_return);
}
void NotificationLayoutScrollReceiver::reciprocal_steps(View& view,std::uint64_t id,float step){
 view.require_pure();auto& owner=*Parent::node(view,id).state;need(owner.slider_&&std::isfinite(step)&&step!=0,"Slider628 reciprocal requires supported nonzero native FP domain");
 view.preflight_store_owned({0,id});
 // Binary32 step is an exact integer significand times a power of two.
 // Integer division preserves x87 truncation in this supported signed31-bit
 // quotient domain even on Windows where C++ long double is binary64.
 const auto bits=std::bit_cast<std::uint32_t>(step),exponent=(bits>>23)&255u;
 need(exponent!=0,"Slider628 subnormal reciprocal requires actual exceptional FP environment");
 const auto significand=(bits&0x7fffffu)|0x800000u;const int shift=150-int(exponent);
 need(shift<=63,"Slider628 reciprocal outside supported native signed31-bit quotient domain");
 const auto magnitude=shift<0?std::uint64_t{0}:(std::uint64_t{1}<<shift)/significand;
 need(magnitude<=INT32_MAX,"Slider628 reciprocal integer domain exceeds selected finite profile");
 const auto integer=(bits>>31)?-static_cast<std::int64_t>(magnitude):static_cast<std::int64_t>(magnitude);
 const auto low=static_cast<std::uint32_t>(static_cast<std::uint64_t>(integer));
 owner.record_owned(NotificationLayoutBaseline::Mutation::receiver_prefix);owner.slider_->steps_628=static_cast<float>(low);
 view.record_store_owned({0,id},Parent::Store::Kind::slider);
 owner.record_owned(NotificationLayoutBaseline::Mutation::receiver_return);
}
void NotificationLayoutScrollReceiver::slider_slot88(View& view,std::uint64_t id,bool disabled){
 view.require_pure();auto& owner=*Parent::node(view,id).state;
 need(owner.slider_&&owner.slider_->initialization_returned_,"Slider slot88 actual initialized typed receiver absent");
 if(bool(owner.base_.flags_40&0x20000)==disabled)return;
 view.preflight_store_owned({0,id});
 owner.record_owned(NotificationLayoutBaseline::Mutation::receiver_prefix);auto& s=*owner.slider_;
 s.state_104=disabled?3u:0u;Slider::set_visual_state_owned(s.shuttle_,s.state_104);
 view.record_store_owned({0,id},Parent::Store::Kind::slider);
 if(disabled){owner.base_.flags_40|=0x20000;
  if(owner.base_.flags_40&0x10000){owner.base_.flags_40&=0xfffeffff;view.disable_hovered_receiver_owned(id);view.require_pure();}
 }else owner.base_.flags_40&=0xfffdffff;
 // GUI13B4 eligibility and persistent cached W/L are Root's actual GUI/window
 // owner dependency. This is an authentic mandatory dispatch, not a no-op.
 view.post_mouse_rehit_owned(id);view.require_pure();view.require_full();view.require_pure();
 Parent::node(view,id).state->record_owned(NotificationLayoutBaseline::Mutation::receiver_return);
}
void NotificationLayoutScrollReceiver::slot88(View& view,std::uint64_t id,bool disabled){
 view.require_pure();const auto kind=Parent::node(view,id).state->kind_;
 if(kind==NotificationLayoutBaseline::Kind::slider){slider_slot88(view,id,disabled);return;}
 auto& owner=*Parent::node(view,id).state;
 need(kind==NotificationLayoutBaseline::Kind::button&&owner.button_,"Scroll slot88 requires genuine Up/DownButton orSlider receiver");
 if(bool(owner.base_.flags_40&0x20000)==disabled)return;
 view.preflight_store_owned({0,id});
 owner.record_owned(NotificationLayoutBaseline::Mutation::receiver_prefix);
 NotificationLayoutButtonState::visual_state_owned(owner,disabled?3u:0u);
 view.record_store_owned({0,id},Parent::Store::Kind::slider);
 if(disabled){owner.base_.flags_40|=0x20000;
  if(owner.base_.flags_40&0x10000){owner.base_.flags_40&=0xfffeffff;view.disable_hovered_receiver_owned(id);view.require_pure();}
 }else owner.base_.flags_40&=0xfffdffff;
 view.post_mouse_rehit_owned(id);view.require_pure();view.require_full();view.require_pure();
 Parent::node(view,id).state->record_owned(NotificationLayoutBaseline::Mutation::receiver_return);
}
}
