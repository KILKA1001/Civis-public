#pragma once
#include "civ5/ui/civis_civilopedia_category_callbacks.hpp"
#include "civ5/ui/top_panel.hpp"

namespace civ5::content { struct Catalog; class RulesDatabase; }
namespace civ5::runtime { struct SavedContentProfile; }

namespace civ5::ui {
struct CivilopediaLocaleObservation {
    std::string language,tag,text,source_key,source_sha256;
};
// Immutable language/source selection for one measured Civilopedia owner.
// Selected EN/RU text layers supply actual payloads. Missing/ambiguous keys,
// changed resource provenance and expired/stale owners are explicit failures.
class SelectedCivilopediaLocale:public std::enable_shared_from_this<SelectedCivilopediaLocale> {
  public:
    static std::shared_ptr<SelectedCivilopediaLocale> open(
        const std::filesystem::path &game_root,const assets::ResourceMountTable &resources,
        const SelectedBnwCivilopediaLayoutSources &sources,
        CivilopediaLayoutOwnerKey identity,std::string language);
    ~SelectedCivilopediaLocale();
    runtime::ScriptLocaleProvider provider();
    // Opt-in bounded plain source references; declare before the first text
    // conversion. Selected language/source identities stay immutable.
    void enable_plain_reference_expansion();
    // Explicit selected stock TextData + attached-DB snapshot. Declare before
    // conversion; old open/three-layer behavior remains the default. No language
    // fallback, inactive DLC or inferred native locale/readiness is enabled.
    void enable_selected_text_data(const std::filesystem::path &game_root,
        const runtime::SavedContentProfile &,const content::Catalog &,
        const assets::ResourceMountTable &,const content::RulesDatabase &);
    // Explicitly requested zero-argument selected Buildings.Description Info
    // names only; SAME TextData clone/source fences, before text conversion.
    void enable_buildings_description_first_form();
    bool buildings_description_first_form_enabled()const;
    const std::vector<std::string> &plain_lookup_tags()const;
    const std::vector<CivilopediaLocaleObservation> &observations()const;
    bool current(const CivilopediaLayoutOwnerKey &)const;
    void observe(const CivilopediaLayoutOwnerKey &);
    void close();
  private:
    struct Impl;std::unique_ptr<Impl> impl_;
    explicit SelectedCivilopediaLocale(std::unique_ptr<Impl>);
};
} // namespace civ5::ui
