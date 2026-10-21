// Inside ui, after complete EndTurnPin and genuine counted Locale helpers.
std::string BnwEngineUiTurnOwner::selected_action_info_lookup_from_owned_weak(
 const std::weak_ptr<BnwEngineUiTurnOwner>& weak,std::string_view key){
 EndTurnPin pin(weak);require_counted_localization_owner(pin);
 need(key.find('\0')==key.npos,"ActionInfo Lookup embeddedNUL key outside source subset");
 pin.engine->require_selected_localization_current();require_counted_localization_owner(pin);
 if(key.empty()){
  // Original Lookup1000ECB0 ->Language1000AD30 strlen0 ->1000AE70
  // len0 branch1000AE90 ->1000AF36 default String, no DB/grammar lookup.
  // toUTF8(bytes&,1)10008EB0 writes bytes0 and returns empty1008AF59.
  // The portable object actually owns that empty UTF8 result, under the SAME
  // current selected Locale/Engine/UI keeper. This is not a missing-key fallback.
  std::string result;
  pin.engine->require_selected_localization_current();require_counted_localization_owner(pin);
  return result;
 }
 // Nonempty action keys retain the existing actual selected TextData/DB route
 // and its grammar refusal. This finite fix does not change Tooltip behavior.
 auto result=pin.engine->selected_localization_text(key);
 require_counted_localization_owner(pin);
 need(result.find('{')==result.npos&&result.find('}')==result.npos,
  "ActionInfo selected text requires an actual argument/grammar formatter");
 pin.engine->require_selected_localization_current();require_counted_localization_owner(pin);
 return result;
}
