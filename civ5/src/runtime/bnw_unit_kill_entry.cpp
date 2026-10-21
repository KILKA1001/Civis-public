#include "civ5/runtime/bnw_unit_kill_entry.hpp"
#include <climits>
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
void require(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
std::string_view name(const char *s) {
    return s?std::string_view(s):std::string_view();
}
std::int64_t scalar(const saves::SavedUnitObservation &unit,
                    std::string_view wanted) {
    const saves::UnitCoreField *found=nullptr;
    require(unit.core.layout==saves::GameStateLayout::expansion2_version1,
            "BNW kill entry unit core unavailable");
    for(const auto &field:unit.core.fields)
        if(name(field.name)==wanted) {
            require(!found,"Duplicate BNW kill entry unit field");
            found=&field;
        }
    require(found && found->encoding==saves::UnitCoreEncoding::i32 &&
            found->values.size()==1 && !found->wire_count,
            "BNW kill entry unit field unavailable");
    require(found->values[0]>=INT32_MIN && found->values[0]<=INT32_MAX,
            "BNW kill entry unit field outside source i32");
    return found->values[0];
}
void same_removal(const BnwUnitRemoval &a,const BnwUnitRemoval &b) {
    require(a.player_index==b.player_index && a.unit_index==b.unit_index &&
            a.plot_index==b.plot_index && a.area_index==b.area_index &&
            a.owner==b.owner && a.id==b.id && a.area_id==b.area_id,
            "BNW kill entry source roster changed");
}
} // namespace

BnwUnitKillEntry plan_bnw_unit_kill_entry(
    const saves::SavedSessionSnapshot &source,
    const BnwUnitRemoval &removal,
    std::int64_t killer) {
    same_removal(removal,plan_bnw_unit_removal(
        source,removal.player_index,removal.unit_index));
    require(killer>=-1 && killer<64,
            "BNW kill entry killer outside player roster");
    const auto &unit=source.all_players->players[removal.player_index]
        .units.units[removal.unit_index];
    BnwUnitKillEntry entry;entry.removal=removal;
    entry.lua_arguments.owner=scalar(unit,"m_eOwner");
    entry.lua_arguments.unit_id=scalar(unit,"m_iID");
    entry.lua_arguments.unit_type=scalar(unit,"m_eUnitType");
    entry.lua_arguments.x=scalar(unit,"m_iX");
    entry.lua_arguments.y=scalar(unit,"m_iY");
    entry.lua_arguments.delayed=false;
    entry.lua_arguments.killer=killer;
    entry.army_id=scalar(unit,"m_iArmyId");
    require(entry.lua_arguments.owner==removal.owner &&
            entry.lua_arguments.unit_id==removal.id &&
            entry.lua_arguments.unit_type>=0 &&
            entry.lua_arguments.x>=0 && entry.lua_arguments.y>=0 &&
            source.map_header.grid.index(entry.lua_arguments.x,
                entry.lua_arguments.y)==removal.plot_index &&
            entry.army_id>=-1,
            "BNW kill entry Lua/army identity differs from source");
    entry.requires_army_service=entry.army_id!=-1;
    return entry;
}
} // namespace civ5::runtime
