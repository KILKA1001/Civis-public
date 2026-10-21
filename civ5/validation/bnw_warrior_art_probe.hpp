#pragma once

#include "civ5/assets/asset_source.hpp"
#include "civ5/assets/dds.hpp"
#include "civ5/content/manifest.hpp"
#include "civ5/graphics/gl_texture.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <array>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>

namespace civ5::graphics {

struct BnwWarriorArtMember {
    const char *art_type;
    const char *fxs_name;
    const char *fxs_sha256;
    const char *gr2_name;
    const char *gr2_sha256;
    const char *diffuse_name;
    const char *diffuse_sha256;
    const char *sref_name;
    const char *sref_sha256;
    int count;
};

inline constexpr std::array<BnwWarriorArtMember,3> bnw_gandhi_warrior_art{{
    {"ART_DEF_UNIT_MEMBER_WARRIOR_V2", "warrior_v2.fxsxml",
     "4898273a63688986e87548510be04e431927e10d21d877035c04bbf42461cdee",
     "warrior_v2.gr2", "6101693ea454cbdfe1a026903fd30ca6489596a462994eddc21465f8b3740272",
     "warrior02_diff.dds", "27ff00793ea2cbcd8a7eec97a83b6e30d505c3b1bcfd158c2efcb8d54106686b",
     "warrior02_sref.dds", "d22cce8615bff74e31f730ba5763e3d7ab73c79d6a4abfe6f0f77352b8547130",3},
    {"ART_DEF_UNIT_MEMBER_WARRIOR", "warrior.fxsxml",
     "849cfc87d68bbae5830faf3ced3913686acb60803c6ed2e86e485fde8873fc35",
     "warrior.gr2", "9d34effba92d5dcc4340951bce65a08d620af2ee688cabf60d0bab8464ac0429",
     "warrior_diff.dds", "b5ca3af3c5584da59e1ecfb9c3c83871792c5f02493641a8f777ac1033b5cd1c",
     "warrior_sref.dds", "5c9a3f48ddd68989bb5de7d6fb141fc8bd0976a95190b6c449a795636c913654",6},
    {"ART_DEF_UNIT_MEMBER_WARRIOR_V4", "warrior_v4.fxsxml",
     "e340cb002f3ad74096c2847c0a0ab3bc4df797ad9f8e2c790a50307f15d002ec",
     "warrior_v4.gr2", "524fe9979ce361e8e486b49dff6fcc8e145dae268a26003aac0adb23bd52728d",
     "warrior04_diff.dds", "9c6dedea8a85891f5444374e419742df8f2eea69168ab02c22682ea5a6eb2fd8",
     "warrior04_sref.dds", "447f5b23a206c40222dd13e300a284904c95cb9144452bad49fa6a40ab42f4e3",3}
}};

inline void require_bnw_warrior_asset_sha(std::string_view observed,
                                           std::string_view expected) {
    if (observed!=expected)
        throw std::runtime_error("Installed BNW Warrior art asset identity changed");
}

inline const content::XmlNode &warrior_xml_child(const content::XmlNode &node,
                                                 const char *name) {
    const auto *child=node.child(name);
    if(!child) throw std::runtime_error("Incomplete BNW Warrior art XML");
    return *child;
}

inline assets::Bytes warrior_archive_asset(const std::shared_ptr<assets::FpkSource> &source,
                                           const char *name) {
    const auto result=source->find(assets::AssetKey(name));
    const auto *handle=std::get_if<assets::AssetHandle>(&result);
    if(!handle) throw std::runtime_error("Missing or ambiguous BNW Warrior archive entry");
    return handle->read_bytes(1'000'000);
}

// This verifies the installed BNW rule/art/FXS chain, then uploads and reads
// back each authored BC3 diffuse/SREF mip. No GR2 vertices, skeleton or
// animation are decoded, so this path never emits a model drawable.
inline std::uint32_t probe_bnw_gandhi_warrior_art(
    const std::filesystem::path &root, const GlContext &context) {
    const auto game=content::read_xml(root /
        "Assets/DLC/Expansion2/Gameplay/XML/Units/CIV5Units.xml");
    const auto &rows=warrior_xml_child(game,"Units");
    bool rule_found=false;
    for(const auto &row:rows.children) {
        const auto *type=row.child("Type");
        if(row.name!="Row" || !type || type->text!="UNIT_WARRIOR") continue;
        if(rule_found || warrior_xml_child(row,"UnitArtInfo").text!=
                          "ART_DEF_UNIT__WARRIOR")
            throw std::runtime_error("Active BNW Warrior rule art binding differs");
        rule_found=true;
    }
    if(!rule_found) throw std::runtime_error("Active BNW Warrior rule missing");
    const auto units=content::read_xml(root / "Assets/Units/Civ5ArtDefines_Units.xml");
    const content::XmlNode *unit_art=nullptr;
    for(const auto &row:units.children) {
        const auto *type=row.child("Type");
        if(row.name=="UnitArtInfo" && type && type->text=="ART_DEF_UNIT__WARRIOR") {
            if(unit_art) throw std::runtime_error("Duplicate BNW Warrior art profile");
            unit_art=&row;
        }
    }
    if(!unit_art || warrior_xml_child(*unit_art,"Formation").text!="Warrior" ||
        warrior_xml_child(*unit_art,"DamageStates").text!="1")
        throw std::runtime_error("BNW Warrior formation differs");
    std::size_t member_index=0;
    for(const auto &row:unit_art->children) {
        if(row.name!="UnitMemberArt") continue;
        if(member_index>=bnw_gandhi_warrior_art.size() ||
            warrior_xml_child(row,"MemberType").text!=
                bnw_gandhi_warrior_art[member_index].art_type ||
            warrior_xml_child(row,"MemberCount").text!=
                std::to_string(bnw_gandhi_warrior_art[member_index].count))
            throw std::runtime_error("BNW Warrior member formation differs");
        ++member_index;
    }
    if(member_index!=bnw_gandhi_warrior_art.size())
        throw std::runtime_error("Incomplete BNW Warrior member formation");
    const auto members=content::read_xml(root /
        "Assets/Units/Civ5ArtDefines_UnitMembers.xml");
    const assets::AssetEnvironment environment(root);
    const auto models_open=environment.open_fpk_archive("Resource/Common/UnitModels.fpk");
    const auto textures_open=environment.open_fpk_archive("Resource/DX9/UnitsTextures.fpk");
    const auto *models=std::get_if<std::shared_ptr<assets::FpkSource>>(&models_open);
    const auto *textures=std::get_if<std::shared_ptr<assets::FpkSource>>(&textures_open);
    if(!models || !textures)
        throw std::runtime_error("BNW Warrior model or texture FPK unavailable");
    std::uint32_t gpu_mips=0;
    for(const auto &spec:bnw_gandhi_warrior_art) {
        const content::XmlNode *member=nullptr;
        for(const auto &row:members.children) {
            const auto *type=row.child("Type");
            if(row.name=="UnitMemberArtInfo" && type && type->text==spec.art_type) {
                if(member) throw std::runtime_error("Duplicate BNW Warrior member art");
                member=&row;
            }
        }
        if(!member || warrior_xml_child(*member,"fScale").text!="0.14" ||
            assets::normalize_asset_name(std::filesystem::path(
                warrior_xml_child(*member,"Granny").text).filename().string())!=
                spec.fxs_name)
            throw std::runtime_error("BNW Warrior member scale/FXS differs");
        const auto fxs=warrior_archive_asset(*models,spec.fxs_name);
        require_bnw_warrior_asset_sha(runtime::identify_bytes(fxs).sha256,spec.fxs_sha256);
        const auto fxs_xml=content::read_xml_metadata(std::string(fxs.begin(),fxs.end()));
        if(fxs_xml.name!="Asset" ||
            assets::normalize_asset_name(
                warrior_xml_child(fxs_xml,"Mesh").attribute("file"))!=spec.gr2_name)
            throw std::runtime_error("BNW Warrior FXS mesh binding differs");
        std::array<std::string,2> expected_textures{spec.diffuse_name,spec.sref_name};
        for(const auto &name:expected_textures) {
            bool found=false;
            for(const auto &child:fxs_xml.children)
                if(child.name=="Texture" &&
                    assets::normalize_asset_name(child.attribute("file"))==name)
                    found=true;
            if(!found) throw std::runtime_error("BNW Warrior FXS texture binding differs");
        }
        const auto gr2=warrior_archive_asset(*models,spec.gr2_name);
        require_bnw_warrior_asset_sha(runtime::identify_bytes(gr2).sha256,spec.gr2_sha256);
        constexpr std::array<std::uint8_t,16> granny_magic{
            0x29,0xde,0x6c,0xc0,0xba,0xa4,0x53,0x2b,
            0x25,0xf5,0xb7,0xa5,0xf6,0x66,0xe2,0xee};
        if(gr2.size()<granny_magic.size() ||
           !std::equal(granny_magic.begin(),granny_magic.end(),gr2.begin()))
            throw std::runtime_error("BNW Warrior GR2 header differs");
        for(const auto &[name,sha]:std::array<std::pair<const char *,const char *>,2>{{
                {spec.diffuse_name,spec.diffuse_sha256},
                {spec.sref_name,spec.sref_sha256}}}) {
            auto raw=warrior_archive_asset(*textures,name);
            require_bnw_warrior_asset_sha(runtime::identify_bytes(raw).sha256,sha);
            auto dds=assets::DdsData::parse(std::move(raw));
            if(dds.width()!=128 || dds.height()!=128 || dds.mip_count()!=8 ||
               dds.format()!=assets::DdsFormat::dxt5 || dds.face_count()!=1)
                throw std::runtime_error("BNW Warrior BC3 texture layout differs");
            GlTexture gpu(context,dds,DdsSamplingColor::linear);
            if(!gpu.belongs_to(context))
                throw std::runtime_error("BNW Warrior texture lost its GL context");
            for(std::uint32_t mip=0;mip<dds.mip_count();++mip) {
                const auto source=dds.payload(0,mip);
                const auto readback=gpu.read_level_raw(0,mip);
                if(readback.size()!=source.size() ||
                   !std::equal(readback.begin(),readback.end(),source.begin()))
                    throw std::runtime_error("BNW Warrior BC3 GL mip readback differs");
                ++gpu_mips;
            }
        }
    }
    return gpu_mips;
}

} // namespace civ5::graphics
