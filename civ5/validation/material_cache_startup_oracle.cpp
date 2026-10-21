#include "civ5/materials/material_cache_startup.hpp"
#include "material_generation_fixture.hpp"
#include <iostream>

using namespace civ5::materials;
using namespace generation_validation;
#ifdef _WIN32
int wmain(int argc, wchar_t **argv) {
#else
int main(int argc, char **argv) {
#endif
    try {
        require(argc == 3, "Usage: material_cache_startup_oracle input.bin fresh-output");
        const std::filesystem::path output = argv[2];
        require(!std::filesystem::exists(output), "Fresh material startup output required");
        const auto input = read(argv[1]);
        Reader r{input};
        const auto magic = r.take(8);
        require(std::string(magic.begin(), magic.end()) == "C5MSIN01", "Wrong material startup schema");
        const auto count = r.word();
        require(count <= 65536, "Too many material startup cases");
        Bytes out{'C', '5', 'M', 'S', 'O', 'U', '0', '1'};
        word(out, count);
        for (std::uint32_t i = 0; i < count; ++i) {
            MaterialCacheAdapterInput adapter;
            // Historical fixture words follow the original table: DeviceId, VendorId.
            adapter.device_id = r.word();
            adapter.vendor_id = r.word();
            adapter.renderer_code = r.integer();
            const auto os32 = r.word();
            require(os32 <= 1, "Invalid OS bitness predicate");
            adapter.os_32bit = os32 != 0;
            const auto result = material_cache_startup(adapter);
            const auto &q = result.quality;
            for (auto value : {result.adapter_rule_index, result.preset, result.flags, result.moving_budget,
                               result.still_budget, std::uint32_t(result.fast_when_still),
                               std::uint32_t(result.force_fast), result.terrain_detail, q.tessellation,
                               result.derived_tessellation, q.compression, q.temporary_size, q.cache_size,
                               q.base_size, q.cache_mode})
                word(out, value);
            raw(out, std::span<const std::uint32_t>(q.pool_counts));
        }
        r.end();
        require(std::filesystem::create_directories(output), "Cannot create material startup output");
        write(output / "startup.bin", out);
        std::cout << "PASS material startup " << count << " cases\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
