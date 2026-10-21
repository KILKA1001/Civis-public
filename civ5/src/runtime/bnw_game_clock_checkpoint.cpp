#include "civ5/runtime/bnw_game_clock_checkpoint.hpp"
#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <variant>

namespace civ5::runtime {
namespace {
constexpr std::array<std::uint8_t,8> magic{'C','I','V','B','C','L','K','1'};
constexpr auto section_id=static_cast<std::uint32_t>(saves::CivisSaveSectionId::live_bnw_game_clock);
constexpr std::size_t wire_size=8+4+64+64+4*8+3;
void require(bool valid,const char *message) {
    if(!valid) throw std::runtime_error(message);
}
void digest(std::string_view hash) {
    require(hash.size()==64 && std::all_of(hash.begin(),hash.end(),[](char c) {
        return (c>='0'&&c<='9')||(c>='a'&&c<='f');
    }),"Invalid BNW clock checkpoint SHA-256");
}
struct ClockMirrors {
    std::int64_t turn=0, elapsed=0;
    bool score_dirty=false;
};
bool &score_dirty(saves::GameStateObservation &game) {
    bool *found=nullptr;
    for(auto &flag:game.flags) {
        if(flag.name && std::string_view(flag.name)=="score_dirty") {
            require(!found,"Duplicate BNW score-dirty flag");
            found=&flag.value;
        }
    }
    require(found,"Missing BNW score-dirty flag");
    return *found;
}
bool score_dirty_value(const saves::GameStateObservation &game) {
    const bool *found=nullptr;
    for(const auto &flag:game.flags) {
        if(flag.name && std::string_view(flag.name)=="score_dirty") {
            require(!found,"Duplicate BNW score-dirty flag");
            found=&flag.value;
        }
    }
    require(found,"Missing BNW score-dirty flag");
    return *found;
}
ClockMirrors mirrors(const saves::SavedSessionSnapshot &world) {
    constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
    require(world.game_state.layout==bnw && world.pregame &&
            world.pregame->layout==bnw &&
            std::holds_alternative<saves::Expansion2PrefixObservation>(world.game_state.prefix),
            "BNW clock checkpoint requires an Expansion2 world");
    const auto found=world.pregame->archive.integers.find("game_turn");
    require(found!=world.pregame->archive.integers.end(),
            "BNW clock checkpoint has no saved game turn");
    const auto elapsed=std::get<saves::Expansion2PrefixObservation>(
        world.game_state.prefix).scalars[1];
    require(found->second>=0 && elapsed>=0,
            "BNW clock checkpoint has a negative clock");
    return {found->second,elapsed,score_dirty_value(world.game_state)};
}
struct Writer {
    std::vector<std::uint8_t> bytes;
    void raw(std::string_view value) { bytes.insert(bytes.end(),value.begin(),value.end()); }
    void u8(bool value) { bytes.push_back(value?1:0); }
    void u32(std::uint32_t value) {
        for(unsigned i=0;i<4;++i) bytes.push_back(static_cast<std::uint8_t>(value>>(8*i)));
    }
    void u64(std::int64_t value) {
        require(value>=0,"Negative BNW clock checkpoint value");
        const auto unsigned_value=static_cast<std::uint64_t>(value);
        for(unsigned i=0;i<8;++i) bytes.push_back(static_cast<std::uint8_t>(unsigned_value>>(8*i)));
    }
};
struct Reader {
    std::span<const std::uint8_t> bytes;
    std::size_t at=0;
    bool u8() {
        const auto value=bytes[at++];
        require(value<=1,"Invalid BNW clock checkpoint flag");
        return value!=0;
    }
    std::uint32_t u32() {
        std::uint32_t value=0;
        for(unsigned i=0;i<4;++i) value|=std::uint32_t(bytes[at++])<<(8*i);
        return value;
    }
    std::int64_t u64() {
        std::uint64_t value=0;
        for(unsigned i=0;i<8;++i) value|=std::uint64_t(bytes[at++])<<(8*i);
        require(value<=static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max()),
                "BNW clock checkpoint value exceeds signed range");
        return static_cast<std::int64_t>(value);
    }
    void hash(std::string_view expected,const char *message) {
        require(std::equal(expected.begin(),expected.end(),bytes.begin()+at),message);
        at+=expected.size();
    }
};
void check_progression(const ClockMirrors &base,const ClockMirrors &live) {
    require(live.turn>=base.turn && live.elapsed>=base.elapsed &&
            live.turn-base.turn==live.elapsed-base.elapsed,
            "BNW clock checkpoint has inconsistent turn progression");
}
} // namespace

saves::CivisSaveSection make_bnw_game_clock_checkpoint(
    const LiveSessionState &state,const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity) {
    digest(identity.payload_sha256);digest(identity.rules_sha256);
    require(!state.game_clock.pending,
            "BNW clock publication must finish before checkpoint");
    const auto base=mirrors(source);
    const auto live=mirrors(state.world);
    require(state.setup.profile==content::TechnologyProfile::bnw &&
            state.setup.game_turn==live.turn &&
            state.maintenance.elapsed_turns==live.elapsed,
            "BNW clock checkpoint owners disagree");
    check_progression(base,live);
    Writer out;out.bytes.reserve(wire_size);
    out.bytes.insert(out.bytes.end(),magic.begin(),magic.end());out.u32(1);
    out.raw(identity.payload_sha256);out.raw(identity.rules_sha256);
    out.u64(base.turn);out.u64(base.elapsed);
    out.u64(live.turn);out.u64(live.elapsed);
    out.u8(base.score_dirty);out.u8(live.score_dirty);
    out.u8(state.game_clock.sent_auto_moves);
    return {section_id,0,std::move(out.bytes)};
}

void restore_bnw_game_clock_checkpoint(
    const saves::CivisSaveSection &section,saves::SavedSessionSnapshot &world,
    BnwGameClockState &clock,const SessionCheckpointIdentity &identity) {
    digest(identity.payload_sha256);digest(identity.rules_sha256);
    require(section.id==section_id && section.flags==0 && section.bytes.size()==wire_size,
            "Unsupported BNW clock checkpoint section");
    require(std::equal(magic.begin(),magic.end(),section.bytes.begin()),
            "Unsupported BNW clock checkpoint magic");
    require(!clock.pending,"Cannot restore BNW clock over pending publication");
    Reader in{section.bytes,magic.size()};
    require(in.u32()==1,"Unsupported BNW clock checkpoint version");
    in.hash(identity.payload_sha256,"BNW clock checkpoint source payload changed");
    in.hash(identity.rules_sha256,"BNW clock checkpoint active rules changed");
    ClockMirrors base,live;
    base.turn=in.u64();base.elapsed=in.u64();
    live.turn=in.u64();live.elapsed=in.u64();
    base.score_dirty=in.u8();live.score_dirty=in.u8();
    const auto sent_auto_moves=in.u8();
    require(in.at==section.bytes.size(),"Trailing BNW clock checkpoint bytes");
    check_progression(base,live);
    auto replacement=world;
    const auto actual=mirrors(replacement);
    require(actual.turn==base.turn && actual.elapsed==base.elapsed &&
            actual.score_dirty==base.score_dirty,
            "BNW clock checkpoint source clock changed");
    replacement.pregame->archive.integers.at("game_turn")=live.turn;
    std::get<saves::Expansion2PrefixObservation>(replacement.game_state.prefix).scalars[1]=live.elapsed;
    score_dirty(replacement.game_state)=live.score_dirty;
    world=std::move(replacement);
    clock.sent_auto_moves=sent_auto_moves;
}
} // namespace civ5::runtime
