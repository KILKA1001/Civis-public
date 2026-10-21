#pragma once
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <stdexcept>
namespace civ5::graphics {
struct BnwWarriorGr2ArtToolMetadata {
    float units_per_meter=0;
    std::array<float,3> origin{},right{},up{},back{};
};
struct BnwWarriorGr2NativeBasis {
    float uniform_scale=0;
    std::array<float,9> linear{};
};
// Restricted actual Warrior metadata, not a general Granny basis converter.
// Native963B10 targets right(1,0,0), up(0,0,1), back(0,-1,0).
// Source SDK metadata is exactly that basis. Native9B3753 applies Float32
// (sourceUnits * compiledScale) / sourceUnits to its identity basis.
inline BnwWarriorGr2NativeBasis compile_bnw_warrior_gr2_native_basis(
    const BnwWarriorGr2ArtToolMetadata &source,float compiled_scale) {
    constexpr float units=39.370079040527344f;
    if(std::bit_cast<std::uint32_t>(source.units_per_meter)!=std::bit_cast<std::uint32_t>(units) ||
       source.origin!=std::array<float,3>{0,0,0} || source.right!=std::array<float,3>{1,0,0} ||
       source.up!=std::array<float,3>{0,0,1} || source.back!=std::array<float,3>{0,-1,0})
        throw std::invalid_argument("Unsupported Warrior source GR2 ArtTool basis");
    if(!std::isfinite(compiled_scale) || compiled_scale<=0)
        throw std::invalid_argument("Invalid compiled Warrior model scale");
    // Volatile materializes native MULSS rounding before native DIVSS.
    volatile float requested_units=source.units_per_meter*compiled_scale;
    const float ratio=requested_units/source.units_per_meter;
    if(!std::isfinite(ratio) || ratio<=0)
        throw std::invalid_argument("Unrepresentable Warrior native model scale");
    return {ratio,{ratio,0,0,0,ratio,0,0,0,ratio}};
}
} // namespace civ5::graphics
