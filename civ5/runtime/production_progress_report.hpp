#pragma once
#include "civ5/runtime/production_progress.hpp"
#include <sstream>

namespace civ5::validation {
inline std::string production_progress_report(const runtime::CityYieldContext &world,
    const runtime::ProductionProgressRules &rules,bool checkpoint_loaded){
    std::ostringstream out;
    const auto pairs=[&](const auto &rows){out<<'[';bool first=true;
        for(const auto &[key,value]:rows){if(!first)out<<',';first=false;out<<'['<<key<<','<<value<<']';}out<<']';};
    out<<"{\"turn_continuation_verified\":false,\"checkpoint_loaded\":"
       <<(checkpoint_loaded?"true":"false")<<",\"profile\":\""
       <<(world.profile==content::TechnologyProfile::vp542?"vp542":"bnw")
       <<"\",\"unit_slots\":"<<rules.unit_slots<<",\"building_slots\":"<<rules.building_slots
       <<",\"project_slots\":"<<rules.project_slots<<",\"players\":[";
    bool first_player=true;
    for(const auto &[owner,player]:world.players){
        if(!first_player)out<<',';first_player=false;
        out<<"{\"id\":"<<owner<<",\"cities\":[";
        bool first_city=true;
        for(const auto &[id,city]:player.cities){
            if(!first_city)out<<',';first_city=false;
            const auto &p=*city.production_progress;
            out<<"{\"id\":"<<id<<",\"fully_resolved\":"<<(p.fully_resolved?"true":"false")
               <<",\"overflow_times100\":"<<p.overflow_times100
               <<",\"feature_times100\":"<<p.feature_times100<<",\"units\":";
            pairs(p.units);out<<",\"unit_times\":";pairs(p.unit_times);
            out<<",\"buildings\":";pairs(p.buildings);
            out<<",\"building_times\":";pairs(p.building_times);
            out<<",\"projects\":";pairs(p.projects);
            out<<",\"unresolved_units\":";pairs(p.unresolved_units);
            out<<",\"unresolved_unit_times\":";pairs(p.unresolved_unit_times);
            out<<",\"unresolved_buildings\":";pairs(p.unresolved_buildings);
            out<<",\"unresolved_building_times\":";pairs(p.unresolved_building_times);
            out<<",\"queue\":[";
            bool first_order=true;
            for(const auto &order:*city.production_orders){
                if(!first_order)out<<',';first_order=false;
                out<<'['<<order.type<<','<<order.item<<','<<order.secondary<<','
                   <<(order.save?"true":"false")<<','<<(order.rush?"true":"false")<<',';
                if(order.source_hash)out<<*order.source_hash;else out<<"null";
                out<<']';
            }
            out<<"],\"current_times100\":";
            try{out<<runtime::current_production_progress_times100(city,rules);}
            catch(const runtime::CityYieldUnavailable&){out<<"null";}
            out<<'}';
        }
        out<<"]}";
    }
    out<<"]}";
    return out.str();
}
} // namespace civ5::validation
