#include "civ5/runtime/bnw_unit_support_death.hpp"
#include "civ5/content/info_types.hpp"
#include <algorithm>
#include <climits>
#include <set>
#include <stdexcept>
#include <string_view>
#include <type_traits>

namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
constexpr std::array<std::string_view,5> counter_names{
    "m_iNumBuilders","m_iUnhappinessFromUnits","m_iExtraUnitCost",
    "m_iNumNukeUnits","m_iNumMilitaryUnits"};
void require(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
std::string_view name(const char *value) {
    return value?std::string_view(value):std::string_view();
}
template<class Player> auto &counter(Player &player,std::string_view wanted) {
    using Field=std::conditional_t<std::is_const_v<Player>,
        const saves::PlayerCoreField,saves::PlayerCoreField>;
    Field *found=nullptr;
    require(player.core.layout==bnw,"BNW unit death player core unavailable");
    for(auto &field:player.core.fields)
        if(name(field.name)==wanted) {
            require(!found,"Duplicate BNW unit death player counter");
            found=&field;
        }
    require(found && found->encoding==saves::PlayerCoreEncoding::i32 &&
            found->values.size()==1 && !found->wire_count,
            "BNW unit death player counter unavailable");
    return found->values[0];
}
std::int64_t unit_type(const saves::SavedUnitObservation &unit,
                       std::string_view wanted,saves::UnitCoreEncoding encoding) {
    const saves::UnitCoreField *found=nullptr;
    for(const auto &field:unit.core.fields)
        if(name(field.name)==wanted) {
            require(!found,"Duplicate BNW unit type ID/hash");found=&field;
        }
    require(found && found->encoding==encoding && found->values.size()==1,
            "BNW unit type ID/hash unavailable");
    return found->values[0];
}
std::int64_t add_checked(std::int64_t before,std::int64_t delta,
                         bool nonnegative) {
    require(before>=INT32_MIN && before<=INT32_MAX &&
            delta>=INT32_MIN && delta<=INT32_MAX &&
            before+delta>=INT32_MIN && before+delta<=INT32_MAX &&
            (!nonnegative || (before>=0 && before+delta>=0)),
            "BNW unit death player counter underflow/overflow");
    return before+delta;
}
void same_class(const BnwUnitClassDeath &a,const BnwUnitClassDeath &b) {
    require(a.removal.player_index==b.removal.player_index &&
            a.removal.unit_index==b.removal.unit_index &&
            a.removal.owner==b.removal.owner &&
            a.removal.id==b.removal.id &&
            a.removal.plot_index==b.removal.plot_index &&
            a.removal.area_index==b.removal.area_index &&
            a.removal.area_id==b.removal.area_id &&
            a.team==b.team && a.unit_class==b.unit_class &&
            a.team_count_before==b.team_count_before &&
            a.player_count_before==b.player_count_before,
            "BNW support death class stage is not source-bound");
}
void same_plan(const BnwUnitSupportDeath &a,const BnwUnitSupportDeath &b) {
    same_class(a.class_death,b.class_death);
    require(a.builder_delta==b.builder_delta &&
            a.unhappiness_delta==b.unhappiness_delta &&
            a.extra_cost_delta==b.extra_cost_delta &&
            a.nuke_delta==b.nuke_delta &&
            a.military_delta==b.military_delta && a.before==b.before &&
            a.needs_active_player_ui_check==b.needs_active_player_ui_check,
            "BNW support death plan is stale or changed");
}
} // namespace

BnwUnitSupportRules load_bnw_unit_support_rules(
    const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    const BnwUnitClassDeathRules &class_rules) {
    require(types.report().all_succeeded() && !class_rules.units.empty(),
            "BNW support death needs active class rules and Info catalog");
    std::set<std::string> domains;
    const auto domain_report=database.prefetch_info("Domains",
        [&](std::uint64_t slot,std::int64_t id,const auto &row) {
            const auto type=row.text("Type");
            require(id>=0 && static_cast<std::uint64_t>(id)==slot &&
                    type && !type->empty() && domains.insert(*type).second,
                    "BNW Domain ID/Type roster changed");
            const auto *info=types.find(*type);
            require(info && info->table=="Domains" && info->id==id &&
                    info->hash==content::saved_info_type_hash(*type) &&
                    types.find_hash(info->hash) &&
                    types.find_hash(info->hash)->type==*type,
                    "BNW Domain Info ID/hash differs from SQL");
            return true;
        });
    require(domain_report.all_succeeded() &&
            domain_report.overlapping_ids==0 &&
            domains.contains("DOMAIN_LAND"),
            "BNW Domains collection incomplete");
    BnwUnitSupportRules result;
    const auto unit_report=database.prefetch_info("Units",
        [&](std::uint64_t slot,std::int64_t id,const auto &row) {
            require(id>=0 && static_cast<std::uint64_t>(id)==slot &&
                    row.has_column("Type") && row.has_column("Class") &&
                    row.has_column("Domain") && row.has_column("WorkRate") &&
                    row.has_column("Unhappiness") &&
                    row.has_column("ExtraMaintenanceCost") &&
                    row.has_column("NukeDamageLevel") &&
                    row.has_column("MilitarySupport"),
                    "BNW Unit support schema or ID changed");
            const auto type=row.text("Type"),domain=row.text("Domain"),
                       unit_class=row.text("Class");
            require(type && !type->empty() && domain &&
                    domains.contains(*domain) && unit_class &&
                    !unit_class->empty(),
                    "BNW Unit support Type/Domain/Class unavailable");
            const auto *info=types.find(*type);
            const auto *class_info=types.find(*unit_class);
            const auto class_rule=class_rules.units.find(id);
            require(info && info->table=="Units" && info->id==id &&
                    info->hash==content::saved_info_type_hash(*type) &&
                    types.find_hash(info->hash) &&
                    types.find_hash(info->hash)->type==*type &&
                    class_rule!=class_rules.units.end() &&
                    class_rule->second.unit_hash==info->hash &&
                    class_info && class_info->table=="UnitClasses" &&
                    class_info->id==class_rule->second.unit_class &&
                    class_info->hash==class_rule->second.class_hash,
                    "BNW Unit support identity differs from class rules");
            const auto work_rate=row.integer("WorkRate");
            const auto unhappiness=row.integer("Unhappiness");
            const auto extra=row.integer("ExtraMaintenanceCost");
            const auto nuke=row.integer("NukeDamageLevel");
            const auto military=row.integer("MilitarySupport");
            require(work_rate>=0 && work_rate<=INT32_MAX &&
                    unhappiness>=INT32_MIN && unhappiness<=INT32_MAX &&
                    extra>=INT32_MIN && extra<=INT32_MAX &&
                    nuke>=-1 && nuke<=INT32_MAX &&
                    (military==0 || military==1),
                    "BNW Unit support rule outside source i32 range");
            require(result.units.emplace(id,BnwUnitSupportRule{
                id,info->hash,*domain=="DOMAIN_LAND",military==1,
                work_rate,unhappiness,extra,nuke}).second,
                "BNW Unit support ID duplicated");
            return true;
        });
    require(unit_report.all_succeeded() &&
            unit_report.overlapping_ids==0 &&
            result.units.size()==class_rules.units.size(),
            "BNW Unit support rules collection incomplete");
    std::size_t catalog_units=0,catalog_domains=0;
    for(const auto &[type,entry]:types.entries()) {
        if(entry.table=="Units") {
            ++catalog_units;
            require(result.units.contains(entry.id),
                    "BNW Unit support catalog contains stale Type");
        }
        if(entry.table=="Domains") {
            ++catalog_domains;
            require(domains.contains(type),
                    "BNW Domain catalog contains stale Type");
        }
    }
    require(catalog_units==result.units.size() &&
            catalog_domains==domains.size(),
            "BNW Unit support catalog roster changed");
    return result;
}

BnwUnitSupportDeath plan_bnw_unit_support_death(
    const saves::SavedSessionSnapshot &source,
    const BnwUnitClassDeath &class_death,
    const BnwUnitClassDeathRules &class_rules,
    const BnwUnitSupportRules &support_rules) {
    same_class(class_death,plan_bnw_unit_class_death(
        source,class_death.removal,class_rules));
    const auto &removal=class_death.removal;
    const auto &player=source.all_players->players[removal.player_index];
    const auto &unit=player.units.units[removal.unit_index];
    const auto type=unit_type(unit,"m_eUnitType",saves::UnitCoreEncoding::i32);
    const auto hash=unit_type(unit,"idUnitType",saves::UnitCoreEncoding::hash32);
    const auto it=support_rules.units.find(type);
    require(it!=support_rules.units.end() && hash>=0 && hash<=UINT32_MAX &&
            it->second.unit_type==type &&
            it->second.unit_hash==static_cast<std::uint32_t>(hash) &&
            support_rules.units.size()==class_rules.units.size(),
            "BNW Unit support rules differ from source unit/class roster");
    const auto &rule=it->second;
    BnwUnitSupportDeath plan;plan.class_death=class_death;
    plan.builder_delta=rule.work_rate>0 && rule.land?-1:0;
    plan.unhappiness_delta=-rule.unhappiness;
    plan.extra_cost_delta=-rule.extra_maintenance_cost;
    plan.nuke_delta=rule.nuke_damage_level!=-1?-1:0;
    plan.military_delta=rule.military_support?-1:0;
    const std::array<std::int64_t,5> deltas{
        plan.builder_delta,plan.unhappiness_delta,plan.extra_cost_delta,
        plan.nuke_delta,plan.military_delta};
    for(std::size_t i=0;i<counter_names.size();++i) {
        plan.before[i]=counter(player,counter_names[i]);
        (void)add_checked(plan.before[i],deltas[i],i==0 || i==3 || i==4);
    }
    plan.needs_active_player_ui_check=
        plan.extra_cost_delta!=0 || plan.military_delta!=0;
    return plan;
}

saves::SavedSessionSnapshot project_bnw_unit_support_death(
    const saves::SavedSessionSnapshot &source,
    const BnwUnitSupportDeath &plan,
    const BnwUnitClassDeathRules &class_rules,
    const BnwUnitSupportRules &support_rules) {
    same_plan(plan,plan_bnw_unit_support_death(
        source,plan.class_death,class_rules,support_rules));
    auto result=project_bnw_unit_class_death(source,plan.class_death,class_rules);
    auto &player=result.all_players->players[plan.class_death.removal.player_index];
    const std::array<std::int64_t,5> deltas{
        plan.builder_delta,plan.unhappiness_delta,plan.extra_cost_delta,
        plan.nuke_delta,plan.military_delta};
    for(std::size_t i=0;i<counter_names.size();++i)
        counter(player,counter_names[i])=add_checked(
            plan.before[i],deltas[i],i==0 || i==3 || i==4);
    return result;
}
} // namespace civ5::runtime
