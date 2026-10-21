#include "civ5/materials/land_material.hpp"
#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace civ5::materials;
namespace {
unsigned checks = 0;
void check(bool ok) {
    ++checks;
    if (!ok)
        throw std::runtime_error("LAND material contract");
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
void run() {
    static_assert(sizeof(void *) == 8, "Only 64-bit runtime is supported");
    for (unsigned selector = 0; selector < 5; ++selector) {
        const auto t = land_technique(selector);
        check(t.selector == selector);
        check(t.input == (selector < 3 ? LandVertexInput::local_grid : LandVertexInput::source_positioned));
        check(t.pixel == (selector == 0                    ? LandPixelProgram::cached_specular
                          : selector == 1 || selector == 3 ? LandPixelProgram::cached_diffuse
                                                           : LandPixelProgram::overlay));
    }
    for (unsigned invalid : {5u, 6u, 0xffffffffu})
        rejects([&] { land_technique(invalid); });
    const std::array<int, 7> states{std::numeric_limits<int>::min(), -1, 0, 1, 2, 3, 4};
    for (auto pass : {LandPass::main, LandPass::overlay})
        for (int mode : {0, 1, -1, std::numeric_limits<int>::min()})
            for (int extra : {0, 1, -1})
                for (int state : states)
                    for (int spec : {0, 1, -1}) {
                        const auto selected = select_land_technique(pass, mode, extra, state, spec);
                        const bool draw = state != 0 && state != 1 && state != 4;
                        check(selected.has_value() == draw);
                        if (!draw)
                            continue;
                        check(selected->technique.selector == (pass == LandPass::overlay ? (mode ? 2u : 4u)
                                                               : spec                    ? 0u
                                                               : mode                    ? 1u
                                                                                         : 3u));
                        const unsigned packet = !mode || !extra             ? (state == 3 ? 0x154 : 0x158)
                                                : pass == LandPass::overlay ? 0x164
                                                : state == 3                ? 0x15c
                                                                            : 0x160;
                        check(selected->packet_offset == packet);
                    }
    rejects([] { select_land_technique(static_cast<LandPass>(-1), 0, 0, 0, 0); });
    const civ5::terrain::LandGrid grid(3, 3, 1);
    std::vector<civ5::terrain::LandGridVertex> vertices(grid.vertices().begin(), grid.vertices().end());
    std::vector<std::uint16_t> indices(grid.indices().begin(), grid.indices().end());
    const LandMaterialMesh owned(LandVertexInput::local_grid, vertices, indices);
    vertices[0].position[0] = 17;
    indices[0] = 7;
    check(owned.vertices()[0].position[0] == 0 && owned.indices()[0] == grid.indices()[0]);
    for (auto kind : {LandVertexInput::local_grid, LandVertexInput::source_positioned}) {
        rejects([&] { LandMaterialMesh bad(kind, {}, indices); });
        rejects([&] { LandMaterialMesh bad(kind, vertices, {}); });
        rejects([&] { LandMaterialMesh bad(kind, vertices, std::span(indices).first(2)); });
        auto outside = indices;
        outside[0] = static_cast<std::uint16_t>(vertices.size());
        rejects([&] { LandMaterialMesh bad(kind, vertices, outside); });
        for (float nonfinite :
             {std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(),
              std::numeric_limits<float>::quiet_NaN()})
            for (unsigned lane = 0; lane < 3; ++lane) {
                auto bad = vertices;
                bad[0].position[lane] = nonfinite;
                rejects([&] { LandMaterialMesh rejected(kind, bad, indices); });
            }
        for (unsigned word = 0; word < 65536; ++word) {
            auto copy = vertices;
            copy[1].uv[0] = static_cast<std::uint16_t>(word);
            if ((word & 0x7c00) == 0x7c00)
                rejects([&] { LandMaterialMesh bad(kind, copy, indices); });
            else {
                const LandMaterialMesh accepted(kind, copy, indices);
                check(accepted.vertices()[1].uv[0] == word);
            }
        }
        auto opaque = vertices;
        for (auto &v : opaque)
            v.normal.fill(0xffff);
        const LandMaterialMesh unused_normal(kind, opaque, indices);
        check(unused_normal.vertices()[0].normal == opaque[0].normal);
    }
    rejects([&] { LandMaterialMesh bad(static_cast<LandVertexInput>(-1), vertices, indices); });
    const LandMaterialConstants c;
    c.validate();
    ++checks;
    for (float bad : {std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity(),
                      -std::numeric_limits<float>::infinity()}) {
        for (auto member :
             {&LandMaterialConstants::view_projection, &LandMaterialConstants::composite_projection,
              &LandMaterialConstants::world, &LandMaterialConstants::shadow_projection})
            for (unsigned row = 0; row < 4; ++row)
                for (unsigned lane = 0; lane < 4; ++lane) {
                    auto modified = c;
                    (modified.*member)[row][lane] = bad;
                    rejects([&] { modified.validate(); });
                }
        for (auto member :
             {&LandMaterialConstants::fow_scale, &LandMaterialConstants::world_scales,
              &LandMaterialConstants::height_texture_scale, &LandMaterialConstants::offset,
              &LandMaterialConstants::camera, &LandMaterialConstants::fog_color, &LandMaterialConstants::fog})
            for (unsigned lane = 0; lane < 4; ++lane) {
                auto modified = c;
                (modified.*member)[lane] = bad;
                rejects([&] { modified.validate(); });
            }
        for (auto member : {&LandMaterialConstants::shadow_fade, &LandMaterialConstants::apply_specular,
                            &LandMaterialConstants::apply_diffuse}) {
            auto modified = c;
            modified.*member = bad;
            rejects([&] { modified.validate(); });
        }
    }
    for (float value : {-100.f, -0.f, 0.f, 0.5f, 100.f}) {
        auto controls = c;
        controls.shadow_fade = controls.apply_specular = controls.apply_diffuse = value;
        controls.validate();
        ++checks;
    }
}
} // namespace
int main() {
    try {
        run();
        std::cout << "PASS LAND material checks=" << checks << '\n';
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
