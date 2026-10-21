#pragma once
#include <limits>
#include <ranges>
#include <stdexcept>

namespace {
// Static scene controls retain their existing submission order. The scheduled
// adapter supplies the original per-view selection after actual completion.
template <class Land> auto scene_land_indices(Land &land, const MaterialWorld &world, unsigned view) {
    if constexpr (requires { land.scene_indices(view); })
        return land.scene_indices(view);
    else
        return std::views::iota(std::size_t{0}, world.cache().patches.size());
}
template <class Land> auto scene_land_images(Land &land, NativeWorld &original, std::size_t index) {
    if constexpr (requires { land.scene_images(index); })
        return land.scene_images(index);
    else
        return original.images(index, false);
}
template <class Land>
void scene_land_prepare(Land &land, const CameraFrameState &camera, const fs::path &output) {
    if constexpr (requires { land.prepare_scene(camera, output); })
        land.prepare_scene(camera, output);
}
template <class Land> void scene_land_finish(Land &land) {
    if constexpr (requires { land.finish_scene(); })
        land.finish_scene();
}
template <class Land> void scene_land_reset(Land &land) {
    if constexpr (requires { land.reset_scene(); })
        land.reset_scene();
}

template <class Land> void scene_land_camera_events(Land &land, std::span<const CameraMotionEvent> events) {
    if constexpr (requires { land.camera_events(events); })
        land.camera_events(events);
}

// The production renderer owns all scheduling, resource leases and workers.
// This adapter runs independent original D3D9 shader controls for each actual
// generated job and binds their matching caches to the complete scene.
class ScheduledMaterialScene {
  public:
    ScheduledMaterialScene(GlMaterialWorldRenderer &renderer, NativeWorld &original,
                           std::shared_ptr<const MaterialWorld> world,
                           const GlMaterialWorldLighting &lighting,
                           const material_preview_settings::Profile &profile)
        : renderer_(renderer), original_(original), world_(std::move(world)), lighting_(lighting),
          reader_(original.cache.device.value), profile_(profile) {
        const auto &quality = profile_.configured.material.quality;
        check(quality.base_size == 64 && quality.cache_mode == 0 && quality.compression == 0,
              "Configured full scene requires ordinary diffuse material settings");
        for (unsigned role : {0u, 1u, 3u, 4u}) {
            D3DSURFACE_DESC descriptor{};
            hr(original_.cache.texture(renderer_.source_bindings().textures[role])
                   ->GetLevelDesc(0, &descriptor),
               "Configured original temporary target descriptor");
            check(descriptor.Width == quality.temporary_size && descriptor.Height == quality.temporary_size &&
                      descriptor.Format == (role < 2 ? D3DFMT_A16B16G16R16F : D3DFMT_A8R8G8B8),
                  "Original active diffuse target quality/format");
        }
        const auto field = world_->terrain().height_field();
        for (int x = 0; x < field.domain().texture_columns(); ++x)
            for (int y = 0; y < field.domain().texture_rows(); ++y) {
                const auto slot = x + 16 * y;
                auto bytes = field.extract_a8_texture(x, y);
                // Water can require a height window before any LAND cache in
                // that window is ready. Height sources are independent owners.
                original_.heights.emplace(slot, original_.cache.shadows.texture(1024, bytes, true));
                windows_.emplace(slot, std::move(bytes));
            }
        reset_scene();
    }
    void reset_scene() {
        check(!open_, "Scheduled scene reset before frame completion");
        renderer_.invalidate_generated_resources();
        const auto &quality = profile_.configured.material.quality;
        renderer_.enable_scheduling(std::span<const std::uint32_t, 4>(quality.pool_counts.data(), 4),
                                    {64, 0, 0, 0, X87Precision::double_precision},
                                    {quality.temporary_size, quality.cache_size});
        camera_motion_ = {};
        camera_event_batches_ = 0;
        initial_cull_ = false;
        higher_.clear();
        original_.finals.clear();
        identities_.clear();
        for (std::size_t i = 0; i < world_->cache().patches.size(); ++i)
            identities_.push_back(&renderer_.patch_state(i));
        for (auto &view : draws_)
            view.clear();
        epoch_frame_ = 0;
        ++epoch_;
    }
    void camera_events(std::span<const CameraMotionEvent> events) {
        check(!open_, "Scene camera events must precede material frame preparation");
        camera_motion_.consume(events);
        ++camera_event_batches_;
    }
    void prepare_scene(const CameraFrameState &camera, const fs::path &output) {
        check(!open_, "Scheduled scene frame was not retired");
        try {
            prepare_scene_impl(camera, output);
        } catch (const std::invalid_argument &error) {
            // A producer refusal is a recoverable frame boundary. Reset the
            // retained resources and publish an empty LAND draw list so the
            // caller can finish this logical frame and retry the next one.
            reset_scene();
            for (auto &drawn : draws_)
                drawn.clear();
            ++rejected_frames_;
            rejected_frame_ = true;
            fs::create_directories(output);
            blend_validation::text(output / "rejection.txt", error.what());
            open_ = true;
        }
    }
    void prepare_scene_impl(const CameraFrameState &camera, const fs::path &output) {
        check(!open_, "Scheduled scene frame was not retired");
        fs::create_directories(output);
        MaterialCacheSchedulerSettings settings{};
        settings.projection.fill(std::numeric_limits<float>::quiet_NaN());
        settings.viewport_width = 1280;
        settings.cull = true;
        material_cache_camera_controls(camera_motion_, profile_.configured.material, settings);
        Writer gl_mips, native_mips, generations;
        unsigned pairs = 0;
        const auto generated = [&](std::size_t index, const GlMaterialWorldPrepared &job) {
            const auto &state = renderer_.patch_state(index);
            const bool high = job.plan.passes.size() == 5;
            check(high || job.plan.passes.size() == 4, "Scheduled scene diffuse cache plan");
            Com<IDirect3DTexture9> texture;
            if (high) {
                const auto size = state.textures.higher_size;
                hr(original_.cache.device->CreateTexture(
                       size, size, 0, D3DUSAGE_RENDERTARGET | D3DUSAGE_AUTOGENMIPMAP, D3DFMT_R5G6B5,
                       D3DPOOL_DEFAULT, &texture.value, nullptr),
                   "Scheduled scene original higher cache");
                original_.cache.resources[job.plan.passes[3].targets[0]] = texture.value;
            }
            original_.execute(index, job, windows_.at(job.height_table_slot), state.textures.final_size);
            const auto mips = [&](const GlTexture &gl, IDirect3DTexture9 *native, unsigned role) {
                const auto width = gl.levels().front().width;
                for (unsigned mip = 0; mip < gl.levels().size(); ++mip) {
                    const auto raw = gl.read_mip_raw(mip);
                    const auto pixels = reader_.read(native, mip);
                    check(raw.size() * 2 == pixels.size(), "Scheduled scene cache mip extent");
                    for (std::size_t pixel = 0; pixel < raw.size() / 2; ++pixel) {
                        const unsigned packed =
                            unsigned(raw[pixel * 2]) | (unsigned(raw[pixel * 2 + 1]) << 8);
                        const unsigned codes[]{packed >> 11, (packed >> 5) & 63, packed & 31};
                        for (unsigned channel = 0; channel < 3; ++channel) {
                            const unsigned bits = channel == 1 ? 6 : 5;
                            const float decoded =
                                float((codes[channel] << (8 - bits)) | (codes[channel] >> (2 * bits - 8))) /
                                255.f;
                            if (std::bit_cast<std::uint32_t>(decoded) !=
                                std::bit_cast<std::uint32_t>(pixels[pixel * 4 + channel])) {
                                save(output / "failed-mip-gl.bin", raw);
                                save(output / "failed-mip-d3d9.f32",
                                     {reinterpret_cast<const std::uint8_t *>(pixels.data()),
                                      pixels.size() * sizeof(float)});
                                std::ostringstream detail;
                                detail << "generation=" << generation_ << " patch=" << index
                                       << " role=" << role << " width=" << width << " mip=" << mip
                                       << " pixel=" << pixel << " channel=" << channel << " gl=" << decoded
                                       << " d3d9=" << pixels[pixel * 4 + channel] << '\n';
                                blend_validation::text(output / "failed-mip.txt", detail.str());
                                save(output / "failed-plan.bin", serialize_plan(job.plan));
                                output_patch(job.patch, output);
                                save(output / "failed-frame.bin",
                                     {reinterpret_cast<const std::uint8_t *>(&job.frame), sizeof(job.frame)});
                                const unsigned roles[]{0, 1, 4, 3};
                                const unsigned passes[]{0, 0, 1, 2}, targets[]{0, 1, 0, 0};
                                for (unsigned part = 0; part < 4; ++part) {
                                    const auto prefix = "failed-intermediate-" + std::to_string(roles[part]);
                                    save(output / (prefix + "-gl.bin"),
                                         renderer_
                                             .temporary_texture(
                                                 {profile_.configured.material.quality.temporary_size,
                                                  profile_.configured.material.quality.cache_size},
                                                 static_cast<MaterialCacheRole>(roles[part]))
                                             .read_mip_raw(0));
                                    save(output / (prefix + "-d3d9.bin"),
                                         original_.cache.pixels(
                                             job.plan.passes[passes[part]].targets[targets[part]], 0));
                                }
                                std::cout << detail.str() << std::flush;
                            }
                            check(std::bit_cast<std::uint32_t>(decoded) ==
                                      std::bit_cast<std::uint32_t>(pixels[pixel * 4 + channel]),
                                  "Scheduled scene original RGB565 mip differs");
                        }
                        check(pixels[pixel * 4 + 3] == 1, "Scheduled scene unwritten original mip");
                    }
                    for (auto *writer : {&gl_mips, &native_mips})
                        for (auto value : {generation_, static_cast<unsigned>(index), role, width, mip})
                            writer->word(value);
                    gl_mips.blob(raw);
                    native_mips.blob({reinterpret_cast<const std::uint8_t *>(pixels.data()),
                                      pixels.size() * sizeof(float)});
                    ++pairs;
                }
            };
            mips(renderer_.final_texture(index), original_.finals.at(index).value, 0);
            if (high) {
                higher_.erase(index);
                higher_.emplace(index, std::move(texture));
                mips(renderer_.higher_texture(index), higher_.at(index).value, 1);
            }
            for (auto value : {generation_, static_cast<unsigned>(index), state.textures.desired_higher_size,
                               state.textures.final_size, high ? state.textures.higher_size : 0u})
                generations.word(value);
            ++generation_;
        };
        const MaterialNormalSeed seed = [](int, int, float squared) {
            return _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(squared)));
        };
        unsigned preload_generated = 0, preload_calls = 0;
        if (!epoch_frame_) {
            // The original perspective-camera corpus uses renderer +758 == 1.
            initial_cull_ = material_cache_initial_cull(profile_.loading, true, 1);
            renderer_.begin_loading(material_cache_camera_views(camera),
                                    profile_.configured.block_on_load != 0, initial_cull_);
            Writer foreground, background, steps;
            for (auto id : renderer_.loading_state().foreground)
                foreground.word(id);
            for (auto id : renderer_.loading_state().background)
                background.word(id);
            for (;;) {
                const auto loaded =
                    renderer_.load_step(frame_ + 1, seed, seed, lighting_, 4, true, generated);
                const auto &state = renderer_.loading_state();
                check(loaded.prepared == loaded.completed && loaded.completed == loaded.generated &&
                          loaded.prepared <= 4,
                      "Scene initial material loading barrier");
                ++preload_calls;
                preload_generated += static_cast<unsigned>(loaded.generated);
                for (auto value : {state.cursor, state.prepared_in_step, state.finished,
                                   std::bit_cast<std::uint32_t>(state.progress)})
                    steps.word(value);
                if (loaded.finished)
                    break;
                check(preload_calls <= (world_->cache().patches.size() + 3) / 4 + 1,
                      "Scene initial loading did not make bounded progress");
            }
            save(output / "loading-foreground.bin", foreground.bytes);
            save(output / "loading-background.bin", background.bytes);
            save(output / "loading-steps.bin", steps.bytes);
        }
        const auto result = renderer_.schedule_camera_frame(camera, settings, frame_ + 2, seed, seed,
                                                            lighting_, 4, true, generated);
        check(result.prepared == result.completed && renderer_.scheduler_state().pending.empty(),
              "Scheduled scene retained completion barrier");
        Writer visible, drawable, states, counts;
        visible.word(camera.view_count);
        drawable.word(camera.view_count);
        for (unsigned view = 0; view < camera.view_count; ++view) {
            const auto &selected = renderer_.scheduler_state().visible[view];
            visible.word(static_cast<unsigned>(selected.size()));
            auto &indices = draws_[view];
            indices.clear();
            for (auto index : selected) {
                visible.word(index);
                const auto state = renderer_.patch_state(index).completed_state;
                if (state != 0 && state != 1 && state != 4)
                    indices.push_back(index);
            }
            drawable.word(static_cast<unsigned>(indices.size()));
            for (auto index : indices)
                drawable.word(index);
        }
        for (std::size_t i = 0; i < identities_.size(); ++i) {
            const auto &s = renderer_.patch_state(i);
            check(&s == identities_[i] && renderer_.scheduled_patch(i).state == identities_[i] &&
                      s.staging == std::array<std::uintptr_t, 5>{},
                  "Scheduled scene changed stable patch owners or retained staging");
            for (auto value : {s.completed_state, s.requested_state, s.frame_stamp, s.pending_jobs.load(),
                               s.textures.final_size, s.textures.higher_size, s.textures.desired_higher_size,
                               s.textures.final[0], s.textures.higher[0]})
                states.word(value);
            if (!s.textures.final[0])
                original_.finals.erase(static_cast<unsigned>(i));
            if (!s.textures.higher[0])
                higher_.erase(i);
        }
        for (auto width : {64u, 128u, 256u, 512u})
            counts.word(static_cast<unsigned>(renderer_.free_cache_count(width)));
        save(output / "visible.bin", visible.bytes);
        save(output / "drawable.bin", drawable.bytes);
        save(output / "states.bin", states.bytes);
        save(output / "free-counts.bin", counts.bytes);
        save(output / "generation-records.bin", generations.bytes);
        const auto save_mips = [&](const char *name, const char *signature, const Writer &records) {
            Writer writer;
            writer.bytes.insert(writer.bytes.end(), signature, signature + 8);
            writer.word(pairs);
            writer.bytes.insert(writer.bytes.end(), records.bytes.begin(), records.bytes.end());
            save(output / name, writer.bytes);
        };
        save_mips("mips-gl.bin", "C5MIPG01", gl_mips);
        save_mips("mips-d3d9.bin", "C5MIPD01", native_mips);
        std::ostringstream report;
        report << "{\"frame\":" << frame_ << ",\"epoch\":" << epoch_ << ",\"epochFrame\":" << epoch_frame_
               << ",\"views\":" << camera.view_count << ",\"budget\":" << settings.attempt_budget
               << ",\"stillBudget\":" << settings.fast_selection_budget
               << ",\"moving\":" << camera_motion_.moving << ",\"fast\":" << settings.fast
               << ",\"forceFast\":" << settings.force_fast << ",\"initialCull\":" << initial_cull_
               << ",\"cameraEventBatches\":" << camera_event_batches_
               << ",\"temporarySize\":" << profile_.configured.material.quality.temporary_size
               << ",\"cacheSize\":" << profile_.configured.material.quality.cache_size
               << ",\"generated\":" << preload_generated + result.generated
               << ",\"preloadGenerated\":" << preload_generated << ",\"preloadCalls\":" << preload_calls
               << ",\"mipPairs\":" << pairs << ",\"stableOwners\":true,\"rejectedFrames\":"
               << rejected_frames_ << ",\"fullGraphicsParity\":false}\n";
        blend_validation::text(output / "result.json", report.str());
        std::cout << "PASS scheduled material scene " << frame_ << " epoch " << epoch_ << " views "
                  << camera.view_count << " generated " << preload_generated + result.generated << " mips "
                  << pairs << '\n'
                  << std::flush;
        ++frame_;
        ++epoch_frame_;
        open_ = true;
    }
    void finish_scene() {
        check(open_, "Scheduled scene completion without preparation");
        if (!rejected_frame_)
            renderer_.finish_frame();
        rejected_frame_ = false;
        open_ = false;
    }
    std::span<const std::uint32_t> scene_indices(unsigned view) const { return draws_.at(view); }
    auto scene_images(std::size_t index) {
        auto images = original_.images(index, false);
        if (renderer_.patch_state(index).completed_state == 5)
            images[0] = higher_.at(index).value;
        return images;
    }
    const GlTexture &height_texture(std::size_t index) const { return renderer_.height_texture(index); }
    LandMaterialConstants land_constants(std::size_t index, LandMaterialConstants frame) const {
        return renderer_.land_constants(index, frame);
    }
    void draw(std::size_t index, const LandMaterialConstants &frame, GlLandMaterialInputs images, int width,
              int height) {
        renderer_.draw(index, frame, images, width, height);
    }

  private:
    GlMaterialWorldRenderer &renderer_;
    NativeWorld &original_;
    std::shared_ptr<const MaterialWorld> world_;
    GlMaterialWorldLighting lighting_;
    mip_validation::NativeMipReader reader_;
    material_preview_settings::Profile profile_;
    MaterialCacheCameraState camera_motion_;
    unsigned camera_event_batches_ = 0;
    bool initial_cull_ = false;
    std::map<unsigned, Bytes> windows_;
    std::map<std::size_t, Com<IDirect3DTexture9>> higher_;
    std::vector<const MaterialCacheLifecycleState *> identities_;
    std::array<std::vector<std::uint32_t>, 4> draws_;
    unsigned frame_ = 0, epoch_ = 0, epoch_frame_ = 0, generation_ = 0, rejected_frames_ = 0;
    bool open_ = false, rejected_frame_ = false;
};
} // namespace
