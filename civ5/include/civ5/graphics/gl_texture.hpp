#pragma once
#include "civ5/assets/dds.hpp"
#include "civ5/assets/lean.hpp"
#include "civ5/assets/rgba8.hpp"
#include "gl_context.hpp"

#include <cstdint>
#include <span>

namespace civ5::graphics {
// Explicit native sampler decode, never inferred from filenames. sRGB is
// supported for BC1/BC2/BC3 and 32-bit RGBA/BGRA (including X8) resources.
enum class DdsSamplingColor { linear, srgb };
// Borrowed immutable producer pixels. Caller retains the original byte owners
// and this level array, without mutation, throughout the synchronous upload.
// This view does not issue CPU-image provenance or resource-registration rights.
struct Bgra8Level {
    std::uint32_t width, height;
    std::span<const std::uint8_t> bgra;
};
enum class RenderTextureFormat { rgba16_float, rgba8, rgb565, depth24_stencil8, rgba32_float };
struct DdsUploadPlan {
    GLenum internal_format = 0, transfer_format = 0, transfer_type = 0;
    std::array<GLint, 4> swizzle{GL_RED, GL_GREEN, GL_BLUE, GL_ALPHA};
    bool compressed = false;
};
// Explicit D3D9 sampling semantics, including missing-channel defaults.
// No automatic sRGB or CPU RGBA8 decode; unverified masks fail explicitly.
DdsUploadPlan dds_upload_plan(const assets::DdsData &data, DdsSamplingColor color = DdsSamplingColor::linear);

class GlTexture {
  public:
    GlTexture(const GlContext &context, const assets::DdsData &data,
              DdsSamplingColor color = DdsSamplingColor::linear);
    // Direct authored LEAN type-2 map, with native L16 channel replication.
    static GlTexture lean(const GlContext &context, const assets::LeanData &data, assets::LeanMap map);
    // Producer-owned A8 bytes, e.g. extracted LAND height windows. No fake DDS.
    static GlTexture alpha8(const GlContext &context, std::uint32_t width, std::uint32_t height,
                            std::span<const std::uint8_t> bytes);
    // Explicit contiguous mip chain of producer pixels, including incomplete
    // chains. No DDS wrapper, mip generation, alpha replacement or sRGB guess.
    static GlTexture rgba8(const GlContext &context, std::span<const assets::Rgba8Level> levels,
                           DdsSamplingColor color = DdsSamplingColor::linear);
    // Explicit BGRA byte order; uploads every supplied mip unchanged, including
    // incomplete chains. No conversion, DDS wrapper or generated levels.
    static GlTexture bgra8(const GlContext &context, std::span<const Bgra8Level> levels,
                           DdsSamplingColor color = DdsSamplingColor::linear);
    // Project-produced cube pixels (e.g. the measured coordinate probe), NOT
    // authored light/color correction. Linear RGBA8; explicit face/mip bytes.
    static GlTexture rgba8_cube(const GlContext &context, std::span<const assets::Rgba8CubeLevel> levels);
    // Original A8R8G8B8 volume bytes, e.g. ColorKeySystem's 32^3 LUT.
    // One linear UNORM level; dimensions and BGRA bytes are explicit.
    static GlTexture bgra8_volume(const GlContext &context, std::uint32_t width, std::uint32_t height,
                                  std::uint32_t depth, std::span<const std::uint8_t> bytes);
    // Uninitialized producer-owned render storage. Effective mip count is
    // explicit; a native request for automatic mips is not a device guarantee.
    static GlTexture render_target(const GlContext &context, std::uint32_t width, std::uint32_t height,
                                   std::uint32_t mip_count, RenderTextureFormat format);
    // Call only after level zero has been fully produced. GL owns filtering;
    // pixel equivalence of generated levels is validated separately per device.
    void generate_mip_chain() const;
    ~GlTexture();
    GlTexture(const GlTexture &) = delete;
    GlTexture &operator=(const GlTexture &) = delete;
    GlTexture(GlTexture &&) noexcept;
    GlTexture &operator=(GlTexture &&) noexcept;
    GLuint id() const { return id_; }
    std::uint32_t face_count() const { return faces_; }
    std::uint32_t depth() const { return depth_; }
    bool belongs_to(const GlContext &context) const;
    const DdsUploadPlan &plan() const { return plan_; }
    const std::vector<assets::DdsLevel> &levels() const { return levels_; }
    assets::Bytes read_level_raw(std::uint32_t face, std::uint32_t mip) const;
    // Whole mip, face-major for cubes; avoids partial compressed-image reads.
    assets::Bytes read_mip_raw(std::uint32_t mip) const;
    void bind(GLuint unit) const;

  private:
    GlTexture() = default;
    // Keeps the actual WGL context alive until its last resource is released.
    // All operations/destruction remain on the creating render thread.
    std::shared_ptr<GlContextState> context_;
    GLuint id_ = 0;
    DdsUploadPlan plan_;
    std::vector<assets::DdsLevel> levels_;
    std::uint32_t faces_ = 0, mips_ = 0;
    std::uint32_t depth_ = 1;
    void release() noexcept;
};
} // namespace civ5::graphics
