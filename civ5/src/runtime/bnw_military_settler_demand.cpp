#include "civ5/runtime/bnw_military_settler_demand.hpp"
#include <set>
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
void require(bool ok,const char *message) {
    if (!ok) throw std::runtime_error(message);
}
template<class Field> std::int64_t scalar(const std::vector<Field> &fields,
                                           std::string_view name) {
    const Field *found=nullptr;
    for (const auto &field:fields)
        if (field.name && std::string_view(field.name)==name) {
            require(!found,"Duplicate BNW settler demand field");
            found=&field;
        }
    require(found && found->values.size()==1,
            "BNW settler demand field unavailable");
    return found->values.front();
}
void increment(std::int64_t &value) {
    require(value<INT32_MAX,"BNW settler demand exceeds SDK int range");
    ++value;
}
} // namespace

BnwSettlerDemandRules load_bnw_settler_demand_rules(
    const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines) {
    require(types.report().all_succeeded() &&
            defines.profile()==content::GlobalDefineProfile::bnw,
            "BNW settler demand requires active BNW rules/catalog");
    BnwSettlerDemandRules rules;
    std::set<std::string> ai_names;
    const auto ai=database.prefetch_info("UnitAIInfos",[&](auto slot,auto id,
                                                             const auto &row) {
        const auto type=row.text("Type");
        require(id>=0 && id<=INT32_MAX &&
                static_cast<std::uint64_t>(id)==slot && type &&
                ai_names.insert(*type).second,
                "BNW UnitAI ID/Type is unresolved");
        const auto *entry=types.find(*type);
        require(entry && entry->table=="UnitAIInfos" && entry->id==id &&
                entry->hash==content::saved_info_type_hash(*type) &&
                types.find_hash(entry->hash) &&
                types.find_hash(entry->hash)->type==*type,
                "BNW UnitAI Info disagrees with active SQL");
        if (*type=="UNITAI_SETTLE") rules.settle_ai=id;
        return true;
    });
    require(ai.all_succeeded() && ai.overlapping_ids==0 &&
            rules.settle_ai>=0,
            "BNW UNITAI_SETTLE is unavailable");
    std::set<std::string> unit_names;
    const auto units=database.prefetch_info("Units",[&](auto slot,auto id,
                                                         const auto &row) {
        require(id>=0 && id<=INT32_MAX &&
                static_cast<std::uint64_t>(id)==slot &&
                row.has_column("Type") && row.has_column("DefaultUnitAI"),
                "BNW settler Unit schema/ID is unresolved");
        const auto type=row.text("Type");
        require(type && !type->empty() && unit_names.insert(*type).second,
                "BNW settler Unit Type is unresolved");
        const auto *entry=types.find(*type);
        require(entry && entry->table=="Units" && entry->id==id &&
                entry->hash!=0 &&
                entry->hash==content::saved_info_type_hash(*type) &&
                types.find_hash(entry->hash) &&
                types.find_hash(entry->hash)->type==*type,
                "BNW settler Unit Info disagrees with active SQL");
        std::int64_t default_ai=-1;
        const auto ai_type=row.text("DefaultUnitAI");
        if (ai_type && !ai_type->empty()) {
            const auto *ai_entry=types.find(*ai_type);
            require(ai_names.contains(*ai_type) && ai_entry &&
                    ai_entry->table=="UnitAIInfos" && ai_entry->id>=0 &&
                    ai_entry->id<=INT32_MAX,
                    "BNW Unit DefaultUnitAI is unresolved");
            default_ai=ai_entry->id;
        }
        require(rules.units.emplace(id,BnwSettlerUnitRule{
                    entry->hash,default_ai}).second,
                "Duplicate BNW settler Unit ID");
        return true;
    });
    require(units.all_succeeded() && units.rows>0 &&
            units.overlapping_ids==0,
            "BNW settler Units collection is incomplete");
    std::size_t catalog_units=0;
    for (const auto &[name,entry]:types.entries())
        if (entry.table=="Units") {
            ++catalog_units;
            require(unit_names.contains(name),
                    "BNW settler catalog has stale Unit");
        }
    require(catalog_units==unit_names.size(),
            "BNW settler Unit roster disagrees with catalog");
    rules.resolved=true;
    return rules;
}

BnwSettlerDemand bind_bnw_settler_demand(
    const LiveSessionState &state,std::int64_t player,
    const BnwSettlerDemandRules &rules,
    const BnwSettlerDemandEvidence &evidence) {
    constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
    require(state.world.game_state.layout==bnw && state.world.all_players &&
            state.world.all_players->layout==bnw &&
            state.setup.profile==content::TechnologyProfile::bnw &&
            state.cities.profile==content::TechnologyProfile::bnw &&
            rules.resolved && rules.settle_ai>=0 && player>=0 &&
            evidence.unit_roster_current && evidence.city_roster_current &&
            evidence.production_queues_current,
            "BNW settler demand requires current unit/city/queue state");
    const saves::SavedPlayerObservation *saved=nullptr;
    for (const auto &candidate:state.world.all_players->players)
        if (candidate.saved_player_id==player) {
            require(!saved,"Duplicate BNW settler demand player");
            saved=&candidate;
        }
    const auto live=state.cities.players.find(player);
    require(saved && saved->units.layout==bnw && saved->cities.layout==bnw &&
            live!=state.cities.players.end() &&
            saved->cities.cities.size()==live->second.cities.size(),
            "BNW settler demand city/unit owner or roster unavailable");
    BnwSettlerDemand result;
    std::set<std::int64_t> unit_ids,city_ids;
    for (const auto &unit:saved->units.units) {
        require(unit.core.layout==bnw,
                "BNW settler demand unit profile mismatch");
        const auto id=scalar(unit.core.fields,"m_iID");
        const auto owner=scalar(unit.core.fields,"m_eOwner");
        const auto type=scalar(unit.core.fields,"m_eUnitType");
        const auto hash=scalar(unit.core.fields,"idUnitType");
        const auto ai=scalar(unit.core.fields,"m_eUnitAIType");
        const auto info=rules.units.find(type);
        require(id>=0 && unit_ids.insert(id).second && owner==player &&
                info!=rules.units.end() && hash==info->second.hash &&
                ai>=-1 && ai<=INT32_MAX,
                "BNW settler demand unit ID/hash/AI disagrees with rules");
        if (ai==rules.settle_ai) increment(result.existing_settlers);
    }
    for (const auto &city:saved->cities.cities) {
        require(city.core.layout==bnw && city.production.layout==bnw,
                "BNW settler demand city profile mismatch");
        const auto id=scalar(city.core.fields,"m_iID");
        const auto owner=scalar(city.core.fields,"m_eOwner");
        const auto found=live->second.cities.find(id);
        require(id>=0 && city_ids.insert(id).second && owner==player &&
                found!=live->second.cities.end(),
                "BNW settler demand city owner/ID mismatch");
        const auto &current=found->second;
        require(current.id==id && current.owner==player &&
                current.source_core==std::optional{std::pair{
                    city.core.start_offset,city.core.end_offset}} &&
                current.production_orders && current.production_progress &&
                current.production_progress->source_profile==
                    content::TechnologyProfile::bnw,
                "BNW settler demand city/queue was not restored coherently");
        increment(result.cities);
        if (current.production_orders->empty()) continue;
        const auto &head=current.production_orders->front();
        if (head.type!=0) continue; // CvCity::isProductionUnit: ORDER_TRAIN.
        const auto unit=rules.units.find(head.item);
        require(unit!=rules.units.end() &&
                (!head.source_hash || *head.source_hash==unit->second.hash),
                "BNW settler demand current train order is unresolved");
        if (unit->second.default_ai==rules.settle_ai)
            increment(result.being_trained);
    }
    require(result.existing_settlers<=INT32_MAX-result.being_trained,
            "BNW settler demand total exceeds SDK int range");
    result.total_settlers=result.existing_settlers+result.being_trained;
    return result;
}

} // namespace civ5::runtime
