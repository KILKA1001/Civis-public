// Inside runtime, AFTER actual SourceOp::Impl and existing source helpers.
std::shared_ptr<const content::BnwGameInfoActionsOwner>
BnwWorldViewSourceOperationLoan::context_gameinfo_actions_owned()const{
 require_origin_without_callbacks();const auto scope=impl_->scope_serial;
 const auto serial=impl_->identity.serial;const auto source=impl_->owner.lock();
 const auto host_origin=impl_->host;need(bool(source),"Actions actual Source absent");
 const auto pure=[&]{source->require_operation_origin_without_callbacks(*this,scope);require_origin_without_callbacks();};
 const auto full=[&]{pure();require_operation(serial);pure();};
 full();const auto host=host_origin.lock();need(bool(host),"Actions actual canonical selectedHost retired");
 const auto database=host->database_;
 need(bool(database),"Actions actual Host RulesDatabase absent");
 const auto current=[&,host,database]{
  full();need(host->database_.get()==database.get()&&!host->database_.owner_before(database)&&
   !database.owner_before(host->database_)&&database->immutable_preview_fingerprints_current(),
   "Actions SAME selected RulesDatabase controls changed");
  if(host->civis_resource_bank_)host->require_same_civis_resources(*host->civis_resource_bank_);
  else{need(bool(host->early_package_),"Actions actual earlyPackage/Bank absent");host->require_same_early_package(*host->early_package_);}
  full();
 };
 current();auto existing=host->bnw_gameinfo_actions_;
 if(existing){existing->require_same_rules(*database);current();
  need(host->bnw_gameinfo_actions_.get()==existing.get()&&!host->bnw_gameinfo_actions_.owner_before(existing)&&
   !existing.owner_before(host->bnw_gameinfo_actions_),"Actions canonical aggregate substituted");return existing;
 }
 need(!host->bnw_gameinfo_actions_querying_,"Actions actual selectedHost construction reentry");
 host->bnw_gameinfo_actions_querying_=true;
 struct End{std::shared_ptr<const ui::SelectedBnwPreviewHost> host;~End(){host->bnw_gameinfo_actions_querying_=false;}}end{host};
 const auto localize=[&](std::string_view key){
  current();auto value=source->action_info_localized_text_owned(*this,key);current();return value;
 };
 const auto created=content::BnwGameInfoActionsOwner::create_owned(database,host->types_,localize,current);
 current();need(!host->bnw_gameinfo_actions_,"Actions aggregate published recursively");
 created->require_same_rules(*database);current();host->bnw_gameinfo_actions_=created;
 current();created->require_same_rules(*database);return created;
}
