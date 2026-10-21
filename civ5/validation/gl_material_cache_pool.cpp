// Storage/lifetime controls. Original shader output is verified separately;
// these diagnostic mip patterns are never used as production cache pixels.
#include "civ5/graphics/gl_material_cache_pool.hpp"
#include "material_cache_acquisition_validation.hpp"
#include <bit>
#include <set>

namespace {
using namespace cache_acquisition_validation;
using namespace civ5::graphics;
struct Allocation {
    unsigned label, handle;
    GLuint id;
    const GlTexture *texture;
    std::vector<Bytes> levels;
};
struct Pools final : civ5::materials::MaterialCacheTexturePool {
    std::shared_ptr<GlContext> context;
    std::unique_ptr<GlMaterialCachePool> storage;
    std::map<unsigned, unsigned> label_to_handle, handle_to_label;
    std::vector<Allocation> allocations;
    std::vector<std::array<std::uint32_t, 3>> takes;
    std::vector<std::uint32_t> returns;
    Pools(std::shared_ptr<GlContext> ctx, const PoolInput &input) : context(std::move(ctx)) {
        std::array<std::uint32_t, 4> counts{};
        for (auto [label, width] : input.sizes) {
            (void)label;
            ++counts[input.size_index(width)];
        }
        storage = std::make_unique<GlMaterialCachePool>(context, counts, true);
        std::set<GLuint> identities;
        const auto &gl = context->api();
        for (auto [label, width] : input.sizes) {
            unsigned actual = 0;
            const auto handle = storage->take(width, actual);
            require(handle && actual == width, "Fixture label requires an exact allocated width");
            label_to_handle.emplace(label, handle);
            handle_to_label.emplace(handle, label);
            const auto &texture = storage->texture(handle);
            require(texture.belongs_to(*context) && identities.insert(texture.id()).second,
                    "Duplicate or foreign physical pool texture");
            require(texture.plan().internal_format == GL_RGB565 &&
                        texture.levels().size() == std::bit_width(width),
                    "Physical pool format or effective mip count differs");
            Allocation allocation{label, handle, texture.id(), &texture, {}};
            for (const auto &level : texture.levels()) {
                GLint internal = 0, actual_width = 0, actual_height = 0;
                gl.GetTextureLevelParameteriv(texture.id(), level.mip, GL_TEXTURE_INTERNAL_FORMAT, &internal);
                gl.GetTextureLevelParameteriv(texture.id(), level.mip, GL_TEXTURE_WIDTH, &actual_width);
                gl.GetTextureLevelParameteriv(texture.id(), level.mip, GL_TEXTURE_HEIGHT, &actual_height);
                require(internal == GL_RGB565 && actual_width == int(level.width) &&
                            actual_height == int(level.height),
                        "Actual driver pool storage descriptor differs");
                const auto marker = static_cast<std::uint16_t>(label * 391u + level.mip * 23u);
                Bytes bytes(level.length);
                for (std::size_t i = 0; i < bytes.size(); i += 2) {
                    bytes[i] = static_cast<std::uint8_t>(marker);
                    bytes[i + 1] = static_cast<std::uint8_t>(marker >> 8);
                }
                gl.PixelStorei(GL_UNPACK_ALIGNMENT, 1);
                gl.TextureSubImage2D(texture.id(), level.mip, 0, 0, level.width, level.height, GL_RGB,
                                     GL_UNSIGNED_SHORT_5_6_5, bytes.data());
                allocation.levels.push_back(std::move(bytes));
            }
            allocations.push_back(std::move(allocation));
        }
        for (const auto &list : input.free)
            for (auto label : list)
                storage->give(label_to_handle.at(label));
        context->check("Pool diagnostic patterns initialized");
        validate();
    }
    std::uint32_t take(std::uint32_t request, std::uint32_t &width) override {
        const auto physical = storage->take(request, width);
        const auto label = physical ? handle_to_label.at(physical) : 0;
        takes.push_back({request, label, label ? width : 0});
        return label;
    }
    void give(std::uint32_t label) override {
        returns.push_back(label);
        storage->give(label ? label_to_handle.at(label) : 0);
    }
    std::vector<std::uint32_t> free_handles(std::uint32_t width) const {
        std::vector<std::uint32_t> result;
        for (auto handle : storage->state().free_handles(width))
            result.push_back(handle_to_label.at(handle));
        return result;
    }
    void validate() const {
        for (const auto &allocation : allocations) {
            require(context->api().IsTexture(allocation.id) && allocation.texture->id() == allocation.id,
                    "Acquisition or return replaced/deleted physical storage");
            if (storage->state().available(allocation.handle)) {
                bool rejected = false;
                try {
                    (void)storage->texture(allocation.handle);
                } catch (const std::invalid_argument &) {
                    rejected = true;
                }
                require(rejected, "Free texture remains exposed as a leased resource");
            } else
                require(&storage->texture(allocation.handle) == allocation.texture,
                        "Lease changed storage identity");
            for (unsigned mip = 0; mip < allocation.levels.size(); ++mip)
                require(allocation.texture->read_mip_raw(mip) == allocation.levels[mip],
                        "Acquisition or return changed retained mip pixels");
        }
        context->check("Pool storage and leases retained");
    }
    void finish() {
        storage.reset();
        for (const auto &allocation : allocations)
            require(!context->api().IsTexture(allocation.id), "Pool destruction retained a physical texture");
        context->check("Pool storage released");
    }
};
void context_recovery_controls() {
    auto original = std::make_shared<GlContext>();
    const std::array<std::uint32_t, 4> counts{1, 0, 0, 0};
    GlMaterialCachePool pool(original, counts, true);
    unsigned actual = 0;
    const auto handle = pool.take(64, actual);
    require(handle && actual == 64, "Pool recovery fixture acquisition");
    auto replacement = std::make_shared<GlContext>();
    bool rejected_active = false;
    try {
        pool.rebuild_context(replacement);
    } catch (const std::invalid_argument &) {
        rejected_active = true;
    }
    require(rejected_active, "Pool recovery must reject active leases");
    pool.give(handle);
    bool rejected_same = false;
    try {
        pool.rebuild_context(original);
    } catch (const std::invalid_argument &) {
        rejected_same = true;
    }
    require(rejected_same, "Pool recovery must reject the creating context");
    pool.rebuild_context(replacement);
    const auto recovered = pool.take(64, actual);
    require(recovered && actual == 64, "Pool recovery fixture reacquisition");
    require(pool.texture(recovered).belongs_to(*replacement), "Pool recovery refreshed texture owner");
    pool.give(recovered);
    replacement->check("Pool recovery replacement context");
}
} // namespace
int main(int argc, char **argv) {
    try {
        context_recovery_controls();
        const auto context = std::make_shared<GlContext>();
        std::cout << context->description() << '\n';
        return cache_acquisition_validation::run(argc, argv,
                                                 [&](const auto &input) { return Pools(context, input); });
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
