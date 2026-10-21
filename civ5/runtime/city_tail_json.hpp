#pragma once
#include "civ5/saves/vp_city_tail_profile.hpp"
void city_tail_node_json(const civ5::saves::CityTailNode &n) {
    std::cout << "{\"name\":" << quote(n.name) << ",\"encoding\":" << quote(n.encoding)
              << ",\"start_offset\":" << n.start_offset << ",\"end_offset\":" << n.end_offset
              << ",\"wire_count\":";
    if (n.wire_count)
        std::cout << *n.wire_count;
    else
        std::cout << "null";
    std::cout << ",\"values\":[";
    for (std::size_t i = 0; i < n.values.size(); ++i) {
        if (i)
            std::cout << ',';
        std::cout << n.values[i];
    }
    std::cout << "],\"children\":[";
    for (std::size_t i = 0; i < n.children.size(); ++i) {
        if (i)
            std::cout << ',';
        city_tail_node_json(n.children[i]);
    }
    std::cout << "]}";
}
void city_tail_json(const civ5::saves::CityTailObservation &p, std::uint64_t player_index = 0,
                    std::uint64_t city_index = 0) {
    std::cout << "{\"start_offset\":" << p.start_offset << ",\"end_offset\":" << p.end_offset
              << ",\"context\":{\"vp_rows\":";
    if (const auto *d = civ5::saves::vp_city_tail_dimensions(p.context))
        std::cout << "{\"yields\":" << d->yields
                  << ",\"emphases\":" << d->emphases << '}';
    else
        std::cout << "null";
    std::cout << ",\"bnw_city_version\":";
    if (p.context.bnw_city_version)
        std::cout << *p.context.bnw_city_version;
    else
        std::cout << "null";
    std::cout << "},\"blocks\":[";
    for (std::size_t i = 0; i < p.blocks.size(); ++i) {
        if (i)
            std::cout << ',';
        city_tail_node_json(p.blocks[i]);
    }
    std::cout << "],\"player_index\":" << player_index << ",\"city_index\":" << city_index
              << ",\"next_field\":\"next_city_or_units\","
                 "\"saved_content_profile_verified\":false,\"load_side_effects_applied\":false}";
}
