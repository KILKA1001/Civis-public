#include "civ5/runtime/bnw_player_core_checkpoint.hpp"
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
constexpr std::array<std::uint8_t,8> magic{'C','I','V','B','P','C','R','1'};
constexpr std::size_t players=64,max_values=1'000'000,max_bytes=16'000'000;
void require(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
std::string_view name(const char *value) {
    return value?std::string_view(value):std::string_view();
}
void hash_shape(std::string_view hash) {
    require(hash.size()==64 && std::all_of(hash.begin(),hash.end(),[](char c){
        return (c>='0'&&c<='9')||(c>='a'&&c<='f');
    }),"Invalid BNW player core checkpoint SHA-256");
}
struct Writer {
    std::vector<std::uint8_t> bytes;
    void u32(std::uint32_t value) {
        for(unsigned i=0;i<4;++i) bytes.push_back(static_cast<std::uint8_t>(value>>(8*i)));
    }
    void u64(std::uint64_t value) {
        for(unsigned i=0;i<8;++i) bytes.push_back(static_cast<std::uint8_t>(value>>(8*i)));
    }
    void i32(std::int64_t value) {
        require(value>=INT32_MIN && value<=INT32_MAX,"BNW player object ID exceeds i32");
        u32(static_cast<std::uint32_t>(static_cast<std::int32_t>(value)));
    }
    void i64(std::int64_t value) {u64(std::bit_cast<std::uint64_t>(value));}
    void raw(std::string_view value) {bytes.insert(bytes.end(),value.begin(),value.end());}
    void string(std::string_view value) {
        require(!value.empty() && value.size()<=UINT32_MAX,
                "BNW player core schema name is empty or too long");
        u32(static_cast<std::uint32_t>(value.size()));raw(value);
    }
};
struct Reader {
    std::span<const std::uint8_t> bytes;
    std::size_t at=0;
    std::uint32_t u32() {
        require(at<=bytes.size() && bytes.size()-at>=4,"Truncated BNW player core section");
        std::uint32_t value=0;
        for(unsigned i=0;i<4;++i) value|=std::uint32_t(bytes[at++])<<(8*i);
        return value;
    }
    std::uint64_t u64() {
        require(at<=bytes.size() && bytes.size()-at>=8,"Truncated BNW player core section");
        std::uint64_t value=0;
        for(unsigned i=0;i<8;++i) value|=std::uint64_t(bytes[at++])<<(8*i);
        return value;
    }
    std::int64_t i64() {return std::bit_cast<std::int64_t>(u64());}
    std::uint8_t u8() {
        require(at<bytes.size(),"Truncated BNW player core byte payload");
        return bytes[at++];
    }
    void hash(std::string_view expected) {
        require(at<=bytes.size() && bytes.size()-at>=expected.size() &&
                std::equal(expected.begin(),expected.end(),bytes.begin()+at),
                "BNW player core source, rules, roster, or schema changed");
        at+=expected.size();
    }
};
template<class Core> std::int64_t scalar(const Core &core,std::string_view wanted) {
    const typename decltype(core.fields)::value_type *found=nullptr;
    for(const auto &field:core.fields)
        if(name(field.name)==wanted) {
            require(!found,"Duplicate BNW player object ID");found=&field;
        }
    require(found && found->values.size()==1,"Missing BNW player object ID");
    return found->values[0];
}
void shape(const saves::SavedSessionSnapshot &world) {
    require(world.game_state.layout==bnw && world.pregame &&
            world.pregame->layout==bnw && world.all_players &&
            world.all_players->layout==bnw &&
            world.all_players->context.player_count==players &&
            world.all_players->players.size()==players &&
            world.all_players->context.profile_extensions.values.empty(),
            "BNW player core requires complete plain 64-player roster");
}
std::string roster_hash(const saves::SavedSessionSnapshot &world) {
    shape(world);
    const auto teams=world.pregame->archive.integer_arrays.find("team_types");
    require(teams!=world.pregame->archive.integer_arrays.end() &&
            teams->second.size()>=players,"BNW player core team roster unavailable");
    Writer out;
    for(std::size_t i=0;i<players;++i) {
        const auto &player=world.all_players->players[i];
        require(player.index==i && player.saved_player_id==static_cast<std::int64_t>(i) &&
                player.core.layout==bnw && player.cities.layout==bnw &&
                player.units.layout==bnw &&
                scalar(player.core,"m_eID")==static_cast<std::int64_t>(i) &&
                player.cities.header.wire_city_count>=0 &&
                static_cast<std::uint64_t>(player.cities.header.wire_city_count)==
                    player.cities.cities.size() &&
                player.units.header.wire_unit_count>=0 &&
                static_cast<std::uint64_t>(player.units.header.wire_unit_count)==
                    player.units.units.size() &&
                teams->second[i]>=0 && teams->second[i]<64,
                "BNW player core object roster or team changed");
        out.u32(static_cast<std::uint32_t>(i));out.i32(teams->second[i]);
        out.u64(player.cities.cities.size());
        for(std::size_t j=0;j<player.cities.cities.size();++j) {
            const auto &city=player.cities.cities[j];
            require(city.index==j && city.core.layout==bnw,
                    "BNW player core city order changed");
            out.i32(scalar(city.core,"m_iID"));
        }
        out.u64(player.units.units.size());
        for(std::size_t j=0;j<player.units.units.size();++j) {
            const auto &unit=player.units.units[j];
            require(unit.index==j && unit.core.layout==bnw &&
                    scalar(unit.core,"m_eOwner")==static_cast<std::int64_t>(i),
                    "BNW player core unit order or owner changed");
            out.i32(scalar(unit.core,"m_iID"));
        }
    }
    return identify_bytes(out.bytes).sha256;
}
void field_layout(const saves::PlayerCoreField &field,std::uint64_t yields) {
    using E=saves::PlayerCoreEncoding;
    const auto count=field.wire_count.value_or(0);
    require(field.bytes.size()<=max_bytes && field.values.size()<=max_values &&
            count<=UINT32_MAX &&
            static_cast<unsigned>(field.encoding)<=static_cast<unsigned>(E::yield_rows),
            "Unknown BNW player core field encoding or size");
    for(const auto n:field.row_counts)
        require(n<=UINT32_MAX,"BNW player core nested row exceeds u32 wire range");
    for(const auto hash:field.hashes)
        require(hash<=UINT32_MAX,"BNW player core hash exceeds u32 wire range");
    if(field.encoding==E::string32) {
        require(field.wire_count && count==field.bytes.size() &&
                field.values.empty() && field.row_counts.empty() &&
                field.hashes.empty(),
                "BNW player core string wire count changed");
        return;
    }
    require(field.bytes.empty(),"BNW player core non-string has bytes");
    switch(field.encoding) {
    case E::i32:case E::u32:case E::i64:case E::boolean:
        require(!field.wire_count && field.values.size()==1 &&
                field.row_counts.empty() && field.hashes.empty(),
                "BNW player core scalar shape changed");break;
    case E::fraction:
        require(!field.wire_count && field.values.size()==2 &&
                field.row_counts.empty() && field.hashes.empty(),
                "BNW player core fraction shape changed");break;
    case E::vector_i32:case E::vector_bool:case E::vector_u8:case E::set_i32:
        require(field.wire_count && count==field.values.size() &&
                field.row_counts.empty() && field.hashes.empty(),
                "BNW player core vector count changed");break;
    case E::nested_i32: {
        require(field.wire_count && count==field.row_counts.size() &&
                field.hashes.empty(),
                "BNW player core nested row count changed");
        std::uint64_t total=0;
        for(const auto n:field.row_counts) {
            require(total<=field.values.size() &&
                    n<=field.values.size()-total,
                    "BNW player core nested row exceeds values");
            total+=n;
        }
        require(total==field.values.size(),
                "BNW player core nested values disagree with rows");
        break;
    }
    case E::options:
        require(field.wire_count && count<=max_values/2 &&
                field.values.size()==count*2 &&
                field.row_counts.empty() && field.hashes.empty(),
                "BNW player core option row count changed");break;
    case E::yield_rows:
        require(field.wire_count && count<=max_values/yields &&
                field.values.size()==count*yields &&
                field.row_counts.empty() && field.hashes.empty(),
                "BNW player core yield row count changed");break;
    case E::hashed_i32: {
        require(field.wire_count && count==field.hashes.size() &&
                field.row_counts.empty(),
                "BNW player core hashed row count changed");
        std::size_t present=0;
        for(const auto hash:field.hashes) if(hash) ++present;
        require(present==field.values.size(),
                "BNW player core hashed values disagree with hashes");
        break;
    }
    case E::string32:break;
    }
}
std::string schema_hash(const saves::SavedSessionSnapshot &world) {
    shape(world);
    Writer out;
    const auto &all=*world.all_players;
    out.u64(all.start_offset);out.u64(all.end_offset);
    for(const auto &player:all.players) {
        const auto &core=player.core;
        require(core.version && *core.version>=1 && *core.version<=16 &&
                core.yield_slots==6 && !core.fields.empty(),
                "Unknown BNW player core version or yield dimension");
        out.u64(core.start_offset);out.u64(core.end_offset);
        out.u64(*core.version);out.u64(core.yield_slots);
        out.u64(core.fields.size());
        for(const auto &field:core.fields) {
            field_layout(field,core.yield_slots);
            out.string(name(field.name));
            out.u32(static_cast<std::uint32_t>(field.encoding));
            out.u64(field.start_offset);out.u64(field.end_offset);
            out.u32(field.wire_count.has_value()?1:0);
            if(field.wire_count) out.u64(*field.wire_count);
            out.u64(field.values.size());out.u64(field.bytes.size());
            out.u64(field.row_counts.size());
            for(const auto n:field.row_counts) out.u64(n);
            out.u64(field.hashes.size());
            for(const auto hash:field.hashes) out.u64(hash);
        }
    }
    return identify_bytes(out.bytes).sha256;
}
void value_shape(const saves::PlayerCoreField &field,std::size_t index,
                 std::int64_t value) {
    using E=saves::PlayerCoreEncoding;
    switch(field.encoding) {
    case E::i64:break;
    case E::u32:
        require(value>=0 && value<=UINT32_MAX,
                "BNW player core u32 exceeds wire range");break;
    case E::boolean:case E::vector_bool:
        require(value==0 || value==1,"BNW player core bool is invalid");break;
    case E::vector_u8:
        require(value>=0 && value<=UINT8_MAX,
                "BNW player core u8 exceeds wire range");break;
    case E::options:
        require(value>=(index%2?INT32_MIN:0) &&
                value<=(index%2?INT32_MAX:static_cast<std::int64_t>(UINT32_MAX)),
                "BNW player core option exceeds wire range");break;
    default:
        require(value>=INT32_MIN && value<=INT32_MAX,
                "BNW player core i32 exceeds wire range");break;
    }
}
template<class World,class Fn> void walk_values(World &world,Fn &&visit) {
    for(auto &player:world.all_players->players)
        for(auto &field:player.core.fields) {
            for(std::size_t i=0;i<field.values.size();++i)
                visit(field,i,field.values[i]);
        }
}
template<class World,class Fn> void walk_bytes(World &world,Fn &&visit) {
    for(auto &player:world.all_players->players)
        for(auto &field:player.core.fields)
            for(auto &byte:field.bytes) visit(byte);
}
} // namespace

saves::CivisSaveSection make_bnw_player_core_checkpoint(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity,
    bool allow_unchanged) {
    hash_shape(identity.payload_sha256);hash_shape(identity.rules_sha256);
    const auto roster=roster_hash(source),schema=schema_hash(source);
    require(roster_hash(world)==roster && schema_hash(world)==schema,
            "BNW player core object roster or wire structure changed");
    std::vector<std::int64_t> baseline;
    walk_values(source,[&](const auto &field,std::size_t i,const auto &value){
        require(baseline.size()<max_values,"BNW player core value budget exceeded");
        value_shape(field,i,value);baseline.push_back(value);
    });
    std::vector<std::uint8_t> old_bytes;
    walk_bytes(source,[&](const auto &value){
        require(old_bytes.size()<max_bytes,"BNW player core byte budget exceeded");
        old_bytes.push_back(value);
    });
    Writer out;out.bytes.insert(out.bytes.end(),magic.begin(),magic.end());
    out.u32(1);
    out.raw(identity.payload_sha256);out.raw(identity.rules_sha256);
    out.raw(roster);out.raw(schema);out.u32(players);
    out.u64(baseline.size());out.u64(old_bytes.size());
    bool changed=false;std::size_t index=0;
    walk_values(world,[&](const auto &field,std::size_t i,const auto &value){
        require(index<baseline.size(),"BNW player core value count changed");
        value_shape(field,i,value);
        changed|=value!=baseline[index++];out.i64(value);
    });
    require(index==baseline.size(),"BNW player core value count changed");
    index=0;
    walk_bytes(world,[&](const auto &value){
        require(index<old_bytes.size(),"BNW player core string length changed");
        changed|=value!=old_bytes[index++];out.bytes.push_back(value);
    });
    require(index==old_bytes.size() && (changed || allow_unchanged),
            "BNW player core checkpoint unchanged or incomplete");
    if(!changed) out.bytes[8]=2;
    return {bnw_player_core_checkpoint_section_id,0,std::move(out.bytes)};
}

void restore_bnw_player_core_checkpoint(
    const saves::CivisSaveSection &section,
    saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity) {
    hash_shape(identity.payload_sha256);hash_shape(identity.rules_sha256);
    require(section.id==bnw_player_core_checkpoint_section_id &&
            section.flags==0 && section.bytes.size()>=8+4+64*4+4+8+8 &&
            std::equal(magic.begin(),magic.end(),section.bytes.begin()),
            "Unsupported BNW player core checkpoint section");
    Reader in{section.bytes,magic.size()};
    const auto version=in.u32();
    require(version==1 || version==2,
            "Unsupported BNW player core checkpoint version");
    in.hash(identity.payload_sha256);in.hash(identity.rules_sha256);
    const auto roster=roster_hash(source),schema=schema_hash(source);
    in.hash(roster);in.hash(schema);
    require(in.u32()==players && roster_hash(world)==roster &&
            schema_hash(world)==schema,
            "BNW player core destination roster or structure changed");
    const auto values=in.u64(),bytes=in.u64();
    require(values<=max_values && bytes<=max_bytes &&
            values<=(section.bytes.size()-in.at)/8 &&
            bytes==section.bytes.size()-in.at-values*8,
            "BNW player core section counts or length changed");
    std::vector<std::int64_t> baseline;
    baseline.reserve(static_cast<std::size_t>(values));
    walk_values(source,[&](const auto &field,std::size_t i,const auto &value){
        require(baseline.size()<max_values,"BNW player core value budget exceeded");
        value_shape(field,i,value);baseline.push_back(value);
    });
    std::vector<std::uint8_t> old_bytes;
    old_bytes.reserve(static_cast<std::size_t>(bytes));
    walk_bytes(source,[&](const auto &value){
        require(old_bytes.size()<max_bytes,"BNW player core byte budget exceeded");
        old_bytes.push_back(value);
    });
    require(baseline.size()==values && old_bytes.size()==bytes,
            "BNW player core source value count changed");
    std::vector<std::pair<std::int64_t*,std::int64_t>> value_writes;
    std::vector<std::pair<std::uint8_t*,std::uint8_t>> byte_writes;
    value_writes.reserve(static_cast<std::size_t>(values));
    byte_writes.reserve(static_cast<std::size_t>(bytes));
    bool changed=false;std::size_t index=0;
    walk_values(world,[&](const auto &field,std::size_t i,auto &value){
        require(index<baseline.size() && value==baseline[index],
                "BNW player core destination is not source baseline");
        const auto next=in.i64();value_shape(field,i,next);
        changed|=next!=baseline[index++];value_writes.emplace_back(&value,next);
    });
    require(index==baseline.size(),"BNW player core destination value count changed");
    index=0;
    walk_bytes(world,[&](auto &value){
        require(index<old_bytes.size() && value==old_bytes[index],
                "BNW player core string destination is not source baseline");
        const auto next=in.u8();changed|=next!=old_bytes[index++];
        byte_writes.emplace_back(&value,next);
    });
    require(index==old_bytes.size() && changed==(version==1) &&
            in.at==section.bytes.size(),
            "BNW player core section incomplete or unchanged");
    for(const auto &[target,value]:value_writes) *target=value;
    for(const auto &[target,value]:byte_writes) *target=value;
}
} // namespace civ5::runtime
