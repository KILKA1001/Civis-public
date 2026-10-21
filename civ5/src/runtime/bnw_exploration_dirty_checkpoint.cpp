#include "civ5/runtime/bnw_exploration_dirty_checkpoint.hpp"
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
constexpr std::array<std::uint8_t,8> magic{'C','I','V','B','E','X','P','1'};
constexpr std::size_t players=64;
constexpr std::size_t wire_size=8+4+64*3+4+players*12;
void require(bool ok,const char *message) {if(!ok) throw std::runtime_error(message);}
std::string_view name(const char *value) {
    return value?std::string_view(value):std::string_view();
}
void hash_shape(std::string_view hash) {
    require(hash.size()==64 && std::all_of(hash.begin(),hash.end(),[](char c){
        return (c>='0'&&c<='9')||(c>='a'&&c<='f');
    }),"Invalid BNW exploration checkpoint SHA-256");
}
struct Writer {
    std::vector<std::uint8_t> bytes;
    void u32(std::uint32_t value) {
        for(unsigned i=0;i<4;++i) bytes.push_back(static_cast<std::uint8_t>(value>>(8*i)));
    }
    void i32(std::int64_t value) {
        require(value>=INT32_MIN && value<=INT32_MAX,"BNW exploration roster value exceeds i32");
        u32(static_cast<std::uint32_t>(static_cast<std::int32_t>(value)));
    }
    void raw(std::string_view value) {bytes.insert(bytes.end(),value.begin(),value.end());}
};
struct Reader {
    std::span<const std::uint8_t> bytes;
    std::size_t at=0;
    std::uint32_t u32() {
        require(at<=bytes.size() && bytes.size()-at>=4,"Truncated BNW exploration checkpoint");
        std::uint32_t value=0;
        for(unsigned i=0;i<4;++i) value|=std::uint32_t(bytes[at++])<<(8*i);
        return value;
    }
    void hash(std::string_view expected) {
        require(at<=bytes.size() && bytes.size()-at>=expected.size() &&
                std::equal(expected.begin(),expected.end(),bytes.begin()+at),
                "BNW exploration source, rules, or roster identity changed");
        at+=expected.size();
    }
};
const saves::NamedSaveInteger &flag(const saves::SavedPlayerObservation &player) {
    const saves::PlayerStrategyBlock *economic=nullptr;
    for(const auto &block:player.strategy.blocks)
        if(name(block.name)=="economic_ai") {
            require(!economic,"Duplicate BNW EconomicAI block");economic=&block;
        }
    require(economic,"Missing BNW EconomicAI block");
    const saves::NamedSaveInteger *found=nullptr;
    for(const auto &field:economic->fields)
        if(name(field.name)=="exploration_plots_dirty") {
            require(!found,"Duplicate BNW exploration dirty flag");found=&field;
        }
    require(found && (found->value==0 || found->value==1),
            "BNW exploration dirty flag is unavailable or nonboolean");
    return *found;
}
saves::NamedSaveInteger &mutable_flag(saves::SavedPlayerObservation &player) {
    (void)flag(player);
    for(auto &block:player.strategy.blocks)
        if(name(block.name)=="economic_ai")
            for(auto &field:block.fields)
                if(name(field.name)=="exploration_plots_dirty") return field;
    throw std::runtime_error("Missing BNW exploration dirty flag");
}
std::int64_t alive(const saves::SavedPlayerObservation &player) {
    const saves::PlayerCoreField *found=nullptr;
    for(const auto &field:player.core.fields)
        if(name(field.name)=="m_bAlive") {
            require(!found,"Duplicate BNW alive field");found=&field;
        }
    require(found && found->encoding==saves::PlayerCoreEncoding::boolean &&
            found->values.size()==1 &&
            (found->values[0]==0 || found->values[0]==1),
            "BNW exploration player alive field unavailable");
    return found->values[0];
}
void shape(const saves::SavedSessionSnapshot &world) {
    require(world.game_state.layout==bnw && world.pregame &&
            world.pregame->layout==bnw && world.all_players &&
            world.all_players->layout==bnw &&
            world.all_players->context.player_count==players &&
            world.all_players->players.size()==players,
            "BNW exploration requires complete player roster");
}
std::string roster_hash(const saves::SavedSessionSnapshot &world) {
    shape(world);
    Writer out;
    const auto &mapping=world.pregame->archive.integer_arrays.find("team_types");
    require(mapping!=world.pregame->archive.integer_arrays.end() &&
            mapping->second.size()>=players,"BNW exploration team roster unavailable");
    for(std::size_t i=0;i<players;++i) {
        const auto &player=world.all_players->players[i];
        require(player.index==i && player.saved_player_id==static_cast<std::int64_t>(i) &&
                player.core.layout==bnw && player.strategy.layout==bnw &&
                mapping->second[i]>=0 && mapping->second[i]<64,
                "BNW exploration player ID, team, or profile changed");
        out.u32(static_cast<std::uint32_t>(i));
        out.i32(mapping->second[i]);
        out.i32(alive(player));
        (void)flag(player);
    }
    return identify_bytes(out.bytes).sha256;
}
void same_strategy_except_flags(const saves::SavedSessionSnapshot &world,
                                const saves::SavedSessionSnapshot &source) {
    shape(world);shape(source);
    const auto &x=*world.all_players,&y=*source.all_players;
    require(x.start_offset==y.start_offset && x.end_offset==y.end_offset,
            "BNW exploration player collection changed");
    for(std::size_t i=0;i<players;++i) {
        const auto &a=x.players[i].strategy,&b=y.players[i].strategy;
        require(a.start_offset==b.start_offset && a.end_offset==b.end_offset &&
                a.dimensions.profile_extensions.values.empty() &&
                b.dimensions.profile_extensions.values.empty() &&
                a.dimensions.major==b.dimensions.major &&
                a.dimensions.civ_players==b.dimensions.civ_players &&
                a.dimensions.yields==b.dimensions.yields &&
                a.dimensions.grand==b.dimensions.grand &&
                a.blocks.size()==b.blocks.size(),
                "BNW exploration strategy shape changed");
        for(std::size_t j=0;j<a.blocks.size();++j) {
            const auto &p=a.blocks[j],&q=b.blocks[j];
            require(name(p.name)==name(q.name) &&
                    p.start_offset==q.start_offset && p.end_offset==q.end_offset &&
                    p.fields.size()==q.fields.size() &&
                    p.arrays.size()==q.arrays.size() && p.hashes.size()==q.hashes.size(),
                    "BNW exploration strategy block changed");
            for(std::size_t k=0;k<p.fields.size();++k) {
                const auto &f=p.fields[k],&g=q.fields[k];
                require(name(f.name)==name(g.name),"BNW exploration field order changed");
                if(!(name(p.name)=="economic_ai" &&
                     name(f.name)=="exploration_plots_dirty"))
                    require(f.value==g.value,
                            "BNW exploration has unsupported strategy scalar mutation");
            }
            for(std::size_t k=0;k<p.arrays.size();++k) {
                const auto &f=p.arrays[k],&g=q.arrays[k];
                require(name(f.name)==name(g.name) && f.columns==g.columns &&
                        f.values==g.values,
                        "BNW exploration has unsupported strategy array mutation");
            }
            for(std::size_t k=0;k<p.hashes.size();++k) {
                const auto &f=p.hashes[k],&g=q.hashes[k];
                require(name(f.name)==name(g.name) && f.wire_count==g.wire_count &&
                        f.rows.size()==g.rows.size(),
                        "BNW exploration strategy hash shape changed");
                for(std::size_t n=0;n<f.rows.size();++n)
                    require(f.rows[n].type_hash==g.rows[n].type_hash &&
                            f.rows[n].values==g.rows[n].values,
                            "BNW exploration has unsupported strategy hash mutation");
            }
        }
    }
}
} // namespace

saves::CivisSaveSection make_bnw_exploration_dirty_checkpoint(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity) {
    hash_shape(identity.payload_sha256);hash_shape(identity.rules_sha256);
    const auto source_roster=roster_hash(source);
    require(roster_hash(world)==source_roster,"BNW exploration roster changed");
    same_strategy_except_flags(world,source);
    Writer out;
    out.bytes.insert(out.bytes.end(),magic.begin(),magic.end());
    out.u32(1);out.raw(identity.payload_sha256);out.raw(identity.rules_sha256);
    out.raw(source_roster);out.u32(players);
    bool changed=false;
    for(std::size_t i=0;i<players;++i) {
        const auto before=flag(source.all_players->players[i]).value;
        const auto after=flag(world.all_players->players[i]).value;
        require(after>=before,"BNW exploration dirty flag cannot clear on reveal");
        require(after==before || alive(source.all_players->players[i])==1,
                "BNW exploration cannot dirty a dead player");
        changed|=after!=before;
        out.u32(static_cast<std::uint32_t>(i));
        out.u32(static_cast<std::uint32_t>(before));
        out.u32(static_cast<std::uint32_t>(after));
    }
    require(changed,"BNW exploration checkpoint contains no changed flags");
    require(out.bytes.size()==wire_size,"BNW exploration checkpoint wire size changed");
    return {bnw_exploration_dirty_checkpoint_section_id,0,std::move(out.bytes)};
}

void restore_bnw_exploration_dirty_checkpoint(
    const saves::CivisSaveSection &section,
    saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity) {
    hash_shape(identity.payload_sha256);hash_shape(identity.rules_sha256);
    require(section.id==bnw_exploration_dirty_checkpoint_section_id &&
            section.flags==0 && section.bytes.size()==wire_size &&
            std::equal(magic.begin(),magic.end(),section.bytes.begin()),
            "Unsupported BNW exploration checkpoint section");
    Reader in{section.bytes,magic.size()};
    require(in.u32()==1,"Unsupported BNW exploration checkpoint version");
    in.hash(identity.payload_sha256);in.hash(identity.rules_sha256);
    const auto source_roster=roster_hash(source);
    in.hash(source_roster);
    require(in.u32()==players && roster_hash(world)==source_roster,
            "BNW exploration player count or roster changed");
    same_strategy_except_flags(world,source);
    std::array<std::uint32_t,players> values{};
    bool changed=false;
    for(std::size_t i=0;i<players;++i) {
        const auto id=in.u32(),before=in.u32(),after=in.u32();
        require(id==i && before<=1 && after<=1 && after>=before &&
                flag(source.all_players->players[i]).value==before &&
                flag(world.all_players->players[i]).value==before &&
                (after==before || alive(source.all_players->players[i])==1),
                "BNW exploration flag, player ID, or source state changed");
        changed|=before!=after;values[i]=after;
    }
    require(changed && in.at==section.bytes.size(),
            "BNW exploration checkpoint has no change or trailing bytes");
    // All validation and allocations finish before these nonthrowing writes.
    for(std::size_t i=0;i<players;++i)
        mutable_flag(world.all_players->players[i]).value=values[i];
}
} // namespace civ5::runtime
