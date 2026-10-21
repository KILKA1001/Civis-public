#pragma once
#include "height_addressing.hpp"
#include "land_styles.hpp"
#include "world_layout.hpp"

namespace civ5::terrain {
// Callback supplies explicit initial allocator bytes. It is not an assumed
// zero allocator and its contents are not retained by reference.
using PageAllocator = std::function<std::vector<std::uint8_t>(std::int32_t)>;
class PageStorage {
  public:
    static PageStorage initialize(std::int32_t width, std::int32_t height, std::int32_t element_bytes,
                                  const PageStorage *previous, const PageAllocator &allocate);
    std::int32_t element_bytes() const { return element_bytes_; }
    const HeightDomain &domain() const { return domain_; }
    std::int32_t page_columns() const { return domain_.width() / 16; }
    std::int32_t page_rows() const { return domain_.height() / 16; }
    std::int32_t active_pages() const { return active_; }
    std::int32_t capacity_pages() const { return capacity_; }
    std::vector<std::uint8_t> header_bytes() const { return headers_; }
    std::vector<std::uint8_t> pool_bytes() const { return pool_; }
    PageStorage with_pool_bytes(std::span<const std::uint8_t> bytes) const;
    // Populate already allocated native pages from a computed row-major field.
    // Header bytes, spare capacity and inactive pool bytes are preserved.
    // No allocation/state-1 expansion is invented by these writes.
    PageStorage with_dense_values(std::span<const std::uint16_t> values) const;
    PageStorage with_dense_values(std::span<const std::uint8_t> values) const;
    std::uint16_t value_at(std::int32_t x, std::int32_t y) const;
    HeightField height_field() const;

  private:
    template <class T> PageStorage with_dense_values_impl(std::span<const T> values) const;
    PageStorage(std::int32_t element_bytes, HeightDomain domain, std::int32_t active, std::int32_t capacity,
                std::vector<std::uint8_t> headers, std::vector<std::uint8_t> pool);
    std::int32_t element_bytes_;
    HeightDomain domain_;
    std::int32_t active_, capacity_;
    std::vector<std::uint8_t> headers_, pool_;
};
struct HeightOwnerParameters {
    std::int32_t width, height;
    float world_span_x, world_span_y, center_x, center_y, center_z, scale_x, scale_y, scale_z;
    bool enabled;
    float factor;
    static HeightOwnerParameters native_call(std::int32_t width, std::int32_t height);
    static HeightOwnerParameters from_world(const WorldLayout &world);
};
struct HeightOwnerState {
    HeightOwnerParameters parameters;
    PageStorage height, material;
    LandStyleGrid styles;
    IntMatrix4 first_matrix, second_matrix;
    FactoryDescriptor coordinate_descriptor;
    float normalized_scale_x() const;
    float normalized_scale_y() const;
};
float post_initialization_scalar(float source_value, float height_scale);
HeightOwnerState initialize_height_owner(const HeightOwnerParameters &parameters, std::int32_t source_width,
                                         std::int32_t source_height, std::span<const std::uint8_t> cells,
                                         const std::array<std::int32_t, 3> &translation,
                                         const LandStyleGrid &prior_styles,
                                         const PageStorage *previous_height,
                                         const PageStorage *previous_material, const PageAllocator &allocate);
} // namespace civ5::terrain
