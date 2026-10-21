#include "civ5/materials/material_cache_acquisition.hpp"

namespace civ5::materials {
bool material_cache_acquire_base(MaterialCacheTextureState &patch, MaterialCacheTexturePool &pool,
                                 std::uint32_t base_size) {
    if (!patch.specular) {
        if (patch.final[0] == 0)
            patch.final[0] = pool.take(base_size, patch.final_size);
        return patch.final[0] != 0;
    }
    if (patch.final[0] != 0 && patch.final[1] != 0)
        return true;
    patch.final[1] = pool.take(base_size, patch.final_size);
    patch.final[0] = pool.take(base_size, patch.final_size);
    if (patch.final[0] != 0 && patch.final[1] != 0)
        return true;
    pool.give(patch.final[0]);
    pool.give(patch.final[1]);
    patch.final = {};
    return false;
}
bool material_cache_acquire_higher(MaterialCacheTextureState &patch, MaterialCacheTexturePool &pool,
                                   std::uint32_t base_size) {
    const auto entering_final_size = patch.final_size;
    const auto final_specular = patch.specular ? pool.take(base_size, patch.final_size) : 0;
    const auto final_diffuse = pool.take(base_size, patch.final_size);
    std::uint32_t higher_size = 0, specular_size = 0;
    const auto higher_diffuse = pool.take(patch.desired_higher_size, higher_size);
    const auto higher_specular = patch.specular ? pool.take(patch.desired_higher_size, specular_size) : 0;
    if (final_diffuse != 0 && higher_diffuse != 0 &&
        (!patch.specular || (final_specular != 0 && higher_specular != 0 && higher_size == specular_size))) {
        patch.higher_size = higher_size;
        pool.give(patch.higher[0]);
        pool.give(patch.higher[1]);
        pool.give(patch.final[0]);
        pool.give(patch.final[1]);
        patch.final = {final_diffuse, final_specular};
        patch.higher = {higher_diffuse, higher_specular};
        patch.final_size = entering_final_size;
        return true;
    }
    pool.give(higher_diffuse);
    pool.give(higher_specular);
    pool.give(final_diffuse);
    pool.give(final_specular);
    return false;
}
} // namespace civ5::materials
