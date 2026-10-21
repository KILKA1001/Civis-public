#pragma once
#include "bnw_warrior_skin_diagnostic.hpp"
namespace civ5::graphics {
struct BnwWarriorMaterialPose {
    std::vector<float> vertex_pnuv;
    std::size_t nonzero_fourth_influences=0;
    bool native_three_influence_compatible=false;
};
// Retains all authored SDK influences, and passes RAW blended normals to the
// fragment stage. Native shader54 VS has only three influences; until native
// resource conversion is reproduced V2/V4 must not claim matching VS inputs.
inline BnwWarriorMaterialPose prepare_bnw_warrior_material_pose(
    const BnwWarriorSourceMesh &mesh,const BnwWarriorSkinDiagnostic &source,int sample) {
    BnwWarriorMaterialPose result;result.vertex_pnuv=source.pose(mesh,sample);
    const auto matrices=source.matrices(sample);bool order=true;
    for(std::size_t v=0;v<mesh.vertex_pnuv.size()/8;v++){
        const auto fourth=source.weights[v*4+3];result.nonzero_fourth_influences+=fourth!=0;
        order=order && !(source.weights[v*4+1]==0 && source.weights[v*4+2]!=0);
        std::array<double,3> raw{};
        for(unsigned j=0;j<4;j++){
            const auto weight=double(source.weights[v*4+j])/255;
            const auto &m=matrices[std::size_t(source.palette[std::size_t(source.indices[v*4+j])])];
            for(unsigned r=0;r<3;r++)for(unsigned c=0;c<3;c++)raw[r]+=weight*m[r*4+c]*mesh.vertex_pnuv[v*8+3+c];
        }
        double length2=0;for(float x:raw){if(!std::isfinite(x))throw std::runtime_error("Warrior raw material normal invalid");length2+=double(x)*x;}
        if(length2<1e-12)throw std::runtime_error("Warrior raw material normal zero");
        for(unsigned r=0;r<3;r++)result.vertex_pnuv[v*8+3+r]=float(raw[r]);
    }
    result.native_three_influence_compatible=order && result.nonzero_fourth_influences==0;
    return result;
}
} // namespace civ5::graphics
