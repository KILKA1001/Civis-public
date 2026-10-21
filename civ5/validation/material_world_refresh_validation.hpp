#pragma once
namespace {
void validate_material_world_refresh(std::shared_ptr<GlContext> context, const fs::path &root,
    PrimitiveInput &input, const std::array<fs::path, 4> &shaders, const fs::path &land_shaders,
    const LightingCubeAssets &cubes, std::span<const std::uint8_t> geometry,
    const GlMaterialWorldLighting &light, GlLandMaterialInputs scene, const fs::path &out) {
    const civ5::assets::AssetEnvironment environment(root);
    const auto textures = archive(environment, "Resource/DX9/TerrainTextures.fpk");
    const auto found = textures->find(civ5::assets::AssetKey("marsh_stamp.dds"));
    check(std::holds_alternative<civ5::assets::AssetHandle>(found), "Authored marsh mask missing");
    TerrainMarshStamp::Storage prior;
    for (std::size_t i = 0; i < prior.size(); ++i) prior[i] = std::uint8_t(i*37+19);
    const auto stamp = TerrainMarshStamp::prepare(civ5::assets::DdsData::parse(
        std::get<civ5::assets::AssetHandle>(found).read_bytes(65536)), prior);
    const std::array<TerrainFeature2Source, 3> sources{{{{-10, -6, 16}, 2, 1}, {{0, 0, 0}, 2, 2}, {{10, 6, -16}, 2, 3}}};
    std::vector<std::int32_t> writes;
    const auto world = build_world(root, input, 0, &stamp, sources,
        [&](auto, std::int32_t index, auto) { writes.push_back(index); });
    check(!writes.empty() && world->terrain().marshes(), "Marsh GPU fixture made no material writes");
    const auto count = world->cache().patches.size();
    std::vector<std::uint32_t> compare_patches;
    for (auto at : writes) {
        const auto width = world->terrain().owner().parameters.width;
        const auto x = (at%width)/64, y = (at/width)/64;
        const auto id = std::uint32_t(x*world->cache().rows+y);
        if (std::find(compare_patches.begin(), compare_patches.end(), id) == compare_patches.end())
            compare_patches.push_back(id);
    }
    GlMaterialWorldRenderer renderer(context, world, cubes, {false, true, true},
        LightingCubeSampling::measured_coordinates, LandShadowArithmetic::measured_fma_t_x_z_y,
        LandMipSampling::d3d9_trilinear);
    const auto field = world->terrain().height_field();
    NativeWorld native(shaders, land_shaders, world->assets(), cubes, 64, geometry,
                       field.extract_a8_texture(0, 0), renderer.source_bindings());
    mip_validation::NativeMipReader reader(native.cache.device.value);
    const MaterialNormalSeed seed = [](int, int, float squared) {
        return _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(squared)));
    };
    const MaterialPatchSettings patch_settings{64, 0, 0, 0, X87Precision::double_precision};
    renderer.enable_scheduling(std::array<std::uint32_t, 4>{std::uint32_t(count+16), 16, 16, 16}, patch_settings, {64,64});
    const auto &regions = world->terrain().marshes()->applications;
    const auto refreshed = renderer.refresh_material_regions(regions, 17, seed);
    check(refreshed > 0 && renderer.source_refresh_stamp() == 17, "Marsh region jobs or final stamp missing");
    std::size_t marked = 0, skipped_loading = 0, skipped_frame = 0;
    for (std::size_t i = 0; i < count; ++i) {
        const auto &p = renderer.scheduled_patch(i);
        if (p.source_revision == 17) {
            ++marked;
            check(p.material_parent_flag == 1 && renderer.patch_height_metrics(i), "Marsh refresh lost live patch state");
            const auto expected = world->measure_patch(i, X87Precision::double_precision);
            const auto actual = *renderer.patch_height_metrics(i);
            check(expected.first == actual.first && expected.maximum == actual.maximum && expected.category == actual.category,
                  "Renderer refresh did not retain measured heights");
            check(p.blocked == actual.category, "Measured height category did not reach the scheduler");
        }
        check(renderer.patch_state(i).requested_state == 0 && renderer.patch_state(i).completed_state == 0,
              "Source refresh falsely completed texture generation");
        skipped_loading += p.blocked == 1;
        skipped_frame += p.blocked == 1 || p.blocked == 2;
    }
    std::erase_if(compare_patches, [&](auto id) { return renderer.scheduled_patch(id).blocked == 1; });
    check(compare_patches.size() >= 2, "Expected two affected loadable patches for original shader comparison");
    compare_patches.resize(2);
    std::cout << "PASS retained refresh: " << refreshed << " jobs, " << marked << " patches, " << writes.size()
              << " marsh material writes; skip loading=" << skipped_loading << "; skip frame=" << skipped_frame
              << '\n' << std::flush;
    std::size_t generated_count = 0, mip_pairs = 0;
    const auto generated = [&](std::size_t id, const GlMaterialWorldPrepared &job) {
        ++generated_count;
        if (generated_count % 128 == 0)
            std::cout << "Loading marsh world: " << generated_count << '/' << count << '\n' << std::flush;
        if (std::find(compare_patches.begin(), compare_patches.end(), id) == compare_patches.end()) return;
        native.execute(id, job, field.extract_a8_texture(job.height_table_slot%16, job.height_table_slot/16), 64);
        const auto &texture = renderer.final_texture(id);
        for (unsigned mip = 0; mip < 7; ++mip) {
            const auto raw = texture.read_mip_raw(mip);
            const auto original = reader.read(native.finals.at(id).value, mip);
            check(raw.size()*2 == original.size(), "Refreshed mip extent differs");
            for (std::size_t px = 0; px < raw.size()/2; ++px) {
                const auto packed = unsigned(raw[px*2]) | unsigned(raw[px*2+1]) << 8;
                const unsigned codes[]{packed >> 11, (packed >> 5)&63, packed&31};
                for (unsigned c = 0; c < 3; ++c) {
                    const unsigned bits = c == 1 ? 6 : 5;
                    const float decoded = float((codes[c] << (8-bits)) | (codes[c] >> (2*bits-8)))/255.f;
                    check(std::bit_cast<std::uint32_t>(decoded) == std::bit_cast<std::uint32_t>(original[px*4+c]),
                          "Refreshed material mip differs from original D3D9 shaders");
                }
            }
            ++mip_pairs;
        }
    };
    const std::array<MaterialCacheSchedulerView, 1> views{{{identity_matrix(), {}}}};
    renderer.begin_loading(views, true, false);
    bool rejected = false;
    try { renderer.refresh_material_regions(regions, 99, seed); }
    catch (const std::invalid_argument &) { rejected = true; }
    check(rejected && renderer.source_refresh_stamp() == 17, "Refresh interrupted initial loading");
    for (std::size_t calls = 0;; ++calls) {
        check(calls <= (count+3)/4+1, "Marsh-world loading stalled");
        const auto loaded = renderer.load_step(18, seed, seed, light, 4, true, generated);
        if (loaded.finished) break;
    }
    check(generated_count == count-skipped_loading && mip_pairs == compare_patches.size()*7,
          "Incomplete refreshed world GPU check");
    for (std::size_t i = 0; i < count; ++i)
        check(renderer.patch_state(i).completed_state == (renderer.scheduled_patch(i).blocked == 1 ? 0u : 1u),
              "Initial loading ignored the refreshed height category");
    MaterialCacheSchedulerSettings settings{};
    settings.projection = identity_matrix(); settings.viewport_width = 1280;
    const auto ready = renderer.schedule_frame(views, settings, 19, seed, seed, light, 4, true);
    check(ready.generated == 0 && ready.completed == count-skipped_frame,
          "Refreshed base world failed to become drawable");
    for (std::size_t i = 0; i < count; ++i) {
        const auto category = renderer.scheduled_patch(i).blocked;
        check(renderer.patch_state(i).completed_state == (category == 1 ? 0u : category == 2 ? 1u : 3u),
              "Frame scheduling ignored the refreshed height category");
    }
    SceneTarget target(context, 1280, 960); target.begin();
    const auto frame = overview_frame();
    for (std::size_t i = 0; i < count; ++i) renderer.draw(i, frame, scene, 1280, 960);
    const auto pixels = target.pixels();
    check(std::all_of(pixels.begin(), pixels.end(), [](float v) { return std::isfinite(v); }), "Nonfinite refreshed frame");
    blend_validation::write(out/"marsh-refresh-frame.f32", pixels); renderer.finish_frame();
    std::ostringstream summary;
    summary << "PASS marsh writes=" << writes.size() << "; refresh jobs=" << refreshed << "; marked patches=" << marked
            << "; generated caches=" << generated_count << "; original D3D9 mip comparisons=" << mip_pairs
            << "; skipped loading=" << skipped_loading << "; skipped frame=" << skipped_frame << '\n'
            << "Controlled source world and three test marshes; not a saved-map frame.\n";
    blend_validation::text(out/"result.txt", summary.str()); std::cout << summary.str() << std::flush;
}
} // namespace
