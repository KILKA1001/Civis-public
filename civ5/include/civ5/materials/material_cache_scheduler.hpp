#pragma once
#include "civ5/terrain/camera_frame.hpp"
#include "civ5/terrain/camera_controller.hpp"
#include "material_cache_lifecycle.hpp"
#include "material_cache_projection.hpp"
#include "material_cache_startup.hpp"
#include <vector>

namespace civ5::materials {
// A scheduler record references the same persistent state that owns submitted
// jobs and GPU resources. No x86 object layout or copied lifecycle state is
// used.
struct MaterialCacheSchedulerPatch {
    MaterialCacheLifecycleState *state = nullptr;
    std::array<float, 6> bounds{};
    std::array<float, 4> sphere{}; // original +124 center XYZ and radius
    std::array<float, 6> plane_distances{};
    std::uint32_t enabled = 0, blocked = 0, priority = 0, eviction_stamp = 0;
    std::uint32_t source_revision = 0; // compared to the live state's frame_stamp
    std::int32_t material_parent_flag = 0; // +300, set by source-region refresh
    bool frustum_visible = false, submitted_view = false;
};
struct MaterialCacheSchedulerView {
    terrain::Matrix4 matrix;
    terrain::ViewFrustum frustum;
};
struct MaterialCacheSchedulerSettings {
    // Original D91120, from CameraFrameState::projection. Selection uses
    // view * projection in the original dispatch-x87 matrix order.
    terrain::Matrix4 projection;
    std::uint32_t viewport_width = 0, base_size = 64;
    std::uint32_t attempt_budget = 0, fast_selection_budget = 0;
    bool fast = false, force_fast = false, cull = false, compressed_base = false;
    terrain::X87Precision precision = terrain::X87Precision::double_precision;
    terrain::Vec4Path vector_path = terrain::Vec4Path::sse;
    std::uintptr_t dispatch = 0;
};
// Original renderer +75FE4, initialized to zero by 779529. The registered
// 4B3D10/5DAC90 consumers follow start/stop notifications from 53D970;
// camera-change and wrap notifications do not change this retained state.
struct MaterialCacheCameraState {
    bool moving = false;
    void consume(std::span<const terrain::CameraMotionEvent> events);
};
// Connect the recovered startup controls to the scheduler. The still budget
// limits higher-cache selection; fast mode retains the original eight-attempt
// limit inside material_cache_schedule. No cold-start override is introduced.
// Projection, viewport, culling, resources and arithmetic stay caller-owned.
void material_cache_camera_controls(const MaterialCacheCameraState &, const MaterialCacheStartup &,
                                    MaterialCacheSchedulerSettings &);
// Copy every active original camera view/frustum in order (0..4). Wrapped
// views and repeated patch membership must not be discarded or deduplicated.
std::vector<MaterialCacheSchedulerView> material_cache_camera_views(const terrain::CameraFrameState &);
struct MaterialCacheSchedulerState {
    std::uint32_t background_cursor = 0;
    std::array<std::vector<std::uint32_t>, 4> visible;
    std::vector<std::uint32_t> eviction_candidates;
    // Original +1B00 completion queue. Existing entries are retained; the
    // owner drains it after preparation/workers, outside this selection call.
    std::vector<std::uint32_t> pending;
};
class MaterialCacheSchedulerBackend {
  public:
    virtual ~MaterialCacheSchedulerBackend() = default;
    virtual std::size_t free_count(std::uint32_t width) const = 0;
    virtual std::size_t compressed_base_free_count() const = 0;
    virtual bool acquire_base(std::uint32_t patch) = 0;
    virtual bool acquire_higher(std::uint32_t patch) = 0;
    // Original 68ADB0: return higher textures/invalidate, then completed=1.
    virtual void downgrade(std::uint32_t patch) = 0;
    virtual void invalidate_geometry(std::uint32_t patch) = 0;
    // Original 455A60 argument order; resource callbacks see live patch state.
    virtual void release_cache(std::uint32_t patch, bool geometry, bool final, bool higher) = 0;
    // Original 6920C0 material selection and 6BAB00 preparation respectively.
    virtual void refresh_selection(std::uint32_t patch) = 0;
    virtual void prepare(std::uint32_t patch, std::uintptr_t dispatch) = 0;
};
// Original 536A80. All six SSE-order distances are calculated; distances are
// published only for a visible sphere. An invisible result preserves old ones.
bool material_cache_cull(MaterialCacheSchedulerPatch &, const terrain::ViewFrustum &);
// Full outer 644660/6A31C0 selection, eviction and preparation-call order.
// Supports 0..4 views, every completed-state branch, background preparation,
// repeated view membership and growing pending/selection vectors. Backends
// execute actual resource/job operations; this call never completes jobs or
// increments pending_jobs. Owner serialization and stable records are required.
bool material_cache_schedule(std::span<MaterialCacheSchedulerPatch> patches,
                             std::span<const std::uint32_t> active, std::span<const std::uint32_t> background,
                             std::span<const MaterialCacheSchedulerView> views,
                             const MaterialCacheSchedulerSettings &, MaterialCacheSchedulerState &,
                             MaterialCacheSchedulerBackend &);
} // namespace civ5::materials
