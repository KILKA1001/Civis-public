#pragma once
// Included inside inspect_main's anonymous namespace after quote/bytes_hex.
void game_history_json(const civ5::saves::GameHistoryObservation &value) {
    const auto number = [](const auto &v) {
        if (v)
            std::cout << *v;
        else
            std::cout << "null";
    };
    const auto flag = [](const std::optional<bool> &v) {
        if (v)
            std::cout << (*v ? "true" : "false");
        else
            std::cout << "null";
    };
    std::cout << "{\"layout\":"
              << quote(value.layout == civ5::saves::GameStateLayout::vp542_corpus ? "vp542_corpus_comparison"
                                                                                  : "expansion2_version1")
              << ",\"start_offset\":" << value.start_offset
              << ",\"replay_end_offset\":" << value.replay_end_offset
              << ",\"deals_offset\":" << value.deals_offset << ",\"end_offset\":" << value.end_offset
              << ",\"next_field\":\"game_religions\",\"saved_content_profile_verified\":false"
                 ",\"load_side_effects_applied\":false,\"replay_version\":";
    number(value.replay_version);
    std::cout << ",\"replay\":[";
    for (std::size_t i = 0; i < value.replay.size(); ++i) {
        const auto &message = value.replay[i];
        if (i)
            std::cout << ',';
        std::cout << "{\"turn\":" << message.turn << ",\"type\":" << message.type
                  << ",\"player\":" << message.player << ",\"plots\":[";
        for (std::size_t j = 0; j < message.plots.size(); ++j) {
            if (j)
                std::cout << ',';
            std::cout << '[' << message.plots[j].x << ',' << message.plots[j].y << ']';
        }
        std::cout << "],\"text_hex\":" << quote(bytes_hex(message.text)) << '}';
    }
    std::cout << "],\"sessions\":" << value.sessions << ",\"extra_yields\":[";
    for (std::size_t i = 0; i < value.extra_yields.size(); ++i) {
        const auto &entry = value.extra_yields[i];
        if (i)
            std::cout << ',';
        std::cout << "{\"x\":" << entry.plot.x << ",\"y\":" << entry.plot.y << ",\"yields\":[";
        for (std::size_t j = 0; j < entry.yields.size(); ++j) {
            if (j)
                std::cout << ',';
            std::cout << entry.yields[j];
        }
        std::cout << "]}";
    }
    std::cout << "],\"extra_costs\":[";
    for (std::size_t i = 0; i < value.extra_costs.size(); ++i) {
        const auto &entry = value.extra_costs[i];
        if (i)
            std::cout << ',';
        std::cout << "{\"x\":" << entry.plot.x << ",\"y\":" << entry.plot.y << ",\"cost\":" << entry.cost
                  << '}';
    }
    std::cout << "],\"archaeology_triggered\":" << (value.archaeology_triggered ? "true" : "false")
              << ",\"earliest_barbarian_release_turn\":" << value.earliest_barbarian_release_turn
              << ",\"deals\":{\"version\":";
    number(value.deals.version);
    std::cout << ",\"next_deal_id_derived\":";
    number(value.deals.next_deal_id);
    for (std::size_t group = 0; group < value.deals.groups.size(); ++group) {
        const auto &deals = value.deals.groups[group];
        std::cout << ',' << quote(civ5::saves::DealsObservation::group_names[group]) << ":[";
        for (std::size_t i = 0; i < deals.size(); ++i) {
            const auto &deal = deals[i];
            if (i)
                std::cout << ',';
            std::cout << "{\"version\":";
            number(deal.version);
            std::cout << ",\"id\":";
            number(deal.id);
            std::cout << ",\"from_player\":" << deal.from_player << ",\"to_player\":" << deal.to_player
                      << ",\"final_turn\":" << deal.final_turn << ",\"duration\":" << deal.duration
                      << ",\"start_turn\":" << deal.start_turn << ",\"is_gift\":";
            flag(deal.is_gift);
            std::cout << ",\"considering_renewal\":";
            flag(deal.considering_renewal);
            std::cout << ",\"checked_renewal\":";
            flag(deal.checked_renewal);
            std::cout << ",\"cancelled\":";
            flag(deal.cancelled);
            std::cout << ",\"peace_treaty\":" << deal.peace_treaty
                      << ",\"surrendering_player\":" << deal.surrendering_player
                      << ",\"demanding_player\":" << deal.demanding_player
                      << ",\"requesting_player\":" << deal.requesting_player << ",\"items\":[";
            for (std::size_t j = 0; j < deal.items.size(); ++j) {
                const auto &item = deal.items[j];
                if (j)
                    std::cout << ',';
                std::cout << "{\"version\":";
                number(item.version);
                std::cout << ",\"type\":" << item.type << ",\"duration\":" << item.duration
                          << ",\"final_turn\":" << item.final_turn << ",\"from_player\":" << item.from_player
                          << ",\"data\":[" << item.data[0] << ',' << item.data[1] << ',' << item.data[2]
                          << "],\"flag1\":" << (item.flag1 ? "true" : "false") << ",\"from_renewed\":";
                flag(item.from_renewed);
                std::cout << ",\"to_renewed\":";
                flag(item.to_renewed);
                std::cout << '}';
            }
            std::cout << "]}";
        }
        std::cout << ']';
    }
    std::cout << "}}";
}
