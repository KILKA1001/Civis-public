#include "civ5/runtime/bnw_player_administration_checkpoint.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <climits>
#include <stdexcept>
#include <string>
#include <string_view>

namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
constexpr std::array<std::uint8_t,8> magic{'C','I','V','B','A','D','M','1'};
constexpr std::array<std::string_view,3> blocks{
    "notifications","treasury","pre_city"};
constexpr std::size_t players=64,max_values=4'000'000,max_blob_bytes=32'000'000;
constexpr std::size_t max_section_bytes=128'000'512;
void require(bool condition,const char *why) {
    if(!condition) throw std::runtime_error(why);
}
std::string_view name(const char *text) {
    return text?std::string_view(text):std::string_view();
}
void digest(std::string_view sha) {
    require(sha.size()==64 &&
            std::all_of(sha.begin(),sha.end(),[](char c) {
                return (c>='0'&&c<='9')||(c>='a'&&c<='f');
            }),"Invalid BNW player administration source/rules SHA-256");
}
bool boolean(std::string_view key) {
    return key=="has_notifications" || key=="m_bDismissed" ||
        key=="m_bEverPoppedGoody" || key=="m_bEverTrainedBuilder";
}
bool signed_count(std::string_view key) {
    return key=="m_UnitCycle_wire_count" || key=="m_researchQueue_wire_count" ||
        key=="m_cityNames_wire_count";
}
bool unsigned_field(std::string_view key) {
    return key=="version" || key=="m_eNotificationType_bits" ||
        (key.ends_with("_wire_count") && !signed_count(key)) ||
        key.ends_with("_wire_bytes");
}
bool structural(std::string_view key) {
    return key=="version" || key=="has_notifications" ||
        key.ends_with("_wire_count") || key.ends_with("_wire_bytes");
}
std::int64_t field(const saves::PlayerAdministrationRecord &record,
                   std::string_view wanted) {
    const saves::NamedSaveInteger *found=nullptr;
    for(const auto &item:record.fields)
        if(name(item.name)==wanted) {
            require(!found,"Duplicate BNW player administration field");
            found=&item;
        }
    require(found,"Missing BNW player administration field");
    return found->value;
}
std::size_t count(const saves::PlayerAdministrationRecord &record,
                  std::string_view key) {
    const auto value=field(record,key);
    return value<=0?0:static_cast<std::size_t>(value);
}
void record_shape(const saves::PlayerAdministrationRecord &record,
                  std::size_t &value_count,std::size_t &blob_count) {
    require(!name(record.name).empty() && record.start_offset<=record.end_offset,
            "BNW player administration record name or offset invalid");
    for(const auto &entry:record.fields) {
        const auto key=name(entry.name);
        const auto value=entry.value;
        require(!key.empty() &&
                (boolean(key)?(value==0 || value==1):
                 unsigned_field(key)?(value>=0 && value<=UINT32_MAX):
                 (value>=INT32_MIN && value<=INT32_MAX)),
                "BNW player administration scalar exceeds source wire range");
        require(value_count<max_values,"BNW player administration value budget exceeded");
        ++value_count;
    }
    for(const auto &array:record.arrays) {
        require(!name(array.name).empty() && (array.columns==1 || array.columns==6) &&
                array.values.size()<=max_values-value_count,
                "BNW player administration array shape invalid");
        value_count+=array.values.size();
        for(const auto value:array.values)
            require(value>=INT32_MIN && value<=INT32_MAX,
                    "BNW player administration array value exceeds i32");
    }
    for(const auto &blob:record.blobs) {
        const auto key=name(blob.name);
        require(!key.empty() && field(record,std::string(key)+"_wire_bytes")==
                    static_cast<std::int64_t>(blob.bytes.size()) &&
                blob.bytes.size()<=max_blob_bytes-blob_count,
                "BNW player administration string length or byte budget changed");
        blob_count+=blob.bytes.size();
    }
    for(std::size_t i=0;i<record.children.size();++i) {
        require(record.children[i].index==i &&
                record.children[i].start_offset>=record.start_offset &&
                record.children[i].end_offset<=record.end_offset,
                "BNW player administration child ID or offset changed");
        record_shape(record.children[i],value_count,blob_count);
    }
}
void array_shape(const saves::PlayerAdministrationRecord &record,
                 std::size_t index,std::string_view expected,
                 std::string_view count_key,std::size_t columns) {
    require(index<record.arrays.size() &&
            name(record.arrays[index].name)==expected &&
            record.arrays[index].columns==columns &&
            count(record,count_key)<=max_values/columns &&
            record.arrays[index].values.size()==count(record,count_key)*columns,
            "BNW player administration array dimension changed");
}
void shape(const saves::SavedSessionSnapshot &world) {
    require(world.game_state.layout==bnw && world.pregame &&
            world.pregame->layout==bnw && world.all_players &&
            world.all_players->layout==bnw &&
            world.all_players->context.player_count==players &&
            world.all_players->players.size()==players &&
            world.all_players->context.profile_extensions.values.empty(),
            "BNW player administration needs complete plain 64-player roster");
    const auto teams=world.pregame->archive.integer_arrays.find("team_types");
    require(teams!=world.pregame->archive.integer_arrays.end() &&
            teams->second.size()>=players,
            "BNW player administration team roster unavailable");
    std::size_t value_count=0,blob_count=0;
    for(std::size_t i=0;i<players;++i) {
        const auto &player=world.all_players->players[i];
        const auto &admin=player.administration;
        require(player.index==i && player.saved_player_id==static_cast<std::int64_t>(i) &&
                teams->second[i]>=0 && teams->second[i]<64 &&
                admin.layout==bnw && admin.start_offset<=admin.end_offset &&
                admin.blocks.size()==blocks.size(),
                "BNW player administration player ID or block roster changed");
        for(std::size_t j=0;j<blocks.size();++j) {
            const auto &block=admin.blocks[j];
            require(name(block.name)==blocks[j] && block.index==0 &&
                    block.start_offset>=admin.start_offset &&
                    block.end_offset<=admin.end_offset,
                    "BNW player administration block order or offset changed");
            record_shape(block,value_count,blob_count);
        }
        const auto &notifications=admin.blocks[0];
        const bool has_notifications=field(notifications,"has_notifications")!=0;
        require(notifications.fields.size()==(has_notifications?6:1) &&
                notifications.arrays.empty() && notifications.blobs.empty() &&
                notifications.children.size()==(has_notifications?100:0),
                "BNW notification presence or slot count changed");
        if(has_notifications) {
            const auto version=field(notifications,"version");
            require(version>=0 && version<=2,
                    "Unsupported BNW notification version");
            for(const auto &slot:notifications.children)
                require(name(slot.name)=="notification" && slot.fields.size()==11 &&
                        slot.arrays.empty() && slot.blobs.size()==2 &&
                        slot.children.empty() &&
                        name(slot.blobs[0].name)=="m_strMessage" &&
                        name(slot.blobs[1].name)=="m_strSummary",
                        "BNW notification slot shape changed");
        }
        const auto &treasury=admin.blocks[1];
        const auto treasury_version=field(treasury,"version");
        require(treasury_version>=0 && treasury_version<=1 &&
                treasury.fields.size()==13 && treasury.arrays.size()==2 &&
                treasury.blobs.empty() && treasury.children.empty(),
                "BNW treasury structure changed");
        array_shape(treasury,0,"m_GoldBalanceForTurnTimes100",
                    "m_GoldBalanceForTurnTimes100_wire_count",1);
        array_shape(treasury,1,"m_GoldChangeForTurnTimes100",
                    "m_GoldChangeForTurnTimes100_wire_count",1);
        const auto &pre_city=admin.blocks[2];
        require(pre_city.fields.size()==9 && pre_city.arrays.size()==4 &&
                pre_city.blobs.empty() &&
                pre_city.children.size()==count(pre_city,"m_cityNames_wire_count"),
                "BNW pre-city collection shape changed");
        array_shape(pre_city,0,"m_ppaaiSpecialistExtraYield",
                    "m_ppaaiSpecialistExtraYield_wire_count",6);
        array_shape(pre_city,1,"m_ppaaiImprovementYieldChange",
                    "m_ppaaiImprovementYieldChange_wire_count",6);
        array_shape(pre_city,2,"m_UnitCycle","m_UnitCycle_wire_count",1);
        array_shape(pre_city,3,"m_researchQueue","m_researchQueue_wire_count",1);
        for(const auto &city_name:pre_city.children)
            require(name(city_name.name)=="city_name" &&
                    city_name.fields.size()==1 && city_name.arrays.empty() &&
                    city_name.blobs.size()==1 && city_name.children.empty() &&
                    name(city_name.blobs[0].name)=="value",
                    "BNW city-name record shape changed");
    }
}
struct Writer {
    std::vector<std::uint8_t> bytes;
    void u32(std::uint32_t value) {
        for(unsigned i=0;i<4;++i)
            bytes.push_back(static_cast<std::uint8_t>(value>>(i*8)));
    }
    void u64(std::uint64_t value) {
        for(unsigned i=0;i<8;++i)
            bytes.push_back(static_cast<std::uint8_t>(value>>(i*8)));
    }
    void i64(std::int64_t value) {u64(std::bit_cast<std::uint64_t>(value));}
    void raw(std::string_view value) {bytes.insert(bytes.end(),value.begin(),value.end());}
    void string(std::string_view value) {
        require(!value.empty() && value.size()<=UINT32_MAX,
                "BNW player administration schema name unavailable");
        u32(static_cast<std::uint32_t>(value.size()));raw(value);
    }
};
struct Reader {
    std::span<const std::uint8_t> bytes;
    std::size_t at=0;
    std::uint32_t u32() {
        require(at<=bytes.size() && bytes.size()-at>=4,
                "Truncated BNW player administration section");
        std::uint32_t result=0;
        for(unsigned i=0;i<4;++i) result|=std::uint32_t(bytes[at++])<<(i*8);
        return result;
    }
    std::uint64_t u64() {
        require(at<=bytes.size() && bytes.size()-at>=8,
                "Truncated BNW player administration section");
        std::uint64_t result=0;
        for(unsigned i=0;i<8;++i) result|=std::uint64_t(bytes[at++])<<(i*8);
        return result;
    }
    std::int64_t i64() {return std::bit_cast<std::int64_t>(u64());}
    std::uint8_t u8() {
        require(at<bytes.size(),"Truncated BNW player administration byte");
        return bytes[at++];
    }
    void hash(std::string_view expected) {
        require(at<=bytes.size() && bytes.size()-at>=expected.size() &&
                std::equal(expected.begin(),expected.end(),bytes.begin()+at),
                "BNW player administration source, rules, roster or schema changed");
        at+=expected.size();
    }
};
void append_schema(Writer &out,const saves::PlayerAdministrationRecord &record) {
    out.string(name(record.name));out.u64(record.index);
    out.u64(record.start_offset);out.u64(record.end_offset);
    out.u64(record.fields.size());
    for(const auto &entry:record.fields) {
        const auto key=name(entry.name);out.string(key);
        if(structural(key)) out.i64(entry.value);
    }
    out.u64(record.arrays.size());
    for(const auto &array:record.arrays) {
        out.string(name(array.name));out.u64(array.columns);
        out.u64(array.values.size());
    }
    out.u64(record.blobs.size());
    for(const auto &blob:record.blobs) {
        out.string(name(blob.name));out.u64(blob.bytes.size());
    }
    out.u64(record.children.size());
    for(const auto &child:record.children) append_schema(out,child);
}
std::string schema_hash(const saves::SavedSessionSnapshot &world) {
    shape(world);Writer out;
    out.u64(world.all_players->start_offset);
    out.u64(world.all_players->end_offset);
    const auto &teams=world.pregame->archive.integer_arrays.at("team_types");
    for(std::size_t i=0;i<players;++i) {
        const auto &player=world.all_players->players[i];
        const auto &admin=player.administration;
        out.u32(static_cast<std::uint32_t>(i));out.i64(teams[i]);
        out.u64(admin.start_offset);out.u64(admin.end_offset);
        out.u64(admin.blocks.size());
        for(const auto &block:admin.blocks) append_schema(out,block);
    }
    return identify_bytes(out.bytes).sha256;
}
struct Flat {
    std::vector<std::int64_t> values;
    std::vector<std::uint8_t> blobs;
};
void append(Flat &flat,const saves::PlayerAdministrationRecord &record) {
    for(const auto &entry:record.fields) flat.values.push_back(entry.value);
    for(const auto &array:record.arrays)
        flat.values.insert(flat.values.end(),array.values.begin(),array.values.end());
    for(const auto &blob:record.blobs)
        flat.blobs.insert(flat.blobs.end(),blob.bytes.begin(),blob.bytes.end());
    for(const auto &child:record.children) append(flat,child);
}
Flat flatten(const saves::SavedSessionSnapshot &world) {
    shape(world);Flat result;
    for(const auto &player:world.all_players->players)
        for(const auto &block:player.administration.blocks)
            append(result,block);
    require(result.values.size()<=max_values && result.blobs.size()<=max_blob_bytes,
            "BNW player administration data budget exceeded");
    return result;
}
void assign(saves::PlayerAdministrationRecord &record,const Flat &flat,
            std::size_t &value_at,std::size_t &blob_at) {
    for(auto &entry:record.fields) entry.value=flat.values[value_at++];
    for(auto &array:record.arrays)
        for(auto &value:array.values) value=flat.values[value_at++];
    for(auto &blob:record.blobs)
        for(auto &byte:blob.bytes) byte=flat.blobs[blob_at++];
    for(auto &child:record.children) assign(child,flat,value_at,blob_at);
}
void assign(saves::SavedSessionSnapshot &world,const Flat &flat) {
    std::size_t value_at=0,blob_at=0;
    for(auto &player:world.all_players->players)
        for(auto &block:player.administration.blocks)
            assign(block,flat,value_at,blob_at);
    require(value_at==flat.values.size() && blob_at==flat.blobs.size(),
            "BNW player administration replacement length changed");
}
} // namespace

void validate_bnw_player_administration_composition(
    std::span<const saves::CivisSaveSection> sections) {
    std::size_t whole=0,overlap=0;
    for(const auto &item:sections) {
        whole+=item.id==bnw_player_administration_checkpoint_section_id;
        overlap+=item.id==8 || item.id==10 || item.id==11 || item.id==15 ||
                 item.id==30 || item.id==33 || item.id==34;
    }
    require(whole<=1 && !(whole && overlap),
            "BNW full player administration overlaps research, treasury or maintenance sections");
}

saves::CivisSaveSection make_bnw_player_administration_checkpoint(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity) {
    digest(identity.payload_sha256);digest(identity.rules_sha256);
    const auto schema=schema_hash(source);
    require(schema_hash(world)==schema,
            "BNW player administration structure or roster changed");
    const auto before=flatten(source),after=flatten(world);
    require(before.values.size()==after.values.size() &&
            before.blobs.size()==after.blobs.size() &&
            (before.values!=after.values || before.blobs!=after.blobs) &&
            before.values.size()<=max_values && before.blobs.size()<=max_blob_bytes &&
            before.values.size()*16+before.blobs.size()*2<=
                max_section_bytes-(8+4+64*3+8*2),
            "BNW player administration unchanged or section budget exceeded");
    Writer out;
    out.bytes.insert(out.bytes.end(),magic.begin(),magic.end());out.u32(1);
    out.raw(identity.payload_sha256);out.raw(identity.rules_sha256);out.raw(schema);
    out.u64(before.values.size());out.u64(before.blobs.size());
    for(std::size_t i=0;i<before.values.size();++i) {
        out.i64(before.values[i]);out.i64(after.values[i]);
    }
    for(std::size_t i=0;i<before.blobs.size();++i) {
        out.bytes.push_back(before.blobs[i]);out.bytes.push_back(after.blobs[i]);
    }
    return {bnw_player_administration_checkpoint_section_id,0,std::move(out.bytes)};
}

saves::SavedSessionSnapshot restore_bnw_player_administration_checkpoint(
    const saves::CivisSaveSection &section,
    const saves::SavedSessionSnapshot &world_before,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity) {
    digest(identity.payload_sha256);digest(identity.rules_sha256);
    require(section.id==bnw_player_administration_checkpoint_section_id &&
            section.flags==0 && section.bytes.size()>=8+4+64*3+8*2 &&
            section.bytes.size()<=max_section_bytes &&
            std::equal(magic.begin(),magic.end(),section.bytes.begin()),
            "Unsupported BNW player administration section");
    const auto schema=schema_hash(source);
    require(schema_hash(world_before)==schema,
            "BNW player administration destination structure changed");
    Reader in{section.bytes,magic.size()};
    require(in.u32()==1,"Unsupported BNW player administration version");
    in.hash(identity.payload_sha256);in.hash(identity.rules_sha256);in.hash(schema);
    const auto value_count=in.u64(),blob_count=in.u64();
    require(value_count<=max_values && blob_count<=max_blob_bytes &&
            value_count*16+blob_count*2==section.bytes.size()-in.at,
            "BNW player administration count or section length changed");
    const auto baseline=flatten(source);
    require(baseline.values.size()==value_count &&
            baseline.blobs.size()==blob_count,
            "BNW player administration source count changed");
    const auto destination=flatten(world_before);
    require(destination.values==baseline.values &&
            destination.blobs==baseline.blobs,
            "BNW player administration destination is not source baseline");
    Flat replacement;
    replacement.values.reserve(static_cast<std::size_t>(value_count));
    replacement.blobs.reserve(static_cast<std::size_t>(blob_count));
    bool changed=false;
    for(std::size_t i=0;i<value_count;++i) {
        const auto old=in.i64(),next=in.i64();
        require(old==baseline.values[i],
                "BNW player administration source scalar changed");
        replacement.values.push_back(next);changed|=old!=next;
    }
    for(std::size_t i=0;i<blob_count;++i) {
        const auto old=in.u8(),next=in.u8();
        require(old==baseline.blobs[i],
                "BNW player administration source string byte changed");
        replacement.blobs.push_back(next);changed|=old!=next;
    }
    require(changed && in.at==section.bytes.size(),
            "BNW player administration section incomplete or unchanged");
    auto result=world_before;
    assign(result,replacement);
    require(schema_hash(result)==schema,
            "BNW player administration version, count or wire value invalid");
    return result;
}

} // namespace civ5::runtime
