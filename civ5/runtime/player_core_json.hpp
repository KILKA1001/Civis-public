#pragma once
void player_core_json(const civ5::saves::PlayerCoreObservation &p) {
    std::cout << "{\"start_offset\":" << p.start_offset << ",\"end_offset\":" << p.end_offset
              << ",\"yield_slots\":" << p.yield_slots << ",\"version\":";
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
                  << ",\"encoding\":" << quote(civ5::saves::player_core_encoding_name(f.encoding))
                  << ",\"start_offset\":" << f.start_offset << ",\"end_offset\":" << f.end_offset
                  << ",\"wire_count\":";
        if (f.wire_count)
            std::cout << *f.wire_count;
        else
            std::cout << "null";
        std::cout << ",\"values\":";
        values(f.values);
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
    std::cout << "],\"player_index\":0,\"next_field\":\"player_policies\","
                 "\"saved_content_profile_verified\":false,\"load_side_effects_applied\":false}";
}
