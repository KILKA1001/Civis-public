#include "civ5/materials/terrain_decal_store.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace civ5::materials;
namespace {
void require(bool ok, const char *why) { if (!ok) throw std::runtime_error(why); }
template<class F> void rejects(F f) {
    bool caught = false;
    try { f(); } catch (const std::invalid_argument &) { caught = true; }
    require(caught, "Invalid world domain accepted");
}
void guards() {
    for (float bad : {0.f, -1.f, std::numeric_limits<float>::infinity(),
                      std::numeric_limits<float>::quiet_NaN()})
        for (unsigned i = 0; i < 4; ++i) {
            std::array<float, 4> values{1024, 768, 1, 1}; values[i] = bad;
            rejects([&] { terrain_decal_store_domain(values[0], values[1], values[2], values[3]); });
        }
    rejects([] { terrain_decal_store_domain(268435456, 1, 1, 1); });
    rejects([] { terrain_decal_store_domain(262144, 262144, 1, 1); });
    rejects([] { terrain_decal_store_domain(1, 1, std::numeric_limits<float>::min(), 1); });
}
void oracle(const char *path) {
    std::ifstream in(path, std::ios::binary); std::array<char, 8> magic{};
    require(bool(in.read(magic.data(), 8)) && magic == std::array<char, 8>{'C','5','D','D','1','0','6',0}, "Domain oracle header");
    const auto word = [&] {
        std::array<unsigned char, 4> b{}; require(bool(in.read(reinterpret_cast<char *>(b.data()), 4)), "Truncated domain oracle");
        return std::uint32_t(b[0]) | std::uint32_t(b[1])<<8 | std::uint32_t(b[2])<<16 | std::uint32_t(b[3])<<24;
    };
    const auto count = word(); require(count > 0 && count <= 1000, "Domain case count");
    for (unsigned c = 0; c < count; ++c) {
        std::array<float, 4> input{}; for (auto &v : input) v = std::bit_cast<float>(word());
        const auto domain = terrain_decal_store_domain(input[0], input[1], input[2], input[3]);
        for (float v : domain.bounds) require(std::bit_cast<std::uint32_t>(v) == word(), "Domain bounds differ from original");
        const TerrainDecalStore store(domain.bounds, domain.columns, domain.rows, 0);
        for (float v : {store.scales().x, store.scales().y}) require(std::bit_cast<std::uint32_t>(v) == word(), "Domain scale differs from original");
        require(domain.columns == word() && domain.rows == word(), "Domain dimensions differ from original");
        require(store.cell_heads().size() == std::size_t(domain.columns)*domain.rows, "Grid size");
        for (auto head : store.cell_heads()) require(head == no_terrain_decal_slot, "Initial domain cell is not empty");
    }
    require(in.peek() == std::char_traits<char>::eof(), "Trailing domain corpus bytes");
    std::cout << "PASS original world decal domain: " << count << " cases / " << count*32 << " state bytes\n";
}
}
int main(int argc, char **argv) {
    try { guards(); for (int i = 1; i < argc; ++i) oracle(argv[i]); }
    catch (const std::exception &e) { std::cerr << "FAIL " << e.what() << '\n'; return 1; }
}
