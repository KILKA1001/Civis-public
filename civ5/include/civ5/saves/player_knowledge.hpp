#pragma once
#include "civ5/saves/team_state.hpp"

namespace civ5::saves {
struct PlayerKnowledgeDecodeContext {
    runtime::ProfileExtensions profile_extensions;
};
struct PlayerKnowledgeBlock {
    const char *name;
    std::uint64_t start_offset = 0, end_offset = 0;
    std::vector<NamedSaveInteger> fields;
    // VP TechAI weights: [saved technology ID, weight]. Other arrays are flat.
    std::vector<PlotArrayObservation> arrays;
    // BNW preserves hash holes, unknown hashes, duplicates and saved weights.
    std::vector<TeamHashedArray> hashes;
};
struct PlayerKnowledgeObservation {
    GameStateLayout layout;
    runtime::ProfileExtensions profile_extensions;
    std::uint64_t start_offset = 0, end_offset = 0;
    std::vector<PlayerKnowledgeBlock> blocks;
};
// Full PlayerReligions, ReligionAI, VP PlayerCorporations, PlayerTechs (with
// TechAI), FlavorManager. Next is TacticalAI of the same player. No faith
// spending, technology selection, hash remapping or flavor propagation.
PlayerKnowledgeObservation
observe_player_knowledge(runtime::BinaryReader &input, GameStateLayout layout,
                         PlayerKnowledgeDecodeContext context = {},
                         GameStateBudget budget = {});
} // namespace civ5::saves
