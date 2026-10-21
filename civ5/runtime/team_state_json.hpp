#pragma once
#include "civ5/saves/vp_team_profile.hpp"
void teams_json(const civ5::saves::TeamsObservation &m) {
    const auto &d = m.dimensions;
    std::cout << "{\"start_offset\":" << m.start_offset << ",\"end_offset\":" << m.end_offset
              << ",\"dimensions\":{"
              << "\"teams\":" << d.teams << ",\"players\":" << d.players << ",\"civ_teams\":" << d.civ_teams
              << ",\"fixed_players\":" << d.fixed_players << ",\"fixed_teams\":" << d.fixed_teams
              << ",\"domains\":" << d.domains << ",\"major\":" << d.major << ",\"yields\":" << d.yields
              << ",\"vp_rows\":";
    if (const auto *vp_rows = civ5::saves::vp_team_dimensions(d)) {
        const auto &c = *vp_rows;
        std::cout << "{\"vote_sources\":" << c.vote_sources << ",\"small_awards\":" << c.small_awards
                  << ",\"techs\":" << c.techs << ",\"routes\":" << c.routes << ",\"builds\":" << c.builds
                  << ",\"projects\":" << c.projects << ",\"unit_classes\":" << c.unit_classes
                  << ",\"building_classes\":" << c.building_classes << ",\"buildings\":" << c.buildings
                  << ",\"terrains\":" << c.terrains << ",\"features\":" << c.features
                  << ",\"improvements\":" << c.improvements << ",\"victories\":" << c.victories << '}';
    } else
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
    std::cout << "},\"teams\":[";
    for (std::size_t i = 0; i < m.teams.size(); ++i) {
        if (i)
            std::cout << ',';
        const auto &t = m.teams[i];
        std::cout << "{\"index\":" << t.index << ",\"start_offset\":" << t.start_offset
                  << ",\"end_offset\":" << t.end_offset << ",\"fields\":{";
        for (std::size_t j = 0; j < t.fields.size(); ++j) {
            if (j)
                std::cout << ',';
            std::cout << quote(t.fields[j].name) << ':' << t.fields[j].value;
        }
        std::cout << "},\"arrays\":[";
        for (std::size_t j = 0; j < t.arrays.size(); ++j) {
            if (j)
                std::cout << ',';
            const auto &a = t.arrays[j];
            std::cout << "{\"name\":" << quote(a.name) << ",\"columns\":" << a.columns << ",\"values\":";
            values(a.values);
            std::cout << '}';
        }
        std::cout << "],\"hashes\":[";
        for (std::size_t j = 0; j < t.hashes.size(); ++j) {
            if (j)
                std::cout << ',';
            const auto &a = t.hashes[j];
            std::cout << "{\"name\":" << quote(a.name) << ",\"wire_count\":" << a.wire_count << ",\"rows\":[";
            for (std::size_t k = 0; k < a.rows.size(); ++k) {
                if (k)
                    std::cout << ',';
                std::cout << "{\"type_hash\":" << a.rows[k].type_hash << ",\"values\":";
                values(a.rows[k].values);
                std::cout << '}';
            }
            std::cout << "]}";
        }
        std::cout << "],\"project_art\":[";
        for (std::size_t j = 0; j < t.project_art.size(); ++j) {
            if (j)
                std::cout << ',';
            const auto &a = t.project_art[j];
            std::cout << "{\"key\":" << a.key << ",\"source_count\":" << a.source_count << ",\"wire_count\":";
            if (a.wire_count)
                std::cout << *a.wire_count;
            else
                std::cout << "null";
            std::cout << ",\"values\":";
            values(a.values);
            std::cout << '}';
        }
        std::cout << "]}";
    }
    std::cout << "],\"next_field\":\"players\",\"saved_content_profile_verified\":false,\"load_side_effects_"
                 "applied\":false}";
}
