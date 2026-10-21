#pragma once
#include "civ5/runtime/player_research.hpp"
#include "civ5/runtime/lua_map_services.hpp"
#include "civ5/runtime/profile_extensions.hpp"

namespace civ5::runtime {
struct ResourceRevealInfo { std::int64_t technology=-1,policy=-1; };
struct ResourceVisibilityRules {
    content::TechnologyProfile profile=content::TechnologyProfile::bnw;
    std::map<std::int64_t,ResourceRevealInfo> resources;
    std::uint64_t policy_slots=0;
};
ResourceVisibilityRules load_resource_visibility_rules(const content::RulesDatabase&,const content::InfoTypeCatalog&,
    content::TechnologyProfile);
struct ResourceVisibilityState {
    content::TechnologyProfile profile=content::TechnologyProfile::bnw;
    struct Team {
        std::vector<std::int64_t> forced_resources;
    };
    std::map<std::int64_t,Team> teams;
    ProfileExtensions profile_extensions;
};
// Restores policy ownership into the existing shared players, not a second roster.
ResourceVisibilityState restore_resource_visibility(const saves::SavedSessionSnapshot&,ResearchContext&,
    const ResourceVisibilityRules&,const content::InfoTypeCatalog&);
struct ResourceVisibilityContext {
    const saves::SavedMapProjection &map;
    ResourceVisibilityState &state;
    const ResourceVisibilityRules &rules;
    const ResearchContext &players;
    const PlayerResearchContext &roster;
    std::span<const TeamTechnologies> technologies;
    const content::InfoTypeCatalog &types;
    const content::GlobalDefineCache &defines;
    bool debug_mode; // explicit host state, never guessed from the save
};
void refresh_resource_observer(ResourceVisibilityContext&,std::int64_t team);
bool resource_revealed_by_team(const ResourceVisibilityContext&,std::int64_t team,std::int64_t resource);
std::int64_t plot_resource_type(const ResourceVisibilityContext&,std::int64_t plot,std::int64_t team=-1,
    bool ignore_technology=false);
ScriptMapProvider with_resource_visibility(ScriptMapProvider,const ResourceVisibilityContext&);
} // namespace civ5::runtime
