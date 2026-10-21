#include "civ5/runtime/bnw_unit_checkpoint.hpp"
#include "civ5/runtime/bnw_area_unit_checkpoint.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/saves/loaded_save.hpp"
#include <algorithm>
#include <optional>
#include <vector>
#include <chrono>
#include <array>
#include <climits>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {
using namespace civ5;
void check(bool value,const char *why) {if(!value) throw std::runtime_error(why);}
template<class F> void rejects(F &&f) {
    bool rejected=false;
    try {f();} catch(const std::exception &) {rejected=true;}
    check(rejected,"Unsupported BNW unit checkpoint transition was accepted");
}
std::int64_t &unit_field(saves::SavedUnitObservation &unit,std::string_view wanted) {
    for(auto &field:unit.core.fields)
        if(field.name && wanted==field.name &&
           field.encoding==saves::UnitCoreEncoding::i32 && field.values.size()==1)
            return field.values.front();
    throw std::runtime_error("Selected BNW unit core field unavailable");
}
std::int64_t &plot_field(saves::PlotObservation &plot,std::string_view wanted) {
    for(auto &field:plot.fields)
        if(field.name && wanted==field.name) return field.value;
    throw std::runtime_error("Selected BNW plot field unavailable");
}
std::vector<std::int64_t> &plot_units(saves::PlotObservation &plot) {
    for(auto &array:plot.arrays)
        if(array.name && std::string_view(array.name)=="units" && array.columns==2)
            return array.values;
    throw std::runtime_error("Selected BNW plot unit handles unavailable");
}
struct Selection {std::size_t player=0,unit=0,old_plot=0,new_plot=0;};
Selection select_simple_move(saves::SavedSessionSnapshot &world) {
    const auto &grid=world.map_header.grid;
    for(std::size_t p=0;p<world.all_players->players.size();++p) {
        auto &player=world.all_players->players[p];
        for(std::size_t u=0;u<player.units.units.size();++u) {
            auto &unit=player.units.units[u];
            const auto owner=unit_field(unit,"m_eOwner"),id=unit_field(unit,"m_iID");
            const auto x=unit_field(unit,"m_iX"),y=unit_field(unit,"m_iY");
            if(owner!=player.saved_player_id || owner<0 || id<0 ||
               x<0 || y<0 || static_cast<std::uint64_t>(x)>=grid.width ||
               static_cast<std::uint64_t>(y)>=grid.height) continue;
            const auto old_plot=grid.index(static_cast<std::uint64_t>(x),static_cast<std::uint64_t>(y));
            if(plot_units(world.map_plots->plots[old_plot])!=
               std::vector<std::int64_t>{owner,id}) continue;
            for(const auto [dx,dy]:{std::pair{-1,0},std::pair{1,0},
                                    std::pair{0,-1},std::pair{0,1}}) {
                const auto nx=x+dx,ny=y+dy;
                if(nx<0 || ny<0 || static_cast<std::uint64_t>(nx)>=grid.width ||
                   static_cast<std::uint64_t>(ny)>=grid.height) continue;
                const auto target=grid.index(static_cast<std::uint64_t>(nx),static_cast<std::uint64_t>(ny));
                if(plot_units(world.map_plots->plots[target]).empty())
                    return {p,u,old_plot,target};
            }
        }
    }
    throw std::runtime_error("Selected Gandhi save has no simple unit relocation fixture");
}
// Full affected unit/roster domains, rather than only the moved scalar, for
// failure-before-publication checks. This is test observation, never authority.
void stamp_tail(std::ostream &out,const saves::UnitTailNode &node) {
    out << (node.name?node.name:"") << ':' << (node.encoding?node.encoding:"") << ':'
        << node.start_offset << ':' << node.end_offset << ':' << node.wire_count.has_value() << ':';
    if(node.wire_count) out << *node.wire_count;
    out << ':' << node.values.size() << ':';
    for(auto v:node.values) out << v << ',';
    out << ':' << node.bytes.size() << ':';
    for(auto v:node.bytes) out << static_cast<unsigned>(v) << ',';
    out << ':' << node.children.size() << ':';
    for(const auto &child:node.children) stamp_tail(out,child);
}
std::string affected_stamp(const saves::SavedSessionSnapshot &world) {
    std::ostringstream out;
    for(const auto &p:world.all_players->players) {
        out << p.index << ':' << p.saved_player_id << ':' << p.units.units.size() << ':';
        for(const auto &u:p.units.units) {
            out << u.index << ':' << u.core.start_offset << ':' << u.core.end_offset << ':'
                << u.core.fields.size() << ':';
            for(const auto &f:u.core.fields) {
                out << (f.name?f.name:"") << ':' << static_cast<unsigned>(f.encoding) << ':'
                    << f.start_offset << ':' << f.end_offset << ':' << f.values.size() << ':';
                for(auto v:f.values) out << v << ',';
                out << ':' << f.bytes.size() << ':';
                for(auto v:f.bytes) out << static_cast<unsigned>(v) << ',';
            }
            for(const auto &node:u.tail.blocks) stamp_tail(out,node);
        }
    }
    for(const auto &p:world.map_plots->plots) {
        out << p.index << ':';
        for(const auto &f:p.fields) out << (f.name?f.name:"") << ':' << f.value << ';';
        for(const auto &a:p.arrays) {
            out << (a.name?a.name:"") << ':' << a.columns << ':' << a.values.size() << ':';
            for(auto v:a.values) out << v << ',';
        }
    }
    for(const auto &cell:world.map_projection->cells) {
        out << cell.index << ':' << cell.units.has_value() << ':';
        if(cell.units) for(const auto &u:*cell.units) out << u.owner << ':' << u.id << ',';
    }
    return out.str();
}
saves::CivisSaveSection legacy_v1(const saves::SavedSessionSnapshot &source,
                                const saves::SavedSessionSnapshot &target,
                                const Selection &s,
                                const runtime::SessionCheckpointIdentity &identity) {
    // Independent old236 layout; not produced by slicing the new encoder.
    std::vector<std::uint8_t> bytes{'C','I','V','B','U','N','T','1',1,0,0,0};
    for(const auto &hash:{identity.payload_sha256,identity.rules_sha256})
        bytes.insert(bytes.end(),hash.begin(),hash.end());
    auto scalar=[&](std::uint64_t v) {
        for(unsigned i=0;i<8;++i) bytes.push_back(static_cast<std::uint8_t>(v>>(i*8)));
    };
    const auto &a=source.all_players->players[s.player].units.units[s.unit];
    const auto &b=target.all_players->players[s.player].units.units[s.unit];
    auto value=[](const saves::SavedUnitObservation &u,std::string_view name) {
        for(const auto &f:u.core.fields) if(f.name && name==f.name) return f.values.at(0);
        throw std::runtime_error("Missing legacy golden scalar");
    };
    scalar(source.map_header.grid.width);scalar(source.map_header.grid.height);
    scalar(source.all_players->players.size());scalar(source.all_players->players[s.player].units.units.size());
    scalar(s.player);scalar(s.unit);
    for(auto v:{value(a,"m_eOwner"),value(a,"m_iID"),value(a,"m_iX"),value(a,"m_iY"),value(b,"m_iX"),value(b,"m_iY")})
        scalar(static_cast<std::uint64_t>(v));
    check(bytes.size()==236,"Legacy golden wire size changed");
    return {runtime::bnw_unit_checkpoint_section_id,0,std::move(bytes)};
}
void write_i32(saves::CivisSaveSection &section,std::size_t at,std::int32_t value) {
    const auto bits=static_cast<std::uint32_t>(value);
    for(unsigned i=0;i<4;++i) section.bytes.at(at+i)=static_cast<std::uint8_t>(bits>>(8*i));
}
void test_moves(const saves::SavedSessionSnapshot &original,const Selection &chosen,
                const saves::SavedSessionSnapshot &relocated,
                const runtime::SessionCheckpointIdentity &identity) {
    // These are paired DETACHED codec variants, not genuine issuing mutations.
    constexpr std::array<std::int32_t,4> edges{INT32_MIN,-1,0,INT32_MAX};
    for(const auto target:edges) {
        auto source=original;
        auto &old=source.all_players->players[chosen.player].units.units[chosen.unit];
        unit_field(old,"m_iMoves")=target==INT32_MIN?INT32_MAX:INT32_MIN;
        auto changed=source;
        unit_field(changed.all_players->players[chosen.player].units.units[chosen.unit],"m_iMoves")=target;
        const auto section=runtime::make_bnw_unit_checkpoint(changed,source,identity);
        check(section.bytes.size()==244 && section.bytes[8]==2,"V2 fixed244 layout unavailable");
        auto restored=source;runtime::restore_bnw_unit_checkpoint(section,restored,identity);
        check(affected_stamp(restored)==affected_stamp(changed),"Signed moves-only restore altered another affected field");
        check(runtime::make_bnw_unit_checkpoint(restored,source,identity).bytes==section.bytes,"Moves roundtrip is nondeterministic");
        const auto bits=static_cast<std::uint32_t>(target);
        for(unsigned i=0;i<4;++i) check(section.bytes[240+i]==static_cast<std::uint8_t>(bits>>(8*i)),"Native signed i32 LE bytes changed");
    }
    auto source=original;
    auto &old=source.all_players->players[chosen.player].units.units[chosen.unit];
    unit_field(old,"m_iMoves")=-9;
    auto changed=source;unit_field(changed.all_players->players[chosen.player].units.units[chosen.unit],"m_iMoves")=-17;
    const auto section=runtime::make_bnw_unit_checkpoint(changed,source,identity);
    const auto pair=runtime::make_bnw_map_unit_checkpoint_pair(changed,source,identity,{});
    check(pair.map.bytes==runtime::make_bnw_map_plot_checkpoint(source,source,identity,{}).bytes,
          "Moves-only map pair weakened strict no-op map view");
    auto restored=source;runtime::restore_bnw_map_unit_checkpoint_pair(&pair.map,&pair.units,restored,identity,{});
    check(affected_stamp(restored)==affected_stamp(changed),"Moves-only pair changed raw/projected roster");
    const auto unchanged=affected_stamp(source);
    auto failed=source;
    auto map_delta=source;
    auto &anchor=plot_field(map_delta.map_plots->plots[chosen.old_plot],"world_anchor");
    anchor=anchor==-1?0:-1;
    const auto changed_map=runtime::make_bnw_map_plot_checkpoint(map_delta,source,identity,{});
    auto map_and_moves=changed;
    plot_field(map_and_moves.map_plots->plots[chosen.old_plot],"world_anchor")=anchor;
    rejects([&]{(void)runtime::make_bnw_map_unit_checkpoint_pair(map_and_moves,source,identity,{});});
    rejects([&]{runtime::restore_bnw_map_unit_checkpoint_pair(&changed_map,&pair.units,failed,identity,{});});
    check(affected_stamp(failed)==unchanged,"Pure moves accepted/published an independent map delta");
    for(std::size_t length=0;length<section.bytes.size();++length) {
        auto truncated=section;truncated.bytes.resize(length);
        rejects([&]{runtime::restore_bnw_unit_checkpoint(truncated,failed,identity);});
    }
    for(const auto at:std::array<std::size_t,13>{0,8,12,76,140,156,172,180,188,196,204,212,236}) {
        auto corrupt=section;corrupt.bytes[at]^=1;
        rejects([&]{runtime::restore_bnw_unit_checkpoint(corrupt,failed,identity);});
    }
    auto trailing=section;trailing.bytes.push_back(0);
    rejects([&]{runtime::restore_bnw_unit_checkpoint(trailing,failed,identity);});
    auto flags=section;flags.flags=1;
    rejects([&]{runtime::restore_bnw_unit_checkpoint(flags,failed,identity);});
    auto wrong=identity;wrong.rules_sha256=std::string(64,'b');
    rejects([&]{runtime::restore_bnw_unit_checkpoint(section,failed,wrong);});
    wrong=identity;wrong.payload_sha256=std::string(64,'c');
    rejects([&]{runtime::restore_bnw_unit_checkpoint(section,failed,wrong);});
    check(affected_stamp(failed)==unchanged,"Rejected wire published partial units/map effects");
    auto bad_source=source;unit_field(bad_source.all_players->players[chosen.player].units.units[chosen.unit],"m_iMoves")=-8;
    const auto bad_before=affected_stamp(bad_source);
    rejects([&]{runtime::restore_bnw_unit_checkpoint(section,bad_source,identity);});
    check(affected_stamp(bad_source)==bad_before,"Wrong old moves receiver was mutated");
    auto other=changed;
    auto &fields=other.all_players->players[chosen.player].units.units[chosen.unit].core.fields;
    for(auto &f:fields) if(f.name && std::string_view(f.name)=="m_iMoves") f.encoding=saves::UnitCoreEncoding::hash32;
    rejects([&]{(void)runtime::make_bnw_unit_checkpoint(other,source,identity);});
    other=changed;auto &tail=other.all_players->players[chosen.player].units.units[chosen.unit].tail.blocks;
    check(!tail.empty(),"Actual selected unit lacks mandatory tail fixture");tail.front().bytes.push_back(1);
    rejects([&]{(void)runtime::make_bnw_unit_checkpoint(other,source,identity);});
    other=changed;unit_field(other.all_players->players[chosen.player].units.units[chosen.unit],"m_iDamage")+=1;
    rejects([&]{(void)runtime::make_bnw_unit_checkpoint(other,source,identity);});
    other=changed;unit_field(other.all_players->players[chosen.player].units.units[chosen.unit],"m_iMoves")=static_cast<std::int64_t>(INT32_MAX)+1;
    rejects([&]{(void)runtime::make_bnw_unit_checkpoint(other,source,identity);});
    for(unsigned mode=0;mode<3;++mode) {
        auto malformed=source;
        auto &core=malformed.all_players->players[chosen.player].units.units[chosen.unit].core.fields;
        const auto it=std::find_if(core.begin(),core.end(),[](const auto &f){
            return f.name && std::string_view(f.name)=="m_iMoves";
        });
        check(it!=core.end(),"Actual moves schema missing");
        if(mode==0) core.erase(it);
        else if(mode==1) {const auto duplicate_field=*it;core.push_back(duplicate_field);}
        else it->values.push_back(0);
        const auto before=affected_stamp(malformed);
        rejects([&]{runtime::restore_bnw_unit_checkpoint(section,malformed,identity);});
        check(affected_stamp(malformed)==before,"Invalid moves schema receiver changed");
    }
    auto duplicate_home=source;
    auto &home=plot_units(duplicate_home.map_plots->plots[chosen.old_plot]);
    const auto original_home=home;
    home.insert(home.end(),original_home.begin(),original_home.end());
    const auto duplicate_handle=duplicate_home.map_projection->cells[chosen.old_plot].units->front();
    duplicate_home.map_projection->cells[chosen.old_plot].units->push_back(duplicate_handle);
    auto duplicate_home_changed=duplicate_home;
    unit_field(duplicate_home_changed.all_players->players[chosen.player].units.units[chosen.unit],"m_iMoves")=-17;
    rejects([&]{(void)runtime::make_bnw_unit_checkpoint(duplicate_home_changed,duplicate_home,identity);});
    bool second_unit=false;
    for(std::size_t p=0;p<changed.all_players->players.size() && !second_unit;++p)
        for(std::size_t u=0;u<changed.all_players->players[p].units.units.size();++u)
            if(p!=chosen.player || u!=chosen.unit) {
                other=changed;
                auto &moves=unit_field(other.all_players->players[p].units.units[u],"m_iMoves");
                moves=moves==INT32_MIN?INT32_MAX:INT32_MIN;
                rejects([&]{(void)runtime::make_bnw_unit_checkpoint(other,source,identity);});
                second_unit=true;break;
            }
    check(second_unit,"Selected save lacks second-unit strictness fixture");
    other=changed;plot_units(other.map_plots->plots[chosen.old_plot]).clear();
    other.map_projection->cells[chosen.old_plot].units=std::vector<saves::SavedPlotUnitReference>{};
    rejects([&]{(void)runtime::make_bnw_unit_checkpoint(other,source,identity);});
    auto duplicate=source;duplicate.all_players->players[chosen.player].units.units.push_back(old);
    auto duplicate_changed=duplicate;unit_field(duplicate_changed.all_players->players[chosen.player].units.units[chosen.unit],"m_iMoves")=-17;
    rejects([&]{(void)runtime::make_bnw_unit_checkpoint(duplicate_changed,duplicate,identity);});
    // Legacy v1 relocation never writes even an already-negative moves scalar.
    auto legacy_source=original;unit_field(legacy_source.all_players->players[chosen.player].units.units[chosen.unit],"m_iMoves")=-9;
    auto legacy=legacy_v1(legacy_source,relocated,chosen,identity);
    auto legacy_restored=legacy_source;runtime::restore_bnw_unit_checkpoint(legacy,legacy_restored,identity);
    check(unit_field(legacy_restored.all_players->players[chosen.player].units.units[chosen.unit],"m_iMoves")==-9,
          "Legacy v1 wrote/clamped source moves");
    auto wrong_length=section;write_i32(wrong_length,8,1);
    rejects([&]{runtime::restore_bnw_unit_checkpoint(wrong_length,failed,identity);});
    legacy.bytes[8]=2;rejects([&]{runtime::restore_bnw_unit_checkpoint(legacy,failed,identity);});
    // Another valid target i32 is diagnostic data, not authenticated corruption
    // or legal-move authority; all signed values remain representable.
    auto different_target=section;write_i32(different_target,240,-23);
    auto diagnostic=source;runtime::restore_bnw_unit_checkpoint(different_target,diagnostic,identity);
    check(unit_field(diagnostic.all_players->players[chosen.player].units.units[chosen.unit],"m_iMoves")==-23,"Valid signed target was clamped");
    // An area triplet still requires a real roster transfer. Its outer staged
    // copy must not publish moves-only work before that mandatory join fails.
    saves::CivisSaveSection dummy_area{runtime::bnw_area_unit_checkpoint_section_id,0,{}};
    failed=source;
    rejects([&]{runtime::restore_bnw_map_unit_area_checkpoint_triplet(&pair.map,&pair.units,&dummy_area,failed,identity,{});});
    check(affected_stamp(failed)==unchanged,"Area refusal published moves-only pair effects");
}
void test_batch_moves(const saves::SavedSessionSnapshot &source,
                      const runtime::SessionCheckpointIdentity &identity,
                      const saves::LoadedSave &loaded,std::string_view source_sha256) {
    std::vector<std::pair<std::size_t,std::size_t>> picked;
    for(std::size_t p=0;p<source.all_players->players.size() && picked.size()<2;++p)
        for(std::size_t u=0;u<source.all_players->players[p].units.units.size() && picked.size()<2;++u) {
            auto unit=source.all_players->players[p].units.units[u];
            const auto owner=unit_field(unit,"m_eOwner"),id=unit_field(unit,"m_iID");
            const auto x=unit_field(unit,"m_iX"),y=unit_field(unit,"m_iY");
            if(owner!=source.all_players->players[p].saved_player_id || owner<0 || id<0 ||
               x<0 || y<0 || static_cast<std::uint64_t>(x)>=source.map_header.grid.width ||
               static_cast<std::uint64_t>(y)>=source.map_header.grid.height)continue;
            const auto home=source.map_header.grid.index(static_cast<std::uint64_t>(x),static_cast<std::uint64_t>(y));
            std::size_t hits=0;bool off_home=false;
            for(std::size_t i=0;i<source.map_plots->plots.size();++i) {
                const auto &plot=source.map_plots->plots[i];
                for(const auto &row:plot.arrays)if(row.name && std::string_view(row.name)=="units")
                    for(std::size_t j=0;j+1<row.values.size();j+=2)
                        if(row.values[j]==owner && row.values[j+1]==id){++hits;off_home=off_home || i!=home;}
            }
            if(hits==1 && !off_home)picked.emplace_back(p,u);
        }
    check(picked.size()==2,"Actual selected save lacks two full-ID moves codec operands");
    auto changed=source;
    for(std::size_t i=0;i<picked.size();++i) {
        const auto [p,u]=picked[i];
        auto &moves=unit_field(changed.all_players->players[p].units.units[u],"m_iMoves");
        moves=i==0?(moves==INT32_MIN?INT32_MAX:INT32_MIN):(moves==-1?0:-1);
    }
    const auto pair=runtime::make_bnw_map_unit_moves_checkpoint_pair(changed,source,identity,{});
    // Independent wire assembly, not expected values from the production decoder.
    std::vector<std::uint8_t> golden{'C','I','V','B','U','N','T','1',3,0,0,0};
    for(const auto &hash:{identity.payload_sha256,identity.rules_sha256})
        golden.insert(golden.end(),hash.begin(),hash.end());
    golden.insert(golden.end(),64,0); // Opaque source digest: independent mutation refusals below.
    auto u64=[&](std::uint64_t v){for(unsigned i=0;i<8;++i)golden.push_back(static_cast<std::uint8_t>(v>>(i*8)));};
    auto i32=[&](std::int64_t v){const auto bits=static_cast<std::uint32_t>(v);for(unsigned i=0;i<4;++i)golden.push_back(static_cast<std::uint8_t>(bits>>(i*8)));};
    u64(source.map_header.grid.width);u64(source.map_header.grid.height);
    u64(source.all_players->players.size());u64(2);
    for(const auto [p,u]:picked) {
        auto old=source.all_players->players[p].units.units[u];
        auto next=changed.all_players->players[p].units.units[u];
        u64(p);u64(u);u64(source.all_players->players[p].units.units.size());
        for(auto v:{unit_field(old,"m_eOwner"),unit_field(old,"m_iID"),unit_field(old,"m_iX"),unit_field(old,"m_iY")})u64(static_cast<std::uint64_t>(v));
        i32(unit_field(old,"m_iMoves"));i32(unit_field(next,"m_iMoves"));
    }
    check(golden.size()==364 && pair.units.bytes.size()==364 &&
          std::equal(golden.begin(),golden.begin()+140,pair.units.bytes.begin()) &&
          std::all_of(pair.units.bytes.begin()+140,pair.units.bytes.begin()+204,[](std::uint8_t c){return (c>='0' && c<='9') || (c>='a' && c<='f');}) &&
          std::equal(golden.begin()+204,golden.end(),pair.units.bytes.begin()+204),"Two-unit independent V3 wire differs");
    auto restored=source;
    runtime::restore_bnw_map_unit_checkpoint_pair(&pair.map,&pair.units,restored,identity,{});
    check(affected_stamp(restored)==affected_stamp(changed),"V3 changed another unit or roster field");
    check(runtime::make_bnw_unit_moves_checkpoint(restored,source,identity).bytes==pair.units.bytes,"V3 re-encoding differs");
    const auto unchanged=affected_stamp(source);
    auto refusal=[&](const saves::CivisSaveSection &bad,const saves::CivisSaveSection &map,
                     const runtime::SessionCheckpointIdentity &bound) {
        auto receiver=source;
        rejects([&]{runtime::restore_bnw_map_unit_checkpoint_pair(&map,&bad,receiver,bound,{});});
        check(affected_stamp(receiver)==unchanged,"Late V3 refusal partially restored an earlier record/map");
    };
    auto bad=pair.units;
    bad.bytes.at(236+64+32)^=1; // Second record full ID; the first remains valid.
    refusal(bad,pair.map,identity);
    bad=pair.units;write_i32(bad,236+64+56,INT32_MIN); // Second old moves mismatch.
    // If actual old moves was INT32_MIN, use another explicit native value.
    auto old_second=source.all_players->players[picked[1].first].units.units[picked[1].second];
    if(unit_field(old_second,"m_iMoves")==INT32_MIN)write_i32(bad,236+64+56,INT32_MAX);
    refusal(bad,pair.map,identity);
    for(std::size_t length=0;length<pair.units.bytes.size();++length) {
        bad=pair.units;bad.bytes.resize(length);refusal(bad,pair.map,identity);
    }
    bad=pair.units;bad.bytes.push_back(0);refusal(bad,pair.map,identity);
    bad=pair.units;bad.bytes.at(228)=255;refusal(bad,pair.map,identity); // Declared count cannot allocate.
    auto wrong=identity;wrong.rules_sha256=std::string(64,'b');refusal(pair.units,pair.map,wrong);
    wrong=identity;wrong.payload_sha256=std::string(64,'c');refusal(pair.units,pair.map,wrong);
    auto receiver=source;
    const auto [late_p,late_u]=picked[1];
    ++unit_field(receiver.all_players->players[late_p].units.units[late_u],"m_iMoves");
    const auto late_before=affected_stamp(receiver);
    rejects([&]{runtime::restore_bnw_map_unit_checkpoint_pair(&pair.map,&pair.units,receiver,identity,{});});
    check(affected_stamp(receiver)==late_before,"Changed late source unit allowed an earlier store");
    receiver=source;receiver.all_players->players[late_p].units.header.wire_unit_count+=1;
    const auto roster_before=affected_stamp(receiver);
    rejects([&]{runtime::restore_bnw_map_unit_checkpoint_pair(&pair.map,&pair.units,receiver,identity,{});});
    check(affected_stamp(receiver)==roster_before,"Changed native collection header was partially restored");
    receiver=source;++unit_field(receiver.all_players->players[late_p].units.units[late_u],"m_iDamage");
    const auto core_before=affected_stamp(receiver);
    rejects([&]{runtime::restore_bnw_map_unit_checkpoint_pair(&pair.map,&pair.units,receiver,identity,{});});
    check(affected_stamp(receiver)==core_before,"Changed source unit core was partially restored");
    // Altered collection count is part of the native header contract. It is
    // also exercised by comparing two source/target headers during encoding.
    auto altered=changed;altered.all_players->players[late_p].units.header.wire_unit_count+=1;
    rejects([&]{(void)runtime::make_bnw_unit_moves_checkpoint(altered,source,identity);});
    auto mutated=changed;
    ++unit_field(mutated.all_players->players[picked[1].first].units.units[picked[1].second],"m_iDamage");
    rejects([&]{(void)runtime::make_bnw_unit_moves_checkpoint(mutated,source,identity);});
    mutated=changed;
    auto &anchor=plot_field(mutated.map_plots->plots[0],"world_anchor");anchor=anchor==-1?0:-1;
    rejects([&]{(void)runtime::make_bnw_map_unit_moves_checkpoint_pair(mutated,source,identity,{});});
    const auto changed_map=runtime::make_bnw_map_plot_checkpoint(mutated,source,identity,{});
    refusal(pair.units,changed_map,identity);
    mutated=changed;
    ++unit_field(mutated.all_players->players[picked[1].first].units.units[picked[1].second],"m_iX");
    rejects([&]{(void)runtime::make_bnw_unit_moves_checkpoint(mutated,source,identity);});
    auto duplicate_source=source;
    auto &second=duplicate_source.all_players->players[picked[1].first].units.units[picked[1].second];
    auto first=duplicate_source.all_players->players[picked[0].first].units.units[picked[0].second];
    if(picked[0].first==picked[1].first) {
        unit_field(second,"m_iID")=unit_field(first,"m_iID");
        rejects([&]{(void)runtime::make_bnw_unit_moves_checkpoint(changed,duplicate_source,identity);});
    }
    auto document=saves::make_civis_import_document(loaded,source_sha256);
    document.sections.push_back(pair.map);document.sections.push_back(pair.units);
    const auto output=std::filesystem::current_path()/(
        "bnw-batch-moves-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".CivisSave");
    struct Cleanup {std::filesystem::path path;~Cleanup(){std::error_code e;std::filesystem::remove(path,e);}} cleanup{output};
    saves::write_civis_save(output,document);
    const auto sealed=runtime::identify_file(output);
    rejects([&]{saves::write_civis_save(output,document);});
    check(runtime::identify_file(output).sha256==sealed.sha256,"Exclusive writer altered an existing output");
    const auto reread=saves::read_civis_save(output);
    const saves::CivisSaveSection *map=nullptr,*units=nullptr;
    for(const auto &part:reread.sections) {
        if(part.id==pair.map.id)map=&part;
        if(part.id==pair.units.id)units=&part;
    }
    check(map && units && units->bytes==pair.units.bytes,"Atomic container lost batch wire");
    restored=source;runtime::restore_bnw_map_unit_checkpoint_pair(map,units,restored,identity,{});
    check(affected_stamp(restored)==affected_stamp(changed),"Physical V3 roundtrip differs");
}
void test_scalar_batch(const saves::SavedSession &imported,
    const runtime::SessionCheckpointIdentity &identity,const std::string &original_hash) {
    const auto &source=imported.snapshot;auto live=source;std::size_t units_changed=0;
    for(auto &player:live.all_players->players)for(auto &unit:player.units.units){
        if(units_changed==2)continue;
        auto &damage=unit_field(unit,"m_iDamage");auto &moves=unit_field(unit,"m_iMoves");
        damage=damage==17?19:17;moves=moves==11?13:11;
        ++units_changed;if(units_changed==2)break;
    }
    check(units_changed>=2,"Actual BNW fixture has fewer than two scalar mutation units");
    const auto pair=runtime::make_bnw_map_unit_scalar_checkpoint_pair(live,source,identity,{});
    check(pair.units.bytes.at(8)==4,"Scalar checkpoint did not emit v4");
    auto restored=source;runtime::restore_bnw_map_unit_checkpoint_pair(&pair.map,&pair.units,restored,identity,{});
    check(affected_stamp(restored)==affected_stamp(live),"Scalar batch roundtrip lost actual unit data");
    auto failed=source;const auto before=affected_stamp(failed);
    auto truncated=pair.units;truncated.bytes.pop_back();
    rejects([&]{runtime::restore_bnw_unit_checkpoint(truncated,failed,identity);});
    check(affected_stamp(failed)==before,"Truncated scalar batch partially published");
    auto late=pair.units;
    // Corrupt the LAST record's old value, after all earlier records parse.
    late=pair.units;late.bytes.at(late.bytes.size()-8)^=1;
    rejects([&]{runtime::restore_bnw_unit_checkpoint(late,failed,identity);});
    check(affected_stamp(failed)==before,"Late scalar validation partially published");
    auto identity_bad=identity;identity_bad.rules_sha256=std::string(64,'b');
    rejects([&]{runtime::restore_bnw_unit_checkpoint(pair.units,failed,identity_bad);});
    check(affected_stamp(failed)==before,"Wrong-rule scalar validation partially published");
    auto replay=restored;const auto after=affected_stamp(replay);
    rejects([&]{runtime::restore_bnw_unit_checkpoint(pair.units,replay,identity);});
    check(affected_stamp(replay)==after,"Duplicate scalar overlay partially published");
    auto invalid=live;auto selection_world=source;const auto chosen=select_simple_move(selection_world);
    auto &u=invalid.all_players->players[chosen.player].units.units[chosen.unit];unit_field(u,"m_eOwner")+=1;
    rejects([&]{(void)runtime::make_bnw_unit_scalar_checkpoint(invalid,source,identity);});
    invalid=live;unit_field(invalid.all_players->players[chosen.player].units.units[chosen.unit],"m_iDamage")=std::int64_t(INT32_MAX)+1;
    rejects([&]{(void)runtime::make_bnw_unit_scalar_checkpoint(invalid,source,identity);});
    invalid=live;unit_field(invalid.all_players->players[chosen.player].units.units[chosen.unit],"m_iX")+=1;
    rejects([&]{(void)runtime::make_bnw_unit_scalar_checkpoint(invalid,source,identity);});
    rejects([&]{(void)runtime::make_bnw_unit_scalar_checkpoint(source,source,identity);});
    auto document=saves::make_civis_import_document(imported.source,original_hash);
    document.sections.push_back(pair.map);document.sections.push_back(pair.units);
    const auto output=std::filesystem::current_path()/("bnw-unit-scalars-"+
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".CivisSave");
    struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove(p,e);}} cleanup{output};
    saves::write_civis_save(output,document);
    saves::SavedSessionDecodeContext context;context.decode_map_regions=true;context.decode_teams=true;
    context.decode_all_players=true;context.decode_supporting_classes=true;context.all_players=saves::AllPlayersContext{};
    context.permit_live_bnw_map_plots_overlay=true;context.permit_live_bnw_units_overlay=true;
    const auto loaded=saves::read_saved_session(output,context);
    check(loaded.source.live_bnw_units==pair.units.bytes,"Normal loader dropped v4 scalar overlay");
    auto normal=loaded.snapshot;
    runtime::restore_bnw_map_unit_checkpoint_pair(&pair.map,&pair.units,normal,identity,{});
    check(affected_stamp(normal)==affected_stamp(live),"Container/load scalar checkpoint lost data");
}

auto&cycle_fixture_block(auto&player){
    for(auto&b:player.administration.blocks)if(b.name&&std::string_view(b.name)=="pre_city")return b;
    throw std::runtime_error("Natural BNW source lacks pre_city block");
}
auto&cycle_fixture_ids(auto&player){
    for(auto&a:cycle_fixture_block(player).arrays)if(a.name&&std::string_view(a.name)=="m_UnitCycle")return a.values;
    throw std::runtime_error("Natural BNW source lacks physical cycle list");
}
auto&cycle_fixture_count(auto&player){
    for(auto&f:cycle_fixture_block(player).fields)if(f.name&&std::string_view(f.name)=="m_UnitCycle_wire_count")return f.value;
    throw std::runtime_error("Natural BNW source lacks cycle signed count");
}
std::string cycle_fixture_stamp(const saves::SavedSessionSnapshot&world){
    std::ostringstream out;out<<affected_stamp(world);
    const auto record=[&](auto&&self,const saves::PlayerAdministrationRecord&r)->void{
        out<<(r.name?r.name:"")<<':'<<r.index<<':'<<r.start_offset<<':'<<r.end_offset<<';';
        for(const auto&f:r.fields)out<<(f.name?f.name:"")<<'='<<f.value<<';';
        for(const auto&a:r.arrays){out<<(a.name?a.name:"")<<':'<<a.columns<<'[';for(auto v:a.values)out<<v<<',';out<<']';}
        for(const auto&b:r.blobs){out<<(b.name?b.name:"")<<'[';for(auto v:b.bytes)out<<static_cast<unsigned>(v)<<',';out<<']';}
        for(const auto&child:r.children)self(self,child);
    };
    for(const auto&p:world.all_players->players)for(const auto&b:p.administration.blocks)record(record,b);
    return out.str();
}
void test_cycle_lists(const saves::SavedSession&imported,
    const runtime::SessionCheckpointIdentity&identity,const std::string&source_hash){
    const auto&source=imported.snapshot;auto changed=source;std::optional<std::size_t>actor;
    for(std::size_t p=0;p<changed.all_players->players.size();++p){
        auto&player=changed.all_players->players[p];if(player.units.units.size()<2)continue;
        std::vector<std::int64_t>ids;
        for(auto&u:player.units.units)ids.push_back(unit_field(u,"m_iID"));
        // Actual source full IDs, arranged differently; detached encoding is
        // not a fabricated live UnitCycler receipt or runtime owner grant.
        std::reverse(ids.begin(),ids.end());if(ids==cycle_fixture_ids(player))std::rotate(ids.begin(),ids.begin()+1,ids.end());
        cycle_fixture_ids(player)=ids;cycle_fixture_count(player)=static_cast<std::int64_t>(ids.size());actor=p;break;
    }
    check(actor.has_value(),"Actual BNW save lacks two-unit cycle fixture");
    const auto wire=runtime::make_bnw_unit_cycle_checkpoint(changed,source,identity);
    check(wire.bytes.at(8)==5,"Actual cycle codec did not emit version5");
    auto restored=source;runtime::restore_bnw_unit_checkpoint(wire,restored,identity);
    check(cycle_fixture_stamp(restored)==cycle_fixture_stamp(changed),"Cycle roundtrip differs from actual ordered source IDs");
    auto failed=source;const auto before=cycle_fixture_stamp(failed);
    auto bad=wire;write_i32(bad,bad.bytes.size()-4,-1);
    rejects([&]{runtime::restore_bnw_unit_checkpoint(bad,failed,identity);});
    check(cycle_fixture_stamp(failed)==before,"Late invalid cycle full ID published earlier list stores");
    for(const auto size:std::array<std::size_t,3>{12,wire.bytes.size()-8,wire.bytes.size()-1}){
        bad=wire;bad.bytes.resize(size);rejects([&]{runtime::restore_bnw_unit_checkpoint(bad,failed,identity);});
        check(cycle_fixture_stamp(failed)==before,"Truncated cycle list mutated receiver");
    }
    bad=wire;bad.bytes.push_back(0);rejects([&]{runtime::restore_bnw_unit_checkpoint(bad,failed,identity);});
    auto wrong=identity;wrong.rules_sha256=std::string(64,'b');rejects([&]{runtime::restore_bnw_unit_checkpoint(wire,failed,wrong);});
    check(cycle_fixture_stamp(failed)==before,"Wrong cycle identity published receiver");
    auto malformed=changed;auto&player=malformed.all_players->players[*actor];
    cycle_fixture_ids(player).back()=cycle_fixture_ids(player).front();rejects([&]{(void)runtime::make_bnw_unit_cycle_checkpoint(malformed,source,identity);});
    malformed=changed;cycle_fixture_count(malformed.all_players->players[*actor])+=1;rejects([&]{(void)runtime::make_bnw_unit_cycle_checkpoint(malformed,source,identity);});
    malformed=restored;
    const auto altered=cycle_fixture_stamp(malformed);rejects([&]{runtime::restore_bnw_unit_checkpoint(wire,malformed,identity);});
    check(cycle_fixture_stamp(malformed)==altered,"Changed actual old cycle source was mutated");
    // The ordinary loader has always required ID17 with ID18, including
    // a cycle-only change. Never fabricate a map delta to bypass that join.
    const auto map=runtime::make_bnw_map_plot_checkpoint(source,source,identity,{});
    auto document=saves::make_civis_import_document(imported.source,source_hash);
    document.sections.push_back(wire);
    rejects([&]{saves::validate_civis_import_document(document);});
    document.sections.push_back(map);
    saves::validate_civis_import_document(document);
    failed=source;
    rejects([&]{runtime::restore_bnw_map_unit_checkpoint_pair(nullptr,&wire,failed,identity,{});});
    check(cycle_fixture_stamp(failed)==before,"Missing cycle map companion changed receiver");
    auto changed_map_source=source;
    auto&anchor=plot_field(changed_map_source.map_plots->plots[0],"world_anchor");anchor=anchor==-1?0:-1;
    const auto changed_map=runtime::make_bnw_map_plot_checkpoint(changed_map_source,source,identity,{});
    rejects([&]{runtime::restore_bnw_map_unit_checkpoint_pair(&changed_map,&wire,failed,identity,{});});
    check(cycle_fixture_stamp(failed)==before,"Cycle accepted or published an independent map delta");
    bad=wire;write_i32(bad,bad.bytes.size()-4,-1);
    rejects([&]{runtime::restore_bnw_map_unit_checkpoint_pair(&map,&bad,failed,identity,{});});
    check(cycle_fixture_stamp(failed)==before,"Invalid cycle pair published a partial map/list");
    const auto output=std::filesystem::current_path()/("bnw-unit-cycle-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".CivisSave");
    struct Cleanup{std::filesystem::path p;~Cleanup(){std::error_code e;std::filesystem::remove(p,e);}}cleanup{output};
    saves::write_civis_save(output,document);
    saves::SavedSessionDecodeContext context;context.decode_map_regions=true;context.decode_teams=true;
    context.decode_all_players=true;context.decode_supporting_classes=true;context.all_players=saves::AllPlayersContext{};context.permit_live_bnw_units_overlay=true;
    rejects([&]{(void)saves::read_saved_session(output,context);});
    context.permit_live_bnw_map_plots_overlay=true;
    const auto loaded=saves::read_saved_session(output,context);
    check(loaded.source.live_bnw_units==wire.bytes&&loaded.source.live_bnw_map_plots==map.bytes,
          "Normal loader lost actual cycle/map companion sections");
    auto normal=loaded.snapshot;runtime::restore_bnw_map_unit_checkpoint_pair(&map,&wire,normal,identity,{});
    check(cycle_fixture_stamp(normal)==cycle_fixture_stamp(changed),"Physical CivisSave load lost cycle order/count or changed other domains");
}

void test_selected(const std::filesystem::path &path) {
    const auto original=runtime::identify_file(path);
    saves::SavedSessionDecodeContext context;
    context.decode_map_regions=true;context.decode_teams=true;
    context.decode_all_players=true;context.decode_supporting_classes=true;
    context.all_players=saves::AllPlayersContext{};
    auto imported=saves::read_saved_session(path,context);
    check(imported.snapshot.map_plots && imported.snapshot.map_projection &&
          imported.snapshot.all_players,"Selected Gandhi save lacks unit map/players");
    const auto chosen=select_simple_move(imported.snapshot);
    auto live=imported.snapshot;
    auto &unit=live.all_players->players[chosen.player].units.units[chosen.unit];
    const auto xy=live.map_header.grid.coordinates(chosen.new_plot);
    check(xy[0]<=static_cast<std::uint64_t>(INT32_MAX) &&
          xy[1]<=static_cast<std::uint64_t>(INT32_MAX),
          "Selected relocation coordinate exceeds native signed i32");
    const auto new_x=static_cast<std::int64_t>(xy[0]),new_y=static_cast<std::int64_t>(xy[1]);
    unit_field(unit,"m_iX")=new_x;unit_field(unit,"m_iY")=new_y;
    unit_field(unit,"m_iMoves")=-17;
    const auto owner=unit_field(unit,"m_eOwner"),id=unit_field(unit,"m_iID");
    plot_units(live.map_plots->plots[chosen.old_plot]).clear();
    plot_units(live.map_plots->plots[chosen.new_plot])={owner,id};
    live.map_projection->cells[chosen.old_plot].units=
        std::vector<saves::SavedPlotUnitReference>{};
    live.map_projection->cells[chosen.new_plot].units=
        std::vector<saves::SavedPlotUnitReference>{{owner,id}};
    auto &anchor=plot_field(live.map_plots->plots[chosen.old_plot],"world_anchor");
    anchor=anchor==-1?0:-1; // Independent map section content.
    const runtime::SessionCheckpointIdentity identity{
        runtime::identify_bytes(imported.source.bytes).sha256,std::string(64,'a')};
    rejects([&]{(void)runtime::make_bnw_map_plot_checkpoint(
        live,imported.snapshot,identity,{});});
    const auto pair=runtime::make_bnw_map_unit_checkpoint_pair(
        live,imported.snapshot,identity,{});
    check(pair.units.id==runtime::bnw_unit_checkpoint_section_id &&
          pair.units.bytes.at(8)==2 && pair.units.bytes.size()==244,"BNW unit section identity/version changed");
    auto bad=live;
    ++unit_field(bad.all_players->players[chosen.player].units.units[chosen.unit],"m_iDamage");
    rejects([&]{(void)runtime::make_bnw_unit_checkpoint(bad,imported.snapshot,identity);});
    bad=live;
    bad.all_players->players[chosen.player].units.units.pop_back();
    rejects([&]{(void)runtime::make_bnw_unit_checkpoint(bad,imported.snapshot,identity);});
    bad=live;
    plot_units(bad.map_plots->plots[chosen.new_plot]).push_back(999);
    rejects([&]{(void)runtime::make_bnw_unit_checkpoint(bad,imported.snapshot,identity);});
    auto document=saves::make_civis_import_document(imported.source,original.sha256);
    document.sections.push_back(pair.map);document.sections.push_back(pair.units);
    const auto output=std::filesystem::current_path()/(
        "bnw-unit-checkpoint-"+
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+
        ".CivisSave");
    saves::write_civis_save(output,document);
    struct Cleanup {std::filesystem::path path;~Cleanup(){
        std::error_code error;std::filesystem::remove(path,error);
    }} cleanup{output};
    const auto reloaded=saves::read_civis_save(output);
    rejects([&]{(void)saves::read_saved_session(output,context);});
    context.permit_live_bnw_map_plots_overlay=true;
    rejects([&]{(void)saves::read_saved_session(output,context);});
    context.permit_live_bnw_units_overlay=true;
    const auto imported_pair=saves::read_saved_session(output,context);
    check(imported_pair.source.live_bnw_map_plots==pair.map.bytes &&
          imported_pair.source.live_bnw_units==pair.units.bytes,
          "Overlay-aware import lost the paired BNW sections");
    const saves::CivisSaveSection *map=nullptr,*units=nullptr;
    for(const auto &section:reloaded.sections) {
        if(section.id==pair.map.id) map=&section;
        if(section.id==pair.units.id) units=&section;
    }
    check(map && units && map->bytes==pair.map.bytes && units->bytes==pair.units.bytes,
          "Atomic CivisSave lost BNW map or unit section");
    auto restored=imported.snapshot;
    rejects([&]{runtime::restore_bnw_map_unit_checkpoint_pair(
        map,nullptr,restored,identity,{});});
    check(unit_field(restored.all_players->players[chosen.player].units.units[chosen.unit],"m_iX")==
          unit_field(imported.snapshot.all_players->players[chosen.player].units.units[chosen.unit],"m_iX"),
          "Missing unit section mutated source world");
    auto corrupt=*units;corrupt.bytes.at(8)=3;
    rejects([&]{runtime::restore_bnw_map_unit_checkpoint_pair(
        map,&corrupt,restored,identity,{});});
    check(plot_units(restored.map_plots->plots[chosen.old_plot])==
          plot_units(imported.snapshot.map_plots->plots[chosen.old_plot]),
          "Failed unit restore partially published map mutation");
    runtime::restore_bnw_map_unit_checkpoint_pair(map,units,restored,identity,{});
    auto &loaded=restored.all_players->players[chosen.player].units.units[chosen.unit];
    check(unit_field(loaded,"m_iX")==new_x && unit_field(loaded,"m_iY")==new_y &&
          unit_field(loaded,"m_iMoves")==-17 &&
          plot_units(restored.map_plots->plots[chosen.old_plot]).empty() &&
          plot_units(restored.map_plots->plots[chosen.new_plot])==
              std::vector<std::int64_t>{owner,id} &&
          restored.map_projection->cells[chosen.new_plot].units==
              std::vector<saves::SavedPlotUnitReference>{{owner,id}} &&
          plot_field(restored.map_plots->plots[chosen.old_plot],"world_anchor")==anchor &&
          runtime::identify_file(path).sha256==original.sha256,
          "BNW unit and map sections did not restore one world");
    test_cycle_lists(imported,identity,original.sha256);
    test_scalar_batch(imported,identity,original.sha256);
    test_moves(imported.snapshot,chosen,live,identity);
    test_batch_moves(imported.snapshot,identity,imported.source,original.sha256);
    std::cout << "selected BNW unit=" << owner << ':' << id << " plot " <<
        chosen.old_plot << " -> " << chosen.new_plot << '\n';
}
} // namespace

int main(int argc,char **argv) {
    try {
        check(argc==2,"Supply selected Gandhi BNW save path");
        test_selected(argv[1]);
        std::cout << "PASS BNW unit checkpoint\n";
        return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL BNW unit checkpoint: " << error.what() << '\n';
        return 1;
    }
}
