#pragma once
#include "civ5/graphics/bnw_warrior_material_frame.hpp"
#include <algorithm>
#include <numeric>
namespace civ5::graphics {
// Actual owned twelve draw packets, not an outstanding callback receipt.
// The scoped Warrior UnitShader_Skinned family excludes native58/59 (4CD260).
class BnwWarriorRetainedMaterialPackets {
    BnwWarriorMaterialFrameOwner frame_;
    std::int32_t unit_;
    std::uint64_t revision_;
    std::array<std::uint16_t,12> family_{};
    std::array<unsigned,12> order_{};
    std::size_t visible_count_{12},flushes_{};
    void apply(const BnwWarriorAlphaEffects& effects) {
        for(auto i:effects.packet_updates) {
            if(i>=12)throw std::logic_error("Foreign Warrior packet effect");
            family_[i]=1; // native4CD260(member,1), verified skinned family.
        }
        if(effects.refresh_packet_time)for(unsigned i=0;i<12;++i)
            if(frame_.packet_transition_window(i,*effects.refresh_packet_time,unit_,revision_))family_[i]=2;
        if(effects.flush_visibility) {
            // 61D150 receiver is instance+354. Raw8EF7A0 comparator puts
            // visible mask1 packets first, then ascending alpha member index.
            std::sort(order_.begin(),order_.end(),[&](unsigned a,unsigned b){
                const bool av=(frame_.slot(a,unit_,revision_).visibility&1)!=0;
                const bool bv=(frame_.slot(b,unit_,revision_).visibility&1)!=0;
                return av==bv?a<b:av;
            });
            visible_count_=0;
            while(visible_count_<12&&(frame_.slot(order_[visible_count_],unit_,revision_).visibility&1)!=0)++visible_count_;
            ++flushes_;
        }
    }
public:
    BnwWarriorRetainedMaterialPackets(std::int32_t unit,std::uint64_t revision)
        :frame_(unit,revision,12),unit_(unit),revision_(revision) {
        family_.fill(1);std::iota(order_.begin(),order_.end(),0u);
    }
    void visibility(std::span<const std::uint8_t,12> states,float clock,std::int32_t unit,std::uint64_t revision) {
        // Scope is native primary visibility bit. Foreign pass/mask values
        // need their actual compiled packet masks, never guessed nonzero=true.
        for(auto state:states)if(state>1)throw std::invalid_argument("Unsupported Warrior packet visibility mask");
        apply(frame_.visibility_frame(states,clock,unit,revision));
    }
    void tick(float clock,std::int32_t unit,std::uint64_t revision){apply(frame_.tick(clock,unit,revision));}
    const BnwWarriorMaterialFrameOwner& frame()const{return frame_;}
    std::span<const unsigned> draw_order()const{return std::span(order_).first(visible_count_);}
    std::uint16_t family(unsigned i,std::int32_t unit,std::uint64_t revision)const {
        static_cast<void>(frame_.slot(i,unit,revision));return family_.at(i);
    }
    std::size_t flushes()const{return flushes_;}
};
} // namespace civ5::graphics
