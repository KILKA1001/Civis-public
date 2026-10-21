#include "civ5/terrain/worker_gates.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace civ5::terrain {
namespace {
void require(bool condition, const char *message) {
    if (!condition)
        throw std::invalid_argument(message);
}
void validate_domain(HeightDomain domain, int limit) {
    require(domain.width() >= 64 && domain.height() >= 64 && domain.width() <= limit &&
                domain.height() <= limit && domain.width() % 64 == 0 && domain.height() % 64 == 0,
            "Unsupported worker gate domain");
}
void validate_point(Point2 p) {
    require(std::isfinite(p.x) && std::isfinite(p.y) && std::abs(p.x) <= 65536.f && std::abs(p.y) <= 65536.f,
            "Worker gate point outside supported bounds");
}
std::int32_t floor_mod(std::int32_t x, std::int32_t period) {
    const auto r = x % period;
    return r < 0 ? r + period : r;
}
LogicalHex transform(LogicalHex v, const IntMatrix4 &m) {
    const auto c = [&](int i) {
        return wrap_add(
            wrap_add(wrap_add(wrap_mul(v.x, m[i]), wrap_mul(v.z, m[8 + i])), wrap_mul(v.y, m[4 + i])),
            m[12 + i]);
    };
    return {c(0), c(1), c(2)};
}
} // namespace

std::array<std::uint8_t, 4096> classify_worker_gates64(HeightDomain domain, std::int32_t ox, std::int32_t oy,
                                                       std::span<const WorkerGateLine> boundaries,
                                                       std::span<const WorkerGateLine> fills,
                                                       std::span<const WorkerGateSeed> seeds) {
    validate_domain(domain, 65536);
    require(ox >= 0 && oy >= 0 && ox <= domain.width() - 64 && oy <= domain.height() - 64 && ox % 64 == 0 &&
                oy % 64 == 0,
            "Invalid gate worker origin");
    require(boundaries.size() <= 32 && fills.size() <= 32 && seeds.size() <= 16,
            "Worker gate packet capacity exceeded");
    std::size_t point_count = 0;
    std::uint64_t work = 0;
    for (const auto lines : {boundaries, fills})
        for (const auto &line : lines) {
            require(!line.points.empty() && line.points.size() <= max_coast_curve_controls, "Invalid worker gate line");
            point_count += line.points.size();
            for (auto p : line.points)
                validate_point(p);
            for (std::size_t i = 1; i < line.points.size(); ++i) {
                const auto a = line.points[i - 1], b = line.points[i];
                work += std::max(std::abs(static_cast<int>(b.x) - static_cast<int>(a.x)),
                                 std::abs(static_cast<int>(b.y) - static_cast<int>(a.y))) +
                        1;
            }
        }
    require(point_count <= max_coast_worker_points && work <= 16777216, "Worker gate traversal capacity exceeded");
    for (const auto &line : fills)
        require((line.gate & 255) != 4, "Gate 4 cannot mark flood visits");
    for (auto seed : seeds)
        require(seed.x >= -65536 && seed.x <= 65536 && seed.y >= -65536 && seed.y <= 65536 &&
                    (seed.gate & 255) != 4,
                "Unsafe worker gate seed");
    std::array<std::uint8_t, 4096> gates;
    gates.fill(4);
    std::array<std::pair<int, int>, 4096> stack;
    std::size_t queued = 0;
    const auto normalize_x = [&](int x) { return (domain.width() * 100 + x) % domain.width(); };
    const auto normalize_y = [&](int y) { return (domain.height() * 100 + y) % domain.height(); };
    const auto index = [&](int x, int y) {
        const auto nx = normalize_x(x), ny = normalize_y(y);
        if (nx < ox || nx >= ox + 64 || ny < oy || ny >= oy + 64)
            return -1;
        return (ny % 64) * 64 + nx % 64;
    };
    const auto visit = [&](int x, int y, std::uint8_t gate) {
        const auto i = index(x, y);
        if (i < 0 || gates[i] != 4)
            return;
        gates[i] = gate; // Mark on enqueue, not on pop.
        if (queued == stack.size())
            throw std::logic_error("Worker flood stack exceeded");
        stack[queued++] = {x, y};
    };
    const auto flood = [&](std::uint8_t gate) {
        while (queued) {
            const auto [x, y] = stack[--queued];
            visit(x + 1, y, gate);
            visit(x - 1, y, gate);
            visit(x, y + 1, gate);
            visit(x, y - 1, gate);
        }
    };
    const auto walk = [](const WorkerGateLine &line, auto put) {
        for (std::size_t i = 1; i < line.points.size(); ++i) {
            int x0 = static_cast<int>(line.points[i - 1].x), y0 = static_cast<int>(line.points[i - 1].y);
            int x1 = static_cast<int>(line.points[i].x), y1 = static_cast<int>(line.points[i].y);
            const bool steep = std::abs(x1 - x0) < std::abs(y1 - y0);
            if (steep) {
                std::swap(x0, y0);
                std::swap(x1, y1);
            }
            if (x1 < x0) {
                std::swap(x0, x1);
                std::swap(y0, y1);
            }
            const int dx = x1 - x0, dy = std::abs(y1 - y0), direction = y0 < y1 ? 1 : -1;
            int error = dx / 2;
            for (int x = x0; x <= x1; ++x) {
                if (steep)
                    put(y0, x);
                else
                    put(x, y0);
                error -= dy;
                if (error < 0) {
                    y0 += direction;
                    error += dx;
                }
            }
        }
    };
    for (const auto &line : boundaries)
        walk(line, [&](int x, int y) {
            const auto i = index(x, y);
            if (i >= 0)
                gates[i] = static_cast<std::uint8_t>(line.gate);
        });
    for (const auto &line : fills) {
        // Fill lines normalize BEFORE enqueue. Explicit seeds retain their
        // coordinates: the signed +100-period helper is deliberately not floorMod.
        walk(line, [&](int x, int y) {
            if (index(x, y) >= 0)
                visit(normalize_x(x), normalize_y(y), static_cast<std::uint8_t>(line.gate));
        });
        flood(static_cast<std::uint8_t>(line.gate));
    }
    for (auto seed : seeds) {
        visit(seed.x, seed.y, static_cast<std::uint8_t>(seed.gate));
        flood(static_cast<std::uint8_t>(seed.gate));
    }
    for (auto &gate : gates)
        if (gate == 4)
            gate = 2;
    return gates;
}

WorkerGateRouting::WorkerGateRouting(HeightDomain domain, std::vector<std::uint8_t> categories,
                                     std::vector<Point2> points, std::vector<RoutedGateWorker> workers)
    : domain_(domain), categories_(std::move(categories)), points_(std::move(points)),
      workers_(std::move(workers)) {}

std::array<std::uint8_t, 4096> WorkerGateRouting::classify_worker64(std::size_t index) const {
    require(index < workers_.size(), "Worker gate index outside domain");
    const auto &worker = workers_[index];
    const auto lines = [&](const auto &ranges) {
        std::vector<WorkerGateLine> result;
        for (auto range : ranges)
            result.push_back({std::span(points_).subspan(range.first, range.end - range.first), range.gate});
        return result;
    };
    return classify_worker_gates64(domain_, worker.origin_x, worker.origin_y, lines(worker.boundaries),
                                   lines(worker.fills), worker.seeds);
}
std::vector<std::uint8_t> WorkerGateRouting::build_gate_field64() const {
    std::vector<std::uint8_t> result(std::size_t(domain_.width()) * domain_.height());
    for (std::size_t i = 0; i < workers_.size(); ++i) {
        const auto block = classify_worker64(i);
        const auto &worker = workers_[i];
        for (int y = 0; y < 64; ++y)
            std::copy_n(block.begin() + y * 64, 64,
                        result.begin() + std::size_t(worker.origin_y + y) * domain_.width() +
                            worker.origin_x);
    }
    return result;
}

WorkerGateRouting route_worker_gates(HeightDomain domain, const WorkerGateSource &s,
                                     std::span<const Point2> points,
                                     std::span<const WorkerGateRange> boundaries,
                                     std::span<const WorkerGateRange> fills) {
    validate_domain(domain, 65536);
    // The original category owner has 255x255 byte slots. Worker addressing
    // already supports the same 65536-pixel bound as classify_worker_gates64.
    require(s.width >= 1 && s.width <= 255 && s.height >= 1 && s.height <= 255 &&
                s.masks.size() == std::size_t(s.width) * s.height,
            "Invalid logical gate mask grid");
    require(s.precision == X87Precision::single || s.precision == X87Precision::double_precision,
            "Unsupported worker transform precision");
    require(s.transform_path == Vec4Path::legacy_x87 || s.transform_path == Vec4Path::sse,
            "Unsupported worker transform path");
    for (auto v : s.world_transform)
        require(std::isfinite(v), "Nonfinite worker world transform");
    require(points.size() <= max_coast_worker_points && boundaries.size() <= max_coast_contours &&
                fills.size() <= max_coast_contours * 2,
            "Worker gate source capacity exceeded");
    for (auto p : points)
        validate_point(p);
    std::uint64_t work = 0;
    for (const auto ranges : {boundaries, fills})
        for (auto r : ranges) {
            require(r.first <= r.end && r.end <= points.size(), "Invalid gate source range");
            work += r.end - r.first;
        }
    require(work <= 16777216, "Worker gate range traversal capacity exceeded");
    const auto columns = domain.width() / 64, rows = domain.height() / 64, count = columns * rows;
    std::vector<RoutedGateWorker> workers;
    for (int i = 0; i < count; ++i)
        workers.push_back({i % columns * 64, i / columns * 64, {}, {}, {}});
    std::vector<std::uint8_t> categories;
    for (int y = 0; y < s.height; ++y)
        for (int x = 0; x < s.width; ++x) {
            const auto mask = s.masks[x * s.height + y];
            categories.push_back((mask & 0xf3ef9fffu) ? 0 : (mask & 0x0c102000u) ? 1 : 2);
        }
    const auto assign = [&](auto ranges, bool boundary) {
        for (auto range : ranges) {
            int last = -1;
            for (auto i = range.first; i < range.end; ++i) {
                // Range ownership uses vertices, not all cells crossed by edges.
                const auto x = floor_mod(static_cast<int>(points[i].x), domain.width()) / 64;
                const auto y = floor_mod(static_cast<int>(points[i].y), domain.height()) / 64;
                const auto index = y * columns + x;
                if (index == last)
                    continue;
                last = index;
                auto &list = boundary ? workers[index].boundaries : workers[index].fills;
                if (std::any_of(list.begin(), list.end(), [&](auto r) {
                        return r.first == range.first && r.end == range.end; // Gate is NOT part of identity.
                    }))
                    continue;
                require(list.size() < 32, "Worker gate range packet exceeded");
                list.push_back(range);
            }
        }
    };
    assign(boundaries, true);
    assign(fills, false);
    constexpr std::array<std::pair<int, int>, 4> offsets{{{1, 1}, {-1, 1}, {1, -1}, {-1, -1}}};
    for (int y = 0; y < s.height; ++y)
        for (int x = 0; x < s.width; ++x) {
            const auto a = x - (y >> 1);
            const auto mapped = transform({a, y, -a - y}, s.logical_to_map);
            const LogicalHex position{mapped.x, mapped.y, wrap_neg(wrap_add(mapped.x, mapped.y))};
            const auto lookup = transform(position, s.map_to_logical);
            // C++20 signed right shift is arithmetic; additions retain original wrapping.
            auto column = wrap_add(lookup.x, lookup.y >> 1);
            auto category = s.outside_category;
            if (lookup.y >= 0 && lookup.y < s.height && (s.wrap_x || (column >= 0 && column < s.width))) {
                if (s.wrap_x)
                    column = floor_mod(column, s.width);
                category = categories[lookup.y * s.width + column];
            }
            const auto world =
                vector4_transform({static_cast<float>(position.x), static_cast<float>(position.y),
                                   static_cast<float>(position.z), 1.f},
                                  s.world_transform, s.precision, s.transform_path);
            require(std::isfinite(world[0]) && std::isfinite(world[1]) && std::abs(world[0]) <= 65535.f &&
                        std::abs(world[1]) <= 65535.f,
                    "Worker seed projection outside supported CVTT bounds");
            for (auto [dx, dy] : offsets) {
                const auto sx = static_cast<int>(world[0]) + dx, sy = static_cast<int>(world[1]) + dy;
                const auto wx = sx / 64, wy = sy / 64; // Signed truncation, not floor or wrap.
                if (wx >= columns || wy >= rows)
                    continue;
                const auto index = wy * columns + wx;
                // Native has upper-coordinate checks only. Preserve negative-X
                // flattened aliases, but reject an actual out-of-arena write.
                require(index >= 0 && index < count, "Worker seed routing leaves bounded arena");
                require(workers[index].seeds.size() < 16, "Worker seed packet exceeded");
                workers[index].seeds.push_back({sx, sy, category});
            }
        }
    return WorkerGateRouting(domain, std::move(categories), {points.begin(), points.end()},
                             std::move(workers));
}
} // namespace civ5::terrain
