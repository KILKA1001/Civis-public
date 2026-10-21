#include "civ5/runtime/bnw_team_tech_suffix_receipt_checkpoint.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <climits>
#include <span>
#include <set>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace civ5::runtime {
namespace {
constexpr std::array<std::uint8_t,8> magic{'C','I','V','T','S','R','0','1'};
void require(bool ok,const char *reason) {
    if(!ok) throw std::runtime_error(reason);
}
void digest(std::string_view value) {
    require(value.size()==64 &&
            std::all_of(value.begin(),value.end(),[](char c) {
                return (c>='0'&&c<='9')||(c>='a'&&c<='f');
            }),"Invalid BNW suffix receipt digest");
}
void binding_shape(const BnwTeamTechSuffixCheckpointBinding &binding) {
    digest(binding.source.payload_sha256);
    digest(binding.source.rules_sha256);
    digest(binding.world_sha256);
    digest(binding.lua_vm_sha256);
    digest(binding.external_escrow_sha256);
    require(binding.revision>0,"Invalid BNW suffix checkpoint revision");
}
void marker_shape(const BnwTeamTechSuffixCallbackMarker &marker) {
    require(marker.team>=0 && marker.team<=63 && marker.technology>=0 &&
            marker.technology<=INT32_MAX,
            "Invalid BNW suffix callback identity");
    const bool uncertain_ui=marker.ui_query_attempted &&
        !marker.ui_query_confirmed && !marker.no_reward_popups &&
        !marker.engine_attempted && !marker.engine_confirmed &&
        marker.uncertain;
    const bool uncertain_engine=marker.ui_query_attempted &&
        marker.ui_query_confirmed && marker.no_reward_popups.has_value() &&
        marker.engine_attempted && !marker.engine_confirmed &&
        marker.uncertain;
    const bool confirmed=marker.ui_query_attempted &&
        marker.ui_query_confirmed && marker.no_reward_popups.has_value() &&
        marker.engine_attempted && marker.engine_confirmed &&
        !marker.uncertain;
    require(uncertain_ui || uncertain_engine || confirmed,
            "Unsupported BNW suffix callback receipt state");
}
struct Writer {
    std::vector<std::uint8_t> bytes;
    void raw(std::string_view value) {
        bytes.insert(bytes.end(),value.begin(),value.end());
    }
    void u8(std::uint8_t value) {bytes.push_back(value);}
    void u32(std::uint32_t value) {
        for(unsigned i=0;i<4;++i) u8(static_cast<std::uint8_t>(value>>(8*i)));
    }
    void u64(std::uint64_t value) {
        for(unsigned i=0;i<8;++i) u8(static_cast<std::uint8_t>(value>>(8*i)));
    }
};
const saves::CivisSaveSection &unique_section(
    const saves::CivisSaveDocument &document,std::uint32_t id) {
    const saves::CivisSaveSection *found=nullptr;
    for(const auto &section:document.sections)
        if(section.id==id) {
            require(!found,"Duplicate BNW suffix dependency section");
            found=&section;
        }
    require(found,"Missing BNW suffix dependency section");
    return *found;
}
std::string bundle_digest(const saves::CivisSaveDocument &document,
                          const std::vector<std::uint32_t> &ids) {
    require(!ids.empty(),"Empty BNW suffix dependency bundle");
    Writer out;
    out.raw("CIVIS-SECTION-BUNDLE-V1");
    out.u32(static_cast<std::uint32_t>(ids.size()));
    for(auto id:ids) {
        const auto &section=unique_section(document,id);
        out.u32(section.id);out.u32(section.flags);
        out.u64(section.bytes.size());
        out.raw(identify_bytes(section.bytes).sha256);
    }
    return identify_bytes(out.bytes).sha256;
}
struct Reader {
    std::span<const std::uint8_t> bytes;
    std::size_t at=0;
    std::span<const std::uint8_t> raw(std::size_t count) {
        require(at<=bytes.size() && count<=bytes.size()-at,
                "Truncated BNW suffix callback receipt");
        const auto value=bytes.subspan(at,count);at+=count;return value;
    }
    std::uint8_t u8() {return raw(1)[0];}
    bool boolean() {
        const auto value=u8();
        require(value<=1,"Invalid BNW suffix callback boolean");
        return value!=0;
    }
    std::uint32_t u32() {
        const auto bytes=raw(4);
        std::uint32_t value=0;
        for(unsigned i=0;i<4;++i) value|=std::uint32_t(bytes[i])<<(8*i);
        return value;
    }
    std::uint64_t u64() {
        const auto bytes=raw(8);
        std::uint64_t value=0;
        for(unsigned i=0;i<8;++i) value|=std::uint64_t(bytes[i])<<(8*i);
        return value;
    }
    void equal(std::string_view expected,const char *reason) {
        const auto value=raw(expected.size());
        require(std::equal(value.begin(),value.end(),expected.begin()),reason);
    }
};
} // namespace

BnwTeamTechSuffixCheckpointBinding bind_bnw_team_tech_suffix_document(
    const saves::CivisSaveDocument &document,
    const SessionCheckpointIdentity &source,std::uint64_t revision,
    const std::vector<std::uint32_t> &world_section_ids,
    std::uint32_t lua_vm_section_id,
    std::uint32_t external_escrow_section_id) {
    require(document.version==1 && document.flags==0,
            "Unsupported BNW suffix document envelope");
    require(!world_section_ids.empty() &&
            std::is_sorted(world_section_ids.begin(),world_section_ids.end()),
            "BNW suffix world dependency IDs must be sorted");
    std::set<std::uint32_t> used;
    for(auto id:world_section_ids)
        require(id && id!=bnw_team_tech_suffix_receipt_section_id &&
                used.insert(id).second,
                "Invalid BNW suffix world dependency ID");
    for(auto id:{lua_vm_section_id,external_escrow_section_id})
        require(id && id!=bnw_team_tech_suffix_receipt_section_id &&
                used.insert(id).second,
                "Invalid BNW suffix Lua or escrow dependency ID");
    BnwTeamTechSuffixCheckpointBinding result{
        source,revision,bundle_digest(document,world_section_ids),
        bundle_digest(document,{lua_vm_section_id}),
        bundle_digest(document,{external_escrow_section_id})};
    binding_shape(result);
    return result;
}

saves::CivisSaveSection make_bnw_team_tech_suffix_receipt_checkpoint(
    const BnwTeamTechSuffixCallbackMarker &marker,
    const BnwTeamTechSuffixCheckpointBinding &binding) {
    binding_shape(binding);
    marker_shape(marker);
    Writer out;
    out.raw({reinterpret_cast<const char*>(magic.data()),magic.size()});
    out.u32(1);
    out.raw(binding.source.payload_sha256);
    out.raw(binding.source.rules_sha256);
    out.raw(binding.world_sha256);
    out.raw(binding.lua_vm_sha256);
    out.raw(binding.external_escrow_sha256);
    out.u64(binding.revision);
    out.u32(static_cast<std::uint32_t>(marker.team));
    out.u32(static_cast<std::uint32_t>(marker.technology));
    out.u8(marker.ui_query_attempted);
    out.u8(marker.ui_query_confirmed);
    out.u8(marker.no_reward_popups ? (*marker.no_reward_popups ? 2:1):0);
    out.u8(marker.engine_attempted);
    out.u8(marker.engine_confirmed);
    out.u8(marker.uncertain);
    return {bnw_team_tech_suffix_receipt_section_id,0,std::move(out.bytes)};
}

void restore_bnw_team_tech_suffix_receipt_checkpoint(
    const saves::CivisSaveSection &section,LiveSessionState &state,
    const BnwTeamTechSuffixCheckpointBinding &expected) {
    binding_shape(expected);
    require(section.id==bnw_team_tech_suffix_receipt_section_id &&
            section.flags==0,"Unsupported BNW suffix callback section");
    require(!state.setup.extensions.find<BnwTeamTechSuffixCallbackMarker>(),
            "BNW suffix callback receipt already restored");
    Reader in{section.bytes};
    const auto found_magic=in.raw(magic.size());
    require(std::equal(found_magic.begin(),found_magic.end(),magic.begin()),
            "Invalid BNW suffix callback receipt magic");
    require(in.u32()==1,"Unsupported BNW suffix callback receipt version");
    in.equal(expected.source.payload_sha256,"BNW suffix source payload changed");
    in.equal(expected.source.rules_sha256,"BNW suffix rules changed");
    in.equal(expected.world_sha256,"BNW suffix world checkpoint changed");
    in.equal(expected.lua_vm_sha256,"BNW suffix Lua VM checkpoint changed");
    in.equal(expected.external_escrow_sha256,
             "BNW suffix external escrow checkpoint changed");
    require(in.u64()==expected.revision,
            "BNW suffix checkpoint revision changed");
    BnwTeamTechSuffixCallbackMarker marker;
    marker.team=in.u32();marker.technology=in.u32();
    marker.ui_query_attempted=in.boolean();
    marker.ui_query_confirmed=in.boolean();
    const auto option=in.u8();
    require(option<=2,"Invalid BNW suffix reward option receipt");
    if(option) marker.no_reward_popups=option==2;
    marker.engine_attempted=in.boolean();
    marker.engine_confirmed=in.boolean();
    marker.uncertain=in.boolean();
    require(in.at==section.bytes.size(),"Trailing BNW suffix receipt bytes");
    marker_shape(marker);
    state.setup.extensions.emplace<BnwTeamTechSuffixCallbackMarker>(marker);
}
} // namespace civ5::runtime
