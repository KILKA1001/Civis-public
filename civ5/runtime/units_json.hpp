#pragma once
#include "unit_core_json.hpp"
void unit_tail_node_json(const civ5::saves::UnitTailNode &n) {
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
    std::string hex;
    for (auto b : n.bytes) {
        hex += "0123456789abcdef"[b >> 4];
        hex += "0123456789abcdef"[b & 15];
    }
    std::cout << "],\"bytes_hex\":" << quote(hex) << ",\"children\":[";
    for (std::size_t i = 0; i < n.children.size(); ++i) {
        if (i)
            std::cout << ',';
        unit_tail_node_json(n.children[i]);
    }
    std::cout << "]}";
}
void unit_tail_json(const civ5::saves::UnitTailObservation &p, std::uint64_t unit_index = 0) {
    std::cout << "{\"start_offset\":" << p.start_offset << ",\"end_offset\":" << p.end_offset
              << ",\"context\":{\"bnw_unit_version\":";
    if (p.context.bnw_unit_version)
        std::cout << *p.context.bnw_unit_version;
    else
        std::cout << "null";
    std::cout << "},\"blocks\":[";
    for (std::size_t i = 0; i < p.blocks.size(); ++i) {
        if (i)
            std::cout << ',';
        unit_tail_node_json(p.blocks[i]);
    }
    std::cout << "],\"player_index\":0,\"unit_index\":" << unit_index
              << ",\"next_field\":\"next_unit_or_armyAIs\","
                 "\"saved_content_profile_verified\":false,\"load_side_effects_applied\":false}";
}
void units_json(const civ5::saves::UnitsObservation &p) {
    std::cout << "{\"start_offset\":" << p.start_offset << ",\"end_offset\":" << p.end_offset
              << ",\"retained_elements\":" << p.retained_elements
              << ",\"retained_bytes\":" << p.retained_bytes << ",\"header\":";
    unit_collection_json(p.header);
    std::cout << ",\"units\":[";
    for (const auto &u : p.units) {
        if (u.index)
            std::cout << ',';
        std::cout << "{\"index\":" << u.index << ",\"core\":";
        unit_core_json(u.core, 0, u.index);
        std::cout << ",\"tail\":";
        unit_tail_json(u.tail, u.index);
        std::cout << '}';
    }
    std::cout << "],\"player_index\":0,\"next_field\":\"m_armyAIs\","
                 "\"saved_content_profile_verified\":false,\"load_side_effects_applied\":false}";
}
