#pragma once
// Input-only validation: original authored numeric source and already-perturbed
// contours -> C++ curves/strips/dispatch and geometry/gates/worker/coast fields.
#include "civ5/terrain/coast_strip.hpp"
#include "coast_geometry_io.hpp"
#include <algorithm>
namespace {
std::vector<std::uint32_t> palette(Reader &r) {
    const auto count = r.word();
    check(count <= 65536, "Coast strip palette size exceeded");
    std::vector<std::uint32_t> result;
    for (std::uint32_t i = 0; i < count; ++i)
        result.push_back(r.word());
    return result;
}
void write_strip(Writer &out, const CoastStrip &strip) {
    Writer vertices, indices, coefficients;
    for (auto v : strip.vertices()) {
        vertices.scalar(v.x);
        vertices.scalar(v.y);
        // Transport only: three zero padding bytes, not a native ABI contract.
        vertices.word(v.material);
        vertices.scalar(v.height);
        vertices.scalar(v.opacity);
    }
    indices.u16(strip.local_indices());
    for (std::size_t i = 0; i < strip.triangle_count(); ++i)
        for (auto v : CoastStrip::coefficients(i))
            coefficients.scalar(v);
    out.blob(vertices.bytes);
    out.blob(indices.bytes);
    out.blob(coefficients.bytes);
    out.word(static_cast<std::uint32_t>(strip.chunks().size()));
    for (auto c : strip.chunks()) {
        out.word(c.first_vertex);
        out.word(c.vertex_count);
        out.word(c.first_triangle);
        out.word(c.triangle_count);
    }
}
CoastCurveSet direct_curves(Reader &r) {
    CoastCurves curves;
    for (auto &curve : curves) {
        const auto bytes = r.blob();
        check(bytes.size() % 14 == 0 && bytes.size() / 14 >= 3 && bytes.size() / 14 <= max_coast_curve_controls,
              "Invalid explicit strip controls");
        Reader p{bytes};
        while (p.at < bytes.size()) {
            const auto x = p.scalar(), y = p.scalar(), t = p.scalar();
            const auto styles = p.take(2);
            curve.push_back({x, y, t, styles[0], styles[1]});
        }
    }
    return CoastCurveSet(std::move(curves), 0);
}
CoastStrip strip_for(const CoastCurveSet &curves, std::span<const std::uint8_t> request, const Library &lib) {
    Reader r{request};
    r.take(12); // precision, owner, closure
    const auto family = r.word(), common = r.word();
    r.take(8); // wrap offset
    const auto keys = palette(r);
    return build_coast_strip(curves, lib.profiles, keys, family, common);
}
void write_plan(Writer &out, const CoastDispatchPlan &plan, std::span<const CoastStrip> strips) {
    out.word(plan.gate);
    out.word(plan.direct_random_calls);
    for (const auto &phase : plan.phases) {
        out.word(static_cast<std::uint32_t>(phase.size()));
        for (auto job : phase) {
            const auto chunk = strips[job.strip].chunks()[job.chunk];
            out.word(static_cast<std::uint32_t>(job.strip));
            out.word(static_cast<std::uint32_t>(job.chunk));
            out.word(job.allocation_flag);
            out.word(chunk.first_vertex);
            out.word(chunk.vertex_count);
            out.word(chunk.first_triangle);
            out.word(chunk.triangle_count);
            out.word(0);
            out.raw(plan.remap_from);
            out.raw(plan.remap_to);
        }
    }
}
Bytes coast_field(const WorkerField &worker, bool wrap, std::span<const CoastStrip> strips,
                  const CoastDispatchPlan &plan, bool reverse, const fs::path &height_path) {
    std::vector<CoastRasterJob> jobs;
    for (auto phase : plan.phases) {
        if (reverse)
            std::reverse(phase.begin(), phase.end());
        for (auto job : phase) {
            check(job.allocation_flag == 0, "Unsupported nonzero coast integrity flag");
            jobs.push_back(strips[job.strip].raster_job(job.chunk, plan.remap_from, plan.remap_to));
        }
    }
    const auto &domain = worker.domain();
    const auto count = std::size_t(domain.width()) * domain.height();
    Bytes hm(count), mm(count);
    Writer traces, current;
    traces.word(static_cast<std::uint32_t>(jobs.size()));
    const auto field = apply_coast_jobs(
        domain, worker.heights(), worker.materials(), jobs, wrap, CoastBoundsPrecision::double_or_extended,
        [&](CoastWriteChannel channel, std::int32_t index, std::uint32_t value) {
            const bool material = channel == CoastWriteChannel::material;
            (material ? mm : hm)[index] = 1;
            current.bytes.push_back(static_cast<std::uint8_t>(material));
            current.word(index);
            current.word(value);
        },
        [&](std::size_t) {
            traces.blob(current.bytes);
            current.bytes.clear();
        });
    Writer out, h;
    h.u16(field.heights());
    out.blob(h.bytes);
    out.blob(field.materials());
    out.blob(hm);
    out.blob(mm);
    out.raw(traces.bytes);
    save(height_path, h.bytes);
    return out.bytes;
}
void dispatch_tail(Writer &out, std::span<const CoastCurveSet> curves, std::span<const CoastStrip> strips,
                   HeightDomain domain, std::uint16_t gate, std::span<const std::uint32_t> keys,
                   std::span<const std::uint32_t> randoms, std::uint32_t helper, bool route, X87Precision pc,
                   std::span<const std::uint8_t> gate_source, std::span<const std::uint8_t> worker,
                   const fs::path &prefix,
                   const civ5::validation::WorkerCompositionSource *computed_source = nullptr) {
    const auto plan = build_coast_dispatch(strips, static_cast<std::uint16_t>(gate), keys, randoms, helper);
    write_plan(out, plan, strips);
    if (route) {
        const auto geometry = build_worker_geometry(curves, pc);
        write_geometry(out, geometry);
        auto pre = prefix;
        pre += "-pre";
        out.blob(compose(geometry, gate_source, worker, pre, [&](const WorkerField &field, bool wrap) {
            check(field.domain().width() == domain.width() && field.domain().height() == domain.height(),
                  "Dispatch/worker domain mismatch");
            for (bool reverse : {false, true}) {
                auto file = prefix;
                file += reverse ? "-reverse" : "-submission";
                auto height_file = file;
                height_file += ".u16";
                const auto raw = coast_field(field, wrap, strips, plan, reverse, height_file);
                file += ".bin";
                save(file, raw);
            }
        }, computed_source));
    }
}
Bytes run_strip(std::uint32_t kind, Reader &r, const Library &lib, std::span<const std::uint8_t> worker,
                std::span<const std::uint8_t> gate_source, const fs::path &prefix) {
    Writer out;
    if (kind == 0) {
        const auto keys = palette(r);
        const auto family = r.word(), common = r.word();
        const auto curves = direct_curves(r);
        r.end();
        write_strip(out, build_coast_strip(curves, lib.profiles, keys, family, common));
    } else if (kind == 1) {
        const auto request = r.blob();
        r.end();
        Reader item{request};
        const auto curves = read_curve_request(item, false, lib);
        write_curves(out, curves);
        write_strip(out, strip_for(curves, request, lib));
    } else {
        check(kind == 2, "Invalid coast strip operation");
        const auto width = r.integer(), height = r.integer();
        const HeightDomain domain(width, height);
        const auto gate = r.word(), helper = r.word();
        check(gate <= 65535, "Invalid dispatch gate");
        const auto randoms = palette(r), keys = palette(r);
        const auto pc = precision(r);
        const auto route = r.word(), count = r.word();
        check(route <= 1 && count <= max_coast_contours && (!route || count > 0), "Invalid strip set count/route flag");
        std::vector<CoastCurveSet> curves;
        std::vector<CoastStrip> strips;
        out.word(count);
        for (std::uint32_t i = 0; i < count; ++i) {
            const auto request = r.blob();
            Reader item{request};
            curves.push_back(read_curve_request(item, false, lib));
            strips.push_back(strip_for(curves.back(), request, lib));
            write_curves(out, curves.back());
            write_strip(out, strips.back());
        }
        r.end();
        dispatch_tail(out, curves, strips, domain, static_cast<std::uint16_t>(gate), keys, randoms, helper,
                      route != 0, pc, gate_source, worker, prefix);
    }
    return out.bytes;
}
} // namespace
