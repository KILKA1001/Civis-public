#pragma once
#include "civ5/runtime/session_checkpoint_identity.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/saves/civis_save.hpp"
#include "civ5/saves/session_snapshot.hpp"

namespace civ5::runtime {

// Version 6 adds exact six signed-short plot yields, complete baseline cache
// identity and source map GUID/wrap. Versions 1..5 remain readable. Other raw
// plot/player-core changes still refuse. This section is not a full live save:
// city terrain deltas and private mandatory UI/history need separate owners.
saves::CivisSaveSection make_bnw_map_plot_checkpoint(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity,
    const saves::SavedTerrainFeatureResolver &features,
    const content::RulesDatabase *rules=nullptr,
    const content::InfoTypeCatalog *types=nullptr,
    const content::GlobalDefineCache *defines=nullptr);

// Applies to a detached owned snapshot, then rebuilds both derived map views.
// Caller must run load hooks and query-cache rebuilds after this operation.
void restore_bnw_map_plot_checkpoint(
    const saves::CivisSaveSection &section,
    saves::SavedSessionSnapshot &world,
    const SessionCheckpointIdentity &identity,
    const saves::SavedTerrainFeatureResolver &features,
    const content::RulesDatabase *rules=nullptr,
    const content::InfoTypeCatalog *types=nullptr,
    const content::GlobalDefineCache *defines=nullptr);

} // namespace civ5::runtime
