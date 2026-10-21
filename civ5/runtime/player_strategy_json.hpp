#pragma once
void player_strategy_json(const civ5::saves::PlayerStrategyObservation &p) {
    const auto &d = p.dimensions;
    std::cout << "{\"start_offset\":" << p.start_offset << ",\"end_offset\":" << p.end_offset
              << ",\"dimensions\":{\"major\":" << d.major << ",\"civ_players\":" << d.civ_players
              << ",\"yields\":" << d.yields << ",\"grand\":" << d.grand << ",\"vp_rows\":";
    if (const auto *profile=civ5::saves::vp_player_strategy_dimensions(d)) {
        const auto &c = *profile;
        std::cout << "{\"policies\":" << c.policies << ",\"branches\":" << c.branches
                  << ",\"flavors\":" << c.flavors << ",\"economic\":" << c.economic
                  << ",\"military\":" << c.military << ",\"grand\":" << c.grand << '}';
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
    std::cout << "],\"player_index\":0,\"next_field\":\"diplomacy_ai\","
                 "\"saved_content_profile_verified\":false,\"load_side_effects_applied\":false}";
}
