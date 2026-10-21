// runtime namespace; same Source2 receiver with complete bundle definition.
std::shared_ptr<const assets::CivisResourceBankOwner> detail::BnwFontResourceLoan::bank_owner_owned(
 const BnwWorldViewSourceOperationLoan& op)const{
 require_current_owned(op);auto bank=bundle_->bank_->retain_owner();require_current_owned(op);
 require(bank.get()==bundle_->bank_.get()&&!bank.owner_before(bundle_->bank_)&&!bundle_->bank_.owner_before(bank),"Font actual Bank control substituted");
 return bank;
}
const assets::CivisResourceBankCapability& detail::BnwFontResourceLoan::bank_capability_owned(
 const BnwWorldViewSourceOperationLoan& op)const{
 require_current_owned(op);bundle_->capability_.require_current();require_current_owned(op);return bundle_->capability_;
}

#include "bnw_font_original_texture_input.inc"
