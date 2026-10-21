// Genuine activated Engine localization DATA, only for canonical GUI height MISS.
detail::BnwLabelSystemFontLanguageData BnwEngineUiTurnOwner::system_font_language_data_from_owned_weak(
 const std::weak_ptr<BnwEngineUiTurnOwner>& borrowed){
 const auto weak=borrowed;EndTurnPin pin(weak);require_counted_localization_owner(pin);
 const auto saved=pin.storage;
 pin.engine->require_selected_localization_current();require_counted_localization_owner(pin);
 const auto* const scope_identity=saved->locale_scope.get();
 const auto* const database_identity=saved->locale_database.get();
 detail::BnwLabelSystemFontLanguageData result;
 {
 const auto full=[&]{
  require_counted_localization_owner(pin);
  pin.engine->require_selected_localization_current();require_counted_localization_owner(pin);
  need(saved->locale_scope&&saved->locale_scope.get()==scope_identity&&saved->locale_database&&
       saved->locale_database.get()==database_identity,"System font activated locale/database allocation replaced");
  require_counted_localization_owner(pin);
 };
 full();const auto current_language=selected_localization_language_from_owned_weak(weak);full();
 const auto selected=saved->locale_scope->locale();full();
 need(current_language.type==selected.language&&selected.language==saved->language&&
      !selected.owner_token.empty()&&selected.revision,"System font actual selected locale identity differs");
 const auto same_locale=[&]{
  full();
  {const auto after=saved->locale_scope->locale();full();
   need(after==selected&&saved->language==selected.language,"System font selected locale changed across query");}
  // The copied locale strings have already completed destruction.
  require_counted_localization_owner(pin);
  saved->locale_scope->require_selected_locale_without_callbacks(selected);
  require_counted_localization_owner(pin);
 };
 same_locale();
 std::optional<detail::BnwLabelSystemFontLanguageData> value;std::uint64_t count=0;
 std::uint64_t rows=0;
 {
 content::DatabaseStatement statement{
  "SELECT FontName, FontWidthScale, FontHeightScale FROM Localization.Languages WHERE Type = ? LIMIT 1",
  {selected.language}};
 same_locale();
 const content::RulesDatabase::RowVisitor visitor=[&](const auto columns){
  same_locale();need(columns.size()==3&&count==0,"System font Languages result row/column shape differs");
  const auto* name=std::get_if<std::string>(&columns[0]);
  need(name&&!name->empty()&&name->find('\0')==std::string::npos,"System font Languages FontName missing or malformed");
  const auto numeric=[](const content::DatabaseValue& column){
   double number;
   if(const auto* v=std::get_if<double>(&column))number=*v;
   else if(const auto* v=std::get_if<std::int64_t>(&column))number=static_cast<double>(*v);
   else throw std::runtime_error("System font Languages scale is not numeric");
   need(std::isfinite(number),"System font Languages scale nonfinite");
   const auto result=static_cast<float>(number);
   need(std::isfinite(result),"System font Languages native float conversion nonfinite");return result;
  };
  const auto width=numeric(columns[1]),height=numeric(columns[2]);same_locale();
  // DTO copies may allocate; no semantic GUI/cache store is performed here.
  value.emplace(detail::BnwLabelSystemFontLanguageData{selected,*name,width,height});
  same_locale();++count;
 };
 same_locale();rows=saved->locale_database->query(statement,visitor);same_locale();
 }
 // Actual SQLite Results and local callback/statement DATA cleanup returned.
 same_locale();need(rows==count&&count==1&&value,"System font selected Languages row absent");
 const auto after_language=selected_localization_language_from_owned_weak(weak);same_locale();
 need(after_language.type==current_language.type&&after_language.display_name==current_language.display_name,
      "System font actual config language changed across Results cleanup");
 result=std::move(*value);same_locale();
 }
 // All copied metadata/locale strings, optional value, callbacks and Results
 // are gone. Only counted storage and the actual result survive this cut.
 require_counted_localization_owner(pin);
 need(saved->locale_scope&&saved->locale_scope.get()==scope_identity&&saved->locale_database&&
      saved->locale_database.get()==database_identity&&saved->language==result.locale.language,
      "System font activated locale/database changed across local cleanup");
 saved->locale_scope->require_selected_locale_without_callbacks(result.locale);
 require_counted_localization_owner(pin);return result;
}
