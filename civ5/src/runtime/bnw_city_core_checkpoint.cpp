#include "civ5/runtime/bnw_city_core_checkpoint.hpp"
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
constexpr std::array<std::uint8_t,8> magic{'C','I','V','B','C','C','R','1'};
constexpr std::size_t players=64,max_values=1'000'000,max_bytes=16'000'000;
void require(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
std::string_view name(const char *value) {
    return value?std::string_view(value):std::string_view();
}
void hash_shape(std::string_view hash) {
    require(hash.size()==64 && std::all_of(hash.begin(),hash.end(),[](char c){
        return (c>='0'&&c<='9')||(c>='a'&&c<='f');
    }),"Invalid BNW city core checkpoint SHA-256");
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
        require(value>=INT32_MIN && value<=INT32_MAX,"BNW city object ID exceeds i32");
        u32(static_cast<std::uint32_t>(static_cast<std::int32_t>(value)));
    }
    void i64(std::int64_t value) {u64(std::bit_cast<std::uint64_t>(value));}
    void raw(std::string_view value) {bytes.insert(bytes.end(),value.begin(),value.end());}
    void string(std::string_view value) {
        require(!value.empty() && value.size()<=UINT32_MAX,
                "BNW city schema name empty or too long");
        u32(static_cast<std::uint32_t>(value.size()));raw(value);
    }
};
struct Reader {
    std::span<const std::uint8_t> bytes;
    std::size_t at=0;
    std::uint32_t u32() {
        require(at<=bytes.size() && bytes.size()-at>=4,"Truncated BNW city core section");
        std::uint32_t value=0;
        for(unsigned i=0;i<4;++i) value|=std::uint32_t(bytes[at++])<<(8*i);
        return value;
    }
    std::uint64_t u64() {
        require(at<=bytes.size() && bytes.size()-at>=8,"Truncated BNW city core section");
        std::uint64_t value=0;
        for(unsigned i=0;i<8;++i) value|=std::uint64_t(bytes[at++])<<(8*i);
        return value;
    }
    std::int64_t i64() {return std::bit_cast<std::int64_t>(u64());}
    std::uint8_t u8() {
        require(at<bytes.size(),"Truncated BNW city core byte payload");
        return bytes[at++];
    }
    void hash(std::string_view expected) {
        require(at<=bytes.size() && bytes.size()-at>=expected.size() &&
                std::equal(expected.begin(),expected.end(),bytes.begin()+at),
                "BNW city core source, rules, roster, or schema changed");
        at+=expected.size();
    }
};
std::int64_t scalar(const saves::CityCoreObservation &core,
                    std::string_view wanted) {
    const saves::CityCoreField *found=nullptr;
    for(const auto &field:core.fields)
        if(name(field.name)==wanted) {
            require(!found,"Duplicate BNW city identity field");found=&field;
        }
    require(found && found->encoding==saves::CityCoreEncoding::i32 &&
            found->values.size()==1,"BNW city identity field unavailable");
    return found->values[0];
}
void shape(const saves::SavedSessionSnapshot &world) {
    require(world.game_state.layout==bnw && world.all_players &&
            world.all_players->layout==bnw &&
            world.all_players->context.player_count==players &&
            world.all_players->players.size()==players &&
            world.all_players->context.profile_extensions.values.empty(),
            "BNW city core needs complete plain player roster");
}
std::string roster_hash(const saves::SavedSessionSnapshot &world) {
    shape(world);Writer out;
    for(std::size_t i=0;i<players;++i) {
        const auto &player=world.all_players->players[i];
        const auto &cities=player.cities;
        require(player.index==i && player.saved_player_id==static_cast<std::int64_t>(i) &&
                cities.layout==bnw && cities.context.profile_extensions.values.empty() &&
                cities.header.layout==bnw && cities.header.wire_city_count>=0 &&
                static_cast<std::uint64_t>(cities.header.wire_city_count)==
                    cities.cities.size(),
                "BNW city owner or collection count changed");
        out.u32(static_cast<std::uint32_t>(i));
        out.u64(cities.cities.size());
        for(std::size_t j=0;j<cities.cities.size();++j) {
            const auto &city=cities.cities[j];
            require(city.index==j && city.core.layout==bnw &&
                    scalar(city.core,"m_eOwner")==static_cast<std::int64_t>(i),
                    "BNW city order or owner changed");
            out.i32(scalar(city.core,"m_iID"));
            out.i32(scalar(city.core,"m_eOwner"));
            out.i32(scalar(city.core,"m_iX"));
            out.i32(scalar(city.core,"m_iY"));
        }
    }
    return identify_bytes(out.bytes).sha256;
}
void field_layout(const saves::CityCoreField &field) {
    using E=saves::CityCoreEncoding;
    const auto count=field.wire_count.value_or(0);
    require(count<=UINT32_MAX && field.bytes.size()<=max_bytes &&
            field.values.size()<=max_values &&
            static_cast<unsigned>(field.encoding)<=static_cast<unsigned>(E::hashed_i32),
            "Unknown BNW city core encoding or size");
    for(const auto n:field.row_counts)
        require(n<=UINT32_MAX,"BNW city nested row exceeds u32 wire range");
    for(const auto h:field.hashes)
        require(h<=UINT32_MAX,"BNW city hash exceeds u32 wire range");
    if(field.encoding==E::string32) {
        require(field.wire_count && count==field.bytes.size() &&
                field.values.empty() && field.row_keys.empty() &&
                field.row_counts.empty() && field.hashes.empty(),
                "BNW city string shape changed");return;
    }
    require(field.bytes.empty(),"BNW city non-string has byte payload");
    const bool no_rows=field.row_keys.empty() && field.row_counts.empty() &&
                       field.hashes.empty();
    switch(field.encoding) {
    case E::i32:case E::hash32:case E::boolean:
        require(!field.wire_count && no_rows && field.values.size()==1,
                "BNW city scalar shape changed");break;
    case E::fraction:
        require(!field.wire_count && no_rows && field.values.size()==2,
                "BNW city fraction shape changed");break;
    case E::vector_i32:case E::vector_bool:case E::set_i32:
        require(field.wire_count && no_rows && count==field.values.size(),
                "BNW city vector shape changed");break;
    case E::vector_fraction:case E::map_ii:
        require(field.wire_count && no_rows && count<=max_values/2 &&
                field.values.size()==count*2,
                "BNW city pair-row shape changed");break;
    case E::nested_map_i_iii: {
        require(field.wire_count && field.hashes.empty() &&
                count==field.row_keys.size() && count==field.row_counts.size(),
                "BNW city nested map keys/count changed");
        std::uint64_t total=0;
        for(const auto n:field.row_counts) {
            require(total<=field.values.size() &&
                    n<=(field.values.size()-total)/3,
                    "BNW city nested map row exceeds values");
            total+=n*3;
        }
        require(total==field.values.size(),
                "BNW city nested map values disagree with rows");break;
    }
    case E::hashed_i32: {
        require(field.wire_count && field.row_keys.empty() &&
                field.row_counts.empty() && count==field.hashes.size(),
                "BNW city hashed row shape changed");
        std::size_t present=0;
        for(const auto h:field.hashes) if(h) ++present;
        require(present==field.values.size(),
                "BNW city hashed values disagree with keys");break;
    }
    case E::string32:break;
    }
}
std::string schema_hash(const saves::SavedSessionSnapshot &world) {
    shape(world);Writer out;
    for(const auto &player:world.all_players->players)
        for(const auto &city:player.cities.cities) {
            const auto &core=city.core;
            require(core.version && *core.version<=6 &&
                    !core.fields.empty(),"Unknown BNW city core version");
            out.u64(core.start_offset);out.u64(core.end_offset);
            out.u64(*core.version);out.u64(core.fields.size());
            for(const auto &field:core.fields) {
                field_layout(field);out.string(name(field.name));
                out.u32(static_cast<std::uint32_t>(field.encoding));
                out.u64(field.start_offset);out.u64(field.end_offset);
                out.u32(field.wire_count.has_value()?1:0);
                if(field.wire_count) out.u64(*field.wire_count);
                out.u64(field.values.size());out.u64(field.bytes.size());
                out.u64(field.row_keys.size());
                for(const auto key:field.row_keys) out.i32(key);
                out.u64(field.row_counts.size());
                for(const auto count:field.row_counts) out.u64(count);
                out.u64(field.hashes.size());
                for(const auto hash:field.hashes) out.u64(hash);
            }
        }
    return identify_bytes(out.bytes).sha256;
}
void value_shape(const saves::CityCoreField &field,std::int64_t value) {
    using E=saves::CityCoreEncoding;
    if(field.encoding==E::boolean || field.encoding==E::vector_bool)
        require(value==0 || value==1,"BNW city boolean is invalid");
    else if(field.encoding==E::hash32)
        require(value>=0 && value<=UINT32_MAX,"BNW city hash exceeds u32");
    else
        require(value>=INT32_MIN && value<=INT32_MAX,"BNW city value exceeds i32");
}
template<class World,class Fn> void walk_values(World &world,Fn &&visit) {
    for(auto &player:world.all_players->players)
        for(auto &city:player.cities.cities)
            for(auto &field:city.core.fields)
                for(auto &value:field.values) visit(field,value);
}
template<class World,class Fn> void walk_bytes(World &world,Fn &&visit) {
    for(auto &player:world.all_players->players)
        for(auto &city:player.cities.cities)
            for(auto &field:city.core.fields)
                for(auto &value:field.bytes) visit(value);
}
std::string core_payload_hash(const saves::SavedSessionSnapshot &world) {
    shape(world);Writer out;std::uint64_t values=0,bytes=0;
    walk_values(world,[&](const auto &field,const auto &value){
        require(values<max_values,"BNW baseline core value budget exceeded");
        value_shape(field,value);++values;out.i64(value);
    });
    // Count separates the numeric stream from the following byte stream.
    out.u64(values);
    walk_bytes(world,[&](const auto &value){
        require(bytes<max_bytes,"BNW baseline core byte budget exceeded");
        ++bytes;out.bytes.push_back(value);
    });
    out.u64(bytes);return identify_bytes(out.bytes).sha256;
}
// The city core is only one of four serialized trees. An unchanged shape in
// production/management/tail is insufficient: reject any mutation there until
// those trees have their own complete checkpoints.
void write_record(Writer &out,const saves::CityProductionRecord &record) {
    out.string(name(record.name));out.u64(record.index);
    out.u64(record.start_offset);out.u64(record.end_offset);
    out.u64(record.fields.size());
    for(const auto &field:record.fields) {
        out.string(name(field.name));out.i64(field.value);
    }
    out.u64(record.arrays.size());
    for(const auto &array:record.arrays) {
        out.string(name(array.name));out.u64(array.columns);
        out.u64(array.values.size());
        for(const auto value:array.values) out.i64(value);
    }
    out.u64(record.blobs.size());
    for(const auto &blob:record.blobs) {
        out.string(name(blob.name));out.u64(blob.bytes.size());
        out.bytes.insert(out.bytes.end(),blob.bytes.begin(),blob.bytes.end());
    }
    out.u64(record.children.size());
    for(const auto &child:record.children) write_record(out,child);
}
void write_node(Writer &out,const saves::CityTailNode &node) {
    out.string(name(node.name));out.string(name(node.encoding));
    out.u64(node.start_offset);out.u64(node.end_offset);
    out.u32(node.wire_count.has_value()?1:0);
    if(node.wire_count) out.i64(*node.wire_count);
    out.u64(node.values.size());
    for(const auto value:node.values) out.i64(value);
    out.u64(node.children.size());
    for(const auto &child:node.children) write_node(out,child);
}
std::string other_city_hash(const saves::SavedSessionSnapshot &world) {
    shape(world);Writer out;
    for(const auto &player:world.all_players->players) {
        const auto &cities=player.cities;
        out.u64(cities.start_offset);out.u64(cities.end_offset);
        out.u32(cities.context.bnw_projects.has_value()?1:0);
        if(cities.context.bnw_projects) out.u64(*cities.context.bnw_projects);
        out.u64(cities.retained_elements);out.u64(cities.retained_string_bytes);
        out.u64(cities.header.start_offset);out.u64(cities.header.end_offset);
        out.u64(cities.header.source_slots);out.i64(cities.header.wire_city_count);
        out.u64(cities.header.fields.size());
        for(const auto &field:cities.header.fields) {
            out.string(name(field.name));out.i64(field.value);
        }
        out.u64(cities.header.next_free_indices.size());
        for(const auto value:cities.header.next_free_indices) out.i64(value);
        for(const auto &city:cities.cities) {
            const auto &production=city.production;
            const auto &management=city.management;
            const auto &tail=city.tail;
            require(production.layout==bnw && management.layout==bnw &&
                    tail.layout==bnw &&
                    production.context.profile_extensions.values.empty() &&
                    management.context.profile_extensions.values.empty() &&
                    tail.context.profile_extensions.values.empty(),
                    "Unknown BNW city companion profile");
            out.u64(production.start_offset);out.u64(production.end_offset);
            out.u32(production.context.bnw_city_version.has_value()?1:0);
            if(production.context.bnw_city_version)
                out.u64(*production.context.bnw_city_version);
            out.u64(production.blocks.size());
            for(const auto &record:production.blocks) write_record(out,record);
            out.u64(management.start_offset);out.u64(management.end_offset);
            out.u32(management.context.bnw_projects.has_value()?1:0);
            if(management.context.bnw_projects)
                out.u64(*management.context.bnw_projects);
            out.u64(management.city_plot_slots);out.u64(management.blocks.size());
            for(const auto &block:management.blocks) {
                out.string(name(block.name));
                out.u64(block.start_offset);out.u64(block.end_offset);
                out.u64(block.fields.size());
                for(const auto &field:block.fields) {
                    out.string(name(field.name));out.i64(field.value);
                }
                out.u64(block.arrays.size());
                for(const auto &array:block.arrays) {
                    out.string(name(array.name));out.u64(array.columns);
                    out.u64(array.values.size());
                    for(const auto value:array.values) out.i64(value);
                }
                out.u64(block.hashes.size());
                for(const auto &hash:block.hashes) {
                    out.string(name(hash.name));out.i64(hash.wire_count);
                    out.u64(hash.rows.size());
                    for(const auto &row:hash.rows) {
                        out.u64(row.type_hash);out.u64(row.values.size());
                        for(const auto value:row.values) out.i64(value);
                    }
                }
            }
            out.u64(tail.start_offset);out.u64(tail.end_offset);
            out.u32(tail.context.bnw_city_version.has_value()?1:0);
            if(tail.context.bnw_city_version)
                out.u64(*tail.context.bnw_city_version);
            out.u64(tail.blocks.size());
            for(const auto &node:tail.blocks) write_node(out,node);
        }
    }
    return identify_bytes(out.bytes).sha256;
}
} // namespace

saves::CivisSaveSection make_bnw_city_core_checkpoint(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity,
    bool allow_unchanged) {
    hash_shape(identity.payload_sha256);hash_shape(identity.rules_sha256);
    const auto roster=roster_hash(source),schema=schema_hash(source);
    const auto companions=other_city_hash(source);
    require(roster_hash(world)==roster && schema_hash(world)==schema &&
            other_city_hash(world)==companions,
            "BNW city roster, core structure or companion state changed");
    std::vector<std::int64_t> baseline;
    walk_values(source,[&](const auto &field,const auto &value){
        require(baseline.size()<max_values,"BNW city value budget exceeded");
        value_shape(field,value);baseline.push_back(value);
    });
    std::vector<std::uint8_t> old_bytes;
    walk_bytes(source,[&](const auto &value){
        require(old_bytes.size()<max_bytes,"BNW city byte budget exceeded");
        old_bytes.push_back(value);
    });
    Writer out;out.bytes.insert(out.bytes.end(),magic.begin(),magic.end());
    out.u32(1);
    out.raw(identity.payload_sha256);out.raw(identity.rules_sha256);
    out.raw(roster);out.raw(schema);out.raw(companions);
    out.u32(players);out.u64(baseline.size());out.u64(old_bytes.size());
    bool changed=false;std::size_t index=0;
    walk_values(world,[&](const auto &field,const auto &value){
        require(index<baseline.size(),"BNW city core value count changed");
        value_shape(field,value);changed|=value!=baseline[index++];out.i64(value);
    });
    require(index==baseline.size(),"BNW city core value count changed");
    index=0;
    walk_bytes(world,[&](const auto &value){
        require(index<old_bytes.size(),"BNW city core byte count changed");
        changed|=value!=old_bytes[index++];out.bytes.push_back(value);
    });
    require(index==old_bytes.size() && (changed || allow_unchanged),
            "BNW city core checkpoint unchanged or incomplete");
    if(!changed) out.bytes[8]=2;
    return {bnw_city_core_checkpoint_section_id,0,std::move(out.bytes)};
}

saves::CivisSaveSection make_bnw_city_core_baseline_checkpoint(
    const saves::SavedSessionSnapshot &world,const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity,bool allow_unchanged) {
    auto section=make_bnw_city_core_checkpoint(world,source,identity,allow_unchanged);
    const auto baseline=core_payload_hash(source);
    constexpr std::size_t offset=8+4+64*5;
    require(section.bytes.size()>=offset,"BNW city core legacy header missing");
    section.bytes.insert(section.bytes.begin()+offset,baseline.begin(),baseline.end());
    section.bytes[8]=section.bytes[8]==1?3:4;
    return section;
}

void restore_bnw_city_core_checkpoint(
    const saves::CivisSaveSection &section,
    saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity) {
    hash_shape(identity.payload_sha256);hash_shape(identity.rules_sha256);
    require(section.id==bnw_city_core_checkpoint_section_id &&
            section.flags==0 && section.bytes.size()>=8+4+64*5+4+8+8 &&
            std::equal(magic.begin(),magic.end(),section.bytes.begin()),
            "Unsupported BNW city core checkpoint section");
    Reader in{section.bytes,magic.size()};
    const auto version=in.u32();
    require(version==1 || version==2 || version==3 || version==4,
            "Unsupported BNW city core checkpoint version");
    in.hash(identity.payload_sha256);in.hash(identity.rules_sha256);
    const auto roster=roster_hash(source),schema=schema_hash(source);
    const auto companions=other_city_hash(source);
    in.hash(roster);in.hash(schema);in.hash(companions);
    if(version>=3) in.hash(core_payload_hash(source));
    require(in.u32()==players && roster_hash(world)==roster &&
            schema_hash(world)==schema && other_city_hash(world)==companions,
            "BNW city core destination roster, structure or companion changed");
    const auto values=in.u64(),bytes=in.u64();
    require(values<=max_values && bytes<=max_bytes &&
            values<=(section.bytes.size()-in.at)/8 &&
            bytes==section.bytes.size()-in.at-values*8,
            "BNW city core section counts or length changed");
    std::vector<std::int64_t> baseline;
    baseline.reserve(static_cast<std::size_t>(values));
    walk_values(source,[&](const auto &field,const auto &value){
        require(baseline.size()<max_values,"BNW city value budget exceeded");
        value_shape(field,value);baseline.push_back(value);
    });
    std::vector<std::uint8_t> old_bytes;
    old_bytes.reserve(static_cast<std::size_t>(bytes));
    walk_bytes(source,[&](const auto &value){
        require(old_bytes.size()<max_bytes,"BNW city byte budget exceeded");
        old_bytes.push_back(value);
    });
    require(baseline.size()==values && old_bytes.size()==bytes,
            "BNW city core source value count changed");
    std::vector<std::pair<std::int64_t*,std::int64_t>> value_writes;
    std::vector<std::pair<std::uint8_t*,std::uint8_t>> byte_writes;
    value_writes.reserve(static_cast<std::size_t>(values));
    byte_writes.reserve(static_cast<std::size_t>(bytes));
    bool changed=false;std::size_t index=0;
    walk_values(world,[&](const auto &field,auto &value){
        require(index<baseline.size() && value==baseline[index],
                "BNW city core destination is not source baseline");
        const auto next=in.i64();value_shape(field,next);
        changed|=next!=baseline[index++];value_writes.emplace_back(&value,next);
    });
    require(index==baseline.size(),"BNW city core destination value count changed");
    index=0;
    walk_bytes(world,[&](auto &value){
        require(index<old_bytes.size() && value==old_bytes[index],
                "BNW city core string destination is not source baseline");
        const auto next=in.u8();changed|=next!=old_bytes[index++];
        byte_writes.emplace_back(&value,next);
    });
    require(index==old_bytes.size() && changed==(version==1 || version==3) &&
            in.at==section.bytes.size(),
            "BNW city core section incomplete or unchanged");
    for(const auto &[target,value]:value_writes) *target=value;
    for(const auto &[target,value]:byte_writes) *target=value;
}
} // namespace civ5::runtime
