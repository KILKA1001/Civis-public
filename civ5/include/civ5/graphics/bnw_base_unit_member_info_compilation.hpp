#pragma once
#include "civ5/assets/resource_mounts.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/save_preflight.hpp"
#include <array>
#include <set>
namespace civ5::ui::detail {class BnwNativeCompletedUnitArtInputs;}
namespace civ5::graphics {
struct BnwUnitMemberInfoSourceRecord {
    std::string request,sha256;
    std::optional<std::uint64_t> mount;
    assets::Provenance provenance;
};
// Source54AB70 fixed XML list and seven-column info insertion only. Combat,
// weapons and UnitInfos are separate mandatory stages, never inferred complete.
// Apply before host sealing and before DLC/mod database actions. No selected
// host mutation, cloned DB authority, XML-to-compiledScale shortcut or float API.
class BnwBaseUnitMemberInfoCompilation {
  public:
    static BnwBaseUnitMemberInfoCompilation compile(
        const assets::ResourceMountTable& same_source_stage,
        const runtime::SavedContentProfile& selected_profile);
    static BnwBaseUnitMemberInfoCompilation compile_from_completed_native_inputs(
        const std::shared_ptr<const ui::detail::BnwNativeCompletedUnitArtInputs>&,
        const runtime::SavedContentProfile&);
    void apply_to_unsealed_database_from_completed_native_inputs(content::RulesDatabase&,
        const std::shared_ptr<const ui::detail::BnwNativeCompletedUnitArtInputs>&,
        const runtime::SavedContentProfile&)const;
    std::span<const BnwUnitMemberInfoSourceRecord> sources() const {return sources_;}
    std::size_t member_count() const {return statements_.size();}
    void apply_to_unsealed_database(content::RulesDatabase&,
        const assets::ResourceMountTable& same_source_stage,
        const runtime::SavedContentProfile& same_selected_profile) const;
  private:
    friend class BnwBaseUnitArtCompilation;
    struct CompletedDocument {std::string sha256;content::XmlNode xml;};
    static std::shared_ptr<const ui::detail::BnwNativeCompletedUnitArtInputs> canonical_completed_inputs(
        const std::shared_ptr<const ui::detail::BnwNativeCompletedUnitArtInputs>&);
    static CompletedDocument read_completed_document(const ui::detail::BnwNativeCompletedUnitArtInputs&,
        unsigned,const std::string* expected_sha256=nullptr);
    static void append_document(BnwBaseUnitMemberInfoCompilation&,const content::XmlNode&,std::set<std::string>&);
    std::shared_ptr<const ui::detail::BnwNativeCompletedUnitArtInputs> completed_inputs_;
    std::array<std::string,3> completed_sha256_{};
    const assets::ResourceMountTable* resources_=nullptr;
    std::string profile_key_;
    std::vector<BnwUnitMemberInfoSourceRecord> sources_;
    std::vector<content::DatabaseStatement> statements_;
};
} // namespace civ5::graphics

