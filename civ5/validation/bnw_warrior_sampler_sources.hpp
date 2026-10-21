#pragma once
#include "bnw_warrior_material_sources.hpp"
#include "civ5/graphics/bnw_warrior_sampler_policy.hpp"
namespace civ5::graphics {
inline BnwWarriorNativeSamplerPolicy load_bnw_warrior_sampler_policy(const BnwWarriorMaterialSources &material,
    const std::filesystem::path &sampler_metadata,unsigned member,std::int32_t unit,std::uint64_t revision) {
    if(material.resources.size()!=9)throw std::invalid_argument("Missing actual Warrior texture resources");
    const auto raw=warrior_read_bounded(sampler_metadata,12000);
    const auto sha=runtime::identify_bytes(assets::Bytes(raw.begin(),raw.end())).sha256;
    return {material.metadata_sha256,sha,member,unit,revision};
}
} // namespace civ5::graphics
