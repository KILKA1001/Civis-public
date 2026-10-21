#pragma once
#include "civ5/graphics/bnw_warrior_idle_window_producer.hpp"
namespace civ5::graphics {
struct BnwWarriorNativeSkeletonBone {
    std::string_view name;
    std::int32_t parent;
    // Actual raw152-byte bone+4C storage order, not a guessed SDK transpose.
    std::array<float,16> inverse_bind;
};
struct BnwWarriorNativeBoneMatrices {
    std::array<std::array<float,16>,23> world;
    std::array<std::array<float,12>,20> packed_mesh_palette;
};
inline void validate_bnw_warrior_native_affine(const std::array<float,16>& m) {
    warrior_curve_require(std::ranges::all_of(m,[](float x){return std::isfinite(x);})&&m[3]==0&&m[7]==0&&m[11]==0&&m[15]==1,"Warrior native finite affine matrix");
}
// Selected no-scale-shear source995370/994F90 route. Root placement is explicit;
// local23, skeleton and inverse bind must share the same source coordinate basis.
// Source model-basis/scale adoption and actual controller ownership are separate.
inline BnwWarriorNativeBoneMatrices build_bnw_warrior_native_idle_bone_matrices(std::span<const BnwWarriorIdleLocalTransform> local,std::span<const BnwWarriorNativeSkeletonBone> skeleton,std::span<const std::uint32_t> existing_mesh_palette,const std::array<float,16>& initial_placement) {
    warrior_curve_require(local.size()==23&&skeleton.size()==23&&existing_mesh_palette.size()==20,"Warrior native skeleton/local/palette cardinality");validate_bnw_warrior_native_affine(initial_placement);
    BnwWarriorNativeBoneMatrices out{};
    for(unsigned bone=0;bone<23;++bone) {
        const auto& b=skeleton[bone];const auto& pose=local[bone];
        warrior_curve_require(!b.name.empty()&&std::count_if(skeleton.begin(),skeleton.end(),[&](const auto& v){return v.name==b.name;})==1&&b.parent>=-1&&b.parent<int(bone),"Warrior native names/parent ordering");validate_bnw_warrior_native_affine(b.inverse_bind);
        const auto& p=pose.position_orientation_scale;warrior_curve_require(pose.flags<=3&&std::ranges::all_of(p,[](float x){return std::isfinite(x);}),"Warrior native unsupported scale/flags or nonfinite local pose");
        // Source flag4 is authoritative. Raw bind rows may retain inactive
        // nonidentity shear bytes even when bit4 is clear; do not reinterpret
        // those unused fields as an active scale or silently set the flag.
        const auto& parent=b.parent==-1?initial_placement:out.world[std::size_t(b.parent)];auto& m=out.world[bone];
        if(pose.flags&2) {
            // Exact988980 SSE products/doubling and parent-row evaluation order.
            const float x=p[3],y=p[4],z=p[5],w=p[6];
            auto twice=[](float a,float b){const float v=a*b;return v+v;};
            const float xx=twice(x,x),yy=twice(y,y),zz=twice(z,z);
            const float xy=twice(y,x),xz=twice(x,z),yz=twice(z,y),wx=twice(x,w),wy=twice(w,y),wz=twice(w,z);
            const float yyzz=zz+yy,xxzz=xx+zz,xxyy=xx+yy;
            const std::array<std::array<float,3>,3> r{{{1.f-yyzz,xy+wz,xz-wy},{xy-wz,1.f-xxzz,yz+wx},{xz+wy,yz-wx,1.f-xxyy}}};
            for(unsigned row=0;row<3;++row)for(unsigned col=0;col<4;++col){const float a=r[row][0]*parent[col],b0=r[row][1]*parent[4+col],c=r[row][2]*parent[8+col];const float pair=a+b0;m[row*4+col]=pair+c;}
            for(unsigned col=0;col<4;++col){const float a=p[0]*parent[col],b0=p[1]*parent[4+col],c=p[2]*parent[8+col];const float pair=a+b0,translated=pair+parent[12+col];m[12+col]=translated+c;}
        } else {
            m=parent; // Actual source flags0 uses raw copy, including signed zero.
            if(pose.flags&1) {
                // Source982740 has distinct scalar addition order in each lane.
                const float x0=parent[8]*p[2],y0=parent[4]*p[1],z0=p[0]*parent[0];const float xy0=x0+y0,xyz0=xy0+z0;m[12]=xyz0+parent[12];
                const float x1=parent[1]*p[0],y1=parent[9]*p[2],z1=parent[5]*p[1];const float xy1=x1+y1,xyz1=xy1+z1;m[13]=xyz1+parent[13];
                const float x2=parent[2]*p[0],y2=parent[10]*p[2],z2=parent[6]*p[1];const float xy2=x2+y2,xyz2=xy2+z2;m[14]=xyz2+parent[14];
            }
        }
        validate_bnw_warrior_native_affine(m);
    }
    // Source995010 ->9826D0 ->9B01F0 ->9AFC60: inverse*world with four
    // unfused left-associated terms, then transpose first three columns to the
    // native48-byte uniformBoneMatrices record. Palette is caller's same owner.
    for(unsigned member=0;member<20;++member) {
        const auto bone=existing_mesh_palette[member];warrior_curve_require(bone<23,"Warrior native existing mesh palette index");const auto& inv=skeleton[bone].inverse_bind;const auto& world=out.world[bone];auto& packed=out.packed_mesh_palette[member];
        for(unsigned row=0;row<4;++row)for(unsigned col=0;col<3;++col){const float a=inv[row*4]*world[col],b=inv[row*4+1]*world[4+col],c=inv[row*4+2]*world[8+col],d=inv[row*4+3]*world[12+col];const float ab=a+b,abc=ab+c;packed[col*4+row]=abc+d;}
        warrior_curve_require(std::ranges::all_of(packed,[](float x){return std::isfinite(x);}),"Warrior native palette matrix overflow");
    }
    return out;
}
inline BnwWarriorNativeBoneMatrices sample_bnw_warrior_produced_idle_bone_matrices(std::span<const BnwWarriorIdleTrack> tracks,std::span<const BnwWarriorNativeSkeletonBone> skeleton,const BnwWarriorPoseAnimTimeInput& time,std::span<const BnwWarriorIdleTrackWindowOperands> operands,std::span<const std::uint32_t> existing_mesh_palette,const std::array<float,16>& initial_placement) {
    warrior_curve_require(skeleton.size()==23,"Warrior native sampled skeleton count");std::array<std::string_view,23> names;for(unsigned i=0;i<23;++i)names[i]=skeleton[i].name;
    const auto local=sample_bnw_warrior_produced_idle_local_pose(tracks,names,time,operands);return build_bnw_warrior_native_idle_bone_matrices(local,skeleton,existing_mesh_palette,initial_placement);
}
} // namespace civ5::graphics
