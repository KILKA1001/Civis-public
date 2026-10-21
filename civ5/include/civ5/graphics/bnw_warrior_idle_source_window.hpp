#pragma once
#include "civ5/graphics/bnw_warrior_idle_curve.hpp"
namespace civ5::graphics {
// Numeric consumer of a SOURCE9B2410 output, not a replacement window producer.
// Neighbors/clock are explicit controlled operands until the native graph owner
// supplies them. No backend/controller/BoneMatrices readiness is implied.
struct BnwWarriorIdleSourceWindow {
    std::uint32_t format,dimension,knot_count,source_index,copied_flag;
    float local_clock,duration;
    bool back_loop,forward_loop;
    std::uint32_t out_knots_byte_offset,out_controls_byte_offset;
    std::array<float,4> knots;
    std::vector<float> controls;
};
inline std::vector<float> sample_bnw_warrior_source_idle_window(const BnwWarriorIdleSourceWindow& window,bool normalize) {
    const auto d=window.dimension;
    warrior_curve_require((window.format==18&&d==3)||((window.format==8||window.format==9)&&d==4),"Warrior source window codec/dimension");
    warrior_curve_require(window.knot_count>=3&&window.knot_count<=1024&&window.source_index<window.knot_count&&window.copied_flag<=1,"Warrior source window index/copied flag");
    warrior_curve_require(std::bit_cast<std::uint32_t>(window.duration)==0x40c00001u&&std::isfinite(window.local_clock)&&window.local_clock>=0&&window.local_clock<=window.duration,"Warrior source window pinned IdleA time");
    warrior_curve_require(window.out_knots_byte_offset==8&&window.out_controls_byte_offset==d*8&&window.controls.size()==d*4,"Warrior source window centers/extent");
    warrior_curve_require(std::ranges::all_of(window.knots,[](float x){return std::isfinite(x);})&&std::ranges::all_of(window.controls,[](float x){return std::isfinite(x);})&&std::is_sorted(window.knots.begin(),window.knots.end()),"Warrior source window values/order");
    auto controls=window.controls;
    // Native caller99A3F0 gates98F360 on window AL !=0, normalization and dim4.
    // Exact98F360 SSE dot order is w+z+y+x, unlike a generic x+y+z+w reduction.
    if(normalize&&d==4&&window.copied_flag) {
        std::array<float,4> previous{};
        for(unsigned row=0;row<4;++row) {
            float* value=controls.data()+row*4;
            const float w=value[3]*previous[3],z=value[2]*previous[2],y=value[1]*previous[1],x=value[0]*previous[0];
            const float wz=w+z,wzy=wz+y,dot=wzy+x;
            if(dot<0)for(unsigned j=0;j<4;++j)value[j]=-value[j];
            for(unsigned j=0;j<4;++j)previous[j]=value[j];
        }
    }
    const auto& k=window.knots;const float t=window.local_clock;
    const float den0=k[2]-k[0],den1=k[2]-k[1],den2=k[3]-k[1];
    warrior_curve_require(den0>0&&den1>0&&den2>0,"Warrior source window degree2 denominators");
    const float a=(t-k[0])/den0,b=(t-k[1])/den1;
    const float ab=a+b,product=a*b,combined=ab-product;
    const float next=(t-k[1])/den2,right=next*b,middle=combined-right,left=1.f-combined;
    std::vector<float> out(d);
    for(unsigned j=0;j<d;++j) {
        const float l=controls[j]*left,m=controls[d+j]*middle,r=controls[2*d+j]*right;
        const float pair=l+m;out[j]=pair+r;
    }
    if(normalize&&d==4) {
        const float x=out[0]*out[0],y=out[1]*out[1],z=out[2]*out[2],w=out[3]*out[3];
        const float xy=x+y,xyz=xy+z,norm=xyz+w;
        warrior_curve_require(norm>0&&std::isfinite(norm),"Warrior source window normalization");
        const float inverse=1.f/std::sqrt(norm);for(float& value:out)value*=inverse;
    }
    warrior_curve_require(std::ranges::all_of(out,[](float x){return std::isfinite(x);}),"Warrior source window result nonfinite");return out;
}
} // namespace civ5::graphics
