#include "civ5/graphics/gl_material_lighting.hpp"
#include "civ5/graphics/gl_lighting_cubes.hpp"
#include "civ5/graphics/gl_material_blend.hpp"
#include "civ5/graphics/gl_material_normals.hpp"
#include "civ5/graphics/gl_material_shadows.hpp"
#include "civ5/graphics/gl_material_textures.hpp"
#include "civ5/graphics/gl_render_api.hpp"
#include "civ5/terrain/height_addressing.hpp"
#include "material_lighting_native.hpp"
#include <algorithm>
#include <iostream>
#include <limits>
#include <sstream>
#include <thread>
using namespace lighting_validation;
using namespace civ5::graphics;
namespace fs = std::filesystem;
namespace {
unsigned checks = 0;
void check(bool ok, const char *why) {
    ++checks;
    require(ok, why);
}
template <class F> void rejects(F f) {
    bool failed = false;
    try {
        f();
    } catch (const std::exception &) {
        failed = true;
    }
    check(failed, "Missing lighting rejection");
}
LightingCubeAssets cubes(const fs::path &root) {
    AssetEnvironment environment(root);
    const auto archive = [&](const char *name) {
        auto result = environment.open_fpk_archive(name);
        require(std::holds_alternative<std::shared_ptr<FpkSource>>(result),
                "Exact lighting archive unavailable");
        return std::get<std::shared_ptr<FpkSource>>(result);
    };
    const auto base = archive("Resource/DX9/TerrainTextures.fpk"),
               upgrade = archive("Resource/DX9/Upgrade1Textures.fpk");
    return LightingCubeAssets::load([&](const LightingSource &request) {
        const auto name = request.native_path.substr(request.native_path.find_last_of('\\') + 1);
        const auto found = (name.ends_with("_2.dds") ? upgrade : base)->find(AssetKey(name));
        const auto *handle = std::get_if<AssetHandle>(&found);
        require(handle != nullptr, "Exact cube unresolved");
        return *handle;
    });
}
Bytes height_a8(std::span<const std::uint8_t> input) {
    std::vector<std::int32_t> common;
    int width = 0, height = 0;
    unsigned count = 0;
    for (const auto &record : generation_validation::records(input))
        if (record.op == 5) {
            generation_validation::Reader r{record.input};
            const generation_validation::Source source(r);
            if (count++ == 0) {
                width = source.width;
                height = source.height;
                common = source.heights;
            } else
                require(width == source.width && height == source.height && common == source.heights,
                        "Lighting patches must share source U16 field");
        }
    require(count == 3, "Three full source patches required");
    std::vector<std::uint16_t> values;
    for (auto v : common) {
        require(v >= 0 && v <= 65535, "Source height outside U16");
        values.push_back(static_cast<std::uint16_t>(v));
    }
    const civ5::terrain::HeightField field(civ5::terrain::HeightDomain(width, height), values);
    return field.extract_a8_texture(0, 0);
}
std::vector<float> lut_values() {
    std::vector<float> out(16 * 16 * 16 * 4);
    for (unsigned i = 0; i < out.size(); ++i) {
        const auto x = i / 4 % 16, y = i / 4 / 16 % 16, z = i / 4 / 256;
        switch (i % 4) {
        case 0:
            out[i] = (x + y) / 30.f;
            break;
        case 1:
            out[i] = std::sqrt(y / 15.f) * .8f + .1f;
            break;
        case 2:
            out[i] = z * z / 225.f;
            break;
        default:
            out[i] = 1;
        }
    }
    return out;
}
std::array<std::vector<float>, 4> fixture(const std::string &name) {
    std::array<std::array<double, 3>, 6> directions{
        {{1, 0, .125}, {0, 1, .25}, {-1, 0, .5}, {0, -1, .125}, {1, 1, .25}, {-1, -1, .5}}};
    std::array<std::array<float, 3>, 6> encoded{};
    for (unsigned d = 0; d < 6; ++d) {
        const auto &v = directions[d];
        const auto length = std::sqrt((v[0] * v[0] + v[1] * v[1]) + v[2] * v[2]);
        for (unsigned i = 0; i < 3; ++i)
            encoded[d][i] = static_cast<float>(.5 + .5 * (v[i] / length));
    }
    std::array<std::vector<float>, 4> values;
    for (auto &v : values)
        v.resize(64 * 64 * 4);
    for (unsigned i = 0; i < 64 * 64 * 4; ++i) {
        const auto x = i / 4 % 64, y = i / 4 / 64, c = i % 4, axis = x / 10 % 6;
        values[0][i] = c == 3                 ? 0
                       : name == "flat"       ? (c == 2 ? 1.f : .5f)
                       : name == "hemisphere" ? encoded[axis][c]
                       : c == axis / 2        ? (axis % 2 == 0 ? 1.f : 0.f)
                                              : .5f;
        values[1][i] = c == 3 ? 1.f : .15f + (x + c * 17) / 160.f;
        values[2][i] = c == 0 ? (name == "flat" ? 0.f : y / 64.f) : .731f;
        values[3][i] = c == 3 ? .03125f + (i / 4 % 8) / 16.f : .015f * (c + 1);
    }
    return values;
}
struct Control {
    std::shared_ptr<GlContext> context;
    GlRenderApi gl;
    GLuint fbo = 0;
    std::vector<GLuint> owned;
    std::array<GLuint, 6> targets{};
    explicit Control(std::shared_ptr<GlContext> c) : context(std::move(c)), gl(*context) {
        try {
            gl.CreateFramebuffers(1, &fbo);
            for (auto &t : targets)
                t = texture(std::vector<float>(64 * 64 * 4), 64);
            gl.Viewport(0, 0, 64, 64);
            for (auto state : {GL_BLEND, GL_DEPTH_TEST, GL_CULL_FACE, GL_SCISSOR_TEST, GL_DITHER,
                               GL_FRAMEBUFFER_SRGB, GL_RASTERIZER_DISCARD, GL_TEXTURE_CUBE_MAP_SEAMLESS})
                gl.Disable(state);
            context->check("Lighting own control objects");
        } catch (...) {
            release();
            throw;
        }
    }
    void release() noexcept {
        gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
        gl.DeleteFramebuffers(1, &fbo);
        fbo = 0;
        context->api().DeleteTextures(static_cast<GLsizei>(owned.size()), owned.data());
        owned.clear();
    }
    ~Control() { release(); }
    std::vector<float> readback(GLuint id, unsigned size = 64, bool volume = false) {
        std::vector<float> result(std::size_t(size) * size * (volume ? size : 1) * 4);
        context->api().GetTextureImage(id, 0, GL_RGBA, GL_FLOAT, static_cast<GLsizei>(result.size() * 4),
                                       result.data());
        context->check("Lighting float readback");
        require(std::all_of(result.begin(), result.end(), [](float v) { return std::isfinite(v); }),
                "Nonfinite/unwritten lighting pixels");
        return result;
    }
    GLuint texture(std::span<const float> values, unsigned size, bool volume = false) {
        require(values.size() == std::size_t(size) * size * (volume ? size : 1) * 4,
                "Lighting upload extent");
        GLuint id = 0;
        const auto &api = context->api();
        api.CreateTextures(volume ? GL_TEXTURE_3D : GL_TEXTURE_2D, 1, &id);
        owned.push_back(id);
        if (volume) {
            const auto storage =
                reinterpret_cast<PFNGLTEXTURESTORAGE3DPROC>(context->proc_address("glTextureStorage3D"));
            storage(id, 1, GL_RGBA32F, size, size, size);
            api.TextureSubImage3D(id, 0, 0, 0, 0, size, size, size, GL_RGBA, GL_FLOAT, values.data());
        } else {
            api.TextureStorage2D(id, 1, GL_RGBA32F, size, size);
            api.TextureSubImage2D(id, 0, 0, 0, size, size, GL_RGBA, GL_FLOAT, values.data());
        }
        const auto actual = readback(id, size, volume);
        require(std::memcmp(actual.data(), values.data(), values.size_bytes()) == 0,
                "C++ lighting upload bytes differ");
        return id;
    }
    void attach(std::initializer_list<GLuint> textures, bool poison = false) {
        require(textures.size() > 0 && textures.size() <= 2, "Lighting target count");
        unsigned i = 0;
        for (auto t : textures)
            gl.NamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT0 + i++, t, 0);
        if (i == 1)
            gl.NamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT1, 0, 0);
        const GLenum buffers[]{GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
        gl.NamedFramebufferDrawBuffers(fbo, i, buffers);
        require(gl.CheckNamedFramebufferStatus(fbo, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
                "Lighting FBO incomplete");
        gl.BindFramebuffer(GL_FRAMEBUFFER, fbo);
        const auto value = poison ? std::numeric_limits<float>::quiet_NaN() : 0.f;
        const float clear[]{value, value, value, value};
        for (unsigned j = 0; j < i; ++j)
            gl.ClearNamedFramebufferfv(fbo, GL_COLOR, j, clear);
    }
    std::vector<float> light(GlMaterialLightingRenderer &renderer, const std::array<GLuint, 4> &images,
                             const GlTexture &sun, const GlTexture &shade, GLuint lut, bool specular) {
        if (specular)
            attach({targets[4], targets[5]}, true);
        else
            attach({targets[4]}, true);
        renderer.draw(images[0], images[1], images[2], images[3], sun, shade, lut, 64, 64,
                      {.125f, -.25f, .75f});
        auto out = readback(targets[4]);
        for (unsigned i = 3; i < out.size(); i += 4)
            require(out[i] == 1, "Lighting diffuse alpha/coverage");
        if (specular) {
            const auto second = readback(targets[5]);
            out.insert(out.end(), second.begin(), second.end());
        }
        return out;
    }
};
void guards(Control &c, const LightingPatchMesh &mesh, const std::array<GLuint, 4> &images,
            const GlLightingCubes &cubes, const LightingCubeAssets &assets, GLuint lut) {
    rejects([&] { GlMaterialLightingRenderer bad(nullptr, mesh, false); });
    rejects([&] {
        GlMaterialLightingRenderer bad(c.context, mesh, false, static_cast<LightingCubeSampling>(-1));
    });
    auto moved = mesh;
    const auto retained = std::move(moved);
    rejects([&] { GlMaterialLightingRenderer bad(c.context, moved, false); });
    GlMaterialLightingRenderer renderer(c.context, mesh, true, LightingCubeSampling::measured_coordinates);
    const auto draw = [&](GLuint normal, GLuint cc, int width, int height, const std::array<float, 3> &half) {
        renderer.draw(normal, images[1], images[2], images[3], cubes.sunlight(0), cubes.shadowlight(0), cc,
                      width, height, half);
    };
    const auto good = c.light(renderer, images, cubes.sunlight(0), cubes.shadowlight(0), lut, true);
    check(!c.gl.IsEnabled(GL_TEXTURE_CUBE_MAP_SEAMLESS), "Seamless false state leaked");
    c.gl.Enable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    draw(images[0], lut, 64, 64, {.125f, -.25f, .75f});
    check(c.gl.IsEnabled(GL_TEXTURE_CUBE_MAP_SEAMLESS), "Seamless true state lost");
    c.gl.Disable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    for (auto size : {0, -1, std::numeric_limits<int>::min()}) {
        rejects([&] { draw(images[0], lut, size, 64, {0, 0, 1}); });
        rejects([&] { draw(images[0], lut, 64, size, {0, 0, 1}); });
    }
    rejects([&] { draw(0, lut, 64, 64, {0, 0, 1}); });
    rejects([&] { draw(images[0], images[0], 64, 64, {0, 0, 1}); });
    rejects([&] { draw(lut, lut, 64, 64, {0, 0, 1}); });
    for (unsigned i = 0; i < 3; ++i) {
        std::array<float, 3> half{0, 0, 1};
        half[i] = std::numeric_limits<float>::quiet_NaN();
        rejects([&] { draw(images[0], lut, 64, 64, half); });
    }
    rejects([&] { draw(c.targets[4], lut, 64, 64, {0, 0, 1}); });
    rejects([&] { draw(c.targets[5], lut, 64, 64, {0, 0, 1}); });
    const auto all_inputs = [&](std::array<GLuint, 4> input, const GlTexture &sun, const GlTexture &shade,
                                GLuint cc) {
        renderer.draw(input[0], input[1], input[2], input[3], sun, shade, cc, 64, 64, {0, 0, 1});
    };
    for (unsigned slot = 0; slot < 4; ++slot)
        for (GLuint invalid : {0u, lut, c.targets[4], c.targets[5]}) {
            auto input = images;
            input[slot] = invalid;
            rejects([&] { all_inputs(input, cubes.sunlight(0), cubes.shadowlight(0), lut); });
        }
    const auto &api = c.context->api();
    GLuint empty = 0;
    api.CreateTextures(GL_TEXTURE_2D, 1, &empty);
    rejects([&] { draw(empty, lut, 64, 64, {0, 0, 1}); });
    api.DeleteTextures(1, &empty);
    rejects([&] { draw(empty, lut, 64, 64, {0, 0, 1}); });
    const auto bad_cube = [&](const GlTexture &cube) {
        rejects([&] { all_inputs(images, cube, cubes.shadowlight(0), lut); });
        rejects([&] { all_inputs(images, cubes.sunlight(0), cube, lut); });
    };
    auto linear = GlTexture(*c.context, assets.cubes()[0].data);
    bad_cube(linear);
    auto transferred = std::move(linear);
    bad_cube(linear);
    const std::array<std::uint8_t, 1> zero{};
    auto two_d = GlTexture::alpha8(*c.context, 1, 1, zero);
    bad_cube(two_d);
    for (auto [parameter, value] :
         std::array<std::pair<GLenum, GLint>, 2>{{{GL_TEXTURE_BASE_LEVEL, 1}, {GL_TEXTURE_MAX_LEVEL, 6}}}) {
        const auto sun = cubes.sunlight(0).id();
        api.TextureParameteri(sun, parameter, value);
        rejects([&] { draw(images[0], lut, 64, 64, {0, 0, 1}); });
        api.TextureParameteri(sun, parameter, parameter == GL_TEXTURE_BASE_LEVEL ? 0 : 7);
    }
    for (auto attachment : {lut, cubes.sunlight(0).id(), cubes.shadowlight(0).id()}) {
        c.gl.NamedFramebufferTexture(c.fbo, GL_COLOR_ATTACHMENT2, attachment, 0);
        rejects([&] { draw(images[0], lut, 64, 64, {0, 0, 1}); });
        c.gl.NamedFramebufferTexture(c.fbo, GL_COLOR_ATTACHMENT2, 0, 0);
    }
    {
        GlContext foreign_context;
        GlTexture foreign_cube(foreign_context, assets.cubes()[0].data, DdsSamplingColor::srgb);
        bad_cube(foreign_cube);
    }
    c.context->api();
    c.context->check("Rejected lighting inputs left clean state");
    const auto still = c.readback(c.targets[4]);
    check(std::memcmp(still.data(), good.data(), still.size() * 4) == 0,
          "Invalid input wrote lighting target");
    bool foreign = false;
    std::thread other([&] {
        try {
            draw(images[0], lut, 64, 64, {0, 0, 1});
        } catch (const std::exception &) {
            foreign = true;
        }
    });
    other.join();
    check(foreign, "Lighting foreign thread accepted");
    {
        auto owned = std::make_shared<GlContext>();
        std::weak_ptr<GlContext> weak = owned;
        auto retained_renderer = std::make_unique<GlMaterialLightingRenderer>(owned, mesh, false);
        owned.reset();
        check(!weak.expired(), "Lighting renderer failed to retain context");
        retained_renderer.reset();
        check(weak.expired(), "Lighting renderer leaked context");
    }
    c.context->api();
}
} // namespace
int main(int argc, char **argv) {
    try {
        require(argc == 6, "Usage: civ5_gl_material_lighting generation-input asset-root "
                           "original-lighting-shaders legacy-input-resources fresh-output");
        const fs::path output(argv[5]);
        require(!fs::exists(output), "Fresh lighting output required");
        const auto input = read(argv[1]);
        require(is_generation_input(input), "Lighting pipeline requires source-generation inputs");
        const auto patches = generation_validation::gpu_patches(input);
        auto assets = std::make_shared<const MaterialAssets>(authored(argv[2]));
        const auto authored_cubes = cubes(argv[2]);
        const auto lut_data = lut_values();
        const auto a8 = height_a8(input);
        auto context = std::make_shared<GlContext>();
        Control c(context);
        GlMaterialTextures material_textures(context, assets);
        GlLightingCubes lights(*context, authored_cubes);
        NativeLighting native(argv[3], authored_cubes, lut_data);
        const auto lut = c.texture(lut_data, 16, true);
        std::vector<float> expanded(a8.size() * 4);
        for (std::size_t i = 0; i < a8.size(); ++i)
            expanded[i * 4 + 3] = a8[i] / 255.f;
        const auto height_float = c.texture(expanded, 1024);
        const auto height_unorm = GlTexture::alpha8(*context, 1024, 1024, a8);
        fs::create_directories(output);
        write(output / "computed-a8.bin", a8);
        write(output / "explicit-lut.bin", lut_data);
        text(output / "lighting-vertex.glsl", material_lighting_vertex_shader());
        for (bool spec : {false, true})
            text(output / (spec ? "lighting-specular.glsl" : "lighting-diffuse.glsl"),
                 material_lighting_fragment_shader(spec));
        for (unsigned i = 0; i < 10; ++i)
            write(output / ("cube-" + std::to_string(i) + ".dds"),
                  authored_cubes.cubes()[i].data.original_bytes());
        auto probe = make_lighting_coordinate_probe(*context);
        for (unsigned face = 0; face < 6; ++face)
            for (unsigned mip = 0; mip < 8; ++mip)
                write(output / ("coordinate-" + std::to_string(face) + "-" + std::to_string(mip) + ".bin"),
                      probe.read_level_raw(face, mip));
        std::array<float, 384> coefficients{};
        assets->write_constants(2070.375f, coefficients);
        write(output / "authored-coefficients.bin", coefficients);
        const std::array<LightingCubeSampling, 3> modes{LightingCubeSampling::ordinary,
                                                        LightingCubeSampling::integer_no_coordinates,
                                                        LightingCubeSampling::measured_coordinates};
        const std::array<const char *, 3> mode_names{"ordinary", "integer", "measured"};
        unsigned cases = 0;
        for (unsigned p = 0; p < 3; ++p) {
            const auto &patch = patches[p].patch;
            const auto selected = patches[p].selected.sources();
            LightingPatchMesh mesh(patch.columns, patch.rows, patch.streams.normal24, patch.indices);
            write(output / ("patch-" + std::to_string(p) + "-vb.bin"), mesh.vertices());
            write(
                output / ("patch-" + std::to_string(p) + "-ib.bin"),
                {reinterpret_cast<const std::uint8_t *>(mesh.indices().data()), mesh.indices().size_bytes()});
            std::array<std::array<std::unique_ptr<GlMaterialLightingRenderer>, 2>, 3> renderers;
            for (unsigned mode = 0; mode < 3; ++mode)
                for (unsigned spec = 0; spec < 2; ++spec)
                    renderers[mode][spec] =
                        std::make_unique<GlMaterialLightingRenderer>(context, mesh, spec != 0, modes[mode]);
            for (const std::string name :
                 {"flat", "hemisphere", "axes", "upstream", "cpp-upstream", "cpp-upstream-a8"}) {
                auto values = fixture(name);
                std::array<GLuint, 4> images{};
                images[3] = c.texture(values[3], 64);
                if (name == "upstream") {
                    // Input-only legacy GPU fields, never expected lighting outputs.
                    // Old Kotlin used CPU-decoded RGBA8 material mips; production
                    // now keeps raw BC3. Retain the exact old lighting corpus too.
                    for (unsigned i = 0; i < 3; ++i) {
                        const auto data = read(fs::path(argv[4]) / (name + "-p" + std::to_string(p) + "-s" +
                                                                    std::to_string(i) + ".bin"));
                        require(data.size() == 64 * 64 * 16, "Legacy lighting source extent");
                        std::memcpy(values[i].data(), data.data(), data.size());
                        images[i] = c.texture(values[i], 64);
                    }
                } else if (name.starts_with("cpp-upstream")) {
                    const auto blend_mesh = patch.mesh();
                    GlMaterialBlendRenderer blend(context, blend_mesh);
                    c.attach({c.targets[0], c.targets[1]});
                    blend.draw(coefficients, {0, 0}, material_textures.bind(selected),
                               MaterialSampling::ordinary0);
                    GlMaterialNormalRenderer normals(context);
                    c.attach({c.targets[2]});
                    normals.draw(c.targets[1], 64);
                    GlMaterialShadowRenderer shadows(context, blend_mesh);
                    c.attach({c.targets[3]});
                    const auto parameters = material_shadow_parameters(
                        {1, .866f, .2f}, {.75f, 1, .5f}, {1.f / 1024, -1.f / 1024}, {64, 64}, {0, 0}, 4);
                    const std::array<MaterialShadowParameters, 4> samples{parameters, parameters, parameters,
                                                                          parameters};
                    shadows.draw_accumulated(name == "cpp-upstream-a8" ? height_unorm.id() : height_float,
                                             samples, 64, 64);
                    images[0] = c.targets[2];
                    images[1] = c.targets[0];
                    images[2] = c.targets[3];
                } else
                    for (unsigned i = 0; i < 3; ++i)
                        images[i] = c.texture(values[i], 64);
                std::vector<Com<IDirect3DTexture9>> native_images;
                std::array<IDirect3DTexture9 *, 4> pointers{};
                for (unsigned i = 0; i < 4; ++i) {
                    values[i] = c.readback(images[i]);
                    write(output / (name + "-p" + std::to_string(p) + "-s" + std::to_string(i) + ".bin"),
                          values[i]);
                    native_images.push_back(native.image(values[i]));
                    pointers[i] = native_images.back().value;
                }
                if (p == 0 && name == "flat")
                    guards(c, mesh, images, lights, authored_cubes, lut);
                for (unsigned style = 0; style < 5; ++style)
                    for (unsigned spec = 0; spec < 2; ++spec) {
                        const auto id = name + "-p" + std::to_string(p) + "-style" + std::to_string(style) +
                                        (spec ? "-spec" : "-diffuse");
                        write(output / (id + "-d3d9.bin"),
                              native.draw(mesh, pointers, style, spec != 0, {.125f, -.25f, .75f}));
                        for (unsigned mode = 0; mode < 3; ++mode)
                            write(output / (id + "-" + mode_names[mode] + ".bin"),
                                  c.light(*renderers[mode][spec], images, lights.sunlight(style),
                                          lights.shadowlight(style), lut, spec != 0));
                        check(!c.gl.IsEnabled(GL_TEXTURE_CUBE_MAP_SEAMLESS),
                              "Lighting case leaked seamless state");
                        ++cases;
                    }
            }
        }
        require(cases == 180, "Incomplete lighting cases");
        std::ostringstream report;
        report << context->description() << "\nDONE cases=" << cases << "; checks=" << checks
               << "; nativeUploadBytes=" << native.uploaded
               << "; cppSourceGeneration=true; directGpuUpstream=true; originalVSAndBothPS=true; "
                  "originalGameProcess=false; fullScene=false\n";
        text(output / "execution.txt", report.str());
        std::cout << report.str();
    } catch (const std::exception &e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
