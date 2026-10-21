#include "civ5/terrain/terrain_decal_assets.hpp"
#include "../assets/attribute_xml.hpp"
#include <cmath>

namespace civ5::terrain {
namespace {
constexpr std::size_t record_limit = 4096;
void require(bool condition, const char *message) {
    if (!condition) throw std::invalid_argument(message);
}
void resource_name(const std::string &name) {
    require(!name.empty() && name.size() <= 1024 && assets::AssetKey(name).safe_relative_path(),
            "Invalid terrain decal texture name");
}
float priority(std::string_view value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    require(first != value.npos, "Empty terrain decal priority");
    value = value.substr(first, value.find_last_not_of(" \t\r\n") - first + 1);
    if (value.starts_with('+')) {
        value.remove_prefix(1);
        require(!value.empty() && value.front() != '+' && value.front() != '-',
                "Invalid terrain decal priority sign");
    }
    float result = 0;
    const auto parsed = std::from_chars(value.data(), value.data()+value.size(), result);
    require(!value.empty() && parsed.ec == std::errc{} && parsed.ptr == value.data()+value.size() &&
            std::isfinite(result), "Invalid terrain decal priority");
    return result;
}
void validate(const TerrainDecalDefinition &definition) {
    std::size_t count = 0;
    for (const auto &family : definition.families) {
        if (!family) continue;
        require(!family->empty() && family->size() <= record_limit-count,
                "Invalid terrain decal family size");
        count += family->size();
        for (const auto &row : *family) {
            require(std::isfinite(row.priority), "Nonfinite terrain decal priority");
            for (const auto *name : {&row.albedo, &row.heightmap, &row.specular})
                if (*name) resource_name(**name);
        }
    }
}
} // namespace
TerrainDecalDefinition parse_terrain_decal_definition(std::string_view xml) {
    const auto root = assets::detail::AttributeXml(xml).document();
    require(root.name == "DecalTypes", "Wrong terrain decal XML root");
    TerrainDecalDefinition result;
    for (const auto &node : root.children) {
        const auto family = std::find(terrain_decal_families.begin(), terrain_decal_families.end(), node.name);
        if (family == terrain_decal_families.end()) continue;
        auto &rows = result.families[static_cast<std::size_t>(family-terrain_decal_families.begin())];
        require(!rows, "Duplicate terrain decal family");
        rows.emplace();
        for (const auto &child : node.children) {
            require(child.children.empty(), "Nested terrain decal record");
            const auto name = [&](const char *key) -> std::optional<std::string> {
                const auto it = child.attributes.find(key);
                if (it == child.attributes.end()) return {};
                resource_name(it->second);
                return it->second;
            };
            const auto p = child.attributes.find("priority");
            rows->push_back({p == child.attributes.end() ? 0.1f : priority(p->second),
                             name("albedo"), name("heightmap"), name("spec")});
        }
    }
    validate(result);
    return result;
}
TerrainDecalPalette append_terrain_decal_definition(const TerrainDecalPalette &prior,
    const TerrainDecalDefinition &definition, std::uint32_t blank_specular,
    const TerrainDecalResource &resolve) {
    validate(definition);
    require(bool(resolve) && prior.records.size() <= record_limit, "Invalid terrain decal palette input");
    std::size_t count = prior.records.size();
    for (const auto &r : prior.ranges)
        require(r.first <= r.end && r.end <= count, "Invalid prior terrain decal range");
    for (const auto &f : definition.families) {
        if (!f) continue;
        require(f->size() <= record_limit-count, "Terrain decal palette exceeds record limit");
        count += f->size();
    }
    auto result = prior;
    result.records.reserve(count);
    for (std::size_t i = 0; i < definition.families.size(); ++i) {
        const auto &family = definition.families[i];
        if (!family) continue;
        auto &range = result.ranges[i];
        range.first = static_cast<std::uint32_t>(result.records.size());
        for (const auto &row : *family) {
            const auto albedo = row.albedo ? resolve(*row.albedo) : 0;
            const auto height = row.heightmap ? resolve(*row.heightmap) : 0;
            const auto specular = row.specular ? resolve(*row.specular) : blank_specular;
            result.records.push_back({row.priority, albedo, height, specular});
        }
        range.end = static_cast<std::uint32_t>(result.records.size());
    }
    return result;
}
TerrainDecalAssets TerrainDecalAssets::load(assets::AssetHandle definition, const assets::AssetSource &source) {
    const auto raw = definition.read_bytes(assets::detail::attribute_xml_text_limit);
    const auto document = parse_terrain_decal_definition(
        {reinterpret_cast<const char *>(raw.data()), raw.size()});
    TerrainDecalAssets result(std::move(definition));
    std::unordered_map<std::string, std::uint32_t> resources;
    const auto resource = [&](std::string_view name) -> std::uint32_t {
        const assets::AssetKey key{std::string(name)};
        require(key.safe_relative_path(), "Unsafe terrain decal resource key");
        if (const auto it = resources.find(key.normalized_name()); it != resources.end()) return it->second;
        auto found = source.find(key);
        auto *asset = std::get_if<assets::AssetHandle>(&found);
        require(asset != nullptr, "Terrain decal texture is missing or ambiguous");
        auto data = assets::DdsData::parse(asset->read_bytes(64*1024*1024),
            assets::NumericDdsProfile::legacy_d3d9, assets::DdsHeaderProfile::civ5_d3dx_size24);
        result.textures_.push_back({std::move(*asset), std::move(data)});
        const auto id = static_cast<std::uint32_t>(result.textures_.size());
        resources.emplace(key.normalized_name(), id);
        return id;
    };
    result.blank_specular_ = resource("blank_texture_spec_decal.dds");
    result.palette_ = append_terrain_decal_definition({}, document, result.blank_specular_, resource);
    return result;
}
} // namespace civ5::terrain
