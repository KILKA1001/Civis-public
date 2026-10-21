#include "civ5/runtime/player_economy_research_stage.hpp"
#include <stdexcept>

namespace civ5::runtime {
PlayerEconomyResearchStageResult do_player_economy_research_stage(
    TreasuryTurnContext &gold, ResearchTurnContext &research, std::int64_t player,
    const GameCoreAfterGoldStep &after_gold) {
    // Fail before either stage mutates state if the caller assembled contexts
    // from different imports or different rules profiles.
    if (&gold.state != &research.treasury || &gold.world != &research.economy ||
        &gold.world.setup != &research.progress.setup ||
        &gold.world.diplomacy != &research.progress.players ||
        gold.world.technologies.data() != research.progress.teams.data() ||
        gold.world.technologies.size() != research.progress.teams.size() ||
        &gold.maintenance != &research.maintenance ||
        &gold.rules != &research.treasury_rules ||
        &gold.city_rules != &research.city_rules ||
        &gold.maintenance_rules != &research.maintenance_rules ||
        &gold.defines != &research.progress.defines ||
        gold.options != research.progress.options ||
        gold.active_player != research.active_player ||
        gold.state.profile != research.state.profile ||
        gold.state.profile != research.science.profile ||
        after_gold.profile != gold.state.profile || !after_gold.run)
        throw std::invalid_argument("Economy/research stage requires one live world and rules profile");
    PlayerEconomyResearchStageResult result;
    result.gold = do_gold(gold, player);
    after_gold.run(gold,player);
    result.research = do_research(research, player);
    return result;
}
} // namespace civ5::runtime
