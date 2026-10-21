#include "civ5/materials/material_assets.hpp"
#include "civ5/materials/material_setup.hpp"
#include <charconv>
#include <cmath>
#include <stdexcept>
#include <type_traits>

namespace civ5::materials {
namespace {
std::string_view trim(std::string_view s) {
    constexpr auto whitespace = " \t\r\n\f\v";
    const auto first = s.find_first_not_of(whitespace);
    if (first == s.npos)
        return {};
    return s.substr(first, s.find_last_not_of(whitespace) - first + 1);
}
template <class T> T number(std::string_view value, int base = 10) {
    // Kotlin's bounded authored subset accepts a leading plus; from_chars does
    // not.
    if (!value.empty() && value.front() == '+') {
        value.remove_prefix(1);
        if (!value.empty() && (value.front() == '+' || value.front() == '-'))
            throw std::invalid_argument("Invalid TerrainDef numeric sign");
    }
    if (value.empty())
        throw std::invalid_argument("Empty TerrainDef numeric field");
    T result{};
    std::from_chars_result parsed;
    if constexpr (std::is_floating_point_v<T>)
        parsed = std::from_chars(value.data(), value.data() + value.size(), result);
    else
        parsed = std::from_chars(value.data(), value.data() + value.size(), result, base);
    if (parsed.ec != std::errc{} || parsed.ptr != value.data() + value.size())
        throw std::invalid_argument("Invalid TerrainDef numeric field");
    if constexpr (std::is_floating_point_v<T>)
        if (!std::isfinite(result))
            throw std::invalid_argument("Nonfinite TerrainDef numeric field");
    return result;
}
std::vector<std::int32_t> projections(std::span<const MaterialRow> rows) {
    std::vector<std::int32_t> result;
    for (const auto &row : rows)
        result.push_back(row.projection);
    return result;
}
} // namespace
std::vector<MaterialRow> parse_material_rows(std::string_view csv) {
    std::vector<MaterialRow> rows;
    bool header = true;
    while (!csv.empty()) {
        const auto end = csv.find_first_of("\r\n");
        const auto line = trim(csv.substr(0, end));
        if (end == csv.npos)
            csv = {};
        else {
            const auto skip = csv[end] == '\r' && end + 1 < csv.size() && csv[end + 1] == '\n' ? 2 : 1;
            csv.remove_prefix(end + skip);
        }
        if (header) {
            header = false;
            continue;
        }
        if (line.empty())
            continue;
        if (rows.size() == 64 || line.find('"') != line.npos || line.find('\0') != line.npos)
            throw std::invalid_argument("Unsupported TerrainDef row");
        std::array<std::string_view, 8> fields{};
        auto remaining = line;
        for (std::size_t i = 0; i < fields.size(); ++i) {
            const auto comma = remaining.find(',');
            if ((i == 7) != (comma == remaining.npos))
                throw std::invalid_argument("TerrainDef requires exactly eight columns");
            fields[i] = trim(remaining.substr(0, comma));
            if (i < 7)
                remaining.remove_prefix(comma + 1);
        }
        auto name = [&](std::size_t i) -> std::optional<std::string> {
            if (fields[i] == "NULL")
                return std::nullopt;
            if (fields[i].empty())
                throw std::invalid_argument("Empty TerrainDef texture name");
            return std::string(fields[i]);
        };
        auto id = fields[7];
        if (id.starts_with("0x") || id.starts_with("0X"))
            id.remove_prefix(2);
        rows.push_back({number<std::int32_t>(fields[0]), name(1), name(2), name(3), number<float>(fields[4]),
                        number<float>(fields[5]), number<std::int32_t>(fields[6]),
                        number<std::uint32_t>(id, 16)});
    }
    if (rows.empty())
        throw std::invalid_argument("TerrainDef requires 1..64 rows");
    return rows;
}
MaterialAssets::MaterialAssets(assets::AssetHandle definition, std::vector<MaterialRow> rows)
    : definition_(std::move(definition)), rows_(std::move(rows)), expansion_(projections(rows_)) {}
MaterialAssets MaterialAssets::load(assets::AssetHandle definition, const assets::AssetSource &textures) {
    return load_rows(std::move(definition), textures, false, nullptr);
}
MaterialAssets MaterialAssets::load_terrain_provider(assets::AssetHandle definition,
                                                     const assets::AssetSource &textures,
                                                     const assets::AssetSource *later_textures) {
    return load_rows(std::move(definition), textures, true, later_textures);
}
MaterialAssets MaterialAssets::load_rows(assets::AssetHandle definition, const assets::AssetSource &textures,
                                         bool terrain_provider, const assets::AssetSource *later_textures) {
    const auto csv = definition.read_bytes();
    auto rows = parse_material_rows(std::string_view(reinterpret_cast<const char *>(csv.data()), csv.size()));
    if (terrain_provider)
        rows = terrain_provider_rows(rows);
    MaterialAssets result(std::move(definition), std::move(rows));
    for (const auto &row : result.rows_) {
        auto resource = [&](const std::optional<std::string> &name) -> std::uint32_t {
            if (!name)
                return 0;
            const assets::AssetKey key(*name);
            if (const auto found = result.resource_ids_.find(key.normalized_name());
                found != result.resource_ids_.end())
                return found->second;
            auto lookup = later_textures ? later_textures->find(key) : assets::AssetLookup{};
            if (std::holds_alternative<std::monostate>(lookup))
                lookup = textures.find(key);
            auto *asset = std::get_if<assets::AssetHandle>(&lookup);
            if (!asset)
                throw std::runtime_error("Unresolved exact material asset: " + *name);
            auto data = assets::DdsData::parse(asset->read_bytes(), assets::NumericDdsProfile::legacy_d3d9,
                                               assets::DdsHeaderProfile::civ5_d3dx_size24);
            if (data.face_count() != 1)
                throw std::invalid_argument("Ordinary material source must be a 2D texture");
            result.textures_.push_back({*asset, std::move(data)});
            const auto id = static_cast<std::uint32_t>(result.textures_.size());
            result.resource_ids_.emplace(key.normalized_name(), id);
            return id;
        };
        result.diffuse_ids_.push_back(resource(row.diffuse));
        result.height_ids_.push_back(resource(row.height));
        result.specular_ids_.push_back(resource(row.specular));
    }
    return result;
}
const MaterialTexture &MaterialAssets::texture(const std::string &name) const {
    const auto id = resource_ids_.at(assets::normalize_asset_name(name));
    return textures_.at(id - 1);
}
std::array<std::uint16_t, 16>
MaterialAssets::selected_resources(std::span<const std::int32_t> selected) const {
    return expansion_.ordinary_slots(selected, diffuse_ids_, height_ids_);
}
void MaterialAssets::write_constants(float global_span, std::span<float, 384> destination) const {
    std::vector<float> scales;
    for (const auto &row : rows_)
        scales.push_back(row.scale);
    write_material_constants(expansion_.source_rows(), expansion_.source_modes(), scales, global_span,
                             destination);
}
} // namespace civ5::materials
