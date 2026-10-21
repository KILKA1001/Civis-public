#include "civ5/saves/all_players.hpp"
#include "civ5/saves/vp_player_strategy_profile.hpp"
#include "civ5/saves/vp_all_players_profile.hpp"
#include "civ5/saves/vp_player_knowledge_profile.hpp"
#include "civ5/saves/vp_cities_profile.hpp"
#include <algorithm>
#include <stdexcept>
#include <string>

namespace civ5::saves {
namespace {
std::int64_t player_id(const PlayerCoreObservation &core) {
    const auto it = std::find_if(core.fields.begin(), core.fields.end(),
                                 [](const auto &f) { return std::string_view(f.name) == "m_eID"; });
    if (it == core.fields.end() || it->values.size() != 1)
        throw std::runtime_error("Missing saved player ID");
    return it->values.front();
}
std::uint64_t promotion_slots(const PlayerCoreObservation &core) {
    const auto it = std::find_if(core.fields.begin(), core.fields.end(), [](const auto &f) {
        return std::string_view(f.name) == "m_paiFreePromotionCount";
    });
    if (it == core.fields.end() || it->values.empty())
        throw std::runtime_error("Missing saved promotion slots");
    return it->values.size();
}
} // namespace

AllPlayersContext resolve_saved_player_context(const GameProgressionObservation &progression,
                                               AllPlayersContext context) {
    if (progression.layout == GameStateLayout::vp542_corpus)
        return context;
    if (progression.layout != GameStateLayout::expansion2_version1)
        throw std::runtime_error("Unknown saved player context layout");
    std::optional<std::uint64_t> slots;
    for (const auto &array : progression.arrays) {
        if (std::string_view(array.name) != "project_created_count")
            continue;
        if (slots)
            throw std::runtime_error("Ambiguous BNW saved project collection");
        slots = array.rows.size();
    }
    if (!slots)
        throw std::runtime_error("Missing BNW saved project collection");
    if (context.bnw_projects && *context.bnw_projects != *slots)
        throw std::runtime_error("BNW project context disagrees with the saved collection");
    context.bnw_projects = slots;
    return context;
}

AllPlayersObservation observe_all_players(runtime::BinaryReader &input, GameStateLayout layout,
                                          AllPlayersContext context, GameStateBudget budget) {
    const bool vp = layout == GameStateLayout::vp542_corpus;
    const auto *vp_dimensions = context.profile_extensions.find<VpAllPlayersWireDimensions>();
    if (!context.player_count || context.player_count > budget.elements)
        throw std::runtime_error("Invalid all-player wire count or element budget");
    if (vp) {
        if (!context.profile_extensions.find<VpPlayerKnowledgeWireDimensions>() ||
            !context.traits || !context.trade_culture || !vp_dimensions ||
            !vp_dimensions->city_strategies || !vp_dimensions->city_emphases || !vp_dimensions->buildings)
            throw std::runtime_error("VP all-player context is incomplete");
        if (!vp_player_strategy_dimensions(context.strategy) || context.strategy.yields == 0 || context.diplomacy.players == 0)
            throw std::runtime_error("VP all-player source dimensions are incomplete");
    } else if (!context.bnw_projects)
        throw std::runtime_error("BNW all-player project dimensions are incomplete");

    AllPlayersObservation result{};
    result.layout = layout;
    result.context = context;
    result.start_offset = input.position();
    for (std::uint64_t index = 0; index < context.player_count; ++index) {
        SavedPlayerObservation p{};
        p.index = index;
        p.start_offset = input.position();
        try {
            p.core = observe_player_core(
                input, layout, vp ? std::optional<std::uint64_t>(context.strategy.yields) : std::nullopt,
                budget);
            p.saved_player_id = player_id(p.core);
            p.strategy = observe_player_strategy(input, layout, context.strategy, budget);
            p.diplomacy = observe_player_diplomacy(input, layout, context.diplomacy, budget);
            PlayerKnowledgeDecodeContext knowledge_context;
            if (vp)
                knowledge_context.profile_extensions.emplace<VpPlayerKnowledgeWireDimensions>(
                    *context.profile_extensions.find<VpPlayerKnowledgeWireDimensions>());
            p.knowledge = observe_player_knowledge(input, layout, std::move(knowledge_context), budget);
            p.planning = observe_player_planning(input, layout, context.planning, budget);
            p.traits = observe_player_traits(input, layout, context.traits, budget);
            p.espionage = observe_player_espionage(input, layout, budget);
            p.trade_culture = observe_player_trade_culture(input, layout, context.trade_culture, budget);
            p.administration = observe_player_administration(input, layout, budget);

            CitiesContext cities_context;
            if (vp)
                cities_context.profile_extensions.emplace<VpCitiesWireDimensions>(
                    VpCitiesWireDimensions{vp_player_strategy_dimensions(context.strategy)->flavors, vp_dimensions->city_strategies,
                                           promotion_slots(p.core), vp_dimensions->buildings,
                                           context.strategy.yields, vp_dimensions->city_emphases});
            else
                cities_context.bnw_projects = *context.bnw_projects;
            p.cities = observe_cities(input, layout, cities_context, CitiesBudget{budget});
            p.units = observe_units(input, layout, UnitsBudget{budget});
            p.army_operations =
                observe_army_operations(input, layout, ArmyOperationsContext{p.core.version}, {});

            std::optional<PlayerTailVpContext> tail_context;
            if (vp)
                tail_context = PlayerTailVpContext{p.core.yield_slots, context.diplomacy.players,
                                                   context.diplomacy.major, p.saved_player_id};
            p.tail = observe_player_tail(input, layout, tail_context, budget);
        } catch (const std::exception &error) {
            throw std::runtime_error("Player " + std::to_string(index) + " at offset " +
                                     std::to_string(p.start_offset) + ": " + error.what());
        }
        p.end_offset = input.position();
        result.players.push_back(std::move(p));
    }
    result.end_offset = input.position();
    return result;
}
} // namespace civ5::saves
