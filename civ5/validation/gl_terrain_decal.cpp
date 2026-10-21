#include "terrain_decal_gpu_fixture.hpp"
#include "civ5/graphics/gl_terrain_decal.hpp"
#include "civ5/graphics/gl_render_api.hpp"
#include "civ5/graphics/gl_material_cache_raster.hpp"
#include "civ5/materials/material_cache_raster.hpp"
#include "civ5/terrain/terrain_decal_geometry.hpp"
#include "material_blend_native.hpp"
#include <cmath>
#include <iomanip>
#include <iostream>
#include <map>

using namespace civ5::graphics;
using namespace civ5::materials;
using namespace civ5::terrain;
namespace assets=civ5::assets;
using blend_validation::Com;
using blend_validation::hr;
using blend_validation::require;
namespace {
using namespace terrain_decal_validation;
template<class I> std::vector<float> draw_gl(GlTerrainDecalRenderer &renderer,const TerrainDecalDrawList &list,I &inputs) {
    auto context=inputs.context;context->api();GlRenderApi gl(*context);
    std::array<GlTexture,2> targets{GlTexture::render_target(*context,W,H,1,RenderTextureFormat::rgba32_float),
                                  GlTexture::render_target(*context,W,H,1,RenderTextureFormat::rgba32_float)};
    GLuint fbo=0;gl.CreateFramebuffers(1,&fbo);
    try {
        for(unsigned i=0;i<2;++i)gl.NamedFramebufferTexture(fbo,GL_COLOR_ATTACHMENT0+i,targets[i].id(),0);
        const std::array<GLenum,2> attachments{GL_COLOR_ATTACHMENT0,GL_COLOR_ATTACHMENT1};gl.NamedFramebufferDrawBuffers(fbo,2,attachments.data());
        require(gl.CheckNamedFramebufferStatus(fbo,GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE,"GL decal MRT");
        gl.BindFramebuffer(GL_FRAMEBUFFER,fbo);gl.Viewport(0,0,W,H);
        for(auto state:{GL_BLEND,GL_CULL_FACE,GL_DEPTH_TEST,GL_STENCIL_TEST,GL_SCISSOR_TEST,GL_DITHER,GL_FRAMEBUFFER_SRGB})gl.Disable(state);
        gl.ColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);const std::array<float,4> zero{};
        for(unsigned i=0;i<2;++i)gl.ClearNamedFramebufferfv(fbo,GL_COLOR,i,zero.data());
        renderer.draw(list,[&](auto id,auto color)->const GlTexture &{return inputs.get(id,color);});
        std::array<float,4> viewport{};gl.GetFloati_v(GL_VIEWPORT,0,viewport.data());
        GLint depth=0;context->api().GetIntegerv(GL_CLIP_DEPTH_MODE,&depth);
        require(viewport==std::array<float,4>{0,0,W,H}&&depth==GL_NEGATIVE_ONE_TO_ONE,"Decal changed caller clip state");
        std::vector<float> pixels(2*W*H*4);
        for(unsigned i=0;i<2;++i) {const auto raw=targets[i].read_level_raw(0,0);std::memcpy(pixels.data()+i*W*H*4,raw.data(),raw.size());}
        gl.BindFramebuffer(GL_FRAMEBUFFER,0);gl.DeleteFramebuffers(1,&fbo);return pixels;
    } catch(...) {gl.BindFramebuffer(GL_FRAMEBUFFER,0);gl.DeleteFramebuffers(1,&fbo);throw;}
}
template<class I> assets::Bytes cache_gl(GlTerrainDecalRenderer &renderer,const TerrainDecalDrawList &list,I &inputs,bool shadow) {
    const auto format=shadow?RenderTextureFormat::rgba8:RenderTextureFormat::rgba16_float;
    std::array<GlTexture,2> textures{GlTexture::render_target(*inputs.context,W,H,1,format),
                                  GlTexture::render_target(*inputs.context,W,H,1,format)};
    const std::array<GlMaterialCacheTarget,2> map{{{1,&textures[0]},{2,&textures[1]}}};
    const auto targets=std::span(map).first(shadow?1:2);
    MaterialCachePass pass{};pass.kind=shadow?MaterialCachePassKind::decal_shadow:MaterialCachePassKind::decal;
    pass.targets=shadow?std::array<std::uint32_t,4>{1,0,0,0}:std::array<std::uint32_t,4>{1,2,0,0};
    pass.depth_surface=77;pass.flags=4;pass.clear_color=material_cache_clear_rgba(0x33557799);
    GlMaterialCacheRaster raster(inputs.context);raster.begin(pass,targets,false);
    const auto background=textures[0].read_mip_raw(0);
    pass.flags=0;pass.blend_state=shadow?4:8;
    raster.begin(pass,targets,true);
    require(textures[0].read_mip_raw(0)==background,"Decal pass cleared existing cache");
    renderer.draw(list,[&](auto id,auto color)->const GlTexture &{return inputs.get(id,color);});
    raster.end();assets::Bytes pixels;
    require(textures[0].read_mip_raw(0)!=background,"Cache decal produced no pixels");
    for(const auto &t:targets) {const auto raw=t.texture->read_mip_raw(0);pixels.insert(pixels.end(),raw.begin(),raw.end());}
    return pixels;
}
std::array<float,16> matrix() {return {1.f/256,0,0,0,0,1.f/192,0,0,0,0,1,0,-1.f/256,-1.f/192,0,1};}
template<class F> void rejects(F f) {
    bool caught=false;try{f();}catch(const std::invalid_argument &){caught=true;}require(caught,"Invalid GPU decal input accepted");
}
float compare(std::span<const float> a,std::span<const float> b) {
    require(a.size()==b.size(),"GPU comparison extent");float error=0;
    for(std::size_t i=0;i<a.size();++i) {
        require(std::isfinite(a[i])&&std::isfinite(b[i]),"Nonfinite GPU pixels");error=std::max(error,std::abs(a[i]-b[i]));
    }
    return error;
}
std::size_t covered(std::span<const float> pixels) {
    std::size_t count=0;
    for(std::size_t i=0;i<W*H;++i)if(pixels[i*4+3]!=0)++count;
    return count;
}
struct OwnedInputs {
    std::shared_ptr<GlContext> context;
    GlTerrainDecalTextures textures;
    OwnedInputs(std::shared_ptr<GlContext> c,std::shared_ptr<const TerrainDecalAssets> assets)
        :context(c),textures(std::move(c),std::move(assets)) {}
    const GlTexture &get(std::uint16_t id,DdsSamplingColor color) {return textures.resource(id,color);}
};
} // namespace
int main(int argc,char **argv) {
    try {
        require(argc==4||argc==5,"Usage: civ5_gl_terrain_decal original-shaders asset-root fresh-output [original-resource-zero.dds]");
        const std::filesystem::path output(argv[3]);require(!std::filesystem::exists(output),"Fresh output required");std::filesystem::create_directories(output);
        std::ofstream report(output/"report.json");report<<"{\"cases\":[";unsigned cases=0;float maximum=0;
        auto context=std::make_shared<GlContext>(W,H);std::vector<assets::DdsData> data;
        for(unsigned i=0;i<4;++i)data.push_back(control_texture(i));
        GlTerrainDecalRenderer renderer(context);Native native(argv[1],data);Both both(renderer,native);Inputs inputs{context,data,{}};
        TerrainDecalStore store({-512,-512,512,512},4,4,0);
        const auto run=[&](const char *name,std::uint32_t mode=0,std::uint32_t capacity=131072) {
            const auto list=prepare_terrain_decal_draws(store,{-256,-192,256,192},mode,capacity,matrix(),renderer.index_buffer());
            const auto reference=native.draw(list),actual=draw_gl(renderer,list,inputs);const auto error=compare(reference,actual);
            const auto coverage=covered(actual);
            std::cout<<name<<" draws="<<list.draws.size()<<" covered="<<coverage<<" maxAbsError="<<error<<std::endl;
            require(list.draws.empty()?coverage==0:coverage>500,"Unexpected decal coverage");
            require(error<=1e-6f,"Packed decal GPU pixels differ from original");maximum=std::max(maximum,error);
            if(cases++)report<<',';report<<"{\"name\":"<<std::quoted(name)<<",\"draws\":"<<list.draws.size()<<",\"covered\":"<<coverage<<",\"maxAbsError\":"<<error<<'}';
            return actual;
        };
        const auto a=store.submit({.5f,1,2,0},quad(-220,-150,400,280,3.75f));
        const auto b=store.submit({.125f,3,4,0},quad(-130,-80,240,160,1.25f));
        store.submit({.25f,3,2,0},quad(-50,-40,80,70,17.25f),1);
        run("unuploaded");store.upload_pending(both);
        blend_validation::write(output/"control-mrt.f32",std::span<const float>(run("overlap-priority-repeat")));
        run("mode-one-minified",1);run("capacity-drop",0,1002);
        store.submit({.75f,3,4,0},quad(20,10,100,90,2.75f));run("pending-batch-large-only");store.upload_pending(both);run("uploaded-again");
        store.remove(a);run("removed-large-hole");store.remove(b);store.compact(0);run("compacted-pending");store.upload_pending(both);run("compacted-uploaded");
        auto huge=quad(-220,-120,140,100);huge.indices.resize(65532);for(std::size_t i=0;i<huge.indices.size();++i)huge.indices[i]=static_cast<std::uint16_t>((i%3)==0?2:(i%3)==1?1:0);
        const auto large_batch=store.submit({.375f,1,2,0},huge);store.upload_pending(both);run("second-batch");store.remove(large_batch);
        const auto list=prepare_terrain_decal_draws(store,{-256,-192,256,192},0,131072,matrix(),1);
        auto bad=list;bad.draws[0].first_vertex=65534;rejects([&]{draw_gl(renderer,bad,inputs);});
        bad=list;bad.draws[0].index_buffer=99;rejects([&]{draw_gl(renderer,bad,inputs);});
        bad=list;++bad.selected;rejects([&]{draw_gl(renderer,bad,inputs);});
        bad=list;bad.draws[0].constants[0]=std::numeric_limits<float>::infinity();rejects([&]{draw_gl(renderer,bad,inputs);});
        rejects([&]{renderer.draw(list,[&](auto id,auto)->const GlTexture &{return inputs.get(id,DdsSamplingColor::linear);});});
        rejects([&]{renderer.upload(0,{});});rejects([&]{renderer.rebuild_context(context);});
        const auto before=draw_gl(renderer,list,inputs);auto replacement=std::make_shared<GlContext>(W,H);Inputs replacement_inputs{replacement,data,{}};
        renderer.rebuild_context(replacement);require(draw_gl(renderer,list,replacement_inputs)==before,"Packed context recovery changed pixels");
        rejects([&]{renderer.draw(list,[&](auto id,auto color)->const GlTexture &{return inputs.get(id,color);});});
        for(const auto &batch:store.batches())require(renderer.read_vertices(batch.buffer)==batch.vertices,"Packed context recovery lost uploaded bytes");
        std::cout<<"PASS recovery and input guards"<<std::endl;
        const auto loaded=std::make_shared<TerrainDecalAssets>(authored(argv[2]));
        std::vector<assets::DdsData> owned;for(const auto &texture:loaded->textures())owned.push_back(texture.data);
        std::optional<assets::DdsData> zero;
        if(argc==5)zero.emplace(assets::DdsData::parse(blend_validation::read(argv[4])));
        GlTerrainDecalRenderer authored_renderer(replacement);Native authored_native(argv[1],owned,nullptr,zero?&*zero:nullptr);
        Both authored_both(authored_renderer,authored_native);OwnedInputs authored_inputs(replacement,loaded);
        TerrainDecalStore authored_store({-512,-512,512,512},4,4,loaded->blank_specular());
        std::size_t zero_bytes=0;
        if(zero) {
            require(zero->width()==256&&zero->height()==256&&zero->mip_count()==7,"Original zero texture dimensions");
            for(auto color:{DdsSamplingColor::linear,DdsSamplingColor::srgb}) {
                const auto &texture=authored_inputs.get(0,color);
                require(texture.id()==authored_inputs.get(0,color).id(),"Zero texture is not retained");
                require(texture.levels().size()==7,"Zero texture mip range changed");
                for(unsigned mip=0;mip<7;++mip) {
                    auto expected=assets::Bytes(zero->payload(0,mip).begin(),zero->payload(0,mip).end());
                    for(std::size_t i=0;i<expected.size();i+=4)std::swap(expected[i],expected[i+2]);
                    require(texture.read_mip_raw(mip)==expected,"Owned zero texture differs from original D3DX pixels");
                    zero_bytes+=expected.size();
                }
            }
            require(authored_inputs.get(0,DdsSamplingColor::linear).id()!=authored_inputs.get(0,DdsSamplingColor::srgb).id(),"Zero sampler decodes share wrong storage");
            rejects([&]{authored_inputs.get(65535,DdsSamplingColor::linear);});
            rejects([&]{authored_inputs.get(0,static_cast<DdsSamplingColor>(255));});
        }
        report<<"],\"authored\":[";float authored_maximum=0;
        for(unsigned family=zero?0:3;family<10;++family) {
            const auto material=loaded->palette().records.at(loaded->palette().ranges[family].first);
            require(zero||(material.albedo&&material.heightmap),"Authored fixture requires explicit textures");
            const auto handle=authored_store.submit(material,quad(-220,-150,440,300,3.75f));authored_store.upload_pending(authored_both);
            const auto authored_list=prepare_terrain_decal_draws(authored_store,{-256,-192,256,192},0,131072,matrix(),1);
            const auto authored_pixels=draw_gl(authored_renderer,authored_list,authored_inputs);
            const auto reference=authored_native.draw(authored_list);const auto error=compare(reference,authored_pixels);
            const auto coverage=covered(authored_pixels);require(coverage>500,"Empty authored decal capture");
            std::cout<<terrain_decal_families[family]<<" covered="<<coverage<<" maxAbsError="<<error<<std::endl;
            if(error>1e-6f) {
                blend_validation::write(output/"failed-authored-gl.f32",std::span<const float>(authored_pixels));
                blend_validation::write(output/"failed-authored-d3d9.f32",std::span<const float>(reference));
            }
            require(error<=1e-6f,"Authored decal GPU pixels differ");authored_maximum=std::max(authored_maximum,error);
            if(family==4)blend_validation::write(output/"authored-mrt.f32",std::span<const float>(authored_pixels));
            if(family!=(zero?0u:3u))report<<',';
            report<<"{\"family\":"<<std::quoted(std::string(terrain_decal_families[family]))<<",\"covered\":"<<coverage<<",\"maxAbsError\":"<<error<<'}';
            authored_store.remove(handle);
        }
        report<<"],\"zeroResources\":[";
        unsigned zero_cases=0;float zero_maximum=0;
        if(zero) {
            auto material=loaded->palette().records.at(loaded->palette().ranges[4].first);
            for(unsigned role=0;role<3;++role)for(float uv:{.5f,1.f,3.75f,17.25f,128.f}) {
                auto selected=material;if(role!=2)selected.albedo=0;if(role!=1)selected.heightmap=0;
                auto mesh=quad(-220,-150,440,300,uv);
                const auto handle=authored_store.submit(selected,mesh);authored_store.upload_pending(authored_both);
                const auto commands=prepare_terrain_decal_draws(authored_store,{-256,-192,256,192},0,131072,matrix(),1);
                const auto actual=draw_gl(authored_renderer,commands,authored_inputs),expected=authored_native.draw(commands);
                const auto error=compare(actual,expected);require(covered(actual)>500,"Zero texture produced no coverage");
                std::cout<<"zero role="<<role<<" uv="<<uv<<" maxAbsError="<<error<<std::endl;
                require(error<=1e-6f,"Zero-resource decal GPU pixels differ");
                zero_maximum=std::max(zero_maximum,error);
                if(zero_cases++)report<<',';
                report<<"{\"role\":"<<role<<",\"uv\":"<<uv<<",\"maxAbsError\":"<<error<<'}';
                authored_store.remove(handle);
            }
            const auto handle=authored_store.submit({.125f,0,0,loaded->blank_specular()},quad(-220,-150,440,300,3.75f));
            authored_store.upload_pending(authored_both);
            const auto commands=prepare_terrain_decal_draws(authored_store,{-256,-192,256,192},0,131072,matrix(),1);
            const auto before_zero=draw_gl(authored_renderer,commands,authored_inputs);
            auto new_context=std::make_shared<GlContext>(W,H);
            OwnedInputs new_inputs(new_context,loaded);authored_renderer.rebuild_context(new_context);
            require(draw_gl(authored_renderer,commands,new_inputs)==before_zero,"Zero resource context recovery changed pixels");
            rejects([&]{draw_gl(authored_renderer,commands,authored_inputs);});
            authored_inputs=std::move(new_inputs);authored_store.remove(handle);
        }
        report<<"],\"zeroTextureBytes\":"<<zero_bytes<<",\"zeroMaxAbsError\":"<<zero_maximum<<",\"cachePasses\":[";
        unsigned cache_cases=0,nonexact_cache_cases=0;
        for(unsigned mode=0;mode<2;++mode) {
            authored_native.cache_targets(mode!=0);
            for(unsigned family:{0u,1u,2u,3u,4u,7u}) {
                if(!zero&&family<4)continue;
                const auto material=loaded->palette().records.at(loaded->palette().ranges[family].first);
                const auto first=authored_store.submit(material,quad(-220,-150,340,220,3.75f),mode);
                const auto second=authored_store.submit(material,quad(-80,-60,290,200,2.75f),mode);
                authored_store.upload_pending(authored_both);
                const auto commands=prepare_terrain_decal_draws(authored_store,{-256,-192,256,192},mode,131072,matrix(),1);
                const auto actual=cache_gl(authored_renderer,commands,authored_inputs,mode!=0);
                authored_native.render(commands,0x33557799,mode?4:8);
                const auto expected=authored_native.pixels(mode?1:2,mode?4:8);
                if(actual!=expected) {
                    blend_validation::write(output/("cache-"+std::to_string(mode)+"-"+std::to_string(family)+"-gl.bin"),actual);
                    blend_validation::write(output/("cache-"+std::to_string(mode)+"-"+std::to_string(family)+"-d3d9.bin"),expected);
                }
                std::size_t different_channels=0;unsigned maximum_steps=0;
                const unsigned stride=mode?1:2;
                for(std::size_t at=0;at<actual.size();at+=stride) {
                    const unsigned a=actual[at]+(mode?0:unsigned(actual[at+1])*256);
                    const unsigned b=expected[at]+(mode?0:unsigned(expected[at+1])*256);
                    if(a!=b)++different_channels;
                    maximum_steps=std::max(maximum_steps,a>b?a-b:b-a);
                }
                if(different_channels)++nonexact_cache_cases;
                // Existing authored controls remain exact. The new zero/blank
                // controls report every difference and fail the strict corpus
                // at the end, retaining all evidence instead of stopping early.
                if(family>=4)require(actual==expected,"Prior physical cache regression");
                if(cache_cases++)report<<',';
                report<<"{\"mode\":"<<mode<<",\"family\":"<<family<<",\"comparedBytes\":"<<actual.size()
                      <<",\"differentChannels\":"<<different_channels<<",\"maximumStorageSteps\":"<<maximum_steps<<'}';
                if(!mode&&family==4)blend_validation::write(output/"cache-floodplains.rgba16f",actual);
                std::cout<<(different_channels?"DIFF":"PASS")<<" physical cache decals mode="<<mode<<" family="<<family<<" bytes="<<actual.size()
                         <<" differentChannels="<<different_channels<<" maximumStorageSteps="<<maximum_steps<<std::endl;
                authored_store.remove(first);authored_store.remove(second);
            }
        }
        report<<"],\"nonexactCacheCases\":"<<nonexact_cache_cases<<",\"authoredMaxAbsError\":"<<authored_maximum
              <<",\"maximumControlError\":"<<maximum<<",\"contextRecoveryBitExact\":true,\"originalShaderExecuted\":true,\"worldConnected\":false}\n";
        report.close();require(bool(report),"GPU decal report write");
        require(nonexact_cache_cases==0,"Strict cache parity is incomplete; all differing channels retained in report");
        std::cout<<"PASS packed terrain decal GPU lifecycle"<<std::endl;
    }catch(const std::exception &e){std::cerr<<"FAIL "<<e.what()<<std::endl;return 1;}
}
