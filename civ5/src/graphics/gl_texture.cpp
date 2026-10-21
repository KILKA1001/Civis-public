#include "civ5/graphics/gl_texture.hpp"
#include "gl_context_internal.hpp"
#include <limits>
#include <stdexcept>
#include <utility>

namespace civ5::graphics {
namespace {
using Mask = std::array<std::uint32_t, 4>;
// EXT_texture_compression_s3tc; not OpenGL Core enumerants.
constexpr GLenum dxt1 = 0x83f1, dxt3 = 0x83f2, dxt5 = 0x83f3;
// Save every affected pixel-store field, including PBOs and compressed blocks.
// A previously bound PBO must never reinterpret a CPU pointer as an offset.
class PixelTransfer {
    const GlApi &gl_;
    bool pack_;
    GLint buffer_ = 0;
    std::array<GLint, 12> old_{};
    std::array<GLenum, 12> fields_;

  public:
    PixelTransfer(const GlApi &gl, bool pack)
        : gl_(gl), pack_(pack),
          fields_(pack ? std::array<GLenum, 12>{GL_PACK_ALIGNMENT, GL_PACK_ROW_LENGTH, GL_PACK_IMAGE_HEIGHT,
                                                GL_PACK_SKIP_PIXELS, GL_PACK_SKIP_ROWS, GL_PACK_SKIP_IMAGES,
                                                GL_PACK_SWAP_BYTES, GL_PACK_LSB_FIRST,
                                                GL_PACK_COMPRESSED_BLOCK_WIDTH,
                                                GL_PACK_COMPRESSED_BLOCK_HEIGHT,
                                                GL_PACK_COMPRESSED_BLOCK_DEPTH, GL_PACK_COMPRESSED_BLOCK_SIZE}
                       : std::array<GLenum, 12>{
                             GL_UNPACK_ALIGNMENT, GL_UNPACK_ROW_LENGTH, GL_UNPACK_IMAGE_HEIGHT,
                             GL_UNPACK_SKIP_PIXELS, GL_UNPACK_SKIP_ROWS, GL_UNPACK_SKIP_IMAGES,
                             GL_UNPACK_SWAP_BYTES, GL_UNPACK_LSB_FIRST, GL_UNPACK_COMPRESSED_BLOCK_WIDTH,
                             GL_UNPACK_COMPRESSED_BLOCK_HEIGHT, GL_UNPACK_COMPRESSED_BLOCK_DEPTH,
                             GL_UNPACK_COMPRESSED_BLOCK_SIZE}) {
        gl_.GetIntegerv(pack_ ? GL_PIXEL_PACK_BUFFER_BINDING : GL_PIXEL_UNPACK_BUFFER_BINDING, &buffer_);
        for (std::size_t i = 0; i < fields_.size(); ++i)
            gl_.GetIntegerv(fields_[i], &old_[i]);
        gl_.BindBuffer(pack_ ? GL_PIXEL_PACK_BUFFER : GL_PIXEL_UNPACK_BUFFER, 0);
        for (std::size_t i = 0; i < fields_.size(); ++i)
            gl_.PixelStorei(fields_[i], i == 0 ? 1 : 0);
    }
    ~PixelTransfer() {
        for (std::size_t i = 0; i < fields_.size(); ++i)
            gl_.PixelStorei(fields_[i], old_[i]);
        gl_.BindBuffer(pack_ ? GL_PIXEL_PACK_BUFFER : GL_PIXEL_UNPACK_BUFFER, static_cast<GLuint>(buffer_));
    }
};
class TextureBinding {
    const GlApi &gl_;
    GLenum target_;
    GLint old_ = 0;

  public:
    TextureBinding(const GlApi &gl, GLuint texture, bool cube)
        : gl_(gl), target_(cube ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D) {
        gl_.GetIntegerv(cube ? GL_TEXTURE_BINDING_CUBE_MAP : GL_TEXTURE_BINDING_2D, &old_);
        gl_.BindTexture(target_, texture);
    }
    ~TextureBinding() { gl_.BindTexture(target_, static_cast<GLuint>(old_)); }
};
} // namespace
DdsUploadPlan dds_upload_plan(const assets::DdsData &data, DdsSamplingColor color) {
    using F = assets::DdsFormat;
    DdsUploadPlan p;
    if (color != DdsSamplingColor::linear && color != DdsSamplingColor::srgb)
        throw std::invalid_argument("Invalid explicit DDS sampling color");
    const bool srgb = color == DdsSamplingColor::srgb;
    const auto masks = data.channel_masks();
    const bool rgba8 =
        data.format() == F::masked_rgb && data.bits_per_pixel() == 32 &&
        (masks == Mask{0xff0000, 0xff00, 0xff, 0xff000000} || masks == Mask{0xff0000, 0xff00, 0xff, 0} ||
         masks == Mask{0xff, 0xff00, 0xff0000, 0xff000000} || masks == Mask{0xff, 0xff00, 0xff0000, 0});
    if (srgb && !rgba8 && data.format() != F::dxt1 && data.format() != F::dxt3 && data.format() != F::dxt5)
        throw std::invalid_argument("No verified sRGB upload mapping for this DDS format");
    switch (data.format()) {
    case F::dxt1:
        p.internal_format = srgb ? 0x8c4d : dxt1;
        p.compressed = true;
        return p;
    case F::dxt3:
        p.internal_format = srgb ? 0x8c4e : dxt3;
        p.compressed = true;
        return p;
    case F::dxt5:
        p.internal_format = srgb ? 0x8c4f : dxt5;
        p.compressed = true;
        return p;
    case F::alpha8:
        return {GL_R8, GL_RED, GL_UNSIGNED_BYTE, {GL_ZERO, GL_ZERO, GL_ZERO, GL_RED}};
    case F::rgba16_unorm:
        return {GL_RGBA16, GL_RGBA, GL_UNSIGNED_SHORT};
    case F::rgba16_snorm:
        return {GL_RGBA16_SNORM, GL_RGBA, GL_SHORT};
    case F::r16_float:
        return {GL_R16F, GL_RED, GL_HALF_FLOAT, {GL_RED, GL_ONE, GL_ONE, GL_ONE}};
    case F::rg16_float:
        return {GL_RG16F, GL_RG, GL_HALF_FLOAT, {GL_RED, GL_GREEN, GL_ONE, GL_ONE}};
    case F::rgba16_float:
        return {GL_RGBA16F, GL_RGBA, GL_HALF_FLOAT};
    case F::r32_float:
        return {GL_R32F, GL_RED, GL_FLOAT, {GL_RED, GL_ONE, GL_ONE, GL_ONE}};
    case F::rg32_float:
        return {GL_RG32F, GL_RG, GL_FLOAT, {GL_RED, GL_GREEN, GL_ONE, GL_ONE}};
    case F::rgba32_float:
        return {GL_RGBA32F, GL_RGBA, GL_FLOAT};
    default:
        break;
    }
    const auto &m = data.channel_masks();
    const auto bpp = data.bits_per_pixel();
    if (data.format() == F::luminance) {
        if (bpp == 8 && m == Mask{255, 0, 0, 0})
            return {GL_R8, GL_RED, GL_UNSIGNED_BYTE, {GL_RED, GL_RED, GL_RED, GL_ONE}};
        if (bpp == 16 && m == Mask{65535, 0, 0, 0})
            return {GL_R16, GL_RED, GL_UNSIGNED_SHORT, {GL_RED, GL_RED, GL_RED, GL_ONE}};
        if (bpp == 16 && m == Mask{255, 0, 0, 0xff00})
            return {GL_RG8, GL_RG, GL_UNSIGNED_BYTE, {GL_RED, GL_RED, GL_RED, GL_GREEN}};
    } else if (data.format() == F::masked_rgb) {
        if (bpp == 32 &&
            (m == Mask{0xff0000, 0xff00, 0xff, 0xff000000} || m == Mask{0xff0000, 0xff00, 0xff, 0}))
            return {static_cast<GLenum>(srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8),
                    GL_BGRA,
                    GL_UNSIGNED_BYTE,
                    {GL_RED, GL_GREEN, GL_BLUE, m[3] ? GL_ALPHA : GL_ONE}};
        if (bpp == 32 &&
            (m == Mask{0xff, 0xff00, 0xff0000, 0xff000000} || m == Mask{0xff, 0xff00, 0xff0000, 0}))
            return {static_cast<GLenum>(srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8),
                    GL_RGBA,
                    GL_UNSIGNED_BYTE,
                    {GL_RED, GL_GREEN, GL_BLUE, m[3] ? GL_ALPHA : GL_ONE}};
        if (bpp == 32 && m == Mask{0xffff, 0xffff0000, 0, 0})
            return {GL_RG16, GL_RG, GL_UNSIGNED_SHORT, {GL_RED, GL_GREEN, GL_ONE, GL_ONE}};
        if (bpp == 24 && m == Mask{0xff0000, 0xff00, 0xff, 0})
            return {GL_RGB8, GL_BGR, GL_UNSIGNED_BYTE};
        if (bpp == 16 && m == Mask{0xf800, 0x7e0, 0x1f, 0})
            return {GL_RGB565, GL_RGB, GL_UNSIGNED_SHORT_5_6_5};
        if (bpp == 16 && (m == Mask{0x7c00, 0x3e0, 0x1f, 0x8000} || m == Mask{0x7c00, 0x3e0, 0x1f, 0}))
            return {GL_RGB5_A1,
                    GL_BGRA,
                    GL_UNSIGNED_SHORT_1_5_5_5_REV,
                    {GL_RED, GL_GREEN, GL_BLUE, m[3] ? GL_ALPHA : GL_ONE}};
        if (bpp == 16 && (m == Mask{0xf00, 0xf0, 0xf, 0xf000} || m == Mask{0xf00, 0xf0, 0xf, 0}))
            return {GL_RGBA4,
                    GL_BGRA,
                    GL_UNSIGNED_SHORT_4_4_4_4_REV,
                    {GL_RED, GL_GREEN, GL_BLUE, m[3] ? GL_ALPHA : GL_ONE}};
    }
    throw std::invalid_argument("DDS masks have no verified OpenGL transfer mapping; no RGBA8 fallback");
}
GlTexture::GlTexture(const GlContext &context, const assets::DdsData &data, DdsSamplingColor color)
    : context_(context.state_), plan_(dds_upload_plan(data, color)), levels_(data.levels()),
      faces_(data.face_count()), mips_(data.mip_count()) {
    context_->current();
    auto &gl = context_->gl;
    context_->check("Before DDS upload");
    if (plan_.compressed && !context_->s3tc)
        throw std::runtime_error("S3TC extension required for original compressed DDS");
    GLint maximum = 0;
    gl.GetIntegerv(faces_ == 6 ? GL_MAX_CUBE_MAP_TEXTURE_SIZE : GL_MAX_TEXTURE_SIZE, &maximum);
    if (data.width() > static_cast<std::uint32_t>(maximum) ||
        data.height() > static_cast<std::uint32_t>(maximum))
        throw std::invalid_argument("DDS exceeds GPU limits");
    try {
        {
            PixelTransfer transfer(gl, false);
            gl.CreateTextures(faces_ == 6 ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D, 1, &id_);
            gl.TextureStorage2D(id_, mips_, plan_.internal_format, data.width(), data.height());
            gl.TextureParameteri(id_, GL_TEXTURE_BASE_LEVEL, 0);
            gl.TextureParameteri(id_, GL_TEXTURE_MAX_LEVEL, mips_ - 1);
            gl.TextureParameteriv(id_, GL_TEXTURE_SWIZZLE_RGBA, plan_.swizzle.data());
            for (const auto &level : levels_) {
                const auto bytes = data.payload(level.face, level.mip);
                if (bytes.size() > static_cast<std::size_t>(std::numeric_limits<GLsizei>::max()))
                    throw std::invalid_argument("DDS level exceeds OpenGL transfer size");
                const auto size = static_cast<GLsizei>(bytes.size());
                if (plan_.compressed) {
                    if (faces_ == 6)
                        gl.CompressedTextureSubImage3D(id_, level.mip, 0, 0, level.face, level.width,
                                                       level.height, 1, plan_.internal_format, size,
                                                       bytes.data());
                    else
                        gl.CompressedTextureSubImage2D(id_, level.mip, 0, 0, level.width, level.height,
                                                       plan_.internal_format, size, bytes.data());
                } else {
                    if (faces_ == 6)
                        gl.TextureSubImage3D(id_, level.mip, 0, 0, level.face, level.width, level.height, 1,
                                             plan_.transfer_format, plan_.transfer_type, bytes.data());
                    else
                        gl.TextureSubImage2D(id_, level.mip, 0, 0, level.width, level.height,
                                             plan_.transfer_format, plan_.transfer_type, bytes.data());
                }
                context_->check("DDS authored face/mip upload");
            }
        }
        context_->check("DDS pixel-store restoration");
    } catch (...) {
        release();
        throw;
    }
}
GlTexture GlTexture::alpha8(const GlContext &context, std::uint32_t width, std::uint32_t height,
                            std::span<const std::uint8_t> bytes) {
    if (!width || !height || std::uint64_t{width} * height != bytes.size())
        throw std::invalid_argument("A8 producer extent differs from payload");
    context.state_->current();
    context.check("Before A8 producer upload");
    const auto &gl = context.api();
    GLint limit = 0;
    gl.GetIntegerv(GL_MAX_TEXTURE_SIZE, &limit);
    if (width > static_cast<std::uint32_t>(limit) || height > static_cast<std::uint32_t>(limit))
        throw std::invalid_argument("A8 producer exceeds GPU limits");
    GlTexture result;
    result.context_ = context.state_;
    result.plan_ = {GL_R8, GL_RED, GL_UNSIGNED_BYTE, {GL_ZERO, GL_ZERO, GL_ZERO, GL_RED}};
    result.faces_ = result.mips_ = 1;
    result.levels_.push_back({0, 0, width, height, 0, bytes.size(), width});
    context.state_->current();
    {
        PixelTransfer transfer(gl, false);
        gl.CreateTextures(GL_TEXTURE_2D, 1, &result.id_);
        gl.TextureStorage2D(result.id_, 1, GL_R8, width, height);
        gl.TextureParameteri(result.id_, GL_TEXTURE_BASE_LEVEL, 0);
        gl.TextureParameteri(result.id_, GL_TEXTURE_MAX_LEVEL, 0);
        gl.TextureParameteriv(result.id_, GL_TEXTURE_SWIZZLE_RGBA, result.plan_.swizzle.data());
        gl.TextureSubImage2D(result.id_, 0, 0, 0, width, height, GL_RED, GL_UNSIGNED_BYTE, bytes.data());
        context.check("A8 producer upload");
    }
    context.check("A8 producer upload state restoration");
    return result;
}
GlTexture GlTexture::render_target(const GlContext &context, std::uint32_t width, std::uint32_t height,
                                   std::uint32_t mip_count, RenderTextureFormat format) {
    if (!width || !height || !mip_count)
        throw std::invalid_argument("Render texture requires positive dimensions and mip count");
    std::uint32_t maximum_levels = 1;
    for (auto dimension = std::max(width, height); dimension > 1; dimension /= 2)
        ++maximum_levels;
    if (mip_count > maximum_levels)
        throw std::invalid_argument("Render texture mip count exceeds its extent");
    DdsUploadPlan plan;
    std::size_t bytes_per_pixel = 0;
    switch (format) {
    case RenderTextureFormat::rgba32_float:
        plan = {GL_RGBA32F, GL_RGBA, GL_FLOAT};
        bytes_per_pixel = 16;
        break;
    case RenderTextureFormat::rgba16_float:
        plan = {GL_RGBA16F, GL_RGBA, GL_HALF_FLOAT};
        bytes_per_pixel = 8;
        break;
    case RenderTextureFormat::rgba8:
        plan = {GL_RGBA8, GL_BGRA, GL_UNSIGNED_BYTE};
        bytes_per_pixel = 4;
        break;
    case RenderTextureFormat::rgb565:
        plan = {GL_RGB565, GL_RGB, GL_UNSIGNED_SHORT_5_6_5};
        bytes_per_pixel = 2;
        break;
    case RenderTextureFormat::depth24_stencil8:
        if (mip_count != 1)
            throw std::invalid_argument("Depth/stencil storage requires one level");
        plan = {GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8};
        bytes_per_pixel = 4;
        break;
    default:
        throw std::invalid_argument("Unsupported render texture format");
    }
    const auto &gl = context.api();
    context.check("Before render texture allocation");
    GLint limit = 0;
    gl.GetIntegerv(GL_MAX_TEXTURE_SIZE, &limit);
    if (limit <= 0 || width > static_cast<std::uint32_t>(limit) || height > static_cast<std::uint32_t>(limit))
        throw std::invalid_argument("Render texture exceeds GPU limits");
    GlTexture result;
    result.context_ = context.state_;
    result.plan_ = plan;
    result.faces_ = 1;
    result.mips_ = mip_count;
    std::size_t offset = 0;
    auto w = width, h = height;
    for (std::uint32_t mip = 0; mip < mip_count; ++mip) {
        const std::size_t row = std::size_t(w) * bytes_per_pixel, size = row * h;
        result.levels_.push_back({0, mip, w, h, offset, size, row});
        offset += size;
        w = std::max(1u, w / 2);
        h = std::max(1u, h / 2);
    }
    context.state_->current();
    gl.CreateTextures(GL_TEXTURE_2D, 1, &result.id_);
    gl.TextureStorage2D(result.id_, mip_count, plan.internal_format, width, height);
    gl.TextureParameteri(result.id_, GL_TEXTURE_BASE_LEVEL, 0);
    gl.TextureParameteri(result.id_, GL_TEXTURE_MAX_LEVEL, mip_count - 1);
    context.check("Native-format render texture storage");
    return result;
}
void GlTexture::generate_mip_chain() const {
    if (!context_ || !id_ || faces_ != 1 || plan_.compressed)
        throw std::invalid_argument("Mip generation requires a live uncompressed 2D texture");
    context_->current();
    context_->check("Before render mip generation");
    if (mips_ > 1) {
        const auto generate =
            reinterpret_cast<PFNGLGENERATETEXTUREMIPMAPPROC>(context_->address("glGenerateTextureMipmap"));
        generate(id_);
        context_->check("Generate render texture mip chain");
    }
}
GlTexture GlTexture::rgba8(const GlContext &context, std::span<const assets::Rgba8Level> levels,
                           DdsSamplingColor color) {
    if (color != DdsSamplingColor::linear && color != DdsSamplingColor::srgb)
        throw std::invalid_argument("Invalid RGBA8 sampler decode");
    if (levels.empty() || !levels[0].width || !levels[0].height)
        throw std::invalid_argument("RGBA8 producer requires nonempty mip dimensions");
    auto width = levels[0].width, height = levels[0].height;
    for (std::size_t i = 0; i < levels.size(); ++i) {
        const auto &level = levels[i];
        // Division avoids overflow for caller-controlled U32 dimensions.
        if (level.width != width || level.height != height || level.rgba.size() % 4 != 0 ||
            std::uint64_t{width} * height != level.rgba.size() / 4 ||
            (width == 1 && height == 1 && i + 1 != levels.size()))
            throw std::invalid_argument("Invalid RGBA8 producer mip chain/payload");
        width = std::max(1u, width / 2);
        height = std::max(1u, height / 2);
    }
    context.state_->current();
    context.check("Before RGBA8 producer upload");
    const auto &gl = context.api();
    GLint limit = 0;
    gl.GetIntegerv(GL_MAX_TEXTURE_SIZE, &limit);
    if (levels[0].width > static_cast<std::uint32_t>(limit) ||
        levels[0].height > static_cast<std::uint32_t>(limit))
        throw std::invalid_argument("RGBA8 producer exceeds GPU limits");
    GlTexture result;
    result.context_ = context.state_;
    result.plan_ = {static_cast<GLenum>(color == DdsSamplingColor::srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8),
                    GL_RGBA, GL_UNSIGNED_BYTE};
    result.faces_ = 1;
    result.mips_ = static_cast<std::uint32_t>(levels.size());
    std::uint64_t offset = 0;
    for (std::uint32_t i = 0; i < result.mips_; ++i) {
        const auto &level = levels[i];
        result.levels_.push_back(
            {0, i, level.width, level.height, offset, level.rgba.size(), std::uint64_t{level.width} * 4});
        offset += level.rgba.size();
    }
    context.state_->current();
    {
        PixelTransfer transfer(gl, false);
        gl.CreateTextures(GL_TEXTURE_2D, 1, &result.id_);
        gl.TextureStorage2D(result.id_, result.mips_, result.plan_.internal_format, levels[0].width, levels[0].height);
        gl.TextureParameteri(result.id_, GL_TEXTURE_BASE_LEVEL, 0);
        gl.TextureParameteri(result.id_, GL_TEXTURE_MAX_LEVEL, result.mips_ - 1);
        gl.TextureParameteriv(result.id_, GL_TEXTURE_SWIZZLE_RGBA, result.plan_.swizzle.data());
        for (std::uint32_t i = 0; i < result.mips_; ++i) {
            const auto &level = levels[i];
            gl.TextureSubImage2D(result.id_, i, 0, 0, level.width, level.height, GL_RGBA, GL_UNSIGNED_BYTE,
                                 level.rgba.data());
            context.check("RGBA8 producer mip upload");
        }
    }
    context.check("RGBA8 producer upload state restoration");
    return result;
}
GlTexture GlTexture::bgra8(const GlContext &context, std::span<const Bgra8Level> levels,
                           DdsSamplingColor color) {
    if (color != DdsSamplingColor::linear && color != DdsSamplingColor::srgb)
        throw std::invalid_argument("Invalid BGRA8 sampler decode");
    if (levels.empty() || !levels[0].width || !levels[0].height)
        throw std::invalid_argument("BGRA8 producer requires nonempty mip dimensions");
    auto width = levels[0].width, height = levels[0].height;
    for (std::size_t i = 0; i < levels.size(); ++i) {
        const auto &level = levels[i];
        // Division avoids overflow for caller-controlled U32 dimensions.
        if (level.width != width || level.height != height || !level.bgra.data() ||
            level.bgra.size() % 4 != 0 ||
            std::uint64_t{width} * height != level.bgra.size() / 4 ||
            (width == 1 && height == 1 && i + 1 != levels.size()))
            throw std::invalid_argument("Invalid BGRA8 producer mip chain/payload");
        width = std::max(1u, width / 2);
        height = std::max(1u, height / 2);
    }
    context.state_->current();
    context.check("Before BGRA8 producer upload");
    const auto &gl = context.api();
    GLint limit = 0;
    gl.GetIntegerv(GL_MAX_TEXTURE_SIZE, &limit);
    if (limit <= 0 || levels[0].width > static_cast<std::uint32_t>(limit) ||
        levels[0].height > static_cast<std::uint32_t>(limit))
        throw std::invalid_argument("BGRA8 producer exceeds GPU limits");
    GlTexture result;
    result.context_ = context.state_;
    result.plan_ = {static_cast<GLenum>(color == DdsSamplingColor::srgb ? GL_SRGB8_ALPHA8 : GL_RGBA8),
                    GL_BGRA, GL_UNSIGNED_BYTE};
    result.faces_ = 1;
    result.mips_ = static_cast<std::uint32_t>(levels.size());
    std::uint64_t offset = 0;
    for (std::uint32_t i = 0; i < result.mips_; ++i) {
        const auto &level = levels[i];
        if (level.bgra.size() > std::numeric_limits<std::uint64_t>::max() - offset)
            throw std::invalid_argument("BGRA8 producer total payload extent overflow");
        result.levels_.push_back(
            {0, i, level.width, level.height, offset, level.bgra.size(), std::uint64_t{level.width} * 4});
        offset += level.bgra.size();
    }
    context.state_->current();
    {
        PixelTransfer transfer(gl, false);
        gl.CreateTextures(GL_TEXTURE_2D, 1, &result.id_);
        gl.TextureStorage2D(result.id_, result.mips_, result.plan_.internal_format, levels[0].width, levels[0].height);
        gl.TextureParameteri(result.id_, GL_TEXTURE_BASE_LEVEL, 0);
        gl.TextureParameteri(result.id_, GL_TEXTURE_MAX_LEVEL, result.mips_ - 1);
        gl.TextureParameteriv(result.id_, GL_TEXTURE_SWIZZLE_RGBA, result.plan_.swizzle.data());
        for (std::uint32_t i = 0; i < result.mips_; ++i) {
            const auto &level = levels[i];
            gl.TextureSubImage2D(result.id_, i, 0, 0, level.width, level.height, GL_BGRA, GL_UNSIGNED_BYTE,
                                 level.bgra.data());
            context.check("BGRA8 producer mip upload");
        }
    }
    context.check("BGRA8 producer upload state restoration");
    return result;
}
GlTexture GlTexture::lean(const GlContext &context, const assets::LeanData &data, assets::LeanMap map) {
    const auto &levels = data.levels(map); // validates the map before GPU work
    if (levels.empty())
        throw std::invalid_argument("LEAN upload requires an owned mip chain");
    context.state_->current();
    context.check("Before LEAN upload");
    const auto &gl = context.api();
    GLint limit = 0;
    gl.GetIntegerv(GL_MAX_TEXTURE_SIZE, &limit);
    if (limit <= 0 || data.width() > static_cast<std::uint32_t>(limit) ||
        data.height() > static_cast<std::uint32_t>(limit))
        throw std::invalid_argument("LEAN map exceeds GPU limits");
    GlTexture result;
    result.context_ = context.state_;
    result.plan_ = map == assets::LeanMap::rgba16
                       ? DdsUploadPlan{GL_RGBA16, GL_RGBA, GL_UNSIGNED_SHORT}
                       : DdsUploadPlan{GL_R16, GL_RED, GL_UNSIGNED_SHORT, {GL_RED, GL_RED, GL_RED, GL_ONE}};
    result.faces_ = 1;
    result.mips_ = data.mip_count();
    for (const auto &level : levels)
        result.levels_.push_back(
            {0, level.mip, level.width, level.height, level.offset, level.length, level.row_pitch});
    context.state_->current();
    {
        PixelTransfer transfer(gl, false);
        gl.CreateTextures(GL_TEXTURE_2D, 1, &result.id_);
        gl.TextureStorage2D(result.id_, result.mips_, result.plan_.internal_format, data.width(),
                            data.height());
        gl.TextureParameteri(result.id_, GL_TEXTURE_BASE_LEVEL, 0);
        gl.TextureParameteri(result.id_, GL_TEXTURE_MAX_LEVEL, result.mips_ - 1);
        gl.TextureParameteriv(result.id_, GL_TEXTURE_SWIZZLE_RGBA, result.plan_.swizzle.data());
        for (const auto &level : levels) {
            const auto bytes = data.payload(map, level.mip);
            gl.TextureSubImage2D(result.id_, level.mip, 0, 0, level.width, level.height,
                                 result.plan_.transfer_format, result.plan_.transfer_type, bytes.data());
            context.check("Authored LEAN mip upload");
        }
    }
    context.check("LEAN transfer state restoration");
    return result;
}
void GlTexture::release() noexcept {
    if (id_) {
        try {
            context_->current();
            context_->gl.DeleteTextures(1, &id_);
        } catch (...) {
            std::terminate();
        }
        id_ = 0;
    }
}
bool GlTexture::belongs_to(const GlContext &context) const { return id_ != 0 && context_ == context.state_; }
GlTexture GlTexture::rgba8_cube(const GlContext &context, std::span<const assets::Rgba8CubeLevel> levels) {
    if (levels.empty() || !levels[0].size)
        throw std::invalid_argument("RGBA8 cube requires nonempty mip dimensions");
    auto size = levels[0].size;
    for (std::size_t mip = 0; mip < levels.size(); ++mip) {
        const auto &level = levels[mip];
        if (level.size != size || (size == 1 && mip + 1 != levels.size()))
            throw std::invalid_argument("Invalid RGBA8 cube mip chain");
        for (const auto &face : level.faces)
            if (face.size() % 4 != 0 || std::uint64_t{size} * size != face.size() / 4)
                throw std::invalid_argument("Invalid RGBA8 cube face payload");
        size = std::max(1u, size / 2);
    }
    context.state_->current();
    context.check("Before RGBA8 cube producer upload");
    const auto &gl = context.api();
    GLint limit = 0;
    gl.GetIntegerv(GL_MAX_CUBE_MAP_TEXTURE_SIZE, &limit);
    if (levels[0].size > static_cast<std::uint32_t>(limit))
        throw std::invalid_argument("RGBA8 cube producer exceeds GPU limits");
    GlTexture result;
    result.context_ = context.state_;
    result.plan_ = {GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE};
    result.faces_ = 6;
    result.mips_ = static_cast<std::uint32_t>(levels.size());
    std::uint64_t offset = 0;
    for (std::uint32_t face = 0; face < 6; ++face)
        for (std::uint32_t mip = 0; mip < result.mips_; ++mip) {
            const auto &level = levels[mip];
            const auto length = level.faces[face].size();
            result.levels_.push_back(
                {face, mip, level.size, level.size, offset, length, std::uint64_t{level.size} * 4});
            offset += length;
        }
    context.state_->current();
    {
        PixelTransfer transfer(gl, false);
        gl.CreateTextures(GL_TEXTURE_CUBE_MAP, 1, &result.id_);
        gl.TextureStorage2D(result.id_, result.mips_, GL_RGBA8, levels[0].size, levels[0].size);
        gl.TextureParameteri(result.id_, GL_TEXTURE_BASE_LEVEL, 0);
        gl.TextureParameteri(result.id_, GL_TEXTURE_MAX_LEVEL, result.mips_ - 1);
        gl.TextureParameteriv(result.id_, GL_TEXTURE_SWIZZLE_RGBA, result.plan_.swizzle.data());
        for (std::uint32_t mip = 0; mip < result.mips_; ++mip)
            for (std::uint32_t face = 0; face < 6; ++face) {
                const auto &level = levels[mip];
                gl.TextureSubImage3D(result.id_, mip, 0, 0, face, level.size, level.size, 1, GL_RGBA,
                                     GL_UNSIGNED_BYTE, level.faces[face].data());
                context.check("RGBA8 producer cube face/mip upload");
            }
    }
    context.check("RGBA8 cube producer transfer state restoration");
    return result;
}
GlTexture::~GlTexture() { release(); }
GlTexture GlTexture::bgra8_volume(const GlContext &context, std::uint32_t width, std::uint32_t height,
                                  std::uint32_t depth, std::span<const std::uint8_t> bytes) {
    if (!width || !height || !depth || bytes.size() % 4 != 0 || bytes.size() / 4 % depth != 0 ||
        std::uint64_t(width) * height != bytes.size() / 4 / depth)
        throw std::invalid_argument("BGRA8 volume extent differs from payload");
    context.state_->current();
    context.check("Before BGRA8 volume upload");
    const auto &gl = context.api();
    GLint maximum = 0;
    gl.GetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &maximum);
    if (maximum <= 0 || width > static_cast<unsigned>(maximum) || height > static_cast<unsigned>(maximum) ||
        depth > static_cast<unsigned>(maximum))
        throw std::invalid_argument("BGRA8 volume exceeds GPU limits");
    GlTexture result;
    result.context_ = context.state_;
    result.plan_ = {GL_RGBA8, GL_BGRA, GL_UNSIGNED_BYTE};
    result.faces_ = result.mips_ = 1;
    result.depth_ = depth;
    result.levels_.push_back({0, 0, width, height, 0, bytes.size(), std::uint64_t(width) * 4});
    context.state_->current();
    {
        PixelTransfer transfer(gl, false);
        gl.CreateTextures(GL_TEXTURE_3D, 1, &result.id_);
        gl.TextureStorage3D(result.id_, 1, GL_RGBA8, width, height, depth);
        gl.TextureParameteri(result.id_, GL_TEXTURE_BASE_LEVEL, 0);
        gl.TextureParameteri(result.id_, GL_TEXTURE_MAX_LEVEL, 0);
        gl.TextureParameteriv(result.id_, GL_TEXTURE_SWIZZLE_RGBA, result.plan_.swizzle.data());
        gl.TextureSubImage3D(result.id_, 0, 0, 0, 0, width, height, depth, GL_BGRA, GL_UNSIGNED_BYTE,
                             bytes.data());
        context.check("Original BGRA8 volume upload");
    }
    context.check("BGRA8 volume transfer state restoration");
    return result;
}
GlTexture::GlTexture(GlTexture &&other) noexcept
    : context_(std::move(other.context_)), id_(std::exchange(other.id_, 0)), plan_(other.plan_),
      levels_(std::move(other.levels_)), faces_(other.faces_), mips_(other.mips_), depth_(other.depth_) {}
GlTexture &GlTexture::operator=(GlTexture &&other) noexcept {
    if (this != &other) {
        release();
        context_ = std::move(other.context_);
        id_ = std::exchange(other.id_, 0);
        plan_ = other.plan_;
        levels_ = std::move(other.levels_);
        faces_ = other.faces_;
        mips_ = other.mips_;
        depth_ = other.depth_;
    }
    return *this;
}
assets::Bytes GlTexture::read_level_raw(std::uint32_t face, std::uint32_t mip) const {
    if (!id_ || face >= faces_ || mip >= mips_)
        throw std::out_of_range("Invalid texture or face/mip");
    auto result = read_mip_raw(mip);
    const auto size = static_cast<std::size_t>(levels_[mip].length);
    if (faces_ == 1)
        return result;
    return {result.begin() + face * size, result.begin() + (face + 1) * size};
}
assets::Bytes GlTexture::read_mip_raw(std::uint32_t mip) const {
    if (!id_ || mip >= mips_)
        throw std::out_of_range("Invalid texture or mip");
    context_->current();
    const auto &gl = context_->gl;
    const auto size = levels_[mip].length * faces_;
    if (size > static_cast<std::uint64_t>(std::numeric_limits<GLsizei>::max()))
        throw std::out_of_range("Whole GPU mip exceeds bounded readback size");
    assets::Bytes result(static_cast<std::size_t>(size));
    context_->current();
    {
        PixelTransfer transfer(gl, true);
        if (plan_.compressed) {
            // Full-face core API. Explicit size preflight bounds this older
            // command; DSA partial/whole-cube controls are retained in tests.
            TextureBinding binding(gl, id_, faces_ == 6);
            for (std::uint32_t face = 0; face < faces_; ++face) {
                const auto target = faces_ == 6 ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + face : GL_TEXTURE_2D;
                GLint actual_size = 0;
                gl.GetTexLevelParameteriv(target, mip, GL_TEXTURE_COMPRESSED_IMAGE_SIZE, &actual_size);
                context_->check("Compressed face size preflight");
                if (actual_size < 0 || static_cast<std::uint64_t>(actual_size) != levels_[mip].length)
                    throw std::runtime_error("GPU compressed face size differs from bounded source level");
                gl.GetCompressedTexImage(target, mip, result.data() + face * levels_[mip].length);
                context_->check("DDS compressed face readback");
            }
        } else if (faces_ == 1 && depth_ == 1 && mips_ > 1 &&
                   (plan_.internal_format == GL_RGBA8 || plan_.internal_format == GL_SRGB8_ALPHA8)) {
            // Intel 32.0.101.7082 changes fractional-LOD filtering when an
            // RGBA8 mip chain is read before its first sample (also with
            // legacy/SubImage reads). Single-level textures/volumes do not
            // interpolate mips and need no staging.
            // Read a one-level GPU copy to leave the sampled source untouched.
            // The staging allocation lives only for this mip's readback.
            GLuint staging = 0;
            gl.CreateTextures(GL_TEXTURE_2D, 1, &staging);
            try {
                const auto &level = levels_[mip];
                gl.TextureStorage2D(staging, 1, plan_.internal_format, level.width, level.height);
                const auto copy = reinterpret_cast<PFNGLCOPYIMAGESUBDATAPROC>(
                    context_->address("glCopyImageSubData"));
                copy(id_, GL_TEXTURE_2D, mip, 0, 0, 0, staging, GL_TEXTURE_2D, 0, 0, 0, 0,
                     level.width, level.height, 1);
                context_->check("RGBA8 readback staging copy");
                gl.GetTextureImage(staging, 0, plan_.transfer_format, plan_.transfer_type,
                                   static_cast<GLsizei>(result.size()), result.data());
                context_->check("RGBA8 staged readback");
            } catch (...) {
                gl.DeleteTextures(1, &staging);
                throw;
            }
            gl.DeleteTextures(1, &staging);
        } else
            gl.GetTextureImage(id_, mip, plan_.transfer_format, plan_.transfer_type,
                               static_cast<GLsizei>(result.size()), result.data());
        context_->check("DDS raw GPU readback");
    }
    context_->check("DDS readback pixel-store restoration");
    return result;
}
void GlTexture::bind(GLuint unit) const {
    if (!id_)
        throw std::invalid_argument("Moved-from texture");
    context_->current();
    context_->gl.BindTextureUnit(unit, id_);
    context_->check("Bind DDS texture unit");
}
} // namespace civ5::graphics
