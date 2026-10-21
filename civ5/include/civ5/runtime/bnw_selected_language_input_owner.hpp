#pragma once
#include "civ5/runtime/bnw_selected_locale_owner.hpp"
#include <locale>
#include "civ5/runtime/lua_script_host.hpp"
namespace civ5::assets {class CivisResourceBankOwner;}
namespace civ5::ui {class BnwNativeResourceBootSettingsOwner;}
namespace civ5::ui::detail {class BnwNativePackageDescriptorCoreWriter;class BnwNativeGlobalResourceWarmOwner;class BnwNativeOwnedSkinBootPolicy;class BnwNativePreDx9DiscoveryOwner;}
namespace civ5::runtime {
//First-start diagnostic, copied choices never issue a selected input owner.
class CivisLanguageSelectionRequired final : public std::runtime_error {
    const std::vector<ScriptLanguage> choices_;
public:
    explicit CivisLanguageSelectionRequired(std::vector<ScriptLanguage> choices)
        :std::runtime_error("Choose a local UI language with --ui-language TYPE or --civis-settings FILE"),choices_(std::move(choices)){}
    const std::vector<ScriptLanguage>& available_languages()const noexcept{return choices_;}
};
//Standalone platform source: actual selected [User Settings] Language bytes.
//No Steamworks/network/default language or late game-rules dependency.
//Original input routes require the real early Boot profile or CreationConfig.
//The disjoint own Bank factory below does not issue either Original affinity.
//Copied strings/diagnostic frames cannot issue a selected input owner.
class BnwSelectedLanguageInputOwner final {
    friend class ::civ5::ui::BnwNativeResourceBootSettingsOwner;
    friend class BnwSelectedLocaleOwner;
    friend class BnwSelectedWorldViewContextOwner;
    friend class BnwContextLuaProfileConfigLoan;
    std::int32_t selected_lua_logging_value_owned(
        const std::shared_ptr<const assets::CivisResourceBankOwner>&)const;
    std::int32_t selected_lua_debug_value_owned(const std::shared_ptr<const assets::CivisResourceBankOwner>&)const;
    friend class BnwCreationConfig;
    friend class BnwSelectedUserOptionsOwner;
    friend class BnwNetMessageDebugConfigOwner;
    friend class ::civ5::ui::detail::BnwNativePackageDescriptorCoreWriter;
    friend class ::civ5::ui::detail::BnwNativeGlobalResourceWarmOwner;
    friend class ::civ5::ui::detail::BnwNativeOwnedSkinBootPolicy;
    friend class ::civ5::ui::detail::BnwNativePreDx9DiscoveryOwner;
    static std::shared_ptr<const BnwSelectedLanguageInputOwner> capture_from_initial_package_core(
        const std::shared_ptr<::civ5::ui::detail::BnwNativePreDx9DiscoveryOwner>&);
    void bind_actual_initial_boot(const std::shared_ptr<const ui::BnwNativeResourceBootSettingsOwner>&)const;
    void require_early_boot_input()const;
    void require_same_early_game_root(const std::filesystem::path&)const;
    void require_same_boot(const ui::BnwNativeResourceBootSettingsOwner&)const;
    int platform_ascii_compare(std::string_view,std::string_view)const;
    std::string platform_lowercase_ascii(std::string_view)const;
    std::string platform_uppercase_ascii(std::string_view)const;
    std::locale platform_stream_locale()const;
    struct Impl;
    const std::shared_ptr<Impl> impl_;
    explicit BnwSelectedLanguageInputOwner(std::shared_ptr<Impl>);
    static std::shared_ptr<const BnwSelectedLanguageInputOwner> capture_from_early_boot(
        const std::shared_ptr<const ui::BnwNativeResourceBootSettingsOwner>&,
        const std::filesystem::path& actual_profile_config);
    static std::shared_ptr<const BnwSelectedLanguageInputOwner> capture_from_creation_config(
        const std::shared_ptr<const BnwCreationConfig>&,std::filesystem::path actual_selected_path);
public:
    // Own Civis Bank origin only: actual process selection and local Languages.
    // Does not issue Original early Boot or CreationConfig affinity receipts.
    static std::shared_ptr<const BnwSelectedLanguageInputOwner> capture_from_civis_resources(
        const std::shared_ptr<assets::CivisResourceBankOwner>&);
    ~BnwSelectedLanguageInputOwner();
    void require_current()const;
    std::string language()const;
    BnwSelectedLocaleSource source()const;
    BnwSelectedLanguageInputOwner(const BnwSelectedLanguageInputOwner&)=delete;
    BnwSelectedLanguageInputOwner&operator=(const BnwSelectedLanguageInputOwner&)=delete;
};
}
