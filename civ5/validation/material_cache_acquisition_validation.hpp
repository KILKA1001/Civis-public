#pragma once
#include "civ5/materials/material_cache_acquisition.hpp"
#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <span>
#include <stdexcept>
#include <vector>

namespace cache_acquisition_validation {
using Bytes = std::vector<std::uint8_t>;
inline void require(bool ok, const char *message) {
    if (!ok)
        throw std::runtime_error(message);
}
struct Reader {
    Bytes data;
    std::size_t at = 0;
    explicit Reader(const std::filesystem::path &path) {
        std::ifstream stream(path, std::ios::binary);
        require(bool(stream), "Input file unavailable");
        data = Bytes(std::istreambuf_iterator<char>(stream), {});
    }
    std::uint32_t word() {
        require(at <= data.size() && data.size() - at >= 4, "Truncated input word");
        std::uint32_t result = 0;
        for (unsigned lane = 0; lane < 4; ++lane)
            result |= std::uint32_t(data[at++]) << (lane * 8);
        return result;
    }
    Bytes bytes(std::size_t size) {
        require(at <= data.size() && data.size() - at >= size, "Truncated input bytes");
        Bytes result(data.begin() + at, data.begin() + at + size);
        at += size;
        return result;
    }
};
inline void word(Bytes &out, std::uint32_t value) {
    for (unsigned lane = 0; lane < 4; ++lane)
        out.push_back(static_cast<std::uint8_t>(value >> (lane * 8)));
}
inline std::uint32_t get(const Bytes &data, unsigned offset) {
    std::uint32_t value = 0;
    for (unsigned lane = 0; lane < 4; ++lane)
        value |= std::uint32_t(data.at(offset + lane)) << (lane * 8);
    return value;
}
inline void put(Bytes &data, unsigned offset, std::uint32_t value) {
    for (unsigned lane = 0; lane < 4; ++lane)
        data.at(offset + lane) = static_cast<std::uint8_t>(value >> (lane * 8));
}
constexpr std::array<unsigned, 4> widths{64, 128, 256, 512};
struct PoolInput {
    std::array<std::vector<std::uint32_t>, 4> free;
    std::map<std::uint32_t, std::uint32_t> sizes;
    unsigned size_index(unsigned width) const {
        const auto found = std::find(widths.begin(), widths.end(), width);
        require(found != widths.end(), "Unsupported fixture pool width");
        return static_cast<unsigned>(found - widths.begin());
    }
};
inline void vector_words(Bytes &out, std::span<const std::uint32_t> values) {
    word(out, static_cast<std::uint32_t>(values.size()));
    for (auto value : values)
        word(out, value);
}
template <class Factory> int run(int argc, char **argv, Factory make_pool) {
    try {
        require(argc == 3, "Usage: acquisition_oracle input fresh-output");
        Reader input(argv[1]);
        const auto magic = input.bytes(8);
        require(magic == Bytes{'C', '5', 'A', 'C', 'Q', 'I', '0', '1'}, "Acquisition input schema");
        const auto count = input.word();
        require(count > 0 && count <= 20000, "Acquisition case count");
        const std::filesystem::path path(argv[2]);
        require(!std::filesystem::exists(path), "Fresh output required");
        Bytes output{'C', '5', 'A', 'C', 'Q', 'O', '0', '1'};
        word(output, count);
        unsigned calls = 0;
        for (unsigned index = 0; index < count; ++index) {
            const auto kind = input.word(), base_size = input.word(), repeats = input.word();
            require(kind <= 1 && repeats >= 1 && repeats <= 2, "Acquisition operation");
            auto patch_bytes = input.bytes(1024);
            PoolInput initial;
            const auto metadata_count = input.word();
            require(metadata_count <= 512, "Metadata extent");
            for (unsigned entry = 0; entry < metadata_count; ++entry) {
                const auto handle = input.word(), width = input.word();
                initial.size_index(width);
                require(handle != 0 && initial.sizes.emplace(handle, width).second,
                        "Duplicate/zero metadata handle");
            }
            for (unsigned slot = 0; slot < 4; ++slot) {
                const auto size = input.word();
                require(size <= 128, "Free list extent");
                for (unsigned entry = 0; entry < size; ++entry) {
                    const auto handle = input.word();
                    require(initial.sizes.contains(handle) && initial.sizes.at(handle) == widths[slot],
                            "Free texture metadata mismatch");
                    for (const auto &items : initial.free)
                        require(std::find(items.begin(), items.end(), handle) == items.end(),
                                "Duplicate entering free handle");
                    initial.free[slot].push_back(handle);
                }
            }
            auto pool = make_pool(initial);
            civ5::materials::MaterialCacheTextureState patch{
                get(patch_bytes, 0x178),
                get(patch_bytes, 0x17c),
                get(patch_bytes, 0x180),
                {get(patch_bytes, 0x190), get(patch_bytes, 0x194)},
                {get(patch_bytes, 0x198), get(patch_bytes, 0x19c)},
                get(patch_bytes, 0x2fc) != 0};
            word(output, repeats);
            for (unsigned repeat = 0; repeat < repeats; ++repeat) {
                pool.takes.clear();
                pool.returns.clear();
                const bool success =
                    kind == 0 ? civ5::materials::material_cache_acquire_base(patch, pool, base_size)
                              : civ5::materials::material_cache_acquire_higher(patch, pool, base_size);
                put(patch_bytes, 0x178, patch.higher_size);
                put(patch_bytes, 0x17c, patch.final_size);
                put(patch_bytes, 0x190, patch.higher[0]);
                put(patch_bytes, 0x194, patch.higher[1]);
                put(patch_bytes, 0x198, patch.final[0]);
                put(patch_bytes, 0x19c, patch.final[1]);
                word(output, success);
                output.insert(output.end(), patch_bytes.begin(), patch_bytes.end());
                word(output, static_cast<unsigned>(pool.takes.size()));
                for (const auto &take : pool.takes)
                    for (auto value : take)
                        word(output, value);
                vector_words(output, pool.returns);
                for (auto width : widths)
                    vector_words(output, pool.free_handles(width));
                pool.validate();
                ++calls;
            }
            pool.finish();
            if ((index + 1) % 64 == 0)
                std::cout << "checked acquisition cases " << index + 1 << '/' << count << '\n' << std::flush;
        }
        require(input.at == input.data.size(), "Trailing primitive input");
        std::ofstream stream(path, std::ios::binary);
        require(bool(stream), "Output unavailable");
        stream.write(reinterpret_cast<const char *>(output.data()),
                     static_cast<std::streamsize>(output.size()));
        require(bool(stream), "Incomplete acquisition output");
        std::cout << "PASS native acquisition " << count << " cases /" << calls << " calls\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
}
} // namespace cache_acquisition_validation
