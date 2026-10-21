#pragma once
void unit_collection_json(const civ5::saves::UnitCollectionHeader &h) {
    std::cout << "{\"start_offset\":" << h.start_offset << ",\"end_offset\":" << h.end_offset
              << ",\"source_slots\":" << h.source_slots << ",\"wire_unit_count\":" << h.wire_unit_count
              << ",\"fields\":{";
    for (std::size_t i = 0; i < h.fields.size(); ++i) {
        if (i)
            std::cout << ',';
        std::cout << quote(h.fields[i].name) << ':' << h.fields[i].value;
    }
    std::cout << "},\"next_free_indices\":[";
    for (std::size_t i = 0; i < h.next_free_indices.size(); ++i) {
        if (i)
            std::cout << ',';
        std::cout << h.next_free_indices[i];
    }
    std::cout << "],\"player_index\":0,\"next_field\":"
              << quote(h.wire_unit_count > 0 ? "first_unit" : "m_armyAIs")
              << ",\"saved_content_profile_verified\":false,\"load_side_effects_applied\":false}";
}
void unit_core_json(const civ5::saves::UnitCoreObservation &p, std::uint64_t player_index = 0,
                    std::uint64_t unit_index = 0) {
    std::cout << "{\"start_offset\":" << p.start_offset << ",\"end_offset\":" << p.end_offset
              << ",\"version\":";
    if (p.version)
        std::cout << *p.version;
    else
        std::cout << "null";
    std::cout << ",\"fields\":[";
    for (std::size_t i = 0; i < p.fields.size(); ++i) {
        if (i)
            std::cout << ',';
        const auto &f = p.fields[i];
        std::cout << "{\"name\":" << quote(f.name)
                  << ",\"encoding\":" << quote(civ5::saves::unit_core_encoding_name(f.encoding))
                  << ",\"start_offset\":" << f.start_offset << ",\"end_offset\":" << f.end_offset
                  << ",\"wire_count\":";
        if (f.wire_count)
            std::cout << *f.wire_count;
        else
            std::cout << "null";
        std::cout << ",\"values\":[";
        for (std::size_t j = 0; j < f.values.size(); ++j) {
            if (j)
                std::cout << ',';
            std::cout << f.values[j];
        }
        std::string hex;
        for (auto b : f.bytes) {
            hex += "0123456789abcdef"[b >> 4];
            hex += "0123456789abcdef"[b & 15];
        }
        std::cout << "],\"bytes_hex\":" << quote(hex) << '}';
    }
    std::cout << "],\"player_index\":" << player_index << ",\"unit_index\":" << unit_index
              << ",\"next_field\":\"unit_promotions\","
                 "\"saved_content_profile_verified\":false,\"load_side_effects_applied\":false}";
}
