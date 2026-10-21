#include "civ5/runtime/bnw_unit_jump_checkpoint.hpp"
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
constexpr std::array<std::uint8_t,8> magic{'C','I','V','B','J','M','P','1'};
constexpr std::uint64_t max_values=1'000'000,max_bytes=16'000'000;
void require(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
std::string_view name(const char *value) {
    return value?std::string_view(value):std::string_view();
}
void check_hash(std::string_view hash) {
    require(hash.size()==64 && std::all_of(hash.begin(),hash.end(),[](char c){
        return (c>='0'&&c<='9')||(c>='a'&&c<='f');
    }),"Invalid BNW jump checkpoint SHA-256");
}
struct Writer {
    std::vector<std::uint8_t> bytes;
    void u8(std::uint8_t value) {bytes.push_back(value);}
    void u32(std::uint32_t value) {
        for(unsigned i=0;i<4;++i) u8(static_cast<std::uint8_t>(value>>(8*i)));
    }
    void u64(std::uint64_t value) {
        for(unsigned i=0;i<8;++i) u8(static_cast<std::uint8_t>(value>>(8*i)));
    }
    void i64(std::int64_t value) {u64(std::bit_cast<std::uint64_t>(value));}
    void raw(std::string_view value) {bytes.insert(bytes.end(),value.begin(),value.end());}
    void string(std::string_view value) {
        require(value.size()<=UINT32_MAX,"BNW jump string too long");
        u32(static_cast<std::uint32_t>(value.size()));raw(value);
    }
    void values(const std::vector<std::int64_t> &value) {
        require(value.size()<=max_values,"BNW jump field has too many values");
        u32(static_cast<std::uint32_t>(value.size()));
        for(auto item:value) i64(item);
    }
    void octets(const std::vector<std::uint8_t> &value) {
        require(value.size()<=max_bytes,"BNW jump field has too many bytes");
        u32(static_cast<std::uint32_t>(value.size()));
        bytes.insert(bytes.end(),value.begin(),value.end());
    }
};
struct Reader {
    std::span<const std::uint8_t> bytes;
    std::size_t at=0;
    std::uint8_t u8() {
        require(at<bytes.size(),"Truncated BNW jump checkpoint");
        return bytes[at++];
    }
    std::uint32_t u32() {
        require(at<=bytes.size() && bytes.size()-at>=4,
                "Truncated BNW jump checkpoint");
        std::uint32_t value=0;
        for(unsigned i=0;i<4;++i) value|=std::uint32_t(u8())<<(8*i);
        return value;
    }
    std::uint64_t u64() {
        require(at<=bytes.size() && bytes.size()-at>=8,
                "Truncated BNW jump checkpoint");
        std::uint64_t value=0;
        for(unsigned i=0;i<8;++i) value|=std::uint64_t(u8())<<(8*i);
        return value;
    }
    std::int64_t i64() {return std::bit_cast<std::int64_t>(u64());}
    std::string_view raw(std::size_t count) {
        require(at<=bytes.size() && count<=bytes.size()-at,
                "Truncated BNW jump checkpoint payload");
        auto result=std::string_view(
            reinterpret_cast<const char *>(bytes.data()+at),count);
        at+=count;return result;
    }
    void expect(std::string_view value) {
        require(raw(value.size())==value,"BNW jump source/rules/digest changed");
    }
    std::vector<std::int64_t> values() {
        const auto count=u32();
        require(count<=max_values && count<=(bytes.size()-at)/8,
                "Invalid BNW jump field value count");
        std::vector<std::int64_t> result;
        result.reserve(count);
        for(std::uint32_t i=0;i<count;++i) result.push_back(i64());
        return result;
    }
    std::vector<std::uint8_t> octets() {
        const auto count=u32();
        require(count<=max_bytes && count<=bytes.size()-at,
                "Invalid BNW jump field byte count");
        std::vector<std::uint8_t> result(
            bytes.begin()+at,bytes.begin()+at+count);
        at+=count;return result;
    }
};
void write_node(Writer &out,const saves::UnitTailNode &node,bool schema) {
    if(schema) {
        out.string(name(node.name));out.string(name(node.encoding));
        out.u64(node.start_offset);out.u64(node.end_offset);
        out.u32(static_cast<std::uint32_t>(node.children.size()));
    }
    out.u8(node.wire_count.has_value());
    if(node.wire_count) out.i64(*node.wire_count);
    out.values(node.values);out.octets(node.bytes);
    for(const auto &child:node.children) write_node(out,child,schema);
}
void read_node(Reader &in,saves::UnitTailNode &node) {
    const auto present=in.u8();
    require(present<=1,"Invalid BNW jump node count flag");
    node.wire_count=present?std::optional<std::int64_t>(in.i64()):std::nullopt;
    node.values=in.values();node.bytes=in.octets();
    for(auto &child:node.children) read_node(in,child);
}
bool same_node_schema(const saves::UnitTailNode &a,
                      const saves::UnitTailNode &b) {
    if(name(a.name)!=name(b.name) || name(a.encoding)!=name(b.encoding) ||
       a.start_offset!=b.start_offset || a.end_offset!=b.end_offset ||
       a.children.size()!=b.children.size()) return false;
    for(std::size_t i=0;i<a.children.size();++i)
        if(!same_node_schema(a.children[i],b.children[i])) return false;
    return true;
}
void unit_data(Writer &out,const saves::SavedUnitObservation &unit,bool schema) {
    if(schema) {
        out.u64(unit.index);out.u64(unit.core.start_offset);
        out.u64(unit.core.end_offset);out.u64(unit.core.version.value_or(UINT64_MAX));
        out.u64(unit.tail.start_offset);out.u64(unit.tail.end_offset);
        out.u64(unit.tail.context.bnw_unit_version.value_or(UINT64_MAX));
    }
    out.u32(static_cast<std::uint32_t>(unit.core.fields.size()));
    for(const auto &field:unit.core.fields) {
        if(schema) {
            out.string(name(field.name));out.u32(static_cast<std::uint32_t>(field.encoding));
            out.u64(field.start_offset);out.u64(field.end_offset);
        }
        out.u8(field.wire_count.has_value());
        if(field.wire_count) out.u64(*field.wire_count);
        out.values(field.values);out.octets(field.bytes);
    }
    out.u32(static_cast<std::uint32_t>(unit.tail.blocks.size()));
    for(const auto &node:unit.tail.blocks) write_node(out,node,schema);
}
void read_unit_data(Reader &in,saves::SavedUnitObservation &unit) {
    require(in.u32()==unit.core.fields.size(),
            "BNW jump unit core field count changed");
    for(auto &field:unit.core.fields) {
        const auto present=in.u8();
        require(present<=1,"Invalid BNW jump field count flag");
        field.wire_count=present?std::optional<std::uint64_t>(in.u64()):std::nullopt;
        field.values=in.values();field.bytes=in.octets();
    }
    require(in.u32()==unit.tail.blocks.size(),
            "BNW jump unit tail block count changed");
    for(auto &node:unit.tail.blocks) read_node(in,node);
}
bool same_unit_schema(const saves::SavedUnitObservation &a,
                      const saves::SavedUnitObservation &b) {
    if(a.index!=b.index || a.core.layout!=bnw || b.core.layout!=bnw ||
       a.tail.layout!=bnw || b.tail.layout!=bnw ||
       a.core.start_offset!=b.core.start_offset ||
       a.core.end_offset!=b.core.end_offset ||
       a.core.version!=b.core.version ||
       a.tail.start_offset!=b.tail.start_offset ||
       a.tail.end_offset!=b.tail.end_offset ||
       a.tail.context.bnw_unit_version!=b.tail.context.bnw_unit_version ||
       a.core.fields.size()!=b.core.fields.size() ||
       a.tail.blocks.size()!=b.tail.blocks.size()) return false;
    for(std::size_t i=0;i<a.core.fields.size();++i) {
        const auto &x=a.core.fields[i],&y=b.core.fields[i];
        if(name(x.name)!=name(y.name) || x.encoding!=y.encoding ||
           x.start_offset!=y.start_offset || x.end_offset!=y.end_offset)
            return false;
    }
    for(std::size_t i=0;i<a.tail.blocks.size();++i)
        if(!same_node_schema(a.tail.blocks[i],b.tail.blocks[i])) return false;
    return true;
}
std::int64_t scalar(const saves::SavedUnitObservation &unit,
                    std::string_view wanted) {
    const saves::UnitCoreField *found=nullptr;
    for(const auto &field:unit.core.fields)
        if(name(field.name)==wanted) {
            require(!found,"Duplicate BNW jump unit identity field");found=&field;
        }
    require(found && found->encoding==saves::UnitCoreEncoding::i32 &&
            found->values.size()==1,
            "Missing BNW jump unit identity scalar");
    return found->values[0];
}
const saves::PlotArrayObservation &roster(const saves::PlotObservation &plot) {
    const saves::PlotArrayObservation *found=nullptr;
    for(const auto &array:plot.arrays)
        if(name(array.name)=="units") {
            require(!found,"Duplicate BNW jump plot roster");
            found=&array;
        }
    require(found && found->columns==2 && found->values.size()%2==0,
            "Missing BNW jump plot roster");
    return *found;
}
saves::PlotArrayObservation &roster(saves::PlotObservation &plot) {
    (void)roster(static_cast<const saves::PlotObservation &>(plot));
    for(auto &array:plot.arrays)
        if(name(array.name)=="units") return array;
    throw std::runtime_error("Missing BNW jump plot roster");
}
std::vector<saves::SavedPlotUnitReference> refs(
    const std::vector<std::int64_t> &raw) {
    require(raw.size()%2==0,"Invalid BNW jump roster parity");
    std::vector<saves::SavedPlotUnitReference> result;
    for(std::size_t i=0;i<raw.size();i+=2)
        result.push_back({raw[i],raw[i+1]});
    return result;
}
void header_data(Writer &out,const saves::UnitCollectionHeader &header) {
    out.u32(static_cast<std::uint32_t>(header.layout));
    out.u64(header.start_offset);out.u64(header.end_offset);
    out.u64(header.source_slots);out.i64(header.wire_unit_count);
    out.values(header.next_free_indices);
    out.u32(static_cast<std::uint32_t>(header.fields.size()));
    for(const auto &field:header.fields) {
        out.string(name(field.name));out.i64(field.value);
    }
}
void shape(const saves::SavedSessionSnapshot &world) {
    require(world.game_state.layout==bnw && world.map_header.layout==bnw &&
            world.all_players && world.all_players->layout==bnw &&
            world.map_plots && world.map_plots->layout==bnw &&
            world.map_projection &&
            world.map_plots->plots.size()==world.map_header.grid.size() &&
            world.map_projection->cells.size()==world.map_header.grid.size(),
            "BNW jump requires owned map and unit roster");
    for(std::size_t p=0;p<world.all_players->players.size();++p) {
        const auto &player=world.all_players->players[p];
        require(player.index==p && player.saved_player_id==static_cast<std::int64_t>(p) &&
                player.units.layout==bnw && player.units.header.layout==bnw &&
                player.units.header.wire_unit_count>=0 &&
                static_cast<std::uint64_t>(player.units.header.wire_unit_count)==
                    player.units.units.size(),
                "BNW jump player order changed");
        for(std::size_t u=0;u<player.units.units.size();++u) {
            const auto &unit=player.units.units[u];
            require(unit.index==u && scalar(unit,"m_eOwner")==player.saved_player_id &&
                    scalar(unit,"m_iID")>=0,
                    "BNW jump unit order or owner changed");
        }
    }
    for(std::size_t i=0;i<world.map_header.grid.size();++i) {
        const auto &raw=world.map_plots->plots[i];
        const auto &cell=world.map_projection->cells[i];
        const auto xy=world.map_header.grid.coordinates(i);
        require(raw.index==i && raw.x==xy[0] && raw.y==xy[1] &&
                cell.index==i && cell.x==xy[0] && cell.y==xy[1] &&
                cell.units && *cell.units==refs(roster(raw).values),
                "BNW jump raw/projected plot roster disagrees");
    }
}
std::string state_digest(const saves::SavedSessionSnapshot &world) {
    shape(world);
    Writer out;
    out.u64(world.map_header.grid.width);out.u64(world.map_header.grid.height);
    out.u64(world.all_players->players.size());
    for(const auto &player:world.all_players->players) {
        out.i64(player.saved_player_id);
        header_data(out,player.units.header);
        out.u64(player.units.units.size());
        for(const auto &unit:player.units.units) unit_data(out,unit,true);
    }
    out.u64(world.map_plots->plots.size());
    for(const auto &plot:world.map_plots->plots)
        out.values(roster(plot).values);
    return identify_bytes(out.bytes).sha256;
}
void compare_only_target(const saves::SavedSessionSnapshot &source,
                         const saves::SavedSessionSnapshot &world,
                         BnwUnitJumpCheckpointTarget target) {
    shape(source);shape(world);
    require(source.map_header.grid.width==world.map_header.grid.width &&
            source.map_header.grid.height==world.map_header.grid.height &&
            source.all_players->players.size()==world.all_players->players.size() &&
            target.player_index<source.all_players->players.size() &&
            target.from_plot<source.map_header.grid.size() &&
            target.to_plot<source.map_header.grid.size() &&
            target.from_plot!=target.to_plot,
            "BNW jump map or player roster changed");
    for(std::size_t p=0;p<source.all_players->players.size();++p) {
        const auto &a=source.all_players->players[p],&b=world.all_players->players[p];
        Writer ah,bh;header_data(ah,a.units.header);header_data(bh,b.units.header);
        require(a.saved_player_id==b.saved_player_id &&
                ah.bytes==bh.bytes &&
                a.units.units.size()==b.units.units.size(),
                "BNW jump player unit count changed");
        for(std::size_t u=0;u<a.units.units.size();++u) {
            const auto &x=a.units.units[u],&y=b.units.units[u];
            require(same_unit_schema(x,y),"BNW jump unit schema changed");
            if(p==target.player_index && u==target.unit_index) continue;
            Writer ax,by;unit_data(ax,x,false);unit_data(by,y,false);
            require(ax.bytes==by.bytes,"BNW jump changed another unit");
        }
    }
    const auto &player=source.all_players->players[target.player_index];
    require(target.unit_index<player.units.units.size(),
            "BNW jump unit index is invalid");
    const auto &before=player.units.units[target.unit_index];
    const auto &after=world.all_players->players[target.player_index]
                           .units.units[target.unit_index];
    const auto owner=scalar(before,"m_eOwner"),id=scalar(before,"m_iID");
    require(scalar(after,"m_eOwner")==owner && scalar(after,"m_iID")==id &&
            scalar(before,"m_iX")==
                static_cast<std::int64_t>(source.map_header.grid.coordinates(target.from_plot)[0]) &&
            scalar(before,"m_iY")==
                static_cast<std::int64_t>(source.map_header.grid.coordinates(target.from_plot)[1]) &&
            scalar(after,"m_iX")==
                static_cast<std::int64_t>(world.map_header.grid.coordinates(target.to_plot)[0]) &&
            scalar(after,"m_iY")==
                static_cast<std::int64_t>(world.map_header.grid.coordinates(target.to_plot)[1]),
            "BNW jump selected unit identity or coordinates changed");
    const std::vector<std::int64_t> handle{owner,id};
    for(std::size_t i=0;i<source.map_header.grid.size();++i) {
        const auto &old=roster(source.map_plots->plots[i]).values;
        const auto &now=roster(world.map_plots->plots[i]).values;
        if(i==target.from_plot) require(old==handle && now.empty(),
            "BNW jump source plot must contain only the moved unit");
        else if(i==target.to_plot) require(old.empty() && now==handle,
            "BNW jump destination plot must be empty before move");
        else require(old==now,"BNW jump changed another plot roster");
    }
}
} // namespace

saves::CivisSaveSection make_bnw_unit_jump_checkpoint(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity,
    BnwUnitJumpCheckpointTarget target) {
    check_hash(identity.payload_sha256);check_hash(identity.rules_sha256);
    compare_only_target(source,world,target);
    const auto source_hash=state_digest(source);
    const auto target_hash=state_digest(world);
    Writer out;
    out.bytes.insert(out.bytes.end(),magic.begin(),magic.end());
    out.u32(1);
    out.raw(identity.payload_sha256);out.raw(identity.rules_sha256);
    out.raw(source_hash);out.raw(target_hash);
    out.u64(target.player_index);out.u64(target.unit_index);
    out.u64(target.from_plot);out.u64(target.to_plot);
    unit_data(out,world.all_players->players[target.player_index]
                       .units.units[target.unit_index],false);
    out.values(roster(world.map_plots->plots[target.from_plot]).values);
    out.values(roster(world.map_plots->plots[target.to_plot]).values);
    return {bnw_unit_jump_checkpoint_section_id,0,std::move(out.bytes)};
}

void restore_bnw_unit_jump_checkpoint(
    const saves::CivisSaveSection &section,
    saves::SavedSessionSnapshot &world,
    const SessionCheckpointIdentity &identity) {
    check_hash(identity.payload_sha256);check_hash(identity.rules_sha256);
    require(section.id==bnw_unit_jump_checkpoint_section_id &&
            section.flags==0 && section.bytes.size()>=8+4+64*4+8*4 &&
            std::equal(magic.begin(),magic.end(),section.bytes.begin()),
            "Unsupported BNW jump checkpoint section");
    Reader in{section.bytes,magic.size()};
    require(in.u32()==1,"Unsupported BNW jump checkpoint version");
    in.expect(identity.payload_sha256);in.expect(identity.rules_sha256);
    const auto source_hash=in.raw(64),target_hash=in.raw(64);
    check_hash(source_hash);check_hash(target_hash);
    BnwUnitJumpCheckpointTarget target{
        in.u64(),in.u64(),in.u64(),in.u64()};
    require(source_hash==state_digest(world),
            "BNW jump checkpoint source state digest changed");
    require(target.player_index<world.all_players->players.size() &&
            target.unit_index<
                world.all_players->players[target.player_index].units.units.size() &&
            target.from_plot<world.map_header.grid.size() &&
            target.to_plot<world.map_header.grid.size() &&
            target.from_plot!=target.to_plot,
            "BNW jump checkpoint target is outside source roster");
    auto replacement=world;
    auto &unit=replacement.all_players->players[target.player_index]
                           .units.units[target.unit_index];
    read_unit_data(in,unit);
    roster(replacement.map_plots->plots[target.from_plot]).values=in.values();
    roster(replacement.map_plots->plots[target.to_plot]).values=in.values();
    replacement.map_projection->cells[target.from_plot].units=
        refs(roster(replacement.map_plots->plots[target.from_plot]).values);
    replacement.map_projection->cells[target.to_plot].units=
        refs(roster(replacement.map_plots->plots[target.to_plot]).values);
    require(in.at==section.bytes.size(),
            "BNW jump checkpoint has trailing bytes");
    compare_only_target(world,replacement,target);
    require(target_hash==state_digest(replacement),
            "BNW jump checkpoint target state digest changed");
    world=std::move(replacement);
}
} // namespace civ5::runtime
