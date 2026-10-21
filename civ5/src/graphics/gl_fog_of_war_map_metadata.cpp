#include "gl_fog_of_war_map_impl.hpp"

// These reads use the same private storage mutated by render/rebuild_context.
// Keep them separate from GL operations so CPU receipt validation needs no GL link.
namespace civ5::graphics {
std::uint64_t GlFogOfWarMap::render_revision() const { return impl_->render_revision; }
const GlTexture &GlFogOfWarMap::mask() const { return impl_->mask; }
} // namespace civ5::graphics
