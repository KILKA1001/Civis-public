#include "civ5/materials/material_cache_free_lists.hpp"
#include "material_cache_acquisition_validation.hpp"

namespace {
using namespace cache_acquisition_validation;
struct Pools final : civ5::materials::MaterialCacheTexturePool {
    civ5::materials::MaterialCacheFreeLists state;
    std::vector<std::array<std::uint32_t, 3>> takes;
    std::vector<std::uint32_t> returns;
    explicit Pools(const PoolInput &input) {
        for (auto [handle, width] : input.sizes)
            state.add_texture(handle, width, false);
        for (const auto &list : input.free)
            for (auto handle : list)
                state.give(handle);
    }
    std::uint32_t take(std::uint32_t request, std::uint32_t &width) override {
        const auto handle = state.take(request, width);
        takes.push_back({request, handle, handle ? width : 0});
        return handle;
    }
    void give(std::uint32_t handle) override {
        returns.push_back(handle);
        state.give(handle);
    }
    std::span<const std::uint32_t> free_handles(std::uint32_t width) const {
        return state.free_handles(width);
    }
    void validate() const {}
    void finish() const {}
};
} // namespace
int main(int argc, char **argv) {
    return cache_acquisition_validation::run(argc, argv, [](const auto &input) { return Pools(input); });
}
