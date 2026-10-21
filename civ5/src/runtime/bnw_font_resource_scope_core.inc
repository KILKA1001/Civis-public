// Inside runtime with the complete canonical Source::State.
detail::BnwFontResourceLoan::BnwFontResourceLoan(
 std::shared_ptr<const BnwCivisOwnedStockSourceBundle> bundle,
 std::weak_ptr<BnwSelectedWorldViewContextOwner> source,std::uint64_t serial,std::uint64_t generation,std::uint64_t scope,
 std::string request,assets::CivisLoadedResource loaded)
 :bundle_(std::move(bundle)),source_(std::move(source)),serial_(serial),generation_(generation),scope_(scope),
 request_(std::move(request)),loaded_(std::move(loaded)){
 if(!loaded_.bytes||!loaded_.xml)throw ScriptError("Font owned parser source not genuinely staged");
 parser_document_=std::make_shared<const content::XmlNode>(*loaded_.xml);
 // Pure portable parser allocation. No callbacks, hooks, path reopen or Bank mutation.
}
void detail::BnwFontResourceLoan::require_current_owned(const BnwWorldViewSourceOperationLoan& operation)const{
 life_->first(1);
 if(creator_!=std::this_thread::get_id())throw ScriptError("Font resource loan foreign creator");
 operation.require_origin_without_callbacks();
 const auto actual=source_.lock();require(bool(actual)&&bool(bundle_)&&generation_,"Font resource Source/bundle retired");
 actual->require_operation_origin_without_callbacks(operation,scope_);
 require(actual->state_->native_generation==generation_&&actual->state_->civis_sources.get()==bundle_.get()&&
  !actual->state_->civis_sources.owner_before(bundle_)&&!bundle_.owner_before(actual->state_->civis_sources),"Font canonical bundle/generation replaced");
 operation.require_operation(serial_);
 actual->require_sources_current();operation.require_operation(serial_);
 bundle_->require_current();operation.require_operation(serial_);
 require(loaded_.bytes&&loaded_.xml&&parser_document_&&parser_document_.get()!=loaded_.xml.get(),"Font source/transient parser was not genuinely captured/constructed");
 require(identify_bytes(*loaded_.bytes).sha256==loaded_.payload_identity.sha256,"Font captured source bytes changed");
}
#include "bnw_font_resource_bank_views.inc"
const content::XmlNode& detail::BnwFontResourceLoan::xml_owned(const BnwWorldViewSourceOperationLoan& op)const{
 require_current_owned(op);return *parser_document_;
}
std::unique_ptr<detail::BnwFontResourceLoan> BnwSelectedWorldViewContextOwner::font_resource_loan_owned(
 const BnwWorldViewSourceOperationLoan& operation,std::string request)const{
 const auto saved=state_;const auto scope=saved->scope_serial;
 try{
  const auto pure=[&]{require_operation_origin_without_callbacks(operation,scope);
   const auto actual=saved->canonical_owner.lock();require(actual&&actual.get()==this&&actual->state_.get()==saved.get(),"Font canonical Source owner changed");};
  pure();const auto serial=operation.operation_serial();
  require(saved->civis_sources&&saved->native_generation,"Font requires canonical current Civis Source bundle");
  saved->current();pure();saved->civis_sources->require_current();pure();
  // Memory-only realBank entry. Missing/unloaded/ambiguous is a supplier error,
  // not native parse-false or permission to reopen a physical file.
  auto loaded=saved->civis_sources->loaded(request+".ggxml");pure();
  require(loaded.xml&&loaded.bytes,"Font GGXML was not staged before callback");
  auto loan=std::unique_ptr<detail::BnwFontResourceLoan>(new detail::BnwFontResourceLoan(
   saved->civis_sources,saved->canonical_owner,serial,saved->native_generation,scope,std::move(request),std::move(loaded)));
  pure();loan->require_current_owned(operation);pure();return loan;
 }catch(...){saved->failed.store(true);saved->lease.invalidate();throw;}
}
