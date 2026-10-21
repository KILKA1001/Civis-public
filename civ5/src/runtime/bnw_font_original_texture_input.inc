assets::CivisLoadedResource detail::BnwFontResourceLoan::original_texture_source_owned(
 const BnwWorldViewSourceOperationLoan& op,const ui::detail::BnwFontTextureRequest& request)const{
 require_current_owned(op);request.require_current_owned(op,*this);
 require(!request.optional_index_owned(op,*this),"Font BC input must use real original base src, not optional file");
 const std::string original_src=request.source_owned(op,*this);
 require_current_owned(op);request.require_current_owned(op,*this);
 // Canonical actual queried payload: memory-only selected Bank capability.
 // Missing/indexed-unloaded/ambiguous is STOP, never guessed nativeReadfalse0.
 auto loaded=bundle_->loaded(original_src);
 require_current_owned(op);request.require_current_owned(op,*this);
 require(bool(loaded.bytes)&&identify_bytes(*loaded.bytes).sha256==loaded.payload_identity.sha256,
  "Font original-source actual winner bytes unavailable/changed");
 return loaded;
}
