// Original archives and primitive map/edge state -> owned material world.
#include "material_world_input.hpp"
#ifdef _WIN32
int wmain(int argc, wchar_t **argv) {
#else
int main(int argc, char **argv) {
#endif
    try {
        check(argc == 4, "Usage: material_world_oracle asset-root primitive-input fresh-output");
        const fs::path root = argv[1], output = argv[3];
        check(!fs::exists(output), "Material world output must be fresh");
        auto bytes = input_bytes(argv[2]);
        Reader r{bytes};
        const auto magic = r.take(8);
        check(std::string(magic.begin(), magic.end()) == "C5MWIN01", "Wrong material world schema");
        Reader wr{r.blob()};
        const auto world_magic = wr.take(8);
        check(std::string(world_magic.begin(), world_magic.end()) == "C5TWLD01",
              "Wrong material world source schema");
        const auto count = wr.word();
        check(count >= 1 && count <= 16, "Invalid material world count");
        const civ5::assets::AssetKey piece_key(key(wr)), coast_key(key(wr));
        std::vector<WorldRequest> worlds_input;
        for (unsigned i = 0; i < count; ++i) {
            Reader source{wr.blob()};
            worlds_input.push_back(request(source));
            check(worlds_input.back().keys.empty(),
                  "Computed material palette must not enter primitive inputs");
        }
        wr.end();
        check(r.word() == count, "Material world/river count differs");
        std::vector<MaterialRequest> material_input;
        for (unsigned i = 0; i < count; ++i) {
            Reader mr{r.blob()};
            material_input.push_back(material_request(mr));
        }
        r.end();
        check(fs::create_directories(output), "Cannot create material world output");
        std::vector<MaterialWorld> worlds;
        {
            const civ5::assets::LooseSource loose(root);
            const auto pieces = TerrainPieceLibrary::load(loose, piece_key);
            civ5::assets::AssetEnvironment environment(root);
            const auto models = archive(environment, "Resource/Common/TerrainModels.fpk");
            const auto textures = archive(environment, "Resource/DX9/TerrainTextures.fpk");
            const auto coasts = CoastLibrary::load(*models, coast_key, X87Precision::double_precision);
            const auto definition = models->find(civ5::assets::AssetKey("TerrainDef.csv"));
            check(std::holds_alternative<civ5::assets::AssetHandle>(definition),
                  "Unresolved exact terrain provider");
            Writer source;
            write_asset(source, *pieces.asset(), pieces.serialized_bytes());
            save(output / "source-pieces.bin", source.bytes);
            write_sources(coasts, output / "source-coasts.bin");
            for (unsigned i = 0; i < count; ++i) {
                auto &q = worlds_input[i];
                auto &m = material_input[i];
                auto &input = q.input;
                input.cooked_to_primary = q.mapping;
                input.expanded_logical_cells = q.cells;
                input.coast_palette = {};
                check(input.map_width == m.map_width && input.map_height == m.map_height &&
                          input.mode == m.mode,
                      "Material world/river source map differs");
                const auto path = output / std::to_string(i);
                check(fs::create_directory(path), "Cannot create material world case");
                auto assets = MaterialAssets::load_terrain_provider(
                    std::get<civ5::assets::AssetHandle>(definition), *textures);
                Writer traces, current, pre;
                Bytes pre_material, height_mask, material_mask;
                std::size_t jobs = 0, reciprocal_calls = 0;
                TerrainWorldObservers observers;
                observers.pre_coast = [&](const WorkerField &field) {
                    pre.u16(field.heights());
                    pre_material.assign(field.materials().begin(), field.materials().end());
                    height_mask.assign(field.heights().size(), 0);
                    material_mask.assign(field.heights().size(), 0);
                };
                observers.coast_write = [&](CoastWriteChannel channel, int index, std::uint32_t value) {
                    const bool material = channel == CoastWriteChannel::material;
                    (material ? material_mask : height_mask).at(index) = 1;
                    current.bytes.push_back(material);
                    current.word(index);
                    current.word(value);
                };
                observers.coast_job_complete = [&](std::size_t index) {
                    check(index == jobs++, "Material coast job completion order differs");
                    traces.blob(current.bytes);
                    current.bytes.clear();
                };
                const LandStyleGrid prior(q.prior);
                auto world = MaterialWorld::build(
                    pieces, coasts, std::move(assets), input,
                    {prior, q.masks, nullptr, nullptr, [&](int n) { return Bytes(n, std::uint8_t(q.fill)); }},
                    [&](float determinant) {
                        check(reciprocal_calls < 2 && std::bit_cast<std::uint32_t>(determinant) ==
                                                          q.reciprocal[reciprocal_calls * 2],
                              "Material world reciprocal input differs");
                        return std::bit_cast<float>(q.reciprocal[reciprocal_calls++ * 2 + 1]);
                    },
                    [&](unsigned, std::span<const CoastDispatchJob> phase) {
                        std::vector<std::size_t> order(phase.size());
                        std::iota(order.begin(), order.end(), 0);
                        if (q.order)
                            std::reverse(order.begin(), order.end());
                        return order;
                    },
                    observers);
                check(reciprocal_calls == 2, "Material world reciprocal observations unused");
                save(path / "worker-height.bin", pre.bytes);
                save(path / "worker-material.bin", pre_material);
                save(path / "coast-traces.bin", traces.bytes);
                save(path / "coast-height-mask.bin", height_mask);
                save(path / "coast-material-mask.bin", material_mask);
                write_field(world.terrain(), path, "coast-");
                const auto &coast = world.terrain().coast();
                Writer chain;
                chain.word(static_cast<std::uint32_t>(coast.curves.size()));
                for (std::size_t c = 0; c < coast.curves.size(); ++c) {
                    write_curves(chain, coast.curves[c]);
                    write_strip(chain, coast.strips[c]);
                }
                write_plan(chain, coast.dispatch, coast.strips);
                save(path / "coast-chain.bin", chain.bytes);
                Writer store, coverage;
                TerrainRiverObservers river_observers;
                river_observers.write = [&](RiverPass pass, RiverWriteChannel channel, int index,
                                            std::uint32_t value) {
                    store.bytes.push_back(std::uint8_t(pass));
                    store.bytes.push_back(std::uint8_t(channel));
                    store.word(index);
                    store.word(value);
                };
                river_observers.sample = [&](RiverPass pass, std::size_t triangle, int x, int y,
                                             const std::array<float, 3> &w) {
                    coverage.bytes.push_back(std::uint8_t(pass));
                    coverage.word(static_cast<std::uint32_t>(triangle));
                    coverage.word(x);
                    coverage.word(y);
                    for (auto f : w)
                        coverage.scalar(f);
                };
                river_observers.pass_complete = [&](RiverPass pass, const RiverRasterField &field) {
                    if (pass == RiverPass::banks) {
                        Writer h;
                        h.u16(field.heights);
                        save(path / "banksHeight.bin", h.bytes);
                        save(path / "banksMaterial.bin", field.materials);
                    }
                };
                auto complete = std::move(world).with_rivers(
                    m.edges, m.seed, [&](float squared) { return m.rivers.take(squared); }, m.precision,
                    river_observers);
                m.rivers.end();
                save(path / "stores.bin", store.bytes);
                save(path / "coverage.bin", coverage.bytes);
                worlds.push_back(std::move(complete));
                std::cout << "PASS authored material world/river " << i << ", " << jobs << " coast jobs\n"
                          << std::flush;
            }
        }
        worlds_input.clear();
        worlds_input.shrink_to_fit();
        bytes.clear();
        bytes.shrink_to_fit();
        for (unsigned i = 0; i < count; ++i) {
            auto moved = std::move(worlds[i]);
            auto &q = material_input[i];
            const auto path = output / std::to_string(i);
            write_field(moved.terrain(), path, "");
            write_retained(moved.terrain(), path);
            Writer layout;
            cache_layout(layout, moved.cache());
            save(path / "cache-layout.bin", layout.bytes);
            std::array<float, 384> constants;
            constants.fill(std::bit_cast<float>(q.constants_fill));
            moved.write_constants(constants);
            Writer c;
            for (auto f : constants)
                c.scalar(f);
            save(path / "constants.bin", c.bytes);
            for (std::size_t ordinal = 0; ordinal < q.patches.size(); ++ordinal) {
                auto &p = q.patches[ordinal];
                Writer instruction_estimates;
                auto seed = [&](Estimates &values, int x, int y, float squared) {
                    const auto native = values.take(squared);
                    instruction_estimates.word(x);
                    instruction_estimates.word(y);
                    instruction_estimates.scalar(squared);
                    instruction_estimates.scalar(native);
                    instruction_estimates.scalar(_mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(squared))));
                    return native;
                };
                auto patch = moved.build_patch(
                    p.index, p.settings,
                    [&](int x, int y, float squared) { return seed(p.scores, x, y, squared); },
                    [&](int x, int y, float squared) { return seed(p.vertices, x, y, squared); });
                p.scores.end();
                p.vertices.end();
                const auto folder = path / ("patch-" + std::to_string(ordinal));
                check(fs::create_directory(folder), "Cannot create material patch output");
                output_patch(patch, folder);
                save(folder / "instruction-estimates.bin", instruction_estimates.bytes);
                std::cout << "PASS world " << i << " material patch " << p.index << '\n' << std::flush;
            }
        }
        std::cout << "PASS owned provider/world/material pipeline after "
                     "input/library destruction\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
