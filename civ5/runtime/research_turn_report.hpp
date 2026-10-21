#pragma once
#include "civ5/runtime/research_turn.hpp"
#include "civ5/runtime/vp_science_profile.hpp"
#include <iomanip>
#include <sstream>

namespace civ5::validation {
struct ResearchTurnProbeWorld final : runtime::ResearchProgressServices,runtime::ResearchTurnServices {
    std::vector<std::string> events;
    std::int64_t science_times100(std::int64_t) override {throw std::logic_error("Research turn must use its real science binding");}
    void game_data_and_score_dirty(std::int64_t) override {events.push_back("game_data_and_score_dirty");}
    void acquire_technology(std::int64_t,std::int64_t,std::int64_t) override {
        events.push_back("technology_world_required");throw runtime::ResearchTurnUnavailable("Technology completion requires full world effects");
    }
    void city_specializations_dirty(std::int64_t) override {throw std::logic_error("Probe crossed technology completion boundary");}
    std::int64_t culture_per_tech(std::int64_t) override {throw std::logic_error("Probe crossed technology completion boundary");}
    void add_culture(std::int64_t,std::int64_t) override {throw std::logic_error("Probe crossed technology completion boundary");}
    void choose_research_ai(std::int64_t) override {
        events.push_back("research_ai_required");throw runtime::ResearchTurnUnavailable("Research AI choice requires live queue/AI services");
    }
    void choose_research_ui(std::int64_t) override {events.push_back("choose_research_ui");}
    void research_buttons_dirty(std::int64_t) override {events.push_back("research_buttons_dirty");}
    void unlock_research_achievement(std::int64_t,runtime::ResearchAchievement a) override {
        events.push_back(a==runtime::ResearchAchievement::all_technologies?"all_technologies_achievement_requested":"catherine_achievement_requested");
    }
};
inline std::string research_turn_report(const runtime::ResearchTurnState &source,const runtime::PlayerResearchContext &players,
    const runtime::ResearchContext &setup,const content::ResearchRules &rules,const content::TechnologyRules &techs,
    const content::GlobalDefineCache &d,const content::CustomModOptions *o,const std::vector<runtime::TeamTechnologies> &source_teams,
    const runtime::ScienceContext &source_science,const runtime::CityYieldContext &cities,const runtime::TreasuryContext &treasury,
    const runtime::MaintenanceContext &maintenance,const runtime::TreasuryRules &tr,const runtime::CityYieldRules &cr,
    const runtime::MaintenanceRules &mr,std::int64_t active_player) {
    std::ostringstream out;
    const auto list=[&](const auto &v){out<<'[';bool comma=false;for(auto n:v){if(comma)out<<',';comma=true;out<<n;}out<<']';};
    const auto pairs=[&](const auto &v){out<<'[';bool comma=false;for(auto [a,b]:v){if(comma)out<<',';comma=true;out<<'['<<a<<','<<b<<']';}out<<']';};
    out<<"{\"disposable_copies\":true,\"full_turn\":false,\"steam_called\":false,\"science_is_calculated\":true,\"elapsed_turns\":"<<maintenance.elapsed_turns
       <<",\"no_science\":"<<(source_science.no_science?"true":"false")<<",\"active_player\":"<<active_player
       <<",\"enable_achievements\":"<<((!o||o->enabled("ENABLE_ACHIEVEMENTS"))?"true":"false")
       <<",\"ai_controls_research\":"<<((o&&o->enabled("AI_CONTROL_TECH_CHOICE"))?"true":"false")<<",\"players\":[";
    bool comma=false;
    for(const auto &[id,before]:source.players) {
        if(comma)out<<',';comma=true;
        auto state=source;auto p=players;auto science=source_science;auto m=maintenance;auto teams=source_teams;
        runtime::MaintenanceWorld world{setup,p,cities,teams};ResearchTurnProbeWorld services;
        const auto active_team=setup.players.contains(active_player)?setup.players.at(active_player).team:-1;
        runtime::ResearchProgressContext progress{p,setup,rules,techs,d,o,teams,services,active_team,science.no_science};
        runtime::ResearchTurnContext context{state,progress,science,world,treasury,m,tr,cr,mr,services,active_player,{}};
        std::optional<runtime::ResearchTurnResult> result;std::string error;
        try {result=runtime::do_research(context,id);}
        catch(const runtime::ResearchTurnUnavailable &e){error=e.what();}
        catch(const runtime::ScienceUnavailable &e){error=e.what();}
        catch(const runtime::TreasuryUnavailable &e){error=e.what();}
        catch(const runtime::MaintenanceUnavailable &e){error=e.what();}
        catch(const runtime::CityYieldUnavailable &e){error=e.what();}
        const auto &after=state.players.at(id);const auto tech=players.players.at(id).current_research();
        const auto technology=[&](const auto &values) {
            const auto tid=setup.players.at(id).team;
            for(const auto &t:values)if(t.team()==tid){const auto v=tech<0?runtime::TeamTechnologyValue{}:t.at(tech);
                out<<'['<<v.progress_times100<<','<<v.has_tech<<','<<v.no_trade<<','<<v.count<<']';return;}
            throw std::runtime_error("Report player has no team");
        };
        out<<"{\"id\":"<<id<<",\"status\":"<<std::quoted(result?"applied":"unresolved")<<",\"error\":"<<std::quoted(error)
           <<",\"founded_first_city\":"<<(before.founded_first_city?"true":"false")<<",\"cities_founded\":"<<before.cities_founded
           <<",\"unmapped_researching\":"<<before.unmapped_researching<<",\"leader_type\":";
        if(before.leader_type)out<<std::quoted(*before.leader_type);else out<<"null";
        out<<",\"technology\":"<<tech<<",\"technology_before\":";technology(source_teams);out<<",\"technology_after\":";technology(teams);
        out<<",\"overflow_before\":"<<players.players.at(id).overflow_times100
           <<",\"overflow_after\":"<<p.players.at(id).overflow_times100;
        if(setup.profile==content::TechnologyProfile::vp542)
            out<<",\"last_science_before\":"<<runtime::vp_science_profile(source_science.players.at(id)).last_turn_times100
               <<",\"last_science_after\":"<<runtime::vp_science_profile(science.players.at(id)).last_turn_times100;
        out<<",\"queue_before\":";list(players.players.at(id).queue);out<<",\"queue_after\":";list(p.players.at(id).queue);
        out<<",\"researching_before\":";list(before.researching);out<<",\"researching_after\":";list(after.researching);
        out<<",\"agreements_before\":";pairs(before.agreement_counters);out<<",\"agreements_after\":";pairs(after.agreement_counters);
        out<<",\"result\":";
        if(result){const auto &r=*result;out<<"{\"disabled\":"<<(r.disabled?"true":"false")<<",\"research_enabled\":"<<(r.research_enabled?"true":"false")
            <<",\"forced_choice\":"<<(r.forced_choice?"true":"false")<<",\"overflow_spent\":"<<r.overflow_spent<<",\"overflow_added\":"<<r.overflow_added
            <<",\"science_queries\":"<<r.science_queries<<",\"beakers_times100\":";
            if(r.beakers_times100)out<<*r.beakers_times100;else out<<"null";out<<",\"agreement_science\":";
            if(r.agreement_science)out<<*r.agreement_science;else out<<"null";out<<'}';}
        else out<<"null";
        out<<",\"events\":[";bool ec=false;for(const auto &event:services.events){if(ec)out<<',';ec=true;out<<std::quoted(event);}out<<"]}";
    }
    out<<"]}";return out.str();
}
} // namespace civ5::validation
