// Input-only offline validation transport. No expected heights, native process,
// source capture pointers, Python or JVM are linked into the production module.
#include "civ5/terrain/worker_field.hpp"
#include "worker_source_input.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace civ5::terrain;
using Bytes = std::vector<std::uint8_t>;
namespace {
void check(bool ok, const char *message) {
    if (!ok)
        throw std::invalid_argument(message);
}
struct Reader {
    std::span<const std::uint8_t> bytes;
    std::size_t at = 0;
    std::span<const std::uint8_t> take(std::size_t count) {
        check(count <= bytes.size() - at, "Truncated worker input");
        auto result = bytes.subspan(at, count);
        at += count;
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
        const auto x0 = integer(), y0 = integer(), x1 = integer(), y1 = integer();
        return {x0, y0, x1, y1};
    }
    void end() const { check(at == bytes.size(), "Trailing worker input"); }
};
struct Writer {
    Bytes bytes;
    void word(std::uint32_t v) {
        for (int i = 0; i < 4; ++i)
            bytes.push_back(static_cast<std::uint8_t>(v >> (8 * i)));
    }
    void raw(std::span<const std::uint8_t> data) { bytes.insert(bytes.end(), data.begin(), data.end()); }
    void blob(std::span<const std::uint8_t> data) {
        check(data.size() <= 0xffffffffu, "Output too large");
        word(static_cast<std::uint32_t>(data.size()));
        raw(data);
    }
    void u16(std::span<const std::uint16_t> data) {
        for (auto v : data) {
            bytes.push_back(static_cast<std::uint8_t>(v));
            bytes.push_back(static_cast<std::uint8_t>(v >> 8));
        }
    }
};
X87Precision precision(Reader &r) {
    const auto pc = r.word();
    check(pc == 24 || pc == 53, "Invalid precision input");
    return pc == 24 ? X87Precision::single : X87Precision::double_precision;
}
Bytes packets_bytes(const WorkerPackets &packets) {
    Writer w;
    w.word(static_cast<std::uint32_t>(packets.size()));
    for (const auto &packet : packets) {
        w.word(static_cast<std::uint32_t>(packet.size()));
        for (auto v : packet)
            w.word(v);
    }
    return w.bytes;
}
std::vector<TerrainDescriptor> sources(Reader &r) { return civ5::validation::read_worker_sources(r, check); }
Bytes scratch_bytes(const WorkerAccumulation &state, std::span<const std::uint8_t> gates) {
    Writer w;
    for (std::size_t i = 0; i < 4096; ++i) {
        const auto &cell = state.cells()[i];
        w.word(std::bit_cast<std::uint32_t>(cell.numerator));
        w.word(std::bit_cast<std::uint32_t>(cell.denominator));
        w.bytes.insert(w.bytes.end(), {gates[i], cell.material, cell.priority, 0});
    }
    return w.bytes;
}
Bytes run(std::uint32_t kind, Reader &r) {
    const auto width = r.integer(), height = r.integer();
    const HeightDomain domain(width, height);
    Writer out;
    if (kind == 0) {
        const auto count = r.word();
        check(count <= 65536 && std::size_t(count) * 16 <= r.bytes.size() - r.at, "Invalid rectangle count");
        std::vector<SpatialRectangle> rectangles;
        for (std::uint32_t i = 0; i < count; ++i)
            rectangles.push_back(r.rectangle());
        r.end();
        out.blob(packets_bytes(route_worker_descriptors(domain, rectangles)));
        return out.bytes;
    }
    check(kind == 1 || kind == 2, "Unknown worker operation");
    std::int32_t ox = 0, oy = 0;
    if (kind == 1) {
        ox = r.integer();
        oy = r.integer();
    }
    const auto bias = r.scalar();
    const auto pc = precision(r);
    const auto descriptors = sources(r);
    const auto gates = r.blob();
    if (kind == 1) {
        const auto count = r.word();
        check(count <= 256, "Too many packet indices");
        std::vector<std::uint32_t> packet;
        for (std::uint32_t i = 0; i < count; ++i)
            packet.push_back(r.word());
        r.end();
        const auto state = accumulate_worker64(domain, ox, oy, descriptors, packet, gates, bias);
        out.blob(scratch_bytes(state, gates));
        Writer h;
        h.u16(state.finalize(pc));
        out.blob(h.bytes);
        Bytes materials;
        for (const auto &c : state.cells())
            materials.push_back(c.material);
        out.blob(materials);
        return out.bytes;
    }
    r.end();
    // Exercise the public complete-field path, not a validation-only assembler.
    const auto field = compose_worker_field(domain, descriptors, gates, bias, pc);
    const auto height_field = field.height_field();
    std::vector<SpatialRectangle> rectangles;
    for (const auto &d : descriptors)
        rectangles.push_back(d.rectangle);
    const auto packets = route_worker_descriptors(domain, rectangles);
    out.blob(packets_bytes(packets));
    Writer scratch;
    for (std::size_t i = 0; i < packets.size(); ++i) {
        ox = static_cast<std::int32_t>(i % (width / 64)) * 64;
        oy = static_cast<std::int32_t>(i / (width / 64)) * 64;
        std::array<std::uint8_t, 4096> local_gates;
        for (int y = 0; y < 64; ++y)
            std::copy_n(gates.begin() + (oy + y) * width + ox, 64, local_gates.begin() + y * 64);
        const auto state = accumulate_worker64(domain, ox, oy, descriptors, packets[i], local_gates, bias);
        scratch.raw(scratch_bytes(state, local_gates));
        const auto local_heights = state.finalize(pc);
        for (int y = 0; y < 64; ++y)
            for (int x = 0; x < 64; ++x) {
                const auto target = (oy + y) * width + ox + x, source = y * 64 + x;
                check(field.heights()[target] == local_heights[source] &&
                          field.materials()[target] == state.cells()[source].material,
                      "Whole-field/local composition mismatch");
                check(height_field.u16_at(ox + x, oy + y) == local_heights[source] &&
                          height_field.a8_at(ox + x, oy + y) == local_heights[source] >> 8,
                      "HeightField bridge mismatch");
            }
    }
    out.blob(scratch.bytes);
    Writer h;
    h.u16(field.heights());
    out.blob(h.bytes);
    out.blob(field.materials());
    return out.bytes;
}
} // namespace
#ifdef _WIN32
int wmain(int argc, wchar_t **argv) {
#else
int main(int argc, char **argv) {
#endif
    try {
        check(argc == 3, "Usage: civ5_worker_field_oracle input.bin fresh-output-directory");
        const std::filesystem::path input = argv[1], output = argv[2];
        check(!std::filesystem::exists(output), "Output must be fresh");
        const auto size = std::filesystem::file_size(input);
        check(size >= 12 && size <= 256 * 1024 * 1024, "Invalid input file size");
        Bytes bytes(static_cast<std::size_t>(size));
        std::ifstream file(input, std::ios::binary);
        file.read(reinterpret_cast<char *>(bytes.data()), bytes.size());
        check(bool(file), "Cannot read fixture");
        Reader r{bytes};
        const auto magic = r.take(8);
        check(std::string(magic.begin(), magic.end()) == "C5WRKF01", "Wrong worker input magic");
        const auto count = r.word();
        check(count > 0 && count <= 4096, "Invalid record count");
        check(std::filesystem::create_directories(output), "Cannot create output directory");
        for (std::uint32_t i = 0; i < count; ++i) {
            const auto kind = r.word();
            Reader record{r.blob()};
            const auto result = run(kind, record);
            std::ofstream target(output / (std::to_string(i) + ".bin"), std::ios::binary);
            target.write(reinterpret_cast<const char *>(result.data()), result.size());
            check(bool(target), "Cannot write worker result");
        }
        r.end();
        std::cout << "PASS " << count << " worker records (64-bit C++)\n";
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
