// Input dimensions/state only. Expected native output is never loaded here.
#include "civ5/terrain/land_grid.hpp"
#include <bit>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace civ5::terrain;
namespace {
void require(bool ok, const char *reason) {
    if (!ok)
        throw std::runtime_error(reason);
}
std::uint32_t read(std::istream &in) {
    std::array<unsigned char, 4> b{};
    in.read(reinterpret_cast<char *>(b.data()), 4);
    require(bool(in), "Truncated grid fixture");
    return b[0] | (std::uint32_t(b[1]) << 8) | (std::uint32_t(b[2]) << 16) | (std::uint32_t(b[3]) << 24);
}
void write(std::ostream &out, std::uint32_t v) {
    for (unsigned i = 0; i < 4; ++i)
        out.put(static_cast<char>(v >> (8 * i)));
}
template <class T> void blob(std::ostream &out, std::span<const T> values) {
    static_assert(std::endian::native == std::endian::little);
    write(out, static_cast<std::uint32_t>(values.size_bytes()));
    out.write(reinterpret_cast<const char *>(values.data()),
              static_cast<std::streamsize>(values.size_bytes()));
    require(bool(out), "Grid output write failed");
}
} // namespace
int main(int argc, char **argv) {
    try {
        require(argc == 3, "Usage: civ5_land_grid_oracle cases.bin fresh-output.bin");
        require(!std::filesystem::exists(argv[2]), "Refusing output overwrite");
        std::ifstream in(argv[1], std::ios::binary);
        require(bool(in), "Input open failed");
        std::array<char, 8> magic{};
        in.read(magic.data(), 8);
        require(std::string_view(magic.data(), 8) == "C5GRID01", "Invalid grid fixture magic");
        const auto count = read(in);
        require(count > 0 && count <= 1000, "Invalid grid case count");
        std::ofstream out(argv[2], std::ios::binary);
        require(bool(out), "Output open failed");
        out.write(magic.data(), 8);
        write(out, count);
        std::size_t compared = 0;
        for (std::uint32_t i = 0; i < count; ++i) {
            std::array<std::uint32_t, 5> c;
            for (auto &v : c) {
                v = read(in);
                write(out, v);
            }
            require(c[0] >= 2 && c[0] <= 65 && c[1] >= 2 && c[1] <= 65 && c[2] >= 1 && c[2] <= 3 &&
                        (c[3] == 9 || c[3] == 10) && (c[4] == 24 || c[4] == 53),
                    "Invalid grid state");
            LandGrid grid(c[0], c[1], c[2]);
            const auto control =
                c[3] == 10 ? land_control_stream(c[0], c[1]).addresses : std::vector<std::uint16_t>{};
            blob(out, grid.vertices());
            blob(out, grid.indices());
            blob(out, std::span<const std::uint16_t>(control));
            compared += grid.vertices().size_bytes() + grid.indices().size_bytes() + control.size() * 2;
        }
        require(in.peek() == std::char_traits<char>::eof() && !in.bad(), "Trailing grid input");
        out.close();
        require(bool(out), "Output close failed");
        std::cout << "PASS generated " << count << " LAND grid cases / " << compared
                  << " bytes; no oracle outputs loaded\n";
    } catch (const std::exception &e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
