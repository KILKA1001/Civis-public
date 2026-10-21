#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
#include <string_view>
namespace civ5::graphics {
// Native9B22D0/default98D460: first exact case-sensitive named bone match.
// This helper does not pretend to own an installed custom comparer callback.
inline std::optional<std::uint32_t> lookup_bnw_warrior_native_bone(
    std::span<const std::string_view> skeleton,std::string_view name) {
    if(name.empty())throw std::invalid_argument("Missing GSD bone name");
    for(std::size_t i=0;i<skeleton.size();++i){
        if(skeleton[i].empty())throw std::invalid_argument("Anonymous GSD skeleton bone");
        if(skeleton[i]==name)return std::uint32_t(i);
    }
    return std::nullopt;
}
// Current three Warrior meshes have20 bindings to a23-bone skeleton.
// Buffer indices remain in mesh-binding space; gather BoneMatrices through
// this palette. Never reinterpret packed indices as direct skeleton indices.
inline std::array<std::uint32_t,20> bind_bnw_warrior_native_mesh_palette(
    std::span<const std::string_view> bindings,std::span<const std::string_view> skeleton) {
    if(bindings.size()!=20||skeleton.size()!=23)throw std::invalid_argument("Unknown selected Warrior mesh binding cardinality");
    std::array<std::uint32_t,20> result;
    for(unsigned i=0;i<20;++i){
        const auto bone=lookup_bnw_warrior_native_bone(skeleton,bindings[i]);
        // Original994C80 logs missing bones and writes0. Unknown required
        // binding is a readiness error in Civis, not a fabricated bone0.
        if(!bone)throw std::invalid_argument("Required Warrior mesh binding unavailable");
        result[i]=*bone;
    }
    return result;
}
} // namespace civ5::graphics
