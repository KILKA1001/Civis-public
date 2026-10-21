#pragma once
#include "gl_texture.hpp"
#include "civ5/materials/water_current.hpp"

namespace civ5::graphics {
// One generated current map with its own original resource0, target and raster
// state. Construction completes the draw; no reference pixels or shader files
// are accepted. The caller selects this resource only when water is enabled.
class GlWaterCurrentMap {
  public:
    GlWaterCurrentMap(std::shared_ptr<GlContext> context, std::int32_t worker_width,
                      std::int32_t worker_height, materials::WaterCurrentSettings settings = {},
                      bool conditional_clear = false);
    ~GlWaterCurrentMap();
    GlWaterCurrentMap(const GlWaterCurrentMap &) = delete;
    GlWaterCurrentMap &operator=(const GlWaterCurrentMap &) = delete;
    const GlTexture &texture() const;
    const GlTexture &source_texture() const;
    const materials::WaterCurrentConstants &constants() const;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace civ5::graphics
