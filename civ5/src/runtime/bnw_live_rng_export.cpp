#include "civ5/runtime/bnw_live_rng_export.hpp"
#include "civ5/runtime/bnw_game_supporting_checkpoint.hpp"
#include "civ5/runtime/bnw_game_score_checkpoint.hpp"
#include "civ5/runtime/bnw_game_deals_checkpoint.hpp"
#include "civ5/runtime/bnw_team_turn_pair.hpp"
#include "civ5/runtime/bnw_treasury_balance_checkpoint.hpp"
#include "civ5/runtime/bnw_maintenance_expense_checkpoint.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <array>
#include <stdexcept>
#include <optional>
#include <string_view>

namespace civ5::runtime {
namespace {
void require(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
constexpr std::uint64_t mask=0xffffffffULL;
constexpr std::uint64_t multiplier=1103515245ULL,increment=12345ULL;
std::uint64_t seed_after_draws(std::uint64_t seed,std::uint64_t draws) {
    std::uint64_t factor=1,offset=0;
    std::uint64_t step_factor=multiplier,step_offset=increment;
    while(draws) {
        if(draws&1) {
            factor=(step_factor*factor)&mask;
            offset=(step_factor*offset+step_offset)&mask;
        }
        step_offset=(step_factor*step_offset+step_offset)&mask;
        step_factor=(step_factor*step_factor)&mask;
        draws>>=1;
    }
    return (factor*seed+offset)&mask;
}
bool same(const saves::RandomStateObservation &a,
          const saves::RandomStateObservation &b) {
    return a.seed==b.seed && a.calls==b.calls && a.resets==b.resets;
}
void validate_stream(const SimulationRandom &live,
                     const saves::RandomStateObservation &raw,
                     const char *name) {
    require(live.algorithm()==RandomAlgorithm::bnw && raw.seed<=mask,
            "BNW RNG export has mixed or invalid algorithm");
    const auto &now=live.state();
    require(now.resets==raw.resets && now.calls>=raw.calls &&
            now.seed==seed_after_draws(raw.seed,now.calls-raw.calls),name);
}
void validate_successor(const SimulationRandom &live,
                        const SimulationRandom &previous,
                        const char *name) {
    const auto &now=live.state(),&old=previous.state();
    require(live.algorithm()==previous.algorithm() &&
            now.resets==old.resets && now.calls>=old.calls &&
            now.seed==seed_after_draws(old.seed,now.calls-old.calls),name);
}
bool validate_rng(const LiveSessionState &state,
                  const saves::SavedSession &source,
                  const SessionCheckpointIdentity &identity,
                  bool allow_team_turn_pair=false,bool allow_treasury_pair=false) {
    require((!source.source.live_team_research ||
             (allow_team_turn_pair && source.source.live_bnw_team_turn)) &&
            (!source.source.live_treasury ||
             (allow_treasury_pair && source.source.live_bnw_treasury_balance)),
            "BNW research/treasury overlays need raw-world reconciliation");
    require(source.snapshot.progression.layout==
                saves::GameStateLayout::expansion2_version1 &&
            state.world.progression.layout==
                saves::GameStateLayout::expansion2_version1,
            "BNW RNG export has mixed progression profile");
    for(std::size_t i=0;i<2;++i)
        require(same(source.snapshot.progression.random[i],
                     state.world.progression.random[i]),
                "BNW raw progression RNG changed without a raw checkpoint");
    const auto &raw=source.snapshot.progression.random;
    validate_stream(state.random.map,raw[0],
                    "BNW map RNG is not a draw-only source successor");
    validate_stream(state.random.gameplay,raw[1],
                    "BNW gameplay RNG is not a draw-only source successor");
    if(source.source.live_simulation_random) {
        auto previous=restore_session_random(source.snapshot.progression);
        restore_simulation_random_checkpoint(
            {static_cast<std::uint32_t>(
                 saves::CivisSaveSectionId::live_simulation_random),0,
             *source.source.live_simulation_random},previous,identity);
        validate_stream(previous.map,raw[0],
                        "Previous BNW map RNG is not a draw-only source successor");
        validate_stream(previous.gameplay,raw[1],
                        "Previous BNW gameplay RNG is not a draw-only source successor");
        validate_successor(state.random.map,previous.map,
                           "BNW map RNG regresses the previous save");
        validate_successor(state.random.gameplay,previous.gameplay,
                           "BNW gameplay RNG regresses the previous save");
    }
    return state.random.map.state().calls!=raw[0].calls ||
           state.random.gameplay.state().calls!=raw[1].calls;
}
void replace_rng(saves::CivisSaveDocument &document,
                 const SessionRandomStreams &streams,
                 const SessionCheckpointIdentity &identity) {
    auto rng=make_simulation_random_checkpoint(streams,identity);
    auto &parts=document.sections;
    const auto id=static_cast<std::uint32_t>(
        saves::CivisSaveSectionId::live_simulation_random);
    parts.erase(std::remove_if(parts.begin(),parts.end(),[&](const auto &part) {
        return part.id==id;
    }),parts.end());
    parts.push_back(std::move(rng));
}
void replace_game_supporting(saves::CivisSaveDocument &document,
    const LiveSessionState &state,const saves::SavedSession &source,
    const SessionCheckpointIdentity &identity) {
    if(source.source.live_bnw_game_supporting) {
        auto prior=source.snapshot;
        restore_bnw_game_supporting_checkpoint(
            {static_cast<std::uint32_t>(
                 saves::CivisSaveSectionId::live_bnw_game_supporting),0,
             *source.source.live_bnw_game_supporting},prior,identity);
    }
    auto checkpoint=make_bnw_game_supporting_checkpoint(
        state.world,source.snapshot,identity);
    auto &parts=document.sections;
    const auto id=checkpoint.id;
    parts.erase(std::remove_if(parts.begin(),parts.end(),[&](const auto &part) {
        return part.id==id;
    }),parts.end());
    parts.push_back(std::move(checkpoint));
}
void replace_game_score(saves::CivisSaveDocument &document,
    const LiveSessionState &state,const saves::SavedSession &source,
    const SessionCheckpointIdentity &identity) {
    if(source.source.live_bnw_game_score_replay) {
        auto prior=source.snapshot;
        restore_bnw_game_score_checkpoint(
            {static_cast<std::uint32_t>(
                 saves::CivisSaveSectionId::live_bnw_game_score_replay),0,
             *source.source.live_bnw_game_score_replay},prior,
            source.snapshot,identity);
    }
    auto checkpoint=make_bnw_game_score_checkpoint(
        state.world,source.snapshot,identity);
    auto &parts=document.sections;
    const auto id=checkpoint.id;
    parts.erase(std::remove_if(parts.begin(),parts.end(),[&](const auto &part) {
        return part.id==id;
    }),parts.end());
    parts.push_back(std::move(checkpoint));
}
void replace_game_deals(saves::CivisSaveDocument &document,
    const LiveSessionState &state,const saves::SavedSession &source,
    const SessionCheckpointIdentity &identity) {
    if(source.source.live_bnw_game_deals) {
        auto prior=source.snapshot;
        restore_bnw_game_deals_checkpoint(
            {static_cast<std::uint32_t>(
                 saves::CivisSaveSectionId::live_bnw_game_deals),0,
             *source.source.live_bnw_game_deals},prior,
            source.snapshot,identity);
    }
    auto checkpoint=make_bnw_game_deals_checkpoint(
        state.world,source.snapshot,identity);
    auto &parts=document.sections;
    const auto id=checkpoint.id;
    parts.erase(std::remove_if(parts.begin(),parts.end(),[&](const auto &part) {
        return part.id==id;
    }),parts.end());
    parts.push_back(std::move(checkpoint));
}
void replace_team_turn(saves::CivisSaveDocument &document,
    const LiveSessionState &state,const saves::SavedSession &source,
    const SessionCheckpointIdentity &identity,
    const content::RulesDatabase &rules,
    const content::TechnologyRules &technologies,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines) {
    require(!source.source.live_bnw_team_progress &&
            !source.source.live_bnw_research_queue &&
            (!source.source.live_team_research ||
             source.source.live_bnw_team_turn),
            "BNW team turn export requires exclusive ID40 and ID8 ownership");
    const auto baseline_setup=restore_research_context(source.snapshot,rules);
    require(source.snapshot.teams.has_value(),
            "BNW team turn source teams unavailable");
    const auto baseline_technologies=restore_team_technologies(
        *source.snapshot.teams,technologies,types);
    const auto baseline_players=restore_player_research_context(
        source.snapshot,baseline_setup,defines);
    if(source.source.live_bnw_team_turn) {
        require(source.source.live_team_research.has_value(),
                "BNW previous team turn lacks owned research companion");
        (void)restore_bnw_team_turn_checkpoint_pair(
            {{static_cast<std::uint32_t>(
                  saves::CivisSaveSectionId::live_bnw_team_turn),0,
              *source.source.live_bnw_team_turn},
             {static_cast<std::uint32_t>(
                  saves::CivisSaveSectionId::live_team_research),0,
              *source.source.live_team_research}},
            source.snapshot,baseline_setup,baseline_technologies,
            baseline_players,technologies,types,identity);
    }
    auto pair=make_bnw_team_turn_checkpoint_pair(
        state,source.snapshot,baseline_technologies,baseline_players,
        technologies,types,identity);
    auto &parts=document.sections;
    parts.erase(std::remove_if(parts.begin(),parts.end(),[&](const auto &part) {
        return part.id==pair.raw.id || part.id==pair.owned.id;
    }),parts.end());
    parts.push_back(std::move(pair.raw));
    parts.push_back(std::move(pair.owned));
}
bool treasury_values_match_raw(const TreasuryContext &owned,
                               const saves::SavedSessionSnapshot &world) {
    require(world.all_players && owned.players.size()==
            world.all_players->players.size(),"BNW treasury export raw roster missing");
    for(const auto &raw:world.all_players->players) {
        const auto it=owned.players.find(raw.saved_player_id);
        require(it!=owned.players.end(),"BNW treasury export player identity changed");
        const auto &player=it->second;
        const saves::PlayerAdministrationRecord *record=nullptr;
        for(const auto &block:raw.administration.blocks)
            if(block.name && std::string_view(block.name)=="treasury") {
                require(!record,"BNW treasury export duplicate administration");record=&block;
            }
        require(record,"BNW treasury export administration missing");
        const auto value=[&](std::string_view key) {
            const saves::NamedSaveInteger *found=nullptr;
            for(const auto &field:record->fields)
                if(field.name && std::string_view(field.name)==key) {
                    require(!found,"BNW treasury export duplicate scalar");found=&field;
                }
            require(found,"BNW treasury export scalar missing");return found->value;
        };
        const auto history=[&](std::string_view key)->const std::vector<std::int64_t>& {
            const saves::PlotArrayObservation *found=nullptr;
            for(const auto &array:record->arrays)
                if(array.name && std::string_view(array.name)==key) {
                    require(!found,"BNW treasury export duplicate history");found=&array;
                }
            require(found && found->columns==1,"BNW treasury export history missing");
            return found->values;
        };
        if(player.balance_times100!=value("m_iGold") ||
           player.lifetime_gross_gold!=value("m_iLifetimeGrossGoldIncome") ||
           player.balance_history_times100!=history("m_GoldBalanceForTurnTimes100") ||
           player.income_history_times100!=history("m_GoldChangeForTurnTimes100")) return false;
    }
    return true;
}
const saves::NamedSaveInteger &treasury_raw_field(const saves::SavedPlayerObservation &player,
                                                 std::string_view key) {
    const saves::NamedSaveInteger *found=nullptr;
    for(const auto &block:player.administration.blocks)
        if(block.name && std::string_view(block.name)=="treasury")
            for(const auto &field:block.fields)
                if(field.name && std::string_view(field.name)==key) {
                    require(!found,"BNW expense export duplicate raw field");found=&field;
                }
    require(found,"BNW expense export raw field missing");return *found;
}
saves::NamedSaveInteger &treasury_raw_field(saves::SavedPlayerObservation &player,
                                           std::string_view key) {
    saves::NamedSaveInteger *found=nullptr;
    for(auto &block:player.administration.blocks)
        if(block.name && std::string_view(block.name)=="treasury")
            for(auto &field:block.fields)
                if(field.name && std::string_view(field.name)==key) {
                    require(!found,"BNW expense export duplicate raw field");found=&field;
                }
    require(found,"BNW expense export raw field missing");return *found;
}
struct TreasuryComposition {bool treasury=false,expenses=false;};
TreasuryComposition replace_treasury(saves::CivisSaveDocument &document,
    const LiveSessionState &state,const saves::SavedSession &source,
    const SessionCheckpointIdentity &identity,
    const content::RulesDatabase &rules,const content::TechnologyRules &technologies,
    const content::InfoTypeCatalog &types,const content::GlobalDefineCache &defines) {
    require(!state.player_gold_publication && !state.game_clock.pending,
            "BNW treasury export has unpublished gold/clock effects");
    require(source.source.live_treasury.has_value()==
            source.source.live_bnw_treasury_balance.has_value(),
            "BNW treasury export requires exclusive raw ID30 and owned ID10 pair");
    require(!source.source.live_bnw_player_administration,
            "BNW treasury export overlaps full player administration");
    // Reconstruct the immutable baseline using the same rules/restorers as
    // LiveSession::restore_bnw. No live mutation or copied readiness grant.
    auto setup=restore_research_context(source.snapshot,rules);
    require(source.snapshot.teams.has_value(),"BNW treasury source teams missing");
    const auto teams=restore_team_technologies(*source.snapshot.teams,technologies,types);
    const auto players=restore_player_research_context(source.snapshot,setup,defines);
    const auto city_rules=load_city_yield_rules(rules,content::TechnologyProfile::bnw);
    const auto cities=restore_city_yields(source.snapshot,setup,rules,city_rules,types);
    const MaintenanceWorld baseline_world{setup,players,cities,teams};
    const auto treasury_rules=load_treasury_rules(rules,content::TechnologyProfile::bnw,types);
    const auto baseline=restore_treasury(source.snapshot,baseline_world,treasury_rules,types);
    const auto maintenance_rules=load_maintenance_rules(rules,content::TechnologyProfile::bnw,types);
    const auto baseline_maintenance=restore_maintenance(source.snapshot,baseline_world,maintenance_rules,types);
    // A gold section owns four values/counts, while ID34 owns the expense
    // scalars. Normalize only those TWO scalars in a LOCAL gold projection.
    // The true current raw/owned expenses are independently proved below.
    auto gold_projection=state;
    bool expenses_changed=false;
    require(state.world.all_players && source.snapshot.all_players &&
            state.world.all_players->players.size()==source.snapshot.all_players->players.size(),
            "BNW expense export source roster changed");
    for(std::size_t i=0;i<state.world.all_players->players.size();++i) {
        const auto id=static_cast<std::int64_t>(i);
        const auto &live=state.maintenance.players.at(id);
        const auto &old=baseline_maintenance.players.at(id);
        const auto &actual=state.world.all_players->players[i];
        const auto &source_player=source.snapshot.all_players->players[i];
        require(treasury_raw_field(actual,"m_iExpensePerTurnUnitMaintenance").value==live.cached_unit_expense &&
                treasury_raw_field(actual,"m_iExpensePerTurnUnitSupply").value==live.cached_supply_expense,
                "BNW expense export requires actual raw/owned cache mirror");
        expenses_changed|=live.cached_unit_expense!=old.cached_unit_expense ||
                          live.cached_supply_expense!=old.cached_supply_expense;
        for(const auto key:{"m_iExpensePerTurnUnitMaintenance","m_iExpensePerTurnUnitSupply"})
            treasury_raw_field(gold_projection.world.all_players->players[i],key).value=
                treasury_raw_field(source_player,key).value;
    }
    require(treasury_values_match_raw(state.treasury,state.world),
            "BNW treasury export owned/raw values have not been reconciled");
    if(source.source.live_treasury || source.source.live_bnw_maintenance_expense) {
        auto prior_world=source.snapshot;
        if(source.source.live_bnw_game_clock) {
            BnwGameClockState clock;
            restore_bnw_game_clock_checkpoint(
                {static_cast<std::uint32_t>(saves::CivisSaveSectionId::live_bnw_game_clock),0,
                 *source.source.live_bnw_game_clock},prior_world,clock,identity);
        }
        auto prior_setup=restore_research_context(prior_world,rules);
        std::optional<BnwTreasuryBalanceRestored> restored_gold;
        if(source.source.live_treasury) restored_gold=restore_bnw_treasury_balance_checkpoint_pair(
            {{bnw_treasury_balance_checkpoint_section_id,0,*source.source.live_bnw_treasury_balance},
             {static_cast<std::uint32_t>(saves::CivisSaveSectionId::live_treasury),0,
              *source.source.live_treasury}},source.snapshot,baseline,prior_setup,identity,&prior_world);
        const auto &previous=restored_gold?restored_gold->treasury:baseline;
        for(const auto &[id,old]:previous.players) {
            const auto found=state.treasury.players.find(id);
            require(found!=state.treasury.players.end(),"BNW treasury saved player retired");
            const auto &now=found->second;
            const auto retains=[](const auto &a,const auto &b) {
                return b.size()>=a.size() && std::equal(a.begin(),a.end(),b.begin());
            };
            require(now.lifetime_gross_gold>=old.lifetime_gross_gold &&
                    retains(old.balance_history_times100,now.balance_history_times100) &&
                    retains(old.income_history_times100,now.income_history_times100),
                    "BNW treasury export regresses previous lifetime/history");
        }
        if(source.source.live_bnw_maintenance_expense) {
            if(restored_gold) prior_world=std::move(restored_gold->world);
            auto prior_maintenance=restore_maintenance(prior_world,baseline_world,maintenance_rules,types);
            std::optional<saves::CivisSaveSection> prior_gold;
            if(source.source.live_bnw_treasury_balance) prior_gold=
                saves::CivisSaveSection{bnw_treasury_balance_checkpoint_section_id,0,*source.source.live_bnw_treasury_balance};
            (void)restore_bnw_maintenance_expense_checkpoint(
                {bnw_maintenance_expense_checkpoint_section_id,0,*source.source.live_bnw_maintenance_expense},
                prior_world,source.snapshot,prior_maintenance,baseline_maintenance,identity,
                prior_gold?&*prior_gold:nullptr);
        }
    }
    const auto current=make_treasury_checkpoint(state.treasury,state.setup,identity);
    const auto original=make_treasury_checkpoint(baseline,setup,identity);
    std::optional<BnwTreasuryBalanceCheckpointPair> pair;
    if(current.bytes==original.bytes)
        require(!source.source.live_treasury,
                "BNW treasury revert requires an explicit baseline checkpoint schema");
    else pair=make_bnw_treasury_balance_checkpoint_pair(gold_projection,source.snapshot,baseline,identity);
    std::optional<saves::CivisSaveSection> expense;
    if(expenses_changed)
        expense=make_bnw_maintenance_expense_checkpoint(state,source.snapshot,
            baseline_maintenance,identity,pair?&pair->raw:nullptr);
    else require(!source.source.live_bnw_maintenance_expense,
                 "BNW expense revert requires an explicit baseline checkpoint schema");
    // Compose using the exact production load order: clock, raw/owned gold,
    // expense. Any mismatch refuses the entire local document before return.
    auto verified_world=source.snapshot;
    const auto clock=std::find_if(document.sections.begin(),document.sections.end(),[](const auto &part) {
        return part.id==static_cast<std::uint32_t>(saves::CivisSaveSectionId::live_bnw_game_clock);
    });
    require(clock!=document.sections.end(),"BNW expense export requires composed clock");
    BnwGameClockState verified_clock;
    restore_bnw_game_clock_checkpoint(*clock,verified_world,verified_clock,identity);
    if(pair) verified_world=restore_bnw_treasury_balance_checkpoint_pair(
        *pair,source.snapshot,baseline,state.setup,identity,&verified_world).world;
    auto verified_maintenance=restore_maintenance(verified_world,baseline_world,maintenance_rules,types);
    if(expense) {
        auto restored=restore_bnw_maintenance_expense_checkpoint(*expense,verified_world,
            source.snapshot,verified_maintenance,baseline_maintenance,identity,pair?&pair->raw:nullptr);
        verified_world=std::move(restored.world);verified_maintenance=std::move(restored.maintenance);
    }
    require(treasury_values_match_raw(state.treasury,verified_world),
            "BNW restored treasury composition differs from actual owner");
    for(std::size_t i=0;i<verified_world.all_players->players.size();++i) {
        const auto id=static_cast<std::int64_t>(i);
        const auto &expected=state.maintenance.players.at(id);
        const auto &owned=verified_maintenance.players.at(id);
        auto &actual=verified_world.all_players->players[i];
        require(treasury_raw_field(actual,"m_iExpensePerTurnUnitMaintenance").value==expected.cached_unit_expense &&
                treasury_raw_field(actual,"m_iExpensePerTurnUnitSupply").value==expected.cached_supply_expense &&
                owned.cached_unit_expense==expected.cached_unit_expense &&
                owned.cached_supply_expense==expected.cached_supply_expense,
                "BNW restored expense composition differs from actual owner");
    }
    std::erase_if(document.sections,[](const auto &part) {
        return part.id==bnw_treasury_balance_checkpoint_section_id ||
               part.id==static_cast<std::uint32_t>(saves::CivisSaveSectionId::live_treasury) ||
               part.id==bnw_maintenance_expense_checkpoint_section_id;
    });
    if(pair) {
        document.sections.push_back(std::move(pair->raw));
        document.sections.push_back(std::move(pair->owned));
    }
    if(expense) document.sections.push_back(std::move(*expense));
    return {pair.has_value(),expense.has_value()};
}

void replace_standalone_services(saves::CivisSaveDocument &document,const LiveSessionState &state) {
    auto section=saves::make_standalone_services_section(state.standalone_services);
    std::erase_if(document.sections,[&](const auto &part){return part.id==section.id;});
    document.sections.push_back(std::move(section));
}
constexpr std::string_view uncovered[]={
    "pregame setup and source prefix mutation",
    "game core, progression history and clock",
    "game religion, culture, leagues, trade and final systems",
    "saved database, Lua VM and mod user data",
    "map regions, live visibility side effects and pathfinding caches",
    "unit relocation/roster, area accounting and full setXY effects",
    "team technologies, diplomacy, first-contact AI and Lua effects",
    "player non-strategy economy, religion, policy effects and AI state",
    "variable strategy collections, live AI caches and pending strategy effects",
    "dynamic city creation, deletion, roster and collection headers",
    "live city yields, population, unhappiness and maintenance contexts",
    "city SDK load effects, AI and derived query caches",
    "unit core, tail, orders and movement",
    "supporting classes and object references",
    "owned research selection, research turn and technology effects",
    "owned treasury, science and other gameplay contexts",
    "RNG reseed/reset paths and their world effects",
    "pending UI, engine and Lua publication intents"
};
} // namespace

BnwRngPartialExport make_bnw_live_rng_partial_export(
    const LiveSessionState &state,
    const saves::SavedSession &source,
    const SessionCheckpointIdentity &identity,
    std::string_view original_source_sha256,
    const saves::SavedTerrainFeatureResolver &features,
    const content::RulesDatabase &rules,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines) {
    require(!state.player_early_publication,
            "BNW RNG export has unpublished early player UI effects");
    require(validate_rng(state,source,identity),
            "BNW RNG export has no draw-only change");
    auto base=make_bnw_live_map_player_city_partial_export(
        state,source,identity,original_source_sha256,
        features,rules,types,defines);
    replace_standalone_services(base.document,state);
    replace_rng(base.document,state.random,identity);
    BnwRngPartialExport result;
    result.document=std::move(base.document);
    result.uncovered_domains.assign(std::begin(uncovered),std::end(uncovered));
    return result;
}

BnwTurnBasePartialExport make_bnw_live_turn_base_partial_export(
    const LiveSessionState &state,
    const saves::SavedSession &source,
    const SessionCheckpointIdentity &identity,
    std::string_view original_source_sha256,
    const saves::SavedTerrainFeatureResolver &features,
    const content::RulesDatabase &rules,
    const content::TechnologyRules &technologies,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines) {
    require(!state.player_early_publication,
            "BNW turn export has unpublished early player UI effects");
    require(!state.player_gold_publication && !state.game_clock.pending,
            "BNW turn export has unpublished gold/clock effects");
    (void)validate_rng(state,source,identity,true,true);
    auto base=make_bnw_live_map_player_city_clock_partial_export(
        state,source,identity,original_source_sha256,
        features,rules,types,defines);
    replace_standalone_services(base.document,state);
    replace_rng(base.document,state.random,identity);
    replace_game_supporting(base.document,state,source,identity);
    replace_game_score(base.document,state,source,identity);
    replace_game_deals(base.document,state,source,identity);
    replace_team_turn(base.document,state,source,identity,
                      rules,technologies,types,defines);
    const auto has_treasury=replace_treasury(base.document,state,source,identity,
                                             rules,technologies,types,defines);
    BnwTurnBasePartialExport result;
    result.document=std::move(base.document);
    result.uncovered_domains.assign(std::begin(uncovered),std::end(uncovered));
    for(auto &domain:result.uncovered_domains)
        if(domain=="game core, progression history and clock")
            domain="game core and progression history";
        else if(has_treasury.treasury && domain=="owned treasury, science and other gameplay contexts")
            domain="treasury non-balance contexts, science and other gameplay contexts";
        else if(has_treasury.expenses && domain=="live city yields, population, unhappiness and maintenance contexts")
            domain="live city yields, population, unhappiness and non-cache maintenance contexts";
        else if(domain=="supporting classes and object references")
            domain="other supporting classes and object references";
    return result;
}

} // namespace civ5::runtime
