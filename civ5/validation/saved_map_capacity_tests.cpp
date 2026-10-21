// Original-output differential checks for actual saved coast sources and large
// worker domains. Expected bytes never enter a production terrain function.
#include "coast_map_io.hpp"

namespace {
Bytes gates(Reader &r) {
    const auto dw = r.integer(), dh = r.integer();
    Reader source{r.blob()};
    const auto w = source.integer(), h = source.integer();
    const auto pc = precision(source);
    const bool sse = flag(source), wrap = flag(source);
    flag(source); // Coast scan flag is irrelevant to seed/range routing.
    const auto masks = palette(source);
    IntMatrix4 m1, m2;
    Matrix4 world;
    for (auto &v : m1) v = source.integer();
    for (auto &v : m2) v = source.integer();
    for (auto &v : world) v = source.scalar();
    source.end();
    const auto np = r.word();
    check(np <= 65536, "Excessive point count");
    std::vector<Point2> points;
    for (std::uint32_t i = 0; i < np; ++i) {
        const auto x = r.scalar(), y = r.scalar();
        points.push_back({x, y});
    }
    const auto ranges = [&] {
        const auto count = r.word();
        check(count <= 4096, "Excessive range count");
        std::vector<WorkerGateRange> result;
        for (std::uint32_t i = 0; i < count; ++i) {
            const auto a = r.word(), b = r.word(), gate = r.word();
            result.push_back({a, b, gate});
        }
        return result;
    };
    const auto boundaries = ranges(), fills = ranges();
    r.end();
    const auto routing = route_worker_gates(HeightDomain(dw, dh),
        {w, h, masks, m1, m2, world, wrap, 255, pc, sse ? Vec4Path::sse : Vec4Path::legacy_x87},
        points, boundaries, fills);
    Writer out;
    out.blob(routing.logical_categories());
    out.word(static_cast<std::uint32_t>(routing.workers().size()));
    for (const auto &worker : routing.workers()) {
        out.word(worker.origin_x);
        out.word(worker.origin_y);
        for (const auto *items : {&worker.boundaries, &worker.fills}) {
            out.word(static_cast<std::uint32_t>(items->size()));
            for (const auto &v : *items) { out.word(v.first); out.word(v.end); out.word(v.gate); }
        }
        out.word(static_cast<std::uint32_t>(worker.seeds.size()));
        for (const auto &v : worker.seeds) { out.word(v.x); out.word(v.y); out.word(v.gate); }
    }
    return out.bytes;
}
Bytes descriptors(Reader &r) {
    const auto w = r.integer(), h = r.integer();
    const auto n = r.word();
    check(n <= 65536, "Excessive descriptor count");
    std::vector<SpatialRectangle> rectangles;
    for (std::uint32_t i = 0; i < n; ++i) rectangles.push_back(r.rectangle());
    r.end();
    const auto packets = route_worker_descriptors(HeightDomain(w, h), rectangles);
    Writer out;
    out.word(static_cast<std::uint32_t>(packets.size()));
    for (const auto &packet : packets) {
        out.word(static_cast<std::uint32_t>(packet.size()));
        for (auto index : packet) out.word(index);
    }
    return out.bytes;
}
void guards() {
    std::vector<std::uint32_t> masks(256, 0x2000);
    IntMatrix4 identity{};
    for (unsigned i = 0; i < 16; i += 5) identity[i] = 1;
    for (auto dims : {std::array<int, 2>{256, 1}, {1, 256}}) {
        bool rejected = false;
        try { CoastMapGraph::build({dims[0], dims[1], masks, identity, identity, identity_matrix(),
                                    true, X87Precision::double_precision, Vec4Path::sse}); }
        catch (const std::invalid_argument &) { rejected = true; }
        check(rejected, "Out-of-owner coast source accepted");
        rejected = false;
        try { route_worker_gates(HeightDomain(64, 64),
            {dims[0], dims[1], masks, identity, identity, identity_matrix(), true, 255,
             X87Precision::double_precision, Vec4Path::sse}, {}, {}, {}); }
        catch (const std::invalid_argument &) { rejected = true; }
        check(rejected, "Out-of-owner category source accepted");
    }
}
} // namespace
int main(int argc, char **argv) {
    try {
        check(argc <= 2, "Usage: civ5_saved_map_capacity_tests [oracle.bin]");
        guards();
        if (argc == 1) { std::cout << "PASS saved map capacity guards\n"; return 0; }
        std::ifstream file(argv[1], std::ios::binary | std::ios::ate);
        check(bool(file), "Cannot open capacity oracle");
        const auto size = file.tellg();
        check(size > 12 && size <= 64 * 1024 * 1024, "Invalid capacity oracle size");
        Bytes bytes(static_cast<std::size_t>(size));
        file.seekg(0);
        check(bool(file.read(reinterpret_cast<char *>(bytes.data()), bytes.size())), "Truncated capacity oracle");
        Reader all{bytes};
        const auto magic = all.take(8);
        check(std::string(magic.begin(), magic.end()) == "C5MCAP01", "Wrong capacity oracle magic");
        const auto count = all.word();
        check(count > 0 && count <= 100, "Invalid capacity case count");
        std::size_t compared = 0;
        for (std::uint32_t i = 0; i < count; ++i) {
            const auto kind = all.word();
            Reader input{all.blob()};
            Bytes actual;
            if (kind == 0) {
                const auto source = map_input(input);
                Writer out;
                graph_output(out, source);
                actual = std::move(out.bytes);
            } else if (kind == 1) actual = gates(input);
            else { check(kind == 2, "Invalid capacity case kind"); actual = descriptors(input); }
            const auto expected = all.blob();
            const auto mismatch = std::mismatch(actual.begin(), actual.end(), expected.begin(), expected.end());
            if (mismatch.first != actual.end() || mismatch.second != expected.end())
                throw std::runtime_error("Original capacity case " + std::to_string(i) + " kind " +
                    std::to_string(kind) + " differs at byte " + std::to_string(mismatch.first - actual.begin()));
            compared += actual.size();
            std::cout << "PASS original capacity case " << i << " kind " << kind << ": " << actual.size() << " bytes\n";
        }
        all.end();
        std::cout << "PASS saved map capacity: " << count << " original cases, " << compared << " exact bytes\n";
    } catch (const std::exception &e) { std::cerr << "FAIL " << e.what() << '\n'; return 1; }
}
