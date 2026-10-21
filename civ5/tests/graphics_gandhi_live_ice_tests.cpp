#include "../validation/gandhi_minimap_live_ice.hpp"
#include <filesystem>
#include <stdexcept>

void require(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

int main(int argc, char **argv) {
    using civ5::graphics::validate_gandhi_live_ice_record;
    using civ5::graphics::gandhi_live_ice_record_sha256;
    using civ5::graphics::gandhi_live_ice_save_sha256;
    civ5::ui::MinimapNativeIceRecords records;
    records.seed = 0; // Captured diagnostic envelope marks the unknown RNG seed.
    records.shader_box = std::array<float, 4>{-1182.1975f, -765.7902f, 2046.4596f, 710.5345f};
    records.packed_vertices = 19465;
    records.indices = 93126;
    records.regions.resize(1);
    records.regions[0].vertices.resize(19465);
    records.regions[0].indices.resize(93126);
    validate_gandhi_live_ice_record(gandhi_live_ice_save_sha256,
                                    gandhi_live_ice_record_sha256, records);
    const auto must_reject = [&](auto &&test) {
        bool rejected = false;
        try { test(); } catch (const std::runtime_error &) { rejected = true; }
        require(rejected, "Gandhi live ice invalid source was accepted");
    };
    must_reject([&] { validate_gandhi_live_ice_record(
        "another-save", gandhi_live_ice_record_sha256, records); });
    must_reject([&] { validate_gandhi_live_ice_record(
        gandhi_live_ice_save_sha256, "another-record", records); });
    records.indices -= 3;
    must_reject([&] { validate_gandhi_live_ice_record(
        gandhi_live_ice_save_sha256, gandhi_live_ice_record_sha256, records); });
    if (argc == 2) {
        auto actual = civ5::ui::read_minimap_native_ice_records(
            std::filesystem::path(argv[1]));
        validate_gandhi_live_ice_record(gandhi_live_ice_save_sha256,
                                        gandhi_live_ice_record_sha256, actual);
        require(actual.regions[0].indices.size() / 3 == 31042,
                "Gandhi live ice triangle count differs");
    }
}
