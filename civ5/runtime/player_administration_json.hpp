#pragma once
void player_administration_record_json(const civ5::saves::PlayerAdministrationRecord &b) {
    const auto values = [](const auto &items) {
        std::cout << '[';
        for (std::size_t i = 0; i < items.size(); ++i) {
            if (i)
                std::cout << ',';
            std::cout << +items[i];
        }
        std::cout << ']';
    };
    std::cout << "{\"name\":" << quote(b.name) << ",\"index\":" << b.index
              << ",\"start_offset\":" << b.start_offset << ",\"end_offset\":" << b.end_offset
              << ",\"fields\":{";
    for (std::size_t i = 0; i < b.fields.size(); ++i) {
        if (i)
            std::cout << ',';
        std::cout << quote(b.fields[i].name) << ':' << b.fields[i].value;
    }
    std::cout << "},\"arrays\":[";
    for (std::size_t i = 0; i < b.arrays.size(); ++i) {
        if (i)
            std::cout << ',';
        const auto &a = b.arrays[i];
        std::cout << "{\"name\":" << quote(a.name) << ",\"columns\":" << a.columns << ",\"values\":";
        values(a.values);
        std::cout << '}';
    }
    std::cout << "],\"blobs\":[";
    for (std::size_t i = 0; i < b.blobs.size(); ++i) {
        if (i)
            std::cout << ',';
        std::cout << "{\"name\":" << quote(b.blobs[i].name) << ",\"bytes\":";
        values(b.blobs[i].bytes);
        std::cout << '}';
    }
    std::cout << "],\"children\":[";
    for (std::size_t i = 0; i < b.children.size(); ++i) {
        if (i)
            std::cout << ',';
        player_administration_record_json(b.children[i]);
    }
    std::cout << "]}";
}
void player_administration_json(const civ5::saves::PlayerAdministrationObservation &p) {
    std::cout << "{\"start_offset\":" << p.start_offset << ",\"end_offset\":" << p.end_offset
              << ",\"blocks\":[";
    for (std::size_t i = 0; i < p.blocks.size(); ++i) {
        if (i)
            std::cout << ',';
        player_administration_record_json(p.blocks[i]);
    }
    std::cout << "],\"player_index\":0,\"next_field\":\"m_cities\","
                 "\"saved_content_profile_verified\":false,\"load_side_effects_applied\":false}";
}
