#pragma once
#include "civ5/assets/resource_mounts.hpp"
#include "civ5/graphics/bnw_unit_member_transform_rules.hpp"
#include "civ5/runtime/file_identity.hpp"
namespace civ5::ui {class BnwNativePackageDiscoveryOwner;class BnwNativeUserDirectoryOwner;class SelectedBnwPreviewHost;}
namespace civ5::graphics {
struct BnwUnitVisIniPendingEffect {
    std::filesystem::path target;
    unsigned source_getter_ordinal=0;
    std::string serialized_bytes,serialized_sha256;
};
struct BnwUnitArtScaleConfigRecord {
    bool file_present=false,scale_defaulted=false,multiplier_defaulted=false,seed_defaulted=false,threaded_defaulted=false;
    runtime::FileIdentity file;
    std::filesystem::path source_path;
    // Explicit source getter flush request; does not claim persisted INI parity.
    unsigned source_ini_flush_requests=0;
    bool input_file_present=false;
    runtime::FileIdentity input_file;
    std::vector<BnwUnitVisIniPendingEffect> pending_effects;
};
struct BnwUnitArtScaleFrame {
    float initial_asset_global_scale=0,current_global_scale=0;
    float caller_scale=0,caller_member_multiplier=0;
    std::int32_t caller_placement_seed=-1,caller_threaded_render=1;
    bool caller_fields_owned=false,publish_global=false,source_transition_applied=false;
    std::uint64_t source_command_revision=0,library_reload_revision=0;
    std::string initial_config_sha256,rules_sha256;
    assets::Provenance initial_config_provenance;
    std::optional<std::uint64_t> initial_config_mount;
    BnwUnitArtScaleConfigRecord caller_configuration;
};
class BnwUnitArtScaleCaller;
class BnwUnitCompiledMemberScaleQuote {
  public:
    const BnwUnitMemberTransformSource& xml_declaration() const;
    const BnwUnitMemberTransformRules& compiled_rules() const;
    const std::string& model() const;
    // Resource consumes compiled scale. Existing placement receives scale1.
    BnwUnitMemberTransformRules placement_after_resource_conversion() const;
  private:
    friend class BnwUnitArtScaleCaller;
    struct Data;std::shared_ptr<const Data> data_;
    explicit BnwUnitCompiledMemberScaleQuote(std::shared_ptr<const Data>);
};
// Owns the scale-dependent SOURCE COMMAND ReloadUnitSystem, literal flags2 at
// 51ECC5/6CDD3B. Its name does not certify primary429511 boot mode. Unknown
// initial caller fields cannot compile a member until this command is applied.
class BnwUnitArtScaleCaller {
  public:
    static std::shared_ptr<BnwUnitArtScaleCaller> create(
        std::shared_ptr<const ui::BnwNativePackageDiscoveryOwner> same_resources,
        std::shared_ptr<const ui::BnwNativeUserDirectoryOwner> same_directory,
        std::shared_ptr<const ui::SelectedBnwPreviewHost> same_rules_host);
    ~BnwUnitArtScaleCaller();
    void apply_reload_unit_system_scale_stage();
    void reload_unit_libraries();
    const BnwUnitArtScaleFrame& current() const;
    BnwUnitCompiledMemberScaleQuote compile_member(std::string_view selected_type) const;
    void validate_quote(const BnwUnitCompiledMemberScaleQuote&) const;
    const assets::ResourceMountTable& resources_for_quote(const BnwUnitCompiledMemberScaleQuote&) const;
  private:
    struct Impl;std::shared_ptr<Impl> impl_;
    explicit BnwUnitArtScaleCaller(std::shared_ptr<Impl>);
};
} // namespace civ5::graphics


