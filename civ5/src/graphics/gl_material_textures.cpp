#include "civ5/graphics/gl_material_textures.hpp"
#include <stdexcept>

namespace civ5::graphics {
GlMaterialTextures::GlMaterialTextures(std::shared_ptr<GlContext> context,
                                       std::shared_ptr<const materials::MaterialAssets> source)
    : context_(std::move(context)), source_(std::move(source)) {
    if (!context_ || !source_)
        throw std::invalid_argument("Material textures require owned context and source");
    context_->api();
    resources_.resize(source_->textures().size() + 1);
}
const GlTexture &GlMaterialTextures::resource(std::uint16_t local_id) {
    context_->api();
    if (local_id >= resources_.size())
        throw std::invalid_argument("Unresolved local material resource ID");
    auto &texture = resources_[local_id];
    if (!texture) {
        if (local_id == 0)
            texture =
                std::make_unique<GlTexture>(GlTexture::rgba8(*context_, materials::material_resource_zero()));
        else
            texture = std::make_unique<GlTexture>(*context_, source_->textures()[local_id - 1].data);
    }
    return *texture;
}
std::array<GLuint, 16> GlMaterialTextures::bind(std::span<const std::int32_t> selected) {
    const auto ids = source_->selected_resources(selected);
    std::array<GLuint, 16> names{};
    for (std::size_t slot = 0; slot < names.size(); ++slot)
        names[slot] = resource(ids[slot]).id();
    const auto &gl = context_->api();
    for (GLuint slot = 0; slot < names.size(); ++slot)
        gl.BindTextureUnit(slot, names[slot]);
    context_->check("Bind all16 native material texture slots");
    return names;
}
void GlMaterialTextures::rebuild_context(std::shared_ptr<GlContext> context) {
    if (!context)
        throw std::invalid_argument("Material textures replacement requires an owned context");
    if (!context_ || !source_)
        throw std::invalid_argument("Moved-from material texture owner");
    if (context == context_)
        throw std::invalid_argument("Material textures replacement requires a different context");
    GlMaterialTextures replacement(context, source_);
    for (std::uint16_t id = 0; id < resources_.size(); ++id)
        if (resources_[id])
            (void)replacement.resource(id);
    context_.swap(replacement.context_);
    resources_.swap(replacement.resources_);
}
} // namespace civ5::graphics
