#pragma once
#include "civ5/graphics/bnw_warrior_native_clip_time.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>
namespace civ5::graphics {
struct BnwWarriorIdleCurveData {
    std::uint32_t format,degree,dimension,knot_count;
    std::vector<std::uint8_t> header,payload;
};
struct BnwWarriorDecodedIdleCurve {
    std::uint32_t format,degree,dimension;
    float quantizer;
    std::vector<std::uint32_t> quantized_knots;
    std::vector<float> knots,controls;
};
inline void warrior_curve_require(bool ok,const char* message) {
    if(!ok)throw std::invalid_argument(message);
}
inline std::uint32_t warrior_curve_word(std::span<const std::uint8_t> bytes,std::size_t at,unsigned count=4) {
    warrior_curve_require(count<=4&&at+count<=bytes.size(),"Warrior curve byte extent");
    std::uint32_t value=0;for(unsigned i=0;i<count;++i)value|=std::uint32_t(bytes[at+i])<<(i*8);return value;
}
inline float warrior_curve_float(std::span<const std::uint8_t> bytes,std::size_t at) {
    const auto value=std::bit_cast<float>(warrior_curve_word(bytes,at));
    warrior_curve_require(std::isfinite(value),"Warrior curve nonfinite source scalar");return value;
}
// Actual retailB56910 range/offset table; signed second half is significant.
inline constexpr std::array<std::uint32_t,32> warrior_curve_range_bits{
    0x3fb504f3,0xbf3504f3,0x3f3504f3,0xbeb504f3,0x3eb504f3,0xbf07c3b6,0x3eb504f3,0xbe3504f3,
    0x3eb504f3,0x3e3504f3,0x3e3504f3,0xbe3504f3,0x3e3504f3,0xbdb504f3,0x3e3504f3,0,
    0xbfb504f3,0x3f3504f3,0xbf3504f3,0x3eb504f3,0xbeb504f3,0x3f07c3b6,0xbeb504f3,0x3e3504f3,
    0xbeb504f3,0xbe3504f3,0xbe3504f3,0x3e3504f3,0xbe3504f3,0x3db504f3,0xbe3504f3,0x80000000};
inline BnwWarriorDecodedIdleCurve decode_bnw_warrior_idle_curve(const BnwWarriorIdleCurveData& source) {
    const auto f=source.format,d=source.dimension,n=source.knot_count;
    const auto expected=f==2?4u:f==4?16u:f==5?20u:(f==8||f==9)?16u:f==18?36u:0u;
    warrior_curve_require(expected&&source.header.size()==expected&&source.header[0]==f&&source.header[1]==source.degree,"Warrior selected codec/header identity");
    BnwWarriorDecodedIdleCurve out{f,source.degree,d,0,{},{},{}};
    if(f==2) {
        warrior_curve_require(source.degree==0&&n==0&&(d==3||d==4||d==9)&&warrior_curve_word(source.header,2,2)==d&&source.payload.empty(),"Warrior identity curve declaration");
        out.knots={0};out.controls.resize(d);if(d==4)out.controls[3]=1;else if(d==9)out.controls[0]=out.controls[4]=out.controls[8]=1;
        return out;
    }
    if(f==4||f==5) {
        warrior_curve_require(source.degree==0&&n==1&&d==(f==4?3u:4u)&&source.payload.empty(),"Warrior constant curve declaration");
        out.knots={0};for(unsigned j=0;j<d;++j)out.controls.push_back(warrior_curve_float(source.header,4+j*4));return out;
    }
    const auto width=f==8?2u:1u,count_offset=f==18?28u:8u,per_knot=f==18?2u:4u;
    warrior_curve_require(source.degree==2&&n>=3&&n<=1024&&d==(f==18?3u:4u),"Warrior degree2 curve dimension/count");
    warrior_curve_require(warrior_curve_word(source.header,count_offset)==n*per_knot&&warrior_curve_word(source.header,count_offset+4)==0&&source.payload.size()==std::size_t(n)*per_knot*width,"Warrior bounded pointer-free packed array");
    out.quantizer=f==18?std::bit_cast<float>(warrior_curve_word(source.header,2,2)<<16):warrior_curve_float(source.header,4);
    warrior_curve_require(std::isfinite(out.quantizer)&&out.quantizer>0,"Warrior positive native knot quantizer");
    const float inverse=1.f/out.quantizer;
    for(unsigned k=0;k<n;++k) {
        const auto q=warrior_curve_word(source.payload,k*width,width);out.quantized_knots.push_back(q);out.knots.push_back(float(q)*inverse);
        warrior_curve_require(std::isfinite(out.knots.back())&&(k==0||out.knots[k]>=out.knots[k-1]),"Warrior nondecreasing finite knots");
    }
    if(f==18) {
        for(unsigned k=0;k<n;++k)for(unsigned j=0;j<3;++j) {
            const float product=float(source.payload[n+k])*warrior_curve_float(source.header,4+j*4);
            out.controls.push_back(product+warrior_curve_float(source.header,16+j*4));
        }
    }else {
        const auto ranges=warrior_curve_word(source.header,2,2),mask=f==8?0x7fffu:0x7fu,sign=f==8?0x8000u:0x80u,shift=f==8?14u:6u;
        const float ratio=std::bit_cast<float>(f==8?0x38000100u:0x3c010204u);
        std::array<float,4> scales{},offsets{};
        for(unsigned j=0;j<4;++j) {
            const auto index=((ranges>>(j*4))&15)*2;
            scales[j]=std::bit_cast<float>(warrior_curve_range_bits[index])*ratio;
            offsets[j]=std::bit_cast<float>(warrior_curve_range_bits[index+1]);
        }
        for(unsigned k=0;k<n;++k) {
            std::array<std::uint32_t,3> packed{};for(unsigned j=0;j<3;++j)packed[j]=warrior_curve_word(source.payload,(n+k*3+j)*width,width);
            const auto omitted=((packed[1]&sign)|(packed[2]>>1))>>shift;
            warrior_curve_require(omitted<4,"Warrior omitted quaternion component");std::array<float,4> value{};std::array<float,3> decoded{};
            for(unsigned j=0;j<3;++j) {
                const auto component=(omitted+1+j)&3;
                const float product=float(packed[j]&mask)*scales[component];decoded[j]=value[component]=product+offsets[component];
            }
            const float z2=decoded[2]*decoded[2],y2=decoded[1]*decoded[1],x2=decoded[0]*decoded[0];
            const float yz=z2+y2,xyz=yz+x2,missing=1.f-xyz;
            warrior_curve_require(missing>=0,"Warrior native quaternion square root invalid; no clamp");
            value[omitted]=std::sqrt(missing);if(packed[0]&sign)value[omitted]=-value[omitted];
            out.controls.insert(out.controls.end(),value.begin(),value.end());
        }
    }
    for(float value:out.controls)warrior_curve_require(std::isfinite(value),"Warrior decoded source control nonfinite");
    return out;
}
// Bounded retail degree2 branch. Looped windows are explicitly unsupported.
// Required caller flags and local clock are not inferred from an Idle FSM.
// Normalization is native99D260 sqrt/divide, not approximate SDK normalization.
inline std::vector<float> sample_bnw_warrior_idle_curve(const BnwWarriorDecodedIdleCurve& curve,float local_clock,float duration,bool normalize,bool back_loop,bool forward_loop) {
    warrior_curve_require(std::bit_cast<std::uint32_t>(duration)==0x40c00001u&&std::isfinite(local_clock)&&local_clock>=0&&local_clock<=duration,"Warrior pinned IdleA local clock range");
    warrior_curve_require(!back_loop&&!forward_loop,"Warrior native looped curve window remains unsupported");
    warrior_curve_require(curve.dimension==3||curve.dimension==4||curve.dimension==9,"Warrior sample dimension");
    const auto d=curve.dimension;const auto n=curve.knots.size();
    warrior_curve_require(!curve.controls.empty()&&curve.controls.size()==n*d&&std::ranges::all_of(curve.controls,[](float x){return std::isfinite(x);}),"Warrior sample decoded controls extent");
    if(curve.degree==0) {
        warrior_curve_require((curve.format==2||curve.format==4||curve.format==5)&&n==1,"Warrior constant sample declaration");
        return curve.controls; // Source constant/identity branch does not renormalize.
    }
    warrior_curve_require(curve.degree==2&&((curve.format==18&&d==3)||((curve.format==8||curve.format==9)&&d==4))&&n>=3&&curve.quantized_knots.size()==n&&std::isfinite(curve.quantizer)&&curve.quantizer>0&&std::is_sorted(curve.quantized_knots.begin(),curve.quantized_knots.end())&&std::is_sorted(curve.knots.begin(),curve.knots.end()),"Warrior degree2 sample declaration");
    const float scaled=local_clock*curve.quantizer;
    warrior_curve_require(std::isfinite(scaled)&&scaled<2147483648.f,"Warrior native floor-int quantizer overflow");
    const auto q=std::uint32_t(std::floor(scaled))&(curve.format==8?65535u:255u);
    const auto upper=std::upper_bound(curve.quantized_knots.begin(),curve.quantized_knots.end(),q);
    const auto index=std::min<std::ptrdiff_t>(std::distance(curve.quantized_knots.begin(),upper),std::ptrdiff_t(n)-1);
    std::array<float,4> knots{};std::array<std::array<float,4>,4> controls{};
    for(std::ptrdiff_t k=0;k<4;++k) {
        const auto source=std::clamp<std::ptrdiff_t>(index-2+k,0,std::ptrdiff_t(n)-1);
        knots[std::size_t(k)]=curve.knots[std::size_t(source)];
        for(unsigned j=0;j<d;++j)controls[std::size_t(k)][j]=curve.controls[std::size_t(source)*d+j];
    }
    if(normalize&&d==4) {
        std::array<float,4> previous{};
        for(auto& control:controls) {
            const float x=control[0]*previous[0],y=control[1]*previous[1],z=control[2]*previous[2],w=control[3]*previous[3];
            const float xy=x+y,xyz=xy+z,dot=xyz+w;if(dot<0)for(float& value:control)value=-value;previous=control;
        }
    }
    const float first_den=knots[2]-knots[0],second_den=knots[2]-knots[1],third_den=knots[3]-knots[1];
    warrior_curve_require(first_den>0&&second_den>0&&third_den>0,"Warrior source degree2 denominators unavailable");
    const float first=(local_clock-knots[0])/first_den,second=(local_clock-knots[1])/second_den;
    const float sum=first+second,product=first*second,combined=sum-product;
    const float next=(local_clock-knots[1])/third_den,right=next*second,middle=combined-right,left=1.f-combined;
    std::vector<float> out(d);
    for(unsigned j=0;j<d;++j) {
        const float first_term=controls[0][j]*left,second_term=controls[1][j]*middle,third_term=controls[2][j]*right;
        const float pair=first_term+second_term;out[j]=pair+third_term;
    }
    if(normalize&&d==4) {
        const float x=out[0]*out[0],y=out[1]*out[1],z=out[2]*out[2],w=out[3]*out[3];
        const float xy=x+y,xyz=xy+z,norm=xyz+w;warrior_curve_require(norm>0&&std::isfinite(norm),"Warrior native normalization invalid");
        const float inverse=1.f/std::sqrt(norm);for(float& value:out)value*=inverse;
    }
    for(float value:out)warrior_curve_require(std::isfinite(value),"Warrior sampled local value nonfinite");return out;
}
struct BnwWarriorIdleTrack {std::string name;std::array<BnwWarriorDecodedIdleCurve,3> curves;};
struct BnwWarriorIdleLocalTransform {std::uint32_t flags;std::array<float,16> position_orientation_scale;};
// Reuses caller's actual skeleton order. Neither a new palette nor a scene owner
// is created here. Every named SDK track must bind exactly once to that roster.
inline std::array<BnwWarriorIdleLocalTransform,23> sample_bnw_warrior_idle_local_pose(std::span<const BnwWarriorIdleTrack> tracks,std::span<const std::string_view> skeleton_names,const BnwWarriorPoseAnimTimeInput& time) {
    warrior_curve_require(tracks.size()==23&&skeleton_names.size()==23,"Warrior local transform roster cardinality");
    const auto clock=map_bnw_warrior_native_clip_time(time);
    warrior_curve_require(!clock.curve_boundary_wrap,"Warrior current ClampedLoop requires unsupported looped window");
    std::array<BnwWarriorIdleLocalTransform,23> out{};
    for(std::size_t bone=0;bone<23;++bone) {
        warrior_curve_require(!skeleton_names[bone].empty()&&std::count(skeleton_names.begin(),skeleton_names.end(),skeleton_names[bone])==1,"Warrior skeleton identity missing/duplicate");
        const auto track=std::find_if(tracks.begin(),tracks.end(),[&](const auto& value){return value.name==skeleton_names[bone];});
        warrior_curve_require(track!=tracks.end()&&std::count_if(tracks.begin(),tracks.end(),[&](const auto& value){return value.name==skeleton_names[bone];})==1,"Warrior track/skeleton binding missing/duplicate");
        std::size_t at=0;for(unsigned channel=0;channel<3;++channel) {
            warrior_curve_require(track->curves[channel].dimension==std::array<unsigned,3>{3,4,9}[channel],"Warrior named channel dimensions differ");
            const auto values=sample_bnw_warrior_idle_curve(track->curves[channel],clock.curve_local_clock,time.duration,channel==1,false,false);
            if(track->curves[channel].format!=2)out[bone].flags|=1u<<channel;
            for(float value:values)out[bone].position_orientation_scale[at++]=value;
        }
    }
    return out;
}
} // namespace civ5::graphics
