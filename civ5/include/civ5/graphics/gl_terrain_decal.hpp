#pragma once
#include "civ5/materials/terrain_decal_draw.hpp"
#include "gl_texture.hpp"
#include <functional>

namespace civ5::graphics {
// Resolve the original U16 resource ID with the explicit sampler decode.
// The texture and this backend must belong to the same live GL context.
using TerrainDecalTextureResolver = std::function<const GlTexture &(std::uint16_t, DdsSamplingColor)>;

// Own authored local IDs and both original sampler decodes. ID0 is the shared
// renderer's exact seven-level diagnostic texture, including dithered alpha.
class GlTerrainDecalTextures {
  public:
    GlTerrainDecalTextures(std::shared_ptr<GlContext>, std::shared_ptr<const terrain::TerrainDecalAssets>);
    const GlTexture &resource(std::uint16_t, DdsSamplingColor);
  private:
    std::shared_ptr<GlContext> context_;
    std::shared_ptr<const terrain::TerrainDecalAssets> assets_;
    std::vector<std::array<std::unique_ptr<GlTexture>, 2>> textures_;
};

// Original half XY/UV batches -> shader 0x4E, technique 0, two MRT outputs.
// Owns buffers, shader, declaration and sampler. Caller owns the store, input
// textures, complete framebuffer, viewport and pass blend/depth/cull state.
// IDs are opaque, stable backend-local handles (not GL object names).
class GlTerrainDecalRenderer final : public materials::TerrainDecalBufferBackend {
  public:
    explicit GlTerrainDecalRenderer(std::shared_ptr<GlContext>);
    ~GlTerrainDecalRenderer();
    GlTerrainDecalRenderer(const GlTerrainDecalRenderer &) = delete;
    GlTerrainDecalRenderer &operator=(const GlTerrainDecalRenderer &) = delete;
    std::uint32_t create() override;
    void upload(std::uint32_t, std::span<const std::array<std::uint16_t,4>>) override;
    std::uint32_t index_buffer() const { return 1; }
    // Resolves/validates the complete list before issuing any draw. Consume
    // immediately after prepare_terrain_decal_draws; no intervening mutation.
    // Resolver callbacks must not mutate or reenter the renderer/store.
    void draw(const materials::TerrainDecalDrawList &, const TerrainDecalTextureResolver &);
    // Read-only preflight for a future pass, independent of the currently bound
    // framebuffer. Targets are the future color attachment object names. The
    // caller must exclude storage aliases made with texture views separately.
    // Consume immediately, without changing buffers, textures or their context.
    void validate(const materials::TerrainDecalDrawList &, const TerrainDecalTextureResolver &,
                  const GlContext &, std::span<const GLuint> targets) const;
    // Retains/reuploads the last successfully uploaded prefixes and stable
    // handles. Caller recreates textures/targets. Old context must be usable.
    void rebuild_context(std::shared_ptr<GlContext>);
    // Exact uploaded prefix, useful for checking retained GPU ownership.
    std::vector<std::array<std::uint16_t,4>> read_vertices(std::uint32_t) const;
  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace civ5::graphics
