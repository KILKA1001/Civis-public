#pragma once
void city_core_json(const civ5::saves::CityCoreObservation &p, std::uint64_t player_index = 0,
                    std::uint64_t city_index = 0) {
    std::cout << "{\"start_offset\":" << p.start_offset << ",\"end_offset\":" << p.end_offset
              << ",\"version\":";
    if (p.version)
        std::cout << *p.version;
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
    std::cout << ",\"fields\":[";
    for (std::size_t i = 0; i < p.fields.size(); ++i) {
        if (i)
            std::cout << ',';
        const auto &f = p.fields[i];
        std::cout << "{\"name\":" << quote(f.name)
                  << ",\"encoding\":" << quote(civ5::saves::city_core_encoding_name(f.encoding))
                  << ",\"start_offset\":" << f.start_offset << ",\"end_offset\":" << f.end_offset
                  << ",\"wire_count\":";
        if (f.wire_count)
            std::cout << *f.wire_count;
        else
            std::cout << "null";
        std::cout << ",\"values\":";
        values(f.values);
        std::cout << ",\"row_keys\":";
        values(f.row_keys);
        std::cout << ",\"row_counts\":";
        values(f.row_counts);
        std::cout << ",\"hashes\":";
        values(f.hashes);
        std::string hex;
        for (auto b : f.bytes) {
            hex += "0123456789abcdef"[b >> 4];
            hex += "0123456789abcdef"[b & 15];
        }
        std::cout << ",\"bytes_hex\":" << quote(hex) << '}';
    }
    std::cout << "],\"player_index\":" << player_index << ",\"city_index\":" << city_index
              << ",\"next_field\":\"city_buildings\","
                 "\"saved_content_profile_verified\":false,\"load_side_effects_applied\":false}";
}

void city_collection_json(const civ5::saves::CityCollectionHeader &h) {
    std::cout << "{\"start_offset\":" << h.start_offset << ",\"end_offset\":" << h.end_offset
              << ",\"source_slots\":" << h.source_slots << ",\"wire_city_count\":" << h.wire_city_count
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
              << quote(h.wire_city_count > 0 ? "first_city" : "m_units")
              << ",\"saved_content_profile_verified\":false,\"load_side_effects_applied\":false}";
}
