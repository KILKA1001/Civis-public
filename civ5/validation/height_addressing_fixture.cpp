// Validation-only executable. No research/fixture input is linked into runtime.
#include "civ5/terrain/height_addressing.hpp"
#include <bit>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>

using namespace civ5::terrain;
namespace {
class Writer {
public:
    explicit Writer(const char* path) {
        if (std::filesystem::exists(path)) throw std::runtime_error("Fresh output required");
        stream.exceptions(std::ios::failbit | std::ios::badbit);
        stream.open(path, std::ios::binary);
    }
    void word(std::uint32_t x) { for (int shift : {24, 16, 8, 0}) stream.put(static_cast<char>(x >> shift)); }
    void number(float x) { word(std::bit_cast<std::uint32_t>(x)); }
    template<std::size_t N> void numbers(const std::array<float, N>& values) { for (float x : values) number(x); }
    void uv(HeightUv x) { number(x.u); number(x.v); }
    void bytes(const std::vector<std::uint8_t>& bytes) {
        word(static_cast<std::uint32_t>(bytes.size()));
        stream.write(reinterpret_cast<const char*>(bytes.data()), bytes.size());
    }
    void finish() { stream.close(); }
private:
    std::ofstream stream;
};
void emit(Writer& out, int width, int height) {
    const HeightDomain domain(width, height);
    const LandLayout layout(domain);
    out.word(width); out.word(height); out.word(domain.texture_columns()); out.word(domain.texture_rows());
    out.word(layout.columns()); out.word(layout.rows()); out.number(layout.center_x()); out.number(layout.center_y());
    for (const int x : {std::numeric_limits<int>::min(), -1, 0, width, std::numeric_limits<int>::max()})
        for (const int y : {-1, 0, height - 1, height}) {
            out.word(domain.descriptor_destination_index(x, y).value_or(-1));
            out.word(domain.texture_source_index(x, y));
        }
    for (int x = 0; x < layout.columns(); ++x) for (int y = 0; y < layout.rows(); ++y) {
        const auto p = layout.patch(x, y);
        for (const int value : {p.patch_index(), p.source_x(), p.source_y(), p.source_samples_x(), p.source_samples_y(),
             p.texture_table_slot(), p.texture_offset_x(), p.texture_offset_y()}) out.word(value);
        out.numbers(p.offset_constants()); out.numbers(p.world_translation()); out.numbers(p.culling_bounds());
        for (const float d : {-0.0f, 0.25f, 31.5f, 64.0f, -0.5f, 65.0f}) {
            out.uv(p.displacement_uv(d, 0.5f));
            out.uv(p.height_window().source_to_uv(static_cast<float>(p.source_x()) + d, static_cast<float>(p.source_y()) + 0.5f));
        }
    }
    std::vector<std::uint16_t> source(static_cast<std::size_t>(width) * height);
    for (std::size_t i = 0; i < source.size(); ++i) source[i] = static_cast<std::uint16_t>(static_cast<std::uint32_t>(i) * 919u + 1357u);
    const HeightField field(domain, source);
    for (const float x : {-0.0f, -0.25f, 0.0f, 0.25f, 895.5f, static_cast<float>(width) - 0.5f,
                         std::numeric_limits<float>::max(), -std::numeric_limits<float>::max()})
        for (const float y : {-0.25f, 0.5f, static_cast<float>(height) + 0.25f}) out.number(field.sample_a8_linear(x, y));
    out.bytes(field.extract_a8_texture(0, 0));
    out.bytes(field.extract_a8_texture(domain.texture_columns() - 1, domain.texture_rows() - 1));
}
} // namespace
int main(int argc, char** argv) {
    try {
        if (argc != 3 && argc != 4) throw std::runtime_error("Usage: civ5_height_addressing_fixture cases.txt fresh-output.bin [--spheres-single|--spheres-double]");
        const bool spheres = argc == 4;
        X87Precision precision = X87Precision::double_precision;
        if (spheres) {
            const std::string_view flag(argv[3]);
            if (flag == "--spheres-single") precision = X87Precision::single;
            else if (flag != "--spheres-double") throw std::runtime_error("Invalid sphere precision flag");
        }
        std::ifstream input(argv[1]);
        if (!input) throw std::runtime_error("Missing cases input");
        std::vector<std::array<int, 2>> cases;
        std::string line;
        while (std::getline(input, line)) {
            if (line.find_first_not_of(" \t\r\n") == std::string::npos) continue;
            std::istringstream row(line);
            int w, h; std::string extra;
            if (!(row >> w >> h) || row >> extra) throw std::runtime_error("Invalid cases input");
            cases.push_back({w, h});
        }
        if (input.bad() || cases.empty()) throw std::runtime_error("Invalid cases input");
        Writer out(argv[2]); out.word(spheres ? 0x43354c53 : 0x43354c41); out.word(1); out.word(static_cast<std::uint32_t>(cases.size()));
        for (const auto& c : cases) {
            if (!spheres) { emit(out, c[0], c[1]); continue; }
            const LandLayout layout(HeightDomain(c[0], c[1]));
            out.word(c[0]); out.word(c[1]); out.word(layout.columns()); out.word(layout.rows());
            for (int x = 0; x < layout.columns(); ++x)
                for (int y = 0; y < layout.rows(); ++y)
                    out.numbers(layout.patch(x, y).culling_sphere(precision));
        }
        out.finish();
        std::cout << "Wrote " << cases.size() << " complete addressing/texture cases\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
