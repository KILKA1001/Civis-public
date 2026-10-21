#include "civ5/runtime/bnw_team_turn_checkpoint.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <climits>
#include <span>
#include <set>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
constexpr std::array<std::uint8_t,8> magic{'C','I','V','B','T','T','0','1'};
constexpr std::size_t teams=64,civil_teams=63,tech_budget=1'000'000;
constexpr std::size_t award_budget=1'000'000;
constexpr std::size_t byte_budget=32*1024*1024;
void require(bool valid,const char *why) {
    if(!valid) throw std::runtime_error(why);
}
std::string_view name(const char *value) {
    return value?std::string_view(value):std::string_view();
}
void digest(std::string_view hash) {
    require(hash.size()==64 &&
            std::all_of(hash.begin(),hash.end(),[](char c) {
                return (c>='0'&&c<='9')||(c>='a'&&c<='f');
            }),"Invalid BNW team turn checkpoint SHA-256");
}
std::int32_t i32(std::int64_t value) {
    require(value>=INT32_MIN && value<=INT32_MAX,
            "BNW team turn value exceeds source i32");
    return static_cast<std::int32_t>(value);
}
std::uint32_t u32(std::int64_t value) {
    require(value>=0 && value<=UINT32_MAX,
            "BNW team technology hash exceeds source u32");
    return static_cast<std::uint32_t>(value);
}
struct Writer {
    std::vector<std::uint8_t> bytes;
    void raw(std::span<const std::uint8_t> value) {
        require(value.size()<=byte_budget &&
                bytes.size()<=byte_budget-value.size(),
                "BNW team turn section byte budget exceeded");
        bytes.insert(bytes.end(),value.begin(),value.end());
    }
    void raw(std::string_view value) {
        raw({reinterpret_cast<const std::uint8_t*>(value.data()),value.size()});
    }
    void u8(bool value) {
        require(bytes.size()<byte_budget,
                "BNW team turn section byte budget exceeded");
        bytes.push_back(value?1:0);
    }
    void u32(std::uint32_t value) {
        require(bytes.size()<=byte_budget-4,
                "BNW team turn section byte budget exceeded");
        for(unsigned i=0;i<4;++i)
            bytes.push_back(static_cast<std::uint8_t>(value>>(8*i)));
    }
    void i32(std::int64_t value) {
        u32(std::bit_cast<std::uint32_t>(::civ5::runtime::i32(value)));
    }
};
struct Reader {
    std::span<const std::uint8_t> bytes;
    std::size_t at=0;
    std::span<const std::uint8_t> raw(std::size_t length) {
        require(at<=bytes.size() && length<=bytes.size()-at,
                "Truncated BNW team turn checkpoint");
        const auto value=bytes.subspan(at,length);at+=length;return value;
    }
    bool u8() {
        const auto value=raw(1)[0];
        require(value<=1,"Invalid BNW team turn boolean");
        return value!=0;
    }
    std::uint32_t u32() {
        const auto bytes=raw(4);
        std::uint32_t value=0;
        for(unsigned i=0;i<4;++i) value|=std::uint32_t(bytes[i])<<(8*i);
        return value;
    }
    std::int32_t i32() {return std::bit_cast<std::int32_t>(u32());}
    void hash(std::string_view expected,const char *why) {
        const auto value=raw(expected.size());
        require(std::equal(value.begin(),value.end(),expected.begin()),why);
    }
};
const saves::NamedSaveInteger &field(const saves::TeamObservation &team,
                                      std::string_view wanted) {
    const saves::NamedSaveInteger *found=nullptr;
    for(const auto &value:team.fields)
        if(name(value.name)==wanted) {
            require(!found,"Duplicate BNW team turn field");
            found=&value;
        }
    require(found,"Missing BNW team turn field");
    return *found;
}
const saves::PlotArrayObservation &array(const saves::TeamObservation &team,
                                          std::string_view wanted) {
    const saves::PlotArrayObservation *found=nullptr;
    for(const auto &value:team.arrays)
        if(name(value.name)==wanted) {
            require(!found,"Duplicate BNW team turn array");
            found=&value;
        }
    require(found && found->columns==1,"Missing BNW team turn array");
    return *found;
}
saves::PlotArrayObservation &array(saves::TeamObservation &team,
                                   std::string_view wanted) {
    (void)array(static_cast<const saves::TeamObservation&>(team),wanted);
    for(auto &value:team.arrays)
        if(name(value.name)==wanted) return value;
    throw std::logic_error("Missing mutable BNW team turn array");
}
saves::NamedSaveInteger &field(saves::TeamObservation &team,
                               std::string_view wanted) {
    (void)field(static_cast<const saves::TeamObservation&>(team),wanted);
    for(auto &value:team.fields) if(name(value.name)==wanted) return value;
    throw std::logic_error("Missing mutable BNW team turn field");
}
const saves::TeamHashedArray &awards(const saves::TeamObservation &team) {
    const saves::TeamHashedArray *found=nullptr;
    for(const auto &value:team.hashes) if(name(value.name)=="small_award_achieved") {
        require(!found,"Duplicate BNW small-award array");found=&value;
    }
    require(found,"Missing BNW small-award array");return *found;
}
saves::TeamHashedArray &awards(saves::TeamObservation &team) {
    (void)awards(static_cast<const saves::TeamObservation&>(team));
    for(auto &value:team.hashes)
        if(name(value.name)=="small_award_achieved") return value;
    throw std::logic_error("Missing mutable BNW small-award array");
}
struct AwardFlag {
    std::uint32_t hash=0;
    bool achieved=false; // hash0 has no saved value; canonical placeholder only.
};
struct TechFlag {
    std::uint32_t hash=0;
    bool no_trade=false;
    bool operator==(const TechFlag &) const = default;
};
struct Team {
    std::int32_t id=0;
    std::array<std::int32_t,teams> turns_at_war{};
    std::array<std::int32_t,teams> turns_locked_at_war{};
    std::vector<TechFlag> technologies;
    std::int32_t victory_points=0;
    std::vector<AwardFlag> small_awards;
};
using Frame=std::array<Team,teams>;
Frame frame(const saves::SavedSessionSnapshot &world,std::uint32_t version) {
    require(world.game_state.layout==bnw && world.teams &&
            world.teams->layout==bnw &&
            world.teams->dimensions.teams==teams &&
            world.teams->dimensions.civ_teams==civil_teams &&
            world.teams->dimensions.profile_extensions.values.empty() &&
            world.teams->teams.size()==teams,
            "BNW team turn requires complete plain 64-team roster");
    Frame result;
    std::size_t tech_rows=0,award_rows=0;
    for(std::size_t t=0;t<teams;++t) {
        const auto &source=world.teams->teams[t];
        auto &live=result[t];
        require(source.index==t &&
                field(source,"id").value==static_cast<std::int64_t>(t) &&
                field(source,"tech_version").value==1,
                "BNW team turn ID or technology version changed");
        live.id=static_cast<std::int32_t>(t);
        const auto &war=array(source,"turns_at_war");
        const auto &locked=array(source,"turns_locked_at_war");
        require(war.values.size()==teams && locked.values.size()==teams,
                "BNW team war counter dimensions changed");
        for(std::size_t other=0;other<teams;++other) {
            live.turns_at_war[other]=i32(war.values[other]);
            live.turns_locked_at_war[other]=i32(locked.values[other]);
        }
        const auto saved=field(source,"saved_techs").value;
        require(saved>=0 && saved<=static_cast<std::int64_t>(tech_budget),
                "BNW team saved technology count changed");
        const auto &hashes=array(source,"tech_hashes");
        const auto &flags=array(source,"no_trade_tech");
        require(hashes.values.size()==static_cast<std::size_t>(saved) &&
                flags.values.size()==static_cast<std::size_t>(saved) &&
                flags.values.size()<=tech_budget-tech_rows,
                "BNW team no-trade row count changed");
        tech_rows+=flags.values.size();
        live.technologies.reserve(flags.values.size());
        for(std::size_t tech=0;tech<flags.values.size();++tech) {
            require(flags.values[tech]==0 || flags.values[tech]==1,
                    "Invalid BNW no-trade technology flag");
            live.technologies.push_back(
                {u32(hashes.values[tech]),flags.values[tech]!=0});
        }
        if(version==2) {
            live.victory_points=i32(field(source,"victory_points").value);
            const auto &saved_awards=awards(source);
            require(saved_awards.wire_count>=0 &&
                    saved_awards.wire_count<=static_cast<std::int64_t>(award_budget) &&
                    static_cast<std::size_t>(saved_awards.wire_count)==saved_awards.rows.size() &&
                    saved_awards.rows.size()<=award_budget-award_rows,
                    "BNW small-award complete wire count changed");
            award_rows+=saved_awards.rows.size();
            live.small_awards.reserve(saved_awards.rows.size());
            std::set<std::uint32_t> seen;
            for(const auto &row:saved_awards.rows) {
                require(row.type_hash<=UINT32_MAX,"BNW small-award hash exceeds u32");
                const auto hash=static_cast<std::uint32_t>(row.type_hash);
                if(!hash) require(row.values.empty(),"BNW zero-hash award has a value");
                else require(seen.insert(hash).second && row.values.size()==1 &&
                             (row.values[0]==0||row.values[0]==1),
                             "Ambiguous or nonboolean BNW small-award row");
                live.small_awards.push_back({hash,hash?row.values[0]!=0:false});
            }
        }
    }
    return result;
}
void same_roster(const Frame &base,const Frame &live,std::uint32_t version) {
    for(std::size_t t=0;t<teams;++t) {
        require(base[t].id==live[t].id &&
                base[t].technologies.size()==live[t].technologies.size(),
                "BNW team turn team or technology roster changed");
        for(std::size_t row=0;row<base[t].technologies.size();++row)
            require(base[t].technologies[row].hash==
                    live[t].technologies[row].hash,
                    "BNW team turn technology hashes changed");
        if(version==2) {
            require(base[t].small_awards.size()==live[t].small_awards.size(),
                    "BNW small-award roster count changed");
            for(std::size_t row=0;row<base[t].small_awards.size();++row)
                require(base[t].small_awards[row].hash==live[t].small_awards[row].hash,
                        "BNW small-award hash order changed");
        }
        require(base[t].turns_at_war[63]==live[t].turns_at_war[63] &&
                base[t].turns_locked_at_war[63]==
                    live[t].turns_locked_at_war[63],
                "BNW team turn changed barbarian war slot");
        if(t==63)
            require(base[t].turns_at_war==live[t].turns_at_war &&
                    base[t].turns_locked_at_war==
                        live[t].turns_locked_at_war,
                    "BNW team turn changed barbarian war counters");
    }
}
std::vector<std::uint8_t> encode(const Frame &value,std::uint32_t version) {
    Writer out;
    out.u32(teams);out.u32(civil_teams);
    for(const auto &team:value) {
        out.i32(team.id);
        for(const auto n:team.turns_at_war) out.i32(n);
        for(const auto n:team.turns_locked_at_war) out.i32(n);
        require(team.technologies.size()<=UINT32_MAX,
                "BNW team no-trade count exceeds section wire");
        out.u32(static_cast<std::uint32_t>(team.technologies.size()));
        for(const auto &tech:team.technologies) {
            out.u32(tech.hash);out.u8(tech.no_trade);
        }
        if(version==2) {
            out.i32(team.victory_points);
            out.u32(static_cast<std::uint32_t>(team.small_awards.size()));
            for(const auto &award:team.small_awards) {
                out.u32(award.hash);
                if(award.hash) out.u8(award.achieved);
            }
        }
    }
    return out.bytes;
}
Frame decode(std::span<const std::uint8_t> bytes,std::uint32_t version) {
    Reader in{bytes};
    require(in.u32()==teams && in.u32()==civil_teams,
            "BNW team turn wire dimensions changed");
    Frame result;
    std::size_t tech_rows=0,award_rows=0;
    for(std::size_t t=0;t<teams;++t) {
        auto &team=result[t];
        team.id=in.i32();
        require(team.id==static_cast<std::int32_t>(t),
                "BNW team turn wire team ID changed");
        for(auto &value:team.turns_at_war) value=in.i32();
        for(auto &value:team.turns_locked_at_war) value=in.i32();
        const auto rows=in.u32();
        require(rows<=tech_budget-tech_rows &&
                rows<=(bytes.size()-in.at)/5,
                "BNW team no-trade wire row count exceeds input or budget");
        tech_rows+=rows;team.technologies.reserve(rows);
        for(std::size_t i=0;i<rows;++i)
            team.technologies.push_back({in.u32(),in.u8()});
        if(version==2) {
            team.victory_points=in.i32();
            const auto count=in.u32();
            require(count<=award_budget-award_rows && count<=(bytes.size()-in.at)/4,
                    "BNW small-award wire count exceeds input or budget");
            award_rows+=count;team.small_awards.reserve(count);
            std::set<std::uint32_t> seen;
            for(std::size_t i=0;i<count;++i) {
                const auto hash=in.u32();
                require(!hash||seen.insert(hash).second,"Duplicate BNW award wire hash");
                team.small_awards.push_back({hash,hash?in.u8():false});
            }
        }
    }
    require(in.at==bytes.size(),"Trailing BNW team turn bytes");
    return result;
}
} // namespace

saves::CivisSaveSection make_bnw_team_turn_checkpoint(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity) {
    digest(identity.payload_sha256);digest(identity.rules_sha256);
    constexpr std::uint32_t version=2;
    const auto base=frame(source,version),live=frame(world,version);
    same_roster(base,live,version);
    const auto old_bytes=encode(base,version),new_bytes=encode(live,version);
    constexpr std::size_t overhead=8+4+128+8;
    require(new_bytes.size()<=byte_budget-overhead &&
            old_bytes.size()<=byte_budget-overhead-new_bytes.size(),
            "BNW team turn section budget exceeded");
    Writer out;
    out.raw(magic);out.u32(version);
    out.raw(identity.payload_sha256);out.raw(identity.rules_sha256);
    out.u32(static_cast<std::uint32_t>(old_bytes.size()));out.raw(old_bytes);
    out.u32(static_cast<std::uint32_t>(new_bytes.size()));out.raw(new_bytes);
    return {bnw_team_turn_checkpoint_section_id,0,std::move(out.bytes)};
}

void restore_bnw_team_turn_checkpoint(
    const saves::CivisSaveSection &section,
    saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity) {
    digest(identity.payload_sha256);digest(identity.rules_sha256);
    require(section.id==bnw_team_turn_checkpoint_section_id &&
            section.flags==0 && section.bytes.size()>=8+4+128+8 &&
            section.bytes.size()<=byte_budget &&
            std::equal(magic.begin(),magic.end(),section.bytes.begin()),
            "Unsupported BNW team turn section");
    Reader in{section.bytes,magic.size()};
    const auto version=in.u32();
    require(version==1||version==2,"Unsupported BNW team turn section version");
    in.hash(identity.payload_sha256,"BNW team turn payload changed");
    in.hash(identity.rules_sha256,"BNW team turn active rules changed");
    const auto old_wire=in.raw(in.u32()),new_wire=in.raw(in.u32());
    require(in.at==section.bytes.size(),"Trailing BNW team turn section bytes");
    const auto base=frame(source,version),destination=frame(world,version);
    const auto old_bytes=encode(base,version);
    require(old_wire.size()==old_bytes.size() &&
            std::equal(old_wire.begin(),old_wire.end(),old_bytes.begin()) &&
            encode(destination,version)==old_bytes,
            "BNW team turn source or destination baseline changed");
    const auto live=decode(new_wire,version);
    same_roster(base,live,version);
    // All potentially throwing lookups and checks precede these scalar writes.
    std::array<std::array<std::vector<std::int64_t>*,3>,teams> targets{};
    std::array<std::int64_t*,teams> point_targets{};
    std::array<std::vector<std::int64_t*>,teams> award_targets{};
    for(std::size_t t=0;t<teams;++t) {
        auto &team=world.teams->teams[t];
        if(version==2) {
            point_targets[t]=&field(team,"victory_points").value;
            const auto &rows=awards(team).rows;
            award_targets[t].reserve(rows.size());
            for(auto &row:awards(team).rows)
                award_targets[t].push_back(row.type_hash?&row.values[0]:nullptr);
        }
        targets[t]={&array(team,"turns_at_war").values,
                    &array(team,"turns_locked_at_war").values,
                    &array(team,"no_trade_tech").values};
    }
    for(std::size_t t=0;t<teams;++t) {
        for(std::size_t row=0;row<teams;++row) {
            (*targets[t][0])[row]=live[t].turns_at_war[row];
            (*targets[t][1])[row]=live[t].turns_locked_at_war[row];
        }
        for(std::size_t row=0;row<live[t].technologies.size();++row)
            (*targets[t][2])[row]=live[t].technologies[row].no_trade;
        if(version==2) {
            *point_targets[t]=live[t].victory_points;
            for(std::size_t row=0;row<live[t].small_awards.size();++row)
                if(award_targets[t][row])
                    *award_targets[t][row]=live[t].small_awards[row].achieved;
        }
    }
}
} // namespace civ5::runtime
