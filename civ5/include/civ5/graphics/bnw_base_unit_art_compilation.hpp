#pragma once
#include "civ5/graphics/bnw_base_unit_member_info_compilation.hpp"
namespace civ5::graphics {
struct BnwBaseUnitArtStats {
    std::size_t unit_infos=0,info_members=0,member_infos=0,combats=0,weapons=0;
};
// Source54AB70/43E9D0/6ECBC0 stock art XML stage. These are unsealed engine
// rules, before actions and actual host sealing. No public SQL, paths, scale,
// cloned selected-host authority or post-seal mutation input.
class BnwBaseUnitArtCompilation {
 public:
    static BnwBaseUnitArtCompilation compile(const assets::ResourceMountTable&,
        const runtime::SavedContentProfile& selected_profile);
    static BnwBaseUnitArtCompilation compile_from_completed_native_inputs(
        const std::shared_ptr<const ui::detail::BnwNativeCompletedUnitArtInputs>&,
        const runtime::SavedContentProfile&);
    void require_same_completed_native_inputs(
        const std::shared_ptr<const ui::detail::BnwNativeCompletedUnitArtInputs>&,
        const runtime::SavedContentProfile&)const;
    void apply_to_unsealed_database_from_completed_native_inputs(content::RulesDatabase&,
        const std::shared_ptr<const ui::detail::BnwNativeCompletedUnitArtInputs>&,
        const runtime::SavedContentProfile&)const;
    std::span<const BnwUnitMemberInfoSourceRecord> sources() const{return sources_;}
    const BnwBaseUnitArtStats& stats() const{return stats_;}
    void apply_to_unsealed_database(content::RulesDatabase&,
        const assets::ResourceMountTable& same_source_stage,
        const runtime::SavedContentProfile& same_selected_profile) const;
 private:
    static void append_documents(BnwBaseUnitArtCompilation&,BnwBaseUnitMemberInfoCompilation&,
        const std::vector<content::XmlNode>&);
    std::shared_ptr<const ui::detail::BnwNativeCompletedUnitArtInputs> completed_inputs_;
    std::array<std::string,3> completed_sha256_{};
    const assets::ResourceMountTable* resources_=nullptr;
    std::string profile_key_;
    BnwBaseUnitArtStats stats_;
    std::vector<BnwUnitMemberInfoSourceRecord> sources_;
    std::vector<content::DatabaseStatement> statements_;
};
}
