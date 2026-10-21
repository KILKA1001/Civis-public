#pragma once
#include <cmath>
#include <bit>
#include <cstdint>
#include <stdexcept>
namespace civ5::graphics {
// Raw98E070 ->98D780 operands. No default speed/offset/loop policy, and no
// inferred imported animation start time. This is a stateless source kernel.
struct BnwWarriorPoseAnimTimeInput {
    float graph_clock,offset,speed,duration;
    std::int32_t loop_count,add_clamp_offset,clamped_loop;
};
struct BnwWarriorPoseAnimTimeResult {
    float signed_local_clock,curve_local_clock;
    bool curve_boundary_wrap;
};
// Selected IdleA only (duration0x40c00001); this is not a generic clip query.
// Requires round-to-nearest Float32 arithmetic, no FTZ/DAZ, and unfused build
// (-ffp-contract=off). Source FPREM loops on C2 until completion: this helper
// returns the completed remainder, never a single partial-FPREM result.
// FPREM receives stored Float32 operands; binary remainder fits Float64 before
// conversion back to the source Float32. Source98E070 adds duration for a
// negative remainder; signed -0 remains -0.
inline BnwWarriorPoseAnimTimeResult map_bnw_warrior_native_clip_time(
    const BnwWarriorPoseAnimTimeInput& input) {
    if(!std::isfinite(input.graph_clock)||!std::isfinite(input.offset)||!std::isfinite(input.speed)||!std::isfinite(input.duration)||input.duration<=0)
        throw std::invalid_argument("Missing/nonfinite/invalid Warrior authored clock operands");
    if(std::bit_cast<std::uint32_t>(input.duration)!=0x40c00001u)
        throw std::invalid_argument("Warrior attested IdleA duration identity differs");
    const float product=input.graph_clock*input.speed;
    const float value=product+input.offset;
    if(!std::isfinite(value))throw std::invalid_argument("Warrior source clock arithmetic overflow");
    float local;
    if(input.loop_count>0){
        const float count=float(input.loop_count);
        const float loops_duration=count*input.duration;
        const float scaled=loops_duration*input.speed;
        const float limit=scaled+(input.add_clamp_offset!=0?input.offset:0.f);
        if(!std::isfinite(limit))throw std::invalid_argument("Warrior source loop bound overflow");
        if(value<=0.f)local=0.f;
        else if(value>=limit)local=input.duration;
        else local=float(std::fmod(double(value),double(input.duration)));
    }else local=float(std::fmod(double(value),double(input.duration)));
    const float curve=local<0.f?input.duration+local:local;
    return {local,curve,input.clamped_loop==0};
}
} // namespace civ5::graphics
