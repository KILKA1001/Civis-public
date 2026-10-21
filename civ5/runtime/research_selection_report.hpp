#pragma once
#include "civ5/runtime/research_selection.hpp"
#include <iomanip>
#include <sstream>

namespace civ5::validation {
inline std::string research_selection_report(const runtime::ResearchSelectionState &source,
    const runtime::ResearchTurnState &source_turn,const runtime::PlayerResearchContext &source_players,
    const runtime::ResearchContext &setup,const content::TechnologyRules &technologies,const content::CustomModOptions *options,
    const std::vector<runtime::TeamTechnologies> &teams,const std::map<std::int64_t,std::int64_t> &eras,
    runtime::ResearchPrerequisiteLimits limits,bool no_science) {
    struct Scripts final:runtime::ResearchSelectionServices {
        std::optional<bool> test_all(std::string_view event,std::int64_t,std::int64_t) override {
            throw runtime::ResearchSelectionUnavailable("Lua script system required: "+std::string(event));
        }
        void research_queue_dirty(std::int64_t) override {throw std::logic_error("Availability query mutated queue");}
    } scripts;
    auto state=source;auto turn=source_turn;auto players=source_players;
    runtime::ResearchSelectionContext c{state,turn,players,setup,technologies,options,teams,eras,limits,scripts,-1,no_science};
    std::ostringstream out;
    const auto list=[&](const auto &values){out<<'[';bool comma=false;for(auto n:values){if(comma)out<<',';comma=true;out<<n;}out<<']';};
    const auto values=[&](const auto &v){out<<"{\"extent\":"<<v.extent<<",\"default\":"<<v.default_value<<",\"values\":[";
        bool comma=false;for(auto [index,tech]:v.values){if(comma)out<<',';comma=true;out<<'['<<index<<','<<tech<<']';}out<<"]}";};
    out<<"{\"full_turn\":false,\"script_host\":\"unavailable\",\"state_mutated\":false,\"no_science\":"<<(no_science?"true":"false")
       <<",\"era_restriction\":"<<((options&&options->enabled("BALANCE_ERA_RESTRICTION"))?"true":"false")
       <<",\"or_limit\":"<<limits.or_count<<",\"and_limit\":"<<limits.and_count<<",\"technologies\":[";
    bool comma=false;for(const auto &[id,t]:technologies.entries){if(comma)out<<',';comma=true;
        out<<"{\"id\":"<<id<<",\"disabled\":"<<(t.booleans.at("Disable")?"true":"false")<<",\"era\":"<<t.era<<",\"and\":";
        values(t.and_prerequisites);out<<",\"or\":";values(t.or_prerequisites);out<<'}';}
    out<<"],\"civilization_restrictions\":[";comma=false;
    for(const auto &[type,disabled]:source.disabled_technologies){if(comma)out<<',';comma=true;out<<'['<<std::quoted(type)<<',';list(disabled);out<<']';}
    out<<"],\"columns\":[\"tech\",\"ever\",\"research\",\"trade\"],\"players\":[";comma=false;
    for(const auto &[id,p]:source.players){if(comma)out<<',';comma=true;
        out<<"{\"id\":"<<id<<",\"civilization\":";if(p.civilization)out<<std::quoted(*p.civilization);else out<<"null";
        out<<",\"advanced_start_points\":"<<p.advanced_start_points<<",\"team_era\":"<<eras.at(setup.players.at(id).team)<<",\"availability\":[";
        bool tc=false;for(const auto &[slot,t]:technologies.entries){if(slot>INT64_MAX)throw std::overflow_error("Selection technology exceeds int64");
            if(tc)out<<',';tc=true;const auto tech=static_cast<std::int64_t>(slot);out<<'['<<tech;
            for(int query=0;query<3;++query){out<<',';try {const bool value=query==0?runtime::can_ever_research(c,id,tech):runtime::can_research(c,id,tech,query==2);out<<(value?"true":"false");}
                catch(const runtime::ResearchSelectionUnavailable &e){out<<std::quoted(e.what());}}
            out<<']';}
        out<<"]}";}
    out<<"]}";return out.str();
}
} // namespace civ5::validation
