#include "civ5/terrain/height_owner.hpp"
#include "civ5/terrain/land_membership.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace civ5::terrain;
using Bytes = std::vector<std::uint8_t>;
namespace {
int checks = 0;
void check(bool ok, const char *label) {
    ++checks;
    if (!ok)
        throw std::runtime_error(label);
}
template <class E = std::invalid_argument, class F> void rejects(F f) {
    bool caught = false;
    try {
        f();
    } catch (const E &) {
        caught = true;
    }
    check(caught, "Expected explicit rejection");
}
IntMatrix4 identity_int() {
    IntMatrix4 m{};
    for (int i = 0; i < 16; i += 5)
        m[i] = 1;
    return m;
}
auto filled(std::uint8_t byte) {
    return [=](std::int32_t size) { return Bytes(size, byte); };
}
void math_membership() {
    const auto int_min = std::numeric_limits<std::int32_t>::min();
    check(wrap_neg(int_min) == int_min && wrap_mul(int_min, 2) == 0 && wrap_add(int_min, -1) == 2147483647,
          "x86 integer wrap without UB");
    const auto m = unshifted_global_map_matrix({});
    check(m[0] == 0 && m[1] == -1 && m[15] == 1, "Literal cube basis");
    rejects([] { unshifted_global_map_matrix({0, 0, 0, 1, 0}); });
    check(unsigned_wrapped_candidate(-1, 7) == 6 && unsigned_wrapped_candidate(-8, 7) == 3 &&
              unsigned_wrapped_candidate(8, 7) == 1 && unsigned_wrapped_candidate(-8, 8) == 0,
          "One-period unsigned DIV, not floor-mod");
    rejects([] { unsigned_wrapped_candidate(0, 0); });
    const std::array<float, 6> bounds{0, 0, 0, 1, 1, 1};
    check(edge_intersects(bounds, {-1, .5f}, {2, .5f}) && edge_intersects(bounds, {-1, 0}, {0, 0}),
          "Inclusive edge crossing/endpoint");
    check(!edge_intersects(bounds, {-1, 2}, {2, 2}), "Outside edge");
    auto world = identity_matrix();
    world[12] = 32;
    world[13] = 32;
    const LandLayout layout(HeightDomain(64, 64));
    auto result = build_land_membership(layout, 1, 1, identity_int(), world, {32, 32 * .866f},
                                        X87Precision::double_precision, MatrixPath::sse);
    check(result.hexes.size() == 1 && result.patches.size() == 1 && result.patches[0].size() == 9,
          "Repeated candidates retained");
    for (const auto &h : result.patches[0])
        check(h == LogicalHex{0, 0, 0}, "Repeated logical member");
    for (const auto &c : result.hexes[0].candidates)
        check(c.patch_index == 0 && c.intersects, "Repeated hit");
    auto empty = build_land_membership(layout, 0, 1, identity_int(), world, {32, 32 * .866f},
                                       X87Precision::single, MatrixPath::reference_x87);
    check(empty.hexes.empty() && empty.patches.size() == 1 && empty.patches[0].empty(),
          "Empty traversal keeps patches");
    rejects([&] {
        build_land_membership(layout, 2, 1, identity_int(), world, {32, 32 * .866f},
                              X87Precision::double_precision, MatrixPath::sse);
    });
    rejects([&] {
        build_land_membership(layout, 256, 1, identity_int(), world, {0, 0}, X87Precision::double_precision,
                              MatrixPath::sse);
    });
    Matrix4 a, b;
    a.fill(1);
    for (int row = 0; row < 4; ++row)
        for (int c = 0; c < 4; ++c)
            b[row * 4 + c] = std::array{16777216.0f, 1.0f, -16777216.0f, 1.0f}[row];
    const auto sse = matrix_multiply(a, b, X87Precision::single, MatrixPath::sse);
    const auto dispatch = matrix_multiply(a, b, X87Precision::single, MatrixPath::dispatch_x87);
    check(sse[0] == 1 && sse[4] == 1 && dispatch[12] == 2, "Matrix reduction paths separately rounded");
    rejects([] { inverse_sse(identity_matrix(), {}); });
    rejects([] { inverse_sse(Matrix4{}, [](float) { return 1.0f; }); });
    auto nan = identity_matrix();
    nan[0] = std::numeric_limits<float>::quiet_NaN();
    rejects([&] { inverse_sse(nan, [](float) { return 1.0f; }); });
    rejects([] { build_world_layout(INT32_MAX / 64, 1, 0, {}, [](float) { return 1.0f; }); });
    rejects([] { build_world_layout(1, 1, 3, {}, [](float) { return 1.0f; }); });
    int calls = 0;
    auto native = build_world_layout(40, 24, 1, {}, [&](float d) {
        ++calls;
        check(std::bit_cast<std::uint32_t>(d) == 0x397fffffu, "Original determinant bits");
        return std::bit_cast<float>(0x45800001u);
    });
    check(calls == 2 && native.worker_grid.width == 40 && native.worker_grid.height == 28,
          "Worker world dimensions");
    auto hardware =
        build_world_layout(40, 24, 1, {}, [](float) { return std::bit_cast<float>(0x45800800u); });
    check(native.transforms == hardware.transforms, "Explicit measured hardware seed alternative");
    const auto p = HeightOwnerParameters::from_world(native);
    check(p.width == 2560 && p.height == 1792 && p.center_x == 1280 && p.center_y == 775.9359741210938f,
          "World owner parameters");
    native.worker_grid.width = int_min;
    rejects([&] { HeightOwnerParameters::from_world(native); });
}
void styles() {
    Bytes prior(LandStyleGrid::size, 93), cells(2 * 3 * 8, 0xab);
    for (int i = 0; i < 6; ++i)
        cells[i * 8 + 4] = static_cast<std::uint8_t>(i);
    const LandStyleGrid original(prior);
    prior[0] = 255;
    const auto grid = original.copy_source_cells(2, 3, cells);
    const auto bytes = grid.owner_bytes();
    check(bytes[0] == 0 && bytes[1] == 3 && bytes[2] == 93 && bytes[255] == 1 && bytes[256] == 4 &&
              bytes[510] == 2 && bytes[511] == 5 && bytes.back() == 93,
          "X-major byte4 copy and untouched tail");
    check(original.owner_bytes()[0] == 93, "Source grid value ownership");
    auto returned = grid.owner_bytes();
    returned[0] = 99;
    check(grid.owner_bytes()[0] == 0, "Returned buffer not aliased");
    auto smaller = grid.copy_source_cells(1, 1, Bytes(8, 7));
    check(smaller.owner_bytes()[0] == 7 && smaller.owner_bytes()[255] == 1,
          "Small reload preserves previous rows");
    const std::vector<std::vector<LogicalHex>> one{{{0, 0, 0}}};
    for (std::uint32_t style = 0; style < 256; ++style) {
        auto raw = original.owner_bytes();
        raw[0] = static_cast<std::uint8_t>(style);
        auto result = accumulate_land_styles(one, identity_int(), LandStyleGrid(raw), std::array{0u});
        check(result.masks[0] == (1u << (style & 31)), "Style SHL low5 mask");
        check(result.ordinary_styles[0] ==
                  ((style & 31) >= 1 && (style & 31) <= 4 ? static_cast<int>(style & 31) : 0),
              "First ordinary bit1..4");
    }
    Bytes two(LandStyleGrid::size, 2);
    auto duplicates = accumulate_land_styles({{{0, 0, 0}, {0, 0, 0}}}, identity_int(), LandStyleGrid(two),
                                             std::array{0x80000010u});
    check(duplicates.reads.size() == 2 && duplicates.masks[0] == 0x80000014u &&
              duplicates.ordinary_styles[0] == 2,
          "Initial mask and duplicate reads");
    auto alias = accumulate_land_styles({{{-1, 1, 0}}}, identity_int(), LandStyleGrid(two), std::array{0u});
    check(alias.reads[0].flat_address == 254, "Flat-address range, not independent axis clamping");
    rejects<std::out_of_range>(
        [&] { accumulate_land_styles({{{-1, 0, 0}}}, identity_int(), LandStyleGrid(two), std::array{0u}); });
    auto matrix = identity_int();
    matrix[0] = std::numeric_limits<std::int32_t>::min();
    two[0] = 255;
    auto wrapped = accumulate_land_styles({{{2, 0, 0}}}, matrix, LandStyleGrid(two), std::array{0u});
    check(wrapped.reads[0].flat_address == 0 && wrapped.masks[0] == 0x80000000u,
          "Original signed IMUL wraps");
    check(land_traversal_extent(0, 0, 4095, 4095, 1, 1).width == 65, "Traversal uses63, not64");
    check(land_traversal_extent(0, 0, std::nextafter(2520.0f, 0.0f), 2520, 1, 1).width == 39 &&
              land_traversal_extent(0, 0, 2520, 2520, 1, 1).width == 40,
          "Traversal float boundary");
    rejects([] { land_traversal_extent(0, 0, 1, 1, 0, 1); });
    rejects([] { land_traversal_extent(0, 0, 1, 1, std::numeric_limits<float>::denorm_min(), 1); });
    rejects([] { LandStyleGrid invalid(Bytes(1)); });
    rejects([&] { grid.copy_source_cells(256, 1, {}); });
    rejects([&] { accumulate_land_styles(one, identity_int(), grid, {}); });
    rejects([&] {
        accumulate_land_styles({std::vector<LogicalHex>(17, {0, 0, 0})}, identity_int(), grid,
                               std::array{0u});
    });
}
void owner() {
    check(std::bit_cast<std::uint32_t>(post_initialization_scalar(-0.0f, .2f)) == 0x80000000u,
          "Post-init negative zero");
    check(std::bit_cast<std::uint32_t>(post_initialization_scalar(42, .2f)) == 1158012929u,
          "Post-init distinct multiply rounding");
    for (int element : {1, 2}) {
        auto aligned = PageStorage::initialize(64, 48, element, nullptr, filled(0xa7));
        check(aligned.active_pages() == 12 && aligned.capacity_pages() == 12, "Aligned active pages");
        const auto pool = aligned.pool_bytes(), headers = aligned.header_bytes();
        check(std::all_of(pool.begin(), pool.end(), [](auto b) { return b == 0; }), "Active pool zero");
        for (std::size_t i = 0; i < headers.size(); i += 8)
            check(headers[i + 2] == 0xa7 && headers[i + 3] == 0xa7, "Allocator header padding retained");
    }
    auto partial = PageStorage::initialize(17, 33, 2, nullptr, filled(0xa7));
    check(partial.domain().width() == 32 && partial.domain().height() == 48 && partial.active_pages() == 2,
          "Header ceil / clear floor");
    const auto headers = partial.header_bytes();
    for (int i = 0; i < 6; ++i)
        check(headers[i * 8] == std::array{0, 1, 0, 1, 1, 1}[i], "Partial page state");
    auto poisoned = partial.with_pool_bytes(Bytes(partial.pool_bytes().size(), 0x77));
    check(poisoned.value_at(15, 31) == 0x7777 && poisoned.value_at(16, 31) == 0 &&
              poisoned.value_at(0, 32) == 0,
          "Native uniform zero vs pool pages");
    check(partial.value_at(15, 31) == 0, "Prior page state not aliased");
    auto large = PageStorage::initialize(64, 64, 2, nullptr, filled(0xa7));
    large = large.with_pool_bytes(Bytes(large.pool_bytes().size(), 0xe9));
    auto small = PageStorage::initialize(16, 16, 2, &large, filled(0xa7));
    auto raw = small.pool_bytes();
    check(small.active_pages() == 1 && small.capacity_pages() == 16 && raw[511] == 0 && raw[512] == 0xe9 &&
              raw.back() == 0xe9,
          "Shrink retains inactive pool tail");
    check(large.pool_bytes()[0] == 0xe9, "Shrink does not modify previous state");
    auto initial = PageStorage::initialize(16, 16, 2, nullptr, filled(0xa7));
    std::vector<int> allocations;
    auto growth = PageStorage::initialize(32, 32, 2, &initial, [&](int size) {
        allocations.push_back(size);
        return Bytes(size, 0x88);
    });
    check(allocations == std::vector<int>{2048, 32} && growth.pool_bytes().back() == 0,
          "Growth pool-before-header / no old pool copy");
    allocations.clear();
    auto uniform = PageStorage::initialize(15, 31, 1, nullptr, [&](int size) {
        allocations.push_back(size);
        return Bytes(size, 0xab);
    });
    check(allocations == std::vector<int>{16} && uniform.active_pages() == 0 && uniform.value_at(0, 0) == 0,
          "Subpage does not allocate pool");
    auto full = initial.pool_bytes();
    full[0] = 0x34;
    full[1] = 0x12;
    auto field = initial.with_pool_bytes(full).height_field();
    check(field.u16_at(0, 0) == 0x1234 && field.a8_at(0, 0) == 0x12,
          "Preserve full U16 until texture extraction");
    const auto result = initialize_height_owner(HeightOwnerParameters::native_call(17, 33), 1, 1, Bytes(8, 4),
                                                {1, -2, 1}, LandStyleGrid(Bytes(LandStyleGrid::size, 93)),
                                                nullptr, nullptr, filled(0xa7));
    check(result.styles.owner_bytes()[0] == 4 && result.styles.owner_bytes()[1] == 93,
          "Owner logical styles tail");
    check(result.normalized_scale_x() == 31.0f / 17 && result.normalized_scale_y() == 47.0f / 33,
          "Normalized aligned extent");
    check(result.parameters.center_x == 8 && result.parameters.center_y == 16 * .866f,
          "Odd dimensions use integer half");
    check(result.first_matrix[12] == -1 && result.first_matrix[13] == 2 && result.second_matrix[13] == -2 &&
              result.coordinate_descriptor == FactoryDescriptor{1, -2, 1, 0, 0},
          "Owner inverse translations and descriptor");
    rejects([&] { PageStorage::initialize(0, 16, 2, nullptr, filled(0)); });
    rejects([&] { PageStorage::initialize(16, 16, 3, nullptr, filled(0)); });
    rejects([&] { PageStorage::initialize(16, 16, 1, &initial, filled(0)); });
    rejects([&] { PageStorage::initialize(16, 16, 2, nullptr, [](int) { return Bytes{}; }); });
    rejects([&] { initial.with_pool_bytes({}); });
    rejects([&] { uniform.height_field(); });
    rejects<std::out_of_range>([&] { initial.value_at(-1, 0); });
    rejects([&] { PageStorage::initialize(std::numeric_limits<int>::max(), 1, 2, nullptr, filled(0)); });
    rejects([&] { HeightOwnerParameters::native_call(-1, 1); });
}
void populated_pages() {
    const int width = 64, height = 48;
    std::vector<std::uint16_t> heights(width * height);
    Bytes materials(width * height);
    for (int i = 0; i < width * height; ++i) {
        heights[i] = static_cast<std::uint16_t>(i * 919 + 1357);
        materials[i] = static_cast<std::uint8_t>(i * 7 + 19);
    }
    const auto empty_height = PageStorage::initialize(width, height, 2, nullptr, filled(0xa7));
    const auto empty_material = PageStorage::initialize(width, height, 1, nullptr, filled(0xa7));
    auto h = empty_height.with_dense_values(std::span<const std::uint16_t>(heights));
    auto m = empty_material.with_dense_values(std::span<const std::uint8_t>(materials));
    check(h.header_bytes() == empty_height.header_bytes() && m.header_bytes() == empty_material.header_bytes(),
          "Populated pages preserve header padding and page indices");
    const auto pool = h.pool_bytes();
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x) {
            const auto index = y * width + x;
            const auto page = (y / 16) * (width / 16) + x / 16;
            const auto address = (page * 256 + (y % 16) * 16 + x % 16) * 2;
            check(h.value_at(x, y) == heights[index] && m.value_at(x, y) == materials[index],
                  "Computed field populates correct page and texel");
            check(pool[address] == (heights[index] & 255) && pool[address + 1] == (heights[index] >> 8),
                  "U16 page data is little-endian, without height quantization");
        }
    auto shrink = PageStorage::initialize(16, 16, 2, &h, filled(0x93));
    const auto prior = shrink.pool_bytes();
    auto filled_page = shrink.with_dense_values(std::span<const std::uint16_t>(heights).first(256));
    const auto after = filled_page.pool_bytes();
    check(std::equal(prior.begin() + 512, prior.end(), after.begin() + 512),
          "Computed values retain spare page capacity bytes");
    heights.assign(heights.size(), 0); materials.assign(materials.size(), 0);
    check(h.value_at(0, 0) == 1357 && m.value_at(0, 0) == 19 && empty_height.value_at(0, 0) == 0,
          "Published pages own the field and do not change prior owners");
    rejects([&] { h.with_dense_values(std::span<const std::uint16_t>(heights).first(255)); });
    rejects([&] { h.with_dense_values(std::span<const std::uint8_t>(materials)); });
    rejects([&] { m.with_dense_values(std::span<const std::uint16_t>(heights)); });
    const auto partial = PageStorage::initialize(17, 33, 2, nullptr, filled(0));
    std::vector<std::uint16_t> padded(32 * 48);
    rejects([&] { partial.with_dense_values(std::span<const std::uint16_t>(padded)); });
}
} // namespace
int main() {
    try {
        math_membership();
        styles();
        owner();
        populated_pages();
        std::cout << "PASS: " << checks << " world/LAND/owner checks\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
