#pragma once
#include "civ5/content/rules_database.hpp"
#include "civ5/ui/civis_civilopedia_category_callbacks.hpp"

namespace civ5::ui {
class CivisCivilopediaUnitsQuery;
class CivisCivilopediaUnitsIterator;
// Diagnostic Civis policy, not the unavailable native CreateQuery ABI:
// only the decoded installed first Units SQL is allowed; a new call resets
// its prepared cursor and invalidates earlier iterators, including at EOF.
// Every operation requires the original context owner and owner thread.
class CivisCivilopediaUnitsQueryOwner {
  public:
    static std::shared_ptr<CivisCivilopediaUnitsQueryOwner> open(
        const std::filesystem::path &game_root,const SelectedBnwCivilopediaLayoutSources &sources,
        const content::RulesDatabase &selected_rules,CivilopediaLayoutOwnerKey identity,
        const std::shared_ptr<CivisCivilopediaCallbackOwner> &current_owner);
    // Separately requested diagnostic policy: the exact installed first Units
    // and first Buildings statements share this SAME owner/clone/cursor types.
    // The legacy open() above remains Units-only. No generic SQL/native parity.
    static std::shared_ptr<CivisCivilopediaUnitsQueryOwner> open_with_buildings(
        const std::filesystem::path &game_root,const SelectedBnwCivilopediaLayoutSources &sources,
        const content::RulesDatabase &selected_rules,CivilopediaLayoutOwnerKey identity,
        const std::shared_ptr<CivisCivilopediaCallbackOwner> &current_owner);
    ~CivisCivilopediaUnitsQueryOwner();
    std::shared_ptr<CivisCivilopediaUnitsQuery> prepare(std::string_view sql);
    std::string declared_sql()const;
    std::string declared_buildings_sql()const;
    void close();
    // Source extraction checks the exact bounded declaration/call grammar,
    // including Lua long-string newline decoding. It does not execute SQL.
    static std::string declared_sql_from_source(std::string_view source);
    static std::string declared_buildings_sql_from_source(std::string_view source);
  private:
    struct Impl;std::unique_ptr<Impl> impl_;
    explicit CivisCivilopediaUnitsQueryOwner(std::unique_ptr<Impl>);
};
class CivisCivilopediaUnitsQuery {
  public:
    ~CivisCivilopediaUnitsQuery();
    // Exactly one copied string parameter; unknown/empty eras produce real EOF.
    std::shared_ptr<CivisCivilopediaUnitsIterator> iterate(std::span<const content::DatabaseScriptValue> arguments);
    void close();
  private:
    friend class CivisCivilopediaUnitsQueryOwner;
    friend class CivisCivilopediaUnitsIterator;
    struct Impl;std::shared_ptr<Impl> impl_;
    explicit CivisCivilopediaUnitsQuery(std::shared_ptr<Impl>);
};
class CivisCivilopediaUnitsIterator {
  public:
    ~CivisCivilopediaUnitsIterator();
    // Owned values, lazy Step, sticky EOF. SQL errors are explicit failures.
    std::optional<content::DatabaseScriptRow> next();
  private:
    friend class CivisCivilopediaUnitsQuery;
    struct Impl;std::unique_ptr<Impl> impl_;
    explicit CivisCivilopediaUnitsIterator(std::unique_ptr<Impl>);
};
}
