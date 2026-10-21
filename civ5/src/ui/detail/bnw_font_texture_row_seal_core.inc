void detail::BnwFontTextureRequest::require_current_owned(const runtime::BnwWorldViewSourceOperationLoan& op,
 const runtime::detail::BnwFontResourceLoan& loan)const{
 life_->require_current_owned(life_->occurrence_);
 impl_->require_storage_current_owned(op,loan,impl_->adopted_store.has_value());
}
void detail::BnwFontTextureRequest::require_pending_query_owned(const runtime::BnwWorldViewSourceOperationLoan& op,
 const runtime::detail::BnwFontResourceLoan& loan)const{
 life_->require_current_owned(life_->occurrence_);
 require(!impl_->adopted_store,"Font texture native row sealed; no fresh9AF940 query permission");
 const auto f=impl_->family.lock();require(bool(f),"Font query family retired");
 f->pending_query(impl_->role==Impl::Role::optional_index);
 impl_->require_storage_current_owned(op,loan,false);
}
void detail::BnwFontTextureRequest::seal_native_row_append_owned(const runtime::BnwWorldViewSourceOperationLoan& op,
 const runtime::detail::BnwFontResourceLoan& loan){
 life_->require_current_owned(life_->occurrence_);
 require(!impl_->adopted_store,"Font native row adoption replay");
 // FIRST/fences, actual vector/row/store history and callbacks precede seal.
 // This verifies after-shape directly; it does not move a public expectedcount.
 impl_->require_storage_current_owned(op,loan,true);
 impl_->adopted_store=impl_->before_texture_store_sequence+1;
 require_current_owned(op,loan);
}
