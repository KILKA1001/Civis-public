#pragma once
#include "civ5/runtime/lua_script_host.hpp"
#include "civ5/runtime/map_geometry.hpp"
#include "civ5/runtime/profile_extensions.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/saves/session_snapshot.hpp"

namespace civ5::runtime {
struct PlotPassability {
    saves::GameStateLayout layout=saves::GameStateLayout::expansion2_version1;
    // General impassability is common to the source GameCore profiles.
    struct Plot {bool impassable=false;};
    std::vector<Plot> plots;
    ProfileExtensions extensions;
};
// BNW rebuilds its unserialized cache from the selected Info collections.
// VP retains its serialized general/team flags, before later load hooks.
PlotPassability restore_plot_passability(const saves::SavedSessionSnapshot&,
    const content::RulesDatabase&,const content::InfoTypeCatalog&);
// Both references must outlive the host. Graphics and scripts can consume the
// exact same projection; getters read live state instead of duplicating plots.
ScriptMapProvider lua_map_services(const saves::SavedMapProjection&,const PlotPassability&);
} // namespace civ5::runtime
