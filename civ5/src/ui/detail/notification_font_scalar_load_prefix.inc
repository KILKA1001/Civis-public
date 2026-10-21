// Inside GuiPort::begin_font_lookup_owned AFTER actual Font ctor pending.
require(copied_request.has_value(),"Font actual copied request C-string missing before loader");
pending_font_cut();auto& actual_font=*registry->pending[pending_index].font;
actual_font.load_history.reserve(32);pending_font_cut();
auto resource=operation.font_resource_loan_owned(*copied_request);
pending_font_cut();resource->require_current_owned(operation);pending_font_cut();
const auto resource_allocation=resource.get();
const std::weak_ptr<runtime::detail::BnwFontResourceLoan::Life> resource_life=resource->life_;
const auto resource_epoch=resource->life_->epoch.load();
const auto resource_cut=[&]{
 const auto first=resource_life.lock();require(bool(first),"Font XML scope fence expired");first->first(resource_epoch);
 pending_font_cut();first->first(resource_epoch);
 require(resource.get()==resource_allocation&&resource->life_.get()==first.get()&&
  !resource->life_.owner_before(first)&&!first.owner_before(resource->life_),"Font XML scope allocation/control substituted");
 resource->require_current_owned(operation);first->first(resource_epoch);pending_font_cut();first->first(resource_epoch);
};
resource_cut();
// Quote data stays local through loan DTOR and actual postchecks. It is never
// a completed Font lender, parser-success receipt or UI publication.
detail::NotificationContextFontRegistryFields::FontObject::ResourceQuote quote{
 resource->bundle_,resource->loaded_,resource->request_};
resource_cut();
std::exception_ptr partial_failure;
try{
 resource_cut();const auto& root=resource->xml_owned(operation);resource_cut();
 struct OwnedParserCursor final {const std::thread::id creator=std::this_thread::get_id();
 bool alive=true;const content::XmlNode* node=nullptr;
 const content::XmlNode& current()const{require(creator==std::this_thread::get_id()&&alive&&node,"Font owned parser cursor retired/foreign/empty");return *node;}
 ~OwnedParserCursor(){alive=false;node=nullptr;}};
 std::array<std::unique_ptr<OwnedParserCursor>,6> parser_cursors;
 for(auto& cursor:parser_cursors){cursor=std::make_unique<OwnedParserCursor>();resource_cut();}
 parser_cursors[0]->node=&root;
 const auto record=[&](std::string key,bool result,std::uint32_t offset){
  resource_cut();actual_font.load_history.reserve(actual_font.load_history.size()+1);resource_cut();
  actual_font.load_history.push_back({std::move(key),result,offset});resource_cut();};
 const auto child=[&](const content::XmlNode& node,std::string_view name)->const content::XmlNode*{
  resource_cut();parser_cursors[5]->node=&node;
  for(const auto& c:parser_cursors[5]->current().children)if(c.name==name)return &c;return nullptr;};
 const content::XmlNode* textures_operand=nullptr;const content::XmlNode* glyphs_operand=nullptr;
 const auto attribute=[&](std::string_view name)->const std::string*{
  resource_cut();const auto supplier=(name=="height"||name=="width")?textures_operand:glyphs_operand;
  if(!supplier)return nullptr;
  parser_cursors[4]->node=supplier;
  for(const auto& a:parser_cursors[4]->current().attributes)if(a.first==name)return &a.second;return nullptr;};
 const auto owned_xml_text=[&](const std::string* value)->std::string{
  resource_cut();require(bool(value),"Font owned XML copy missing operand");
  const auto count=value->size();std::string owned;owned.resize(count);resource_cut();
  require(value->size()==count,"Font immutable XML text changed across allocation");
  std::copy(value->begin(),value->end(),owned.begin());resource_cut();return owned;
 };
 const auto query=[&](std::string_view name)->std::string{
  resource_cut();
  const auto value=attribute(name);record(std::string(name)+".query",bool(value),0);
  if(!value)throw std::runtime_error("Font672FE0 missing "+std::string(name)+": genuine native warning/false-return cleanup receiver required");
  return owned_xml_text(value);};
 const auto local=[&](std::string_view name)->std::int32_t{
  const auto& text=query(name);std::int32_t result;
  const auto converted=detail::bnw_native_font_integer(text,result);
  record(std::string(name)+".integer",converted,0);
  if(!converted)throw std::runtime_error("Font835330 false leaves uninitialized native local "+std::string(name)+"; no guessed value");
  return result;};
 const auto scalar=[&](std::string_view name,std::size_t index){
  const auto& text=query(name);auto value=std::bit_cast<std::int32_t>(actual_font.scalar_40c08_40c18[index]);
  const auto converted=detail::bnw_native_font_integer(text,value);
  // Native ignores this bool. ASCII rejection produces false and NO store;
  // ctor value remains known. Malformed/overflow stream domain stops before
  // a current-library failure write can masquerade as nativeMSVC90 behavior.
  resource_cut();
  if(converted)actual_font.scalar_40c08_40c18[index]=std::bit_cast<std::uint32_t>(value);
  record(std::string(name)+".integer",converted,static_cast<std::uint32_t>(0x40c08+4*index));
  resource_cut();};
 resource_cut();std::string filename=*copied_request+".ggxml";resource_cut();
 auto separator=filename.find_last_of('\\');if(separator==filename.npos)separator=filename.find(':');
 std::string basename=separator==filename.npos?filename:filename.substr(separator+1);resource_cut();
 *actual_font.name_40c1c=std::move(basename);resource_cut();
 record("name.81DA10",true,0x40c1c);
 const auto textures=child(parser_cursors[0]->current(),"textures"),styles=child(parser_cursors[0]->current(),"styles"),glyphs=child(parser_cursors[0]->current(),"glyphs");
 textures_operand=textures;glyphs_operand=glyphs;
 parser_cursors[1]->node=textures;parser_cursors[2]->node=styles;parser_cursors[3]->node=glyphs;
 record("textures.query",bool(textures),0);record("styles.query",bool(styles),0);record("glyphs.query",bool(glyphs),0);
 const auto spacing=local("spacing");actual_font.spacing=spacing;
 scalar("descent",1);scalar("ascent",0);
 actual_font.scalar_40c08_40c18[2]=actual_font.scalar_40c08_40c18[0]+actual_font.scalar_40c08_40c18[1];
 record("unadjusted.lineheight",true,0x40c10);
 const auto ascadj=attribute("ascadj");record("ascadj.query",bool(ascadj),0);
 std::int32_t adjustment=2; // Explicit original absent-attribute branch only.
 if(ascadj){const auto owned_adjustment=owned_xml_text(ascadj);
  const auto converted=detail::bnw_native_font_integer(owned_adjustment,adjustment);resource_cut();
  record("ascadj.integer",converted,0);
  if(!converted)throw std::runtime_error("Font835330 false ascadj local unknown; absent-default branch not applicable");}
 actual_font.scalar_40c08_40c18[0]-=std::bit_cast<std::uint32_t>(adjustment);
 record("adjusted.ascent",true,0x40c08);
 actual_font.texture_height=local("height");actual_font.texture_width=local("width");
 const auto first= textures?child(*textures,"texture"):nullptr;record("texture.first.query",bool(first),0);
 if(!first)throw std::runtime_error("Font672FE0 no texture entries: genuine warning/false-return cleanup receiver required");
 for(std::size_t texture_cursor=0;texture_cursor<parser_cursors[1]->current().children.size();++texture_cursor){
  const auto* first_texture=&parser_cursors[1]->current().children[texture_cursor];if(first_texture->name!="texture")continue;
  resource_cut();
  #include "notification_font_texture_request_start.inc"
  resource_cut();
  bool next=false;for(std::size_t i=texture_cursor+1;i<textures->children.size();++i)if(textures->children[i].name=="texture"){next=true;break;}
  record("texture.next.query",next,0);
 }
 #include "notification_font_style_glyph_loops.inc"
 // Original iterator DTOR8353F0 has only cursor-vtable teardown. Portable
 // cursor objects are real allocations and retire before parser/loan cleanup.
 for(auto& cursor:parser_cursors){cursor->alive=false;cursor->node=nullptr;cursor.reset();}
 record("loader.portable.cursor.cleanup",true,0);
}catch(...){
 partial_failure=std::current_exception();
 // Cleanup never calls Source; independent allocation Life FIRST protects
 // the raw retained Font even if the original Source guard just failed.
 try{const auto first=font_life.lock();require(bool(first),"Font cleanup allocation retired");first->first(font_epoch);
  require(pending_index<registry->pending.size()&&registry->pending[pending_index].font.get()==font_allocation,
   "Font cleanup pending owner replaced");
  if(actual_font.pending_row_family){actual_font.pending_row_family->life->failed.store(true);
   actual_font.pending_row_family->retire();actual_font.pending_row_family.reset();}
 }catch(...){/* A retired allocation must not be dereferenced for cleanup. */}
}
resource.reset(); // Portable parser cleanup: retire loan FIRST, free owned cloned tree; immutable Bank quote retained.
pending_font_cut();
quote.bundle->require_current();pending_font_cut();
actual_font.resource_quote.emplace(std::move(quote));pending_font_cut();
if(partial_failure)std::rethrow_exception(partial_failure);
require(actual_font.textures_40c2c&&!actual_font.textures_40c2c->empty()&&actual_font.styles_40c20&&
 !actual_font.styles_40c20->empty()&&actual_font.glyphs_40c38&&!actual_font.glyphs_40c38->empty()&&
 actual_font.glyph_lookup_40c44&&!actual_font.pending_row_family&&!actual_font.completion,
 "Font actual complete loader shape missing");
actual_font.completion.emplace(detail::NotificationContextFontRegistryFields::FontObject::CompletedShape{
 serial,saved->generation,actual_font.textures_40c2c->size(),actual_font.styles_40c20->size(),actual_font.glyphs_40c38->size()});
pending_font_cut();
