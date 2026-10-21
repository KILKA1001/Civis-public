#include "civ5/runtime/bnw_player_proximity_checkpoint.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <array>
#include <climits>
#include <span>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
constexpr std::array<std::uint8_t,8> magic{'C','I','V','B','P','R','X','1'};
constexpr std::size_t wire_size=8+4+64+64+64+4+4+4+4+4*4;
void require(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
std::string_view name(const char *value) {
    return value?std::string_view(value):std::string_view();
}
void hash_shape(std::string_view hash) {
    require(hash.size()==64 && std::all_of(hash.begin(),hash.end(),[](char c){
        return (c>='0'&&c<='9')||(c>='a'&&c<='f');
    }),"Invalid BNW player proximity checkpoint SHA-256");
}
struct Writer {
    std::vector<std::uint8_t> bytes;
    void u32(std::uint32_t value) {
        for(unsigned i=0;i<4;++i)
            bytes.push_back(static_cast<std::uint8_t>(value>>(8*i)));
    }
    void i32(std::int64_t value) {
        require(value>=INT32_MIN && value<=INT32_MAX,
                "BNW player proximity value exceeds i32 wire range");
        u32(static_cast<std::uint32_t>(static_cast<std::int32_t>(value)));
    }
    void u64(std::uint64_t value) {
        for(unsigned i=0;i<8;++i)
            bytes.push_back(static_cast<std::uint8_t>(value>>(8*i)));
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
                "Truncated BNW player proximity checkpoint");
        std::uint32_t result=0;
        for(unsigned i=0;i<4;++i)
            result|=std::uint32_t(bytes[at++])<<(8*i);
        return result;
    }
    std::int64_t i32() {return static_cast<std::int32_t>(u32());}
    void hash(std::string_view expected) {
        require(at<=bytes.size() && bytes.size()-at>=expected.size() &&
                std::equal(expected.begin(),expected.end(),bytes.begin()+at),
                "BNW player proximity source or rules identity changed");
        at+=expected.size();
    }
};
const saves::PlayerCoreField &core_field(
    const saves::SavedPlayerObservation &player,std::string_view wanted) {
    const saves::PlayerCoreField *found=nullptr;
    for(const auto &field:player.core.fields)
        if(name(field.name)==wanted) {
            require(!found,"Duplicate BNW player core field");found=&field;
        }
    require(found,"Missing BNW player core field");
    return *found;
}
saves::PlayerCoreField &mutable_core_field(
    saves::SavedPlayerObservation &player,std::string_view wanted) {
    (void)core_field(player,wanted);
    for(auto &field:player.core.fields)
        if(name(field.name)==wanted) return field;
    throw std::runtime_error("Missing BNW player core field");
}
std::int64_t scalar(const saves::CityCoreObservation &core,
                    std::string_view wanted) {
    const saves::CityCoreField *found=nullptr;
    for(const auto &field:core.fields)
        if(name(field.name)==wanted) {
            require(!found,"Duplicate BNW city roster ID");found=&field;
        }
    require(found && found->encoding==saves::CityCoreEncoding::i32 &&
            found->values.size()==1,"Missing BNW city roster ID");
    return found->values.front();
}
std::int64_t scalar(const saves::UnitCoreObservation &core,
                    std::string_view wanted) {
    const saves::UnitCoreField *found=nullptr;
    for(const auto &field:core.fields)
        if(name(field.name)==wanted) {
            require(!found,"Duplicate BNW unit roster ID");found=&field;
        }
    require(found && found->encoding==saves::UnitCoreEncoding::i32 &&
            found->values.size()==1,"Missing BNW unit roster ID");
    return found->values.front();
}
void shape(const saves::SavedSessionSnapshot &world) {
    require(world.game_state.layout==bnw && world.pregame &&
            world.pregame->layout==bnw && world.all_players &&
            world.all_players->layout==bnw &&
            world.all_players->context.player_count==64 &&
            world.all_players->players.size()==64,
            "BNW player proximity requires complete 64-slot player roster");
}
std::string roster_hash(const saves::SavedSessionSnapshot &world) {
    shape(world);
    Writer out;
    const auto &archive=world.pregame->archive;
    for(const auto key:{"team_types","slot_status","slot_claims"}) {
        const auto found=archive.integer_arrays.find(key);
        require(found!=archive.integer_arrays.end() &&
                found->second.size()>=64,
                "BNW player proximity pregame roster array is unavailable");
        for(std::size_t i=0;i<64;++i) out.i32(found->second[i]);
    }
    const auto minors=archive.flag_arrays.find("minor_nation_civs");
    require(minors!=archive.flag_arrays.end() && minors->second.size()>=64,
            "BNW player proximity minor roster is unavailable");
    for(std::size_t i=0;i<64;++i) out.u32(minors->second[i]?1:0);
    for(std::size_t i=0;i<64;++i) {
        const auto &player=world.all_players->players[i];
        require(player.index==i && player.saved_player_id==
                    static_cast<std::int64_t>(i) &&
                player.core.layout==bnw && player.cities.layout==bnw &&
                player.units.layout==bnw &&
                player.cities.header.wire_city_count>=0 &&
                static_cast<std::uint64_t>(player.cities.header.wire_city_count)==
                    player.cities.cities.size() &&
                player.units.header.wire_unit_count>=0 &&
                static_cast<std::uint64_t>(player.units.header.wire_unit_count)==
                    player.units.units.size(),
                "BNW player proximity roster ID or collection count changed");
        const auto &alive=core_field(player,"m_bAlive");
        require(alive.values.size()==1 &&
                (alive.values[0]==0 || alive.values[0]==1),
                "BNW player proximity alive flag is unavailable");
        out.u32(static_cast<std::uint32_t>(i));
        out.i32(alive.values[0]);
        out.u64(player.cities.cities.size());
        for(std::size_t j=0;j<player.cities.cities.size();++j) {
            const auto &city=player.cities.cities[j];
            require(city.index==j && city.core.layout==bnw,
                    "BNW player proximity city roster order changed");
            out.i32(scalar(city.core,"m_iID"));
        }
        out.u64(player.units.units.size());
        for(std::size_t j=0;j<player.units.units.size();++j) {
            const auto &unit=player.units.units[j];
            require(unit.index==j && unit.core.layout==bnw &&
                    scalar(unit.core,"m_eOwner")==
                        static_cast<std::int64_t>(i),
                    "BNW player proximity unit roster order or owner changed");
            out.i32(scalar(unit.core,"m_iID"));
        }
    }
    return identify_bytes(out.bytes).sha256;
}
void same_core_except_pair(const saves::SavedSessionSnapshot &world,
                           const saves::SavedSessionSnapshot &source,
                           std::int64_t a,std::int64_t b) {
    shape(world);shape(source);
    require(world.all_players->start_offset==source.all_players->start_offset &&
            world.all_players->end_offset==source.all_players->end_offset,
            "BNW player proximity player collection identity changed");
    for(std::size_t i=0;i<64;++i) {
        const auto &x=world.all_players->players[i];
        const auto &y=source.all_players->players[i];
        require(x.index==y.index && x.saved_player_id==y.saved_player_id &&
                x.core.start_offset==y.core.start_offset &&
                x.core.end_offset==y.core.end_offset &&
                x.core.version==y.core.version &&
                x.core.fields.size()==y.core.fields.size(),
                "BNW player proximity core shape changed");
        for(std::size_t j=0;j<x.core.fields.size();++j) {
            const auto &now=x.core.fields[j],&old=y.core.fields[j];
            require(name(now.name)==name(old.name) &&
                    now.encoding==old.encoding &&
                    now.start_offset==old.start_offset &&
                    now.end_offset==old.end_offset &&
                    now.wire_count==old.wire_count &&
                    now.row_counts==old.row_counts &&
                    now.hashes==old.hashes && now.bytes==old.bytes &&
                    now.values.size()==old.values.size(),
                    "BNW player proximity core field structure changed");
            if(name(now.name)!="m_aiProximityToPlayer") {
                require(now.values==old.values,
                        "BNW player proximity has unsupported player-core mutation");
                continue;
            }
            require(now.encoding==saves::PlayerCoreEncoding::vector_i32 &&
                    now.values.size()==64,
                    "BNW player proximity vector shape changed");
            for(std::size_t k=0;k<64;++k)
                if(!((i==static_cast<std::size_t>(a) &&
                      k==static_cast<std::size_t>(b)) ||
                     (i==static_cast<std::size_t>(b) &&
                      k==static_cast<std::size_t>(a))))
                    require(now.values[k]==old.values[k],
                            "BNW player proximity has unsupported slot mutation");
        }
    }
}
void pair_shape(const std::vector<BnwSightProximityChange> &changes) {
    require(changes.size()==2 && changes[0].player>=0 &&
            changes[0].player<63 && changes[0].other>=0 &&
            changes[0].other<63 &&
            changes[0].player!=changes[0].other &&
            changes[1].player==changes[0].other &&
            changes[1].other==changes[0].player &&
            changes[0].new_value==changes[1].new_value,
            "BNW player proximity requires one reciprocal SDK pair");
    for(const auto &change:changes)
        require(change.old_value>=-1 && change.old_value<=3 &&
                change.new_value>=-1 && change.new_value<=3,
                "BNW player proximity enum is invalid");
}
std::int64_t slot(const saves::SavedSessionSnapshot &world,
                  std::int64_t player,std::int64_t other) {
    const auto &field=core_field(world.all_players->players.at(
        static_cast<std::size_t>(player)),"m_aiProximityToPlayer");
    require(field.encoding==saves::PlayerCoreEncoding::vector_i32 &&
            field.values.size()==64,"BNW player proximity vector is unavailable");
    return field.values.at(static_cast<std::size_t>(other));
}
} // namespace

saves::CivisSaveSection make_bnw_player_proximity_checkpoint(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity,
    const std::vector<BnwSightProximityChange> &changes) {
    hash_shape(identity.payload_sha256);hash_shape(identity.rules_sha256);
    pair_shape(changes);
    const auto source_roster=roster_hash(source);
    require(roster_hash(world)==source_roster,
            "BNW player proximity source player roster changed");
    same_core_except_pair(world,source,changes[0].player,changes[0].other);
    for(const auto &change:changes)
        require(slot(source,change.player,change.other)==change.old_value &&
                slot(world,change.player,change.other)==change.new_value,
                "BNW player proximity SDK pair disagrees with owned state");
    Writer out;
    out.bytes.insert(out.bytes.end(),magic.begin(),magic.end());
    out.u32(1);
    out.raw(identity.payload_sha256);out.raw(identity.rules_sha256);
    out.raw(source_roster);
    out.u32(64);out.u32(63);
    out.u32(static_cast<std::uint32_t>(changes[0].player));
    out.u32(static_cast<std::uint32_t>(changes[0].other));
    for(const auto &change:changes) {
        out.i32(change.old_value);out.i32(change.new_value);
    }
    require(out.bytes.size()==wire_size,
            "BNW player proximity checkpoint wire size changed");
    return {bnw_player_proximity_checkpoint_section_id,0,std::move(out.bytes)};
}

void restore_bnw_player_proximity_checkpoint(
    const saves::CivisSaveSection &section,
    saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity) {
    hash_shape(identity.payload_sha256);hash_shape(identity.rules_sha256);
    require(section.id==bnw_player_proximity_checkpoint_section_id &&
            section.flags==0 && section.bytes.size()==wire_size &&
            std::equal(magic.begin(),magic.end(),section.bytes.begin()),
            "Unsupported BNW player proximity checkpoint section");
    Reader in{section.bytes,magic.size()};
    require(in.u32()==1,"Unsupported BNW player proximity checkpoint version");
    in.hash(identity.payload_sha256);in.hash(identity.rules_sha256);
    const auto source_roster=roster_hash(source);
    in.hash(source_roster);
    require(in.u32()==64 && in.u32()==63,
            "BNW player proximity roster dimensions changed");
    const auto a=in.u32(),b=in.u32();
    std::vector<BnwSightProximityChange> changes(2);
    changes[0]={static_cast<std::int64_t>(a),static_cast<std::int64_t>(b),
                in.i32(),in.i32()};
    changes[1]={static_cast<std::int64_t>(b),static_cast<std::int64_t>(a),
                in.i32(),in.i32()};
    require(in.at==section.bytes.size(),
            "BNW player proximity checkpoint has trailing bytes");
    pair_shape(changes);
    require(roster_hash(world)==source_roster,
            "BNW player proximity restore player roster changed");
    same_core_except_pair(world,source,a,b);
    for(const auto &change:changes)
        require(slot(source,change.player,change.other)==change.old_value &&
                slot(world,change.player,change.other)==change.old_value,
                "BNW player proximity restore baseline differs from source");
    auto &first=mutable_core_field(world.all_players->players.at(a),
                                   "m_aiProximityToPlayer").values[b];
    auto &second=mutable_core_field(world.all_players->players.at(b),
                                    "m_aiProximityToPlayer").values[a];
    // All allocations, structural checks and throwing reads are complete.
    // The two indexed i64 assignments below cannot allocate or throw.
    first=changes[0].new_value;
    second=changes[1].new_value;
}
} // namespace civ5::runtime
