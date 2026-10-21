#pragma once
namespace {
// End-to-end controls use the same primitive patches, authored assets and
// original shader binaries as the existing cache chain. No readback is fed
// into the complete GL execution.
template<class Patches>
void run_decal_chain(char **argv, const Patches &patches, const std::shared_ptr<GlContext> &context,
    const MaterialAssets &assets, const Palette &palette, const LightingCubeAssets &cube_assets,
    const GlMaterialCacheFrame &frame, const std::vector<GlMaterialCacheTarget> &inputs,
    const std::vector<GlMaterialCacheVolume> &volumes, const Bytes &geometry, const Bytes &a8,
    const std::vector<float> &lut) {
    namespace fixture = terrain_decal_validation;
    using civ5::terrain::TerrainDecalAssets;
    const fs::path out(argv[8]);
    const auto source = bindings();
    const auto loaded = fixture::authored(argv[2]);
    std::vector<civ5::assets::DdsData> data;
    for (const auto &t : loaded.textures()) data.push_back(t.data);
    std::ofstream report(out / "decal-chain.txt");
    report << context->description() << '\n';
    unsigned cases = 0, guards = 0;
    std::uint64_t compared = 0;
    for (unsigned size : {64u, 256u}) {
        NativeCacheChain native({argv[3],argv[4],argv[5],argv[6]},assets,cube_assets,lut,size,
                                geometry,a8,source,200,201,202);
        GlMaterialCacheTextures cache(context,material_cache_texture_plan(size,size,false),
                                      {native.automatic[0],native.automatic[2],true});
        const unsigned levels = size == 64 ? 7 : 9;
        auto final = GlTexture::render_target(*context,size,size,levels,RenderTextureFormat::rgb565);
        auto higher = GlTexture::render_target(*context,size,size,levels,RenderTextureFormat::rgba8);
        auto resources = inputs;
        for (unsigned i=0;i<13;++i)
            resources.push_back({source.textures[i],&cache.texture(static_cast<MaterialCacheRole>(i))});
        resources.push_back({201,&final}); resources.push_back({202,&higher});
        const auto &computed = patches.at(size == 64 ? 0 : 2);
        const auto material = computed.patch.mesh();
        const LightingPatchMesh lighting(material.columns(),material.rows(),computed.patch.streams.normal24,material.indices());
        GlMaterialCacheRenderer renderer(context,material,lighting,source,LightingCubeSampling::measured_coordinates);
        const auto snapshot = [&] {
            std::vector<Bytes> pixels;
            for (unsigned slot : {0u,1u,4u,3u}) {
                const auto &t=cache.texture(static_cast<MaterialCacheRole>(slot));
                pixels.emplace_back();
                for (unsigned mip=0;mip<t.levels().size();++mip) {
                    const auto b=t.read_mip_raw(mip);pixels.back().insert(pixels.back().end(),b.begin(),b.end());
                }
            }
            pixels.emplace_back();
            for (unsigned mip=0;mip<levels;++mip) {
                const auto b=final.read_mip_raw(mip);pixels.back().insert(pixels.back().end(),b.begin(),b.end());
            }
            return pixels;
        };
        std::vector<Bytes> baseline;
        for (unsigned test=0;test<12;++test) {
            fixture::Native native_decals(argv[9],data,native.device.value);
            GlTerrainDecalRenderer packed(context);
            fixture::Both both(packed,native_decals);
            fixture::Inputs textures{context,data,{}};
            TerrainDecalStore store({-512,-512,512,512},4,4,loaded.blank_specular());
            const float extent=static_cast<float>(material.columns()-1);
            if (test != 1) for (unsigned mode=0;mode<2;++mode) for (unsigned i=0;i<2;++i) {
                const unsigned family=mode ? 3 : i ? 7 : 4;
                const auto row=loaded.palette().records.at(loaded.palette().ranges[family].first);
                // Native shader adds +1 before projecting bounds [origin-1, origin+columns-2].
                store.submit(row,fixture::quad(62+extent*(.1f+i*.2f),62+extent*(.15f+i*.1f),
                                               extent*.6f,extent*.65f,3.75f),static_cast<std::uint8_t>(mode));
            }
            if (test != 2) store.upload_pending(both);
            MaterialCachePassSettings settings{size,size,test>=8?1u:0u,test==7||test==11?5u:0u,false,false,false,false,
                                                 civ5::terrain::X87Precision::double_precision};
            MaterialCachePassPatch patch{static_cast<unsigned>(material.columns()),static_cast<unsigned>(material.rows()),
                                          {64,64},1,200,0,{202,0},{201,0},0,0,0};
            auto layers=prepare_material_cache_decals(settings,patch,{64,64},store,packed.index_buffer(),143,
                {test==3||test==5||test==10?1000u:131072u,test==3||test==4||test==9?1000u:131072u});
            const auto *provider=test==0||test==8?nullptr:&layers;
            const Bytes entering(material_cache_constant_bytes(settings,provider),test%2?0xa5:0);
            const auto plan=material_cache_pass_plan(settings,patch,source,palette.get(computed.selected.sources()),entering,{},provider);
            unsigned resolves=0, expected_resolves=0;
            for (const auto &layer:layers.layers) {
                std::set<std::pair<std::uint16_t,DdsSamplingColor>> unique;
                for (const auto &d:layer.draws) {unique.emplace(d.albedo,DdsSamplingColor::srgb);unique.emplace(d.heightmap,DdsSamplingColor::linear);}
                expected_resolves+=static_cast<unsigned>(unique.size());
            }
            GlMaterialCacheDecals binding{packed,143,[&](auto id,auto color)->const GlTexture & {
                require(++resolves<=expected_resolves,"Cache resolver called again after preflight");
                return textures.get(id,color);
            }};
            const bool conditional=test%2!=0;
            renderer.execute(plan,resources,volumes,frame,conditional,provider?&binding:nullptr);
            require(resolves==(provider?expected_resolves:0),"Cache resolver count");
            native.execute(plan,material,lighting,frame,conditional,nullptr,{},[&](const auto &pass,const auto &draw) {
                const auto &p=plan.packets.at(draw.packet); TerrainDecalDraw d{};
                d.index_buffer=p.geometry[0];d.vertex_buffer=p.geometry[1];d.triangles=p.geometry[2];d.first_vertex=p.geometry[3];
                d.albedo=p.pixel_textures[0];d.heightmap=p.pixel_textures[1];
                require(draw.constant_bytes==112,"Native decal local extent");
                std::memcpy(d.constants.data(),plan.constants.data()+draw.constant_offset,100);
                native_decals.render({1,false,{d}},0,pass.blend_state,false);
            });
            const auto now=snapshot();
            if (test==0 || test==8) baseline=now;
            else if (test<4) require(now==baseline,"Empty/unuploaded/rejected decals changed complete cache");
            else {
                const bool main=test!=5 && test!=10, shadow=test!=4 && test!=9;
                require((now[0]!=baseline[0])==main && (now[1]!=baseline[1])==main &&
                            (now[2]!=baseline[2])==main && (now[3]!=baseline[3])==shadow && now[4]!=baseline[4],
                        "Decals did not propagate through height/normal/shadow/final mip chain");
            }
            const auto name=std::to_string(size)+"-"+std::to_string(test);
            for (unsigned slot : {0u,1u,4u,3u,201u}) {
                const auto &texture=slot==201?final:cache.texture(static_cast<MaterialCacheRole>(slot));
                const auto gl=texture.read_mip_raw(0);
                const auto dx=native.pixels(slot==201?201:source.textures[slot],0);
                require(gl.size()==dx.size(),"Cache chain readback extent");
                write(out/(name+"-"+std::to_string(slot)+"-gl.bin"),gl);
                write(out/(name+"-"+std::to_string(slot)+"-d3d9.bin"),dx);
                compared+=gl.size();
            }
            if (test==7 || test==11) {
                write(out/(name+"-higher-gl.bin"),higher.read_mip_raw(0));
                write(out/(name+"-higher-d3d9.bin"),native.pixels(202,0));
            }
            if (test==6) {
                // Fail late, after the earlier material/decal passes have been
                // prepared, and verify every mip remains unchanged.
                binding.resolve=[&](auto id,auto color)->const GlTexture & {return textures.get(id,color);};
                const auto rejects=[&](const auto &p,const GlMaterialCacheDecals *b) {
                    bool caught=false;
                    try {renderer.execute(p,resources,volumes,frame,true,b);} catch(const std::invalid_argument &) {caught=true;}
                    require(caught && snapshot()==now,"Rejected decal plan changed an earlier cache target");++guards;
                };
                const auto mutate=[&](auto change) {auto p=plan;change(p);rejects(p,&binding);};
                rejects(plan,nullptr);
                mutate([](auto &p){p.passes[4].blend_state=8;});
                mutate([](auto &p){p.passes[4].flags=4;});
                mutate([](auto &p){p.passes[4].targets=p.passes[2].targets;});
                mutate([](auto &p){p.passes[4].draws[0].shader_binding=999;});
                mutate([](auto &p){p.passes[4].draws[0].constant_bytes=100;});
                mutate([](auto &p){p.passes[4].draws[0].constant_offset=0xffffffff;});
                for (unsigned field=0;field<5;++field)
                    mutate([&](auto &p){p.packets[p.passes[4].draws[0].packet].geometry[field]=field==2?0:0xffffffff;});
                mutate([](auto &p){p.packets[p.passes[4].draws[0].packet].pixel_samplers[0]=0;});
                mutate([](auto &p){const std::uint32_t nan=0x7fc00000;std::memcpy(p.constants.data()+p.passes[4].draws[0].constant_offset,&nan,4);});
                auto bad=binding;bad.resolve={};rejects(plan,&bad);
                bad.resolve=[&](auto id,auto)->const GlTexture & {return textures.get(id,DdsSamplingColor::linear);};rejects(plan,&bad);
                bad.resolve=[&](auto id,auto color)->const GlTexture & {
                    return color==DdsSamplingColor::linear?cache.texture(MaterialCacheRole::height_temporary):textures.get(id,color);
                };rejects(plan,&bad);
                auto other=std::make_shared<GlContext>();GlTerrainDecalRenderer foreign(other);
                GlMaterialCacheDecals foreign_binding{foreign,143,binding.resolve};rejects(plan,&foreign_binding);
                // A currently bound FBO is not the future decal destination.
                GlRenderApi gl(*context);GLuint fbo=0;gl.CreateFramebuffers(1,&fbo);
                gl.NamedFramebufferTexture(fbo,GL_COLOR_ATTACHMENT0,textures.get(layers.layers[0].draws[0].heightmap,DdsSamplingColor::linear).id(),0);
                gl.BindFramebuffer(GL_FRAMEBUFFER,fbo);
                renderer.execute(plan,resources,volumes,frame,conditional,&binding);
                gl.DeleteFramebuffers(1,&fbo);
                require(snapshot()==now,"Future-pass preflight depended on caller framebuffer");
                renderer.rebuild_context(other);packed.rebuild_context(other);
                renderer.rebuild_context(context);packed.rebuild_context(context);
                renderer.execute(plan,resources,volumes,frame,conditional,&binding);
                require(snapshot()==now,"Complete decal cache context recovery changed pixels");
            }
            report<<"PASS "<<name<<" main="<<layers.layers[0].draws.size()<<" shadow="<<layers.layers[1].draws.size()<<'\n';
            std::cout<<"PASS decal chain "<<name<<std::endl;++cases;
        }
    }
    report<<"cases="<<cases<<" guards="<<guards<<" comparedBaseBytes="<<compared<<'\n';
}
} // namespace
