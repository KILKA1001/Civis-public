// Inside runtime AFTER complete actual SourceOp Impl. Root caller only.
std::unique_ptr<const ui::detail::NotificationContextRootSmallUiPolicyLoan>
BnwWorldViewSourceOperationLoan::context_small_ui_policy_owned()const{
 require_origin_without_callbacks();
 // Snapshot all immutable Source metadata before ANY callback/full operation.
 const auto scope=impl_->scope_serial;const auto serial=impl_->identity.serial;
 const auto source=impl_->owner.lock();const auto host_origin=impl_->host;
 need(bool(source),"SmallUI Source facade absent");
 const auto pure=[&]{source->require_operation_origin_without_callbacks(*this,scope);require_origin_without_callbacks();};
 const auto full=[&]{pure();require_operation(serial);pure();};
 full();const auto host=host_origin.lock();need(bool(host),"SmallUI SAME selectedHost retired");
 // Capture the actual Host Bank by VALUE. Its presence only selects a lane;
 // all SAME Host/Source/Bank controls are proved before borrowing UserOptions.
 const auto bank=host->civis_resource_bank_;
 if(bank){
  const auto bank_current=[&,host,bank]{
   full();host->require_same_civis_resources(*bank);full();
   const auto selected=selected_provider_startup_bank_owned();full();
   need(selected&&selected.get()==bank.get()&&!selected.owner_before(bank)&&!bank.owner_before(selected)&&
    host->civis_resource_bank_.get()==bank.get()&&!host->civis_resource_bank_.owner_before(bank)&&
    !bank.owner_before(host->civis_resource_bank_),"SmallUI SAME selected Host/Source Bank controls changed");
   full();
  };
  bank_current();const auto options=bank->context_selected_user_options_owned();bank_current();
  need(bool(options),"SmallUI actual Bank UserOptions producer returned no owner");
  // Capture both canonical owners by VALUE, never a mutable field lvalueref.
  // Repeated lending must return the SAME Options control after all callbacks.
  const auto current=[&,host,bank,options]{
   bank_current();options->require_current();bank_current();
   const auto repeated=bank->context_selected_user_options_owned();bank_current();
   need(repeated&&repeated.get()==options.get()&&!repeated.owner_before(options)&&!options.owner_before(repeated),
    "SmallUI actual Bank UserOptions canonical controls changed");
   bank_current();
  };
  current();auto result=options->context_small_ui_policy_owned(*this,source,scope);current();
  need(bool(result),"SmallUI actual Bank UserOptions loan absent");
  result->require_current_without_callbacks(*this);current();return result;
 }
 // Preserve the existing authentic native early-Package route unchanged.
 const auto package=host->early_package_;need(bool(package),"SmallUI actual selectedHost earlyPackage/UserSettings producer absent");
 host->require_same_early_package(*package);full();
 const auto options=package->borrow_selected_user_options_for_source();full();
 host->require_same_early_package(*package);full();
 need(options&&host->early_package_.get()==package.get()&&!host->early_package_.owner_before(package)&&
  !package.owner_before(host->early_package_),"SmallUI actual startup Package/UserOptions controls changed");
 const auto current=[&]{full();host->require_same_early_package(*package);full();
  need(host->early_package_.get()==package.get()&&!host->early_package_.owner_before(package)&&
   !package.owner_before(host->early_package_),"SmallUI startup Package changed during producer");};
 current();auto result=options->context_small_ui_policy_owned(*this,source,scope);current();
 result->require_current_without_callbacks(*this);return result;
}

bool BnwWorldViewSourceOperationLoan::context_options_small_ui_assets_owned()const{
 require_origin_without_callbacks();
 const auto actual=context_small_ui_policy_owned();
 need(bool(actual),"Options SmallUI genuine UserOptions loan absent");
 actual->require_current_owned(*this);
 const bool value=actual->small_ui_assets_without_callbacks();
 actual->require_current_owned(*this);
 actual->require_current_without_callbacks(*this);
 require_origin_without_callbacks();return value;
}

bool BnwWorldViewSourceOperationLoan::context_options_minimap_game_setting_owned(std::string_view key)const{
 require_origin_without_callbacks();const auto scope=impl_->scope_serial;
 const auto serial=impl_->identity.serial;const auto source=impl_->owner.lock();
 const auto host_origin=impl_->host;need(bool(source),"MiniMap actual Source absent");
 const auto pure=[&]{source->require_operation_origin_without_callbacks(*this,scope);require_origin_without_callbacks();};
 const auto full=[&]{pure();require_operation(serial);pure();};
 full();const auto host=host_origin.lock();need(bool(host),"MiniMap actual selectedHost retired");
 const auto bank=host->civis_resource_bank_;
 if(bank){
  const auto bank_current=[&,host,bank]{
   full();host->require_same_civis_resources(*bank);full();
   const auto selected=selected_provider_startup_bank_owned();full();
   need(selected&&selected.get()==bank.get()&&!selected.owner_before(bank)&&!bank.owner_before(selected)&&
    host->civis_resource_bank_.get()==bank.get()&&!host->civis_resource_bank_.owner_before(bank)&&
    !bank.owner_before(host->civis_resource_bank_),"MiniMap SAME selected Bank controls changed");full();
  };
  bank_current();const auto options=bank->context_selected_user_options_owned();bank_current();
  need(bool(options),"MiniMap actual Bank UserOptions absent");
  const auto current=[&,host,bank,options]{
   bank_current();options->require_current();bank_current();
   const auto repeated=bank->context_selected_user_options_owned();bank_current();
   need(repeated&&repeated.get()==options.get()&&!repeated.owner_before(options)&&!options.owner_before(repeated),
    "MiniMap canonical UserOptions changed");bank_current();
  };
  current();const bool value=options->minimap_game_setting_owned(key);current();return value;
 }
 const auto package=host->early_package_;need(bool(package),"MiniMap actual earlyPackage/UserOptions absent");
 host->require_same_early_package(*package);full();
 const auto options=package->borrow_selected_user_options_for_source();full();
 const auto current=[&]{full();host->require_same_early_package(*package);full();
  need(options&&host->early_package_.get()==package.get()&&!host->early_package_.owner_before(package)&&
   !package.owner_before(host->early_package_),"MiniMap actual startup Package changed");
  options->require_current();full();
  const auto repeated=package->borrow_selected_user_options_for_source();full();
  need(repeated&&repeated.get()==options.get()&&!repeated.owner_before(options)&&!options.owner_before(repeated),
   "MiniMap actual earlyPackage canonical UserOptions changed");full();
 };
 current();const bool value=options->minimap_game_setting_owned(key);current();return value;
}
