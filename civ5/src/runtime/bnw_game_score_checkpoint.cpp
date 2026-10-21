#include "civ5/runtime/bnw_game_score_checkpoint.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <climits>
#include <limits>
#include <span>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
constexpr std::array<std::uint8_t,8> magic{'C','I','V','B','S','C','0','1'};
constexpr std::size_t slots=64,score_arrays=6;
constexpr std::size_t max_section_bytes=64*1024*1024;
constexpr std::size_t max_name_bytes=16*1024*1024;
constexpr std::size_t max_data_sets=64*1024;
constexpr std::size_t max_turn_values=2*1024*1024;
constexpr std::array<std::string_view,score_arrays> array_names{
    "rank_player","player_rank","player_score",
    "rank_team","team_rank","team_score"};
void require(bool ok,const char *why) {
    if(!ok) throw std::runtime_error(why);
}
std::string_view name(const char *value) {
    return value?std::string_view(value):std::string_view();
}
void digest(std::string_view value) {
    require(value.size()==64 &&
            std::all_of(value.begin(),value.end(),[](char c) {
                return (c>='0'&&c<='9')||(c>='a'&&c<='f');
            }),"Invalid BNW score checkpoint identity");
}
std::int32_t i32(std::int64_t value) {
    require(value>=INT32_MIN && value<=INT32_MAX,
            "BNW score checkpoint value exceeds source i32");
    return static_cast<std::int32_t>(value);
}
std::uint32_t u32(std::int64_t value) {
    require(value>=0 && value<=UINT32_MAX,
            "BNW replay turn exceeds source u32");
    return static_cast<std::uint32_t>(value);
}
struct Writer {
    std::vector<std::uint8_t> bytes;
    void raw(std::span<const std::uint8_t> value) {
        require(value.size()<=max_section_bytes &&
                bytes.size()<=max_section_bytes-value.size(),
                "BNW score checkpoint byte budget exceeded");
        bytes.insert(bytes.end(),value.begin(),value.end());
    }
    void raw(std::string_view value) {
        raw({reinterpret_cast<const std::uint8_t*>(value.data()),value.size()});
    }
    void u32(std::uint32_t value) {
        require(bytes.size()<=max_section_bytes-4,
                "BNW score checkpoint byte budget exceeded");
        for(unsigned i=0;i<4;++i)
            bytes.push_back(static_cast<std::uint8_t>(value>>(8*i)));
    }
    void i32(std::int32_t value) {
        u32(std::bit_cast<std::uint32_t>(value));
    }
};
struct Reader {
    std::span<const std::uint8_t> bytes;
    std::size_t at=0;
    std::span<const std::uint8_t> raw(std::size_t count) {
        require(at<=bytes.size() && count<=bytes.size()-at,
                "Truncated BNW score checkpoint");
        auto result=bytes.subspan(at,count);at+=count;return result;
    }
    std::uint32_t u32() {
        auto b=raw(4);std::uint32_t result=0;
        for(unsigned i=0;i<4;++i) result|=std::uint32_t(b[i])<<(8*i);
        return result;
    }
    std::int32_t i32() {return std::bit_cast<std::int32_t>(u32());}
    void hash(std::string_view expected,const char *why) {
        auto value=raw(expected.size());
        require(std::equal(value.begin(),value.end(),expected.begin()),why);
    }
};
struct DataSet {
    std::vector<std::uint8_t> name;
    std::vector<std::pair<std::uint32_t,std::int32_t>> turns;
};
struct PlayerReplay {
    std::int32_t id=0;
    std::vector<DataSet> sets;
};
struct Frame {
    std::array<std::array<std::int32_t,slots>,score_arrays> scores{};
    std::array<std::int32_t,slots> teams{};
    std::array<PlayerReplay,slots> players{};
};
const saves::PlayerTailNode &field(const saves::PlayerTailObservation &tail,
                                   std::string_view wanted,
                                   std::string_view encoding) {
    const saves::PlayerTailNode *found=nullptr;
    for(const auto &candidate:tail.fields)
        if(name(candidate.name)==wanted) {
            require(!found,"Duplicate BNW replay tail field");
            found=&candidate;
        }
    require(found && name(found->encoding)==encoding &&
            found->wire_count &&
            *found->wire_count>=0 &&
            static_cast<std::uint64_t>(*found->wire_count)==found->children.size() &&
            found->values.empty() && found->bytes.empty(),
            "BNW replay tail field layout changed");
    return *found;
}
Frame frame(const saves::SavedSessionSnapshot &world) {
    require(world.game_state.layout==bnw &&
            world.game_state.roster &&
            world.game_state.roster->dimensions.player_slots==slots &&
            world.game_state.roster->dimensions.team_slots==slots &&
            world.game_state.roster->dimensions.profile_extensions.values.empty() &&
            world.pregame && world.pregame->layout==bnw &&
            world.all_players && world.all_players->layout==bnw &&
            world.all_players->context.player_count==slots &&
            world.all_players->context.profile_extensions.values.empty() &&
            world.all_players->players.size()==slots,
            "BNW score checkpoint requires complete plain 64-player world");
    const auto &roster=*world.game_state.roster;
    const auto teams=world.pregame->archive.integer_arrays.find("team_types");
    require(teams!=world.pregame->archive.integer_arrays.end() &&
            teams->second.size()>=slots &&
            roster.arrays.size()==7,
            "BNW score checkpoint roster dimensions changed");
    Frame result;
    std::size_t names_used=0,turns_used=0,sets_used=0;
    for(std::size_t i=0;i<score_arrays;++i) {
        require(name(saves::RosterObservation::array_names[i+1])==
                    array_names[i] && roster.arrays[i+1].size()==slots,
                "BNW score checkpoint array layout changed");
        for(std::size_t slot=0;slot<slots;++slot)
            result.scores[i][slot]=i32(roster.arrays[i+1][slot]);
    }
    for(std::size_t slot=0;slot<slots;++slot) {
        result.teams[slot]=i32(teams->second[slot]);
        require(result.teams[slot]>=0 && result.teams[slot]<64,
                "BNW score checkpoint team ID changed");
        const auto &player=world.all_players->players[slot];
        require(player.index==slot &&
                player.saved_player_id==static_cast<std::int64_t>(slot) &&
                player.tail.layout==bnw,
                "BNW score checkpoint player order or layout changed");
        auto &output=result.players[slot];
        output.id=i32(player.saved_player_id);
        const auto &names=field(player.tail,"m_ReplayDataSets","strings");
        const auto &values=field(player.tail,"m_ReplayDataSetValues","turn_sets");
        require(names.children.size()==values.children.size() &&
                names.children.size()<=max_data_sets-sets_used,
                "BNW replay dataset names and values disagree");
        sets_used+=names.children.size();
        output.sets.reserve(names.children.size());
        for(std::size_t d=0;d<names.children.size();++d) {
            const auto &n=names.children[d],&v=values.children[d];
            require(name(n.name)=="name" && name(n.encoding)=="string32" &&
                    n.wire_count && *n.wire_count>=0 &&
                    static_cast<std::uint64_t>(*n.wire_count)==n.bytes.size() &&
                    n.values.empty() && n.children.empty() &&
                    name(v.name)=="turns" && name(v.encoding)=="turn_data" &&
                    v.wire_count && *v.wire_count>=0 &&
                    static_cast<std::uint64_t>(*v.wire_count)<=max_turn_values &&
                    static_cast<std::uint64_t>(*v.wire_count)==v.values.size()/2 &&
                    v.values.size()%2==0 && v.bytes.empty() && v.children.empty(),
                    "BNW replay dataset node layout changed");
            require(n.bytes.size()<=max_name_bytes-names_used &&
                    v.values.size()/2<=max_turn_values-turns_used,
                    "BNW replay collection budget exceeded");
            names_used+=n.bytes.size();
            turns_used+=v.values.size()/2;
            auto &data=output.sets.emplace_back();
            data.name=n.bytes;
            data.turns.reserve(v.values.size()/2);
            for(std::size_t t=0;t<v.values.size();t+=2) {
                const auto turn=u32(v.values[t]);
                const auto value=i32(v.values[t+1]);
                require(data.turns.empty() || data.turns.back().first<turn,
                        "BNW replay turns are not strictly ordered");
                data.turns.emplace_back(turn,value);
            }
        }
    }
    return result;
}
void validate_change(const Frame &base,const Frame &live) {
    require(base.teams==live.teams,
            "BNW score checkpoint team roster changed");
    for(std::size_t i=0;i<score_arrays;++i)
        require(base.scores[i][63]==live.scores[i][63],
                "BNW score checkpoint changed barbarian ranking");
    for(std::size_t p=0;p<slots;++p)
        require(base.players[p].id==live.players[p].id,
                "BNW score checkpoint player roster changed");
    require(base.players[63].sets.size()==live.players[63].sets.size(),
            "BNW score checkpoint changed barbarian replay");
    // CvGame::updateScore scores MAX_CIV_PLAYERS=63; barbarian replay is
    // unrelated to this transaction and must have its own future owner.
    for(std::size_t d=0;d<base.players[63].sets.size();++d)
        require(base.players[63].sets[d].name==live.players[63].sets[d].name &&
                base.players[63].sets[d].turns==live.players[63].sets[d].turns,
                "BNW score checkpoint changed barbarian replay");
}
std::vector<std::uint8_t> encode(const Frame &value) {
    Writer out;
    out.u32(slots);out.u32(slots);
    for(const auto &array:value.scores)
        for(auto item:array) out.i32(item);
    for(auto team:value.teams) out.i32(team);
    for(const auto &player:value.players) {
        out.i32(player.id);
        require(player.sets.size()<=max_data_sets,
                "BNW replay dataset budget exceeded");
        out.u32(static_cast<std::uint32_t>(player.sets.size()));
        for(const auto &set:player.sets) {
            require(set.name.size()<=max_name_bytes &&
                    set.turns.size()<=max_turn_values,
                    "BNW replay name or turn budget exceeded");
            out.u32(static_cast<std::uint32_t>(set.name.size()));
            out.raw(set.name);
            out.u32(static_cast<std::uint32_t>(set.turns.size()));
            for(const auto &[turn,score]:set.turns) {
                out.u32(turn);out.i32(score);
            }
        }
    }
    return out.bytes;
}
Frame decode(std::span<const std::uint8_t> bytes) {
    Reader in{bytes};
    require(in.u32()==slots && in.u32()==slots,
            "BNW score checkpoint wire dimensions changed");
    Frame result;
    for(auto &array:result.scores)
        for(auto &item:array) item=in.i32();
    for(auto &team:result.teams) {
        team=in.i32();
        require(team>=0 && team<64,
                "BNW score checkpoint team ID invalid");
    }
    std::size_t names_used=0,turns_used=0,sets_used=0;
    for(std::size_t p=0;p<slots;++p) {
        auto &player=result.players[p];
        player.id=in.i32();
        require(player.id==static_cast<std::int32_t>(p),
                "BNW score checkpoint player ID changed");
        const auto count=in.u32();
        require(count<=max_data_sets-sets_used,
                "BNW replay dataset budget exceeded");
        sets_used+=count;player.sets.reserve(count);
        for(std::size_t d=0;d<count;++d) {
            auto &set=player.sets.emplace_back();
            const auto length=in.u32();
            require(length<=max_name_bytes-names_used,
                    "BNW replay name byte budget exceeded");
            names_used+=length;
            const auto raw=in.raw(length);
            set.name.assign(raw.begin(),raw.end());
            const auto rows=in.u32();
            require(rows<=max_turn_values-turns_used &&
                    rows<=(in.bytes.size()-in.at)/8,
                    "BNW replay turn budget or section length changed");
            turns_used+=rows;set.turns.reserve(rows);
            for(std::size_t t=0;t<rows;++t) {
                const auto turn=in.u32();
                const auto value=in.i32();
                require(set.turns.empty() || set.turns.back().first<turn,
                        "BNW replay turns are not strictly ordered");
                set.turns.emplace_back(turn,value);
            }
        }
    }
    require(in.at==bytes.size(),"Trailing BNW score checkpoint bytes");
    return result;
}
void apply(saves::SavedSessionSnapshot &world,const Frame &value) {
    auto &roster=*world.game_state.roster;
    for(std::size_t i=0;i<score_arrays;++i)
        for(std::size_t p=0;p<slots;++p)
            roster.arrays[i+1][p]=value.scores[i][p];
    for(std::size_t p=0;p<slots;++p) {
        auto &tail=world.all_players->players[p].tail;
        auto &names=const_cast<saves::PlayerTailNode&>(
            field(tail,"m_ReplayDataSets","strings"));
        auto &values=const_cast<saves::PlayerTailNode&>(
            field(tail,"m_ReplayDataSetValues","turn_sets"));
        names.children.resize(value.players[p].sets.size());
        values.children.resize(value.players[p].sets.size());
        names.wire_count=static_cast<std::int64_t>(names.children.size());
        values.wire_count=static_cast<std::int64_t>(values.children.size());
        for(std::size_t d=0;d<names.children.size();++d) {
            const auto &data=value.players[p].sets[d];
            auto &n=names.children[d],&v=values.children[d];
            n.name="name";n.encoding="string32";
            n.wire_count=static_cast<std::int64_t>(data.name.size());
            n.values.clear();n.bytes=data.name;n.children.clear();
            v.name="turns";v.encoding="turn_data";
            v.wire_count=static_cast<std::int64_t>(data.turns.size());
            v.values.clear();v.bytes.clear();v.children.clear();
            v.values.reserve(data.turns.size()*2);
            for(const auto &[turn,score]:data.turns) {
                v.values.push_back(turn);v.values.push_back(score);
            }
        }
    }
}
} // namespace

saves::CivisSaveSection make_bnw_game_score_checkpoint(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity) {
    digest(identity.payload_sha256);digest(identity.rules_sha256);
    const auto base=frame(source),live=frame(world);
    validate_change(base,live);
    const auto old_bytes=encode(base),new_bytes=encode(live);
    constexpr std::size_t header_bytes=8+4+128+8;
    require(old_bytes.size()<=UINT32_MAX && new_bytes.size()<=UINT32_MAX &&
            new_bytes.size()<=max_section_bytes-header_bytes &&
            old_bytes.size()<=max_section_bytes-header_bytes-new_bytes.size(),
            "BNW score checkpoint section budget exceeded");
    Writer out;
    out.raw(magic);out.u32(1);
    out.raw(identity.payload_sha256);out.raw(identity.rules_sha256);
    out.u32(static_cast<std::uint32_t>(old_bytes.size()));
    out.raw(old_bytes);
    out.u32(static_cast<std::uint32_t>(new_bytes.size()));
    out.raw(new_bytes);
    return {bnw_game_score_checkpoint_section_id,0,std::move(out.bytes)};
}

void restore_bnw_game_score_checkpoint(
    const saves::CivisSaveSection &section,
    saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity) {
    digest(identity.payload_sha256);digest(identity.rules_sha256);
    require(section.id==bnw_game_score_checkpoint_section_id &&
            section.flags==0 && section.bytes.size()<=max_section_bytes &&
            section.bytes.size()>=8+4+128+8 &&
            std::equal(magic.begin(),magic.end(),section.bytes.begin()),
            "Unsupported BNW score checkpoint section");
    Reader in{section.bytes,magic.size()};
    require(in.u32()==1,"Unsupported BNW score checkpoint version");
    in.hash(identity.payload_sha256,
            "BNW score checkpoint source payload changed");
    in.hash(identity.rules_sha256,
            "BNW score checkpoint active rules changed");
    const auto old_length=in.u32();
    const auto old_wire=in.raw(old_length);
    const auto new_length=in.u32();
    const auto new_wire=in.raw(new_length);
    require(in.at==section.bytes.size(),
            "Trailing BNW score checkpoint bytes");
    const auto source_frame=frame(source);
    const auto destination_frame=frame(world);
    require(encode(source_frame)==std::vector<std::uint8_t>(
                old_wire.begin(),old_wire.end()) &&
            encode(destination_frame)==std::vector<std::uint8_t>(
                old_wire.begin(),old_wire.end()),
            "BNW score checkpoint destination is not source baseline");
    const auto live=decode(new_wire);
    validate_change(source_frame,live);
    auto replacement=world;
    apply(replacement,live);
    require(encode(frame(replacement))==
                std::vector<std::uint8_t>(new_wire.begin(),new_wire.end()),
            "BNW score checkpoint restored state differs");
    world=std::move(replacement);
}
} // namespace civ5::runtime
