// Automatic preparation through complete native fields and material streams.
// References are read in place; no duplicate full-world output is written.
#include "material_world_input.hpp"
#include "coast_road_network_reference.hpp"
#include <iostream>

namespace {
std::size_t compared_bytes = 0;
void equal(std::span<const std::uint8_t> actual, std::span<const std::uint8_t> expected) {
    check(std::equal(actual.begin(), actual.end(), expected.begin(), expected.end()),
          "Automatically prepared material world differs from original");
    compared_bytes += actual.size();
}
Bytes original_field(Reader &references, const fs::path &repository) {
    const fs::path relative = key(references);
    check(!relative.is_absolute() && relative.generic_string().starts_with("build/civ5_terrain/"),
          "Original field must stay under the native reference directory");
    for (const auto &part : relative)
        check(part != "..", "Original field path traversal");
    return input_bytes(repository/relative);
}
void compare_field(const MaterialWorld &world, std::span<const std::uint8_t> expected_height,
                   std::span<const std::uint8_t> expected_material) {
    const auto &owner = world.terrain().owner();
    const auto &domain = world.terrain().land().domain();
    Writer height; Bytes material;
    for (int y = 0; y < domain.height(); ++y)
        for (int x = 0; x < domain.width(); ++x) {
            const auto h = owner.height.value_at(x, y);
            height.u16(std::span(&h, 1));
            material.push_back(std::uint8_t(owner.material.value_at(x, y)));
        }
    equal(height.bytes, expected_height); equal(material, expected_material);
}
struct Expected {
    Bytes height, material, layout, constants;
    std::vector<Bytes> patches;
};
Bytes copy_blob(Reader &r) { const auto b = r.blob(); return {b.begin(), b.end()}; }
} // namespace

#ifdef _WIN32
int wmain(int argc, wchar_t **argv) {
#else
int main(int argc, char **argv) {
#endif
    try {
        check(argc >= 5 && argc <= 8, "Usage: material_preparation_world_tests asset-root primitive-input checks.bin repository-root [coast-decal-paths.bin [coast-road-network.bin [coast-scene-mask.bin]]]");
        const fs::path root = argv[1], repository = argv[4];
        Bytes decal_points, decal_ranges, road_network, scene_mask;
        std::uint32_t scene_width=0,scene_height=0;
        if (argc >= 6) {
            const auto raw = input_bytes(argv[5]); Reader decals{raw};
            const auto signature = decals.take(8);
            check(std::string(signature.begin(), signature.end()) == "C5CDW001", "Wrong coast decal references");
            decal_points = copy_blob(decals);
            const auto count = decals.word(); check(count <= max_coast_contours, "Invalid decal path count");
            Writer ranges; ranges.word(count);
            for (std::uint32_t i = 0; i < count*4; ++i) ranges.word(decals.word());
            decal_ranges = std::move(ranges.bytes); decals.end();
        }
        if (argc >= 7) {
            const auto raw = input_bytes(argv[6]); Reader network{raw};
            const auto signature = network.take(8);
            check(std::string(signature.begin(),signature.end()) == "C5RDW001", "Wrong coast road references");
            road_network = copy_blob(network); network.end();
        }
        if (argc == 8) {
            const auto raw=input_bytes(argv[7]);Reader mask{raw};const auto signature=mask.take(8);
            check(std::string(signature.begin(),signature.end())=="C5SCW001","Wrong coast scene mask reference");
            scene_width=mask.word();scene_height=mask.word();scene_mask=copy_blob(mask);mask.end();
        }
        auto bytes = input_bytes(argv[2]); Reader input{bytes};
        const auto magic = input.take(8);
        check(std::string(magic.begin(), magic.end()) == "C5MWIN01", "Wrong material input");
        Reader source{input.blob()}; const auto source_magic = source.take(8);
        check(std::string(source_magic.begin(), source_magic.end()) == "C5TWLD01", "Wrong source input");
        const auto count = source.word(); check(count == 2, "Missing two world-order controls");
        const civ5::assets::AssetKey piece_key(key(source)), coast_key(key(source));
        std::vector<WorldRequest> requests;
        for (unsigned i = 0; i < count; ++i) { Reader body{source.blob()}; requests.push_back(request(body)); }
        source.end(); check(input.word() == count, "Material request count differs");
        std::vector<MaterialRequest> materials;
        for (unsigned i = 0; i < count; ++i) { Reader body{input.blob()}; materials.push_back(material_request(body)); }
        input.end();
        const auto reference_bytes = input_bytes(argv[3]); Reader references{reference_bytes};
        const auto reference_magic = references.take(8);
        check(std::string(reference_magic.begin(), reference_magic.end()) == "C5MWP001" && references.word() == count,
              "Wrong material preparation references");
        const auto pre_height = original_field(references, repository), pre_material = original_field(references, repository);
        std::vector<MaterialWorld> worlds;
        std::vector<Expected> expected;
        {
            const civ5::assets::LooseSource loose(root);
            const auto pieces = TerrainPieceLibrary::load(loose, piece_key);
            const civ5::assets::AssetEnvironment environment(root);
            const auto models = archive(environment, "Resource/Common/TerrainModels.fpk");
            const auto textures = archive(environment, "Resource/DX9/TerrainTextures.fpk");
            const auto coasts = CoastLibrary::load(*models, coast_key, X87Precision::double_precision);
            const auto definition = models->find(civ5::assets::AssetKey("TerrainDef.csv"));
            check(std::holds_alternative<civ5::assets::AssetHandle>(definition), "Missing terrain provider");
            for (unsigned i = 0; i < count; ++i) {
                auto &q = requests[i]; auto &m = materials[i]; auto &v = q.input;
                v.expanded_logical_cells = q.cells;
                // None of these captured setup values may enter production.
                for (auto &orientation : v.orientations) orientation.fill(INT32_MIN);
                const std::array<std::uint32_t, 1> invalid{UINT32_MAX};
                v.cooked_to_primary = invalid; v.maximum_pass = -1; v.coast_palette = invalid;
                const auto coast_height = original_field(references, repository);
                const auto coast_material = original_field(references, repository);
                Expected e{original_field(references, repository), original_field(references, repository),
                           copy_blob(references), copy_blob(references), {}};
                check(references.word() == m.patches.size(), "Original patch count differs");
                for (std::size_t j = 0; j < m.patches.size(); ++j) e.patches.push_back(copy_blob(references));
                auto assets = MaterialAssets::load_terrain_provider(std::get<civ5::assets::AssetHandle>(definition), *textures);
                const LandStyleGrid prior(q.prior);
                std::size_t reciprocal_calls = 0, pre_calls = 0;
                TerrainWorldObservers observers;
                observers.pre_coast = [&](const WorkerField &field) {
                    Writer h; h.u16(field.heights()); equal(h.bytes, pre_height);
                    equal(field.materials(), pre_material); ++pre_calls;
                };
                auto world = MaterialWorld::build(pieces, coasts, std::move(assets), v,
                    {prior, q.masks, nullptr, nullptr, [&](int n) { return Bytes(n, std::uint8_t(q.fill)); }},
                    [&](float determinant) {
                        check(reciprocal_calls < 2 && std::bit_cast<std::uint32_t>(determinant) == q.reciprocal[reciprocal_calls*2],
                              "World reciprocal input differs");
                        return std::bit_cast<float>(q.reciprocal[reciprocal_calls++*2+1]);
                    }, [&](unsigned, std::span<const CoastDispatchJob> phase) {
                        std::vector<std::size_t> order(phase.size()); std::iota(order.begin(), order.end(), 0);
                        if (q.order) std::reverse(order.begin(), order.end()); return order;
                    }, observers);
                check(reciprocal_calls == 2 && pre_calls == 1, "World observation count differs");
                check(world.terrain().source().cooked_to_primary == q.mapping, "Prepared mapping differs");
                compare_field(world, coast_height, coast_material);
                worlds.push_back(std::move(world).with_rivers(m.edges, m.seed,
                    [&](float squared) { return m.rivers.take(squared); }, m.precision));
                m.rivers.end(); expected.push_back(std::move(e));
                std::cout << "PASS automatically prepared coast/river world " << i << '\n' << std::flush;
            }
        }
        references.end(); requests.clear(); bytes.clear(); bytes.shrink_to_fit();
        for (unsigned i = 0; i < count; ++i) {
            auto world = std::move(worlds[i]); auto &m = materials[i]; const auto &e = expected[i];
            if (argc >= 6) {
                const auto &decals = world.terrain().coast().decal_paths;
                Writer points, ranges;
                for (auto p : decals.points) { points.scalar(p.x); points.scalar(p.y); }
                ranges.word(static_cast<std::uint32_t>(decals.paths.size()));
                for (auto p : decals.paths) {
                    ranges.word(p.first); ranges.word(p.count); ranges.word(p.source); ranges.word(p.closed);
                }
                equal(points.bytes, decal_points); equal(ranges.bytes, decal_ranges);
                std::cout << "PASS retained coast decals " << i << ": " << decals.paths.size() << " paths, "
                          << decals.points.size() << " points\n" << std::flush;
            }
            if (argc >= 7) {
                const auto &network = world.terrain().coast().road_network;
                equal(coast_road_reference::snapshot(network),road_network);
                std::size_t points=0; for (const auto &segment:network.segments) points+=segment.points.size();
                std::cout << "PASS retained coast road network " << i << ": " << network.segments.size()
                          << " segments, " << network.junctions.size() << " junctions, " << points << " points\n" << std::flush;
            }
            if (argc == 8) {
                const auto &mask=world.terrain().coast().scene_mask_after_coast;
                check(mask.width()==scene_width && mask.height()==scene_height,"Retained coast scene mask dimensions");
                equal(Bytes(mask.flags().begin(),mask.flags().end()),scene_mask);
                std::cout<<"PASS retained coast scene mask "<<i<<": "<<mask.width()<<'x'<<mask.height()<<"\n"<<std::flush;
            }
            compare_field(world, e.height, e.material);
            Writer layout; cache_layout(layout, world.cache()); equal(layout.bytes, e.layout);
            std::array<float, 384> values; values.fill(std::bit_cast<float>(m.constants_fill));
            world.write_constants(values); Writer constants;
            for (auto f : values) constants.scalar(f); equal(constants.bytes, e.constants);
            for (std::size_t j = 0; j < m.patches.size(); ++j) {
                auto &p = m.patches[j];
                const auto patch = world.build_patch(p.index, p.settings,
                    [&](int, int, float squared) { return p.scores.take(squared); },
                    [&](int, int, float squared) { return p.vertices.take(squared); });
                p.scores.end(); p.vertices.end(); equal(material_patch_bytes(patch), e.patches[j]);
            }
            std::cout << "PASS original material world " << i << ": " << m.patches.size()
                      << " complete patches after input/library destruction\n" << std::flush;
        }
        std::cout << "PASS automatic material preparation: " << compared_bytes << " original bytes\n";
        return 0;
    } catch (const std::exception &e) { std::cerr << "FAIL " << e.what() << '\n'; return 1; }
}
