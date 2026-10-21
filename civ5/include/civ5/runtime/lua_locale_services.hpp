#pragma once
#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/lua_script_host.hpp"

namespace civ5::runtime {
// One session's UI language, backed by its activated Localization database.
// The owner must outlive every Lua host using provider(). Database writes by
// mods stay visible; revision lets the graphics layer invalidate rendered text.
class ScriptLocaleSession {
    const content::RulesDatabase &database_;
    std::string type_;
    std::uint64_t revision_=0;
    std::optional<ScriptLanguage> find(std::string_view type)const {
        std::optional<ScriptLanguage> value;
        const auto report=database_.query_rows(
            {"SELECT Type,Name FROM Localization.Languages WHERE Type=? LIMIT 1",
             {std::string(type)}},
            [&](const content::RulesRowView &row){
                const auto kind=row.text("Type"),name=row.text("Name");
                if(kind&&name)value=ScriptLanguage{*kind,*name};
            },1);
        if(!report.all_succeeded())throw ScriptError("Selected localization database has no readable Languages table");
        return value;
    }
    std::string plain_text(std::string_view tag,std::span<const ScriptValue> arguments)const {
        if(tag.empty()||tag.find('\0')!=tag.npos)
            throw ScriptError("Invalid localized text key");
        if(!arguments.empty())
            throw ScriptError("Locale.ConvertTextKey parameter formatting is not implemented");
        std::optional<std::string> value;
        const auto read=[&](const content::RulesRowView &row){
            value=row.text("Text");
        };
        const auto priority=database_.query_rows(
            {"SELECT t.Text FROM Localization.LanguagePriorities p "
             "JOIN Localization.LocalizedText t ON t.Language=p.Language "
             "WHERE p.LanguageSet=? AND t.Tag=? ORDER BY p.Priority DESC LIMIT 1",
             {type_,std::string(tag)}},read,1);
        if(!priority.all_succeeded())
            throw ScriptError("Selected localization database has no readable text priorities");
        // The retail configuration permits English fallback. The active language
        // priority list is consulted first, including priorities added by mods.
        if(!value&&type_!="en_US"){
            const auto fallback=database_.query_rows(
                {"SELECT Text FROM Localization.LocalizedText "
                 "WHERE Language='en_US' AND Tag=? LIMIT 1",{std::string(tag)}},read,1);
            if(!fallback.all_succeeded())
                throw ScriptError("Selected localization database has no readable text table");
        }
        if(!value)throw ScriptError("Localized text key is absent from activated Localization");
        // Gender and plurality describe this value when used as an argument to
        // another key; a bare key still returns its stored text unchanged.
        if(value->find('{')!=value->npos||value->find('}')!=value->npos)
            throw ScriptError("Locale.ConvertTextKey grammar formatting is not implemented for " +
                              std::string(tag));
        return *value;
    }
  public:
    ScriptLocaleSession(const content::RulesDatabase &database,std::string type)
        :database_(database),type_(std::move(type)) {
        if(!find(type_))throw ScriptError("Selected UI language is absent from activated Localization");
    }
    ScriptLocaleProvider provider(){
        return {
            [this]{
                const auto language=find(type_);
                if(!language)throw ScriptError("Current UI language is absent from activated Localization");
                return *language;
            },
            [this](std::string_view requested){
                const auto language=find(requested);
                if(!language)return false;
                if(revision_==UINT64_MAX)throw ScriptError("UI language revision overflow");
                type_=language->type;++revision_;
                return true;
            },
            [this](std::string_view tag,std::span<const ScriptValue> arguments){
                return plain_text(tag,arguments);
            }
        };
    }
    std::uint64_t revision()const{return revision_;}
};
} // namespace civ5::runtime
