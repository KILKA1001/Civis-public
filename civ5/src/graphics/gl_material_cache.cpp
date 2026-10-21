#include "civ5/graphics/gl_material_cache.hpp"
#include <algorithm>
#include <set>
#include <stdexcept>

namespace civ5::graphics {
namespace {
std::pair<RenderTextureFormat, bool> format_and_mips(materials::MaterialCacheFormat format,
                                                     MaterialCacheMipPolicy policy) {
    using F = materials::MaterialCacheFormat;
    switch (format) {
    case F::rgba16_float:
        return {RenderTextureFormat::rgba16_float, policy.rgba16_float};
    case F::rgba8:
        return {RenderTextureFormat::rgba8, policy.rgba8};
    case F::rgb565:
        return {RenderTextureFormat::rgb565, policy.rgb565};
    default:
        throw std::invalid_argument("Material cache compression has no proven OpenGL render mapping");
    }
}
} // namespace
GlMaterialCacheTextures::GlMaterialCacheTextures(std::shared_ptr<GlContext> context,
                                                 std::span<const materials::MaterialCacheTexturePlan> plans,
                                                 MaterialCacheMipPolicy mip_policy)
    : context_(std::move(context)), plans_(plans.begin(), plans.end()), mip_policy_(mip_policy) {
    if (!context_)
        throw std::invalid_argument("Material cache requires an owned context");
    std::set<std::pair<materials::MaterialCacheRole, std::uint32_t>> identities;
    for (const auto &plan : plans_) {
        format_and_mips(plan.format, mip_policy);
        if (!identities.emplace(plan.role, plan.index).second)
            throw std::invalid_argument("Duplicate material cache resource identity");
        if (!plan.width || !plan.height || plan.width > 16384 || plan.height > 16384 ||
            plan.native_pool != 0 || plan.native_usage != (plan.requested_levels ? 1u : 0x401u))
            throw std::invalid_argument("Invalid original material render resource plan");
        std::uint32_t maximum = 1;
        for (auto n = std::max(plan.width, plan.height); n > 1; n /= 2)
            ++maximum;
        if (plan.requested_levels > maximum)
            throw std::invalid_argument("Material cache plan has excess mip levels");
    }
    textures_.reserve(plans_.size());
    for (const auto &plan : plans_) {
        const auto [format, autogen] = format_and_mips(plan.format, mip_policy);
        auto levels = plan.requested_levels;
        if (!levels) {
            levels = 1;
            if (autogen)
                for (auto n = std::max(plan.width, plan.height); n > 1; n /= 2)
                    ++levels;
        }
        textures_.push_back(GlTexture::render_target(*context_, plan.width, plan.height, levels, format));
    }
}
void GlMaterialCacheTextures::rebuild_context(std::shared_ptr<GlContext> context) {
    if (!context)
        throw std::invalid_argument("Material cache replacement requires an owned context");
    if (!context_)
        throw std::invalid_argument("Moved-from material cache texture owner");
    if (context == context_)
        throw std::invalid_argument("Material cache replacement requires a different context");
    GlMaterialCacheTextures replacement(context, plans_, mip_policy_);
    textures_.swap(replacement.textures_);
    context_.swap(replacement.context_);
}
std::size_t GlMaterialCacheTextures::locate(materials::MaterialCacheRole role, std::uint32_t index) const {
    if (!context_ || plans_.size() != textures_.size())
        throw std::invalid_argument("Moved-from material cache texture owner");
    for (std::size_t i = 0; i < plans_.size(); ++i)
        if (plans_[i].role == role && plans_[i].index == index)
            return i;
    throw std::out_of_range("Material cache resource was not allocated");
}
const GlTexture &GlMaterialCacheTextures::texture(materials::MaterialCacheRole role,
                                                  std::uint32_t index) const {
    return textures_[locate(role, index)];
}
void GlMaterialCacheTextures::generate_mips(materials::MaterialCacheRole role, std::uint32_t index) {
    textures_[locate(role, index)].generate_mip_chain();
}
} // namespace civ5::graphics
