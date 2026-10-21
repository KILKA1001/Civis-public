#include "civ5/materials/fog_of_war_frame.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace civ5::materials;
using namespace civ5::terrain;
namespace {
unsigned checks = 0;
void check(bool ok) {
    ++checks;
    if (!ok)
        throw std::runtime_error("FOW frame ownership/extent contract check " + std::to_string(checks));
}
auto snapshot(const FogOfWarState &state) {
    auto values = state.visibility();
    values.insert(values.end(), state.visibility_override().begin(), state.visibility_override().end());
    values.insert(values.end(), state.culling().begin(), state.culling().end());
    for (auto value : state.frame_scale())
        values.push_back(std::bit_cast<std::uint32_t>(value));
    values.push_back(state.dirty());
    return values;
}
void run() {
    auto world = build_world_layout(16, 16, 1, {}, [](float value) { return 1.f / value; });
    FogOfWarState state(16, 16, {}, world.bounds[0], world.bounds[3]);
    state.tick(.125f);
    FogOfWarCamera camera{{0, 0}, {1, .866f}, identity_matrix()};
    std::vector<FogOfWarView> views{
        {identity_matrix(), identity_matrix(), {0, 16, 0, 16}, {-32, 32, -32, 32}}};
    const auto before = snapshot(state);
    const auto make = [&](std::uint32_t quality = 2, bool enabled = true, bool clear = false) {
        return fog_of_war_frame(world, state, camera, views, quality, enabled, clear, false,
                                X87Precision::double_precision);
    };
    const auto good = make();
    check(good.passes.size() == 3 && !good.draws.empty());
    check(snapshot(state) == before);
    const auto reject = [&](auto action) {
        bool caught = false;
        try {
            action();
        } catch (const std::invalid_argument &) {
            caught = true;
        }
        check(caught);
        check(snapshot(state) == before);
    };
    const auto saved_world = world;
    const auto saved_camera = camera;
    const auto saved_views = views;
    world.source_grid.width = 15;
    reject([&] { make(); });
    world = saved_world;
    for (auto value : {std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity()}) {
        camera.height_center[0] = value;
        reject([&] { make(); });
        camera = saved_camera;
        camera.height_scale[1] = value;
        reject([&] { make(); });
        camera = saved_camera;
        camera.projection[11] = value;
        reject([&] { make(); });
        camera = saved_camera;
        world.transforms[31] = value;
        reject([&] { make(); });
        world = saved_world;
        world.bounds[1] = value;
        reject([&] { make(); });
        world = saved_world;
        views[0].overlay[4] = value;
        reject([&] { make(); });
        views = saved_views;
        views[0].view[6] = value;
        reject([&] { make(); });
        views = saved_views;
        views[0].bounds[2] = value;
        reject([&] { make(); });
        views = saved_views;
    }
    views.resize(5, views[0]);
    reject([&] { make(); });
    views = saved_views;
    views[0].tiles[0] = -32769;
    reject([&] { make(); });
    views = saved_views;
    views[0].tiles = {-32768, 32767, -32768, 32767};
    reject([&] { make(); });
    views = saved_views;
    views[0].bounds[0] = -std::numeric_limits<float>::max();
    reject([&] { make(); });
    views = saved_views;
    views[0].bounds = {-100000, 100000, -100000, 100000};
    reject([&] { make(); });
    views = saved_views;
    views.assign(4, {identity_matrix(), identity_matrix(), {0, 0, 0, 0}, {-16384, 16384, -256, 256}});
    reject([&] { make(2, true, true); });
    views = saved_views;
    world.transforms[16] = std::numeric_limits<float>::max();
    views[0].overlay[0] = std::numeric_limits<float>::max();
    reject([&] { make(); });
    reject([&] { make(0); });
    world = saved_world;
    views = saved_views;
    reject([&] {
        fog_of_war_frame(world, state, camera, views, 2, true, false, false, static_cast<X87Precision>(99));
    });
    check(make(0).passes.size() == 1 && make(2, false).passes.size() == 1);
    const auto clear = make(2, false, true);
    check(clear.draws.empty() && clear.passes[0].clear_color == Vector4{1, 1, 1, 1});
    views.clear();
    const auto empty = make();
    check(empty.draws.empty() && empty.passes.size() == 3);
    check(snapshot(state) == before);
}
} // namespace
int main() {
    try {
        run();
        std::cout << "PASS " << checks << " FOW frame guards and ownership checks\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
