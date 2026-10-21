#pragma once
// Included after quote/bytes_hex inside inspect_main's anonymous namespace.
void game_religion_culture_json(const civ5::saves::GameReligionCultureObservation &state) {
    const auto number = [](const auto &value) {
        if (value)
            std::cout << *value;
        else
            std::cout << "null";
    };
    const auto flag = [](const std::optional<bool> &value) {
        if (value)
            std::cout << (*value ? "true" : "false");
        else
            std::cout << "null";
    };
    const auto numbers = [](const auto &values) {
        std::cout << '[';
        for (std::size_t i = 0; i < values.size(); ++i) {
            if (i)
                std::cout << ',';
            std::cout << values[i];
        }
        std::cout << ']';
    };
    std::cout << "{\"layout\":"
              << quote(state.layout == civ5::saves::GameStateLayout::vp542_corpus ? "vp542_corpus_comparison"
                                                                                  : "expansion2_version1")
              << ",\"start_offset\":" << state.start_offset << ",\"culture_offset\":" << state.culture_offset
              << ",\"end_offset\":" << state.end_offset
              << ",\"next_field\":\"game_leagues\""
                 ",\"saved_content_profile_verified\":false,\"load_side_effects_applied\":false,\"religions_"
                 "version\":";
    number(state.religions_version);
    std::cout << ",\"minimum_faith_next_pantheon\":" << state.minimum_faith_next_pantheon
              << ",\"obsolete_minimum_faith_next_prophet\":";
    number(state.obsolete_minimum_faith_next_prophet);
    std::cout << ",\"religions\":[";
    for (std::size_t i = 0; i < state.religions.size(); ++i) {
        const auto &religion = state.religions[i];
        if (i)
            std::cout << ',';
        std::cout << "{\"version\":";
        number(religion.version);
        std::cout << ",\"religion\":" << religion.religion << ",\"founder\":" << religion.founder
                  << ",\"holy_city_x\":" << religion.holy_city_x
                  << ",\"holy_city_y\":" << religion.holy_city_y
                  << ",\"turn_founded\":" << religion.turn_founded
                  << ",\"pantheon\":" << (religion.pantheon ? "true" : "false")
                  << ",\"enhanced\":" << (religion.enhanced ? "true" : "false") << ",\"reformed\":";
        flag(religion.reformed);
        std::cout << ",\"custom_name_hex\":" << quote(bytes_hex(religion.custom_name))
                  << ",\"beliefs\":{\"version\":";
        const auto &beliefs = religion.beliefs;
        number(beliefs.version);
        std::cout << ",\"religion\":";
        number(beliefs.religion);
        std::cout << ",\"cached_values\":{";
        for (std::size_t j = 0; j < beliefs.cached_values.size(); ++j) {
            if (j)
                std::cout << ',';
            std::cout << quote(beliefs.cached_values[j].name) << ':' << beliefs.cached_values[j].value;
        }
        std::cout << "},\"belief_hashes\":";
        numbers(beliefs.belief_hashes);
        std::cout << ",\"belief_ids\":";
        numbers(beliefs.belief_ids);
        std::cout << ",\"belief_lookup\":";
        numbers(beliefs.belief_lookup);
        std::cout << ",\"building_class_enabled\":[";
        for (std::size_t j = 0; j < beliefs.building_class_enabled.size(); ++j) {
            if (j)
                std::cout << ',';
            const auto &entry = beliefs.building_class_enabled[j];
            std::cout << "{\"type_hash\":" << entry.type_hash << ",\"value\":";
            number(entry.value);
            std::cout << '}';
        }
        std::cout << "]}}";
    }
    std::cout << "],\"culture_version\":";
    number(state.culture_version);
    std::cout << ",\"reported_someone_influential\":"
              << (state.reported_someone_influential ? "true" : "false") << ",\"great_works\":[";
    for (std::size_t i = 0; i < state.great_works.size(); ++i) {
        if (i)
            std::cout << ',';
        const auto &work = state.great_works[i];
        std::cout << "{\"version\":";
        number(work.version);
        std::cout << ",\"creator_hex\":" << quote(bytes_hex(work.creator)) << ",\"legacy_name_hex\":";
        if (work.legacy_name)
            std::cout << quote(bytes_hex(*work.legacy_name));
        else
            std::cout << "null";
        std::cout << ",\"type\":" << work.type << ",\"class_type\":";
        number(work.class_type);
        std::cout << ",\"turn_founded\":" << work.turn_founded << ",\"era\":" << work.era
                  << ",\"player\":" << work.player << '}';
    }
    std::cout << "]}";
}
