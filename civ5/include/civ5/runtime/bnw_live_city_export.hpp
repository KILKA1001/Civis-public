#pragma once
#include "civ5/runtime/bnw_city_tail_checkpoint.hpp"
#include "civ5/runtime/live_session.hpp"

namespace civ5::runtime {

struct BnwCityPartialExport {
    saves::CivisSaveDocument document;
    std::vector<std::string_view> uncovered_domains;
    bool full_persistence=false;
};

// Exports four raw BNW city trees from one detached owned world.
// Replaces prior ID25-28 on re-save. ID7 live production is refused until
// active rules/types can prove it agrees with raw city production and core.
// Live CityYieldContext and post-load caches remain outside this partial save.
BnwCityPartialExport make_bnw_live_city_partial_export(
    const LiveSessionState &state,
    const saves::SavedSession &source,
    const SessionCheckpointIdentity &identity,
    std::string_view original_source_sha256);

} // namespace civ5::runtime
