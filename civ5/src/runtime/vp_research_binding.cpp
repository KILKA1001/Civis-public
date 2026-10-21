#include "civ5/runtime/vp_research_binding.hpp"
#include "civ5/runtime/vp_research_cost_profile.hpp"
#include "research_binding_helpers.hpp"
#include <stdexcept>

namespace civ5::runtime {
BoundTeamResearch bind_vp_team_research(const ResearchContext &context,
    const content::ResearchRules &rules,const content::GlobalDefineCache &defines,
    const content::CustomModOptions &options,const TeamTechnologies &team) {
    if(context.profile!=content::TechnologyProfile::vp542||team.profile()!=context.profile||
       rules.profile!=context.profile||defines.profile()!=content::GlobalDefineProfile::vp542)
        throw std::runtime_error("Research binding profiles differ");
    if(!rules.all_succeeded()||!options.all_succeeded())
        throw std::runtime_error("Research binding requires complete Info/CustomMods caches");
    using research_binding_detail::average;
    using research_binding_detail::define;
    BoundTeamResearch result;
    auto &f=result.factors;
    auto &vp=ensure_vp_research_cost_profile(f);
    f.members=context.team_members.at(team.team());
    vp.current_era=context.current_era;
    f.world_percent=context.world_percent;
    vp.human_for_handicap=false;
    const bool human_ai=options.enabled("HUMAN_USES_AI_HANDICAP");
    std::vector<std::int64_t> handicaps;
    for(const auto &[id,player]:context.players) {
        if(!player.alive||player.team!=team.team())continue;
        const bool human=context.is_human(player);
        vp.human_for_handicap=vp.human_for_handicap||(human&&!human_ai);
        handicaps.push_back(human&&human_ai?define(defines,"AI_HANDICAP"):player.handicap);
    }
    result.alive_players=handicaps.size();
    result.handicap_id=handicaps.empty()?define(defines,"STANDARD_HANDICAP"):average(handicaps);
    const auto *handicap=rules.handicaps.find(result.handicap_id);
    if(handicap) {
        f.team_handicap_percent=handicap->percent;
        vp.team_handicap_per_era_modifier=handicap->per_era_modifier;
    } else result.missing_infos.push_back("team_handicap");
    if(handicap&&!vp.human_for_handicap) {
        const auto *ai=rules.handicaps.find(context.game_handicap);
        vp.game_ai_handicap_percent=ai?ai->ai_percent:0;
        vp.game_ai_per_era_modifier=ai?ai->ai_per_era_modifier:0;
        if(!ai)result.missing_infos.push_back("game_handicap");
    }
    const auto percent=[&](const auto &collection,std::int64_t id,const char *name) {
        const auto *info=collection.find(id);
        if(info)return info->percent;
        result.missing_infos.push_back(name);
        return std::int64_t{0};
    };
    f.speed_percent=percent(rules.speeds,context.speed,"game_speed");
    f.start_era_percent=percent(rules.eras,context.start_era,"start_era");
    f.extra_team_member_modifier=define(defines,"TECH_COST_EXTRA_TEAM_MEMBER_MODIFIER");
    vp.eurekas_enabled=options.enabled("CIV6_EUREKAS");
    return result;
}
} // namespace civ5::runtime
