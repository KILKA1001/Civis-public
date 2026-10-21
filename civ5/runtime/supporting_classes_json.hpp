#pragma once
void supporting_classes_json(const civ5::saves::SupportingClassesObservation &state) {
    const auto version = [](const auto &value) {
        if (value)
            std::cout << *value;
        else
            std::cout << "null";
    };
    std::cout << "{\"start_offset\":" << state.start_offset << ",\"end_offset\":" << state.end_offset
              << ",\"plot_count\":" << state.plot_count << ",\"major_players\":" << state.major_players
              << ",\"game_version\":";
    version(state.game_version);
    std::cout << ",\"barbarians_version\":";
    version(state.barbarians_version);
    std::cout << ",\"goody_huts_version\":";
    version(state.goody_huts_version);
    std::cout << ",\"barbarians\":" << state.barbarians.size()
              << ",\"goody_hut_rows\":" << state.goody_hut_results.size()
              << ",\"saved_content_profile_verified\":false,\"load_side_effects_applied\":false}";
}
