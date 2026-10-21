// Native corpus inputs only. The full pre-coast field is COMPUTED inside this
// executable by compose_worker_field, never loaded from an expected U16 oracle.
#include "civ5/terrain/coast_raster.hpp"
#include "worker_source_input.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
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
        check(n <= bytes.size() - at, "Truncated coast input");
        auto v = bytes.subspan(at, n);
        at += n;
        return v;
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
    void end() const { check(at == bytes.size(), "Trailing coast input"); }
};
struct Writer {
    Bytes bytes;
    void word(std::uint32_t v) {
        for (int i = 0; i < 4; ++i)
            bytes.push_back(static_cast<std::uint8_t>(v >> (8 * i)));
    }
    void raw(std::span<const std::uint8_t> v) { bytes.insert(bytes.end(), v.begin(), v.end()); }
    void blob(std::span<const std::uint8_t> v) {
        check(v.size() <= 0xffffffffu, "Oversized coast output");
        word(static_cast<std::uint32_t>(v.size()));
        raw(v);
    }
    void u16(std::span<const std::uint16_t> v) {
        for (auto x : v) {
            bytes.push_back(static_cast<std::uint8_t>(x));
            bytes.push_back(static_cast<std::uint8_t>(x >> 8));
        }
    }
};
Bytes read(const fs::path &path) {
    const auto size = fs::file_size(path);
    check(size >= 12 && size <= 256 * 1024 * 1024, "Invalid coast input size");
    Bytes result(static_cast<std::size_t>(size));
    std::ifstream in(path, std::ios::binary);
    in.read(reinterpret_cast<char *>(result.data()), result.size());
    check(bool(in), "Coast input read failed");
    return result;
}
void save(const fs::path &path, std::span<const std::uint8_t> raw) {
    check(!fs::exists(path), "Refusing coast output overwrite");
    std::ofstream out(path, std::ios::binary);
    out.write(reinterpret_cast<const char *>(raw.data()), raw.size());
    out.close();
    check(bool(out), "Coast output write failed");
}
WorkerField compute_worker(const fs::path &path) {
    const auto bytes = read(path);
    Reader r{bytes};
    const auto magic = r.take(8);
    check(std::string(magic.begin(), magic.end()) == "C5WRKF01", "Wrong worker input magic");
    const auto count = r.word();
    check(count > 0 && count <= 4096, "Invalid worker record count");
    std::optional<WorkerField> result;
    for (std::uint32_t i = 0; i < count; ++i) {
        const auto kind = r.word();
        Reader item{r.blob()};
        check(kind <= 2, "Invalid worker operation");
        if (kind != 2)
            continue;
        check(!result, "Duplicate worker full-field input");
        const auto width = item.integer(), height = item.integer();
        const auto bias = item.scalar();
        const auto pc = item.word();
        check(pc == 24 || pc == 53, "Invalid worker precision");
        const auto descriptors = civ5::validation::read_worker_sources(item, check);
        const auto gates = item.blob();
        item.end();
        result = compose_worker_field(HeightDomain(width, height), descriptors, gates, bias,
                                      pc == 24 ? X87Precision::single : X87Precision::double_precision);
    }
    r.end();
    check(bool(result), "Missing computed worker field");
    return std::move(*result);
}
std::vector<std::uint16_t> heights(std::span<const std::uint8_t> raw, std::size_t count) {
    check(raw.size() == count * 2, "Invalid coast height source size");
    std::vector<std::uint16_t> values(count);
    for (std::size_t i = 0; i < count; ++i)
        values[i] = std::uint16_t(raw[i * 2]) | std::uint16_t(raw[i * 2 + 1]) << 8;
    return values;
}
CoastVertex vertex(Reader &r) {
    const auto x = r.scalar(), y = r.scalar();
    const auto material = r.word();
    const auto h = r.scalar(), opacity = r.scalar();
    check(material <= 255, "Invalid coast material byte");
    return {x, y, static_cast<std::uint8_t>(material), h, opacity};
}
std::vector<CoastRasterJob> jobs(Reader &r) {
    const auto count = r.word();
    check(count <= 65536, "Too many coast jobs");
    std::vector<CoastRasterJob> result;
    for (std::uint32_t i = 0; i < count; ++i) {
        const auto remaps = r.take(8);
        std::array<std::uint8_t, 4> from, to;
        std::copy_n(remaps.begin(), 4, from.begin());
        std::copy_n(remaps.begin() + 4, 4, to.begin());
        const auto triangles = r.word();
        check(triangles <= 65536 && std::size_t(triangles) * 72 <= r.bytes.size() - r.at,
              "Invalid coast triangle count");
        std::vector<CoastTriangle> items;
        for (std::uint32_t j = 0; j < triangles; ++j) {
            const auto a = vertex(r), b = vertex(r), c = vertex(r);
            const auto ab = r.scalar(), bc = r.scalar(), ca = r.scalar();
            items.push_back({a, b, c, ab, bc, ca});
        }
        result.emplace_back(items, from, to);
    }
    return result;
}
Bytes run(Reader &r, const WorkerField *worker) {
    const auto width = r.integer(), height = r.integer();
    const HeightDomain domain(width, height);
    const auto wrap = r.word(), pc = r.word();
    check(wrap <= 1 && (pc == 24 || pc == 53 || pc == 64), "Invalid coast state");
    std::vector<std::uint16_t> h;
    Bytes m;
    std::span<const std::uint16_t> initial_h;
    std::span<const std::uint8_t> initial_m;
    const auto count = std::size_t(width) * height;
    if (worker) {
        check(worker->domain().width() == width && worker->domain().height() == height,
              "Worker/coast domain mismatch");
        initial_h = worker->heights();
        initial_m = worker->materials();
    } else {
        h = heights(r.blob(), count);
        const auto raw = r.blob();
        check(raw.size() == count, "Invalid coast material source size");
        m.assign(raw.begin(), raw.end());
        initial_h = h;
        initial_m = m;
    }
    const auto ordered = jobs(r);
    r.end();
    Bytes height_mask(count), material_mask(count);
    Writer traces, current;
    traces.word(static_cast<std::uint32_t>(ordered.size()));
    const auto field = apply_coast_jobs(
        domain, initial_h, initial_m, ordered, wrap != 0,
        pc == 24 ? CoastBoundsPrecision::single : CoastBoundsPrecision::double_or_extended,
        [&](CoastWriteChannel channel, std::int32_t index, std::uint32_t value) {
            const bool material = channel == CoastWriteChannel::material;
            (material ? material_mask : height_mask)[index] = 1;
            current.bytes.push_back(static_cast<std::uint8_t>(material));
            current.word(index);
            current.word(value);
        },
        [&](std::size_t) {
            traces.blob(current.bytes);
            current.bytes.clear();
        });
    Writer out, height_bytes;
    height_bytes.u16(field.heights());
    out.blob(height_bytes.bytes);
    out.blob(field.materials());
    out.blob(height_mask);
    out.blob(material_mask);
    out.raw(traces.bytes);
    const auto bridge = field.height_field();
    for (int y = 0; y < height; ++y)
        for (int x = 0; x < width; ++x)
            check(bridge.u16_at(x, y) == field.heights()[y * width + x] &&
                      bridge.a8_at(x, y) == field.heights()[y * width + x] >> 8,
                  "Post-coast HeightField bridge mismatch");
    return out.bytes;
}
} // namespace
#ifdef _WIN32
int wmain(int argc, wchar_t **argv) {
#else
int main(int argc, char **argv) {
#endif
    try {
        check(argc == 4, "Usage: coast_raster_oracle worker-input.bin coast-input.bin fresh-output");
        const fs::path worker_input = argv[1], coast_input = argv[2], output = argv[3];
        check(!fs::exists(output), "Output must be fresh");
        const auto bytes = read(coast_input);
        Reader r{bytes};
        const auto magic = r.take(8);
        check(std::string(magic.begin(), magic.end()) == "C5COAS01", "Wrong coast input magic");
        const auto count = r.word();
        check(count > 0 && count <= 4096, "Invalid coast record count");
        check(fs::create_directories(output), "Cannot create coast output");
        std::optional<WorkerField> worker;
        for (std::uint32_t i = 0; i < count; ++i) {
            const auto kind = r.word();
            check(kind <= 1, "Invalid coast operation");
            Reader item{r.blob()};
            if (kind == 1 && !worker) {
                worker = compute_worker(worker_input);
                Writer h;
                h.u16(worker->heights());
                save(output / "pre-coast.u16", h.bytes);
                save(output / "pre-coast.u8", worker->materials());
            }
            const auto result = run(item, kind == 1 ? &*worker : nullptr);
            save(output / (std::to_string(i) + ".bin"), result);
            if (kind == 1) {
                Reader generated{result};
                save(output / (std::to_string(i) + ".u16"), generated.blob());
            }
        }
        r.end();
        std::cout << "PASS " << count << " C++ coast records; full fields computed from worker sources\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
