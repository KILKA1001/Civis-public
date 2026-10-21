#pragma once
#include <limits>
#include <map>
#include <set>
#include <stdexcept>

namespace {
// The same production owner used by the complete-scene parity route, with
// lightweight observations for the interactive preview instead of D3D9 work.
class ScheduledMaterialPreview {
  public:
    ScheduledMaterialPreview(std::shared_ptr<GlContext> context, GlMaterialWorldRenderer &renderer,
                             std::shared_ptr<const MaterialWorld> world,
                             GlMaterialWorldLighting lighting, bool capture,
                             const material_preview_settings::Profile &profile, bool audit_height_bindings)
        : context_(std::move(context)), renderer_(renderer), world_(std::move(world)), lighting_(lighting),
          profile_(profile), capture_(capture), audit_height_bindings_(audit_height_bindings) {
        const auto &quality = profile_.configured.material.quality;
        check(quality.base_size == 64 && quality.cache_mode == 0 && quality.compression == 0,
              "Configured preview requires ordinary diffuse material settings");
        renderer_.enable_scheduling(std::span<const std::uint32_t, 4>(quality.pool_counts.data(), 4),
                                    {64, 0, 0, 0, X87Precision::double_precision},
                                    {quality.temporary_size, quality.cache_size});
        for (std::size_t i = 0; i < world_->cache().patches.size(); ++i)
            owners_.push_back(&renderer_.patch_state(i));
        if (audit_height_bindings_) {
            check(context_ != nullptr, "LAND height binding audit requires its live GL context");
            const auto field = world_->terrain().height_field();
            const auto &land = world_->terrain().land();
            std::set<GLuint> unique_textures;
            for (std::size_t index = 0; index < world_->cache().patches.size(); ++index) {
                const auto patch = land.patch(index / world_->cache().rows, index % world_->cache().rows);
                const auto slot = patch.texture_table_slot();
                const auto &texture = renderer_.height_texture(index);
                const auto [found, inserted] = height_windows_.emplace(slot, texture.id());
                check(found->second == texture.id(), "One LAND source window has multiple GPU textures");
                if (inserted) {
                    const auto expected = field.extract_a8_texture(slot % 16, slot / 16);
                    check(texture.read_mip_raw(0) == expected,
                          "Scheduled LAND height window GPU bytes differ from selected world source");
                    check(unique_textures.insert(texture.id()).second,
                          "Different LAND source windows alias one GPU texture");
                }
            }
            // The source table includes a terminal row when the field extent is
            // exactly divisible by 896. No patch starts in that row on this map.
            const auto used_columns = (land.columns() - 1) * 64 / 896 + 1;
            const auto used_rows = (land.rows() - 1) * 64 / 896 + 1;
            check(height_windows_.size() == std::size_t(used_columns) * used_rows,
                  "Scheduled LAND patches do not cover their addressable height windows");
        }
    }
    void camera_events(std::span<const civ5::terrain::CameraMotionEvent> events) {
        check(!open_, "Camera events must precede material frame preparation");
        camera_motion_.consume(events);
    }
    void save_camera_motion() { initial_camera_motion_ = camera_motion_; }
    void restore_camera_motion() { camera_motion_ = initial_camera_motion_; }
    void prepare_scene(const CameraFrameState &camera, const fs::path &output) {
        check(!open_, "Preview material preparation must follow frame completion");
        try {
            prepare_scene_impl(camera, output);
        } catch (const std::invalid_argument &error) {
            // Producer refusals are recoverable at the scene boundary. The
            // renderer rolls back its scheduler transaction; publish no LAND
            // membership for this logical frame and let water/next frame run.
            renderer_.invalidate_generated_resources();
            const auto &quality = profile_.configured.material.quality;
            renderer_.enable_scheduling(std::span<const std::uint32_t, 4>(quality.pool_counts.data(), 4),
                                        {64, 0, 0, 0, X87Precision::double_precision},
                                        {quality.temporary_size, quality.cache_size});
            owners_.clear();
            for (std::size_t i = 0; i < world_->cache().patches.size(); ++i)
                owners_.push_back(&renderer_.patch_state(i));
            for (auto &drawn : draws_)
                drawn.clear();
            ++rejected_frames_;
            rejected_frame_ = true;
            if (capture_) {
                fs::create_directories(output);
                blend_validation::text(output / "rejection.txt", error.what());
            }
            open_ = true;
        }
    }
    void prepare_scene_impl(const CameraFrameState &camera, const fs::path &output) {
        check(!open_, "Preview material preparation must follow frame completion");
        MaterialCacheSchedulerSettings settings{};
        settings.projection.fill(std::numeric_limits<float>::quiet_NaN());
        settings.viewport_width = 1280;
        settings.cull = true;
        material_cache_camera_controls(camera_motion_, profile_.configured.material, settings);
        const MaterialNormalSeed seed = [](int, int, float squared) {
            return _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(squared)));
        };
        if (!frames_) {
            // The original perspective-camera fixture uses renderer +758 == 1.
            // camera is already prepared before the original initial-cull prerequisite.
            initial_cull_ = material_cache_initial_cull(profile_.loading, true, 1);
            renderer_.begin_loading(material_cache_camera_views(camera),
                                    profile_.configured.block_on_load != 0, initial_cull_);
            Writer load_steps, foreground, background;
            for (auto id : renderer_.loading_state().foreground)
                foreground.word(id);
            for (auto id : renderer_.loading_state().background)
                background.word(id);
            unsigned calls = 0, generated = 0;
            for (;;) {
                const auto loaded = renderer_.load_step(1, seed, seed, lighting_, 4, true);
                const auto &state = renderer_.loading_state();
                check(loaded.prepared == loaded.completed && loaded.completed == loaded.generated &&
                          loaded.prepared <= 4,
                      "Preview initial loading barrier");
                ++calls;
                generated += static_cast<unsigned>(loaded.generated);
                for (auto value : {state.cursor, state.prepared_in_step, state.finished,
                                   std::bit_cast<std::uint32_t>(state.progress)})
                    load_steps.word(value);
                if (loaded.finished)
                    break;
                check(calls <= (world_->cache().patches.size() + 3) / 4 + 1,
                      "Preview initial loading did not make bounded progress");
            }
            preload_generations_ = generated;
            preload_calls_ = calls;
            if (capture_) {
                fs::create_directories(output);
                save(output / "loading-steps.bin", load_steps.bytes);
                save(output / "loading-foreground.bin", foreground.bytes);
                save(output / "loading-background.bin", background.bytes);
            }
        }
        const auto result =
            renderer_.schedule_camera_frame(camera, settings, frames_ + 2, seed, seed, lighting_, 4, true);
        check(result.prepared == result.completed && renderer_.scheduler_state().pending.empty(),
              "Preview material completion barrier");
        Writer visible, drawable, states, counts;
        visible.word(camera.view_count);
        drawable.word(camera.view_count);
        for (unsigned view = 0; view < camera.view_count; ++view) {
            const auto &selected = renderer_.scheduler_state().visible[view];
            auto &drawn = draws_[view];
            drawn.clear();
            visible.word(static_cast<unsigned>(selected.size()));
            for (auto index : selected) {
                visible.word(index);
                const auto state = renderer_.patch_state(index).completed_state;
                if (state != 0 && state != 1 && state != 4)
                    drawn.push_back(index);
            }
            drawable.word(static_cast<unsigned>(drawn.size()));
            for (auto index : drawn)
                drawable.word(index);
        }
        for (std::size_t i = 0; i < owners_.size(); ++i) {
            const auto &state = renderer_.patch_state(i);
            check(&state == owners_[i] && renderer_.scheduled_patch(i).state == owners_[i] &&
                      state.staging == std::array<std::uintptr_t, 5>{},
                  "Preview stable material owners and staging retirement");
            if (capture_)
                for (auto value :
                     {state.completed_state, state.requested_state, state.frame_stamp,
                      state.pending_jobs.load(), state.textures.final_size, state.textures.higher_size,
                      state.textures.desired_higher_size, state.textures.final[0], state.textures.higher[0]})
                    states.word(value);
        }
        if (capture_) {
            fs::create_directories(output);
            for (auto width : {64u, 128u, 256u, 512u})
                counts.word(static_cast<unsigned>(renderer_.free_cache_count(width)));
            save(output / "visible.bin", visible.bytes);
            save(output / "drawable.bin", drawable.bytes);
            save(output / "states.bin", states.bytes);
            save(output / "free-counts.bin", counts.bytes);
            std::ostringstream report;
            report << "{\"frame\":" << frames_ << ",\"views\":" << camera.view_count
                   << ",\"budget\":" << settings.attempt_budget
                   << ",\"stillBudget\":" << settings.fast_selection_budget
                   << ",\"moving\":" << camera_motion_.moving << ",\"fast\":" << settings.fast
                   << ",\"forceFast\":" << settings.force_fast << ",\"generated\":" << result.generated
                   << ",\"stableOwners\":true,\"importedCaches\":false,\"fullGraphicsParity\":false}\n";
            blend_validation::text(output / "result.json", report.str());
        }
        generations_ += result.generated;
        multiple_view_frames_ += camera.view_count > 1;
        ++frames_;
        open_ = true;
    }
    void finish_scene() {
        check(open_, "Preview material frame completed twice");
        if (!rejected_frame_)
            renderer_.finish_frame();
        ++finished_;
        rejected_frame_ = false;
        open_ = false;
    }
    std::span<const std::uint32_t> scene_indices(unsigned view) const { return draws_.at(view); }
    void draw(std::size_t index, const LandMaterialConstants &frame, GlLandMaterialInputs images, int width,
              int height) {
        check(open_, "Preview material draw outside its logical frame");
        renderer_.draw(index, frame, images, width, height);
        if (audit_height_bindings_) {
            const auto get_integer =
                reinterpret_cast<PFNGLGETINTEGERI_VPROC>(context_->proc_address("glGetIntegeri_v"));
            GLint bound = 0;
            get_integer(GL_TEXTURE_BINDING_2D, 10, &bound);
            context_->check("Scheduled LAND height texture unit-10 binding");
            check(bound == static_cast<GLint>(renderer_.height_texture(index).id()),
                  "Scheduled LAND draw bound another patch's height window");
            ++height_draws_;
            height_draw_patches_.insert(index);
        }
    }
    void write_report(const fs::path &output) const {
        check(!open_ && frames_ == finished_, "Every preview material frame must complete once");
        if (audit_height_bindings_) {
            check(height_draws_ && !height_draw_patches_.empty(),
                  "Scheduled LAND height audit did not observe an actual draw");
            std::ostringstream heights;
            heights << "{\"schema\":\"civ5-scheduled-land-height-binding-v1\",\"sourceWindows\":"
                    << world_->terrain().height_field().domain().texture_columns() *
                           world_->terrain().height_field().domain().texture_rows()
                    << ",\"patchSelectedWindows\":" << height_windows_.size()
                    << ",\"mappedPatches\":" << world_->cache().patches.size()
                    << ",\"drawCallsChecked\":" << height_draws_
                    << ",\"drawPatchesChecked\":" << height_draw_patches_.size()
                    << ",\"gpuReadbackMatchesSource\":true,\"boundUnit10MatchesPatch\":true"
                    << ",\"fullGraphicsParity\":false}\n";
            blend_validation::text(output / "land-height-bindings.json", heights.str());
        }
        std::ostringstream report;
        report << "{\"schema\":\"civ5-configured-material-preview-candidate-v1\",\"logicalFrames\":"
               << frames_ << ",\"finishedFrames\":" << finished_ << ",\"generations\":" << generations_
               << ",\"preloadGenerations\":" << preload_generations_ << ",\"preloadCalls\":" << preload_calls_
               << ",\"initialCull\":" << initial_cull_ << ",\"configuredMaterialSettings\":true"
               << ",\"multipleViewFrames\":" << multiple_view_frames_
               << ",\"stableOwners\":true,\"rejectedFrames\":" << rejected_frames_
               << ",\"importedCaches\":false,\"fullGraphicsParity\":false}\n";
        blend_validation::text(output / "scheduled-materials.json", report.str());
    }

  private:
    std::shared_ptr<GlContext> context_;
    GlMaterialWorldRenderer &renderer_;
    std::shared_ptr<const MaterialWorld> world_;
    GlMaterialWorldLighting lighting_;
    material_preview_settings::Profile profile_;
    MaterialCacheCameraState camera_motion_, initial_camera_motion_;
    bool initial_cull_ = false;
    bool capture_ = false, open_ = false, rejected_frame_ = false;
    bool audit_height_bindings_ = false;
    std::map<int, GLuint> height_windows_;
    std::set<std::size_t> height_draw_patches_;
    std::size_t height_draws_ = 0;
    unsigned frames_ = 0, finished_ = 0, generations_ = 0, multiple_view_frames_ = 0;
    unsigned preload_generations_ = 0, preload_calls_ = 0, rejected_frames_ = 0;
    std::vector<const MaterialCacheLifecycleState *> owners_;
    std::array<std::vector<std::uint32_t>, 4> draws_;
};
} // namespace
