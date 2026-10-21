#include "civ5/runtime/research_turn.hpp"
#include "civ5/runtime/vp_science_profile.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include <algorithm>

namespace civ5::runtime {
namespace {
using content::TechnologyProfile;
template<class T> const T &named(const std::vector<T> &v,std::string_view name) {
    const T *found=nullptr;for(const auto &f:v)if(f.name&&std::string_view(f.name)==name){if(found)throw std::runtime_error("Duplicate research turn field");found=&f;}
    if(!found)throw std::runtime_error("Missing research turn field: "+std::string(name));return *found;
}
template<class T> std::int64_t scalar(const std::vector<T> &v,std::string_view name) {
    const auto &a=named(v,name).values;if(a.size()!=1)throw std::runtime_error("Invalid research turn scalar");return a.front();
}
std::int64_t id64(std::uint64_t n) {if(n>INT64_MAX)throw std::overflow_error("Research turn ID exceeds int64");return static_cast<std::int64_t>(n);}
const TeamTechnologies &team(const ResearchTurnContext &c,std::int64_t id) {
    const TeamTechnologies *p=nullptr;for(const auto &t:c.progress.teams)if(t.team()==id){if(p)throw std::runtime_error("Duplicate research turn team");p=&t;}
    if(!p)throw ResearchTurnUnavailable("Research turn team is not loaded");return *p;
}
bool vp(const ResearchTurnContext &c) {return c.state.profile==TechnologyProfile::vp542;}
void validate(const ResearchTurnContext &c) {
    const auto &p=c.progress;const auto profile=c.state.profile;
    if(profile!=p.setup.profile||profile!=p.players.profile||profile!=p.rules.profile||profile!=p.technologies.profile||
       profile!=c.science.profile||profile!=c.treasury.profile||profile!=c.maintenance.profile||profile!=c.economy.cities.profile||profile!=c.treasury_rules.profile||
       profile!=c.city_rules.profile||profile!=c.maintenance_rules.profile||!p.rules.all_succeeded()||!p.technologies.all_succeeded()||
       p.defines.profile()!=(vp(c)?content::GlobalDefineProfile::vp542:content::GlobalDefineProfile::bnw)||
       (vp(c)?(!p.options||!p.options->all_succeeded()):p.options!=nullptr)||p.no_science!=c.science.no_science||
       &c.economy.setup!=&p.setup||&c.economy.diplomacy!=&p.players||
       c.economy.technologies.data()!=p.teams.data()||c.economy.technologies.size()!=p.teams.size())
        throw std::runtime_error("Research turn requires one shared world and matching rules");
}
// Low-level BNW completion checks query science again. Bind them to the real
// income calculation and forward all other required effects to the host.
class ProgressWorld final : public ResearchProgressServices {
    ResearchTurnContext &c;ResearchTurnResult &r;
  public:
    ProgressWorld(ResearchTurnContext &context,ResearchTurnResult &result):c(context),r(result){}
    std::int64_t science(std::int64_t id,bool excluded=false) {
        r.science_queries=wide::add(r.science_queries,1);
        return runtime::science_times100(c.science,c.economy,c.treasury,c.maintenance,c.treasury_rules,c.city_rules,
            c.maintenance_rules,c.progress.defines,c.progress.options,id,excluded,c.income_services).total_times100;
    }
    std::int64_t science_times100(std::int64_t id) override {return science(id);}
    void game_data_and_score_dirty(std::int64_t id) override {c.progress.world.game_data_and_score_dirty(id);}
    void acquire_technology(std::int64_t team,std::int64_t tech,std::int64_t id) override {c.progress.world.acquire_technology(team,tech,id);}
    void city_specializations_dirty(std::int64_t id) override {c.progress.world.city_specializations_dirty(id);}
    std::int64_t culture_per_tech(std::int64_t id) override {return c.progress.world.culture_per_tech(id);}
    void add_culture(std::int64_t id,std::int64_t amount) override {c.progress.world.add_culture(id,amount);}
};
}
ResearchTurnState restore_research_turn(const saves::SavedSessionSnapshot &s,const ResearchContext &setup,
    const content::TechnologyRules &techs,const content::InfoTypeCatalog &types,const content::RulesDatabase &db) {
    const bool is_vp=setup.profile==TechnologyProfile::vp542;
    const auto layout=is_vp?saves::GameStateLayout::vp542_corpus:saves::GameStateLayout::expansion2_version1;
    if(!s.all_players||!s.pregame||s.all_players->layout!=layout||s.pregame->layout!=layout||techs.profile!=setup.profile||
       !techs.all_succeeded()||!types.report().all_succeeded())throw std::runtime_error("Research turn import requires complete matching inputs");
    std::set<std::string> leaders,minors;
    for(auto [table,destination]:{std::pair{"Leaders",&leaders},std::pair{"MinorCivilizations",&minors}}) {
        const auto result=db.query_rows({std::string("SELECT Type FROM ")+table,{}},[&](const auto &row){destination->insert(row.text("Type").value_or(""));});
        if(!result.all_succeeded())throw std::runtime_error("Research turn leader catalog unavailable");
    }
    ResearchTurnState state;state.profile=setup.profile;
    const auto &minor_flags=s.pregame->archive.flag_arrays.at("minor_nation_civs");
    for(const auto &saved:s.all_players->players) {
        const auto id=saved.saved_player_id;
        if(id<0||!setup.players.contains(id)||saved.core.layout!=layout||saved.knowledge.layout!=layout)
            throw std::runtime_error("Research turn player/profile mismatch");
        ResearchTurnPlayer p;p.cities_founded=scalar(saved.core.fields,"m_iNumCitiesFounded");
        if(!is_vp)p.founded_first_city=scalar(saved.core.fields,"m_bFoundedFirstCity")!=0;
        const auto &counters=named(saved.core.fields,"m_aiResearchAgreementCounter").values;
        for(std::size_t i=0;i<counters.size();++i)if(counters[i])p.agreement_counters[id64(i)]=counters[i];
        const auto &knowledge=named(saved.knowledge.blocks,"player_techs");
        if(is_vp) {
            const auto &flags=named(knowledge.arrays,"researching_tech").values;
            if(flags.size()!=techs.collection.slots)throw std::runtime_error("VP researching flags disagree with technology catalog");
            for(std::size_t i=0;i<flags.size();++i)if(flags[i])p.researching.insert(id64(i));
        } else for(const auto &row:named(knowledge.hashes,"researching_tech").rows) {
            if(row.type_hash==0)continue; // ReadHashed uses zero alone as its empty sentinel.
            if(row.values.size()!=1)throw std::runtime_error("Invalid BNW researching flag row");
            const auto *type=types.find_hash(row.type_hash);
            if(!type||type->id<0||static_cast<std::uint64_t>(type->id)>=techs.collection.slots){++p.unmapped_researching;continue;}
            if(row.values.front())p.researching.insert(type->id);else p.researching.erase(type->id);
        }
        const auto index=static_cast<std::uint64_t>(id);
        if(index>=minor_flags.size())throw std::runtime_error("Research turn minor flag missing");
        const bool minor=minor_flags[index];
        const auto &keys=minor?s.pregame->archive.string_arrays.at("minor_civ_types"):s.pregame->slots.string_arrays.at("leader_keys");
        if(index<keys.size()&&(minor?minors:leaders).contains(keys[index]))p.leader_type=keys[index];
        if(!state.players.emplace(id,std::move(p)).second)throw std::runtime_error("Duplicate research turn player");
    }
    if(state.players.size()!=setup.players.size())throw std::runtime_error("Research turn roster mismatch");
    return state;
}
void update_research_agreements(ResearchTurnContext &c,std::int64_t id,std::int64_t amount) {
    auto &counters=c.state.players.at(id).agreement_counters;
    const auto tid=c.progress.setup.players.at(id).team;
    const auto &agreements=c.science.research_agreements.at(tid);
    for(auto other:c.progress.players.major_players) {
        const auto other_team=c.progress.setup.players.at(other).team;
        if(other==id||other_team==tid)continue;
        const auto old=counters.contains(other)?counters.at(other):0;
        const auto next=agreements.contains(other_team)?wide::add(old,amount):0;
        if(next)counters[other]=next;else counters.erase(other);
    }
}
void clear_research_queue(ResearchTurnContext &c,std::int64_t id) {
    c.progress.players.players.at(id).queue.clear();c.state.players.at(id).researching.clear();
    const auto tid=c.progress.setup.players.at(id).team;
    if(tid==c.progress.active_team){c.services.research_buttons_dirty(tid);c.progress.world.game_data_and_score_dirty(tid);}
}
void check_research_achievements(ResearchTurnContext &c,std::int64_t id) {
    const auto &base=c.progress.setup;
    if((vp(c)&&!c.progress.options->enabled("ENABLE_ACHIEVEMENTS"))||!base.is_human(base.players.at(id))||
       base.game_type==1||base.game_type==2||base.game_type==3)return;
    const auto &p=c.state.players.at(id);if(!p.leader_type)throw ResearchTurnUnavailable("Research achievement requires a resolved leader Info");
    const auto &own=team(c,base.players.at(id).team);
    if(*p.leader_type=="LEADER_CATHERINE")for(const auto &[slot,info]:c.progress.technologies.entries)
        if(info.base.type=="TECH_HORSEBACK_RIDING"&&own.has_tech(id64(slot))) {
            std::int64_t count=0;for(auto other:c.progress.players.major_players)
                if(!c.science.barbarian_players.contains(other)&&!c.progress.players.players.at(other).pregame_minor&&
                   team(c,base.players.at(other).team).has_tech(id64(slot)))count=wide::add(count,1);
            if(count<=1)c.services.unlock_research_achievement(id,ResearchAchievement::catherine_horseback);
        }
    const auto current=c.progress.players.players.at(id).current_research();
    const auto *info=current<0?nullptr:c.progress.technologies.find(static_cast<std::uint64_t>(current));
    if(!info||!info->booleans.at("Repeat"))return;
    // Source excludes the final slot, not all repeatable Infos. Visit existing
    // Infos sparsely, so a wide logical catalog does not imply a dense loop.
    const auto slots=c.progress.technologies.collection.slots;
    for(const auto &[slot,t]:c.progress.technologies.entries)if(slots>0&&slot<slots-1&&!own.has_tech(id64(slot)))return;
    if(own.at(current).count<1)return;
    c.services.unlock_research_achievement(id,ResearchAchievement::all_technologies);
}
ResearchTurnResult do_research(ResearchTurnContext &c,std::int64_t id) {
    validate(c);ResearchTurnResult r;
    if(c.science.no_science){r.disabled=true;return r;}
    auto &p=c.progress.players.players.at(id);const auto &state=c.state.players.at(id);
    r.research_enabled=vp(c)?state.cities_founded>0:state.founded_first_city;
    ProgressWorld queries(c,r);
    ResearchProgressContext progress{c.progress.players,c.progress.setup,c.progress.rules,c.progress.technologies,c.progress.defines,
        c.progress.options,c.progress.teams,queries,c.progress.active_team,c.progress.no_science};
    const auto modifier=[&](auto tech){return player_research_modifier(progress.players,progress.setup,progress.rules,progress.technologies,
        progress.defines,progress.options,progress.teams,id,tech).value;};
    if(r.research_enabled) {
        if(!vp(c)&&p.current_research()==-1) {
            if(id==c.active_player&&queries.science(id)>0)c.services.choose_research_ui(id);
            if(c.maintenance.elapsed_turns>4){c.services.choose_research_ai(id);r.forced_choice=true;}
        }
        r.technology=p.current_research();
        if(r.technology==-1) {
            const auto science=queries.science(id);
            r.overflow_added=science/(vp(c)?100:std::max<std::int64_t>(1,modifier(-1)));
            change_player_research_overflow_times100(progress,id,r.overflow_added);
        } else {
            r.overflow_spent=vp(c)?p.overflow_times100:wide::multiply_divide(p.overflow_times100,modifier(r.technology),100);
            set_player_research_overflow_times100(progress,id,0);
            const auto tid=progress.setup.players.at(id).team;(void)team(c,tid);
            r.beakers_times100=wide::add(queries.science(id),r.overflow_spent);
            r.progress_change=change_research_progress_times100(progress,tid,r.technology,*r.beakers_times100,id,vp(c)?r.overflow_spent:0,100);
            if(!vp(c)){r.agreement_science=queries.science(id)/100;update_research_agreements(c,id,*r.agreement_science);}
        }
        if(vp(c)) {
            r.agreement_science=queries.science(id,true);update_research_agreements(c,id,*r.agreement_science);
            vp_science_profile(c.science.players.at(id)).last_turn_times100=*r.agreement_science;
            if(p.current_research()==-1) {
                if(!progress.setup.is_human(progress.setup.players.at(id))||progress.options->enabled("AI_CONTROL_TECH_CHOICE"))c.services.choose_research_ai(id);
                else if(id==c.active_player&&queries.science(id)>0)c.services.choose_research_ui(id);
            }
        } else if(r.forced_choice)clear_research_queue(c,id);
    }
    check_research_achievements(c,id);return r;
}
ResearchTurnsLeftQuote research_turns_left(ResearchTurnContext &c,std::int64_t id,
    std::int64_t tech,bool include_overflow) {
    validate(c);
    if(tech<0||!c.progress.technologies.find(static_cast<std::uint64_t>(tech)))
        throw ResearchTurnUnavailable("Research-turn estimate requires a loaded technology");
    const auto &owner=c.progress.setup.players.at(id);
    ResearchTurnsLeftQuote r;
    for(const auto &[other,member]:c.progress.setup.players) {
        if(!member.alive||member.team!=owner.team)continue;
        const auto &player=c.progress.players.players.at(other);
        if(other!=id&&player.current_research()!=tech)continue;
        const auto science=runtime::science_times100(c.science,c.economy,c.treasury,c.maintenance,
            c.treasury_rules,c.city_rules,c.maintenance_rules,c.progress.defines,c.progress.options,
            other,false,c.income_services).total_times100;
        r.research_rate_times100=wide::add(r.research_rate_times100,science);
        const auto modifier=vp(c)?0:player_research_modifier(c.progress.players,c.progress.setup,
            c.progress.rules,c.progress.technologies,c.progress.defines,c.progress.options,
            c.progress.teams,id,tech).value;
        const auto overflow=vp(c)?player.overflow_times100:
            wide::multiply_divide(player.overflow_times100/100,modifier,100);
        r.overflow_adjustment=wide::add(r.overflow_adjustment,overflow);
    }
    if(r.research_rate_times100==0)return r;
    const auto quote=player_research_cost(c.progress.players,c.progress.setup,c.progress.rules,
        c.progress.technologies,c.progress.defines,c.progress.options,c.progress.teams,id,tech);
    r.player_cost=vp(c)?wide::multiply_divide(quote.cost,100,1):quote.cost;
    const TeamTechnologies *team=nullptr;
    for(const auto &candidate:c.progress.teams)if(candidate.team()==owner.team) {
        if(team)throw ResearchTurnUnavailable("Duplicate team for research-turn estimate");
        team=&candidate;
    }
    if(!team)throw ResearchTurnUnavailable("Team unavailable for research-turn estimate");
    r.team_progress=vp(c)?team->progress_times100(tech):team->progress(tech);
    r.remaining=std::max<std::int64_t>(0,wide::subtract(r.player_cost,r.team_progress));
    if(include_overflow)r.remaining=wide::subtract(r.remaining,r.overflow_adjustment);
    // BNW scales whole beakers by 10,000 before dividing by science in
    // hundredths. VP's remaining amount is already in hundredths and uses
    // a factor of 100. Nonpositive remaining still takes source minimum 1.
    const auto factor=vp(c)?100:10000;
    const auto hundredths=r.remaining<=0?std::int64_t{1}:
        std::max<std::int64_t>(1,wide::multiply_add_divide(r.remaining,factor,
            r.research_rate_times100-1,r.research_rate_times100));
    r.turns_times100=hundredths;
    r.turns=wide::add(hundredths/100,hundredths%100!=0?1:0);
    return r;
}
} // namespace civ5::runtime
