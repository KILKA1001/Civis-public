#pragma once
#include "civ5/saves/vp_city_management_profile.hpp"
void city_management_json(const civ5::saves::CityManagementObservation &p, std::uint64_t player_index = 0,
                          std::uint64_t city_index = 0) {
    std::cout << "{\"start_offset\":" << p.start_offset << ",\"end_offset\":" << p.end_offset
              << ",\"city_plot_slots\":" << p.city_plot_slots << ",\"context\":{\"vp_rows\":";
    if (const auto *d = civ5::saves::vp_city_management_dimensions(p.context)) {
        std::cout << "{\"flavors\":" << d->flavors << ",\"strategies\":" << d->strategies
                  << ",\"promotions\":" << d->promotions << ",\"buildings\":" << d->buildings << '}';
    } else
        std::cout << "null";
    std::cout << ",\"bnw_projects\":";
    if (p.context.bnw_projects)
        std::cout << *p.context.bnw_projects;
    else
        std::cout << "null";
    const auto values = [](const auto &a) {
        std::cout << '[';
        for (std::size_t i = 0; i < a.size(); ++i) {
            if (i)
                std::cout << ',';
            std::cout << a[i];
        }
        std::cout << ']';
    };
    std::cout << "},\"blocks\":[";
    for (std::size_t i = 0; i < p.blocks.size(); ++i) {
        if (i)
            std::cout << ',';
        const auto &b = p.blocks[i];
        std::cout << "{\"name\":" << quote(b.name) << ",\"start_offset\":" << b.start_offset
                  << ",\"end_offset\":" << b.end_offset << ",\"fields\":{";
        for (std::size_t j = 0; j < b.fields.size(); ++j) {
            if (j)
                std::cout << ',';
            std::cout << quote(b.fields[j].name) << ':' << b.fields[j].value;
        }
        std::cout << "},\"arrays\":[";
        for (std::size_t j = 0; j < b.arrays.size(); ++j) {
            if (j)
                std::cout << ',';
            const auto &a = b.arrays[j];
            std::cout << "{\"name\":" << quote(a.name) << ",\"columns\":" << a.columns << ",\"values\":";
            values(a.values);
            std::cout << '}';
        }
        std::cout << "],\"hashes\":[";
        for (std::size_t j = 0; j < b.hashes.size(); ++j) {
            if (j)
                std::cout << ',';
            const auto &h = b.hashes[j];
            std::cout << "{\"name\":" << quote(h.name) << ",\"wire_count\":" << h.wire_count << ",\"rows\":[";
            for (std::size_t k = 0; k < h.rows.size(); ++k) {
                if (k)
                    std::cout << ',';
                std::cout << "{\"type_hash\":" << h.rows[k].type_hash << ",\"values\":";
                values(h.rows[k].values);
                std::cout << '}';
            }
            std::cout << "]}";
        }
        std::cout << "]}";
    }
    std::cout << "],\"player_index\":" << player_index << ",\"city_index\":" << city_index
              << ",\"next_field\":\"city_religions\","
                 "\"saved_content_profile_verified\":false,\"load_side_effects_applied\":false}";
}
