#pragma once
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include "civ5/graphics/bnw_warrior_material.hpp"
namespace civ5::graphics {
// GameplayUnitCreated -> GetPlayerColor -> PlayerColors.SecondaryColor -> Colors.
// This source projection must be produced from the live session/rules owner;
// civilization defaults do not substitute for CvPreGame::playerColor(owner).
struct BnwWarriorTeamColorSource {
    std::int32_t player_id, unit_id;
    std::uint64_t scene_revision;
    std::string selected_player_color_type, secondary_color_type;
    WarriorColor secondary_rgba;
};
struct BnwWarriorTeamColorReceipt {
    BnwWarriorTeamColorSource source;
    std::uint32_t packed_rgba;
    WarriorColor team_color_s;
};
inline std::uint32_t pack_bnw_warrior_secondary_rgba(const WarriorColor &rgba) {
    std::uint32_t packed=0;
    for(float channel:rgba) {
        if(!std::isfinite(channel)||channel<0.f||channel>1.f)
            throw std::invalid_argument("Unknown/out-of-range live Warrior secondary RGBA");
        // 51CD59..51CE1A: x87 multiply by exactly255, FISTP in truncate mode.
        // Float input * integer255 is exact in double, before truncation.
        const auto byte=static_cast<std::uint32_t>(static_cast<double>(channel)*255.0);
        packed=(packed<<8)+byte;
    }
    return packed;
}
inline WarriorColor unpack_bnw_warrior_team_color_s(std::uint32_t packed) {
    // 5F29D0: memory bytes[3,2,1,0] times pinned float32 0x3b808081.
    constexpr float factor=std::bit_cast<float>(std::uint32_t{0x3b808081});
    return {float((packed>>24)&255)*factor,float((packed>>16)&255)*factor,
            float((packed>>8)&255)*factor,float(packed&255)*factor};
}
inline BnwWarriorTeamColorReceipt project_bnw_warrior_team_color(
    const BnwWarriorTeamColorSource &source,std::int32_t expected_player,
    std::int32_t expected_unit,std::uint64_t expected_revision) {
    if(source.player_id<0||source.unit_id<0||source.player_id!=expected_player||
       source.unit_id!=expected_unit||source.scene_revision!=expected_revision||
       source.selected_player_color_type.empty()||source.secondary_color_type.empty())
        throw std::invalid_argument("Missing/foreign live Warrior player-color projection");
    const auto packed=pack_bnw_warrior_secondary_rgba(source.secondary_rgba);
    return {source,packed,unpack_bnw_warrior_team_color_s(packed)};
}
inline constexpr std::string_view bnw_warrior_code_ps_sha=
 "dc1ec109d194df604c88b1ddd94cf499fd194af5d073c4ac09bd7d3e249f843c";
inline constexpr unsigned bnw_warrior_code_shader_family=0x5a;
// PS05DA0AF8 adds c0=water height and c5=member AlphaValuesS to opaque RGB.
// These values require actual owners; there is intentionally no default.
struct BnwWarriorCodeAlphaInputs {
    float world_z, water_height;
    std::array<float,3> alpha_values_s;
};
inline float evaluate_bnw_warrior_code_alpha(const BnwWarriorCodeAlphaInputs &s) {
    if(!std::isfinite(s.world_z)||!std::isfinite(s.water_height))
        throw std::invalid_argument("Unknown Warrior native water-height input");
    for(float a:s.alpha_values_s)if(!std::isfinite(a)||a<0.f||a>1.f)
        throw std::invalid_argument("Unknown Warrior native member alpha");
    const float water_gate=s.world_z>=s.water_height?1.f:std::bit_cast<float>(std::uint32_t{0x3ea8f5c3});
    const float delta=s.alpha_values_s[1]-s.alpha_values_s[0];
    return (s.alpha_values_s[2]*delta+s.alpha_values_s[0])*water_gate;
}
} // namespace civ5::graphics
