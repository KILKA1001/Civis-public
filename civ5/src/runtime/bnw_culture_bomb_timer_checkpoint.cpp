#include "civ5/runtime/bnw_culture_bomb_timer_checkpoint.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <array>
#include <climits>
#include <span>
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
constexpr std::array<std::uint8_t,8> magic{'C','I','V','B','C','B','T','1'};
constexpr std::size_t player_count=64;
constexpr std::size_t section_bytes=8+4+64*3+4+player_count*12;
void require(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
void sha(std::string_view value) {
    require(value.size()==64 &&
            std::all_of(value.begin(),value.end(),[](char c) {
                return (c>='0' && c<='9') || (c>='a' && c<='f');
            }),"Invalid BNW culture bomb timer SHA-256");
}
std::string_view name(const char *value) {
    return value?std::string_view(value):std::string_view();
}
struct Writer {
    std::vector<std::uint8_t> bytes;
    void u32(std::uint32_t value) {
        for(unsigned i=0;i<4;++i)
            bytes.push_back(static_cast<std::uint8_t>(value>>(8*i)));
    }
    void u64(std::uint64_t value) {
        for(unsigned i=0;i<8;++i)
            bytes.push_back(static_cast<std::uint8_t>(value>>(8*i)));
    }
    void i32(std::int64_t value) {
        require(value>=INT32_MIN && value<=INT32_MAX,
                "BNW culture bomb timer exceeds i32 wire range");
        u32(static_cast<std::uint32_t>(static_cast<std::int32_t>(value)));
    }
    void i64(std::int64_t value) {u64(static_cast<std::uint64_t>(value));}
    void string(std::string_view value) {
        require(!value.empty() && value.size()<=UINT32_MAX,
                "BNW culture bomb source schema name unavailable");
        u32(static_cast<std::uint32_t>(value.size()));
        bytes.insert(bytes.end(),value.begin(),value.end());
    }
    void raw(std::string_view value) {
        bytes.insert(bytes.end(),value.begin(),value.end());
    }
};
struct Reader {
    std::span<const std::uint8_t> bytes;
    std::size_t at=0;
    std::uint32_t u32() {
        require(at<=bytes.size() && bytes.size()-at>=4,
                "Truncated BNW culture bomb timer section");
        std::uint32_t value=0;
        for(unsigned i=0;i<4;++i) value|=std::uint32_t(bytes[at++])<<(8*i);
        return value;
    }
    std::int64_t i32() {return static_cast<std::int32_t>(u32());}
    void hash(std::string_view expected) {
        require(at<=bytes.size() && bytes.size()-at>=expected.size() &&
                std::equal(expected.begin(),expected.end(),bytes.begin()+at),
                "BNW culture bomb source, rules or roster changed");
        at+=expected.size();
    }
};
void source_shape(const saves::SavedSessionSnapshot &source) {
    require(source.game_state.layout==bnw && source.all_players &&
            source.all_players->layout==bnw &&
            source.all_players->context.player_count==player_count &&
            source.all_players->players.size()==player_count &&
            source.all_players->context.profile_extensions.values.empty(),
            "BNW culture bomb timer needs complete plain player roster");
    for(std::size_t i=0;i<player_count;++i) {
        const auto &player=source.all_players->players[i];
        require(player.index==i && player.saved_player_id==static_cast<std::int64_t>(i) &&
                player.core.layout==bnw,
                "BNW culture bomb player ID or profile changed");
        for(const auto &field:player.core.fields)
            require(name(field.name)!="m_iCultureBombTimer",
                    "BNW source unexpectedly serialized culture bomb timer");
    }
}
std::string roster_hash(const saves::SavedSessionSnapshot &source) {
    source_shape(source);
    Writer out;
    out.u64(source.all_players->start_offset);
    out.u64(source.all_players->end_offset);
    for(const auto &player:source.all_players->players) {
        const auto &core=player.core;
        out.u64(player.index);out.i32(player.saved_player_id);
        out.u64(core.start_offset);out.u64(core.end_offset);
        out.u64(core.yield_slots);
        out.u32(core.version.has_value());
        if(core.version) out.u64(*core.version);
        out.u64(core.fields.size());
        for(const auto &field:core.fields) {
            out.string(name(field.name));
            out.u32(static_cast<std::uint32_t>(field.encoding));
            out.u64(field.start_offset);out.u64(field.end_offset);
            out.u32(field.wire_count.has_value());
            if(field.wire_count) out.u64(*field.wire_count);
            out.u64(field.values.size());
            for(const auto value:field.values) out.i64(value);
            out.u64(field.row_counts.size());
            for(const auto count:field.row_counts) out.u64(count);
            out.u64(field.hashes.size());
            for(const auto hash:field.hashes) out.u64(hash);
            out.u64(field.bytes.size());
            out.bytes.insert(out.bytes.end(),field.bytes.begin(),field.bytes.end());
        }
    }
    return identify_bytes(out.bytes).sha256;
}
void timer_roster(const BnwCultureBombTimers &timers,bool baseline) {
    require(timers.size()==player_count,
            "BNW culture bomb timer player roster changed");
    for(std::size_t i=0;i<player_count;++i) {
        const auto found=timers.find(static_cast<std::int64_t>(i));
        require(found!=timers.end() && found->second>=0 &&
                (!baseline || found->second==0),
                "BNW culture bomb timer ID, range or reset baseline changed");
    }
}
} // namespace

saves::CivisSaveSection make_bnw_culture_bomb_timer_checkpoint(
    const BnwCultureBombTimers &timers,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity) {
    sha(identity.payload_sha256);sha(identity.rules_sha256);
    timer_roster(timers,false);
    const auto source_hash=roster_hash(source);
    Writer out;out.bytes.reserve(section_bytes);
    out.bytes.insert(out.bytes.end(),magic.begin(),magic.end());
    out.u32(1);out.raw(identity.payload_sha256);out.raw(identity.rules_sha256);
    out.raw(source_hash);out.u32(player_count);
    for(std::size_t i=0;i<player_count;++i) {
        out.i32(static_cast<std::int64_t>(i));out.i32(0);
        out.i32(timers.at(static_cast<std::int64_t>(i)));
    }
    require(out.bytes.size()==section_bytes,
            "BNW culture bomb timer section length changed");
    return {bnw_culture_bomb_timer_checkpoint_section_id,0,
            std::move(out.bytes)};
}

void restore_bnw_culture_bomb_timer_checkpoint(
    const saves::CivisSaveSection &section,
    BnwCultureBombTimers &timers,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity) {
    sha(identity.payload_sha256);sha(identity.rules_sha256);
    timer_roster(timers,true);
    require(section.id==bnw_culture_bomb_timer_checkpoint_section_id &&
            section.flags==0 && section.bytes.size()==section_bytes &&
            std::equal(magic.begin(),magic.end(),section.bytes.begin()),
            "Unsupported BNW culture bomb timer section");
    const auto source_hash=roster_hash(source);
    Reader in{section.bytes,magic.size()};
    require(in.u32()==1,"Unsupported BNW culture bomb timer version");
    in.hash(identity.payload_sha256);in.hash(identity.rules_sha256);
    in.hash(source_hash);
    require(in.u32()==player_count,"BNW culture bomb player count changed");
    BnwCultureBombTimers staged;
    for(std::size_t i=0;i<player_count;++i) {
        const auto id=in.i32(),old=in.i32(),live=in.i32();
        require(id==static_cast<std::int64_t>(i) && old==0 && live>=0,
                "BNW culture bomb timer row or reset baseline changed");
        require(staged.emplace(id,static_cast<std::int32_t>(live)).second,
                "Duplicate BNW culture bomb timer player");
    }
    require(in.at==section.bytes.size(),
            "Trailing BNW culture bomb timer bytes");
    // All checks and allocations precede the only destination mutation.
    timers.swap(staged);
}

} // namespace civ5::runtime
