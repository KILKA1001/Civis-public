#include "civ5/materials/material_normals.hpp"
#include <bit>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace civ5::materials;
namespace {
int checks = 0;
void check(bool ok) {
    ++checks;
    if (!ok)
        throw std::runtime_error("Material-normal contract failed");
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
} // namespace
int main() {
    try {
        const auto prior = std::bit_cast<std::array<float, 8>>(std::array<std::uint32_t, 8>{
            0xdeadbeef, 0, 0xffffffff, 0x7fc12345, 0x87654321, 0x80000000, 0x7f800000, 0xff800000});
        for (auto resolution : {1, 16, 32, 64, 127, 128, 255, 256, 512, 1024, 16384, 2147483647}) {
            const auto values = material_normal_parameters(resolution, prior);
            check(values[0] == 1.f / static_cast<float>(resolution) && values[1] == values[0]);
            check(std::bit_cast<std::uint32_t>(values[2]) == 0 && values[4] == 16);
            for (auto i : {3, 5, 6, 7})
                check(std::bit_cast<std::uint32_t>(values[i]) == std::bit_cast<std::uint32_t>(prior[i]));
        }
        for (auto resolution : {0, -1, std::numeric_limits<int>::min()})
            rejects([&] { material_normal_parameters(resolution); });
        const auto flat = encode_material_normal(std::array<float, 8>{});
        check(flat == std::array<float, 4>{.5f, .5f, 1, 0});
        check(encode_material_normal(std::array<float, 8>{1, 1, 1, 1, 1, 1, 1, 1}) == flat);
        const auto x = encode_material_normal(std::array<float, 8>{0, .5f, 1, 0, 1, 0, .5f, 1});
        const auto y = encode_material_normal(std::array<float, 8>{0, 0, 0, .5f, .5f, 1, 1, 1});
        check(x[0] > .99f && x[1] == .5f && x[2] > .5f && x[3] == 0);
        check(y[0] == .5f && y[1] < .01f && y[2] == x[2] && y[3] == 0);
        check(encode_material_normal(std::array<float, 8>{0, 1, 0, 1, 0, 1, 0, 1}, 0) == flat);
        for (auto invalid : {std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(),
                             std::numeric_limits<float>::quiet_NaN()}) {
            rejects([&] { encode_material_normal(std::array<float, 8>{}, invalid); });
            for (int at = 0; at < 8; ++at) {
                std::array<float, 8> taps{};
                taps[at] = invalid;
                rejects([&] { encode_material_normal(taps); });
            }
        }
        std::cout << "PASS material normals checks=" << checks << '\n';
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
