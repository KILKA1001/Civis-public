#pragma once
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;class BnwSelectedUserOptionsOwner;}
#include "civ5/assets/resource_mounts.hpp"
#include "civ5/content/manifest.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/bnw_stock_lua_client_extent.hpp"
#include "civ5/runtime/save_preflight.hpp"
#include <memory>
#include <span>

namespace civ5::graphics { class GlBnwFontShader; }
namespace civ5::ui { class BnwNativeEarlyInputLease; namespace detail { class CivisFrontendPartySceneSource; class CivisFrontendPartyWorldSource; } }
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;}
namespace civ5::assets {
namespace detail { struct CivisResourceBankState; class CivisLoadedXmlPhysicalPathLoan; class CivisLoadedBytesPhysicalPathLoan; }
struct CivisResourceBankBudget {
    std::uint64_t resident_bytes = 512ULL * 1024 * 1024;
    std::uint64_t resource_bytes = 64ULL * 1024 * 1024;
    std::size_t backing_files = 131072, indexed_names = 1048576;
};
enum class CivisResourceParse { bytes, xml };
enum class CivisResourceSelection { winner, all_matching };
struct CivisRequiredResource {
    std::string name;
    CivisResourceParse parse = CivisResourceParse::bytes;
    CivisResourceSelection selection = CivisResourceSelection::winner;
};
struct CivisResourceMountDescriptor {
    std::uint64_t id, group, priority;
    bool enabled, preferred;
};
struct CivisResourceBankSourceRecipe {
    std::filesystem::path save_path, game_root, canonical_payload_root;
    content::Catalog catalog;
    runtime::SavedContentProfile profile;
    std::vector<CivisResourceMountDescriptor> mounts;
    std::string recipe_identity;
};
struct CivisLoadedResource {
    AssetCandidate candidate;
    runtime::FileIdentity payload_identity;
    std::shared_ptr<const Bytes> bytes;
    std::shared_ptr<const content::XmlNode> xml;
};
// Own Civis session authority only. It is not an Original completed-resource
// bank, native-32 receipt, rules-database readiness, or game activation token.
class CivisResourceBankCapability final {
  public:
    CivisResourceBankCapability(const CivisResourceBankCapability&) = delete;
    CivisResourceBankCapability& operator=(const CivisResourceBankCapability&) = delete;
    CivisResourceBankCapability(CivisResourceBankCapability&&) = delete;
    void require_current() const; // Pure creator/owner/epoch checks.
    CivisLoadedResource loaded(const AssetKey&) const; // Pure; never lazy IO.
  private:
    friend class CivisResourceBankOwner;
    friend class ::civ5::runtime::BnwWorldViewSourceOperationLoan;
    std::shared_ptr<const detail::CivisLoadedXmlPhysicalPathLoan> loaded_xml_physical_path_owned(const AssetKey&)const;
    std::shared_ptr<const detail::CivisLoadedBytesPhysicalPathLoan> loaded_bytes_physical_path_owned(const AssetKey&)const;
    std::weak_ptr<detail::CivisResourceBankState> state_;
    std::uint64_t epoch_;
    explicit CivisResourceBankCapability(
        const std::shared_ptr<detail::CivisResourceBankState>&);
};
class CivisResourceBankOwner final {
  public:
    // Verified selected zero-mod BNW profile; desktop Common-platform DX9
    // recipe. Required closure is staged before publication. No caller bool.
    static std::shared_ptr<CivisResourceBankOwner> capture(
        const std::shared_ptr<const ui::BnwNativeEarlyInputLease>&,
        std::span<const CivisRequiredResource>, CivisResourceBankBudget = {});
    ~CivisResourceBankOwner();
    CivisResourceBankOwner(const CivisResourceBankOwner&) = delete;
    CivisResourceBankOwner& operator=(const CivisResourceBankOwner&) = delete;
    // Canonical factory control block; consumers must compare owner_before as
    // well as get() before retaining a supplied public shared pointer.
    std::shared_ptr<CivisResourceBankOwner> retain_owner() const;
    CivisResourceBankCapability capability() const;
    // Explicit IO boundary. Loaded bytes are never silently refreshed.
    void load(const AssetKey&, CivisResourceParse = CivisResourceParse::bytes);
    void load_all(const AssetKey&, CivisResourceParse = CivisResourceParse::bytes);
    // Checks actual client/recipe anchors and held loaded-source identities.
    // Full frozen namespace path validation occurs once at initial publication.
    // Actual retained input HWND value only; no window/audio activation grant.
    std::uintptr_t actual_hwnd() const;
    runtime::BnwStockLuaClientExtent actual_client_extent() const;
    void require_boundary() const;
    void require_current() const;
    const ResourceMountTable& resources() const;
    bool has_indexed(const AssetKey&) const;
    CivisResourceBankSourceRecipe source_recipe() const;
    std::string_view recipe_id() const { require_current(); return "Civis.BNW.ZeroMod.Common.DX9.v1"; }
    // Memory-only adapter. An indexed but unloaded resource throws; missing
    // names return monostate. Call load outside real Lua/client callbacks.
    const ResourceMountTable& resources(const CivisResourceBankCapability&) const;
  private:
    friend class CivisResourceBankCapability;
    friend class ::civ5::runtime::BnwWorldViewSourceOperationLoan;
    friend class ::civ5::ui::detail::CivisFrontendPartySceneSource;
    friend class ::civ5::ui::detail::CivisFrontendPartyWorldSource;
    std::shared_ptr<const ui::BnwNativeEarlyInputLease> frontend_party_input_owned()const;
    std::vector<std::string> context_lua_include_names_owned()const;
    std::shared_ptr<const runtime::BnwSelectedUserOptionsOwner> context_selected_user_options_owned()const;
    friend class ::civ5::graphics::GlBnwFontShader;
    const std::filesystem::path& original_root_for_font_program_without_callbacks() const;
    void load_selected(const AssetKey&, CivisResourceParse, CivisResourceSelection);
    std::shared_ptr<detail::CivisResourceBankState> state_;
    explicit CivisResourceBankOwner(std::shared_ptr<detail::CivisResourceBankState>);
    static std::shared_ptr<const detail::CivisLoadedXmlPhysicalPathLoan> capture_loaded_xml_physical_path_owned(
        const std::shared_ptr<detail::CivisResourceBankState>&,const AssetKey&);
    static std::shared_ptr<const detail::CivisLoadedBytesPhysicalPathLoan> capture_loaded_bytes_physical_path_owned(
        const std::shared_ptr<detail::CivisResourceBankState>&,const AssetKey&);
    static std::string physical_id(const AssetHandle&);
    static void capture_handle(detail::CivisResourceBankState&, const AssetHandle&);
    static ResourceMountHit memory_lookup(
        const std::weak_ptr<detail::CivisResourceBankState>&, const AssetKey&);
    static std::vector<ResourceMountHit> memory_lookup_all(
        const std::weak_ptr<detail::CivisResourceBankState>&, const AssetKey&);
};
} // namespace civ5::assets
