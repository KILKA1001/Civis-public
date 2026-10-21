#pragma once
void player_traits_json(const civ5::saves::PlayerTraitsObservation &p) {
    const auto values = [](const auto &a) {
        std::cout << '[';
        for (std::size_t i = 0; i < a.size(); ++i) {
            if (i)
                std::cout << ',';
            std::cout << a[i];
        }
        std::cout << ']';
    };
    std::cout << "{\"start_offset\":" << p.start_offset << ",\"end_offset\":" << p.end_offset
              << ",\"dimensions\":{\"yields\":" << p.dimensions.yields
              << ",\"terrains\":" << p.dimensions.terrains << "},\"version\":";
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
                  << ",\"encoding\":" << quote(civ5::saves::player_traits_encoding_name(f.encoding))
                  << ",\"start_offset\":" << f.start_offset << ",\"end_offset\":" << f.end_offset
                  << ",\"columns\":" << f.columns << ",\"wire_count\":";
        if (f.wire_count)
            std::cout << *f.wire_count;
        else
            std::cout << "null";
        std::cout << ",\"values\":";
        values(f.values);
        std::cout << ",\"hashes\":";
        values(f.hashes);
        std::cout << ",\"rows\":[";
        for (std::size_t j = 0; j < f.rows.size(); ++j) {
            if (j)
                std::cout << ',';
            const auto &row = f.rows[j];
            std::cout << "{\"key\":" << row.key << ",\"wire_count\":" << row.wire_count << ",\"values\":";
            values(row.values);
            std::cout << '}';
        }
        std::cout << "]}";
    }
    std::cout << "],\"player_index\":0,\"next_field\":\"espionage\","
                 "\"saved_content_profile_verified\":false,\"load_side_effects_applied\":false}";
}
