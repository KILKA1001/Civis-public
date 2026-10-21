#include "civ5/graphics/gl_land_material.hpp"
#include "civ5/graphics/gl_lighting_cubes.hpp"
#include "civ5/graphics/gl_material_blend.hpp"
#include "civ5/graphics/gl_material_lighting.hpp"
#include "civ5/graphics/gl_material_normals.hpp"
#include "civ5/graphics/gl_material_shadows.hpp"
#include "civ5/graphics/gl_material_textures.hpp"
#include "civ5/graphics/gl_render_api.hpp"
#include "civ5/terrain/height_addressing.hpp"
#include "land_material_native.hpp"
#include <algorithm>
#include <iostream>
#include <limits>
#include <sstream>
using namespace land_material_validation;
using namespace civ5::graphics;
namespace fs = std::filesystem;
#include "land_material_probe.hpp"
#include "land_material_validation_inputs.hpp"
int main(int argc, char **argv) {
    try {
        require(argc == 5 || argc == 6 || (argc == 7 && std::string_view(argv[6]) == "affine"),
                "Usage: civ5_gl_land_material generation-input asset-root "
                "original-shaders fresh-output [diagnostic-prefixes [affine]]");
        const fs::path output(argv[4]);
        require(!fs::exists(output), "Fresh LAND material output required");
        const auto input = read(argv[1]);
        require(is_generation_input(input), "LAND pipeline requires source-generation inputs");
        const auto patches = generation_validation::gpu_patches(input);
        const auto records = generation_validation::records(input);
        const auto record =
            std::find_if(records.begin(), records.end(), [](const auto &r) { return r.op == 5; });
        require(record != records.end(), "Missing source field");
        generation_validation::Reader reader{record->input};
        const generation_validation::Source source(reader);
        for (const auto &r : records)
            if (r.op == 5) {
                generation_validation::Reader check_reader{r.input};
                const generation_validation::Source check_source(check_reader);
                require(source.width == check_source.width && source.height == check_source.height &&
                            source.heights == check_source.heights,
                        "LAND patches must share source U16 field");
            }
        std::vector<std::uint16_t> heights;
        for (auto value : source.heights) {
            require(value >= 0 && value <= 65535, "Source U16 range");
            heights.push_back(static_cast<std::uint16_t>(value));
        }
        const civ5::terrain::HeightField field(civ5::terrain::HeightDomain(source.width, source.height),
                                               heights);
        const auto a8 = field.extract_a8_texture(0, 0);
        auto context = std::make_shared<GlContext>();
        Control c(context);
        NativeLand native(argv[3]);
        const auto height = GlTexture::alpha8(*context, 1024, 1024, a8);
        auto native_height = native.height(a8, 1024);
        auto images = fixtures();
        GlLandMaterialInputs resources;
        resources.height = height.id();
        std::vector<Com<IDirect3DBaseTexture9>> native_images;
        std::array<IDirect3DBaseTexture9 *, 10> native_pointers{};
        fs::create_directories(output);
        write(output / "computed-a8.bin", a8);
        for (unsigned i = 0; i < 10; ++i) {
            resources.pixel[i] = c.image(images[i]);
            native_images.push_back(native.image(images[i]));
            native_pointers[i] = native_images.back().value;
            write(output / ("source-" + std::to_string(i) + ".bin"), images[i].values);
        }
        for (auto input_type : {LandVertexInput::local_grid, LandVertexInput::source_positioned})
            text(output / (input_type == LandVertexInput::local_grid ? "displaced.glsl" : "positioned.glsl"),
                 land_material_vertex_shader(input_type));
        for (auto program :
             {LandPixelProgram::cached_specular, LandPixelProgram::cached_diffuse, LandPixelProgram::overlay})
            text(output / (program == LandPixelProgram::cached_specular  ? "specular.glsl"
                           : program == LandPixelProgram::cached_diffuse ? "diffuse.glsl"
                                                                         : "overlay.glsl"),
                 land_material_fragment_shader(program));
        for (auto program :
             {LandPixelProgram::cached_specular, LandPixelProgram::cached_diffuse, LandPixelProgram::overlay})
            text(output / (program == LandPixelProgram::cached_specular  ? "specular-measured.glsl"
                           : program == LandPixelProgram::cached_diffuse ? "diffuse-measured.glsl"
                                                                         : "overlay-measured.glsl"),
                 land_material_fragment_source(program, LandShadowArithmetic::measured_fma_t_x_z_y));
        unsigned guard_checks = 0;
        const auto rejects = [&](auto operation) {
            bool rejected = false;
            try {
                operation();
            } catch (const std::invalid_argument &) {
                rejected = true;
            }
            require(rejected, "Missing LAND input rejection");
            context->check("LAND negative input left GL clean");
            ++guard_checks;
        };
        {
            const civ5::terrain::LandGrid grid(3, 3, 1);
            const LandMaterialMesh mesh(LandVertexInput::local_grid, grid.vertices(), grid.indices());
            const auto parameters = constants(3, 3, 1);
            rejects([&] { GlLandMaterialRenderer bad(nullptr, mesh, 1); });
            rejects([&] { GlLandMaterialRenderer bad(context, mesh, 5); });
            rejects(
                [&] { GlLandMaterialRenderer bad(context, mesh, 1, static_cast<LandShadowArithmetic>(-1)); });
            rejects([&] { GlLandMaterialRenderer bad(context, mesh, 3); });
            auto moving = mesh;
            const auto moved = std::move(moving);
            rejects([&] { GlLandMaterialRenderer bad(context, moving, 1); });
            GlLandMaterialRenderer renderer(context, moved, 0);
            c.attach({c.targets[6]}, true);
            GlLandMaterialRenderer native_cache(context, moved, 1,
                                                  LandShadowArithmetic::measured_fma_t_x_z_y,
                                                  LandMipSampling::d3d9_rgb565);
            rejects([&] { native_cache.draw(parameters, resources, 64, 64); });
            rejects([&] { renderer.draw(parameters, resources, 0, 64); });
            rejects([&] { renderer.draw(parameters, resources, 64, -1); });
            auto bad_constants = parameters;
            bad_constants.offset[0] = std::numeric_limits<float>::infinity();
            rejects([&] { renderer.draw(bad_constants, resources, 64, 64); });
            rejects([&] { renderer.capture(bad_constants, height.id(), false); });
            for (unsigned slot = 0; slot < 10; ++slot) {
                auto bad = resources;
                bad.pixel[slot] = 0;
                rejects([&] { renderer.draw(parameters, bad, 64, 64); });
                bad.pixel[slot] = resources.pixel[slot == 6 || slot == 7 ? 0 : 6];
                rejects([&] { renderer.draw(parameters, bad, 64, 64); });
                if (slot != 6 && slot != 7) {
                    bad.pixel[slot] = c.targets[6];
                    rejects([&] { renderer.draw(parameters, bad, 64, 64); });
                }
            }
            auto bad = resources;
            bad.height = 0;
            rejects([&] { renderer.draw(parameters, bad, 64, 64); });
            rejects([&] { renderer.capture(parameters, 0, false); });
            bad.height = c.targets[6];
            rejects([&] { renderer.draw(parameters, bad, 64, 64); });
            GLuint empty = 0;
            context->api().CreateTextures(GL_TEXTURE_2D, 1, &empty);
            c.owned.push_back(empty);
            bad = resources;
            bad.pixel[0] = empty;
            rejects([&] { renderer.draw(parameters, bad, 64, 64); });
            // Unused resources are not fabricated into mandatory bindings.
            GlLandMaterialInputs overlay;
            overlay.pixel[1] = resources.pixel[1];
            overlay.height = height.id();
            GlLandMaterialRenderer overlay_renderer(context, mesh, 2);
            c.draw(overlay_renderer, parameters, overlay);
            ++guard_checks;
            auto diffuse = resources;
            diffuse.pixel[8] = diffuse.pixel[9] = 0;
            GlLandMaterialRenderer diffuse_renderer(context, mesh, 1);
            c.draw(diffuse_renderer, parameters, diffuse);
            ++guard_checks;
            GLuint prior = 0;
            c.gl.CreateTransformFeedbacks(1, &prior);
            c.gl.BindTransformFeedback(GL_TRANSFORM_FEEDBACK, prior);
            for (bool discard : {false, true}) {
                if (discard)
                    c.gl.Enable(GL_RASTERIZER_DISCARD);
                else
                    c.gl.Disable(GL_RASTERIZER_DISCARD);
                renderer.capture(parameters, height.id(), true);
                GLint restored = 0;
                context->api().GetIntegerv(GL_TRANSFORM_FEEDBACK_BINDING, &restored);
                require(static_cast<GLuint>(restored) == prior &&
                            bool(c.gl.IsEnabled(GL_RASTERIZER_DISCARD)) == discard,
                        "LAND TF state was not restored");
                ++guard_checks;
            }
            c.gl.Disable(GL_RASTERIZER_DISCARD);
            c.gl.BindTransformFeedback(GL_TRANSFORM_FEEDBACK, 0);
            c.gl.DeleteTransformFeedbacks(1, &prior);
        }
        unsigned cases = 0, affine_cases = 0;
        const std::array<std::array<int, 3>, 4> meshes{{{3, 3, 1}, {9, 7, 2}, {23, 23, 3}, {65, 65, 1}}};
        for (unsigned m = 0; m < meshes.size(); ++m) {
            const auto dims = meshes[m];
            const civ5::terrain::LandGrid grid(dims[0], dims[1], dims[2]);
            for (unsigned selector = 0; selector < 5; ++selector) {
                const auto technique = land_technique(selector);
                std::vector<civ5::terrain::LandGridVertex> vertices(grid.vertices().begin(),
                                                                    grid.vertices().end());
                if (technique.input == LandVertexInput::source_positioned)
                    for (auto &v : vertices) {
                        v.position[1] *= .866f;
                        v.position[2] = 15.f + .125f * v.position[0];
                    }
                LandMaterialMesh mesh(technique.input, vertices, grid.indices());
                GlLandMaterialRenderer renderer(context, mesh, selector);
                GlLandMaterialRenderer measured(context, mesh, selector,
                                                LandShadowArithmetic::measured_fma_t_x_z_y);
                if (argc == 7 && (selector == 0 || selector == 1 || selector == 3)) {
                    const fs::path probes(argv[5]);
                    const std::string label = selector == 0 ? "spec" : "diff";
                    for (unsigned variant = 0; variant < 24; ++variant) {
                        auto parameters = constants(dims[0], dims[1], 5);
                        if (variant != 0) {
                            // Diagnostic matrices only, not native frame/PRNG state.
                            std::uint32_t state = (variant < 12 ? 0x7a621df3u : 0xe52bd09cu) ^ variant;
                            for (unsigned row = 0; row < 4; ++row)
                                for (unsigned lane : {0, 1, 3}) {
                                    state = state * 1664525u + 1013904223u;
                                    const float signed_unit =
                                        (static_cast<int>(state >> 16) - 32768) / 32768.f;
                                    parameters.shadow_projection[row][lane] =
                                        signed_unit * (row == 3    ? 1.5f
                                                       : lane == 3 ? .0005f
                                                                   : .03f);
                                }
                            parameters.shadow_projection[3][3] = 1.25f;
                        }
                        const auto stem = "affine-m" + std::to_string(m) + "-s" + std::to_string(selector) +
                                          "-v" + std::to_string(variant);
                        write(output / (stem + "-full-gl.bin"), c.draw(renderer, parameters, resources));
                        write(output / (stem + "-full-measured.bin"),
                              c.draw(measured, parameters, resources));
                        write(output / (stem + "-full-d3d9.bin"),
                              native.draw(mesh, selector, parameters, native_pointers, native_height.value));
                        ++affine_cases;
                        std::vector<float> matrix;
                        for (const auto &row : parameters.shadow_projection)
                            matrix.insert(matrix.end(), row.begin(), row.end());
                        write(output / (stem + "-matrix.bin"), matrix);
                        for (const std::string key : {std::string("varying-world"), label + "-projection",
                                                      label + "-coordinates", label + "-sample"}) {
                            const auto bytes = read(probes / (key + ".bin"));
                            require(bytes.size() % 4 == 0, "Affine diagnostic alignment");
                            std::vector<DWORD> code(bytes.size() / 4);
                            std::memcpy(code.data(), bytes.data(), bytes.size());
                            Com<IDirect3DPixelShader9> prefix;
                            hr(native.device->CreatePixelShader(code.data(), &prefix.value),
                               "Own affine diagnostic PS");
                            const auto glsl = read(probes / (key + ".glsl"));
                            LandProbeProgram program(
                                *context, mesh, {reinterpret_cast<const char *>(glsl.data()), glsl.size()});
                            c.draw(renderer, parameters, resources);
                            c.attach({c.targets[6]}, true);
                            program.draw(parameters);
                            write(output / (stem + "-" + key + "-gl.bin"), c.pixels(c.targets[6]));
                            write(output / (stem + "-" + key + "-d3d9.bin"),
                                  native.draw(mesh, selector, parameters, native_pointers,
                                              native_height.value, prefix.value));
                            LandProbeProgram checked(
                                *context, mesh, {reinterpret_cast<const char *>(glsl.data()), glsl.size()},
                                true);
                            c.draw(measured, parameters, resources);
                            c.attach({c.targets[6]}, true);
                            checked.draw(parameters);
                            write(output / (stem + "-" + key + "-measured.bin"), c.pixels(c.targets[6]));
                        }
                    }
                }
                if (argc == 6 && m == 1 && selector == 3) {
                    const auto parameters = constants(dims[0], dims[1], 5);
                    const fs::path probes(argv[5]);
                    for (unsigned probe = 0; probe < 42; ++probe) {
                        const auto key =
                            "prefix-" + std::string(probe < 10 ? "0" : "") + std::to_string(probe);
                        const auto bytes = read(probes / (key + ".bin"));
                        require(bytes.size() % 4 == 0, "Diagnostic word alignment");
                        std::vector<DWORD> code(bytes.size() / 4);
                        std::memcpy(code.data(), bytes.data(), bytes.size());
                        Com<IDirect3DPixelShader9> prefix;
                        hr(native.device->CreatePixelShader(code.data(), &prefix.value),
                           "Validation-only native prefix");
                        const auto glsl = read(probes / (key + ".glsl"));
                        LandProbeProgram program(*context, mesh,
                                                 {reinterpret_cast<const char *>(glsl.data()), glsl.size()});
                        c.draw(renderer, parameters, resources);
                        c.attach({c.targets[6]}, true);
                        program.draw(parameters);
                        write(output / (key + "-gl.bin"), c.pixels(c.targets[6]));
                        write(output / (key + "-d3d9.bin"),
                              native.draw(mesh, selector, parameters, native_pointers, native_height.value,
                                          prefix.value));
                    }
                }
                const auto mesh_key = "mesh-" + std::to_string(m) + "-s" + std::to_string(selector);
                write(output / (mesh_key + "-vb.bin"),
                      {reinterpret_cast<const std::uint8_t *>(mesh.vertices().data()),
                       mesh.vertices().size_bytes()});
                write(output / (mesh_key + "-ib.bin"),
                      {reinterpret_cast<const std::uint8_t *>(mesh.indices().data()),
                       mesh.indices().size_bytes()});
                for (unsigned variant = 0; variant < 6; ++variant) {
                    const auto parameters = constants(dims[0], dims[1], variant);
                    const auto key = mesh_key + "-v" + std::to_string(variant);
                    write(output / (key + "-gl.bin"), c.draw(renderer, parameters, resources));
                    write(output / (key + "-measured.bin"), c.draw(measured, parameters, resources));
                    write(output / (key + "-d3d9.bin"),
                          native.draw(mesh, selector, parameters, native_pointers, native_height.value));
                    ++cases;
                    if (variant == 0) {
                        write(output / (mesh_key + "-tf.bin"),
                              renderer.capture(parameters, height.id(), false));
                        write(output / (mesh_key + "-indexed.bin"),
                              renderer.capture(parameters, height.id(), true));
                    }
                }
            }
        }
        // Actual direct upstream chain. Unknown world/cache ownership is not inferred
        // from this explicit local 65x65 LAND consumer fixture.
        auto assets = std::make_shared<const MaterialAssets>(authored(argv[2]));
        GlMaterialTextures textures(context, assets);
        const auto cube_source = cube_assets(argv[2]);
        GlLightingCubes lights(*context, cube_source);
        std::array<float, 384> coefficients{};
        assets->write_constants(2070.375f, coefficients);
        const auto lighting_lut = c.image(images[6]);
        const auto spec = c.image(images[8]);
        for (unsigned p = 0; p < 3; ++p) {
            const auto &patch = patches[p].patch;
            const auto mesh = patch.mesh();
            GlMaterialBlendRenderer blend(context, mesh);
            c.attach({c.targets[0], c.targets[1]});
            blend.draw(coefficients, {0, 0}, textures.bind(patches[p].selected.sources()),
                       MaterialSampling::ordinary0);
            GlMaterialNormalRenderer normals(context);
            c.attach({c.targets[2]});
            normals.draw(c.targets[1], 64);
            GlMaterialShadowRenderer shadows(context, mesh);
            c.attach({c.targets[3]});
            const auto parameter = material_shadow_parameters({1, .866f, .2f}, {.75f, 1, .5f},
                                                              {1.f / 1024, -1.f / 1024}, {64, 64}, {0, 0}, 4);
            const std::array<MaterialShadowParameters, 4> samples{parameter, parameter, parameter, parameter};
            shadows.draw_accumulated(height.id(), samples, 64, 64);
            const LightingPatchMesh lighting_mesh(patch.columns, patch.rows, patch.streams.normal24,
                                                  patch.indices);
            GlMaterialLightingRenderer lighting(context, lighting_mesh, true,
                                                LightingCubeSampling::measured_coordinates);
            const civ5::terrain::LandGrid grid(65, 65, 1);
            const LandMaterialMesh land_mesh(LandVertexInput::local_grid, grid.vertices(), grid.indices());
            for (unsigned style = 0; style < 5; ++style) {
                c.attach({c.targets[4], c.targets[5]}, true);
                lighting.draw(c.targets[2], c.targets[0], c.targets[3], spec, lights.sunlight(style),
                              lights.shadowlight(style), lighting_lut, 64, 64, {.125f, -.25f, .75f});
                auto connected = resources;
                connected.pixel[0] = c.targets[4];
                connected.pixel[8] = c.targets[5];
                auto pointers = native_pointers;
                auto diffuse_values = c.pixels(c.targets[4]), spec_values = c.pixels(c.targets[5]);
                auto diffuse_native = native.image({64, false, diffuse_values}),
                     spec_native = native.image({64, false, spec_values});
                pointers[0] = diffuse_native.value;
                pointers[8] = spec_native.value;
                const auto stem = "direct-p" + std::to_string(p) + "-style" + std::to_string(style);
                write(output / (stem + "-diffuse.bin"), diffuse_values);
                write(output / (stem + "-specular.bin"), spec_values);
                for (unsigned selector = 0; selector < 2; ++selector) {
                    GlLandMaterialRenderer renderer(context, land_mesh, selector);
                    GlLandMaterialRenderer measured(context, land_mesh, selector,
                                                    LandShadowArithmetic::measured_fma_t_x_z_y);
                    const auto parameters = constants(65, 65, 1);
                    const auto key = stem + "-s" + std::to_string(selector);
                    write(output / (key + "-gl.bin"), c.draw(renderer, parameters, connected));
                    write(output / (key + "-measured.bin"), c.draw(measured, parameters, connected));
                    write(output / (key + "-d3d9.bin"),
                          native.draw(land_mesh, selector, parameters, pointers, native_height.value));
                    ++cases;
                }
            }
        }
        require(cases == 150, "Incomplete LAND material corpus");
        require(affine_cases == (argc == 7 ? 288u : 0u), "Incomplete affine control corpus");
        std::ostringstream report;
        report << context->description() << "\nDONE cases=" << cases << "; affineCases=" << affine_cases
               << "; measuredCases=" << cases + affine_cases << "; nativeUploadBytes=" << native.uploaded
               << "; guardChecks=" << guard_checks
               << "; originalAllFivePairs=true; directGpuChain=true; sourceOwnershipProven=false; "
                  "fullScene=false\n";
        text(output / "execution.txt", report.str());
        std::cout << report.str();
    } catch (const std::exception &e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
