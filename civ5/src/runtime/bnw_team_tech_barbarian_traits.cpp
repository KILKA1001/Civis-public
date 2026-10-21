#include "civ5/runtime/bnw_team_tech_barbarian_traits.hpp"
#include "civ5/runtime/pregame_roster.hpp"
#include <climits>
#include <map>
#include <set>
#include <stdexcept>
#include <string_view>
#include <variant>

namespace civ5::runtime {
namespace {
void require(bool value,const char *reason) {
    if(!value) throw std::runtime_error(reason);
}
std::uint64_t info_slots(const content::RulesDatabase &database,const char *table) {
    // CvPlayerTraits::Reset sizes indexed arrays by GC.getNum*Infos(), which
    // is the logical indexed span. Sparse IDs still occupy array slots.
    const auto report=database.prefetch_info(table);
    require(report.all_succeeded() && !report.overlapping_ids &&
            report.rows && report.slots<=INT32_MAX,
            "BNW trait-dependent indexed rules unavailable");
    return report.slots;
}
saves::PlayerCoreField &core(saves::SavedPlayerObservation &player,
                             std::string_view name) {
    saves::PlayerCoreField *found=nullptr;
    for(auto &field:player.core.fields)
        if(field.name && name==field.name) {
            require(!found,"Duplicate BNW player core policy-cost field");
            found=&field;
        }
    require(found && found->values.size()==1,
            "Missing BNW player core policy-cost field");
    return *found;
}
bool preserved(std::string_view name) {
    return name=="m_iBaktunPreviousTurn" ||
           name=="m_aMayaBonusChoices" ||
           name=="m_iUniqueLuxuryCitiesPlaced" ||
           name=="m_aUniqueLuxuryAreas";
}
std::uint64_t expected_rows(std::string_view name,
    std::uint64_t improvements,std::uint64_t specialists,
    std::uint64_t features) {
    if(name=="m_ppaaiImprovementYieldChange") return improvements;
    if(name=="m_ppaaiSpecialistYieldChange") return specialists;
    if(name=="m_ppaaiUnimprovedFeatureYieldChange") return features;
    throw std::runtime_error("Unknown BNW trait yield-row field");
}
void verify_zero(const std::vector<std::int64_t> &values) {
    for(const auto value:values)
        require(value==0,"Barbarian trait field is not at its no-trait reset value");
}
void verify_no_trait_reset(const saves::PlayerTraitsObservation &traits,
    const content::InfoTypeCatalog &types,std::uint64_t terrains,
    std::uint64_t resources,std::uint64_t unit_classes,
    std::uint64_t combats,std::uint64_t improvements,
    std::uint64_t specialists,std::uint64_t features,
    std::uint64_t &checked) {
    require(traits.layout==saves::GameStateLayout::expansion2_version1 &&
            traits.version && *traits.version>=1 && *traits.version<=19 &&
            traits.dimensions.yields==6,
            "Unsupported BNW barbarian trait snapshot");
    std::set<std::string_view> names;
    for(const auto &field:traits.fields) {
        require(field.name && names.insert(field.name).second,
                "Duplicate or unnamed BNW barbarian trait field");
        const std::string_view name=field.name;
        ++checked;
        if(preserved(name)) continue;
        using E=saves::PlayerTraitsEncoding;
        if(field.encoding==E::i32 || field.encoding==E::boolean) {
            require(field.values.size()==1,
                    "BNW barbarian trait scalar width invalid");
            const auto expected=name=="m_eCampGuardType" ||
                                name=="m_eCombatBonusImprovement" ? -1 : 0;
            require(field.values[0]==expected,
                    "BNW barbarian trait scalar differs from Reset/Init");
        } else if(field.encoding==E::yield_array) {
            require(field.values.size()==6,
                    "BNW barbarian trait yield array width invalid");
            verify_zero(field.values);
        } else if(field.encoding==E::hashed_i32) {
            const bool terrain=name=="m_iStrategicResourceQuantityModifier";
            require(terrain || name=="m_aiResourceQuantityModifier",
                    "Unknown BNW barbarian hashed trait field");
            const auto expected=terrain ? terrains : resources;
            require(field.hashes.size()==expected &&
                    field.values.size()==expected,
                    "BNW barbarian hashed trait dimensions differ from Reset");
            std::set<std::uint64_t> hashes;
            for(const auto hash:field.hashes) {
                const auto *mapped=types.find_hash(hash);
                require(hash && hashes.insert(hash).second && mapped &&
                        mapped->table==(terrain ? "Terrains" : "Resources"),
                        "BNW barbarian hashed trait has an unresolved row");
            }
            verify_zero(field.values);
        } else if(field.encoding==E::counted_bool && name=="m_abNoTrain") {
            require(field.wire_count && *field.wire_count>=0 &&
                    static_cast<std::uint64_t>(*field.wire_count)==unit_classes &&
                    field.values.size()==unit_classes,
                    "BNW barbarian no-train dimensions differ from Reset");
            verify_zero(field.values);
        } else if(field.encoding==E::counted_pairs && name=="m_aFreeTraitUnits") {
            require(field.wire_count && *field.wire_count==0 &&
                    field.values.empty(),
                    "BNW barbarian trait free-unit list differs from Reset");
        } else if(field.encoding==E::combat_i32 &&
                  name=="m_paiMovesChangeUnitCombat") {
            require(field.wire_count && *field.wire_count>=0 &&
                    static_cast<std::uint64_t>(*field.wire_count)==combats &&
                    field.values.size()==combats,
                    "BNW barbarian combat-move dimensions differ from Reset");
            verify_zero(field.values);
        } else if(field.encoding==E::shared_combat_i32 &&
                  name=="m_paiMaintenanceModifierUnitCombat") {
            require(field.values.size()==combats,
                    "BNW barbarian combat-maintenance dimensions differ from Reset");
            verify_zero(field.values);
        } else if(field.encoding==E::yield_rows) {
            const auto count=expected_rows(name,improvements,specialists,features);
            require(field.wire_count && *field.wire_count>=0 &&
                    static_cast<std::uint64_t>(*field.wire_count)==count &&
                    field.values.size()==count*6,
                    "BNW barbarian trait yield-row dimensions differ from Reset");
            verify_zero(field.values);
        } else {
            throw std::runtime_error("Unverified BNW barbarian trait reset field");
        }
    }
    require(checked>=90,"BNW barbarian trait snapshot omits reset fields");
}
std::int64_t floor_define(const content::GlobalDefineCache &defines) {
    require(defines.profile()==content::GlobalDefineProfile::bnw,
            "BNW policy cost requires BNW defines");
    const auto *entry=defines.find("POLICY_COST_DISCOUNT_MAX");
    require(entry && entry->lookup.found && entry->lookup.error.empty(),
            "BNW policy cost discount floor unavailable");
    const auto *value=std::get_if<std::int64_t>(&entry->lookup.value);
    require(value && *value>=INT32_MIN && *value<=INT32_MAX,
            "BNW policy cost discount floor outside source range");
    return *value;
}
void verify_no_owned_policy(const saves::SavedPlayerObservation &player,
    const content::RulesDatabase &database,const content::InfoTypeCatalog &types) {
    require(player.strategy.layout==saves::GameStateLayout::expansion2_version1,
            "BNW barbarian policy snapshot unavailable");
    const saves::PlayerStrategyBlock *block=nullptr;
    for(const auto &candidate:player.strategy.blocks)
        if(candidate.name && std::string_view(candidate.name)=="policies") {
            require(!block,"Duplicate BNW barbarian policy block");
            block=&candidate;
        }
    require(block,"Missing BNW barbarian policy block");
    const saves::TeamHashedArray *has_policy=nullptr;
    for(const auto &candidate:block->hashes)
        if(candidate.name && std::string_view(candidate.name)=="has_policy") {
            require(!has_policy,"Duplicate BNW barbarian has_policy array");
            has_policy=&candidate;
        }
    require(has_policy && has_policy->wire_count>=0 &&
            static_cast<std::uint64_t>(has_policy->wire_count)==has_policy->rows.size(),
            "Invalid BNW barbarian has_policy wire count");
    const auto slots=info_slots(database,"Policies");
    std::map<std::int64_t,bool> owned;
    for(const auto &row:has_policy->rows) {
        require(row.type_hash ? row.values.size()==1 : row.values.empty(),
                "Invalid BNW barbarian policy hash row");
        if(!row.type_hash) continue;
        const auto *type=types.find_hash(row.type_hash);
        require(type && type->table=="Policies" && type->id>=0 &&
                static_cast<std::uint64_t>(type->id)<slots,
                "Unresolved BNW barbarian policy hash");
        require(row.values[0]==0 || row.values[0]==1,
                "Invalid BNW barbarian policy ownership flag");
        owned[type->id]=row.values[0]!=0; // ReadHashedDataArray: last row wins.
    }
    for(const auto &[id,yes]:owned)
        require(!yes,"Barbarian owns a policy requiring numeric modifier");
}
}

BnwBarbarianTraitResetResult run_bnw_barbarian_trait_reset(
    LiveSessionState &state,const content::TechnologyRules &technologies,
    const content::RulesDatabase &database,const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,std::int64_t technology) {
    BnwBarbarianTraitResetResult result;
    auto draft=state;
    const auto prior=preflight_bnw_barbarian_player_map_tech(
        draft,technologies,database,types,technology);
    if(prior.first_pending!=BnwTechPlayerMapGate::reset_and_init_traits ||
       prior.ever_alive_members!=std::vector<std::int64_t>{63} ||
       prior.nonzero_player_modifiers || prior.free_promotions) {
        result.blocker=prior.blocker;
        return result;
    }
    try {
        require(draft.world.pregame && draft.world.all_players &&
                draft.world.all_players->players.size()==64 &&
                types.report().all_succeeded() &&
                !types.report().hash_collisions && !types.report().name_overwrites,
                "BNW barbarian trait identity or player snapshot unavailable");
        const auto roster=restore_pregame_roster(*draft.world.pregame,database);
        const auto slot=roster.slots.find(63);
        require(slot!=roster.slots.end() && slot->second.leader>=0 &&
                !slot->second.leader_key.empty(),
                "BNW barbarian leader identity unavailable");
        const auto leader_traits=database.query_rows(
            {"SELECT TraitType FROM Leader_Traits WHERE LeaderType=?",
             {slot->second.leader_key}},
            [&](const content::RulesRowView &) {
                throw std::runtime_error("Barbarian leader has an active trait");
            });
        require(leader_traits.all_succeeded(),
                "BNW leader-trait relation unavailable");

        auto &player=draft.world.all_players->players[63];
        const auto terrains=info_slots(database,"Terrains");
        const auto resources=info_slots(database,"Resources");
        const auto unit_classes=info_slots(database,"UnitClasses");
        const auto combats=info_slots(database,"UnitCombatInfos");
        const auto improvements=info_slots(database,"Improvements");
        const auto specialists=info_slots(database,"Specialists");
        const auto features=info_slots(database,"Features");
        verify_no_trait_reset(player.traits,types,terrains,resources,
            unit_classes,combats,improvements,specialists,features,
            result.checked_trait_fields);

        const auto member=draft.setup.players.find(63);
        require(member!=draft.setup.players.end() &&
                (!member->second.policies_resolved || member->second.policies.empty()),
                "Barbarian owned-policy projection conflicts with raw save");
        verify_no_owned_policy(player,database,types);
        auto &policy_cost=core(player,"m_iPolicyCostModifier");
        const auto building=core(player,"m_iPolicyCostBuildingModifier").values[0];
        const auto minor=core(player,"m_iPolicyCostMinorCivModifier").values[0];
        const auto computed=std::max(building+minor,floor_define(defines));
        require(computed>=INT32_MIN && computed<=INT32_MAX,
                "BNW recomputed policy cost modifier exceeds source int");
        result.policy_cost_before=policy_cost.values[0];
        result.policy_cost_after=computed;

        // Saved trait fields already equal Reset + no-trait Init. The source
        // still recreates this unsaved vector and writes policy cost, so do
        // both on the owned draft before exposing it to later steps.
        draft.setup.extensions.emplace<BnwBarbarianTraitResetTransient>(
            BnwBarbarianTraitResetTransient{63,resources,true});
        policy_cost.values[0]=computed;
        result.blocker="Trait free-unit query, map effects, Lua TeamTechResearched and later acquisition effects remain";
        state=std::move(draft);
        result.applied=true;
    } catch(const std::exception &error) {
        result.blocker=error.what();
    }
    return result;
}

} // namespace civ5::runtime
