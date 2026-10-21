#pragma once
void player_tail_node_json(const civ5::saves::PlayerTailNode &n) {
    std::cout << "{\"name\":" << quote(n.name) << ",\"encoding\":" << quote(n.encoding)
              << ",\"start_offset\":" << n.start_offset << ",\"end_offset\":" << n.end_offset
              << ",\"wire_count\":";
    if (n.wire_count)
        std::cout << *n.wire_count;
    else
        std::cout << "null";
    std::cout << ",\"values\":[";
    for (std::size_t i = 0; i < n.values.size(); ++i) {
        if (i)
            std::cout << ',';
        std::cout << n.values[i];
    }
    std::cout << "],\"bytes_hex\":" << quote(bytes_hex(n.bytes)) << ",\"children\":[";
    for (std::size_t i = 0; i < n.children.size(); ++i) {
        if (i)
            std::cout << ',';
        player_tail_node_json(n.children[i]);
    }
    std::cout << "]}";
}
void player_tail_json(const civ5::saves::PlayerTailObservation &p) {
    std::cout << "{\"start_offset\":" << p.start_offset
              << ",\"base_player_end_offset\":" << p.base_player_end_offset
              << ",\"end_offset\":" << p.end_offset << ",\"retained_elements\":" << p.retained_elements
              << ",\"retained_bytes\":" << p.retained_bytes << ",\"context\":";
    if (p.context) {
        const auto &c = *p.context;
        std::cout << "{\"yields\":" << c.yields << ",\"players\":" << c.players << ",\"major\":" << c.major
                  << ",\"player_id\":" << c.player_id << '}';
    } else
        std::cout << "null";
    std::cout << ",\"fields\":[";
    for (std::size_t i = 0; i < p.fields.size(); ++i) {
        if (i)
            std::cout << ',';
        player_tail_node_json(p.fields[i]);
    }
    std::cout << "],\"player_index\":0,\"next_field\":\"next_player\",\"saved_content_profile_verified\":"
                 "false,\"load_side_effects_applied\":false}";
}
