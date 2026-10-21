#pragma once
#include "city_core_json.hpp"
#include "city_management_json.hpp"
#include "city_production_json.hpp"
#include "city_tail_json.hpp"
#include "civ5/saves/vp_cities_profile.hpp"
void cities_json(const civ5::saves::CitiesObservation &p) {
    std::cout << "{\"start_offset\":" << p.start_offset << ",\"end_offset\":" << p.end_offset
              << ",\"retained_elements\":" << p.retained_elements
              << ",\"retained_string_bytes\":" << p.retained_string_bytes << ",\"context\":{\"vp_rows\":";
    if (const auto *d = p.context.profile_extensions.find<civ5::saves::VpCitiesWireDimensions>()) {
        std::cout << "{\"flavors\":" << d->flavors << ",\"strategies\":" << d->strategies
                  << ",\"promotions\":" << d->promotions << ",\"buildings\":" << d->buildings
                  << ",\"yields\":" << d->yields << ",\"emphases\":" << d->emphases << '}';
    } else
        std::cout << "null";
    std::cout << ",\"bnw_projects\":";
    if (p.context.bnw_projects)
        std::cout << *p.context.bnw_projects;
    else
        std::cout << "null";
    std::cout << "},\"header\":";
    city_collection_json(p.header);
    std::cout << ",\"cities\":[";
    for (const auto &c : p.cities) {
        if (c.index)
            std::cout << ',';
        std::cout << "{\"index\":" << c.index << ",\"core\":";
        city_core_json(c.core, 0, c.index);
        std::cout << ",\"production\":";
        city_production_json(c.production, 0, c.index);
        std::cout << ",\"management\":";
        city_management_json(c.management, 0, c.index);
        std::cout << ",\"tail\":";
        city_tail_json(c.tail, 0, c.index);
        std::cout << '}';
    }
    std::cout << "],\"player_index\":0,\"next_field\":\"m_units\","
                 "\"saved_content_profile_verified\":false,\"load_side_effects_applied\":false}";
}
