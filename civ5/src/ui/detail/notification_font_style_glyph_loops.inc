// Inside the actual loader try, AFTER every actual texture row returned.
resource_cut();
require(actual_font.texture_returns.size()==actual_font.textures_40c2c->size()&&
 !actual_font.pending_row_family,"Font texture loop left a live request family");
const auto required_text=[&](const content::XmlNode& node,std::string_view key)->std::string{
 resource_cut();const std::string* text=nullptr;
 parser_cursors[4]->node=&node;
 for(const auto& a:parser_cursors[4]->current().attributes)if(a.first==key){text=&a.second;break;}
 const auto node_name=owned_xml_text(&node.name);
 record(node_name+"."+std::string(key)+".query",bool(text),0);
 if(!text)throw std::runtime_error("Font actual required attribute missing; retained-iterator/warning branch unsupported");
 const auto owned=owned_xml_text(text);
 for(unsigned char c:owned)if(!c||(c&0x80))throw std::runtime_error("Font string operand outside proven selected ASCII/nonNUL grammar");
 return owned;
};
const auto glyph_integer=[&](const content::XmlNode& node,std::string_view key){
 const auto text=required_text(node,key);std::int32_t result;
 require(detail::bnw_native_font_integer(text,result),"Font glyph integer false/uninitialized operand unsupported");
 resource_cut();return result;
};
const auto first_style=styles?child(*styles,"style"):nullptr;
record("style.first.query",bool(first_style),0);
if(!first_style)throw std::runtime_error("Font no style entries; genuine warning/false-return cleanup unsupported");
for(std::size_t style_cursor=0;style_cursor<parser_cursors[2]->current().children.size();++style_cursor){
 const auto& style_node=parser_cursors[2]->current().children[style_cursor];if(style_node.name!="style")continue;
 resource_cut();
 detail::NotificationContextFontRegistryFields::FontObject::Style style;
 style.layers.fill(UINT32_MAX); // Native64086E..64087B exact initial three slots.
 {
  const auto name=required_text(style_node,"name");
  require(name.size()<64,"Font style64 nonterminated name outside proven safe selected domain");
  style.name.fill('\0');std::copy(name.begin(),name.end(),style.name.begin());
  const auto layers=child(style_node,"layers");record("style.layers.query",bool(layers),0);
  if(!layers)throw std::runtime_error("Font style no layers; genuine warning continuation unsupported");
  const auto first_layer=child(*layers,"layer");record("style.layer.first.query",bool(first_layer),0);
  if(!first_layer)throw std::runtime_error("Font style no layer entries; genuine warning continuation unsupported");
  std::size_t slot=0;
  for(std::size_t layer_cursor=0;layer_cursor<layers->children.size();++layer_cursor){
   const auto& layer=layers->children[layer_cursor];if(layer.name!="layer")continue;
   require(slot<3,"Font style native76 beyond three layer slots unsupported");
   {
    const auto texture_name=required_text(layer,"tex");std::uint32_t match=UINT32_MAX;
    require(actual_font.textures_40c2c->size()<=UINT32_MAX,"Font texture vector outside native index range");
    for(std::size_t i=0;i<actual_font.textures_40c2c->size();++i){
     const auto& texture=actual_font.textures_40c2c->at(i);
     require(texture.name[63]=='\0',"Font texture name not terminated");
     if(std::string_view(texture.name.data())==texture_name)match=static_cast<std::uint32_t>(i);
    }
    require(match!=UINT32_MAX,"Font layer texture not found; genuine warning early-return unsupported");
    style.layers[slot++]=match; // Original last case-sensitive match wins.
   }
   resource_cut();
   bool next=false;for(std::size_t i=layer_cursor+1;i<layers->children.size();++i)if(layers->children[i].name=="layer"){next=true;break;}
   record("style.layer.next.query",next,0);
  }
  // Real ordered semantic76 append occurs BEFORE style name temporary DTOR.
  resource_cut();actual_font.styles_40c20->reserve(actual_font.styles_40c20->size()+1);resource_cut();
  actual_font.styles_40c20->push_back(style);resource_cut();
  record("style.native76.append",true,0x40c20);
 }
 resource_cut();
 bool next=false;for(std::size_t i=style_cursor+1;i<styles->children.size();++i)if(styles->children[i].name=="style"){next=true;break;}
 record("style.next.query",next,0);
}
const auto first_glyph=glyphs?child(*glyphs,"glyph"):nullptr;
record("glyph.first.query",bool(first_glyph),0);
if(!first_glyph)throw std::runtime_error("Font no glyph entries; genuine warning/false-return cleanup unsupported");
const auto add32=[](std::int32_t a,std::int32_t b){return std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(a)+std::bit_cast<std::uint32_t>(b));};
const auto sub32=[](std::int32_t a,std::int32_t b){return std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(a)-std::bit_cast<std::uint32_t>(b));};
const auto offset=std::bit_cast<float>(std::uint32_t{0x3ebf7cee}); // Raw9EB1E0 ee7cbf3e, NOT 0.5.
require(actual_font.spacing&&actual_font.texture_height&&actual_font.texture_width&&
 *actual_font.texture_height!=0&&*actual_font.texture_width!=0,"Font glyph dimensions outside selected nonzero texture profile");
for(std::size_t glyph_cursor=0;glyph_cursor<parser_cursors[3]->current().children.size();++glyph_cursor){
 const auto& node=parser_cursors[3]->current().children[glyph_cursor];if(node.name!="glyph")continue;
 resource_cut();
 const auto code=glyph_integer(node,"ch");const auto u=glyph_integer(node,"u");const auto v=glyph_integer(node,"v");
 auto width=glyph_integer(node,"width");auto height=glyph_integer(node,"height");
 const auto a=glyph_integer(node,"a");const auto b=glyph_integer(node,"b");const auto c=glyph_integer(node,"c");
 const auto origin_y=glyph_integer(node,"originy"); // Original does not read originx/fudgeadv/count.
 auto padding=*actual_font.spacing;if(code==32){width=0;height=0;padding=0;}
 const float p=static_cast<float>(padding),fa=static_cast<float>(a);
 const float fw=static_cast<float>(width),fh=static_cast<float>(height);
 const float fy=static_cast<float>(origin_y),ascent=static_cast<float>(std::bit_cast<std::int32_t>(actual_font.scalar_40c08_40c18[0]));
 const float tw=static_cast<float>(*actual_font.texture_width),th=static_cast<float>(*actual_font.texture_height);
 std::array<float,9> values;
 // Preserve original wrap32 arithmetic, CVTSI2SS and SSE scalar operation order.
 values[0]=static_cast<float>(sub32(u,padding))/tw;
 values[1]=static_cast<float>(sub32(v,padding))/th;
 values[2]=static_cast<float>(add32(add32(width,u),padding))/tw;
 values[3]=static_cast<float>(add32(add32(height,v),padding))/th;
 values[4]=(fa-p)-offset;
 values[5]=(ascent+(static_cast<float>(sub32(0,origin_y))-p))-offset;
 values[6]=((fw+p)+fa)-offset;
 values[7]=(ascent+((fh-fy)+p))-offset;
 values[8]=static_cast<float>(add32(add32(c,b),a));
 detail::NotificationContextFontRegistryFields::FontObject::Glyph glyph;
 for(std::size_t i=0;i<9;++i)glyph.bits[i]=std::bit_cast<std::uint32_t>(values[i]);
 require(actual_font.glyphs_40c38->size()<UINT32_MAX,"Font glyph vector outside native ordinal range");
 const auto before=static_cast<std::uint32_t>(actual_font.glyphs_40c38->size());
 resource_cut();actual_font.glyphs_40c38->reserve(actual_font.glyphs_40c38->size()+1);resource_cut();
 actual_font.glyphs_40c38->push_back(glyph);resource_cut();
 record("glyph.native36.append",true,0x40c38);
 resource_cut();const auto ch=std::bit_cast<std::uint32_t>(code);
 const auto existing=actual_font.glyph_lookup_40c44->find(ch);
 if(existing!=actual_font.glyph_lookup_40c44->end())existing->second=before; // Genuine duplicate overwrites.
 else{
  std::map<std::uint32_t,std::uint32_t> staged;staged.emplace(ch,before);resource_cut();
  auto owned_node=staged.extract(ch);resource_cut();
  actual_font.glyph_lookup_40c44->insert(std::move(owned_node)); // Equal allocator node transfer, no allocation callback.
 }
 resource_cut();record("glyph.native.map.store",true,0x40c44);
 bool next=false;for(std::size_t i=glyph_cursor+1;i<glyphs->children.size();++i)if(glyphs->children[i].name=="glyph"){next=true;break;}
 record("glyph.next.query",next,0);
}
resource_cut();
