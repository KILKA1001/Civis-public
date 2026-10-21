#include "civ5/assets/lean.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>

using namespace civ5::assets;
namespace {
unsigned checks = 0;
void check(bool ok) {
    ++checks;
    if (!ok)
        throw std::runtime_error("LEAN assertion failed");
}
template <class F> void rejects(F action) {
    bool failed = false;
    try {
        action();
    } catch (const std::exception &) {
        failed = true;
    }
    check(failed);
}
void put(Bytes &bytes, std::size_t at, std::uint32_t value) {
    for (unsigned i = 0; i < 4; ++i)
        bytes.at(at + i) = static_cast<std::uint8_t>(value >> (i * 8));
}
Bytes fixture(unsigned width, unsigned height, unsigned mips) {
    Bytes bytes(36, 0xcc);
    for (const auto [at, value] : std::initializer_list<std::pair<unsigned, unsigned>>{
             {0, 0x4e41454c}, {4, 0x3ea8f5c3}, {8, width}, {12, height}, {16, mips}, {20, 2}})
        put(bytes, at, value);
    bytes[32] = 1;
    for (unsigned map = 0; map < 2; ++map) {
        put(bytes, 24 + 4 * map, static_cast<unsigned>(bytes.size()));
        auto w = width, h = height;
        for (unsigned mip = 0; mip < mips; ++mip) {
            for (unsigned i = 0; i < w * h * (map ? 2 : 8); ++i)
                bytes.push_back(static_cast<std::uint8_t>(i * 97 + mip * 41 + map * 13));
            w = std::max(1u, w / 2);
            h = std::max(1u, h / 2);
        }
    }
    return bytes;
}
} // namespace
int main() {
    try {
        for (const auto dims : {std::array{5u, 3u, 3u}, std::array{1u, 8u, 4u}, std::array{8u, 1u, 2u},
                                std::array{1u, 1u, 1u}, std::array{256u, 64u, 9u}}) {
            const auto raw = fixture(dims[0], dims[1], dims[2]);
            auto data = LeanData::parse(raw);
            check(data.width() == dims[0] && data.height() == dims[1] && data.mip_count() == dims[2]);
            check(data.original_bytes() == raw && data.gradient_scale_bits() == 0x3ea8f5c3);
            check(data.source_swap_flag() == 1);
            for (auto map : {LeanMap::rgba16, LeanMap::luminance16}) {
                const auto &last = data.levels(map).back();
                check(last.width == std::max(1u, dims[0] >> (dims[2] - 1)));
                check(last.height == std::max(1u, dims[1] >> (dims[2] - 1)));
                for (const auto &level : data.levels(map)) {
                    const auto payload = data.payload(map, level.mip);
                    check(payload.size() == level.length);
                    check(std::equal(payload.begin(), payload.end(), raw.begin() + level.offset));
                }
                rejects([&] { data.payload(map, dims[2]); });
            }
            rejects([&] { data.levels(static_cast<LeanMap>(2)); });
        }
        const auto raw = fixture(5, 3, 3);
        // Every truncation of a small complete file, both offset fields, giant
        // dimensions, invalid formats and mip overflow must fail before upload.
        for (std::size_t size = 0; size < raw.size(); ++size)
            rejects([&] { LeanData::parse(Bytes(raw.begin(), raw.begin() + size)); });
        for (auto [at, value] : std::initializer_list<std::pair<unsigned, unsigned>>{{0, 0},
                                                                                     {8, 0},
                                                                                     {12, 0},
                                                                                     {8, 0xffffffff},
                                                                                     {12, 0xffffffff},
                                                                                     {16, 0},
                                                                                     {16, 4},
                                                                                     {16, 0xffffffff},
                                                                                     {20, 1},
                                                                                     {20, 3},
                                                                                     {24, 0},
                                                                                     {24, 37},
                                                                                     {28, 36},
                                                                                     {28, 0xffffffff}})
            rejects([&] {
                auto changed = raw;
                put(changed, at, value);
                LeanData::parse(changed);
            });
        rejects([&] {
            auto changed = raw;
            changed.push_back(0);
            LeanData::parse(changed);
        });
        for (unsigned flag : {0u, 1u, 7u, 255u}) {
            auto changed = raw;
            changed[32] = static_cast<std::uint8_t>(flag);
            check(LeanData::parse(changed).original_bytes() == changed);
        }
        std::cout << "LEAN owned raw maps: " << checks << " assertions\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
