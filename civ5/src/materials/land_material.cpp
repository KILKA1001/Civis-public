#include "civ5/materials/land_material.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace civ5::materials {
namespace {
void require(bool ok, const char *why) {
    if (!ok)
        throw std::invalid_argument(why);
}
} // namespace
LandTechnique land_technique(unsigned selector) {
    require(selector < 5, "Unknown native LAND selector");
    const auto input = selector < 3 ? LandVertexInput::local_grid : LandVertexInput::source_positioned;
    const auto pixel = selector == 0                    ? LandPixelProgram::cached_specular
                       : selector == 1 || selector == 3 ? LandPixelProgram::cached_diffuse
                                                        : LandPixelProgram::overlay;
    return {selector, input, pixel};
}
std::optional<LandSelection> select_land_technique(LandPass pass, int displacement, int extra, int state,
                                                   int override_spec) {
    require(pass == LandPass::main || pass == LandPass::overlay, "Invalid LAND pass");
    if (state == 0 || state == 1 || state == 4)
        return std::nullopt;
    const bool displaced = displacement != 0, additional = displaced && extra != 0;
    const unsigned packet = !additional                 ? (state == 3 ? 0x154 : 0x158)
                            : pass == LandPass::overlay ? 0x164
                            : state == 3                ? 0x15c
                                                        : 0x160;
    const unsigned selector = pass == LandPass::main ? (override_spec != 0 ? 0
                                                        : displaced        ? 1
                                                                           : 3)
                              : displaced            ? 2
                                                     : 4;
    return LandSelection{land_technique(selector), packet};
}
LandMaterialMesh::LandMaterialMesh(LandVertexInput input, std::span<const terrain::LandGridVertex> vertices,
                                   std::span<const std::uint16_t> indices)
    : input_(input) {
    require(input == LandVertexInput::local_grid || input == LandVertexInput::source_positioned,
            "Invalid LAND position convention");
    require(!vertices.empty() && vertices.size() <= 65535 && !indices.empty() && indices.size() % 3 == 0 &&
                indices.size() <= 6 * 65535,
            "Invalid LAND material mesh extent");
    require(std::all_of(indices.begin(), indices.end(), [&](auto i) { return i < vertices.size(); }),
            "LAND material index outside vertices");
    for (const auto &v : vertices) {
        require(std::all_of(v.position.begin(), v.position.end(), [](float x) { return std::isfinite(x); }),
                "Nonfinite LAND position");
        for (auto uv : v.uv)
            require((uv & 0x7c00) != 0x7c00, "Nonfinite LAND UV");
        // Original selected PS do not consume the vertex NORMAL output.
    }
    vertices_.assign(vertices.begin(), vertices.end());
    indices_.assign(indices.begin(), indices.end());
}
void LandMaterialConstants::validate() const {
    const auto finite = [](const LandVector &v) {
        return std::all_of(v.begin(), v.end(), [](float x) { return std::isfinite(x); });
    };
    for (const auto *m : {&view_projection, &composite_projection, &world, &shadow_projection})
        for (const auto &v : *m)
            require(finite(v), "Nonfinite LAND matrix");
    for (const auto *v :
         {&fow_scale, &world_scales, &height_texture_scale, &offset, &camera, &fog_color, &fog})
        require(finite(*v), "Nonfinite LAND vector");
    require(std::isfinite(shadow_fade) && std::isfinite(apply_specular) && std::isfinite(apply_diffuse),
            "Nonfinite LAND control");
}
} // namespace civ5::materials
