#pragma once
void army_operations_node_json(const civ5::saves::ArmyOperationsNode &n) {
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
    std::cout << "],\"children\":[";
    for (std::size_t i = 0; i < n.children.size(); ++i) {
        if (i)
            std::cout << ',';
        army_operations_node_json(n.children[i]);
    }
    std::cout << "]}";
}
void army_operations_json(const civ5::saves::ArmyOperationsObservation &p) {
    const auto &h = p.header;
    std::cout << "{\"start_offset\":" << p.start_offset << ",\"end_offset\":" << p.end_offset
              << ",\"armies_end_offset\":" << p.armies_end_offset
              << ",\"operations_end_offset\":" << p.operations_end_offset
              << ",\"wire_operation_count\":" << p.wire_operation_count
              << ",\"retained_elements\":" << p.retained_elements << ",\"context\":{\"bnw_player_version\":";
    if (p.context.bnw_player_version)
        std::cout << *p.context.bnw_player_version;
    else
        std::cout << "null";
    std::cout << "},\"bnw_unused_popup_count\":";
    if (p.bnw_unused_popup_count)
        std::cout << *p.bnw_unused_popup_count;
    else
        std::cout << "null";
    std::cout << ",\"header\":{\"start_offset\":" << h.start_offset << ",\"end_offset\":" << h.end_offset
              << ",\"source_slots\":" << h.source_slots << ",\"wire_army_count\":" << h.wire_army_count
              << ",\"fields\":{";
    for (std::size_t i = 0; i < h.fields.size(); ++i) {
        if (i)
            std::cout << ',';
        std::cout << quote(h.fields[i].name) << ':' << h.fields[i].value;
    }
    std::cout << "},\"next_free_indices\":[";
    for (std::size_t i = 0; i < h.next_free_indices.size(); ++i) {
        if (i)
            std::cout << ',';
        std::cout << h.next_free_indices[i];
    }
    std::cout << "]},\"armies\":[";
    for (std::size_t i = 0; i < p.armies.size(); ++i) {
        if (i)
            std::cout << ',';
        army_operations_node_json(p.armies[i]);
    }
    std::cout << "],\"operations\":[";
    for (std::size_t i = 0; i < p.operations.size(); ++i) {
        if (i)
            std::cout << ',';
        army_operations_node_json(p.operations[i]);
    }
    std::cout << "],\"player_index\":0,\"next_field\":"
              << quote(p.layout == civ5::saves::GameStateLayout::vp542_corpus ? "m_ReplayData"
                                                                              : "m_ReplayDataSets")
              << ",\"saved_content_profile_verified\":false,\"load_side_effects_applied\":false}";
}
