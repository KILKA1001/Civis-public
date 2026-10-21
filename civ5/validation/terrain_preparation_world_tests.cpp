// Build both controlled complete worlds without captured generation setup.
// Compare existing original fields in memory; emit no new large field files.
#include "terrain_world_input.hpp"
#include <numeric>

namespace {
Bytes read_file(const fs::path &path) {
    const auto size = fs::file_size(path);
    check(size > 12 && size <= 32 * 1024 * 1024, "Invalid world reference size");
    Bytes bytes(static_cast<std::size_t>(size));
    std::ifstream file(path, std::ios::binary);
    check(bool(file.read(reinterpret_cast<char *>(bytes.data()), bytes.size())), "World reference read failed");
    return bytes;
}
void equal(std::span<const std::uint8_t> a, std::span<const std::uint8_t> b) {
    check(std::equal(a.begin(), a.end(), b.begin(), b.end()), "Prepared world differs from original field");
}
} // namespace
#ifdef _WIN32
int wmain(int argc, wchar_t **argv) {
#else
int main(int argc, char **argv) {
#endif
    try {
        check(argc == 4 || argc == 5, "Usage: civ5_terrain_preparation_world_tests asset-root world-input.bin native-world-directory [marsh-world.bin]");
        const fs::path root = argv[1], native = argv[3];
        const auto input = read_file(fs::path(argv[2]));
        Reader r{input};
        const auto magic = r.take(8);
        check(std::string(magic.begin(), magic.end()) == "C5TWLD01", "Wrong world input magic");
        const auto count = r.word(); check(count == 2, "Missing world order controls");
        const civ5::assets::AssetKey piece_key(key(r)), coast_key(key(r));
        const civ5::assets::LooseSource loose(root);
        const auto pieces = TerrainPieceLibrary::load(loose, piece_key);
        const auto source = coast_source(root, "Resource/Common/TerrainModels.fpk");
        const auto coasts = CoastLibrary::load(*source, coast_key, X87Precision::double_precision);
        const auto pre_height = read_file(native / "worker-1/pre-coast-pc53.u16");
        const auto pre_material = read_file(native / "worker-1/pre-coast-pc53.u8");
        const auto marsh_bytes = argc == 5 ? read_file(fs::path(argv[4])) : Bytes{};
        Reader marsh{marsh_bytes};
        std::optional<TerrainMarshStamp> stamp;
        std::vector<std::uint32_t> marsh_codes;
        std::vector<TerrainFeature2Source> marsh_sources;
        if (argc == 5) {
            const auto tag = marsh.take(8);
            check(std::string(tag.begin(), tag.end()) == "C5MRW001" && marsh.word() == count, "Wrong marsh world corpus");
            const auto raw_dds = marsh.blob(), raw_prior = marsh.blob();
            check(raw_prior.size() == TerrainMarshStamp::storage_bytes, "Wrong prior marsh allocation");
            TerrainMarshStamp::Storage prior_stamp;
            std::copy(raw_prior.begin(), raw_prior.end(), prior_stamp.begin());
            stamp = TerrainMarshStamp::prepare(civ5::assets::DdsData::parse(Bytes(raw_dds.begin(), raw_dds.end())), prior_stamp);
            Reader codes{marsh.blob()};
            check(codes.bytes.size()%4 == 0 && codes.bytes.size() <= 1024, "Wrong marsh world palette");
            while (codes.at < codes.bytes.size()) marsh_codes.push_back(codes.word());
            Reader records{marsh.blob()};
            check(records.bytes.size()%20 == 0, "Wrong marsh world source bytes");
            while (records.at < records.bytes.size())
                marsh_sources.push_back({{records.integer(), records.integer(), records.integer()}, records.integer(), records.integer()});
        }
        for (unsigned i = 0; i < count; ++i) {
            Reader body{r.blob()}; auto q = request(body); auto &v = q.input;
            v.expanded_logical_cells = q.cells; v.coast_palette = q.keys;
            // Deliberately invalid legacy setup proves it is not consumed.
            for (auto &m : v.orientations) m.fill(INT32_MIN);
            const std::array<std::uint32_t, 1> invalid_mapping{UINT32_MAX};
            v.cooked_to_primary = invalid_mapping; v.maximum_pass = -1;
            const LandStyleGrid prior(q.prior);
            std::size_t reciprocal_calls = 0, pre_calls = 0;
            TerrainWorldObservers observers;
            observers.pre_coast = [&](const WorkerField &field) {
                Writer h; h.u16(field.heights()); equal(h.bytes, pre_height);
                equal(field.materials(), pre_material); ++pre_calls;
            };
            auto world = TerrainWorld::build_from_library(pieces, coasts, v,
                {prior, q.masks, nullptr, nullptr, [&](int n) { return Bytes(n, std::uint8_t(q.fill)); }},
                [&](float determinant) {
                    check(reciprocal_calls < 2 && std::bit_cast<std::uint32_t>(determinant) == q.reciprocal[reciprocal_calls * 2],
                          "Computed world determinant differs");
                    return std::bit_cast<float>(q.reciprocal[reciprocal_calls++ * 2 + 1]);
                }, [&](unsigned, std::span<const CoastDispatchJob> phase) {
                    std::vector<std::size_t> order(phase.size()); std::iota(order.begin(), order.end(), 0);
                    if (q.order) std::reverse(order.begin(), order.end()); return order;
                }, observers);
            check(reciprocal_calls == 2 && pre_calls == 1, "World producer call count differs");
            check(world.source().cooked_to_primary == q.mapping, "Prepared mapping ownership differs");
            const auto &domain = world.land().domain();
            Writer height; Bytes material;
            for (int y = 0; y < domain.height(); ++y)
                for (int x = 0; x < domain.width(); ++x) {
                    const auto h = world.owner().height.value_at(x, y);
                    height.u16(std::span(&h, 1)); material.push_back(std::uint8_t(world.owner().material.value_at(x, y)));
                }
            const auto folder = native / (q.order ? "coast-reverse-1" : "coast-submission-1");
            equal(height.bytes, read_file(folder / "post-coast-pc53.u16"));
            equal(material, read_file(folder / "post-coast-pc53.u8"));
            std::cout << "PASS prepared world " << i << ": " << domain.width() << 'x' << domain.height()
                      << ", " << world.source().generation.occurrences.size() << " occurrences, "
                      << pre_height.size() + pre_material.size() + height.bytes.size() + material.size()
                      << " exact original field bytes\n" << std::flush;
            if (stamp) {
                check(marsh.word() == q.order, "Marsh world order differs");
                auto expected = material;
                Reader changes{marsh.blob()};
                check(changes.bytes.size()%5 == 0, "Malformed marsh world changes");
                while (changes.at < changes.bytes.size()) {
                    const auto index = changes.word(); check(index < expected.size(), "Marsh world index out of bounds");
                    expected[index] = changes.take(1)[0];
                }
                const auto expected_writes = marsh.blob(), expected_applications = marsh.blob();
                auto bad = marsh_sources; bad.front().continent = -1;
                const auto occurrence_count = world.source().generation.occurrences.size();
                bool rejected = false;
                try { auto unused = std::move(world).with_marshes(bad, *stamp, marsh_codes,
                    X87Precision::double_precision, Vec4Path::sse); }
                catch (const std::invalid_argument &) { rejected = true; }
                check(rejected && world.source().generation.occurrences.size() == occurrence_count,
                      "Failed marsh pass consumed original world");
                Writer writes;
                world = std::move(world).with_marshes(marsh_sources, *stamp, marsh_codes,
                    X87Precision::double_precision, Vec4Path::sse,
                    [&](std::size_t application, std::int32_t index, std::uint8_t value) {
                        writes.word(static_cast<std::uint32_t>(application)); writes.word(index); writes.bytes.push_back(value);
                    });
                equal(writes.bytes, expected_writes);
                check(world.marshes() && world.marshes()->state.enabled && world.marshes()->state.pending.empty() &&
                      world.marshes()->state.retained.size() == marsh_sources.size(), "Marsh world state differs");
                equal(world.marshes()->stamp.storage(), stamp->storage());
                check(world.marshes()->material_codes == marsh_codes, "Marsh world palette ownership differs");
                Writer applications;
                for (const auto &a : world.marshes()->applications) {
                    applications.word(a.origin_x); applications.word(a.origin_y); applications.word(a.code);
                    applications.scalar(a.dirty_lower.x); applications.scalar(a.dirty_lower.y);
                    applications.scalar(a.dirty_upper.x); applications.scalar(a.dirty_upper.y);
                }
                equal(applications.bytes, expected_applications);
                Writer final_height; Bytes final_material;
                for (int y = 0; y < world.land().domain().height(); ++y)
                    for (int x = 0; x < world.land().domain().width(); ++x) {
                        const auto value = world.owner().height.value_at(x, y);
                        final_height.u16(std::span(&value, 1));
                        final_material.push_back(static_cast<std::uint8_t>(world.owner().material.value_at(x, y)));
                    }
                equal(final_height.bytes, height.bytes); equal(final_material, expected);
                std::cout << "PASS marsh world " << i << ": " << final_height.bytes.size()+final_material.size()
                          << " original field bytes, " << writes.bytes.size()/9 << " ordered material stores\n" << std::flush;
            }
        }
        r.end();
        if (stamp) marsh.end();
    } catch (const std::exception &e) { std::cerr << "FAIL " << e.what() << '\n'; return 1; }
}
