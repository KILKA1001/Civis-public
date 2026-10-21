#pragma once
#include "civ5/assets/asset_source.hpp"
#include "civ5/content/manifest.hpp"
#include "civ5/graphics/bnw_unit_visual_owner.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <cmath>
#include <optional>

namespace civ5::graphics {
struct BnwUnitMemberTransformRules {
    float scale=1,z_offset=0;
    bool operator==(const BnwUnitMemberTransformRules &) const=default;
};
// Native compiled-art constructor 5816B0 and loader 622390. This is the
// model's authored scale value, not proof of final native draw consumption.
inline BnwUnitMemberTransformRules compile_bnw_unit_member_transform_rules(
    std::optional<float> authored_scale,std::optional<float> authored_z_offset,
    float global_unit_scale) {
    if(!std::isfinite(global_unit_scale)||global_unit_scale<=0 ||
       (authored_scale&&!std::isfinite(*authored_scale)) ||
       (authored_z_offset&&!std::isfinite(*authored_z_offset)))
        throw std::invalid_argument("Invalid authored unit member transform");
    const float scale=(authored_scale && *authored_scale>0 ? *authored_scale : 1.F)*global_unit_scale;
    if(!std::isfinite(scale)||scale<=0)throw std::invalid_argument("Unrepresentable compiled member scale");
    return {scale,authored_z_offset.value_or(0.F)};
}
struct BnwUnitMemberTransformSource {
    std::string type,xml_sha256;
    assets::Provenance provenance;
    BnwUnitMemberTransformRules rules;
};
// Caller resolves the actual selected XML through its mounted VFS. No path,
// missing file, rules revision, or global unit scale is silently substituted.
inline BnwUnitMemberTransformSource read_bnw_unit_member_transform_rules(
    const assets::AssetHandle &handle,std::string_view selected_type,float global_unit_scale) {
    if(selected_type.empty())throw std::invalid_argument("Missing selected member type");
    const auto bytes=handle.read_bytes(64*1024*1024);
    const auto xml=content::read_xml_metadata(std::string(bytes.begin(),bytes.end()));
    const content::XmlNode *selected=nullptr;
    for(const auto &row:xml.children) {
        const auto *type=row.child("Type");
        if(row.name=="UnitMemberArtInfo" && type && type->text==selected_type) {
            if(selected)throw std::invalid_argument("Duplicate selected member art");selected=&row;
        }
    }
    if(!selected)throw std::invalid_argument("Selected member art unavailable");
    const auto optional_float=[&](const char *name)->std::optional<float> {
        const auto *field=selected->child(name);if(!field)return std::nullopt;
        std::size_t consumed=0;const auto value=std::stof(field->text,&consumed);
        if(consumed!=field->text.size())throw std::invalid_argument("Invalid member numeric field");
        return value;
    };
    return {std::string(selected_type),runtime::identify_bytes(bytes).sha256,handle.candidate().provenance,
        compile_bnw_unit_member_transform_rules(optional_float("fScale"),optional_float("fZOffset"),global_unit_scale)};
}
inline std::array<std::array<float,16>,12> bnw_warrior_owned_scene_transforms_with_rules(
    const BnwImportedUnitVisualOwner &units,const saves::SavedSessionSnapshot &current,
    std::int64_t player,std::int64_t unit,const terrain::WorldLayout &world,
    const terrain::HeightOwnerState &height,bool wrap_x,float minimum_height,
    const BnwUnitMemberTransformRules &rules) {
    if(!std::isfinite(rules.z_offset))throw std::invalid_argument("Invalid member ZOffset");
    auto matrices=bnw_warrior_owned_scene_transforms(units,current,player,unit,world,height,wrap_x,minimum_height,rules.scale);
    for(auto &matrix:matrices) {
        matrix[14]+=rules.z_offset;
        if(!std::isfinite(matrix[14]))throw std::invalid_argument("Unrepresentable member world Z");
    }
    return matrices;
}
} // namespace civ5::graphics
