#include "civ5/terrain/river_raster.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace civ5::terrain;
namespace fs = std::filesystem;
using Bytes = std::vector<std::uint8_t>;
namespace {
void check(bool ok, const char *message) {
    if (!ok)
        throw std::invalid_argument(message);
}
struct Reader {
    std::span<const std::uint8_t> bytes;
    std::size_t at = 0;
    auto take(std::size_t size) {
        check(size <= bytes.size() - at, "Truncated river input");
        const auto result = bytes.subspan(at, size);
        at += size;
        return result;
    }
    std::uint32_t word() {
        const auto b = take(4);
        return std::uint32_t(b[0]) | std::uint32_t(b[1]) << 8 | std::uint32_t(b[2]) << 16 |
               std::uint32_t(b[3]) << 24;
    }
    auto integer() { return std::bit_cast<std::int32_t>(word()); }
    auto scalar() { return std::bit_cast<float>(word()); }
    auto blob() { return take(word()); }
    void end() { check(at == bytes.size(), "Trailing river input"); }
};
struct Writer {
    Bytes bytes;
    void byte(std::uint8_t v) { bytes.push_back(v); }
    void word(std::uint32_t v) {
        for (int i = 0; i < 4; ++i)
            byte(static_cast<std::uint8_t>(v >> (i * 8)));
    }
    void u16(std::span<const std::uint16_t> values) {
        for (auto v : values) {
            byte(static_cast<std::uint8_t>(v));
            byte(static_cast<std::uint8_t>(v >> 8));
        }
    }
};
void save(const fs::path &file, std::span<const std::uint8_t> bytes) {
    check(!fs::exists(file), "Refusing river output overwrite");
    std::ofstream out(file, std::ios::binary);
    out.write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
    out.close();
    check(bool(out), "River output failed");
}
RiverVertex vertex(Reader &r) {
    const auto x = r.scalar(), y = r.scalar(), h = r.scalar(), p = r.scalar(), o = r.scalar();
    return {x, y, h, p, o};
}
auto triangles(Reader &r) {
    const auto count = r.word();
    check(count <= 65536 && std::size_t(count) * 60 <= r.bytes.size() - r.at, "Invalid river triangle count");
    std::vector<RiverTriangle> result;
    for (std::uint32_t i = 0; i < count; ++i) {
        const auto a = vertex(r), b = vertex(r), c = vertex(r);
        result.push_back({a, b, c});
    }
    return result;
}
void run(Reader &r, const fs::path &folder) {
    const auto width = r.integer(), height = r.integer();
    const auto pc = r.word();
    check(pc == 24 || pc == 53, "Invalid river precision");
    const auto level = r.scalar();
    const auto code = r.word();
    const auto remaps = r.take(8);
    const auto count = r.word();
    check(count <= 256, "Invalid river code count");
    std::vector<std::uint32_t> codes;
    for (std::uint32_t i = 0; i < count; ++i)
        codes.push_back(r.word());
    const HeightDomain domain(width, height);
    check(std::uint64_t(width) * height <= 16777216, "Excessive river field");
    const auto h = r.blob(), m = r.blob();
    check(h.size() == std::size_t(width) * height * 2 && m.size() == h.size() / 2, "Incomplete river input fields");
    std::vector<std::uint16_t> heights;
    for (std::size_t i = 0; i < h.size(); i += 2)
        heights.push_back(std::uint16_t(h[i]) | std::uint16_t(h[i + 1]) << 8);
    const auto banks = triangles(r), channels = triangles(r);
    r.end();
    RiverRasterSettings settings{level, pc == 24 ? X87Precision::single : X87Precision::double_precision,
                                  {}, {}, codes, code};
    std::copy_n(remaps.begin(), 4, settings.remap_from.begin());
    std::copy_n(remaps.begin() + 4, 4, settings.remap_to.begin());
    Writer stores, coverage;
    const auto result = apply_river_triangles(
        domain, heights, m, banks, channels, settings,
        [&](RiverPass pass, RiverWriteChannel channel, std::int32_t index, std::uint32_t value) {
            stores.byte(static_cast<std::uint8_t>(pass));
            stores.byte(static_cast<std::uint8_t>(channel));
            stores.word(index);
            stores.word(value);
        },
        [&](RiverPass pass, std::size_t triangle, std::int32_t x, std::int32_t y, const std::array<float, 3> &w) {
            coverage.byte(static_cast<std::uint8_t>(pass));
            coverage.word(static_cast<std::uint32_t>(triangle));
            coverage.word(x);
            coverage.word(y);
            for (auto v : w)
                coverage.word(std::bit_cast<std::uint32_t>(v));
        },
        [&](RiverPass pass, const RiverRasterField &field) {
            Writer h;
            h.u16(field.heights);
            const auto prefix = pass == RiverPass::banks ? "banks" : "";
            save(folder / (std::string(prefix) + (pass == RiverPass::banks ? "Height.bin" : "height.bin")), h.bytes);
            save(folder / (std::string(prefix) + (pass == RiverPass::banks ? "Material.bin" : "material.bin")), field.materials);
        });
    save(folder / "visited.bin", result.bank_visited);
    save(folder / "stores.bin", stores.bytes);
    save(folder / "coverage.bin", coverage.bytes);
}
} // namespace
#ifdef _WIN32
int wmain(int argc, wchar_t **argv) {
#else
int main(int argc, char **argv) {
#endif
    try {
        check(argc == 3, "Usage: river_raster_oracle inputs.bin fresh-output");
        const fs::path input = argv[1], output = argv[2];
        check(!fs::exists(output), "River output must be fresh");
        const auto size = fs::file_size(input);
        check(size >= 12 && size <= 256 * 1024 * 1024, "Unsupported river input file size");
        Bytes bytes(static_cast<std::size_t>(size));
        std::ifstream in(input, std::ios::binary);
        in.read(reinterpret_cast<char *>(bytes.data()), bytes.size());
        check(bool(in), "River input read failed");
        Reader r{bytes};
        const auto magic = r.take(8);
        check(std::string(magic.begin(), magic.end()) == "C5RRAS01", "Wrong river input magic");
        const auto count = r.word();
        check(count > 0 && count <= 4096, "Invalid river case count");
        check(fs::create_directories(output), "Cannot create river output");
        for (std::uint32_t i = 0; i < count; ++i) {
            const auto folder = output / std::to_string(i);
            check(fs::create_directory(folder), "Cannot create river case output");
            Reader item{r.blob()};
            run(item, folder);
        }
        r.end();
        std::cout << "PASS river cases=" << count << '\n';
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
