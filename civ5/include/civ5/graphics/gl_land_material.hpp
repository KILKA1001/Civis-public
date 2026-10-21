#pragma once
#include "civ5/materials/land_material.hpp"
#include "gl_context.hpp"
#include "gl_land_geometry_pool.hpp"
#include "civ5/materials/material_cache_passes.hpp"
#include <functional>
#include <string>
#include <string_view>
namespace civ5::graphics {
// Vertex source retains D3D clip Z; draw() supplies GL_ZERO_TO_ONE and the
// half-pixel viewport offset, restoring the caller's clip/viewport state.
std::string_view land_material_vertex_shader(materials::LandVertexInput);
std::string_view land_material_fragment_shader(materials::LandPixelProgram);
// Original key49 ten pixel slots, plus displacement in a distinct GL unit10.
struct GlLandMaterialInputs {
    std::array<GLuint, 10> pixel{};
    GLuint height = 0;
};
// Explicit device-measured arithmetic profile, not a universal SM3 guarantee.
enum class LandShadowArithmetic { instruction_order, measured_fma_t_x_z_y };
enum class LandMipSampling { ordinary, d3d9_trilinear, d3d9_rgb565 };
// The RGB565 profile is device-measured: cache slot0 expands to byte codes,
// filters with eight fractional bits, then normalizes after mip interpolation.
// It requires a GL_RGB565 cache and leaves authored cloud slot5 on hardware.
std::string land_material_fragment_source(materials::LandPixelProgram, LandShadowArithmetic,
                                          LandMipSampling = LandMipSampling::ordinary);
class GlLandMaterialRenderer {
  public:
    GlLandMaterialRenderer(std::shared_ptr<GlContext>, const materials::LandMaterialMesh &, unsigned selector,
                           LandShadowArithmetic = LandShadowArithmetic::instruction_order,
                           LandMipSampling = LandMipSampling::ordinary);
    GlLandMaterialRenderer(std::shared_ptr<GlContext>, std::shared_ptr<GlLandGeometryPool>,
                           const GlLandGeometryLease &, unsigned selector,
                           LandShadowArithmetic = LandShadowArithmetic::instruction_order,
                           LandMipSampling = LandMipSampling::ordinary);
    ~GlLandMaterialRenderer();
    GlLandMaterialRenderer(const GlLandMaterialRenderer &) = delete;
    GlLandMaterialRenderer &operator=(const GlLandMaterialRenderer &) = delete;
    // Rebind a live lease from the same retained pool without rebuilding shaders.
    // Old leases are checked again before every draw/capture.
    void set_geometry(const GlLandGeometryLease &);
    // Caller owns complete textures/target(s), viewport/raster/blend/depth state.
    // Raw texture names must belong to this context; storage-view aliases excluded
    // by caller. API conversion preserves original top-first local raster mapping.
    void draw(const materials::LandMaterialConstants &, const GlLandMaterialInputs &, int width, int height);
    // Consumes an original key49 packet against this renderer's live geometry
    // lease. The resolver maps packet resource IDs to actual same-context GL
    // textures; zero is the unbound resource. Sampler/geometry fields are checked
    // before resolving resources or issuing a draw.
    void draw_packet(const materials::MaterialCachePacket &, const materials::LandMaterialConstants &,
                       const std::function<GLuint(std::uint16_t)> &resolve, int width, int height);
    // Native clip4,world4,cache/fow4,composite4,world3,ndc2 =21 floats per vertex.
    // Captures original pre-API-conversion varyings, no final pixel shortcut.
    std::vector<float> capture(const materials::LandMaterialConstants &, GLuint height, bool indexed);

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace civ5::graphics
