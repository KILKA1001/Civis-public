#pragma once
void all_players_json(const civ5::saves::AllPlayersObservation &p) {
    std::cout << "{\"start_offset\":" << p.start_offset << ",\"end_offset\":" << p.end_offset
              << ",\"player_count\":" << p.players.size() << ",\"players\":[";
    for (std::size_t i = 0; i < p.players.size(); ++i) {
        if (i)
            std::cout << ',';
        const auto &v = p.players[i];
        std::cout << "{\"index\":" << v.index << ",\"saved_player_id\":" << v.saved_player_id
                  << ",\"start_offset\":" << v.start_offset << ",\"end_offset\":" << v.end_offset
                  << ",\"cities\":" << v.cities.cities.size() << ",\"units\":" << v.units.units.size()
                  << ",\"armies\":" << v.army_operations.armies.size()
                  << ",\"operations\":" << v.army_operations.operations.size()
                  << ",\"tail_start_offset\":" << v.tail.start_offset
                  << ",\"tail_end_offset\":" << v.tail.end_offset << '}';
    }
    std::cout << "],\"saved_content_profile_verified\":false,\"load_side_effects_applied\":false}";
}
