#include "civ5/materials/lighting_sources.hpp"
#include <algorithm>
#include <stdexcept>

namespace civ5::materials {
const std::array<LightingSource, 11> &lighting_requests() {
    static const std::array<LightingSource, 11> result = [] {
        constexpr auto root = "assets\\terrain\\lighting\\";
        std::array<LightingSource, 11> sources;
        sources[0] = {std::string(root) + "specscreenmap.dds", LightingLoader::texture_data};
        const std::array<std::string, 5> styles{"Generic", "America", "Asia", "Africa", "Europe"};
        for (unsigned i = 0; i < styles.size(); ++i) {
            sources[1 + i * 2] = {std::string(root) + "Sunlight_" + styles[i] + "_Terrain.dds",
                                  LightingLoader::cube};
            sources[2 + i * 2] = {std::string(root) + "Shadowlight_" + styles[i] + "_Terrain" +
                                      (i == 1 || i == 2 || i == 4 ? "_2" : "") + ".dds",
                                  LightingLoader::cube};
        }
        return sources;
    }();
    return result;
}
std::array<std::uint32_t, 10> LightingHandles::renderer_words() const {
    std::array<std::uint32_t, 10> words;
    std::copy(sunlight.begin(), sunlight.end(), words.begin());
    std::copy(shadowlight.begin(), shadowlight.end(), words.begin() + 5);
    return words;
}
LightingHandles resolve_lighting_handles(const std::function<std::uint32_t(const LightingSource &)> &load) {
    if (!load)
        throw std::invalid_argument("Lighting resolver is required");
    const auto &requests = lighting_requests();
    LightingHandles result{};
    result.spec_screen = load(requests[0]);
    for (unsigned i = 0; i < 5; ++i) {
        result.sunlight[i] = load(requests[1 + i * 2]);
        result.shadowlight[i] = load(requests[2 + i * 2]);
    }
    return result;
}
LightingCubeAssets
LightingCubeAssets::load(const std::function<assets::AssetHandle(const LightingSource &)> &resolve) {
    if (!resolve)
        throw std::invalid_argument("Lighting cube resolver is required");
    LightingCubeAssets result;
    for (const auto &request : lighting_requests()) {
        if (request.loader != LightingLoader::cube)
            continue;
        auto asset = resolve(request);
        auto data = assets::DdsData::parse(asset.read_bytes());
        if (data.face_count() != 6 ||
            (data.format() != assets::DdsFormat::dxt1 && data.format() != assets::DdsFormat::dxt3 &&
             data.format() != assets::DdsFormat::dxt5))
            throw std::invalid_argument("Lighting cube requires complete authored BC1/BC2/BC3 faces");
        result.cubes_.push_back({request, std::move(asset), std::move(data)});
    }
    return result;
}
const LightingCubeAsset &LightingCubeAssets::sunlight(unsigned style) const {
    if (style >= 5)
        throw std::out_of_range("Lighting style must be 0..4");
    return cubes_.at(style * 2);
}
const LightingCubeAsset &LightingCubeAssets::shadowlight(unsigned style) const {
    if (style >= 5)
        throw std::out_of_range("Lighting style must be 0..4");
    return cubes_.at(style * 2 + 1);
}
unsigned material_lighting_style(std::uint32_t mask) {
    for (unsigned style = 1; style <= 4; ++style)
        if (mask & (1u << style))
            return style;
    return 0;
}
MaterialLightingBindings material_lighting_bindings(const MaterialLightingResources &r) {
    const auto style = material_lighting_style(r.mask);
    const std::array<std::uint32_t, 12> textures{r.normal,
                                                 r.albedo,
                                                 r.shadow,
                                                 r.specular,
                                                 r.sunlight[style],
                                                 r.shadowlight[style],
                                                 0,
                                                 0,
                                                 r.color_correct[style],
                                                 r.global_texture9,
                                                 r.land_color_key[style],
                                                 r.parent_texture11};
    MaterialLightingBindings result{
        style, {}, {5, 5, 5, 5, 2, 2, 0, 0, 5, 5, 5, 0}, static_cast<std::uint16_t>(r.world_height), 1};
    for (unsigned i = 0; i < textures.size(); ++i)
        result.pixel_textures[i] = static_cast<std::uint16_t>(textures[i]);
    return result;
}
} // namespace civ5::materials
