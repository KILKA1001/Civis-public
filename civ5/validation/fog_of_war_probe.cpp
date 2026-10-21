// Full original cloud pair, unblended per-layer float diagnostic.
// This deliberately uses a white mask to isolate authored cloud sampling.
#include "civ5/graphics/gl_fog_of_war.hpp"
#include "civ5/graphics/gl_texture.hpp"
#include "fog_of_war_native.hpp"
#include <iostream>
using namespace fow_validation;
using namespace civ5::graphics;
namespace fs = std::filesystem;
int wmain(int argc, wchar_t **argv) {
    try {
        require(argc == 6 || argc == 8 || argc == 9,
                "Expected original shaders, original frame, cpp frame, assets, fresh output "
                "[rgba8/rgba16f/rgba32f blend/opaque [none/gl/d3d9]]");
        const fs::path shaders(argv[1]), original(argv[2]), cpp(argv[3]), assets(argv[4]), out(argv[5]);
        require(!fs::exists(out), "Fresh FOW probe output");
        constexpr unsigned width = 384, height = 256;
        const std::wstring format = argc >= 8 ? argv[6] : L"rgba32f";
        const bool blended = argc >= 8 && std::wstring_view(argv[7]) == L"blend";
        const std::wstring read_mode = argc == 9 ? argv[8] : L"none";
        require(read_mode == L"none" || read_mode == L"gl" || read_mode == L"d3d9", "FOW probe read mode");
        require(argc < 8 || blended || std::wstring_view(argv[7]) == L"opaque",
                "FOW explicit probe blend mode");
        require(format == L"rgba8" || format == L"rgba16f" || format == L"rgba32f", "FOW probe format");
        const bool half = format == L"rgba16f", single = format == L"rgba32f";
        auto context = std::make_shared<GlContext>();
        GlRenderApi gl(*context);
        GlFogOfWarRenderer renderer(context);
        NativeFow native(shaders, original, assets, 2, width, height, half, single);
        std::vector<GlTexture> clouds;
        for (const char *path : FogOfWarAssets::cloud_paths)
            clouds.emplace_back(*context, DdsData::parse(read(assets / normalize_asset_name(path))));
        if (read_mode != L"none")
            for (unsigned i = 0; i < 3; ++i)
                for (unsigned mip = 0; mip < 11; ++mip) {
                    const auto bytes =
                        read_mode == L"gl" ? clouds[i].read_level_raw(0, mip) : native.texture_level(i, mip);
                    require(!bytes.empty(), "FOW probe mip readback extent");
                }
        const civ5::assets::Rgba8Level white{1, 1, {255, 255, 255, 255}};
        auto mask = GlTexture::rgba8(*context, std::span(&white, 1));
        GLuint target = 0, fbo = 0;
        context->api().CreateTextures(GL_TEXTURE_2D, 1, &target);
        context->api().TextureStorage2D(target, 1,
                                        single ? GL_RGBA32F
                                        : half ? GL_RGBA16F
                                               : GL_RGBA8,
                                        width, height);
        gl.CreateFramebuffers(1, &fbo);
        gl.NamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT0, target, 0);
        auto depth =
            GlTexture::render_target(*context, width, height, 1, RenderTextureFormat::depth24_stencil8);
        gl.NamedFramebufferTexture(fbo, GL_DEPTH_STENCIL_ATTACHMENT, depth.id(), 0);
        const GLenum buffer = GL_COLOR_ATTACHMENT0;
        gl.NamedFramebufferDrawBuffers(fbo, 1, &buffer);
        require(gl.CheckNamedFramebufferStatus(fbo, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
                "FOW float probe FBO");
        const auto expected = load_frame(original), frame = load_frame(cpp);
        fs::create_directories(out);
        for (unsigned layer = 0; layer < 4; ++layer) {
            const auto select = [&](const FogOfWarFrame &input) {
                FogOfWarFrame probe;
                probe.passes = {{FogOfWarPassKind::mask, 0, 0, {1, 1, 1, 1}},
                                {FogOfWarPassKind::culling, 0, 0, {}},
                                {FogOfWarPassKind::clouds, 0, 0, {}}};
                for (const auto &draw : input.draws)
                    if (draw.geometry >= 2 && draw.geometry <= 4 &&
                        (layer == 3 || draw.geometry == layer + 2))
                        probe.draws.push_back(draw);
                probe.passes[2].end = probe.draws.size();
                return probe;
            };
            native.render(select(expected), !blended);
            gl.BindFramebuffer(GL_FRAMEBUFFER, fbo);
            gl.Viewport(0, 0, width, height);
            gl.ClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE);
            gl.DepthRange(0, 1);
            for (auto state : {GL_DEPTH_TEST, GL_STENCIL_TEST, GL_BLEND, GL_SCISSOR_TEST, GL_DITHER,
                               GL_FRAMEBUFFER_SRGB, GL_RASTERIZER_DISCARD})
                gl.Disable(state);
            gl.Enable(GL_CULL_FACE);
            gl.FrontFace(GL_CCW);
            gl.CullFace(GL_BACK);
            gl.Enable(GL_DEPTH_TEST);
            gl.DepthFunc(GL_LEQUAL);
            gl.DepthMask(GL_TRUE);
            const float depth_one = 1;
            gl.ClearNamedFramebufferfv(fbo, GL_DEPTH, 0, &depth_one);
            gl.DepthMask(GL_FALSE);
            if (blended) {
                gl.Enable(GL_BLEND);
                gl.BlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
                gl.BlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE);
            }
            gl.ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            const float sentinel[]{187 / 255.f, 51 / 255.f, 119 / 255.f, 0};
            gl.ClearNamedFramebufferfv(fbo, GL_COLOR, 0, sentinel);
            const auto selected = select(frame);
            for (const auto &draw : selected.draws)
                renderer.draw(draw, width, height, clouds[draw.geometry - 2].id(), mask.id());
            Bytes bytes(width * height * (single ? 16 : half ? 8 : 4));
            context->api().GetTextureImage(target, 0, GL_RGBA,
                                           single ? GL_FLOAT
                                           : half ? GL_HALF_FLOAT
                                                  : GL_UNSIGNED_BYTE,
                                           bytes.size(), bytes.data());
            context->check("Full cloud float diagnostic");
            const std::string suffix = single ? "f32" : half ? "f16" : "rgba";
            write(out / ("layer" + std::to_string(layer) + "-gl." + suffix), bytes);
            write(out / ("layer" + std::to_string(layer) + "-d3d9." + suffix), native.pixels(false));
        }
        gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
        gl.DeleteFramebuffers(1, &fbo);
        context->api().DeleteTextures(1, &target);
        text(out / "report.json", "{\"schema\":\"civ5-fow-readback-probe-v1\",\"readMode\":\"" +
                                      std::string(read_mode.begin(), read_mode.end()) + "\",\"format\":\"" +
                                      std::string(format.begin(), format.end()) +
                                      "\",\"blended\":" + (blended ? "true" : "false") +
                                      ",\"completeOriginalCloudPair\":true,\"whiteMaskControl\":true}\n");
        std::cout << "PASS full cloud shader float diagnostic captures\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
