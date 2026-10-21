// Production world ownership, original GPU cache shaders, region refresh and recovery.
#include "material_world_validation.hpp"
#include "terrain_decal_gpu_fixture.hpp"
#include "civ5/terrain/scene_polygon_decals.hpp"
#include "civ5/terrain/scene_polygon_chunks.hpp"
#include "civ5/terrain/scene_polygon_owner.hpp"
#include "civ5/content/landmark_art_registry.hpp"

namespace fixture = terrain_decal_validation;
namespace {
template<class F> void rejects(F f) {
    bool caught = false;
    try { f(); } catch (const std::invalid_argument &) { caught = true; }
    check(caught, "World decal invalid/reentrant operation was accepted");
}
struct Reference {
    NativeCacheChain cache;
    fixture::Native decals;
    std::map<unsigned, Com<IDirect3DTexture9>> heights;
    std::uint32_t height_id, final_id, higher_id;
    Com<IDirect3DTexture9> higher;
    unsigned buffers = 0;
    Reference(const fs::path &base, const MaterialWorld &world, const LightingCubeAssets &cubes,
              const MaterialCachePassSource &source, const std::vector<civ5::assets::DdsData> &data,
              const civ5::assets::DdsData &zero)
        : cache({base/"material-blend-original-shaders-1", base/"material-normal-original-shaders-1",
                 base/"material-shadow-original-shaders-1", base/"material-lighting-original-shaders-1"},
                world.assets(), cubes, identity_volume().values, 64,
                read(base/"cache-raster-release-3/original-normal-geometry.bin"),
                world.terrain().height_field().extract_a8_texture(0, 0), source,
                source.textures[0]+13, source.textures[0]+14, source.textures[0]+30),
          decals(base/"decal-original-shaders-1", data, cache.device.value, &zero),
          height_id(source.textures[0]+13), final_id(source.textures[0]+14), higher_id(source.textures[0]+30) {}
    void execute(const GlMaterialWorldPrepared &job, const MaterialWorld &world,
                 const TerrainDecalStore &store, unsigned higher_size) {
        for (const auto &batch : store.batches()) if (batch.buffer) {
            while (buffers < batch.buffer) check(decals.create() == ++buffers, "Native decal buffer identity");
            check(!batch.upload, "Production world generated before pending decal upload");
            decals.upload(batch.buffer, batch.vertices);
        }
        if (!heights.contains(job.height_table_slot)) {
            const auto slot = job.height_table_slot;
            const auto bytes = world.terrain().height_field().extract_a8_texture(slot % 16, slot / 16);
            heights.emplace(slot, cache.shadows.texture(1024, bytes, true));
        }
        cache.resources[height_id] = heights.at(job.height_table_slot).value;
        bool needs_higher = false;
        for (const auto &pass : job.plan.passes) needs_higher |= pass.targets[0] == higher_id;
        if (needs_higher) {
            if (higher.value) higher.value->Release();
            higher.value = nullptr;
            hr(cache.device->CreateTexture(higher_size, higher_size, 0,
                D3DUSAGE_RENDERTARGET | D3DUSAGE_AUTOGENMIPMAP, D3DFMT_R5G6B5,
                D3DPOOL_DEFAULT, &higher.value, nullptr), "World decal higher target");
            cache.resources[higher_id] = higher.value;
        }
        const auto material = job.patch.geometry.mesh();
        const LightingPatchMesh lighting(material.columns(), material.rows(),
            job.patch.geometry.streams.normal24, material.indices());
        cache.execute(job.plan, material, lighting, job.frame, true, nullptr, {},
            [&](const MaterialCachePass &pass, const MaterialCachePassDraw &draw) {
                const auto &p = job.plan.packets.at(draw.packet);
                TerrainDecalDraw d{};
                d.index_buffer=p.geometry[0]; d.vertex_buffer=p.geometry[1];
                d.triangles=p.geometry[2]; d.first_vertex=p.geometry[3];
                d.albedo=p.pixel_textures[0]; d.heightmap=p.pixel_textures[1];
                check(draw.constant_bytes == 112, "Original decal local extent");
                std::memcpy(d.constants.data(), job.plan.constants.data()+draw.constant_offset, 100);
                decals.render({1, false, {d}}, 0, pass.blend_state, false);
            });
    }
};
}
int main(int argc, char **argv) {
    try {
        check(argc == 5 || argc >= 8,
            "Usage: civ5_gl_material_world_decals asset-root primitive-input reference-base fresh-output"
            " [save-or-directory game-root mount-plan [mod-root ...]]");
        const bool saved_art_mode=argc>=8;
        const bool mod_art_mode=argc>=9;
        const fs::path root(argv[1]), base(argv[3]), out(argv[4]);
        check(!fs::exists(out), "Fresh world decal output required");
        fs::create_directories(out);
        auto input = primitive_input(argv[2]);
        const auto world = build_world(root, input, 0);
        const auto cubes = cube_assets(root);
        auto context = std::make_shared<GlContext>();
        auto control = std::make_unique<Control>(context);
        const auto make_light = [&] {
            auto light = GlMaterialWorldLighting::from_terrain(
                terrain_lighting_defaults(X87Precision::double_precision), [](float v) {
                    return _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v)));
                });
            light.color_correct.fill(control->image(identity_volume()));
            return light;
        };
        auto light = make_light();
        const MaterialNormalSeed seed = [](int, int, float v) {
            return _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(v)));
        };
        const MaterialPatchSettings settings{64, 0, 0, 0, X87Precision::double_precision};
        GlMaterialWorldRenderer owner(context, world, cubes, {false, true, true},
                                     LightingCubeSampling::measured_coordinates);
        auto assets = std::make_shared<TerrainDecalAssets>(fixture::authored(argv[1]));
        std::vector<civ5::assets::DdsData> data;
        for (const auto &t : assets->textures()) data.push_back(t.data);
        const auto zero = civ5::assets::DdsData::parse(read(base/"decal-domain-original-1/resource-zero.dds"));
        Reference reference(base, *world, cubes, owner.source_bindings(), data, zero);
        std::vector<std::size_t> chosen;
        // Spread actual land patches across the source. Include an origin in
        // a later 1024-sample height window to detect wrong offset reuse.
        for (std::size_t i = 0; i < world->cache().patches.size(); ++i)
            if (world->measure_patch(i, settings.precision).category == 3 &&
                (chosen.empty() || i >= chosen.back()+world->cache().patches.size()/5)) chosen.push_back(i);
        check(chosen.size() >= 3, "Need separated real land patches");
        chosen.resize(3);
        bool translated = false;
        for (auto i : chosen) {
            const auto p = world->terrain().land().patch(i/world->cache().rows, i%world->cache().rows);
            translated |= world->cache().patches[i].origin != MaterialOrigin{p.texture_offset_x(), p.texture_offset_y()};
        }
        check(translated, "Missing world-origin versus height-window-offset witness");
        std::map<std::size_t, Bytes> baseline, initial;
        for (auto i : chosen) {
            owner.prepare(i, settings, seed, seed, light, 1, true);
            baseline[i] = owner.final_texture(i).read_mip_raw(0);
        }
        owner.invalidate_generated_resources();
        rejects([&] { owner.enable_decals(nullptr, {131072, 131072}); });
        owner.enable_decals(assets, {131072, 131072});
        rejects([&] { owner.enable_decals(assets, {131072, 131072}); });
        const auto &p = world->terrain().owner().parameters;
        const auto domain = terrain_decal_store_domain(p.world_span_x, p.world_span_y, p.scale_x, p.scale_y);
        check(owner.decal_store().domain() == domain.bounds && owner.decal_store().columns() == domain.columns &&
              owner.decal_store().rows() == domain.rows, "World provider lost original domain");
        const auto row = [&](unsigned family) { return assets->palette().records.at(assets->palette().ranges[family].first); };
        const auto color = row(4), shadow = row(3);
        std::vector<std::uint32_t> handles;
        const auto geometry = [&](std::size_t index, float offset = 0.f) {
            const auto &patch = world->cache().patches[index];
            return fixture::quad(float(patch.origin[0])+4+offset, float(patch.origin[1])+6,
                                 42, 39, 3.75f);
        };
        for (auto i : chosen) {
            handles.push_back(owner.submit_decal(color, geometry(i), 0, 1, seed));
            handles.push_back(owner.submit_decal(shadow, geometry(i, 5), 1, 1, seed));
        }
        // Complete production polygon path, with controlled scene-source inputs.
        // Its eight palette slots are distinct from the ten authored XML families.
        ScenePolygonDecalPalette polygon_palette;
        polygon_palette.records={color,row(7)};
        polygon_palette.ranges.fill({0,1});polygon_palette.ranges[7]={1,2};
        SceneMask polygon_mask(p.width,p.height);
        TerrainRandom polygon_random(12345);
        const SceneDirectionField direction{initial_scene_direction_angle(X87Precision::double_precision),{1024,2048}};
        std::vector<ScenePolygonDecalState> polygon_states;
        unsigned polygon_submissions=0;
        unsigned polygon_point_packets=0;
        std::vector<std::uint16_t> polygon_tiles(8*256,0xffff);
        ScenePolygonPlotSourceEvent plot_event{};
        plot_event.x=2;plot_event.selector=1;plot_event.variant=42;
        plot_event.identity=123;plot_event.gate=777;
        ScenePolygonPlotStore polygon_plots;
        ScenePolygonOwnerGrid polygon_grid{};
        polygon_grid.width=8;polygon_grid.height=8;polygon_grid.tile_ids=polygon_tiles;
        polygon_grid.plot_store=&polygon_plots;
        for(auto *matrix:{&polygon_grid.bounds_matrix,&polygon_grid.tile_matrix})
            for(unsigned diagonal:{0u,5u,10u})(*matrix)[diagonal]=1;
        ScenePolygonOwner polygon_source(polygon_grid);
        unsigned plot_random_draws=0,plot_material_calls=0,plot_notifications=0;
        std::array<std::uint32_t,20> controlled_material_ids;
        controlled_material_ids.fill(0xffffffffu);
        const std::array<ScenePolygonMaterialSource,3> plot_material_sources{{
            {11,0xffffffffu,4,1},{11,0xffffffffu,4,1},{11,0xffffffffu,4,1}}};
        populate_scene_polygon_material_rows(controlled_material_ids,11,plot_material_sources);
        check(controlled_material_ids[2*5+4]==2 && controlled_material_ids[15+4]==3,
            "Original source registry did not populate the third material row");
        check(resolve_scene_polygon_material_id(
            std::span<const std::uint32_t>(controlled_material_ids).subspan(2*5),7)==0,
            "Original row/count overlay did not resolve resource zero");
        auto fpk_result=civ5::assets::FpkSource::open(root/"Resource/Common/MiscData.fpk",
            "Resource/Common/MiscData.fpk");
        check(std::holds_alternative<std::shared_ptr<civ5::assets::FpkSource>>(fpk_result),
            "Missing base landmark FPK");
        const auto landmark_fpk=std::get<std::shared_ptr<civ5::assets::FpkSource>>(
            std::move(fpk_result));
        auto landmark_lookup=landmark_fpk->find(civ5::assets::AssetKey("civ5artdefines_landmarks.xml"));
        check(std::holds_alternative<civ5::assets::AssetHandle>(landmark_lookup),
            "Missing base landmark definitions");
        const auto landmark_bytes=std::get<civ5::assets::AssetHandle>(
            std::move(landmark_lookup)).read_bytes(1024*1024);
        const auto landmark_rows=civ5::content::read_landmark_art_rows(landmark_bytes);
        auto landmark_registry=civ5::content::build_landmark_art_registry(landmark_rows);
        check(landmark_registry.materials.size()==320 && landmark_registry.art_types.size()==77,
            "Unexpected base landmark registry");
        std::size_t selected_art_dlc=0;
        std::size_t selected_art_mods=0,mod_art_action_failures=0;
        bool mod_art_profile_verified=false;
        std::string selected_art_save_sha;
        if(saved_art_mode){
            auto saved=civ5::content::load_saved_landmark_art_registry(
                argv[5],argv[6],argv[7],{true,true,true});
            check(saved.art.registry.materials.size()>=landmark_registry.materials.size() &&
                  !saved.save_sha256.empty(),"Saved landmark art profile is empty");
            selected_art_dlc=saved.selected_dlc;
            selected_art_save_sha=saved.save_sha256;
            if(mod_art_mode){
                std::vector<fs::path> mod_roots;
                for(int i=8;i<argc;++i) mod_roots.emplace_back(argv[i]);
                auto modded=civ5::content::load_saved_mod_landmark_art_registry(
                    saved,argv[5],argv[6],argv[7],mod_roots);
                selected_art_mods=modded.selected_mods;
                mod_art_action_failures=modded.failures.size();
                mod_art_profile_verified=modded.profile_verified;
                landmark_registry=std::move(modded.registry);
            }else landmark_registry=std::move(saved.art.registry);
        }
        std::array<std::uint32_t,20> plot_material_ids;
        plot_material_ids.fill(0xffffffffu);
        populate_scene_polygon_material_rows(plot_material_ids,
            landmark_registry.id("ART_DEF_IMPROVEMENT_FARM"),landmark_registry.materials);
        const auto selected_farm_material=resolve_scene_polygon_material_id(
            std::span<const std::uint32_t>(plot_material_ids).subspan(5),1);
        check(selected_farm_material!=0xffffffffu,
            "Selected farm material range is missing");
        if(!mod_art_mode)
            check(plot_material_ids[1]==17 && plot_material_ids[6]==18 &&
                  plot_material_ids[11]==19 && selected_farm_material==18,
                "Base/DLC farm material range changed");
        const auto source_insert=polygon_source.apply_plot_source_event(plot_event,123,777,2,
            [&]{++plot_random_draws;return 9u;},
            [&](std::uint32_t row,std::uint8_t selector,std::uint8_t variant){
                check(row==61&&selector==1&&variant==42,
                    "Real source plot chose the wrong material range");
                ++plot_material_calls;
                return resolve_scene_polygon_material_id(
                    std::span<const std::uint32_t>(plot_material_ids).subspan((row-60)*5),selector);
            },identity_matrix(),{0.f,0.f},
            [](std::uint32_t){throw std::runtime_error("Unexpected source plot removal");},
            [](std::uint16_t,std::uint32_t,std::uint32_t,bool,std::span<const Point2>){
                throw std::runtime_error("Unexpected source plot mask update");
            },[](std::array<float,4>){throw std::runtime_error("Unexpected source plot dirty command");},
            [&](std::uint16_t tile){check(tile==0,"Source plot notification tile");++plot_notifications;});
        check(source_insert.action==ScenePolygonPlotEventAction::inserted&&
              source_insert.tile_id==0&&polygon_plots.active_count()==1&&
              plot_random_draws==1&&plot_material_calls==1&&plot_notifications==1,
            "Source plot construction changed RNG/material call count");
        check(polygon_plots.slots()[0][0]==selected_farm_material &&
              polygon_plots.slots()[0][1]==0 &&
              polygon_plots.slots()[0][2]==0 &&
              polygon_plots.slots()[0][3]==0,
            "Retained scene plot did not use the real farm source ID");
        std::vector<std::array<Point2,5>> polygon_expected;
        const auto submit_polygon=[&](std::uint16_t tile,std::uint32_t slot,
                std::uint32_t record,std::span<const Point2> assembled){
            const auto &points=polygon_expected.at(record);
            check(tile==0&&slot==polygon_states.size()&&record==slot&&
                assembled.size()==points.size()&&
                std::equal(assembled.begin(),assembled.end(),points.begin(),
                    [](Point2 a,Point2 b){return a.x==b.x&&a.y==b.y;}),
                "Original polygon owner changed controlled source points");
            ScenePolygonDecalState state{0x10,8,1,{absent_terrain_decal,absent_terrain_decal}};
            const auto update=update_scene_polygon_decals(assembled,state,polygon_mask,{0,0},direction,polygon_palette,
                polygon_random,CoastBoundsPrecision::double_or_extended,Vec4Path::sse,
                [](float){return std::bit_cast<float>(0x3f93ce56u);},
                [&](unsigned,std::uint32_t,const auto &material,const auto &mesh){
                    ++polygon_submissions;return owner.submit_decal(material,mesh,0,1,seed);
                });
            check(update.submitted==2&&update.mask_updates.size()==2,
                "Polygon path did not update admission and submit both layers");
            polygon_states.push_back(state);
        };
        for(auto i:chosen){
            const auto &origin=world->cache().patches[i].origin;
            const float x=float(origin[0]),y=float(origin[1]);
            const std::array<Point2,5> points{{{x+35,(y+30)*.866f},{x+62,(y+30)*.866f},
                {x+59,(y+54)*.866f},{x+45,(y+61)*.866f},{x+34,(y+47)*.866f}}};
            polygon_expected.push_back(points);
            ScenePolygonPointChunk packet{0,1,5,{points.begin(),points.end()}};
            packet.points.resize(16,Point2{0,0});
            const ScenePolygonEvent event{{2,0,4},static_cast<std::uint32_t>(i+1),0,std::move(packet)};
            ++polygon_point_packets;
            const bool defer=polygon_expected.size()==chosen.size();
            check(polygon_source.accept(event,submit_polygon,defer)==
                (defer?ScenePolygonEventResult::deferred:ScenePolygonEventResult::completed),
                "Original polygon owner did not preserve source event");
        }
        check(polygon_source.deferred_events().size()==1&&polygon_states.size()==2,
              "Polygon source did not defer its final event");
        check(polygon_source.drain_deferred(submit_polygon)==1,
              "Polygon source did not replay its final event");
        for(std::uint32_t record=0;record<polygon_states.size();++record)
            polygon_source.set_record_handles(record,polygon_states[record].handles);
        check(polygon_source.completed_count()==3&&polygon_source.plot(0).size()==3&&
              polygon_submissions==6&&polygon_random.calls()==9,"Polygon source RNG/submission count");
        auto bad = color; bad.albedo = 65535;
        rejects([&] { owner.submit_decal(bad, geometry(chosen[0]), 0, 1, seed); });
        const std::weak_ptr<const TerrainDecalAssets> retained_assets = assets;
        assets.reset();
        check(!retained_assets.expired(), "World provider did not retain its authored source");
        const auto *store_identity = &owner.decal_store();
        unsigned generated = 0, drawn = 0, guards = 0;
        std::uint64_t compared = 0;
        const auto verify = [&](std::size_t index, const GlMaterialWorldPrepared &job) {
            reference.execute(job, *world, owner.decal_store(), owner.patch_state(index).textures.higher_size);
            bool has_color = false, has_shadow = false, has_higher = false;
            for (const auto &pass : job.plan.passes) {
                has_color |= pass.kind == MaterialCachePassKind::decal;
                has_shadow |= pass.kind == MaterialCachePassKind::decal_shadow;
                has_higher |= pass.targets[0] == reference.higher_id;
                if (pass.kind == MaterialCachePassKind::decal || pass.kind == MaterialCachePassKind::decal_shadow)
                    drawn += static_cast<unsigned>(pass.draws.size());
            }
            check(has_color && has_shadow, "Owned provider missing ordered world passes");
            for (unsigned role : {0u, 1u, 4u, 3u, 201u, 202u}) {
                if (role == 202 && !has_higher) continue;
                const auto &texture = role == 201 ? owner.final_texture(index) : role == 202 ? owner.higher_texture(index) :
                    owner.temporary_texture(64, static_cast<MaterialCacheRole>(role));
                const auto actual = texture.read_mip_raw(0);
                const auto expected = reference.cache.pixels(role == 201 ? reference.final_id :
                    role == 202 ? reference.higher_id : owner.source_bindings().textures[role], 0);
                if (actual != expected) {
                    save(out/"failed-world-gl.bin", actual); save(out/"failed-world-d3d9.bin", expected);
                    throw std::runtime_error("Original world decal cache pixels differ, role="+std::to_string(role));
                }
                compared += actual.size();
            }
            save(out/("final-"+std::to_string(generated)+"-"+std::to_string(index)+".rgb565"), owner.final_texture(index).read_mip_raw(0));
            ++generated;
        };
        for (auto i : chosen) {
            auto job = owner.prepare(i, settings, seed, seed, light, 1, true);
            const auto position = static_cast<std::size_t>(std::find(chosen.begin(), chosen.end(), i)-chosen.begin());
            for (const auto &pass : job.plan.passes)
                if (pass.kind == MaterialCachePassKind::decal || pass.kind == MaterialCachePassKind::decal_shadow) {
                    const auto handle = handles[position*2+(pass.kind == MaterialCachePassKind::decal_shadow)];
                    const auto *record = owner.decal_store().find(handle);
                    const auto contains=[&](const auto *decal){return std::any_of(pass.draws.begin(),pass.draws.end(),[&](const auto &draw){
                        return job.plan.packets.at(draw.packet).geometry[3]==decal->first_vertex;
                    });};
                    check(pass.draws.size() == (pass.kind==MaterialCachePassKind::decal?3u:1u) && contains(record),
                          "World plan selected another patch's decal through the height-window offset");
                    if(pass.kind==MaterialCachePassKind::decal)
                        for(auto polygon:polygon_states[position].handles)
                            check(contains(owner.decal_store().find(polygon)),"World plan lost the corresponding source polygon");
                }
            verify(i, job);
            initial[i] = owner.final_texture(i).read_mip_raw(0);
            check(initial[i] != baseline[i], "Decals did not reach world final texture");
            save(out/("baseline-"+std::to_string(i)+".rgb565"), baseline[i]);
        }
        rejects([&] { owner.remove_decal(handles[0], 2, seed); });
        owner.invalidate_generated_resources();
        check(&owner.decal_store() == store_identity && owner.decal_store().find(handles[0])->uploaded,
              "Generated-resource invalidation lost decal source ownership");
        const auto enable_scheduler = [&] {
            owner.enable_scheduling(std::array<std::uint32_t, 4>{16, 0, 0, 0}, settings, {64, 64});
            for (std::size_t i = 0; i < world->cache().patches.size(); ++i) {
                const bool selected = std::find(chosen.begin(), chosen.end(), i) != chosen.end();
                owner.set_scheduled_patch(i, selected, selected ? 3 : 1);
            }
        };
        enable_scheduler();
        // Pending new source bytes must be uploaded before initial loading.
        const auto extra = owner.submit_decal(color, geometry(chosen[0], 9), 0, 2, seed);
        check(!owner.decal_store().find(extra)->uploaded, "Submission uploaded before original boundary");
        owner.begin_loading({}, false, false);
        rejects([&] { owner.remove_decal(extra, 3, seed); });
        while (!owner.loading_state().finished) owner.load_step(10, seed, seed, light, 1, true, verify);
        check(owner.decal_store().find(extra)->uploaded, "Loading did not upload pending decal");
        MaterialCacheSchedulerSettings schedule{};
        schedule.projection = identity_matrix(); schedule.projection[0] = schedule.projection[5] = .0001f;
        schedule.viewport_width = 64; schedule.attempt_budget = 64;
        const std::array<MaterialCacheSchedulerView, 1> views{{{identity_matrix(), {}}}};
        const auto frame = [&](unsigned stamp) { owner.schedule_frame(views, schedule, stamp, seed, seed, light, 1, true, verify); };
        frame(11); frame(12);
        const auto stamp_before = owner.source_refresh_stamp();
        owner.remove_decal(extra, 20, seed);
        check(owner.source_refresh_stamp() == stamp_before, "Decal removal stamped the whole source owner");
        check(owner.scheduled_patch(chosen[0]).source_revision == 20 && owner.patch_height_metrics(chosen[0]),
              "Removal did not run the source-region refresh job");
        check(owner.scheduled_patch(chosen[1]).source_revision != 20 &&
              owner.scheduled_patch(chosen[2]).source_revision != 20,
              "A local decal change refreshed distant patches");
        frame(21); frame(22);
        check(owner.final_texture(chosen[0]).read_mip_raw(0) == initial.at(chosen[0]), "Removal did not restore the prior final cache");
        const auto pending = owner.submit_decal(color, geometry(chosen[0], 12), 0, 30, seed);
        check(owner.source_refresh_stamp() == stamp_before && !owner.decal_store().find(pending)->uploaded,
              "Submission changed owner stamp or skipped pending state");
        const auto reentrant = [&](std::size_t index, const GlMaterialWorldPrepared &job) {
            rejects([&] { owner.remove_decal(handles[0], 40, seed); }); ++guards; verify(index, job);
        };
        owner.schedule_frame(views, schedule, 31, seed, seed, light, 1, true, reentrant);
        check(guards, "No generation callback exercised mutation guard");
        auto zero_material = color; zero_material.albedo = zero_material.heightmap = 0;
        const auto pending_recovery = owner.submit_decal(zero_material, geometry(chosen[1], 8), 0, 40, seed);
        const auto live = owner.decal_store().small_live()+owner.decal_store().large_live();
        auto replacement = std::make_shared<GlContext>();
        owner.rebuild_context(replacement, cubes);
        check(&owner.decal_store() == store_identity && !owner.decal_store().find(pending_recovery)->uploaded &&
              owner.decal_store().small_live()+owner.decal_store().large_live() == live,
              "Context replacement lost source handles/pending state");
        context = replacement; control = std::make_unique<Control>(context); light = make_light();
        enable_scheduler(); owner.begin_loading({}, false, false);
        while (!owner.loading_state().finished) owner.load_step(50, seed, seed, light, 1, true, verify);
        check(owner.decal_store().find(pending_recovery)->uploaded, "Recovered pending source was not uploaded");
        const auto saved_record = *owner.decal_store().find(handles[0]);
        bool callback_failed = false;
        const MaterialNormalSeed failing_seed = [](int, int, float) -> float {
            throw std::runtime_error("Controlled world refresh producer failure");
        };
        try { owner.remove_decal(handles[0], 60, failing_seed); }
        catch (const std::runtime_error &) { callback_failed = true; }
        const auto *restored_record = owner.decal_store().find(handles[0]);
        check(callback_failed && restored_record && restored_record->bounds == saved_record.bounds &&
              restored_record->first_vertex == saved_record.first_vertex &&
              owner.decal_store().small_live()+owner.decal_store().large_live() == live,
              "Failed refresh did not roll back the removed source handle");
        for (auto i : chosen) check(owner.patch_state(i).completed_state == 0, "Failed refresh kept partial generated caches");
        owner.invalidate_generated_resources(); enable_scheduler(); owner.begin_loading({}, false, false);
        while (!owner.loading_state().finished) owner.load_step(70, seed, seed, light, 1, true, verify);
        for(const auto &state:polygon_states)for(auto handle:state.handles)
            check(owner.decal_store().find(handle)&&owner.decal_store().find(handle)->uploaded,
                "Polygon source handles did not survive world recovery");
        const auto before_polygon_removal=owner.final_texture(chosen[0]).read_mip_raw(0);
        save(out/"polygon-before-removal.rgb565",before_polygon_removal);
        const auto mask_before_removal=std::vector<std::uint8_t>(
            polygon_mask.flags().begin(),polygon_mask.flags().end());
        unsigned removal_dirty=0,removal_mask_calls=0;
        auto refused_event=plot_event;refused_event.gate=778;
        const auto removal=polygon_source.apply_plot_source_event(refused_event,123,777,7,
            []()->std::uint32_t{throw std::runtime_error("Rejected source plot drew RNG");},
            [](std::uint32_t,std::uint8_t,std::uint8_t)->std::uint32_t{
                throw std::runtime_error("Rejected source plot selected material");
            },identity_matrix(),{0.f,0.f},
            [&](std::uint32_t handle){owner.remove_decal(handle,80,seed);},
            [&](std::uint16_t tile,std::uint32_t slot,std::uint32_t index,
                bool setting,std::span<const Point2> points){
                check(tile==0&&slot==index&&!setting,"Unexpected polygon removal mask call");
                const auto updates=polygon_mask.polygon(points,{0,0},setting,
                    CoastBoundsPrecision::double_or_extended,
                    [](float){return std::bit_cast<float>(0x3f93ce56u);});
                check(updates.size()==2,"Polygon removal lost mask commands");
                ++removal_mask_calls;
            },[&](std::array<float,4> region){
                check(region[0]<region[2]&&region[1]<region[3],
                    "Polygon dirty command has an invalid region");
                ++removal_dirty;
            },[](std::uint16_t){throw std::runtime_error("Rejected source plot notified cell");});
        check(removal.action==ScenePolygonPlotEventAction::removed&&
              removal.tile_id==0xffffu&&
              removal_mask_calls==3&&removal_dirty==1&&
              polygon_plots.free_head()==0&&polygon_plots.active_count()==0&&
              polygon_source.active_record_count()==0&&polygon_source.plot(0).empty()&&
              !std::equal(mask_before_removal.begin(),mask_before_removal.end(),
                  polygon_mask.flags().begin()),"Source polygon removal did not clear the scene");
        frame(81);frame(82);
        const auto after_polygon_removal=owner.final_texture(chosen[0]).read_mip_raw(0);
        save(out/"polygon-after-removal.rgb565",after_polygon_removal);
        check(after_polygon_removal!=before_polygon_removal,
            "Removing source polygon layers had no visible cache effect");
        std::ofstream report(out/"report.json");
        report << "{\"generated\":" << generated << ",\"decalDraws\":" << drawn << ",\"exactBytes\":" << compared
               << ",\"reentrantGuards\":" << guards << ",\"polygonSubmissions\":" << polygon_submissions
               << ",\"polygonRandomCalls\":" << polygon_random.calls()
               << ",\"polygonPointPackets\":" << polygon_point_packets
               << ",\"polygonPointPacketConnected\":true,\"polygonEventOwnerConnected\":true,\"polygonProducerConnected\":true,\"refreshFailureRecovered\":true,\"worldProviderConnected\":true"
               << ",\"savedArtConnected\":" << (saved_art_mode?"true":"false")
               << ",\"savedArtDlc\":" << selected_art_dlc
               << ",\"savedArtSaveSha256\":\"" << selected_art_save_sha
               << "\",\"savedModArtConnected\":" << (mod_art_mode?"true":"false")
               << ",\"savedModArtProfileVerified\":" << (mod_art_profile_verified?"true":"false")
               << ",\"savedModArtCount\":" << selected_art_mods
               << ",\"savedModArtActionFailures\":" << mod_art_action_failures
               << ",\"savedArtRegistryRows\":" << landmark_registry.materials.size()
               << ",\"selectedFarmMaterial\":" << selected_farm_material
               << ",\"savedSceneConnected\":false}\n";
        report.close(); check(bool(report), "World decal report write");
        std::cout << "PASS world decals generated=" << generated << " draws=" << drawn << " exactBytes=" << compared
                  << " guards=" << guards << std::endl;
    } catch (const std::exception &e) { std::cerr << "FAIL " << e.what() << std::endl; return 1; }
}
