#include "../validation/gl_bnw_warrior_retained.hpp"
#include "civ5/graphics/bnw_warrior_authored_formation.hpp"
#include <iostream>
#include <fstream>
#include <thread>

namespace {
void require(bool b,const char *why) {if(!b)throw std::runtime_error(why);}
std::size_t different(const civ5::assets::Bytes &a,const civ5::assets::Bytes &b) {
    require(a.size()==b.size(),"Retained framebuffer extent differs");std::size_t n=0;
    for(std::size_t i=0;i<a.size();++i)n+=a[i]!=b[i];return n;
}
std::size_t visible(const civ5::assets::Bytes &a) {
    std::size_t n=0;for(std::size_t i=0;i<a.size();i+=4)if(a[i]>16||a[i+1]>18||a[i+2]>24)++n;return n;
}
}
int main(int argc,char **argv) {
    try {
        using namespace civ5::graphics;
        if(argc!=4)throw std::runtime_error("Usage: gl_bnw_warrior_retained_tests LOCAL_CIV5 LOCAL_SDK_CACHE OUTPUT_PPM");
        auto context=std::make_shared<GlContext>(512,512,false);GlRenderApi gl(*context);
        auto color=GlTexture::render_target(*context,512,512,1,RenderTextureFormat::rgba8);
        auto depth=GlTexture::render_target(*context,512,512,1,RenderTextureFormat::depth24_stencil8);
        GLuint fbo=0;gl.CreateFramebuffers(1,&fbo);gl.NamedFramebufferTexture(fbo,GL_COLOR_ATTACHMENT0,color.id(),0);
        gl.NamedFramebufferTexture(fbo,GL_DEPTH_STENCIL_ATTACHMENT,depth.id(),0);
        const GLenum attachment=GL_COLOR_ATTACHMENT0;gl.NamedFramebufferDrawBuffers(fbo,1,&attachment);
        require(gl.CheckNamedFramebufferStatus(fbo,GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE,"Retained target incomplete");
        gl.BindFramebuffer(GL_FRAMEBUFFER,fbo);gl.Viewport(0,0,512,512);
        const auto clear_depth=reinterpret_cast<PFNGLCLEARDEPTHPROC>(context->proc_address("glClearDepth"));
        const auto active=reinterpret_cast<PFNGLACTIVETEXTUREPROC>(context->proc_address("glActiveTexture"));
        const auto clear=[&](double d){gl.DepthMask(GL_TRUE);clear_depth(d);gl.ClearColor(.03f,.04f,.06f,1);gl.Clear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);};
        GlBnwWarriorRetainedBackend backend(context,argv[1],argv[2]);
        require(backend.retained_meshes()==3 && backend.buffer_uploads()==6,"Retained source buffers not created once");
        backend.select_sdk_sample(0);require(backend.buffer_uploads()==9,"SDK pose upload count differs");
        // Native algorithm, explicit diagnostic pool seed/height/scale.
        // These renderer inputs are not captured placement for unit 16385.
        const auto authored=bnw_warrior_authored_formation(7,1);
        BnwWarriorWorldTransformInput transform;transform.model_scale=.14f;transform.heading_radians=bnw_warrior_initial_heading(1);
        const auto worlds=bnw_warrior_render_transforms(authored,transform);
        GlBnwWarriorFrame frame;frame.width=frame.height=512;
        frame.view={1,0,0,0,.45f,0,1,0,0,1,0,0,0,-8,0,1};
        frame.projection={1.f/40,0,0,0,0,1.f/25,0,0,0,0,1.f/100,0,0,0,.5f,1};
        clear(1);gl.Enable(GL_BLEND);gl.Disable(GL_DEPTH_TEST);gl.Enable(GL_CULL_FACE);
        gl.DepthFunc(GL_GREATER);gl.DepthMask(GL_FALSE);color.bind(0);active(GL_TEXTURE3);
        auto result=backend.draw_diagnostic(worlds,frame);
        require(result.drawn && result.members==12 && result.triangles==7920 && !result.model_drawable && !result.full_graphics_parity,"Retained diagnostic readiness or 3/6/3 draw differs");
        GLint value=0;context->api().GetIntegerv(GL_ACTIVE_TEXTURE,&value);require(value==GL_TEXTURE3,"Retained draw leaked active texture");
        context->api().GetIntegerv(GL_DEPTH_FUNC,&value);require(value==GL_GREATER,"Retained draw leaked depth function");
        context->api().GetIntegerv(GL_DEPTH_WRITEMASK,&value);require(value==0,"Retained draw leaked depth mask");
        require(gl.IsEnabled(GL_BLEND) && !gl.IsEnabled(GL_DEPTH_TEST) && gl.IsEnabled(GL_CULL_FACE),"Retained draw leaked GL enables");
        const auto zero=color.read_level_raw(0,0);require(visible(zero)>1000,"Retained Warrior invisible");
        for(int frame_id=0;frame_id<60;++frame_id) {
            clear(1);backend.select_sdk_sample(0);backend.draw_diagnostic(worlds,frame);
        }
        require(backend.buffer_uploads()==9 && different(zero,color.read_level_raw(0,0))==0,"Retained static frames reupload or drift");
        backend.select_sdk_sample(1);clear(1);backend.draw_diagnostic(worlds,frame);
        const auto one=color.read_level_raw(0,0);const auto motion=different(zero,one);
        require(motion>20 && backend.buffer_uploads()==12,"Retained SDK pose did not change geometry");
        clear(0);backend.draw_diagnostic(worlds,frame);
        require(visible(color.read_level_raw(0,0))==0,"Retained Warrior ignored foreground depth");
        bool rejected=false;std::thread worker([&] {try {backend.select_sdk_sample(2);}catch(const std::logic_error &){rejected=true;}});worker.join();
        require(rejected && backend.buffer_uploads()==12,"Retained backend accepted another GL thread");
        civ5::runtime::LiveSessionState live{.random={civ5::runtime::SimulationRandom(civ5::runtime::RandomAlgorithm::bnw,{1,0,0}),civ5::runtime::SimulationRandom(civ5::runtime::RandomAlgorithm::bnw,{2,0,0})}};BnwRendererSceneReceipt receipt;
        const auto blocked=backend.draw_selected(receipt,live,nullptr,frame);
        require(live.random.map.state()==civ5::runtime::SimulationRandomState{1,0,0} && live.random.gameplay.state()==civ5::runtime::SimulationRandomState{2,0,0},"Authoring helper/backend changed live RNG");
        require(!blocked.drawn && !blocked.model_drawable && blocked.blocker.find("placement receipt")!=std::string::npos,"Missing native placement enabled production");
        rejected=false;try {(void)load_bnw_warrior_placement_receipt({});}catch(const std::exception &){rejected=true;}
        require(rejected,"Empty native capture pin was accepted");
        std::ofstream out(argv[3],std::ios::binary);out<<"P6\n512 512\n255\n";
        for(int y=511;y>=0;--y)for(int x=0;x<512;++x)out.write(reinterpret_cast<const char *>(zero.data()+(std::size_t(y)*512+x)*4),3);
        require(bool(out),"Retained diagnostic image write failed");
        context->check("Authored Warrior formation GL tests");
        gl.BindFramebuffer(GL_FRAMEBUFFER,0);gl.DeleteFramebuffers(1,&fbo);
        std::cout<<"{\"status\":\"PASS\",\"members\":12,\"triangles\":7920,\"staticFrames\":60,\"poseChangedBytes\":"<<motion
                 <<",\"bufferUploads\":12,\"depthOcclusion\":true,\"modelDrawable\":false,\"nativePlacementAvailable\":false}\n";
        return 0;
    } catch(const std::exception &e){std::cerr<<e.what()<<'\n';return 1;}
}
