

#include "civ5/graphics/bnw_unit_visual_owner.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <iomanip>
#include "civ5/graphics/bnw_unit_member_transform_rules.hpp"
#include "gl_bnw_warrior_owned_scene.hpp"
#include "bnw_warrior_gr2_metadata_cache.hpp"
#include "civ5/terrain/terrain_source_rivers.hpp"

// Source-bound validation of generated Civis placement. The camera is a
// focused inspection camera, not an original-game camera capture. Terrain
// uses the existing controlled source generator, independently checked against
// the post-load live map; it is not a universal new-game terrain loader.
int main(int argc,char **argv) {
    try {
        if(argc!=8)throw std::runtime_error("Usage: owned_scene SAVE GAME_ROOT ASSET_ROOT SDK_CACHE TERRAIN_INPUT FEATURE2 OUTPUT_DIR");
        using namespace civ5;
        const fs::path save=argv[1],game=argv[2],assets=argv[3],sdk=argv[4],output=argv[7];
        check(fs::equivalent(assets,fs::path("C:/CrazyProject/civ5-assets")),"Canonical shared AssetRoot required");
        const auto save_identity=runtime::identify_file(save);
        check(save_identity.sha256=="b3a54d5c54d2085236e1c156830643f82dfd78853aee5e1b661c32530a6e82ab","Canonical selected source differs");
        auto bootstrap=ui::bootstrap_selected_bnw_preview(save,game,0);
        if(bootstrap.preview.blocker)throw std::runtime_error(bootstrap.preview.blocker->reason);
        check(bool(bootstrap.preview.host),"BNW selected live host unavailable");
        auto &host=*bootstrap.preview.host;
        const auto &live=host.session().state();const auto &snapshot=live.world;
        const auto map_rng=live.random.map.state(),game_rng=live.random.gameplay.state();
        check(snapshot.map_terrain && snapshot.map_projection && snapshot.pregame && snapshot.map_plots,"Post-load map/art unavailable");
        check(snapshot.map_header.grid.width==40 && snapshot.map_header.grid.height==24 && snapshot.map_terrain->cells.size()==960,"Selected map extent differs");
        const auto *warrior=host.types().find("UNIT_WARRIOR");check(warrior!=nullptr,"Activated BNW Warrior type unavailable");
        const assets::AssetEnvironment environment(assets);
        const auto models=archive(environment,"Resource/Common/UnitModels.fpk");
        const auto config_handle=models->find(assets::AssetKey("UnitVisConfig.ini"));
        check(std::holds_alternative<assets::AssetHandle>(config_handle),"Actual UnitVisConfig FPK entry missing");
        const auto configuration=graphics::read_bnw_unit_visual_configuration(std::get<assets::AssetHandle>(config_handle));
        const graphics::BnwImportedUnitVisualOwner units(snapshot,configuration,warrior->id);
        const auto &selected=units.find(0,16385);
        check(selected.x==32 && selected.y==14 && selected.type==warrior->id && !selected.embarked,"Selected live Warrior identity differs");
        const auto generated=graphics::generate_bnw_gandhi_warrior_owned_terrain(save,assets,argv[5],argv[6],snapshot);
        const auto terrain=generated.world();const auto &generation=generated.receipt();
        const std::size_t verified=generation.verified_masks;const auto &map=snapshot.map_header;
        auto missing=snapshot;missing.map_terrain->cells.at(731).native_terrain_mask_known=false;
        bool missing_rejected=false;try{graphics::verify_bnw_warrior_owned_terrain_cells(missing,terrain->terrain().source().initial_cells);}catch(const std::invalid_argument &){missing_rejected=true;}
        check(missing_rejected,"One missing source mask accepted");
        missing=snapshot;missing.map_terrain->cells.pop_back();
        bool cardinality_rejected=false;try{graphics::verify_bnw_warrior_owned_terrain_cells(missing,terrain->terrain().source().initial_cells);}catch(const std::invalid_argument &){cardinality_rejected=true;}
        check(cardinality_rejected,"Missing source cell accepted");
        missing=snapshot;missing.map_projection->cells.at(592).x++;
        bool position_rejected=false;try{graphics::verify_bnw_warrior_owned_terrain_cells(missing,terrain->terrain().source().initial_cells);}catch(const std::invalid_argument &){position_rejected=true;}
        check(position_rejected,"Wrong source cell position accepted");
        const auto &world=terrain->terrain();const auto &height=world.owner();
        const auto center=graphics::bnw_unit_world_center(world.source().layout,height.parameters,32,14);
        const assets::LooseSource loose(assets);
        const auto member_xml=loose.find(assets::AssetKey("Assets/Units/Civ5ArtDefines_UnitMembers.xml"));
        check(std::holds_alternative<assets::AssetHandle>(member_xml),"Actual selected member XML missing");
        std::optional<graphics::BnwUnitMemberTransformRules> member_rules;
        std::string xml_sha;
        for(const auto &spec:graphics::bnw_gandhi_warrior_art) {
            const auto src=graphics::read_bnw_unit_member_transform_rules(std::get<assets::AssetHandle>(member_xml),spec.art_type,configuration.scene_spacing);
            check(src.rules.scale==.14f*configuration.scene_spacing && src.rules.z_offset==0,"Selected compiled member values differ");
            check(!member_rules || *member_rules==src.rules,"Selected members need different per-art transforms");
            member_rules=src.rules;xml_sha=src.xml_sha256;
        }
        const auto rules=*member_rules;const float member_scale=rules.scale;
        const auto matrices=graphics::bnw_warrior_owned_scene_transforms_with_rules(units,snapshot,0,16385,world.source().layout,height,map.wrap_x,graphics::bnw_unit_initial_minimum_height(),rules);
        const auto same=graphics::bnw_warrior_owned_scene_transforms_with_rules(units,snapshot,0,16385,world.source().layout,height,map.wrap_x,graphics::bnw_unit_initial_minimum_height(),rules);
        check(matrices==same,"Owned same revision placement drift");
        auto shifted_rules=rules;shifted_rules.z_offset=3;
        const auto shifted=graphics::bnw_warrior_owned_scene_transforms_with_rules(units,snapshot,0,16385,world.source().layout,height,map.wrap_x,graphics::bnw_unit_initial_minimum_height(),shifted_rules);
        for(std::size_t i=0;i<12;++i)check(shifted[i][14]==matrices[i][14]+3 && shifted[i][12]==matrices[i][12] && shifted[i][13]==matrices[i][13],"Authored ZOffset order/coordinates differ");
        bool bad_z_rejected=false;try{auto bad=rules;bad.z_offset=std::numeric_limits<float>::infinity();(void)graphics::bnw_warrior_owned_scene_transforms_with_rules(units,snapshot,0,16385,world.source().layout,height,map.wrap_x,graphics::bnw_unit_initial_minimum_height(),bad);}catch(const std::invalid_argument &){bad_z_rejected=true;}
        check(bad_z_rejected,"Invalid authored world Z accepted");
        auto context=std::make_shared<graphics::GlContext>(512,512,false);graphics::GlRenderApi gl(*context);
        auto color=graphics::GlTexture::render_target(*context,512,512,1,graphics::RenderTextureFormat::rgba8);
        auto depth=graphics::GlTexture::render_target(*context,512,512,1,graphics::RenderTextureFormat::depth24_stencil8);
        GLuint fbo=0;gl.CreateFramebuffers(1,&fbo);gl.NamedFramebufferTexture(fbo,GL_COLOR_ATTACHMENT0,color.id(),0);gl.NamedFramebufferTexture(fbo,GL_DEPTH_STENCIL_ATTACHMENT,depth.id(),0);
        const GLenum attachment=GL_COLOR_ATTACHMENT0;gl.NamedFramebufferDrawBuffers(fbo,1,&attachment);
        check(gl.CheckNamedFramebufferStatus(fbo,GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE,"Owned target incomplete");
        gl.BindFramebuffer(GL_FRAMEBUFFER,fbo);gl.Viewport(0,0,512,512);
        const auto clear_depth=reinterpret_cast<PFNGLCLEARDEPTHPROC>(context->proc_address("glClearDepth"));
        const auto clear=[&](double z){gl.DepthMask(GL_TRUE);clear_depth(z);gl.ClearColor(.03f,.04f,.06f,1);gl.Clear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);};
        const auto &original=host.session().source();
        const auto art=saves::observe_saved_map_team_art(*original.snapshot.pregame,*original.snapshot.map_plots);
        auto receipt=graphics::capture_bnw_renderer_scene_receipt(save,save_identity.sha256,original,art);
        check(receipt.has_value(),"Independent source scene receipt unavailable");
        auto renderer_projection=*original.snapshot.map_projection;
        check(graphics::reconcile_bnw_renderer_scene_projection(renderer_projection,*receipt,live),"Post-load scene receipt reconciliation failed");
        fs::create_directories(output);
        const auto malformed_cache=output/"malformed-metadata-cache",missing_cache=output/"missing-metadata-cache";
        fs::create_directories(malformed_cache);fs::create_directories(missing_cache);
        {std::ofstream bad(malformed_cache/"warrior-arttool-metadata.json");bad<<"{broken}";}
        unsigned metadata_rejected=0;
        for(const auto &bad_cache:{malformed_cache,missing_cache})try {
            graphics::GlBnwWarriorRetainedBackend bad(context,assets,bad_cache);
        }catch(const std::exception &){++metadata_rejected;}
        check(metadata_rejected==2,"Retained GL backend accepted missing/malformed metadata");
        graphics::GlBnwWarriorOwnedScene backend(context,assets,sdk,*receipt,live,generated,units,rules,0,16385);
        check(backend.metadata_sha256()==graphics::bnw_warrior_arttool_metadata_sha256,"Owned backend metadata identity differs");
        check(backend.transforms(live)==matrices,"Native metadata scalar was applied twice or placement differs");
        backend.select_sdk_sample(0);
        graphics::GlBnwWarriorFrame frame;frame.width=frame.height=512;
        const float base_z=matrices[0][14];
        frame.view={1,0,0,0,.45f,0,1,0,0,1,0,0,-center.x-.45f*center.y,-base_z-8,-center.y,1};
        frame.projection={1.f/40,0,0,0,0,1.f/25,0,0,0,0,1.f/100,0,0,0,.5f,1};
        clear(1);const auto drawn=backend.draw(live,frame);
        check(drawn.drawn && drawn.members==12 && drawn.triangles==7920 && !drawn.model_drawable,"Owned retained draw route differs");
        const auto zero=color.read_level_raw(0,0);
        const auto visible=[](const assets::Bytes &a){std::size_t n=0;for(std::size_t i=0;i<a.size();i+=4)if(a[i]>16||a[i+1]>18||a[i+2]>24)++n;return n;};
        check(visible(zero)>1000,"Selected generated Warrior invisible");
        for(int f=0;f<60;++f){clear(1);backend.draw(live,frame);}
        check(color.read_level_raw(0,0)==zero && backend.buffer_uploads()==9,"Owned retained frames reupload/drift");
        backend.select_sdk_sample(1);clear(1);backend.draw(live,frame);
        const auto one=color.read_level_raw(0,0);std::size_t motion=0;for(std::size_t i=0;i<one.size();++i)motion+=zero[i]!=one[i];check(motion>20,"Owned SDK samples did not change pose");
        clear(0);backend.draw(live,frame);check(visible(color.read_level_raw(0,0))==0,"Owned draw ignored foreground depth");
        check(live.random.map.state()==map_rng && live.random.gameplay.state()==game_rng && units.matches(snapshot),"Rendering mutated live world/RNG");
        auto changed=live;changed.world.map_terrain->cells.at(731).native_terrain_mask_known=false;
        bool live_mask_rejected=false;try{(void)backend.transforms(changed);}catch(const std::invalid_argument &){live_mask_rejected=true;}
        check(live_mask_rejected,"Retained owner accepted one unknown live mask");
        changed=live;changed.setup.game_turn++;
        bool revision_rejected=false;try{(void)backend.transforms(changed);}catch(const std::invalid_argument &){revision_rejected=true;}
        check(revision_rejected,"Retained owner accepted changed live turn");
        changed=live;changed.world.all_players->players.front().units.units.front().core.fields[3].values.front()++;
        bool roster_rejected=false;try{(void)backend.transforms(changed);}catch(const std::invalid_argument &){roster_rejected=true;}
        check(roster_rejected,"Retained owner accepted changed live unit roster");
        fs::create_directories(output);std::ofstream ppm(output/"owned-selected-warrior.ppm",std::ios::binary);ppm<<"P6\n512 512\n255\n";
        for(int y=511;y>=0;--y)for(int x=0;x<512;++x)ppm.write(reinterpret_cast<const char *>(zero.data()+(std::size_t(y)*512+x)*4),3);
        check(bool(ppm),"Owned PPM write failed");ppm.close();
        std::ofstream report(output/"owned-selected-warrior.json");report<<std::setprecision(9)
            <<"{\"schema\":\"bnw-selected-warrior-owned-scene-v1\",\"status\":\"PASS\",\"sourceSaveSha256\":\""<<save_identity.sha256
            <<"\",\"assetRoot\":\"C:/CrazyProject/civ5-assets\",\"sceneReceiptReconciled\":true,\"retainedSceneOwner\":true,\"worldSource\":\"post-load LiveSession\",\"terrainSource\":\"source-generated controlled terrain checked against selected map\",\"verifiedMapMasks\":"<<verified
            <<",\"terrainInputSha256\":\""<<generation.input_sha256<<"\",\"feature2Sha256\":\""<<generation.feature2_sha256<<"\",\"riverEdgesSha256\":\""<<generation.river_edges_sha256<<"\",\"heightPoolSha256\":\""<<generation.final_height_pool_sha256<<"\",\"originalTerrainLoadingRngRecovered\":false,\"unitOwner\":0,\"unitId\":16385,\"plotIndex\":592,\"mapXY\":[32,14],\"poolIndex\":"<<selected.pool_index<<",\"rosterSize\":"<<units.records().size()
            <<",\"configSha256\":\""<<configuration.config_sha256<<"\",\"sourcePlacementSeed\":"<<configuration.placement_seed<<",\"sceneSpacing\":"<<configuration.scene_spacing
            <<",\"worldCenter\":["<<center.x<<","<<center.y<<"],\"floor\":"<<graphics::bnw_unit_initial_minimum_height()<<",\"authoredScale\":"<<member_scale
            <<",\"memberXmlSha256\":\""<<xml_sha<<"\",\"memberZOffset\":"<<rules.z_offset<<",\"members\":12,\"triangles\":7920,\"staticFrames\":60,\"poseChangedBytes\":"<<motion<<",\"depthOcclusion\":true,\"gameRngUnchanged\":true,\"matrixRows\":[";
        for(std::size_t i=0;i<12;++i){if(i)report<<',';report<<'[';for(std::size_t j=0;j<16;++j){if(j)report<<',';report<<matrices[i][j];}report<<']';}
        report<<"],\"runtimeGr2MetadataRequired\":true,\"metadataSha256\":\""<<backend.metadata_sha256()<<"\",\"malformedBackendMetadataRejected\":"<<metadata_rejected<<",\"nativeModelLoadScaleProven\":true,\"scaleAppliedOnce\":true,\"nativeMissingZOffsetDefaultProven\":true,\"finalNativeScaleConsumptionProven\":false,\"nativePlacementAvailable\":false,\"modelDrawable\":false,\"fullGraphicsParity\":false}\n";
        check(bool(report),"Owned receipt write failed");context->check("Selected owned Warrior scene");gl.BindFramebuffer(GL_FRAMEBUFFER,0);gl.DeleteFramebuffers(1,&fbo);
        std::cout<<"PASS selected owned Warrior scene: 12 members, 7920 triangles, "<<verified<<" checked live map masks; final native scale consumption blocker remains\n";
        return 0;
    }catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
