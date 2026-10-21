#pragma once
#include "civ5/graphics/bnw_warrior_native_resource_basis.hpp"
namespace civ5::graphics {
// Separate animation resource963420 has its own actual ArtTool metadata owner.
// Both verified artifacts use the source9B3490 recipe and same semantic key.
inline BnwWarriorNativeResourceBasis compile_bnw_warrior_native_animation_resource_basis(const BnwWarriorGr2ArtToolMetadata& metadata,std::string_view actual_animation_metadata_identity,float compiled_scale) {
    warrior_curve_require(actual_animation_metadata_identity=="bf3c9b495f73081b6b4ad7b47d389f48a3cd7841d2ae66e8dcfd8e59be8a137b","Foreign actual IdleA ArtTool metadata identity");
    return warrior_resource_detail::compile_resource_basis_recipe(metadata,compiled_scale);
}
enum class BnwWarriorResourceCurveChannel : unsigned { position,orientation,scale_shear };
struct BnwWarriorNativeResourceCurve {BnwWarriorNativeResourceBasisKey basis_key;BnwWarriorResourceCurveChannel channel;BnwWarriorIdleCurveData encoded;};
namespace warrior_resource_detail {
inline void resource_curve_store_float(std::vector<std::uint8_t>& bytes,unsigned offset,float value) {
    warrior_curve_require(std::isfinite(value)&&offset+4<=bytes.size(),"Converted resource curve finite/bounded field");
    const auto word=std::bit_cast<std::uint32_t>(value);for(unsigned i=0;i<4;++i)bytes[offset+i]=std::uint8_t(word>>(i*8));
}
// Source99E550 returns false for the selected positive uniform monomial basis.
// Source99EC20 then constructs the actual identity quaternion permutation from
// three test rotations (.8 around each axis, W=.6), using source990D30. Refuse
// the general decode/recompress route until its codec producer is implemented.
inline void resource_curve_permutation_route(const BnwWarriorNativeResourceBasis& basis) {
    validate_bnw_warrior_native_resource_basis(basis);
    const float product=basis.linear[0]*basis.inverse[0],delta=product-1.f,error=delta*delta;
    warrior_curve_require(std::isfinite(error)&&error<=std::bit_cast<float>(0x2edbe6feu),"Source curve general basis route unsupported");
    for(unsigned axis=0;axis<3;++axis) {
        std::array<float,4> q{0,0,0,std::bit_cast<float>(0x3f19999au)};q[axis]=std::bit_cast<float>(0x3f4ccccdu);
        q=warrior_resource_quaternion(q,basis.linear,basis.inverse);
        if(q[3]<0.f)for(auto& v:q)v=-v;
        warrior_curve_require(q[axis]>.75f,"Source curve orientation axis/sign permutation unsupported");
        for(unsigned j=0;j<3;++j)if(j!=axis)warrior_curve_require(std::fabs(q[j])<=.75f,"Source curve orientation permutation unsupported");
    }
}
} // namespace warrior_resource_detail
// Convert the compressed source HEADER/PAYLOAD before any sampling. The actual
// selected raw curve artifact is verified by the caller's loader. No clock,
// controller, decoded track replacement, matrix or post-sampling scale is used.
inline BnwWarriorNativeResourceCurve convert_bnw_warrior_native_resource_curve(const BnwWarriorIdleCurveData& raw,BnwWarriorResourceCurveChannel channel,std::string_view verified_raw_curve_artifact,const BnwWarriorNativeResourceBasis& basis) {
    warrior_curve_require(verified_raw_curve_artifact=="b0330e1ebf35834d2997f6c702efcf109af34e33f935337acd60300d07ad4409","Foreign actual raw IdleA curve artifact");
    const auto index=unsigned(channel);warrior_curve_require(index<3&&raw.dimension==std::array<unsigned,3>{3,4,9}[index],"Resource curve channel declaration");
    const auto original=decode_bnw_warrior_idle_curve(raw);(void)original;
    warrior_resource_detail::resource_curve_permutation_route(basis);
    BnwWarriorNativeResourceCurve out{basis.key,channel,raw};
    // Source99F660 skips identity curves. All selected shear curves are identity.
    if(raw.format==2)return out;
    warrior_curve_require(channel!=BnwWarriorResourceCurveChannel::scale_shear,"Nonidentity resource shear codec unsupported");
    if(channel==BnwWarriorResourceCurveChannel::position) {
        warrior_curve_require(raw.format==4||raw.format==18,"Selected source position codec");
        for(unsigned j=0;j<3;++j) {
            const float factor=basis.linear[j*4];
            const float product=warrior_curve_float(raw.header,4+j*4)*factor;
            const float scale_or_control=raw.format==4?product+basis.translation[j]:product;
            warrior_resource_detail::resource_curve_store_float(out.encoded.header,4+j*4,scale_or_control);
            if(raw.format==18) {const float offset=warrior_curve_float(raw.header,16+j*4)*factor;warrior_resource_detail::resource_curve_store_float(out.encoded.header,16+j*4,offset+basis.translation[j]);}
        }
    } else {
        warrior_curve_require(raw.format==5||raw.format==8||raw.format==9,"Selected source orientation codec");
        if(raw.format==5)for(unsigned j=0;j<4;++j) {const float product=warrior_curve_float(raw.header,4+j*4)*1.f;warrior_resource_detail::resource_curve_store_float(out.encoded.header,4+j*4,product+0.f);}
        // Source9A6180/9A5F80 identity permutation: nibble ranges, quantized
        // components, omitted index and omitted sign are reconstructed unchanged.
        // The route above proved that permutation/sign selection before this copy.
    }
    (void)decode_bnw_warrior_idle_curve(out.encoded);return out;
}
struct BnwWarriorRawIdleTrack {std::string name;std::array<BnwWarriorIdleCurveData,3> curves;};
struct BnwWarriorNativeResourceTracks {BnwWarriorNativeResourceBasisKey basis_key;std::array<BnwWarriorRawIdleTrack,23> encoded;};
inline BnwWarriorNativeResourceTracks convert_bnw_warrior_native_resource_tracks(std::span<const BnwWarriorRawIdleTrack> raw,std::string_view verified_raw_curve_artifact,const BnwWarriorNativeResourceBasis& basis) {
    warrior_curve_require(raw.size()==23,"Resource source track cardinality");BnwWarriorNativeResourceTracks out{};out.basis_key=basis.key;
    for(unsigned i=0;i<23;++i) {warrior_curve_require(!raw[i].name.empty()&&std::count_if(raw.begin(),raw.end(),[&](const auto& track){return track.name==raw[i].name;})==1,"Resource source track names");out.encoded[i].name=raw[i].name;
        for(unsigned channel=0;channel<3;++channel)out.encoded[i].curves[channel]=convert_bnw_warrior_native_resource_curve(raw[i].curves[channel],BnwWarriorResourceCurveChannel(channel),verified_raw_curve_artifact,basis).encoded;
    }return out;
}
// Projection consumed by the existing native window/local-pose owner. Conversion
// retains encoded data; this projection creates no independent mutable clock.
inline std::array<BnwWarriorIdleTrack,23> decode_bnw_warrior_native_resource_tracks(const BnwWarriorNativeResourceTracks& source,const BnwWarriorNativeResourceBasis& basis) {
    validate_bnw_warrior_native_resource_basis(basis);warrior_curve_require(source.basis_key==basis.key,"Converted tracks/basis identity mismatch");
    std::array<BnwWarriorIdleTrack,23> out{};for(unsigned i=0;i<23;++i){out[i].name=source.encoded[i].name;for(unsigned j=0;j<3;++j)out[i].curves[j]=decode_bnw_warrior_idle_curve(source.encoded[i].curves[j]);}return out;
}
} // namespace civ5::graphics
