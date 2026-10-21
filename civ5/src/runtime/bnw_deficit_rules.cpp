#include "civ5/runtime/bnw_deficit_rules.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <string_view>
#include <variant>

namespace civ5::runtime {
namespace {
void require(bool value,const char *message) {
    if (!value) throw std::runtime_error(message);
}
std::int64_t integer(const content::GlobalDefineCache &defines,
                     std::string_view name) {
    const auto *found=defines.find(name);
    require(found && found->lookup.error.empty() &&
            std::holds_alternative<std::int64_t>(found->lookup.value),
            "BNW deficit define is unavailable");
    return std::get<std::int64_t>(found->lookup.value);
}
std::int64_t source_int(double value) {
    require(std::isfinite(value) && value>=0 &&
            value<static_cast<double>(INT32_MAX),
            "BNW deficit intrinsic power exceeds source int");
    return static_cast<std::int64_t>(value);
}
std::int64_t base_power(std::int64_t combat,std::int64_t ranged,
                        std::int64_t moves,bool suicide,
                        std::int64_t nuke,BnwDeficitDomain domain) {
    require(combat>=0 && combat<=INT32_MAX && ranged>=0 &&
            ranged<=INT32_MAX && moves>=0 && moves<=INT32_MAX,
            "Invalid BNW deficit unit combat or moves");
    auto power=source_int(std::pow(static_cast<double>(combat),1.5));
    auto ranged_power=source_int(std::pow(static_cast<double>(ranged),1.45));
    if (domain==BnwDeficitDomain::sea) ranged_power=ranged_power*3/4;
    power=std::max(power,ranged_power);
    // The SDK casts iPower to float before the movement multiplier.
    power=source_int(static_cast<float>(power)*
        std::pow(std::min(1.0,static_cast<double>(moves)),0.3));
    if (suicide) power/=2;
    if (nuke>0) {
        require(power<=INT32_MAX-4000,
                "BNW deficit nuke power exceeds source int");
        power+=4000;
    }
    return power;
}
struct PairModifier {std::int64_t attack=0,defense=0;};
struct ClassModifier {std::int64_t general=0,attack=0,defense=0;};
struct PromotionPower {
    std::int64_t city_attack=0,attack=0,defense=0,drop_range=0;
    bool blitz=false,setup=false,only_defensive=false;
    std::map<std::int64_t,PairModifier> terrains,features;
    std::map<std::int64_t,std::int64_t> combats,domains;
    std::map<std::int64_t,ClassModifier> classes;
};
std::int64_t percent_bonus(std::int64_t power,std::int64_t value,
                           std::int64_t divisor) {
    if (value<=0) return power;
    require(value<=INT32_MAX && power>=0 && power<=INT32_MAX &&
            power*value<=INT32_MAX,
            "BNW promotion power multiply exceeds source int");
    const auto bonus=(power*value/divisor)/100;
    require(bonus<=INT32_MAX-power,
            "BNW promotion power addition exceeds source int");
    return power+bonus;
}
std::int64_t fraction_bonus(std::int64_t power,std::int64_t divisor,
                            bool subtract=false) {
    const auto bonus=power/divisor;
    require(subtract || bonus<=INT32_MAX-power,
            "BNW promotion power addition exceeds source int");
    return subtract?power-bonus:power+bonus;
}
std::int64_t promoted_power(std::int64_t power,std::int64_t ranged,
                            const std::set<std::int64_t> &free,
                            const std::map<std::int64_t,PromotionPower> &promotions) {
    // Expansion2 CvUnitClasses.cpp:1156 CvUnitEntry::DoUpdatePower loops
    // promotion IDs, then terrain/feature/combat/class/domain IDs. Apply each
    // positive modifier immediately; aggregating first changes truncation.
    for (const auto id:free) {
        const auto &p=promotions.at(id);
        power=percent_bonus(power,p.city_attack,2);
        power=percent_bonus(power,p.attack,2);
        power=percent_bonus(power,p.defense,2);
        if (p.drop_range>0) power=fraction_bonus(power,4);
        if (p.blitz) power=fraction_bonus(power,5);
        if (p.setup) power=fraction_bonus(power,5,true);
        if (p.only_defensive && ranged==0)
            power=fraction_bonus(power,4,true);
        for (const auto &[id,value]:p.terrains) {
            (void)id;
            power=percent_bonus(power,value.attack,4);
            power=percent_bonus(power,value.defense,4);
        }
        for (const auto &[id,value]:p.features) {
            (void)id;
            power=percent_bonus(power,value.attack,4);
            power=percent_bonus(power,value.defense,4);
        }
        for (const auto &[id,value]:p.combats) {
            (void)id;power=percent_bonus(power,value,4);
        }
        for (const auto &[id,value]:p.classes) {
            (void)id;
            // CvUnitClasses.cpp:1324 divides general UnitClass bonuses by eight.
            power=percent_bonus(power,value.general,8);
            power=percent_bonus(power,value.attack,10);
            power=percent_bonus(power,value.defense,10);
        }
        for (const auto &[id,value]:p.domains) {
            (void)id;power=percent_bonus(power,value,4);
        }
    }
    return power;
}
} // namespace

BnwDeficitRuleLoad load_bnw_deficit_rules(
    const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines) {
    require(types.report().all_succeeded() &&
            defines.profile()==content::GlobalDefineProfile::bnw,
            "BNW deficit requires active BNW Info catalog and defines");
    BnwDeficitRuleLoad loaded;
    loaded.rules.max_hit_points=integer(defines,"MAX_HIT_POINTS");
    const auto deletion=integer(defines,"UNIT_DELETE_DISABLED");
    require(loaded.rules.max_hit_points>0 &&
            loaded.rules.max_hit_points<=INT32_MAX &&
            (deletion==0 || deletion==1),
            "Invalid BNW deficit hit-point or deletion define");
    loaded.rules.unit_delete_disabled=deletion==1;

    std::set<std::string> domain_names;
    const auto domains=database.prefetch_info("Domains",[&](auto slot,auto id,
                                                             const auto &row) {
        const auto type=row.text("Type");
        require(id>=0 && static_cast<std::uint64_t>(id)==slot &&
                type && !type->empty() && domain_names.insert(*type).second,
                "BNW deficit Domain ID/Type is unresolved");
        const auto *entry=types.find(*type);
        require(entry && entry->table=="Domains" && entry->id==id &&
                entry->hash==content::saved_info_type_hash(*type) &&
                types.find_hash(entry->hash) &&
                types.find_hash(entry->hash)->type==*type,
                "BNW deficit Domain Info ID/hash disagrees with active SQL");
        return true;
    });
    require(domains.all_succeeded() && domains.rows>0 &&
            domains.overlapping_ids==0 &&
            domain_names.contains("DOMAIN_LAND") &&
            domain_names.contains("DOMAIN_SEA"),
            "BNW deficit Domains collection is incomplete");

    std::set<std::string> unit_names;
    std::map<std::string,std::uint32_t> hashes;
    std::map<std::string,std::int64_t> ranged_by_unit;
    const auto report=database.prefetch_info("Units",[&](auto slot,auto id,
                                                         const auto &row) {
        require(id>=0 && static_cast<std::uint64_t>(id)==slot &&
                row.has_column("Type") && row.has_column("Domain") &&
                row.has_column("Combat") && row.has_column("RangedCombat") &&
                row.has_column("Moves") && row.has_column("Suicide") &&
                row.has_column("NukeDamageLevel"),
                "BNW deficit Units schema or ID is unresolved");
        const auto type=row.text("Type"),domain=row.text("Domain");
        require(type && !type->empty() && domain && !domain->empty() &&
                unit_names.insert(*type).second,
                "Duplicate or missing BNW deficit unit Type/Domain");
        const auto *entry=types.find(*type);
        require(entry && entry->table=="Units" && entry->id==id &&
                entry->hash==content::saved_info_type_hash(*type) &&
                entry->hash!=0 && types.find_hash(entry->hash) &&
                types.find_hash(entry->hash)->type==*type,
                "BNW deficit Unit Info ID/hash disagrees with active SQL");
        const auto *domain_info=types.find(*domain);
        require(domain_names.contains(*domain) && domain_info &&
                domain_info->table=="Domains" &&
                domain_info->id>=0 &&
                types.find_hash(domain_info->hash) &&
                types.find_hash(domain_info->hash)->type==*domain,
                "BNW deficit unit Domain Info is unresolved");
        auto domain_kind=BnwDeficitDomain::other;
        if (*domain=="DOMAIN_LAND") domain_kind=BnwDeficitDomain::land;
        else if (*domain=="DOMAIN_SEA") domain_kind=BnwDeficitDomain::sea;
        const auto intrinsic=base_power(row.integer("Combat"),
            row.integer("RangedCombat"),row.integer("Moves"),
            row.boolean("Suicide"),row.integer("NukeDamageLevel"),domain_kind);
        ranged_by_unit.emplace(*type,row.integer("RangedCombat"));
        require(hashes.emplace(*type,entry->hash).second &&
                loaded.rules.units.emplace(entry->hash,
                    BnwDeficitUnitInfo{domain_kind,intrinsic}).second,
                "BNW deficit unit hash is ambiguous");
        return true;
    });
    require(report.all_succeeded() && report.rows>0 &&
            report.overlapping_ids==0,
            "BNW deficit Units collection is incomplete");
    std::size_t catalog_units=0;
    for (const auto &[name,entry]:types.entries())
        if (entry.table=="Units") {
            ++catalog_units;
            require(unit_names.contains(name),
                    "BNW deficit Info catalog contains stale unit Type");
        }
    require(catalog_units==unit_names.size(),
            "BNW deficit Info catalog unit roster changed");

    const auto category=[&](const char *table) {
        std::map<std::string,std::int64_t> names;
        const auto report=database.prefetch_info(table,[&](auto slot,auto id,
                                                           const auto &row) {
            const auto type=row.text("Type");
            require(id>=0 && static_cast<std::uint64_t>(id)==slot &&
                    type && !type->empty() && names.emplace(*type,id).second,
                    "BNW promotion category ID/Type is unresolved");
            const auto *entry=types.find(*type);
            require(entry && entry->table==table && entry->id==id &&
                    entry->hash==content::saved_info_type_hash(*type) &&
                    types.find_hash(entry->hash) &&
                    types.find_hash(entry->hash)->type==*type,
                    "BNW promotion category Info differs from active SQL");
            return true;
        });
        require(report.all_succeeded() && report.overlapping_ids==0,
                "BNW promotion category collection unavailable");
        for (const auto &[type,entry]:types.entries())
            if (entry.table==table)
                require(names.contains(type),
                        "BNW promotion category catalog is stale");
        return names;
    };
    const auto terrains=category("Terrains");
    const auto features=category("Features");
    const auto combats=category("UnitCombatInfos");
    const auto classes=category("UnitClasses");
    const auto promotion_names=category("UnitPromotions");
    std::map<std::int64_t,PromotionPower> promotion_rules;
    const auto promotion_report=database.prefetch_info("UnitPromotions",
        [&](auto slot,auto id,const auto &row) {
            (void)slot;
            const auto type=row.text("Type");
            require(type && promotion_names.contains(*type) &&
                    row.has_column("CityAttack") &&
                    row.has_column("AttackMod") &&
                    row.has_column("DefenseMod") &&
                    row.has_column("DropRange") &&
                    row.has_column("Blitz") &&
                    row.has_column("MustSetUpToRangedAttack") &&
                    row.has_column("OnlyDefensive"),
                    "BNW promotion power scalar schema incomplete");
            PromotionPower p;
            p.city_attack=row.integer("CityAttack");
            p.attack=row.integer("AttackMod");
            p.defense=row.integer("DefenseMod");
            p.drop_range=row.integer("DropRange");
            p.blitz=row.boolean("Blitz");
            p.setup=row.boolean("MustSetUpToRangedAttack");
            p.only_defensive=row.boolean("OnlyDefensive");
            require(promotion_rules.emplace(id,std::move(p)).second,
                    "Duplicate BNW promotion ID");
            return true;
        });
    require(promotion_report.all_succeeded() &&
            promotion_report.overlapping_ids==0,
            "BNW promotion power collection unavailable");
    const auto links=[&](const char *table,const char *target,
                         const std::map<std::string,std::int64_t> &catalog,
                         const auto &assign) {
        const auto query=database.query_rows(
            {std::string("SELECT * FROM ")+table,{}},[&](const auto &row) {
                const auto promotion=row.text("PromotionType");
                const auto type=row.text(target);
                require(promotion && promotion_names.contains(*promotion) &&
                        type && catalog.contains(*type),
                        "BNW promotion power link is unresolved");
                assign(promotion_rules.at(promotion_names.at(*promotion)),
                       catalog.at(*type),row);
            });
        require(query.all_succeeded(),
                "BNW promotion power link table unavailable");
    };
    links("UnitPromotions_Terrains","TerrainType",terrains,
        [&](PromotionPower &p,std::int64_t id,const auto &row) {
            require(row.has_column("Attack") && row.has_column("Defense") &&
                    p.terrains.emplace(id,PairModifier{
                        row.integer("Attack"),row.integer("Defense")}).second,
                    "Duplicate or incomplete BNW terrain promotion modifier");
        });
    links("UnitPromotions_Features","FeatureType",features,
        [&](PromotionPower &p,std::int64_t id,const auto &row) {
            require(row.has_column("Attack") && row.has_column("Defense") &&
                    p.features.emplace(id,PairModifier{
                        row.integer("Attack"),row.integer("Defense")}).second,
                    "Duplicate or incomplete BNW feature promotion modifier");
        });
    links("UnitPromotions_UnitCombatMods","UnitCombatType",combats,
        [&](PromotionPower &p,std::int64_t id,const auto &row) {
            require(row.has_column("Modifier") &&
                    p.combats.emplace(id,row.integer("Modifier")).second,
                    "Duplicate or incomplete BNW combat promotion modifier");
        });
    links("UnitPromotions_UnitClasses","UnitClassType",classes,
        [&](PromotionPower &p,std::int64_t id,const auto &row) {
            require(row.has_column("Modifier") && row.has_column("Attack") &&
                    row.has_column("Defense") &&
                    p.classes.emplace(id,ClassModifier{row.integer("Modifier"),
                        row.integer("Attack"),row.integer("Defense")}).second,
                    "Duplicate or incomplete BNW class promotion modifier");
        });
    std::map<std::string,std::int64_t> domain_ids;
    for (const auto &type:domain_names)
        domain_ids.emplace(type,types.find(type)->id);
    links("UnitPromotions_Domains","DomainType",domain_ids,
        [&](PromotionPower &p,std::int64_t id,const auto &row) {
            require(row.has_column("Modifier") &&
                    p.domains.emplace(id,row.integer("Modifier")).second,
                    "Duplicate or incomplete BNW domain promotion modifier");
        });

    std::map<std::string,std::set<std::int64_t>> free_by_unit;
    const auto promotions=database.query_rows(
        {"SELECT UnitType,PromotionType FROM Unit_FreePromotions",{}},
        [&](const auto &row) {
            const auto unit=row.text("UnitType");
            const auto promotion=row.text("PromotionType");
            const auto *promo=promotion?types.find(*promotion):nullptr;
            require(unit && hashes.contains(*unit) && promo &&
                    promo->table=="UnitPromotions" && promo->id>=0 &&
                    types.find_hash(promo->hash) &&
                    types.find_hash(promo->hash)->type==*promotion,
                    "BNW deficit free promotion is unresolved");
            free_by_unit[*unit].insert(promo->id);
        });
    require(promotions.all_succeeded(),
            "BNW deficit free-promotion collection unavailable");
    for (const auto &[type,free]:free_by_unit) {
        const auto hash=hashes.at(type);
        const auto info=loaded.rules.units.at(hash);
        loaded.rules.units.at(hash).intrinsic_power=
            promoted_power(info.intrinsic_power,ranged_by_unit.at(type),free,
                           promotion_rules);
    }
    return loaded;
}

} // namespace civ5::runtime
