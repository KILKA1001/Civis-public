// Inside civ5::runtime after complete actual SourceOp::Impl and source helpers.
// CPU supplier for the real outer GameCore frame; no Lua/SQL/activation here.
std::shared_ptr<const content::BnwNotificationEntriesSourceOwner>
BnwWorldViewSourceOperationLoan::context_notification_entries_owned(const content::RulesDatabase** actual_database)const{
 require_origin_without_callbacks();
 const auto scope=impl_->scope_serial;const auto serial=impl_->identity.serial;
 const auto source_origin=impl_->owner;const auto host_origin=impl_->host;
 const BnwSelectedWorldViewContextOwner* source_get=nullptr;
 {const auto source=source_origin.lock();need(bool(source),"Notifications actual Source absent");source_get=source.get();}
 const auto pure=[&]{
  require_origin_without_callbacks();
  const auto source=source_origin.lock();
  need(source&&source.get()==source_get&&!source_origin.owner_before(source)&&!source.owner_before(source_origin),
       "Notifications SAME actual Source get/control changed");
  source->require_operation_origin_without_callbacks(*this,scope);require_origin_without_callbacks();
 };
 const auto full=[&]{pure();require_operation(serial);pure();};
 std::shared_ptr<const content::BnwNotificationEntriesSourceOwner> result;
 std::weak_ptr<const content::RulesDatabase> database_origin;
 const content::RulesDatabase* database_get=nullptr;
 const ui::SelectedBnwPreviewHost* host_get=nullptr;
 full();
 {
  const auto host=host_origin.lock();need(bool(host),"Notifications canonical selectedHost retired");
  const auto database=host->database_;need(bool(database),"Notifications actual Host rules absent");
  host_get=host.get();database_origin=database;database_get=database.get();
  need(database->immutable_preview_fingerprints_current(),"Notifications actual rules scope is not sealed/current");
  if(host->civis_resource_bank_)host->require_same_civis_resources(*host->civis_resource_bank_);
  else{need(bool(host->early_package_),"Notifications actual earlyPackage/Bank absent");host->require_same_early_package(*host->early_package_);}
  full();
  result=host->notification_entries_owned();result->require_current(*database);
  need(host->notification_entries_.get()==result.get()&&!host->notification_entries_.owner_before(result)&&
       !result.owner_before(host->notification_entries_),"Notifications canonical actual cache substituted");
  pure();
 }
 // The actual temporary Host/rules/source locks have ended before fresh FULL.
 const auto verify=[&]{
  pure();
  const auto host=host_origin.lock();const auto database=database_origin.lock();
  need(host&&host.get()==host_get&&!host_origin.owner_before(host)&&!host.owner_before(host_origin),
       "Notifications selectedHost get/control changed after FULL");
  need(database&&database.get()==database_get&&host->database_.get()==database_get&&
       !database_origin.owner_before(database)&&!database.owner_before(database_origin)&&
       !host->database_.owner_before(database)&&!database.owner_before(host->database_)&&
       database->immutable_preview_fingerprints_current(),"Notifications SAME selected rules controls changed after FULL");
  const auto actual=host->notification_entries_owned();actual->require_current(*database);
  need(actual.get()==result.get()&&!actual.owner_before(result)&&!result.owner_before(actual)&&
       host->notification_entries_.get()==result.get()&&!host->notification_entries_.owner_before(result)&&
       !result.owner_before(host->notification_entries_),"Notifications cached owner changed after FULL");
  pure();
 };
 full();verify();full();verify();
 // Publish the genuine borrowed rules pointer only after every FULL and fresh
 // cache/control POST. The returned cache already retains this SAME database.
 if(actual_database)*actual_database=database_get;
 return result;
}
