#include "civ5/graphics/gl_material_cache_raster.hpp"
#include "civ5/graphics/gl_material_blend.hpp"
#include "civ5/graphics/gl_material_cache.hpp"
#include "civ5/graphics/gl_material_normals.hpp"
#include "civ5/graphics/gl_material_textures.hpp"
#include "civ5/graphics/gl_render_api.hpp"
#include "civ5/materials/material_cache_raster.hpp"
#include "material_cache_raster_native.hpp"
#include "material_precision_probe.hpp"
#include <iostream>
#include <sstream>

using namespace cache_raster_validation;
using namespace civ5::graphics;
namespace fs = std::filesystem;
namespace {
struct Control {
    std::shared_ptr<GlContext> context;
    GlRenderApi gl;
    GlMaterialCacheRaster raster;
    GlMaterialCacheTextures cache;
    std::array<GLuint, 3> fbos{};
    std::array<GLuint, 2> float_textures{};
    unsigned size;
    Control(std::shared_ptr<GlContext> c, unsigned resolution, const std::array<bool, 3> &automatic)
        : context(std::move(c)), gl(*context), raster(context),
          cache(context, material_cache_texture_plan(resolution, resolution, false),
                {automatic[0], automatic[2], true}),
          size(resolution) {
        require(automatic[0] == automatic[1], "Equal float cache capabilities");
        gl.CreateFramebuffers(3, fbos.data());
        context->api().CreateTextures(GL_TEXTURE_2D, 2, float_textures.data());
        for (unsigned i = 0; i < 2; ++i) {
            context->api().TextureStorage2D(float_textures[i], 1, GL_RGBA32F, size, size);
            gl.NamedFramebufferTexture(fbos[2], GL_COLOR_ATTACHMENT0 + i, float_textures[i], 0);
        }
        gl.NamedFramebufferTexture(fbos[0], GL_COLOR_ATTACHMENT0, texture(0).id(), 0);
        gl.NamedFramebufferTexture(fbos[0], GL_COLOR_ATTACHMENT1, texture(1).id(), 0);
        gl.NamedFramebufferTexture(fbos[1], GL_COLOR_ATTACHMENT0, texture(2).id(), 0);
        const GLenum attachments[]{GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
        gl.NamedFramebufferDrawBuffers(fbos[0], 2, attachments);
        gl.NamedFramebufferDrawBuffers(fbos[1], 1, attachments);
        gl.NamedFramebufferDrawBuffers(fbos[2], 2, attachments);
        for (auto f : fbos)
            require(gl.CheckNamedFramebufferStatus(f, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
                    "Cache raster FBO");
        gl.Viewport(0, 0, size, size);
        for (auto state : {GL_BLEND, GL_CULL_FACE, GL_DEPTH_TEST, GL_STENCIL_TEST, GL_SCISSOR_TEST, GL_DITHER,
                           GL_FRAMEBUFFER_SRGB, GL_RASTERIZER_DISCARD})
            gl.Disable(state);
        gl.ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        context->check("Cache raster GPU initialization");
    }
    ~Control() {
        gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
        gl.DeleteFramebuffers(3, fbos.data());
        context->api().DeleteTextures(2, float_textures.data());
    }
    const GlTexture &texture(unsigned index) const {
        constexpr std::array roles{MaterialCacheRole::render_temporary, MaterialCacheRole::height_temporary,
                                   MaterialCacheRole::normal_temporary};
        return cache.texture(roles.at(index));
    }
    void target(bool normal, bool float_control = false) {
        if (!float_control) {
            MaterialCachePass pass{};
            pass.kind = normal ? MaterialCachePassKind::normal : MaterialCachePassKind::height;
            pass.targets =
                normal ? std::array<std::uint32_t, 4>{5, 0, 0, 0} : std::array<std::uint32_t, 4>{1, 2, 0, 0};
            pass.flags = 4;
            pass.clear_color = {0, normal ? .5f : 0.f, 0, 1};
            const std::array<GlMaterialCacheTarget, 2> targets{
                {{normal ? 5u : 1u, &texture(normal ? 2 : 0)}, {2, &texture(1)}}};
            raster.begin(pass, std::span(targets).first(normal ? 1 : 2), false);
            return;
        }
        for (unsigned i = 0; i < 16; ++i)
            context->api().BindTextureUnit(i, 0);
        const auto f = fbos[float_control ? 2 : normal ? 1 : 0];
        gl.BindFramebuffer(GL_FRAMEBUFFER, f);
        const auto clear =
            material_cache_clear_rgba(material_cache_clear_argb({0, normal ? .5f : 0.f, 0, 1}));
        for (int i = 0; i < (normal ? 1 : 2); ++i)
            gl.ClearNamedFramebufferfv(f, GL_COLOR, i, clear.data());
    }
    void material(GlMaterialBlendRenderer &renderer, std::span<const float, 384> coeff,
                  const std::array<GLuint, 16> &textures, std::array<float, 2> offsets,
                  bool float_control = false) {
        target(false, float_control);
        renderer.draw(coeff, offsets, textures, MaterialSampling::ordinary0, {int(size), int(size)});
        if (!float_control)
            cache.generate_mips(MaterialCacheRole::height_temporary);
    }
    void normals(GlMaterialNormalRenderer &renderer, bool corrected = true) {
        target(true);
        if (corrected)
            renderer.draw_cache(texture(1).id(), size);
        else
            renderer.draw(texture(1).id(), size);
    }
    Bytes float_pixels(unsigned index) {
        Bytes result(std::size_t(size) * size * 16);
        context->api().GetTextureImage(float_textures.at(index), 0, GL_RGBA, GL_FLOAT,
                                       static_cast<GLsizei>(result.size()), result.data());
        context->check("Cache preconversion readback");
        return result;
    }
    unsigned state_controls() {
        unsigned checked = 0;
        const auto check = [&](bool ok, const char *why) {
            require(ok, why);
            ++checked;
        };
        MaterialCachePass pass{};
        pass.kind = MaterialCachePassKind::shadow;
        pass.targets = {5, 0, 0, 0};
        pass.blend_state = 4;
        pass.depth_surface = 77;
        pass.flags = 128;
        pass.clear_color = {0, .5f, 0, 1};
        const std::array<GlMaterialCacheTarget, 1> map{{{5, &texture(2)}}};
        for (auto state :
             {GL_DEPTH_TEST, GL_STENCIL_TEST, GL_CULL_FACE, GL_SCISSOR_TEST, GL_DITHER, GL_FRAMEBUFFER_SRGB})
            gl.Enable(state);
        gl.DepthMask(GL_TRUE);
        gl.ColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
        gl.PolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        raster.begin(pass, map, true);
        for (auto state :
             {GL_DEPTH_TEST, GL_STENCIL_TEST, GL_CULL_FACE, GL_SCISSOR_TEST, GL_DITHER, GL_FRAMEBUFFER_SRGB})
            check(!gl.IsEnabled(state), "Cache failed to reset previous raster state");
        check(gl.IsEnabled(GL_BLEND), "Native blend4 not enabled");
        std::array<GLint, 4> value{};
        context->api().GetIntegerv(GL_VIEWPORT, value.data());
        check(value == std::array<GLint, 4>{0, 0, int(size), int(size)}, "Cache viewport");
        context->api().GetIntegerv(GL_COLOR_WRITEMASK, value.data());
        check(value == std::array<GLint, 4>{1, 1, 1, 1}, "Cache color mask");
        for (auto [state, expected] :
             std::initializer_list<std::pair<GLenum, GLint>>{{GL_DEPTH_WRITEMASK, 0},
                                                             {GL_DEPTH_FUNC, GL_LEQUAL},
                                                             {GL_BLEND_SRC_RGB, GL_ONE},
                                                             {GL_BLEND_DST_RGB, GL_ONE},
                                                             {GL_BLEND_SRC_ALPHA, GL_ONE},
                                                             {GL_BLEND_DST_ALPHA, GL_ONE},
                                                             {GL_BLEND_EQUATION_RGB, GL_FUNC_ADD},
                                                             {GL_BLEND_EQUATION_ALPHA, GL_FUNC_ADD}}) {
            context->api().GetIntegerv(state, value.data());
            check(value[0] == expected, "Cache native state value");
        }
        const auto clear = texture(2).read_mip_raw(0);
        bool exact = true;
        for (std::size_t i = 0; i < clear.size(); i += 4)
            exact = exact && clear[i] == 0 && clear[i + 1] == 127 && clear[i + 2] == 0 && clear[i + 3] == 255;
        check(exact, "Native .5 clear must truncate to127");
        pass.clear_color = {1, 0, 0, 1};
        raster.begin(pass, map, false);
        check(texture(2).read_mip_raw(0) == clear, "Conditional clear ignored false switch");
        pass.flags = 0;
        pass.blend_state = 0;
        raster.begin(pass, map, true);
        check(!gl.IsEnabled(GL_BLEND), "Native blend0 not disabled");
        pass.flags = 16;
        raster.begin(pass, map, false);
        context->api().GetIntegerv(GL_POLYGON_MODE, value.data());
        check(value[0] == GL_LINE && value[1] == GL_LINE, "Native wireframe flag");
        const auto rejects = [&](const MaterialCachePass &q, std::span<const GlMaterialCacheTarget> mapping) {
            GLint before = 0, after = 0;
            context->api().GetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &before);
            bool caught = false;
            try {
                raster.begin(q, mapping, false);
            } catch (const std::invalid_argument &) {
                caught = true;
            }
            context->api().GetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &after);
            check(caught && before == after, "Invalid cache pass changed framebuffer or was accepted");
        };
        auto q = pass;
        q.kind = MaterialCachePassKind::copy;
        rejects(q, map);
        q = pass;
        q.kind = MaterialCachePassKind::compress;
        rejects(q, map);
        q = pass;
        q.depth_state = 0;
        rejects(q, map);
        q = pass;
        q.blend_state = 1;
        rejects(q, map);
        q = pass;
        q.flags = 0x40000000;
        rejects(q, map);
        rejects(pass, {});
        q = pass;
        q.targets[1] = 9;
        rejects(q, map);
        q = pass;
        q.mip = 32;
        rejects(q, map);
        const std::array<GlMaterialCacheTarget, 1> wrong{{{6, &texture(2)}}};
        rejects(pass, wrong);
        const std::array<GlMaterialCacheTarget, 1> missing{{{5, nullptr}}};
        rejects(pass, missing);
        const std::array<GlMaterialCacheTarget, 2> duplicate{{{5, &texture(2)}, {9, &texture(2)}}};
        q = pass;
        q.targets[1] = 9;
        rejects(q, duplicate);
        context->check("Cache raster state controls");
        raster.end();
        return checked;
    }
};
} // namespace
int main(int argc, char **argv) {
    try {
        require(argc == 7 || argc == 8, "Usage: civ5_gl_material_cache_raster generation-input asset-root "
                           "blend-shaders normal-shaders original-geometry fresh-output [precision-probes]");
        const fs::path out(argv[6]);
        require(!fs::exists(out), "Fresh cache raster output required");
        const auto input = read(argv[1]), geometry = read(argv[5]);
        const auto clear_bytes = read(fs::path(argv[5]).parent_path() / "original-clear-colors.bin");
        require(clear_bytes.size() == 160, "Original clear controls extent");
        require(is_generation_input(input), "Cache raster needs primitive generation input");
        const auto generated = patches(input);
        auto assets = std::make_shared<const MaterialAssets>(authored(argv[2]));
        auto context = std::make_shared<GlContext>();
        GlMaterialTextures textures(context, assets);
        GlMaterialNormalRenderer normal(context);
        std::array<float, 384> coefficients{};
        assets->write_constants(2070.375f, coefficients);
        fs::create_directories(out);
        write(out / "original-normal-geometry.bin", geometry);
        write(out / "original-clear-colors.bin", clear_bytes);
        write(out / "computed-patches.bin", computed_patches(generated));
        write(out / "authored-coefficients.bin", coefficients);
        text(out / "blend-cache-vertex.glsl", material_blend_cache_vertex_shader());
        std::ostringstream report;
        report << context->description() << '\n';
        for (unsigned size : {64u, 128u, 256u, 512u}) {
            NativeCacheRaster native(argv[3], argv[4], *assets, size, geometry);
            Control gl(context, size, native.automatic);
            report << "size=" << size << "; automatic=" << native.automatic[0] << ',' << native.automatic[1]
                   << ',' << native.automatic[2] << "; stateChecks=" << gl.state_controls() << '\n';
            if (size == 64) {
                const std::array native_formats{D3DFMT_A16B16G16R16F, D3DFMT_A8R8G8B8, D3DFMT_R5G6B5};
                const std::array gl_formats{RenderTextureFormat::rgba16_float, RenderTextureFormat::rgba8,
                                            RenderTextureFormat::rgb565};
                for (unsigned format = 0; format < 3; ++format) {
                    auto texture = GlTexture::render_target(*context, 17, 9, 1, gl_formats[format]);
                    for (unsigned i = 0; i < 8; ++i) {
                        const auto name = "clear-" + std::to_string(format) + "-" + std::to_string(i);
                        MaterialCachePass pass{};
                        pass.kind = MaterialCachePassKind::height;
                        pass.targets = {1, 0, 0, 0};
                        pass.flags = 4;
                        std::memcpy(pass.clear_color.data(), clear_bytes.data() + i * 20, 16);
                        std::uint32_t native_packed;
                        std::memcpy(&native_packed, clear_bytes.data() + i * 20 + 16, 4);
                        const std::array<GlMaterialCacheTarget, 1> target{{{1, &texture}}};
                        gl.raster.begin(pass, target, false);
                        write(out / (name + "-gl.bin"), texture.read_mip_raw(0));
                        write(out / (name + "-d3d9.bin"),
                              native.clear_pixels(native_formats[format], native_packed));
                    }
                }
            }
            for (unsigned i = 0; i < generated.size(); ++i) {
                const auto name = std::to_string(size) + "-" + std::to_string(i);
                const auto &patch = generated[i];
                GlMaterialBlendRenderer renderer(context, patch.mesh, MaterialBlendCoordinates::d3d9_cache);
                const std::array<float, 2> offset{-.5f / size, .5f / size};
                gl.material(renderer, coefficients, textures.bind(patch.selected), offset);
                native.material(patch.mesh, coefficients, assets->selected_resources(patch.selected), offset);
                for (unsigned channel = 0; channel < 2; ++channel) {
                    write(out / (name + "-" + std::to_string(channel) + "-gl.bin"),
                          gl.texture(channel).read_mip_raw(0));
                    write(out / (name + "-" + std::to_string(channel) + "-d3d9.bin"), native.pixels(channel));
                }
                gl.normals(normal);
                native.normals();
                write(out / (name + "-2-gl.bin"), gl.texture(2).read_mip_raw(0));
                write(out / (name + "-2-d3d9.bin"), native.pixels(2));
                // Measured negative controls for both original integration mistakes.
                if (size == 256 && i == 0) {
                    gl.normals(normal, false);
                    write(out / "normal-center-negative.bin", gl.texture(2).read_mip_raw(0));
                    GlMaterialBlendRenderer wrong(context, patch.mesh);
                    gl.material(wrong, coefficients, textures.bind(patch.selected), offset);
                    write(out / "blend-coordinates-negative.bin", gl.texture(1).read_mip_raw(0));
                }
                // Independent shader precision check at the old 1e-6 bound,
                // before conversion into the original RGBA16F cache formats.
                gl.material(renderer, coefficients, textures.bind(patch.selected), offset, true);
                native.material(patch.mesh, coefficients, assets->selected_resources(patch.selected), offset,
                                true);
                for (unsigned channel = 0; channel < 2; ++channel) {
                    write(out / (name + "-" + std::to_string(channel) + "-float-gl.bin"),
                          gl.float_pixels(channel));
                    write(out / (name + "-" + std::to_string(channel) + "-float-d3d9.bin"),
                          native.pixels(channel, true));
                }
                // Normal-only isolation: controlled source replacement happens
                // AFTER direct production-chain outputs have been exported.
                const auto native_height = native.pixels(1);
                gl.gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
                context->api().TextureSubImage2D(gl.texture(1).id(), 0, 0, 0, size, size, GL_RGBA,
                                                 GL_HALF_FLOAT, native_height.data());
                require(gl.texture(1).read_mip_raw(0) == native_height, "Identical normal isolation input");
                gl.cache.generate_mips(MaterialCacheRole::height_temporary);
                gl.normals(normal);
                write(out / (name + "-normal-same-input-gl.bin"), gl.texture(2).read_mip_raw(0));
                if (argc == 8 && i == 2 && (size == 64 || size == 256)) {
                    fs::create_directories(out / "precision");
                    for (const auto &entry : fs::directory_iterator(argv[7])) {
                        if (entry.path().extension() != ".glsl")
                            continue;
                        const auto source = read(entry.path());
                        const auto binary = read(fs::path(argv[7]) / (entry.path().stem().string() + ".bin"));
                        require(binary.size() % 4 == 0, "Aligned material probe bytecode");
                        std::vector<DWORD> words(binary.size() / 4);
                        std::memcpy(words.data(), binary.data(), binary.size());
                        Com<IDirect3DPixelShader9> prefix;
                        hr(native.device->CreatePixelShader(words.data(), &prefix.value), "Material prefix PS");
                        MaterialPrecisionProbe probe(*context, std::string_view(
                            reinterpret_cast<const char *>(source.data()), source.size()));
                        gl.material(renderer, coefficients, textures.bind(patch.selected), offset, true);
                        probe.draw(coefficients, offset, size, static_cast<GLsizei>(patch.mesh.indices().size()),
                                   textures.bind(patch.selected));
                        native.material(patch.mesh, coefficients, assets->selected_resources(patch.selected),
                                        offset, true, prefix.value);
                        const auto key = name + "-" + entry.path().stem().string();
                        write(out / "precision" / (key + "-gl.bin"), gl.float_pixels(0));
                        write(out / "precision" / (key + "-d3d9.bin"), native.pixels(0, true));
                        if (entry.path().stem().string().starts_with("full")) {
                            write(out / "precision" / (key + "-height-gl.bin"), gl.float_pixels(1));
                            write(out / "precision" / (key + "-height-d3d9.bin"), native.pixels(1, true));
                        }
                    }
                    report << "PASS executed material precision observations " << name << '\n';
                }
                report << "PASS executed original cache material/normal " << name << '\n';
            }
        }
        text(out / "execution.txt", report.str());
        std::cout << report.str();
    } catch (const std::exception &e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
