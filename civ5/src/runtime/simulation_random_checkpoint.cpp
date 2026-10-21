#include "civ5/runtime/simulation_random_checkpoint.hpp"
#include <algorithm>
#include <array>
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
constexpr std::array<std::uint8_t,8> magic{'C','I','V','R','N','G','0','1'};
constexpr auto section_id=static_cast<std::uint32_t>(saves::CivisSaveSectionId::live_simulation_random);
constexpr std::size_t wire_size=8+4+64+64+2*(4+3*8);
void require(bool condition,const char *message){if(!condition)throw std::runtime_error(message);}
void digest(std::string_view hash){
    require(hash.size()==64&&std::all_of(hash.begin(),hash.end(),[](char c){
        return (c>='0'&&c<='9')||(c>='a'&&c<='f');}),"Invalid simulation RNG checkpoint SHA-256");
}
std::uint32_t wire_algorithm(RandomAlgorithm algorithm){
    switch(algorithm){
    case RandomAlgorithm::bnw:return 1;
    case RandomAlgorithm::vp_reference:return 2;
    }
    throw std::runtime_error("Unsupported simulation RNG algorithm");
}
struct Writer {
    std::vector<std::uint8_t> bytes;
    void raw(std::string_view value){bytes.insert(bytes.end(),value.begin(),value.end());}
    void u32(std::uint32_t value){for(unsigned i=0;i<4;++i)bytes.push_back(static_cast<std::uint8_t>(value>>(8*i)));}
    void u64(std::uint64_t value){for(unsigned i=0;i<8;++i)bytes.push_back(static_cast<std::uint8_t>(value>>(8*i)));}
    void stream(const SimulationRandom &value){
        u32(wire_algorithm(value.algorithm()));
        u64(value.state().seed);u64(value.state().calls);u64(value.state().resets);
    }
};
struct Reader {
    std::span<const std::uint8_t> bytes;
    std::size_t at=0;
    std::uint32_t u32(){
        std::uint32_t value=0;
        for(unsigned i=0;i<4;++i)value|=std::uint32_t(bytes[at++])<<(8*i);
        return value;
    }
    std::uint64_t u64(){
        std::uint64_t value=0;
        for(unsigned i=0;i<8;++i)value|=std::uint64_t(bytes[at++])<<(8*i);
        return value;
    }
    SimulationRandom stream(RandomAlgorithm expected){
        require(u32()==wire_algorithm(expected),"Simulation RNG checkpoint algorithm changed");
        return SimulationRandom(expected,{u64(),u64(),u64()});
    }
};
} // namespace

saves::CivisSaveSection make_simulation_random_checkpoint(
    const SessionRandomStreams &streams,const SessionCheckpointIdentity &identity){
    digest(identity.payload_sha256);digest(identity.rules_sha256);
    Writer out;out.bytes.reserve(wire_size);
    out.bytes.insert(out.bytes.end(),magic.begin(),magic.end());out.u32(1);
    out.raw(identity.payload_sha256);out.raw(identity.rules_sha256);
    out.stream(streams.map);out.stream(streams.gameplay);
    return {section_id,0,std::move(out.bytes)};
}
void restore_simulation_random_checkpoint(
    const saves::CivisSaveSection &section,SessionRandomStreams &streams,
    const SessionCheckpointIdentity &identity){
    digest(identity.payload_sha256);digest(identity.rules_sha256);
    require(section.id==section_id&&section.flags==0&&section.bytes.size()==wire_size,
        "Unsupported simulation RNG checkpoint section");
    Reader in{section.bytes};
    require(std::equal(magic.begin(),magic.end(),section.bytes.begin()),
        "Unsupported simulation RNG checkpoint magic");
    in.at=magic.size();
    require(in.u32()==1,"Unsupported simulation RNG checkpoint version");
    const auto check_hash=[&](std::string_view expected,const char *message){
        require(std::equal(expected.begin(),expected.end(),section.bytes.begin()+in.at),message);
        in.at+=expected.size();
    };
    check_hash(identity.payload_sha256,"Simulation RNG checkpoint source payload changed");
    check_hash(identity.rules_sha256,"Simulation RNG checkpoint active rules changed");
    SessionRandomStreams staged{in.stream(streams.map.algorithm()),in.stream(streams.gameplay.algorithm())};
    require(in.at==section.bytes.size(),"Trailing simulation RNG checkpoint bytes");
    streams=std::move(staged);
}
} // namespace civ5::runtime
