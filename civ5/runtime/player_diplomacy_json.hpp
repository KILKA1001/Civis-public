#pragma once
void player_diplomacy_json(const civ5::saves::PlayerDiplomacyObservation &p) {
    const auto &d = p.dimensions;
    std::cout << "{\"start_offset\":" << p.start_offset << ",\"end_offset\":" << p.end_offset
              << ",\"version\":";
    if (p.version)
        std::cout << *p.version;
    else
        std::cout << "null";
    std::cout << ",\"dimensions\":{\"major\":" << d.major << ",\"minor\":" << d.minor
              << ",\"civ_players\":" << d.civ_players << ",\"players\":" << d.players
              << ",\"approaches\":" << d.approaches << ",\"minor_approaches\":" << d.minor_approaches
              << ",\"statements\":" << d.statements << ",\"log_entries\":" << d.log_entries
              << "},\"fields\":[";
    for (std::size_t i = 0; i < p.fields.size(); ++i) {
        if (i)
            std::cout << ',';
        const auto &f = p.fields[i];
        std::cout << "{\"name\":" << quote(f.name)
                  << ",\"encoding\":" << quote(civ5::saves::diplomacy_encoding_name(f.encoding))
                  << ",\"rows\":" << f.rows << ",\"columns\":" << f.columns << ",\"values\":[";
        for (std::size_t j = 0; j < f.values.size(); ++j) {
            if (j)
                std::cout << ',';
            std::cout << f.values[j];
        }
        std::cout << "],\"spans\":[";
        for (std::size_t j = 0; j < f.spans.size(); ++j) {
            if (j)
                std::cout << ',';
            std::cout << '[' << f.spans[j].start << ',' << f.spans[j].end << ']';
        }
        std::cout << "]}";
    }
    std::cout << "],\"player_index\":0,\"next_field\":\"player_religions\","
                 "\"saved_content_profile_verified\":false,\"load_side_effects_applied\":false}";
}
