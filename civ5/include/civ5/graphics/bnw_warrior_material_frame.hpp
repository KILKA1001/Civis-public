#pragma once
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <optional>
#include <span>
#include <vector>

namespace civ5::graphics {
// Fresh UnitVisSystem718900, not imported transient original renderer memory.
inline constexpr float bnw_warrior_initial_water_height = std::bit_cast<float>(0x4121de22u);
struct BnwWarriorAlphaSlot {
    std::array<float,3> alpha{1.f,0.f,0.f};
    float start{}, duration{};
    std::uint16_t kind{};
    std::uint8_t pending{}, current{1}, visibility{1};
};
struct BnwWarriorAlphaEffects {
    // Native4CD260(member,1), 8EF850 visibility and61D150 flush have owners
    // outside this scalar state. Never treat these returned obligations as done.
    std::vector<unsigned> packet_updates;
    std::vector<unsigned> visibility_updates;
    bool flush_visibility{};
    std::optional<float> refresh_packet_time;
};
class BnwWarriorMaterialFrameOwner {
    std::int32_t unit_;
    std::uint64_t revision_;
    unsigned count_;
    std::uint32_t flags_{2};
    std::array<BnwWarriorAlphaSlot,32> slots_{};
    static void finite(float value) {
        if(!std::isfinite(value)) throw std::invalid_argument("Nonfinite Warrior material time/value");
    }
    void identity(std::int32_t unit,std::uint64_t revision)const {
        if(unit!=unit_||revision!=revision_)throw std::invalid_argument("Foreign/stale Warrior material owner");
    }
    void member(unsigned index)const {
        if(index>=count_)throw std::invalid_argument("Warrior material member outside owned roster");
    }
    void complete(unsigned i,std::uint16_t incoming,bool scheduling,BnwWarriorAlphaEffects& out) {
        auto& s=slots_[i];
        if(s.kind==1) {
            s.current=s.pending; s.visibility=s.pending;
            out.visibility_updates.push_back(i); out.flush_visibility=true;
        } else if(s.kind==2&&(!scheduling||incoming!=2)) {
            flags_=(flags_&0xfffffff0u)|s.pending;
        }
        s.kind=0;
        if(s.alpha[1]>0.f)out.packet_updates.push_back(i);
    }
public:
    BnwWarriorMaterialFrameOwner(std::int32_t unit,std::uint64_t revision,unsigned member_count)
        :unit_(unit),revision_(revision),count_(member_count) {
        if(unit<0||revision==0||member_count==0||member_count>32)
            throw std::invalid_argument("Missing Warrior material owner identity/roster");
    }
    float water_height(std::int32_t unit,std::uint64_t revision)const {
        identity(unit,revision);return bnw_warrior_initial_water_height;
    }
    const BnwWarriorAlphaSlot& slot(unsigned i,std::int32_t unit,std::uint64_t revision)const {
        identity(unit,revision);member(i);return slots_[i];
    }
    std::uint32_t flags()const{return flags_;}
    // Native67CD00 +48 literal. Used only for fresh visual owner events.
    static constexpr float initial_fade_duration(){return .5f;}
    BnwWarriorAlphaEffects schedule(unsigned i,float x,float y,float now,float duration,
        std::uint16_t kind,std::uint8_t pending,std::int32_t unit,std::uint64_t revision) {
        identity(unit,revision);member(i);finite(x);finite(y);finite(now);finite(duration);
        if(kind>2)throw std::invalid_argument("Unknown Warrior alpha transition kind");
        auto& s=slots_[i]; BnwWarriorAlphaEffects out;
        // Match native SSE addss rounding before inclusive comparison.
        const float end=s.start+s.duration;
        if((s.start<=now&&now<=end)||s.kind!=0)complete(i,kind,true,out);
        s.alpha={x,y,0.f};s.start=now;s.duration=duration;s.kind=kind;s.pending=pending;
        return out;
    }
    BnwWarriorAlphaEffects tick(float now,std::int32_t unit,std::uint64_t revision) {
        identity(unit,revision);finite(now);BnwWarriorAlphaEffects out;
        constexpr float neg_eps=std::bit_cast<float>(0xb727c5acu);
        constexpr float pos_eps=std::bit_cast<float>(0x3727c5acu);
        for(unsigned i=0;i<count_;++i) {
            auto& s=slots_[i];
            if(s.duration>neg_eps&&s.duration<pos_eps) {s.alpha[2]=0.f;continue;}
            // Separate float subtraction and division; no FMA/double promotion.
            const float elapsed=now-s.start;
            float z=elapsed/s.duration;
            if(z<0.f)z=0.f;else if(z>1.f)z=1.f;
            s.alpha[2]=z;
            if(z>=1.f)complete(i,0,false,out);
        }
        return out;
    }
    // Scalar subset of6AB1C0; caller must handle member animation mask actions,
    // 4EEDE0 packet route refresh and61D150 flush using the same retained owner.
    BnwWarriorAlphaEffects visibility(unsigned i,std::uint8_t state,float now,
        std::int32_t unit,std::uint64_t revision) {
        identity(unit,revision);member(i);finite(now);auto& s=slots_[i];
        BnwWarriorAlphaEffects out;
        if(state==s.current)return out;
        if(state==0)out=schedule(i,1.f,0.f,now,initial_fade_duration(),1,0,unit,revision);
        else {
            if(s.current==0)out=schedule(i,0.f,1.f,now,initial_fade_duration(),1,state,unit,revision);
            s.visibility=state;out.visibility_updates.push_back(i);
        }
        s.current=state;out.flush_visibility=true;return out;
    }
    // Complete scalar event6AB1C0 with no animation-mask bits. This does not
    // accept optional animation actions, whose native4052D0 owner is separate.
    BnwWarriorAlphaEffects visibility_frame(std::span<const std::uint8_t> states,float now,
        std::int32_t unit,std::uint64_t revision) {
        identity(unit,revision);finite(now);
        if(states.size()!=count_)throw std::invalid_argument("Incomplete Warrior visibility event roster");
        BnwWarriorAlphaEffects out;
        for(unsigned i=0;i<count_;++i) {
            auto part=visibility(i,states[i],now,unit,revision);
            out.packet_updates.insert(out.packet_updates.end(),part.packet_updates.begin(),part.packet_updates.end());
            out.visibility_updates.insert(out.visibility_updates.end(),part.visibility_updates.begin(),part.visibility_updates.end());
        }
        out.flush_visibility=true;
        out.refresh_packet_time=now+std::bit_cast<float>(0x3ba3d70au);
        return out;
    }
    bool packet_transition_window(unsigned i,float time,std::int32_t unit,std::uint64_t revision)const {
        identity(unit,revision);member(i);finite(time);const auto& s=slots_[i];
        const float end=s.start+s.duration;return s.start<=time&&time<=end;
    }
};
} // namespace civ5::graphics
