#include "civ5/terrain/scene_command_queue.hpp"
#include <stdexcept>
#include <utility>

namespace civ5::terrain {
namespace {
std::uint32_t word(std::span<const std::uint8_t> bytes,std::size_t offset){
    if(offset+4>bytes.size())throw std::invalid_argument("Truncated scene command");
    std::uint32_t value=0;
    for(unsigned i=0;i<4;++i)value|=std::uint32_t(bytes[offset+i])<<(i*8);
    return value;
}
}
void SceneCommandQueue::register_handler(std::uint32_t id,Handler handler){
    if(id>=handler_count || !handler)
        throw std::invalid_argument("Invalid scene command handler");
    handlers_[id]=std::move(handler);
}
void SceneCommandQueue::enqueue(std::span<const std::uint8_t> packet){
    if(packet.size()<16 || packet.size()>bank_capacity ||
       word(packet,0)!=packet.size() || word(packet,4)>=handler_count)
        throw std::invalid_argument("Invalid scene command packet");
    auto &bank=banks_[active_];
    if(packet.size()>bank_capacity-bank.size())
        throw std::length_error("Scene command bank full");
    bank.insert(bank.end(),packet.begin(),packet.end());
}
std::size_t SceneCommandQueue::drain(){
    // 6022D0 skips a nested call while its compare-exchange lock is held.
    if(draining_)return 0;
    draining_=true;
    try{
        active_^=1;
        banks_[active_].clear();
        const auto &previous=banks_[active_^1];
        std::size_t count=0;
        for(std::size_t offset=0;offset<previous.size();){
            const std::span bytes(previous);
            const auto size=word(bytes,offset);
            if(size<16 || size>previous.size()-offset)
                throw std::invalid_argument("Corrupt scene command bank");
            const auto id=word(bytes,offset+4);
            if(id>=handler_count || !handlers_[id])
                throw std::invalid_argument("Unregistered scene command");
            handlers_[id](bytes.subspan(offset+16,size-16));
            offset+=size;
            ++count;
        }
        draining_=false;
        return count;
    }catch(...){draining_=false;throw;}
}
} // namespace civ5::terrain
