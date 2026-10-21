#include "civ5/terrain/scene_polygon_plot_store.hpp"
#include <algorithm>
#include <stdexcept>

namespace civ5::terrain {
namespace {
std::uint32_t word(const ScenePolygonPlotBytes &record,std::size_t offset){
    std::uint32_t result=0;
    for(unsigned byte=0;byte<4;++byte)
        result|=std::uint32_t(record[offset+byte])<<(8*byte);
    return result;
}
void put(ScenePolygonPlotBytes &record,std::size_t offset,std::uint32_t value){
    for(unsigned byte=0;byte<4;++byte)
        record[offset+byte]=static_cast<std::uint8_t>(value>>(8*byte));
}
}
void populate_scene_polygon_material_rows(std::span<std::uint32_t> words,
    std::uint32_t farm_improvement,std::span<const ScenePolygonMaterialSource> sources){
    if(words.size()<20 || sources.size()>0xffffffffu)
        throw std::invalid_argument("Invalid scene polygon material table extent");
    // The counters at +4EC..+4FC also occupy the fourth nominal five-word
    // row. Retail 5415F0 can read them during backward fallback.
    std::fill_n(words.begin()+15,5,0u);
    if(farm_improvement==0xffffffffu)return;
    for(std::size_t index=0;index<sources.size();++index){
        const auto &source=sources[index];
        if(source.improvement!=farm_improvement ||
           source.resource!=0xffffffffu ||
           source.era<1 || source.era>4 || source.state!=1)
            continue;
        auto &count=words[15+source.era];
        if(count>=3)continue;
        words[count*5+source.era]=static_cast<std::uint32_t>(index);
        ++count;
    }
}
void populate_scene_special_improvement_material_rows(std::span<std::uint32_t> words,
    std::array<std::uint32_t,3> improvement_ids,
    std::span<const ScenePolygonMaterialSource> sources){
    if(words.size()<21 || sources.size()>0xffffffffu)
        throw std::invalid_argument("Invalid scene special improvement table extent");
    for(std::size_t group=0;group<improvement_ids.size();++group){
        const auto base=group*7;
        const auto improvement=improvement_ids[group];
        words[base]=improvement;
        if(improvement==0xffffffffu)continue;
        for(std::size_t index=0;index<sources.size();++index){
            const auto &source=sources[index];
            if(source.improvement!=improvement || source.resource!=0xffffffffu)
                continue;
            for(std::size_t slot=0;slot<3;++slot)
                if(words[base+1+slot]==source.state)
                    words[base+4+slot]=static_cast<std::uint32_t>(index);
        }
    }
}
std::uint32_t resolve_scene_polygon_material_id(
    std::span<const std::uint32_t> row_words,std::int32_t selector){
    if(selector<0)return 0xffffffffu;
    if(static_cast<std::size_t>(selector)>=row_words.size())
        throw std::invalid_argument("Scene polygon material row is too short");
    for(auto index=selector;;--index){
        if(row_words[static_cast<std::size_t>(index)]!=0xffffffffu)
            return row_words[static_cast<std::size_t>(index)];
        if(index==0)return 0xffffffffu;
    }
}
ScenePolygonPlotBytes make_scene_polygon_plot_record(
    const ScenePolygonPlotSourceEvent &event,std::uint32_t scene_identity,
    std::uint32_t variation_count,const ScenePolygonRandomDraw &random,
    const ScenePolygonMaterialResolve &resolve_material){
    if(event.selector>=8 || !resolve_material || (variation_count && !random))
        throw std::invalid_argument("Invalid scene polygon plot material source");
    ScenePolygonPlotBytes record{};
    // 477CE0 initializes these fields. The controlled source has zeroed
    // scratch storage; the remaining bytes are zero until other scene owners
    // populate them.
    record[4]=event.variant;
    record[6]=2;
    record[7]=event.selector;
    record[8]=event.style;
    auto flags=((event.first_flags&15u)<<4)|(event.second_flags&15u);
    if(event.identity==scene_identity)flags|=0x200u;
    std::uint32_t material_row=60;
    if(variation_count){
        const auto variation=random()%variation_count;
        material_row+=variation;
        flags|=(variation&31u)<<11;
    }
    put(record,12,flags);
    record[16]=static_cast<std::uint8_t>(event.x);
    record[17]=static_cast<std::uint8_t>(std::uint16_t(event.x)>>8);
    record[18]=static_cast<std::uint8_t>(event.y);
    record[19]=static_cast<std::uint8_t>(std::uint16_t(event.y)>>8);
    put(record,132,0xffffffffu);
    put(record,0,resolve_material(material_row,event.selector,event.variant));
    return record;
}
ScenePolygonPlotStore::ScenePolygonPlotStore(std::span<const std::uint8_t> initial){
    if(initial.size()%144 || initial.size()/144>0xffffu)
        throw std::invalid_argument("Invalid scene polygon plot records");
    slots_.resize(initial.size()/144);
    for(std::size_t index=0;index<slots_.size();++index)
        std::copy_n(initial.begin()+index*144,144,slots_[index].begin());
    active_.assign(slots_.size(),true);
    active_count_=slots_.size();
}
std::uint32_t ScenePolygonPlotStore::insert(const ScenePolygonPlotBytes &source){
    std::uint32_t index;
    if(free_head_!=absent){
        index=free_head_;
        free_head_=word(slots_[index],0);
        active_[index]=true;
    }else{
        if(slots_.size()>=0xffffu)
            throw std::length_error("Scene polygon plot index exceeds U16");
        index=static_cast<std::uint32_t>(slots_.size());
        slots_.emplace_back();active_.push_back(true);
    }
    auto &destination=slots_[index];
    const auto old_flags=word(destination,12),new_flags=word(source,12);
    // 4A1150 copies bytes 0..8 and 16..143. It preserves bytes 9..11 and
    // the destination's upper 16 flag bits, even when reusing a freed slot.
    std::copy_n(source.begin(),9,destination.begin());
    put(destination,12,(old_flags&0xffff0000u)|(new_flags&0xffffu));
    std::copy(source.begin()+16,source.end(),destination.begin()+16);
    put(destination,12,word(destination,12)&~0x400u); // 681700 clears freed bit
    ++active_count_;
    return index;
}
void ScenePolygonPlotStore::release(std::uint32_t index){
    if(index>=slots_.size() || !active_[index])
        throw std::invalid_argument("Scene polygon plot slot is not active");
    auto &record=slots_[index];
    put(record,12,word(record,12)|0x400u);
    if(free_head_>=index){
        put(record,0,free_head_);
        free_head_=index;
    }else{
        auto previous=free_head_;
        for(std::size_t visits=0;visits<slots_.size();++visits){
            const auto next=word(slots_[previous],0);
            if(next>=index){
                put(record,0,next);
                put(slots_[previous],0,index);
                break;
            }
            if(next>=slots_.size() || next<=previous)
                throw std::logic_error("Corrupt scene polygon plot free list");
            previous=next;
        }
    }
    active_[index]=false;
    --active_count_;
}
std::array<std::int16_t,2> ScenePolygonPlotStore::plot_coordinates(std::uint32_t index) const{
    if(index>=slots_.size() || !active_[index])
        throw std::invalid_argument("Scene polygon plot is not active");
    const auto &record=slots_[index];
    const auto coordinate=[&](std::size_t offset){
        return static_cast<std::int16_t>(std::uint16_t(record[offset])|
            std::uint16_t(std::uint16_t(record[offset+1])<<8));
    };
    return {coordinate(16),coordinate(18)};
}
} // namespace civ5::terrain
