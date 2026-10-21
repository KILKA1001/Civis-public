#include "civ5/materials/material_cache_settings.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <stdexcept>

static_assert(sizeof(void *) == 8 && std::endian::native == std::endian::little);
using namespace civ5::materials;
namespace {
std::uint32_t read(std::istream &stream) {
    std::uint32_t value = 0;
    if (!stream.read(reinterpret_cast<char *>(&value), 4))
        throw std::runtime_error("Truncated input");
    return value;
}
void write(std::ostream &stream, std::uint32_t value) {
    stream.write(reinterpret_cast<const char *>(&value), 4);
}
} // namespace
int main(int argc, char **argv) {
    try {
        if (argc != 3)
            throw std::runtime_error("usage: configured startup input output");
        std::ifstream input(argv[1], std::ios::binary);
        std::ofstream output(argv[2], std::ios::binary);
        const auto count = read(input);
        if (count > 10000)
            throw std::runtime_error("Too many configuration cases");
        write(output, count);
        for (std::uint32_t i = 0; i < count; ++i) {
            const auto length = read(input);
            if (length > 10000)
                throw std::runtime_error("Oversized path");
            std::string path(length, '\0');
            if (!input.read(path.data(), length))
                throw std::runtime_error("Truncated path");
            MaterialCacheAdapterInput adapter;
            // Historical fixture words follow the original table: DeviceId, VendorId.
            adapter.device_id = read(input);
            adapter.vendor_id = read(input);
            adapter.renderer_code = std::bit_cast<std::int32_t>(read(input));
            const auto os32 = read(input);
            if (os32 > 1)
                throw std::runtime_error("Invalid OS predicate");
            adapter.os_32bit = os32 != 0;
            const auto saved =
                load_material_cache_ini(std::filesystem::path(std::u8string(path.begin(), path.end())));
            const auto result = material_cache_configure(adapter, saved.overrides);
            const auto &s = result.material;
            const auto &q = s.quality;
            for (auto value : {s.adapter_rule_index, s.preset, s.flags, s.moving_budget, s.still_budget,
                               std::uint32_t(s.fast_when_still), result.minimize_gray_tiles,
                               result.block_on_load, s.terrain_detail, q.tessellation, s.derived_tessellation,
                               q.compression, q.temporary_size, q.cache_size, q.base_size, q.cache_mode})
                write(output, value);
            for (auto value : q.pool_counts)
                write(output, value);
            write(output, std::uint32_t(s.force_fast));
            write(output, std::uint32_t(saved.loaded));
        }
        if (input.peek() != std::char_traits<char>::eof() || !output)
            throw std::runtime_error("Invalid IO");
        std::cout << "PASS configured material startup " << count << " cases\n";
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
