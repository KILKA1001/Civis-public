#include "civ5/graphics/gl_material_cache_pool.hpp"
#include <stdexcept>

namespace civ5::graphics {
namespace {
std::vector<materials::MaterialCacheTexturePlan> pool_plans(std::span<const std::uint32_t, 4> counts) {
    return materials::material_cache_pool_plan(
        std::array<std::uint32_t, 5>{counts[0], counts[1], counts[2], counts[3], 0}, false, 64);
}
} // namespace
GlMaterialCachePool::GlMaterialCachePool(std::shared_ptr<GlContext> context,
                                         std::span<const std::uint32_t, 4> counts, bool automatic_mips)
    : context_(context), textures_(std::move(context), pool_plans(counts), {false, false, automatic_mips}) {
    handles_.reserve(textures_.plans().size());
    for (const auto &plan : textures_.plans()) {
        handles_.push_back(&textures_.texture(plan.role, plan.index));
        state_.add_texture(static_cast<std::uint32_t>(handles_.size()), plan.width, true);
    }
}
void GlMaterialCachePool::rebuild_context(std::shared_ptr<GlContext> context) {
    if (!context)
        throw std::invalid_argument("Material pool replacement requires an owned context");
    if (!context_)
        throw std::invalid_argument("Moved-from material pool");
    if (context == context_)
        throw std::invalid_argument("Material pool replacement requires a different context");
    for (std::uint32_t handle = 1; handle <= state_.texture_count(); ++handle)
        if (!state_.available(handle))
            throw std::invalid_argument("Material pool replacement requires no active leases");
    textures_.rebuild_context(context);
    context_ = std::move(context);
    handles_.clear();
    handles_.reserve(textures_.plans().size());
    for (const auto &plan : textures_.plans())
        handles_.push_back(&textures_.texture(plan.role, plan.index));
}
std::uint32_t GlMaterialCachePool::take(std::uint32_t requested_size, std::uint32_t &actual_size) {
    return state_.take(requested_size, actual_size);
}
void GlMaterialCachePool::give(std::uint32_t handle) { state_.give(handle); }
const GlTexture &GlMaterialCachePool::texture(std::uint32_t handle) const {
    if (!state_.contains(handle) || state_.available(handle))
        throw std::invalid_argument("Material pool texture is not leased");
    return *handles_.at(handle - 1);
}
} // namespace civ5::graphics
