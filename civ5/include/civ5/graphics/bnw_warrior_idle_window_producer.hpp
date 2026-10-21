#pragma once
#include "civ5/graphics/bnw_warrior_idle_source_window.hpp"
namespace civ5::graphics {
// Mandatory9B2410 operands, not an inferred animation-controller policy.
// This selected IdleA route supports compressed neighbors of matching dimension.
// Null neighbor means source endpoint copy; non-null neighbor supplies its own
// authored controls. All duration operands are explicit and pinned independently.
struct BnwWarriorIdleWindowOperands {
    const BnwWarriorDecodedIdleCurve* backward_neighbor;
    const BnwWarriorDecodedIdleCurve* forward_neighbor;
    float backward_duration,forward_initial_duration,forward_cycle_duration;
};
inline void validate_bnw_warrior_window_curve(const BnwWarriorDecodedIdleCurve& c,unsigned dimension) {
    const auto n=c.knots.size();
    warrior_curve_require(c.degree==2&&c.dimension==dimension&&((c.format==18&&dimension==3)||((c.format==8||c.format==9)&&dimension==4)),"Warrior producer compressed codec/dimension");
    warrior_curve_require(n>=3&&n<=1024&&c.controls.size()==n*dimension&&c.quantized_knots.size()==n,"Warrior producer decoded extents");
    warrior_curve_require(std::isfinite(c.quantizer)&&c.quantizer>0&&std::ranges::all_of(c.knots,[](float x){return std::isfinite(x);})&&std::ranges::all_of(c.controls,[](float x){return std::isfinite(x);})&&std::is_sorted(c.knots.begin(),c.knots.end())&&std::is_sorted(c.quantized_knots.begin(),c.quantized_knots.end()),"Warrior producer finite/sorted source curve");
    const auto mask=c.format==8?65535u:255u;
    warrior_curve_require(std::ranges::all_of(c.quantized_knots,[&](auto x){return x<=mask;}),"Warrior producer source packed knot width");
}
inline BnwWarriorIdleSourceWindow produce_bnw_warrior_idle_source_window(const BnwWarriorDecodedIdleCurve& curve,float local_clock,const BnwWarriorIdleWindowOperands& operands) {
    const auto d=curve.dimension;validate_bnw_warrior_window_curve(curve,d);
    const auto pinned=[](float x){return std::bit_cast<std::uint32_t>(x)==0x40c00001u;};
    warrior_curve_require(pinned(operands.backward_duration)&&pinned(operands.forward_initial_duration)&&pinned(operands.forward_cycle_duration),"Warrior producer explicit pinned IdleA durations");
    warrior_curve_require(std::isfinite(local_clock)&&local_clock>=0&&local_clock<=operands.forward_initial_duration,"Warrior producer bounded local clock");
    if(operands.backward_neighbor)validate_bnw_warrior_window_curve(*operands.backward_neighbor,d);
    if(operands.forward_neighbor)validate_bnw_warrior_window_curve(*operands.forward_neighbor,d);
    const float scaled=local_clock*curve.quantizer;
    warrior_curve_require(std::isfinite(scaled)&&scaled>=0&&scaled<2147483648.f,"Warrior producer native floor-int overflow");
    const auto quantized=std::uint32_t(std::floor(scaled))&(curve.format==8?65535u:255u);
    const auto upper=std::upper_bound(curve.quantized_knots.begin(),curve.quantized_knots.end(),quantized);
    const int source_index=int(std::min<std::ptrdiff_t>(std::distance(curve.quantized_knots.begin(),upper),std::ptrdiff_t(curve.knots.size())-1));
    int effective_count=int(curve.knots.size());if(operands.forward_neighbor&&effective_count>0)--effective_count;
    int start=source_index-2;const bool before=start<0,after=effective_count<start+4;
    BnwWarriorIdleSourceWindow out{curve.format,d,std::uint32_t(curve.knots.size()),std::uint32_t(source_index),std::uint32_t(before||after),local_clock,operands.forward_initial_duration,operands.backward_neighbor!=nullptr,operands.forward_neighbor!=nullptr,8,d*8,{},std::vector<float>(d*4)};
    // Value-owned result isolates scratch rows. Every copied/decoded row is
    // bounds checked and assigned exactly once before the result is returned.
    std::array<bool,4> written{};
    auto row=[&](int dst,const BnwWarriorDecodedIdleCurve& c,int src,float knot) {
        warrior_curve_require(dst>=0&&dst<4&&src>=0&&src<int(c.knots.size())&&!written[std::size_t(dst)]&&std::isfinite(knot),"Warrior producer source row bounds/overwrite");
        out.knots[std::size_t(dst)]=knot;for(unsigned j=0;j<d;++j)out.controls[std::size_t(dst)*d+j]=c.controls[std::size_t(src)*d+j];written[std::size_t(dst)]=true;
    };
    auto copy=[&](int dst,int src) {
        warrior_curve_require(dst>=0&&dst<4&&src>=0&&src<4&&dst!=src&&written[std::size_t(src)]&&!written[std::size_t(dst)],"Warrior producer endpoint copy alias/bounds");
        out.knots[std::size_t(dst)]=out.knots[std::size_t(src)];for(unsigned j=0;j<d;++j)out.controls[std::size_t(dst)*d+j]=out.controls[std::size_t(src)*d+j];written[std::size_t(dst)]=true;
    };
    if(!before&&!after) {
        for(int w=0;w<4;++w)row(w,curve,start+w,curve.knots[std::size_t(start+w)]);
    } else {
        int prefix=0,count=4;
        if(before){prefix=-start;count=4+start;start=0;}
        if(after)count=effective_count-start;
        warrior_curve_require(prefix>=0&&prefix<=2&&count>0&&prefix+count<=4,"Warrior producer source slice bounds");
        for(int w=0;w<count;++w)row(prefix+w,curve,start+w,curve.knots[std::size_t(start+w)]);
        if(before) {
            if(!operands.backward_neighbor){for(int w=0;w<prefix;++w)copy(w,prefix);}
            else {
                const auto& back=*operands.backward_neighbor;const int cycle_count=int(back.knots.size())-1;
                int src=cycle_count-1;float shift=-operands.backward_duration;
                for(int w=prefix-1;w>=0;--w) {
                    const float knot=back.knots[std::size_t(src)]+shift;row(w,back,src,knot);
                    --src;if(src<0){shift=shift-operands.backward_duration;src=cycle_count-1;}
                }
            }
        }
        if(after) {
            const int last=prefix-1+count;
            if(!operands.forward_neighbor){for(int w=last+1;w<4;++w)copy(w,last);}
            else {
                const auto& forward=*operands.forward_neighbor;const int cycle_count=int(forward.knots.size())-1;
                int src=0;float shift=operands.forward_initial_duration;
                for(int w=last+1;w<4;++w) {
                    const float knot=forward.knots[std::size_t(src)]+shift;row(w,forward,src,knot);
                    ++src;if(src>=cycle_count){shift=shift+operands.forward_cycle_duration;src=0;}
                }
            }
        }
    }
    warrior_curve_require(std::ranges::all_of(written,[](bool x){return x;})&&std::is_sorted(out.knots.begin(),out.knots.end()),"Warrior producer incomplete/unordered window");return out;
}
inline std::vector<float> sample_bnw_warrior_produced_idle_curve(const BnwWarriorDecodedIdleCurve& curve,float local_clock,bool normalize,const BnwWarriorIdleWindowOperands& operands) {
    warrior_curve_require(std::bit_cast<std::uint32_t>(operands.backward_duration)==0x40c00001u&&std::bit_cast<std::uint32_t>(operands.forward_initial_duration)==0x40c00001u&&std::bit_cast<std::uint32_t>(operands.forward_cycle_duration)==0x40c00001u,"Warrior query mandatory IdleA duration operands");
    if(curve.degree==0) {
        warrior_curve_require(operands.backward_neighbor==nullptr&&operands.forward_neighbor==nullptr,"Warrior constant curve neighbors unsupported");
        return sample_bnw_warrior_idle_curve(curve,local_clock,operands.forward_initial_duration,normalize,false,false);
    }
    return sample_bnw_warrior_source_idle_window(produce_bnw_warrior_idle_source_window(curve,local_clock,operands),normalize);
}
// Approved source clock arithmetic entry point. Explicit neighbor presence must
// agree with source ClampedLoop; lower float entry supports independent policy
// fixtures, not a controller-derived timeline.
inline BnwWarriorIdleSourceWindow produce_bnw_warrior_idle_source_window(const BnwWarriorDecodedIdleCurve& curve,const BnwWarriorPoseAnimTimeInput& time,const BnwWarriorIdleWindowOperands& operands) {
    const auto clock=map_bnw_warrior_native_clip_time(time);
    warrior_curve_require((operands.backward_neighbor!=nullptr)==clock.curve_boundary_wrap&&(operands.forward_neighbor!=nullptr)==clock.curve_boundary_wrap,"Warrior approved source clock/explicit neighbor policy mismatch");
    return produce_bnw_warrior_idle_source_window(curve,clock.curve_local_clock,operands);
}
struct BnwWarriorIdleTrackWindowOperands {std::string_view name;std::array<BnwWarriorIdleWindowOperands,3> channels;};
// Uses the same accepted named tracks/skeleton. Clock is the approved source
// IdleA arithmetic; neighbor references and durations remain caller supplied.
inline std::array<BnwWarriorIdleLocalTransform,23> sample_bnw_warrior_produced_idle_local_pose(std::span<const BnwWarriorIdleTrack> tracks,std::span<const std::string_view> skeleton_names,const BnwWarriorPoseAnimTimeInput& time,std::span<const BnwWarriorIdleTrackWindowOperands> operands) {
    warrior_curve_require(tracks.size()==23&&skeleton_names.size()==23&&operands.size()==23,"Warrior produced pose roster cardinality");const auto clock=map_bnw_warrior_native_clip_time(time);
    std::array<BnwWarriorIdleLocalTransform,23> out{};
    for(std::size_t bone=0;bone<23;++bone) {
        const auto name=skeleton_names[bone];warrior_curve_require(!name.empty()&&std::count(skeleton_names.begin(),skeleton_names.end(),name)==1,"Warrior produced skeleton names");
        const auto track=std::find_if(tracks.begin(),tracks.end(),[&](const auto& t){return t.name==name;});const auto args=std::find_if(operands.begin(),operands.end(),[&](const auto& t){return t.name==name;});
        warrior_curve_require(track!=tracks.end()&&args!=operands.end()&&std::count_if(tracks.begin(),tracks.end(),[&](const auto& t){return t.name==name;})==1&&std::count_if(operands.begin(),operands.end(),[&](const auto& t){return t.name==name;})==1,"Warrior produced track/operand binding");
        std::size_t at=0;
        for(unsigned channel=0;channel<3;++channel) {
            const auto& c=track->curves[channel];const auto& a=args->channels[channel];warrior_curve_require(c.dimension==std::array<unsigned,3>{3,4,9}[channel],"Warrior produced pose channel dimensions");
            if(c.degree!=0)warrior_curve_require((a.backward_neighbor!=nullptr)==clock.curve_boundary_wrap&&(a.forward_neighbor!=nullptr)==clock.curve_boundary_wrap,"Warrior produced pose explicit neighbors disagree with approved clock policy");
            const auto values=sample_bnw_warrior_produced_idle_curve(c,clock.curve_local_clock,channel==1,a);if(c.format!=2)out[bone].flags|=1u<<channel;for(float value:values)out[bone].position_orientation_scale[at++]=value;
        }
    }
    return out;
}
} // namespace civ5::graphics
