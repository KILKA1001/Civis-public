// Saved source masks -> graph -> perturbation -> authored curves -> strips.
// Original expected bytes are comparison-only; they never enter the producers.
#include "coast_map_io.hpp"
#include "civ5/terrain/terrain_occurrences.hpp"

namespace {
std::size_t compare(std::span<const std::uint8_t> actual, std::span<const std::uint8_t> expected,
                    std::uint32_t contour, const char *stage) {
    const auto mismatch = std::mismatch(actual.begin(), actual.end(), expected.begin(), expected.end());
    if (mismatch.first != actual.end() || mismatch.second != expected.end())
        throw std::runtime_error("Original contour " + std::to_string(contour) + " " + stage +
            " differs at byte " + std::to_string(mismatch.first - actual.begin()) +
            " (sizes " + std::to_string(actual.size()) + "/" + std::to_string(expected.size()) + ")");
    return actual.size();
}
Bytes read_corpus(const fs::path &path) {
    const auto size = fs::file_size(path);
    check(size > 12 && size <= 128 * 1024 * 1024, "Invalid saved coast corpus size");
    Bytes bytes(static_cast<std::size_t>(size));
    std::ifstream file(path, std::ios::binary);
    check(bool(file.read(reinterpret_cast<char *>(bytes.data()), bytes.size())), "Truncated saved coast corpus");
    return bytes;
}
void dispatch_saved(std::span<const CoastStrip> strips, std::span<const std::uint8_t> source,
                    std::span<const std::uint32_t> keys, std::uint32_t perturbation_final,
                    const fs::path &path) {
    const auto bytes = read_corpus(path);
    Reader expected{bytes}, dimensions{source};
    const auto magic = expected.take(8);
    check(std::string(magic.begin(), magic.end()) == "C5SCD001", "Wrong saved dispatch corpus magic");
    check(expected.word() == dimensions.word() * 64 && expected.word() == dimensions.word() * 64,
          "Original dispatch domain differs");
    check(expected.word() == perturbation_final, "Connected test dispatch cursor differs");
    const auto gate = expected.word();
    check(gate <= 65535, "Invalid original dispatch gate");
    check(expected.word() == unmodified_coast_integrity_return, "Original integrity result differs");
    const auto original_keys = palette(expected);
    check(std::equal(keys.begin(), keys.end(), original_keys.begin(), original_keys.end()),
          "Original dispatch palette differs");
    TerrainRandom random(perturbation_final);
    const auto plan = build_coast_dispatch(strips, static_cast<std::uint16_t>(gate), keys,
                                          random, unmodified_coast_integrity_return);
    Writer actual;
    write_plan(actual, plan, strips);
    const auto compared = compare(actual.bytes, expected.blob(), 0, "dispatch plan");
    check(random.state() == expected.word() && random.calls() == 2, "Original dispatch random cursor differs");
    expected.end();
    std::cout << "PASS saved coast dispatch: " << plan.phases[0].size() + plan.phases[1].size()
              << " jobs, " << compared << " exact bytes, original integrity return 255, final state "
              << random.state() << '\n';
}
void downstream(std::span<const std::uint8_t> source, std::span<const CoastCurveSet> curves,
                  const fs::path &path, const fs::path &gate_path = {}) {
    const auto bytes = read_corpus(path);
    Reader expected{bytes};
    const auto magic = expected.take(8);
    check(std::string(magic.begin(), magic.end()) == "C5SWG001", "Wrong saved worker corpus magic");
    Reader s{source};
    const auto w = s.integer(), h = s.integer();
    const auto pc = precision(s);
    const auto sse = flag(s), wrap = flag(s);
    flag(s);
    const auto masks = palette(s);
    IntMatrix4 m1, m2;
    Matrix4 world;
    for (auto &v : m1) v = s.integer();
    for (auto &v : m2) v = s.integer();
    for (auto &v : world) v = s.scalar();
    s.end();
    const auto geometry = build_worker_geometry(curves, pc);
    Writer g;
    write_geometry(g, geometry);
    auto compared = compare(g.bytes, expected.blob(), 0, "worker geometry");
    const auto routing = route_worker_gates(HeightDomain(w * 64, h * 64),
        {w, h, masks, m1, m2, world, wrap, 255, pc, sse ? Vec4Path::sse : Vec4Path::legacy_x87},
        geometry.points, geometry.boundaries, geometry.fills);
    Writer r;
    r.blob(routing.logical_categories());
    r.word(static_cast<std::uint32_t>(routing.workers().size()));
    std::size_t maximum_packet_points = 0;
    for (const auto &worker : routing.workers()) {
        r.word(worker.origin_x); r.word(worker.origin_y);
        std::size_t packet_points = 0;
        for (const auto *ranges : {&worker.boundaries, &worker.fills}) {
            r.word(static_cast<std::uint32_t>(ranges->size()));
            for (auto range : *ranges) {
                r.word(range.first); r.word(range.end); r.word(range.gate);
                packet_points += range.end - range.first;
            }
        }
        maximum_packet_points = std::max(maximum_packet_points, packet_points);
        r.word(static_cast<std::uint32_t>(worker.seeds.size()));
        for (auto seed : worker.seeds) { r.word(seed.x); r.word(seed.y); r.word(seed.gate); }
    }
    compared += compare(r.bytes, expected.blob(), 0, "worker routing");
    const auto extras = expected.word();
    check(extras <= 8, "Excessive direct geometry cases");
    for (std::uint32_t i = 0; i < extras; ++i) {
        Reader input{expected.blob()};
        const auto count = input.word();
        check(count <= max_coast_contours, "Direct curve count exceeds capacity");
        std::vector<CoastCurveSet> direct;
        for (std::uint32_t j = 0; j < count; ++j) direct.push_back(direct_curves(input));
        input.end();
        Writer actual;
        write_geometry(actual, build_worker_geometry(direct, pc));
        compared += compare(actual.bytes, expected.blob(), i, "direct worker geometry");
    }
    expected.end();
    std::cout << "PASS saved worker geometry/routing: " << compared << " exact bytes, "
              << geometry.points.size() << " points, " << routing.workers().size() << " workers, "
              << maximum_packet_points << " maximum points per packet, " << extras << " direct cases\n";
    if (!gate_path.empty()) {
        const auto gate_bytes = read_corpus(gate_path);
        Reader gate_expected{gate_bytes};
        const auto gate_magic = gate_expected.take(8);
        check(std::string(gate_magic.begin(), gate_magic.end()) == "C5SGP001", "Wrong saved gate corpus magic");
        check(gate_expected.word() == std::uint32_t(w * 64) &&
              gate_expected.word() == std::uint32_t(h * 64), "Saved gate domain differs");
        const auto count = gate_expected.word();
        check(count > 0 && count <= 64, "Invalid selected gate case count");
        std::size_t compared_gates = 0;
        for (std::uint32_t i = 0; i < count; ++i) {
            const auto index = gate_expected.word();
            const auto gates = routing.classify_worker64(index);
            compared_gates += compare(gates, gate_expected.blob(), index, "worker gates");
        }
        gate_expected.end();
        std::cout << "PASS selected saved worker gates: " << count << " workers, "
                  << compared_gates << " exact bytes\n";
    }
}
} // namespace
#ifdef _WIN32
int wmain(int argc, wchar_t **argv) {
#else
int main(int argc, char **argv) {
#endif
    try {
        check(argc >= 3 && argc <= 6,
              "Usage: civ5_saved_coast_geometry_tests asset-root original-map.bin [original-worker.bin [original-gates.bin [original-dispatch.bin]]]");
        const fs::path root = argv[1], path = argv[2];
        const auto bytes = read_corpus(path);
        Reader input{bytes};
        const auto magic = input.take(8);
        check(std::string(magic.begin(), magic.end()) == "C5SCG001", "Wrong saved coast corpus magic");
        const auto source_bytes = input.blob();
        Reader source{source_bytes};
        const auto map = map_input(source);
        check(map.scan, "Saved coast corpus requires graph scan");
        TerrainRandom random(input.word());
        const auto pc = precision(input);
        check(pc == map.pc, "Graph and curve precision differ");
        const auto pow = flag(input) ? CoastPowPath::sse2 : CoastPowPath::legacy_x87;
        const auto keys = palette(input);
        const auto count = input.word();
        const auto contours = map.graph.scan(map.graph.smooth(pc));
        check(count == contours.size(), "Original saved contour count differs");
        const auto assets = coast_source(root, "Resource/Common/TerrainModels.fpk");
        const auto library = CoastLibrary::load(*assets, civ5::assets::AssetKey("coaststyles.xml"), pc);
        std::size_t compared = 0, maximum = 0, vertices = 0, chunks = 0;
        std::vector<CoastCurveSet> produced;
        std::vector<CoastStrip> produced_strips;
        for (std::uint32_t i = 0; i < count; ++i) {
            check(input.word() == i, "Original contour order differs");
            const auto &contour = contours[i];
            const auto wrap = contour.wrap_offset.value_or(Point2{0, 0});
            const auto perturbed = perturb_coast(contour.points, contour.closed, wrap, random, pc, pow);
            Writer p;
            perturbation(p, perturbed);
            compared += compare(p.bytes, input.blob(), i, "perturbation");
            auto curves = build_coast_curves(perturbed.points, library.profiles(), library.characters(),
                {contour.closed, wrap, contour.owner_flag, keys, 0x110000, 0x100000, pc});
            Writer c;
            write_curves(c, curves);
            compared += compare(c.bytes, input.blob(), i, "curves");
            auto strip = build_coast_strip(curves, library.profiles(), keys, 0x110000, 0x100000);
            Writer s;
            write_strip(s, strip);
            compared += compare(s.bytes, input.blob(), i, "strip");
            check(random.state() == input.word(), "Original final CRT state differs");
            check(random.calls() == input.word(), "Original cumulative CRT call count differs");
            compared += 8;
            maximum = std::max(maximum, contour.points.size());
            vertices += strip.vertices().size();
            chunks += strip.chunks().size();
            std::cout << "PASS original contour " << i << ": " << contour.points.size() << " -> "
                      << curves.control_count() << " controls, " << strip.vertices().size() << " vertices\n";
            if (argc >= 4) produced.push_back(std::move(curves));
            if (argc == 6) produced_strips.push_back(std::move(strip));
        }
        input.end();
        std::cout << "PASS saved coast geometry: " << count << " contours, " << compared << " exact bytes, "
                  << maximum << " maximum controls, " << vertices << " vertices, " << chunks << " chunks, "
                  << random.calls() << " random calls, final state " << random.state() << '\n';
        if (argc >= 4) downstream(source_bytes, produced, fs::path(argv[3]), argc >= 5 ? fs::path(argv[4]) : fs::path{});
        if (argc == 6) dispatch_saved(produced_strips, source_bytes, keys, random.state(), fs::path(argv[5]));
    } catch (const std::exception &e) { std::cerr << "FAIL " << e.what() << '\n'; return 1; }
}
