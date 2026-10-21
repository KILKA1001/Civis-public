#include "civ5/runtime/research_turn_checkpoint.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <map>
#include <set>
#include <stdexcept>
#include <utility>

namespace civ5::runtime {
namespace {
constexpr std::array<std::uint8_t,8> magic{'C','I','V','R','T','N','0','1'};
constexpr auto section_id=static_cast<std::uint32_t>(saves::CivisSaveSectionId::live_research_turn);
void require(bool condition,const char *message){if(!condition)throw std::runtime_error(message);}
void digest(std::string_view hash){
    require(hash.size()==64&&std::all_of(hash.begin(),hash.end(),[](char c){
        return (c>='0'&&c<='9')||(c>='a'&&c<='f');}),"Invalid research-turn checkpoint SHA-256");
}
std::uint32_t profile_wire(content::TechnologyProfile profile){
    if(profile==content::TechnologyProfile::bnw)return 1;
    if(profile==content::TechnologyProfile::vp542)return 2;
    throw std::runtime_error("Unsupported research-turn checkpoint profile");
}
void validate(const ResearchTurnState &state,const ResearchContext &setup,
    const content::TechnologyRules &technologies,const SessionCheckpointIdentity &identity,
    const ResearchTurnCheckpointBudget &budget){
    digest(identity.payload_sha256);digest(identity.rules_sha256);
    require(state.profile==setup.profile&&state.profile==technologies.profile&&
        technologies.all_succeeded()&&state.players.size()==setup.players.size()&&
        state.players.size()<=budget.players,"Research-turn checkpoint roster or rules mismatch");
    for(const auto &[id,player]:state.players)
        require(setup.players.contains(id),"Research-turn checkpoint player binding mismatch");
}
struct Writer {
    std::vector<std::uint8_t> data;
    std::uint64_t limit;
    explicit Writer(std::uint64_t n):limit(n){}
    void raw(std::span<const std::uint8_t> bytes){
        require(data.size()<=limit&&bytes.size()<=limit-data.size(),
            "Research-turn checkpoint exceeds host byte budget");
        data.insert(data.end(),bytes.begin(),bytes.end());
    }
    void u8(std::uint8_t value){raw(std::span(&value,1));}
    void u32(std::uint32_t value){for(unsigned i=0;i<4;++i)u8(static_cast<std::uint8_t>(value>>(8*i)));}
    void u64(std::uint64_t value){for(unsigned i=0;i<8;++i)u8(static_cast<std::uint8_t>(value>>(8*i)));}
    void i64(std::int64_t value){u64(std::bit_cast<std::uint64_t>(value));}
    void hash(std::string_view value){
        digest(value);raw({reinterpret_cast<const std::uint8_t*>(value.data()),value.size()});
    }
};
struct Reader {
    std::span<const std::uint8_t> data;
    std::size_t at=0;
    explicit Reader(std::span<const std::uint8_t> bytes):data(bytes){}
    std::size_t remaining()const{return data.size()-at;}
    std::span<const std::uint8_t> raw(std::size_t n){
        require(n<=remaining(),"Truncated research-turn checkpoint");
        const auto result=data.subspan(at,n);at+=n;return result;
    }
    std::uint8_t u8(){return raw(1)[0];}
    std::uint32_t u32(){std::uint32_t n=0;for(unsigned i=0;i<4;++i)n|=std::uint32_t(u8())<<(8*i);return n;}
    std::uint64_t u64(){std::uint64_t n=0;for(unsigned i=0;i<8;++i)n|=std::uint64_t(u8())<<(8*i);return n;}
    std::int64_t i64(){return std::bit_cast<std::int64_t>(u64());}
    void hash(std::string_view expected,const char *message){
        digest(expected);const auto got=raw(expected.size());
        require(std::equal(got.begin(),got.end(),expected.begin()),message);
    }
};
std::uint64_t count(Reader &reader,std::uint64_t &used,std::uint64_t maximum,std::size_t width){
    const auto n=reader.u64();
    require(used<=maximum&&n<=maximum-used&&n<=reader.remaining()/width,
        "Oversized research-turn checkpoint collection");
    used+=n;return n;
}
struct LiveValues {
    std::int64_t cities_founded=0;
    YieldValues agreements;
    std::set<std::int64_t> researching;
};
} // namespace

saves::CivisSaveSection make_research_turn_checkpoint(
    const ResearchTurnState &state,const ResearchContext &setup,
    const content::TechnologyRules &technologies,const SessionCheckpointIdentity &identity,
    ResearchTurnCheckpointBudget budget){
    validate(state,setup,technologies,identity,budget);
    Writer out(budget.bytes);
    out.raw(magic);out.u32(1);out.u32(profile_wire(state.profile));
    out.hash(identity.payload_sha256);out.hash(identity.rules_sha256);
    out.u64(state.players.size());
    std::uint64_t entries=0;
    for(const auto &[id,player]:state.players){
        out.i64(id);out.i64(player.cities_founded);
        require(entries<=budget.entries&&player.agreement_counters.size()<=budget.entries-entries,
            "Research agreement checkpoint entry budget exceeded");
        entries+=player.agreement_counters.size();out.u64(player.agreement_counters.size());
        for(const auto &[other,value]:player.agreement_counters){
            require(other>=0&&value!=0,"Invalid research agreement checkpoint row");
            out.i64(other);out.i64(value);
        }
        require(entries<=budget.entries&&player.researching.size()<=budget.entries-entries,
            "Researching flag checkpoint entry budget exceeded");
        entries+=player.researching.size();out.u64(player.researching.size());
        for(const auto tech:player.researching){
            require(tech>=0&&static_cast<std::uint64_t>(tech)<technologies.collection.slots,
                "Researching checkpoint technology is outside the active catalog");
            out.i64(tech);
        }
    }
    return {section_id,0,std::move(out.data)};
}

void restore_research_turn_checkpoint(
    const saves::CivisSaveSection &section,ResearchTurnState &state,const ResearchContext &setup,
    const content::TechnologyRules &technologies,const SessionCheckpointIdentity &identity,
    ResearchTurnCheckpointBudget budget){
    validate(state,setup,technologies,identity,budget);
    require(section.id==section_id&&section.flags==0&&section.bytes.size()<=budget.bytes,
        "Unsupported research-turn checkpoint section");
    Reader in(section.bytes);
    const auto head=in.raw(magic.size());
    require(std::equal(head.begin(),head.end(),magic.begin())&&in.u32()==1&&
        in.u32()==profile_wire(state.profile),"Unsupported research-turn checkpoint version");
    in.hash(identity.payload_sha256,"Research-turn checkpoint source payload changed");
    in.hash(identity.rules_sha256,"Research-turn checkpoint active rules changed");
    std::uint64_t players=0,entries=0;
    const auto n=count(in,players,budget.players,32);
    require(n==state.players.size(),"Research-turn checkpoint player roster changed");
    std::map<std::int64_t,LiveValues> staged;
    for(std::uint64_t i=0;i<n;++i){
        const auto id=in.i64();
        require(state.players.contains(id),"Research-turn checkpoint player identity changed");
        LiveValues values;values.cities_founded=in.i64();
        const auto agreements=count(in,entries,budget.entries,16);
        for(std::uint64_t j=0;j<agreements;++j){
            const auto other=in.i64(),value=in.i64();
            require(other>=0&&value!=0&&values.agreements.emplace(other,value).second,
                "Invalid or duplicate research agreement checkpoint row");
        }
        const auto flags=count(in,entries,budget.entries,8);
        for(std::uint64_t j=0;j<flags;++j){
            const auto tech=in.i64();
            require(tech>=0&&static_cast<std::uint64_t>(tech)<technologies.collection.slots&&
                values.researching.insert(tech).second,
                "Invalid or duplicate researching checkpoint technology");
        }
        require(staged.emplace(id,std::move(values)).second,
            "Duplicate research-turn checkpoint player");
    }
    require(in.remaining()==0,"Trailing research-turn checkpoint bytes");
    auto next=state;
    for(auto &[id,player]:next.players){
        auto &values=staged.at(id);
        player.cities_founded=values.cities_founded;
        player.agreement_counters=std::move(values.agreements);
        player.researching=std::move(values.researching);
    }
    std::swap(state,next);
}
} // namespace civ5::runtime
