// State retention cannot renew the canonical Source facade. All temporary
// weak facade locks end before Engine/SQL/resource callbacks.
ui::detail::BnwLabelSystemFontLanguageData
BnwSelectedWorldViewContextOwner::system_font_language_data_from_owned_weak(
 const std::weak_ptr<BnwSelectedWorldViewContextOwner>& borrowed,
 const BnwWorldViewSourceOperationLoan& operation){
 operation.require_origin_without_callbacks();operation.require_render_operation_owned();
 const auto weak=borrowed;
 std::shared_ptr<State> saved;const BnwSelectedWorldViewContextOwner* expected=nullptr;
 {const auto actual=weak.lock();require(bool(actual),"System font actual Source facade retired");
  saved=actual->state_;expected=actual.get();}
 operation.require_origin_without_callbacks();const auto scope=saved->scope_serial;
 const auto pure=[&]{
  // Foreign calls refuse before creator-only mutable State reads.
  require(saved->creator==std::this_thread::get_id(),"System font Source query foreign creator");
  require(saved->alive.load()&&!saved->failed.load()&&saved->active==&operation&&saved->scope_serial==scope&&scope,
          "System font Source storage/active occurrence retired or replaced");
  {const auto actual=weak.lock();const auto canonical=saved->canonical_owner.lock();
   require(actual&&canonical&&actual.get()==expected&&canonical.get()==expected&&
           actual->state_.get()==saved.get()&&canonical->state_.get()==saved.get()&&
           !weak.owner_before(saved->canonical_owner)&&!saved->canonical_owner.owner_before(weak),
           "System font canonical Source facade/control replaced");}
  operation.require_origin_without_callbacks();
 };
 try{
  pure();require(saved->civis_sources&&saved->tooltip_locale_bound&&!saved->tooltip_locale.expired()&&!saved->tooltip_querying,
                 "System font actual Source Bank/selected Engine unbound/retired/reentered");
  saved->tooltip_querying=true;
  struct End{std::shared_ptr<State> s;~End(){s->tooltip_querying=false;}}end{saved};
  const auto engine=saved->tooltip_locale;pure();
  auto result=ui::BnwEngineUiTurnOwner::system_font_language_data_from_owned_weak(engine);
  pure();require(!engine.expired()&&!engine.owner_before(saved->tooltip_locale)&&!saved->tooltip_locale.owner_before(engine),
                 "System font actual Source Engine control replaced");
  operation.require_current();pure();return result;
 }catch(...){saved->failed.store(true);throw;}
}
