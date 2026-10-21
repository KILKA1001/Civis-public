#pragma once
#include "civ5/runtime/session_checkpoint_identity.hpp"
#include "civ5/runtime/simulation_random.hpp"
#include "civ5/saves/civis_save.hpp"

namespace civ5::runtime {
// Shared map/gameplay RNG state. The selected GameCore supplies each stream's
// algorithm; no profile-specific counters are stored in this common section.
saves::CivisSaveSection make_simulation_random_checkpoint(
    const SessionRandomStreams&,const SessionCheckpointIdentity&);
// Parses and validates both streams before replacing either live generator.
void restore_simulation_random_checkpoint(
    const saves::CivisSaveSection&,SessionRandomStreams&,const SessionCheckpointIdentity&);
} // namespace civ5::runtime
