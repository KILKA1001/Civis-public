#include "civ5/runtime/bnw_player_strategy_checkpoint.hpp"
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
constexpr std::array<std::uint8_t,8> magic{'C','I','V','B','S','T','R','1'};
constexpr std::array<std::string_view,6> block_names{
    "policies","economic_ai","city_specialization_ai",
    "wonder_production_ai","military_ai","grand_strategy_ai"};
constexpr std::size_t players=64,max_values=1'000'000;
void require(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
std::string_view name(const char *value) {
    return value?std::string_view(value):std::string_view();
}
void hash_shape(std::string_view hash) {
    require(hash.size()==64 && std::all_of(hash.begin(),hash.end(),[](char c){
        return (c>='0'&&c<='9')||(c>='a'&&c<='f');
    }),"Invalid BNW strategy checkpoint SHA-256");
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
        require(value>=INT32_MIN && value<=INT32_MAX,"BNW strategy object ID exceeds i32");
        u32(static_cast<std::uint32_t>(static_cast<std::int32_t>(value)));
    }
    void i64(std::int64_t value) {u64(std::bit_cast<std::uint64_t>(value));}
    void raw(std::string_view value) {bytes.insert(bytes.end(),value.begin(),value.end());}
    void string(std::string_view value) {
        require(value.size()<=UINT32_MAX,"BNW strategy schema name too long");
        u32(static_cast<std::uint32_t>(value.size()));raw(value);
    }
};
struct Reader {
    std::span<const std::uint8_t> bytes;
    std::size_t at=0;
    std::uint32_t u32() {
        require(at<=bytes.size() && bytes.size()-at>=4,"Truncated BNW strategy checkpoint");
        std::uint32_t result=0;
        for(unsigned i=0;i<4;++i) result|=std::uint32_t(bytes[at++])<<(8*i);
        return result;
    }
    std::uint64_t u64() {
        require(at<=bytes.size() && bytes.size()-at>=8,"Truncated BNW strategy checkpoint");
        std::uint64_t result=0;
        for(unsigned i=0;i<8;++i) result|=std::uint64_t(bytes[at++])<<(8*i);
        return result;
    }
    std::int64_t i64() {return std::bit_cast<std::int64_t>(u64());}
    void hash(std::string_view expected) {
        require(at<=bytes.size() && bytes.size()-at>=expected.size() &&
                std::equal(expected.begin(),expected.end(),bytes.begin()+at),
                "BNW strategy source, rules, roster, or schema changed");
        at+=expected.size();
    }
};
template<class Core> std::int64_t scalar(const Core &core,std::string_view wanted) {
    const typename decltype(core.fields)::value_type *found=nullptr;
    for(const auto &field:core.fields)
        if(name(field.name)==wanted) {
            require(!found,"Duplicate BNW strategy object field");found=&field;
        }
    require(found && found->values.size()==1,"Missing BNW strategy object ID");
    return found->values[0];
}
void shape(const saves::SavedSessionSnapshot &world) {
    require(world.game_state.layout==bnw && world.pregame &&
            world.pregame->layout==bnw && world.all_players &&
            world.all_players->layout==bnw &&
            world.all_players->context.player_count==players &&
            world.all_players->players.size()==players &&
            world.all_players->context.profile_extensions.values.empty(),
            "BNW strategy requires complete plain 64-player roster");
}
std::string roster_hash(const saves::SavedSessionSnapshot &world) {
    shape(world);
    const auto team=world.pregame->archive.integer_arrays.find("team_types");
    require(team!=world.pregame->archive.integer_arrays.end() &&
            team->second.size()>=players,"BNW strategy team roster unavailable");
    Writer out;
    for(std::size_t i=0;i<players;++i) {
        const auto &player=world.all_players->players[i];
        require(player.index==i && player.saved_player_id==static_cast<std::int64_t>(i) &&
                player.core.layout==bnw && player.strategy.layout==bnw &&
                player.cities.layout==bnw && player.units.layout==bnw &&
                player.cities.header.wire_city_count>=0 &&
                static_cast<std::uint64_t>(player.cities.header.wire_city_count)==
                    player.cities.cities.size() &&
                player.units.header.wire_unit_count>=0 &&
                static_cast<std::uint64_t>(player.units.header.wire_unit_count)==
                    player.units.units.size() &&
                team->second[i]>=0 && team->second[i]<64,
                "BNW strategy player, city, unit, or team roster changed");
        out.u32(static_cast<std::uint32_t>(i));out.i32(team->second[i]);
        const auto alive=scalar(player.core,"m_bAlive");
        require(alive==0 || alive==1,"BNW strategy player alive flag invalid");
        out.i32(alive);
        out.u64(player.cities.cities.size());
        for(std::size_t j=0;j<player.cities.cities.size();++j) {
            const auto &city=player.cities.cities[j];
            require(city.index==j && city.core.layout==bnw,
                    "BNW strategy city order changed");
            out.i32(scalar(city.core,"m_iID"));
        }
        out.u64(player.units.units.size());
        for(std::size_t j=0;j<player.units.units.size();++j) {
            const auto &unit=player.units.units[j];
            require(unit.index==j && unit.core.layout==bnw &&
                    scalar(unit.core,"m_eOwner")==static_cast<std::int64_t>(i),
                    "BNW strategy unit order or owner changed");
            out.i32(scalar(unit.core,"m_iID"));
        }
    }
    return identify_bytes(out.bytes).sha256;
}
std::string schema_hash(const saves::SavedSessionSnapshot &world) {
    shape(world);
    Writer out;
    const auto &all=*world.all_players;
    out.u64(all.start_offset);out.u64(all.end_offset);
    for(const auto &player:all.players) {
        const auto &strategy=player.strategy;
        require(strategy.dimensions.profile_extensions.values.empty() &&
                strategy.dimensions.major==22 &&
                strategy.dimensions.civ_players==63 &&
                strategy.dimensions.yields==6 &&
                strategy.dimensions.grand==4 &&
                strategy.blocks.size()==block_names.size(),
                "Unknown BNW strategy dimensions or block count");
        out.u64(strategy.start_offset);out.u64(strategy.end_offset);
        for(std::size_t b=0;b<block_names.size();++b) {
            const auto &block=strategy.blocks[b];
            require(name(block.name)==block_names[b],
                    "Unknown BNW strategy block or order");
            out.string(name(block.name));
            out.u64(block.start_offset);out.u64(block.end_offset);
            out.u64(block.fields.size());out.u64(block.arrays.size());
            out.u64(block.hashes.size());
            for(const auto &field:block.fields) out.string(name(field.name));
            for(const auto &array:block.arrays) {
                out.string(name(array.name));out.u64(array.columns);
                out.u64(array.values.size());
                require(array.columns && array.values.size()%array.columns==0,
                        "BNW strategy array row shape changed");
            }
            for(const auto &hash:block.hashes) {
                out.string(name(hash.name));out.i32(hash.wire_count);
                out.u64(hash.rows.size());
                require((hash.wire_count>0?
                             static_cast<std::uint64_t>(hash.wire_count):0)==
                            hash.rows.size(),
                        "BNW strategy hashed row count changed");
                for(const auto &row:hash.rows) {
                    out.u64(row.type_hash);out.u64(row.values.size());
                    require(row.values.size()<=1,
                            "Unknown BNW strategy hashed row shape");
                }
            }
        }
    }
    return identify_bytes(out.bytes).sha256;
}
enum class WireKind {signed_i32,unsigned_i32,boolean};
WireKind field_kind(std::string_view block,std::string_view field) {
    if(field=="version" || field=="policy_ai_version" ||
       (block=="policies" &&
        (field=="branches_chosen_hashes_wire_count" ||
         field=="branch_picked_1_hash" || field=="branch_picked_2_hash" ||
         field=="branch_picked_3_hash"))) return WireKind::unsigned_i32;
    if((block=="economic_ai" && field=="exploration_plots_dirty") ||
       (block=="city_specialization_ai" &&
        (field=="specializations_dirty" || field=="interrupt_wonders")))
        return WireKind::boolean;
    return WireKind::signed_i32;
}
WireKind array_kind(std::string_view block,std::string_view field) {
    if((block=="economic_ai" || block=="military_ai") &&
       field=="using_strategy") return WireKind::boolean;
    if((block=="policies" && field=="branches_chosen_hashes") ||
       (block=="economic_ai" && field=="yields"))
        return WireKind::unsigned_i32;
    return WireKind::signed_i32;
}
WireKind hash_kind(std::string_view block,std::string_view field) {
    if(block=="policies" &&
       (field=="has_policy" || field=="one_shot_fired" ||
        field=="one_shot_free_units_fired" || field=="branch_unlocked" ||
        field=="branch_blocked" || field=="branch_finished"))
        return WireKind::boolean;
    return WireKind::signed_i32;
}
template<class World,class Fn> void walk_values(World &world,Fn &&visit) {
    for(auto &player:world.all_players->players)
        for(auto &block:player.strategy.blocks) {
            for(auto &field:block.fields)
                visit(block,field.name,field_kind(name(block.name),name(field.name)),
                      field.value);
            for(auto &array:block.arrays)
                for(auto &value:array.values)
                    visit(block,array.name,array_kind(name(block.name),name(array.name)),
                          value);
            for(auto &hash:block.hashes)
                for(auto &row:hash.rows)
                    for(auto &value:row.values)
                        visit(block,hash.name,hash_kind(name(block.name),name(hash.name)),
                              value);
        }
}
void value_shape(WireKind kind,std::int64_t value) {
    switch(kind) {
    case WireKind::signed_i32:
        require(value>=INT32_MIN && value<=INT32_MAX,
                "BNW strategy signed value exceeds source wire range");break;
    case WireKind::unsigned_i32:
        require(value>=0 && value<=UINT32_MAX,
                "BNW strategy unsigned value exceeds source wire range");break;
    case WireKind::boolean:
        require(value==0 || value==1,"BNW strategy boolean is invalid");break;
    }
}
bool immutable_field(std::string_view field) {
    return field=="version" || field=="policy_ai_version" ||
           field.ends_with("_wire_count");
}
} // namespace

saves::CivisSaveSection make_bnw_player_strategy_checkpoint(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity,
    bool allow_unchanged) {
    hash_shape(identity.payload_sha256);hash_shape(identity.rules_sha256);
    const auto roster=roster_hash(source),schema=schema_hash(source);
    require(roster_hash(world)==roster && schema_hash(world)==schema,
            "BNW strategy object roster or wire structure changed");
    Writer out;
    out.bytes.insert(out.bytes.end(),magic.begin(),magic.end());
    out.u32(1);
    out.raw(identity.payload_sha256);out.raw(identity.rules_sha256);
    out.raw(roster);out.raw(schema);out.u32(players);
    const auto count_offset=out.bytes.size();out.u64(0);
    std::size_t count=0;bool changed=false;
    auto old_values=std::vector<std::int64_t>{};
    std::vector<bool> immutable;
    walk_values(source,[&](const auto &,const char *field,WireKind kind,
                           const auto &value){
        require(old_values.size()<max_values,
                "BNW strategy source value count exceeds budget");
        value_shape(kind,value);old_values.push_back(value);
        immutable.push_back(immutable_field(name(field)));
    });
    walk_values(world,[&](const auto &,const char *,WireKind kind,
                          const auto &value){
        require(count<old_values.size() && count<max_values,
                "BNW strategy value count exceeds source or budget");
        value_shape(kind,value);
        require(!immutable[count] || value==old_values[count],
                "BNW strategy wire version or collection count changed");
        changed|=value!=old_values[count++];out.i64(value);
    });
    require(count==old_values.size() && (changed || allow_unchanged),
            "BNW strategy checkpoint has no change or incomplete state");
    if(!changed) out.bytes[8]=2;
    for(unsigned i=0;i<8;++i)
        out.bytes[count_offset+i]=static_cast<std::uint8_t>(
            static_cast<std::uint64_t>(count)>>(8*i));
    return {bnw_player_strategy_checkpoint_section_id,0,std::move(out.bytes)};
}

void restore_bnw_player_strategy_checkpoint(
    const saves::CivisSaveSection &section,
    saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity) {
    hash_shape(identity.payload_sha256);hash_shape(identity.rules_sha256);
    require(section.id==bnw_player_strategy_checkpoint_section_id &&
            section.flags==0 && section.bytes.size()>=8+4+64*4+4+8 &&
            std::equal(magic.begin(),magic.end(),section.bytes.begin()),
            "Unsupported BNW strategy checkpoint section");
    Reader in{section.bytes,magic.size()};
    const auto version=in.u32();
    require(version==1 || version==2,
            "Unsupported BNW strategy checkpoint version");
    in.hash(identity.payload_sha256);in.hash(identity.rules_sha256);
    const auto roster=roster_hash(source),schema=schema_hash(source);
    in.hash(roster);in.hash(schema);
    require(in.u32()==players && roster_hash(world)==roster &&
            schema_hash(world)==schema,
            "BNW strategy destination roster or structure changed");
    const auto count=in.u64();
    require(count>0 && count<=max_values &&
            count<=(section.bytes.size()-in.at)/8 &&
            section.bytes.size()-in.at==count*8,
            "BNW strategy value count or section length changed");
    std::vector<std::int64_t> baseline;
    baseline.reserve(static_cast<std::size_t>(count));
    std::vector<bool> immutable;
    walk_values(source,[&](const auto &,const char *field,WireKind kind,
                           const auto &value){
        require(baseline.size()<max_values,
                "BNW strategy source value count exceeds budget");
        value_shape(kind,value);baseline.push_back(value);
        immutable.push_back(immutable_field(name(field)));
    });
    require(baseline.size()==count,"BNW strategy source value count changed");
    std::vector<std::pair<std::int64_t*,std::int64_t>> pending;
    pending.reserve(static_cast<std::size_t>(count));
    bool changed=false;std::size_t index=0;
    walk_values(world,[&](const auto &,const char *,WireKind kind,auto &value){
        require(index<baseline.size() && value==baseline[index],
                "BNW strategy destination is not source baseline");
        const auto next=in.i64();value_shape(kind,next);
        require(!immutable[index] || next==baseline[index],
                "BNW strategy wire version or collection count changed");
        changed|=next!=baseline[index++];pending.emplace_back(&value,next);
    });
    require(index==count && in.at==section.bytes.size() &&
            changed==(version==1),
            "BNW strategy checkpoint incomplete or unchanged");
    // All parsing, checks, and allocations completed before the first write.
    for(const auto &[target,value]:pending) *target=value;
}
} // namespace civ5::runtime
