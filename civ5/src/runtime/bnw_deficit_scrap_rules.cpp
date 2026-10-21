#include "civ5/runtime/bnw_deficit_scrap_rules.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <limits>
#include <set>
#include <stdexcept>
#include <string_view>
#include <variant>

namespace civ5::runtime {
namespace {
void require(bool valid,const char *message) {
    if (!valid) throw std::runtime_error(message);
}
std::int64_t define(const content::GlobalDefineCache &cache,
                    std::string_view name) {
    const auto *entry=cache.find(name);
    require(entry && entry->lookup.found && entry->lookup.error.empty() &&
            std::holds_alternative<std::int64_t>(entry->lookup.value),
            "BNW scrap define is unavailable");
    return std::get<std::int64_t>(entry->lookup.value);
}
void check_catalog_entry(const content::InfoTypeCatalog &types,
                         std::string_view table,std::string_view name,
                         std::int64_t id) {
    const auto *entry=types.find(name);
    require(entry && entry->table==table && entry->id==id &&
            entry->hash!=0 &&
            entry->hash==content::saved_info_type_hash(name) &&
            types.find_hash(entry->hash) &&
            types.find_hash(entry->hash)->type==name,
            "BNW scrap Info ID/hash differs from active SQL");
}
void check_roster(const content::InfoTypeCatalog &types,
                  std::string_view table,const std::set<std::string> &seen) {
    std::size_t total=0;
    for (const auto &[name,entry]:types.entries())
        if (entry.table==table) {
            ++total;
            require(seen.contains(name),
                    "BNW scrap Info catalog contains stale Type");
        }
    require(total==seen.size(),"BNW scrap Info catalog roster differs");
}
} // namespace

BnwDeficitScrapRules load_bnw_deficit_scrap_rules(
    const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,
    const ResearchContext &setup,
    const saves::LoadedSave &source,
    const SessionCheckpointIdentity &identity) {
    require(setup.profile==content::TechnologyProfile::bnw &&
            defines.profile()==content::GlobalDefineProfile::bnw &&
            types.report().all_succeeded() && setup.speed>=0 &&
            identity.payload_sha256.size()==64 &&
            identity.rules_sha256.size()==64 &&
            identify_bytes(source.bytes).sha256==identity.payload_sha256 &&
            identify_bytes(database.gameplay_fingerprint_data()).sha256==
                identity.rules_sha256,
            "BNW scrap requires matching selected payload, rules and profile");
    const auto fresh=database.cache_global_defines(
        content::GlobalDefineProfile::bnw);
    BnwDeficitScrapRules loaded;
    loaded.refund_percent=define(defines,"DISBAND_UNIT_REFUND_PERCENT");
    loaded.max_hit_points=define(defines,"MAX_HIT_POINTS");
    const auto disabled=define(defines,"UNIT_DELETE_DISABLED");
    require(loaded.refund_percent==define(fresh,"DISBAND_UNIT_REFUND_PERCENT") &&
            loaded.max_hit_points==define(fresh,"MAX_HIT_POINTS") &&
            disabled==define(fresh,"UNIT_DELETE_DISABLED"),
            "BNW scrap define cache differs from selected rules");
    require(loaded.refund_percent>=0 && loaded.refund_percent<=INT32_MAX &&
            loaded.max_hit_points>0 && loaded.max_hit_points<=INT32_MAX &&
            (disabled==0 || disabled==1),
            "BNW scrap define is outside source range");
    loaded.unit_delete_disabled=disabled==1;

    std::set<std::string> speeds;
    bool selected=false;
    const auto speed_report=database.prefetch_info("GameSpeeds",
        [&](auto slot,auto id,const auto &row) {
            const auto type=row.text("Type");
            require(id>=0 && static_cast<std::uint64_t>(id)==slot &&
                    type && !type->empty() &&
                    row.has_column("TrainPercent") &&
                    speeds.insert(*type).second,
                    "BNW scrap GameSpeeds ID/Type/TrainPercent unavailable");
            check_catalog_entry(types,"GameSpeeds",*type,id);
            if (id==setup.speed) {
                require(!selected,"Duplicate selected BNW game speed");
                loaded.train_percent=row.integer("TrainPercent");
                selected=true;
            }
            return true;
        });
    require(speed_report.all_succeeded() &&
            speed_report.overlapping_ids==0 && selected &&
            loaded.train_percent>=0 && loaded.train_percent<=INT32_MAX,
            "BNW scrap selected game speed is unavailable");
    check_roster(types,"GameSpeeds",speeds);

    std::set<std::string> units;
    const auto unit_report=database.prefetch_info("Units",
        [&](auto slot,auto id,const auto &row) {
            const auto type=row.text("Type");
            require(id>=0 && static_cast<std::uint64_t>(id)==slot &&
                    type && !type->empty() && row.has_column("Cost") &&
                    units.insert(*type).second,
                    "BNW scrap Units ID/Type/Cost unavailable");
            check_catalog_entry(types,"Units",*type,id);
            const auto cost=row.integer("Cost");
            require(cost>=INT32_MIN && cost<=INT32_MAX &&
                    loaded.production_cost_by_unit_hash.emplace(
                        content::saved_info_type_hash(*type),cost).second,
                    "BNW scrap unit production cost/hash is ambiguous");
            return true;
        });
    require(unit_report.all_succeeded() && unit_report.rows>0 &&
            unit_report.overlapping_ids==0,
            "BNW scrap Units collection unavailable");
    check_roster(types,"Units",units);
    return loaded;
}

} // namespace civ5::runtime
