#pragma once
#include "civ5/ui/civis_civilopedia_category_callbacks.hpp"
#include "civ5/content/saved_rules.hpp"
#include <functional>
#include <memory>

namespace civ5::ui {
struct CivisSelectedCivilopediaTextValue {
    std::string language,tag,text,source_key,source_sha256;
    std::optional<std::size_t> package;
    std::optional<std::uint64_t> mount_id;
};
// Explicit selected stock TextData policy. Private attached-DB clone, retained
// source handles and exact language; no inactive package or language fallback.
// Borrowed keys and activation configuration are copied, and rules are cloned,
// before Current/source callbacks. Callback mutations never retarget a lookup.
// This does not replace native locale composition/grammar or readiness.
class CivisSelectedCivilopediaTextDataOwner
    :public std::enable_shared_from_this<CivisSelectedCivilopediaTextDataOwner> {
  public:
    using Current=std::function<bool(const CivilopediaLayoutOwnerKey &)>;
    static std::shared_ptr<CivisSelectedCivilopediaTextDataOwner> open(
        const std::filesystem::path &game_root,const runtime::SavedContentProfile &,
        const content::Catalog &,const assets::ResourceMountTable &,
        const content::RulesDatabase &,CivilopediaLayoutOwnerKey,std::string language,Current);
    ~CivisSelectedCivilopediaTextDataOwner();
    CivisSelectedCivilopediaTextValue raw(std::string_view tag);
    bool unit_description(std::string_view tag);
    // Separately opt in BEFORE lookup: literal zero-argument selected Building
    // Info names use the already owned clone/source policy, never a fallback.
    void enable_buildings_description_first_form();
    bool building_description(std::string_view tag);
    void close();
    CivisSelectedCivilopediaTextDataOwner(const CivisSelectedCivilopediaTextDataOwner &)=delete;
    CivisSelectedCivilopediaTextDataOwner &operator=(const CivisSelectedCivilopediaTextDataOwner &)=delete;
  private:
    struct Impl;std::unique_ptr<Impl> impl_;
    explicit CivisSelectedCivilopediaTextDataOwner(std::unique_ptr<Impl>);
};
} // namespace civ5::ui
