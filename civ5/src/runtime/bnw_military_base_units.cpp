#include "civ5/runtime/bnw_military_base_units.hpp"
#include <limits>
#include <set>
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
void require(bool ok,const char *message) {
    if (!ok) throw std::runtime_error(message);
}
std::int64_t source_int(std::int64_t value) {
    require(value>=0 && value<=INT32_MAX,
            "BNW military unit rule exceeds SDK int range");
    return value;
}
std::int64_t scalar(const saves::UnitCoreObservation &core,
                    std::string_view name) {
    const saves::UnitCoreField *found=nullptr;
    for (const auto &field:core.fields)
        if (field.name && std::string_view(field.name)==name) {
            require(!found,"Duplicate BNW military unit field");
            found=&field;
        }
    require(found && found->values.size()==1,
            "BNW military unit field unavailable");
    return found->values.front();
}
void increment(std::int64_t &counter) {
    require(counter<INT32_MAX,"BNW military unit count exceeds SDK int range");
    ++counter;
}
} // namespace

BnwMilitaryBaseUnitRules load_bnw_military_base_unit_rules(
    const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines) {
    require(types.report().all_succeeded() &&
            defines.profile()==content::GlobalDefineProfile::bnw,
            "BNW military unit Info catalog/define profile is unavailable");
    BnwMilitaryBaseUnitRules rules;
    const auto explorer=[&](std::string_view type) {
        const auto *entry=types.find(type);
        require(entry && entry->table=="UnitAIInfos" && entry->id>=0 &&
                entry->id<=INT32_MAX &&
                entry->hash==content::saved_info_type_hash(type) &&
                types.find_hash(entry->hash) &&
                types.find_hash(entry->hash)->type==type,
                "BNW explorer UnitAI ID/hash is unavailable");
        return entry->id;
    };
    rules.explore_ai=explorer("UNITAI_EXPLORE");
    rules.explore_sea_ai=explorer("UNITAI_EXPLORE_SEA");
    require(rules.explore_ai!=rules.explore_sea_ai,
            "BNW explorer UnitAI IDs overlap");
    std::set<std::string> domain_names;
    const auto domains=database.prefetch_info("Domains",[&](auto slot,auto id,
                                                             const auto &row) {
        const auto type=row.text("Type");
        require(id>=0 && static_cast<std::uint64_t>(id)==slot && type &&
                domain_names.insert(*type).second,
                "BNW military Domain ID/Type is unresolved");
        const auto *entry=types.find(*type);
        require(entry && entry->table=="Domains" && entry->id==id &&
                entry->hash==content::saved_info_type_hash(*type) &&
                types.find_hash(entry->hash) &&
                types.find_hash(entry->hash)->type==*type,
                "BNW military Domain Info disagrees with SQL");
        return true;
    });
    require(domains.all_succeeded() && domains.overlapping_ids==0 &&
            domain_names.contains("DOMAIN_LAND") &&
            domain_names.contains("DOMAIN_SEA") &&
            domain_names.contains("DOMAIN_AIR"),
            "BNW military Domains collection is incomplete");
    std::set<std::string> unit_names;
    const auto units=database.prefetch_info("Units",[&](auto slot,auto id,
                                                         const auto &row) {
        require(row.has_column("Type") && row.has_column("Domain") &&
                row.has_column("RangedCombat") && row.has_column("Range") &&
                row.has_column("Moves") && id>=0 &&
                static_cast<std::uint64_t>(id)==slot,
                "BNW military Units schema or ID is unresolved");
        const auto type=row.text("Type"),domain=row.text("Domain");
        require(type && !type->empty() && domain &&
                unit_names.insert(*type).second &&
                domain_names.contains(*domain),
                "BNW military Unit Type/Domain is unresolved");
        const auto *entry=types.find(*type);
        require(entry && entry->table=="Units" && entry->id==id &&
                entry->hash!=0 &&
                entry->hash==content::saved_info_type_hash(*type) &&
                types.find_hash(entry->hash) &&
                types.find_hash(entry->hash)->type==*type,
                "BNW military Unit Info disagrees with SQL");
        BnwMilitaryUnitDomain kind=BnwMilitaryUnitDomain::other;
        if (*domain=="DOMAIN_LAND") kind=BnwMilitaryUnitDomain::land;
        else if (*domain=="DOMAIN_SEA") kind=BnwMilitaryUnitDomain::sea;
        else if (*domain=="DOMAIN_AIR") kind=BnwMilitaryUnitDomain::air;
        else if (*domain=="DOMAIN_HOVER") kind=BnwMilitaryUnitDomain::hover;
        require(rules.units.emplace(entry->hash,BnwMilitaryUnitRule{
                    kind,id,source_int(row.integer("RangedCombat")),
                    source_int(row.integer("Range")),
                    source_int(row.integer("Moves"))}).second,
                "BNW military Unit hash is ambiguous");
        return true;
    });
    require(units.all_succeeded() && units.rows>0 &&
            units.overlapping_ids==0,
            "BNW military Units collection is incomplete");
    std::size_t catalog_units=0;
    for (const auto &[name,entry]:types.entries())
        if (entry.table=="Units") {
            ++catalog_units;
            require(unit_names.contains(name),
                    "BNW military Info catalog has stale Unit");
        }
    require(catalog_units==unit_names.size(),
            "BNW military Unit roster disagrees with Info catalog");
    rules.resolved=true;
    return rules;
}

BnwMilitaryBaseUnitCounts count_bnw_military_base_units(
    const LiveSessionState &state,std::int64_t player,
    const BnwMilitaryBaseUnitRules &rules,bool unit_roster_current) {
    constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
    require(state.world.game_state.layout==bnw && state.world.all_players &&
            state.world.all_players->layout==bnw &&
            state.setup.profile==content::TechnologyProfile::bnw &&
            rules.resolved && rules.explore_ai>=0 &&
            rules.explore_sea_ai>=0 && unit_roster_current && player>=0,
            "BNW military counts require current unit roster and active rules");
    const saves::SavedPlayerObservation *owner=nullptr;
    for (const auto &candidate:state.world.all_players->players)
        if (candidate.saved_player_id==player) {
            require(!owner,"Duplicate BNW military player");
            owner=&candidate;
        }
    require(owner && owner->units.layout==bnw,
            "BNW military player units unavailable");
    BnwMilitaryBaseUnitCounts result;
    std::set<std::int64_t> ids;
    for (const auto &unit:owner->units.units) {
        require(unit.core.layout==bnw,
                "BNW military unit profile mismatch");
        const auto id=scalar(unit.core,"m_iID");
        const auto unit_owner=scalar(unit.core,"m_eOwner");
        const auto type_hash=scalar(unit.core,"idUnitType");
        const auto type_id=scalar(unit.core,"m_eUnitType");
        const auto ai=scalar(unit.core,"m_eUnitAIType");
        const auto combat=scalar(unit.core,"m_iBaseCombat");
        const auto only_defensive=scalar(unit.core,"m_iOnlyDefensiveCount");
        const auto extra_range=scalar(unit.core,"m_iExtraRange");
        const auto extra_intercept=scalar(unit.core,"m_iExtraIntercept");
        const auto army=scalar(unit.core,"m_iArmyId");
        require(id>=0 && ids.insert(id).second && unit_owner==player &&
                type_hash>0 && type_hash<=UINT32_MAX &&
                combat>=0 && combat<=INT32_MAX &&
                only_defensive>=INT32_MIN && only_defensive<=INT32_MAX &&
                extra_range>=INT32_MIN && extra_range<=INT32_MAX &&
                extra_intercept>=INT32_MIN && extra_intercept<=INT32_MAX &&
                army>=-1 && army<=INT32_MAX,
                "BNW military unit identity or combat field invalid");
        const auto found=rules.units.find(static_cast<std::uint32_t>(type_hash));
        require(found!=rules.units.end(),
                "BNW military unit Type hash missing in active rules");
        const auto &info=found->second;
        require(type_id==info.id,
                "BNW military Unit ID/hash disagrees with active rules");
        const auto range=info.range+extra_range;
        require(range>=INT32_MIN && range<=INT32_MAX,
                "BNW military unit range exceeds SDK int range");
        // CvUnit::IsCanAttackWithMove / IsCanAttackRanged. Range and ranged
        // combat come from CvUnitEntry, not the saved unit's stale cache.
        const bool can_attack=(combat>0 && only_defensive<=0) ||
            (range>0 && info.ranged_combat>0);
        if (!can_attack || ai==rules.explore_ai ||
            ai==rules.explore_sea_ai) continue;
        require(info.domain!=BnwMilitaryUnitDomain::hover,
                "BNW DOMAIN_HOVER requires live plot-water domain");
        if (info.domain==BnwMilitaryUnitDomain::land) {
            increment(result.land);
            if (army!=-1) increment(result.land_in_armies);
            if (range>0 && info.ranged_combat>0)
                increment(result.ranged_land);
            else if (extra_intercept>0) increment(result.anti_air);
            else if (info.moves>2) increment(result.mobile_land);
            else increment(result.melee_land);
        } else if (info.domain==BnwMilitaryUnitDomain::sea) {
            increment(result.naval);
            if (army!=-1) increment(result.naval_in_armies);
        } else if (info.domain==BnwMilitaryUnitDomain::air)
            increment(result.air);
    }
    return result;
}

} // namespace civ5::runtime
