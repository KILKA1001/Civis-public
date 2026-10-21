#include "civ5/runtime/bnw_unit_class_death.hpp"
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
std::int64_t core(const saves::SavedUnitObservation &unit,
                  std::string_view wanted,saves::UnitCoreEncoding encoding) {
    const saves::UnitCoreField *found=nullptr;
    for(const auto &field:unit.core.fields)
        if(name(field.name)==wanted) {
            require(!found,"Duplicate BNW unit type field");found=&field;
        }
    require(found && found->encoding==encoding && found->values.size()==1,
            "BNW unit type ID/hash unavailable");
    return found->values[0];
}
template<class Player> auto &player_class_counts(Player &player,std::uint64_t slots) {
    using Field=std::conditional_t<std::is_const_v<Player>,
        const saves::PlayerCoreField,saves::PlayerCoreField>;
    Field *found=nullptr;
    require(player.core.layout==bnw,"BNW player core unavailable");
    for(auto &field:player.core.fields)
        if(name(field.name)=="m_paiUnitClassCount") {
            require(!found,"Duplicate BNW player unit-class counts");found=&field;
        }
    require(found && found->encoding==saves::PlayerCoreEncoding::vector_i32 &&
            found->wire_count==slots && found->values.size()==slots,
            "BNW player unit-class count shape changed");
    return found->values;
}
template<class Team> auto &team_class_count(Team &team,
        const BnwUnitClassDeathRules &rules,std::int64_t unit_class) {
    using Array=std::conditional_t<std::is_const_v<Team>,
        const saves::TeamHashedArray,saves::TeamHashedArray>;
    Array *found=nullptr;
    for(auto &array:team.hashes)
        if(name(array.name)=="unit_class_count") {
            require(!found,"Duplicate BNW team unit-class counts");found=&array;
        }
    require(found && found->wire_count>=0 &&
            static_cast<std::uint64_t>(found->wire_count)==rules.class_slots &&
            found->rows.size()==rules.class_slots &&
            rules.class_hashes.size()==rules.class_slots,
            "BNW team unit-class hash roster shape changed");
    require(unit_class>=0 && static_cast<std::uint64_t>(unit_class)<rules.class_slots &&
            rules.class_hashes[static_cast<std::size_t>(unit_class)]!=0,
            "BNW target unit class is a native NULL Info slot");
    std::set<std::uint32_t> seen;std::uint64_t null_rows=0;
    using Value=std::conditional_t<std::is_const_v<Team>,
        const std::int64_t,std::int64_t>;
    Value *target=nullptr;
    for(auto &row:found->rows) {
        if(row.type_hash==0) {
            require(row.values.empty(),"BNW NULL UnitClass hash has an illegal payload");
            ++null_rows;continue;
        }
        require(row.type_hash>0 && row.type_hash<=UINT32_MAX &&
                row.values.size()==1 &&
                seen.insert(static_cast<std::uint32_t>(row.type_hash)).second &&
                std::find(rules.class_hashes.begin(),rules.class_hashes.end(),
                          static_cast<std::uint32_t>(row.type_hash))!=
                    rules.class_hashes.end(),
                "BNW team unit-class hash is unknown or duplicate");
        if(row.type_hash==rules.class_hashes[unit_class]) target=&row.values[0];
    }
    const auto expected_nulls=static_cast<std::uint64_t>(std::count(
        rules.class_hashes.begin(),rules.class_hashes.end(),std::uint32_t{0}));
    require(null_rows==expected_nulls && seen.size()==rules.class_slots-expected_nulls,
            "BNW team UnitClass NULL/live slot universe differs from native Info collection");
    require(target,"BNW unit class is absent from team hashed counts");
    return *target;
}
void same_plan(const BnwUnitClassDeath &a,const BnwUnitClassDeath &b) {
    require(a.removal.player_index==b.removal.player_index &&
            a.removal.unit_index==b.removal.unit_index &&
            a.removal.owner==b.removal.owner && a.removal.id==b.removal.id &&
            a.removal.plot_index==b.removal.plot_index &&
            a.removal.area_index==b.removal.area_index &&
            a.removal.area_id==b.removal.area_id &&
            a.team==b.team && a.unit_class==b.unit_class &&
            a.team_count_before==b.team_count_before &&
            a.player_count_before==b.player_count_before,
            "BNW unit-class death plan does not match source");
}
} // namespace

BnwUnitClassDeathRules load_bnw_unit_class_death_rules(
    const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types) {
    require(types.report().all_succeeded(),
            "BNW unit-class Info catalog unavailable");
    std::map<std::string,std::int64_t> classes;
    std::map<std::int64_t,std::uint32_t> class_hashes;
    const auto class_report=database.prefetch_info("UnitClasses",
        [&](std::uint64_t slot,std::int64_t id,const auto &row) {
            const auto type=row.text("Type");
            require(id>=0 && static_cast<std::uint64_t>(id)==slot &&
                    type && !type->empty() && classes.emplace(*type,id).second,
                    "BNW UnitClasses ID/Type roster is not dense and unique");
            const auto *info=types.find(*type);
            require(info && info->table=="UnitClasses" && info->id==id &&
                    info->hash==content::saved_info_type_hash(*type) &&
                    types.find_hash(info->hash) &&
                    types.find_hash(info->hash)->type==*type,
                    "BNW UnitClasses catalog differs from active SQL");
            require(class_hashes.emplace(id,info->hash).second,
                    "BNW UnitClasses ID duplicated");
            return true;
        });
    require(class_report.all_succeeded() &&
            class_report.overlapping_ids==0 && !classes.empty(),
            "BNW UnitClasses collection incomplete");
    BnwUnitClassDeathRules result;
    // Native PrefetchCollection retains NULL entries for SQL ID holes.
    // Cardinality is not the Info span: report.slots is MAX(ID)+1.
    require(class_report.slots>0 && class_report.slots<=INT32_MAX &&
            class_report.rows==classes.size() && class_hashes.size()==classes.size() &&
            class_report.holes==class_report.slots-class_report.rows,
            "BNW UnitClasses native slot/live-row universe incomplete");
    result.class_slots=class_report.slots;
    result.class_hashes.resize(static_cast<std::size_t>(result.class_slots),0);
    // Hash0 here denotes a proven NULL Info, NOT a fabricated class counter.
    // WriteHashedDataArray writes no payload for precisely these entries.
    std::set<std::uint32_t> unique_hashes;
    for(const auto &[id,hash]:class_hashes) {
        require(id>=0 && static_cast<std::uint64_t>(id)<result.class_slots && hash!=0 &&
                unique_hashes.insert(hash).second,"BNW UnitClasses native ID/hash slot invalid");
        result.class_hashes[static_cast<std::size_t>(id)]=hash;
    }
    std::set<std::string> unit_names;
    const auto unit_report=database.prefetch_info("Units",
        [&](std::uint64_t slot,std::int64_t id,const auto &row) {
            require(row.has_column("Type") && row.has_column("Class") &&
                    id>=0 && static_cast<std::uint64_t>(id)==slot,
                    "BNW Units class schema or ID changed");
            const auto type=row.text("Type"),unit_class=row.text("Class");
            require(type && !type->empty() && unit_class &&
                    classes.contains(*unit_class) && unit_names.insert(*type).second,
                    "BNW Unit Class is unresolved or duplicate");
            const auto *info=types.find(*type);
            require(info && info->table=="Units" && info->id==id &&
                    info->hash!=0 &&
                    info->hash==content::saved_info_type_hash(*type) &&
                    types.find_hash(info->hash) &&
                    types.find_hash(info->hash)->type==*type,
                    "BNW unit type ID/hash differs from active SQL");
            require(result.units.emplace(id,BnwUnitClassBinding{
                        id,classes.at(*unit_class),info->hash,
                        class_hashes.at(classes.at(*unit_class))}).second,
                    "BNW unit type ID duplicated");
            return true;
        });
    require(unit_report.all_succeeded() && unit_report.overlapping_ids==0 &&
            !result.units.empty(),"BNW Units class collection incomplete");
    std::size_t known_classes=0,known_units=0;
    for(const auto &[type,entry]:types.entries()) {
        if(entry.table=="UnitClasses") {
            ++known_classes;require(classes.contains(type),
                "BNW UnitClasses catalog contains stale Type");
        }
        if(entry.table=="Units") {
            ++known_units;require(unit_names.contains(type),
                "BNW Units catalog contains stale Type");
        }
    }
    require(known_classes==classes.size() && known_units==unit_names.size(),
            "BNW unit-class catalog roster changed");
    return result;
}

BnwUnitClassDeath plan_bnw_unit_class_death(
    const saves::SavedSessionSnapshot &source,
    const BnwUnitRemoval &removal,
    const BnwUnitClassDeathRules &rules) {
    require(source.game_state.layout==bnw && source.pregame &&
            source.pregame->layout==bnw && source.all_players &&
            source.all_players->layout==bnw && source.teams &&
            source.teams->layout==bnw && rules.class_slots>0 &&
            rules.class_slots<=INT32_MAX &&
            source.teams->dimensions.profile_extensions.values.empty() &&
            source.all_players->context.profile_extensions.values.empty(),
            "BNW class-death source/rules/team profile mismatch");
    const auto checked=plan_bnw_unit_removal(
        source,removal.player_index,removal.unit_index);
    BnwUnitClassDeath result;result.removal=checked;
    same_plan(BnwUnitClassDeath{removal},BnwUnitClassDeath{checked});
    const auto &player=source.all_players->players[removal.player_index];
    const auto &unit=player.units.units[removal.unit_index];
    const auto unit_type=core(unit,"m_eUnitType",saves::UnitCoreEncoding::i32);
    const auto hash=core(unit,"idUnitType",saves::UnitCoreEncoding::hash32);
    const auto rule=rules.units.find(unit_type);
    require(rule!=rules.units.end() && hash>=0 && hash<=UINT32_MAX &&
            rule->second.unit_type==unit_type &&
            rule->second.unit_hash==static_cast<std::uint32_t>(hash) &&
            rule->second.unit_class>=0 &&
            static_cast<std::uint64_t>(rule->second.unit_class)<rules.class_slots &&
            rules.class_hashes.size()==rules.class_slots &&
            rule->second.class_hash==rules.class_hashes[rule->second.unit_class],
            "BNW unit type ID/hash/class does not match active rules");
    result.unit_class=rule->second.unit_class;
    const auto mapping=source.pregame->archive.integer_arrays.find("team_types");
    require(mapping!=source.pregame->archive.integer_arrays.end() &&
            mapping->second.size()>=source.all_players->players.size(),
            "BNW source player-team mapping unavailable");
    result.team=mapping->second[removal.player_index];
    require(result.team>=0 &&
            static_cast<std::uint64_t>(result.team)<source.teams->teams.size() &&
            source.teams->teams[result.team].index==
                static_cast<std::uint64_t>(result.team),
            "BNW unit owner team unavailable");
    const auto &team_count=team_class_count(
        source.teams->teams[result.team],rules,result.unit_class);
    const auto &player_counts=player_class_counts(player,rules.class_slots);
    result.team_count_before=team_count;
    result.player_count_before=player_counts[result.unit_class];
    require(result.team_count_before>0 && result.team_count_before<=INT32_MAX &&
            result.player_count_before>0 && result.player_count_before<=INT32_MAX,
            "BNW unit-class death counter cannot decrement");
    return result;
}

saves::SavedSessionSnapshot project_bnw_unit_class_death(
    const saves::SavedSessionSnapshot &source,
    const BnwUnitClassDeath &plan,
    const BnwUnitClassDeathRules &rules) {
    same_plan(plan,plan_bnw_unit_class_death(source,plan.removal,rules));
    auto result=source;
    auto &team=result.teams->teams[plan.team];
    auto &player=result.all_players->players[plan.removal.player_index];
    --team_class_count(team,rules,plan.unit_class);
    --player_class_counts(player,rules.class_slots)[plan.unit_class];
    return result;
}
} // namespace civ5::runtime
