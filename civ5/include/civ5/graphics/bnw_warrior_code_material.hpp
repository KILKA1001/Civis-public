#pragma once
#include "civ5/graphics/bnw_warrior_material.hpp"
#include "civ5/graphics/bnw_warrior_team_color.hpp"
#include <cstdint>
#include <string>

namespace civ5::graphics {


// Native 67CD00 initializes the renderer's 32 triples at instance+54.
// This is fresh owned renderer state, not restored native animation state.
class BnwWarriorInitialAlphaOwner {
    std::array<std::array<float,3>,32> values_;
    std::int32_t unit_;
    std::uint64_t revision_;
public:
    BnwWarriorInitialAlphaOwner(std::int32_t unit,std::uint64_t revision):unit_(unit),revision_(revision) {
        if(unit<0||revision==0)throw std::invalid_argument("Unknown Warrior alpha owner identity");
        for(auto &v:values_)v={1.f,0.f,0.f};
    }
    std::array<float,3> member(std::int32_t unit,std::uint64_t revision,unsigned mesh_member)const {
        if(unit!=unit_||revision!=revision_||mesh_member>=values_.size())
            throw std::invalid_argument("Foreign/stale Warrior alpha owner or member");
        return values_[mesh_member];
    }
};
struct BnwWarriorCodeMaterialSamples {
    BnwWarriorMaterialSamples rgb;
    BnwWarriorCodeAlphaInputs alpha;
};
inline WarriorColor evaluate_bnw_warrior_code_material_samples(const BnwWarriorCodeMaterialSamples &s) {
    auto out=evaluate_bnw_warrior_material_samples(s.rgb);
    out[3]=evaluate_bnw_warrior_code_alpha(s.alpha);return out;
}
// CODE uses the same RGB shader arithmetic as main opaque54. Its sole output
// difference is the source PS water/AlphaValuesS expression. All fields are
// required uniforms; there is no zero-water or authored-alpha fallback.
inline const std::string bnw_warrior_code_material_fragment=[] {
    std::string s(bnw_warrior_main_material_fragment);
    const std::string declaration="uniform vec4 TeamColorS;";
    const std::string output="warriorPixel=vec4(encoded+fog*(f3FogColor-encoded),1.0);";
    auto p=s.find(declaration);if(p==std::string::npos)throw std::logic_error("Opaque material declaration changed");
    s.replace(p,declaration.size(),declaration+"\nuniform float fWaterHeight;\nuniform vec3 AlphaValuesS;");
    p=s.find(output);if(p==std::string::npos)throw std::logic_error("Opaque material output changed");
    s.replace(p,output.size(),"precise float alphaDelta=AlphaValuesS.y-AlphaValuesS.x;\n precise float alpha=AlphaValuesS.z*alphaDelta+AlphaValuesS.x;\n precise float waterGate=warriorWorld.z>=fWaterHeight?1.0:0.330000013;\n warriorPixel=vec4(encoded+fog*(f3FogColor-encoded),alpha*waterGate);");
    return s;
}();
} // namespace civ5::graphics
