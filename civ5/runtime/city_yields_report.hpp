#pragma once
#include "civ5/runtime/city_yields.hpp"
#include "civ5/runtime/vp_city_yield_cache_profile.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include <iomanip>
#include <sstream>

namespace civ5::validation {
inline std::string city_yields_report(const runtime::CityYieldContext &state,const runtime::CityYieldRules &rules,
    const content::GlobalDefineCache &defines) {
    std::ostringstream out;
    const auto pairs=[&](const auto &values) {
        out << '['; bool first=true;
        for(const auto &[id,value]:values) {if(!first) out << ',';first=false;out << '[' << id << ',' << value << ']';}
        out << ']';
    };
    const auto numbers=[&](const auto &values) {
        out << '[';bool first=true;for(auto v:values){if(!first)out << ',';first=false;out << v;}out << ']';
    };
    out << "{\"player_science_complete\":false,\"vp_cache_updater_implemented\":false,\"yield_slots\":" << state.yield_slots
        << ",\"no_happiness\":" << (state.no_happiness?"true":"false") << ",\"golden_age_modifiers\":";
    pairs(rules.golden_age_modifiers);
    out << ",\"building_class_slots\":" << rules.building_class_slots << ",\"building_trade_modifiers\":";pairs(rules.building_trade_modifiers);
    out << ",\"civilization_buildings\":[";bool first_civ=true;
    for(const auto &[id,values]:rules.civilization_buildings){if(!first_civ)out << ',';first_civ=false;out << '[' << id << ',';pairs(values);out << ']';}
    out << "],\"defines\":{";
    bool first_define=true;
    for(const auto *name:{"PUPPET_SCIENCE_MODIFIER","PUPPET_GOLD_MODIFIER","CITY_MAX_NUM_BUILDINGS",
                         "VERY_UNHAPPY_PRODUCTION_PENALTY_PER_UNHAPPY","VERY_UNHAPPY_MAX_PRODUCTION_PENALTY",
                         "VERY_UNHAPPY_GOLD_PENALTY_PER_UNHAPPY","VERY_UNHAPPY_MAX_GOLD_PENALTY"}) {
        if(const auto *entry=defines.find(name);entry && std::holds_alternative<std::int64_t>(entry->lookup.value)) {
            if(!first_define)out << ',';first_define=false;out << std::quoted(name) << ':' << std::get<std::int64_t>(entry->lookup.value);
        }
    }
    out << '}';
    out << ",\"belief_follower_maximum\":["; bool first=true;
    for(const auto &[id,values]:rules.belief_follower_maximum) {if(!first)out << ',';first=false;out << '[' << id << ',';pairs(values);out << ']';}
    out << "],\"religions\":[";first=true;
    for(const auto &r:state.religions) {if(!first)out << ',';first=false;out << '[' << r.religion << ',' << r.founder << ',';numbers(r.beliefs);out << ']';}
    out << "],\"area_modifiers\":[";first=true;
    for(const auto &[ids,values]:state.area_modifiers) if(!values.empty()) {if(!first)out << ',';first=false;out << '[' << ids.first << ',' << ids.second << ',';pairs(values);out << ']';}
    out << "],\"trade_routes\":[";first=true;
    for(const auto &r:state.trade_routes) {
        if(!first)out << ',';first=false;
        out << '[' << r.origin_owner << ',' << r.destination_owner << ',' << r.origin_x << ',' << r.origin_y << ',' << r.destination_x << ',' << r.destination_y << ',';
        numbers(r.origin_yields);out << ',';numbers(r.destination_yields);out << ']';
    }
    out << "],\"players\":[";first=true;
    for(const auto &[id,p]:state.players) {
        if(!first)out << ',';first=false;
        out << "{\"id\":" << id << ",\"capital\":" << p.capital << ",\"golden_age\":" << (p.golden_age?"true":"false")
            << ",\"civilization\":" << p.civilization << ",\"minor\":" << (p.minor?"true":"false") << ",\"excess_happiness\":";
        if(p.excess_happiness)out << *p.excess_happiness;else out << "null";
        out << ",\"modifiers\":";pairs(p.modifiers);out << ",\"capital_modifiers\":";pairs(p.capital_modifiers);
        out << ",\"cities\":[";bool first_city=true;std::int64_t science=0;bool resolved=true;
        for(const auto &[city_id,c]:p.cities) {
            if(!first_city)out << ',';first_city=false;
            out << "{\"id\":" << city_id << ",\"x\":" << c.x << ",\"y\":" << c.y << ",\"area\":" << c.area
                << ",\"population\":" << c.population << ",\"resistance\":" << (c.resistance?"true":"false")
                << ",\"razing\":" << (c.razing?"true":"false") << ",\"puppet\":" << (c.puppet?"true":"false")
                << ",\"cached\":";
            if(state.profile==content::TechnologyProfile::vp542) pairs(runtime::vp_city_cached_yields(state,id,city_id));
            else pairs(runtime::YieldValues{});
            out << ",\"source_core\":";
            if(c.source_core)out << '[' << c.source_core->first << ',' << c.source_core->second << ']';else out << "null";
            out << ",\"real_buildings\":";pairs(c.real_buildings);out << ",\"free_buildings\":";pairs(c.free_buildings);
            out << ",\"religions\":[";bool initial=true;
            for(const auto &r:c.religions) {if(!initial)out << ',';initial=false;out << '[' << r.religion << ',' << r.followers << ',' << r.pressure << ']';}
            out << "],\"components\":[";initial=true;
            for(const auto &[yield,v]:c.yields) {
                if(!initial)out << ',';initial=false;
                out << '[' << yield << ',' << v.terrain << ',' << v.buildings << ',' << v.specialists << ',' << v.misc << ',' << v.religion
                    << ',' << v.per_population << ',' << v.per_religion << ',' << v.rate_modifier << ',' << v.resource_modifier << ',' << v.process_modifier << ']';
            }
            out << "],\"quotes\":[";initial=true;
            // Source wire yield span bounds this report, not owned runtime IDs.
            for(std::uint64_t y=0;y<state.yield_slots;++y) {
                if(y>INT64_MAX)throw std::overflow_error("Report yield ID exceeds int64");
                if(!initial)out << ',';initial=false;out << "{\"yield\":" << y;
                try {
                    const auto q=runtime::city_yield_times100(state,rules,defines,id,city_id,static_cast<std::int64_t>(y));
                    out << ",\"status\":\"resolved\",\"cached\":" << (q.cached?"true":"false") << ",\"suppressed\":" << (q.suppressed?"true":"false")
                        << ",\"base_times100\":" << q.base_times100 << ",\"modifier\":" << q.modifier
                        << ",\"process_times100\":" << q.process_times100 << ",\"trade_times100\":" << q.trade_times100 << ",\"total_times100\":" << q.total_times100;
                    if(static_cast<std::int64_t>(y)==state.science)science=runtime::wide::add(science,q.total_times100);
                } catch(const runtime::CityYieldUnavailable &error) {
                    out << ",\"status\":\"unavailable\",\"reason\":" << std::quoted(error.what());
                    if(static_cast<std::int64_t>(y)==state.science)resolved=false;
                }
                out << '}';
            }
            out << "]}";
        }
        out << "],\"science_from_cities_times100\":";
        if(resolved)out << science;else out << "null";
        out << '}';
    }
    out << "]}";return out.str();
}
} // namespace civ5::validation
