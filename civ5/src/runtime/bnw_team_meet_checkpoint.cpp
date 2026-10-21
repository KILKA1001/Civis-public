#include "civ5/runtime/bnw_team_meet_checkpoint.hpp"
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
constexpr std::array<std::uint8_t,8> magic{'C','I','V','B','M','E','T','1'};
constexpr std::size_t wire_size=8+4+64+64+64+4*7;
void require(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
std::string_view name(const char *value) {
    return value?std::string_view(value):std::string_view();
}
void hash_shape(std::string_view hash) {
    require(hash.size()==64 && std::all_of(hash.begin(),hash.end(),[](char c){
        return (c>='0'&&c<='9')||(c>='a'&&c<='f');
    }),"Invalid BNW team meet checkpoint SHA-256");
}
struct Writer {
    std::vector<std::uint8_t> bytes;
    void u32(std::uint32_t value) {
        for(unsigned i=0;i<4;++i)
            bytes.push_back(static_cast<std::uint8_t>(value>>(8*i)));
    }
    void i32(std::int64_t value) {
        require(value>=INT32_MIN && value<=INT32_MAX,
                "BNW team meet value exceeds i32 wire range");
        u32(static_cast<std::uint32_t>(static_cast<std::int32_t>(value)));
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
                "Truncated BNW team meet checkpoint");
        std::uint32_t result=0;
        for(unsigned i=0;i<4;++i)
            result|=std::uint32_t(bytes[at++])<<(8*i);
        return result;
    }
    std::int64_t i32() {return static_cast<std::int32_t>(u32());}
    void hash(std::string_view expected) {
        require(at<=bytes.size() && bytes.size()-at>=expected.size() &&
                std::equal(expected.begin(),expected.end(),bytes.begin()+at),
                "BNW team meet source or rules identity changed");
        at+=expected.size();
    }
};
std::int64_t scalar(const std::vector<saves::NamedSaveInteger> &fields,
                    std::string_view wanted) {
    const saves::NamedSaveInteger *found=nullptr;
    for(const auto &field:fields)
        if(name(field.name)==wanted) {
            require(!found,"Duplicate BNW team meet scalar");found=&field;
        }
    require(found,"Missing BNW team meet scalar");
    return found->value;
}
std::int64_t alive(const saves::SavedPlayerObservation &player) {
    const saves::PlayerCoreField *found=nullptr;
    for(const auto &field:player.core.fields)
        if(name(field.name)=="m_bAlive") {
            require(!found,"Duplicate BNW team meet player alive field");
            found=&field;
        }
    require(found && found->values.size()==1 &&
            (found->values[0]==0 || found->values[0]==1),
            "BNW team meet player alive field is unavailable");
    return found->values[0];
}
void shape(const saves::SavedSessionSnapshot &world) {
    require(world.game_state.layout==bnw && world.pregame &&
            world.pregame->layout==bnw && world.teams &&
            world.teams->layout==bnw &&
            world.teams->dimensions.teams==64 &&
            world.teams->dimensions.civ_teams==63 &&
            world.teams->teams.size()==64 && world.all_players &&
            world.all_players->layout==bnw &&
            world.all_players->players.size()==64,
            "BNW team meet needs complete team/player roster");
}
std::string roster_hash(const saves::SavedSessionSnapshot &world) {
    shape(world);
    Writer out;
    const auto &archive=world.pregame->archive;
    for(const auto key:{"team_types","slot_status","slot_claims"}) {
        const auto found=archive.integer_arrays.find(key);
        require(found!=archive.integer_arrays.end() &&
                found->second.size()>=64,
                "BNW team meet pregame roster is unavailable");
        for(std::size_t i=0;i<64;++i) out.i32(found->second[i]);
    }
    const auto minors=archive.flag_arrays.find("minor_nation_civs");
    require(minors!=archive.flag_arrays.end() && minors->second.size()>=64,
            "BNW team meet minor roster is unavailable");
    for(std::size_t i=0;i<64;++i) out.u32(minors->second[i]?1:0);
    for(std::size_t i=0;i<64;++i) {
        const auto &team=world.teams->teams[i];
        const auto &player=world.all_players->players[i];
        require(team.index==i && scalar(team.fields,"id")==
                    static_cast<std::int64_t>(i) &&
                player.index==i && player.saved_player_id==
                    static_cast<std::int64_t>(i) && player.core.layout==bnw,
                "BNW team meet team/player IDs changed");
        out.u32(static_cast<std::uint32_t>(i));
        out.i32(alive(player));
    }
    return identify_bytes(out.bytes).sha256;
}
const saves::PlotArrayObservation &array(const saves::TeamObservation &team,
                                          std::string_view wanted,
                                          std::size_t width) {
    const saves::PlotArrayObservation *found=nullptr;
    for(const auto &candidate:team.arrays)
        if(name(candidate.name)==wanted) {
            require(!found,"Duplicate BNW team meet array");
            found=&candidate;
        }
    require(found && found->columns==1 && found->values.size()==width,
            "BNW team meet array width is unavailable");
    return *found;
}
saves::PlotArrayObservation &mutable_array(saves::TeamObservation &team,
                                            std::string_view wanted,
                                            std::size_t width) {
    (void)array(team,wanted,width);
    for(auto &candidate:team.arrays)
        if(name(candidate.name)==wanted) return candidate;
    throw std::runtime_error("Missing BNW team meet array");
}
struct Pair {
    std::int64_t a=-1,b=-1,old_a=-1,old_b=-1,turn=-1;
};
void pair_shape(const Pair &pair) {
    require(pair.a>=0 && pair.a<63 && pair.b>=0 && pair.b<63 &&
            pair.a!=pair.b && pair.old_a>=-1 &&
            pair.old_b>=-1 && pair.turn>=0 && pair.turn<=INT32_MAX &&
            pair.old_a<=pair.turn && pair.old_b<=pair.turn,
            "BNW team meet pair, turns or source order is invalid");
}
std::int64_t game_turn(const saves::SavedSessionSnapshot &world) {
    shape(world);
    const auto found=world.pregame->archive.integers.find("game_turn");
    require(found!=world.pregame->archive.integers.end() &&
            found->second>=0 && found->second<=INT32_MAX,
            "BNW team meet game turn is unavailable");
    return found->second;
}
std::int64_t member(const saves::SavedSessionSnapshot &world,
                    std::int64_t team,std::string_view wanted,
                    std::int64_t other) {
    const auto &record=world.teams->teams.at(static_cast<std::size_t>(team));
    const auto &values=array(record,wanted,wanted=="has_met"?64:63).values;
    return values.at(static_cast<std::size_t>(other));
}
void same_team_except_pair(const saves::SavedSessionSnapshot &world,
                           const saves::SavedSessionSnapshot &source,
                           std::int64_t a,std::int64_t b) {
    shape(world);shape(source);
    const auto &x=*world.teams,&y=*source.teams;
    require(x.start_offset==y.start_offset &&
            x.end_offset==y.end_offset &&
            x.dimensions.players==y.dimensions.players &&
            x.dimensions.fixed_players==y.dimensions.fixed_players &&
            x.dimensions.fixed_teams==y.dimensions.fixed_teams &&
            x.dimensions.domains==y.dimensions.domains &&
            x.dimensions.major==y.dimensions.major &&
            x.dimensions.yields==y.dimensions.yields &&
            x.dimensions.profile_extensions.values.empty() &&
            y.dimensions.profile_extensions.values.empty(),
            "BNW team meet collection shape changed");
    for(std::size_t i=0;i<64;++i) {
        const auto &now=x.teams[i],&old=y.teams[i];
        require(now.index==old.index &&
                now.start_offset==old.start_offset &&
                now.end_offset==old.end_offset &&
                now.fields.size()==old.fields.size() &&
                now.arrays.size()==old.arrays.size() &&
                now.hashes.size()==old.hashes.size() &&
                now.project_art.size()==old.project_art.size(),
                "BNW team meet record shape changed");
        for(std::size_t j=0;j<now.fields.size();++j)
            require(name(now.fields[j].name)==name(old.fields[j].name) &&
                    now.fields[j].value==old.fields[j].value,
                    "BNW team meet has unsupported scalar mutation");
        for(std::size_t j=0;j<now.arrays.size();++j) {
            const auto &p=now.arrays[j],&q=old.arrays[j];
            require(name(p.name)==name(q.name) &&
                    p.columns==q.columns &&
                    p.values.size()==q.values.size(),
                    "BNW team meet array shape changed");
            for(std::size_t k=0;k<p.values.size();++k) {
                const bool permitted=(
                    (i==static_cast<std::size_t>(a) &&
                     k==static_cast<std::size_t>(b)) ||
                    (i==static_cast<std::size_t>(b) &&
                     k==static_cast<std::size_t>(a))) &&
                    (name(p.name)=="has_met" ||
                     name(p.name)=="turn_team_met");
                if(!permitted)
                    require(p.values[k]==q.values[k],
                            "BNW team meet has unsupported team-array mutation");
            }
        }
        for(std::size_t j=0;j<now.hashes.size();++j) {
            const auto &p=now.hashes[j],&q=old.hashes[j];
            require(name(p.name)==name(q.name) &&
                    p.wire_count==q.wire_count &&
                    p.rows.size()==q.rows.size(),
                    "BNW team meet hashed-array shape changed");
            for(std::size_t k=0;k<p.rows.size();++k)
                require(p.rows[k].type_hash==q.rows[k].type_hash &&
                        p.rows[k].values==q.rows[k].values,
                        "BNW team meet has unsupported hashed-array mutation");
        }
        for(std::size_t j=0;j<now.project_art.size();++j) {
            const auto &p=now.project_art[j],&q=old.project_art[j];
            require(p.key==q.key && p.source_count==q.source_count &&
                    p.wire_count==q.wire_count && p.values==q.values,
                    "BNW team meet has unsupported project-art mutation");
        }
    }
}
void baseline(const saves::SavedSessionSnapshot &world,
              const saves::SavedSessionSnapshot &source,
              const Pair &pair,bool after) {
    const auto a=pair.a,b=pair.b;
    require(member(source,a,"has_met",b)==0 &&
            member(source,b,"has_met",a)==0 &&
            member(source,a,"turn_team_met",b)==pair.old_a &&
            member(source,b,"turn_team_met",a)==pair.old_b,
            "BNW team meet source pair changed");
    require(member(world,a,"has_met",b)==(after?1:0) &&
            member(world,b,"has_met",a)==(after?1:0) &&
            member(world,a,"turn_team_met",b)==
                (after?pair.turn:pair.old_a) &&
            member(world,b,"turn_team_met",a)==
                (after?pair.turn:pair.old_b),
            "BNW team meet pair does not match SDK transition");
}
} // namespace

saves::CivisSaveSection make_bnw_team_meet_checkpoint(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity,
    const BnwSightMeetTransition &contact) {
    hash_shape(identity.payload_sha256);hash_shape(identity.rules_sha256);
    require(contact.new_contact,
            "BNW team meet checkpoint requires one new contact");
    const auto source_roster=roster_hash(source);
    require(roster_hash(world)==source_roster,
            "BNW team meet source roster changed");
    const Pair pair{contact.revealing_team,contact.other_team,
        member(source,contact.revealing_team,"turn_team_met",
               contact.other_team),
        member(source,contact.other_team,"turn_team_met",
               contact.revealing_team),game_turn(source)};
    pair_shape(pair);
    require(game_turn(world)==pair.turn,
            "BNW team meet owned game turn changed");
    same_team_except_pair(world,source,pair.a,pair.b);
    baseline(world,source,pair,true);
    Writer out;
    out.bytes.insert(out.bytes.end(),magic.begin(),magic.end());
    out.u32(1);
    out.raw(identity.payload_sha256);out.raw(identity.rules_sha256);
    out.raw(source_roster);
    out.u32(64);out.u32(63);
    out.u32(static_cast<std::uint32_t>(pair.a));
    out.u32(static_cast<std::uint32_t>(pair.b));
    out.i32(pair.old_a);out.i32(pair.old_b);out.i32(pair.turn);
    require(out.bytes.size()==wire_size,
            "BNW team meet checkpoint wire size changed");
    return {bnw_team_meet_checkpoint_section_id,0,std::move(out.bytes)};
}

void restore_bnw_team_meet_checkpoint(
    const saves::CivisSaveSection &section,
    saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity) {
    hash_shape(identity.payload_sha256);hash_shape(identity.rules_sha256);
    require(section.id==bnw_team_meet_checkpoint_section_id &&
            section.flags==0 && section.bytes.size()==wire_size &&
            std::equal(magic.begin(),magic.end(),section.bytes.begin()),
            "Unsupported BNW team meet checkpoint section");
    Reader in{section.bytes,magic.size()};
    require(in.u32()==1,"Unsupported BNW team meet checkpoint version");
    in.hash(identity.payload_sha256);in.hash(identity.rules_sha256);
    const auto source_roster=roster_hash(source);
    in.hash(source_roster);
    require(in.u32()==64 && in.u32()==63,
            "BNW team meet roster dimensions changed");
    Pair pair;
    pair.a=in.u32();pair.b=in.u32();
    pair.old_a=in.i32();pair.old_b=in.i32();pair.turn=in.i32();
    require(in.at==section.bytes.size(),
            "BNW team meet checkpoint has trailing bytes");
    pair_shape(pair);
    require(roster_hash(world)==source_roster &&
            game_turn(source)==pair.turn && game_turn(world)==pair.turn,
            "BNW team meet roster or game turn changed");
    same_team_except_pair(world,source,pair.a,pair.b);
    baseline(world,source,pair,false);
    auto &a_met=mutable_array(world.teams->teams.at(
        static_cast<std::size_t>(pair.a)),"has_met",64).values.at(
            static_cast<std::size_t>(pair.b));
    auto &a_turn=mutable_array(world.teams->teams.at(
        static_cast<std::size_t>(pair.a)),"turn_team_met",63).values.at(
            static_cast<std::size_t>(pair.b));
    auto &b_met=mutable_array(world.teams->teams.at(
        static_cast<std::size_t>(pair.b)),"has_met",64).values.at(
            static_cast<std::size_t>(pair.a));
    auto &b_turn=mutable_array(world.teams->teams.at(
        static_cast<std::size_t>(pair.b)),"turn_team_met",63).values.at(
            static_cast<std::size_t>(pair.a));
    // All potentially throwing operations complete before the four writes.
    a_met=1;a_turn=pair.turn;b_met=1;b_turn=pair.turn;
}
} // namespace civ5::runtime
