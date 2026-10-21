#include "civ5/terrain/land_grid.hpp"
#include <algorithm>
#include <bit>
#include <stdexcept>

namespace civ5::terrain {
namespace {
void require(bool ok) {
    if (!ok)
        throw std::invalid_argument("Invalid native LAND grid input");
}
} // namespace
std::uint16_t land_unit_half(float value) {
    require(value >= 0.00006103515625f && value <= 1.0f);
    const auto bits = std::bit_cast<std::uint32_t>(value);
    const auto rounded = bits + 0xfffu + ((bits >> 13) & 1u);
    return static_cast<std::uint16_t>((rounded - 0x38000000u) >> 13);
}
std::vector<std::uint16_t> land_grid_indices(int columns, int rows, int step) {
    require(columns >= 2 && columns <= 67 && rows >= 2 && rows <= 67 && step >= 1 && step <= 3);
    require((columns <= 65 && rows <= 65) || step == 1);
    const auto expected = ((columns - 2 + step) / step) * ((rows - 2 + step) / step) * 6;
    std::vector<std::uint16_t> result;
    result.reserve(expected);
    for (int block = 1; block < columns; block += step * 6)
        for (int y = 1; y < rows; y += step)
            for (int x = block; x < std::min(block + step * 6, columns); x += step) {
                const auto rx = std::min(step - 1, columns - 1 - x), ry = std::min(step - 1, rows - 1 - y);
                const int a = (y - 1) * columns + x - 1, b = (y - 1) * columns + x + rx;
                const int c = (y + ry) * columns + x - 1, d = (y + ry) * columns + x + rx;
                for (int v : {d, b, x % 2 == 0 ? a : c, c, x % 2 == 0 ? d : b, a})
                    result.push_back(static_cast<std::uint16_t>(v));
            }
    if (result.size() != static_cast<std::size_t>(expected))
        throw std::logic_error("LAND index extent");
    return result;
}
LandGrid::LandGrid(int columns, int rows, int step) {
    require(columns >= 2 && columns <= 65 && rows >= 2 && rows <= 65);
    indices_ = land_grid_indices(columns, rows, step);
    vertices_.resize(columns * rows);
    for (int y = 0; y < rows; ++y)
        for (int x = 0; x < columns; ++x) {
            auto &v = vertices_[y * columns + x];
            v.position = {static_cast<float>(x), static_cast<float>(y), 14.0f};
            v.normal = {};
            v.uv = {land_unit_half(static_cast<float>(x + 2) / static_cast<float>(columns + 1)),
                    land_unit_half(static_cast<float>(y + 2) / static_cast<float>(rows + 1))};
        }
}
LandControlStream land_control_stream(int columns, int rows) {
    require(columns >= 2 && columns <= 65 && rows >= 2 && rows <= 65);
    LandControlStream result;
    for (auto &level : result.levels) {
        level = {columns, rows, result.addresses.size(),
                 static_cast<std::size_t>((columns - 1) * (rows - 1) * 16)};
        for (int y = 1; y < rows; ++y)
            for (int x = 1; x < columns; ++x)
                for (int r = 1; r <= 4; ++r)
                    for (int c = 1; c <= 4; ++c)
                        result.addresses.push_back(static_cast<std::uint16_t>(((y + r) << 8) | (x + c)));
        columns = (columns >> 1) + 1;
        rows = (rows >> 1) + 1;
    }
    return result;
}
} // namespace civ5::terrain
