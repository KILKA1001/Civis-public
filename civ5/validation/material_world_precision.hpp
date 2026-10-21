#pragma once
#include "civ5/graphics/gl_material_blend.hpp"
#include "civ5/graphics/gl_material_textures.hpp"
#include "civ5/graphics/gl_material_shadows.hpp"
#include "material_precision_probe.hpp"

// Validation-only subset execution. Indices are requests, never expected
// pixels or material records. Every patch is rebuilt from the primitive world.
void material_world_precision(std::shared_ptr<GlContext> context, const fs::path &root,
                              PrimitiveInput &input, const std::array<fs::path, 4> &shaders,
                              const fs::path &land_shaders, const LightingCubeAssets &cubes,
                              std::span<const std::uint8_t> geometry, const GlMaterialWorldLighting &light,
                              Control &control, const fs::path &requests, const fs::path &probes,
                              const fs::path &out) {
    const auto bytes = input_bytes(requests);
    WorldReader r{bytes};
    const auto magic = r.take(8);
    require(std::string(magic.begin(), magic.end()) == "C5MPAT01", "Material precision request schema");
    const auto count = r.word();
    require(count > 0 && count <= 1120, "Bounded material precision request count");
    std::vector<unsigned> indices;
    for (unsigned i = 0; i < count; ++i) {
        const auto index = r.word();
        require(index < 1120 && std::find(indices.begin(), indices.end(), index) == indices.end(),
                "Unique material precision patch index");
        indices.push_back(index);
    }
    r.end();
    auto world = build_world(root, input, 0);
    GlMaterialWorldRenderer renderer(context, world, cubes, {false, true, true},
        LightingCubeSampling::measured_coordinates, LandShadowArithmetic::measured_fma_t_x_z_y,
        LandMipSampling::d3d9_trilinear);
    const auto field = world->terrain().height_field();
    NativeWorld native(shaders, land_shaders, world->assets(), cubes, 64, geometry,
                       field.extract_a8_texture(0, 0), renderer.source_bindings());
    auto &native_raster = static_cast<cache_raster_validation::NativeCacheRaster &>(native.cache);
    const std::shared_ptr<const MaterialAssets> assets(world, &world->assets());
    GlMaterialTextures textures(context, assets);
    const auto seed = [](int, int, float squared) {
        return _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(squared)));
    };
    save(out / "requests.bin", bytes);
    std::ostringstream execution;
    execution << context->description() << '\n';
    for (const auto index : indices) {
        auto job = renderer.prepare(index, {64, 0, 0, 0, X87Precision::double_precision}, seed, seed,
                                    light, 4, true);
        const auto a8 = field.extract_a8_texture(job.height_table_slot % 16, job.height_table_slot / 16);
        native.execute(index, job, a8);
        const auto folder = out / std::to_string(index);
        fs::create_directory(folder);
        output_patch(job.patch, folder);
        save(folder / "height-window.bin", a8);
        blend_validation::write(folder / "plan.bin", serialize_plan(job.plan));
        blend_validation::write(folder / "coefficients.f32", job.frame.material_coefficients);
        blend_validation::write(folder / "final-gl.bin", renderer.final_texture(index).read_mip_raw(0));
        blend_validation::write(folder / "final-d3d9.bin", native.final_pixels(index));
        for (unsigned slot : {0u, 1u, 3u, 4u}) {
            blend_validation::write(folder / ("temporary-" + std::to_string(slot) + "-gl.bin"),
                renderer.temporary_texture(64, static_cast<MaterialCacheRole>(slot)).read_mip_raw(0));
            blend_validation::write(folder / ("temporary-" + std::to_string(slot) + "-d3d9.bin"),
                native.cache.pixels(renderer.source_bindings().textures[slot], 0));
        }
        const auto mesh = job.patch.geometry.mesh();
        const LightingPatchMesh normal(mesh.columns(), mesh.rows(), job.patch.geometry.streams.normal24,
                                       mesh.indices());
        auto single_shadow = job.plan;
        require(single_shadow.passes.size() == 4 && single_shadow.passes[2].kind == MaterialCachePassKind::shadow,
                "World precision complete shadow pass");
        single_shadow.passes = {single_shadow.passes[2]};
        single_shadow.passes[0].draws.resize(1);
        blend_validation::write(folder / "shadow-float-d3d9.bin",
                                native.cache.float_control(single_shadow, mesh, normal, job.frame));
        const auto shadow_at = single_shadow.passes[0].draws[0].constant_offset;
        require(shadow_at + 0x4c0 <= single_shadow.constants.size(), "World shadow parameters extent");
        MaterialShadowParameters shadow_parameters{job.frame.world_scales, job.frame.sun_direction,
                                                   job.frame.height_texture_scale, {}, {}};
        std::array<float, 2> shadow_offsets{};
        std::memcpy(shadow_parameters.world_rebase.data(), single_shadow.constants.data() + shadow_at + 0x490, 8);
        std::memcpy(shadow_parameters.jitter_add.data(), single_shadow.constants.data() + shadow_at + 0x4b0, 16);
        std::memcpy(shadow_offsets.data(), single_shadow.constants.data() + shadow_at, 8);
        control.attach({control.targets[0]}, true);
        control.gl.Viewport(0, 0, 64, 64);
        control.gl.Disable(GL_BLEND);
        GlMaterialShadowRenderer shadow(context, mesh);
        shadow.draw(renderer.height_texture(index).id(), shadow_parameters, 64, 64, shadow_offsets);
        blend_validation::write(folder / "shadow-float-gl.bin", control.pixels(control.targets[0]));
        GlMaterialBlendRenderer blend(context, mesh, MaterialBlendCoordinates::d3d9_cache);
        std::array<GLuint, 16> names{};
        for (unsigned i = 0; i < names.size(); ++i)
            names[i] = textures.resource(job.patch.resources[i]).id();
        std::array<float, 2> offsets{};
        const auto &draw = job.plan.passes.front().draws.front();
        require(draw.constant_bytes >= 8 && draw.constant_offset + 8 <= job.plan.constants.size(),
                "Material precision original offsets extent");
        std::memcpy(offsets.data(), job.plan.constants.data() + draw.constant_offset, 8);
        const auto gl_draw = [&] {
            control.attach({control.targets[0], control.targets[1]}, true);
            control.gl.Viewport(0, 0, 64, 64);
            control.gl.Disable(GL_BLEND);
            blend.draw(job.frame.material_coefficients, offsets, names, MaterialSampling::ordinary0, {64, 64});
        };
        gl_draw();
        native.cache.material(mesh, job.frame.material_coefficients, job.patch.resources, offsets, true);
        for (unsigned i = 0; i < 2; ++i) {
            blend_validation::write(folder / ("float-" + std::to_string(i) + "-gl.bin"), control.pixels(control.targets[i]));
            blend_validation::write(folder / ("float-" + std::to_string(i) + "-d3d9.bin"), native_raster.pixels(i, true));
        }
        if (!probes.empty()) {
            fs::create_directory(folder / "precision");
            for (const auto &entry : fs::directory_iterator(probes)) {
                if (entry.path().extension() != ".glsl")
                    continue;
                const auto source = input_bytes(entry.path());
                const auto binary = input_bytes(probes / (entry.path().stem().string() + ".bin"));
                require(binary.size() % 4 == 0, "Aligned world precision bytecode");
                std::vector<DWORD> words(binary.size() / 4);
                std::memcpy(words.data(), binary.data(), binary.size());
                Com<IDirect3DPixelShader9> pixel;
                hr(native.cache.device->CreatePixelShader(words.data(), &pixel.value), "World material prefix");
                MaterialPrecisionProbe probe(*context, std::string_view(
                    reinterpret_cast<const char *>(source.data()), source.size()));
                gl_draw();
                probe.draw(job.frame.material_coefficients, offsets, 64,
                           static_cast<GLsizei>(mesh.indices().size()), names);
                native.cache.material(mesh, job.frame.material_coefficients, job.patch.resources, offsets, true, pixel.value);
                const auto name = entry.path().stem().string();
                blend_validation::write(folder / "precision" / (name + "-gl.bin"), control.pixels(control.targets[0]));
                blend_validation::write(folder / "precision" / (name + "-d3d9.bin"), native_raster.pixels(0, true));
                if (name.starts_with("full")) {
                    blend_validation::write(folder / "precision" / (name + "-height-gl.bin"), control.pixels(control.targets[1]));
                    blend_validation::write(folder / "precision" / (name + "-height-d3d9.bin"), native_raster.pixels(1, true));
                }
            }
        }
        execution << "PASS original/GL complete world material patch=" << index << '\n';
    }
    blend_validation::text(out / "precision-execution.txt", execution.str());
    std::cout << execution.str();
}
