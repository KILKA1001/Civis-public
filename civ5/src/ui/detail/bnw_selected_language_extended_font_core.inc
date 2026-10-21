// Inside ui after complete actual Engine State/EndTurnPin. Source alone
// supplies its authentic weak Engine binding; no caller locale/boolean.
std::pair<std::string,std::uint8_t> BnwEngineUiTurnOwner::selected_localization_extended_font_from_owned_weak(
    const std::weak_ptr<BnwEngineUiTurnOwner>& weak){
    // Same config/localization selection already used by native Locale.GetCurrentLanguage.
    const auto current_language=selected_localization_language_from_owned_weak(weak);
    EndTurnPin pin(weak);require_counted_localization_owner(pin);
    pin.engine->require_selected_localization_current();require_counted_localization_owner(pin);
    const auto saved=pin.storage;const auto selected=saved->locale_scope->locale();
    need(current_language.type==selected.language&&selected.language==saved->language&&
        (selected.language=="ru_RU"||selected.language=="en_US"),"Gui980A actual selected locale unsupported/replaced");
    std::optional<std::uint8_t> value;std::size_t count=0;
    //4630A0 executes against localization Languages. The attached effective
    //database has all selected base/DLC TextData applied before Source binding.
    //SQL preparation/row/getter failure STOP; no constructor/default-zero grant.
    const auto rows=saved->locale_database->query({
        "SELECT UseExtendedFont FROM Localization.Languages WHERE Type = ? LIMIT 1",{selected.language}},
        [&](const auto columns){
            require_counted_localization_owner(pin);
            need(columns.size()==1,"Gui980A Languages result shape differs");
            const auto integer=std::get_if<std::int64_t>(&columns[0]);
            need(integer&&(*integer==0||*integer==1),"Gui980A selected Database::GetBool domain unsupported");
            value=static_cast<std::uint8_t>(*integer);++count;
        });
    //RulesDatabase query completes its Results/statement cleanup before return.
    require_counted_localization_owner(pin);pin.engine->require_selected_localization_current();require_counted_localization_owner(pin);
    need(rows==count&&count==1&&value&&saved->locale_scope->locale()==selected&&saved->language==selected.language,
        "Gui980A actual activated row/locale missing or changed across query");
    const auto after=selected_localization_language_from_owned_weak(weak);require_counted_localization_owner(pin);
    need(after.type==current_language.type&&after.display_name==current_language.display_name,"Gui980A actual config language changed across Results cleanup");
    return {selected.language,*value};
}
