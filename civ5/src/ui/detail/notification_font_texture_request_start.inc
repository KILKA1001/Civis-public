{
resource_cut();actual_font.texture_returns.reserve(actual_font.texture_returns.size()+1);resource_cut();
// Inside the live Source2 try, after first actual texture node query.
const auto row_attribute=[&](std::string_view name)->std::string{
 resource_cut();
 parser_cursors[4]->node=first_texture;
 const std::string* value=nullptr;for(const auto& a:parser_cursors[4]->current().attributes)if(a.first==name){value=&a.second;break;}
 record(std::string("texture.")+std::string(name)+".query",bool(value),0);
 //8355C0 false retains previous attr iterator. This limited selected profile
 // refuses before converting unproven retained/uninitialized operands.
 if(!value)throw std::runtime_error("Font538600 missing texture attribute; retained-iterator grammar unimplemented");
 const auto owned=owned_xml_text(value);
 for(unsigned char c:owned)if(c&0x80)throw std::runtime_error("Font538600 ASCII string conversion false/uninitialized operand unsupported");
 return owned;};
const auto texture_name=row_attribute("name");
const auto texture_source=row_attribute("src");
const auto& allow_text=row_attribute("allowcolor");
if(allow_text.empty())throw std::runtime_error("Font835120 empty leaves native local undefined; no default");
bool allow;
if(allow_text[0]=='t'||allow_text[0]=='T')allow=true;
else if(allow_text[0]=='f'||allow_text[0]=='F')allow=false;
else{
 std::int32_t numeric;require(detail::bnw_native_font_integer(allow_text,numeric),"Font boolean highbit conversion false");
 require(numeric==0||numeric==1,"Font MSVC90 boolean failure-write domain unsupported");
 std::stringstream stream(std::ios::in|std::ios::out);stream.write(allow_text.data(),static_cast<std::streamsize>(allow_text.size()));
 stream>>allow;require(!stream.fail()&&!stream.bad()&&allow==(numeric!=0),"Font actual boolean stream not qualified");
}
// Native strncpy64 then byte63=0: only known first63 bytes plus terminator.
resource_cut();const auto copied_name=texture_name.substr(0,63);resource_cut();
record("texture.name/allowcolor.prefix",true,72);
resource_cut();
const auto facade=saved->canonical_gui.lock();require(facade&&facade->impl_.get()==saved.get(),"Font texture actual Gui facade unavailable");
require(!actual_font.pending_row_family,"Font pending-row owner replay");
const auto new_family=std::make_shared<detail::NotificationContextFontRegistryFields::FontObject::PendingRowFamily>(
 serial,actual_font.texture_store_sequence,actual_font.textures_40c2c->size(),&actual_font,*resource,
 resource->life_,resource->life_->epoch.load(),*first_texture,resource->parser_document_,texture_source);
resource_cut();actual_font.pending_row_family=new_family;resource_cut();
const auto family=actual_font.pending_row_family;family->first();pure();
require(registry->request_sequence<UINT64_MAX,"Font texture occurrence exhausted");
const auto occurrence=++registry->request_sequence;
auto request=std::unique_ptr<detail::BnwFontTextureRequest>(new detail::BnwFontTextureRequest(
 std::make_unique<detail::BnwFontTextureRequest::Impl>(facade,saved,registry,*resource,*first_texture,
  registry->pending.back().key,serial,occurrence,texture_source,copied_name,allow)));
request->require_current_owned(operation,*resource);pure();
#include "notification_font_null_row_adoption.inc"
}
// Native538600 append precedes final source/name DTOR. The enclosed real
// Request and copied source/name have now retired; POST precedes normal return.
resource_cut();operation.require_operation(serial);pure();
require(actual_font.textures_40c2c&&actual_font.texture_store_sequence==actual_font.textures_40c2c->size(),"Font after-cleanup vector/history inconsistent");
const auto completed_family=actual_font.pending_row_family;
require(bool(completed_family),"Font after-cleanup actual row family missing");completed_family->first();
completed_family->all_queries_returned();
require(completed_family->resource==resource.get()&&completed_family->row_stored&&
 completed_family->base->life.expired()&&(!completed_family->index||completed_family->index->life.expired()),
 "Font normal row return precedes actual base/index request destruction");
actual_font.texture_returns.push_back({serial,actual_font.texture_store_sequence,actual_font.textures_40c2c->size()-1});
record("texture.native80.family.return.after.DTOR.POST",true,0);
completed_family->retire();
actual_font.pending_row_family.reset(); // Actual row requests destroyed; no XML pins survive next row.

