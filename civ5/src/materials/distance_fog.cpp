#include "civ5/materials/distance_fog.hpp"
#include <algorithm>
#include <bit>
#include <charconv>
#include <cmath>
#include <stdexcept>
#include <string_view>

namespace civ5::materials {
namespace {
std::array<std::string, 3> color_cells(std::span<const std::uint8_t> csv) {
    if (csv.empty() || csv.size() > 65536 || std::find(csv.begin(), csv.end(), 0) != csv.end())
        throw std::invalid_argument("DistanceFog requires bounded nonempty CSV text");
    for (std::size_t i = 0; i < csv.size(); ++i)
        if (csv[i] == '\r' && (i + 1 == csv.size() || csv[i + 1] != '\n'))
            throw std::invalid_argument("DistanceFog CSV requires LF or CRLF line endings");
    // Original 83F2E0 normalizes CR in its first pass and replaces quotes and
    // delimiters with NUL in the second. An opening quote changes the cell's
    // start offset; quotes are not an RFC4180 escape sequence in this reader.
    std::string text(csv.begin(), csv.end());
    std::replace(text.begin(), text.end(), '\r', '\n');
    std::array<std::string, 3> cells;
    std::size_t start = 0, column = 0;
    bool quoted = false;
    for (std::size_t i = 0; i <= text.size(); ++i) {
        const auto c = i == text.size() ? '\n' : text[i];
        if (c == '"') {
            text[i] = 0;
            if (!quoted)
                start = i + 1;
            quoted = !quoted;
        } else if ((c == ',' || c == '\n') && !quoted) {
            cells[column] = text.substr(start, i - start);
            if (const auto nul = cells[column].find('\0'); nul != std::string::npos)
                cells[column].resize(nul);
            if (++column == 3)
                return cells;
            if (c == '\n')
                break;
            start = i + 1;
        }
    }
    throw std::invalid_argument("DistanceFog CSV must contain three cells in row zero");
}
float cell_float(const std::string &cell) {
    if (cell.empty())
        throw std::invalid_argument("DistanceFog color cell is empty");
    std::string_view value(cell);
    while (!value.empty() && (value.front() == ' ' || (value.front() >= '\t' && value.front() <= '\r')))
        value.remove_prefix(1);
    if (!value.empty() && value.front() == '+') {
        value.remove_prefix(1);
        if (!value.empty() && value.front() == '-')
            return 0;
    }
    double parsed = 0;
    const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
    // atof accepts a decimal prefix and returns zero for nonnumeric text.
    if (result.ec == std::errc::invalid_argument)
        parsed = 0;
    else if (result.ec != std::errc{})
        throw std::invalid_argument("DistanceFog color cell is outside finite range");
    const float stored = static_cast<float>(parsed);
    if (!std::isfinite(stored))
        throw std::invalid_argument("DistanceFog color must be finite");
    return stored;
}
} // namespace
DistanceFogState distance_fog_startup(std::span<const std::uint8_t> csv) {
    const auto cells = color_cells(csv);
    DistanceFogState result{
        true, true, std::bit_cast<float>(0x3d4ccccdu), std::bit_cast<float>(0x3851b717u), {}};
    const auto scale = std::bit_cast<float>(0x3b808081u);
    for (unsigned i = 0; i < 3; ++i)
        result.color[i] = cell_float(cells[i]) * scale;
    return result;
}
DistanceFogFrame distance_fog_frame(const DistanceFogState &state, float normalized_zoom,
                                    const DistanceFogFrame &prior) {
    if (!state.loaded)
        return prior;
    if (!std::isfinite(state.density) || state.density <= 0 || !std::isfinite(state.strength) ||
        !std::isfinite(normalized_zoom) ||
        !std::all_of(state.color.begin(), state.color.end(), [](float v) { return std::isfinite(v); }))
        throw std::invalid_argument("DistanceFog frame requires finite inputs and positive density");
    return {{state.color[0], state.color[1], state.color[2], 1},
            {state.density, state.enabled ? state.strength : 0, 0, normalized_zoom}};
}
DistanceFogAssets::DistanceFogAssets(assets::AssetHandle color, assets::AssetHandle ramp,
                                     DistanceFogState state, assets::DdsData data, assets::Bytes color_bytes)
    : color_asset_(std::move(color)), ramp_asset_(std::move(ramp)), state_(state), ramp_(std::move(data)),
      color_bytes_(std::move(color_bytes)) {}
DistanceFogAssets DistanceFogAssets::load(const std::function<assets::AssetHandle(const char *)> &resolve) {
    if (!resolve)
        throw std::invalid_argument("DistanceFog asset resolver is required");
    auto color = resolve(color_path);
    auto color_bytes = color.read_bytes(65536);
    const auto state = distance_fog_startup(color_bytes);
    auto ramp = resolve(ramp_path);
    auto data = assets::DdsData::parse(ramp.read_bytes());
    if (data.face_count() != 1)
        throw std::invalid_argument("DistanceFog ramp must be a two-dimensional texture");
    return DistanceFogAssets(std::move(color), std::move(ramp), state, std::move(data),
                             std::move(color_bytes));
}
} // namespace civ5::materials
