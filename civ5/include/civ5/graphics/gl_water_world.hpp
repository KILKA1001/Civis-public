#pragma once
#include "civ5/terrain/camera_input.hpp"
#include "civ5/terrain/terrain_world.hpp"
#include "gl_texture.hpp"
#include "gl_water_current_map.hpp"
#include "gl_water_reflection_map.hpp"
#include "gl_water_surface.hpp"

namespace civ5::ui::detail {class CivisFrontendAlphaLandFrame;}
namespace civ5::graphics {
// Ocean surface backed by the same immutable terrain generation as LAND.
// It owns the authored wave/color textures and all generated A8 windows.
// A prepared current map uses this world's worker dimensions. Reflection
// storage may be prepared with an explicit shared depth surface. Object,
// FOW/overlay/shadow/LUT producers remain caller inputs.
class GlWaterWorldRenderer {
  public:
    GlWaterWorldRenderer(std::shared_ptr<GlContext> context,
                         std::shared_ptr<const terrain::TerrainWorld> world, const assets::LeanData &waves,
                         const assets::DdsData &color, unsigned selector, terrain::X87Precision precision);
    ~GlWaterWorldRenderer();
    GlWaterWorldRenderer(const GlWaterWorldRenderer &) = delete;
    GlWaterWorldRenderer &operator=(const GlWaterWorldRenderer &) = delete;
    // Build up to four view groups. An unsuccessful rebuild retains the prior
    // geometry. Camera constants and wave phase can change without rebuilding.
    void prepare(std::span<const terrain::Vector4> view_bounds);
    // Compute visibility, light directions and reflection matrices together,
    // then publish them with matching ocean geometry. Failed updates retain
    // the previous frame. The first prepared frame requires an active view;
    // subsequent zero-view frames retain the original inactive bound slots.
    void prepare_frame(const materials::WaterReflectionCamera &camera,
                       const materials::WaterReflectionFrameSettings &settings,
                       const terrain::ReciprocalSqrtEstimate &estimate);
    const materials::WaterReflectionFrameState &frame_state() const;
    // Original perspective camera and map-wrap views feed visibility and the
    // water frame directly. Published geometry and both states change together.
    void prepare_camera_frame(const terrain::PerspectiveCamera &camera,
                              const materials::WaterReflectionFrameSettings &settings,
                              const terrain::ReciprocalEstimate &reciprocal,
                              const terrain::ReciprocalSqrtEstimate &reciprocal_sqrt);
    const terrain::CameraFrameState &camera_state() const;
    // Initialize the original camera from screen/world settings and publish
    // its complete frame. Return the controller state to its runtime owner.
    terrain::CameraControllerState
    prepare_camera_startup(const terrain::CameraStartupSettings &startup,
                           const terrain::CameraControllerState &prior,
                           const materials::WaterReflectionFrameSettings &settings,
                           const terrain::ReciprocalEstimate &reciprocal,
                           const terrain::ReciprocalSqrtEstimate &reciprocal_sqrt);
    // Advance the normal controller, then publish its matching visibility,
    // ocean geometry and reflection frame. Return ordered event payloads only
    // after successful publication; failed updates retain the prior frame.
    terrain::CameraMotionResult
    prepare_camera_motion(const terrain::CameraControllerState &prior, const terrain::CameraMotionInput &input,
                          const materials::WaterReflectionFrameSettings &settings,
                          const terrain::ReciprocalEstimate &reciprocal,
                          const terrain::ReciprocalSqrtEstimate &reciprocal_sqrt);
    // Pointer gestures and wheel events produce the controller used for the
    // frame. Return next pointer state and OS requests after publication.
    terrain::CameraInputResult
    prepare_camera_input(const terrain::CameraControllerState &controller, const terrain::CameraPointerState &pointer,
                         const terrain::CameraInputFrame &input,
                         const materials::WaterReflectionFrameSettings &settings,
                         const terrain::ReciprocalEstimate &reciprocal,
                         const terrain::ReciprocalSqrtEstimate &reciprocal_sqrt);
    // Generate the original current target before drawing to the scene target.
    // Failed rebuilds retain the previously published map. As a rendering pass,
    // this establishes its own raster state and ends on framebuffer zero.
    void prepare_current(materials::WaterCurrentSettings settings = {}, bool conditional_clear = false);
    const GlWaterCurrentMap &current_map() const;
    // Prepare original color storage and complete its clear/object pass before
    // publishing it. Failed preparation retains the preceding reflection map.
    void prepare_reflection(std::uint32_t quality, std::uint32_t reflections,
                            std::shared_ptr<const GlTexture> shared_depth = {},
                            const std::function<void()> &draws = {});
    const GlWaterReflectionMap &reflection_map() const;
    void render_reflection(const std::function<void()> &draws = {});
    materials::WaterConstantBlock constants(const materials::WaterSurfaceSettings &settings,
                                            const materials::WaterSurfaceFrame &frame,
                                            const materials::WaterSurfaceView &view) const;
    // Uses this prepared view's computed shadow/reflection matrices. After
    // prepare_camera_frame, also supplies the owned projection, view and camera
    // position. Overlay, fog and surface inputs retain their own producers.
    materials::WaterConstantBlock constants(std::uint32_t view_index,
                                            const materials::WaterSurfaceSettings &settings,
                                            const materials::WaterSurfaceFrame &frame,
                                            const materials::WaterSurfaceView &view) const;
    // Caller owns framebuffer/depth/blend state. Original slots 0,1,6,7 are
    // supplied here; the matching height window changes with each draw packet.
    // Once prepared, owned reflection/current maps also supply slots 4/5.
    void draw(std::uint32_t view_index, const materials::WaterConstantBlock &constants,
              GlWaterSurfaceInputs scene, int width, int height);
    const materials::WaterSurfaceMesh &mesh() const;
    std::span<const materials::WaterSurfaceDraw> draws() const;
    const GlTexture &height_texture(std::uint32_t window) const;
    const GlTexture &wave_texture(assets::LeanMap map) const;
    const GlTexture &color_texture() const;
    const materials::WaterSurfaceDomain &domain() const;
    // Recreate every ocean-owned GPU object on a different live context.
    // The old context must still support resource release on this thread;
    // this does not recover a lost/unusable WGL context.
    // Source LEAN/DDS data and a replacement shared reflection depth surface
    // are supplied by the caller; prepared CPU mesh/frame state is retained.
    // Render-target reflections are returned unrendered and must be submitted
    // again before sampling. External scene textures and targets are caller
    // owned and must be recreated separately.
    void rebuild_context(std::shared_ptr<GlContext> context, const assets::LeanData &waves,
                         const assets::DdsData &color, std::shared_ptr<const GlTexture> shared_depth = {});

  private:
    friend class ::civ5::ui::detail::CivisFrontendAlphaLandFrame;
    // Pure canonical terrain control for the closed actual Camera join.
    const std::shared_ptr<const terrain::TerrainWorld>& source_world_owned_without_callbacks()const;
    // Same camera/frame producer; the closed alpha companion owns surface GL.
    void use_external_surface_owned();
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace civ5::graphics
