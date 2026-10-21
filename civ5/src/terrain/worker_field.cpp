#include "civ5/terrain/worker_field.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace civ5::terrain {
namespace {
void require(bool condition, const char *message) {
    if (!condition)
        throw std::invalid_argument(message);
}
void validate_precision(X87Precision precision) {
    require(precision == X87Precision::single || precision == X87Precision::double_precision,
            "Unsupported height finalizer precision");
}
void validate_descriptor(HeightDomain domain, const TerrainDescriptor &d) {
    require(bool(d.grids), "Missing terrain grids");
    const auto &r = d.rectangle;
    const auto size = d.grids->size();
    require(std::abs(std::int64_t(r.x1) - r.x0) == size && std::abs(std::int64_t(r.y1) - r.y0) == size &&
                size <= domain.width(),
            "Unscaled terrain descriptor required");
    require(size == (d.record_state_flag != 0 ? 128 : 512), "Descriptor flag/grid mismatch");
    require(std::min(r.x0, r.x1) >= -domain.width() &&
                std::int64_t(std::max(r.x0, r.x1)) <= 2LL * domain.width(),
            "Descriptor exceeds single-period X adjustment");
    require(r.y0 >= -65536 && r.y0 <= 131072 && r.y1 >= -65536 && r.y1 <= 131072,
            "Descriptor exceeds supported Y range");
}
} // namespace
TerrainPieceGrids::TerrainPieceGrids(std::int32_t size, std::span<const std::uint16_t> heights,
                                     std::span<const std::uint8_t> materials,
                                     std::span<const std::uint8_t> weights,
                                     std::span<const std::uint8_t> priorities)
    : size_(size) {
    require(size == 128 || size == 512, "Terrain grids must be 128 or 512 square");
    const auto count = std::size_t(size) * size;
    require(heights.size() == count && materials.size() == count && weights.size() == count &&
                priorities.size() == count,
            "Incomplete terrain grids");
    heights_.assign(heights.begin(), heights.end());
    materials_.assign(materials.begin(), materials.end());
    weights_.assign(weights.begin(), weights.end());
    priorities_.assign(priorities.begin(), priorities.end());
}
WorkerPackets route_worker_descriptors(HeightDomain domain, std::span<const SpatialRectangle> rectangles) {
    const auto width = domain.width(), height = domain.height();
    require(width >= 64 && height >= 64 && width <= 65536 && height <= 65536 && width % 64 == 0 &&
                height % 64 == 0,
            "Unsupported descriptor routing domain");
    require(rectangles.size() <= 65536, "Too many terrain descriptors");
    const auto columns = width / 64, rows = height / 64;
    WorkerPackets packets(std::size_t(columns) * rows);
    std::int64_t visits = 0;
    for (std::size_t index = 0; index < rectangles.size(); ++index) {
        const auto &r = rectangles[index];
        const auto min_x = std::min(r.x0, r.x1), max_x = std::max(r.x0, r.x1);
        const auto min_y = std::min(r.y0, r.y1), max_y = std::max(r.y0, r.y1);
        require(min_x >= -width && max_x <= 2 * width && min_y >= -height && max_y <= 2 * height &&
                    min_x < max_x && min_y < max_y,
                "Unsupported routing rectangle");
        visits += ((max_x - min_x) / 64 + 2LL) * ((max_y - min_y) / 64 + 2LL);
        require(visits <= 16777216, "Descriptor routing exceeds bounded work");
        for (auto wx = min_x / 64; wx <= max_x / 64; ++wx) {
            const auto wrapped_x = (100 * columns + wx) % columns;
            for (auto wy = min_y / 64; wy <= max_y / 64; ++wy) {
                if (wy < 0 || wy >= rows)
                    continue;
                auto &packet = packets[wy * columns + wrapped_x];
                require(packet.size() < 256, "Native worker descriptor capacity exceeded");
                packet.push_back(static_cast<std::uint32_t>(index));
            }
        }
    }
    return packets;
}
WorkerAccumulation accumulate_worker64(HeightDomain domain, std::int32_t origin_x, std::int32_t origin_y,
                                       std::span<const TerrainDescriptor> descriptors,
                                       std::span<const std::uint32_t> ordered_packet,
                                       std::span<const std::uint8_t> local_gates, float bias) {
    const auto width = domain.width(), height = domain.height();
    require(width >= 128 && width <= 65536 && height >= 64 && height <= 65536 && width % 64 == 0 &&
                height % 64 == 0,
            "Unsupported worker domain");
    require(origin_x >= 0 && origin_x <= width - 64 && origin_y >= 0 && origin_y <= height - 64 &&
                origin_x % 64 == 0 && origin_y % 64 == 0,
            "Unsupported worker origin");
    require(local_gates.size() == 4096 && std::isfinite(bias), "Invalid worker gates/bias");
    require(ordered_packet.size() <= 256, "Native worker descriptor capacity exceeded");
    WorkerAccumulation result(bias);
    const auto end_x = origin_x + 64, end_y = origin_y + 64;
    for (const auto index : ordered_packet) {
        require(index < descriptors.size(), "Worker descriptor index out of bounds");
        const auto &d = descriptors[index];
        validate_descriptor(domain, d);
        const auto &r = d.rectangle;
        const auto &grids = *d.grids;
        auto x0 = r.x0, x1 = r.x1;
        // Equality must not shift a rectangle! A per-texel modulo differs here.
        if (std::min(x0, x1) > end_x) {
            x0 -= width;
            x1 -= width;
        }
        if (std::max(x0, x1) < origin_x) {
            x0 += width;
            x1 += width;
        }
        const auto left = std::max(std::min(x0, x1), origin_x), right = std::min(std::max(x0, x1), end_x);
        const auto top = std::max(std::min(r.y0, r.y1), origin_y),
                   bottom = std::min(std::max(r.y0, r.y1), end_y);
        require(left <= right && top <= bottom, "Nonintersecting native worker decode window");
        const auto sx = x1 > x0 ? left - x0 : x0 - right;
        const auto sy = r.y1 > r.y0 ? top - r.y0 : r.y0 - bottom;
        for (auto j = 0; j < bottom - top; ++j)
            for (auto i = 0; i < right - left; ++i) {
                const auto x = x1 > x0 ? left + i : right - 1 - i;
                const auto y = r.y1 > r.y0 ? top + j : bottom - 1 - j;
                const auto target = (y - origin_y) * 64 + x - origin_x;
                if (local_gates[target] != d.cell_gate)
                    continue;
                const auto source = (sy + j) * grids.size() + sx + i;
                auto &cell = result.cells_[target];
                const auto priority = grids.priorities()[source];
                if (priority > cell.priority) {
                    cell.priority = priority;
                    cell.material = grids.materials()[source];
                }
                const float weight = float(grids.weights()[source]) * worker_weight_to_normalized;
                const float normalized = float(grids.heights()[source]) * worker_u16_to_normalized;
                const float shifted = normalized - bias;
                const float term = shifted * weight;
                cell.numerator = term + cell.numerator;
                cell.denominator = cell.denominator + weight;
            }
    }
    return result;
}
std::vector<std::uint16_t> WorkerAccumulation::finalize(X87Precision precision) const {
    validate_precision(precision);
    std::vector<std::uint16_t> result(cells_.size());
    for (std::size_t i = 0; i < result.size(); ++i) {
        const auto &cell = cells_[i];
        const double quotient = round_x87(double(cell.numerator) / double(cell.denominator), precision);
        const double normalized = round_x87(quotient + double(bias_), precision);
        const double scaled = round_x87(normalized * 65535.0, precision);
        require(std::isfinite(scaled) && scaled >= double(std::numeric_limits<std::int32_t>::min()) &&
                    scaled <= double(std::numeric_limits<std::int32_t>::max()),
                "Height outside native FISTP domain");
        result[i] = static_cast<std::uint16_t>(static_cast<std::int32_t>(scaled));
    }
    return result;
}
WorkerField::WorkerField(HeightDomain domain)
    : domain_(domain), heights_(std::size_t(domain.width()) * domain.height()), materials_(heights_.size()) {}
WorkerField compose_worker_field(HeightDomain domain, std::span<const TerrainDescriptor> descriptors,
                                 std::span<const std::uint8_t> gates, float bias, X87Precision precision) {
    require(gates.size() == std::size_t(domain.width()) * domain.height() && std::isfinite(bias),
            "Invalid full-field gates/bias");
    validate_precision(precision);
    require(domain.width() >= 128 && descriptors.size() <= 65536, "Invalid worker field domain/descriptors");
    std::vector<SpatialRectangle> rectangles;
    rectangles.reserve(descriptors.size());
    for (const auto &d : descriptors) {
        validate_descriptor(domain, d);
        rectangles.push_back(d.rectangle);
    }
    const auto packets = route_worker_descriptors(domain, rectangles);
    WorkerField field(domain);
    std::array<std::uint8_t, 4096> local_gates;
    const auto columns = domain.width() / 64;
    for (std::size_t index = 0; index < packets.size(); ++index) {
        const auto ox = static_cast<std::int32_t>(index % columns) * 64;
        const auto oy = static_cast<std::int32_t>(index / columns) * 64;
        for (auto y = 0; y < 64; ++y)
            std::copy_n(gates.begin() + (oy + y) * domain.width() + ox, 64, local_gates.begin() + y * 64);
        const auto state =
            accumulate_worker64(domain, ox, oy, descriptors, packets[index], local_gates, bias);
        const auto heights = state.finalize(precision);
        for (auto y = 0; y < 64; ++y)
            for (auto x = 0; x < 64; ++x) {
                const auto target = (oy + y) * domain.width() + ox + x, source = y * 64 + x;
                field.heights_[target] = heights[source];
                field.materials_[target] = state.cells()[source].material;
            }
    }
    return field;
}
} // namespace civ5::terrain
