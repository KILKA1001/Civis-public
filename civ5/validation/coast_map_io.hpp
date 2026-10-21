#pragma once
#include "civ5/terrain/coast_map.hpp"
#include "civ5/terrain/coast_perturbation.hpp"
#include "coast_asset_io.hpp"
namespace {
void point(Writer &w, Point2 p) {
    w.scalar(p.x);
    w.scalar(p.y);
}
void controls(Writer &w, std::span<const CoastControlPoint> p) {
    Writer body;
    for (auto v : p) {
        body.scalar(v.x);
        body.scalar(v.y);
        body.scalar(v.transition);
        body.bytes.push_back(v.style_a);
        body.bytes.push_back(v.style_b);
    }
    w.blob(body.bytes);
}
std::vector<CoastControlPoint> controls(Reader &r) {
    Reader p{r.blob()};
    check(p.bytes.size() % 14 == 0 && p.bytes.size() / 14 >= 4 && p.bytes.size() / 14 <= max_coast_contour_controls,
          "Invalid coast map controls");
    std::vector<CoastControlPoint> out;
    while (p.at < p.bytes.size()) {
        const auto x = p.scalar(), y = p.scalar(), t = p.scalar();
        const auto styles = p.take(2);
        out.push_back({x, y, t, styles[0], styles[1]});
    }
    return out;
}
bool flag(Reader &r) {
    const auto v = r.word();
    check(v <= 1, "Invalid coast map boolean");
    return v != 0;
}
struct MapInput {
    CoastMapGraph graph;
    X87Precision pc;
    bool scan;
};
MapInput map_input(Reader &r) {
    const auto width = r.integer(), height = r.integer();
    const auto pc = precision(r);
    const bool sse = flag(r), wrap = flag(r), scan = flag(r);
    const auto masks = palette(r);
    IntMatrix4 m1{}, m2{};
    Matrix4 world{};
    for (auto &v : m1)
        v = r.integer();
    for (auto &v : m2)
        v = r.integer();
    for (auto &v : world)
        v = r.scalar();
    r.end();
    return {CoastMapGraph::build(
                {width, height, masks, m1, m2, world, wrap, pc, sse ? Vec4Path::sse : Vec4Path::legacy_x87}),
            pc, scan};
}
void initial(Writer &w, const CoastSegmentGrid &g) {
    point(w, g.wrap_delta());
    for (auto pair : g.offsets())
        for (auto p : pair)
            point(w, p);
    point(w, g.origin());
    for (auto p : g.corners())
        point(w, p);
    Writer cells;
    for (const auto &cell : g.cells()) {
        const auto c = cell.classification;
        for (auto b : {c.category, c.style, c.displacement_flag, c.displacement_flag, c.displacement_flag,
                       std::uint8_t(cell.segments.size()), std::uint8_t(cell.opposite_directions.size())})
            cells.bytes.push_back(b);
        cells.raw(cell.opposite_directions);
        for (auto s : cell.segments) {
            cells.bytes.push_back(s.direction);
            cells.bytes.push_back(s.style);
            point(cells, s.first);
            point(cells, s.second);
        }
    }
    w.blob(cells.bytes);
}
std::vector<CoastContour> graph_output(Writer &w, const MapInput &input) {
    const auto &graph = input.graph;
    initial(w, graph.initial());
    Writer links;
    for (const auto &cell : graph.links())
        for (unsigned k = 0; k < 3; ++k)
            for (bool reverse : {false, true}) {
                // Canonical native initialization only; unused coordinates/padding omitted.
                links.bytes.push_back(0);
                if (k >= cell.size()) {
                    links.bytes.push_back(255);
                    continue;
                }
                const auto n = reverse ? cell[k].reverse : cell[k].forward;
                links.bytes.push_back(std::uint8_t(n.edge));
                links.word(n.x);
                links.word(n.y);
            }
    w.blob(links.bytes);
    w.word(10);
    const auto geometry =
        graph.smooth(input.pc, [&](unsigned stage, unsigned mode, const CoastSegmentGrid &g) {
            w.word(stage);
            w.word(mode);
            Writer points;
            for (const auto &cell : g.cells())
                for (auto s : cell.segments) {
                    point(points, s.first);
                    point(points, s.second);
                }
            w.blob(points.bytes);
        });
    const auto contours = input.scan ? graph.scan(geometry) : std::vector<CoastContour>{};
    w.word(input.scan);
    w.word(static_cast<std::uint32_t>(contours.size()));
    for (const auto &c : contours) {
        w.word(c.start_x);
        w.word(c.start_y);
        w.word(c.start_edge);
        w.word(c.closed);
        w.word(c.owner_flag);
        w.word(c.family);
        if (c.wrap_offset)
            point(w, *c.wrap_offset);
        controls(w, c.points);
    }
    return contours;
}
void perturbation(Writer &w, const CoastPerturbation &p) {
    w.word(p.levels);
    w.word(p.padded_point_count);
    w.word(p.random_calls);
    w.scalar(p.initial_amplitude);
    controls(w, p.points);
}
Bytes run_map(unsigned kind, Reader &r, const Library &lib, std::span<const std::uint8_t> worker,
              std::span<const std::uint8_t> gates, const fs::path &prefix,
              const civ5::validation::WorkerCompositionSource *computed_source = nullptr) {
    Writer out;
    if (kind == 0) {
        const auto input = map_input(r);
        graph_output(out, input);
    } else if (kind == 1) {
        const auto pc = precision(r);
        const auto path = flag(r) ? CoastPowPath::sse2 : CoastPowPath::legacy_x87;
        const auto closed = flag(r);
        const auto wx = r.scalar(), wy = r.scalar();
        const auto p = controls(r);
        const auto randoms = palette(r);
        r.end();
        perturbation(out, perturb_coast(p, closed, {wx, wy}, randoms, pc, path));
    } else {
        check(kind == 2, "Invalid coast map operation");
        Reader map{r.blob()};
        const auto input = map_input(map);
        check(input.scan, "Connected map requires contour scan");
        const auto path = flag(r) ? CoastPowPath::sse2 : CoastPowPath::legacy_x87;
        const auto randoms = palette(r), keys = palette(r);
        const auto route = flag(r);
        const auto dw = r.integer(), dh = r.integer();
        const auto gate = r.word(), helper = r.word();
        check(gate <= 65535, "Invalid coast map dispatch gate");
        const auto dispatch_randoms = palette(r), dispatch_keys = palette(r);
        r.end();
        Writer graph;
        const auto contours = graph_output(graph, input);
        out.blob(graph.bytes);
        std::array<std::vector<CoastCurveSet>, 2> curves;
        std::array<std::vector<CoastStrip>, 2> strips;
        out.word(static_cast<std::uint32_t>(contours.size()));
        std::size_t cursor = 0;
        for (const auto &c : contours) {
            const auto perturbed = perturb_coast(c.points, c.closed, c.wrap_offset.value_or(Point2{0, 0}),
                                                 std::span(randoms).subspan(cursor), input.pc, path);
            cursor += perturbed.random_calls;
            perturbation(out, perturbed);
            for (unsigned loader = 0; loader < 2; ++loader) {
                curves[loader].push_back(
                    build_coast_curves(perturbed.points, lib.profiles, lib.characters[loader],
                                       {c.closed, c.wrap_offset.value_or(Point2{0, 0}), c.owner_flag, keys,
                                        0x110000, 0x100000, input.pc}));
                strips[loader].push_back(
                    build_coast_strip(curves[loader].back(), lib.profiles, keys, 0x110000, 0x100000));
                write_curves(out, curves[loader].back());
                write_strip(out, strips[loader].back());
            }
        }
        out.word(static_cast<std::uint32_t>(cursor));
        check(cursor == randoms.size(), "Connected map did not consume complete explicit native rand stream");
        if (route) {
            Writer full;
            full.word(static_cast<std::uint32_t>(contours.size()));
            for (std::size_t i = 0; i < contours.size(); ++i) {
                write_curves(full, curves[1][i]);
                write_strip(full, strips[1][i]);
            }
            auto full_prefix = prefix;
            full_prefix += "-full";
            dispatch_tail(full, curves[1], strips[1], HeightDomain(dw, dh), static_cast<std::uint16_t>(gate),
                          dispatch_keys, dispatch_randoms, helper, true, input.pc, gates, worker,
                          full_prefix, computed_source);
            auto file = full_prefix;
            file += ".bin";
            save(file, full.bytes);
        }
    }
    return out.bytes;
}
} // namespace
