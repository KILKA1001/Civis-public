#include "civ5/runtime/vp_research_checkpoint.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/vp_player_research_profile.hpp"
#include "civ5/runtime/vp_team_technologies_profile.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <map>
#include <set>
#include <stdexcept>

namespace civ5::runtime {
namespace {
constexpr std::array<std::uint8_t,8> magic{'C','I','V','V','R','P','0','1'};
constexpr auto section_id=static_cast<std::uint32_t>(saves::CivisSaveSectionId::vp_research_profile);
void require(bool condition,const char *message){if(!condition)throw std::runtime_error(message);}
void digest(std::string_view value){
    require(value.size()==64&&std::all_of(value.begin(),value.end(),[](char c){
        return (c>='0'&&c<='9')||(c>='a'&&c<='f');}),"Invalid research checkpoint SHA-256");
}
struct Writer {
    std::vector<std::uint8_t> bytes;
    std::uint64_t limit;
    explicit Writer(std::uint64_t maximum):limit(maximum){}
    void raw(std::span<const std::uint8_t> value){
        require(bytes.size()<=limit&&value.size()<=limit-bytes.size(),
            "Research checkpoint exceeds byte budget");
        bytes.insert(bytes.end(),value.begin(),value.end());
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
    std::span<const std::uint8_t> bytes;
    std::size_t at=0;
    explicit Reader(std::span<const std::uint8_t> source):bytes(source){}
    std::size_t remaining()const{return bytes.size()-at;}
    std::span<const std::uint8_t> raw(std::size_t n){
        require(n<=remaining(),"Truncated research checkpoint");
        const auto result=bytes.subspan(at,n);at+=n;return result;
    }
    std::uint8_t u8(){return raw(1)[0];}
    std::uint32_t u32(){std::uint32_t result=0;for(unsigned i=0;i<4;++i)result|=std::uint32_t(u8())<<(8*i);return result;}
    std::uint64_t u64(){std::uint64_t result=0;for(unsigned i=0;i<8;++i)result|=std::uint64_t(u8())<<(8*i);return result;}
    std::int64_t i64(){return std::bit_cast<std::int64_t>(u64());}
    void match(std::string_view expected,const char *message){
        digest(expected);const auto actual=raw(expected.size());
        require(std::equal(actual.begin(),actual.end(),expected.begin()),message);
    }
};
std::uint64_t count(Reader &reader,std::uint64_t &used,std::uint64_t maximum,std::size_t width){
    const auto n=reader.u64();
    require(used<=maximum&&n<=maximum-used&&n<=reader.remaining()/width,
        "Oversized research checkpoint collection");
    used+=n;return n;
}
} // namespace

ResearchCheckpointProfileCodec research_checkpoint_profile_codec(content::TechnologyProfile profile){
    switch(profile){
    case content::TechnologyProfile::bnw:
        return {1,nullptr,nullptr};
    case content::TechnologyProfile::vp542:
        return {2,&make_vp_research_checkpoint,&restore_vp_research_checkpoint};
    }
    throw std::runtime_error("Unsupported research checkpoint profile");
}

saves::CivisSaveSection make_vp_research_checkpoint(
    std::span<const TeamTechnologies> teams,const PlayerResearchContext &players,
    const saves::CivisSaveSection &common,ResearchCheckpointBudget budget){
    Writer out(budget.bytes);
    out.raw(magic);out.u32(1);out.u32(2);
    out.hash(identify_bytes(common.bytes).sha256);
    out.u64(teams.size());std::uint64_t rows=0;
    for(const auto &team:teams){
        require(team.profile()==content::TechnologyProfile::vp542,
            "VP research checkpoint requires VP teams");
        const auto *state=team.profile_extensions().find<VpTeamTechnologiesProfileState>();
        require(state!=nullptr,"Missing VP team research profile");
        require(rows<=budget.rows&&state->eureka_counters.size()<=budget.rows-rows,
            "VP research checkpoint row budget exceeded");
        rows+=state->eureka_counters.size();
        out.i64(team.team());out.i64(state->known_count);out.u64(state->eureka_counters.size());
        for(const auto &[slot,value]:state->eureka_counters){
            require(slot<=static_cast<std::uint64_t>(INT64_MAX)&&slot<team.slots()&&value!=0,
                "Invalid VP Eureka checkpoint row");
            out.u64(slot);out.i64(value);
        }
    }
    out.u64(players.players.size());
    for(const auto &[id,player]:players.players){
        out.i64(id);out.i64(vp_player_research_profile(player).science_from_league);
    }
    return {section_id,0,std::move(out.bytes)};
}

void restore_vp_research_checkpoint(
    const saves::CivisSaveSection &section,const saves::CivisSaveSection &common,
    std::vector<TeamTechnologies> &teams,PlayerResearchContext &players,
    ResearchCheckpointBudget budget){
    require(section.id==section_id&&section.flags==0&&section.bytes.size()<=budget.bytes,
        "Unsupported VP research checkpoint section");
    Reader in(section.bytes);
    const auto head=in.raw(magic.size());
    require(std::equal(head.begin(),head.end(),magic.begin())&&in.u32()==1&&
        in.u32()==2,"Unsupported VP research checkpoint version");
    in.match(identify_bytes(common.bytes).sha256,"VP research profile is paired with different common state");
    std::map<std::int64_t,std::size_t> team_indices;
    for(std::size_t i=0;i<teams.size();++i)
        require(teams[i].profile()==content::TechnologyProfile::vp542&&
            team_indices.emplace(teams[i].team(),i).second,
            "Duplicate or non-VP research team identity");
    std::uint64_t team_used=0,row_used=0,player_used=0;
    const auto nteams=count(in,team_used,budget.teams,24);
    require(nteams==teams.size(),"VP research checkpoint team roster mismatch");
    std::set<std::int64_t> seen;
    for(std::uint64_t i=0;i<nteams;++i){
        const auto id=in.i64(),known=in.i64();
        const auto match=team_indices.find(id);
        require(match!=team_indices.end()&&seen.insert(id).second,
            "VP research checkpoint team identity mismatch");
        auto &team=teams[match->second];
        VpTeamTechnologiesProfileState staged;staged.known_count=known;
        const auto n=count(in,row_used,budget.rows,16);
        for(std::uint64_t j=0;j<n;++j){
            const auto slot=in.u64();const auto value=in.i64();
            require(slot<=static_cast<std::uint64_t>(INT64_MAX)&&slot<team.slots()&&value!=0&&
                staged.eureka_counters.emplace(slot,value).second,
                "Invalid or duplicate VP Eureka checkpoint row");
        }
        auto *state=team.profile_extensions().find<VpTeamTechnologiesProfileState>();
        require(state!=nullptr,"Missing VP research profile host");
        *state=std::move(staged);
    }
    const auto nplayers=count(in,player_used,budget.players,16);
    require(nplayers==players.players.size(),"VP research checkpoint player roster mismatch");
    seen.clear();
    for(std::uint64_t i=0;i<nplayers;++i){
        const auto id=in.i64(),science=in.i64();
        require(players.players.contains(id)&&seen.insert(id).second,
            "VP research checkpoint player identity mismatch");
        vp_player_research_profile(players.players.at(id)).science_from_league=science;
    }
    require(in.remaining()==0,"Trailing VP research checkpoint bytes");
}
} // namespace civ5::runtime
