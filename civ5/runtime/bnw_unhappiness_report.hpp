#pragma once
#include "civ5/runtime/bnw_unhappiness.hpp"
#include <iomanip>
#include <limits>
#include <sstream>

namespace civ5::validation {
inline std::string bnw_unhappiness_report(const runtime::BnwUnhappinessContext &state,
    const runtime::ResearchContext &setup,const runtime::BnwUnhappinessRules &rules,
    const content::GlobalDefineCache &defines,const std::map<std::int64_t,runtime::BnwUnhappinessQuote> &quotes) {
    std::ostringstream out;out << std::setprecision(std::numeric_limits<double>::max_digits10);
    out << "{\"positive_happiness_recalculated\":false,\"public_opinion_recalculated\":false,\"world_cities_percent\":"
        << state.world_cities_percent << ",\"game_handicap\":" << setup.game_handicap
        << ",\"specialist_slots\":" << rules.specialist_slots << ",\"handicaps\":[";
    bool first=true;
    for(const auto &[id,h]:rules.handicaps) {if(!first)out << ',';first=false;out << '[' << id << ',' << h.cities << ',' << h.population << ',' << h.ai << ']';}
    out << "],\"defines\":{";first=true;
    for(const auto *name:{"UNHAPPINESS_PER_CITY","UNHAPPINESS_PER_CAPTURED_CITY","UNHAPPINESS_PER_POPULATION",
                         "UNHAPPINESS_PER_OCCUPIED_POPULATION","FREE_UNIT_HAPPINESS","DEFAULT_SPECIALIST"}) {
        if(!first)out << ',';first=false;out << std::quoted(name) << ':';
        const auto &v=defines.find(name)->lookup.value;
        if(const auto *n=std::get_if<std::int64_t>(&v))out << *n;else out << std::get<double>(v);
    }
    out << "},\"players\":[";first=true;
    for(const auto &[id,p]:state.players) {
        if(!first)out << ',';first=false;const auto &b=setup.players.at(id);const auto &q=quotes.at(id);
        out << "{\"id\":" << id << ",\"team\":" << b.team << ",\"handicap\":" << b.handicap
            << ",\"human\":" << (setup.is_human(b)?"true":"false") << ",\"alive\":" << (b.alive?"true":"false")
            << ",\"happiness\":" << p.happiness << ",\"units\":" << p.units << ",\"units_modifier\":" << p.units_modifier
            << ",\"population_modifier\":" << p.population_modifier << ",\"cities_modifier\":" << p.cities_modifier
            << ",\"occupied_modifier\":" << p.occupied_modifier << ",\"capital_modifier\":" << p.capital_modifier
            << ",\"trait_cities_modifier\":" << p.trait_cities_modifier << ",\"trait_population_modifier\":" << p.trait_population_modifier
            << ",\"public_opinion\":" << p.public_opinion << ",\"half_specialists\":" << (p.half_specialists?"true":"false") << ",\"cities\":[";
        bool first_city=true;
        for(const auto &[city,c]:p.cities) {
            if(!first_city)out << ',';first_city=false;
            out << "{\"id\":" << city << ",\"occupied\":" << (c.occupied?"true":"false")
                << ",\"no_occupied_unhappiness\":" << (c.no_occupied_unhappiness?"true":"false")
                << ",\"ignore\":" << (c.ignore?"true":"false") << ",\"specialists\":[";
            bool first_specialist=true;
            for(const auto &[type,count]:c.specialists){if(!first_specialist)out << ',';first_specialist=false;out << '[' << type << ',' << count << ']';}
            out << "]}";
        }
        out << "],\"quote\":{\"normal_cities_times100\":" << q.normal_cities_times100
            << ",\"captured_cities_times100\":" << q.captured_cities_times100
            << ",\"normal_population_times100\":" << q.normal_population_times100
            << ",\"occupied_population_times100\":" << q.occupied_population_times100
            << ",\"units_times100\":" << q.units_times100 << ",\"public_opinion\":" << q.public_opinion
            << ",\"before_ai\":" << q.before_ai << ",\"total\":" << q.total << ",\"excess_happiness\":" << q.excess_happiness
            << ",\"ai_discount_applied\":" << (q.ai_discount_applied?"true":"false") << "}}";
    }
    out << "]}";return out.str();
}
} // namespace civ5::validation
