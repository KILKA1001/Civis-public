#include "civ5/runtime/bnw_unit_checkpoint.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <climits>
#include <set>
#include <map>
#include "civ5/runtime/file_identity.hpp"
#include <stdexcept>
#include <string_view>
#include <utility>
#include <tuple>

namespace civ5::runtime {
namespace {
constexpr std::array<std::uint8_t,8> magic{'C','I','V','B','U','N','T','1'};
constexpr std::size_t v1_wire_size=8+4+64+64+8*12;
constexpr std::size_t v2_wire_size=v1_wire_size+4*2;
void require(bool ok,const char *why) { if(!ok) throw std::runtime_error(why); }
void check_hash(std::string_view hash) {
    require(hash.size()==64 && std::all_of(hash.begin(),hash.end(),[](char c){
        return (c>='0'&&c<='9')||(c>='a'&&c<='f');
    }),"Invalid BNW unit checkpoint SHA-256");
}
struct Writer {
    std::vector<std::uint8_t> bytes;
    void u32(std::uint32_t value) {
        for(unsigned i=0;i<4;++i) bytes.push_back(static_cast<std::uint8_t>(value>>(8*i)));
    }
    void u64(std::uint64_t value) {
        for(unsigned i=0;i<8;++i) bytes.push_back(static_cast<std::uint8_t>(value>>(8*i)));
    }
    void i64(std::int64_t value) {u64(static_cast<std::uint64_t>(value));}
    void i32(std::int32_t value) {u32(std::bit_cast<std::uint32_t>(value));}
    void hash(std::string_view value) {bytes.insert(bytes.end(),value.begin(),value.end());}
};
struct Reader {
    std::span<const std::uint8_t> bytes;
    std::size_t at=0;
    std::uint32_t u32() {
        require(at<=bytes.size() && bytes.size()-at>=4,"Truncated BNW unit checkpoint");
        std::uint32_t value=0;
        for(unsigned i=0;i<4;++i) value|=std::uint32_t(bytes[at++])<<(8*i);
        return value;
    }
    std::uint64_t u64() {
        require(at<=bytes.size() && bytes.size()-at>=8,"Truncated BNW unit checkpoint");
        std::uint64_t value=0;
        for(unsigned i=0;i<8;++i) value|=std::uint64_t(bytes[at++])<<(8*i);
        return value;
    }
    std::int64_t i64() {return static_cast<std::int64_t>(u64());}
    std::int32_t i32() {return std::bit_cast<std::int32_t>(u32());}
    void hash(std::string_view expected) {
        require(at<=bytes.size() && bytes.size()-at>=expected.size() &&
                std::equal(expected.begin(),expected.end(),bytes.begin()+at),
                "BNW unit checkpoint source payload or rules changed");
        at+=expected.size();
    }
};
std::string_view name(const char *value) {return value?std::string_view(value):std::string_view();}
const saves::UnitCoreField &core_field(const saves::SavedUnitObservation &unit,
                                      std::string_view wanted) {
    const saves::UnitCoreField *found=nullptr;
    for(const auto &field:unit.core.fields)
        if(name(field.name)==wanted) {
            require(!found,"Duplicate BNW unit core field");found=&field;
        }
    require(found && found->encoding==saves::UnitCoreEncoding::i32 &&
            found->values.size()==1,"Missing BNW unit core scalar");
    return *found;
}
std::int64_t core_value(const saves::SavedUnitObservation &unit,std::string_view wanted) {
    return core_field(unit,wanted).values.front();
}
void set_core_value(saves::SavedUnitObservation &unit,std::string_view wanted,
                    std::int64_t value) {
    require(value>=INT32_MIN && value<=INT32_MAX,"BNW unit core scalar exceeds i32 wire range");
    (void)core_field(unit,wanted);
    for(auto &field:unit.core.fields)
        if(name(field.name)==wanted) {field.values.front()=value;return;}
}
bool same_tail_node(const saves::UnitTailNode &a,const saves::UnitTailNode &b) {
    if(name(a.name)!=name(b.name) || name(a.encoding)!=name(b.encoding) ||
       a.start_offset!=b.start_offset || a.end_offset!=b.end_offset ||
       a.wire_count!=b.wire_count || a.values!=b.values || a.bytes!=b.bytes ||
       a.children.size()!=b.children.size()) return false;
    for(std::size_t i=0;i<a.children.size();++i)
        if(!same_tail_node(a.children[i],b.children[i])) return false;
    return true;
}
void same_unit_except_position_moves(const saves::SavedUnitObservation &a,
                                     const saves::SavedUnitObservation &b,
                                     bool permit_moves) {
    require(a.index==b.index && a.core.layout==b.core.layout &&
            a.core.start_offset==b.core.start_offset &&
            a.core.end_offset==b.core.end_offset && a.core.version==b.core.version &&
            a.core.fields.size()==b.core.fields.size() &&
            a.tail.layout==b.tail.layout &&
            a.tail.context.bnw_unit_version==b.tail.context.bnw_unit_version &&
            a.tail.start_offset==b.tail.start_offset &&
            a.tail.end_offset==b.tail.end_offset &&
            a.tail.blocks.size()==b.tail.blocks.size(),
            "BNW unit identity, version or tail shape changed");
    for(std::size_t i=0;i<a.core.fields.size();++i) {
        const auto &x=a.core.fields[i],&y=b.core.fields[i];
        const auto field_name=name(x.name);
        require(field_name==name(y.name) && x.encoding==y.encoding &&
                x.start_offset==y.start_offset && x.end_offset==y.end_offset &&
                x.wire_count==y.wire_count && x.bytes==y.bytes &&
                ((field_name=="m_iX" || field_name=="m_iY" ||
                  (permit_moves && field_name=="m_iMoves")) || x.values==y.values),
                "BNW unit checkpoint has unsupported core mutation");
    }
    for(std::size_t i=0;i<a.tail.blocks.size();++i)
        require(same_tail_node(a.tail.blocks[i],b.tail.blocks[i]),
                "BNW unit checkpoint has unsupported tail mutation");
}
void same_header(const saves::UnitsObservation &a,const saves::UnitsObservation &b) {
    require(a.layout==saves::GameStateLayout::expansion2_version1 &&
            b.layout==a.layout && a.start_offset==b.start_offset &&
            a.end_offset==b.end_offset && a.header.layout==b.header.layout &&
            a.header.start_offset==b.header.start_offset &&
            a.header.end_offset==b.header.end_offset &&
            a.header.source_slots==b.header.source_slots &&
            a.header.next_free_indices==b.header.next_free_indices &&
            a.header.wire_unit_count==b.header.wire_unit_count &&
            a.header.fields.size()==b.header.fields.size() &&
            a.units.size()==b.units.size(),
            "BNW unit collection or roster changed");
    for(std::size_t i=0;i<a.header.fields.size();++i)
        require(name(a.header.fields[i].name)==name(b.header.fields[i].name) &&
                a.header.fields[i].value==b.header.fields[i].value,
                "BNW unit collection header changed");
}
const saves::PlotArrayObservation &plot_units(const saves::PlotObservation &plot) {
    const saves::PlotArrayObservation *found=nullptr;
    for(const auto &array:plot.arrays)
        if(name(array.name)=="units") {
            require(!found,"Duplicate BNW plot unit roster");found=&array;
        }
    require(found && found->columns==2 && found->values.size()%2==0,
            "Missing or invalid BNW plot unit roster");
    return *found;
}
saves::PlotArrayObservation &mutable_plot_units(saves::PlotObservation &plot) {
    (void)plot_units(plot);
    for(auto &array:plot.arrays)
        if(name(array.name)=="units") return array;
    throw std::runtime_error("Missing BNW plot unit roster");
}
std::vector<saves::SavedPlotUnitReference> references(const saves::PlotObservation &plot) {
    const auto &values=plot_units(plot).values;
    std::vector<saves::SavedPlotUnitReference> result;
    result.reserve(values.size()/2);
    for(std::size_t i=0;i<values.size();i+=2)
        result.push_back({values[i],values[i+1]});
    return result;
}
void map_shape(const saves::SavedSessionSnapshot &world) {
    const auto bnw=saves::GameStateLayout::expansion2_version1;
    require(world.game_state.layout==bnw && world.map_header.layout==bnw &&
            world.all_players && world.all_players->layout==bnw &&
            world.map_plots && world.map_plots->layout==bnw &&
            world.map_projection &&
            world.map_plots->plots.size()==world.map_header.grid.size() &&
            world.map_projection->cells.size()==world.map_header.grid.size(),
            "BNW unit checkpoint requires complete owned players and map");
    for(std::size_t i=0;i<world.map_plots->plots.size();++i) {
        const auto &plot=world.map_plots->plots[i];
        const auto &cell=world.map_projection->cells[i];
        const auto xy=world.map_header.grid.coordinates(i);
        require(plot.index==i && plot.x==xy[0] && plot.y==xy[1] &&
                cell.index==i && cell.x==xy[0] && cell.y==xy[1] &&
                cell.units && *cell.units==references(plot),
                "BNW unit checkpoint raw/projected plot roster disagrees");
    }
}
std::uint64_t index_for(const saves::SavedSessionSnapshot &world,
                        std::int64_t x,std::int64_t y) {
    require(x>=0 && y>=0 &&
            static_cast<std::uint64_t>(x)<world.map_header.grid.width &&
            static_cast<std::uint64_t>(y)<world.map_header.grid.height,
            "BNW unit checkpoint coordinate is outside map");
    return world.map_header.grid.index(x,y);
}
struct Move {
    std::uint64_t player_index=0,unit_index=0,old_plot=0,new_plot=0;
    std::int64_t owner=0,id=0,old_x=0,old_y=0,new_x=0,new_y=0;
    std::optional<std::int32_t> old_moves,new_moves;
};
Move inspect_move(const saves::SavedSessionSnapshot &source,
                  const saves::SavedSessionSnapshot &world,bool permit_moves=true) {
    map_shape(source);map_shape(world);
    require(source.map_header.grid.width==world.map_header.grid.width &&
            source.map_header.grid.height==world.map_header.grid.height &&
            source.all_players->players.size()==world.all_players->players.size(),
            "BNW unit checkpoint source map or players changed");
    std::optional<Move> moved;
    std::set<std::pair<std::int64_t,std::int64_t>> unit_ids;
    for(std::size_t p=0;p<source.all_players->players.size();++p) {
        const auto &a=source.all_players->players[p],&b=world.all_players->players[p];
        require(a.index==b.index && a.saved_player_id==b.saved_player_id &&
                a.start_offset==b.start_offset && a.end_offset==b.end_offset,
                "BNW unit checkpoint player identity changed");
        same_header(a.units,b.units);
        for(std::size_t u=0;u<a.units.units.size();++u) {
            const auto &old=a.units.units[u],&now=b.units.units[u];
            same_unit_except_position_moves(old,now,permit_moves);
            const auto owner=core_value(old,"m_eOwner"),id=core_value(old,"m_iID");
            require(owner==a.saved_player_id && owner>=0 && id>=0 &&
                    unit_ids.insert({owner,id}).second &&
                    core_value(now,"m_eOwner")==owner && core_value(now,"m_iID")==id,
                    "BNW unit owner or ID changed or duplicated");
            const auto x=core_value(old,"m_iX"),y=core_value(old,"m_iY");
            const auto nx=core_value(now,"m_iX"),ny=core_value(now,"m_iY");
            std::optional<std::int32_t> old_moves,new_moves;
            if(permit_moves) {
                const auto a_moves=core_value(old,"m_iMoves"),b_moves=core_value(now,"m_iMoves");
                require(a_moves>=INT32_MIN && a_moves<=INT32_MAX &&
                        b_moves>=INT32_MIN && b_moves<=INT32_MAX,
                        "BNW unit moves exceeds i32 wire range");
                old_moves=static_cast<std::int32_t>(a_moves);
                new_moves=static_cast<std::int32_t>(b_moves);
            }
            if(x==nx && y==ny && old_moves==new_moves) continue;
            require(!moved,"BNW unit checkpoint supports only one changed unit");
            moved=Move{p,u,0,0,owner,id,x,y,nx,ny,old_moves,new_moves};
        }
    }
    require(moved.has_value(),"BNW unit checkpoint has no relocation or moves change");
    auto &move=*moved;
    move.old_plot=index_for(source,move.old_x,move.old_y);
    move.new_plot=index_for(world,move.new_x,move.new_y);
    const std::vector<std::int64_t> handle{move.owner,move.id};
    std::size_t home_occurrences=0;
    for(std::size_t i=0;i<source.map_plots->plots.size();++i) {
        const auto &a=plot_units(source.map_plots->plots[i]).values;
        const auto &b=plot_units(world.map_plots->plots[i]).values;
        if(move.old_plot==move.new_plot) {
            require(a==b,"BNW moves-only checkpoint has a plot roster mutation");
            for(std::size_t j=0;j<a.size();j+=2)
                if(a[j]==move.owner && a[j+1]==move.id) {
                    require(i==move.old_plot,"BNW moves-only handle occurs off its home plot");
                    ++home_occurrences;
                }
        } else if(i==move.old_plot) require(a==handle && b.empty(),
            "BNW unit checkpoint source plot must contain only the moved unit");
        else if(i==move.new_plot) require(a.empty() && b==handle,
            "BNW unit checkpoint destination plot must be empty before move");
        else {
            require(a==b,"BNW unit checkpoint has an unsupported plot roster mutation");
            for(std::size_t j=0;j<a.size();j+=2)
                require(a[j]!=move.owner || a[j+1]!=move.id,
                        "BNW unit checkpoint moved handle occurs on another plot");
        }
    }
    if(move.old_plot==move.new_plot)
        require(home_occurrences==1,"BNW moves-only home handle missing or duplicated");
    return move;
}
void apply_move(saves::SavedSessionSnapshot &world,const Move &move) {
    auto &unit=world.all_players->players.at(move.player_index).units.units.at(move.unit_index);
    set_core_value(unit,"m_iX",move.new_x);
    set_core_value(unit,"m_iY",move.new_y);
    if(move.new_moves) set_core_value(unit,"m_iMoves",*move.new_moves);
    if(move.old_plot==move.new_plot) return;
    mutable_plot_units(world.map_plots->plots.at(move.old_plot)).values.clear();
    mutable_plot_units(world.map_plots->plots.at(move.new_plot)).values={move.owner,move.id};
    world.map_projection->cells.at(move.old_plot).units=std::vector<saves::SavedPlotUnitReference>{};
    world.map_projection->cells.at(move.new_plot).units=
        std::vector<saves::SavedPlotUnitReference>{{move.owner,move.id}};
}
Move read_move(const saves::CivisSaveSection &section,
               const saves::SavedSessionSnapshot &world,
               const SessionCheckpointIdentity &identity) {
    require(section.id==bnw_unit_checkpoint_section_id && section.flags==0 &&
            (section.bytes.size()==v1_wire_size || section.bytes.size()==v2_wire_size) &&
            std::equal(magic.begin(),magic.end(),section.bytes.begin()),
            "Unsupported BNW unit checkpoint section");
    Reader in{section.bytes,magic.size()};
    const auto version=in.u32();
    require((version==1 && section.bytes.size()==v1_wire_size) ||
            (version==2 && section.bytes.size()==v2_wire_size),
            "Unsupported BNW unit checkpoint version or length");
    in.hash(identity.payload_sha256);in.hash(identity.rules_sha256);
    const auto width=in.u64(),height=in.u64();
    const auto player_count=in.u64(),unit_count=in.u64();
    Move move;
    move.player_index=in.u64();move.unit_index=in.u64();
    move.owner=in.i64();move.id=in.i64();
    move.old_x=in.i64();move.old_y=in.i64();
    move.new_x=in.i64();move.new_y=in.i64();
    if(version==2) {move.old_moves=in.i32();move.new_moves=in.i32();}
    require(in.at==section.bytes.size() &&
            width==world.map_header.grid.width && height==world.map_header.grid.height &&
            world.all_players && player_count==world.all_players->players.size() &&
            move.player_index<player_count &&
            unit_count==world.all_players->players[move.player_index].units.units.size() &&
            move.unit_index<unit_count,
            "BNW unit checkpoint dimensions or unit IDs changed");
    move.old_plot=index_for(world,move.old_x,move.old_y);
    move.new_plot=index_for(world,move.new_x,move.new_y);
    return move;
}
constexpr std::size_t v3_header_size=8+4+64+64+64+8*4;
constexpr std::size_t v3_record_size=8*7+4*2;
using UnitKey=std::pair<std::int64_t,std::int64_t>;
std::string batch_source_digest(const saves::SavedSessionSnapshot &world) {
    map_shape(world);
    Writer out;
    const auto text=[&](const char *value) {
        out.u64(value!=nullptr);const auto bytes=name(value);out.u64(bytes.size());
        out.bytes.insert(out.bytes.end(),bytes.begin(),bytes.end());
    };
    const auto values=[&](const auto &items) {
        out.u64(items.size());for(const auto v:items)out.i64(v);
    };
    const auto bytes=[&](const auto &items) {
        out.u64(items.size());out.bytes.insert(out.bytes.end(),items.begin(),items.end());
    };
    const auto optional=[&](const auto &value){out.u64(value.has_value());if(value)out.u64(static_cast<std::uint64_t>(*value));};
    const auto tail=[&](auto &&self,const saves::UnitTailNode &node)->void {
        text(node.name);text(node.encoding);out.u64(node.start_offset);out.u64(node.end_offset);
        optional(node.wire_count);values(node.values);bytes(node.bytes);out.u64(node.children.size());
        for(const auto &child:node.children)self(self,child);
    };
    out.u64(world.map_header.grid.width);out.u64(world.map_header.grid.height);
    out.u64(world.all_players->players.size());
    for(const auto &player:world.all_players->players) {
        out.u64(player.index);out.i64(player.saved_player_id);out.u64(player.start_offset);out.u64(player.end_offset);
        const auto &units=player.units;const auto &h=units.header;
        out.u64(static_cast<std::uint64_t>(units.layout));out.u64(units.start_offset);out.u64(units.end_offset);
        out.u64(static_cast<std::uint64_t>(h.layout));out.u64(h.start_offset);out.u64(h.end_offset);
        out.u64(h.source_slots);out.i64(h.wire_unit_count);values(h.next_free_indices);out.u64(h.fields.size());
        for(const auto &field:h.fields){text(field.name);out.i64(field.value);}
        out.u64(units.units.size());
        for(const auto &unit:units.units) {
            out.u64(unit.index);const auto &core=unit.core;
            out.u64(static_cast<std::uint64_t>(core.layout));out.u64(core.start_offset);out.u64(core.end_offset);optional(core.version);
            out.u64(core.fields.size());
            for(const auto &field:core.fields) {
                text(field.name);out.u64(static_cast<std::uint64_t>(field.encoding));out.u64(field.start_offset);out.u64(field.end_offset);
                optional(field.wire_count);values(field.values);bytes(field.bytes);
            }
            out.u64(static_cast<std::uint64_t>(unit.tail.layout));optional(unit.tail.context.bnw_unit_version);
            out.u64(unit.tail.start_offset);out.u64(unit.tail.end_offset);out.u64(unit.tail.blocks.size());
            for(const auto &node:unit.tail.blocks)tail(tail,node);
        }
    }
    out.u64(world.map_plots->plots.size());
    for(const auto &plot:world.map_plots->plots)values(plot_units(plot).values);
    return identify_bytes(out.bytes).sha256;
}
void validate_moves_homes(const saves::SavedSessionSnapshot &source,
                          const std::vector<Move> &changes) {
    // Native full IDs are never reduced to a slot number or synthesized from
    // collection order. Stacked unchanged units are supported; duplicates of a
    // changed handle anywhere in the map are rejected.
    std::map<UnitKey,std::pair<std::uint64_t,std::size_t>> locations;
    for(const auto &change:changes)
        require(locations.emplace(UnitKey{change.owner,change.id},
                                  std::pair<std::uint64_t,std::size_t>{0,0}).second,
                "BNW moves batch duplicates a changed full ID");
    for(std::size_t i=0;i<source.map_plots->plots.size();++i) {
        const auto &handles=plot_units(source.map_plots->plots[i]).values;
        for(std::size_t j=0;j<handles.size();j+=2) {
            const auto found=locations.find({handles[j],handles[j+1]});
            if(found!=locations.end()) {found->second.first=i;++found->second.second;}
        }
    }
    for(const auto &change:changes) {
        const auto &location=locations.at({change.owner,change.id});
        require(location.second==1 && location.first==change.old_plot,
                "BNW moves batch full ID is missing, duplicated or off its home plot");
    }
}
std::vector<Move> inspect_moves_only(const saves::SavedSessionSnapshot &source,
                                   const saves::SavedSessionSnapshot &world,
                                   bool require_change=true) {
    map_shape(source);map_shape(world);
    require(source.map_header.grid.width==world.map_header.grid.width &&
            source.map_header.grid.height==world.map_header.grid.height &&
            source.all_players->players.size()==world.all_players->players.size(),
            "BNW moves batch map or player dimensions changed");
    for(std::size_t i=0;i<source.map_plots->plots.size();++i)
        require(plot_units(source.map_plots->plots[i]).values==
                    plot_units(world.map_plots->plots[i]).values,
                "BNW moves batch plot roster changed");
    std::vector<Move> changes;
    std::set<UnitKey> ids;
    for(std::size_t p=0;p<source.all_players->players.size();++p) {
        const auto &a=source.all_players->players[p],&b=world.all_players->players[p];
        require(a.index==b.index && a.saved_player_id==b.saved_player_id &&
                a.start_offset==b.start_offset && a.end_offset==b.end_offset,
                "BNW moves batch player identity changed");
        same_header(a.units,b.units);
        for(std::size_t u=0;u<a.units.units.size();++u) {
            const auto &old=a.units.units[u],&now=b.units.units[u];
            same_unit_except_position_moves(old,now,true);
            const auto owner=core_value(old,"m_eOwner"),id=core_value(old,"m_iID");
            const auto x=core_value(old,"m_iX"),y=core_value(old,"m_iY");
            require(owner==a.saved_player_id && owner>=0 && owner<=INT32_MAX &&
                    id>=0 && id<=INT32_MAX && x>=INT32_MIN && x<=INT32_MAX &&
                    y>=INT32_MIN && y<=INT32_MAX &&
                    ids.emplace(owner,id).second && core_value(now,"m_eOwner")==owner &&
                    core_value(now,"m_iID")==id && core_value(now,"m_iX")==x &&
                    core_value(now,"m_iY")==y,
                    "BNW moves batch identity or coordinates changed");
            const auto before=core_value(old,"m_iMoves"),after=core_value(now,"m_iMoves");
            require(before>=INT32_MIN && before<=INT32_MAX &&
                    after>=INT32_MIN && after<=INT32_MAX,
                    "BNW moves batch scalar exceeds native i32");
            if(before==after)continue;
            const auto plot=index_for(source,x,y);
            changes.push_back({p,u,plot,plot,owner,id,x,y,x,y,
                              static_cast<std::int32_t>(before),static_cast<std::int32_t>(after)});
        }
    }
    require(!require_change || !changes.empty(),"BNW moves batch has no change");
    validate_moves_homes(source,changes);
    return changes;
}
bool batch_version(const saves::CivisSaveSection &section) {
    return section.bytes.size()>=12 && section.bytes[8]==3 &&
           section.bytes[9]==0 && section.bytes[10]==0 && section.bytes[11]==0;
}
std::vector<Move> read_moves_batch(const saves::CivisSaveSection &section,
                                 const saves::SavedSessionSnapshot &world,
                                 const SessionCheckpointIdentity &identity) {
    require(section.id==bnw_unit_checkpoint_section_id && section.flags==0 &&
            section.bytes.size()>=v3_header_size &&
            std::equal(magic.begin(),magic.end(),section.bytes.begin()),
            "Unsupported BNW moves batch section");
    Reader in{section.bytes,magic.size()};
    require(in.u32()==3,"Unsupported BNW moves batch version");
    in.hash(identity.payload_sha256);in.hash(identity.rules_sha256);
    in.hash(batch_source_digest(world));
    const auto width=in.u64(),height=in.u64(),players=in.u64(),count=in.u64();
    require(width==world.map_header.grid.width && height==world.map_header.grid.height &&
            world.all_players && players==world.all_players->players.size() && count>0 &&
            (section.bytes.size()-in.at)%v3_record_size==0 &&
            count==(section.bytes.size()-in.at)/v3_record_size,
            "BNW moves batch dimensions, count or physical length changed");
    // Validate the complete source unit shapes before constructing any target.
    (void)inspect_moves_only(world,world,false);
    std::vector<Move> changes;changes.reserve(static_cast<std::size_t>(count));
    std::optional<std::pair<std::uint64_t,std::uint64_t>> previous;
    for(std::uint64_t i=0;i<count;++i) {
        Move move;move.player_index=in.u64();move.unit_index=in.u64();
        const auto unit_count=in.u64();move.owner=in.i64();move.id=in.i64();
        move.old_x=in.i64();move.old_y=in.i64();move.new_x=move.old_x;move.new_y=move.old_y;
        move.old_moves=in.i32();move.new_moves=in.i32();
        const auto order=std::pair{move.player_index,move.unit_index};
        require((!previous || *previous<order) && move.player_index<players,
                "BNW moves batch order or player index changed");
        previous=order;
        const auto &player=world.all_players->players[move.player_index];
        require(unit_count==player.units.units.size() && move.unit_index<unit_count,
                "BNW moves batch unit collection changed");
        const auto &unit=player.units.units[move.unit_index];
        require(player.saved_player_id==move.owner && core_value(unit,"m_eOwner")==move.owner &&
                core_value(unit,"m_iID")==move.id && core_value(unit,"m_iX")==move.old_x &&
                core_value(unit,"m_iY")==move.old_y && core_value(unit,"m_iMoves")==*move.old_moves &&
                move.old_moves!=move.new_moves,
                "BNW moves batch source identity or old moves changed");
        move.old_plot=index_for(world,move.old_x,move.old_y);move.new_plot=move.old_plot;
        changes.push_back(move);
    }
    require(in.at==section.bytes.size(),"Trailing BNW moves batch bytes");
    validate_moves_homes(world,changes);
    return changes;
}
template<class P>auto&cycle_block(P&player){
    require(player.administration.layout==saves::GameStateLayout::expansion2_version1,"BNW cycle administration layout unavailable");
    decltype(&player.administration.blocks.front()) found=nullptr;
    for(auto&b:player.administration.blocks)if(name(b.name)=="pre_city"){
        require(!found,"Duplicate BNW pre_city cycle block");found=&b;
    }
    require(found,"Missing BNW pre_city cycle block");return *found;
}
template<class P>auto&cycle_array(P&player){
    auto&b=cycle_block(player);decltype(&b.arrays.front()) found=nullptr;
    for(auto&a:b.arrays)if(name(a.name)=="m_UnitCycle"){
        require(!found,"Duplicate BNW cycle list");found=&a;
    }
    require(found&&found->columns==1,"Missing or malformed BNW cycle list");return *found;
}
template<class P>auto&cycle_count(P&player){
    auto&b=cycle_block(player);decltype(&b.fields.front()) found=nullptr;
    for(auto&f:b.fields)if(name(f.name)=="m_UnitCycle_wire_count"){
        require(!found,"Duplicate BNW cycle count");found=&f;
    }
    require(found,"Missing BNW cycle count");return *found;
}
void cycle_shape(const saves::SavedSessionSnapshot&world){
    map_shape(world);
    for(const auto&p:world.all_players->players){
        const auto&a=cycle_array(p);const auto count=cycle_count(p).value;
        require(count>=0&&count<=INT32_MAX&&static_cast<std::uint64_t>(count)==a.values.size(),
                "BNW cycle signed wire count disagrees with physical list");
        std::set<std::int64_t>ids;
        for(const auto id:a.values){
            require(id>=0&&id<=INT32_MAX&&ids.insert(id).second,"BNW cycle full ID duplicate/out of range");
            std::size_t matches=0;
            for(const auto&u:p.units.units)if(core_value(u,"m_iID")==id&&core_value(u,"m_eOwner")==p.saved_player_id)++matches;
            require(matches==1,"BNW cycle full ID lacks exact current imported unit");
        }
    }
}
std::string cycle_source_digest(const saves::SavedSessionSnapshot&world){
    cycle_shape(world);Writer out;out.hash(batch_source_digest(world));
    const auto text=[&](const char*n){out.u64(n!=nullptr);const auto v=name(n);out.u64(v.size());out.bytes.insert(out.bytes.end(),v.begin(),v.end());};
    const auto record=[&](auto&&self,const saves::PlayerAdministrationRecord&r)->void{
        text(r.name);out.u64(r.index);out.u64(r.start_offset);out.u64(r.end_offset);
        out.u64(r.fields.size());for(const auto&f:r.fields){text(f.name);out.i64(f.value);}
        out.u64(r.arrays.size());for(const auto&a:r.arrays){text(a.name);out.u64(a.columns);out.u64(a.values.size());for(auto v:a.values)out.i64(v);}
        out.u64(r.blobs.size());for(const auto&b:r.blobs){text(b.name);out.u64(b.bytes.size());out.bytes.insert(out.bytes.end(),b.bytes.begin(),b.bytes.end());}
        out.u64(r.children.size());for(const auto&child:r.children)self(self,child);
    };
    for(const auto&p:world.all_players->players){
        const auto&a=p.administration;out.u64(static_cast<std::uint64_t>(a.layout));out.u64(a.start_offset);out.u64(a.end_offset);
        out.u64(a.blocks.size());for(const auto&b:a.blocks)record(record,b);
    }
    return identify_bytes(out.bytes).sha256;
}
std::vector<std::size_t>inspect_cycles(const saves::SavedSessionSnapshot&source,
    const saves::SavedSessionSnapshot&world,bool changed){
    cycle_shape(source);cycle_shape(world);
    require(batch_source_digest(source)==batch_source_digest(world),"BNW cycle checkpoint changed unit/FLTA/map source");
    auto masked=world;std::vector<std::size_t>result;
    for(std::size_t p=0;p<source.all_players->players.size();++p){
        const auto&old=source.all_players->players[p];const auto&now=world.all_players->players[p];auto&target=masked.all_players->players[p];
        if(cycle_array(old).values!=cycle_array(now).values)result.push_back(p);
        cycle_array(target).values=cycle_array(old).values;cycle_count(target).value=cycle_count(old).value;
    }
    require(cycle_source_digest(source)==cycle_source_digest(masked),"BNW cycle checkpoint changed other administration metadata/state");
    require(!changed||!result.empty(),"BNW cycle checkpoint has no list change");return result;
}
bool cycle_version(const saves::CivisSaveSection&s){
    return s.bytes.size()>=12&&s.bytes[8]==5&&s.bytes[9]==0&&s.bytes[10]==0&&s.bytes[11]==0;
}
saves::SavedSessionSnapshot read_cycle_replacement(const saves::CivisSaveSection&section,
    const saves::SavedSessionSnapshot&world,const SessionCheckpointIdentity&identity){
    require(section.id==bnw_unit_checkpoint_section_id&&section.flags==0&&section.bytes.size()>=8+4+64*3+8&&
        std::equal(magic.begin(),magic.end(),section.bytes.begin()),"Unsupported BNW cycle checkpoint");
    Reader in{section.bytes,magic.size()};require(in.u32()==5,"Unsupported BNW cycle checkpoint version");
    in.hash(identity.payload_sha256);in.hash(identity.rules_sha256);in.hash(cycle_source_digest(world));
    const auto count=in.u64();require(count>0&&count<=world.all_players->players.size(),"BNW cycle player count invalid");
    auto replacement=world;std::optional<std::uint64_t>previous;
    for(std::uint64_t i=0;i<count;++i){
        const auto p=in.u64();require(p<world.all_players->players.size()&&(!previous||*previous<p),"BNW cycle players not in source order");previous=p;
        const auto&before=cycle_array(world.all_players->players[p]);const auto old_count=in.u64();
        require(old_count==before.values.size()&&old_count<=(section.bytes.size()-in.at)/4,"BNW cycle old list length changed");
        for(std::uint64_t j=0;j<old_count;++j)require(in.i32()==before.values[j],"BNW cycle old full ID changed");
        const auto after_count=in.u64();require(after_count<=INT32_MAX&&after_count<=(section.bytes.size()-in.at)/4,"BNW cycle new list exceeds physical bytes");
        std::vector<std::int64_t>after;after.reserve(static_cast<std::size_t>(after_count));
        for(std::uint64_t j=0;j<after_count;++j)after.push_back(in.i32());
        require(after!=before.values,"BNW cycle redundant unchanged list record");
        auto&target=replacement.all_players->players[p];cycle_array(target).values=std::move(after);cycle_count(target).value=static_cast<std::int64_t>(after_count);
    }
    require(in.at==section.bytes.size(),"Trailing BNW cycle checkpoint bytes");
    (void)inspect_cycles(world,replacement,true);return replacement;
}

constexpr std::size_t v4_record_size=8*6+4*3;
struct ScalarChange {
    std::uint64_t player=0,unit=0,unit_count=0,field=0;
    std::int64_t owner=0,id=0;
    saves::UnitCoreEncoding encoding=saves::UnitCoreEncoding::i32;
    std::int32_t before=0,after=0;
};
bool scalar_mutable(const saves::UnitCoreField &field) {
    const auto n=name(field.name);
    return !n.empty() && n!="m_iID" && n!="m_eOwner" && n!="m_iX" && n!="m_iY" &&
           (field.encoding==saves::UnitCoreEncoding::i32 ||
            field.encoding==saves::UnitCoreEncoding::boolean) &&
           field.values.size()==1 && field.bytes.empty();
}
void scalar_range(const saves::UnitCoreField &field,std::int64_t value) {
    require(value>=INT32_MIN && value<=INT32_MAX &&
            (field.encoding!=saves::UnitCoreEncoding::boolean || value==0 || value==1),
            "BNW unit scalar checkpoint exceeds native field range");
}
std::vector<ScalarChange> inspect_scalars(const saves::SavedSessionSnapshot &source,
    const saves::SavedSessionSnapshot &world,bool changed=true) {
    // Reuse source-shaped collection/full-ID/coordinate admission, without
    // granting a moves change or any source-independent unit identity.
    (void)inspect_moves_only(source,source,false);
    map_shape(world);
    require(source.map_header.grid.width==world.map_header.grid.width &&
            source.map_header.grid.height==world.map_header.grid.height &&
            source.all_players->players.size()==world.all_players->players.size(),
            "BNW scalar checkpoint dimensions changed");
    for(std::size_t i=0;i<source.map_plots->plots.size();++i)
        require(plot_units(source.map_plots->plots[i]).values==plot_units(world.map_plots->plots[i]).values,
                "BNW scalar checkpoint plot roster changed");
    std::vector<ScalarChange> result;std::vector<Move> homes;
    for(std::size_t p=0;p<source.all_players->players.size();++p){
        const auto &a=source.all_players->players[p],&b=world.all_players->players[p];
        require(a.index==b.index && a.saved_player_id==b.saved_player_id &&
                a.start_offset==b.start_offset && a.end_offset==b.end_offset,
                "BNW scalar checkpoint player identity changed");
        same_header(a.units,b.units);
        for(std::size_t u=0;u<a.units.units.size();++u){
            const auto &before=a.units.units[u],&after=b.units.units[u];
            auto masked=after;
            require(before.core.fields.size()==after.core.fields.size(),"BNW scalar checkpoint core shape changed");
            std::set<std::string_view> fields;
            const auto owner=core_value(before,"m_eOwner"),id=core_value(before,"m_iID");
            bool has_change=false;
            for(std::size_t f=0;f<before.core.fields.size();++f){
                const auto &x=before.core.fields[f],&y=after.core.fields[f];
                require(!name(x.name).empty() && fields.insert(name(x.name)).second,"BNW scalar checkpoint duplicate field");
                if(!scalar_mutable(x))continue;
                require(scalar_mutable(y),"BNW scalar checkpoint mutable field shape changed");
                scalar_range(x,x.values.front());scalar_range(x,y.values.front());
                masked.core.fields[f].values=x.values;
                if(x.values==y.values)continue;
                result.push_back({p,u,a.units.units.size(),f,owner,id,x.encoding,
                    static_cast<std::int32_t>(x.values.front()),static_cast<std::int32_t>(y.values.front())});
                has_change=true;
            }
            // All metadata/identity/type/vector/tail values must match after
            // masking ONLY the proven typed scalar payload slots above.
            same_unit_except_position_moves(before,masked,false);
            require(core_value(before,"m_iX")==core_value(after,"m_iX") &&
                    core_value(before,"m_iY")==core_value(after,"m_iY"),"BNW scalar checkpoint relocated unit");
            if(has_change){const auto x=core_value(before,"m_iX"),y=core_value(before,"m_iY");
                const auto plot=index_for(source,x,y);homes.push_back({p,u,plot,plot,owner,id,x,y,x,y,{},{}});}
        }
    }
    require(!changed || !result.empty(),"BNW scalar checkpoint has no change");
    validate_moves_homes(source,homes);return result;
}
bool scalar_version(const saves::CivisSaveSection &section) {
    return section.bytes.size()>=12 && section.bytes[8]==4 && section.bytes[9]==0 &&
           section.bytes[10]==0 && section.bytes[11]==0;
}
std::vector<ScalarChange> read_scalar_batch(const saves::CivisSaveSection &section,
    const saves::SavedSessionSnapshot &world,const SessionCheckpointIdentity &identity) {
    require(section.id==bnw_unit_checkpoint_section_id && section.flags==0 &&
            section.bytes.size()>=v3_header_size && std::equal(magic.begin(),magic.end(),section.bytes.begin()),
            "Unsupported BNW scalar checkpoint section");
    Reader in{section.bytes,magic.size()};require(in.u32()==4,"Unsupported BNW scalar checkpoint version");
    in.hash(identity.payload_sha256);in.hash(identity.rules_sha256);in.hash(batch_source_digest(world));
    const auto width=in.u64(),height=in.u64(),players=in.u64(),count=in.u64();
    require(world.all_players && width==world.map_header.grid.width && height==world.map_header.grid.height &&
            players==world.all_players->players.size() && count>0 &&
            (section.bytes.size()-in.at)%v4_record_size==0 && count==(section.bytes.size()-in.at)/v4_record_size,
            "BNW scalar checkpoint dimensions, count or length changed");
    (void)inspect_scalars(world,world,false);
    std::vector<ScalarChange> result;result.reserve(static_cast<std::size_t>(count));
    std::optional<std::tuple<std::uint64_t,std::uint64_t,std::uint64_t>> previous;
    for(std::uint64_t i=0;i<count;++i){
        ScalarChange v;v.player=in.u64();v.unit=in.u64();v.unit_count=in.u64();v.field=in.u64();
        v.owner=in.i64();v.id=in.i64();v.encoding=static_cast<saves::UnitCoreEncoding>(in.u32());
        v.before=in.i32();v.after=in.i32();
        const auto order=std::tuple{v.player,v.unit,v.field};
        require((!previous || *previous<order) && v.player<players,"BNW scalar checkpoint duplicate/out-of-order record");previous=order;
        const auto &player=world.all_players->players[v.player];
        require(v.unit_count==player.units.units.size() && v.unit<v.unit_count,"BNW scalar checkpoint unit roster changed");
        const auto &unit=player.units.units[v.unit];
        require(player.saved_player_id==v.owner && core_value(unit,"m_eOwner")==v.owner &&
                core_value(unit,"m_iID")==v.id && v.field<unit.core.fields.size(),"BNW scalar checkpoint full identity changed");
        const auto &field=unit.core.fields[v.field];
        require(scalar_mutable(field) && field.encoding==v.encoding && field.values.front()==v.before &&
                v.before!=v.after,"BNW scalar checkpoint old value or field type changed");
        scalar_range(field,v.after);result.push_back(v);
    }
    require(in.at==section.bytes.size(),"Trailing BNW scalar checkpoint bytes");return result;
}
} // namespace

std::string bnw_unit_checkpoint_original_identity_digest(const saves::SavedSessionSnapshot &source) {
    (void)inspect_moves_only(source,source,false);
    return batch_source_digest(source);
}
void require_bnw_unit_checkpoint_unchanged_except_serial_mask(
    const saves::SavedSessionSnapshot &masked,const saves::SavedSessionSnapshot &source) {
    require(inspect_scalars(source,masked,false).empty(),
            "Unit serial checkpoint has unsupported existing unit scalar changes");
}

saves::CivisSaveSection make_bnw_unit_moves_checkpoint(
    const saves::SavedSessionSnapshot &world,const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity) {
    check_hash(identity.payload_sha256);check_hash(identity.rules_sha256);
    const auto changes=inspect_moves_only(source,world);
    Writer out;out.bytes.insert(out.bytes.end(),magic.begin(),magic.end());out.u32(3);
    out.hash(identity.payload_sha256);out.hash(identity.rules_sha256);
    out.hash(batch_source_digest(source));
    out.u64(source.map_header.grid.width);out.u64(source.map_header.grid.height);
    out.u64(source.all_players->players.size());out.u64(changes.size());
    for(const auto &move:changes) {
        out.u64(move.player_index);out.u64(move.unit_index);
        out.u64(source.all_players->players[move.player_index].units.units.size());
        out.i64(move.owner);out.i64(move.id);out.i64(move.old_x);out.i64(move.old_y);
        out.i32(*move.old_moves);out.i32(*move.new_moves);
    }
    return {bnw_unit_checkpoint_section_id,0,std::move(out.bytes)};
}

saves::CivisSaveSection make_bnw_unit_scalar_checkpoint(
    const saves::SavedSessionSnapshot &world,const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity) {
    check_hash(identity.payload_sha256);check_hash(identity.rules_sha256);
    const auto changes=inspect_scalars(source,world);
    Writer out;out.bytes.insert(out.bytes.end(),magic.begin(),magic.end());out.u32(4);
    out.hash(identity.payload_sha256);out.hash(identity.rules_sha256);out.hash(batch_source_digest(source));
    out.u64(source.map_header.grid.width);out.u64(source.map_header.grid.height);
    out.u64(source.all_players->players.size());out.u64(changes.size());
    for(const auto &v:changes){out.u64(v.player);out.u64(v.unit);out.u64(v.unit_count);out.u64(v.field);
        out.i64(v.owner);out.i64(v.id);out.u32(static_cast<std::uint32_t>(v.encoding));out.i32(v.before);out.i32(v.after);}
    return {bnw_unit_checkpoint_section_id,0,std::move(out.bytes)};
}

saves::CivisSaveSection make_bnw_unit_checkpoint(
    const saves::SavedSessionSnapshot &world,const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity) {
    check_hash(identity.payload_sha256);check_hash(identity.rules_sha256);
    const auto move=inspect_move(source,world);
    Writer out;out.bytes.reserve(v2_wire_size);
    out.bytes.insert(out.bytes.end(),magic.begin(),magic.end());out.u32(2);
    out.hash(identity.payload_sha256);out.hash(identity.rules_sha256);
    out.u64(source.map_header.grid.width);out.u64(source.map_header.grid.height);
    out.u64(source.all_players->players.size());
    out.u64(source.all_players->players[move.player_index].units.units.size());
    out.u64(move.player_index);out.u64(move.unit_index);
    out.i64(move.owner);out.i64(move.id);
    out.i64(move.old_x);out.i64(move.old_y);out.i64(move.new_x);out.i64(move.new_y);
    out.i32(*move.old_moves);out.i32(*move.new_moves);
    return {bnw_unit_checkpoint_section_id,0,std::move(out.bytes)};
}

saves::CivisSaveSection make_bnw_unit_cycle_checkpoint(
    const saves::SavedSessionSnapshot &world,const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity) {
    check_hash(identity.payload_sha256);check_hash(identity.rules_sha256);
    const auto changes=inspect_cycles(source,world,true);Writer out;
    out.bytes.insert(out.bytes.end(),magic.begin(),magic.end());out.u32(5);
    out.hash(identity.payload_sha256);out.hash(identity.rules_sha256);
    out.hash(cycle_source_digest(source));out.u64(changes.size());
    for(const auto player:changes){
        out.u64(player);const auto&old=cycle_array(source.all_players->players[player]);
        const auto&now=cycle_array(world.all_players->players[player]);
        out.u64(old.values.size());for(auto id:old.values)out.i32(static_cast<std::int32_t>(id));
        out.u64(now.values.size());for(auto id:now.values)out.i32(static_cast<std::int32_t>(id));
    }
    return {bnw_unit_checkpoint_section_id,0,std::move(out.bytes)};
}

void restore_bnw_unit_checkpoint(
    const saves::CivisSaveSection &section,saves::SavedSessionSnapshot &world,
    const SessionCheckpointIdentity &identity) {
    check_hash(identity.payload_sha256);check_hash(identity.rules_sha256);
    map_shape(world);
    if(cycle_version(section)) {
        auto replacement=read_cycle_replacement(section,world,identity);
        const auto canonical=make_bnw_unit_cycle_checkpoint(replacement,world,identity);
        require(canonical.bytes==section.bytes,"Noncanonical BNW cycle checkpoint");
        world=std::move(replacement);return;
    }
    if(scalar_version(section)) {
        const auto changes=read_scalar_batch(section,world,identity);
        auto replacement=world;
        for(const auto &v:changes)
            replacement.all_players->players[v.player].units.units[v.unit].core.fields[v.field].values.front()=v.after;
        const auto canonical=make_bnw_unit_scalar_checkpoint(replacement,world,identity);
        require(canonical.bytes==section.bytes,"Noncanonical BNW scalar checkpoint");
        world=std::move(replacement);return;
    }
    if(batch_version(section)) {
        // All records, including the last identity/old-value/home check, pass
        // before the first store. No source object is mutated on failure.
        const auto changes=read_moves_batch(section,world,identity);
        auto replacement=world;
        for(const auto &move:changes)
            set_core_value(replacement.all_players->players[move.player_index].units.units[move.unit_index],
                           "m_iMoves",*move.new_moves);
        const auto canonical=make_bnw_unit_moves_checkpoint(replacement,world,identity);
        require(canonical.bytes==section.bytes,"Noncanonical BNW moves batch");
        world=std::move(replacement);return;
    }
    const auto move=read_move(section,world,identity);
    const auto &unit=world.all_players->players[move.player_index]
                         .units.units[move.unit_index];
    require(world.all_players->players[move.player_index].saved_player_id==move.owner &&
            core_value(unit,"m_eOwner")==move.owner && core_value(unit,"m_iID")==move.id &&
            core_value(unit,"m_iX")==move.old_x && core_value(unit,"m_iY")==move.old_y &&
            (!move.old_moves || core_value(unit,"m_iMoves")==*move.old_moves),
            "BNW unit checkpoint source unit changed");
    auto replacement=world;
    apply_move(replacement,move);
    const auto proven=inspect_move(world,replacement,move.old_moves.has_value());
    require(proven.player_index==move.player_index &&
            proven.unit_index==move.unit_index && proven.owner==move.owner &&
            proven.id==move.id && proven.old_x==move.old_x &&
            proven.old_y==move.old_y && proven.new_x==move.new_x &&
            proven.new_y==move.new_y && proven.old_moves==move.old_moves &&
            proven.new_moves==move.new_moves,
            "BNW unit checkpoint source roster changed");
    world=std::move(replacement);
}

BnwMapUnitCheckpointPair make_bnw_map_unit_checkpoint_pair(
    const saves::SavedSessionSnapshot &world,const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity,
    const saves::SavedTerrainFeatureResolver &features,
    const content::RulesDatabase *rules,const content::InfoTypeCatalog *types,
    const content::GlobalDefineCache *defines) {
    auto unit_section=make_bnw_unit_checkpoint(world,source,identity);
    const auto move=read_move(unit_section,source,identity);
    auto map_view=world;
    // Moves-only has no map delta to mask; the existing map writer must prove
    // that unchanged rosters and all other map operands form a strict no-op.
    if(move.old_plot!=move.new_plot) {
        mutable_plot_units(map_view.map_plots->plots.at(move.old_plot)).values=
            plot_units(source.map_plots->plots.at(move.old_plot)).values;
        mutable_plot_units(map_view.map_plots->plots.at(move.new_plot)).values=
            plot_units(source.map_plots->plots.at(move.new_plot)).values;
        map_view.map_projection->cells.at(move.old_plot).units=
            source.map_projection->cells.at(move.old_plot).units;
        map_view.map_projection->cells.at(move.new_plot).units=
            source.map_projection->cells.at(move.new_plot).units;
    }
    auto map_section=make_bnw_map_plot_checkpoint(
        map_view,source,identity,features,rules,types,defines);
    if(move.old_plot==move.new_plot)
        require(map_section.bytes==make_bnw_map_plot_checkpoint(
                    source,source,identity,features,rules,types,defines).bytes,
                "BNW moves-only pair requires an unchanged map");
    return {std::move(map_section),std::move(unit_section)};
}

BnwMapUnitCheckpointPair make_bnw_map_unit_moves_checkpoint_pair(
    const saves::SavedSessionSnapshot &world,const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity,const saves::SavedTerrainFeatureResolver &features,
    const content::RulesDatabase *rules,const content::InfoTypeCatalog *types,
    const content::GlobalDefineCache *defines) {
    auto units=make_bnw_unit_moves_checkpoint(world,source,identity);
    auto map=make_bnw_map_plot_checkpoint(world,source,identity,features,rules,types,defines);
    require(map.bytes==make_bnw_map_plot_checkpoint(
                source,source,identity,features,rules,types,defines).bytes,
            "BNW moves batch requires an unchanged map");
    return {std::move(map),std::move(units)};
}

BnwMapUnitCheckpointPair make_bnw_map_unit_scalar_checkpoint_pair(
    const saves::SavedSessionSnapshot &world,const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity,const saves::SavedTerrainFeatureResolver &features,
    const content::RulesDatabase *rules,const content::InfoTypeCatalog *types,
    const content::GlobalDefineCache *defines) {
    auto units=make_bnw_unit_scalar_checkpoint(world,source,identity);
    auto map=make_bnw_map_plot_checkpoint(world,source,identity,features,rules,types,defines);
    require(map.bytes==make_bnw_map_plot_checkpoint(source,source,identity,features,rules,types,defines).bytes,
            "BNW scalar checkpoint requires unchanged map");
    return {std::move(map),std::move(units)};
}

void restore_bnw_map_unit_checkpoint_pair(
    const saves::CivisSaveSection *map,const saves::CivisSaveSection *units,
    saves::SavedSessionSnapshot &world,const SessionCheckpointIdentity &identity,
    const saves::SavedTerrainFeatureResolver &features,
    const content::RulesDatabase *rules,const content::InfoTypeCatalog *types,
    const content::GlobalDefineCache *defines) {
    require(map && units,"BNW unit relocation requires both map and unit sections");
    const bool scalar=scalar_version(*units);
    const bool batch=batch_version(*units);
    const bool cycle=cycle_version(*units);
    if(batch) {
        check_hash(identity.payload_sha256);check_hash(identity.rules_sha256);
        (void)read_moves_batch(*units,world,identity);
    }
    if(scalar) {
        check_hash(identity.payload_sha256);check_hash(identity.rules_sha256);
        (void)read_scalar_batch(*units,world,identity);
    }
    if(cycle) {
        check_hash(identity.payload_sha256);check_hash(identity.rules_sha256);
        // Validate the original ordered cycle and full-ID source before any
        // map store. Version five has no relocation and requires the same
        // strict map no-op as the scalar and moves-only formats.
        (void)read_cycle_replacement(*units,world,identity);
    }
    auto replacement=world;
    restore_bnw_map_plot_checkpoint(*map,replacement,identity,features,rules,types,defines);
    const auto move=(batch || scalar || cycle)?Move{}:read_move(*units,world,identity);
    if(batch || scalar || cycle || move.old_plot==move.new_plot)
        require(make_bnw_map_plot_checkpoint(
                    replacement,world,identity,features,rules,types,defines).bytes==
                make_bnw_map_plot_checkpoint(
                    world,world,identity,features,rules,types,defines).bytes,
                "BNW moves-only pair requires an unchanged map");
    restore_bnw_unit_checkpoint(*units,replacement,identity);
    world=std::move(replacement);
}
} // namespace civ5::runtime
