#include "civ5/runtime/resource_visibility.hpp"
#include "civ5/runtime/vp_resource_visibility_profile.hpp"
#include <algorithm>

namespace civ5::runtime {
namespace {
using content::TechnologyProfile;
template<class T> const T &named(const std::vector<T> &values,std::string_view name){
    const T *result=nullptr;for(const auto &v:values)if(v.name&&std::string_view(v.name)==name){
        if(result)throw std::runtime_error("Duplicate resource visibility field: "+std::string(name));result=&v;
    }
    if(!result)throw std::runtime_error("Missing resource visibility field: "+std::string(name));return *result;
}
std::int64_t i64(std::uint64_t n){if(n>INT64_MAX)throw std::overflow_error("Resource visibility ID exceeds int64");return static_cast<std::int64_t>(n);}
void validate(const ResourceVisibilityContext &c){
    const bool vp=c.rules.profile==TechnologyProfile::vp542;
    if(c.state.profile!=c.rules.profile||c.players.profile!=c.rules.profile||c.roster.profile!=c.rules.profile||
       !c.types.report().all_succeeded()||c.defines.profile()!=(vp?content::GlobalDefineProfile::vp542:content::GlobalDefineProfile::bnw))
        throw std::runtime_error("Resource visibility profile mismatch");
    if(vp)(void)vp_resource_visibility_profile(c.state);
}
bool has_policy(const ResearchPlayerState &p,std::int64_t policy){
    if(!p.policies_resolved)throw std::runtime_error("Resource visibility needs live policy ownership");
    return p.policies.contains(policy); // blocked branches do not affect HasPolicy
}
const TeamTechnologies &techs(const ResourceVisibilityContext &c,std::int64_t team){
    const TeamTechnologies *found=nullptr;
    for(const auto &t:c.technologies)if(t.team()==team){if(found||t.profile()!=c.rules.profile)throw std::runtime_error("Ambiguous resource technology team");found=&t;}
    if(!found)throw std::runtime_error("Missing resource technology team");return *found;
}
bool policy_in_team(const ResourceVisibilityContext &c,std::int64_t team,std::int64_t policy){
    for(const auto &[id,p]:c.players.players)if(p.alive&&p.team==team&&has_policy(p,policy))return true;return false;
}
std::int64_t resolve(const ResourceVisibilityContext &c,const std::optional<std::int64_t> &id,const std::optional<std::uint64_t> &hash){
    if(id&&hash)throw std::runtime_error("Ambiguous resource visibility content identity");if(id)return *id;
    if(hash){if(!*hash)return -1;if(const auto *t=c.types.find_hash(*hash))return t->id;}
    throw std::runtime_error("Unresolved resource visibility content type");
}
std::int64_t define(const ResourceVisibilityContext &c,const char *name){
    const auto *v=c.defines.find(name);if(!v||!v->lookup.error.empty()||!std::holds_alternative<std::int64_t>(v->lookup.value))
        throw std::runtime_error("Missing resource visibility define");
    return std::get<std::int64_t>(v->lookup.value);
}
}
ResourceVisibilityRules load_resource_visibility_rules(const content::RulesDatabase &db,const content::InfoTypeCatalog &types,TechnologyProfile profile){
    if(!types.report().all_succeeded()||(profile!=TechnologyProfile::bnw&&profile!=TechnologyProfile::vp542))throw std::runtime_error("Invalid resource visibility rules profile/types");
    ResourceVisibilityRules r;r.profile=profile;
    const auto policies=db.prefetch_info("Policies");if(!policies.all_succeeded())throw std::runtime_error("Resource Policies Info: "+policies.error);
    r.policy_slots=policies.slots;i64(r.policy_slots);
    const auto resources=db.prefetch_info("Resources",[&](auto slot,auto,const auto &row){
        const auto ref=[&](const char *name){const auto value=row.text(name);const auto *type=value?types.find(value->c_str()):nullptr;return type?type->id:std::int64_t{-1};};
        r.resources.emplace(i64(slot),ResourceRevealInfo{ref("TechReveal"),ref("PolicyReveal")});return true;
    });
    if(!resources.all_succeeded())throw std::runtime_error("Resource Info: "+resources.error);return r;
}
ResourceVisibilityState restore_resource_visibility(const saves::SavedSessionSnapshot &snapshot,ResearchContext &players,
    const ResourceVisibilityRules &rules,const content::InfoTypeCatalog &types){
    const bool vp=rules.profile==TechnologyProfile::vp542;
    const auto layout=vp?saves::GameStateLayout::vp542_corpus:saves::GameStateLayout::expansion2_version1;
    if(!snapshot.teams||!snapshot.all_players||snapshot.teams->layout!=layout||snapshot.all_players->layout!=layout||
       rules.profile!=players.profile||!types.report().all_succeeded())throw std::runtime_error("Resource visibility saved profile mismatch");
    ResourceVisibilityState r;r.profile=rules.profile;
    if(vp)ensure_vp_resource_visibility_profile(r);
    const auto remap=[&](std::uint64_t hash){if(!hash)return std::int64_t{-1};const auto *type=types.find_hash(hash);
        if(!type)throw std::runtime_error("Unknown saved resource/policy hash");return type->id;};
    for(const auto &source:snapshot.teams->teams){
        const auto id=named(source.fields,"id").value;
        if(!players.team_members.contains(id)||r.teams.contains(id))throw std::runtime_error("Resource visibility saved team mismatch");
        ResourceVisibilityState::Team t;
        const auto &a=named(source.arrays,"revealed_resources");if(a.columns!=1)throw std::runtime_error("Invalid team forced-resource dimensions");
        for(const auto value:a.values){
            if(vp)t.forced_resources.push_back(value);
            else {if(value<0||value>UINT32_MAX)throw std::runtime_error("Invalid source resource hash");t.forced_resources.push_back(remap(value));}
        }
        r.teams.emplace(id,std::move(t));
        if(vp)vp_resource_visibility_profile(r).observers.emplace(id,false);
    }
    std::set<std::int64_t> seen;
    for(const auto &source:snapshot.all_players->players){
        const auto id=source.saved_player_id;
        if(!players.players.contains(id)||!seen.insert(id).second||source.strategy.layout!=layout||(vp&&source.traits.layout!=layout))
            throw std::runtime_error("Resource visibility saved player mismatch");
        auto &p=players.players.at(id);std::set<std::int64_t> policies;
        const auto write=[&](std::int64_t policy,std::int64_t flag){
            if(policy<0||static_cast<std::uint64_t>(policy)>=rules.policy_slots||flag<0||flag>255)throw std::runtime_error("Invalid saved policy identity/flag");
            if(flag)policies.insert(policy);else policies.erase(policy);
        };
        const auto &block=named(source.strategy.blocks,"policies");
        if(vp){
            const auto &a=named(block.arrays,"has_policy");
            if(a.columns!=1||a.values.size()!=rules.policy_slots)throw std::runtime_error("Saved policy dimensions differ from active rules");
            for(std::size_t i=0;i<a.values.size();++i)write(i64(i),a.values[i]);
            auto &alternatives=vp_resource_visibility_profile(r).alternate_technologies[id];
            const auto &v=named(source.traits.fields,"m_aiiAlternateResourceTechs");
            if(v.encoding!=saves::PlayerTraitsEncoding::map_iii||v.columns!=3||v.values.size()%3)throw std::runtime_error("Invalid alternate resource technologies");
            for(std::size_t i=0;i<v.values.size();i+=3)
                alternatives.emplace(v.values[i],VpResourceVisibilityProfile::AlternateTechnology{v.values[i+1],v.values[i+2]}); // source insert: first duplicate wins
        }else{
            for(const auto &row:named(block.hashes,"has_policy").rows){
                if(!row.type_hash){if(!row.values.empty())throw std::runtime_error("Policy sentinel has a value");continue;}
                if(row.values.size()!=1)throw std::runtime_error("Invalid policy hash value count");
                write(remap(row.type_hash),row.values[0]); // source assignment: last duplicate wins
            }
        }
        p.policies=std::move(policies);p.policies_resolved=true;
    }
    if(seen.size()!=players.players.size()||r.teams.size()!=players.team_members.size())throw std::runtime_error("Incomplete resource visibility roster");
    return r;
}
void refresh_resource_observer(ResourceVisibilityContext &c,std::int64_t team){
    validate(c);if(c.rules.profile!=TechnologyProfile::vp542)throw std::runtime_error("BNW does not have VP observer cache");
    (void)c.state.teams.at(team);
    auto &observer=vp_resource_visibility_profile(c.state).observers.at(team);observer=false;
    for(const auto id:c.roster.teams.at(team).members)if(id!=-1&&c.players.players.at(id).slot_status==4)observer=true;
}
bool resource_revealed_by_team(const ResourceVisibilityContext &c,std::int64_t team,std::int64_t resource){
    validate(c);if(c.rules.profile!=TechnologyProfile::vp542)throw std::runtime_error("BNW uses separate plot policy and technology gates");
    const auto info=c.rules.resources.find(resource);if(info==c.rules.resources.end())return false;
    for(const auto id:c.roster.teams.at(team).members){
        const auto &p=c.players.players.at(id);if(!p.alive)continue;
        auto technology=info->second.technology;
        const auto &alternatives=vp_resource_visibility_profile(c.state).alternate_technologies.at(id);
        const auto alternative=alternatives.find(resource);
        if(alternative!=alternatives.end()&&alternative->second.reveal!=-1)technology=alternative->second.reveal;
        if((technology==-1||techs(c,team).has_tech(technology))&&(info->second.policy==-1||has_policy(p,info->second.policy)))return true;
    }
    return false;
}
std::int64_t plot_resource_type(const ResourceVisibilityContext &c,std::int64_t plot,std::int64_t team,bool ignore){
    validate(c);const bool vp=c.rules.profile==TechnologyProfile::vp542;
    if(!vp&&ignore)throw std::runtime_error("BNW resource query has no ignore-technology argument");
    if(plot<0||c.map.cells.size()!=c.map.grid.size())throw std::runtime_error("Invalid resource plot/map");
    const auto &p=c.map.cells.at(plot);const auto xy=c.map.grid.coordinates(plot);
    if(p.index!=static_cast<std::uint64_t>(plot)||p.x!=xy[0]||p.y!=xy[1])throw std::runtime_error("Resource plot identity mismatch");
    const auto resource=resolve(c,p.resource_type,p.resource_type_hash);
    if(team==-1||resource==-1)return resource;
    if(vp&&resolve(c,p.improvement_type,p.improvement_type_hash)==define(c,"BARBARIAN_CAMP_IMPROVEMENT"))return -1;
    const auto &state=c.state.teams.at(team);
    if(vp){
        if(c.debug_mode||vp_resource_visibility_profile(c.state).observers.at(team)||ignore||resource_revealed_by_team(c,team,resource))return resource;
    }else{
        const auto &info=c.rules.resources.at(resource);
        if(c.debug_mode)return resource;
        if(info.policy!=-1&&!policy_in_team(c,team,info.policy))return -1;
        if(techs(c,team).has_tech(info.technology))return resource;
    }
    if(std::find(state.forced_resources.begin(),state.forced_resources.end(),resource)!=state.forced_resources.end())return resource;
    if(!p.force_resource_reveal)throw std::runtime_error("Unresolved plot forced-resource flags");
    return p.force_resource_reveal->contains(team)?resource:-1;
}
ScriptMapProvider with_resource_visibility(ScriptMapProvider map,const ResourceVisibilityContext &context){
    validate(context);map.resource_type=[&context](std::int64_t plot,std::int64_t team){return plot_resource_type(context,plot,team);};return map;
}
} // namespace civ5::runtime
