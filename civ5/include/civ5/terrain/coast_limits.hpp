#pragma once
#include <cstddef>

namespace civ5::terrain {
// Native graph: three owned edges per cell. Each edge is visited once by the
// contour scan, so E bounds the contour count C. Each contour repeats two
// closing controls; subdivision produces 8*n-14 controls on each band.
inline constexpr std::size_t max_coast_graph_edges = 255u * 255u * 3u;
inline constexpr std::size_t max_coast_contours = max_coast_graph_edges;
inline constexpr std::size_t max_coast_contour_controls = 2u * max_coast_graph_edges + 2u;
inline constexpr std::size_t max_coast_curve_controls = max_coast_contour_controls * 8u - 14u;
// Across contours: sum(n)=2*E+2*C, sum(8*n-14)=16*E+2*C <= 18*E.
// Worker geometry reserves three point ranges per third curve. Its U32 range
// offsets are distinct from the strip's chunk-local U16 triangle indices.
inline constexpr std::size_t max_coast_worker_points = 54u * max_coast_graph_edges;
} // namespace civ5::terrain
