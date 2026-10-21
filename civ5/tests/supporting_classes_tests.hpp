#pragma once
#include "civ5/saves/supporting_classes.hpp"

namespace {
void append_i32(std::string &bytes, std::int32_t value) {
    for (int i = 0; i < 4; ++i)
        bytes.push_back(static_cast<char>((static_cast<std::uint32_t>(value) >> (8 * i)) & 0xff));
}
void append_i16(std::string &bytes, std::int16_t value) {
    for (int i = 0; i < 2; ++i)
        bytes.push_back(static_cast<char>((static_cast<std::uint16_t>(value) >> (8 * i)) & 0xff));
}
}

void supporting_classes_tests() {
    static_assert(sizeof(void *) == 8);
    std::string bytes;
    append_i32(bytes, 1); // CvGame supporting-data version
    append_i32(bytes, 1); // CvBarbarians version
    append_i16(bytes, -2);
    append_i16(bytes, 2);
    append_i32(bytes, 1); // CvGoodyHuts version
    append_i32(bytes, 1);
    append_i32(bytes, -2);
    append_i32(bytes, 3);
    std::istringstream stream(bytes);
    runtime::BinaryReader input(stream, bytes.size());
    const auto state = saves::observe_supporting_classes(input, saves::GameStateLayout::expansion2_version1, 1, 1);
    check(state.game_version == 1 && state.barbarians_version == 1 && state.goody_huts_version == 1,
          "BNW supporting versions lost");
    check(state.barbarians.size() == 1 && !state.barbarians[0].attacked && state.barbarians[0].counter == -2 &&
              state.barbarians[0].units_spawned == 2,
          "BNW barbarian wire values lost");
    check(state.goody_hut_results.size() == 1 && state.goody_hut_results[0][0] == 1 &&
              state.goody_hut_results[0][1] == -2 && state.goody_hut_results[0][2] == 3,
          "BNW goody hut wire values lost");
    check(input.position() == bytes.size(), "BNW supporting classes boundary lost");

    bytes.clear();
    bytes.push_back('\x01');
    append_i16(bytes, -2);
    append_i16(bytes, 2);
    append_i32(bytes, 1);
    append_i32(bytes, -2);
    append_i32(bytes, 3);
    std::istringstream vp_stream(bytes);
    runtime::BinaryReader vp_input(vp_stream, bytes.size());
    const auto vp = saves::observe_supporting_classes(vp_input, saves::GameStateLayout::vp542_corpus, 1, 1);
    check(!vp.game_version && vp.barbarians[0].attacked && vp.barbarians[0].counter == -2,
          "VP supporting wire values lost");
    check(vp_input.position() == bytes.size(), "VP supporting classes boundary lost");

    std::string invalid_bytes("\x02\0\0\0\0", 5);
    std::istringstream invalid(invalid_bytes);
    runtime::BinaryReader bad(invalid, invalid_bytes.size());
    rejects([&] { saves::observe_supporting_classes(bad, saves::GameStateLayout::vp542_corpus, 1, 0); });
    check(bad.position() == 1, "Invalid VP supporting flag did not stop at field");
}
