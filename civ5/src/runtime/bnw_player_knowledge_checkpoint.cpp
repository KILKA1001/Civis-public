#include "civ5/runtime/bnw_player_knowledge_checkpoint.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <climits>
#include <span>
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
constexpr std::array<std::uint8_t,8> magic{'C','I','V','B','K','N','W','1'};
constexpr std::array<std::string_view,4> blocks{
    "player_religions","religion_ai","player_techs","flavor_manager"};
constexpr std::uint64_t players=64,max_values=4'000'000,max_bytes=80'000'000;
void require(bool condition,const char *why) {
    if(!condition) throw std::runtime_error(why);
}
std::string_view name(const char *value) {
    return value?std::string_view(value):std::string_view();
}
void digest(std::string_view sha) {
    require(sha.size()==64 &&
            std::all_of(sha.begin(),sha.end(),[](char c) {
                return (c>='0'&&c<='9')||(c>='a'&&c<='f');
            }),"Invalid BNW player knowledge source/rules SHA-256");
}
bool structural(std::string_view key) {
    return key=="version" || key=="tech_ai_version" ||
        key.ends_with("_wire_count");
}
struct Writer {
    std::vector<std::uint8_t> bytes;
    void u32(std::uint32_t v) {
        for(unsigned n=0;n<4;++n)
            bytes.push_back(static_cast<std::uint8_t>(v>>(n*8)));
    }
    void u64(std::uint64_t v) {
        for(unsigned n=0;n<8;++n)
            bytes.push_back(static_cast<std::uint8_t>(v>>(n*8)));
    }
    void i64(std::int64_t v) {u64(std::bit_cast<std::uint64_t>(v));}
    void raw(std::string_view text) {
        bytes.insert(bytes.end(),text.begin(),text.end());
    }
    void string(std::string_view text) {
        require(!text.empty() && text.size()<=UINT32_MAX,
                "BNW player knowledge schema name unavailable");
        u32(static_cast<std::uint32_t>(text.size()));raw(text);
    }
};
struct Reader {
    std::span<const std::uint8_t> bytes;
    std::size_t at=0;
    std::uint32_t u32() {
        require(at<=bytes.size() && bytes.size()-at>=4,
                "Truncated BNW player knowledge section");
        std::uint32_t value=0;
        for(unsigned n=0;n<4;++n)
            value|=std::uint32_t(bytes[at++])<<(n*8);
        return value;
    }
    std::uint64_t u64() {
        require(at<=bytes.size() && bytes.size()-at>=8,
                "Truncated BNW player knowledge value");
        std::uint64_t value=0;
        for(unsigned n=0;n<8;++n)
            value|=std::uint64_t(bytes[at++])<<(n*8);
        return value;
    }
    std::int64_t i64() {return std::bit_cast<std::int64_t>(u64());}
    void hash(std::string_view expected) {
        require(at<=bytes.size() && bytes.size()-at>=expected.size() &&
                std::equal(expected.begin(),expected.end(),bytes.begin()+at),
                "BNW player knowledge source, rules, roster or schema changed");
        at+=expected.size();
    }
};
void shape(const saves::SavedSessionSnapshot &world) {
    require(world.game_state.layout==bnw && world.pregame &&
            world.pregame->layout==bnw && world.all_players &&
            world.all_players->layout==bnw &&
            world.all_players->context.player_count==players &&
            world.all_players->players.size()==players &&
            world.all_players->context.profile_extensions.values.empty(),
            "BNW player knowledge needs complete plain 64-player roster");
    const auto teams=world.pregame->archive.integer_arrays.find("team_types");
    require(teams!=world.pregame->archive.integer_arrays.end() &&
            teams->second.size()>=players,
            "BNW player knowledge team roster unavailable");
    for(std::size_t i=0;i<players;++i) {
        const auto &player=world.all_players->players[i];
        const auto &knowledge=player.knowledge;
        require(player.index==i &&
                player.saved_player_id==static_cast<std::int64_t>(i) &&
                teams->second[i]>=0 && teams->second[i]<64 &&
                knowledge.layout==bnw &&
                knowledge.profile_extensions.values.empty() &&
                knowledge.blocks.size()==blocks.size(),
                "BNW player knowledge ID, profile or block roster changed");
        for(std::size_t j=0;j<blocks.size();++j) {
            const auto &block=knowledge.blocks[j];
            require(name(block.name)==blocks[j],
                    "BNW player knowledge block order changed");
            for(const auto &field:block.fields) {
                const auto key=name(field.name);
                const bool version=key=="version" || key=="tech_ai_version";
                require((version && field.value==1) ||
                        (!version && field.value>=INT32_MIN &&
                         field.value<=INT32_MAX),
                        "BNW player knowledge scalar exceeds source wire range");
            }
            for(const auto &array:block.arrays) {
                require(array.columns==1 && array.values.size()<=max_values,
                        "BNW player knowledge array shape changed");
                for(const auto value:array.values)
                    require(value>=INT32_MIN && value<=INT32_MAX,
                            "BNW player knowledge array exceeds source i32");
            }
            for(const auto &array:block.hashes) {
                const bool weights=name(array.name)=="tech_ai_weights";
                require(array.wire_count>=(weights?INT32_MIN:0) &&
                        array.wire_count<=(weights?INT32_MAX:UINT32_MAX) &&
                        array.rows.size()==static_cast<std::uint64_t>(
                            std::max<std::int64_t>(array.wire_count,0)) &&
                        array.rows.size()<=max_values,
                        "BNW player knowledge hash count changed");
                for(const auto &row:array.rows) {
                    require(row.type_hash<=UINT32_MAX &&
                            row.values.size()==
                                static_cast<std::size_t>(weights || row.type_hash!=0),
                            "BNW player knowledge hash row shape changed");
                    for(const auto value:row.values)
                        require(value>=INT32_MIN && value<=INT32_MAX &&
                                (name(array.name)!="researching_tech" ||
                                 value==0 || value==1),
                                "BNW player knowledge hash value invalid");
                }
            }
        }
    }
}
std::string schema_hash(const saves::SavedSessionSnapshot &world) {
    shape(world);Writer out;
    out.u64(world.all_players->start_offset);
    out.u64(world.all_players->end_offset);
    const auto &teams=world.pregame->archive.integer_arrays.at("team_types");
    for(std::size_t i=0;i<players;++i) {
        const auto &player=world.all_players->players[i];
        const auto &knowledge=player.knowledge;
        out.u32(static_cast<std::uint32_t>(i));out.i64(teams[i]);
        out.u64(knowledge.start_offset);out.u64(knowledge.end_offset);
        out.u64(knowledge.blocks.size());
        for(const auto &block:knowledge.blocks) {
            out.string(name(block.name));
            out.u64(block.start_offset);out.u64(block.end_offset);
            out.u64(block.fields.size());
            for(const auto &field:block.fields) {
                const auto key=name(field.name);out.string(key);
                if(structural(key)) out.i64(field.value);
            }
            out.u64(block.arrays.size());
            for(const auto &array:block.arrays) {
                out.string(name(array.name));out.u64(array.columns);
                out.u64(array.values.size());
            }
            out.u64(block.hashes.size());
            for(const auto &array:block.hashes) {
                out.string(name(array.name));out.i64(array.wire_count);
                out.u64(array.rows.size());
                for(const auto &row:array.rows) {
                    out.u64(row.type_hash);out.u64(row.values.size());
                }
            }
        }
    }
    return identify_bytes(out.bytes).sha256;
}
std::vector<std::int64_t> values(const saves::SavedSessionSnapshot &world) {
    shape(world);
    std::vector<std::int64_t> result;
    for(const auto &player:world.all_players->players)
        for(const auto &block:player.knowledge.blocks) {
            for(const auto &field:block.fields) result.push_back(field.value);
            for(const auto &array:block.arrays)
                result.insert(result.end(),array.values.begin(),array.values.end());
            for(const auto &array:block.hashes)
                for(const auto &row:array.rows)
                    result.insert(result.end(),row.values.begin(),row.values.end());
            require(result.size()<=max_values,
                    "BNW player knowledge value budget exceeded");
        }
    return result;
}
void assign_values(saves::SavedSessionSnapshot &world,
                   const std::vector<std::int64_t> &replacement) {
    std::size_t at=0;
    for(auto &player:world.all_players->players)
        for(auto &block:player.knowledge.blocks) {
            for(auto &field:block.fields) field.value=replacement[at++];
            for(auto &array:block.arrays)
                for(auto &value:array.values) value=replacement[at++];
            for(auto &array:block.hashes)
                for(auto &row:array.rows)
                    for(auto &value:row.values) value=replacement[at++];
        }
    require(at==replacement.size(),
            "BNW player knowledge replacement length changed");
}
} // namespace

void validate_bnw_player_knowledge_composition(
    std::span<const saves::CivisSaveSection> sections) {
    std::size_t whole=0,research_queue=0;
    for(const auto &section:sections) {
        whole+=section.id==bnw_player_knowledge_checkpoint_section_id;
        research_queue+=section.id==33; // BNW queue/flag raw owner.
    }
    require(whole<=1 && research_queue<=1 && !(whole && research_queue),
            "BNW full player knowledge and ID33 research flags overlap");
}

saves::CivisSaveSection make_bnw_player_knowledge_checkpoint(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity) {
    digest(identity.payload_sha256);digest(identity.rules_sha256);
    const auto schema=schema_hash(source);
    require(schema_hash(world)==schema,
            "BNW player knowledge structure or roster changed");
    const auto before=values(source),after=values(world);
    require(before.size()==after.size() && before!=after &&
            before.size()<=(max_bytes-(8+4+64*3+8))/16,
            "BNW player knowledge unchanged or exceeds section budget");
    Writer out;
    out.bytes.insert(out.bytes.end(),magic.begin(),magic.end());out.u32(1);
    out.raw(identity.payload_sha256);out.raw(identity.rules_sha256);
    out.raw(schema);out.u64(before.size());
    for(std::size_t i=0;i<before.size();++i) {
        out.i64(before[i]);out.i64(after[i]);
    }
    return {bnw_player_knowledge_checkpoint_section_id,0,std::move(out.bytes)};
}

saves::SavedSessionSnapshot restore_bnw_player_knowledge_checkpoint(
    const saves::CivisSaveSection &section,
    const saves::SavedSessionSnapshot &world_before,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity) {
    digest(identity.payload_sha256);digest(identity.rules_sha256);
    require(section.id==bnw_player_knowledge_checkpoint_section_id &&
            section.flags==0 && section.bytes.size()>=8+4+64*3+8 &&
            section.bytes.size()<=max_bytes &&
            std::equal(magic.begin(),magic.end(),section.bytes.begin()),
            "Unsupported BNW player knowledge section");
    const auto schema=schema_hash(source);
    require(schema_hash(world_before)==schema,
            "BNW player knowledge destination structure changed");
    Reader in{section.bytes,magic.size()};
    require(in.u32()==1,"Unsupported BNW player knowledge version");
    in.hash(identity.payload_sha256);in.hash(identity.rules_sha256);
    in.hash(schema);
    const auto count=in.u64();
    require(count<=max_values &&
            count==(section.bytes.size()-in.at)/16 &&
            (section.bytes.size()-in.at)%16==0,
            "BNW player knowledge value count or section length changed");
    const auto source_values=values(source);
    require(source_values.size()==count && values(world_before)==source_values,
            "BNW player knowledge destination is not source baseline");
    std::vector<std::int64_t> replacement;
    replacement.reserve(static_cast<std::size_t>(count));
    bool changed=false;
    for(std::size_t i=0;i<count;++i) {
        const auto old=in.i64(),next=in.i64();
        require(old==source_values[i],
                "BNW player knowledge source value changed");
        replacement.push_back(next);changed|=old!=next;
    }
    require(in.at==section.bytes.size() && changed,
            "BNW player knowledge section incomplete or unchanged");
    auto result=world_before;
    assign_values(result,replacement);
    require(schema_hash(result)==schema,
            "BNW player knowledge version or dimension changed");
    return result;
}

} // namespace civ5::runtime
