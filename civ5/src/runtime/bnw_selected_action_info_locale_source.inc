// Inside runtime, after actual Source State and ordinary Tooltip supplier.
std::string BnwSelectedWorldViewContextOwner::action_info_localized_text_owned(
 const BnwWorldViewSourceOperationLoan& operation,std::string_view input)const{
 const auto saved=state_;operation.require_origin_without_callbacks();
 const auto scope=saved->scope_serial;const std::string key(input);
 const auto pure=[&]{require_operation_origin_without_callbacks(operation,scope);};
 try{
  pure();require(saved->tooltip_locale_bound&&!saved->tooltip_locale.expired()&&!saved->tooltip_querying,
   "ActionInfo genuine selected Locale unbound/retired/reentered");
  // Empty is an actual SDK Lookup input. NULL Description is skipped by the
  // actual cache producer before this seam; it is not converted to this branch.
  require(key.size()<=4096&&key.find('\0')==std::string::npos,"ActionInfo localized key malformed");
  saved->tooltip_querying=true;
  struct End{std::shared_ptr<State> saved;~End(){saved->tooltip_querying=false;}}end{saved};
  const auto engine=saved->tooltip_locale;
  auto result=ui::BnwEngineUiTurnOwner::selected_action_info_lookup_from_owned_weak(engine,key);
  pure();require(!engine.expired()&&!engine.owner_before(saved->tooltip_locale)&&
   !saved->tooltip_locale.owner_before(engine),"ActionInfo actual Locale owner changed");
  require(result.find('\0')==std::string::npos,"ActionInfo actual UTF8 buffer containsNUL");
  pure();return result;
 }catch(...){saved->failed.store(true);throw;}
}
