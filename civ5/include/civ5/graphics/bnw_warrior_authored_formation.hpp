#pragma once
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <stdexcept>

namespace civ5::graphics {
struct BnwWarriorFormationPoint { float x=0,y=0; };
struct BnwWarriorFormation {
    std::array<BnwWarriorFormationPoint,12> members{};
    std::uint32_t final_graphics_rng_state=0;
    std::uint32_t random_draws=0;
};
// Pinned 522190 Warrior branch, not a native placement receipt. The seed must
// be the renderer pool index (data+304), not a gameplay unit ID. Scene spacing
// is registry+30, which graphics configuration can change from its file value 1.
// No GameCore/RNG reference, no file/GL access, and no readiness flag.
inline BnwWarriorFormation bnw_warrior_authored_formation(
    std::uint32_t graphics_pool_seed, float scene_spacing_multiplier) {
    if(graphics_pool_seed==0xffffffffU || !std::isfinite(scene_spacing_multiplier) ||
       scene_spacing_multiplier<=0 || scene_spacing_multiplier>100)
        throw std::invalid_argument("Warrior formation requires explicit pool seed and positive scene spacing");
    constexpr std::uint32_t modulus=0xffffcfefU;
    constexpr double normalization=std::bit_cast<double>(std::uint64_t{0x3df0000301109066ULL});
    constexpr float authored_randomness=0.4F*0.4F;
    constexpr float native_half_pi=1.5707950592041016F;
    const float spacing=13.0F*scene_spacing_multiplier;
    const float center=spacing*0.5F;
    const float denominator=2.0F*spacing-center;
    auto state=graphics_pool_seed;
    auto random=[&]() {
        state=(state*std::uint32_t{0x343fd}+std::uint32_t{0x269ec3})%modulus;
        return static_cast<double>(state)*normalization;
    };
    BnwWarriorFormation result;
    float minx=0,miny=0,maxx=0,maxy=0; // Native bounds include the origin.
    for(std::size_t n=0;n<result.members.size();++n) {
        const auto column=n%4,row=n/4;
        const float base=(static_cast<float>((column+2)/2)*spacing-center)*
            (column%2==0?1.0F:-1.0F);
        const float curve=static_cast<float>((1.0-std::cos(static_cast<double>(base)/denominator*native_half_pi))*2.0);
        const float x=static_cast<float>(base+(random()-0.5)*2.0*authored_randomness*spacing);
        const float y=static_cast<float>((random()-0.5)*2.0*authored_randomness*spacing+
            static_cast<float>(row)*spacing+curve);
        result.members[n]={x,y};
        minx=std::fmin(minx,x);maxx=std::fmax(maxx,x);
        miny=std::fmin(miny,y);maxy=std::fmax(maxy,y);
    }
    const float cx=(minx+maxx)*0.5F,cy=(miny+maxy)*0.5F;
    for(auto &p:result.members) {p.x-=cx;p.y-=cy;}
    result.final_graphics_rng_state=state;result.random_draws=24;
    return result;
}
// 4FF6A0 maps saved facing direction to these original float32 constants.
inline float bnw_warrior_initial_heading(std::int32_t facing) {
    constexpr std::array<float,6> angles{2.617992877960205F,1.5707963705062866F,
        0.5235998630523682F,5.759585857391357F,4.71238899230957F,3.6651926040649414F};
    if(facing<0||facing>=6)throw std::invalid_argument("Warrior facing direction is not known");
    return angles[static_cast<std::size_t>(facing)];
}
struct BnwWarriorWorldTransformInput {
    float center_x=0,center_y=0,heading_radians=0;
    std::array<float,12> member_height{};
    // Explicit caller-authored render scale. XML fScale is 0.14, but final
    // native scaled draw consumption is not yet proven by this helper.
    float model_scale=0;
};
// Source row-vector Z rotation (4BF4D0 -> D3DXMatrixRotationZ), followed by
// caller-supplied terrain heights. Outputs are generated render transforms,
// never native matrices or BnwWarriorPlacementReceipt.
inline std::array<std::array<float,16>,12> bnw_warrior_render_transforms(
    const BnwWarriorFormation &formation,const BnwWarriorWorldTransformInput &input) {
    if(!std::isfinite(input.center_x)||!std::isfinite(input.center_y)||
       !std::isfinite(input.heading_radians)||!std::isfinite(input.model_scale)||input.model_scale<=0)
        throw std::invalid_argument("Warrior transform requires finite position/heading and explicit scale");
    const float c=std::cos(input.heading_radians),s=std::sin(input.heading_radians);
    std::array<std::array<float,16>,12> result{};
    for(std::size_t i=0;i<12;++i) {
        const auto p=formation.members[i];
        if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(input.member_height[i]))
            throw std::invalid_argument("Warrior transform contains invalid member coordinates");
        result[i]={input.model_scale*c,input.model_scale*s,0,0,
            -input.model_scale*s,input.model_scale*c,0,0,0,0,input.model_scale,0,
            p.x*c-p.y*s+input.center_x,p.x*s+p.y*c+input.center_y,input.member_height[i],1};
    }
    return result;
}
} // namespace civ5::graphics
