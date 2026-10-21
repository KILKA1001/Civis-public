#include "civ5/runtime/treasury_checkpoint.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <map>
#include <stdexcept>
#include <utility>

namespace civ5::runtime {
namespace {
constexpr std::array<std::uint8_t,8> magic{'C','I','V','T','R','E','0','1'};
constexpr auto section_id=static_cast<std::uint32_t>(saves::CivisSaveSectionId::live_treasury);
void require(bool condition,const char *message){if(!condition)throw std::runtime_error(message);}
void digest(std::string_view hash){
    require(hash.size()==64&&std::all_of(hash.begin(),hash.end(),[](char c){
        return (c>='0'&&c<='9')||(c>='a'&&c<='f');}),"Invalid treasury checkpoint SHA-256");
}
std::uint32_t profile_wire(content::TechnologyProfile profile){
    if(profile==content::TechnologyProfile::bnw)return 1;
    if(profile==content::TechnologyProfile::vp542)return 2;
    throw std::runtime_error("Unsupported treasury checkpoint profile");
}
void validate(const TreasuryContext &state,const ResearchContext &setup,
    const SessionCheckpointIdentity &identity,const TreasuryCheckpointBudget &budget){
    digest(identity.payload_sha256);digest(identity.rules_sha256);
    require(state.profile==setup.profile&&state.players.size()==setup.players.size()&&
        state.players.size()<=budget.players,"Treasury checkpoint roster or profile mismatch");
    for(const auto &[id,player]:state.players)
        require(setup.players.contains(id),"Treasury checkpoint player binding mismatch");
}
struct Writer {
    std::vector<std::uint8_t> data;
    std::uint64_t limit;
    explicit Writer(std::uint64_t n):limit(n){}
    void raw(std::span<const std::uint8_t> bytes){
        require(data.size()<=limit&&bytes.size()<=limit-data.size(),
            "Treasury checkpoint exceeds host byte budget");
        data.insert(data.end(),bytes.begin(),bytes.end());
    }
    void u8(std::uint8_t value){raw(std::span(&value,1));}
    void u32(std::uint32_t value){for(unsigned i=0;i<4;++i)u8(static_cast<std::uint8_t>(value>>(8*i)));}
    void u64(std::uint64_t value){for(unsigned i=0;i<8;++i)u8(static_cast<std::uint8_t>(value>>(8*i)));}
    void i64(std::int64_t value){u64(std::bit_cast<std::uint64_t>(value));}
    void hash(std::string_view value){
        digest(value);raw({reinterpret_cast<const std::uint8_t*>(value.data()),value.size()});
    }
    void source(const std::optional<std::pair<std::uint64_t,std::uint64_t>> &range){
        u8(range.has_value());
        if(range){require(range->first<=range->second,"Invalid treasury source offset");
            u64(range->first);u64(range->second);}
    }
};
struct Reader {
    std::span<const std::uint8_t> data;
    std::size_t at=0;
    explicit Reader(std::span<const std::uint8_t> bytes):data(bytes){}
    std::size_t remaining()const{return data.size()-at;}
    std::span<const std::uint8_t> raw(std::size_t n){
        require(n<=remaining(),"Truncated treasury checkpoint");
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
    void source(const std::optional<std::pair<std::uint64_t,std::uint64_t>> &expected){
        const auto has=u8();require(has<=1,"Invalid treasury source-offset flag");
        if(!has){require(!expected,"Treasury source offset changed");return;}
        const auto first=u64(),last=u64();
        require(expected&&first<=last&&*expected==std::pair{first,last},
            "Treasury source offset changed");
    }
};
std::uint64_t count(Reader &reader,std::uint64_t &used,std::uint64_t maximum,std::size_t width){
    const auto n=reader.u64();
    require(used<=maximum&&n<=maximum-used&&n<=reader.remaining()/width,
        "Oversized treasury checkpoint collection");
    used+=n;return n;
}
struct LiveValues {
    std::int64_t balance=0,lifetime=0;
    std::vector<std::int64_t> balance_history,income_history;
};
} // namespace

saves::CivisSaveSection make_treasury_checkpoint(
    const TreasuryContext &state,const ResearchContext &setup,
    const SessionCheckpointIdentity &identity,TreasuryCheckpointBudget budget){
    validate(state,setup,identity,budget);
    Writer out(budget.bytes);
    out.raw(magic);out.u32(1);out.u32(profile_wire(state.profile));
    out.hash(identity.payload_sha256);out.hash(identity.rules_sha256);
    out.u64(state.players.size());
    std::uint64_t entries=0;
    for(const auto &[id,player]:state.players){
        out.i64(id);out.source(player.source_core);out.source(player.source_administration);
        out.i64(player.balance_times100);out.i64(player.lifetime_gross_gold);
        const auto history=[&](const std::vector<std::int64_t> &values){
            require(entries<=budget.history_entries&&values.size()<=budget.history_entries-entries,
                "Treasury history exceeds host entry budget");
            entries+=values.size();out.u64(values.size());
            for(const auto value:values)out.i64(value);
        };
        history(player.balance_history_times100);history(player.income_history_times100);
    }
    return {section_id,0,std::move(out.data)};
}

void restore_treasury_checkpoint(
    const saves::CivisSaveSection &section,TreasuryContext &state,const ResearchContext &setup,
    const SessionCheckpointIdentity &identity,TreasuryCheckpointBudget budget){
    validate(state,setup,identity,budget);
    require(section.id==section_id&&section.flags==0&&section.bytes.size()<=budget.bytes,
        "Unsupported treasury checkpoint section");
    Reader in(section.bytes);
    const auto head=in.raw(magic.size());
    require(std::equal(head.begin(),head.end(),magic.begin())&&in.u32()==1&&
        in.u32()==profile_wire(state.profile),"Unsupported treasury checkpoint version");
    in.hash(identity.payload_sha256,"Treasury checkpoint source payload changed");
    in.hash(identity.rules_sha256,"Treasury checkpoint active rules changed");
    std::uint64_t players=0,entries=0;
    const auto n=count(in,players,budget.players,34);
    require(n==state.players.size(),"Treasury checkpoint player roster changed");
    std::map<std::int64_t,LiveValues> staged;
    for(std::uint64_t i=0;i<n;++i){
        const auto id=in.i64();
        const auto it=state.players.find(id);
        require(it!=state.players.end(),"Treasury checkpoint player identity changed");
        in.source(it->second.source_core);in.source(it->second.source_administration);
        LiveValues values;values.balance=in.i64();values.lifetime=in.i64();
        const auto history=[&](std::vector<std::int64_t> &out){
            const auto size=count(in,entries,budget.history_entries,8);
            out.reserve(static_cast<std::size_t>(size));
            for(std::uint64_t j=0;j<size;++j)out.push_back(in.i64());
        };
        history(values.balance_history);history(values.income_history);
        require(staged.emplace(id,std::move(values)).second,
            "Duplicate treasury checkpoint player");
    }
    require(in.remaining()==0,"Trailing treasury checkpoint bytes");
    auto next=state;
    for(auto &[id,player]:next.players){
        auto &values=staged.at(id);
        player.balance_times100=values.balance;
        player.lifetime_gross_gold=values.lifetime;
        player.balance_history_times100=std::move(values.balance_history);
        player.income_history_times100=std::move(values.income_history);
    }
    std::swap(state,next);
}
} // namespace civ5::runtime
