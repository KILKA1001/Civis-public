#include "civ5/terrain/height_owner.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <utility>

namespace civ5::terrain {
namespace {
std::uint16_t u16(const std::vector<std::uint8_t> &b, std::size_t o) {
    return static_cast<std::uint16_t>(b[o] | (std::uint16_t{b[o + 1]} << 8));
}
std::uint32_t u32(const std::vector<std::uint8_t> &b, std::size_t o) {
    std::uint32_t result = 0;
    for (unsigned i = 0; i < 4; ++i)
        result |= std::uint32_t{b[o + i]} << (8 * i);
    return result;
}
void put(std::vector<std::uint8_t> &b, std::size_t o, std::uint32_t value, unsigned count) {
    for (unsigned i = 0; i < count; ++i)
        b[o + i] = static_cast<std::uint8_t>(value >> (8 * i));
}
} // namespace
PageStorage::PageStorage(std::int32_t element_bytes, HeightDomain domain, std::int32_t active,
                         std::int32_t capacity, std::vector<std::uint8_t> headers,
                         std::vector<std::uint8_t> pool)
    : element_bytes_(element_bytes), domain_(domain), active_(active), capacity_(capacity),
      headers_(std::move(headers)), pool_(std::move(pool)) {}
PageStorage PageStorage::initialize(std::int32_t width, std::int32_t height, std::int32_t element_bytes,
                                    const PageStorage *previous, const PageAllocator &allocate) {
    constexpr auto max = std::numeric_limits<std::int32_t>::max();
    if (width <= 0 || height <= 0 || width > max - 15 || height > max - 15 ||
        (element_bytes != 1 && element_bytes != 2) || (previous && previous->element_bytes_ != element_bytes))
        throw std::invalid_argument("Invalid native page initialization");
    HeightDomain domain((width + 15) & -16, (height + 15) & -16);
    const auto columns = domain.width() / 16, rows = domain.height() / 16;
    const auto clear_columns = width / 16, clear_rows = height / 16;
    const auto active = clear_columns * clear_rows;
    const auto previous_capacity = previous ? previous->capacity_ : 0;
    const auto capacity = std::max(previous_capacity, active);
    if (std::int64_t{capacity} * 256 * element_bytes > max)
        throw std::invalid_argument("Unrepresentable native pool");
    auto allocated = [&](std::int32_t size) {
        auto result = allocate(size);
        if (result.size() != static_cast<std::size_t>(size))
            throw std::invalid_argument("Wrong allocator byte count");
        return result;
    };
    // Reservation follows vector count reset. Growth copies no old pool bytes;
    // otherwise even inactive capacity survives. Pool allocation precedes headers.
    auto pool = capacity > previous_capacity ? allocated(capacity * 256 * element_bytes)
                : previous                   ? previous->pool_
                                             : std::vector<std::uint8_t>{};
    auto headers = allocated(columns * rows * 8);
    for (std::int32_t page = 0; page < columns * rows; ++page) {
        put(headers, page * 8, 1, 2);
        put(headers, page * 8 + 4, 0, element_bytes);
    }
    for (std::int32_t y = 0; y < clear_rows; ++y)
        for (std::int32_t x = 0; x < clear_columns; ++x) {
            const auto header = (y * columns + x) * 8;
            put(headers, header, 0, 2);
            put(headers, header + 4, y * clear_columns + x, 4);
        }
    std::fill_n(pool.begin(), active * 256 * element_bytes, std::uint8_t{0});
    return {element_bytes, domain, active, capacity, std::move(headers), std::move(pool)};
}
PageStorage PageStorage::with_pool_bytes(std::span<const std::uint8_t> bytes) const {
    if (bytes.size() != pool_.size())
        throw std::invalid_argument("Wrong prior pool size");
    return {element_bytes_, domain_, active_, capacity_, headers_, {bytes.begin(), bytes.end()}};
}
template <class T> PageStorage PageStorage::with_dense_values_impl(std::span<const T> values) const {
    if (element_bytes_ != sizeof(T) || values.size() != std::size_t(domain_.width()) * domain_.height())
        throw std::invalid_argument("Dense field does not match native page element size/domain");
    for (std::size_t at = 0; at < headers_.size(); at += 8)
        if (u16(headers_, at) != 0 || u32(headers_, at + 4) >= static_cast<std::uint32_t>(active_))
            throw std::invalid_argument("Dense field requires fully allocated native pages");
    auto result = *this;
    for (std::int32_t y = 0; y < domain_.height(); ++y)
        for (std::int32_t x = 0; x < domain_.width(); ++x) {
            const auto header = ((y / 16) * page_columns() + x / 16) * 8;
            const auto page = u32(headers_, header + 4);
            const auto offset = (std::size_t(page) * 256 + (y & 15) * 16 + (x & 15)) * element_bytes_;
            put(result.pool_, offset, values[std::size_t(y) * domain_.width() + x], element_bytes_);
        }
    return result;
}
PageStorage PageStorage::with_dense_values(std::span<const std::uint16_t> values) const {
    return with_dense_values_impl(values);
}
PageStorage PageStorage::with_dense_values(std::span<const std::uint8_t> values) const {
    return with_dense_values_impl(values);
}
std::uint16_t PageStorage::value_at(std::int32_t x, std::int32_t y) const {
    if (x < 0 || y < 0 || x >= domain_.width() || y >= domain_.height())
        throw std::out_of_range("Native page coordinate");
    const auto header = ((y / 16) * page_columns() + x / 16) * 8;
    const auto state = u16(headers_, header);
    const auto *source = &headers_;
    std::size_t offset = header + 4;
    if (state != 1) {
        if (state != 0)
            throw std::logic_error("Unsupported native page state");
        const auto page = u32(headers_, header + 4);
        if (page >= static_cast<std::uint32_t>(active_))
            throw std::logic_error("Native page outside active pool");
        source = &pool_;
        offset = (page * 256 + (y & 15) * 16 + (x & 15)) * element_bytes_;
    }
    return element_bytes_ == 2 ? u16(*source, offset) : (*source)[offset];
}
HeightField PageStorage::height_field() const {
    if (element_bytes_ != 2)
        throw std::invalid_argument("Height field requires full U16 pages");
    std::vector<std::uint16_t> values(static_cast<std::size_t>(domain_.width()) * domain_.height());
    for (std::int32_t y = 0; y < domain_.height(); ++y)
        for (std::int32_t x = 0; x < domain_.width(); ++x)
            values[static_cast<std::size_t>(y) * domain_.width() + x] = value_at(x, y);
    return HeightField(domain_, values);
}
HeightOwnerParameters HeightOwnerParameters::native_call(std::int32_t width, std::int32_t height) {
    if (width <= 0 || height <= 0)
        throw std::invalid_argument("Invalid height owner extent");
    return {width,
            height,
            static_cast<float>(width),
            static_cast<float>(height) * .866f,
            static_cast<float>(width / 2),
            static_cast<float>(height / 2) * .866f,
            0,
            1,
            .866f,
            .2f,
            true,
            1};
}
HeightOwnerParameters HeightOwnerParameters::from_world(const WorldLayout &world) {
    // WorldGrid is public, so guard the multiplication for non-builder callers.
    if (world.worker_grid.width <= 0 || world.worker_grid.height <= 0 ||
        world.worker_grid.width > std::numeric_limits<std::int32_t>::max() / 64 ||
        world.worker_grid.height > std::numeric_limits<std::int32_t>::max() / 64)
        throw std::invalid_argument("Unrepresentable height owner world extent");
    return native_call(world.worker_grid.width * 64, world.worker_grid.height * 64);
}
float HeightOwnerState::normalized_scale_x() const {
    return static_cast<float>(height.domain().width() - 1) / static_cast<float>(parameters.width);
}
float HeightOwnerState::normalized_scale_y() const {
    return static_cast<float>(height.domain().height() - 1) / static_cast<float>(parameters.height);
}
float post_initialization_scalar(float source_value, float height_scale) {
    return (source_value * height_scale) * 255.0f;
}
HeightOwnerState initialize_height_owner(const HeightOwnerParameters &parameters, std::int32_t source_width,
                                         std::int32_t source_height, std::span<const std::uint8_t> cells,
                                         const std::array<std::int32_t, 3> &translation,
                                         const LandStyleGrid &prior_styles,
                                         const PageStorage *previous_height,
                                         const PageStorage *previous_material,
                                         const PageAllocator &allocate) {
    auto first = unshifted_global_map_matrix({});
    auto second = first;
    for (std::size_t i = 0; i < 3; ++i) {
        first[12 + i] = wrap_neg(translation[i]);
        second[12 + i] = translation[i];
    }
    auto styles = prior_styles.copy_source_cells(source_width, source_height, cells);
    auto height = PageStorage::initialize(parameters.width, parameters.height, 2, previous_height, allocate);
    auto material =
        PageStorage::initialize(parameters.width, parameters.height, 1, previous_material, allocate);
    return {parameters,
            std::move(height),
            std::move(material),
            std::move(styles),
            first,
            second,
            {translation[0], translation[1], translation[2], 0, 0}};
}
} // namespace civ5::terrain
