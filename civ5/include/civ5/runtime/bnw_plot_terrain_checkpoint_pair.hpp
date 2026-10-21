#pragma once
#include "civ5/runtime/bnw_map_plot_checkpoint.hpp"
#include "civ5/runtime/bnw_city_core_checkpoint.hpp"
#include "civ5/runtime/city_yields.hpp"
#include <memory>

namespace civ5::runtime {
class LiveSession;

// Existing component IDs 17 and 25, with no additional persistence format.
// These facts do not contain the mandatory refresh/UI lifecycle history.
struct BnwPlotTerrainCheckpointPair {
    saves::CivisSaveSection map,city_core;
};
struct BnwPlotTerrainRestoredComponents {
    saves::SavedSessionSnapshot world;
    CityYieldContext cities;
};
// Built only from a real rules database. Owns the immutable repository-built
// feature/type snapshot and its constructing thread. Build this on the live
// world's owner thread; pair APIs check it BEFORE borrowing the allocation.
// Callers cannot supply a resolver, source-thread ID or rules-hash flag.
class BnwPlotTerrainRulesSnapshot final {
  public:
    static BnwPlotTerrainRulesSnapshot from_rules(const content::RulesDatabase&);
  private:
    struct Impl;
    std::shared_ptr<const Impl> impl_;
    explicit BnwPlotTerrainRulesSnapshot(std::shared_ptr<const Impl>);
    friend BnwPlotTerrainCheckpointPair make_bnw_plot_terrain_checkpoint_pair(
        const LiveSession&,const SessionCheckpointIdentity&,const BnwPlotTerrainRulesSnapshot&);
    friend BnwPlotTerrainRestoredComponents restore_bnw_plot_terrain_checkpoint_pair(
        const BnwPlotTerrainCheckpointPair&,const LiveSession&,
        const SessionCheckpointIdentity&,const BnwPlotTerrainRulesSnapshot&);
};

// Capture only actual current session-issued early-zero refresh mutations.
// Raw/owned terrain mirrors, source working flags and all before-images must
// agree. No caller supplies a yield delta, freshness or completion assertion.
BnwPlotTerrainCheckpointPair make_bnw_plot_terrain_checkpoint_pair(
    const LiveSession&,const SessionCheckpointIdentity&,
    const BnwPlotTerrainRulesSnapshot&);

// Restores both byte components and their terrain mirrors into detached data.
// Does not publish a session, restore its private cursors, or permit gameplay.
BnwPlotTerrainRestoredComponents restore_bnw_plot_terrain_checkpoint_pair(
    const BnwPlotTerrainCheckpointPair&,const LiveSession& source_baseline,
    const SessionCheckpointIdentity&,
    const BnwPlotTerrainRulesSnapshot&);
}
