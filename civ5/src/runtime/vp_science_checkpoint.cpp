#include "civ5/runtime/vp_science_checkpoint.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <map>
#include <stdexcept>
#include <utility>

namespace civ5::runtime {
namespace {
constexpr std::array<std::uint8_t,8> magic{'C','I','V','V','S','C','0','1'};
constexpr auto section_id=static_cast<std::uint32_t>(saves::CivisSaveSectionId::vp_science_profile);
void require(bool valid,const char *message){if(!valid)throw std::runtime_error(message);}
void check_hash(std::string_view hash){
    require(hash.size()==64&&std::all_of(hash.begin(),hash.end(),[](char c){
        return (c>='0'&&c<='9')||(c>='a'&&c<='f');}),"Invalid VP science checkpoint SHA-256");
}
void validate(const ScienceContext &state,const ResearchContext &setup,
    const SessionCheckpointIdentity &identity,const VpScienceCheckpointBudget &budget){
    check_hash(identity.payload_sha256);check_hash(identity.rules_sha256);
    require(state.profile==content::TechnologyProfile::vp542&&setup.profile==state.profile&&
        state.players.size()==setup.players.size()&&state.players.size()<=budget.players,
        "VP science checkpoint requires the same VP player roster");
    for(const auto &[id,player]:state.players){
        require(setup.players.contains(id),"VP science checkpoint player binding changed");
        (void)vp_science_profile(player);
    }
}
struct Writer {
    std::vector<std::uint8_t> data;
    std::uint64_t limit;
    explicit Writer(std::uint64_t bytes):limit(bytes){}
    void raw(std::span<const std::uint8_t> bytes){
        require(data.size()<=limit&&bytes.size()<=limit-data.size(),"VP science checkpoint exceeds host byte budget");
        data.insert(data.end(),bytes.begin(),bytes.end());
    }
    void u8(std::uint8_t n){raw({&n,1});}
    void u32(std::uint32_t n){for(unsigned i=0;i<4;++i)u8(static_cast<std::uint8_t>(n>>(i*8)));}
    void u64(std::uint64_t n){for(unsigned i=0;i<8;++i)u8(static_cast<std::uint8_t>(n>>(i*8)));}
    void i64(std::int64_t n){u64(std::bit_cast<std::uint64_t>(n));}
    void hash(std::string_view n){check_hash(n);raw({reinterpret_cast<const std::uint8_t*>(n.data()),n.size()});}
};
struct Reader {
    std::span<const std::uint8_t> data;
    std::size_t at=0;
    explicit Reader(std::span<const std::uint8_t> bytes):data(bytes){}
    std::size_t remaining()const{return data.size()-at;}
    std::span<const std::uint8_t> raw(std::size_t n){
        require(n<=remaining(),"Truncated VP science checkpoint");
        auto bytes=data.subspan(at,n);at+=n;return bytes;
    }
    std::uint8_t u8(){return raw(1)[0];}
    std::uint32_t u32(){std::uint32_t n=0;for(unsigned i=0;i<4;++i)n|=std::uint32_t(u8())<<(i*8);return n;}
    std::uint64_t u64(){std::uint64_t n=0;for(unsigned i=0;i<8;++i)n|=std::uint64_t(u8())<<(i*8);return n;}
    std::int64_t i64(){return std::bit_cast<std::int64_t>(u64());}
    void hash(std::string_view expected,const char *message){
        check_hash(expected);const auto bytes=raw(expected.size());
        require(std::equal(bytes.begin(),bytes.end(),expected.begin()),message);
    }
};
} // namespace

saves::CivisSaveSection make_vp_science_checkpoint(
    const ScienceContext &state,const ResearchContext &setup,const SessionCheckpointIdentity &identity,
    VpScienceCheckpointBudget budget){
    validate(state,setup,identity,budget);
    Writer out(budget.bytes);
    out.raw(magic);out.u32(1);out.hash(identity.payload_sha256);out.hash(identity.rules_sha256);
    out.u64(state.players.size());
    for(const auto &[id,player]:state.players){
        out.i64(id);out.i64(vp_science_profile(player).last_turn_times100);
    }
    return {section_id,0,std::move(out.data)};
}

void restore_vp_science_checkpoint(
    const saves::CivisSaveSection &section,ScienceContext &state,const ResearchContext &setup,
    const SessionCheckpointIdentity &identity,VpScienceCheckpointBudget budget){
    validate(state,setup,identity,budget);
    require(section.id==section_id&&section.flags==0&&section.bytes.size()<=budget.bytes,
        "Unsupported VP science checkpoint section");
    Reader in(section.bytes);
    const auto head=in.raw(magic.size());
    require(std::equal(head.begin(),head.end(),magic.begin())&&in.u32()==1,
        "Unsupported VP science checkpoint version");
    in.hash(identity.payload_sha256,"VP science checkpoint source payload changed");
    in.hash(identity.rules_sha256,"VP science checkpoint active rules changed");
    const auto count=in.u64();
    require(count==state.players.size()&&count<=budget.players&&count<=in.remaining()/16,
        "VP science checkpoint player roster changed");
    std::map<std::int64_t,std::int64_t> staged;
    for(std::uint64_t i=0;i<count;++i){
        const auto id=in.i64(),value=in.i64();
        require(state.players.contains(id)&&staged.emplace(id,value).second,
            "Invalid or duplicate VP science checkpoint player");
    }
    require(in.remaining()==0,"Trailing VP science checkpoint bytes");
    for(auto &[id,player]:state.players)
        vp_science_profile(player).last_turn_times100=staged.at(id);
}
} // namespace civ5::runtime
