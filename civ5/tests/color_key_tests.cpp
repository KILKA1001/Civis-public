#include "civ5/materials/color_keys.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace civ5::assets;
using namespace civ5::materials;
namespace {
unsigned checks = 0;
void check(bool value, const char *reason) {
    ++checks;
    if (!value)
        throw std::runtime_error(reason);
}
template <class F> void rejects(F call) {
    bool failed = false;
    try {
        call();
    } catch (const std::exception &) {
        failed = true;
    }
    check(failed, "Expected ColorKey rejection");
}
void run() {
    const auto identity = ColorKeyVolume::identity();
    check(identity.bgra().size() == 131072, "Complete identity volume");
    // Independent ramp observed in the original conversion kernel. Checking
    // every coordinate catches RGBA/BGRA and axis permutation errors.
    for (unsigned z = 0; z < 32; ++z)
        for (unsigned y = 0; y < 32; ++y)
            for (unsigned x = 0; x < 32; ++x) {
                const auto p = identity.bgra().subspan(((z * 32 + y) * 32 + x) * 4, 4);
                check(p[0] == 3 + 8 * z && p[1] == 3 + 8 * y && p[2] == 3 + 8 * x && p[3] == 255,
                      "Original BGRA texel-center identity");
            }
    Bytes tga(18 + 1024 * 33 * 4, 0);
    tga[2] = 2;
    tga[13] = 4;
    tga[14] = 33;
    tga[16] = 32;
    tga[17] = 8;
    for (unsigned row = 0; row < 33; ++row)
        std::fill_n(tga.begin() + 18 + row * 4096, 4096, static_cast<std::uint8_t>(row));
    const auto bottom = ColorKeyVolume::from_tga(tga);
    check(bottom.bgra().front() == 32 && bottom.bgra().back() == 1, "Flip entire atlas before crop");
    tga[17] = 0x28;
    const auto top = ColorKeyVolume::from_tga(tga);
    check(top.bgra().front() == 0 && top.bgra().back() == 31, "Top origin retains rows before crop");
    for (auto [position, value] : std::initializer_list<std::pair<unsigned, unsigned>>{{0, 1},
                                                                                       {1, 1},
                                                                                       {2, 10},
                                                                                       {3, 1},
                                                                                       {7, 1},
                                                                                       {13, 0},
                                                                                       {14, 0},
                                                                                       {14, 31},
                                                                                       {16, 24},
                                                                                       {17, 0},
                                                                                       {17, 0x38}}) {
        auto bad = tga;
        bad[position] = static_cast<std::uint8_t>(value);
        rejects([&] { ColorKeyVolume::from_tga(bad); });
    }
    for (auto size : {0u, 17u, 18u, 131089u, static_cast<unsigned>(tga.size() - 1)})
        rejects([&] { ColorKeyVolume::from_tga(std::span(tga).first(size)); });
    rejects([] { ColorKeyAssets::load({}); });
    unsigned calls = 0;
    const auto missing = ColorKeyAssets::load([&](const ColorKeyRequest &r) -> std::optional<AssetHandle> {
        check(static_cast<unsigned>(r.system) * 5 + r.style == calls++, "System-major requests");
        return {};
    });
    check(calls == 25, "All 25 source requests");
    for (const auto &entry : missing.volumes()) {
        check(!entry.authored && !entry.asset, "Explicit missing asset provenance");
        check(std::equal(entry.volume.bgra().begin(), entry.volume.bgra().end(), identity.bgra().begin()),
              "Missing assets retain original identity");
    }
    rejects([&] { missing.at(ColorKeySystem::terrain, 5); });
    rejects([&] { missing.at(static_cast<ColorKeySystem>(-1), 0); });
    calls = 0;
    rejects([&] {
        ColorKeyAssets::load([&](const ColorKeyRequest &) -> std::optional<AssetHandle> {
            if (++calls == 7)
                throw std::runtime_error("Resolver error must abort incomplete owner");
            return {};
        });
    });
    check(calls == 7, "Resolver failures propagate");
}
} // namespace
int main() {
    try {
        run();
        std::cout << "PASS " << checks << " ColorKey checks\n";
    } catch (const std::exception &e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
