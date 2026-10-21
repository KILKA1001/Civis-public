#pragma once
#include "civ5/saves/game_progression.hpp"
#include "civ5/saves/vp_game_progression_profile.hpp"

// Included inside inspect_main's anonymous namespace after quote/bytes_hex.
void game_progression_json(const civ5::saves::GameProgressionObservation &value) {
    const bool vp = value.layout == civ5::saves::GameStateLayout::vp542_corpus;
    const auto list = [](const auto &items) {
        std::cout << '[';
        for (std::size_t i = 0; i < items.size(); ++i) {
            if (i)
                std::cout << ',';
            std::cout << items[i];
        }
        std::cout << ']';
    };
    std::cout << "{\"layout\":" << quote(vp ? "vp542_corpus_comparison" : "expansion2_version1")
              << ",\"start_offset\":" << value.start_offset
              << ",\"histories_offset\":" << value.histories_offset
              << ",\"random_offset\":" << value.random_offset << ",\"end_offset\":" << value.end_offset
              << ",\"next_field\":\"replay_messages\",\"saved_content_profile_verified\":false"
                 ",\"load_side_effects_applied\":false,\"dimensions_origin\":"
              << quote(vp ? "caller_supplied_unverified" : "retail_403694_expansion2_sdk")
              << ",\"vote_team_slots\":" << value.dimensions.vote_team_slots
              << ",\"victory_award_slots\":" << value.dimensions.victory_award_slots << ",\"vp_rows\":";
    if (const auto *vp_dimensions = civ5::saves::vp_game_progression_dimensions(value.dimensions))
        list(vp_dimensions->rows);
    else
        std::cout << "null";
    std::cout << ",\"arrays\":[";
    for (std::size_t i = 0; i < value.arrays.size(); ++i) {
        const auto &array = value.arrays[i];
        if (i)
            std::cout << ',';
        std::cout << "{\"name\":" << quote(array.name) << ",\"start_offset\":" << array.start_offset
                  << ",\"end_offset\":" << array.end_offset << ",\"columns\":" << array.columns
                  << ",\"rows\":[";
        for (std::size_t j = 0; j < array.rows.size(); ++j) {
            const auto &row = array.rows[j];
            if (j)
                std::cout << ',';
            std::cout << "{\"type_hash\":";
            if (row.type_hash)
                std::cout << *row.type_hash;
            else
                std::cout << "null";
            std::cout << ",\"values\":";
            list(row.values);
            std::cout << '}';
        }
        std::cout << "]}";
    }
    const auto history = [&](const auto &names) {
        std::cout << "{\"hashes\":";
        list(names.hashes);
        std::cout << ",\"strings_hex\":[";
        for (std::size_t i = 0; i < names.strings.size(); ++i) {
            if (i)
                std::cout << ',';
            std::cout << quote(bytes_hex(names.strings[i]));
        }
        std::cout << "]}";
    };
    std::cout << "],\"destroyed_cities\":";
    history(value.destroyed_cities);
    std::cout << ",\"great_people\":";
    history(value.great_people);
    std::cout << ",\"vote_free_lists\":";
    if (!value.vote_free_lists)
        std::cout << "null";
    else {
        std::cout << '[';
        for (std::size_t i = 0; i < value.vote_free_lists->size(); ++i) {
            const auto &free = (*value.vote_free_lists)[i];
            if (i)
                std::cout << ',';
            std::cout << "{\"last_index\":" << free.last_index << ",\"free_head\":" << free.free_head
                      << ",\"free_count\":" << free.free_count << ",\"current_id\":" << free.current_id
                      << ",\"next_free\":";
            list(free.next_free);
            std::cout << ",\"entries\":[";
            for (std::size_t j = 0; j < free.entries.size(); ++j) {
                const auto &entry = free.entries[j];
                if (j)
                    std::cout << ',';
                std::cout << "{\"id\":" << entry.id << ",\"source\":" << entry.source << ",\"options\":[";
                for (std::size_t k = 0; k < entry.options.size(); ++k) {
                    const auto &option = entry.options[k];
                    if (k)
                        std::cout << ',';
                    std::cout << "{\"vote\":" << option.vote << ",\"player\":" << option.player
                              << ",\"city\":" << option.city << ",\"other_player\":" << option.other_player
                              << ",\"text_hex\":" << quote(bytes_hex(option.text)) << '}';
                }
                std::cout << "]}";
            }
            std::cout << "]}";
        }
        std::cout << ']';
    }
    std::cout << ",\"random\":[";
    for (std::size_t i = 0; i < value.random.size(); ++i) {
        const auto &rng = value.random[i];
        if (i)
            std::cout << ',';
        // Decimal strings retain all seed bits in JavaScript report consumers.
        std::cout << "{\"seed_decimal\":" << quote(std::to_string(rng.seed)) << ",\"calls\":" << rng.calls
                  << ",\"resets\":" << rng.resets << '}';
    }
    std::cout << "]}";
}
