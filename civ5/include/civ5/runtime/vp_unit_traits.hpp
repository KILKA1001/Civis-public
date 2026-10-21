#pragma once
#include "civ5/runtime/vp_unit_trait_profile.hpp"
#include "civ5/runtime/research_turn.hpp"
#include "civ5/runtime/vp_city_religion_minor_yields.hpp"

namespace civ5::runtime {
struct VpUnitTraitRules {
    struct Trait {
        std::int64_t obsolete_tech=-1,prereq_tech=-1,obsolete_belief=-1,
            prereq_belief=-1,obsolete_policy=-1,prereq_policy=-1;
    };
    std::map<std::int64_t,Trait> traits;
    std::map<std::string,std::set<std::int64_t>> leader_traits;
    std::uint64_t trait_slots=0,leader_slots=0;
};
VpUnitTraitRules load_vp_unit_trait_rules(const content::RulesDatabase&,const content::InfoTypeCatalog&);
void restore_vp_unit_traits(const VpUnitTraitRules&,ResearchContext&,const ResearchTurnState&,const PlayerResearchContext&);
void rebuild_vp_unit_traits(const VpUnitTraitRules&,ResearchContext&,const ResearchTurnState&,const PlayerResearchContext&,std::int64_t player);
struct VpUnitTraitContext {
    const VpUnitTraitRules &rules;
    ResearchContext &players;
    const ResearchTurnState &turn;
    const PlayerResearchContext &diplomacy;
    const std::vector<TeamTechnologies> &teams;
    const VpCityReligionMinorContext &religion;
};
const std::vector<std::int64_t> &vp_potential_unit_traits(const VpUnitTraitContext&,std::int64_t player);
bool vp_unit_trait_player_has_belief(const VpUnitTraitContext&,std::int64_t player,std::int64_t belief);
bool vp_unit_trait_active(const VpUnitTraitContext&,std::int64_t player,std::int64_t trait);
} // namespace civ5::runtime
