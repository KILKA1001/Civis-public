#include "civ5/terrain/height_addressing.hpp"
#include <bit>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace civ5::terrain;
namespace {
int checks = 0;
void check(bool condition, const char* message) {
    ++checks;
    if (!condition) throw std::runtime_error(message);
}
template<class F> void rejects(F action) {
    bool caught = false;
    try { action(); } catch (const std::invalid_argument&) { caught = true; }
    check(caught, "Expected explicit invalid_argument");
}
void test() {
    const HeightDomain domain(2560, 1792);
    const LandLayout layout(domain);
    check(domain.texture_columns() == 3 && domain.texture_rows() == 3, "Terminal texture lost");
    check(layout.columns() == 40 && layout.rows() == 28, "Incorrect LAND dimensions");
    const auto last = layout.patch(39, 27);
    check(layout.patch(0, 0).source_samples_x() == 65 && last.source_samples_x() == 64, "Shared source sample counts");
    check(last.source_samples_y() == 64 && last.source_x() + last.source_samples_x() - 1 == 2559, "Last source sample");
    check(layout.patch(1, 0).patch_index() == 28 && layout.patch(0, 1).patch_index() == 1, "X-major patch order");
    check(layout.patch(13, 13).texture_table_slot() == 0 && layout.patch(14, 14).texture_table_slot() == 17 &&
        last.texture_table_slot() == 18, "Fixed 16-column texture table");
    const auto p = LandLayout(HeightDomain(6656, 4352)).patch(54, 44);
    check(p.texture_table_slot() == 51 && p.height_window().source_origin_x() == 2624, "Original texture offsets");
    check(p.offset_constants() == std::array<float, 4>{3456, 2816, -2624, -2624}, "Per-draw payload");
    check(p.world_translation() == std::array<float, 3>{-3328, -1884.416015625f, -0.0f}, "World translation");
    check(std::bit_cast<std::uint32_t>(p.world_translation()[2]) == 0x80000000u, "Translation signed zero");
    const auto small = LandLayout(HeightDomain(80, 144));
    check(small.columns() == 2 && small.rows() == 3 && small.patch(1, 2).source_samples_x() == 16 &&
        small.patch(1, 2).source_samples_y() == 16, "Partial last patch");
    const auto bounds = small.patch(0, 0).culling_bounds();
    check(bounds[3] - bounds[0] == 65 && bounds[5] == 255.0f * 0.2f, "Culling counts are not mesh spans");
    check(std::bit_cast<std::uint32_t>(bounds[2]) == 0x80000000u, "Culling signed zero");
    check(!domain.descriptor_destination_index(0, -1) && !domain.descriptor_destination_index(0, 1792), "Descriptor must clip Y");
    check(domain.descriptor_destination_index(-1, 1) == 5119, "Descriptor X floor-mod");
    check(domain.texture_source_index(-1, -1) == 2560 * 1792 - 1, "Texture wraps both axes");
    for (const int x : {0, 13, 14, 27, 28, 39}) for (const int y : {0, 13, 14, 27}) {
        const auto patch = layout.patch(x, y);
        for (const float d : {0.0f, 0.25f, 31.5f, static_cast<float>(patch.source_samples_x() - 1)}) {
            const auto uv = patch.displacement_uv(d, 0.5f);
            const auto window_uv = patch.height_window().source_to_uv(patch.source_x() + d, patch.source_y() + 0.5f);
            check(uv.u == window_uv.u && uv.v + 1.0f == window_uv.v, "UV convention at 896 boundary");
        }
    }
    const HeightDomain tiny(16, 16);
    std::vector<std::uint16_t> source(256);
    source[0] = 0x00ff; source[1] = 0xff00; source[16] = 0x8000; source[17] = 0x4000;
    const HeightField field(tiny, source);
    source[0] = 65535;
    check(field.u16_at(0, 0) == 255 && field.a8_at(0, 0) == 0, "Immutable U16/high-byte extraction");
    check(field.u16_at(16, 16) == 255 && field.u16_at(-15, -16) == 0xff00, "Wrapped U16 access");
    check(field.sample_a8_linear(0.5f, 0.5f) == 111.75f, "Filter A8 after high-byte extraction");
    const auto texture = field.extract_a8_texture(0, 0);
    check(texture.size() == 1024 * 1024 && texture[(1023 - 64) * 1024 + 65] == 255, "Extracted Y-inverted halo texture");
    for (const auto dims : {std::array<int, 2>{0, 16}, {16, -16}, {17, 16}, {65536, 65536}})
        rejects([&] { HeightDomain invalid(dims[0], dims[1]); });
    rejects([] { LandLayout invalid(HeightDomain(14336, 16)); });
    rejects([&] { layout.patch(-1, 0); });
    rejects([&] { layout.patch(40, 0); });
    rejects([&] { domain.texture_window(0, 3); });
    rejects([&] { HeightField invalid(tiny, std::span<const std::uint16_t>{}); });
    for (const float invalid : {std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()}) {
        rejects([&] { field.sample_a8_linear(invalid, 0); });
        rejects([&] { p.displacement_uv(0, invalid); });
        rejects([&] { p.height_window().source_to_uv(invalid, 0); });
    }
}
} // namespace
int main() {
    try { test(); std::cout << "PASS: " << checks << " addressing checks\n"; return 0; }
    catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
