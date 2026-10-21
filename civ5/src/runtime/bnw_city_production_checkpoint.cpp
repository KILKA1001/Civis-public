#include "civ5/runtime/bnw_city_production_checkpoint.hpp"
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
constexpr std::array<std::uint8_t,8> magic{'C','I','V','B','C','P','R','1'};
constexpr std::array<std::string_view,4> blocks{
    "buildings","before_queue","queue","before_strategy"};
constexpr std::size_t players=64,max_values=1'000'000,max_bytes=16'000'000;
void require(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
std::string_view name(const char *value) {
    return value?std::string_view(value):std::string_view();
}
void hash_shape(std::string_view hash) {
    require(hash.size()==64 && std::all_of(hash.begin(),hash.end(),[](char c){
        return (c>='0'&&c<='9')||(c>='a'&&c<='f');
    }),"Invalid BNW city production checkpoint SHA-256");
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
        require(value>=INT32_MIN && value<=INT32_MAX,"BNW city production object ID exceeds i32");
        u32(static_cast<std::uint32_t>(static_cast<std::int32_t>(value)));
    }
    void i64(std::int64_t value) {u64(std::bit_cast<std::uint64_t>(value));}
    void raw(std::string_view value) {bytes.insert(bytes.end(),value.begin(),value.end());}
    void string(std::string_view value) {
        require(!value.empty() && value.size()<=UINT32_MAX,
                "BNW production schema name empty or too long");
        u32(static_cast<std::uint32_t>(value.size()));raw(value);
    }
};
struct Reader {
    std::span<const std::uint8_t> bytes;
    std::size_t at=0;
    std::uint32_t u32() {
        require(at<=bytes.size() && bytes.size()-at>=4,"Truncated BNW production section");
        std::uint32_t value=0;
        for(unsigned i=0;i<4;++i) value|=std::uint32_t(bytes[at++])<<(8*i);
        return value;
    }
    std::uint64_t u64() {
        require(at<=bytes.size() && bytes.size()-at>=8,"Truncated BNW production section");
        std::uint64_t value=0;
        for(unsigned i=0;i<8;++i) value|=std::uint64_t(bytes[at++])<<(8*i);
        return value;
    }
    std::int64_t i64() {return std::bit_cast<std::int64_t>(u64());}
    std::uint8_t u8() {
        require(at<bytes.size(),"Truncated BNW production byte payload");
        return bytes[at++];
    }
    void hash(std::string_view expected) {
        require(at<=bytes.size() && bytes.size()-at>=expected.size() &&
                std::equal(expected.begin(),expected.end(),bytes.begin()+at),
                "BNW production source, rules, roster, or schema changed");
        at+=expected.size();
    }
};
std::int64_t scalar(const saves::CityCoreObservation &core,std::string_view wanted) {
    const saves::CityCoreField *found=nullptr;
    for(const auto &field:core.fields)
        if(name(field.name)==wanted) {
            require(!found,"Duplicate BNW production city identity");found=&field;
        }
    require(found && found->encoding==saves::CityCoreEncoding::i32 &&
            found->values.size()==1,"BNW production city identity unavailable");
    return found->values[0];
}
void shape(const saves::SavedSessionSnapshot &world) {
    require(world.game_state.layout==bnw && world.all_players &&
            world.all_players->layout==bnw &&
            world.all_players->context.player_count==players &&
            world.all_players->players.size()==players &&
            world.all_players->context.profile_extensions.values.empty(),
            "BNW production needs complete plain player roster");
}
std::string roster_hash(const saves::SavedSessionSnapshot &world) {
    shape(world);Writer out;
    for(std::size_t p=0;p<players;++p) {
        const auto &player=world.all_players->players[p];
        const auto &cities=player.cities;
        require(player.index==p && player.saved_player_id==static_cast<std::int64_t>(p) &&
                cities.layout==bnw && cities.header.layout==bnw &&
                cities.context.profile_extensions.values.empty() &&
                cities.header.wire_city_count>=0 &&
                static_cast<std::uint64_t>(cities.header.wire_city_count)==
                    cities.cities.size(),
                "BNW production player or city count changed");
        out.u32(static_cast<std::uint32_t>(p));out.u64(cities.cities.size());
        for(std::size_t i=0;i<cities.cities.size();++i) {
            const auto &city=cities.cities[i];
            require(city.index==i && city.core.layout==bnw &&
                    scalar(city.core,"m_eOwner")==static_cast<std::int64_t>(p),
                    "BNW production city owner or order changed");
            out.i32(scalar(city.core,"m_iID"));
            out.i32(scalar(city.core,"m_eOwner"));
            out.i32(scalar(city.core,"m_iX"));out.i32(scalar(city.core,"m_iY"));
        }
    }
    return identify_bytes(out.bytes).sha256;
}
void record_schema(Writer &out,const saves::CityProductionRecord &record,
                   std::size_t depth=0) {
    require(depth<16,"BNW production record nesting exceeds SDK profile");
    out.string(name(record.name));out.u64(record.index);
    out.u64(record.start_offset);out.u64(record.end_offset);
    out.u64(record.fields.size());
    for(const auto &field:record.fields) {
        out.string(name(field.name));
        if(name(field.name)=="hash") {
            require(field.value>=0 && field.value<=UINT32_MAX,
                    "BNW production type hash exceeds u32");
            out.u32(static_cast<std::uint32_t>(field.value));
        }
    }
    out.u64(record.arrays.size());
    for(const auto &array:record.arrays) {
        out.string(name(array.name));out.u64(array.columns);
        out.u64(array.values.size());
        require(array.columns && array.values.size()%array.columns==0,
                "BNW production array row shape changed");
    }
    out.u64(record.blobs.size());
    for(const auto &blob:record.blobs) {
        out.string(name(blob.name));out.u64(blob.bytes.size());
    }
    out.u64(record.children.size());
    for(const auto &child:record.children) record_schema(out,child,depth+1);
}
std::string schema_hash(const saves::SavedSessionSnapshot &world) {
    shape(world);Writer out;
    for(const auto &player:world.all_players->players)
        for(const auto &city:player.cities.cities) {
            const auto &production=city.production;
            require(production.layout==bnw &&
                    production.context.profile_extensions.values.empty() &&
                    production.context.bnw_city_version==city.core.version &&
                    production.blocks.size()==blocks.size(),
                    "Unknown BNW city production profile or block count");
            out.u64(production.start_offset);out.u64(production.end_offset);
            out.u64(*production.context.bnw_city_version);
            for(std::size_t i=0;i<blocks.size();++i) {
                require(name(production.blocks[i].name)==blocks[i],
                        "Unknown BNW production block or order");
                record_schema(out,production.blocks[i]);
            }
        }
    return identify_bytes(out.bytes).sha256;
}
bool fixed_field(std::string_view field) {
    return field=="version" || field=="hash" ||
           field=="wire_count" || field.ends_with("_wire_count") ||
           field=="m_strName_wire_bytes";
}
void value_shape(std::string_view field,std::int64_t value) {
    const bool boolean=field=="m_bSoldBuildingThisTurn" ||
        field=="bSave" || field=="bRush" ||
        field=="m_bPopulationRankValid" ||
        field=="m_bRouteToCapitalConnectedLastTurn" ||
        field=="m_bRouteToCapitalConnectedThisTurn" ||
        field=="m_bOwedCultureBuilding" ||
        field=="m_abBaseYieldRankValid" ||
        field=="m_abYieldRankValid";
    if(boolean)
        require(value==0 || value==1,"BNW production boolean invalid");
    else if(field=="hash" || field=="iData1_hash")
        require(value>=0 && value<=UINT32_MAX,
                "BNW production hash exceeds u32 wire range");
    else if(fixed_field(field))
        require(value>=INT32_MIN && value<=UINT32_MAX,
                "BNW production fixed wire value exceeds source range");
    else
        require(value>=INT32_MIN && value<=INT32_MAX,
                "BNW production value exceeds i32 wire range");
}
template<class Record,class Fn> void visit_record(Record &record,Fn &&visit) {
    for(auto &field:record.fields) visit(field.name,field.value);
    for(auto &array:record.arrays)
        for(auto &value:array.values) visit(array.name,value);
    for(auto &child:record.children) visit_record(child,visit);
}
template<class World,class Fn> void walk_values(World &world,Fn &&visit) {
    for(auto &player:world.all_players->players)
        for(auto &city:player.cities.cities)
            for(auto &block:city.production.blocks) visit_record(block,visit);
}
template<class Record,class Fn> void visit_blobs(Record &record,Fn &&visit) {
    for(auto &blob:record.blobs)
        for(auto &byte:blob.bytes) visit(byte);
    for(auto &child:record.children) visit_blobs(child,visit);
}
template<class World,class Fn> void walk_bytes(World &world,Fn &&visit) {
    for(auto &player:world.all_players->players)
        for(auto &city:player.cities.cities)
            for(auto &block:city.production.blocks) visit_blobs(block,visit);
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
            const auto &management=city.management;
            const auto &tail=city.tail;
            require(management.layout==bnw && tail.layout==bnw &&
                    management.context.profile_extensions.values.empty() &&
                    tail.context.profile_extensions.values.empty(),
                    "Unknown BNW city production companion profile");
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

saves::CivisSaveSection make_bnw_city_production_checkpoint(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity,
    bool allow_unchanged) {
    hash_shape(identity.payload_sha256);hash_shape(identity.rules_sha256);
    const auto roster=roster_hash(source),schema=schema_hash(source);
    const auto companions=other_city_hash(source);
    require(roster_hash(world)==roster && schema_hash(world)==schema &&
            other_city_hash(world)==companions,
            "BNW production roster, structure or companions changed");
    std::vector<std::int64_t> baseline;
    std::vector<bool> immutable;
    walk_values(source,[&](const char *field,const auto &value){
        require(baseline.size()<max_values,"BNW production value budget exceeded");
        value_shape(name(field),value);baseline.push_back(value);
        immutable.push_back(fixed_field(name(field)));
    });
    std::vector<std::uint8_t> old_bytes;
    walk_bytes(source,[&](const auto &value){
        require(old_bytes.size()<max_bytes,"BNW production byte budget exceeded");
        old_bytes.push_back(value);
    });
    Writer out;out.bytes.insert(out.bytes.end(),magic.begin(),magic.end());
    out.u32(1);
    out.raw(identity.payload_sha256);out.raw(identity.rules_sha256);
    out.raw(roster);out.raw(schema);out.raw(companions);
    out.u32(players);out.u64(baseline.size());out.u64(old_bytes.size());
    bool changed=false;std::size_t index=0;
    walk_values(world,[&](const char *field,const auto &value){
        require(index<baseline.size(),"BNW production value count changed");
        value_shape(name(field),value);
        require(!immutable[index] || value==baseline[index],
                "BNW production version, hash or collection count changed");
        changed|=value!=baseline[index++];out.i64(value);
    });
    require(index==baseline.size(),"BNW production value count changed");
    index=0;
    walk_bytes(world,[&](const auto &value){
        require(index<old_bytes.size(),"BNW production blob length changed");
        changed|=value!=old_bytes[index++];out.bytes.push_back(value);
    });
    require(index==old_bytes.size() && (changed || allow_unchanged),
            "BNW production checkpoint unchanged or incomplete");
    if(!changed) out.bytes[8]=2;
    return {bnw_city_production_checkpoint_section_id,0,std::move(out.bytes)};
}

void restore_bnw_city_production_checkpoint(
    const saves::CivisSaveSection &section,
    saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity) {
    hash_shape(identity.payload_sha256);hash_shape(identity.rules_sha256);
    require(section.id==bnw_city_production_checkpoint_section_id &&
            section.flags==0 && section.bytes.size()>=8+4+64*5+4+8+8 &&
            std::equal(magic.begin(),magic.end(),section.bytes.begin()),
            "Unsupported BNW production checkpoint section");
    Reader in{section.bytes,magic.size()};
    const auto version=in.u32();
    require(version==1 || version==2,
            "Unsupported BNW production checkpoint version");
    in.hash(identity.payload_sha256);in.hash(identity.rules_sha256);
    const auto roster=roster_hash(source),schema=schema_hash(source);
    const auto companions=other_city_hash(source);
    in.hash(roster);in.hash(schema);in.hash(companions);
    require(in.u32()==players && roster_hash(world)==roster &&
            schema_hash(world)==schema && other_city_hash(world)==companions,
            "BNW production destination roster, structure or companions changed");
    const auto values=in.u64(),bytes=in.u64();
    require(values<=max_values && bytes<=max_bytes &&
            values<=(section.bytes.size()-in.at)/8 &&
            bytes==section.bytes.size()-in.at-values*8,
            "BNW production section counts or length changed");
    std::vector<std::int64_t> baseline;
    baseline.reserve(static_cast<std::size_t>(values));
    std::vector<bool> immutable;
    immutable.reserve(static_cast<std::size_t>(values));
    walk_values(source,[&](const char *field,const auto &value){
        require(baseline.size()<max_values,"BNW production value budget exceeded");
        value_shape(name(field),value);baseline.push_back(value);
        immutable.push_back(fixed_field(name(field)));
    });
    std::vector<std::uint8_t> old_bytes;
    old_bytes.reserve(static_cast<std::size_t>(bytes));
    walk_bytes(source,[&](const auto &value){
        require(old_bytes.size()<max_bytes,"BNW production byte budget exceeded");
        old_bytes.push_back(value);
    });
    require(baseline.size()==values && old_bytes.size()==bytes,
            "BNW production source value count changed");
    std::vector<std::pair<std::int64_t*,std::int64_t>> pending_values;
    std::vector<std::pair<std::uint8_t*,std::uint8_t>> pending_bytes;
    pending_values.reserve(static_cast<std::size_t>(values));
    pending_bytes.reserve(static_cast<std::size_t>(bytes));
    bool changed=false;std::size_t index=0;
    walk_values(world,[&](const char *field,auto &value){
        require(index<baseline.size() && value==baseline[index],
                "BNW production destination is not source baseline");
        const auto next=in.i64();value_shape(name(field),next);
        require(!immutable[index] || next==baseline[index],
                "BNW production version, hash or collection count changed");
        changed|=next!=baseline[index++];pending_values.emplace_back(&value,next);
    });
    require(index==baseline.size(),"BNW production destination value count changed");
    index=0;
    walk_bytes(world,[&](auto &value){
        require(index<old_bytes.size() && value==old_bytes[index],
                "BNW production blob destination is not source baseline");
        const auto next=in.u8();changed|=next!=old_bytes[index++];
        pending_bytes.emplace_back(&value,next);
    });
    require(index==old_bytes.size() && changed==(version==1) &&
            in.at==section.bytes.size(),
            "BNW production section incomplete or unchanged");
    for(const auto &[target,value]:pending_values) *target=value;
    for(const auto &[target,value]:pending_bytes) *target=value;
}

BnwCityCoreProductionCheckpointPair make_bnw_city_core_production_checkpoint_pair(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity) {
    auto core_only=source;
    require(roster_hash(world)==roster_hash(source) &&
            world.all_players->players.size()==core_only.all_players->players.size(),
            "BNW city pair roster changed");
    for(std::size_t p=0;p<players;++p) {
        const auto &from=world.all_players->players[p].cities.cities;
        auto &to=core_only.all_players->players[p].cities.cities;
        require(from.size()==to.size(),"BNW city pair count changed");
        for(std::size_t i=0;i<from.size();++i) to[i].core=from[i].core;
    }
    BnwCityCoreProductionCheckpointPair pair;
    pair.core=make_bnw_city_core_checkpoint(core_only,source,identity);
    pair.production=make_bnw_city_production_checkpoint(world,source,identity);
    return pair;
}

saves::SavedSessionSnapshot restore_bnw_city_core_production_checkpoint_pair(
    const BnwCityCoreProductionCheckpointPair &pair,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity) {
    auto replacement=source;
    restore_bnw_city_core_checkpoint(pair.core,replacement,source,identity);
    restore_bnw_city_production_checkpoint(pair.production,replacement,source,identity);
    return replacement;
}
} // namespace civ5::runtime
