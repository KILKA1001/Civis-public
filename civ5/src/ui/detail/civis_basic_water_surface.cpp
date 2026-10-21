#include "civis_basic_water_surface.hpp"
#include "civ5/graphics/gl_texture.hpp"
#include "civ5/graphics/gl_render_api.hpp"
#include "civ5/materials/water_surface.hpp"
#include "civ5/terrain/view_bounds.hpp"
#include "../../graphics/gl_native_clip.hpp"
#include "../../graphics/gl_sampled_input.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <exception>
#include <limits>
#include <map>
#include <span>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

namespace civ5::ui::detail {
namespace {
void water_need(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class A,class B>bool water_same_control(const A& a,const B& b){return !a.owner_before(b)&&!b.owner_before(a);}
constexpr const char* water_vertex=R"GLSL(#version 460 core
layout(location=0) in vec4 position;
uniform vec4 vp[4];uniform float oceanWorldZ;
out vec2 heightUV;
void main(){
    vec4 p=vec4(position.xy,oceanWorldZ,1.0);
    vec4 clip=p.x*vp[0]+p.y*vp[1]+p.z*vp[2]+vp[3];
    gl_Position=vec4(clip.x,-clip.y,clip.z,clip.w);
    heightUV=position.zw;
}
)GLSL";
constexpr const char* water_fragment=R"GLSL(#version 460 core
layout(binding=0) uniform sampler2D waterColour;
layout(binding=1) uniform sampler2D terrainHeight;
uniform float oceanAlpha;
in vec2 heightUV;layout(location=0) out vec4 colour;
void main(){
    float height=texture(terrainHeight,heightUV).a;
    if(height>=oceanAlpha)discard;
    float depth=clamp(1.0-height/oceanAlpha,0.0,1.0);
    colour=vec4(textureLod(waterColour,vec2(depth,0.5),0.0).rgb,1.0);
}
)GLSL";
struct WaterRasterScope {
    const graphics::GlApi& api;
    const graphics::GlRenderApi& gl;
    static constexpr std::array<GLenum,20> caps{
        GL_DEPTH_TEST,GL_CULL_FACE,GL_STENCIL_TEST,GL_POLYGON_OFFSET_FILL,
        GL_POLYGON_OFFSET_LINE,GL_POLYGON_OFFSET_POINT,GL_COLOR_LOGIC_OP,
        GL_DITHER,GL_FRAMEBUFFER_SRGB,GL_PRIMITIVE_RESTART,
        GL_PRIMITIVE_RESTART_FIXED_INDEX,GL_DEPTH_CLAMP,
        GL_CLIP_DISTANCE0,GL_CLIP_DISTANCE1,GL_CLIP_DISTANCE2,GL_CLIP_DISTANCE3,
        GL_CLIP_DISTANCE4,GL_CLIP_DISTANCE5,GL_CLIP_DISTANCE6,GL_CLIP_DISTANCE7};
    std::array<GLboolean,caps.size()> enabled{};
    std::array<GLboolean,4> color_mask{};
    std::array<GLdouble,2> depth_range{};
    std::array<GLint,2> polygon{};
    GLboolean blend=GL_FALSE,scissor=GL_FALSE,write=GL_FALSE;
    GLint program=0,vao=0,depth_function=0;
    GLint active_texture=0;std::array<GLint,2> textures{},samplers{};
    WaterRasterScope(const graphics::GlContext& c,const graphics::GlRenderApi& render):api(c.api()),gl(render) {
        for(std::size_t n=0;n<caps.size();++n)enabled[n]=gl.IsEnabled(caps[n]);
        blend=gl.IsEnabledi(GL_BLEND,0);scissor=gl.IsEnabledi(GL_SCISSOR_TEST,0);
        gl.GetBooleani_v(GL_COLOR_WRITEMASK,0,color_mask.data());
        gl.GetDoublei_v(GL_DEPTH_RANGE,0,depth_range.data());
        api.GetIntegerv(GL_POLYGON_MODE,polygon.data());
        // Core contexts expose one FRONT_AND_BACK mode. Reject an unsupported
        // split before changing any state, so restoration stays exact.
        water_need(polygon[0]==polygon[1],"Basic water requires core polygon state");
        api.GetIntegerv(GL_CURRENT_PROGRAM,&program);
        api.GetIntegerv(GL_VERTEX_ARRAY_BINDING,&vao);
        api.GetIntegerv(GL_DEPTH_FUNC,&depth_function);gl.GetBooleanv(GL_DEPTH_WRITEMASK,&write);
        api.GetIntegerv(GL_ACTIVE_TEXTURE,&active_texture);
        for(unsigned n=0;n<2;++n){gl.ActiveTexture(GL_TEXTURE0+n);
            api.GetIntegerv(GL_TEXTURE_BINDING_2D,&textures[n]);gl.GetIntegeri_v(GL_SAMPLER_BINDING,n,&samplers[n]);}
        gl.ActiveTexture(static_cast<GLenum>(active_texture));
        // OWN opaque plane must produce colour and canonical D24 depth even
        // when the preceding LAND/cache pass used masks, stencil or lines.
        for(auto cap:caps)gl.Disable(cap);
        gl.Enable(GL_DEPTH_TEST);gl.DepthFunc(GL_LEQUAL);gl.DepthMask(GL_TRUE);
        gl.DepthRangeIndexed(0,0,1);gl.PolygonMode(GL_FRONT_AND_BACK,GL_FILL);
        gl.Disablei(GL_BLEND,0);gl.Disablei(GL_SCISSOR_TEST,0);
        gl.ColorMaski(0,GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
    }
    ~WaterRasterScope(){
        gl.UseProgram(static_cast<GLuint>(program));gl.BindVertexArray(static_cast<GLuint>(vao));
        gl.DepthFunc(static_cast<GLenum>(depth_function));gl.DepthMask(write);
        gl.DepthRangeIndexed(0,depth_range[0],depth_range[1]);
        gl.PolygonMode(GL_FRONT_AND_BACK,static_cast<GLenum>(polygon[0]));
        gl.ColorMaski(0,color_mask[0],color_mask[1],color_mask[2],color_mask[3]);
        for(unsigned n=0;n<2;++n){gl.ActiveTexture(GL_TEXTURE0+n);
            api.BindTexture(GL_TEXTURE_2D,static_cast<GLuint>(textures[n]));gl.BindSampler(n,static_cast<GLuint>(samplers[n]));}
        gl.ActiveTexture(static_cast<GLenum>(active_texture));
        for(std::size_t n=0;n<caps.size();++n)if(enabled[n])gl.Enable(caps[n]);else gl.Disable(caps[n]);
        if(blend)gl.Enablei(GL_BLEND,0);else gl.Disablei(GL_BLEND,0);
        if(scissor)gl.Enablei(GL_SCISSOR_TEST,0);else gl.Disablei(GL_SCISSOR_TEST,0);
    }
    WaterRasterScope(const WaterRasterScope&)=delete;
    WaterRasterScope& operator=(const WaterRasterScope&)=delete;
};
}
struct CivisBasicWaterSurface::State final {
    const std::thread::id creator=std::this_thread::get_id();
    const std::shared_ptr<graphics::GlContext> context;
    const std::shared_ptr<const materials::MaterialWorld> world;
    const std::weak_ptr<graphics::GlContext> context_control;
    const CivisSavedWorldLoadingOwner::RendererLoadingReturn* const loading_identity;
    const terrain::X87Precision precision;
    graphics::GlRenderApi gl;
    const materials::WaterSurfaceDomain domain;
    const assets::AssetHandle colour_source;
    const assets::DdsData colour_data;
    const graphics::GlTexture colour;
    std::map<unsigned,graphics::GlTexture> heights;
    std::atomic<bool> alive{true},failed{false};bool busy=false;
    GLuint program=0,vao=0,vb=0,ib=0,sampler_colour=0,sampler_height=0;
    GLint vp=-1,z=-1,alpha=-1;
    explicit State(std::shared_ptr<graphics::GlContext> c,
        std::shared_ptr<const materials::MaterialWorld> w,
        const CivisSavedWorldLoadingOwner::RendererLoadingReturn* loading,terrain::X87Precision pc,
        assets::AssetHandle asset,assets::DdsData data)
      :context(std::move(c)),world(std::move(w)),context_control(context),loading_identity(loading),precision(pc),gl(*context),
       domain(materials::WaterSurfaceDomain::from_world(world->terrain())),
       colour_source(std::move(asset)),colour_data(std::move(data)),
       // Explicit OWN colour decode policy for this new two-sampler shader;
       // no Original sampler/decode/pixel-equivalence claim is issued.
       colour(*context,colour_data,graphics::DdsSamplingColor::srgb){
        water_need(colour_data.face_count()==1,"Basic water colour must be actual 2D authored DDS");
    }
    void first()const{
        water_need(creator==std::this_thread::get_id(),"Basic water foreign render creator");
        water_need(alive.load()&&!failed.load(),"Basic water retired/quarantined");
    }
    void current(const FrameInputsLoan& actual)const {
        first();actual.require_current();first();
        water_need(water_same_control(context_control,actual.context_control_owned()),"Basic water context changed");
        const auto& actual_world=actual.world_owned();
        water_need(actual_world.get()==world.get()&&water_same_control(actual_world,world),"Basic water uses another material generation");
        water_need(&actual.layout_owned()==&world->terrain().source().layout,
                   "Basic water current camera/layout owner differs");first();
        water_need(&actual.loading_owned()==loading_identity,"Basic water loading source generation changed");first();
        actual.require_current();first(); // PURE loan identity/currentness only.
    }
    void boundary(const FrameInputsLoan& actual)const{
        first();actual.require_boundary();first();current(actual);
    }
    void target(const FrameInputsLoan& actual,int width,int height)const{
        const auto& c=actual.color_owned();const auto& d=actual.depth_owned();
        water_need(c.belongs_to(*context)&&d.belongs_to(*context),"Basic water frame target context differs");
        water_need(c.face_count()==1&&d.face_count()==1&&c.depth()==1&&d.depth()==1&&
            !c.levels().empty()&&!d.levels().empty()&&c.id()!=d.id(),"Basic water actual 2D target storage absent/aliased");
        water_need(d.plan().internal_format==GL_DEPTH24_STENCIL8,"Basic water requires actual canonical D24S8 depth");
        GLint fbo=0,colour_name=0,depth_name=0;
        context->api().GetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&fbo);
        water_need(fbo>0,"Basic water requires actual bound LAND framebuffer");
        gl.GetNamedFramebufferAttachmentParameteriv(static_cast<GLuint>(fbo),GL_COLOR_ATTACHMENT0,
            GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,&colour_name);
        gl.GetNamedFramebufferAttachmentParameteriv(static_cast<GLuint>(fbo),GL_DEPTH_ATTACHMENT,
            GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,&depth_name);
        water_need(static_cast<GLuint>(colour_name)==c.id()&&static_cast<GLuint>(depth_name)==d.id(),
                   "Basic water target aliases or differs from actual LAND colour/depth");
        for(const auto attachment:{GL_COLOR_ATTACHMENT0,GL_DEPTH_ATTACHMENT}){
            GLint type=0,level=0,layered=0;
            gl.GetNamedFramebufferAttachmentParameteriv(static_cast<GLuint>(fbo),attachment,GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE,&type);
            gl.GetNamedFramebufferAttachmentParameteriv(static_cast<GLuint>(fbo),attachment,GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL,&level);
            gl.GetNamedFramebufferAttachmentParameteriv(static_cast<GLuint>(fbo),attachment,GL_FRAMEBUFFER_ATTACHMENT_LAYERED,&layered);
            water_need(type==GL_TEXTURE&&level==0&&!layered,"Basic water requires exact level0 2D target attachments");
        }
        water_need(gl.CheckNamedFramebufferStatus(static_cast<GLuint>(fbo),GL_DRAW_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE,
                   "Basic water actual framebuffer incomplete");
        GLint draw_buffer=0;context->api().GetIntegerv(GL_DRAW_BUFFER0,&draw_buffer);
        water_need(draw_buffer==GL_COLOR_ATTACHMENT0,"Basic water colour output is not the actual LAND attachment");
        GLint draw_buffers=0;context->api().GetIntegerv(GL_MAX_DRAW_BUFFERS,&draw_buffers);
        for(GLint slot=1;slot<draw_buffers;++slot){
            context->api().GetIntegerv(GL_DRAW_BUFFER0+slot,&draw_buffer);
            water_need(draw_buffer==GL_NONE,"Basic water refuses additional active colour outputs");
        }
        water_need(c.levels().front().width==static_cast<unsigned>(width)&&
                   c.levels().front().height==static_cast<unsigned>(height)&&
                   d.levels().front().width==static_cast<unsigned>(width)&&
                   d.levels().front().height==static_cast<unsigned>(height),"Basic water framebuffer extent differs");
        GLint viewport[4]{};context->api().GetIntegerv(GL_VIEWPORT,viewport);
        water_need(viewport[0]==0&&viewport[1]==0&&viewport[2]==width&&viewport[3]==height,
                   "Basic water current viewport differs");
        water_need(!gl.IsEnabled(GL_RASTERIZER_DISCARD),"Basic water cannot discard actual surface");
        GLint feedback=0;context->api().GetIntegerv(GL_TRANSFORM_FEEDBACK_ACTIVE,&feedback);
        water_need(!feedback,"Basic water refuses nested transform feedback");
        context->check("Basic water actual target/viewport");
    }
    void initialize(const FrameInputsLoan& actual){
        try{
            current(actual);
            program=gl.CreateProgram();
            for(const auto stage:{std::pair{GL_VERTEX_SHADER,water_vertex},std::pair{GL_FRAGMENT_SHADER,water_fragment}}){
                const auto shader=gl.CreateShader(stage.first);gl.ShaderSource(shader,1,&stage.second,nullptr);gl.CompileShader(shader);
                GLint okay=0;gl.GetShaderiv(shader,GL_COMPILE_STATUS,&okay);
                if(!okay){char message[4096]{};gl.GetShaderInfoLog(shader,sizeof(message),nullptr,message);gl.DeleteShader(shader);throw std::runtime_error(message);}
                gl.AttachShader(program,shader);gl.DeleteShader(shader);current(actual);
            }
            gl.LinkProgram(program);GLint okay=0;gl.GetProgramiv(program,GL_LINK_STATUS,&okay);
            if(!okay){char message[4096]{};gl.GetProgramInfoLog(program,sizeof(message),nullptr,message);throw std::runtime_error(message);}
            vp=gl.GetUniformLocation(program,"vp[0]");z=gl.GetUniformLocation(program,"oceanWorldZ");alpha=gl.GetUniformLocation(program,"oceanAlpha");
            water_need(vp>=0&&z>=0&&alpha>=0,"Basic water actual uniforms unavailable");
            current(actual);
            gl.CreateVertexArrays(1,&vao);gl.CreateBuffers(1,&vb);gl.CreateBuffers(1,&ib);
            gl.VertexArrayVertexBuffer(vao,0,vb,0,16);gl.VertexArrayElementBuffer(vao,ib);
            gl.EnableVertexArrayAttrib(vao,0);gl.VertexArrayAttribBinding(vao,0,0);gl.VertexArrayAttribFormat(vao,0,4,GL_FLOAT,GL_FALSE,0);
            current(actual);
            gl.CreateSamplers(1,&sampler_colour);gl.CreateSamplers(1,&sampler_height);
            for(auto sampler:{sampler_colour,sampler_height}){
                gl.SamplerParameteri(sampler,GL_TEXTURE_MIN_FILTER,GL_LINEAR);gl.SamplerParameteri(sampler,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
                gl.SamplerParameteri(sampler,GL_TEXTURE_WRAP_S,sampler==sampler_height?GL_REPEAT:GL_CLAMP_TO_EDGE);
                gl.SamplerParameteri(sampler,GL_TEXTURE_WRAP_T,sampler==sampler_height?GL_REPEAT:GL_CLAMP_TO_EDGE);
            }
            current(actual);
            const auto field=world->terrain().height_field();
            current(actual);
            water_need(field.domain().texture_columns()<=16&&field.domain().texture_rows()<=16,
                "Basic water generated height exceeds its fixed window table");
            for(int y=0;y<field.domain().texture_rows();++y)for(int x=0;x<field.domain().texture_columns();++x){
                {
                    const auto bytes=field.extract_a8_texture(x,y);current(actual);
                    heights.emplace(static_cast<unsigned>(x+16*y),graphics::GlTexture::alpha8(*context,1024,1024,bytes));current(actual);
                }
                current(actual); // Extracted byte owner/temporary texture retired.
            }
            context->check("Basic water real colour/height geometry pipeline");
        }catch(...){failed.store(true);release();throw;}
    }
    void release()noexcept {
        try{
            if(creator!=std::this_thread::get_id())std::terminate();
            context->api();GLint current=0;context->api().GetIntegerv(GL_CURRENT_PROGRAM,&current);
            if(program&&static_cast<GLuint>(current)==program)gl.UseProgram(0);
            gl.DeleteBuffers(1,&vb);gl.DeleteBuffers(1,&ib);gl.DeleteVertexArrays(1,&vao);
            gl.DeleteSamplers(1,&sampler_colour);gl.DeleteSamplers(1,&sampler_height);if(program)gl.DeleteProgram(program);
            program=vao=vb=ib=sampler_colour=sampler_height=0;
        }catch(...){std::terminate();}
    }
    ~State(){release();} // All textures release on this same actual creator/context.
};
CivisBasicWaterSurface::CivisBasicWaterSurface(std::shared_ptr<State> state):state_(std::move(state)){}
CivisBasicWaterSurface::~CivisBasicWaterSurface(){
    if(creator_!=std::this_thread::get_id())std::terminate();
    state_->alive.store(false); // Facade authority retires before GPU targets.
}
std::unique_ptr<CivisBasicWaterSurface> CivisBasicWaterSurface::capture_owned(const FrameInputsLoan& actual){
    std::shared_ptr<State> state;
    std::unique_ptr<CivisBasicWaterSurface> result;
    try{
        {
            actual.require_boundary();
            auto context=actual.context_control_owned().lock();water_need(bool(context),"Basic water actual context retired");
            auto world=actual.world_owned();actual.require_current();
            const auto* loading=&actual.loading_owned();
            // The Return's public precision getter is FULL. Cache it exactly
            // once outside raster; draw uses no effectful Return accessor.
            const auto precision=loading->precision_owned();actual.require_boundary();
            auto handle=actual.stage_asset_owned(assets::AssetKey("waterdepthcolor_new.dds"));actual.require_current();
            water_need(handle.requested_key().normalized_name()==assets::AssetKey("waterdepthcolor_new.dds").normalized_name(),
                "Basic water Bank returned another authored resource");
            auto bytes=handle.read_bytes();actual.require_boundary();
            // Actual Civ5 authored D3D9 header profiles; every face/mip byte
            // remains exact. There is no decode fixture or generated colour.
            auto data=assets::DdsData::parse(std::move(bytes),assets::NumericDdsProfile::legacy_d3d9,
                assets::DdsHeaderProfile::civ5_d3dx_size24);actual.require_boundary();
            water_need(data.face_count()==1,"Basic water authored colour must be 2D before GPU upload");
            state=std::make_shared<State>(std::move(context),std::move(world),loading,precision,std::move(handle),std::move(data));
            state->current(actual);state->initialize(actual);state->current(actual);
        } // Producer bytes, source copies and upload temporaries retire first.
        state->boundary(actual);
        result=std::unique_ptr<CivisBasicWaterSurface>(new CivisBasicWaterSurface(std::move(state)));
        result->state_->boundary(actual);return result;
    }catch(...){
        const auto error=std::current_exception();
        if(state)state->failed.store(true);
        if(result)result->state_->failed.store(true);
        result.reset();state.reset(); // Actual partial targets retire before POST.
        try{actual.require_boundary();}catch(...){}
        std::rethrow_exception(error);
    }
}
void CivisBasicWaterSurface::draw_owned(const FrameInputsLoan& actual){
    water_need(creator_==std::this_thread::get_id(),"Basic water foreign facade creator");
    auto operation_state=state_; // Creator FIRST; State pin cannot mask facade retirement.
    try{
        auto& s=*operation_state;s.first();
        if(s.busy){s.failed.store(true);throw std::runtime_error("Basic water draw reentry");}
        s.busy=true;struct End{State& s;~End(){s.busy=false;}}end{s};
        s.current(actual); // Caller established FULL before its outer FOW/raster scope.
        {
            const auto width=actual.width_owned(),height=actual.height_owned();
            water_need(width<=INT32_MAX&&height<=INT32_MAX&&width&&height,"Basic water frame dimensions invalid");
            const auto camera=actual.camera_owned();s.current(actual);
            water_need(camera.view_count>0&&camera.view_count<=4,"Basic water actual camera views missing");
            for(float value:camera.projection)water_need(std::isfinite(value),"Basic water nonfinite actual projection");
            const auto& layout=s.world->terrain().source().layout;
            const auto& scales=s.world->terrain().owner().parameters;
            for(unsigned view=0;view<camera.view_count;++view)for(float value:camera.views[view])
                water_need(std::isfinite(value),"Basic water nonfinite actual view");
            // The same current wrap frusta and world/scales drive actual LAND
            // visibility. Consume only their clipped world XY intersection.
            const auto bounds=terrain::view_bounds(layout,std::span(camera.frusta).first(camera.view_count),
                {scales.scale_x,scales.scale_y},{},s.precision);
            s.current(actual);
            const auto& rivers=s.world->terrain().rivers();water_need(bool(rivers),"Basic water requires retained CPU river loading");
            const float ocean=rivers->level;
            water_need(std::isfinite(ocean)&&ocean>0&&ocean<=1,"Basic water own sea level invalid");
            const float world_z=(ocean*s.domain.scales[2])*255.f;
            water_need(std::isfinite(world_z),"Basic water own world sea level unrepresentable");
            // This entire method runs in the caller's FOW scene callback.
            // Every loan accessor and loading identity check here is PURE.
            s.current(actual);
            for(unsigned view=0;view<camera.view_count;++view){
                s.current(actual); // Fresh pure POST after prior view's CPU destruction.
                const auto& low=bounds.clipped_low[view];const auto& high=bounds.clipped_high[view];
                // Actual view_bounds retains inverted extents for a wrap
                // view wholly outside this world. It produces no surface.
                if(low[0]>=high[0]||low[1]>=high[1])continue;
                const std::array<terrain::Vector4,1> one_view{{{low[0],low[1],high[0],high[1]}}};
                auto mesh=materials::build_water_surface_mesh(s.domain,one_view,0,s.precision);
                s.current(actual);
                water_need(mesh.vertices.size()<=static_cast<std::size_t>(std::numeric_limits<GLsizei>::max()),
                    "Basic water actual view exceeds GL extent");
                water_need(mesh.counts.size()==256&&mesh.prefixes.size()==256&&mesh.vertices.size()%4==0,
                    "Basic water actual one-view helper groups differ");
                std::size_t quads=0;
                for(unsigned window=0;window<256;++window){
                    water_need(mesh.prefixes[window]==quads,"Basic water helper prefix differs");quads+=mesh.counts[window];
                    if(mesh.counts[window])water_need(s.heights.contains(window),
                        "Basic water helper window lacks actual generated height");
                }
                water_need(quads*4==mesh.vertices.size(),"Basic water helper groups do not cover actual stream");
                if(!quads)continue;
                for(const auto& vertex:mesh.vertices)for(float value:vertex)
                    water_need(std::isfinite(value),"Basic water nonfinite actual mesh vertex");
                // OWN U32 representation of the helper's genuine quad
                // vertices. Its legacy wrapping U16 index stream is never
                // read/uploaded. One view keeps its true counts/prefixes
                // below 65536 even at the native 224 by 224 window limit.
                std::vector<std::uint32_t> indices(quads*6);
                constexpr std::array<std::uint32_t,6> offsets{0,1,2,0,2,3};
                for(std::size_t quad=0;quad<quads;++quad)for(std::size_t n=0;n<offsets.size();++n)
                    indices[quad*6+n]=static_cast<std::uint32_t>(quad*4)+offsets[n];
                const auto vp=terrain::matrix_multiply(camera.views[view],camera.projection,
                    s.precision,terrain::MatrixPath::sse);
                for(float value:vp)water_need(std::isfinite(value),"Basic water nonfinite actual VP product");
                // Both allocating CPU streams are complete before fresh
                // pure authority/target checks and any water GL binding.
                s.current(actual);
                s.target(actual,static_cast<int>(width),static_cast<int>(height));
                {
                    WaterRasterScope raster(*s.context,s.gl);
                    const graphics::detail::NativeClipScope clip(*s.context,s.gl);
                    s.gl.NamedBufferData(s.vb,mesh.vertices.size()*sizeof(terrain::Vector4),mesh.vertices.data(),GL_DYNAMIC_DRAW);
                    s.gl.NamedBufferData(s.ib,indices.size()*sizeof(std::uint32_t),indices.data(),GL_DYNAMIC_DRAW);
                    s.gl.ProgramUniform1f(s.program,s.z,world_z);s.gl.ProgramUniform1f(s.program,s.alpha,ocean);
                    graphics::detail::validate_sampled_texture(*s.context,s.gl,s.colour.id(),GL_TEXTURE_2D);
                    s.context->api().BindTextureUnit(0,s.colour.id());s.gl.BindSampler(0,s.sampler_colour);
                    s.gl.UseProgram(s.program);s.gl.BindVertexArray(s.vao);
                    s.gl.ProgramUniform4fv(s.program,s.vp,4,vp.data());
                    for(unsigned window=0;window<256;++window){
                        const auto count=mesh.counts.at(window);if(!count)continue;
                        const auto found=s.heights.find(window);water_need(found!=s.heights.end(),"Basic water window outside actual generated height");
                        graphics::detail::validate_sampled_texture(*s.context,s.gl,found->second.id(),GL_TEXTURE_2D);
                        s.context->api().BindTextureUnit(1,found->second.id());s.gl.BindSampler(1,s.sampler_height);
                        s.gl.DrawElements(GL_TRIANGLES,static_cast<GLsizei>(count*6u),GL_UNSIGNED_INT,
                            reinterpret_cast<const void*>(static_cast<std::uintptr_t>(mesh.prefixes.at(window))*6u*sizeof(std::uint32_t)));
                    }
                    s.context->check("Basic water real colour/depth surface");
                } // Restore this actual view's clip/raster before CPU retirement.
                s.current(actual); // This actual view's CPU streams retire next.
            }
        } // CPU mesh and all view/material temporaries retire before POST.
        s.current(actual); // Caller runs FULL only after outer callback/raster retirement.
    }catch(...){
        const auto error=std::current_exception();operation_state->failed.store(true);
        // Retire this counted operation pin after mesh/raster cleanup. The
        // Model caller retires failed frame/water targets and performs fresh
        // Source POST only after its complete FOW callback/raster has ended.
        operation_state.reset();
        std::rethrow_exception(error);
    }
}
}
