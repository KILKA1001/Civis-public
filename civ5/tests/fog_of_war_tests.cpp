#include "civ5/materials/fog_of_war.hpp"
#include <bit>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace civ5::materials;
namespace {
unsigned checks = 0;
void check(bool value) {
    ++checks;
    if (!value)
        throw std::runtime_error("FOW input ownership contract");
}
void rejects(auto action) {
    bool failed = false;
    try {
        action();
    } catch (const std::logic_error &) {
        failed = true;
    }
    check(failed);
}
auto snapshot(const FogOfWarState &state) {
    auto result = state.visibility();
    result.insert(result.end(), state.visibility_override().begin(), state.visibility_override().end());
    result.insert(result.end(), state.culling().begin(), state.culling().end());
    for (float value : state.frame_scale())
        result.push_back(std::bit_cast<std::uint32_t>(value));
    result.push_back(std::bit_cast<std::uint32_t>(state.elapsed()));
    result.push_back(std::bit_cast<std::uint32_t>(state.normalized_scale()));
    result.push_back(state.dirty());
    result.push_back(static_cast<std::uint32_t>(state.width()));
    result.push_back(static_cast<std::uint32_t>(state.height()));
    return result;
}
void run() {
    rejects([] { FogOfWarAssets::load(2, {}); });
    unsigned requested = 0;
    const auto fallback = FogOfWarAssets::load(0, [&](const char *) -> civ5::assets::AssetHandle {
        ++requested;
        throw std::runtime_error("Unexpected quality-zero asset request");
    });
    check(requested == 0 && fallback.clouds().empty());
    FogOfWarState state(9, 7, {0, 0, 0}, -1000, 1000);
    state.visibility_event({2, 3, -5}, 0);
    state.tick(.125f);
    state.visibility_event_all(1);
    const auto before = snapshot(state);
    for (auto dims : {std::array{0, 7}, std::array{9, -1}, std::array{32769, 7}, std::array{2048, 2048}}) {
        rejects([&] { state.restart(dims[0], dims[1], {0, 0, 0}, -1000, 1000); });
        check(snapshot(state) == before);
    }
    rejects([&] { state.restart(9, 7, {1, 2, 3}, -1000, 1000); });
    check(snapshot(state) == before);
    const float nan = std::numeric_limits<float>::quiet_NaN();
    const float inf = std::numeric_limits<float>::infinity();
    const float maximum = std::numeric_limits<float>::max();
    for (auto bounds : {std::array{0.f, 0.f}, std::array{1.f, -1.f}, std::array{nan, 100.f},
                        std::array{0.f, inf}, std::array{-maximum, maximum}, std::array{0.f, maximum}}) {
        rejects([&] { state.restart(9, 7, {0, 0, 0}, bounds[0], bounds[1]); });
        check(snapshot(state) == before);
    }
    for (auto cube : {std::array{1, 1, 1}, std::array{32768, 0, -32768}, std::array{0, -1, 1},
                      std::array{10, 0, -10}, std::array{0, 7, -7}}) {
        rejects([&] { state.visibility_event(cube, 0); });
        check(snapshot(state) == before);
    }
    for (float delta : {nan, inf, -inf, maximum}) {
        rejects([&] { state.tick(delta); });
        check(snapshot(state) == before);
    }
}
} // namespace
int main() {
    try {
        run();
        std::cout << "PASS FOW guards and failure atomicity: " << checks << '\n';
    } catch (const std::exception &e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
