#pragma once
#include "civ5/runtime/bnw_creation_config.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

namespace civ5::runtime {
enum class BnwSelectedLocaleError {
    parent_unavailable, wrong_thread, input_unavailable,
    invalid_language_input, input_changed, wrong_config
};
class BnwSelectedLocaleRefusal : public std::runtime_error {
 public:
    BnwSelectedLocaleRefusal(BnwSelectedLocaleError, const std::string&);
    BnwSelectedLocaleError code() const noexcept { return code_; }
 private:
    BnwSelectedLocaleError code_;
};
enum class BnwSelectedLanguageSourceKind {selected_ini,explicit_process_language};
struct BnwSelectedLocaleSource {
    BnwSelectedLanguageSourceKind kind=BnwSelectedLanguageSourceKind::selected_ini;
    FileIdentity selection_identity; //Process selection identity only; no virtual file.
    std::filesystem::path config_ini;
    FileIdentity file;
    std::string language;
};
// Read-only, selected INI + SAME activated CreationConfig allocation.
// No public provider, language setter, completion flag or VM capability.
class BnwSelectedLanguageInputOwner;
class BnwSelectedLocaleOwner {
 public:
    static std::shared_ptr<const BnwSelectedLocaleOwner> from_creation_config(
        std::shared_ptr<const BnwCreationConfig>, std::filesystem::path config_ini);
    static std::shared_ptr<const BnwSelectedLocaleOwner> from_creation_config(
        std::shared_ptr<const BnwCreationConfig>, std::shared_ptr<const BnwSelectedLanguageInputOwner> actual_selected_input,
        std::filesystem::path same_selected_config_ini);
    static std::shared_ptr<const BnwSelectedLocaleOwner> from_creation_config_and_early_input(
        std::shared_ptr<const BnwCreationConfig>,std::shared_ptr<const BnwSelectedLanguageInputOwner>);
    void require_current() const;
    void require_same_config(const BnwCreationConfig&) const;
    std::string localized_text(std::string_view tag) const;
    std::string language() const;
    BnwSelectedLocaleSource source() const;
 private:
    struct Impl;
    explicit BnwSelectedLocaleOwner(std::shared_ptr<Impl>);
    std::shared_ptr<Impl> impl_;
};
} // namespace civ5::runtime
