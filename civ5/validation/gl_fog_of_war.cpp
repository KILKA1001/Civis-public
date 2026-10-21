#include "civ5/graphics/gl_fog_of_war_map.hpp"
#include "fog_of_war_native.hpp"
#include <iostream>
#include <limits>

using namespace fow_validation;
using namespace civ5::graphics;
namespace fs = std::filesystem;
namespace {
// D3DX expands the unused texels in the final 2x2/1x1 BC block. Those
// selector bits cannot be sampled. It also substitutes equivalent endpoints
// for constant 1x1 blocks. Compare that one actual color as exact rational
// channel values; full source storage is checked separately on the GL side.
Bytes active_bc(Bytes bytes, unsigned width, unsigned height, bool bc1) {
    if (width >= 4 && height >= 4)
        return bytes;
    require(width <= 4 && height <= 4 && bytes.size() == (bc1 ? 8 : 16), "FOW terminal BC block");
    const unsigned color = bc1 ? 4 : 12;
    if (width == 1 && height == 1) {
        const unsigned base = color - 4, lo = bytes[base] | (unsigned(bytes[base + 1]) << 8),
                       hi = bytes[base + 2] | (unsigned(bytes[base + 3]) << 8), selector = bytes[color] & 3;
        const auto rgb = [](unsigned value) {
            const unsigned r = value >> 11, g = (value >> 5) & 63, b = value & 31;
            return std::array<unsigned, 3>{(r << 3) | (r >> 2), (g << 2) | (g >> 4), (b << 3) | (b >> 2)};
        };
        const auto a = rgb(lo), b = rgb(hi);
        std::array<unsigned, 4> value{};
        for (unsigned channel = 0; channel < 3; ++channel) {
            if (selector < 2)
                value[channel] = 6 * (selector ? b[channel] : a[channel]);
            else if (!bc1 || lo > hi)
                value[channel] =
                    2 * ((selector == 2 ? 2 : 1) * a[channel] + (selector == 2 ? 1 : 2) * b[channel]);
            else
                value[channel] = selector == 2 ? 3 * (a[channel] + b[channel]) : 0;
        }
        value[3] = 210 * 255;
        if (bc1 && lo <= hi && selector == 3)
            value[3] = 0;
        if (!bc1) {
            const unsigned index = bytes[2] & 7, x = bytes[0], y = bytes[1];
            if (index < 2)
                value[3] = 210 * (index ? y : x);
            else if (x > y)
                value[3] = 30 * ((8 - index) * x + (index - 1) * y);
            else if (index < 6)
                value[3] = 42 * ((6 - index) * x + (index - 1) * y);
            else
                value[3] = index == 6 ? 0 : 210 * 255;
        }
        Bytes result(sizeof(value));
        std::memcpy(result.data(), value.data(), result.size());
        return result;
    }
    for (unsigned y = 0; y < 4; ++y)
        for (unsigned x = 0; x < 4; ++x)
            if (x >= width || y >= height) {
                const unsigned bit = (y * 4 + x) * 2;
                bytes[color + bit / 8] &= static_cast<std::uint8_t>(~(3u << (bit % 8)));
                if (!bc1)
                    for (unsigned k = 0; k < 3; ++k) {
                        const unsigned alpha = (y * 4 + x) * 3 + k;
                        bytes[2 + alpha / 8] &= static_cast<std::uint8_t>(~(1u << (alpha % 8)));
                    }
            }
    return bytes;
}
} // namespace
int wmain(int argc, wchar_t **argv) {
    try {
        require(argc == 7 || argc == 8, "Usage: civ5_gl_fog_of_war original-shaders original-frame cpp-frame "
                                        "assets quality fresh-output [rgba16f/rgba32f]");
        const fs::path shaders(argv[1]), original(argv[2]), cpp(argv[3]), asset_folder(argv[4]), out(argv[6]);
        const unsigned quality = std::stoul(argv[5]);
        require(quality <= 2 && !fs::exists(out), "FOW GPU quality/fresh output");
        const bool half = argc == 8 && std::wstring_view(argv[7]) == L"rgba16f";
        const bool single = argc == 8 && std::wstring_view(argv[7]) == L"rgba32f";
        require(argc == 7 || half || single, "FOW explicit scene format");
        constexpr unsigned width = 384, height = 256;
        auto context = std::make_shared<GlContext>();
        GlRenderApi gl(*context);
        LooseSource source(asset_folder);
        const auto assets = FogOfWarAssets::load(quality, [&](const char *path) {
            const auto result = source.find(AssetKey(path));
            require(std::holds_alternative<AssetHandle>(result), "Original FOW asset");
            return std::get<AssetHandle>(result);
        });
        GlFogOfWarMap map(context, assets);
        auto color = GlTexture::render_target(*context, width, height, 1,
                                              single ? RenderTextureFormat::rgba32_float
                                              : half ? RenderTextureFormat::rgba16_float
                                                     : RenderTextureFormat::rgba8);
        auto depth =
            GlTexture::render_target(*context, width, height, 1, RenderTextureFormat::depth24_stencil8);
        NativeFow native(shaders, original, asset_folder, quality, width, height, half, single);
        fs::create_directories(out);
        text(out / "device.txt", context->description());
        for (unsigned index = 0; index < 6; ++index) {
            text(out / ("technique" + std::to_string(index) + "-vs.glsl"), fog_of_war_vertex_shader(index));
            text(out / ("technique" + std::to_string(index) + "-ps.glsl"), fog_of_war_fragment_shader(index));
        }
        const auto geometry = fog_of_war_geometry();
        const auto geometry_exact = [&](const char *name, const auto &values) {
            const auto actual = std::span<const std::uint8_t>(
                reinterpret_cast<const std::uint8_t *>(values.data()), sizeof(values));
            const auto expected = read(original / name);
            require(expected.size() == actual.size() &&
                        std::equal(expected.begin(), expected.end(), actual.begin()),
                    "Native/C++ FOW geometry bytes");
            write(out / name, actual);
        };
        geometry_exact("1-index.bin", geometry.mask_indices);
        geometry_exact("2-vertex.bin", geometry.mask_vertices);
        geometry_exact("3-index.bin", geometry.layer_indices);
        geometry_exact("4-vertex.bin", geometry.layer_vertices);
        const auto expected = load_frame(original), frame = load_frame(cpp);
        native.render(expected);
        const auto clear_scene = [&] {
            GLuint fbo = 0;
            gl.CreateFramebuffers(1, &fbo);
            gl.NamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT0, color.id(), 0);
            gl.NamedFramebufferTexture(fbo, GL_DEPTH_STENCIL_ATTACHMENT, depth.id(), 0);
            const GLenum buffer = GL_COLOR_ATTACHMENT0;
            gl.NamedFramebufferDrawBuffers(fbo, 1, &buffer);
            require(gl.CheckNamedFramebufferStatus(fbo, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
                    "FOW validation scene FBO");
            gl.ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
            gl.DepthMask(GL_TRUE);
            gl.Disable(GL_SCISSOR_TEST);
            const float sentinel[]{187 / 255.f, 51 / 255.f, 119 / 255.f, 0}, one = 1;
            gl.ClearNamedFramebufferfv(fbo, GL_COLOR, 0, sentinel);
            gl.ClearNamedFramebufferfv(fbo, GL_DEPTH, 0, &one);
            gl.DeleteFramebuffers(1, &fbo);
        };
        clear_scene();
        map.render(frame, color, depth);
        const auto read_rgba = [&](const GlTexture &texture) {
            const bool is_half = texture.plan().internal_format == GL_RGBA16F;
            const bool is_single = texture.plan().internal_format == GL_RGBA32F;
            const auto &level = texture.levels()[0];
            Bytes bytes(level.width * level.height * (is_single ? 16 : is_half ? 8 : 4));
            context->api().GetTextureImage(texture.id(), 0, GL_RGBA,
                                           is_single ? GL_FLOAT
                                           : is_half ? GL_HALF_FLOAT
                                                     : GL_UNSIGNED_BYTE,
                                           bytes.size(), bytes.data());
            context->check("FOW RGBA readback");
            return bytes;
        };
        write(out / "mask-d3d9.rgba", native.pixels(true));
        write(out / "mask-gl.rgba", read_rgba(map.mask()));
        write(out / "scene-d3d9.rgba", native.pixels(false));
        write(out / "scene-gl.rgba", read_rgba(color));
        unsigned guards = 0;
        const auto baseline_mask = read_rgba(map.mask()), baseline_color = read_rgba(color),
                   baseline_depth = depth.read_level_raw(0, 0);
        const auto native_mask = native.pixels(true), native_color = native.pixels(false);
        unsigned native_callbacks = 0;
        native.render(expected, false, [&](IDirect3DTexture9 *mask) {
            require(mask == native.mask.value && native.pixels(true) == native_mask,
                    "Native scene callback ran before complete mask");
            hr(native.device->SetRenderState(D3DRS_SCISSORTESTENABLE, TRUE), "Poison native scissor");
            hr(native.device->SetRenderState(D3DRS_STENCILENABLE, TRUE), "Poison native stencil");
            ++native_callbacks;
        });
        require(native_callbacks == 1 && native.pixels(false) == native_color,
                "Native scene callback changed FOW pass results");
        unsigned callbacks = 0;
        bool recursion_rejected = false;
        map.render(frame, color, depth, [&](const GlTexture &mask) {
            require(read_rgba(mask) == baseline_mask, "Scene callback ran before complete mask");
            try {
                map.render(frame, color, depth);
            } catch (const std::invalid_argument &) {
                recursion_rejected = true;
            }
            clear_scene();
            // Scene rendering may alter all these states. The subsequent
            // original cloud passes must establish their state again.
            gl.Enable(GL_STENCIL_TEST);
            gl.Enable(GL_SCISSOR_TEST);
            gl.Enable(GL_RASTERIZER_DISCARD);
            gl.ClipControl(GL_UPPER_LEFT, GL_NEGATIVE_ONE_TO_ONE);
            gl.DepthRange(.25, .75);
            ++callbacks;
        });
        require(callbacks == 1 && recursion_rejected && read_rgba(color) == baseline_color &&
                    depth.read_level_raw(0, 0) == baseline_depth,
                "Scene callback changed FOW pass results");
        bool callback_exception = false;
        try {
            map.render(frame, color, depth,
                       [&](const GlTexture &) { throw std::runtime_error("Controlled scene failure"); });
        } catch (const std::runtime_error &) {
            callback_exception = true;
        }
        GLint bound_framebuffer = -1;
        context->api().GetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &bound_framebuffer);
        require(callback_exception && bound_framebuffer == 0,
                "Throwing scene callback left the FOW framebuffer bound");
        clear_scene();
        map.render(frame, color, depth);
        require(read_rgba(color) == baseline_color && depth.read_level_raw(0, 0) == baseline_depth,
                "FOW owner failed to recover after scene exception");
        const auto rejects = [&](auto action) {
            bool caught = false;
            try {
                action();
            } catch (const std::invalid_argument &) {
                caught = true;
            }
            require(caught, "Missing FOW frame guard");
            context->check("Rejected FOW frame leaves GL clean");
            require(read_rgba(map.mask()) == baseline_mask && read_rgba(color) == baseline_color &&
                        depth.read_level_raw(0, 0) == baseline_depth,
                    "Rejected FOW frame changed target pixels");
            ++guards;
        };
        rejects([&] { GlFogOfWarMap invalid(nullptr, assets); });
        rejects([&] { map.render(frame, color, color); });
        rejects([&] { map.render(frame, map.mask(), depth); });
        auto small_depth =
            GlTexture::render_target(*context, width / 2, height, 1, RenderTextureFormat::depth24_stencil8);
        rejects([&] { map.render(frame, color, small_depth); });
        rejects([&] { map.render({}, color, depth); });
        auto bad = frame;
        bad.passes[0].begin = 1;
        rejects([&] { map.render(bad, color, depth); });
        bad = frame;
        bad.passes[0].end = frame.draws.size() + 1;
        rejects([&] { map.render(bad, color, depth); });
        bad = frame;
        bad.passes[0].kind = FogOfWarPassKind::clouds;
        rejects([&] { map.render(bad, color, depth); });
        bad = frame;
        bad.passes[0].clear_color[0] = std::numeric_limits<float>::infinity();
        rejects([&] { map.render(bad, color, depth); });
        if (!frame.draws.empty()) {
            const auto index = frame.draws.size() - 1;
            bad = frame;
            bad.draws[index].geometry = 7;
            rejects([&] { map.render(bad, color, depth); });
            bad = frame;
            bad.draws[index].selector = 7;
            rejects([&] { map.render(bad, color, depth); });
            bad = frame;
            bad.draws[index].constants.pop_back();
            rejects([&] { map.render(bad, color, depth); });
            bad = frame;
            bad.draws[index].constants.back()[3] = std::numeric_limits<float>::quiet_NaN();
            rejects([&] { map.render(bad, color, depth); });
            bad = frame;
            bad.draws.push_back(frame.draws[0]);
            rejects([&] { map.render(bad, color, depth); });
        }
        const bool cull_control = frame.passes.size() == 3 && frame.passes[1].end > frame.passes[1].begin;
        if (cull_control) {
            const auto without_culling = [](FogOfWarFrame value) {
                const auto begin = value.passes[1].begin, end = value.passes[1].end, count = end - begin;
                value.draws.erase(value.draws.begin() + begin, value.draws.begin() + end);
                value.passes[1].end = begin;
                value.passes[2].begin -= count;
                value.passes[2].end -= count;
                return value;
            };
            // Explicit negative control, after the complete unmodified frame.
            // Both inputs are derived from their own independently loaded queues.
            native.render(without_culling(expected));
            clear_scene();
            map.render(without_culling(frame), color, depth);
            write(out / "scene-no-cull-d3d9.rgba", native.pixels(false));
            write(out / "scene-no-cull-gl.rgba", read_rgba(color));
        }
        // Inspect the actual sampled resources after all draws. On this
        // device a compressed readback before drawing changes subsequent
        // sampling; the separate probe retains that reproducible observation.
        unsigned levels = 0, raw_levels = 0;
        for (unsigned i = 0; i < map.clouds().size(); ++i) {
            require(native.clouds[i]->GetLevelCount() == assets.clouds()[i].mip_count(),
                    "Original cloud mip count");
            for (unsigned mip = 0; mip < assets.clouds()[i].mip_count(); ++mip) {
                const auto dx = native.texture_level(i, mip), actual = map.clouds()[i].read_level_raw(0, mip);
                write(out / ("cloud" + std::to_string(i) + "-mip" + std::to_string(mip) + "-d3d9.bc"), dx);
                write(out / ("cloud" + std::to_string(i) + "-mip" + std::to_string(mip) + "-gl.bc"), actual);
                const auto &level = assets.clouds()[i].levels()[mip];
                const auto payload = assets.clouds()[i].payload(0, mip);
                require(actual.size() == payload.size() &&
                            std::equal(actual.begin(), actual.end(), payload.begin()),
                        "GL authored FOW mip bytes");
                const bool bc1 = assets.clouds()[i].format() == DdsFormat::dxt1;
                require(active_bc(dx, level.width, level.height, bc1) ==
                            active_bc(actual, level.width, level.height, bc1),
                        "D3DX/OpenGL compressed actual texels");
                raw_levels += dx == actual;
                ++levels;
            }
        }
        // Exercise the owner-level recovery contract after the complete
        // authored-resource comparison. Scene targets are caller-owned, so
        // recreate them on the replacement context as the game would after a
        // device reset, then submit the retained CPU frame again.
        bool physical_recovery = false;
        bool physical_recovery_bit_exact = false;
        {
            auto recovered_context = std::make_shared<GlContext>();
            GlRenderApi recovered_gl(*recovered_context);
            auto recovered_color = GlTexture::render_target(
                *recovered_context, width, height, 1,
                single ? RenderTextureFormat::rgba32_float
                : half ? RenderTextureFormat::rgba16_float
                       : RenderTextureFormat::rgba8);
            auto recovered_depth = GlTexture::render_target(
                *recovered_context, width, height, 1, RenderTextureFormat::depth24_stencil8);
            const auto clear_recovered_scene = [&] {
                recovered_context->api();
                GLuint fbo = 0;
                recovered_gl.CreateFramebuffers(1, &fbo);
                recovered_gl.NamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT0, recovered_color.id(), 0);
                recovered_gl.NamedFramebufferTexture(fbo, GL_DEPTH_STENCIL_ATTACHMENT, recovered_depth.id(), 0);
                const GLenum buffer = GL_COLOR_ATTACHMENT0;
                recovered_gl.NamedFramebufferDrawBuffers(fbo, 1, &buffer);
                require(recovered_gl.CheckNamedFramebufferStatus(fbo, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
                        "FOW recovery scene FBO");
                recovered_gl.ColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
                recovered_gl.DepthMask(GL_TRUE);
                recovered_gl.Disable(GL_SCISSOR_TEST);
                const float sentinel[]{187 / 255.f, 51 / 255.f, 119 / 255.f, 0}, one = 1;
                recovered_gl.ClearNamedFramebufferfv(fbo, GL_COLOR, 0, sentinel);
                recovered_gl.ClearNamedFramebufferfv(fbo, GL_DEPTH, 0, &one);
                recovered_gl.DeleteFramebuffers(1, &fbo);
                recovered_context->check("FOW recovery scene clear");
            };
            const auto read_recovered = [&](const GlTexture &texture) {
                recovered_context->api();
                const bool recovered_half = texture.plan().internal_format == GL_RGBA16F;
                const bool recovered_single = texture.plan().internal_format == GL_RGBA32F;
                const auto &level = texture.levels()[0];
                Bytes bytes(level.width * level.height *
                            (recovered_single ? 16 : recovered_half ? 8 : 4));
                recovered_context->api().GetTextureImage(
                    texture.id(), 0, GL_RGBA,
                    recovered_single ? GL_FLOAT
                    : recovered_half ? GL_HALF_FLOAT
                                      : GL_UNSIGNED_BYTE,
                    bytes.size(), bytes.data());
                recovered_context->check("FOW recovery RGBA readback");
                return bytes;
            };
            map.rebuild_context(recovered_context, assets);
            clear_recovered_scene();
            map.render(frame, recovered_color, recovered_depth);
            physical_recovery = true;
            physical_recovery_bit_exact =
                read_recovered(map.mask()) == baseline_mask &&
                read_recovered(recovered_color) == baseline_color &&
                recovered_depth.read_level_raw(0, 0) == baseline_depth;
            require(physical_recovery_bit_exact,
                    "FOW context recovery changed the complete submitted frame");
        }
        text(out / "report.json",
             "{\"schema\":\"civ5-fow-gpu-capture-v1\",\"width\":384,\"height\":256,\"maskExtent\":" +
                 std::to_string(assets.state().mask_extent) +
                 ",\"originalCompressedActiveMipsExact\":" + std::to_string(levels) +
                 ",\"originalCompressedRawMipsExact\":" + std::to_string(raw_levels) +
                 ",\"guards\":" + std::to_string(guards) + ",\"sceneFormat\":\"" +
                 (single ? "rgba32f"
                  : half ? "rgba16f"
                         : "rgba8") +
                 "\"" + ",\"sceneCallbacks\":" + std::to_string(callbacks) +
                 ",\"nativeSceneCallbacks\":" + std::to_string(native_callbacks) +
                 ",\"sceneExceptionRecovery\":true,\"recursiveSceneRejected\":true" +
                 ",\"cullingNegativeControl\":" + (cull_control ? "true" : "false") +
                 ",\"executedDraws\":" + std::to_string(expected.draws.size()) +
                 ",\"originalCompleteDrawShadersExecuted\":true,\"originalGameProcessAccess\":false,"
                 "\"physicalGlRecoveryExecuted\":" + (physical_recovery ? "true" : "false") +
                 ",\"physicalGlRecoverySceneBitExact\":" +
                 (physical_recovery_bit_exact ? "true" : "false") +
                 ",\"fullGraphicsParity\":false}\n");
        std::cout << "PASS FOW D3D9/OpenGL GPU capture and " << levels << " authored compressed mips\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
