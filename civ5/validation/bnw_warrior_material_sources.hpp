#pragma once
#include "bnw_warrior_geometry_cache.hpp"
#include "civ5/graphics/bnw_warrior_material.hpp"
namespace civ5::graphics {
inline constexpr const char *bnw_warrior_material_metadata_sha256="1b5ec3c71ae04c4ace02074e120ac828a8a5c36968163755208e37018f54c1d3";
struct BnwWarriorMaterialResource {std::string name,sha256;assets::DdsData data;};
struct BnwWarriorMaterialSources {
    std::string metadata_sha256;
    // 0..5: member diffuse/SREF pairs; 6:irradiance,7:dull,8:sharp cubes.
    std::vector<BnwWarriorMaterialResource> resources;
    const BnwWarriorMaterialResource &sampler(std::size_t member,unsigned slot) const {
        if(member>=3||slot>=5||resources.size()!=9)throw std::invalid_argument("Unsupported Warrior resource slot");
        return resources[slot<2?member*2+slot:slot+4];
    }
};
inline BnwWarriorMaterialSources load_bnw_warrior_material_sources(
    const std::filesystem::path &asset_root,const std::filesystem::path &cache,const std::filesystem::path &metadata) {
    if(!std::filesystem::equivalent(asset_root,"C:/CrazyProject/civ5-assets"))throw std::runtime_error("Canonical shared Warrior AssetRoot required");
    const auto raw=warrior_read_bounded(metadata,20000);
    const auto sha=runtime::identify_bytes(assets::Bytes(raw.begin(),raw.end())).sha256;
    require_bnw_warrior_asset_sha(sha,bnw_warrior_material_metadata_sha256);
    const assets::AssetEnvironment environment(asset_root);
    const auto models_result=environment.open_fpk_archive("Resource/Common/UnitModels.fpk");
    const auto textures_result=environment.open_fpk_archive("Resource/DX9/UnitsTextures.fpk");
    const auto *models=std::get_if<std::shared_ptr<assets::FpkSource>>(&models_result);
    const auto *textures=std::get_if<std::shared_ptr<assets::FpkSource>>(&textures_result);
    if(!models||!textures)throw std::runtime_error("Actual Warrior material archive unavailable");
    BnwWarriorMaterialSources result{sha,{}};
    const auto add=[&](const char *name,const char *hash,bool cube){
        auto bytes=warrior_archive_asset(*textures,name);require_bnw_warrior_asset_sha(runtime::identify_bytes(bytes).sha256,hash);
        auto dds=assets::DdsData::parse(std::move(bytes));
        if(dds.width()!=(cube?32u:128u)||dds.height()!=dds.width()||dds.face_count()!=(cube?6u:1u)||dds.mip_count()!=(cube?6u:8u)||dds.format()!=(cube?assets::DdsFormat::dxt1:assets::DdsFormat::dxt5))
            throw std::runtime_error("Actual Warrior material DDS layout changed");
        result.resources.push_back({name,hash,std::move(dds)});
    };
    for(const auto &m:bnw_gandhi_warrior_art){
        require_bnw_warrior_asset_sha(runtime::identify_file(cache/m.gr2_name).sha256,m.gr2_sha256);
        const auto gr2=warrior_archive_asset(*models,m.gr2_name);
        require_bnw_warrior_asset_sha(runtime::identify_bytes(gr2).sha256,m.gr2_sha256);
        const auto fxs=warrior_archive_asset(*models,m.fxs_name);
        require_bnw_warrior_asset_sha(runtime::identify_bytes(fxs).sha256,m.fxs_sha256);
        add(m.diffuse_name,m.diffuse_sha256,false);add(m.sref_name,m.sref_sha256,false);
    }
    add("unit_irradiance.dds","1e454181755a473eabda627edfe634a33907ce93624adde886eaed27c4518196",true);
    add("unit_environment_dull.dds","a2280baf9c227c93a26855f439655cf8d2193fdb0ffeaac7bdbeab4f0f9898a1",true);
    add("unit_environment_sharp.dds","518608500076198a4e3c0382fb42a01e187d44a1cab943762b244034036f2841",true);
    return result;
}
} // namespace civ5::graphics
