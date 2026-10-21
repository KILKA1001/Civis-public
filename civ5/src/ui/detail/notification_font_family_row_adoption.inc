// Genuine live row family. Actual Source/Gui reentry guards enclose both queries.
request->require_pending_query_owned(operation,*resource);pure();
auto observation=operation.font_texture_query_owned(*resource,*request);
require(bool(observation),"Font base provider has no actual observation");
observation->require_current_owned(operation,*resource,*request);pure();
const bool base_null=observation->observed_null_owned();
observation->require_current_owned(operation,*resource,*request);pure();
const auto base=observation->borrower_owned();
observation->require_current_owned(operation,*resource,*request);pure();
require(base_null?!base:bool(base),"Font base outcome/custody inconsistent");
family->expected_base=base;family->expected_active=base;
family->returned(false);
std::shared_ptr<const graphics::detail::BnwFontTextureBorrower> active=base;
std::unique_ptr<detail::BnwFontTextureRequest> index_request;
std::unique_ptr<graphics::detail::BnwFontTextureObservation> index_observation;
std::int32_t marker=0;
if(!base_null){
 base->require_current_owned(operation,*resource,*request);pure();
 const auto base_quote=base->captured_source_owned(operation,*resource,*request);
 const auto bank_quote=resource->original_texture_source_owned(operation,*request);
 require(base_quote.bytes&&bank_quote.bytes&&base_quote.bytes.get()==bank_quote.bytes.get()&&
  !base_quote.bytes.owner_before(bank_quote.bytes)&&!bank_quote.bytes.owner_before(base_quote.bytes)&&
  base_quote.payload_identity.sha256==bank_quote.payload_identity.sha256&&base_quote.payload_identity.bytes==bank_quote.payload_identity.bytes,
  "Font index metadata is not actualBASE canonicalBank payload");
 base->require_current_owned(operation,*resource,*request);pure();
 const auto filename=detail::bnw_native_font_optional_filename(family->original_src);
 marker=detail::bnw_native_font_bc_marker(*base_quote.bytes);
 base->require_current_owned(operation,*resource,*request);pure();
 family->first();require(!family->derived&&!family->index,"Font derived index replay");
 family->derived.emplace(detail::NotificationContextFontRegistryFields::FontObject::PendingRowFamily::DerivedIndex{
  filename,marker,base_quote.payload_identity});
 actual_font.optional_metadata.emplace_back(filename,marker,base_quote.payload_identity);
 if(marker!=0){
  // Only the actual owned reached metadata slot can select indexrole/key.
  // No supplied key/bool/token/default factory is accepted by the constructor.
  request->require_current_owned(operation,*resource);family->first();pure();
  require(registry->request_sequence<UINT64_MAX,"Font index occurrence exhausted");
  const auto index_occurrence=++registry->request_sequence;
  index_request=std::unique_ptr<detail::BnwFontTextureRequest>(new detail::BnwFontTextureRequest(
   std::make_unique<detail::BnwFontTextureRequest::Impl>(facade,saved,registry,*resource,*first_texture,
    registry->pending.back().key,serial,index_occurrence,family->derived->filename,copied_name,allow,&*family->derived)));
  index_request->require_pending_query_owned(operation,*resource);request->require_current_owned(operation,*resource);pure();
  index_observation=operation.font_texture_query_owned(*resource,*index_request);
  require(bool(index_observation),"Font optional provider has no actual observation");
  index_observation->require_current_owned(operation,*resource,*index_request);pure();
  const bool index_null=index_observation->observed_null_owned();
  index_observation->require_current_owned(operation,*resource,*index_request);pure();
  active=index_observation->borrower_owned();
  index_observation->require_current_owned(operation,*resource,*index_request);pure();
  require(index_null?!active:bool(active),"Font optional outcome/custody inconsistent");
  family->expected_active=active;
  family->returned(true); // Genuine optionalNULL has no second warning.
 }
}
family->all_queries_returned();
if(base)base->require_current_owned(operation,*resource,*request);
if(active&&index_request)active->require_current_owned(operation,*resource,*index_request);
pure();
// Query outcomes/metadata read temporaries retire before row append. Bothreal
// requests remain alive for fresh individual cache/Source/Bank POST validation.
index_observation.reset();observation.reset();pure();
if(base)base->require_current_owned(operation,*resource,*request);
if(active&&index_request)active->require_current_owned(operation,*resource,*index_request);
request->require_current_owned(operation,*resource);
if(index_request)index_request->require_current_owned(operation,*resource);
family->all_queries_returned();pure();
detail::NotificationContextFontRegistryFields::FontObject::Texture row;
row.name.fill('\0');std::copy(copied_name.begin(),copied_name.end(),row.name.begin());
row.active=active;row.base=base;row.allow_color=allow;row.bc=static_cast<float>(marker);
row.request_occurrence=occurrence;row.index_occurrence=index_request?index_request->texture_occurrence_owned(operation,*resource):0;
row.source_serial=serial;row.ordinal=family->ordinal;
require(actual_font.texture_store_sequence<UINT64_MAX,"Font native row ledger exhausted");
row.store_sequence=actual_font.texture_store_sequence+1;
actual_font.textures_40c2c->push_back(std::move(row));++actual_font.texture_store_sequence;
family->row_stored=true;
request->seal_native_row_append_owned(operation,*resource);
if(index_request)index_request->seal_native_row_append_owned(operation,*resource);
if(base)base->require_current_owned(operation,*resource,*request);
if(active&&index_request)active->require_current_owned(operation,*resource,*index_request);
pure();record("texture.native80.family.row.append",true,64);
// index_request then base request/source/name retire when lexicalscope ends.
