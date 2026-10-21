#pragma once
#include "civ5/ui/frontend_menu.hpp"
#include "civ5/ui/frontend_selected_save_input.hpp"
#include "civ5/runtime/save_preflight.hpp"
#include <memory>
#include <unordered_map>

namespace civ5::ui {
// Read-only catalog/save adapter. A ready content check is never permission
// to execute gameplay; the host/backend remains responsible for that boundary.
class FrontendCatalogContentAdapter {
  public:
    FrontendCatalogContentAdapter(std::shared_ptr<const content::Catalog> catalog,
        std::vector<std::filesystem::path> explicit_saves = {});
    const FrontendData &data() const { return data_; }
    // Cached selection facts only: no file reads, hashes, payload decode or
    // activation. check_content remains mandatory before an actual effect.
    FrontendContentCheck check_selection(const FrontendRequest &request) const;
    FrontendContentCheck check_content(const FrontendRequest &request) const;
    // Throws on unavailable/stale input; successful return is input custody only.
    FrontendSelectedSaveInput prepare_selected_save_input(const FrontendRequest &request) const;
    static std::string package_key(const content::PackageManifest &package);
  private:
    struct PackageSnapshot {
        std::optional<runtime::FileIdentity> identity;
        std::string error;
    };
    struct SaveSnapshot {
        std::optional<runtime::FileIdentity> identity;
        std::optional<saves::SaveHeader> header;
        std::string error;
    };
    std::shared_ptr<const content::Catalog> catalog_;
    FrontendData data_;
    std::unordered_map<std::string, std::size_t> package_indices_;
    std::vector<PackageSnapshot> package_snapshots_;
    std::vector<SaveSnapshot> save_snapshots_;
    FrontendContentCheck evaluate_content(const FrontendRequest &,
        std::optional<FrontendSelectedSaveInput> *retained_input) const;
    void verify_package_sources(const runtime::SavedContentProfile &profile,
        const content::Catalog &catalog) const;
};
} // namespace civ5::ui
