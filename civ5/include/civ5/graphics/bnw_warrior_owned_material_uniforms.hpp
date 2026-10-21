#pragma once
#include "civ5/graphics/bnw_warrior_material_frame.hpp"
#include "civ5/graphics/bnw_warrior_team_color.hpp"
namespace civ5::graphics {
// Source global21 c0x and dynamic member c5 use the SAME retained frame owner.
// World Z remains the actual supplied generated transform/camera draw input.
// This fresh-init producer does not claim save-native ocean/renderer restoration.
inline BnwWarriorCodeAlphaInputs bnw_warrior_code_alpha_from_owned_frame(
    const BnwWarriorMaterialFrameOwner& owner,std::int32_t unit,std::uint64_t revision,
    unsigned member,float world_z) {
    if(!std::isfinite(world_z))throw std::invalid_argument("Missing Warrior material world Z");
    BnwWarriorCodeAlphaInputs result{world_z,owner.water_height(unit,revision),owner.slot(member,unit,revision).alpha};
    // The shared CODE CPU/GL contract admits native visibility alpha[0..1].
    // Arbitrary scalar differential inputs never bypass that required gate.
    static_cast<void>(evaluate_bnw_warrior_code_alpha(result));return result;
}
} // namespace civ5::graphics
