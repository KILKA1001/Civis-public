#include "civ5/runtime/research_checkpoint.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <set>
#include <stdexcept>
#include <utility>

namespace civ5::runtime {
namespace {
constexpr std::array<std::uint8_t,8> common_magic{'C','I','V','R','E','S','0','1'};
constexpr auto common_id=static_cast<std::uint32_t>(saves::CivisSaveSectionId::live_team_research);
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
    bool boolean(){const auto v=u8();require(v<=1,"Invalid research checkpoint boolean");return v!=0;}
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
void common_row(Writer &writer,std::uint64_t slot,const TeamTechnologyValue &value,
    std::uint64_t slots){
    require(slot<=static_cast<std::uint64_t>(INT64_MAX)&&slot<slots&&
        value!=TeamTechnologyValue{},"Invalid common research checkpoint row");
    writer.u64(slot);writer.u8(value.has_tech);writer.u8(value.no_trade);
    writer.i64(value.progress_times100);writer.i64(value.count);
}
void validate_world(std::span<const TeamTechnologies> teams,const PlayerResearchContext &players,
    const ResearchContext &setup,const content::TechnologyRules &rules,
    const ResearchCheckpointIdentity &identity,const ResearchCheckpointBudget &budget){
    digest(identity.payload_sha256);digest(identity.rules_sha256);
    require(rules.all_succeeded()&&rules.profile==setup.profile&&players.profile==setup.profile,
        "Research checkpoint rules or profile mismatch");
    require(teams.size()==setup.team_members.size()&&teams.size()<=budget.teams&&
        players.players.size()==setup.players.size()&&players.players.size()<=budget.players,
        "Research checkpoint roster mismatch");
    std::set<std::int64_t> ids;
    for(const auto &team:teams)
        require(team.profile()==setup.profile&&team.slots()==rules.collection.slots&&
            team.remap().all_mapped()&&setup.team_members.contains(team.team())&&
            ids.insert(team.team()).second,"Research checkpoint team binding mismatch");
    for(const auto &[id,player]:players.players)
        require(setup.players.contains(id),"Research checkpoint player binding mismatch");
}
struct StagedTeam {
    std::int64_t last=-1;
    std::map<std::uint64_t,TeamTechnologyValue> values;
};
struct StagedPlayer {
    std::int64_t overflow=0,max_cities=1;
    std::vector<std::int64_t> queue;
};
} // namespace

ResearchCheckpointSections make_research_checkpoint(
    std::span<const TeamTechnologies> teams,const PlayerResearchContext &players,
    const ResearchContext &setup,const content::TechnologyRules &rules,
    const ResearchCheckpointIdentity &identity,ResearchCheckpointBudget budget){
    validate_world(teams,players,setup,rules,identity,budget);
    Writer out(budget.bytes);
    const auto codec=research_checkpoint_profile_codec(setup.profile);
    require((codec.make==nullptr)==(codec.restore==nullptr),
        "Incomplete research checkpoint profile codec");
    out.raw(common_magic);out.u32(1);out.u32(codec.wire_id);
    out.hash(identity.payload_sha256);out.hash(identity.rules_sha256);
    out.u64(teams.size());std::uint64_t rows=0;
    for(const auto &team:teams){
        require(team.last_tech()==-1||(team.last_tech()>=0&&
            static_cast<std::uint64_t>(team.last_tech())<team.slots()),
            "Invalid live last technology");
        const auto &values=team.nondefault_values();
        require(rows<=budget.rows&&values.size()<=budget.rows-rows,
            "Research checkpoint row budget exceeded");
        rows+=values.size();
        out.i64(team.team());out.u64(team.slots());out.i64(team.last_tech());out.u64(values.size());
        for(const auto &[slot,value]:values)common_row(out,slot,value,team.slots());
    }
    out.u64(players.players.size());
    for(const auto &[id,player]:players.players){
        require(player.queue.size()<=budget.rows-rows,"Research checkpoint queue budget exceeded");
        rows+=player.queue.size();
        out.i64(id);out.i64(setup.players.at(id).team);
        out.i64(player.overflow_times100);out.i64(player.max_effective_cities);
        out.u64(player.queue.size());
        for(const auto tech:player.queue){
            require(tech>=0&&rules.find(static_cast<std::uint64_t>(tech)),
                "Unresolved queued technology in research checkpoint");
            out.i64(tech);
        }
    }
    ResearchCheckpointSections result{{common_id,0,std::move(out.bytes)},std::nullopt};
    if(codec.make)
        result.profile=codec.make(teams,players,result.common,budget);
    return result;
}

void restore_research_checkpoint(
    const saves::CivisSaveSection &common,
    const std::optional<saves::CivisSaveSection> &profile,
    std::vector<TeamTechnologies> &teams,PlayerResearchContext &players,
    const ResearchContext &setup,const content::TechnologyRules &rules,
    const ResearchCheckpointIdentity &identity,ResearchCheckpointBudget budget){
    validate_world(teams,players,setup,rules,identity,budget);
    const auto codec=research_checkpoint_profile_codec(setup.profile);
    require((codec.make==nullptr)==(codec.restore==nullptr),
        "Incomplete research checkpoint profile codec");
    require(common.id==common_id&&common.flags==0&&common.bytes.size()<=budget.bytes&&
        profile.has_value()==(codec.restore!=nullptr),
        "Unsupported research checkpoint section pairing");
    Reader in(common.bytes);
    const auto head=in.raw(common_magic.size());
    require(std::equal(head.begin(),head.end(),common_magic.begin())&&in.u32()==1&&
        in.u32()==codec.wire_id,"Unsupported research checkpoint version");
    in.match(identity.payload_sha256,"Research checkpoint source payload changed");
    in.match(identity.rules_sha256,"Research checkpoint active rules changed");
    std::uint64_t team_used=0,row_used=0,player_used=0;
    const auto team_count=count(in,team_used,budget.teams,32);
    require(team_count==teams.size(),"Research checkpoint team roster mismatch");
    std::map<std::int64_t,std::size_t> team_indices;
    for(std::size_t i=0;i<teams.size();++i)
        require(team_indices.emplace(teams[i].team(),i).second,
            "Duplicate live research team identity");
    std::map<std::int64_t,StagedTeam> staged_team;
    for(std::uint64_t i=0;i<team_count;++i){
        const auto id=in.i64();const auto slots=in.u64();const auto last=in.i64();
        const auto match=team_indices.find(id);
        require(match!=team_indices.end()&&slots==teams[match->second].slots()&&
            (last==-1||(last>=0&&static_cast<std::uint64_t>(last)<slots)),
            "Research checkpoint team identity mismatch");
        StagedTeam staged;staged.last=last;
        const auto n=count(in,row_used,budget.rows,26);
        for(std::uint64_t j=0;j<n;++j){
            const auto slot=in.u64();
            TeamTechnologyValue value{in.boolean(),in.boolean(),in.i64(),in.i64()};
            require(slot<=static_cast<std::uint64_t>(INT64_MAX)&&slot<slots&&
                value!=TeamTechnologyValue{}&&staged.values.emplace(slot,value).second,
                "Invalid or duplicate research technology row");
        }
        require(staged_team.emplace(id,std::move(staged)).second,
            "Duplicate research checkpoint team");
    }
    const auto player_count=count(in,player_used,budget.players,40);
    require(player_count==players.players.size(),"Research checkpoint player roster mismatch");
    std::map<std::int64_t,StagedPlayer> staged_player;
    for(std::uint64_t i=0;i<player_count;++i){
        const auto id=in.i64(),team=in.i64();
        require(players.players.contains(id)&&setup.players.at(id).team==team,
            "Research checkpoint player identity mismatch");
        StagedPlayer staged;staged.overflow=in.i64();staged.max_cities=in.i64();
        const auto n=count(in,row_used,budget.rows,8);
        for(std::uint64_t j=0;j<n;++j){
            const auto tech=in.i64();
            require(tech>=0&&rules.find(static_cast<std::uint64_t>(tech)),
                "Unresolved queued research checkpoint technology");
            staged.queue.push_back(tech);
        }
        require(staged_player.emplace(id,std::move(staged)).second,
            "Duplicate research checkpoint player");
    }
    require(in.remaining()==0,"Trailing research checkpoint bytes");
    auto next_teams=teams;
    auto next_players=players;
    for(auto &team:next_teams){
        auto &staged=staged_team.at(team.team());
        team.replace_live_values(staged.last,std::move(staged.values));
    }
    for(auto &[id,player]:next_players.players){
        auto &staged=staged_player.at(id);
        player.overflow_times100=staged.overflow;
        player.max_effective_cities=staged.max_cities;
        player.queue=std::move(staged.queue);
    }
    if(profile)
        codec.restore(*profile,common,next_teams,next_players,budget);
    teams.swap(next_teams);
    std::swap(players,next_players);
}
} // namespace civ5::runtime
