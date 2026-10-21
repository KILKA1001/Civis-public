#pragma once
void game_final_systems_json(const civ5::saves::GameFinalSystemsObservation &s) {
    std::cout << "{\"layout\":"
              << quote(s.layout == civ5::saves::GameStateLayout::vp542_corpus ? "vp542_corpus_comparison"
                                                                              : "expansion2_version1")
              << ",\"start_offset\":" << s.start_offset << ",\"end_offset\":" << s.end_offset
              << ",\"next_field\":\"save_game_database\",\"load_side_effects_applied\":false,\"saved_content_"
                 "profile_verified\":false,\"vp\":";
    if (!s.vp) {
        std::cout << "null}";
        return;
    }
    const auto &vp = *s.vp;
    std::cout << "{\"game_id_hex\":" << quote(bytes_hex(vp.game_id)) << ",\"corporations\":[";
    for (std::size_t i = 0; i < vp.corporations.size(); ++i) {
        if (i)
            std::cout << ',';
        const auto &c = vp.corporations[i];
        std::cout << "{\"type\":" << c.type << ",\"founder\":" << c.founder
                  << ",\"headquarters_x\":" << c.headquarters_x << ",\"headquarters_y\":" << c.headquarters_y
                  << ",\"turn_founded\":" << c.turn_founded << '}';
    }
    std::cout << ']';
    const auto contracts = [](const char *name, const auto &group) {
        std::cout << ',' << quote(name) << ":[";
        for (std::size_t i = 0; i < group.size(); ++i) {
            if (i)
                std::cout << ',';
            const auto &c = group[i];
            std::cout << "{\"type\":" << c.type << ",\"holder\":" << c.holder << ",\"turns\":" << c.turns
                      << ",\"maintenance\":" << c.maintenance << ",\"turn_start\":" << c.turn_start << '}';
        }
        std::cout << ']';
    };
    contracts("active_contracts", vp.active_contracts);
    contracts("inactive_contracts", vp.inactive_contracts);
    std::cout << "}}";
}
void saved_database_json(const civ5::saves::SavedDatabaseObservation &s) {
    std::cout << "{\"start_offset\":" << s.start_offset << ",\"data_offset\":" << s.data_offset
              << ",\"end_offset\":" << s.end_offset << ",\"bytes\":" << s.image.size()
              << ",\"image_hex\":" << quote(bytes_hex(s.image))
              << ",\"quick_check_passed\":" << (s.quick_check_passed ? "true" : "false")
              << ",\"next_field\":\"map\",\"cache_written\":false,\"saved_content_profile_verified\":false,"
                 "\"gameplay_rules_database\":false}";
}
