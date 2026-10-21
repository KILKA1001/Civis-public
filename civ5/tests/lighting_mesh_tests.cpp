#include "civ5/materials/lighting_mesh.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace civ5::materials;
namespace {
unsigned checks = 0;
void check(bool ok) {
    ++checks;
    if (!ok)
        throw std::runtime_error("Lighting normal24 mesh contract");
}
template <class F> void rejects(F f) {
    bool failed = false;
    try {
        f();
    } catch (const std::invalid_argument &) {
        failed = true;
    }
    check(failed);
}
void run() {
    civ5::assets::Bytes bytes(4 * 24);
    const std::array<std::uint16_t, 6> indices{0, 1, 2, 1, 3, 2};
    const LightingPatchMesh mesh(2, 2, bytes, indices);
    check(mesh.columns() == 2 && mesh.rows() == 2);
    check(std::ranges::equal(mesh.vertices(), bytes) && std::ranges::equal(mesh.indices(), indices));
    bytes[0] = 255;
    check(mesh.vertices()[0] == 0);
    for (int invalid : {std::numeric_limits<int>::min(), -1, 0, 1, 68, std::numeric_limits<int>::max()}) {
        rejects([&] { LightingPatchMesh bad(invalid, 2, bytes, indices); });
        rejects([&] { LightingPatchMesh bad(2, invalid, bytes, indices); });
    }
    rejects([&] { LightingPatchMesh bad(2, 2, std::span(bytes).first(95), indices); });
    rejects([&] { LightingPatchMesh bad(2, 2, bytes, std::span(indices).first(5)); });
    auto bad_indices = indices;
    bad_indices[5] = 4;
    rejects([&] { LightingPatchMesh bad(2, 2, bytes, bad_indices); });
    bad_indices[5] = 65535;
    rejects([&] { LightingPatchMesh bad(2, 2, bytes, bad_indices); });
    for (unsigned offset : {12, 14, 16, 20, 22})
        for (unsigned vertex = 0; vertex < 4; ++vertex)
            for (unsigned value : {0x7c00, 0xfc00, 0x7e01, 0xffff}) {
                auto bad = bytes;
                bad[vertex * 24 + offset] = value & 255;
                bad[vertex * 24 + offset + 1] = value >> 8;
                rejects([&] { LightingPatchMesh rejected(2, 2, bad, indices); });
            }
    // Every binary16 pattern: all finite subnormals/signs survive verbatim.
    for (unsigned word = 0; word <= 65535; ++word) {
        auto copy = bytes;
        copy[12] = word & 255;
        copy[13] = word >> 8;
        if ((word & 0x7c00) == 0x7c00)
            rejects([&] { LightingPatchMesh rejected(2, 2, copy, indices); });
        else {
            const LightingPatchMesh accepted(2, 2, copy, indices);
            check(std::ranges::equal(accepted.vertices(), copy));
        }
    }
    // Shader-unused POSITION bytes and NORMAL.w are opaque, including NaN bits.
    auto opaque = bytes;
    for (unsigned v = 0; v < 4; ++v) {
        std::fill_n(opaque.begin() + v * 24, 12, 255);
        opaque[v * 24 + 18] = 255;
        opaque[v * 24 + 19] = 255;
    }
    const LightingPatchMesh preserved(2, 2, opaque, indices);
    check(std::ranges::equal(preserved.vertices(), opaque));
    const auto copy = preserved;
    auto moving = copy;
    const auto moved = std::move(moving);
    check(std::ranges::equal(moved.vertices(), copy.vertices()) &&
          std::ranges::equal(moved.indices(), copy.indices()));
    civ5::assets::Bytes large(67 * 67 * 24);
    std::vector<std::uint16_t> large_indices(66 * 66 * 6, 4488);
    const LightingPatchMesh maximum(67, 67, large, large_indices);
    check(maximum.vertices().size() == 107736 && maximum.indices().size() == 26136);
}
} // namespace
int main() {
    try {
        run();
        std::cout << "PASS lighting mesh checks=" << checks << '\n';
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
