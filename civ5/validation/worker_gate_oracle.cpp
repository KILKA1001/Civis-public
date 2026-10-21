// Input-only offline runner. Native geometry remains explicit, but all categories,
// range/seed packets, gate fields and pre-coast heights are computed in C++.
#include "civ5/terrain/worker_gates.hpp"
#include "worker_source_input.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace civ5::terrain;
using Bytes = std::vector<std::uint8_t>;
namespace fs = std::filesystem;
namespace {
void check(bool ok, const char *message) {
    if (!ok)
        throw std::invalid_argument(message);
}
struct Reader {
    std::span<const std::uint8_t> bytes;
    std::size_t at = 0;
    auto take(std::size_t n) {
        check(n <= bytes.size() - at, "Truncated gate input");
        auto result = bytes.subspan(at, n);
        at += n;
        return result;
    }
    std::uint32_t word() {
        const auto b = take(4);
        return std::uint32_t(b[0]) | std::uint32_t(b[1]) << 8 | std::uint32_t(b[2]) << 16 |
               std::uint32_t(b[3]) << 24;
    }
    std::int32_t integer() { return std::bit_cast<std::int32_t>(word()); }
    float scalar() { return std::bit_cast<float>(word()); }
    auto blob() { return take(word()); }
    SpatialRectangle rectangle() {
        const auto x = integer(), y = integer(), ex = integer(), ey = integer();
        return {x, y, ex, ey};
    }
    void end() const { check(at == bytes.size(), "Trailing gate input"); }
};
struct Writer {
    Bytes bytes;
    void word(std::uint32_t v) {
        for (int i = 0; i < 4; ++i)
            bytes.push_back(static_cast<std::uint8_t>(v >> (8 * i)));
    }
    void raw(std::span<const std::uint8_t> b) { bytes.insert(bytes.end(), b.begin(), b.end()); }
    void blob(std::span<const std::uint8_t> b) {
        check(b.size() <= 0xffffffffu, "Gate output too large");
        word(static_cast<std::uint32_t>(b.size()));
        raw(b);
    }
    void u16(std::span<const std::uint16_t> values) {
        for (auto v : values) {
            bytes.push_back(static_cast<std::uint8_t>(v));
            bytes.push_back(static_cast<std::uint8_t>(v >> 8));
        }
    }
};
X87Precision precision(Reader &r) {
    const auto pc = r.word();
    check(pc == 24 || pc == 53, "Invalid gate precision");
    return pc == 24 ? X87Precision::single : X87Precision::double_precision;
}
std::vector<Point2> points(Reader &r, std::uint32_t limit) {
    const auto n = r.word();
    check(n <= limit && std::size_t(n) * 8 <= r.bytes.size() - r.at, "Invalid gate point count");
    std::vector<Point2> result;
    for (std::uint32_t i = 0; i < n; ++i) {
        const auto x = r.scalar(), y = r.scalar();
        result.emplace_back(x, y);
    }
    return result;
}
std::vector<WorkerGateRange> ranges(Reader &r) {
    const auto n = r.word();
    check(n <= max_coast_contours * 2 && std::size_t(n) * 12 <= r.bytes.size() - r.at, "Invalid range count");
    std::vector<WorkerGateRange> result;
    for (std::uint32_t i = 0; i < n; ++i) {
        const auto a = r.word(), b = r.word(), g = r.word();
        result.push_back({a, b, g});
    }
    return result;
}
struct Lines {
    std::vector<std::vector<Point2>> owned;
    std::vector<WorkerGateLine> views;
    explicit Lines(Reader &r) {
        const auto n = r.word();
        check(n <= 32, "Invalid gate line count");
        owned.reserve(n);
        for (std::uint32_t i = 0; i < n; ++i) {
            const auto gate = r.word();
            owned.push_back(points(r, max_coast_curve_controls));
            views.push_back({owned.back(), gate});
        }
    }
};
void save(const fs::path &path, std::span<const std::uint8_t> raw) {
    check(!fs::exists(path), "Refusing gate output overwrite");
    std::ofstream out(path, std::ios::binary);
    out.write(reinterpret_cast<const char *>(raw.data()), raw.size());
    out.close();
    check(bool(out), "Gate output write failed");
}
Bytes run(std::uint32_t kind, Reader &r, std::span<const std::uint8_t> source, const fs::path &prefix) {
    const auto width = r.integer(), height = r.integer();
    const HeightDomain domain(width, height);
    Writer out;
    if (kind == 0) {
        const auto ox = r.integer(), oy = r.integer();
        const Lines boundaries(r), fills(r);
        const auto count = r.word();
        check(count <= 16, "Invalid gate seed count");
        std::vector<WorkerGateSeed> seeds;
        for (std::uint32_t i = 0; i < count; ++i) {
            const auto x = r.integer(), y = r.integer();
            const auto g = r.word();
            seeds.push_back({x, y, g});
        }
        r.end();
        out.blob(classify_worker_gates64(domain, ox, oy, boundaries.views, fills.views, seeds));
        return out.bytes;
    }
    check(kind == 1 || kind == 2, "Unknown gate operation");
    const auto mw = r.integer(), mh = r.integer();
    check(mw >= 1 && mh >= 1 && mw <= 255 && mh <= 255, "Invalid gate logical dimensions");
    std::vector<std::uint32_t> masks;
    for (int i = 0; i < mw * mh; ++i)
        masks.push_back(r.word());
    IntMatrix4 m1, m2;
    Matrix4 world;
    for (auto &v : m1)
        v = r.integer();
    for (auto &v : m2)
        v = r.integer();
    for (auto &v : world)
        v = r.scalar();
    const auto wrap = r.word(), outside = r.word();
    const auto pc = precision(r);
    const auto path = r.word(), field_flag = r.word();
    check(wrap <= 1 && outside <= 255 && path <= 1 && field_flag <= 1, "Invalid gate routing state");
    check(kind != 2 || field_flag == 1, "Composition requires computed gates");
    const auto p = points(r, max_coast_worker_points);
    const auto b = ranges(r), f = ranges(r);
    r.end();
    const WorkerGateSource grid{mw,        mh,
                                masks,     m1,
                                m2,        world,
                                wrap == 1, static_cast<std::uint8_t>(outside),
                                pc,        path ? Vec4Path::sse : Vec4Path::legacy_x87};
    const auto routing = route_worker_gates(domain, grid, p, b, f);
    out.blob(routing.logical_categories());
    out.word(static_cast<std::uint32_t>(routing.workers().size()));
    for (const auto &worker : routing.workers()) {
        out.word(worker.origin_x);
        out.word(worker.origin_y);
        for (const auto *list : {&worker.boundaries, &worker.fills}) {
            out.word(static_cast<std::uint32_t>(list->size()));
            for (auto range : *list) {
                out.word(range.first);
                out.word(range.end);
                out.word(range.gate);
            }
        }
        out.word(static_cast<std::uint32_t>(worker.seeds.size()));
        for (auto seed : worker.seeds) {
            out.word(seed.x);
            out.word(seed.y);
            out.word(seed.gate);
        }
    }
    if (field_flag) {
        const auto gates = routing.build_gate_field64();
        out.blob(gates);
        if (kind == 2) {
            Reader s{source};
            const auto w = s.integer(), h = s.integer();
            check(w == width && h == height, "Gate/descriptor domain mismatch");
            const auto bias = s.scalar();
            const auto compose_pc = precision(s);
            const auto descriptors = civ5::validation::read_worker_sources(s, check);
            s.end();
            const auto field = compose_worker_field(domain, descriptors, gates, bias, compose_pc);
            Writer heights;
            heights.u16(field.heights());
            out.blob(heights.bytes);
            out.blob(field.materials());
            auto height_path = prefix, material_path = prefix;
            height_path += ".u16";
            material_path += ".u8";
            save(height_path, heights.bytes);
            save(material_path, field.materials());
        }
    }
    return out.bytes;
}
} // namespace
#ifdef _WIN32
int wmain(int argc, wchar_t **argv) {
#else
int main(int argc, char **argv) {
#endif
    try {
        check(argc == 3, "Usage: civ5_worker_gate_oracle input.bin fresh-output-directory");
        const fs::path input = argv[1], output = argv[2];
        check(!fs::exists(output), "Output must be fresh");
        const auto size = fs::file_size(input);
        check(size >= 16 && size <= 256 * 1024 * 1024, "Invalid gate input size");
        Bytes bytes(static_cast<std::size_t>(size));
        std::ifstream file(input, std::ios::binary);
        file.read(reinterpret_cast<char *>(bytes.data()), bytes.size());
        check(bool(file), "Gate input read failed");
        Reader r{bytes};
        const auto magic = r.take(8);
        check(std::string(magic.begin(), magic.end()) == "C5GATE01", "Wrong gate input magic");
        const auto count = r.word();
        check(count > 0 && count <= 4096, "Invalid gate record count");
        const auto source = r.blob();
        check(fs::create_directories(output), "Cannot create gate output directory");
        for (std::uint32_t i = 0; i < count; ++i) {
            const auto kind = r.word();
            Reader item{r.blob()};
            const auto result = run(kind, item, source, output / std::to_string(i));
            save(output / (std::to_string(i) + ".bin"), result);
        }
        r.end();
        std::cout << "PASS " << count << " gate/routing records (64-bit C++)\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
