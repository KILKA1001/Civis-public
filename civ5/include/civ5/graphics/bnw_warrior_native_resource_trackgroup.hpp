#pragma once
#include "civ5/graphics/bnw_warrior_native_resource_curves.hpp"
#include <optional>
namespace civ5::graphics {
struct BnwWarriorRawIdleTrackgroupFields {
    std::string name;
    BnwWarriorIdleLocalTransform initial_placement;
    std::array<float,3> accumulated_loop;
    std::optional<std::array<float,12>> periodic_loop;
    std::uint32_t flags=0,vector_tracks=0,text_tracks=0,transform_tracks=23,transform_lods=0;
};
struct BnwWarriorNativeResourceTrackgroup {
    BnwWarriorNativeResourceTracks tracks;
    BnwWarriorRawIdleTrackgroupFields fields;
};
namespace warrior_resource_detail {
// Source99F730/99F7D2: source ordered Float32 dot products; no translation.
inline std::array<float,3> resource_trackgroup_vector(std::array<float,3> v,const std::array<float,9>& b) {
    const float a0=b[2]*v[2],b0=b[1]*v[1],c0=b[0]*v[0],pair0=a0+b0;
    const float a1=b[5]*v[2],b1=b[4]*v[1],c1=b[3]*v[0],pair1=a1+b1;
    const float a2=b[6]*v[0],b2=b[8]*v[2],c2=b[7]*v[1],pair2=a2+b2;
    return {pair0+c0,pair1+c1,pair2+c2};
}
// Source numeric recipe only: controlled nonnull periodic fixtures do not claim
// that a periodic descriptor is present in the actual selected IdleA artifact.
inline BnwWarriorRawIdleTrackgroupFields resource_trackgroup_fields(BnwWarriorRawIdleTrackgroupFields out,const BnwWarriorNativeResourceBasis& basis) {
    validate_bnw_warrior_native_resource_basis(basis);
    warrior_curve_require(out.initial_placement.flags<=7&&std::ranges::all_of(out.initial_placement.position_orientation_scale,[](float v){return std::isfinite(v);})&&std::ranges::all_of(out.accumulated_loop,[](float v){return std::isfinite(v);}),"Trackgroup finite initial/accumulated fields");
    out.accumulated_loop=resource_trackgroup_vector(out.accumulated_loop,basis.linear);
    if(out.periodic_loop) {
        auto& p=*out.periodic_loop;warrior_curve_require(std::ranges::all_of(p,[](float v){return std::isfinite(v);}),"Trackgroup finite periodic fields");
        for(unsigned i=0;i<3;++i){auto v=resource_trackgroup_vector({p[3+i*3],p[4+i*3],p[5+i*3]},basis.linear);std::copy(v.begin(),v.end(),p.begin()+3+i*3);}
        for(unsigned index:std::array<unsigned,3>{2,0,1}) {
            const unsigned at=3+index*3;const float xx=p[at]*p[at],yy=p[at+1]*p[at+1],zz=p[at+2]*p[at+2],pair=xx+yy,sum=pair+zz;
            warrior_curve_require(std::isfinite(sum),"Periodic vector length overflow");float length=float(std::sqrt(double(sum)));
            if(length>std::bit_cast<float>(0x3727c5acu)){const float reciprocal=1.f/length;for(unsigned j=0;j<3;++j)p[at+j]=p[at+j]*reciprocal;}
            else{length=0.f;for(unsigned j=0;j<3;++j)p[at+j]=0.f;}
            // Actual99FAA4 updates +0;99FA25 updates +8. Source never rescales
            // scalar+4 for vector+18, including its zero-length branch.
            if(index!=1)p[index]=p[index]*length;
        }
        warrior_curve_require(std::ranges::all_of(p,[](float v){return std::isfinite(v);}),"Trackgroup periodic conversion overflow");
    }
    out.initial_placement=warrior_resource_local_transform(out.initial_placement,basis);
    warrior_curve_require(std::ranges::all_of(out.accumulated_loop,[](float v){return std::isfinite(v);}),"Trackgroup accumulated conversion overflow");return out;
}
} // namespace warrior_resource_detail
// Caller verifies the separate SDK trackgroup artifact and raw curve artifact.
// All fields and encoded tracks are converted on one basis BEFORE any sampling.
inline BnwWarriorNativeResourceTrackgroup convert_bnw_warrior_native_resource_trackgroup(const BnwWarriorRawIdleTrackgroupFields& fields,std::span<const BnwWarriorRawIdleTrack> raw,std::string_view verified_trackgroup_artifact,std::string_view verified_raw_curve_artifact,const BnwWarriorNativeResourceBasis& basis) {
    warrior_curve_require(verified_trackgroup_artifact=="e5b17d2ab593cb57bc098327707ee5d3cb6b029715bacb30febc5b08a6147239","Foreign actual IdleA trackgroup artifact");
    warrior_curve_require(fields.name=="Point_WORLD"&&fields.flags==0&&fields.vector_tracks==0&&fields.text_tracks==0&&fields.transform_tracks==23&&fields.transform_lods==0&&!fields.periodic_loop,"Unsupported selected actual trackgroup layout");
    const std::array<float,16> actual_initial{0,0,0,0,0,0,1,1,0,0,0,1,0,0,0,1};
    warrior_curve_require(fields.initial_placement.flags==0,"Actual IdleA initial flags differ");
    for(unsigned i=0;i<16;++i)warrior_curve_require(std::bit_cast<std::uint32_t>(fields.initial_placement.position_orientation_scale[i])==std::bit_cast<std::uint32_t>(actual_initial[i]),"Actual IdleA initial placement differs");
    for(float v:fields.accumulated_loop)warrior_curve_require(std::bit_cast<std::uint32_t>(v)==0,"Actual IdleA accumulated loop differs");
    return {convert_bnw_warrior_native_resource_tracks(raw,verified_raw_curve_artifact,basis),warrior_resource_detail::resource_trackgroup_fields(fields,basis)};
}
inline std::array<BnwWarriorIdleTrack,23> decode_bnw_warrior_native_resource_trackgroup(const BnwWarriorNativeResourceTrackgroup& source,const BnwWarriorNativeResourceBasis& basis) {return decode_bnw_warrior_native_resource_tracks(source.tracks,basis);}
} // namespace civ5::graphics
