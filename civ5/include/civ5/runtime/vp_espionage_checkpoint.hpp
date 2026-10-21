#pragma once
#include "civ5/runtime/session_checkpoint_identity.hpp"
#include "civ5/runtime/vp_city_espionage.hpp"
#include "civ5/runtime/vp_espionage_turn.hpp"
#include "civ5/saves/civis_save.hpp"

namespace civ5::runtime {
struct VpEspionageCheckpointBudget {
    std::uint64_t players=1'000'000,cities=1'000'000,entries=100'000'000;
    std::uint64_t bytes=1024ULL*1024*1024;
};
// Versioned VP GameCore overlay for live spies, queued notifications/intrigue,
// rate modifiers and city activity. Version 1 reimports queues from the source.
saves::CivisSaveSection make_vp_espionage_checkpoint(
    const ResearchContext&,const CityYieldContext&,const SessionCheckpointIdentity&,
    VpEspionageCheckpointBudget={});
// Validate the complete section and both live rosters before replacing either.
void restore_vp_espionage_checkpoint(
    const saves::CivisSaveSection&,ResearchContext&,CityYieldContext&,
    const SessionCheckpointIdentity&,VpEspionageCheckpointBudget={});
} // namespace civ5::runtime
