#include "civ5/terrain/terrain_auxiliary_owners.hpp"
#include "civ5/terrain/height_owner.hpp"
#include <algorithm>
#include <bit>
#include <climits>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace civ5::terrain {
namespace {
template <class Grid> void validate_storage(const Grid &grid) {
    if (grid.cells.size() != Grid::capacity)
        throw std::invalid_argument("Invalid auxiliary owner prior storage");
}
template <class Grid> void validate(const Grid &grid) {
    validate_storage(grid);
    if (grid.coordinates.width <= 0 || grid.coordinates.height <= 0 ||
        std::uint32_t(grid.coordinates.width) > Grid::stride ||
        std::uint32_t(grid.coordinates.height) > Grid::stride)
        throw std::invalid_argument("Invalid auxiliary owner extent or prior storage");
}
std::array<std::int32_t, 2> offset(LogicalHex p, const WorldGrid &grid) {
    const auto &m = grid.first_matrix;
    const auto transform = [&](unsigned i) {
        return wrap_add(wrap_add(wrap_add(wrap_mul(p.x, m[i]), wrap_mul(p.y, m[4+i])),
                                 wrap_mul(p.z, m[8+i])), m[12+i]);
    };
    const auto y = transform(1);
    return {wrap_add(transform(0), y >> 1), y};
}
LogicalHex logical_from_cell(std::int32_t x, std::int32_t y, const WorldGrid &grid) {
    const auto &m = grid.second_matrix;
    const auto q = wrap_add(x, wrap_neg(y >> 1));
    const auto s = wrap_neg(wrap_add(q, y));
    const auto transform = [&](unsigned lane) {
        return wrap_add(wrap_add(wrap_add(wrap_mul(q, m[lane]),
                                          wrap_mul(y, m[4 + lane])),
                                 wrap_mul(s, m[8 + lane])), m[12 + lane]);
    };
    LogicalHex logical{transform(0), transform(1), 0};
    logical.z = wrap_neg(wrap_add(logical.x, logical.y));
    return logical;
}
constexpr LogicalHex ice_directions[]{
    {0, 1, -1}, {-1, 0, 1}, {1, -1, 0},
    {0, -1, 1}, {1, 0, -1}, {-1, 1, 0}};
template <class Grid> bool contains(const Grid &grid, std::array<std::int32_t, 2> p, bool allow_wrap) {
    return (allow_wrap || (p[0] >= 0 && p[0] < grid.coordinates.width)) &&
        p[1] >= 0 && p[1] < grid.coordinates.height;
}
template <class Grid> auto &physical(Grid &grid, std::array<std::int32_t, 2> p, bool wrap) {
    if (wrap) {
        p[0] %= grid.coordinates.width;
        if (p[0] < 0) p[0] += grid.coordinates.width;
    }
    const auto index = wrap_add(p[0], wrap_mul(p[1], static_cast<std::int32_t>(Grid::stride)));
    if (index < 0 || std::uint32_t(index) >= Grid::capacity)
        throw std::invalid_argument("Auxiliary record addresses storage outside its owner");
    return grid.cells[static_cast<std::size_t>(index)];
}
template <class Grid> auto &bounded(Grid &grid, LogicalHex p) {
    const auto cell = offset(p, grid.coordinates);
    return contains(grid, cell, grid.wrap_x) ? physical(grid, cell, grid.wrap_x) : grid.fallback;
}
void apply_relief(TerrainAuxiliaryOwnerState &state, TerrainReliefSource source) {
    const LogicalHex p{source.x, source.y, -std::int32_t(source.x)-std::int32_t(source.y)};
    if (state.relief_enabled) {
        const auto cell = offset(p, state.relief.coordinates);
        if (source.kind <= 1 && contains(state.relief, cell, state.relief.wrap_x))
            physical(state.relief, cell, state.relief.wrap_x)[4] |= source.kind == 0 ? 2 : 4;
        const auto mask = static_cast<std::uint8_t>(source.kind <= 3 ? 2u << source.kind : 0u);
        auto &center = bounded(state.adjacency, p);
        center[0] = static_cast<std::uint8_t>((center[0] & ~0xc0u) |
            (source.kind == 0 ? 0x40 : source.kind == 2 ? 0x80 : 0));
        for (unsigned i = 6; i < 9; ++i) center[i] |= mask;
        // Original C7FB2C/38/44 neighbors and their shared-corner bytes.
        static constexpr LogicalHex neighbors[]{{0, -1, 1}, {1, 0, -1}, {-1, 1, 0}};
        for (unsigned i = 0; i < 3; ++i) {
            const auto d = neighbors[i];
            const auto cell = offset({wrap_add(p.x, d.x), wrap_add(p.y, d.y), wrap_add(p.z, d.z)},
                                     state.adjacency.coordinates);
            if (contains(state.adjacency, cell, state.adjacency.wrap_x))
                physical(state.adjacency, cell, state.adjacency.wrap_x)[6+i] |= mask;
        }
    }
    // 42AB00 is a raw 256-stride accessor: no logical bounds or X wrapping.
    if (state.water_enabled && source.kind == 0)
        physical(state.water, offset(p, state.water.coordinates), false)[2] = 1;
}
} // namespace
TerrainIceGrid initialize_terrain_ice_grid(const TerrainIceGrid &prior,
    std::int32_t width, std::int32_t height, const FactoryDescriptor &factory) {
    // The old header may still be uninitialized: 6DC370 replaces it before
    // using dimensions or matrices. Only retained storage has a precondition.
    if (prior.cells.size() != TerrainIceGrid::capacity)
        throw std::invalid_argument("Invalid prior ice storage");
    auto result = prior;
    result.coordinates = build_world_grid_layout(width, height, 1, factory).source_grid;
    validate(result);
    for (std::int32_t x = 0; x < width; ++x)
        for (std::int32_t y = 0; y < height; ++y)
            physical(result, {x, y}, result.wrap_x)[0] = 0;
    return result;
}
TerrainIceInput build_terrain_ice_input(const TerrainIceGrid &prior) {
    validate(prior);
    TerrainIceInput result{prior};
    for (std::int32_t y = 0; y < prior.coordinates.height; ++y)
        for (std::int32_t x = 0; x < prior.coordinates.width; ++x) {
            const auto logical = logical_from_cell(x, y, prior.coordinates);
            auto &center = physical(result.grid, {x, y}, prior.wrap_x)[0];
            if ((center & 0x80u) == 0) continue;
            ++result.ice_cells;
            center |= 0x40u;
            for (unsigned direction = 0; direction < 6; ++direction) {
                const auto d = ice_directions[direction];
                const LogicalHex neighbor{
                    wrap_add(logical.x, d.x), wrap_add(logical.y, d.y),
                    wrap_add(logical.z, d.z)};
                const auto cell = offset(neighbor, prior.coordinates);
                if (!contains(prior, cell, prior.wrap_x) ||
                    (physical(result.grid, cell, prior.wrap_x)[0] & 0x80u) == 0) {
                    center |= static_cast<std::uint8_t>(1u << direction);
                    ++result.exposed_sides;
                }
            }
        }
    return result;
}
TerrainIceGrouping group_terrain_ice_input(const TerrainIceInput &input) {
    validate(input.grid);
    TerrainIceGrouping result{input.grid};
    result.vertices.reserve(input.ice_cells);
    result.boundary_flags.reserve(input.ice_cells);
    std::vector<LogicalHex> stack;
    stack.reserve(input.ice_cells);
    std::int32_t scan_x = 0, scan_y = 0;
    while (result.vertices.size() < input.ice_cells) {
        while (scan_y < input.grid.coordinates.height &&
               (physical(result.grid, {scan_x, scan_y}, result.grid.wrap_x)[0] & 0x40u) == 0) {
            if (++scan_x == input.grid.coordinates.width) {
                scan_x = 0;
                ++scan_y;
            }
        }
        if (scan_y == input.grid.coordinates.height)
            throw std::invalid_argument("Ice input count exceeds marked cells");
        const auto first = static_cast<std::uint32_t>(result.vertices.size());
        stack.push_back(logical_from_cell(scan_x, scan_y, result.grid.coordinates));
        physical(result.grid, {scan_x, scan_y}, result.grid.wrap_x)[0] &= 0xbfu;
        while (!stack.empty()) {
            const auto p = stack.back();
            stack.pop_back();
            const auto center = physical(result.grid, offset(p, result.grid.coordinates),
                                         result.grid.wrap_x)[0];
            std::uint8_t flag = 0;
            if (center > 0x80u) {
                for (unsigned direction = 0; direction < 6; ++direction) {
                    const auto d = ice_directions[direction];
                    const LogicalHex neighbor{wrap_add(p.x, d.x), wrap_add(p.y, d.y),
                                              wrap_add(p.z, d.z)};
                    if ((center & (1u << direction)) != 0 &&
                        contains(result.grid, offset(neighbor, result.grid.coordinates),
                                 result.grid.wrap_x)) flag = 1;
                }
            }
            result.vertices.push_back(p);
            result.boundary_flags.push_back(flag);
            for (unsigned direction = 0; direction < 6; ++direction) {
                if ((center & (1u << direction)) != 0) continue;
                const auto d = ice_directions[direction];
                const LogicalHex neighbor{wrap_add(p.x, d.x), wrap_add(p.y, d.y),
                                          wrap_add(p.z, d.z)};
                const auto cell = offset(neighbor, result.grid.coordinates);
                if (!contains(result.grid, cell, result.grid.wrap_x))
                    throw std::invalid_argument("Ice input has an unmarked outer boundary");
                auto &neighbor_byte = physical(result.grid, cell, result.grid.wrap_x)[0];
                if ((neighbor_byte & 0x40u) != 0) {
                    neighbor_byte &= 0xbfu;
                    stack.push_back(neighbor);
                }
            }
            if (result.vertices.size() > input.ice_cells)
                throw std::invalid_argument("Ice input count is smaller than marked cells");
        }
        result.groups.push_back({first,
            static_cast<std::uint32_t>(result.vertices.size()) - first});
    }
    return result;
}
namespace {
constexpr unsigned ice_next_side[]{4, 5, 3, 1, 2, 0};
constexpr unsigned ice_neighbor_side[]{5, 3, 4, 2, 0, 1};

LogicalHex ice_neighbor(LogicalHex p, unsigned direction) {
    const auto d = ice_directions[direction];
    return {wrap_add(p.x, d.x), wrap_add(p.y, d.y), wrap_add(p.z, d.z)};
}
bool same_hex(LogicalHex a, LogicalHex b) {
    return a.x == b.x && a.y == b.y && a.z == b.z;
}
bool step_ice_contour(TerrainIceGrid &grid, LogicalHex &p, unsigned &side,
                      bool wrap_bounds) {
    auto &center = physical(grid, offset(p, grid.coordinates), grid.wrap_x)[0];
    const unsigned next = ice_next_side[side];
    if ((center & (1u << next)) != 0) {
        side = next;
        return true;
    }
    const auto adjacent = ice_neighbor(p, next);
    const auto cell = offset(adjacent, grid.coordinates);
    if (contains(grid, cell, wrap_bounds) &&
        (physical(grid, cell, grid.wrap_x)[0] & (1u << ice_neighbor_side[side])) != 0) {
        p = adjacent;
        side = ice_neighbor_side[side];
        return true;
    }
    return false;
}
void collect_ice_contours(TerrainIceContours &result,
                          TerrainIceContourGroup &group, bool wrap_bounds,
                          std::size_t edge_limit) {
    for (std::uint32_t i = 0; i < group.vertex_count; ++i) {
        const auto start = result.vertices[group.first_vertex + i];
        auto &start_byte = physical(result.grid, offset(start, result.grid.coordinates),
                                    result.grid.wrap_x)[0];
        while (start_byte > 0x80u) {
            unsigned side = 0;
            while ((start_byte & (1u << side)) == 0) {
                if (++side == 6)
                    throw std::invalid_argument("Ice contour has no exposed side bit");
            }
            const auto first = static_cast<std::uint32_t>(result.edges.size());
            result.segments.push_back({first, 0});
            auto &segment = result.segments.back();
            auto p = start;
            for (;;) {
                result.edges.push_back({p, static_cast<std::uint8_t>(side)});
                ++segment.edge_count;
                if (result.edges.size() > edge_limit)
                    throw std::invalid_argument("Ice contour exceeded six edges per marked cell");
                auto &current = physical(result.grid, offset(p, result.grid.coordinates),
                                         result.grid.wrap_x)[0];
                current &= static_cast<std::uint8_t>(~(1u << side));
                if (!step_ice_contour(result.grid, p, side, wrap_bounds)) break;
            }
        }
    }
    group.segment_count = static_cast<std::uint32_t>(result.segments.size()) - group.first_segment;
}
bool ice_contour_closed(const TerrainIceContourSegment &segment,
                        const std::vector<TerrainIceContourEdge> &edges) {
    if (segment.edge_count == 0) return false;
    const auto &first = edges[segment.first_edge];
    const auto &last = edges[segment.first_edge + segment.edge_count - 1];
    const auto next = ice_next_side[last.side];
    if (same_hex(first.cube, last.cube)) return first.side == next;
    return same_hex(first.cube, ice_neighbor(last.cube, next)) &&
           first.side == ice_next_side[(next + 3) % 6];
}
} // namespace
TerrainIceContours build_terrain_ice_contours_mode1(const TerrainIceGrouping &input) {
    validate(input.grid);
    if (!input.grid.wrap_x || input.vertices.size() != input.boundary_flags.size() ||
        input.vertices.size() > static_cast<std::size_t>(input.grid.coordinates.width) *
                                    input.grid.coordinates.height)
        throw std::invalid_argument("Invalid wrapped ice grouping");
    TerrainIceContours result{input.grid, {}, input.vertices, input.boundary_flags};
    const auto edge_limit = input.vertices.size() * 6;
    std::size_t next_vertex = 0;
    for (const auto source : input.groups) {
        if (source.first_vertex != next_vertex || source.vertex_count == 0 ||
            source.vertex_count > input.vertices.size() - next_vertex)
            throw std::invalid_argument("Invalid ice group vertex span");
        next_vertex += source.vertex_count;
        const auto first_edge = result.edges.size();
        result.groups.push_back({static_cast<std::uint32_t>(result.segments.size()),
                                 source.first_vertex, 0, source.vertex_count});
        auto &group = result.groups.back();
        collect_ice_contours(result, group, true, edge_limit);
        bool all_closed = true;
        for (std::uint32_t i = 0; i < group.segment_count; ++i)
            if (!ice_contour_closed(result.segments[group.first_segment + i], result.edges)) {
                all_closed = false;
                break;
            }
        if (!all_closed) {
            for (std::size_t i = first_edge; i < result.edges.size(); ++i) {
                const auto &edge = result.edges[i];
                physical(result.grid, offset(edge.cube, result.grid.coordinates),
                         result.grid.wrap_x)[0] |= static_cast<std::uint8_t>(1u << edge.side);
            }
            for (std::uint32_t i = 0; i < group.vertex_count; ++i) {
                auto &vertex = result.vertices[group.first_vertex + i];
                auto cell = offset(vertex, result.grid.coordinates);
                cell[0] %= result.grid.coordinates.width;
                if (cell[0] < 0) cell[0] += result.grid.coordinates.width;
                vertex = logical_from_cell(cell[0], cell[1], result.grid.coordinates);
                auto &byte = physical(result.grid, cell, result.grid.wrap_x)[0];
                for (unsigned direction = 0; direction < 6; ++direction)
                    if (!contains(result.grid,
                                  offset(ice_neighbor(vertex, direction), result.grid.coordinates),
                                  false))
                        byte |= static_cast<std::uint8_t>(1u << direction);
            }
            result.segments.resize(group.first_segment);
            result.edges.resize(first_edge);
            group.segment_count = 0;
            collect_ice_contours(result, group, false, edge_limit);
            continue;
        }
        std::uint32_t outside = 0, last_outside = group.first_vertex;
        for (std::uint32_t i = 0; i < group.vertex_count; ++i) {
            const auto index = group.first_vertex + i;
            if (!contains(result.grid,
                          offset(result.vertices[index], result.grid.coordinates), false)) {
                ++outside;
                last_outside = index;
            }
        }
        if (outside <= group.vertex_count / 2) continue;
        const auto old = result.vertices[last_outside];
        auto cell = offset(old, result.grid.coordinates);
        cell[0] %= result.grid.coordinates.width;
        if (cell[0] < 0) cell[0] += result.grid.coordinates.width;
        const auto canonical = logical_from_cell(cell[0], cell[1], result.grid.coordinates);
        const LogicalHex delta{wrap_add(canonical.x, wrap_neg(old.x)),
                               wrap_add(canonical.y, wrap_neg(old.y)),
                               wrap_add(canonical.z, wrap_neg(old.z))};
        for (std::uint32_t i = 0; i < group.vertex_count; ++i) {
            auto &p = result.vertices[group.first_vertex + i];
            p = {wrap_add(p.x, delta.x), wrap_add(p.y, delta.y), wrap_add(p.z, delta.z)};
        }
        for (std::size_t i = first_edge; i < result.edges.size(); ++i) {
            auto &p = result.edges[i].cube;
            p = {wrap_add(p.x, delta.x), wrap_add(p.y, delta.y), wrap_add(p.z, delta.z)};
        }
    }
    if (next_vertex != input.vertices.size())
        throw std::invalid_argument("Ice groups omit vertices");
    return result;
}
TerrainIceAllocation count_terrain_ice_allocation(const TerrainIceContours &contours,
    std::uint32_t lattice_vertices_per_hex) {
    validate(contours.grid);
    std::size_t next_edge = 0;
    std::uint32_t inside = 0, outside = 0;
    for (const auto &segment : contours.segments) {
        if (segment.first_edge != next_edge || segment.edge_count == 0 ||
            segment.edge_count > contours.edges.size() - next_edge)
            throw std::invalid_argument("Invalid ice contour segment span");
        for (std::uint32_t i = 0; i < segment.edge_count; ++i) {
            const auto &edge = contours.edges[next_edge + i];
            if (edge.side >= 6)
                throw std::invalid_argument("Invalid ice contour edge side");
            const auto adjacent = ice_neighbor(edge.cube, edge.side);
            if (contains(contours.grid, offset(adjacent, contours.grid.coordinates),
                         contours.grid.wrap_x))
                ++inside;
            else
                ++outside;
        }
        next_edge += segment.edge_count;
    }
    if (next_edge != contours.edges.size())
        throw std::invalid_argument("Ice contour segments omit edges");
    return {lattice_vertices_per_hex * inside + outside, inside};
}
TerrainIceMeshFlagPlan plan_terrain_ice_mesh_flags(const TerrainIceContours &contours,
    std::uint32_t lattice_vertices_per_hex) {
    if (lattice_vertices_per_hex == 0)
        throw std::invalid_argument("Ice lattice vertex count must be positive");
    const auto allocation = count_terrain_ice_allocation(contours, lattice_vertices_per_hex);
    TerrainIceMeshFlagPlan result;
    result.segments.reserve(contours.segments.size());
    result.flags.reserve(allocation.vertices);
    for (const auto &segment : contours.segments) {
        const auto count = static_cast<std::size_t>(segment.edge_count);
        std::vector<std::uint8_t> interior;
        interior.reserve(count);
        for (std::size_t i = 0; i < count; ++i) {
            const auto &edge = contours.edges[segment.first_edge + i];
            interior.push_back(contains(contours.grid,
                offset(ice_neighbor(edge.cube, edge.side), contours.grid.coordinates),
                contours.grid.wrap_x) ? 1 : 0);
        }
        std::size_t first = 0;
        for (std::size_t i = count; i != 0; --i)
            if (interior[i - 1] != interior[0]) {
                first = i % count;
                break;
            }
        const auto first_vertex = static_cast<std::uint32_t>(result.flags.size());
        for (std::size_t processed = 0; processed < count;) {
            const auto value = interior[first];
            std::size_t run = 1;
            while (processed + run < count && interior[(first + run) % count] == value)
                ++run;
            result.flags.insert(result.flags.end(),
                run * (value ? lattice_vertices_per_hex : 1u), value);
            processed += run;
            first = (first + run) % count;
        }
        result.segments.push_back({first_vertex,
            static_cast<std::uint32_t>(result.flags.size()) - first_vertex});
    }
    if (result.flags.size() != allocation.vertices)
        throw std::invalid_argument("Ice mesh flags disagree with allocation count");
    return result;
}
TerrainIceCurveParameters build_terrain_ice_curve_parameters(const WorldGrid &world,
    const TerrainIceProjection &projection, std::uint32_t lattice_vertices_per_hex) {
    if (world.width <= 0 || world.height <= 0 || lattice_vertices_per_hex == 0)
        throw std::invalid_argument("Invalid ice curve world or lattice setting");
    const auto project = [&](float x, float y, float z) {
        const auto &m = projection.world_matrix;
        const float px = ((x * m[0] + m[8] * z) + m[4] * y) + m[12];
        const float py = ((x * m[1] + m[9] * z) + m[5] * y) + m[13];
        return std::array<float, 2>{projection.scale_x * px - projection.center_x,
                                    projection.scale_y * py - projection.center_y};
    };
    const auto negative = [](float x) {
        return std::bit_cast<float>(std::bit_cast<std::uint32_t>(x) ^ 0x80000000u);
    };
    const auto x = static_cast<float>(world.first_matrix[12]);
    const auto y = static_cast<float>(world.first_matrix[13]);
    const auto z = static_cast<float>(wrap_neg(wrap_add(world.first_matrix[12],
                                                       world.first_matrix[13])));
    const auto origin = project(x, y, z);
    static constexpr std::array<std::array<float, 2>, 6> hex_corner{{
        {0, 1}, {-1, 0}, {1, -1}, {0, -1}, {1, 0}, {-1, 1}}};
    std::array<float, 38> v{};
    for (std::size_t i = 0; i < hex_corner.size(); ++i) {
        const float cx = hex_corner[i][0] + x;
        const float cy = hex_corner[i][1] + y;
        const auto corner = project(cx, cy, negative(cx) - cy);
        v[26 + i * 2] = negative(origin[0]) + corner[0];
        v[27 + i * 2] = corner[1] + negative(origin[1]);
    }
    const float third = std::bit_cast<float>(0x3eaaaaabu);
    const float quarter = 0.25f, three_quarters = 0.75f;
    float a = (v[34] + v[26]) * third;
    float b = (v[35] + v[27]) * third;
    float c = (v[36] + v[26]) * third;
    float d = (v[27] + v[37]) * third;
    v[2] = c * quarter + a * three_quarters;
    v[4] = c * three_quarters + a * quarter;
    v[3] = d * quarter + b * three_quarters;
    v[5] = d * three_quarters + b * quarter;
    d = (v[37] + v[29]) * third;
    b = (v[32] + v[28]) * third;
    a = (v[36] + v[28]) * third;
    c = (v[29] + v[33]) * third;
    v[6] = b * quarter + a * three_quarters;
    v[7] = c * quarter + d * three_quarters;
    v[9] = c * three_quarters + d * quarter;
    v[8] = b * three_quarters + a * quarter;
    d = (v[33] + v[31]) * third;
    b = (v[34] + v[30]) * third;
    a = (v[32] + v[30]) * third;
    c = (v[31] + v[35]) * third;
    v[10] = b * quarter + a * three_quarters;
    v[11] = c * quarter + d * three_quarters;
    v[13] = c * three_quarters + d * quarter;
    v[12] = b * three_quarters + a * quarter;
    d = (v[29] + v[33]) * third;
    b = (v[30] + v[32]) * third;
    a = (v[28] + v[32]) * third;
    c = (v[33] + v[31]) * third;
    v[14] = b * quarter + a * three_quarters;
    v[17] = c * three_quarters + d * quarter;
    v[15] = c * quarter + d * three_quarters;
    v[16] = b * three_quarters + a * quarter;
    d = (v[31] + v[35]) * third;
    b = (v[26] + v[34]) * third;
    a = (v[30] + v[34]) * third;
    c = (v[35] + v[27]) * third;
    v[18] = b * quarter + a * three_quarters;
    v[21] = c * three_quarters + d * quarter;
    v[19] = c * quarter + d * three_quarters;
    v[20] = b * three_quarters + a * quarter;
    b = (v[26] + v[36]) * third;
    d = (v[27] + v[37]) * third;
    a = (v[28] + v[36]) * third;
    c = (v[37] + v[29]) * third;
    v[22] = a * quarter + b * three_quarters;
    v[23] = c * quarter + d * three_quarters;
    v[24] = a * three_quarters + b * quarter;
    v[25] = c * three_quarters + d * quarter;
    TerrainIceCurveParameters result;
    result.radius = std::sqrt(v[27] * v[27] + v[26] * v[26]);
    result.lattice_vertices_per_hex = lattice_vertices_per_hex;
    std::copy_n(v.begin() + 2, 24, result.controls.begin());
    std::copy_n(v.begin() + 26, 12, result.corners.begin());
    return result;
}
TerrainIceRandomState initial_terrain_ice_random_state() {
    return {0, 0, 0, 0xffffcfefu, 2.3283131071116735e-10};
}
float draw_terrain_ice_region_prune_random(TerrainIceRandomState &random,
    std::uint32_t x87_precision) {
    if (x87_precision != 24 && x87_precision != 53)
        throw std::invalid_argument("Unsupported ice region RNG x87 precision");
    if (random.modulus != 0xffffcfefu)
        throw std::invalid_argument("Invalid native ice region RNG modulus");
    ++random.draws;
    random.value = (random.value * 0x343fdu + 0x269ec3u) % random.modulus;
    // FILD treats the remainder as signed; the native negative path adds
    // 2^32 before FMUL. PC24 rounds that addition to the float mantissa.
    const long double input = x87_precision == 24 && (random.value & 0x80000000u)
        ? static_cast<long double>(static_cast<float>(random.value))
        : static_cast<long double>(random.value);
    return static_cast<float>(input *
                              static_cast<long double>(random.inverse_modulus));
}
bool should_prune_terrain_ice_region_record(TerrainIceRandomState &random,
    std::uint32_t vertex_count, bool boundary, float boundary_kill_rate,
    float interior_kill_rate,
    std::uint32_t x87_precision) {
    if (vertex_count > 16) return false;
    const auto draw = draw_terrain_ice_region_prune_random(random, x87_precision);
    return (boundary ? boundary_kill_rate : interior_kill_rate) > draw;
}
TerrainIceSourceAdjacency build_terrain_ice_source_adjacency(
    std::uint32_t point_count,
    std::span<const std::array<std::uint32_t, 2>> edges) {
    if (edges.size() > std::numeric_limits<std::uint32_t>::max() / 2)
        throw std::invalid_argument("Ice source adjacency is too large");
    TerrainIceSourceAdjacency result;
    result.ranges.resize(point_count);
    for (const auto &edge : edges) {
        if (edge[0] >= point_count || edge[1] >= point_count)
            throw std::invalid_argument("Ice source adjacency endpoint escaped points");
        ++result.ranges[edge[0]][1];
        ++result.ranges[edge[1]][1];
    }
    std::vector<std::uint32_t> cursor(point_count);
    std::uint32_t offset = 0;
    for (std::size_t point = 0; point < result.ranges.size(); ++point) {
        auto &range = result.ranges[point];
        range[0] = offset;
        offset += range[1];
        cursor[point] = offset;
    }
    result.edge_indices.resize(2 * edges.size());
    for (std::uint32_t edge = 0; edge < edges.size(); ++edge)
        for (const auto point : edges[edge])
            result.edge_indices[--cursor[point]] = edge;
    return result;
}
void weight_terrain_ice_source_graph(
    std::span<TerrainIceSourceWeightPoint> points,
    std::span<TerrainIceSourceWeightEdge> edges,
    const TerrainIceSourceAdjacency &adjacency,
    TerrainIceSourceWeightSettings settings) {
    if (points.size() > UINT32_MAX || edges.size() > UINT32_MAX / 2 ||
        (!adjacency.ranges.empty() && adjacency.ranges.size() != points.size()) ||
        (!adjacency.ranges.empty() && adjacency.edge_indices.size() != 2 * edges.size()) ||
        (adjacency.ranges.empty() && !adjacency.edge_indices.empty()) ||
        !std::isfinite(settings.crack_cost) ||
        !std::isfinite(settings.initial_potential) ||
        !std::isfinite(settings.min_clip) || !std::isfinite(settings.max_clip) ||
        !std::isfinite(settings.source_scale))
        throw std::invalid_argument("Invalid ice source-weight input");
    for (auto &point : points) {
        if (!std::isfinite(point.xy[0]) || !std::isfinite(point.xy[1]))
            throw std::invalid_argument("Nonfinite ice source point");
        point.potential = point.active ? settings.crack_cost : 0.0f;
    }
    for (auto &edge : edges) {
        if (edge.first >= points.size() || edge.second >= points.size())
            throw std::invalid_argument("Ice source-weight edge escaped points");
        edge.weights = edge.boundary ? std::array{1.0f, 1.0f} :
                                       std::array{0.0f, 0.0f};
        edge.pending = {0.0f, 0.0f};
        if (edge.boundary) {
            points[edge.first].active = 1;
            points[edge.second].active = 1;
        }
    }
    std::vector<std::uint32_t> active;
    active.reserve(edges.size());
    for (std::size_t i = 0; i < edges.size(); ++i) {
        auto &edge = edges[i];
        edge.propagation = {
            points[edge.first].potential > 0 ? settings.crack_cost : 0.0f,
            points[edge.second].potential > 0 ? settings.crack_cost : 0.0f};
        if (edge.propagation[0] > 0 || edge.propagation[1] > 0)
            active.push_back(static_cast<std::uint32_t>(i));
    }
    const auto negative = [](float value) {
        return std::bit_cast<float>(std::bit_cast<std::uint32_t>(value) ^ 0x80000000u);
    };
    const auto length = [](float x, float y) {
        const double dx = x, dy = y;
        return static_cast<float>(std::sqrt(dx * dx + dy * dy));
    };
    const float invalid_share = std::bit_cast<float>(0xffc00000u);
    const auto distribute = [&](std::uint32_t point_index,
                                std::array<float, 2> direction,
                                float incoming_length, float potential) {
        if (adjacency.ranges.empty()) return;
        const auto [first, count] = adjacency.ranges[point_index];
        if (first > adjacency.edge_indices.size() ||
            count > adjacency.edge_indices.size() - first)
            throw std::invalid_argument("Invalid ice source adjacency range");
        std::vector<float> shares(count, 0.0f);
        float sum = 0.0f;
        for (std::uint32_t i = 0; i < count; ++i) {
            const auto edge_index = adjacency.edge_indices[first + i];
            if (edge_index >= edges.size())
                throw std::invalid_argument("Invalid ice source adjacency edge");
            const auto &edge = edges[edge_index];
            if (edge.first != point_index && edge.second != point_index)
                throw std::invalid_argument("Nonincident ice source edge");
            if (1.0f <= edge.weights[0] + edge.weights[1]) continue;
            const auto other = edge.first == point_index ? edge.second : edge.first;
            const float dx = points[other].xy[0] + negative(points[point_index].xy[0]);
            const float dy = points[other].xy[1] + negative(points[point_index].xy[1]);
            const float dot = (dy * direction[1]) + (dx * direction[0]);
            if (!(dot > 0.0f)) continue;
            const double nx = dx, ny = dy;
            const double denominator = std::sqrt(nx * nx + ny * ny) *
                                       static_cast<double>(incoming_length);
            float cosine = static_cast<float>(static_cast<double>(dot) / denominator);
            if (!(cosine > 0.0f)) cosine = 0.0f;
            else if (cosine > 1.0f) cosine = 1.0f;
            shares[i] = cosine * potential;
            sum += cosine;
        }
        const float reciprocal = 1.0f / sum;
        for (std::uint32_t i = 0; i < count; ++i) {
            auto &edge = edges[adjacency.edge_indices[first + i]];
            const unsigned slot = edge.first == point_index ? 0 : 1;
            const float contribution = sum == 0.0f ? invalid_share :
                                       shares[i] * reciprocal;
            edge.pending[slot] += contribution;
        }
    };
    constexpr float epsilon = std::bit_cast<float>(0x34000000u);
    for (std::size_t pass = 0; !active.empty(); ++pass) {
        if (pass > edges.size())
            throw std::runtime_error("Ice source-weight propagation did not finish");
        for (const auto index : active) {
            auto &edge = edges[index];
            const float dx = points[edge.second].xy[0] +
                             negative(points[edge.first].xy[0]);
            const float dy = points[edge.second].xy[1] +
                             negative(points[edge.first].xy[1]);
            const float span = length(dx, dy);
            const float remaining = 1.0f - (edge.weights[0] + edge.weights[1]);
            const float cost = (remaining * settings.initial_potential) * span;
            if (epsilon <= cost) {
                const float ratio = 1.0f / cost;
                edge.weights[0] += (ratio * edge.propagation[0]) * remaining;
                edge.weights[1] += (ratio * edge.propagation[1]) * remaining;
            } else {
                edge.weights[0] = edge.propagation[0] > 0.0f ? 1.0f : 0.0f;
                edge.weights[1] = edge.propagation[1] > 0.0f ? 1.0f : 0.0f;
            }
            if (edge.weights[0] >= 1.0f && edge.weights[1] == 0.0f &&
                edge.propagation[0] > cost) {
                points[edge.second].active = 1;
                distribute(edge.second, {dx, dy}, span,
                           edge.propagation[0] - cost);
            } else if (edge.weights[1] >= 1.0f && edge.weights[0] == 0.0f &&
                       edge.propagation[1] > cost) {
                points[edge.first].active = 1;
                distribute(edge.first, {negative(dx), negative(dy)}, span,
                           edge.propagation[1] - cost);
            }
            edge.propagation = {0.0f, 0.0f};
        }
        active.clear();
        for (std::size_t i = 0; i < edges.size(); ++i) {
            auto &edge = edges[i];
            if (edge.pending[0] + edge.pending[1] > 0.0f) {
                edge.propagation = edge.pending;
                edge.pending = {0.0f, 0.0f};
                active.push_back(static_cast<std::uint32_t>(i));
            }
        }
    }
    const float min_clip = std::max(0.05f, settings.min_clip * settings.source_scale);
    const float max_clip = std::min(0.99f, settings.max_clip);
    for (auto &edge : edges) {
        if ((edge.weights[0] > 0.0f || edge.weights[1] > 0.0f) &&
            max_clip > 0.0f) {
            edge.weights = {1.0f, 1.0f};
            points[edge.first].active = 1;
            points[edge.second].active = 1;
            continue;
        }
        const float dx = points[edge.second].xy[0] +
                         negative(points[edge.first].xy[0]);
        const float dy = points[edge.second].xy[1] +
                         negative(points[edge.first].xy[1]);
        const float span = length(dx, dy);
        for (auto &weight : edge.weights)
            if (weight < 1.0f && weight * span < min_clip) weight = 0.0f;
    }
}
TerrainIceIntermediateRegionCounts count_terrain_ice_intermediate_regions(
    std::span<const std::uint8_t> source24_flags,
    std::span<const std::array<float, 2>> source36_weights) {
    constexpr auto limit = std::numeric_limits<std::uint32_t>::max();
    if (source24_flags.size() > limit || source36_weights.size() > limit)
        throw std::invalid_argument("Ice intermediate region input is too large");
    TerrainIceIntermediateRegionCounts result;
    result.source24_to_active.resize(source24_flags.size(), limit);
    for (std::size_t index = 0; index < source24_flags.size(); ++index)
        if (source24_flags[index] != 0)
            result.source24_to_active[index] = result.active_source24++;
    std::uint64_t records20 = result.active_source24, links12 = 0;
    for (const auto &weights : source36_weights) {
        const float sum = weights[0] + weights[1];
        if (!(1.0f > sum)) {
            ++links12;
            continue;
        }
        if (weights[0] > 0.0f) { ++records20; ++links12; }
        if (weights[1] > 0.0f) { ++records20; ++links12; }
    }
    if (records20 > limit || links12 > limit)
        throw std::invalid_argument("Ice intermediate region count overflow");
    result.records20 = static_cast<std::uint32_t>(records20);
    result.links12 = static_cast<std::uint32_t>(links12);
    return result;
}
TerrainIceIntermediateFill build_terrain_ice_intermediate_regions(
    std::span<const TerrainIceIntermediateSourcePoint> points,
    std::span<const TerrainIceIntermediateSourceEdge> edges) {
    std::vector<std::uint8_t> flags;
    flags.reserve(points.size());
    for (const auto &point : points) flags.push_back(point.active);
    std::vector<std::array<float, 2>> weights;
    weights.reserve(edges.size());
    for (const auto &edge : edges) weights.push_back(edge.weights);
    auto plan = count_terrain_ice_intermediate_regions(flags, weights);
    TerrainIceIntermediateFill result;
    result.source24_to_active = std::move(plan.source24_to_active);
    result.points.reserve(plan.records20);
    result.links.reserve(plan.links12);
    for (const auto &point : points)
        if (point.active != 0) result.points.push_back({point.xy, 0, 0});
    const auto active_index = [&](std::uint32_t source) {
        if (source >= result.source24_to_active.size() ||
            result.source24_to_active[source] == std::numeric_limits<std::uint32_t>::max())
            throw std::invalid_argument("Ice intermediate edge references an inactive point");
        return result.source24_to_active[source];
    };
    const auto interpolate = [](float first, float second, float weight) {
        const auto negative_first = std::bit_cast<float>(
            std::bit_cast<std::uint32_t>(first) ^ 0x80000000u);
        const float difference = second + negative_first;
        const float weighted = difference * weight;
        return first + weighted;
    };
    const auto add_split = [&](std::uint32_t source, std::uint32_t other,
                               float weight, std::uint8_t boundary) {
        const auto from = active_index(source);
        active_index(other);
        const auto &a = points[source].xy, &b = points[other].xy;
        const std::array<float, 2> xy{
            interpolate(a[0], b[0], weight),
            interpolate(a[1], b[1], weight)};
        const auto new_index = static_cast<std::uint32_t>(result.points.size());
        result.points.push_back({xy, 0, 0});
        result.links.push_back({from, new_index, boundary});
    };
    for (const auto &edge : edges) {
        const float sum = edge.weights[0] + edge.weights[1];
        if (!(1.0f > sum)) {
            result.links.push_back({active_index(edge.first),
                                    active_index(edge.second), edge.boundary});
            continue;
        }
        if (edge.weights[0] > 0.0f)
            add_split(edge.first, edge.second, edge.weights[0], edge.boundary);
        if (edge.weights[1] > 0.0f)
            add_split(edge.second, edge.first, edge.weights[1], edge.boundary);
    }
    if (result.points.size() != plan.records20 || result.links.size() != plan.links12)
        throw std::runtime_error("Ice intermediate fill disagrees with allocation prepass");
    return result;
}
void build_terrain_ice_intermediate_adjacency(TerrainIceIntermediateFill &fill) {
    constexpr auto limit = std::numeric_limits<std::uint32_t>::max();
    if (fill.points.size() > limit || fill.links.size() > limit / 2 ||
        !fill.adjacency_link_indices.empty())
        throw std::invalid_argument("Invalid ice intermediate adjacency extent or prior state");
    for (const auto &point : fill.points)
        if (point.adjacency_first != 0 || point.link_count != 0)
            throw std::invalid_argument("Ice intermediate adjacency is already built");
    for (const auto &link : fill.links) {
        if (link.first >= fill.points.size() || link.second >= fill.points.size())
            throw std::invalid_argument("Ice intermediate link endpoint is outside points");
        ++fill.points[link.first].link_count;
        ++fill.points[link.second].link_count;
    }
    std::vector<std::uint32_t> cursor(fill.points.size());
    std::uint32_t total = 0;
    for (std::size_t i = 0; i < fill.points.size(); ++i) {
        auto &point = fill.points[i];
        point.adjacency_first = total;
        total += point.link_count;
        cursor[i] = total;
    }
    fill.adjacency_link_indices.resize(total);
    for (std::uint32_t i = 0; i < fill.links.size(); ++i) {
        const auto &link = fill.links[i];
        fill.adjacency_link_indices[--cursor[link.first]] = i;
        fill.adjacency_link_indices[--cursor[link.second]] = i;
    }
}
void sort_terrain_ice_intermediate_adjacency(TerrainIceIntermediateFill &fill) {
    if (fill.links.size() > std::numeric_limits<std::uint32_t>::max() / 2 ||
        fill.adjacency_link_indices.size() != 2 * fill.links.size())
        throw std::invalid_argument("Invalid ice intermediate adjacency extent");
    for (std::size_t point_index = 0; point_index < fill.points.size(); ++point_index) {
        const auto &center = fill.points[point_index];
        const auto first = static_cast<std::size_t>(center.adjacency_first);
        const auto degree = static_cast<std::size_t>(center.link_count);
        if (first > fill.adjacency_link_indices.size() ||
            degree > fill.adjacency_link_indices.size() - first)
            throw std::invalid_argument("Ice intermediate adjacency range is outside links");
        const auto neighbor = [&](std::size_t position) -> const TerrainIceIntermediatePoint & {
            const auto link_index = fill.adjacency_link_indices[first + position];
            if (link_index >= fill.links.size())
                throw std::invalid_argument("Invalid ice intermediate adjacency link index");
            const auto &link = fill.links[link_index];
            const auto other = link.first == point_index ? link.second : link.first;
            if ((link.first != point_index && link.second != point_index) ||
                other >= fill.points.size())
                throw std::invalid_argument("Ice intermediate adjacency is not incident");
            return fill.points[other];
        };
        const auto score = [&](std::size_t position) {
            const auto &other = neighbor(position);
            const float dx = other.xy[0] - center.xy[0];
            const float dy = other.xy[1] - center.xy[1];
            const double numerator = static_cast<double>(other.xy[0]) - center.xy[0];
            return numerator / std::sqrt(static_cast<double>(dx) * dx +
                                         static_cast<double>(dy) * dy);
        };
        std::size_t upper = 0, lower = degree;
        while (upper < lower) {
            if (neighbor(upper).xy[1] > center.xy[1]) {
                ++upper;
            } else {
                --lower;
                if (upper != lower)
                    std::swap(fill.adjacency_link_indices[first + upper],
                              fill.adjacency_link_indices[first + lower]);
            }
        }
        const auto selection = [&](std::size_t begin, std::size_t end, bool ascending) {
            for (std::size_t i = begin; i < end; ++i) {
                std::size_t selected = i;
                auto best = score(i);
                for (std::size_t j = i + 1; j < end; ++j) {
                    const auto candidate = score(j);
                    if (ascending ? candidate < best : candidate > best) {
                        best = candidate;
                        selected = j;
                    }
                }
                if (selected != i)
                    std::swap(fill.adjacency_link_indices[first + i],
                              fill.adjacency_link_indices[first + selected]);
            }
        };
        selection(0, upper, true);
        selection(upper, degree, false);
    }
}
void mark_terrain_ice_closed_walks(TerrainIceIntermediateFill &fill) {
    if (fill.links.size() > std::numeric_limits<std::uint32_t>::max() / 2 ||
        fill.adjacency_link_indices.size() != 2 * fill.links.size() ||
        !fill.closed_walk_flags.empty())
        throw std::invalid_argument("Invalid ice closed-walk extent or prior state");
    const auto total_slots = fill.adjacency_link_indices.size();
    const auto range = [&](std::size_t point_index) {
        if (point_index >= fill.points.size())
            throw std::invalid_argument("Ice closed walk left its point table");
        const auto &point = fill.points[point_index];
        const auto first = static_cast<std::size_t>(point.adjacency_first);
        const auto degree = static_cast<std::size_t>(point.link_count);
        if (first > total_slots || degree > total_slots - first)
            throw std::invalid_argument("Ice closed-walk adjacency range is invalid");
        return std::pair{first, degree};
    };
    const auto incident = [&](std::size_t point, std::size_t slot) {
        const auto [first, degree] = range(point);
        if (slot >= degree)
            throw std::invalid_argument("Ice closed-walk slot is invalid");
        const auto link_index = fill.adjacency_link_indices[first + slot];
        if (link_index >= fill.links.size())
            throw std::invalid_argument("Ice closed-walk link is invalid");
        const auto &link = fill.links[link_index];
        if (link.first != point && link.second != point)
            throw std::invalid_argument("Ice closed-walk link is not incident");
        return std::pair{first + slot, link_index};
    };
    fill.closed_walk_flags.assign(total_slots, 0);
    std::vector<std::size_t> path;
    for (std::size_t start_point = 0; start_point < fill.points.size(); ++start_point) {
        const auto degree = range(start_point).second;
        for (std::size_t start_slot = 0; start_slot < degree; ++start_slot) {
            const auto [start_offset, start_link] = incident(start_point, start_slot);
            if (fill.links[start_link].boundary == 0 ||
                fill.closed_walk_flags[start_offset] != 0)
                continue;
            path.clear();
            std::size_t point = start_point, slot = start_slot;
            for (;;) {
                const auto [offset, link_index] = incident(point, slot);
                path.push_back(offset);
                if (path.size() > total_slots || path.size() > fill.points.size())
                    throw std::invalid_argument("Ice closed walk exceeds original scratch capacity");
                const auto &link = fill.links[link_index];
                const auto next_point = static_cast<std::size_t>(
                    link.first == point ? link.second : link.first);
                const auto [next_first, next_degree] = range(next_point);
                if (next_degree == 0)
                    throw std::invalid_argument("Ice closed walk reached an isolated point");
                std::size_t reverse_slot = 0;
                for (; reverse_slot < next_degree; ++reverse_slot) {
                    const auto other_link = fill.adjacency_link_indices[next_first + reverse_slot];
                    if (other_link >= fill.links.size())
                        throw std::invalid_argument("Ice closed-walk reverse link is invalid");
                    const auto &candidate = fill.links[other_link];
                    if ((candidate.first == point && candidate.second == next_point) ||
                        (candidate.second == point && candidate.first == next_point))
                        break;
                }
                if (reverse_slot == next_degree)
                    throw std::invalid_argument("Ice closed walk has no reverse link");
                point = next_point;
                slot = (reverse_slot + 1) % next_degree;
                const auto next_link = incident(point, slot).second;
                if (fill.links[next_link].boundary == 0 ||
                    (point == start_point && slot == start_slot)) {
                    if (point == start_point && slot == start_slot)
                        for (const auto visited : path) fill.closed_walk_flags[visited] = 1;
                    break;
                }
            }
        }
    }
}
std::vector<TerrainIceIntermediateRegion> trace_terrain_ice_intermediate_regions(
    TerrainIceIntermediateFill &fill) {
    if (fill.points.size() > std::numeric_limits<std::uint32_t>::max() ||
        fill.links.size() > std::numeric_limits<std::uint32_t>::max() / 2 ||
        fill.adjacency_link_indices.size() != 2 * fill.links.size() ||
        fill.closed_walk_flags.size() != fill.adjacency_link_indices.size())
        throw std::invalid_argument("Invalid ice region-walk extent or prior state");
    const auto total_slots = fill.adjacency_link_indices.size();
    const auto range = [&](std::size_t point_index) {
        if (point_index >= fill.points.size())
            throw std::invalid_argument("Ice region walk left its point table");
        const auto &point = fill.points[point_index];
        const auto first = static_cast<std::size_t>(point.adjacency_first);
        const auto degree = static_cast<std::size_t>(point.link_count);
        if (first > total_slots || degree > total_slots - first)
            throw std::invalid_argument("Ice region-walk adjacency range is invalid");
        return std::pair{first, degree};
    };
    const auto incident = [&](std::size_t point, std::size_t slot) {
        const auto [first, degree] = range(point);
        if (slot >= degree)
            throw std::invalid_argument("Ice region-walk slot is invalid");
        const auto link_index = fill.adjacency_link_indices[first + slot];
        if (link_index >= fill.links.size())
            throw std::invalid_argument("Ice region-walk link is invalid");
        const auto &link = fill.links[link_index];
        if (link.first != point && link.second != point)
            throw std::invalid_argument("Ice region-walk link is not incident");
        return std::pair{first + slot, link_index};
    };
    std::vector<TerrainIceIntermediateRegion> regions;
    for (std::size_t start_point = 0; start_point < fill.points.size(); ++start_point) {
        const auto degree = range(start_point).second;
        for (std::size_t start_slot = 0; start_slot < degree; ++start_slot) {
            const auto start_offset = incident(start_point, start_slot).first;
            if (fill.closed_walk_flags[start_offset] != 0)
                continue;
            auto &region = regions.emplace_back();
            std::size_t point = start_point, slot = start_slot;
            for (;;) {
                const auto [offset, link_index] = incident(point, slot);
                fill.closed_walk_flags[offset] = 1;
                const auto &link = fill.links[link_index];
                const auto next_point = static_cast<std::size_t>(
                    link.first == point ? link.second : link.first);
                region.directed_edges.push_back({static_cast<std::uint32_t>(point),
                                                 static_cast<std::uint32_t>(next_point)});
                if (region.directed_edges.size() > total_slots)
                    throw std::invalid_argument("Ice region walk did not return to start");
                const auto [next_first, next_degree] = range(next_point);
                if (next_degree == 0)
                    throw std::invalid_argument("Ice region walk reached an isolated point");
                std::size_t reverse_slot = 0;
                for (; reverse_slot < next_degree; ++reverse_slot) {
                    const auto other_index = fill.adjacency_link_indices[next_first + reverse_slot];
                    if (other_index >= fill.links.size())
                        throw std::invalid_argument("Ice region reverse link is invalid");
                    const auto &candidate = fill.links[other_index];
                    if ((candidate.first == point && candidate.second == next_point) ||
                        (candidate.second == point && candidate.first == next_point))
                        break;
                }
                if (reverse_slot == next_degree)
                    throw std::invalid_argument("Ice region walk has no reverse link");
                point = next_point;
                slot = (reverse_slot + 1) % next_degree;
                if (point == start_point && slot == start_slot)
                    break;
            }
        }
    }
    return regions;
}
TerrainIceIntermediateRegionGeometry build_terrain_ice_intermediate_region_geometry(
    const TerrainIceIntermediateFill &fill,
    std::span<const TerrainIceIntermediateRegion> regions,
    float edge_pull, std::uint32_t x87_precision) {
    if (x87_precision != 24 && x87_precision != 53)
        throw std::invalid_argument("Invalid ice region geometry x87 precision");
    constexpr auto limit = std::numeric_limits<std::uint32_t>::max();
    if (regions.size() > limit)
        throw std::invalid_argument("Invalid ice region geometry extent");
    const auto negate = [](float value) {
        return std::bit_cast<float>(std::bit_cast<std::uint32_t>(value) ^ 0x80000000u);
    };
    const auto length = [&](float x, float y) {
        if (x87_precision == 24) {
            const float squared = x * x + y * y;
            return std::sqrt(squared);
        }
        return static_cast<float>(std::sqrt(static_cast<double>(x) * x +
                                            static_cast<double>(y) * y));
    };
    const auto pull_corner = [&](std::array<float, 2> &center,
                                 const std::array<float, 2> &previous,
                                 const std::array<float, 2> &following) {
        const std::array<float, 2> a{previous[0] + negate(center[0]),
                                     previous[1] + negate(center[1])};
        const std::array<float, 2> b{following[0] + negate(center[0]),
                                     following[1] + negate(center[1])};
        const float la = length(a[0], a[1]), lb = length(b[0], b[1]);
        const float ia = 1.0f / la, ib = 1.0f / lb;
        const float ax = ia * a[0], ay = ia * a[1];
        const float bx = ib * b[0], by = ib * b[1];
        const float dx = bx + negate(ax), dy = ay + negate(by);
        const float inverse = 1.0f / length(dx, dy);
        float nx = inverse * dy, ny = inverse * dx;
        const float dot = ay * by + bx * ax;
        if (dot > std::bit_cast<float>(0x3f7ff972u)) {
            nx = negate(ax);
            ny = negate(ay);
        }
        const float radius = la <= lb ? la : lb;
        center[0] += (nx * edge_pull) * radius;
        center[1] += (ny * edge_pull) * radius;
    };
    TerrainIceIntermediateRegionGeometry result;
    result.regions.reserve(regions.size());
    for (std::size_t region_index = 0; region_index < regions.size(); ++region_index) {
        const auto &walk = regions[region_index].directed_edges;
        if (walk.empty() || walk.size() > limit ||
            result.vertices.size() > limit - walk.size() ||
            result.indices.size() > limit - walk.size())
            throw std::invalid_argument("Invalid ice region geometry walk extent");
        auto &record = result.regions.emplace_back();
        record.first_vertex = static_cast<std::uint32_t>(result.vertices.size());
        record.first_index = static_cast<std::uint32_t>(result.indices.size());
        record.vertex_count = static_cast<std::uint32_t>(walk.size());
        for (std::size_t i = 0; i < walk.size(); ++i) {
            const auto [source, target] = walk[i];
            if (source >= fill.points.size() || target >= fill.points.size() ||
                (i > 0 && walk[i - 1][1] != source))
                throw std::invalid_argument("Invalid ice region geometry edge sequence");
            result.vertices.push_back(fill.points[source].xy);
            const auto &point = fill.points[source];
            const auto first = static_cast<std::size_t>(point.adjacency_first);
            const auto degree = static_cast<std::size_t>(point.link_count);
            if (first > fill.adjacency_link_indices.size() ||
                degree > fill.adjacency_link_indices.size() - first)
                throw std::invalid_argument("Invalid ice region geometry point adjacency");
            for (std::size_t slot = 0; slot < degree; ++slot) {
                const auto link_index = fill.adjacency_link_indices[first + slot];
                if (link_index >= fill.links.size())
                    throw std::invalid_argument("Invalid ice region geometry point link");
                record.boundary |= fill.links[link_index].boundary != 0;
            }
            if (i == 0) continue;
            const auto index = static_cast<std::uint32_t>(result.vertices.size() - 1);
            result.indices.push_back({index - 1, index,
                                      static_cast<std::uint32_t>(region_index)});
            if (degree > 1) {
                auto &vertex = result.vertices.back();
                pull_corner(vertex, result.vertices[result.vertices.size() - 2],
                            fill.points[target].xy);
            }
        }
        if (walk.back()[1] != walk.front()[0])
            throw std::invalid_argument("Ice region geometry walk is not closed");
        const auto first = record.first_vertex;
        const auto last = static_cast<std::uint32_t>(result.vertices.size() - 1);
        result.indices.push_back({last, first, static_cast<std::uint32_t>(region_index)});
        if (fill.points[walk.front()[0]].link_count > 1 && walk.size() > 1)
            pull_corner(result.vertices[first], result.vertices[last],
                        result.vertices[first + 1]);
    }
    result.incidence.resize(result.vertices.size());
    for (std::size_t i = 0; i < result.indices.size(); ++i) {
        const auto &edge = result.indices[i];
        result.incidence[edge[0]].push_back(static_cast<std::uint32_t>(i));
        result.incidence[edge[1]].push_back(static_cast<std::uint32_t>(i));
    }
    return result;
}
std::array<float, 4> terrain_ice_region_component_box(
    const TerrainIceIntermediateRegionGeometry &geometry) {
    if (geometry.vertices.empty())
        throw std::invalid_argument("Empty ice region component bounds");
    std::array<float, 4> box{
        std::numeric_limits<float>::infinity(),
        std::numeric_limits<float>::infinity(),
        -std::numeric_limits<float>::infinity(),
        -std::numeric_limits<float>::infinity()};
    for (const auto &point : geometry.vertices) {
        if (!std::isfinite(point[0]) || !std::isfinite(point[1]))
            throw std::invalid_argument("Nonfinite ice region component vertex");
        box[0] = std::min(box[0], point[0]);
        box[1] = std::min(box[1], point[1]);
        box[2] = std::max(box[2], point[0]);
        box[3] = std::max(box[3], point[1]);
    }
    return box;
}
void shrink_terrain_ice_intermediate_region(
    TerrainIceIntermediateRegionGeometry &geometry, std::size_t region_index,
    float shrink_amount, std::uint32_t x87_precision) {
    if (x87_precision != 24 && x87_precision != 53)
        throw std::invalid_argument("Invalid ice region shrink x87 precision");
    if (region_index >= geometry.regions.size())
        throw std::invalid_argument("Invalid ice region shrink record");
    const auto &region = geometry.regions[region_index];
    const std::size_t first = region.first_index, count = region.vertex_count;
    if (count == 0 || count > 16 || first > geometry.indices.size() ||
        count > geometry.indices.size() - first)
        throw std::invalid_argument("Invalid ice region shrink index extent");
    float sum_x = 0.0f, sum_y = 0.0f;
    for (std::size_t i = 0; i < count; ++i) {
        const auto vertex_index = geometry.indices[first + i][0];
        if (vertex_index >= geometry.vertices.size())
            throw std::invalid_argument("Invalid ice region shrink vertex index");
        const auto &vertex = geometry.vertices[vertex_index];
        sum_x += vertex[0];
        sum_y += vertex[1];
    }
    const float center_x = x87_precision == 24
        ? (1.0f / static_cast<float>(count)) * sum_x
        : static_cast<float>((1.0 / static_cast<double>(count)) *
                             static_cast<double>(sum_x));
    const float center_y = x87_precision == 24
        ? (1.0f / static_cast<float>(count)) * sum_y
        : static_cast<float>((1.0 / static_cast<double>(count)) *
                             static_cast<double>(sum_y));
    const auto negate = [](float value) {
        return std::bit_cast<float>(std::bit_cast<std::uint32_t>(value) ^ 0x80000000u);
    };
    const float scale = 1.0f - shrink_amount;
    for (std::size_t i = 0; i < count; ++i) {
        auto &vertex = geometry.vertices[geometry.indices[first + i][0]];
        const float x = vertex[0] + negate(center_x);
        const float y = vertex[1] + negate(center_y);
        vertex[0] = x * scale + center_x;
        vertex[1] = y * scale + center_y;
    }
}
void prune_terrain_ice_intermediate_region(
    TerrainIceIntermediateRegionGeometry &geometry, std::size_t region_index) {
    if (region_index >= geometry.regions.size())
        throw std::invalid_argument("Invalid ice region prune record");
    auto &region = geometry.regions[region_index];
    const std::size_t first = region.first_index, count = region.vertex_count;
    if (count == 0 || first > geometry.indices.size() ||
        count > geometry.indices.size() - first ||
        region.first_vertex >= geometry.vertices.size())
        throw std::invalid_argument("Invalid ice region prune extent");
    auto next_vertex = region.first_vertex;
    for (std::size_t i = 0; i < count; ++i) {
        const auto &edge = geometry.indices[first + i];
        if (edge[0] != next_vertex || edge[1] >= geometry.vertices.size() ||
            edge[2] != region_index)
            throw std::invalid_argument("Invalid ice region prune chain");
        next_vertex = edge[1];
    }
    if (next_vertex != region.first_vertex)
        throw std::invalid_argument("Ice region prune chain is not closed");
    if (geometry.incidence.empty()) {
        geometry.incidence.resize(geometry.vertices.size());
        for (std::size_t i = 0; i < geometry.indices.size(); ++i) {
            const auto &edge = geometry.indices[i];
            if (edge[0] == std::numeric_limits<std::uint32_t>::max())
                continue;
            if (edge[0] >= geometry.vertices.size() ||
                edge[1] >= geometry.vertices.size())
                throw std::invalid_argument("Invalid ice region graph edge");
            geometry.incidence[edge[0]].push_back(static_cast<std::uint32_t>(i));
            geometry.incidence[edge[1]].push_back(static_cast<std::uint32_t>(i));
        }
    }
    if (geometry.incidence.size() != geometry.vertices.size())
        throw std::invalid_argument("Invalid ice region graph incidence");
    constexpr auto deleted = std::numeric_limits<std::uint32_t>::max();
    for (std::size_t i = 0; i < count; ++i) {
        const auto edge_index = static_cast<std::uint32_t>(first + i);
        const auto edge = geometry.indices[edge_index];
        for (const auto endpoint : {edge[0], edge[1]}) {
            auto &list = geometry.incidence[endpoint];
            const auto it = std::find(list.begin(), list.end(), edge_index);
            if (it == list.end())
                throw std::invalid_argument("Invalid ice region graph endpoint");
            list.erase(it);
        }
        geometry.indices[edge_index] = {deleted, deleted, deleted};
    }
    region.vertex_count = 0;
}
void process_terrain_ice_intermediate_regions(
    TerrainIceIntermediateRegionGeometry &geometry, TerrainIceRandomState &random,
    float boundary_kill_rate, float interior_kill_rate, float shrink_amount,
    std::uint32_t x87_precision) {
    for (std::size_t i = 0; i < geometry.regions.size(); ++i) {
        const auto &region = geometry.regions[i];
        if (region.vertex_count > 16)
            continue;
        const bool prune = should_prune_terrain_ice_region_record(
            random, region.vertex_count, region.boundary != 0,
            boundary_kill_rate, interior_kill_rate, x87_precision);
        if (prune)
            prune_terrain_ice_intermediate_region(geometry, i);
        else
            shrink_terrain_ice_intermediate_region(geometry, i,
                                                   shrink_amount, x87_precision);
    }
}
TerrainIceIntermediateRegionGeometry finalize_terrain_ice_intermediate_regions(
    const TerrainIceIntermediateRegionGeometry &geometry) {
    constexpr auto deleted = std::numeric_limits<std::uint32_t>::max();
    if (geometry.regions.size() > deleted || geometry.vertices.size() > deleted ||
        geometry.indices.size() > deleted / 2)
        throw std::invalid_argument("Invalid ice region finalization extent");
    TerrainIceIntermediateRegionGeometry result;
    result.regions = geometry.regions;
    result.indices = geometry.indices;
    std::size_t cursor = 0;
    while (cursor < result.indices.size()) {
        const auto &edge = result.indices[cursor];
        if (edge[0] == deleted) {
            if (edge[1] != deleted || edge[2] != deleted)
                throw std::invalid_argument("Partial deleted ice region edge");
            result.indices[cursor] = result.indices.back();
            result.indices.pop_back();
        } else {
            if (edge[0] >= geometry.vertices.size() ||
                edge[1] >= geometry.vertices.size() ||
                edge[0] == edge[1] || edge[2] >= geometry.regions.size())
                throw std::invalid_argument("Invalid active ice region edge");
            ++cursor;
        }
    }
    std::vector<std::uint8_t> used(geometry.vertices.size());
    for (const auto &edge : result.indices) {
        used[edge[0]] = 1;
        used[edge[1]] = 1;
    }
    std::vector<std::uint32_t> mapped(geometry.vertices.size(), deleted);
    result.vertices.reserve(geometry.vertices.size());
    for (std::size_t index = 0; index < geometry.vertices.size(); ++index) {
        if (!used[index]) continue;
        mapped[index] = static_cast<std::uint32_t>(result.vertices.size());
        result.vertices.push_back(geometry.vertices[index]);
    }
    TerrainIceIntermediateFill fill;
    fill.points.reserve(result.vertices.size());
    fill.links.reserve(result.indices.size());
    for (const auto &point : result.vertices)
        fill.points.push_back({point, 0, 0});
    for (auto &edge : result.indices) {
        edge[0] = mapped[edge[0]];
        edge[1] = mapped[edge[1]];
        fill.links.push_back({edge[0], edge[1], 0});
    }
    build_terrain_ice_intermediate_adjacency(fill);
    sort_terrain_ice_intermediate_adjacency(fill);
    result.incidence.resize(result.vertices.size());
    for (std::size_t index = 0; index < result.vertices.size(); ++index) {
        const auto &point = fill.points[index];
        const auto first = point.adjacency_first;
        result.incidence[index].assign(
            fill.adjacency_link_indices.begin() + first,
            fill.adjacency_link_indices.begin() + first + point.link_count);
    }
    return result;
}
void recount_terrain_ice_region_faces(
    TerrainIceIntermediateRegionGeometry &geometry, std::uint32_t face_count) {
    if (face_count < geometry.regions.size() ||
        face_count - geometry.regions.size() > geometry.indices.size() ||
        geometry.indices.size() > std::numeric_limits<std::uint32_t>::max())
        throw std::invalid_argument("Invalid ice face recount extent");
    for (const auto &edge : geometry.indices)
        if (edge[0] >= geometry.vertices.size() ||
            edge[1] >= geometry.vertices.size() ||
            edge[2] >= face_count)
            throw std::invalid_argument("Invalid ice face recount edge");
    geometry.regions.resize(face_count);
    for (auto &face : geometry.regions) face.vertex_count = 0;
    for (std::uint32_t i = 0; i < geometry.indices.size(); ++i) {
        const auto &edge = geometry.indices[i];
        auto &face = geometry.regions[edge[2]];
        ++face.vertex_count;
        face.first_vertex = edge[0];
        face.first_index = i;
    }
}
std::vector<std::array<std::uint32_t, 2>> find_terrain_ice_edge_intersections(
    const TerrainIceIntermediateRegionGeometry &geometry) {
    if (geometry.indices.size() > std::numeric_limits<std::uint32_t>::max() ||
        geometry.vertices.size() > std::numeric_limits<std::uint32_t>::max())
        throw std::invalid_argument("Oversized ice crossing graph");
    constexpr double cell_size = 16.0;
    const auto cell = [](float coordinate) -> std::int32_t {
        if (!std::isfinite(coordinate))
            throw std::invalid_argument("Nonfinite ice crossing vertex");
        const double index = std::floor(static_cast<double>(coordinate) / cell_size);
        if (index < std::numeric_limits<std::int32_t>::min() ||
            index > std::numeric_limits<std::int32_t>::max())
            throw std::invalid_argument("Ice crossing coordinate is out of range");
        return static_cast<std::int32_t>(index);
    };
    const auto key = [](std::int32_t x, std::int32_t y) {
        return (std::uint64_t{static_cast<std::uint32_t>(x)} << 32) |
               static_cast<std::uint32_t>(y);
    };
    std::unordered_map<std::uint64_t, std::vector<std::uint32_t>> cells;
    for (std::uint32_t i = 0; i < geometry.indices.size(); ++i) {
        const auto &edge = geometry.indices[i];
        if (edge[0] >= geometry.vertices.size() ||
            edge[1] >= geometry.vertices.size() || edge[0] == edge[1] ||
            edge[2] >= geometry.regions.size())
            throw std::invalid_argument("Invalid ice crossing edge");
        const auto &a = geometry.vertices[edge[0]];
        const auto &b = geometry.vertices[edge[1]];
        const auto x0 = cell(std::min(a[0], b[0]));
        const auto x1 = cell(std::max(a[0], b[0]));
        const auto y0 = cell(std::min(a[1], b[1]));
        const auto y1 = cell(std::max(a[1], b[1]));
        const std::uint64_t width = std::int64_t{x1} - x0 + 1;
        const std::uint64_t height = std::int64_t{y1} - y0 + 1;
        if (width > 1'000'000 / height)
            throw std::invalid_argument("Oversized ice crossing edge bounds");
        for (std::int64_t x = x0; x <= x1; ++x)
            for (std::int64_t y = y0; y <= y1; ++y)
                cells[key(static_cast<std::int32_t>(x),
                          static_cast<std::int32_t>(y))].push_back(i);
    }
    std::vector<std::array<std::uint32_t, 2>> result;
    std::unordered_set<std::uint64_t> found;
    for (const auto &[_, members] : cells)
        for (std::size_t a_index = 0; a_index < members.size(); ++a_index) {
            const auto first = members[a_index];
            const auto &a = geometry.indices[first];
            const auto &pa = geometry.vertices[a[0]];
            const auto &pb = geometry.vertices[a[1]];
            const double ax = pa[0], ay = pa[1];
            const double bx = pb[0], by = pb[1];
            const double dx = bx - ax, dy = by - ay;
            for (std::size_t b_index = a_index + 1; b_index < members.size(); ++b_index) {
                const auto second = members[b_index];
                const auto low = std::min(first, second), high = std::max(first, second);
                const auto pair = (std::uint64_t{low} << 32) | high;
                if (found.contains(pair)) continue;
                const auto &b = geometry.indices[second];
                if (a[0] == b[0] || a[0] == b[1] ||
                    a[1] == b[0] || a[1] == b[1]) continue;
                const auto &pc = geometry.vertices[b[0]];
                const auto &pd = geometry.vertices[b[1]];
                const double cx = pc[0], cy = pc[1];
                const double ex = pd[0], ey = pd[1];
                if (std::max(ax, bx) < std::min(cx, ex) ||
                    std::max(cx, ex) < std::min(ax, bx) ||
                    std::max(ay, by) < std::min(cy, ey) ||
                    std::max(cy, ey) < std::min(ay, by)) continue;
                const double fx = ex - cx, fy = ey - cy;
                const double denominator = dx * fy - dy * fx;
                if (denominator == 0.0) continue;
                const double rx = cx - ax, ry = cy - ay;
                const double t = (rx * fy - ry * fx) / denominator;
                const double u = (rx * dy - ry * dx) / denominator;
                if (t > 0.0 && t < 1.0 && u > 0.0 && u < 1.0) {
                    found.insert(pair);
                    result.push_back({low, high});
                }
            }
        }
    std::sort(result.begin(), result.end());
    return result;
}
std::vector<TerrainIceDirectedIntersection> build_terrain_ice_directed_intersections(
    const TerrainIceIntermediateRegionGeometry &geometry) {
    const auto pairs = find_terrain_ice_edge_intersections(geometry);
    std::vector<TerrainIceDirectedIntersection> result;
    result.reserve(2 * pairs.size());
    const auto position = [&](std::uint32_t first, std::uint32_t second) {
        const auto &a = geometry.indices[first];
        const auto &b = geometry.indices[second];
        const auto &start = geometry.vertices[a[0]];
        const auto &end = geometry.vertices[a[1]];
        const auto &other_start = geometry.vertices[b[0]];
        const auto &other_end = geometry.vertices[b[1]];
        const float other_dx = other_end[0] - other_start[0];
        const float other_dy = other_end[1] - other_start[1];
        const float dx = end[0] - start[0];
        const float dy = end[1] - start[1];
        const float denominator = (dx * other_dy) - (other_dx * dy);
        const float inverse = 1.0f / denominator;
        const float offset_y = start[1] - other_start[1];
        const float offset_x = start[0] - other_start[0];
        const float numerator = (offset_y * other_dx) -
                                (offset_x * other_dy);
        return numerator * inverse;
    };
    for (const auto [first, second] : pairs) {
        result.push_back({first, second, position(first, second)});
        result.push_back({second, first, position(second, first)});
    }
    std::sort(result.begin(), result.end(),
              [](const auto &a, const auto &b) {
                  if (a.edge != b.edge) return a.edge < b.edge;
                  if (a.position != b.position) return a.position < b.position;
                  return a.other < b.other;
              });
    return result;
}
std::vector<TerrainIceCrossingVertex> build_terrain_ice_crossing_vertices(
    const TerrainIceIntermediateRegionGeometry &geometry,
    std::span<const TerrainIceDirectedIntersection> events) {
    std::vector<TerrainIceCrossingVertex> result;
    result.reserve(events.size() / 2);
    for (const auto &event : events) {
        if (event.edge >= geometry.indices.size() ||
            event.other >= geometry.indices.size() ||
            !std::isfinite(event.position) || event.position < 0.0f ||
            event.position > 1.0f)
            throw std::invalid_argument("Invalid ice crossing event");
        if (event.edge >= event.other) continue;
        const auto &edge = geometry.indices[event.edge];
        if (edge[0] >= geometry.vertices.size() ||
            edge[1] >= geometry.vertices.size())
            throw std::invalid_argument("Invalid ice crossing source edge");
        const auto &first = geometry.vertices[edge[0]];
        const auto &second = geometry.vertices[edge[1]];
        const float dx = second[0] - first[0];
        const float dy = second[1] - first[1];
        const float x = first[0] + dx * event.position;
        const float y = first[1] + dy * event.position;
        result.push_back({event.edge, event.other, {x, y}});
    }
    return result;
}
TerrainIceIntermediateRegionGeometry split_terrain_ice_intersecting_edges(
    const TerrainIceIntermediateRegionGeometry &geometry) {
    const auto events = build_terrain_ice_directed_intersections(geometry);
    if (events.empty()) return geometry;
    const auto crossings = build_terrain_ice_crossing_vertices(geometry, events);
    if (geometry.vertices.size() > UINT32_MAX - crossings.size() ||
        events.size() > (UINT32_MAX - geometry.indices.size()) / 2)
        throw std::invalid_argument("Oversized ice crossing split graph");
    TerrainIceIntermediateRegionGeometry result = geometry;
    result.vertices.reserve(geometry.vertices.size() + crossings.size());
    result.indices.reserve(geometry.indices.size() + events.size() * 2);
    result.incidence.clear();
    const auto key = [](std::uint32_t a, std::uint32_t b) {
        return (std::uint64_t{std::min(a, b)} << 32) | std::max(a, b);
    };
    std::unordered_map<std::uint64_t, std::uint32_t> vertex_for_pair;
    vertex_for_pair.reserve(crossings.size());
    for (const auto &crossing : crossings) {
        const auto inserted = vertex_for_pair.emplace(
            key(crossing.first_edge, crossing.second_edge),
            static_cast<std::uint32_t>(result.vertices.size()));
        if (!inserted.second)
            throw std::invalid_argument("Duplicate ice crossing vertex");
        result.vertices.push_back(crossing.xy);
    }
    constexpr auto deleted = std::numeric_limits<std::uint32_t>::max();
    for (std::size_t first = 0; first < events.size();) {
        const auto edge_id = events[first].edge;
        if (edge_id >= geometry.indices.size())
            throw std::invalid_argument("Invalid ice crossing edge ID");
        std::size_t last = first + 1;
        while (last < events.size() && events[last].edge == edge_id) ++last;
        const auto original = geometry.indices[edge_id];
        std::uint32_t previous = original[0];
        for (std::size_t i = first; i < last; ++i) {
            const auto it = vertex_for_pair.find(key(edge_id, events[i].other));
            if (it == vertex_for_pair.end() || it->second == previous)
                throw std::invalid_argument("Invalid ice crossing split vertex");
            result.indices.push_back({previous, it->second, original[2]});
            previous = it->second;
        }
        result.indices.push_back({previous, original[1], original[2]});
        result.indices[edge_id] = {deleted, deleted, deleted};
        first = last;
    }
    return result;
}
TerrainIceIntermediateRegionGeometry rebuild_terrain_ice_crossing_faces(
    const TerrainIceIntermediateRegionGeometry &geometry) {
    const auto events = build_terrain_ice_directed_intersections(geometry);
    if (events.empty()) return geometry;
    const auto split = split_terrain_ice_intersecting_edges(geometry);
    auto compact = finalize_terrain_ice_intermediate_regions(split);
    std::vector<std::uint32_t> source_edge(split.indices.size());
    for (std::uint32_t i = 0; i < geometry.indices.size(); ++i)
        source_edge[i] = i;
    std::size_t appended = geometry.indices.size();
    for (std::size_t first = 0; first < events.size();) {
        std::size_t last = first + 1;
        while (last < events.size() && events[last].edge == events[first].edge)
            ++last;
        for (std::size_t i = first; i <= last; ++i)
            source_edge[appended++] = events[first].edge;
        first = last;
    }
    if (appended != split.indices.size())
        throw std::invalid_argument("Invalid ice split edge lineage");
    std::vector<std::uint8_t> deleted_edge(split.indices.size());
    for (std::size_t i = 0; i < split.indices.size(); ++i)
        deleted_edge[i] = split.indices[i][0] == UINT32_MAX;
    for (std::size_t i = 0; i < source_edge.size();) {
        if (deleted_edge[i]) {
            source_edge[i] = source_edge.back();
            source_edge.pop_back();
            deleted_edge[i] = deleted_edge.back();
            deleted_edge.pop_back();
        } else {
            ++i;
        }
    }
    if (source_edge.size() != compact.indices.size() ||
        compact.vertices.size() != split.vertices.size() ||
        compact.incidence.size() != compact.vertices.size())
        throw std::invalid_argument("Invalid compact ice crossing graph");
    const auto old_face_count = geometry.regions.size();
    std::vector<std::uint8_t> visited(compact.indices.size());
    auto result = compact;
    for (std::size_t vertex = geometry.vertices.size();
         vertex < compact.vertices.size(); ++vertex) {
        for (const auto seed : compact.incidence[vertex]) {
            if (seed >= compact.indices.size())
                throw std::invalid_argument("Invalid ice crossing incidence");
            const auto old_face = compact.indices[seed][2];
            if (compact.indices[seed][0] != vertex || visited[seed]) continue;
            if (old_face >= old_face_count || result.regions.size() >= UINT32_MAX)
                throw std::invalid_argument("Invalid ice crossing source face");
            const auto new_face = static_cast<std::uint32_t>(result.regions.size());
            result.regions.emplace_back();
            auto current = seed;
            std::size_t steps = 0;
            do {
                if (current >= compact.indices.size() || visited[current] ||
                    compact.indices[current][2] != old_face ||
                    ++steps > compact.indices.size())
                    throw std::invalid_argument("Invalid ice crossing face walk");
                visited[current] = 1;
                result.indices[current][2] = new_face;
                const auto endpoint = compact.indices[current][1];
                if (endpoint >= compact.incidence.size())
                    throw std::invalid_argument("Invalid ice crossing endpoint");
                std::uint32_t next = UINT32_MAX;
                std::uint32_t straight = UINT32_MAX;
                unsigned outgoing = 0;
                for (const auto candidate : compact.incidence[endpoint]) {
                    if (candidate >= compact.indices.size())
                        throw std::invalid_argument("Invalid ice crossing adjacency");
                    const auto &edge = compact.indices[candidate];
                    if (edge[0] != endpoint || edge[2] != old_face) continue;
                    ++outgoing;
                    if (source_edge[candidate] == source_edge[current])
                        straight = candidate;
                    else
                        next = candidate;
                }
                if (outgoing == 1) next = straight == UINT32_MAX ? next : straight;
                if (outgoing == 0 || outgoing > 2 || next == UINT32_MAX ||
                    (outgoing == 2 && straight == UINT32_MAX))
                    throw std::invalid_argument("Ambiguous ice crossing turn");
                current = next;
            } while (current != seed);
        }
    }
    return result;
}
void prune_terrain_ice_crossing_faces(
    TerrainIceIntermediateRegionGeometry &geometry,
    std::uint32_t first_new_vertex, std::uint32_t first_new_face) {
    if (first_new_vertex > geometry.vertices.size() ||
        first_new_face > geometry.regions.size() ||
        geometry.vertices.size() > UINT32_MAX ||
        geometry.indices.size() > UINT32_MAX ||
        geometry.regions.size() > UINT32_MAX ||
        geometry.incidence.size() != geometry.vertices.size())
        throw std::invalid_argument("Invalid ice crossing prune extent");
    if (first_new_vertex == geometry.vertices.size()) return;
    const auto face_count = geometry.regions.size();
    std::vector<std::vector<std::uint32_t>> face_edges(face_count);
    for (std::uint32_t i = 0; i < geometry.indices.size(); ++i) {
        const auto &edge = geometry.indices[i];
        if (edge[0] >= geometry.vertices.size() ||
            edge[1] >= geometry.vertices.size() || edge[2] >= face_count)
            throw std::invalid_argument("Invalid ice crossing prune edge");
        if (edge[2] >= first_new_face) face_edges[edge[2]].push_back(i);
    }
    std::vector<std::array<std::uint32_t, 2>> paired_faces;
    for (std::uint32_t vertex = first_new_vertex;
         vertex < geometry.vertices.size(); ++vertex) {
        std::array<std::uint32_t, 2> pair{UINT32_MAX, UINT32_MAX};
        for (const auto edge_id : geometry.incidence[vertex]) {
            if (edge_id >= geometry.indices.size())
                throw std::invalid_argument("Invalid ice crossing prune incidence");
            const auto &edge = geometry.indices[edge_id];
            if (edge[0] != vertex && edge[1] != vertex)
                throw std::invalid_argument("Nonincident ice crossing prune edge");
            const auto face = edge[2];
            if (face < first_new_face)
                throw std::invalid_argument("Unrebuilt ice crossing face");
            if (face == pair[0] || face == pair[1]) continue;
            if (pair[0] == UINT32_MAX) pair[0] = face;
            else if (pair[1] == UINT32_MAX) pair[1] = face;
            else throw std::invalid_argument("Ambiguous ice crossing face pair");
        }
        if (pair[1] != UINT32_MAX) paired_faces.push_back(pair);
    }
    std::vector<std::uint8_t> live(face_count, 1);
    for (;;) {
        std::uint32_t smallest = UINT32_MAX;
        std::size_t smallest_size = std::numeric_limits<std::size_t>::max();
        for (const auto [a, b] : paired_faces) {
            if (!live[a] || !live[b]) continue;
            for (const auto face : {a, b}) {
                const auto size = face_edges[face].size();
                if (size == 0)
                    throw std::invalid_argument("Empty live ice crossing face");
                if (size < smallest_size ||
                    (size == smallest_size && face < smallest)) {
                    smallest = face;
                    smallest_size = size;
                }
            }
        }
        if (smallest == UINT32_MAX) break;
        live[smallest] = 0;
        for (const auto edge_id : face_edges[smallest])
            geometry.indices[edge_id] = {UINT32_MAX, UINT32_MAX, UINT32_MAX};
    }
    geometry.incidence.clear();
}
TerrainIceIntermediateRegionGeometry postprocess_terrain_ice_crossings(
    const TerrainIceIntermediateRegionGeometry &geometry) {
    if (geometry.vertices.size() > UINT32_MAX ||
        geometry.regions.size() > UINT32_MAX)
        throw std::invalid_argument("Oversized ice crossing postprocess graph");
    auto rebuilt = rebuild_terrain_ice_crossing_faces(geometry);
    prune_terrain_ice_crossing_faces(
        rebuilt, static_cast<std::uint32_t>(geometry.vertices.size()),
        static_cast<std::uint32_t>(geometry.regions.size()));
    auto result = finalize_terrain_ice_intermediate_regions(rebuilt);
    recount_terrain_ice_region_faces(
        result, static_cast<std::uint32_t>(rebuilt.regions.size()));
    return result;
}
TerrainIceLargeSourceGraph select_terrain_ice_large_source_graph(
    const TerrainIceIntermediateRegionGeometry &graph,
    std::uint32_t max_polygon_vertices) {
    if (max_polygon_vertices < 3 || graph.regions.size() > UINT32_MAX ||
        graph.vertices.size() > UINT32_MAX || graph.indices.size() > UINT32_MAX)
        throw std::invalid_argument("Invalid large ice source extent");
    TerrainIceLargeSourceGraph result;
    result.edges = graph.indices;
    for (const auto &point : graph.vertices)
        if (!std::isfinite(point[0]) || !std::isfinite(point[1]))
            throw std::invalid_argument("Nonfinite large ice source point");
    for (const auto &edge : graph.indices)
        if (edge[0] >= graph.vertices.size() ||
            edge[1] >= graph.vertices.size() ||
            edge[0] == edge[1] || edge[2] >= graph.regions.size())
            throw std::invalid_argument("Invalid large ice source edge");
    std::size_t cursor = 0;
    while (cursor < result.edges.size()) {
        const auto &edge = result.edges[cursor];
        if (graph.regions[edge[2]].vertex_count <= max_polygon_vertices) {
            result.edges[cursor] = result.edges.back();
            result.edges.pop_back();
        } else {
            ++cursor;
        }
    }
    std::vector<std::uint8_t> retained(graph.vertices.size());
    for (const auto &edge : result.edges) {
        retained[edge[0]] = 1;
        retained[edge[1]] = 1;
    }
    std::vector<std::uint32_t> mapped(graph.vertices.size(), UINT32_MAX);
    result.points.reserve(graph.vertices.size());
    for (std::size_t index = 0; index < graph.vertices.size(); ++index) {
        if (!retained[index]) continue;
        mapped[index] = static_cast<std::uint32_t>(result.points.size());
        result.points.push_back(graph.vertices[index]);
    }
    for (auto &edge : result.edges) {
        edge[0] = mapped[edge[0]];
        edge[1] = mapped[edge[1]];
    }
    return result;
}
std::vector<TerrainIceContourIntersection> produce_terrain_ice_contour_intersections(
    const TerrainIceLargeSourceGraph &source,
    std::uint32_t axis, float coordinate,
    std::uint32_t first_new_point) {
    if (axis > 1 || !std::isfinite(coordinate) ||
        source.points.empty() || source.points.size() > 1'000'000 ||
        source.edges.size() > 1'000'000 ||
        first_new_point < source.points.size() || first_new_point > 1'000'000)
        throw std::invalid_argument("Invalid ice contour sweep extent");
    struct Candidate {
        float position;
        std::uint32_t source_point;
        bool tangent;
        std::uint32_t source_edge;
        float position_on_edge;
    };
    struct VertexSides {
        std::uint8_t mask = 0;
        std::uint8_t degree = 0;
    };
    std::vector<VertexSides> sides(source.points.size());
    std::vector<std::uint32_t> touched;
    std::vector<Candidate> candidates;
    const auto other = 1u - axis;
    const auto touch = [&](std::uint32_t point, float neighbor_coordinate) {
        auto &record = sides[point];
        if (record.degree++ == 0) touched.push_back(point);
        record.mask |= neighbor_coordinate < coordinate ? 1u : 2u;
    };
    for (std::size_t edge_index = 0; edge_index < source.edges.size(); ++edge_index) {
        const auto &edge = source.edges[edge_index];
        if (edge[0] >= source.points.size() || edge[1] >= source.points.size() ||
            edge[0] == edge[1])
            throw std::invalid_argument("Invalid ice contour source edge");
        const auto &a = source.points[edge[0]];
        const auto &b = source.points[edge[1]];
        if (!std::isfinite(a[0]) || !std::isfinite(a[1]) ||
            !std::isfinite(b[0]) || !std::isfinite(b[1]))
            throw std::invalid_argument("Nonfinite ice contour source point");
        const float first = a[axis], second = b[axis];
        if (first == coordinate && second == coordinate)
            throw std::invalid_argument("Collinear ice contour sweep edge");
        if (first == coordinate) {
            touch(edge[0], second);
        } else if (second == coordinate) {
            touch(edge[1], first);
        } else if ((first < coordinate && coordinate < second) ||
                   (second < coordinate && coordinate < first)) {
            const float inverse = 1.0f / (second - first);
            const float delta = coordinate - first;
            float position = (b[other] - a[other]) * inverse;
            position *= delta;
            position += a[other];
            if (!std::isfinite(position))
                throw std::invalid_argument("Nonfinite ice contour intersection");
            candidates.push_back({position, UINT32_MAX, false,
                                  static_cast<std::uint32_t>(edge_index),
                                  delta * inverse});
        }
    }
    for (const auto point : touched) {
        const auto &record = sides[point];
        if (record.degree != 2 || record.mask == 0)
            throw std::invalid_argument("Invalid ice contour vertex incidence");
        candidates.push_back({source.points[point][other], point,
                              record.mask != 3, UINT32_MAX, 0.0f});
    }
    std::sort(candidates.begin(), candidates.end(),
              [](const Candidate &a, const Candidate &b) {
                  return a.position < b.position;
              });
    std::vector<TerrainIceContourIntersection> result;
    result.reserve(candidates.size());
    std::uint32_t parity = 0, next_point = first_new_point;
    float previous = -std::numeric_limits<float>::infinity();
    for (const auto &candidate : candidates) {
        if (candidate.position <= previous)
            throw std::invalid_argument("Tied ice contour intersections");
        previous = candidate.position;
        if (candidate.tangent) {
            if (parity)
                result.push_back({candidate.position, candidate.source_point,
                                  2, UINT32_MAX, 0.0f});
            continue;
        }
        std::uint32_t point = candidate.source_point;
        if (point == UINT32_MAX) {
            if (next_point >= 1'000'000)
                throw std::invalid_argument("Oversized ice contour sweep output");
            point = next_point++;
        }
        result.push_back({candidate.position, point, parity,
                          candidate.source_edge, candidate.position_on_edge});
        parity ^= 1;
    }
    if (parity)
        throw std::invalid_argument("Unpaired ice contour intersections");
    return result;
}
TerrainIceVerticalInsertionGraph insert_terrain_ice_vertical_lattice(
    const TerrainIceLargeSourceGraph &source,
    std::span<const TerrainIceVerticalSweepLine> lines,
    float spacing, const std::array<float, 4> *component_box) {
    if (source.points.empty() || source.points.size() > 1'000'000 ||
        lines.size() > 10'000 || !std::isfinite(spacing) || spacing <= 0.0f)
        throw std::invalid_argument("Invalid vertical ice lattice extent");
    float min_x = std::numeric_limits<float>::infinity();
    float max_x = -std::numeric_limits<float>::infinity();
    float min_y = std::numeric_limits<float>::infinity();
    float max_y = -std::numeric_limits<float>::infinity();
    for (const auto &point : source.points) {
        if (!std::isfinite(point[0]) || !std::isfinite(point[1]))
            throw std::invalid_argument("Nonfinite vertical ice source point");
        min_x = std::min(min_x, point[0]);
        max_x = std::max(max_x, point[0]);
        min_y = std::min(min_y, point[1]);
        max_y = std::max(max_y, point[1]);
    }
    if (component_box) {
        if (!std::isfinite((*component_box)[0]) ||
            !std::isfinite((*component_box)[1]) ||
            !std::isfinite((*component_box)[2]) ||
            !std::isfinite((*component_box)[3]) ||
            (*component_box)[0] > min_x || (*component_box)[1] > min_y ||
            (*component_box)[2] < max_x || (*component_box)[3] < max_y)
            throw std::invalid_argument("Invalid vertical ice component bounds");
        min_x = (*component_box)[0];
        max_x = (*component_box)[2];
        min_y = (*component_box)[1];
    }
    TerrainIceVerticalInsertionGraph result;
    result.points = source.points;
    const float origin_y = min_y - 0.01f;
    float expected_x = min_x + spacing;
    const auto advance = [spacing](float &coordinate) {
        const float next = coordinate + spacing;
        if (!std::isfinite(next) || next <= coordinate)
            throw std::invalid_argument("Vertical ice lattice failed to advance");
        coordinate = next;
    };
    for (const auto &line : lines) {
        if (expected_x >= max_x ||
            std::bit_cast<std::uint32_t>(line.x) !=
                std::bit_cast<std::uint32_t>(expected_x) ||
            line.events.size() % 2 || line.events.size() > 1'000'000)
            throw std::invalid_argument("Invalid vertical ice sweep line");
        float prior_y = -std::numeric_limits<float>::infinity();
        for (std::size_t i = 0; i < line.events.size(); ++i) {
            const auto &event = line.events[i];
            if (!std::isfinite(event.y) || event.y <= prior_y ||
                event.state != i % 2 || event.point > result.points.size())
                throw std::invalid_argument("Invalid vertical ice intersection");
            if (event.point == result.points.size()) {
                if (result.points.size() >= 1'000'000)
                    throw std::invalid_argument("Oversized vertical ice lattice");
                result.points.push_back({line.x, event.y});
            } else if (std::bit_cast<std::uint32_t>(
                           result.points[event.point][0]) !=
                           std::bit_cast<std::uint32_t>(line.x) ||
                       std::bit_cast<std::uint32_t>(
                           result.points[event.point][1]) !=
                           std::bit_cast<std::uint32_t>(event.y)) {
                throw std::invalid_argument("Vertical ice source intersection differs");
            }
            prior_y = event.y;
        }
        for (std::size_t i = 0; i < line.events.size(); i += 2) {
            const auto &enter = line.events[i];
            const auto &exit = line.events[i + 1];
            float y = origin_y;
            std::size_t steps = 0;
            while (y <= enter.y) {
                if (++steps > 1'000'000)
                    throw std::invalid_argument("Oversized vertical ice interval");
                advance(y);
            }
            std::uint32_t prior = enter.point;
            while (y < exit.y) {
                if (result.points.size() >= 1'000'000 ||
                    result.edges.size() >= 2'000'000)
                    throw std::invalid_argument("Oversized vertical ice lattice");
                const auto next = static_cast<std::uint32_t>(result.points.size());
                result.points.push_back({line.x, y});
                result.edges.push_back({prior, next, 0});
                prior = next;
                advance(y);
            }
            if (result.edges.size() >= 2'000'000)
                throw std::invalid_argument("Oversized vertical ice lattice");
            result.edges.push_back({prior, exit.point, 0});
        }
        advance(expected_x);
    }
    if (expected_x < max_x)
        throw std::invalid_argument("Incomplete vertical ice sweep lines");
    return result;
}
TerrainIceVerticalInsertionGraph insert_terrain_ice_vertical_lattice(
    const TerrainIceLargeSourceGraph &source, float spacing,
    std::vector<TerrainIceRefinementFrontierPoint> *frontier,
    const std::array<float, 4> *component_box) {
    if (source.points.empty() || source.points.size() > 1'000'000 ||
        !std::isfinite(spacing) || spacing <= 0.0f)
        throw std::invalid_argument("Invalid generated vertical ice lattice extent");
    float min_x = std::numeric_limits<float>::infinity();
    float max_x = -std::numeric_limits<float>::infinity();
    float min_y = std::numeric_limits<float>::infinity();
    float max_y = -std::numeric_limits<float>::infinity();
    for (const auto &point : source.points) {
        if (!std::isfinite(point[0]) || !std::isfinite(point[1]))
            throw std::invalid_argument("Nonfinite generated vertical ice source point");
        min_x = std::min(min_x, point[0]);
        max_x = std::max(max_x, point[0]);
        min_y = std::min(min_y, point[1]);
        max_y = std::max(max_y, point[1]);
    }
    if (component_box) {
        if (!std::isfinite((*component_box)[0]) ||
            !std::isfinite((*component_box)[1]) ||
            !std::isfinite((*component_box)[2]) ||
            !std::isfinite((*component_box)[3]) ||
            (*component_box)[0] > min_x || (*component_box)[1] > min_y ||
            (*component_box)[2] < max_x || (*component_box)[3] < max_y)
            throw std::invalid_argument("Invalid generated vertical ice component bounds");
        min_x = (*component_box)[0];
        max_x = (*component_box)[2];
        min_y = (*component_box)[1];
    }
    const float origin_y = min_y - 0.01f;
    float x = min_x + spacing;
    std::uint32_t next_point = static_cast<std::uint32_t>(source.points.size());
    std::vector<TerrainIceVerticalSweepLine> lines;
    const auto advance = [spacing](float &coordinate) {
        const float next = coordinate + spacing;
        if (!std::isfinite(next) || next <= coordinate)
            throw std::invalid_argument("Generated ice lattice failed to advance");
        coordinate = next;
    };
    while (x < max_x) {
        if (lines.size() >= 10'000)
            throw std::invalid_argument("Oversized generated vertical ice lattice");
        TerrainIceVerticalSweepLine line;
        line.x = x;
        for (const auto &hit : produce_terrain_ice_contour_intersections(
                 source, 0, x, next_point)) {
            if (hit.state == 2 || hit.point > next_point)
                throw std::invalid_argument("Unsupported vertical contour tangent");
            line.events.push_back({hit.position, hit.point, hit.state});
            if (frontier && hit.source_edge != UINT32_MAX)
                frontier->push_back({hit.point, hit.source_edge,
                                     hit.position_on_edge});
            if (hit.point == next_point) {
                if (next_point >= 1'000'000)
                    throw std::invalid_argument("Oversized generated vertical ice lattice");
                ++next_point;
            }
        }
        for (std::size_t i = 0; i < line.events.size(); i += 2) {
            float y = origin_y;
            std::size_t steps = 0;
            while (y <= line.events[i].y) {
                if (++steps > 1'000'000)
                    throw std::invalid_argument("Oversized generated vertical ice interval");
                advance(y);
            }
            while (y < line.events[i + 1].y) {
                if (next_point >= 1'000'000)
                    throw std::invalid_argument("Oversized generated vertical ice lattice");
                ++next_point;
                advance(y);
            }
        }
        lines.push_back(std::move(line));
        advance(x);
    }
    return insert_terrain_ice_vertical_lattice(source, lines, spacing,
                                               component_box);
}
TerrainIceVerticalInsertionGraph sample_terrain_ice_horizontal_lattice(
    TerrainIceVerticalInsertionGraph graph,
    std::span<const TerrainIceHorizontalSweepLine> lines) {
    if (graph.points.empty() || graph.points.size() > 1'000'000 ||
        graph.edges.size() > 2'000'000 || lines.size() > 10'000)
        throw std::invalid_argument("Invalid horizontal ice lattice extent");
    using RowPoint = std::pair<float, std::uint32_t>;
    std::unordered_map<std::uint32_t, std::vector<RowPoint>> rows;
    for (std::uint32_t index = 0; index < graph.points.size(); ++index) {
        const auto &point = graph.points[index];
        if (!std::isfinite(point[0]) || !std::isfinite(point[1]))
            throw std::invalid_argument("Nonfinite horizontal ice source point");
        rows[std::bit_cast<std::uint32_t>(point[1])].emplace_back(point[0], index);
    }
    for (auto &[_, row] : rows)
        std::sort(row.begin(), row.end());
    float prior_y = -std::numeric_limits<float>::infinity();
    for (const auto &line : lines) {
        if (!std::isfinite(line.y) || line.y <= prior_y ||
            line.events.size() > 1'000'000)
            throw std::invalid_argument("Invalid horizontal ice sweep line");
        const auto row_key = std::bit_cast<std::uint32_t>(line.y);
        const auto row_it = rows.find(row_key);
        if (row_it == rows.end())
            throw std::invalid_argument("Missing horizontal ice lattice row");
        const auto &row = row_it->second;
        float prior_x = -std::numeric_limits<float>::infinity();
        std::vector<const TerrainIceHorizontalEvent *> intersections;
        std::unordered_set<std::uint32_t> contour_vertices;
        for (std::size_t i = 0; i < line.events.size(); ++i) {
            const auto &event = line.events[i];
            if (!std::isfinite(event.x) || event.x <= prior_x ||
                event.state > 2 || event.point > graph.points.size())
                throw std::invalid_argument("Invalid horizontal ice intersection");
            if (event.point == graph.points.size()) {
                if (event.state == 2 || graph.points.size() >= 1'000'000)
                    throw std::invalid_argument("Oversized horizontal ice lattice");
                graph.points.push_back({event.x, line.y});
            } else if (std::bit_cast<std::uint32_t>(
                           graph.points[event.point][0]) !=
                           std::bit_cast<std::uint32_t>(event.x) ||
                       std::bit_cast<std::uint32_t>(
                           graph.points[event.point][1]) != row_key) {
                throw std::invalid_argument("Horizontal ice source intersection differs");
            }
            if (event.state == 2) {
                contour_vertices.insert(event.point);
            } else {
                if (event.state != intersections.size() % 2)
                    throw std::invalid_argument("Invalid horizontal ice interval order");
                intersections.push_back(&event);
            }
            prior_x = event.x;
        }
        if (intersections.size() % 2)
            throw std::invalid_argument("Unpaired horizontal ice intersection");
        for (std::size_t i = 0; i < intersections.size(); i += 2) {
            const auto &enter = *intersections[i];
            const auto &exit = *intersections[i + 1];
            auto cursor = std::upper_bound(
                row.begin(), row.end(), enter.x,
                [](float x, const RowPoint &point) { return x < point.first; });
            std::uint32_t previous = enter.point;
            while (cursor != row.end() && cursor->first < exit.x) {
                if (graph.edges.size() >= 2'000'000)
                    throw std::invalid_argument("Oversized horizontal ice lattice");
                if (contour_vertices.contains(cursor->second))
                    graph.edges.push_back({previous, cursor->second, 0});
                else
                    graph.edges.push_back({cursor->second, previous, 0});
                previous = cursor->second;
                ++cursor;
            }
            if (graph.edges.size() >= 2'000'000)
                throw std::invalid_argument("Oversized horizontal ice lattice");
            graph.edges.push_back({previous, exit.point, 0});
        }
        prior_y = line.y;
    }
    return graph;
}
TerrainIceVerticalInsertionGraph sample_terrain_ice_horizontal_lattice(
    TerrainIceVerticalInsertionGraph graph,
    const TerrainIceLargeSourceGraph &source, float spacing,
    std::vector<TerrainIceRefinementFrontierPoint> *frontier,
    const std::array<float, 4> *component_box) {
    if (source.points.empty() || graph.points.size() > 1'000'000 ||
        !std::isfinite(spacing) || spacing <= 0.0f)
        throw std::invalid_argument("Invalid generated horizontal ice lattice extent");
    float min_y = std::numeric_limits<float>::infinity();
    float max_y = -std::numeric_limits<float>::infinity();
    for (const auto &point : source.points) {
        if (!std::isfinite(point[1]))
            throw std::invalid_argument("Nonfinite generated horizontal ice source point");
        min_y = std::min(min_y, point[1]);
        max_y = std::max(max_y, point[1]);
    }
    if (component_box) {
        if (!std::isfinite((*component_box)[1]) ||
            !std::isfinite((*component_box)[3]) ||
            (*component_box)[1] > min_y || (*component_box)[3] < max_y)
            throw std::invalid_argument("Invalid generated horizontal ice component bounds");
        min_y = (*component_box)[1];
        max_y = (*component_box)[3];
    }
    const float origin_y = min_y - 0.01f;
    float y = origin_y + spacing;
    std::uint32_t next_point = static_cast<std::uint32_t>(graph.points.size());
    std::vector<std::uint8_t> degree(graph.points.size());
    for (const auto &edge : graph.edges) {
        if (edge.first_point >= degree.size() ||
            edge.second_point >= degree.size() ||
            ++degree[edge.first_point] > 2 ||
            ++degree[edge.second_point] > 2)
            throw std::invalid_argument("Invalid generated horizontal ice source edges");
    }
    std::unordered_set<std::uint32_t> occupied_rows;
    for (std::size_t i = 0; i < graph.points.size(); ++i)
        if (degree[i] == 2)
            occupied_rows.insert(std::bit_cast<std::uint32_t>(graph.points[i][1]));
    std::vector<TerrainIceHorizontalSweepLine> lines;
    while (y < max_y) {
        if (occupied_rows.erase(std::bit_cast<std::uint32_t>(y))) {
            if (lines.size() >= 10'000)
                throw std::invalid_argument("Oversized generated horizontal ice lattice");
            TerrainIceHorizontalSweepLine line;
            line.y = y;
            for (const auto &hit : produce_terrain_ice_contour_intersections(
                     source, 1, y, next_point)) {
                if (hit.point > next_point)
                    throw std::invalid_argument("Invalid generated horizontal intersection");
                line.events.push_back({hit.position, hit.point, hit.state});
                if (frontier && hit.source_edge != UINT32_MAX)
                    frontier->push_back({hit.point, hit.source_edge,
                                         hit.position_on_edge});
                if (hit.point == next_point) {
                    if (next_point >= 1'000'000)
                        throw std::invalid_argument("Oversized generated horizontal ice lattice");
                    ++next_point;
                }
            }
            lines.push_back(std::move(line));
        }
        const float next = y + spacing;
        if (!std::isfinite(next) || next <= y)
            throw std::invalid_argument("Generated horizontal ice lattice failed to advance");
        y = next;
    }
    if (!occupied_rows.empty())
        throw std::invalid_argument("Generated horizontal ice rows are off lattice");
    return sample_terrain_ice_horizontal_lattice(std::move(graph), lines);
}
TerrainIceTessellationGraph assemble_terrain_ice_refined_graph(
    std::span<const std::array<float, 2>> point_positions,
    std::span<const TerrainIceTessellationEdge> edges) {
    if (point_positions.size() > 1'000'000 || edges.size() > 2'000'000 ||
        edges.size() > UINT32_MAX / 2)
        throw std::invalid_argument("Invalid refined ice graph extent");
    TerrainIceTessellationGraph result;
    result.points.resize(point_positions.size());
    result.edges.assign(edges.begin(), edges.end());
    for (std::size_t i = 0; i < point_positions.size(); ++i) {
        if (!std::isfinite(point_positions[i][0]) ||
            !std::isfinite(point_positions[i][1]))
            throw std::invalid_argument("Invalid refined ice point");
        result.points[i].position = point_positions[i];
    }
    for (const auto &edge : result.edges) {
        if (edge.first_point >= result.points.size() ||
            edge.second_point >= result.points.size() ||
            edge.first_point == edge.second_point || edge.active > 1)
            throw std::invalid_argument("Invalid refined ice edge");
        ++result.points[edge.first_point].halfedge_count;
        ++result.points[edge.second_point].halfedge_count;
    }
    std::uint32_t cursor = 0;
    std::vector<std::uint32_t> remaining;
    remaining.reserve(result.points.size());
    for (auto &point : result.points) {
        if (!point.halfedge_count || point.halfedge_count > 16)
            throw std::invalid_argument("Invalid refined ice incidence degree");
        point.first_halfedge = cursor;
        cursor += point.halfedge_count;
        remaining.push_back(point.halfedge_count);
    }
    result.halfedge_edges.resize(cursor);
    for (std::uint32_t edge_id = 0; edge_id < result.edges.size(); ++edge_id) {
        const auto &edge = result.edges[edge_id];
        for (const auto endpoint : {edge.first_point, edge.second_point}) {
            auto &left = remaining[endpoint];
            result.halfedge_edges[
                result.points[endpoint].first_halfedge + --left] = edge_id;
        }
    }
    for (std::size_t point_id = 0; point_id < result.points.size(); ++point_id) {
        const auto &point = result.points[point_id];
        const auto start = result.halfedge_edges.begin() + point.first_halfedge;
        const auto finish = start + point.halfedge_count;
        const auto angle = [&](std::uint32_t edge_id) {
            const auto &edge = result.edges[edge_id];
            const auto neighbor = edge.first_point == point_id ?
                edge.second_point : edge.first_point;
            const auto &position = result.points[neighbor].position;
            const double dx = static_cast<double>(position[0]) - point.position[0];
            const double dy = static_cast<double>(position[1]) - point.position[1];
            if (dx == 0.0 && dy == 0.0)
                throw std::invalid_argument("Degenerate refined ice edge");
            const double a = std::atan2(dy, dx);
            return dy == 0.0 && dx < 0.0 ? -3.14159265358979323846 : a;
        };
        std::stable_sort(start, finish, [&](std::uint32_t a, std::uint32_t b) {
            return angle(a) > angle(b);
        });
    }
    return result;
}
std::vector<std::uint8_t> select_terrain_ice_refinement_boundary_points(
    std::uint32_t point_count,
    std::span<const TerrainIceTessellationEdge> sampled_edges) {
    if (point_count > 1'000'000 || sampled_edges.size() > 2'000'000)
        throw std::invalid_argument("Invalid ice refinement boundary extent");
    std::vector<std::uint8_t> degree(point_count);
    for (const auto &edge : sampled_edges) {
        if (edge.first_point >= point_count ||
            edge.second_point >= point_count ||
            edge.first_point == edge.second_point || edge.active)
            throw std::invalid_argument("Invalid sampled ice refinement edge");
        if (++degree[edge.first_point] > 4 ||
            ++degree[edge.second_point] > 4)
            throw std::invalid_argument("Oversubscribed sampled ice point");
    }
    for (auto &value : degree) value = static_cast<std::uint8_t>(value < 4);
    return degree;
}
std::vector<TerrainIceTessellationEdge> stitch_terrain_ice_refinement_boundary(
    std::uint32_t point_count,
    std::span<const std::array<std::uint32_t, 2>> source_edges,
    std::span<const TerrainIceRefinementFrontierPoint> frontier) {
    if (point_count > 1'000'000 || source_edges.size() > 1'000'000 ||
        frontier.size() > 1'000'000)
        throw std::invalid_argument("Invalid ice refinement stitching extent");
    for (const auto &edge : source_edges)
        if (edge[0] >= point_count || edge[1] >= point_count || edge[0] == edge[1])
            throw std::invalid_argument("Invalid ice refinement source edge");
    std::vector<TerrainIceRefinementFrontierPoint> ordered(frontier.begin(),
                                                           frontier.end());
    for (const auto &entry : ordered)
        if (entry.point >= point_count || entry.source_edge >= source_edges.size() ||
            !std::isfinite(entry.position_on_edge))
            throw std::invalid_argument("Invalid ice refinement frontier point");
    std::sort(ordered.begin(), ordered.end(), [](const auto &a, const auto &b) {
        if (a.source_edge != b.source_edge) return a.source_edge < b.source_edge;
        return a.position_on_edge < b.position_on_edge;
    });
    std::vector<TerrainIceTessellationEdge> result;
    result.reserve(source_edges.size() + ordered.size());
    std::size_t cursor = 0;
    for (std::size_t index = 0; index < source_edges.size(); ++index) {
        std::uint32_t prior = source_edges[index][0];
        while (cursor < ordered.size() && ordered[cursor].source_edge == index) {
            if (cursor && ordered[cursor - 1].source_edge == index &&
                ordered[cursor - 1].position_on_edge == ordered[cursor].position_on_edge)
                throw std::invalid_argument("Ambiguous ice refinement frontier order");
            const auto next = ordered[cursor++].point;
            if (next == prior)
                throw std::invalid_argument("Degenerate ice refinement boundary edge");
            result.push_back({prior, next, 1});
            prior = next;
        }
        if (prior == source_edges[index][1])
            throw std::invalid_argument("Degenerate ice refinement source chain");
        result.push_back({prior, source_edges[index][1], 1});
    }
    return result;
}
TerrainIceTessellationGraph construct_terrain_ice_refined_graph(
    const TerrainIceLargeSourceGraph &source, float spacing,
    const std::array<float, 4> *component_box) {
    std::vector<TerrainIceRefinementFrontierPoint> frontier;
    auto graph = insert_terrain_ice_vertical_lattice(
        source, spacing, &frontier, component_box);
    graph = sample_terrain_ice_horizontal_lattice(std::move(graph), source,
                                                   spacing, &frontier,
                                                   component_box);
    const auto boundary = select_terrain_ice_refinement_boundary_points(
        static_cast<std::uint32_t>(graph.points.size()), graph.edges);
    std::size_t count = 0;
    for (const auto flag : boundary) count += flag;
    if (count != source.points.size() + frontier.size())
        throw std::invalid_argument("Generated ice boundary count differs");
    for (const auto &entry : frontier)
        if (entry.point >= boundary.size() || !boundary[entry.point])
            throw std::invalid_argument("Generated ice frontier escaped boundary");
    std::vector<std::array<std::uint32_t, 2>> source_edges;
    source_edges.reserve(source.edges.size());
    for (const auto &edge : source.edges)
        source_edges.push_back({edge[0], edge[1]});
    auto stitched = stitch_terrain_ice_refinement_boundary(
        static_cast<std::uint32_t>(graph.points.size()), source_edges, frontier);
    graph.edges.insert(graph.edges.end(), stitched.begin(), stitched.end());
    return assemble_terrain_ice_refined_graph(graph.points, graph.edges);
}
std::vector<std::uint8_t> select_terrain_ice_exterior_halfedges(
    const TerrainIceTessellationGraph &graph) {
    const auto point_count = graph.points.size();
    const auto edge_count = graph.edges.size();
    const auto halfedge_count = graph.halfedge_edges.size();
    if (point_count > 1'000'000 || edge_count > 2'000'000 ||
        halfedge_count != 2 * edge_count)
        throw std::invalid_argument("Invalid ice tessellation graph extent");
    std::vector<std::array<std::uint8_t, 2>> incidence(edge_count);
    std::size_t cursor = 0;
    for (std::size_t vertex = 0; vertex < point_count; ++vertex) {
        const auto &point = graph.points[vertex];
        if (point.first_halfedge != cursor || !point.halfedge_count ||
            point.halfedge_count > 16 ||
            point.halfedge_count > halfedge_count - cursor ||
            !std::isfinite(point.position[0]) || !std::isfinite(point.position[1]))
            throw std::invalid_argument("Invalid ice tessellation point");
        for (std::uint32_t local = 0; local < point.halfedge_count; ++local) {
            const auto edge_id = graph.halfedge_edges[cursor + local];
            if (edge_id >= edge_count)
                throw std::invalid_argument("Ice tessellation halfedge escaped edges");
            const auto &edge = graph.edges[edge_id];
            if (edge.first_point >= point_count || edge.second_point >= point_count ||
                edge.first_point == edge.second_point || edge.active > 1)
                throw std::invalid_argument("Invalid ice tessellation edge");
            const unsigned side = edge.first_point == vertex ? 0 :
                                  edge.second_point == vertex ? 1 : 2;
            if (side == 2 || ++incidence[edge_id][side] != 1)
                throw std::invalid_argument("Invalid ice tessellation incidence");
        }
        cursor += point.halfedge_count;
    }
    if (cursor != halfedge_count ||
        std::any_of(incidence.begin(), incidence.end(), [](const auto &pair) {
            return pair[0] != 1 || pair[1] != 1;
        }))
        throw std::invalid_argument("Incomplete ice tessellation incidence");

    std::vector<std::uint8_t> selected(halfedge_count);
    std::vector<std::uint32_t> walk;
    for (std::uint32_t start_point = 0; start_point < point_count; ++start_point) {
        const auto &start = graph.points[start_point];
        for (std::uint32_t start_slot = 0; start_slot < start.halfedge_count; ++start_slot) {
            const auto start_halfedge = start.first_halfedge + start_slot;
            if (!graph.edges[graph.halfedge_edges[start_halfedge]].active ||
                selected[start_halfedge]) continue;
            walk.clear();
            auto point = start_point, slot = start_slot;
            for (std::size_t steps = 0; steps <= halfedge_count; ++steps) {
                if (steps == halfedge_count)
                    throw std::invalid_argument("Ice exterior boundary walk did not terminate");
                const auto &current = graph.points[point];
                const auto halfedge = current.first_halfedge + slot;
                walk.push_back(halfedge);
                const auto &edge = graph.edges[graph.halfedge_edges[halfedge]];
                const auto next_point = edge.first_point == point ?
                                        edge.second_point : edge.first_point;
                const auto &neighbor = graph.points[next_point];
                std::uint32_t matching = 0;
                for (; matching < neighbor.halfedge_count; ++matching) {
                    const auto &candidate = graph.edges[
                        graph.halfedge_edges[neighbor.first_halfedge + matching]];
                    if ((candidate.first_point == point &&
                         candidate.second_point == next_point) ||
                        (candidate.first_point == next_point &&
                         candidate.second_point == point)) break;
                }
                if (matching == neighbor.halfedge_count)
                    throw std::invalid_argument("Ice exterior edge has no opposite incidence");
                const auto next_slot = (matching + 1) % neighbor.halfedge_count;
                if (!graph.edges[graph.halfedge_edges[
                        neighbor.first_halfedge + next_slot]].active) break;
                if (next_point == start_point && next_slot == start_slot) {
                    for (const auto index : walk) selected[index] = 1;
                    break;
                }
                point = next_point;
                slot = next_slot;
            }
        }
    }
    return selected;
}

TerrainIceTessellationPreparation prepare_terrain_ice_tessellation(
    const TerrainIceTessellationGraph &graph) {
    TerrainIceTessellationPreparation result;
    result.flags = select_terrain_ice_exterior_halfedges(graph);
    const auto halfedge_count = result.flags.size();
    std::uint32_t pending_vertices = 0;
    for (std::uint32_t start_point = 0; start_point < graph.points.size(); ++start_point) {
        const auto &start = graph.points[start_point];
        for (std::uint32_t start_slot = 0; start_slot < start.halfedge_count; ++start_slot) {
            const auto start_halfedge = start.first_halfedge + start_slot;
            if (result.flags[start_halfedge]) continue;
            auto point = start_point, slot = start_slot;
            for (std::size_t steps = 0; steps < halfedge_count; ++steps) {
                const auto &current = graph.points[point];
                const auto halfedge = current.first_halfedge + slot;
                result.flags[halfedge] = 2;
                if (pending_vertices == UINT32_MAX)
                    throw std::invalid_argument("Ice tessellation polygon is too large");
                ++pending_vertices;
                const auto &edge = graph.edges[graph.halfedge_edges[halfedge]];
                const auto next_point = edge.first_point == point ?
                                        edge.second_point : edge.first_point;
                const auto &neighbor = graph.points[next_point];
                std::uint32_t matching = 0;
                for (; matching < neighbor.halfedge_count; ++matching) {
                    const auto &candidate = graph.edges[
                        graph.halfedge_edges[neighbor.first_halfedge + matching]];
                    if ((candidate.first_point == point &&
                         candidate.second_point == next_point) ||
                        (candidate.first_point == next_point &&
                         candidate.second_point == point)) break;
                }
                if (matching == neighbor.halfedge_count)
                    throw std::invalid_argument("Ice polygon side has no opposite incidence");
                const auto next_slot = (matching + 1) % neighbor.halfedge_count;
                if (next_point == start_point && next_slot == start_slot) {
                    if (pending_vertices >= 3) {
                        const auto indices = 3ull * pending_vertices - 6;
                        if (indices > UINT32_MAX - result.polygon_indices)
                            throw std::invalid_argument("Ice polygon index count overflow");
                        result.polygon_indices += static_cast<std::uint32_t>(indices);
                        result.max_polygon_vertices = std::max(
                            result.max_polygon_vertices, pending_vertices);
                        pending_vertices = 0;
                    }
                    break;
                }
                const auto next_halfedge = neighbor.first_halfedge + next_slot;
                if (result.flags[next_halfedge] == 2)
                    throw std::invalid_argument("Ice polygon walk revisited a marked side");
                point = next_point;
                slot = next_slot;
                if (steps + 1 == halfedge_count)
                    throw std::invalid_argument("Ice polygon walk did not terminate");
            }
        }
    }
    return result;
}

namespace {
void append_terrain_ice_cap_indices(
    const std::vector<std::array<float, 2>> &outline,
    const std::vector<std::uint32_t> &vertex_ids,
    std::vector<std::uint32_t> &indices) {
    const auto count = outline.size();
    if (count < 3 || count != vertex_ids.size())
        throw std::invalid_argument("Invalid native ice cap outline");
    const auto signed_area = [&](std::uint32_t a, std::uint32_t b,
                                 std::uint32_t c) {
        const auto &p = outline[a], &q = outline[b], &r = outline[c];
        const float dx1 = q[0] - p[0], dy1 = q[1] - p[1];
        const float dx2 = r[0] - p[0], dy2 = r[1] - p[1];
        return dx1 * dy2 - dy1 * dx2;
    };
    std::vector<std::uint32_t> next(count);
    for (std::uint32_t corner = 0; corner < count; ++corner)
        next[corner] = (corner + 1) % static_cast<std::uint32_t>(count);
    std::uint32_t lowest = 0;
    for (std::uint32_t corner = 1; corner < count; ++corner)
        if (outline[corner][1] < outline[lowest][1]) lowest = corner;
    const bool clockwise = signed_area(
        (lowest + static_cast<std::uint32_t>(count) - 1) %
            static_cast<std::uint32_t>(count),
        lowest, next[lowest]) < 0.0f;
    std::uint32_t a = 0, b = 1, c = 2;
    for (std::size_t remaining = count; remaining >= 3; --remaining) {
        std::size_t examined = remaining;
        while ((signed_area(a, b, c) < 0.0f) != clockwise) {
            a = b; b = c; c = next[c];
            if (--examined == 0)
                throw std::invalid_argument("Degenerate native ice polygon");
        }
        std::size_t rechecks = 0;
        while (next[c] != a) {
            auto point = next[c];
            bool contains_other_point = false;
            while (point != a) {
                const auto inside = [&](float area) {
                    return clockwise ? area < 0.0f : area > 0.0f;
                };
                if (inside(signed_area(a, b, point)) &&
                    inside(signed_area(b, c, point)) &&
                    inside(signed_area(c, a, point))) {
                    contains_other_point = true;
                    break;
                }
                point = next[point];
            }
            if (!contains_other_point) break;
            if (++rechecks > remaining * remaining)
                throw std::invalid_argument("Unresolved native ice polygon overlap");
            std::size_t spins = remaining;
            while (true) {
                a = b; b = c; c = next[c];
                if ((signed_area(a, b, c) < 0.0f) == clockwise) break;
                if (--spins == 0)
                    throw std::invalid_argument("Degenerate native ice polygon");
            }
        }
        indices.insert(indices.end(),
                       {vertex_ids[a], vertex_ids[b], vertex_ids[c]});
        next[a] = c;
        b = c;
        c = next[c];
    }
}
std::array<float, 2> terrain_ice_skirt_xy(
    const std::array<float, 2> &center,
    const std::array<float, 2> &following,
    const std::array<float, 2> &preceding,
    float skirt_distance) {
    const float next_x = following[0] - center[0];
    const float next_y = following[1] - center[1];
    const float prev_x = preceding[0] - center[0];
    const float prev_y = preceding[1] - center[1];
    const float next_length = static_cast<float>(std::sqrt(
        static_cast<double>(next_x) * next_x +
        static_cast<double>(next_y) * next_y));
    const float prev_length = static_cast<float>(std::sqrt(
        static_cast<double>(prev_x) * prev_x +
        static_cast<double>(prev_y) * prev_y));
    if (!(next_length > 0.0f) || !(prev_length > 0.0f))
        throw std::invalid_argument("Degenerate native ice skirt edge");
    const float next_inverse = 1.0f / next_length;
    const float prev_inverse = 1.0f / prev_length;
    const float nx = next_inverse * next_x;
    const float ny = next_inverse * next_y;
    const float px = prev_inverse * prev_x;
    const float py = prev_inverse * prev_y;
    const float diff_x = px - nx;
    const float diff_y = ny - py;
    const float diff_length = static_cast<float>(std::sqrt(
        static_cast<double>(diff_x) * diff_x +
        static_cast<double>(diff_y) * diff_y));
    if (!(diff_length > 0.0f))
        throw std::invalid_argument("Degenerate native ice skirt corner");
    const float diff_inverse = 1.0f / diff_length;
    float offset_x = diff_inverse * diff_y;
    float offset_y = diff_inverse * diff_x;
    const float dot = ny * py + px * nx;
    if (dot > std::bit_cast<float>(0x3f7ff972u)) {
        offset_x = -nx;
        offset_y = -ny;
    }
    return {center[0] + offset_x * skirt_distance,
            center[1] + offset_y * skirt_distance};
}
} // namespace

std::vector<std::uint32_t> build_terrain_ice_large_indices(
    const TerrainIceTessellationGraph &graph, std::uint32_t first_vertex) {
    if (graph.points.size() > UINT32_MAX - first_vertex)
        throw std::invalid_argument("Large ice vertex index overflow");
    const auto selected = select_terrain_ice_exterior_halfedges(graph);
    std::vector<std::uint8_t> visited(selected.size());
    std::vector<std::uint32_t> indices;
    std::vector<std::uint32_t> ring;
    std::vector<std::array<float, 2>> outline;
    std::vector<std::uint32_t> vertex_ids;
    for (std::uint32_t start_point = 0; start_point < graph.points.size(); ++start_point) {
        const auto &start = graph.points[start_point];
        for (std::uint32_t start_slot = 0; start_slot < start.halfedge_count; ++start_slot) {
            const auto start_halfedge = start.first_halfedge + start_slot;
            if (selected[start_halfedge] || visited[start_halfedge]) continue;
            ring.clear();
            auto point = start_point, slot = start_slot;
            bool closed = false;
            for (std::size_t steps = 0; steps < selected.size(); ++steps) {
                const auto &current = graph.points[point];
                const auto halfedge = current.first_halfedge + slot;
                if (selected[halfedge] || visited[halfedge])
                    throw std::invalid_argument("Large ice face revisited a side");
                visited[halfedge] = 1;
                ring.push_back(point);
                const auto &edge = graph.edges[graph.halfedge_edges[halfedge]];
                const auto next_point = edge.first_point == point ?
                                        edge.second_point : edge.first_point;
                const auto &neighbor = graph.points[next_point];
                std::uint32_t matching = 0;
                for (; matching < neighbor.halfedge_count; ++matching) {
                    const auto &candidate = graph.edges[
                        graph.halfedge_edges[neighbor.first_halfedge + matching]];
                    if ((candidate.first_point == point &&
                         candidate.second_point == next_point) ||
                        (candidate.first_point == next_point &&
                         candidate.second_point == point)) break;
                }
                if (matching == neighbor.halfedge_count)
                    throw std::invalid_argument("Large ice face has no opposite incidence");
                const auto next_slot = (matching + 1) % neighbor.halfedge_count;
                if (next_point == start_point && next_slot == start_slot) {
                    closed = true;
                    break;
                }
                point = next_point;
                slot = next_slot;
            }
            if (!closed)
                throw std::invalid_argument("Large ice face did not close");
            if (ring.size() < 3) continue;
            outline.clear();
            vertex_ids.clear();
            outline.reserve(ring.size());
            vertex_ids.reserve(ring.size());
            for (const auto vertex : ring) {
                outline.push_back(graph.points[vertex].position);
                vertex_ids.push_back(first_vertex + vertex);
            }
            append_terrain_ice_cap_indices(outline, vertex_ids, indices);
        }
    }
    std::vector<std::uint32_t> bottom(graph.points.size(), UINT32_MAX);
    std::vector<std::uint32_t> outgoing(graph.points.size(), UINT32_MAX);
    std::uint32_t bottom_count = 0;
    for (std::uint32_t point = 0; point < graph.points.size(); ++point) {
        const auto &vertex = graph.points[point];
        bool incoming = false;
        for (std::uint32_t slot = 0; slot < vertex.halfedge_count; ++slot) {
            const auto edge_id = graph.halfedge_edges[vertex.first_halfedge + slot];
            const auto &edge = graph.edges[edge_id];
            if (!edge.active) continue;
            if (edge.first_point == point) outgoing[point] = edge_id;
            else incoming = true;
        }
        if (incoming != (outgoing[point] != UINT32_MAX))
            throw std::invalid_argument("Open large ice skirt boundary");
        if (!incoming) continue;
        const std::uint64_t index = static_cast<std::uint64_t>(first_vertex) +
                                    graph.points.size() + bottom_count;
        if (index > UINT32_MAX)
            throw std::invalid_argument("Large ice skirt vertex index overflow");
        bottom[point] = static_cast<std::uint32_t>(index);
        ++bottom_count;
    }
    if (bottom_count != std::count(selected.begin(), selected.end(), std::uint8_t{1}))
        throw std::invalid_argument("Large ice skirt count differs from boundary");
    for (std::uint32_t point = 0; point < graph.points.size(); ++point) {
        if (outgoing[point] == UINT32_MAX) continue;
        const auto &edge = graph.edges[outgoing[point]];
        const auto u = edge.first_point, v = edge.second_point;
        if (u != point || bottom[v] == UINT32_MAX)
            throw std::invalid_argument("Invalid large ice skirt endpoint");
        indices.insert(indices.end(),
                       {bottom[u], first_vertex + v, first_vertex + u,
                        bottom[u], bottom[v], first_vertex + v});
    }
    return indices;
}

std::vector<std::array<float, 4>> build_terrain_ice_large_top_vertices(
    const TerrainIceTessellationGraph &graph,
    std::span<const std::uint8_t> displacement_pixels,
    std::uint32_t width, std::uint32_t height, std::uint32_t pitch,
    const TerrainIceLargeTopSettings &settings) {
    if (width < 2 || height < 2 || pitch < width ||
        !std::isfinite(settings.scale) || settings.scale <= 0.0f ||
        !std::isfinite(settings.displacement_frequency) ||
        !std::isfinite(settings.displacement_min) ||
        !std::isfinite(settings.displacement_max) ||
        !std::isfinite(settings.height) ||
        !std::isfinite(settings.height_ceiling) ||
        settings.displacement_max < settings.displacement_min ||
        settings.height_ceiling < 0.0f)
        throw std::invalid_argument("Invalid large ice displacement settings");
    const float coordinate_scale = 0.5f / settings.scale;
    const float inverse_byte = std::bit_cast<float>(0x3b808081u);
    const float range = settings.displacement_max - settings.displacement_min;
    std::vector<std::array<float, 4>> result;
    result.reserve(graph.points.size());
    for (const auto &point : graph.points) {
        const float x = (point.position[0] * coordinate_scale) *
                        settings.displacement_frequency;
        const float y = (point.position[1] * coordinate_scale) *
                        settings.displacement_frequency;
        const float u = static_cast<float>(std::fmod(
            std::fabs(static_cast<double>(x)), static_cast<double>(width - 1)));
        const float v = static_cast<float>(std::fmod(
            std::fabs(static_cast<double>(y)), static_cast<double>(height - 1)));
        if (!std::isfinite(u) || !std::isfinite(v))
            throw std::invalid_argument("Nonfinite large ice displacement coordinate");
        const auto ix = static_cast<std::uint32_t>(std::floor(u));
        const auto iy = static_cast<std::uint32_t>(std::floor(v));
        const float fx = u - static_cast<float>(ix);
        const float fy = v - static_cast<float>(iy);
        const std::size_t offset = static_cast<std::size_t>(iy) * pitch + ix;
        if (offset + pitch + 1 >= displacement_pixels.size())
            throw std::invalid_argument("Large ice displacement pixels are incomplete");
        const float a = static_cast<float>(displacement_pixels[offset]);
        const float b = static_cast<float>(displacement_pixels[offset + 1]);
        const float c = static_cast<float>(displacement_pixels[offset + pitch]);
        const float d = static_cast<float>(displacement_pixels[offset + pitch + 1]);
        const float upper = a + (b - a) * fx;
        const float lower = c + (d - c) * fx;
        const float sample = (upper + (lower - upper) * fy) * inverse_byte;
        const float z = settings.height +
                        (settings.displacement_min + sample * range);
        result.push_back({point.position[0], point.position[1],
                          std::clamp(z, 0.0f, settings.height_ceiling), 0.0f});
    }
    return result;
}

std::vector<std::array<float, 4>> build_terrain_ice_large_border_vertices(
    const TerrainIceTessellationGraph &graph, float scale,
    float skirt_slope, float depth) {
    if (!std::isfinite(scale) || !std::isfinite(skirt_slope) ||
        !std::isfinite(depth) || scale <= 0.0f || depth < 0.0f)
        throw std::invalid_argument("Invalid large ice skirt settings");
    const float skirt_distance = skirt_slope * scale;
    if (!std::isfinite(skirt_distance))
        throw std::invalid_argument("Invalid large ice skirt distance");
    const auto selected = select_terrain_ice_exterior_halfedges(graph);
    std::vector<std::array<float, 4>> result;
    for (std::uint32_t point = 0; point < graph.points.size(); ++point) {
        const auto &vertex = graph.points[point];
        std::uint32_t incoming = UINT32_MAX, outgoing = UINT32_MAX;
        for (std::uint32_t slot = 0; slot < vertex.halfedge_count; ++slot) {
            const auto &edge = graph.edges[
                graph.halfedge_edges[vertex.first_halfedge + slot]];
            if (!edge.active) continue;
            if (edge.first_point == point) outgoing = edge.second_point;
            else incoming = edge.first_point;
        }
        if ((incoming == UINT32_MAX) != (outgoing == UINT32_MAX))
            throw std::invalid_argument("Open large ice skirt boundary");
        if (incoming == UINT32_MAX) continue;
        const auto xy = terrain_ice_skirt_xy(
            vertex.position, graph.points[outgoing].position,
            graph.points[incoming].position, skirt_distance);
        result.push_back({xy[0], xy[1], -depth, 0.0f});
    }
    if (result.size() != static_cast<std::size_t>(
            std::count(selected.begin(), selected.end(), std::uint8_t{1})))
        throw std::invalid_argument("Large ice skirt vertex count differs from boundary");
    return result;
}

TerrainIceRawMesh build_terrain_ice_small_mesh(
    const TerrainIceIntermediateRegionGeometry &graph,
    const TerrainIceSmallMeshSettings &settings, std::uint32_t random_seed) {
    constexpr std::uint32_t modulus = 0xffffcfefu;
    constexpr double inverse = 2.3283131071116735e-10;
    if (!std::isfinite(settings.scale) || !std::isfinite(settings.height_min) ||
        !std::isfinite(settings.height_max) || !std::isfinite(settings.depth_min) ||
        !std::isfinite(settings.depth_max) || !std::isfinite(settings.skirt_slope) ||
        settings.max_polygon_vertices < 3 || graph.regions.size() > UINT32_MAX ||
        graph.vertices.size() > UINT32_MAX || graph.indices.size() > UINT32_MAX)
        throw std::invalid_argument("Invalid native small ice mesh settings or graph");
    std::vector<std::vector<std::array<std::uint32_t, 2>>> region_edges(
        graph.regions.size());
    for (const auto &edge : graph.indices) {
        if (edge[0] >= graph.vertices.size() || edge[1] >= graph.vertices.size() ||
            edge[2] >= graph.regions.size())
            throw std::invalid_argument("Invalid native small ice contour edge");
        region_edges[edge[2]].push_back({edge[0], edge[1]});
    }
    TerrainIceRawMesh result;
    const float skirt_distance = settings.skirt_slope * settings.scale;
    const auto next_draw = [&]() {
        random_seed = (random_seed * 0x343fdu + 0x269ec3u) % modulus;
        return static_cast<double>(random_seed) * inverse;
    };
    const auto vertex = [](float x, float y, float z) {
        return std::bit_cast<std::array<std::uint8_t, 16>>(
            std::array<float, 4>{x, y, z, 0.0f});
    };
    for (std::size_t region_index = 0; region_index < graph.regions.size(); ++region_index) {
        const auto &region = graph.regions[region_index];
        const auto count = static_cast<std::size_t>(region.vertex_count);
        if (count < 3 || count > settings.max_polygon_vertices) continue;
        if (region_edges[region_index].size() != count ||
            region.first_vertex >= graph.vertices.size() ||
            result.vertices.size() > UINT32_MAX - 2 * count)
            throw std::invalid_argument("Unsupported native small ice contour");
        std::vector<std::array<float, 2>> outline;
        outline.reserve(count);
        std::vector<std::uint32_t> visited;
        visited.reserve(count);
        auto point = region.first_vertex;
        for (std::size_t corner = 0; corner < count; ++corner) {
            if (std::find(visited.begin(), visited.end(), point) != visited.end())
                throw std::invalid_argument("Repeated native small ice contour point");
            visited.push_back(point);
            outline.push_back(graph.vertices[point]);
            const auto it = std::find_if(region_edges[region_index].begin(),
                region_edges[region_index].end(),
                [&](const auto &edge) { return edge[0] == point; });
            if (it == region_edges[region_index].end())
                throw std::invalid_argument("Open native small ice contour");
            point = (*it)[1];
        }
        if (point != region.first_vertex)
            throw std::invalid_argument("Unclosed native small ice contour");
        const double top_draw = next_draw();
        const float top_z = static_cast<float>(
            static_cast<double>(settings.height_min) +
            (static_cast<double>(settings.height_max) - settings.height_min) * top_draw);
        const double bottom_draw = next_draw();
        const float bottom_z = static_cast<float>(
            -(static_cast<double>(settings.depth_min) +
              (static_cast<double>(settings.depth_max) - settings.depth_min) * bottom_draw));
        const auto first = static_cast<std::uint32_t>(result.vertices.size());
        for (const auto &xy : outline)
            result.vertices.push_back(vertex(xy[0], xy[1], top_z));
        for (std::size_t corner = 0; corner < count; ++corner) {
            const auto &center = outline[corner];
            const auto &following = outline[(corner + 1) % count];
            const auto &preceding = outline[(corner + count - 1) % count];
            const auto xy = terrain_ice_skirt_xy(
                center, following, preceding, skirt_distance);
            result.vertices.push_back(vertex(xy[0], xy[1], bottom_z));
        }
        // Retail 0x6BA100 is shared with the large-ice face builder.
        std::vector<std::uint32_t> cap_vertices(count);
        for (std::uint32_t corner = 0; corner < count; ++corner)
            cap_vertices[corner] = first + corner;
        append_terrain_ice_cap_indices(outline, cap_vertices, result.indices);
        for (std::uint32_t corner = 0; corner < count; ++corner) {
            const auto following = static_cast<std::uint32_t>((corner + 1) % count);
            result.indices.insert(result.indices.end(),
                                  {first + static_cast<std::uint32_t>(count) + corner,
                                   first + following, first + corner,
                                   first + static_cast<std::uint32_t>(count) + corner,
                                   first + static_cast<std::uint32_t>(count) + following,
                                   first + following});
        }
    }
    result.local_random_end = random_seed;
    return result;
}
TerrainIceControlPoint build_terrain_ice_control_point(
    const TerrainIceContourEdge &previous, const TerrainIceContourEdge &center,
    const TerrainIceContourEdge &following, const TerrainIceCurveParameters &curve,
    const TerrainIceProjection &projection, TerrainIceRandomState &random,
    std::uint32_t x87_precision) {
    if (previous.side >= 6 || center.side >= 6 || following.side >= 6)
        throw std::invalid_argument("Invalid ice control-point edge side");
    if (x87_precision != 24 && x87_precision != 53)
        throw std::invalid_argument("Unsupported ice control-point x87 precision");
    const auto project = [&](const TerrainIceContourEdge &edge) {
        const auto p = ice_neighbor(edge.cube, edge.side);
        const auto &m = projection.world_matrix;
        const float x = static_cast<float>(p.x), y = static_cast<float>(p.y),
                    z = static_cast<float>(p.z);
        const float px = ((x * m[0] + m[8] * z) + m[4] * y) + m[12];
        const float py = ((x * m[1] + m[9] * z) + m[5] * y) + m[13];
        return std::array<float, 2>{projection.scale_x * px - projection.center_x,
                                    projection.scale_y * py - projection.center_y};
    };
    const auto p = project(previous), c = project(center), n = project(following);
    const auto value = [&](std::size_t index) -> float {
        if (index < 2 || index >= 38)
            throw std::invalid_argument("Invalid ice curve control index");
        return index < 26 ? curve.controls[index - 2] : curve.corners[index - 26];
    };
    const auto previous_side = (previous.side + 3u) % 6u;
    const auto following_side = (following.side + 3u) % 6u;
    const auto center_side = (center.side + 3u) % 6u + 1u;
    const float quarter = 0.25f, half = 0.5f, tenth = 0.1f;
    const float y_first = (value(center_side * 4 + 1) + c[1]) * quarter;
    const float y_third = (value(previous_side * 4 + 5) + p[1]) * quarter;
    const float y_middle = (value(previous_side * 4 + 3) + p[1]) * half;
    std::array<float, 4> out{
        ((c[0] + value(center_side * 4)) * quarter +
         (value(previous_side * 4 + 2) + p[0]) * half) +
         (value(previous_side * 4 + 4) + p[0]) * quarter,
        (y_first + y_middle) + y_third,
        ((value(previous_side * 4 + 2) + p[0]) * quarter +
         (value(previous_side * 4 + 4) + p[0]) * half) +
         (value(following_side * 4 + 2) + n[0]) * quarter,
        ((value(previous_side * 4 + 3) + p[1]) * quarter +
         (value(previous_side * 4 + 5) + p[1]) * half) +
         (value(following_side * 4 + 3) + n[1]) * quarter};
    const auto base = out;
    const auto draw = [&] {
        ++random.draws;
        random.value = random.value * 0x343fdu + 0x269ec3u;
        if (random.modulus) random.value %= random.modulus;
        return random.value;
    };
    const auto centered = [&](std::uint32_t value) {
        if (x87_precision == 24) {
            // FILD keeps a positive 32-bit integer exact. Negative signed values
            // take the native FADD 2^32 path, which rounds to the x87 PC24 width.
            const long double input = value & 0x80000000u
                ? static_cast<long double>(static_cast<float>(value))
                : static_cast<long double>(value);
            float centered24 = static_cast<float>(input *
                                                  static_cast<long double>(random.inverse_modulus));
            centered24 *= 2.0f;
            centered24 -= 1.0f;
            return static_cast<long double>(centered24);
        }
        return static_cast<long double>(value) *
               static_cast<long double>(random.inverse_modulus) * 2.0L - 1.0L;
    };
    const float noise_x1 = static_cast<float>(centered(draw()));
    out[0] = curve.radius * noise_x1 * tenth + out[0];
    const auto noise_y1 = centered(draw());
    out[1] = x87_precision == 24
        ? static_cast<float>(static_cast<float>(static_cast<float>(noise_y1) * curve.radius) * tenth + out[1])
        : static_cast<float>(noise_y1 * static_cast<long double>(curve.radius) *
            static_cast<long double>(tenth) + static_cast<long double>(out[1]));
    const float noise_x2 = static_cast<float>(centered(draw()));
    out[2] = curve.radius * noise_x2 * tenth + out[2];
    const auto noise_y2 = centered(draw());
    out[3] = x87_precision == 24
        ? static_cast<float>(static_cast<float>(static_cast<float>(noise_y2) * curve.radius) * tenth + out[3])
        : static_cast<float>(noise_y2 * static_cast<long double>(curve.radius) *
            static_cast<long double>(tenth) + static_cast<long double>(out[3]));
    return {base, out};
}
TerrainIceSplineSelection select_terrain_ice_spline_triplet(
    std::uint32_t point_count, float parameter, std::int32_t boundary) {
    if (point_count < 3 || point_count > INT32_MAX || !std::isfinite(parameter) ||
        parameter < 0 || parameter >= static_cast<float>(INT32_MAX) || boundary < 0)
        throw std::invalid_argument("Invalid ice spline parameter or point span");
    const auto integral = static_cast<std::int32_t>(parameter);
    std::int64_t first = static_cast<std::int64_t>(integral) - boundary;
    float local_t = parameter - std::floor(parameter);
    if (integral >= static_cast<std::int32_t>(point_count) - 2) {
        --first;
        local_t = 1.0f;
    }
    if (first < 0 || first + 2 >= point_count)
        throw std::invalid_argument("Ice spline triplet is outside its point span");
    return {static_cast<std::uint32_t>(first), local_t};
}
std::array<float, 2> interpolate_terrain_ice_spline_triplet(
    const std::array<float, 6> &points, float local_t) {
    const float u2 = local_t * local_t;
    const float w0 = (0.5f * u2 + -1.0f * local_t) + 0.5f;
    const float w1 = (-1.0f * u2 + local_t) + 0.5f;
    const float w2 = (0.5f * u2 + 0.0f * local_t) + 0.0f;
    return {(points[2] * w1 + points[0] * w0) + points[4] * w2,
            (points[3] * w1 + points[1] * w0) + points[5] * w2};
}
std::vector<TerrainIceExteriorVertex> build_terrain_ice_exterior_vertices(
    const TerrainIceContours &contours, const TerrainIceCurveParameters &curve,
    const TerrainIceProjection &projection) {
    const auto plan = plan_terrain_ice_mesh_flags(contours, curve.lattice_vertices_per_hex);
    constexpr float third = std::bit_cast<float>(0x3eaaaaabu);
    constexpr float two_thirds = std::bit_cast<float>(0x3f2aaaabu);
    static constexpr std::array<std::array<float, 2>, 6> side_corner{{
        {-third, two_thirds}, {-third, -third}, {two_thirds, -third},
        {third, -two_thirds}, {third, third}, {-two_thirds, third}}};
    const auto project = [&](const TerrainIceContourEdge &edge) {
        const auto &m = projection.world_matrix;
        const auto &corner = side_corner[edge.side];
        const float x = corner[0] + static_cast<float>(edge.cube.x);
        const float y = corner[1] + static_cast<float>(edge.cube.y);
        const float z = std::bit_cast<float>(std::bit_cast<std::uint32_t>(x) ^ 0x80000000u) - y;
        const float px = ((x * m[0] + m[8] * z) + m[4] * y) + m[12];
        const float py = ((x * m[1] + m[9] * z) + m[5] * y) + m[13];
        const float out_x = projection.scale_x * px - projection.center_x;
        float out_y = projection.scale_y * py - projection.center_y;
        if (edge.side == 0 || edge.side == 5) out_y += curve.radius;
        if (edge.side == 2 || edge.side == 3) out_y -= curve.radius;
        return std::array<float, 2>{out_x, out_y};
    };
    std::vector<TerrainIceExteriorVertex> result;
    std::size_t vertex = 0;
    for (const auto &segment : contours.segments) {
        const auto count = static_cast<std::size_t>(segment.edge_count);
        std::vector<std::uint8_t> inside(count);
        for (std::size_t i = 0; i < count; ++i) {
            const auto &edge = contours.edges[segment.first_edge + i];
            inside[i] = contains(contours.grid,
                offset(ice_neighbor(edge.cube, edge.side), contours.grid.coordinates),
                contours.grid.wrap_x) ? 1 : 0;
        }
        std::size_t first = 0;
        for (std::size_t i = count; i != 0; --i)
            if (inside[i - 1] != inside[0]) {
                first = i % count;
                break;
            }
        for (std::size_t processed = 0; processed < count; ++processed) {
            const auto i = (first + processed) % count;
            if (inside[i]) {
                vertex += curve.lattice_vertices_per_hex;
            } else {
                if (vertex >= plan.flags.size() || plan.flags[vertex] != 0)
                    throw std::invalid_argument("Ice exterior vertex order differs from flag plan");
                result.push_back({static_cast<std::uint32_t>(vertex),
                                  static_cast<std::uint32_t>(segment.first_edge + i),
                                  project(contours.edges[segment.first_edge + i])});
                ++vertex;
            }
        }
    }
    if (vertex != plan.flags.size())
        throw std::invalid_argument("Ice exterior vertices omit the mesh tail");
    return result;
}
TerrainIceMeshGeometry build_terrain_ice_mesh_geometry(
    const TerrainIceContours &contours, const TerrainIceCurveParameters &curve,
    const TerrainIceProjection &projection, TerrainIceRandomState &random,
    std::uint32_t x87_precision) {
    const auto plan = plan_terrain_ice_mesh_flags(contours, curve.lattice_vertices_per_hex);
    TerrainIceMeshGeometry result{plan.segments,
                                  std::vector<std::array<float, 2>>(plan.flags.size()),
                                  plan.flags};
    for (const auto &exterior : build_terrain_ice_exterior_vertices(contours, curve, projection))
        result.vertices[exterior.index] = exterior.position;
    std::size_t next_vertex = 0;
    for (const auto &segment : contours.segments) {
        const auto count = static_cast<std::size_t>(segment.edge_count);
        std::vector<std::uint8_t> inside(count);
        for (std::size_t i = 0; i < count; ++i) {
            const auto &edge = contours.edges[segment.first_edge + i];
            inside[i] = contains(contours.grid,
                offset(ice_neighbor(edge.cube, edge.side), contours.grid.coordinates),
                contours.grid.wrap_x) ? 1 : 0;
        }
        std::size_t first = 0;
        for (std::size_t i = count; i != 0; --i)
            if (inside[i - 1] != inside[0]) {
                first = i % count;
                break;
            }
        for (std::size_t processed = 0; processed < count;) {
            const auto value = inside[first];
            std::size_t run = 1;
            while (processed + run < count && inside[(first + run) % count] == value)
                ++run;
            if (!value) {
                next_vertex += run;
            } else {
                std::vector<std::array<float, 2>> scratch;
                scratch.reserve(2 * (run + 1));
                for (std::size_t edge_index = 0; edge_index <= run; ++edge_index) {
                    const auto current_index = (first + edge_index) % count;
                    const auto preceding_index = (current_index + count - 1) % count;
                    const auto following_index = (current_index + 1) % count;
                    // 87AC40 passes current in ECX, preceding on the stack,
                    // and following in EAX, including the closing call.
                    const auto control = build_terrain_ice_control_point(
                        contours.edges[segment.first_edge + current_index],
                        contours.edges[segment.first_edge + preceding_index],
                        contours.edges[segment.first_edge + following_index],
                        curve, projection, random, x87_precision);
                    scratch.push_back({control.displaced[0], control.displaced[1]});
                    scratch.push_back({control.displaced[2], control.displaced[3]});
                }
                const auto point_count = static_cast<std::uint32_t>(2 * run);
                const auto output_count = run * curve.lattice_vertices_per_hex;
                const float step = static_cast<float>(
                    (static_cast<long double>(point_count) - 2.0L) /
                    static_cast<long double>(output_count));
                float parameter = 0;
                for (std::size_t output = 0; output < output_count; ++output) {
                    const auto selected = select_terrain_ice_spline_triplet(
                        point_count, parameter);
                    const auto index = selected.first_point;
                    const std::array<float, 6> points{
                        scratch[index][0], scratch[index][1],
                        scratch[index + 1][0], scratch[index + 1][1],
                        scratch[index + 2][0], scratch[index + 2][1]};
                    result.vertices[next_vertex + output] =
                        interpolate_terrain_ice_spline_triplet(points, selected.local_t);
                    parameter += step;
                }
                next_vertex += output_count;
            }
            processed += run;
            first = (first + run) % count;
        }
    }
    if (next_vertex != result.vertices.size())
        throw std::invalid_argument("Ice mesh geometry disagrees with the flag plan");
    return result;
}
TerrainIceProjection terrain_ice_projection(const WorldLayout &world,
                                            const HeightOwnerParameters &owner) {
    if (world.source_grid.width <= 0 || world.source_grid.height <= 0 ||
        world.worker_grid.width <= 0 || world.worker_grid.height <= 0 ||
        static_cast<std::int64_t>(world.worker_grid.width) * 64 != owner.width ||
        static_cast<std::int64_t>(world.worker_grid.height) * 64 != owner.height ||
        !std::isfinite(owner.scale_x) || !std::isfinite(owner.scale_y) ||
        !std::isfinite(owner.center_x) || !std::isfinite(owner.center_y) ||
        owner.scale_x == 0 || owner.scale_y == 0)
        throw std::invalid_argument("Invalid terrain ice world projection");
    TerrainIceProjection projection;
    std::copy_n(world.transforms.begin() + 16, 16, projection.world_matrix.begin());
    for (const auto value : projection.world_matrix)
        if (!std::isfinite(value))
            throw std::invalid_argument("Non-finite terrain ice world matrix");
    projection.scale_x = owner.scale_x;
    projection.scale_y = owner.scale_y;
    projection.center_x = owner.center_x;
    projection.center_y = owner.center_y;
    return projection;
}
std::vector<std::array<float, 2>> project_terrain_ice_group_vertices(
    const TerrainIceContours &contours, const TerrainIceProjection &projection) {
    std::vector<std::array<float, 2>> result(contours.vertices.size());
    std::size_t covered = 0;
    const auto &m = projection.world_matrix;
    for (const auto &group : contours.groups) {
        if (group.first_vertex != covered ||
            group.vertex_count > result.size() - covered)
            throw std::invalid_argument("Ice group vertex spans are not contiguous");
        for (std::size_t i = 0; i < group.vertex_count; ++i) {
            const auto &cube = contours.vertices[covered + i];
            const float x = static_cast<float>(cube.x);
            const float y = static_cast<float>(cube.y);
            const float z = static_cast<float>(cube.z);
            const float world_x = ((x * m[0] + z * m[8]) + y * m[4]) + m[12];
            const float world_y = ((x * m[1] + z * m[9]) + y * m[5]) + m[13];
            result[covered + i] = {
                projection.scale_x * world_x - projection.center_x,
                projection.scale_y * world_y - projection.center_y};
        }
        covered += group.vertex_count;
    }
    if (covered != result.size())
        throw std::invalid_argument("Ice group vertex spans omit positions");
    return result;
}
std::array<float, 4> bound_terrain_ice_mesh_geometry(
    std::span<const std::array<float, 2>> vertices) {
    constexpr float largest = std::bit_cast<float>(0x7f7fffffu);
    constexpr float smallest = std::bit_cast<float>(0xff7fffffu);
    std::array<float, 4> bounds{largest, largest, smallest, smallest};
    for (const auto &point : vertices) {
        // COMISS selects the new value on equality and unordered comparison;
        // retain that selection rather than std::min/std::max's tie behavior.
        if (!(point[0] > bounds[0])) bounds[0] = point[0];
        if (!(bounds[2] > point[0])) bounds[2] = point[0];
        if (!(point[1] > bounds[1])) bounds[1] = point[1];
        if (!(bounds[3] > point[1])) bounds[3] = point[1];
    }
    return bounds;
}
std::array<std::uint8_t, 8> pack_terrain_ice_vertex(
    const std::array<float, 4> &component_box,
    const std::array<std::uint8_t, 16> &source,
    float height_scale, float height_offset) {
    static_assert(std::endian::native == std::endian::little);
    const auto input = std::bit_cast<std::array<float, 4>>(source);
    if (!(std::ranges::all_of(component_box, [](float v) { return std::isfinite(v); }) &&
          std::isfinite(input[0]) && std::isfinite(input[1]) && std::isfinite(input[2]) &&
          std::isfinite(height_scale) && std::isfinite(height_offset) &&
          component_box[2] > component_box[0] &&
          component_box[3] > component_box[1]))
        throw std::invalid_argument("Invalid native ice vertex or component bounds");
    const auto packed_xy = [](float value, float low, float high) {
        float unit = (value - low) / (high - low);
        if (!(unit > 0.0f)) unit = 0.0f;
        else if (unit > 1.0f) unit = 1.0f;
        return static_cast<std::int16_t>(static_cast<std::int32_t>(unit * 65535.0f) - 32768);
    };
    const auto x = packed_xy(input[0], component_box[0], component_box[2]);
    const auto y = packed_xy(input[1], component_box[1], component_box[3]);
    const float z = ((input[2] + height_offset) /
                     (height_scale + height_offset)) * 255.0f;
    // CVTTSS2SI returns INT32_MIN for NaN, infinity and overflow. Its low
    // byte is zero, including the retail selected save's 0/0 height setup.
    const auto height = (std::isfinite(z) && z >= static_cast<float>(INT32_MIN) &&
                         z < 2147483648.0f)
        ? static_cast<std::uint8_t>(static_cast<std::int32_t>(z))
        : std::uint8_t{0};
    const auto unsigned_x = static_cast<std::uint16_t>(x);
    const auto unsigned_y = static_cast<std::uint16_t>(y);
    return {static_cast<std::uint8_t>(unsigned_x),
            static_cast<std::uint8_t>(unsigned_x >> 8),
            static_cast<std::uint8_t>(unsigned_y),
            static_cast<std::uint8_t>(unsigned_y >> 8),
            source[12], source[13], source[14], height};
}
void prepare_terrain_ice_vertex_normals(
    std::span<std::array<std::uint8_t, 16>> vertices,
    std::span<const std::uint32_t> indices) {
    if (indices.size() % 3)
        throw std::invalid_argument("Invalid native ice normal triangle count");
    std::vector<std::array<float, 3>> positions;
    positions.reserve(vertices.size());
    for (const auto &source : vertices) {
        const auto value = std::bit_cast<std::array<float, 4>>(source);
        if (!std::isfinite(value[0]) || !std::isfinite(value[1]) ||
            !std::isfinite(value[2]))
            throw std::invalid_argument("Non-finite native ice vertex position");
        positions.push_back({value[0], value[1], value[2]});
    }
    for (auto index : indices)
        if (index >= vertices.size())
            throw std::invalid_argument("Native ice normal index escaped vertices");
    std::vector<std::array<float, 4>> sums(vertices.size());
    const auto corner_weight = [](const std::array<float, 3> &center,
                                  const std::array<float, 3> &first,
                                  const std::array<float, 3> &second) {
        std::array<float, 3> a{}, b{};
        for (unsigned lane = 0; lane < 3; ++lane) {
            a[lane] = first[lane] - center[lane];
            b[lane] = second[lane] - center[lane];
        }
        const double length_a = std::sqrt(
            static_cast<double>(a[2]) * a[2] +
            static_cast<double>(a[0]) * a[0] +
            static_cast<double>(a[1]) * a[1]);
        const double length_b = std::sqrt(
            static_cast<double>(b[2]) * b[2] +
            static_cast<double>(b[1]) * b[1] +
            static_cast<double>(b[0]) * b[0]);
        // Retail 0x879112/0x879199 divides by the x87 square root before
        // storing the reciprocal to Float32. Rounding the length first
        // changes a few packed normals on the authored-height surface.
        const float inv_a = static_cast<float>(1.0 / length_a);
        const float inv_b = static_cast<float>(1.0 / length_b);
        for (unsigned lane = 0; lane < 3; ++lane) {
            a[lane] *= inv_a;
            b[lane] *= inv_b;
        }
        const double dot = ((static_cast<double>(a[2]) * b[2] +
                             static_cast<double>(a[1]) * b[1]) +
                             static_cast<double>(a[0]) * b[0]);
        return static_cast<float>(std::abs(std::acos(dot)) *
                                  static_cast<double>(0.3183099031448364f));
    };
    for (std::size_t triangle = 0; triangle < indices.size(); triangle += 3) {
        const auto ia = indices[triangle];
        const auto ib = indices[triangle + 1];
        const auto ic = indices[triangle + 2];
        const auto &a = positions[ia];
        const auto &b = positions[ib];
        const auto &c = positions[ic];
        const float bax = b[0] - a[0], bay = b[1] - a[1], baz = b[2] - a[2];
        const float cax = c[0] - a[0], cay = c[1] - a[1], caz = c[2] - a[2];
        const std::array<float, 3> cross{
            bay * caz - baz * cay,
            baz * cax - caz * bax,
            cay * bax - bay * cax};
        const double magnitude = std::sqrt(
            static_cast<double>(cross[0]) * cross[0] +
            static_cast<double>(cross[1]) * cross[1] +
            static_cast<double>(cross[2]) * cross[2]);
        if (!(magnitude > static_cast<double>(0.05000000074505806f))) continue;
        const float inv = 1.0f / static_cast<float>(magnitude);
        const std::array<float, 3> normal{
            cross[0] * inv, cross[1] * inv, cross[2] * inv};
        const std::array<float, 3> angles{
            corner_weight(a, c, b),
            corner_weight(b, c, a),
            corner_weight(c, a, b)};
        for (unsigned corner = 0; corner < 3; ++corner) {
            auto &sum = sums[indices[triangle + corner]];
            for (unsigned lane = 0; lane < 3; ++lane)
                sum[lane] += normal[lane] * angles[corner];
            sum[3] += angles[corner];
        }
    }
    for (std::size_t index = 0; index < vertices.size(); ++index) {
        const auto &sum = sums[index];
        const float inv_weight = 1.0f / sum[3];
        const std::array<float, 3> average{
            sum[0] * inv_weight, sum[1] * inv_weight, sum[2] * inv_weight};
        const double norm = std::sqrt(
            static_cast<double>(average[2]) * average[2] +
            static_cast<double>(average[1]) * average[1] +
            static_cast<double>(average[0]) * average[0]);
        const float inv_norm = static_cast<float>(1.0 / norm);
        for (unsigned lane = 0; lane < 3; ++lane) {
            float unit = average[lane] * inv_norm;
            unit = unit * 0.5f + 0.5f;
            if (!(unit > 0.0f)) unit = 0.0f;
            else if (unit > 1.0f) unit = 1.0f;
            vertices[index][12 + lane] = static_cast<std::uint8_t>(
                static_cast<std::int32_t>(unit * 255.0f));
        }
    }
}
std::vector<TerrainIcePackedRegion> partition_terrain_ice_mesh(
    const std::array<float, 4> &root_box,
    const std::array<float, 4> &component_box,
    std::span<const std::array<std::uint8_t, 16>> vertices,
    std::span<const std::uint32_t> indices,
    float height_scale, float height_offset) {
    if (indices.size() % 3 || vertices.empty() || indices.empty() ||
        vertices.size() > std::numeric_limits<std::uint32_t>::max() ||
        indices.size() > std::numeric_limits<std::uint32_t>::max() ||
        !std::ranges::all_of(root_box, [](float v) { return std::isfinite(v); }) ||
        root_box[0] >= root_box[2] || root_box[1] >= root_box[3])
        throw std::invalid_argument("Invalid native ice partition source");
    std::vector<std::array<float, 2>> xy;
    xy.reserve(vertices.size());
    for (const auto &source : vertices) {
        const auto floats = std::bit_cast<std::array<float, 4>>(source);
        if (!std::isfinite(floats[0]) || !std::isfinite(floats[1]))
            throw std::invalid_argument("Non-finite native ice source position");
        xy.push_back({floats[0], floats[1]});
    }
    std::vector<std::uint32_t> order(indices.begin(), indices.end());
    for (auto index : order)
        if (index >= vertices.size())
            throw std::invalid_argument("Native ice triangle escaped the source mesh");
    std::vector<std::uint16_t> scratch(vertices.size());
    std::vector<TerrainIcePackedRegion> regions;
    const auto visit = [&](auto &&self, std::size_t begin, std::size_t end,
                           std::uint32_t unique, std::array<float, 4> bounds,
                           unsigned depth) -> void {
        if (depth > 64 || begin == end)
            throw std::invalid_argument("Native ice partition failed to converge");
        if (unique < 65535) {
            TerrainIcePackedRegion region;
            region.bounds = bounds;
            region.indices.reserve(end - begin);
            for (auto i = begin; i < end; ++i) {
                const auto index = order[i];
                auto &mapped = scratch[index];
                if (!mapped) {
                    region.vertices.push_back(pack_terrain_ice_vertex(
                        component_box, vertices[index], height_scale, height_offset));
                    mapped = static_cast<std::uint16_t>(region.vertices.size());
                }
                region.indices.push_back(static_cast<std::uint16_t>(mapped - 1));
            }
            for (auto i = begin; i < end; ++i) scratch[order[i]] = 0;
            regions.push_back(std::move(region));
            return;
        }
        const float width = bounds[2] - bounds[0];
        const float height = bounds[3] - bounds[1];
        const unsigned axis = width > height ? 0u : 1u;
        const float midpoint = (bounds[axis] + bounds[axis + 2]) * 0.5f;
        constexpr float largest = std::numeric_limits<float>::max();
        std::array<float, 4> first{largest, largest, -largest, -largest};
        std::array<float, 4> second = first;
        std::uint32_t first_unique = 0, second_unique = 0;
        std::size_t left = begin, right = end;
        while (left < right) {
            const auto a = xy[order[left]];
            const auto b = xy[order[left + 1]];
            const auto c = xy[order[left + 2]];
            const std::array<float, 4> triangle{
                std::min({a[0], b[0], c[0]}),
                std::min({a[1], b[1], c[1]}),
                std::max({a[0], b[0], c[0]}),
                std::max({a[1], b[1], c[1]})};
            const float center = (triangle[axis] + triangle[axis + 2]) * 0.5f;
            const bool front = center <= midpoint;
            auto &selected = front ? first : second;
            selected[0] = std::min(selected[0], triangle[0]);
            selected[1] = std::min(selected[1], triangle[1]);
            selected[2] = std::max(selected[2], triangle[2]);
            selected[3] = std::max(selected[3], triangle[3]);
            const std::uint16_t bit = front ? 1u : 2u;
            auto &count = front ? first_unique : second_unique;
            for (unsigned lane = 0; lane < 3; ++lane) {
                auto &mark = scratch[order[left + lane]];
                if (!(mark & bit)) { mark |= bit; ++count; }
            }
            if (front) left += 3;
            else {
                right -= 3;
                for (unsigned lane = 0; lane < 3; ++lane)
                    std::swap(order[left + lane], order[right + lane]);
            }
        }
        for (auto i = begin; i < end; ++i) scratch[order[i]] = 0;
        if (left == begin || left == end ||
            first_unique >= unique || second_unique >= unique)
            throw std::invalid_argument("Native ice partition did not split the mesh");
        self(self, begin, left, first_unique, first, depth + 1);
        self(self, left, end, second_unique, second, depth + 1);
    };
    visit(visit, 0, order.size(), static_cast<std::uint32_t>(vertices.size()),
          root_box, 0);
    return regions;
}
std::vector<TerrainIcePackedRegion> build_terrain_ice_gpu_regions(
    const std::array<float, 4> &component_box,
    std::span<std::array<std::uint8_t, 16>> combined_vertices,
    std::span<const std::uint32_t> indices,
    float height_scale, float height_offset) {
    prepare_terrain_ice_vertex_normals(combined_vertices, indices);
    return partition_terrain_ice_mesh(component_box, component_box,
                                      combined_vertices, indices,
                                      height_scale, height_offset);
}
TerrainIceCompleteMesh build_terrain_ice_complete_mesh(
    const TerrainIceIntermediateRegionGeometry &geometry,
    const std::array<float, 4> &component_box,
    const TerrainIceSmallMeshSettings &small_settings,
    const TerrainIceLargeTopSettings &large_settings,
    std::span<const std::uint8_t> displacement_pixels,
    std::uint32_t displacement_width, std::uint32_t displacement_height,
    std::uint32_t displacement_pitch,
    float large_skirt_slope, float large_depth,
    std::uint32_t random_seed) {
    if (std::bit_cast<std::uint32_t>(small_settings.scale) !=
            std::bit_cast<std::uint32_t>(large_settings.scale) ||
        !std::isfinite(small_settings.scale) || small_settings.scale <= 0.0f)
        throw std::invalid_argument("Ice mesh scales differ");
    if (!std::isfinite(component_box[0]) || !std::isfinite(component_box[1]) ||
        !std::isfinite(component_box[2]) || !std::isfinite(component_box[3]) ||
        component_box[2] <= component_box[0] ||
        component_box[3] <= component_box[1])
        throw std::invalid_argument("Invalid ice component box");
    auto source = select_terrain_ice_large_source_graph(
        geometry, small_settings.max_polygon_vertices);
    TerrainIceCompleteMesh result;
    // Retail computes this box before small-region pruning. Recomputing it
    // from the selected large graph loses extrema owned by small regions.
    result.component_box = component_box;
    result.combined = build_terrain_ice_small_mesh(
        geometry, small_settings, random_seed);
    if (source.points.empty()) return result;
    const auto graph = construct_terrain_ice_refined_graph(
        source, small_settings.scale / 12.0f, &component_box);
    if (result.combined.vertices.size() > UINT32_MAX)
        throw std::invalid_argument("Oversized small ice mesh");
    const auto large_indices = build_terrain_ice_large_indices(
        graph, static_cast<std::uint32_t>(result.combined.vertices.size()));
    const auto top = build_terrain_ice_large_top_vertices(
        graph, displacement_pixels, displacement_width,
        displacement_height, displacement_pitch, large_settings);
    const auto border = build_terrain_ice_large_border_vertices(
        graph, small_settings.scale, large_skirt_slope, large_depth);
    result.combined.vertices.reserve(result.combined.vertices.size() +
                                     top.size() + border.size());
    for (const auto &vertex : top)
        result.combined.vertices.push_back(
            std::bit_cast<std::array<std::uint8_t, 16>>(vertex));
    for (const auto &vertex : border)
        result.combined.vertices.push_back(
            std::bit_cast<std::array<std::uint8_t, 16>>(vertex));
    result.combined.indices.insert(result.combined.indices.end(),
                                   large_indices.begin(), large_indices.end());
    return result;
}
TerrainAuxiliaryOwnerState apply_terrain_auxiliary_flags(const TerrainAuxiliaryOwnerState &prior,
    const TerrainAuxiliarySources &sources) {
    validate_storage(prior.relief); validate_storage(prior.adjacency); validate_storage(prior.water);
    validate_storage(prior.ice); validate_storage(prior.flood);
    // Inactive original handlers return before reading their grid headers.
    if (!sources.relief.empty()) {
        if (prior.relief_enabled) { validate(prior.relief); validate(prior.adjacency); }
        if (prior.water_enabled && std::any_of(sources.relief.begin(), sources.relief.end(),
                [](const auto &p) { return p.kind == 0; })) validate(prior.water);
    }
    if (!sources.feature0.empty()) validate(prior.ice);
    if (!sources.feature4.empty()) validate(prior.flood);
    auto result = prior;
    for (const auto p : sources.relief) apply_relief(result, p);
    // 717290 wraps only X when enabled and performs no logical bounds check.
    for (const auto p : sources.feature0)
        physical(result.ice, offset(p, result.ice.coordinates), result.ice.wrap_x)[0] |= 0x80;
    for (const auto p : sources.feature4) {
        const auto cell = offset(p, result.flood.coordinates);
        if (contains(result.flood, cell, false)) physical(result.flood, cell, false)[0] |= 2;
        result.flood_state_flag = false; // also cleared for rejected positions.
    }
    return result;
}
TerrainAdjacencyGrid finalize_terrain_auxiliary_adjacency(const TerrainAdjacencyGrid &prior) {
    validate(prior);
    auto result = prior;
    const auto &m = prior.coordinates.second_matrix;
    // A2D44C indexes the C7FB08 cube-neighbor table. A2D45C selects the
    // three shared-corner flag bytes for each of the two output bytes.
    static constexpr LogicalHex directions[]{{0, 1, -1}, {-1, 1, 0},
                                              {0, -1, 1}, {1, -1, 0}};
    static constexpr unsigned corners[][3]{{0, 1, 2}, {2, 0, 1}};
    for (std::int32_t x = 0; x < prior.coordinates.width; ++x)
        for (std::int32_t y = 0; y < prior.coordinates.height; ++y) {
            const auto a = wrap_add(x, wrap_neg(y >> 1));
            const auto c = wrap_neg(wrap_add(a, y));
            const auto transform = [&](unsigned i) {
                return wrap_add(wrap_add(wrap_add(wrap_mul(a, m[i]), wrap_mul(y, m[4+i])),
                                         wrap_mul(c, m[8+i])), m[12+i]);
            };
            LogicalHex position{transform(0), transform(1), 0};
            position.z = wrap_neg(wrap_add(position.x, position.y));
            auto &center = physical(result, {x, y}, result.wrap_x);
            for (unsigned corner = 0; corner < 2; ++corner) {
                const auto d0 = directions[corner*2], d1 = directions[corner*2+1];
                const auto p0 = LogicalHex{wrap_add(position.x, d0.x), wrap_add(position.y, d0.y),
                                           wrap_add(position.z, d0.z)};
                const auto p1 = LogicalHex{wrap_add(position.x, d1.x), wrap_add(position.y, d1.y),
                                           wrap_add(position.z, d1.z)};
                const auto &n0 = bounded(result, p0);
                const auto &n1 = bounded(result, p1);
                const std::uint8_t f0 = center[6+corners[corner][0]];
                const std::uint8_t f1 = n0[6+corners[corner][1]];
                const std::uint8_t f2 = n1[6+corners[corner][2]];
                const bool has11 = ((f0 | f1 | f2) & 0x11) != 0;
                const bool has2 = ((f0 | f1 | f2) & 0x02) != 0;
                const bool all8 = (f0 & f1 & f2 & 0x08) != 0;
                const bool has10 = ((f0 | f1 | f2) & 0x10) != 0;
                auto &output = center[4+corner];
                if (all8) output = 0x08;
                else if (has11 && has2) output = 0x20;
                else if (has2) output = 0x02;
                if (has10) output |= 0x10;
            }
        }
    return result;
}
TerrainAuxiliaryInitializedTables initialize_terrain_auxiliary_tables(
    const TerrainAuxiliaryTwoByteGrid &two_byte, const TerrainAdjacencyGrid &adjacency,
    std::array<std::uint8_t, 12> scratch) {
    validate_storage(two_byte);
    validate_storage(adjacency);
    TerrainAuxiliaryInitializedTables result{two_byte, adjacency};
    result.two_byte.coordinates.width = result.two_byte.coordinates.height = 255;
    result.adjacency.coordinates.width = result.adjacency.coordinates.height = 255;
    const std::array<std::uint8_t, 2> mask_record{0xff, 0x7f};
    std::fill(result.two_byte.cells.begin(), result.two_byte.cells.end(), mask_record);
    const std::uint32_t first_word =
        (std::uint32_t(scratch[0]) | (std::uint32_t(scratch[1]) << 8) |
         (std::uint32_t(scratch[2]) << 16) | (std::uint32_t(scratch[3]) << 24)) & 0xfffffe00u;
    const std::array<std::uint8_t, 12> adjacency_record{
        static_cast<std::uint8_t>(first_word), static_cast<std::uint8_t>(first_word >> 8),
        static_cast<std::uint8_t>(first_word >> 16), static_cast<std::uint8_t>(first_word >> 24),
        1, 1, 1, 1, 1, scratch[9], scratch[10], scratch[11]};
    std::fill(result.adjacency.cells.begin(), result.adjacency.cells.end(), adjacency_record);
    result.adjacency.fallback = adjacency_record;
    return result;
}
} // namespace civ5::terrain
