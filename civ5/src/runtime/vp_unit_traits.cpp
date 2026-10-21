#include "civ5/runtime/vp_unit_traits.hpp"
#include "civ5/runtime/vp_religion_beliefs_profile.hpp"
#include <algorithm>
#include <stdexcept>

namespace civ5::runtime {
namespace {
constexpr auto vp=content::TechnologyProfile::vp542;
std::int64_t id64(std::uint64_t id){if(id>INT64_MAX)throw std::overflow_error("VP trait ID exceeds int64");return static_cast<std::int64_t>(id);}
const TeamTechnologies &team(const VpUnitTraitContext &c,std::int64_t id){const auto i=std::find_if(c.teams.begin(),c.teams.end(),[&](const auto &t){return t.team()==id;});
    if(i==c.teams.end()||i->profile()!=vp)throw std::runtime_error("Unresolved VP trait team technologies");return *i;}
void validate(const VpUnitTraitContext &c,std::int64_t player){const auto &w=c.religion.religion.water;
    if(c.players.profile!=vp||c.turn.profile!=vp||c.diplomacy.profile!=vp||w.cities.profile!=vp||&w.players!=&c.players||
       c.religion.treasury.profile!=vp||!c.players.players.contains(player)||!c.turn.players.contains(player)||
       !w.cities.players.contains(player)||!c.religion.treasury.players.contains(player))
        throw std::runtime_error("Unresolved shared VP trait state");
    (void)vp_potential_traits(c.players,player);
}
}
VpUnitTraitRules load_vp_unit_trait_rules(const content::RulesDatabase &db,const content::InfoTypeCatalog &types){
    VpUnitTraitRules r;const auto type=[&](const auto &row,const char *field){const auto text=row.text(field);if(text)if(const auto *entry=types.find(*text))return entry->id;return std::int64_t{-1};};
    const auto traits=db.prefetch_info("Traits",[&](auto id,auto,const auto &row){r.traits[id64(id)]={
        type(row,"ObsoleteTech"),type(row,"PrereqTech"),type(row,"ObsoleteBelief"),
        type(row,"PrereqBelief"),type(row,"ObsoletePolicy"),type(row,"PrereqPolicy")};return true;});
    if(!traits.all_succeeded())throw std::runtime_error("Unresolved VP trait Info: "+traits.error);r.trait_slots=traits.slots;
    const auto leaders=db.prefetch_info("Leaders",[&](auto,auto,const auto &row){const auto name=row.text("Type");if(!name)throw std::runtime_error("VP leader missing type");
        auto &set=r.leader_traits[std::string(*name)];const auto query=db.query_rows({
            "select Traits.ID from Leader_Traits inner join Traits on TraitType = Traits.Type where LeaderType = ?",
            {content::DatabaseValue(std::string(*name))}},[&](const auto &v){const auto id=v.integer_at(0);if(id<0||!r.traits.contains(id))throw std::runtime_error("Unresolved VP leader trait");set.insert(id);});
        if(!query.all_succeeded())throw std::runtime_error("Unresolved VP leader trait relation: "+query.error);return true;});
    if(!leaders.all_succeeded())throw std::runtime_error("Unresolved VP leader Info: "+leaders.error);r.leader_slots=leaders.slots;return r;
}
void rebuild_vp_unit_traits(const VpUnitTraitRules &r,ResearchContext &players,const ResearchTurnState &turn,const PlayerResearchContext &dip,std::int64_t id){
    if(players.profile!=vp||turn.profile!=vp||dip.profile!=vp)throw std::runtime_error("VP unit traits require VP roster");
    auto &p=players.players.at(id);const auto &t=turn.players.at(id);std::vector<std::int64_t> potential;
    if(p.alive&&dip.major_players.contains(id)){
        if(!t.leader_type||!r.leader_traits.contains(*t.leader_type))throw std::runtime_error("Unresolved VP major leader trait Info");
        const auto &owned=r.leader_traits.at(*t.leader_type);for(const auto &[trait,info]:r.traits)if(owned.contains(trait))potential.push_back(trait);
    }
    ensure_vp_unit_trait_profile(players).potential_by_player[id]=std::move(potential);
}
void restore_vp_unit_traits(const VpUnitTraitRules &r,ResearchContext &players,const ResearchTurnState &turn,const PlayerResearchContext &dip){
    if(players.players.size()!=turn.players.size())throw std::runtime_error("VP trait roster mismatch");
    for(const auto &[id,p]:players.players)rebuild_vp_unit_traits(r,players,turn,dip,id);
}
const std::vector<std::int64_t> &vp_potential_unit_traits(const VpUnitTraitContext &c,std::int64_t id){validate(c,id);return vp_potential_traits(c.players,id);}
bool vp_unit_trait_player_has_belief(const VpUnitTraitContext &c,std::int64_t id,std::int64_t belief){validate(c,id);
    if(belief<0)throw std::out_of_range("Invalid VP trait belief");const auto &w=c.religion.religion.water;
    auto religion=vp_owned_religion(c.religion,id);if(religion==-1)religion=vp_created_religion(w,id,true);
    const auto *r=vp_find_religion(w.cities,religion,id);if(!r)return false;
    const auto &lookup=vp_religion_beliefs(*r).lookup;
    const auto found=lookup.find(belief);return found!=lookup.end()&&found->second==1;
}
bool vp_unit_trait_active(const VpUnitTraitContext &c,std::int64_t id,std::int64_t trait){validate(c,id);
    const auto &r=c.rules.traits.at(trait);const auto &p=c.players.players.at(id);
    const auto &potential=vp_potential_traits(c.players,id);
    if(std::find(potential.begin(),potential.end(),trait)==potential.end())return false;
    if(r.obsolete_belief!=-1&&vp_unit_trait_player_has_belief(c,id,r.obsolete_belief))return false;
    if(r.prereq_belief!=-1&&!vp_unit_trait_player_has_belief(c,id,r.prereq_belief))return false;
    if(r.obsolete_policy!=-1&&p.policies.contains(r.obsolete_policy))return false;
    if(r.prereq_policy!=-1&&!p.policies.contains(r.prereq_policy))return false;
    if(r.obsolete_tech!=-1||r.prereq_tech!=-1){const auto &techs=team(c,p.team);
        if(r.obsolete_tech!=-1&&techs.has_tech(r.obsolete_tech))return false;
        if(r.prereq_tech!=-1&&!techs.has_tech(r.prereq_tech))return false;}
    return true;
}
} // namespace civ5::runtime
