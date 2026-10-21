#pragma once
void lua_locale_tests(){
    using namespace civ5::runtime;
    RulesDatabase rules,localization;
    localization.apply_sql("CREATE TABLE Languages(Type TEXT PRIMARY KEY,Name TEXT);"
        "CREATE TABLE LanguagePriorities(LanguageSet TEXT,Language TEXT,Priority INTEGER,"
            "PRIMARY KEY(LanguageSet,Language));"
        "CREATE TABLE LocalizedText(Language TEXT,Tag TEXT,Text TEXT,Gender TEXT,Plurality TEXT,"
            "PRIMARY KEY(Language,Tag));"
        "INSERT INTO Languages VALUES('en_US','English');"
        "INSERT INTO Languages VALUES('ru_RU','Русский (Russian)');"
        "INSERT INTO LanguagePriorities VALUES('en_US','en_US',100);"
        "INSERT INTO LanguagePriorities VALUES('ru_RU','ru_RU',100);"
        "INSERT INTO LocalizedText VALUES('ru_RU','TXT_TEST_HELLO','Привет',NULL,NULL);"
        "INSERT INTO LocalizedText VALUES('en_US','TXT_TEST_HELLO','Hello',NULL,NULL);"
        "INSERT INTO LocalizedText VALUES('en_US','TXT_TEST_FALLBACK','Fallback',NULL,NULL);"
        "INSERT INTO LocalizedText VALUES('en_US','TXT_TEST_FORMAT','{1} units',NULL,NULL);"
        "INSERT INTO LocalizedText VALUES('en_US','TXT_TEST_GENDER','leader','male','1');");
    rules.attach_localization(localization);
    ScriptLocaleSession language(rules,"ru_RU");
    LuaScriptHost host(ScriptBudget{},ScriptSourceProvider{},ScriptModdingProvider{},ScriptGameplayProvider{},
        ScriptSaveDataProvider{},ScriptPregameProvider{},ScriptMapProvider{},ScriptUiEventProvider{},
        ScriptContentProvider{},ScriptUserDataProvider{},ScriptEnumProvider{},language.provider());
    host.execute("first",R"lua(
        local current=Locale.GetCurrentLanguage()
        assert(current.Type=='ru_RU' and current.DisplayName=='Русский (Russian)')
        assert(Locale.ConvertTextKey('TXT_TEST_HELLO')=='Привет')
        assert(Locale.ConvertTextKey('TXT_TEST_FALLBACK')=='Fallback')
        assert(Locale.SetCurrentLanguage('en_US'))
        assert(Locale.GetCurrentLanguage().Type=='en_US')
        assert(Locale.ConvertTextKey('TXT_TEST_HELLO')=='Hello')
        assert(not Locale.SetCurrentLanguage('uninstalled'))
    )lua","=selected-language");
    check(language.revision()==1,"Changing selected language raises one UI invalidation revision");
    host.execute("second","assert(Locale.GetCurrentLanguage().DisplayName=='English')",
        "=shared-language-selection");
    rules.apply_sql("UPDATE Localization.Languages SET Name='English changed' WHERE Type='en_US'");
    host.execute("first","assert(Locale.GetCurrentLanguage().DisplayName=='English changed')",
        "=live-localization-database");
    rules.apply_sql("UPDATE Localization.LocalizedText SET Text='Hello changed' "
                    "WHERE Language='en_US' AND Tag='TXT_TEST_HELLO'");
    host.execute("first","assert(Locale.ConvertTextKey('TXT_TEST_HELLO')=='Hello changed')",
        "=live-localized-text");
    const auto locale=language.provider();
    check(locale.convert_text_key("TXT_TEST_HELLO",{})=="Hello changed",
        "Plain localization lookup reads the activated database after mod updates");
    rejects([&]{(void)locale.convert_text_key("TXT_TEST_FORMAT",{});});
    check(locale.convert_text_key("TXT_TEST_GENDER",{})=="leader",
        "Bare text does not require inflection for gender and plurality metadata");
    rejects([&]{(void)locale.convert_text_key("TXT_TEST_HELLO",std::vector<ScriptValue>{1.0});});
    rejects([&]{(void)locale.convert_text_key("TXT_TEST_MISSING",{});});
    host.complete_loading();check(host.loading_complete(),
        "Lua language selection reads the activated database and shares session state");
    rejects([&]{ScriptLocaleSession invalid(rules,"xx_XX");});
    LuaScriptHost absent;
    absent.execute("none","assert(Locale==nil)","=no-selected-language");
    absent.complete_loading();check(absent.loading_complete(),
        "Generic Lua host does not invent an active language");
    rejects([&]{(void)LuaScriptHost(ScriptBudget{},ScriptSourceProvider{},ScriptModdingProvider{},
        ScriptGameplayProvider{},ScriptSaveDataProvider{},ScriptPregameProvider{},ScriptMapProvider{},
        ScriptUiEventProvider{},ScriptContentProvider{},ScriptUserDataProvider{},ScriptEnumProvider{},
        ScriptLocaleProvider{[]{return ScriptLanguage{"en_US","English"};},{}});});
}
