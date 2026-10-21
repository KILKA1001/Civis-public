#include "civ5/graphics/gl_color_keys.hpp"
#include <stdexcept>

namespace civ5::graphics {
GlColorKeys::GlColorKeys(const GlContext &context, const materials::ColorKeyAssets &assets)
    : assets_(std::make_shared<const materials::ColorKeyAssets>(assets)) {
    if (assets.volumes().size() != 25)
        throw std::invalid_argument("Complete ColorKey asset owner required");
    volumes_.reserve(25);
    for (const auto &asset : assets.volumes())
        volumes_.push_back(GlTexture::bgra8_volume(context, 32, 32, 32, asset.volume.bgra()));
}
void GlColorKeys::rebuild_context(std::shared_ptr<GlContext> context) {
    if (!context)
        throw std::invalid_argument("ColorKey replacement requires an owned context");
    if (!assets_ || volumes_.empty())
        throw std::invalid_argument("Moved-from ColorKey owner");
    if (volumes_.front().belongs_to(*context))
        throw std::invalid_argument("ColorKey replacement requires a different context");
    GlColorKeys replacement(*context, *assets_);
    volumes_.swap(replacement.volumes_);
}
const GlTexture &GlColorKeys::at(materials::ColorKeySystem system, unsigned style) const {
    const auto index = static_cast<unsigned>(system);
    if (index >= 5 || style >= 5)
        throw std::out_of_range("ColorKey system and style must be 0..4");
    return volumes_.at(index * 5 + style);
}
std::array<GLuint, 5> GlColorKeys::color_correct() const {
    std::array<GLuint, 5> result;
    for (unsigned style = 0; style < 5; ++style)
        result[style] = at(materials::ColorKeySystem::terrain, style).id();
    return result;
}
bool GlColorKeys::belongs_to(const GlContext &context) const {
    return !volumes_.empty() && volumes_[0].belongs_to(context);
}
} // namespace civ5::graphics
