#include "civ5/materials/material_cache_settings.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace civ5::materials;
static_assert(sizeof(void *) == 8 && std::endian::native == std::endian::little);
namespace {
std::uint32_t read(std::istream &in) {
    std::uint32_t v = 0;
    if (!in.read(reinterpret_cast<char *>(&v), 4))
        throw std::runtime_error("Truncated input");
    return v;
}
void write(std::ostream &out, std::uint32_t value) { out.write(reinterpret_cast<const char *>(&value), 4); }
} // namespace
int main(int argc, char **argv) {
    try {
        if (argc != 3)
            throw std::runtime_error("usage: minimal load oracle input output");
        std::ifstream input(argv[1], std::ios::binary);
        std::ofstream output(argv[2], std::ios::binary);
        const auto count = read(input);
        if (count > 10000)
            throw std::runtime_error("Too many cases");
        write(output, count);
        for (std::uint32_t i = 0; i < count; ++i) {
            const auto size = read(input);
            if (size > 10000)
                throw std::runtime_error("Oversized path");
            std::string path(size, '\0');
            if (!input.read(path.data(), size))
                throw std::runtime_error("Truncated path");
            const auto loaded =
                load_material_load_ini(std::filesystem::path(std::u8string(path.begin(), path.end())));
            write(output, loaded.loaded);
            write(output, loaded.settings.minimal_build_on_load);
            const auto callers = read(input);
            if (callers > 32)
                throw std::runtime_error("Too many caller cases");
            write(output, callers);
            for (std::uint32_t j = 0; j < callers; ++j) {
                const auto present = read(input), mode = read(input);
                if (present > 1)
                    throw std::runtime_error("Invalid renderer presence");
                write(output, material_cache_initial_cull(loaded.settings, present != 0, mode));
            }
        }
        if (input.peek() != std::char_traits<char>::eof() || !output)
            throw std::runtime_error("Invalid IO");
        std::cout << "PASS native minimal load " << count << " files\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
