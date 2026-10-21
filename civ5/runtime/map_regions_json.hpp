#pragma once
#include "map_state_json.hpp"
void map_regions_json(const civ5::saves::MapRegionsObservation &m) {
    std::cout << "{\"start_offset\":" << m.start_offset << ",\"end_offset\":" << m.end_offset
              << ",\"dimensions\":";
    if (m.dimensions) {
        const auto &d = *m.dimensions;
        std::cout << "{\"fixed_players\":" << d.fixed_players << ",\"fixed_teams\":" << d.fixed_teams
                  << ",\"players\":" << d.players << ",\"yields\":" << d.yields << '}';
    } else
        std::cout << "null";
    const auto values = [](const auto &items) {
        std::cout << '[';
        for (std::size_t i = 0; i < items.size(); ++i) {
            if (i)
                std::cout << ',';
            std::cout << items[i];
        }
        std::cout << ']';
    };
    std::cout << ",\"containers\":[";
    for (std::size_t i = 0; i < m.containers.size(); ++i) {
        if (i)
            std::cout << ',';
        const auto &c = m.containers[i];
        std::cout << "{\"name\":" << quote(c.name) << ",\"start_offset\":" << c.start_offset
                  << ",\"end_offset\":" << c.end_offset << ",\"wire_count\":" << c.wire_count
                  << ",\"legacy_metadata\":";
        values(c.legacy_metadata);
        std::cout << ",\"next_free\":";
        values(c.next_free);
        std::cout << ",\"records\":[";
        for (std::size_t j = 0; j < c.records.size(); ++j) {
            if (j)
                std::cout << ',';
            const auto &r = c.records[j];
            std::cout << "{\"start_offset\":" << r.start_offset << ",\"end_offset\":" << r.end_offset
                      << ",\"version\":";
            if (r.version)
                std::cout << *r.version;
            else
                std::cout << "null";
            std::cout << ",\"fields\":{";
            for (std::size_t k = 0; k < r.fields.size(); ++k) {
                if (k)
                    std::cout << ',';
                std::cout << quote(r.fields[k].name) << ':' << r.fields[k].value;
            }
            std::cout << "},\"arrays\":[";
            for (std::size_t k = 0; k < r.arrays.size(); ++k) {
                if (k)
                    std::cout << ',';
                const auto &a = r.arrays[k];
                std::cout << "{\"name\":" << quote(a.name) << ",\"columns\":" << a.columns << ",\"values\":";
                values(a.values);
                std::cout << '}';
            }
            std::cout << "],\"yield_modifiers\":[";
            for (std::size_t k = 0; k < r.yield_modifiers.size(); ++k) {
                if (k)
                    std::cout << ',';
                const auto &v = r.yield_modifiers[k];
                std::cout << "{\"key\":" << v.key << ",\"wire_count\":" << v.wire_count << ",\"values\":";
                values(v.values);
                std::cout << '}';
            }
            std::cout << "],\"resources\":";
            map_resource_json(r.resources);
            std::cout << ",\"improvements\":";
            map_resource_json(r.improvements);
            std::cout << '}';
        }
        std::cout << "]}";
    }
    std::cout << "],\"ai_map_hints\":" << m.ai_map_hints
              << ",\"next_field\":\"teams\",\"saved_content_profile_verified\":false,"
                 "\"load_side_effects_applied\":false}";
}
