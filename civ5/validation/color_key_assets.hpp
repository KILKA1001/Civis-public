#pragma once
#include "civ5/graphics/gl_color_keys.hpp"

namespace color_key_validation {
inline civ5::materials::ColorKeyAssets load(const std::filesystem::path &root) {
    using namespace civ5::assets;
    const LooseSource source(root);
    return civ5::materials::ColorKeyAssets::load(
        [&](const civ5::materials::ColorKeyRequest &request) -> std::optional<AssetHandle> {
            const auto found = source.find(AssetKey(request.native_path));
            if (std::holds_alternative<std::monostate>(found))
                return {};
            if (!std::holds_alternative<AssetHandle>(found))
                throw std::invalid_argument("Ambiguous ColorKey input");
            return std::get<AssetHandle>(found);
        });
}
} // namespace color_key_validation
