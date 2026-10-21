#pragma once
#include <array>
#include <cstdint>
#include <span>
#include <stdexcept>
namespace civ5::graphics {
// Exact records produced by native8E3A90 ->8F6D20; not a selected material policy.
// Native constructor ignores its V argument and stores W in both V and W.
struct BnwWarriorSamplerRecord {
    std::uint32_t address_u,address_v,address_w,mag_filter,min_filter,mip_filter,max_anisotropy;
    bool srgb;
    friend bool operator==(const BnwWarriorSamplerRecord&,const BnwWarriorSamplerRecord&)=default;
};
inline constexpr std::array<BnwWarriorSamplerRecord,12> bnw_native_sampler_records{{
    {1,1,1,2,2,2,1,false},{1,1,1,2,2,0,1,false},
    {1,1,1,2,2,2,1,true},{3,3,3,2,2,2,1,true},
    {1,1,1,2,2,0,1,true},{3,3,3,2,2,2,1,false},
    {1,1,1,2,3,2,8,false},{1,1,1,2,3,2,8,true},
    {3,3,3,2,3,2,8,false},{1,1,1,1,1,1,1,false},
    {3,3,3,1,1,1,1,false},{1,1,1,1,1,1,1,true}
}};
inline BnwWarriorSamplerRecord resolve_bnw_native_sampler_record(std::uint16_t index) {
    // Native count=13 but no source writer for record12 has been established.
    if(index>=bnw_native_sampler_records.size())throw std::invalid_argument("Unknown native sampler registry record");
    return bnw_native_sampler_records[index];
}
// Native8F6D60 issue order/state IDs. The consumer translates each value to its
// actual API equivalent; this function neither guesses indices nor mutates GL.
inline std::array<std::array<std::uint32_t,2>,8> bnw_native_sampler_state_writes(const BnwWarriorSamplerRecord&r) {
    return {{{1,r.address_u},{2,r.address_v},{3,r.address_w},{5,r.mag_filter},
             {6,r.min_filter},{7,r.mip_filter},{10,r.max_anisotropy},{11,r.srgb?1u:0u}}};
}
inline std::array<BnwWarriorSamplerRecord,6> resolve_bnw_warrior_packet_sampler_records(std::span<const std::uint8_t> packet) {
    if(packet.size()!=100||packet[0]!=0x5a||packet[1]||packet[2]||packet[3]||packet[0x60]!=6)
        throw std::invalid_argument("Missing/foreign CODE model mesh packet");
    std::array<BnwWarriorSamplerRecord,6> out;
    for(unsigned i=0;i<6;++i){const auto p=0x38+2*i;
        out[i]=resolve_bnw_native_sampler_record(std::uint16_t(packet[p])|(std::uint16_t(packet[p+1])<<8));}
    return out;
}
} // namespace civ5::graphics
