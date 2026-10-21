#pragma once
#include "civ5/runtime/save_preflight.hpp"
#include "civ5/saves/loaded_save.hpp"
#include <memory>
namespace civ5::ui {
class FrontendCatalogContentAdapter;
// Owned, immutable menu-selected INPUT. This is not a running session,
// application activation, native callback permission or gameplay readiness.
class FrontendSelectedSaveInput final {
    friend class FrontendCatalogContentAdapter;
    struct State;
    std::unique_ptr<const State> state_;
    explicit FrontendSelectedSaveInput(std::unique_ptr<const State>);
    const State& state() const;
public:
    ~FrontendSelectedSaveInput();
    FrontendSelectedSaveInput(FrontendSelectedSaveInput&&) noexcept;
    FrontendSelectedSaveInput& operator=(FrontendSelectedSaveInput&&) noexcept;
    FrontendSelectedSaveInput(const FrontendSelectedSaveInput&)=delete;
    FrontendSelectedSaveInput& operator=(const FrontendSelectedSaveInput&)=delete;
    const std::filesystem::path& save_path() const;
    const runtime::FileIdentity& save_identity() const;
    const saves::LoadedSave& loaded_save() const;
    const content::Catalog& selected_catalog() const;
    const runtime::SavedContentProfile& selected_profile() const;
    const std::vector<std::string>& required_packages() const;
    // Fresh file-content comparisons only. No held OS handle or world authority.
    void require_files_current() const;
};
}
