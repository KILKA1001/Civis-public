#pragma once
// Direct animated ocean on the existing independent LAND/color/depth targets.
// Geometry and all source textures persist; only time and constants advance.
namespace {
void render_water_animation(std::shared_ptr<GlContext> context, GlWaterWorldRenderer &water,
                            water_validation::NativeWater &native, SceneTarget &target,
                            NativeSceneTarget &native_target, WaterSurfaceSettings settings,
                            const WaterSurfaceFrame &frame, const WaterSurfaceView &view,
                            GlWaterSurfaceInputs scene, std::array<IDirect3DBaseTexture9 *, 14> images,
                            const NativeWorld &original, const fs::path &folder,
                            bool produced_current = false, bool computed_frame = false,
                            unsigned view_count = 1) {
    const auto out = folder / "water-animation";
    fs::create_directory(out);
    WaterFrameClock clock;
    std::vector<double> samples;
    std::vector<float> phases;
    std::vector<WaterConstantBlock> constant_blocks;
    const auto rgba8 = [&](const std::vector<float> &pixels) {
        std::vector<std::uint8_t> bytes;
        bytes.reserve(pixels.size());
        for (std::size_t i = 0; i < pixels.size(); ++i) {
            const float v = std::clamp(pixels[i], 0.f, 1.f);
            const float display = i % 4 == 3       ? v
                                  : v <= .0031308f ? 12.92f * v
                                                   : 1.055f * std::pow(v, 1.f / 2.4f) - .055f;
            bytes.push_back(static_cast<std::uint8_t>(std::lround(display * 255.f)));
        }
        return bytes;
    };
    for (unsigned tick = 0; tick <= 240; ++tick) {
        const double sampled = double(tick) / 60.;
        samples.push_back(sampled);
        clock.tick(sampled);
        settings = advance_water_surface(settings, clock.delta);
        phases.push_back(settings.river_phase);
        phases.push_back(settings.water_phase);
        if (tick % 4 != 0)
            continue;
        std::vector<WaterConstantBlock> view_constants;
        for (unsigned index = 0; index < view_count; ++index) {
            const auto c = computed_frame ? water.constants(index, settings, frame, view)
                                          : water.constants(settings, frame, view);
            if (original.distance_fog)
                check(c == original.distance_fog->animation.at((tick / 4) * view_count + index),
                      "Original animated water fog constants differ");
            view_constants.push_back(c);
            constant_blocks.push_back(c);
        }
        // Opaque water occupies the same plane and coverage on every frame.
        // LEQUAL permits replacement over its prior depth; LAND is retained.
        target.gl.BindFramebuffer(GL_FRAMEBUFFER, target.fbo);
        for (unsigned index = 0; index < view_count; ++index)
            water.draw(index, view_constants[index], scene, 1280, 960);
        const auto pixels = target.pixels();
        water_validation::save(out / (std::to_string(tick) + ".rgba8"), rgba8(pixels));
        if (tick % 60 == 0) {
            blend_validation::write(out / (std::to_string(tick) + "-gl.f32"), pixels);
            hr(native.device->BeginScene(), "Animated native ocean begin");
            for (const auto &draw : water.draws()) {
                images[7] = original.heights.at(draw.window).value;
                native.bind(3,
                            original.distance_fog
                                ? original.distance_fog->animation.at((tick / 4) * view_count + draw.view)
                                : view_constants.at(draw.view),
                            images);
                native.draw(water.mesh(), draw.first_index, draw.triangle_count);
            }
            hr(native.device->EndScene(), "Animated native ocean end");
            blend_validation::write(out / (std::to_string(tick) + "-d3d9.f32"), native_target.pixels());
            std::cout << "PASS water animation checkpoint " << tick << "/240\n" << std::flush;
        }
    }
    water_validation::save(out / "clock-samples.f64", samples);
    water_validation::save(out / "phases.f32", phases);
    water_validation::save(out / "constants.bin", constant_blocks);
    blend_validation::text(
        out / "scope.txt",
        "241 explicit host clock samples at 60 Hz;61 rendered frames at15 Hz;5 "
        "full native/GL comparisons.\n"
        "Original timer policy, constructor rates and framewise phase "
        "accumulation.\n"
        "Color/depth targets retain the same opaque ocean plane; camera and "
        "world are stationary.\n"
        "RGBA8 frames use standard linear-to-sRGB transfer for display only.\n"
        "Reflection/LUT/frame/raster producers remain controlled; full "
        "graphics parity is false.\n" +
            std::string(produced_current
                            ? "Current input is each API's directly generated key5d render target.\n"
                            : "Current input remains an explicit neutral control.\n"));
    context->check("Complete direct water animation");
}
} // namespace
