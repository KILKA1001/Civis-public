#include "civ5/runtime/research_selection_checkpoint.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <set>
#include <stdexcept>
#include <utility>

namespace civ5::runtime {
namespace {
constexpr std::array<std::uint8_t,8> magic{'C','I','V','R','S','L','0','1'};
constexpr auto section_id=static_cast<std::uint32_t>(
    saves::CivisSaveSectionId::live_research_selection);
constexpr auto bnw=content::TechnologyProfile::bnw;
void require(bool condition,const char *message) {
    if (!condition) throw std::runtime_error(message);
}
void digest(std::string_view hash) {
    require(hash.size()==64 && std::all_of(hash.begin(),hash.end(),[](char c) {
        return (c>='0'&&c<='9') || (c>='a'&&c<='f');
    }),"Invalid research-selection checkpoint SHA-256");
}

void validate_bindings(const ResearchSelectionState &selection,
    const PlayerResearchContext &players,const ResearchTurnState &turn,
    const ResearchContext &setup,const content::TechnologyRules &technologies,
    const SessionCheckpointIdentity &identity,
    const ResearchSelectionCheckpointBudget &budget) {
    digest(identity.payload_sha256);digest(identity.rules_sha256);
    require(selection.profile==bnw && players.profile==bnw &&
        turn.profile==bnw && setup.profile==bnw &&
        technologies.profile==bnw && technologies.all_succeeded(),
        "Research-selection checkpoint requires complete BNW rules");
    require(selection.players.size()==setup.players.size() &&
        players.players.size()==setup.players.size() &&
        turn.players.size()==setup.players.size() &&
        setup.players.size()<=budget.players,
        "Research-selection checkpoint roster mismatch");
    for (const auto &[id,record]:setup.players) {
        require(id>=0 && record.team>=0 && selection.players.contains(id) &&
            players.players.contains(id) && turn.players.contains(id),
            "Research-selection checkpoint player binding mismatch");
    }
}
void validate_tech(std::int64_t tech,const content::TechnologyRules &rules) {
    require(tech>=0 && rules.find(static_cast<std::uint64_t>(tech)),
            "Unresolved research-selection checkpoint technology");
}
void validate_player(const ResearchSelectionPlayer &selected,
    const PlayerResearchState &player,const ResearchTurnPlayer &turn,
    const ResearchSelectionState &selection,const content::TechnologyRules &rules) {
    require(selected.advanced_start_points>=-1 &&
        (!selected.civilization ||
         (!selected.civilization->empty() &&
          selection.disabled_technologies.contains(*selected.civilization))),
        "Unresolved research-selection checkpoint civilization");
    std::set<std::int64_t> queued;
    for (const auto tech:player.queue) {
        validate_tech(tech,rules);queued.insert(tech);
    }
    for (const auto tech:turn.researching) validate_tech(tech,rules);
    require(queued==turn.researching,
            "Research-selection queue and researching flags disagree");
}

struct Writer {
    std::vector<std::uint8_t> bytes;
    std::uint64_t limit;
    explicit Writer(std::uint64_t maximum):limit(maximum){}
    void raw(std::span<const std::uint8_t> value) {
        require(bytes.size()<=limit && value.size()<=limit-bytes.size(),
                "Research-selection checkpoint exceeds byte budget");
        bytes.insert(bytes.end(),value.begin(),value.end());
    }
    void u8(std::uint8_t value){raw(std::span(&value,1));}
    void u32(std::uint32_t value){for(unsigned i=0;i<4;++i)u8(static_cast<std::uint8_t>(value>>(8*i)));}
    void u64(std::uint64_t value){for(unsigned i=0;i<8;++i)u8(static_cast<std::uint8_t>(value>>(8*i)));}
    void i64(std::int64_t value){u64(std::bit_cast<std::uint64_t>(value));}
    void hash(std::string_view value) {
        digest(value);
        raw({reinterpret_cast<const std::uint8_t*>(value.data()),value.size()});
    }
    void string(std::string_view value,std::uint64_t maximum) {
        require(value.size()<=maximum,"Research-selection civilization exceeds string budget");
        u64(value.size());
        raw({reinterpret_cast<const std::uint8_t*>(value.data()),value.size()});
    }
};
struct Reader {
    std::span<const std::uint8_t> bytes;
    std::size_t at=0;
    explicit Reader(std::span<const std::uint8_t> source):bytes(source){}
    std::size_t remaining()const{return bytes.size()-at;}
    std::span<const std::uint8_t> raw(std::size_t n) {
        require(n<=remaining(),"Truncated research-selection checkpoint");
        const auto result=bytes.subspan(at,n);at+=n;return result;
    }
    std::uint8_t u8(){return raw(1)[0];}
    bool boolean(){const auto value=u8();require(value<=1,"Invalid research-selection boolean");return value!=0;}
    std::uint32_t u32(){std::uint32_t n=0;for(unsigned i=0;i<4;++i)n|=std::uint32_t(u8())<<(8*i);return n;}
    std::uint64_t u64(){std::uint64_t n=0;for(unsigned i=0;i<8;++i)n|=std::uint64_t(u8())<<(8*i);return n;}
    std::int64_t i64(){return std::bit_cast<std::int64_t>(u64());}
    void hash(std::string_view expected,const char *message) {
        digest(expected);const auto actual=raw(expected.size());
        require(std::equal(actual.begin(),actual.end(),expected.begin()),message);
    }
    std::string string(std::uint64_t maximum) {
        const auto size=u64();
        require(size<=maximum && size<=remaining(),
                "Oversized research-selection civilization key");
        const auto value=raw(static_cast<std::size_t>(size));
        if(value.empty()) return {};
        return {reinterpret_cast<const char*>(value.data()),value.size()};
    }
};
std::uint64_t count(Reader &reader,std::uint64_t &used,
                    std::uint64_t maximum,std::size_t minimum_width) {
    const auto n=reader.u64();
    require(used<=maximum && n<=maximum-used &&
            n<=reader.remaining()/minimum_width,
            "Oversized research-selection checkpoint collection");
    used+=n;return n;
}
struct StagedPlayer {
    ResearchSelectionPlayer selection;
    std::vector<std::int64_t> queue;
    std::set<std::int64_t> researching;
};
} // namespace

saves::CivisSaveSection make_research_selection_checkpoint(
    const ResearchSelectionState &selection,const PlayerResearchContext &players,
    const ResearchTurnState &turn,const ResearchContext &setup,
    const content::TechnologyRules &technologies,
    const SessionCheckpointIdentity &identity,
    ResearchSelectionCheckpointBudget budget) {
    validate_bindings(selection,players,turn,setup,technologies,identity,budget);
    Writer out(budget.bytes);
    out.raw(magic);out.u32(1);out.u32(1); // version, BNW profile
    out.hash(identity.payload_sha256);out.hash(identity.rules_sha256);
    out.u64(selection.players.size());
    std::uint64_t entries=0,strings=0;
    for (const auto &[id,selected]:selection.players) {
        const auto &player=players.players.at(id);
        const auto &flags=turn.players.at(id);
        validate_player(selected,player,flags,selection,technologies);
        out.i64(id);out.i64(setup.players.at(id).team);
        out.i64(selected.advanced_start_points);
        out.u8(selected.civilization.has_value());
        if (selected.civilization) {
            require(strings<=budget.string_bytes &&
                    selected.civilization->size()<=budget.string_bytes-strings,
                    "Research-selection string budget exceeded");
            strings+=selected.civilization->size();
            out.string(*selected.civilization,budget.string_bytes);
        }
        require(entries<=budget.entries &&
                player.queue.size()<=budget.entries-entries,
                "Research-selection queue budget exceeded");
        entries+=player.queue.size();out.u64(player.queue.size());
        for (const auto tech:player.queue) out.i64(tech);
        require(entries<=budget.entries &&
                flags.researching.size()<=budget.entries-entries,
                "Research-selection flag budget exceeded");
        entries+=flags.researching.size();out.u64(flags.researching.size());
        for (const auto tech:flags.researching) out.i64(tech);
    }
    return {section_id,0,std::move(out.bytes)};
}

void restore_research_selection_checkpoint(
    const saves::CivisSaveSection &section,ResearchSelectionState &selection,
    PlayerResearchContext &players,ResearchTurnState &turn,
    const ResearchContext &setup,const content::TechnologyRules &technologies,
    const SessionCheckpointIdentity &identity,bool require_queue_match,
    bool require_flags_match,ResearchSelectionCheckpointBudget budget) {
    validate_bindings(selection,players,turn,setup,technologies,identity,budget);
    require(section.id==section_id && section.flags==0 &&
            section.bytes.size()<=budget.bytes,
            "Unsupported research-selection checkpoint section");
    Reader in(section.bytes);
    const auto head=in.raw(magic.size());
    require(std::equal(head.begin(),head.end(),magic.begin()) &&
            in.u32()==1 && in.u32()==1,
            "Unsupported research-selection checkpoint version");
    in.hash(identity.payload_sha256,
            "Research-selection checkpoint source payload changed");
    in.hash(identity.rules_sha256,
            "Research-selection checkpoint active rules changed");
    std::uint64_t player_used=0,entries=0,strings=0;
    const auto n=count(in,player_used,budget.players,41);
    require(n==selection.players.size(),
            "Research-selection checkpoint player roster changed");
    std::map<std::int64_t,StagedPlayer> staged;
    std::int64_t previous=-1;
    for (std::uint64_t i=0;i<n;++i) {
        const auto id=in.i64(),team=in.i64();
        require(id>previous && setup.players.contains(id) &&
                setup.players.at(id).team==team,
                "Research-selection checkpoint player/team identity changed");
        previous=id;
        StagedPlayer row;
        row.selection.advanced_start_points=in.i64();
        if (in.boolean()) {
            require(strings<=budget.string_bytes,
                    "Research-selection string budget exceeded");
            row.selection.civilization=in.string(budget.string_bytes-strings);
            strings+=row.selection.civilization->size();
        }
        const auto queue_count=count(in,entries,budget.entries,8);
        for (std::uint64_t j=0;j<queue_count;++j) {
            const auto tech=in.i64();validate_tech(tech,technologies);
            row.queue.push_back(tech);
        }
        const auto flag_count=count(in,entries,budget.entries,8);
        for (std::uint64_t j=0;j<flag_count;++j) {
            const auto tech=in.i64();validate_tech(tech,technologies);
            require(row.researching.insert(tech).second,
                    "Duplicate research-selection flag");
        }
        PlayerResearchState queue_holder;queue_holder.queue=row.queue;
        ResearchTurnPlayer flags_holder;flags_holder.researching=row.researching;
        validate_player(row.selection,queue_holder,flags_holder,
                        selection,technologies);
        require(staged.emplace(id,std::move(row)).second,
                "Duplicate research-selection player");
    }
    require(in.remaining()==0,"Trailing research-selection checkpoint bytes");
    if (require_queue_match || require_flags_match) {
        for (const auto &[id,row]:staged) {
            if (require_queue_match)
                require(players.players.at(id).queue==row.queue,
                        "Research-selection and team-research queue sections disagree");
            if (require_flags_match)
                require(turn.players.at(id).researching==row.researching,
                        "Research-selection and research-turn flag sections disagree");
        }
    }
    auto next_selection=selection;
    auto next_players=players;
    auto next_turn=turn;
    for (const auto &[id,row]:staged) {
        next_selection.players.at(id)=row.selection;
        next_players.players.at(id).queue=row.queue;
        next_turn.players.at(id).researching=row.researching;
    }
    std::swap(selection,next_selection);
    std::swap(players,next_players);
    std::swap(turn,next_turn);
}

} // namespace civ5::runtime
