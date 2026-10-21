#include "material_cache_geometry_fixture.hpp"
#include "material_generation_fixture.hpp"
#include <bit>

namespace cv = cache_geometry_validation;
namespace gv = generation_validation;
using namespace civ5::materials;
namespace {
struct Source {
    int width, height;
    gv::Bytes bytes;
    std::uint16_t height_at(int x, int y) const {
        gv::require(x >= 0 && x < width && y >= 0 && y < height, "Geometry source address");
        std::uint16_t value;
        std::memcpy(&value, bytes.data() + (std::size_t(y) * width + x) * 2, 2);
        return value;
    }
};
}
int main(int argc, char **argv) {
    try {
        gv::require(argc == 3, "Geometry job primitive input and fresh output required");
        const auto input = gv::read(argv[1]);
        gv::Reader in{input};
        const auto magic = in.take(8);
        gv::require(std::string_view(reinterpret_cast<const char *>(magic.data()), 8) == "C5GJOBI1",
                    "Geometry job input schema");
        const auto source_count = in.word();
        gv::require(source_count > 0 && source_count <= 8, "Geometry source count");
        std::vector<Source> sources;
        for (unsigned i = 0; i < source_count; ++i) {
            const auto width = in.word(), height = in.word();
            gv::require(width && height && std::uint64_t(width) * height <= 32000000, "Geometry source size");
            const auto bytes = in.take(std::size_t(width) * height * 2);
            sources.push_back({static_cast<int>(width), static_cast<int>(height), {bytes.begin(), bytes.end()}});
        }
        const auto count = in.word();
        gv::require(count > 0 && count <= 10000, "Geometry job case count");
        gv::Bytes out{'C', '5', 'G', 'J', 'O', 'B', 'O', '1'};
        gv::word(out, count);
        for (unsigned i = 0; i < count; ++i) {
            const auto &source = sources.at(in.word());
            const auto stride = in.word(), precision = in.word(), availability = in.word(), wrap = in.word();
            gv::require((precision == 24 || precision == 53) && availability < 4 && wrap < 2,
                        "Geometry primitive settings");
            civ5::terrain::Vector3 scales;
            for (auto &value : scales)
                value = std::bit_cast<float>(in.word());
            const auto before = in.take(1024);
            cv::Case test({before.begin(), before.end()});
            cv::Backend backend(test);
            cv::observe_staging(backend);
            backend.failure_mask = availability ^ 3;
            const MaterialCacheGeometryDomain domain{
                {static_cast<int>(cv::get(test.entering, 0xd4)), static_cast<int>(cv::get(test.entering, 0xd8)),
                 static_cast<int>(stride)},
                {std::bit_cast<std::int32_t>(cv::get(test.entering, 0xdc)),
                 std::bit_cast<std::int32_t>(cv::get(test.entering, 0xe0))},
                source.width, source.height, scales,
                precision == 24 ? civ5::terrain::X87Precision::single : civ5::terrain::X87Precision::double_precision,
                wrap != 0};
            material_cache_prepare(test.state, domain.patch, backend, false, 0x27000000);
            const auto prepared = backend.snapshot();
            unsigned status = 0, height_calls = 0;
            if (test.state.prepared) {
                gv::require(backend.queued == &test.state.geometry_job, "Native geometry job not queued");
                try {
                    material_cache_execute_geometry_job(*backend.queued, domain, backend, [&](int x, int y) {
                        ++height_calls;
                        return source.height_at(x, y);
                    });
                    status = 1;
                } catch (const std::length_error &) {
                    status = 2;
                    gv::require(!height_calls && backend.snapshot() == prepared,
                                "Rejected geometry changed owner or visited source");
                }
            } else
                gv::require(!backend.queued && backend.allocations.empty(), "Failed preparation retained work");
            backend.validate_guards();
            gv::word(out, status);
            gv::blob(out, prepared);
            gv::blob(out, backend.snapshot());
            gv::word(out, static_cast<std::uint32_t>(backend.events.size()));
            for (const auto &event : backend.events) {
                gv::word(out, event.kind);
                gv::word(out, static_cast<std::uint32_t>(event.arguments.size()));
                for (const auto argument : event.arguments) {
                    gv::word(out, static_cast<std::uint32_t>(argument));
                    gv::word(out, static_cast<std::uint32_t>(argument >> 32));
                }
                gv::blob(out, event.patch);
            }
            for (unsigned slot = 0; slot < 2; ++slot) {
                const auto bytes = test.state.staging[slot] ? backend.staging_memory(test.state.staging[slot])
                                                           : std::span<std::uint8_t>{};
                gv::blob(out, bytes);
            }
            gv::word(out, height_calls);
            material_cache_clear_staging(test.state, backend);
            gv::require(backend.allocations.empty(), "Geometry cleanup leaked owned staging");
        }
        in.end();
        gv::write(argv[2], out);
        std::cout << "PASS native geometry preparation and workers " << count << " cases\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
