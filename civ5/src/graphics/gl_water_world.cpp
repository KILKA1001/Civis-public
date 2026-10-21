#include "civ5/graphics/gl_water_world.hpp"
#include <algorithm>
#include <map>
#include <optional>
#include <stdexcept>

namespace civ5::graphics {
namespace {
void require(bool ok, const char *message) {
    if (!ok)
        throw std::invalid_argument(message);
}
struct ReflectionPass {
    bool &active;
    explicit ReflectionPass(bool &value) : active(value) {
        require(!active, "Water reflection pass cannot be reentered");
        active = true;
    }
    ~ReflectionPass() { active = false; }
};
struct FrameOperation {
    unsigned &active;
    explicit FrameOperation(unsigned &value) : active(value) { ++active; }
    ~FrameOperation() { --active; }
};
} // namespace
struct GlWaterWorldRenderer::Impl {
    std::shared_ptr<GlContext> context;
    std::shared_ptr<const terrain::TerrainWorld> world;
    materials::WaterSurfaceDomain domain;
    GlTexture rgba, luminance, color;
    std::map<unsigned, GlTexture> heights;
    unsigned selector;
    terrain::X87Precision precision;
    enum class SurfacePolicy { owned_surface, external_surface };
    SurfacePolicy surface_policy=SurfacePolicy::owned_surface;
    struct Prepared {
        materials::WaterSurfaceMesh mesh{};
        std::vector<materials::WaterSurfaceDraw> draws;
        std::unique_ptr<GlWaterSurfaceRenderer> renderer;
        std::optional<materials::WaterReflectionFrameState> frame;
        std::optional<terrain::CameraFrameState> camera;
    };
    std::unique_ptr<Prepared> prepared;
    std::unique_ptr<GlWaterCurrentMap> current;
    std::unique_ptr<GlWaterReflectionMap> reflection;
    std::optional<std::pair<materials::WaterCurrentSettings, bool>> current_parameters;
    std::optional<std::pair<std::uint32_t, std::uint32_t>> reflection_parameters;
    bool reflection_active = false;
    unsigned frame_operations = 0;
    std::unique_ptr<Prepared> prepare(std::span<const terrain::Vector4> bounds);
    Impl(std::shared_ptr<GlContext> c, std::shared_ptr<const terrain::TerrainWorld> w,
         const assets::LeanData &waves, const assets::DdsData &colors, unsigned selected,
         terrain::X87Precision pc)
        : context(std::move(c)), world(std::move(w)),
          domain(materials::WaterSurfaceDomain::from_world(*world)),
          rgba(GlTexture::lean(*context, waves, assets::LeanMap::rgba16)),
          luminance(GlTexture::lean(*context, waves, assets::LeanMap::luminance16)),
          color(*context, colors, DdsSamplingColor::srgb), selector(selected), precision(pc) {
        require(colors.face_count() == 1, "Water depth color must be two-dimensional");
        const auto field = world->height_field();
        for (int y = 0; y < field.domain().texture_rows(); ++y)
            for (int x = 0; x < field.domain().texture_columns(); ++x)
                heights.emplace(x + 16 * y,
                                GlTexture::alpha8(*context, 1024, 1024, field.extract_a8_texture(x, y)));
    }
};
GlWaterWorldRenderer::GlWaterWorldRenderer(std::shared_ptr<GlContext> context,
                                           std::shared_ptr<const terrain::TerrainWorld> world,
                                           const assets::LeanData &waves, const assets::DdsData &color,
                                           unsigned selector, terrain::X87Precision precision) {
    require(context && world, "Water renderer requires an owned world and context");
    require(selector == 0 || selector == 2 || selector == 3 || selector == 5,
            "Water world requires an ocean selector");
    require(precision == terrain::X87Precision::single ||
                precision == terrain::X87Precision::double_precision,
            "Invalid water world precision");
    require(color.face_count() == 1, "Water depth color must be two-dimensional");
    impl_ = std::make_unique<Impl>(std::move(context), std::move(world), waves, color, selector, precision);
}
GlWaterWorldRenderer::~GlWaterWorldRenderer() = default;
const std::shared_ptr<const terrain::TerrainWorld>& GlWaterWorldRenderer::source_world_owned_without_callbacks()const{
    return impl_->world; // Memory only; caller owns current creator/lifetime checks.
}
void GlWaterWorldRenderer::use_external_surface_owned(){
    auto& s=*impl_;
    require(!s.prepared&&!s.frame_operations&&!s.reflection_active,"External water surface policy requires idle fresh camera producer");
    s.surface_policy=Impl::SurfacePolicy::external_surface;
}
void GlWaterWorldRenderer::prepare_current(materials::WaterCurrentSettings settings, bool conditional_clear) {
    auto &s = *impl_;
    const auto &grid = s.world->source().layout.worker_grid;
    auto next =
        std::make_unique<GlWaterCurrentMap>(s.context, grid.width, grid.height, settings, conditional_clear);
    s.current = std::move(next);
    s.current_parameters = std::make_pair(settings, conditional_clear);
}
const GlWaterCurrentMap &GlWaterWorldRenderer::current_map() const {
    require(bool(impl_->current), "Unprepared water current map");
    return *impl_->current;
}
void GlWaterWorldRenderer::prepare_reflection(std::uint32_t quality, std::uint32_t reflections,
                                              std::shared_ptr<const GlTexture> shared_depth,
                                              const std::function<void()> &draws) {
    auto &s = *impl_;
    const ReflectionPass pass(s.reflection_active);
    require(materials::water_reflection_resource(quality, reflections).render_target || !draws,
            "Reflection fallback cannot accept object draws");
    auto next =
        std::make_unique<GlWaterReflectionMap>(s.context, quality, reflections, std::move(shared_depth));
    if (next->resource().render_target)
        next->render(draws);
    s.reflection = std::move(next);
    s.reflection_parameters = std::make_pair(quality, reflections);
}
const GlWaterReflectionMap &GlWaterWorldRenderer::reflection_map() const {
    require(bool(impl_->reflection), "Unprepared reflection map");
    return *impl_->reflection;
}
void GlWaterWorldRenderer::render_reflection(const std::function<void()> &draws) {
    const ReflectionPass pass(impl_->reflection_active);
    require(bool(impl_->reflection), "Unprepared reflection map");
    impl_->reflection->render(draws);
}
std::unique_ptr<GlWaterWorldRenderer::Impl::Prepared>
GlWaterWorldRenderer::Impl::prepare(std::span<const terrain::Vector4> bounds) {
    auto next = std::make_unique<Prepared>();
    if(surface_policy==SurfacePolicy::external_surface)return next;
    next->mesh = materials::build_water_surface_mesh(domain, bounds, prepared ? prepared->mesh.capacity : 0,
                                                     precision);
    // Logical handles describe resources in this producer, never GL names
    // truncated to the original U16 ABI. Actual GL names bind at submission.
    materials::WaterSurfaceResources resources{1, 2, 3, 4, 5, {6, 7, 8, 9, 10, 11, 12}, {}};
    for (const auto &[slot, texture] : heights)
        resources.height_windows[slot] = 13 + slot;
    next->draws = materials::water_surface_draws(next->mesh, resources, selector);
    for (const auto &draw : next->draws)
        require(heights.contains(draw.window), "Water culling addressed a height window outside its world");
    next->renderer = prepared ? std::make_unique<GlWaterSurfaceRenderer>(*prepared->renderer, next->mesh)
                              : std::make_unique<GlWaterSurfaceRenderer>(context, next->mesh, selector);
    return next;
}
void GlWaterWorldRenderer::prepare(std::span<const terrain::Vector4> bounds) {
    require(impl_->surface_policy==Impl::SurfacePolicy::owned_surface,"External surface camera mode cannot prepare legacy geometry");
    impl_->prepared = impl_->prepare(bounds);
}
void GlWaterWorldRenderer::prepare_frame(const materials::WaterReflectionCamera &camera,
                                         const materials::WaterReflectionFrameSettings &settings,
                                         const terrain::ReciprocalSqrtEstimate &estimate) {
    auto &s = *impl_;
    const FrameOperation operation(s.frame_operations);
    const auto prior =
        s.prepared && s.prepared->frame ? *s.prepared->frame : materials::WaterReflectionFrameState{};
    const auto frame = materials::water_reflection_frame(s.world->source().layout, camera, settings, prior,
                                                         s.precision, estimate);
    std::array<terrain::Vector4, 4> bounds;
    for (unsigned view = 0; view < camera.view_count; ++view)
        bounds[view] = {frame.bounds.clipped_low[view][0], frame.bounds.clipped_low[view][1],
                        frame.bounds.clipped_high[view][0], frame.bounds.clipped_high[view][1]};
    auto next = s.prepare(std::span(bounds).first(camera.view_count));
    next->frame = frame;
    s.prepared = std::move(next);
}
const materials::WaterReflectionFrameState &GlWaterWorldRenderer::frame_state() const {
    require(impl_->prepared && impl_->prepared->frame, "Unprepared water camera frame");
    return *impl_->prepared->frame;
}
void GlWaterWorldRenderer::prepare_camera_frame(const terrain::PerspectiveCamera &camera,
                                                const materials::WaterReflectionFrameSettings &settings,
                                                const terrain::ReciprocalEstimate &reciprocal,
                                                const terrain::ReciprocalSqrtEstimate &reciprocal_sqrt) {
    auto &s = *impl_;
    const FrameOperation operation(s.frame_operations);
    const materials::WaterCameraFrameState prior{
        s.prepared && s.prepared->camera ? *s.prepared->camera : terrain::CameraFrameState{},
        s.prepared && s.prepared->frame ? *s.prepared->frame : materials::WaterReflectionFrameState{}};
    const auto &source = s.world->source();
    const auto frame = materials::water_camera_frame(source.layout, source.map_mode, camera, settings, prior,
                                                     s.precision, reciprocal, reciprocal_sqrt);
    std::array<terrain::Vector4, 4> bounds;
    for (unsigned view = 0; view < frame.camera.view_count; ++view)
        bounds[view] = {frame.water.bounds.clipped_low[view][0], frame.water.bounds.clipped_low[view][1],
                        frame.water.bounds.clipped_high[view][0], frame.water.bounds.clipped_high[view][1]};
    auto next = s.prepare(std::span(bounds).first(frame.camera.view_count));
    next->frame = frame.water;
    next->camera = frame.camera;
    s.prepared = std::move(next);
}
const terrain::CameraFrameState &GlWaterWorldRenderer::camera_state() const {
    require(impl_->prepared && impl_->prepared->camera, "Unprepared perspective camera frame");
    return *impl_->prepared->camera;
}
terrain::CameraControllerState GlWaterWorldRenderer::prepare_camera_startup(
    const terrain::CameraStartupSettings &startup, const terrain::CameraControllerState &prior,
    const materials::WaterReflectionFrameSettings &settings, const terrain::ReciprocalEstimate &reciprocal,
    const terrain::ReciprocalSqrtEstimate &reciprocal_sqrt) {
    const FrameOperation operation(impl_->frame_operations);
    const auto &source = impl_->world->source();
    const auto controller =
        terrain::initialize_camera(source.layout, source.map_mode, startup, prior, impl_->precision);
    prepare_camera_frame(controller.camera, settings, reciprocal, reciprocal_sqrt);
    return controller;
}
terrain::CameraMotionResult GlWaterWorldRenderer::prepare_camera_motion(
    const terrain::CameraControllerState &prior, const terrain::CameraMotionInput &input,
    const materials::WaterReflectionFrameSettings &settings, const terrain::ReciprocalEstimate &reciprocal,
    const terrain::ReciprocalSqrtEstimate &reciprocal_sqrt) {
    const FrameOperation operation(impl_->frame_operations);
    const auto &source = impl_->world->source();
    auto motion = terrain::advance_camera(source.layout, source.map_mode, prior, input, impl_->precision, reciprocal);
    prepare_camera_frame(motion.controller.camera, settings, reciprocal, reciprocal_sqrt);
    return motion;
}
materials::WaterConstantBlock GlWaterWorldRenderer::constants(const materials::WaterSurfaceSettings &settings,
                                                              const materials::WaterSurfaceFrame &frame,
                                                              const materials::WaterSurfaceView &view) const {
    return materials::water_surface_constants(impl_->domain, settings, frame, view, impl_->precision);
}
terrain::CameraInputResult GlWaterWorldRenderer::prepare_camera_input(
    const terrain::CameraControllerState &controller, const terrain::CameraPointerState &pointer,
    const terrain::CameraInputFrame &input, const materials::WaterReflectionFrameSettings &settings,
    const terrain::ReciprocalEstimate &reciprocal, const terrain::ReciprocalSqrtEstimate &reciprocal_sqrt) {
    const FrameOperation operation(impl_->frame_operations);
    const auto &source = impl_->world->source();
    auto result = terrain::advance_camera_input(source.layout, source.map_mode, controller, pointer, input,
                                               impl_->precision, reciprocal);
    prepare_camera_frame(result.motion.controller.camera, settings, reciprocal, reciprocal_sqrt);
    return result;
}
materials::WaterConstantBlock GlWaterWorldRenderer::constants(std::uint32_t view_index,
                                                              const materials::WaterSurfaceSettings &settings,
                                                              const materials::WaterSurfaceFrame &frame,
                                                              const materials::WaterSurfaceView &view) const {
    const auto &state = frame_state();
    require(impl_->surface_policy==Impl::SurfacePolicy::owned_surface,"External surface camera mode has no legacy draw constants");
    require(view_index < impl_->prepared->mesh.counts.size() / 256, "Unprepared water camera view");
    auto prepared_view = view;
    auto prepared_frame = frame;
    prepared_view.shadow = state.matrices.shadow[view_index];
    prepared_view.reflection = state.matrices.reflection[view_index];
    if (impl_->prepared->camera) {
        const auto &camera = *impl_->prepared->camera;
        prepared_frame.projection = camera.projection;
        prepared_view.view = camera.views[view_index];
        std::copy_n(camera.inverse_views[view_index].begin() + 12, 3, prepared_view.camera.begin());
    }
    return constants(settings, prepared_frame, prepared_view);
}
void GlWaterWorldRenderer::draw(std::uint32_t view, const materials::WaterConstantBlock &constants,
                                GlWaterSurfaceInputs scene, int width, int height) {
    auto &s = *impl_;
    require(s.surface_policy==Impl::SurfacePolicy::owned_surface,"External surface camera mode cannot draw legacy water");
    require(s.prepared && view < s.prepared->mesh.counts.size() / 256, "Unprepared water view");
    scene.pixel[0] = s.rgba.id();
    scene.pixel[1] = s.luminance.id();
    scene.pixel[6] = s.color.id();
    if (s.current)
        scene.pixel[5] = s.current->texture().id();
    if (s.reflection)
        scene.pixel[4] = s.reflection->texture().id();
    for (const auto &draw : s.prepared->draws) {
        if (draw.view != view)
            continue;
        scene.pixel[7] = s.heights.at(draw.window).id();
        s.prepared->renderer->draw(constants, scene, draw.first_index, draw.triangle_count, width, height);
    }
}
const materials::WaterSurfaceMesh &GlWaterWorldRenderer::mesh() const {
    require(impl_->surface_policy==Impl::SurfacePolicy::owned_surface,"External surface camera mode has no legacy mesh");
    require(bool(impl_->prepared), "Unprepared water mesh");
    return impl_->prepared->mesh;
}
std::span<const materials::WaterSurfaceDraw> GlWaterWorldRenderer::draws() const {
    require(impl_->surface_policy==Impl::SurfacePolicy::owned_surface,"External surface camera mode has no legacy draw packets");
    require(bool(impl_->prepared), "Unprepared water draws");
    return impl_->prepared->draws;
}
const GlTexture &GlWaterWorldRenderer::height_texture(std::uint32_t window) const {
    const auto found = impl_->heights.find(window);
    require(found != impl_->heights.end(), "Unknown water height window");
    return found->second;
}
const GlTexture &GlWaterWorldRenderer::wave_texture(assets::LeanMap map) const {
    require(map == assets::LeanMap::rgba16 || map == assets::LeanMap::luminance16, "Invalid water LEAN map");
    return map == assets::LeanMap::rgba16 ? impl_->rgba : impl_->luminance;
}
const GlTexture &GlWaterWorldRenderer::color_texture() const { return impl_->color; }
const materials::WaterSurfaceDomain &GlWaterWorldRenderer::domain() const { return impl_->domain; }
void GlWaterWorldRenderer::rebuild_context(std::shared_ptr<GlContext> context, const assets::LeanData &waves,
                                            const assets::DdsData &color,
                                            std::shared_ptr<const GlTexture> shared_depth) {
    auto &old = *impl_;
    require(context != nullptr, "Water context recovery needs a live replacement context");
    require(context != old.context, "Water context recovery requires a different OpenGL context");
    require(!old.reflection_active, "Water context recovery cannot interrupt reflection rendering");
    require(old.frame_operations == 0, "Water context recovery cannot interrupt camera preparation");
    if (old.reflection && old.reflection->resource().render_target)
        require(shared_depth && shared_depth->belongs_to(*context),
                "Water context recovery needs replacement reflection depth");

    // Build the replacement completely before swapping it into the owner. A
    // failed source upload leaves the old water renderer usable on its context.
    auto replacement = std::make_unique<Impl>(context, old.world, waves, color, old.selector, old.precision);
    replacement->surface_policy=old.surface_policy;
    if (old.prepared) {
        auto prepared = std::make_unique<Impl::Prepared>();
        if(old.surface_policy==Impl::SurfacePolicy::owned_surface){
            prepared->mesh=old.prepared->mesh;prepared->draws=old.prepared->draws;
            prepared->renderer = std::make_unique<GlWaterSurfaceRenderer>(context,prepared->mesh,old.selector);
        }
        prepared->frame = old.prepared->frame;
        prepared->camera = old.prepared->camera;
        replacement->prepared = std::move(prepared);
    }
    if (old.current_parameters) {
        const auto &grid = old.world->source().layout.worker_grid;
        replacement->current = std::make_unique<GlWaterCurrentMap>(
            context, grid.width, grid.height, old.current_parameters->first, old.current_parameters->second);
        replacement->current_parameters = old.current_parameters;
    }
    if (old.reflection_parameters) {
        replacement->reflection = std::make_unique<GlWaterReflectionMap>(
            context, old.reflection_parameters->first, old.reflection_parameters->second,
            old.reflection && old.reflection->resource().render_target ? std::move(shared_depth) : nullptr);
        replacement->reflection_parameters = old.reflection_parameters;
    }
    impl_.swap(replacement);
}
} // namespace civ5::graphics
