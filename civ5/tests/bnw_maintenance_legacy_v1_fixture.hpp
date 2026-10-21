#pragma once
// Test-only pinned previous production ID34 v1 implementation.
#include "civ5/runtime/bnw_maintenance_expense_checkpoint.hpp"
#include "civ5/runtime/bnw_treasury_balance_checkpoint.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <array>
#include <climits>
#include <span>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <variant>

namespace civ5::runtime::maintenance_legacy_fixture {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
constexpr std::array<std::uint8_t,8> magic{'C','I','V','B','M','E','X','1'};
constexpr std::size_t players=64,wire_bytes=8+4+64*3+4+players*20;
void require(bool condition,const char *reason) {
    if(!condition) throw std::runtime_error(reason);
}
std::string_view name(const char *value) {
    return value?std::string_view(value):std::string_view();
}
void sha(std::string_view hash) {
    require(hash.size()==64 &&
        std::all_of(hash.begin(),hash.end(),[](char c) {
            return (c>='0' && c<='9') || (c>='a' && c<='f');
        }),"Invalid BNW maintenance source/rules identity");
}
struct Writer {
    std::vector<std::uint8_t> bytes;
    void u32(std::uint32_t value) {
        for(unsigned n=0;n<4;++n)
            bytes.push_back(static_cast<std::uint8_t>(value>>(n*8)));
    }
    void u64(std::uint64_t value) {
        for(unsigned n=0;n<8;++n)
            bytes.push_back(static_cast<std::uint8_t>(value>>(n*8)));
    }
    void i32(std::int64_t value) {
        require(value>=0 && value<=INT32_MAX,
                "BNW maintenance expense exceeds source i32 range");
        u32(static_cast<std::uint32_t>(value));
    }
    void i64(std::int64_t value) {u64(static_cast<std::uint64_t>(value));}
    void raw(std::string_view value) {
        bytes.insert(bytes.end(),value.begin(),value.end());
    }
    void string(std::string_view value) {
        require(!value.empty() && value.size()<=UINT32_MAX,
                "BNW maintenance schema name unavailable");
        u32(static_cast<std::uint32_t>(value.size()));raw(value);
    }
};
struct Reader {
    std::span<const std::uint8_t> bytes;
    std::size_t at=0;
    std::uint32_t u32() {
        require(at<=bytes.size() && bytes.size()-at>=4,
                "Truncated BNW maintenance expense section");
        std::uint32_t value=0;
        for(unsigned n=0;n<4;++n)
            value|=std::uint32_t(bytes[at++])<<(n*8);
        return value;
    }
    std::int64_t i32() {return static_cast<std::int32_t>(u32());}
    void hash(std::string_view expected) {
        require(at<=bytes.size() && bytes.size()-at>=expected.size() &&
                std::equal(expected.begin(),expected.end(),bytes.begin()+at),
                "BNW maintenance source, rules, roster or schema changed");
        at+=expected.size();
    }
};
const saves::PlayerAdministrationRecord &treasury(
    const saves::SavedPlayerObservation &player) {
    require(player.administration.layout==bnw &&
            player.administration.blocks.size()>1 &&
            name(player.administration.blocks[1].name)=="treasury",
            "BNW treasury administration block unavailable");
    return player.administration.blocks[1];
}
saves::PlayerAdministrationRecord &treasury(saves::SavedPlayerObservation &player) {
    (void)treasury(std::as_const(player));
    return player.administration.blocks[1];
}
const saves::NamedSaveInteger &field(
    const saves::PlayerAdministrationRecord &block,std::string_view wanted) {
    const saves::NamedSaveInteger *found=nullptr;
    for(const auto &entry:block.fields) if(name(entry.name)==wanted) {
        require(!found,"Duplicate BNW maintenance expense field");found=&entry;
    }
    require(found,"Missing BNW maintenance expense field");return *found;
}
saves::NamedSaveInteger &field(
    saves::PlayerAdministrationRecord &block,std::string_view wanted) {
    (void)field(std::as_const(block),wanted);
    for(auto &entry:block.fields) if(name(entry.name)==wanted) return entry;
    throw std::runtime_error("Missing mutable BNW maintenance expense field");
}
template<class Core> std::int64_t core_id(const Core &core,
                                          std::string_view wanted) {
    const typename decltype(core.fields)::value_type *found=nullptr;
    for(const auto &entry:core.fields) if(name(entry.name)==wanted) {
        require(!found,"Duplicate BNW maintenance roster ID");found=&entry;
    }
    require(found && found->values.size()==1,
            "Missing BNW maintenance roster ID");
    return found->values[0];
}
void shape(const saves::SavedSessionSnapshot &world) {
    require(world.game_state.layout==bnw && world.pregame &&
            world.pregame->layout==bnw && world.all_players &&
            world.all_players->layout==bnw &&
            world.all_players->context.player_count==players &&
            world.all_players->players.size()==players &&
            world.all_players->context.profile_extensions.values.empty(),
            "BNW maintenance needs complete plain 64-player roster");
    for(std::size_t i=0;i<players;++i) {
        const auto &player=world.all_players->players[i];
        require(player.index==i &&
                player.saved_player_id==static_cast<std::int64_t>(i) &&
                player.cities.layout==bnw && player.units.layout==bnw &&
                player.cities.header.wire_city_count>=0 &&
                static_cast<std::uint64_t>(
                    player.cities.header.wire_city_count)==player.cities.cities.size() &&
                player.units.header.wire_unit_count>=0 &&
                static_cast<std::uint64_t>(
                    player.units.header.wire_unit_count)==player.units.units.size() &&
                core_id(player.core,"m_eID")==static_cast<std::int64_t>(i),
                "BNW maintenance player, city or unit roster changed");
        (void)field(treasury(player),"m_iExpensePerTurnUnitMaintenance");
        (void)field(treasury(player),"m_iExpensePerTurnUnitSupply");
    }
}
bool gold_field(std::string_view key) {
    return key=="m_iGold" || key=="m_iLifetimeGrossGoldIncome";
}
bool expense_field(std::string_view key) {
    return key=="m_iExpensePerTurnUnitMaintenance" ||
           key=="m_iExpensePerTurnUnitSupply";
}
bool history(std::string_view key) {
    return key=="m_GoldBalanceForTurnTimes100" ||
           key=="m_GoldChangeForTurnTimes100";
}
std::string schema_hash(const saves::SavedSessionSnapshot &world) {
    shape(world);Writer out;
    const auto &all=*world.all_players;
    out.u64(all.start_offset);out.u64(all.end_offset);
    for(std::size_t i=0;i<players;++i) {
        const auto &player=all.players[i];
        const auto &block=treasury(player);
        out.u32(static_cast<std::uint32_t>(i));
        out.u64(player.administration.start_offset);
        out.u64(player.administration.end_offset);
        out.u64(player.cities.cities.size());
        for(std::size_t j=0;j<player.cities.cities.size();++j) {
            const auto &city=player.cities.cities[j];
            require(city.index==j && city.core.layout==bnw &&
                    core_id(city.core,"m_eOwner")==static_cast<std::int64_t>(i),
                    "BNW maintenance city owner or order changed");
            out.i64(core_id(city.core,"m_iID"));
        }
        out.u64(player.units.units.size());
        for(std::size_t j=0;j<player.units.units.size();++j) {
            const auto &unit=player.units.units[j];
            require(unit.index==j && unit.core.layout==bnw &&
                    core_id(unit.core,"m_eOwner")==static_cast<std::int64_t>(i),
                    "BNW maintenance unit owner or order changed");
            out.i64(core_id(unit.core,"m_iID"));
        }
        out.string(name(block.name));out.u64(block.index);
        out.u64(block.start_offset);out.u64(block.end_offset);
        out.u64(block.fields.size());
        for(const auto &entry:block.fields) {
            const auto key=name(entry.name);out.string(key);
            out.i64(gold_field(key) || expense_field(key)?0:entry.value);
        }
        out.u64(block.arrays.size());
        for(const auto &entry:block.arrays) {
            const auto key=name(entry.name);out.string(key);
            out.u64(entry.columns);
            out.u64(history(key)?0:entry.values.size());
            if(!history(key)) for(const auto value:entry.values) out.i64(value);
        }
        require(block.blobs.empty() && block.children.empty(),
                "BNW maintenance treasury has unsupported nested state");
    }
    return identify_bytes(out.bytes).sha256;
}
std::pair<std::int64_t,std::int64_t> expenses(
    const saves::SavedPlayerObservation &player) {
    const auto &block=treasury(player);
    const auto unit=field(block,"m_iExpensePerTurnUnitMaintenance").value;
    const auto supply=field(block,"m_iExpensePerTurnUnitSupply").value;
    require(unit>=0 && unit<=INT32_MAX && supply>=0 && supply<=INT32_MAX,
            "BNW maintenance expense outside source i32 range");
    return {unit,supply};
}
void assign_expenses(saves::SavedPlayerObservation &player,
                     std::pair<std::int64_t,std::int64_t> value) {
    auto &block=treasury(player);
    field(block,"m_iExpensePerTurnUnitMaintenance").value=value.first;
    field(block,"m_iExpensePerTurnUnitSupply").value=value.second;
}
void gold_companion_matches(const saves::SavedSessionSnapshot &world,
                            const saves::SavedSessionSnapshot &source,
                            const SessionCheckpointIdentity &identity,
                            const saves::CivisSaveSection *companion) {
    const auto *reference=&source;
    saves::SavedSessionSnapshot restored;
    if(companion) {
        restored=source;
        restore_bnw_treasury_balance_checkpoint(*companion,restored,source,identity);
        reference=&restored;
    }
    for(std::size_t i=0;i<players;++i) {
        const auto &actual=treasury(world.all_players->players[i]);
        const auto &expected=treasury(reference->all_players->players[i]);
        for(const auto key:{"m_iGold","m_iLifetimeGrossGoldIncome"})
            require(field(actual,key).value==field(expected,key).value,
                    "BNW maintenance requires matching ID30 gold companion");
        for(const auto key:{"m_GoldBalanceForTurnTimes100",
                            "m_GoldChangeForTurnTimes100"}) {
            const auto find=[](const auto &block,std::string_view wanted)
                ->const saves::PlotArrayObservation & {
                for(const auto &row:block.arrays)
                    if(name(row.name)==wanted) return row;
                throw std::runtime_error("Missing BNW gold history companion");
            };
            require(find(actual,key).values==find(expected,key).values,
                    "BNW maintenance requires matching ID30 history companion");
        }
    }
}
bool unit_equal(const MaintenanceUnit &a,const MaintenanceUnit &b) {
    return a.type==b.type && a.domain==b.domain && a.combat==b.combat &&
        a.military==b.military && a.civilian==b.civilian &&
        a.no_maintenance==b.no_maintenance && a.garrison==b.garrison &&
        a.source_core==b.source_core && a.profile_extensions.values.empty() &&
        b.profile_extensions.values.empty();
}
void owned_shape(const MaintenanceContext &live,
                 const MaintenanceContext &baseline,
                 const saves::SavedSessionSnapshot &world) {
    require(live.profile==content::TechnologyProfile::bnw &&
            baseline.profile==live.profile &&
            live.extensions.values.empty() && baseline.extensions.values.empty() &&
            live.players.size()==players && baseline.players.size()==players &&
            live.estimate_end_turn==baseline.estimate_end_turn &&
            live.default_end_turn==baseline.default_end_turn &&
            live.team_eras==baseline.team_eras &&
            std::holds_alternative<saves::Expansion2PrefixObservation>(
                world.game_state.prefix) &&
            live.elapsed_turns==std::get<saves::Expansion2PrefixObservation>(
                world.game_state.prefix).scalars[1],
            "BNW maintenance owned profile, clock or roster changed");
    for(std::size_t i=0;i<players;++i) {
        const auto id=static_cast<std::int64_t>(i);
        require(live.players.contains(id) && baseline.players.contains(id),
                "BNW maintenance owned player missing");
        const auto &a=live.players.at(id),&b=baseline.players.at(id);
        require(a.base_free==b.base_free &&
                a.gold_per_unit_times100==b.gold_per_unit_times100 &&
                a.extra_unit_cost==b.extra_unit_cost &&
                a.outside_units==b.outside_units &&
                a.unit_modifier==b.unit_modifier &&
                a.supply_modifier==b.supply_modifier &&
                a.building_modifier==b.building_modifier &&
                a.improvement_modifier==b.improvement_modifier &&
                a.land_modifier==b.land_modifier &&
                a.naval_modifier==b.naval_modifier &&
                a.base_building==b.base_building &&
                a.base_improvement==b.base_improvement &&
                a.diplomacy_gold==b.diplomacy_gold &&
                a.free_garrison==b.free_garrison &&
                a.barbarian==b.barbarian &&
                a.combat_modifiers==b.combat_modifiers &&
                a.units.size()==b.units.size() &&
                a.profile_extensions.values.empty() &&
                b.profile_extensions.values.empty(),
                "BNW maintenance changed state outside expense cache");
        for(const auto &[unit,old]:b.units)
            require(a.units.contains(unit) && unit_equal(a.units.at(unit),old),
                    "BNW maintenance changed unit context");
        const auto raw=expenses(world.all_players->players[i]);
        require(a.cached_unit_expense>=0 && a.cached_unit_expense<=INT32_MAX &&
                a.cached_supply_expense>=0 && a.cached_supply_expense<=INT32_MAX &&
                (raw==std::pair{a.cached_unit_expense,a.cached_supply_expense} ||
                 raw==std::pair{b.cached_unit_expense,b.cached_supply_expense}),
                "BNW maintenance raw/owned expense cache diverged");
    }
}
} // namespace

saves::CivisSaveSection make_bnw_maintenance_expense_checkpoint(
    const LiveSessionState &state,const saves::SavedSessionSnapshot &source,
    const MaintenanceContext &baseline,const SessionCheckpointIdentity &identity,
    const saves::CivisSaveSection *gold_companion) {
    sha(identity.payload_sha256);sha(identity.rules_sha256);
    require(!state.player_early_publication && !state.player_gold_publication &&
            !state.player_research_publication &&
            !state.game_clock.pending,
            "BNW maintenance has unpublished gameplay or clock effects");
    const auto source_hash=schema_hash(source);
    require(schema_hash(state.world)==source_hash,
            "BNW maintenance treasury schema or roster changed");
    gold_companion_matches(state.world,source,identity,gold_companion);
    owned_shape(state.maintenance,baseline,state.world);
    Writer out;out.bytes.insert(out.bytes.end(),magic.begin(),magic.end());
    out.u32(1);out.raw(identity.payload_sha256);out.raw(identity.rules_sha256);
    out.raw(source_hash);out.u32(players);
    bool changed=false;
    for(std::size_t i=0;i<players;++i) {
        const auto id=static_cast<std::int64_t>(i);
        const auto previous=expenses(source.all_players->players[i]);
        const auto current=expenses(state.world.all_players->players[i]);
        const auto &old=baseline.players.at(id);
        const auto &live=state.maintenance.players.at(id);
        require(previous==std::pair{old.cached_unit_expense,
                                     old.cached_supply_expense} &&
                (current==previous ||
                 current==std::pair{live.cached_unit_expense,
                                    live.cached_supply_expense}),
                "BNW maintenance baseline or raw transition changed");
        changed|=previous!=std::pair{live.cached_unit_expense,
                                     live.cached_supply_expense};
        out.i32(id);out.i32(previous.first);out.i32(live.cached_unit_expense);
        out.i32(previous.second);out.i32(live.cached_supply_expense);
    }
    require(changed && out.bytes.size()==wire_bytes,
            "BNW maintenance checkpoint unchanged or incomplete");
    return {bnw_maintenance_expense_checkpoint_section_id,0,
            std::move(out.bytes)};
}

BnwMaintenanceExpenseRestored restore_bnw_maintenance_expense_checkpoint(
    const saves::CivisSaveSection &section,
    const saves::SavedSessionSnapshot &world_before,
    const saves::SavedSessionSnapshot &source,
    const MaintenanceContext &maintenance_before,
    const MaintenanceContext &baseline,
    const SessionCheckpointIdentity &identity,
    const saves::CivisSaveSection *gold_companion) {
    sha(identity.payload_sha256);sha(identity.rules_sha256);
    require(section.id==bnw_maintenance_expense_checkpoint_section_id &&
            section.flags==0 && section.bytes.size()==wire_bytes &&
            std::equal(magic.begin(),magic.end(),section.bytes.begin()),
            "Unsupported BNW maintenance expense section");
    const auto source_hash=schema_hash(source);
    require(schema_hash(world_before)==source_hash,
            "BNW maintenance destination schema or roster changed");
    gold_companion_matches(world_before,source,identity,gold_companion);
    owned_shape(maintenance_before,baseline,world_before);
    Reader in{section.bytes,magic.size()};
    require(in.u32()==1,"Unsupported BNW maintenance expense version");
    in.hash(identity.payload_sha256);in.hash(identity.rules_sha256);
    in.hash(source_hash);
    require(in.u32()==players,"BNW maintenance player count changed");
    std::array<std::pair<std::int64_t,std::int64_t>,players> replacement{};
    bool changed=false;
    for(std::size_t i=0;i<players;++i) {
        const auto id=in.i32(),old_unit=in.i32(),next_unit=in.i32();
        const auto old_supply=in.i32(),next_supply=in.i32();
        require(id==static_cast<std::int64_t>(i) && old_unit>=0 &&
                next_unit>=0 && old_supply>=0 && next_supply>=0 &&
                expenses(source.all_players->players[i])==
                    std::pair{old_unit,old_supply} &&
                expenses(world_before.all_players->players[i])==
                    std::pair{old_unit,old_supply} &&
                baseline.players.at(id).cached_unit_expense==old_unit &&
                baseline.players.at(id).cached_supply_expense==old_supply &&
                maintenance_before.players.at(id).cached_unit_expense==old_unit &&
                maintenance_before.players.at(id).cached_supply_expense==old_supply,
                "BNW maintenance expense source or destination changed");
        replacement[i]={next_unit,next_supply};
        changed|=old_unit!=next_unit || old_supply!=next_supply;
    }
    require(in.at==in.bytes.size() && changed,
            "BNW maintenance expense section unchanged or incomplete");
    BnwMaintenanceExpenseRestored result{world_before,maintenance_before};
    for(std::size_t i=0;i<players;++i) {
        assign_expenses(result.world.all_players->players[i],replacement[i]);
        auto &owned=result.maintenance.players.at(static_cast<std::int64_t>(i));
        owned.cached_unit_expense=replacement[i].first;
        owned.cached_supply_expense=replacement[i].second;
    }
    return result;
}

} // namespace civ5::runtime
