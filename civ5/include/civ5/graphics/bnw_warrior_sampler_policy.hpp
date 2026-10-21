#pragma once
#include "civ5/graphics/bnw_warrior_sampler_records.hpp"
#include <string_view>
namespace civ5::graphics {
inline constexpr std::string_view bnw_warrior_sampler_metadata_sha="69ff533c080ee9eb9c99d86d61d6cec09db99c7dd9f4d74070b641e3ad65e78e";
inline constexpr std::string_view bnw_warrior_authored_material_sha="1b5ec3c71ae04c4ace02074e120ac828a8a5c36968163755208e37018f54c1d3";
// UnitLibrary72FA60/5C2E10 ->58CD60 null base+288;966FF0 authored
// texture branch writes sampler0.756140 replaces cubes2..4 and fog5 with0.
// Scope is the three verified UnitShader_Skinned Warrior source resources only.
class BnwWarriorNativeSamplerPolicy {
    unsigned member_;
    std::int32_t unit_;
    std::uint64_t revision_;
public:
    BnwWarriorNativeSamplerPolicy(std::string_view material_sha,std::string_view sampler_sha,
        unsigned member,std::int32_t unit,std::uint64_t revision):member_(member),unit_(unit),revision_(revision) {
        if(material_sha!=bnw_warrior_authored_material_sha||sampler_sha!=bnw_warrior_sampler_metadata_sha||
           member>=3||unit<0||revision==0)throw std::invalid_argument("Missing/foreign source Warrior sampler policy");
    }
    BnwWarriorSamplerRecord slot(unsigned slot,unsigned member,std::int32_t unit,std::uint64_t revision)const {
        if(slot>=6||member!=member_||unit!=unit_||revision!=revision_)
            throw std::invalid_argument("Foreign/stale Warrior sampler owner or slot");
        return resolve_bnw_native_sampler_record(0);
    }
    unsigned member()const{return member_;}
    std::int32_t unit()const{return unit_;}
    std::uint64_t revision()const{return revision_;}
};
} // namespace civ5::graphics
