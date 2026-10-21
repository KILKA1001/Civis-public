#include "civ5/runtime/bnw_game_supporting_checkpoint.hpp"
#include <algorithm>
#include <array>
#include <climits>
#include <limits>
#include <span>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <variant>

namespace civ5::runtime {
namespace {
constexpr std::array<std::uint8_t,8> magic{'C','I','V','B','G','S','0','1'};
constexpr auto id=static_cast<std::uint32_t>(
    saves::CivisSaveSectionId::live_bnw_game_supporting);
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void require(bool valid,const char *why) {
    if(!valid) throw std::runtime_error(why);
}
void digest(std::string_view hash) {
    require(hash.size()==64 && std::all_of(hash.begin(),hash.end(),[](char c) {
        return (c>='0' && c<='9') || (c>='a' && c<='f');
    }),"Invalid BNW game supporting checkpoint identity");
}
const saves::SupportingClassesObservation &supporting(
    const saves::SavedSessionSnapshot &world) {
    require(world.game_state.layout==bnw && world.map_header.layout==bnw &&
            world.supporting_classes && world.supporting_classes->layout==bnw &&
            std::holds_alternative<saves::Expansion2PrefixObservation>(
                world.game_state.prefix),
            "BNW game supporting checkpoint requires decoded Expansion2 state");
    const auto &saved=*world.supporting_classes;
    const auto plots=world.map_header.grid.size();
    require(saved.game_version==1 && saved.barbarians_version==1 &&
            saved.goody_huts_version==1 && plots>0 &&
            saved.plot_count==plots && saved.barbarians.size()==plots &&
            saved.major_players<=64 &&
            saved.goody_hut_results.size()==saved.major_players,
            "BNW game supporting checkpoint object counts or versions disagree");
    for(const auto &plot:saved.barbarians)
        require(!plot.attacked &&
                plot.counter>=INT16_MIN && plot.counter<=INT16_MAX &&
                plot.units_spawned>=INT16_MIN && plot.units_spawned<=INT16_MAX,
                "BNW barbarian counters exceed source short range");
    for(const auto &row:saved.goody_hut_results)
        for(const auto value:row)
            require(value>=INT32_MIN && value<=INT32_MAX,
                    "BNW goody history exceeds source int range");
    const auto progress=std::get<saves::Expansion2PrefixObservation>(
        world.game_state.prefix).scalars[23];
    require(progress>=INT32_MIN && progress<=INT32_MAX,
            "BNW religion cache exceeds source int range");
    return saved;
}
std::int64_t progress(const saves::SavedSessionSnapshot &world) {
    return std::get<saves::Expansion2PrefixObservation>(
        world.game_state.prefix).scalars[23];
}
struct Writer {
    std::vector<std::uint8_t> bytes;
    void raw(std::string_view value) {
        bytes.insert(bytes.end(),value.begin(),value.end());
    }
    void number(std::uint64_t value,unsigned width) {
        for(unsigned i=0;i<width;++i)
            bytes.push_back(static_cast<std::uint8_t>(value>>(8*i)));
    }
    void i16(std::int64_t value) {
        require(value>=INT16_MIN && value<=INT16_MAX,
                "BNW supporting short overflow");
        number(static_cast<std::uint16_t>(value),2);
    }
    void i32(std::int64_t value) {
        require(value>=INT32_MIN && value<=INT32_MAX,
                "BNW supporting int overflow");
        number(static_cast<std::uint32_t>(value),4);
    }
};
struct Reader {
    std::span<const std::uint8_t> bytes;
    std::size_t at=0;
    std::uint64_t number(unsigned width) {
        require(at<=bytes.size() && width<=bytes.size()-at,
                "Truncated BNW game supporting checkpoint");
        std::uint64_t value=0;
        for(unsigned i=0;i<width;++i)
            value|=std::uint64_t(bytes[at++])<<(8*i);
        return value;
    }
    std::int64_t i16() { return static_cast<std::int16_t>(number(2)); }
    std::int64_t i32() { return static_cast<std::int32_t>(number(4)); }
    void hash(std::string_view expected,const char *why) {
        require(at<=bytes.size() && expected.size()<=bytes.size()-at &&
                std::equal(expected.begin(),expected.end(),bytes.begin()+at),why);
        at+=expected.size();
    }
};
void same_shape(const saves::SupportingClassesObservation &base,
                const saves::SupportingClassesObservation &live) {
    require(base.plot_count==live.plot_count &&
            base.major_players==live.major_players &&
            base.barbarians.size()==live.barbarians.size() &&
            base.goody_hut_results.size()==live.goody_hut_results.size(),
            "BNW game supporting object counts changed");
}
} // namespace

saves::CivisSaveSection make_bnw_game_supporting_checkpoint(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity) {
    digest(identity.payload_sha256);digest(identity.rules_sha256);
    const auto &base=supporting(source),&live=supporting(world);
    same_shape(base,live);
    require(source.map_header.grid.width==world.map_header.grid.width &&
            source.map_header.grid.height==world.map_header.grid.height,
            "BNW game supporting map dimensions changed");
    Writer out;
    out.bytes.insert(out.bytes.end(),magic.begin(),magic.end());
    out.number(1,4);
    out.raw(identity.payload_sha256);out.raw(identity.rules_sha256);
    out.number(base.plot_count,8);out.number(base.major_players,8);
    out.i32(progress(source));out.i32(progress(world));
    for(std::size_t i=0;i<base.barbarians.size();++i) {
        out.i16(base.barbarians[i].counter);
        out.i16(base.barbarians[i].units_spawned);
        out.i16(live.barbarians[i].counter);
        out.i16(live.barbarians[i].units_spawned);
    }
    for(std::size_t i=0;i<base.goody_hut_results.size();++i)
        for(std::size_t j=0;j<3;++j) {
            out.i32(base.goody_hut_results[i][j]);
            out.i32(live.goody_hut_results[i][j]);
        }
    return {id,0,std::move(out.bytes)};
}

void restore_bnw_game_supporting_checkpoint(
    const saves::CivisSaveSection &section,
    saves::SavedSessionSnapshot &world,
    const SessionCheckpointIdentity &identity) {
    digest(identity.payload_sha256);digest(identity.rules_sha256);
    require(section.id==id && section.flags==0 &&
            section.bytes.size()>=magic.size()+4+128+16+8,
            "Unsupported BNW game supporting checkpoint section");
    require(std::equal(magic.begin(),magic.end(),section.bytes.begin()),
            "Unsupported BNW game supporting checkpoint magic");
    const auto &base=supporting(world);
    Reader in{section.bytes,magic.size()};
    require(in.number(4)==1,"Unsupported BNW game supporting version");
    in.hash(identity.payload_sha256,"BNW game supporting payload changed");
    in.hash(identity.rules_sha256,"BNW game supporting rules changed");
    require(in.number(8)==base.plot_count &&
            in.number(8)==base.major_players,
            "BNW game supporting source object counts changed");
    const auto source_progress=in.i32(),live_progress=in.i32();
    require(source_progress==progress(world),
            "BNW game supporting source religion cache changed");
    auto replacement=*world.supporting_classes;
    for(std::size_t i=0;i<base.barbarians.size();++i) {
        const auto old_counter=in.i16(),old_units=in.i16();
        const auto new_counter=in.i16(),new_units=in.i16();
        require(old_counter==base.barbarians[i].counter &&
                old_units==base.barbarians[i].units_spawned,
                "BNW barbarian source counters changed");
        replacement.barbarians[i].counter=new_counter;
        replacement.barbarians[i].units_spawned=new_units;
    }
    for(std::size_t i=0;i<base.goody_hut_results.size();++i)
        for(std::size_t j=0;j<3;++j) {
            const auto old_value=in.i32(),new_value=in.i32();
            require(old_value==base.goody_hut_results[i][j],
                    "BNW goody source history changed");
            replacement.goody_hut_results[i][j]=new_value;
        }
    require(in.at==section.bytes.size(),
            "Trailing BNW game supporting checkpoint bytes");
    world.supporting_classes=std::move(replacement);
    std::get<saves::Expansion2PrefixObservation>(
        world.game_state.prefix).scalars[23]=live_progress;
}
} // namespace civ5::runtime
