#pragma once
#include "civ5/content/mod_activation.hpp"
#include "civ5/content/rules_bootstrap.hpp"
#include <functional>

namespace civ5::graphics { class BnwBaseUnitArtCompilation; }
namespace civ5::ui::detail {class BnwNativeCompletedUnitArtInputs;}

namespace civ5::content {
// Ordered retained assets selected by the SAME localization activation picker.
// Null package means base text; package indices retain the caller's activation
// order. This API does not discover/activate packages or infer native readiness.
struct SelectedLocalizationSource {
    std::optional<std::size_t> package;
    assets::AssetHandle asset;
};
std::vector<SelectedLocalizationSource> selected_localization_sources(
    const std::filesystem::path &game_root,std::span<const std::size_t> package_indices,
    const Catalog &catalog,const assets::ResourceMountTable &resources);

struct LocalizationPackageActivationReport {
    std::size_t package = 0;
    std::string id;
    std::uint64_t requested = 0, resolved = 0, missing = 0, applied = 0, failed = 0;
};
struct LocalizationActivationReport {
    bool base_schema_applied = false;
    bool attached = false;
    std::uint64_t base_requested = 0, base_resolved = 0, base_applied = 0, base_failed = 0;
    std::vector<LocalizationPackageActivationReport> packages;
};
struct SavedRulesActivationReport {
    runtime::ContentProfileKind profile_kind = runtime::ContentProfileKind::vanilla;
    bool profile_can_activate = false;
    LocalizationActivationReport localization;
    NativeGameplayReport base;
    DlcGameplayReport dlc;
    DatabaseActivationPlan mod_plan;
    DatabaseActivationReport mods;
    bool rules_database_ready = false;
};
// Apply the selected save profile into one isolated RulesDatabase. The order
// is base gameplay, saved DLC GameData, optional caller-owned art import,
// then saved mod UpdateDatabase actions. The optional callback must leave the
// database ready for mod actions; failures propagate. This does not deserialize
// the map/players or execute DLL/Lua/EXE.
SavedRulesActivationReport activate_saved_rules(
    RulesDatabase &database, const std::filesystem::path &game_root,
    const runtime::SavedContentProfile &profile, const Catalog &catalog,
    const assets::ResourceMountTable &resources,
    std::uint64_t source_budget = 64ULL * 1024 * 1024,
    const std::function<void(RulesDatabase &)> &before_mod_actions = {},
    std::span<const std::size_t> use_installed_bytes = {},
    std::span<const runtime::VerifiedManifestHash> verified_hashes = {});

// Source-ordered stock art path: base schema/gameplay -> typed base unit art
// -> DLC -> mods. The producer validates its SAME early table/profile/payloads
// before applying. The rules resolver is a distinct epoch, not relabelled as
// the early table. This API alone does not authenticate a client or final host.
SavedRulesActivationReport activate_saved_rules_with_base_unit_art(
    RulesDatabase&, const std::filesystem::path& game_root,
    const runtime::SavedContentProfile&, const Catalog&,
    const assets::ResourceMountTable&,
    const graphics::BnwBaseUnitArtCompilation&,
    const assets::ResourceMountTable& same_early_art_resources,
    std::uint64_t source_budget = 64ULL * 1024 * 1024);

// Closed actual native art input branch at the SAME pre-DLC art slot. Generic
// resources below serve rules/localization, not art source or native authority.
SavedRulesActivationReport activate_saved_rules_with_completed_base_unit_art(
    RulesDatabase&,const std::filesystem::path& game_root,
    const runtime::SavedContentProfile&,const Catalog&,
    const assets::ResourceMountTable& rules_resources,
    const graphics::BnwBaseUnitArtCompilation&,
    const std::shared_ptr<const ui::detail::BnwNativeCompletedUnitArtInputs>&,
    std::uint64_t source_budget=64ULL*1024*1024);
} // namespace civ5::content
