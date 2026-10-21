// Offline validation transport: input parameters only, no expected payloads.
#include "civ5/materials/material_sources.hpp"
#include <bit>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace civ5::materials;
using Bytes = civ5::assets::Bytes;
namespace {
void require(bool ok, const char *why) {
    if (!ok)
        throw std::runtime_error(why);
}
void word(Bytes &out, std::uint32_t v) {
    for (unsigned i = 0; i < 4; ++i)
        out.push_back(static_cast<std::uint8_t>(v >> (i * 8)));
}
void blob(Bytes &out, std::span<const std::uint8_t> v) {
    word(out, static_cast<std::uint32_t>(v.size()));
    out.insert(out.end(), v.begin(), v.end());
}
struct Reader {
    std::span<const std::uint8_t> data;
    std::size_t at = 0;
    std::span<const std::uint8_t> take(std::size_t n) {
        require(n <= data.size() - at, "Truncated material input");
        auto v = data.subspan(at, n);
        at += n;
        return v;
    }
    std::uint32_t word() {
        const auto b = take(4);
        return b[0] | (std::uint32_t(b[1]) << 8) | (std::uint32_t(b[2]) << 16) | (std::uint32_t(b[3]) << 24);
    }
    std::span<const std::uint8_t> blob() { return take(word()); }
    template <class T> std::vector<T> values() {
        const auto count = word();
        require(count <= 64, "Oversized material input array");
        std::vector<T> v;
        for (std::uint32_t i = 0; i < count; ++i)
            v.push_back(std::bit_cast<T>(word()));
        return v;
    }
    void end() { require(at == data.size(), "Trailing material input"); }
};
Bytes run(std::uint32_t op, Reader input) {
    Bytes result;
    if (op == 1) {
        const auto projections = input.values<std::int32_t>();
        const auto diffuse = input.values<std::uint32_t>(), height = input.values<std::uint32_t>();
        MaterialExpansion e(projections);
        word(result, static_cast<std::uint32_t>(e.source_rows().size()));
        for (auto v : e.source_rows())
            word(result, static_cast<std::uint32_t>(v));
        for (auto v : e.source_modes())
            word(result, static_cast<std::uint32_t>(v));
        word(result, static_cast<std::uint32_t>(e.base_slots().size()));
        for (auto v : e.base_slots())
            word(result, static_cast<std::uint32_t>(v));
        const auto packets = input.word();
        require(packets <= 64, "Oversized packet count");
        word(result, packets);
        for (std::uint32_t i = 0; i < packets; ++i) {
            const auto slots = e.ordinary_slots(input.values<std::int32_t>(), diffuse, height);
            for (auto value : slots) {
                result.push_back(static_cast<std::uint8_t>(value));
                result.push_back(static_cast<std::uint8_t>(value >> 8));
            }
        }
    } else if (op == 2) {
        const auto scales = input.values<float>();
        const auto rows = input.values<std::int32_t>(), modes = input.values<std::int32_t>();
        const float minimum = std::bit_cast<float>(input.word()),
                    maximum = std::bit_cast<float>(input.word());
        std::array<float, 384> resident{}; // explicit zero initial allocation in these native cases
        write_material_constants(rows, modes, scales, maximum - minimum, resident);
        for (float v : resident)
            word(result, std::bit_cast<std::uint32_t>(v));
    } else if (op == 3) {
        word(result, 7);
        for (const auto &mip : material_resource_zero()) {
            word(result, mip.width);
            word(result, mip.height);
            blob(result, mip.rgba);
        }
    } else
        throw std::runtime_error("Unknown material opcode");
    input.end();
    return result;
}
} // namespace
int main(int argc, char **argv) {
    try {
        require(argc == 3, "Usage: civ5_material_oracle input.bin fresh-output.bin");
        require(!std::filesystem::exists(argv[2]), "Refusing output overwrite");
        const auto size = std::filesystem::file_size(argv[1]);
        require(size >= 12 && size <= 16 * 1024 * 1024, "Invalid input size");
        Bytes data(static_cast<std::size_t>(size));
        std::ifstream in(argv[1], std::ios::binary);
        in.read(reinterpret_cast<char *>(data.data()), static_cast<std::streamsize>(data.size()));
        require(bool(in), "Input read failed");
        Reader r{data};
        auto magic = r.take(8);
        require(std::string_view(reinterpret_cast<const char *>(magic.data()), 8) == "C5MATS01",
                "Wrong material schema");
        const auto count = r.word();
        require(count > 0 && count <= 1000, "Invalid material record count");
        Bytes out(magic.begin(), magic.end());
        word(out, count);
        for (std::uint32_t i = 0; i < count; ++i) {
            const auto op = r.word();
            auto label = r.blob();
            require(!label.empty() && label.size() <= 1024, "Invalid material label");
            auto value = run(op, Reader{r.blob()});
            word(out, op);
            blob(out, label);
            blob(out, value);
        }
        r.end();
        std::ofstream output(argv[2], std::ios::binary);
        output.write(reinterpret_cast<const char *>(out.data()), static_cast<std::streamsize>(out.size()));
        output.close();
        require(bool(output), "Output write failed");
        std::cout << "PASS generated " << count << " material records / " << out.size() << " output bytes\n";
    } catch (const std::exception &e) {
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
