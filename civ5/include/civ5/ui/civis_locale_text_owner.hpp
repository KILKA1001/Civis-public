#pragma once
#include "civ5/runtime/locale_text_provider.hpp"
#include "civ5/ui/selected_civilopedia_locale.hpp"

namespace civ5::ui {
struct CivisLocaleTextPolicy {
    std::string backend,language,locale_name;
    std::uint32_t casing_flags=0,comparison_flags=0,nls_version=0,defined_version=0;
};
// Portable owner/provider interface. The current backend is explicitly
// Windows NLS EN/RU linguistic lowercase and word collation. An unavailable
// backend/language fails; it cannot fall back to ASCII or key echo.
class CivisLocaleTextOwner:public std::enable_shared_from_this<CivisLocaleTextOwner> {
  public:
    static std::shared_ptr<CivisLocaleTextOwner> open(
        std::shared_ptr<SelectedCivilopediaLocale>,CivilopediaLayoutOwnerKey,
        std::string requested_language);
    ~CivisLocaleTextOwner();
    runtime::ScriptLocaleTextProvider provider();
    const CivisLocaleTextPolicy &policy()const;
    void close();
  private:
    struct Impl;std::unique_ptr<Impl> impl_;
    explicit CivisLocaleTextOwner(std::unique_ptr<Impl>);
};
} // namespace civ5::ui
