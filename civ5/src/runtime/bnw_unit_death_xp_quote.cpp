#include "civ5/runtime/bnw_unit_death_xp_quote.hpp"
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
void require(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
std::string_view name(const char *value) {
    return value?std::string_view(value):std::string_view();
}
template<class Player> auto &counter(Player &player,std::string_view wanted) {
    using Field=std::conditional_t<std::is_const_v<Player>,
        const saves::PlayerCoreField,saves::PlayerCoreField>;
    Field *found=nullptr;
    require(player.core.layout==bnw,"BNW death XP player core unavailable");
    for(auto &field:player.core.fields)
        if(name(field.name)==wanted) {
            require(!found,"Duplicate BNW death XP player counter");
            found=&field;
        }
    require(found && found->encoding==saves::PlayerCoreEncoding::i32 &&
            found->values.size()==1 && !found->wire_count,
            "BNW death XP player counter unavailable");
    return found->values[0];
}
std::int64_t unit_value(const saves::SavedUnitObservation &unit,
                        std::string_view wanted,saves::UnitCoreEncoding encoding) {
    const saves::UnitCoreField *found=nullptr;
    require(unit.core.layout==bnw,"BNW death XP unit core unavailable");
    for(const auto &field:unit.core.fields)
        if(name(field.name)==wanted) {
            require(!found,"Duplicate BNW death XP unit field");found=&field;
        }
    require(found && found->encoding==encoding && found->values.size()==1,
            "BNW death XP unit field unavailable");
    return found->values[0];
}
void same_support(const BnwUnitSupportDeath &a,const BnwUnitSupportDeath &b) {
    const auto &x=a.class_death,&y=b.class_death;
    const auto &r=x.removal,&s=y.removal;
    require(r.player_index==s.player_index && r.unit_index==s.unit_index &&
            r.owner==s.owner && r.id==s.id && r.plot_index==s.plot_index &&
            r.area_index==s.area_index && r.area_id==s.area_id &&
            x.team==y.team && x.unit_class==y.unit_class &&
            x.team_count_before==y.team_count_before &&
            x.player_count_before==y.player_count_before &&
            a.builder_delta==b.builder_delta &&
            a.unhappiness_delta==b.unhappiness_delta &&
            a.extra_cost_delta==b.extra_cost_delta &&
            a.nuke_delta==b.nuke_delta &&
            a.military_delta==b.military_delta && a.before==b.before &&
            a.needs_active_player_ui_check==b.needs_active_player_ui_check,
            "BNW death XP prior support plan is stale");
}
void same_quote(const BnwDeathXpQuote &a,const BnwDeathXpQuote &b) {
    same_support(a.support,b.support);
    require(a.killer==b.killer && a.unit_experience==b.unit_experience &&
            a.loss==b.loss && a.triggered==b.triggered &&
            a.naval==b.naval &&
            a.setter_branch_unported==b.setter_branch_unported &&
            a.current_before==b.current_before &&
            a.current_after==b.current_after &&
            a.lifetime_before==b.lifetime_before &&
            a.lifetime_after==b.lifetime_after,
            "BNW death XP quote is stale or changed");
}
} // namespace

BnwDeathXpRules load_bnw_death_xp_rules(
    const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,
    const BnwUnitClassDeathRules &class_rules) {
    require(types.report().all_succeeded() && !class_rules.units.empty() &&
            defines.profile()==content::GlobalDefineProfile::bnw,
            "BNW death XP requires active BNW rules and catalog");
    const auto *define=defines.find("UNIT_DEATH_XP_GREAT_GENERAL_LOSS");
    require(define && define->lookup.error.empty() &&
            std::holds_alternative<std::int64_t>(define->lookup.value),
            "BNW death XP global define unavailable");
    BnwDeathXpRules result;
    result.loss_percent=std::get<std::int64_t>(define->lookup.value);
    require(result.loss_percent>=0 && result.loss_percent<=INT32_MAX,
            "BNW death XP loss percent outside source i32 range");
    std::set<std::string> domains;
    const auto domain_report=database.prefetch_info("Domains",
        [&](std::uint64_t slot,std::int64_t id,const auto &row) {
            const auto type=row.text("Type");
            require(id>=0 && static_cast<std::uint64_t>(id)==slot &&
                    type && domains.insert(*type).second,
                    "BNW death XP Domain ID/Type roster changed");
            const auto *info=types.find(*type);
            require(info && info->table=="Domains" && info->id==id &&
                    info->hash==content::saved_info_type_hash(*type) &&
                    types.find_hash(info->hash) &&
                    types.find_hash(info->hash)->type==*type,
                    "BNW death XP Domain catalog identity changed");
            return true;
        });
    require(domain_report.all_succeeded() && !domain_report.overlapping_ids &&
            domains.contains("DOMAIN_LAND") && domains.contains("DOMAIN_SEA"),
            "BNW death XP Domain collection incomplete");
    const auto unit_report=database.prefetch_info("Units",
        [&](std::uint64_t slot,std::int64_t id,const auto &row) {
            require(id>=0 && static_cast<std::uint64_t>(id)==slot &&
                    row.has_column("Type") && row.has_column("Domain"),
                    "BNW death XP Units schema or ID changed");
            const auto type=row.text("Type"),domain=row.text("Domain");
            require(type && !type->empty() && domain && domains.contains(*domain),
                    "BNW death XP Unit Type/Domain unavailable");
            const auto *info=types.find(*type);
            const auto it=class_rules.units.find(id);
            require(info && info->table=="Units" && info->id==id &&
                    info->hash==content::saved_info_type_hash(*type) &&
                    types.find_hash(info->hash) &&
                    types.find_hash(info->hash)->type==*type &&
                    it!=class_rules.units.end() &&
                    it->second.unit_hash==info->hash,
                    "BNW death XP Unit identity differs from class rules");
            const auto kind=*domain=="DOMAIN_SEA"?BnwDeathXpDomain::sea:
                *domain=="DOMAIN_LAND"?BnwDeathXpDomain::land:
                *domain=="DOMAIN_HOVER"?BnwDeathXpDomain::hover:
                BnwDeathXpDomain::other;
            require(result.units.emplace(id,BnwDeathXpUnitRule{
                id,info->hash,kind}).second,
                "BNW death XP Unit ID duplicated");
            return true;
        });
    require(unit_report.all_succeeded() && !unit_report.overlapping_ids &&
            result.units.size()==class_rules.units.size(),
            "BNW death XP Unit roster incomplete");
    std::size_t catalog_units=0,catalog_domains=0;
    for(const auto &[type,info]:types.entries()) {
        if(info.table=="Units") {
            ++catalog_units;
            require(result.units.contains(info.id),
                    "BNW death XP catalog Unit roster changed");
        }
        if(info.table=="Domains") {
            ++catalog_domains;
            require(domains.contains(type),
                    "BNW death XP catalog Domain roster changed");
        }
    }
    require(catalog_units==result.units.size() &&
            catalog_domains==domains.size(),
            "BNW death XP active catalog count changed");
    return result;
}

BnwDeathXpQuote quote_bnw_unit_death_xp(
    const saves::SavedSessionSnapshot &source,
    const BnwUnitSupportDeath &support,
    const BnwUnitClassDeathRules &class_rules,
    const BnwUnitSupportRules &support_rules,
    const BnwDeathXpRules &xp_rules,
    std::int64_t killer) {
    same_support(support,plan_bnw_unit_support_death(
        source,support.class_death,class_rules,support_rules));
    require(killer>=-1 && killer<64,"BNW death XP killer ID invalid");
    const auto &removal=support.class_death.removal;
    const auto &player=source.all_players->players[removal.player_index];
    const auto &unit=player.units.units[removal.unit_index];
    const auto type=unit_value(unit,"m_eUnitType",saves::UnitCoreEncoding::i32);
    const auto hash=unit_value(unit,"idUnitType",saves::UnitCoreEncoding::hash32);
    const auto it=xp_rules.units.find(type);
    require(it!=xp_rules.units.end() && hash>=0 && hash<=UINT32_MAX &&
            it->second.unit_type==type &&
            it->second.unit_hash==static_cast<std::uint32_t>(hash) &&
            xp_rules.units.size()==class_rules.units.size() &&
            xp_rules.loss_percent>=0 && xp_rules.loss_percent<=INT32_MAX,
            "BNW death XP rules differ from source unit roster");
    BnwDeathXpQuote quote;quote.support=support;quote.killer=killer;
    quote.unit_experience=unit_value(unit,"m_iExperience",
        saves::UnitCoreEncoding::i32);
    require(quote.unit_experience>=0 && quote.unit_experience<=INT32_MAX,
            "BNW death XP unit experience outside source range");
    quote.triggered=quote.unit_experience>0 && killer!=-1;
    if(!quote.triggered) return quote;
    // Expansion2 CvUnit::getDomainType resolves HOVER from the current plot.
    require(it->second.domain!=BnwDeathXpDomain::hover,
            "BNW death XP DOMAIN_HOVER needs current-plot domain binding");
    // Source arithmetic is signed int; reject cases that would overflow it.
    const auto product=quote.unit_experience*xp_rules.loss_percent;
    require(product<=INT32_MAX,"BNW death XP source int multiplication overflows");
    quote.loss=product/100;
    quote.naval=it->second.domain==BnwDeathXpDomain::sea;
    quote.current_before=counter(player,quote.naval?
        "m_iNavalCombatExperience":"m_iCombatExperience");
    quote.lifetime_before=counter(player,"m_iLifetimeCombatExperience");
    require(quote.current_before>=0 && quote.current_before<=INT32_MAX &&
            quote.lifetime_before>=INT32_MIN &&
            quote.lifetime_before<=INT32_MAX &&
            quote.lifetime_before-quote.loss>=INT32_MIN,
            "BNW death XP player counter outside source range");
    quote.current_after=std::max<std::int64_t>(
        0,quote.current_before-quote.loss);
    quote.lifetime_after=quote.lifetime_before-quote.loss;
    quote.setter_branch_unported=quote.current_after!=quote.current_before;
    return quote;
}

saves::SavedSessionSnapshot project_bnw_unit_death_xp_safe(
    const saves::SavedSessionSnapshot &source,
    const BnwDeathXpQuote &quote,
    const BnwUnitClassDeathRules &class_rules,
    const BnwUnitSupportRules &support_rules,
    const BnwDeathXpRules &xp_rules) {
    same_quote(quote,quote_bnw_unit_death_xp(source,quote.support,
        class_rules,support_rules,xp_rules,quote.killer));
    require(!quote.setter_branch_unported,
            "BNW death XP setter/Great Person threshold effects unported");
    auto result=source;
    if(quote.triggered)
        counter(result.all_players->players[quote.support.class_death.removal.player_index],
            "m_iLifetimeCombatExperience")=quote.lifetime_after;
    return result;
}
} // namespace civ5::runtime
