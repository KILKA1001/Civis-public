#include "civ5/graphics/gl_bnw_font_shader.hpp"
#include "civ5/ui/detail/bnw_label_paint_positioned_data.hpp"
#include "civ5/ui/detail/bnw_windowed_renderer_shared_descriptor_identity.hpp"
#include "detail/bnw_label_positioned_numeric_data_core.inc"
#include <atomic>
#include "civ5/graphics/detail/bnw_font_shader_packed_validation.hpp"
#include "civ5/graphics/detail/bnw_renderer_context_pin.hpp"
#include "civ5/graphics/detail/bnw_owned_gl_retirement_node.hpp"
#include <atomic>
#include <bit>
#include <exception>
#include <cstring>
#include "civ5/assets/civis_resource_bank_owner.hpp"
#include <new>
#include <cstddef>
#include <optional>
#include "civ5/graphics/gl_render_api.hpp"
#include "civ5/graphics/bnw_warrior_sampler_records.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <thread>
#include <utility>
#include <windows.h>
#include <bcrypt.h>
namespace civ5::graphics {
namespace {
void need(bool v,const char*s){if(!v)throw std::runtime_error(s);}
// Successful descriptor publication invokes no project allocator or observer.
// Rejection precedes mutation/receipt; ordinary ABI exception storage may allocate.
class DescriptorPublicationFailure final : public std::exception {
public:
 const char*what()const noexcept override{return "Renderer descriptor physical provenance rejected";}
};
void descriptor_need(bool condition){if(!condition)throw DescriptorPublicationFailure{};}
constexpr char vs[]=R"glsl(#version 450 core
layout(location=0) in vec2 aPosition;
layout(location=1) in vec2 aUv;
layout(location=2) in vec4 aDiagnosticColor;
uniform vec4 uRows[4];
uniform vec2 uTranslation;
uniform vec2 uDepth;
uniform float uParentAlpha;
out vec2 vUv;
out vec4 vColor;
void main(){
 vec2 p=aPosition+uTranslation;
 vec4 clip=p.x*uRows[0]+p.y*uRows[1]+uRows[3];
 gl_Position=clip*(uDepth.x+uDepth.y);
 vUv=aUv;vColor=vec4(aDiagnosticColor.rgb,aDiagnosticColor.a*uParentAlpha);
})glsl";
constexpr char ps[]=R"glsl(#version 450 core
layout(origin_upper_left) in vec4 gl_FragCoord;
in vec2 vUv;
in vec4 vColor;
uniform sampler2D uBlockTexture;
uniform vec4 uClip;
uniform float uAllowColor;
uniform vec2 uFragmentBias;
uniform bool uNativeTopAtRowZero;
uniform float uTargetHeight;
layout(location=0) out vec4 result;
void main(){
 vec2 p=gl_FragCoord.xy;
 if(uNativeTopAtRowZero)p.y=uTargetHeight-p.y;
 p+=uFragmentBias;
 if(p.x<uClip.x || p.x>uClip.y || p.y<uClip.z || p.y>uClip.w)discard;
 float coverage=1.0-texture(uBlockTexture,vUv).r;
 result=vec4(uAllowColor<=0.0?vec3(coverage):vColor.rgb,coverage*vColor.a);
})glsl";
bool native_current(const GlContextNativeIdentity&i){
 return i.owner_thread && i.window && i.dc && i.context &&
  reinterpret_cast<std::uintptr_t>(wglGetCurrentContext())==i.context &&
  reinterpret_cast<std::uintptr_t>(wglGetCurrentDC())==i.dc &&
  reinterpret_cast<std::uintptr_t>(WindowFromDC(reinterpret_cast<HDC>(i.dc)))==i.window;
}
bool same_identity(const GlContextNativeIdentity&a,const GlContextNativeIdentity&b){
 return a.window==b.window&&a.dc==b.dc&&a.context==b.context&&b.owner_thread;
}
bool same_pair(const GlBnwSystemFontAtlasFrame&a,const GlBnwSystemFontAtlasFrame&b){
 return a.green_texture==b.green_texture&&a.shadow_texture==b.shadow_texture&&
  a.width==b.width&&a.height==b.height&&a.revision==b.revision&&
  a.pair_storage_available==b.pair_storage_available;
}
struct Extra {
 PFNGLGETVERTEXATTRIBIVPROC GetVertexAttrib;
 PFNGLGETINTEGERI_VPROC GetIntegeri;
 PFNGLGETBOOLEANI_VPROC GetBooleani;
 PFNGLGETBOOLEANVPROC GetBoolean;
 PFNGLGETDOUBLEI_VPROC GetDoublei;
 PFNGLISENABLEDIPROC IsEnabledi;
 PFNGLENABLEIPROC Enablei;
 PFNGLDISABLEIPROC Disablei;
 PFNGLCOLORMASKIPROC ColorMaski;
 PFNGLBLENDFUNCSEPARATEIPROC BlendFuncSeparatei;
 PFNGLBLENDEQUATIONSEPARATEIPROC BlendEquationSeparatei;
 PFNGLACTIVETEXTUREPROC ActiveTexture;
 PFNGLDEPTHRANGEINDEXEDPROC DepthRangeIndexed;
 PFNGLGETQUERYIVPROC GetQuery;
 explicit Extra(const GlContext&c){
#define LOAD(field,type,name) field=reinterpret_cast<type>(c.proc_address(name))
  LOAD(GetVertexAttrib,PFNGLGETVERTEXATTRIBIVPROC,"glGetVertexAttribiv");
  LOAD(GetIntegeri,PFNGLGETINTEGERI_VPROC,"glGetIntegeri_v");
  LOAD(GetBooleani,PFNGLGETBOOLEANI_VPROC,"glGetBooleani_v");
  LOAD(GetBoolean,PFNGLGETBOOLEANVPROC,"glGetBooleanv");
  LOAD(GetDoublei,PFNGLGETDOUBLEI_VPROC,"glGetDoublei_v");
  LOAD(IsEnabledi,PFNGLISENABLEDIPROC,"glIsEnabledi");
  LOAD(Enablei,PFNGLENABLEIPROC,"glEnablei");LOAD(Disablei,PFNGLDISABLEIPROC,"glDisablei");
  LOAD(ColorMaski,PFNGLCOLORMASKIPROC,"glColorMaski");
  LOAD(BlendFuncSeparatei,PFNGLBLENDFUNCSEPARATEIPROC,"glBlendFuncSeparatei");
  LOAD(BlendEquationSeparatei,PFNGLBLENDEQUATIONSEPARATEIPROC,"glBlendEquationSeparatei");
  LOAD(ActiveTexture,PFNGLACTIVETEXTUREPROC,"glActiveTexture");
  LOAD(DepthRangeIndexed,PFNGLDEPTHRANGEINDEXEDPROC,"glDepthRangeIndexed");
  LOAD(GetQuery,PFNGLGETQUERYIVPROC,"glGetQueryiv");
#undef LOAD
 }
};
struct DrawState {
 const GlApi&a;const GlRenderApi&r;const Extra&e;GlContextNativeIdentity identity;
 GLint program=0,vao=0,active=0,origin=0,depth_mode=0,polygon[2]{};
 std::array<GLint,2>textures{},samplers{};
 std::array<GLint,6>blend{};std::array<GLboolean,4>mask{};
 GLboolean blend_enabled=false,scissor=false,depth_mask=false;
 GLfloat viewport[4]{};GLdouble depth_range[2]{};
 const std::array<GLenum,9>caps{GL_DEPTH_TEST,GL_CULL_FACE,GL_STENCIL_TEST,
  GL_FRAMEBUFFER_SRGB,GL_DITHER,GL_RASTERIZER_DISCARD,GL_COLOR_LOGIC_OP,
  GL_PRIMITIVE_RESTART,GL_PRIMITIVE_RESTART_FIXED_INDEX};
 std::array<GLboolean,9>enabled{};std::vector<GLboolean>clip_enabled;
 bool restored=false;
 DrawState(const GlApi&api,const GlRenderApi&render,const Extra&extra,GlContextNativeIdentity id):a(api),r(render),e(extra),identity(id){
  a.GetIntegerv(GL_CURRENT_PROGRAM,&program);a.GetIntegerv(GL_VERTEX_ARRAY_BINDING,&vao);
  a.GetIntegerv(GL_ACTIVE_TEXTURE,&active);a.GetIntegerv(GL_CLIP_ORIGIN,&origin);
  a.GetIntegerv(GL_CLIP_DEPTH_MODE,&depth_mode);a.GetIntegerv(GL_POLYGON_MODE,polygon);
  need(polygon[0]==polygon[1],"Separate front/back polygon state unsupported by bounded font draw");
  for(unsigned k=0;k<2;++k){e.GetIntegeri(GL_TEXTURE_BINDING_2D,k,&textures[k]);e.GetIntegeri(GL_SAMPLER_BINDING,k,&samplers[k]);}
  const std::array<GLenum,6>fields{GL_BLEND_SRC_RGB,GL_BLEND_DST_RGB,GL_BLEND_SRC_ALPHA,GL_BLEND_DST_ALPHA,GL_BLEND_EQUATION_RGB,GL_BLEND_EQUATION_ALPHA};
  for(unsigned k=0;k<6;++k)e.GetIntegeri(fields[k],0,&blend[k]);
  e.GetBooleani(GL_COLOR_WRITEMASK,0,mask.data());blend_enabled=e.IsEnabledi(GL_BLEND,0);
  scissor=e.IsEnabledi(GL_SCISSOR_TEST,0);e.GetBoolean(GL_DEPTH_WRITEMASK,&depth_mask);
  r.GetFloati_v(GL_VIEWPORT,0,viewport);e.GetDoublei(GL_DEPTH_RANGE,0,depth_range);
  for(unsigned k=0;k<caps.size();++k)enabled[k]=r.IsEnabled(caps[k]);
  GLint n=0;a.GetIntegerv(GL_MAX_CLIP_DISTANCES,&n);need(n>=0&&n<=64,"Unbounded GL clip distance state");
  clip_enabled.resize(static_cast<std::size_t>(n));for(GLint k=0;k<n;++k)clip_enabled[k]=r.IsEnabled(GL_CLIP_DISTANCE0+k);
 }
 void restore()noexcept{
  if(restored||!native_current(identity))return;
  r.UseProgram(static_cast<GLuint>(program));r.BindVertexArray(static_cast<GLuint>(vao));
  for(unsigned k=0;k<2;++k){e.ActiveTexture(GL_TEXTURE0+k);a.BindTexture(GL_TEXTURE_2D,static_cast<GLuint>(textures[k]));r.BindSampler(k,static_cast<GLuint>(samplers[k]));}
  e.ActiveTexture(static_cast<GLenum>(active));
  e.BlendFuncSeparatei(0,blend[0],blend[1],blend[2],blend[3]);e.BlendEquationSeparatei(0,blend[4],blend[5]);
  (blend_enabled?e.Enablei:e.Disablei)(GL_BLEND,0);(scissor?e.Enablei:e.Disablei)(GL_SCISSOR_TEST,0);
  e.ColorMaski(0,mask[0],mask[1],mask[2],mask[3]);r.DepthMask(depth_mask);
  r.PolygonMode(GL_FRONT_AND_BACK,static_cast<GLenum>(polygon[0]));
  r.ClipControl(static_cast<GLenum>(origin),static_cast<GLenum>(depth_mode));
  r.ViewportIndexedf(0,viewport[0],viewport[1],viewport[2],viewport[3]);e.DepthRangeIndexed(0,depth_range[0],depth_range[1]);
  for(unsigned k=0;k<caps.size();++k)(enabled[k]?r.Enable:r.Disable)(caps[k]);
  for(unsigned k=0;k<clip_enabled.size();++k)(clip_enabled[k]?r.Enable:r.Disable)(GL_CLIP_DISTANCE0+k);
  restored=true;
 }
 ~DrawState(){restore();}
};
GLuint program(const GlRenderApi&r){
 const GLuint p=r.CreateProgram();need(p!=0,"FontShader GL program allocation failed");
 try{
  for(auto [stage,source]:{std::pair{GL_VERTEX_SHADER,vs},std::pair{GL_FRAGMENT_SHADER,ps}}){
   const GLuint s=r.CreateShader(stage);need(s!=0,"FontShader stage allocation failed");
   try{r.ShaderSource(s,1,&source,nullptr);r.CompileShader(s);GLint ok=0;r.GetShaderiv(s,GL_COMPILE_STATUS,&ok);
    if(!ok){char log[8192]{};r.GetShaderInfoLog(s,sizeof(log),nullptr,log);throw std::runtime_error(log);}r.AttachShader(p,s);
   }catch(...){r.DeleteShader(s);throw;}r.DeleteShader(s);
  }
  r.LinkProgram(p);GLint ok=0;r.GetProgramiv(p,GL_LINK_STATUS,&ok);
  if(!ok){char log[8192]{};r.GetProgramInfoLog(p,sizeof(log),nullptr,log);throw std::runtime_error(log);}
 }catch(...){r.DeleteProgram(p);throw;}return p;
}
}
struct GlBnwFontShader::Impl {
 std::shared_ptr<GlContext>context; // Legacy diagnostic factory only.
 std::shared_ptr<const detail::BnwRendererContextPin>context_pin; // SAME Atlas token.
 detail::BnwOwnedGlRetirementNode::Owner retirement_node; // Exact owned names; handoff never allocates.
 std::shared_ptr<GlBnwSystemFontAtlas>atlas;
 std::shared_ptr<const ui::BnwSystemFontCacheOwner>cache;
 std::shared_ptr<const ui::BnwSystemFontCpuOwner>cpu;
 GlContextNativeIdentity identity;std::thread::id thread=std::this_thread::get_id();
 const GlApi*api=nullptr;std::unique_ptr<GlRenderApi>render;std::unique_ptr<Extra>extra;
 // New physical original-program branch only; source/dispatch are captured
 // independently of legacy public Context ownership and old diagnostic paths.
 std::optional<OriginalProgramSource::Guard> original_program_guard;
 unsigned original_program_selector=0;
 bool native_font_sampler_policy_applied=false; // Original-program branch only.
 std::optional<GlApi> owned_normal_api;
 void original_program_first()const{
  need(std::this_thread::get_id()==thread&&!failed&&context_pin&&!context&&atlas&&original_program_guard&&owned_normal_api,
   "Original FontShader physical scope absent/retired/foreign");
  original_program_guard->require_current_without_callbacks();context_pin->first_without_callbacks();
  atlas->require_same_owned_context_pin_no_current(context_pin);
  need(identity.window==context_pin->identity_.window&&identity.dc==context_pin->identity_.dc&&
   identity.context==context_pin->identity_.context&&context_pin->identity_.owner_thread,
   "Original FontShader physical identity changed");
  const auto*current=detail::BnwRendererContextPin::NormalAccess::api_current_without_switch_owned(context_pin);
  original_program_guard->require_current_without_callbacks();context_pin->first_without_callbacks();
  need(current&&api==&*owned_normal_api&&current->GetError==owned_normal_api->GetError&&
   current->GetGraphicsResetStatus==owned_normal_api->GetGraphicsResetStatus,
   "Original FontShader loaded normal dispatch changed");
  const auto pair=original_program_guard->native_selector_without_callbacks(original_program_selector);
  need(pair.native_selector==original_program_selector,"Original FontShader physical code selector changed");
  original_program_guard->require_current_without_callbacks();context_pin->first_without_callbacks();
 }
 bool busy=false,failed=false;
 GLuint shader=0,vao=0,vertices=0,indices=0,sampler=0;
 BnwFontShaderDrawResult draw_engine(const BnwFontShaderBatch*,const BnwFontShaderTarget&,
  const BnwFontShaderPackedBatch*);
 void vertex_declaration(bool packed){
  const auto&r=*render;r.VertexArrayVertexBuffer(vao,0,vertices,0,packed?24:32);
  for(unsigned k=0;k<3;++k){r.EnableVertexArrayAttrib(vao,k);r.VertexArrayAttribBinding(vao,k,0);
   r.VertexArrayAttribFormat(vao,k,k==2?4:packed&&k==0?3:2,
    packed&&k==2?GL_UNSIGNED_BYTE:GL_FLOAT,packed&&k==2?GL_TRUE:GL_FALSE,
    k==0?0:k==1?(packed?16:8):(packed?12:16));
  }
  context->check("Configure FontShader vertex declaration");
  // DrawState has already retained the external VAO; this owned VAO must be
  // bound for glGetVertexAttribiv. All attribute fields use its supported
  // query domain; binding stride uses the indexed vertex-binding domain.
  GLint actual=-1;api->GetIntegerv(GL_VERTEX_ARRAY_BINDING,&actual);
  context->check("Query actual FontShader VAO binding");
  need(actual==static_cast<GLint>(vao),"FontShader declaration query requires its bound VAO");
  actual=-1;extra->GetIntegeri(GL_VERTEX_BINDING_STRIDE,0,&actual);
  context->check("Query actual FontShader vertex stride");
  need(actual==(packed?24:32),"Actual FontShader vertex stride differs");
  for(unsigned k=0;k<3;++k){
   for(auto [field,value]:std::array<std::pair<GLenum,GLint>,5>{{
    {GL_VERTEX_ATTRIB_ARRAY_SIZE,k==2?4:packed&&k==0?3:2},
    {GL_VERTEX_ATTRIB_ARRAY_TYPE,packed&&k==2?GL_UNSIGNED_BYTE:GL_FLOAT},
    {GL_VERTEX_ATTRIB_ARRAY_NORMALIZED,packed&&k==2?GL_TRUE:GL_FALSE},
    {GL_VERTEX_ATTRIB_BINDING,0},
    {GL_VERTEX_ATTRIB_RELATIVE_OFFSET,k==0?0:k==1?(packed?16:8):(packed?12:16)}}}){
    actual=-1;extra->GetVertexAttrib(k,field,&actual);
    context->check("Query actual FontShader vertex attribute declaration");
    need(actual==value,"Actual FontShader vertex declaration differs");
   }
  }
 }
 GlContext&actual_context()const{
  if(context_pin)return context_pin->context_reference_without_callbacks();
  need(bool(context),"FontShader legacy Context missing");return *context;
 }
 void native()const{
  need(std::this_thread::get_id()==thread,"FontShader used on foreign thread");
  need(!failed,"FontShader consumer quarantined after caught reentrant draw");
  if(context_pin){
   need(!context,"Owned FontShader cannot retain a public strong Context clone");
   context_pin->first_without_callbacks();
   atlas->require_same_owned_context_pin_no_current(context_pin);
   const auto sources=atlas->sources_identity_no_current();
   need(sources.context==&actual_context()&&sources.cache.get()==cache.get()&&
    !sources.cache.owner_before(cache)&&!cache.owner_before(sources.cache)&&
    sources.window==identity.window,"Owned FontShader Atlas/cache control changed");
   need(same_identity(identity,actual_context().native_identity())&&native_current(identity),
    "Owned FontShader rejects foreign current HWND/DC/WGL context without switching");
   context_pin->first_without_callbacks();return;
  }
  need(same_identity(identity,context->native_identity())&&native_current(identity),"FontShader rejects foreign current HWND/DC/WGL context without switching");
  const auto sources=atlas->sources_no_current();
  need(sources.context.get()==context.get()&&sources.cache.get()==cache.get()&&sources.window==identity.window,"FontShader source atlas ownership changed");
 }
 void create_owned_program(){
  need(context_pin&&!context&&retirement_node,"Owned FontShader retirement storage missing");native();
  const auto&r=*render;auto&names=retirement_node->shader_resources_storage_without_callbacks();
  // Actual names enter the preallocated node before any POST.
  names.program=r.CreateProgram();native();need(names.program!=0,"FontShader GL program allocation failed");
  const std::array<GLenum,2>types{GL_VERTEX_SHADER,GL_FRAGMENT_SHADER};
  const std::array<const char*,2>sources{vs,ps};
  for(unsigned index=0;index<types.size();++index){
   names.stages[index]=r.CreateShader(types[index]);native();
   need(names.stages[index]!=0,"FontShader stage allocation failed");
   const auto stage=names.stages[index];const auto*source=sources[index];
   r.ShaderSource(stage,1,&source,nullptr);native();r.CompileShader(stage);native();
   GLint ok=0;r.GetShaderiv(stage,GL_COMPILE_STATUS,&ok);native();
   if(!ok){char log[8192]{};r.GetShaderInfoLog(stage,sizeof(log),nullptr,log);native();throw std::runtime_error(log);}
   r.AttachShader(names.program,stage);native();
  }
  r.LinkProgram(names.program);native();GLint ok=0;r.GetProgramiv(names.program,GL_LINK_STATUS,&ok);native();
  if(!ok){char log[8192]{};r.GetProgramInfoLog(names.program,sizeof(log),nullptr,log);native();throw std::runtime_error(log);}
  // Attached stages remain owned until verified node drain, including failures.
  // No unchecked DeleteShader/clearing of an actual name occurs here.
  shader=names.program; // Non-owning read alias; node has sole custody.
 }
 ~Impl(){
  if(context_pin){
   // Foreign thread/current handoff retains SAME Pin without GL/proc/allocation.
   shader=vao=vertices=indices=sampler=0;retirement_node.reset();return;
  }
  if(render&&std::this_thread::get_id()==thread&&native_current(identity)){
  if(sampler)render->DeleteSamplers(1,&sampler);if(indices)render->DeleteBuffers(1,&indices);
  if(vertices)render->DeleteBuffers(1,&vertices);if(vao)render->DeleteVertexArrays(1,&vao);if(shader)render->DeleteProgram(shader);
 }}
 void request_current(const BnwFontShaderBatch&b,const BnwFontShaderTarget&t)const{
  need(!context_pin,"Owned FontShader current requires genuine sealed preparedRun/Glyph issuer");
  native();need(b.current&&t.current,"Missing current diagnostic batch/target owner");
  need(b.current(),"Stale FontShader batch owner");native();
  need(t.current(),"Stale FontShader target owner");native();
  const auto f=cpu->current();native();
  need(cpu->native_window_handle()==identity.window,"Foreign font CPU client HWND");native();
  need(f.client.surface_owner==t.retained_client.surface_owner&&f.client.revision==t.retained_client.revision&&
   f.client.width==t.retained_client.width&&f.client.height==t.retained_client.height,
   "Stale diagnostic client owner/extent/revision");
  for(const auto&l:b.layers)for(const auto&q:l.quads){need(cache->current(q.uploaded_slot),"Stale source glyph quote");native();
   need(atlas->has_uploaded_quote_no_current(q.uploaded_slot),"Missing SAME accepted atlas pair upload");}
  need(same_pair(b.retained_pair,atlas->sources_no_current().pair),"Changed native font texture pair");
  RECT rect{};DWORD process=0;GetWindowThreadProcessId(reinterpret_cast<HWND>(identity.window),&process);
  need(process==GetCurrentProcessId()&&GetClientRect(reinterpret_cast<HWND>(identity.window),&rect)&&
   rect.right-rect.left==t.width&&rect.bottom-rect.top==t.height&&t.width>0&&t.height>0&&
   f.client.width==t.width&&f.client.height==t.height,"FontShader target must use SAME actual current client extent");
 }
 void gpu_inputs(const BnwFontShaderBatch&b,const BnwFontShaderTarget&t)const{
  native();const auto&a=*api;const auto&r=*render;GLint actual=0;
  need(t.framebuffer&&t.color_texture&&a.IsTexture(t.color_texture),"Missing real FontShader RGBA8 target");
  a.GetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING,&actual);need(static_cast<GLuint>(actual)==t.framebuffer,"Foreign bound draw framebuffer");
  need(r.CheckNamedFramebufferStatus(t.framebuffer,GL_DRAW_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE,"Incomplete actual FontShader target");
  for(auto [field,expected]:std::array<std::pair<GLenum,GLuint>,3>{{{GL_FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE,GL_TEXTURE},{GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME,t.color_texture},{GL_FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL,0}}}){
   r.GetNamedFramebufferAttachmentParameteriv(t.framebuffer,GL_COLOR_ATTACHMENT0,field,&actual);need(static_cast<GLuint>(actual)==static_cast<GLuint>(expected),"Substituted FontShader target attachment");}
  for(auto [field,expected]:{std::pair{GL_TEXTURE_WIDTH,t.width},std::pair{GL_TEXTURE_HEIGHT,t.height},std::pair{GL_TEXTURE_INTERNAL_FORMAT,static_cast<int>(GL_RGBA8)}}){
   a.GetTextureLevelParameteriv(t.color_texture,0,field,&actual);need(actual==expected,"Changed target texture extent/format");}
  a.GetIntegerv(GL_DRAW_BUFFER0,&actual);need(actual==GL_COLOR_ATTACHMENT0,"Unexpected FontShader draw buffer0");
  GLint maximum=0;a.GetIntegerv(GL_MAX_DRAW_BUFFERS,&maximum);for(GLint k=1;k<maximum;++k){a.GetIntegerv(GL_DRAW_BUFFER0+k,&actual);need(actual==GL_NONE,"Bounded FontShader requires one color output");}
  for(const auto texture:{b.retained_pair.green_texture,b.retained_pair.shadow_texture}){
   need(texture&&a.IsTexture(texture)&&texture!=t.color_texture,"Missing/substituted/feedback native font plane");
   for(auto [field,expected]:{std::pair{GL_TEXTURE_WIDTH,b.retained_pair.width},std::pair{GL_TEXTURE_HEIGHT,b.retained_pair.height},std::pair{GL_TEXTURE_INTERNAL_FORMAT,static_cast<int>(GL_R8)}}){a.GetTextureLevelParameteriv(texture,0,field,&actual);need(actual==expected,"Changed native brightness plane storage");}
   a.GetTextureParameteriv(texture,GL_TEXTURE_BASE_LEVEL,&actual);need(actual==0,"Changed font plane base level");a.GetTextureParameteriv(texture,GL_TEXTURE_MAX_LEVEL,&actual);need(actual==0,"Changed font plane mip range");
   GLint swizzle[4]{};a.GetTextureParameteriv(texture,GL_TEXTURE_SWIZZLE_RGBA,swizzle);need(swizzle[0]==GL_RED&&swizzle[1]==GL_RED&&swizzle[2]==GL_RED&&swizzle[3]==GL_ONE,"Changed native font plane swizzle");
  }
  a.GetIntegerv(GL_TRANSFORM_FEEDBACK_ACTIVE,&actual);need(actual==0,"External transform feedback active during FontShader control");
  for(const auto query:{GL_SAMPLES_PASSED,GL_ANY_SAMPLES_PASSED,GL_ANY_SAMPLES_PASSED_CONSERVATIVE,GL_PRIMITIVES_GENERATED,GL_TRANSFORM_FEEDBACK_PRIMITIVES_WRITTEN,GL_TIME_ELAPSED}){extra->GetQuery(query,GL_CURRENT_QUERY,&actual);need(actual==0,"External GL query active during bounded FontShader control");}
 }
};
GlBnwFontShader::GlBnwFontShader(std::unique_ptr<Impl>i):impl_(std::move(i)){}
GlBnwFontShader::~GlBnwFontShader()=default;
std::shared_ptr<GlBnwFontShader>GlBnwFontShader::create(std::shared_ptr<GlContext>c,std::shared_ptr<GlBnwSystemFontAtlas>a){
 need(c&&a,"Missing actual SAME font context/atlas");auto i=std::make_unique<Impl>();i->context=std::move(c);i->atlas=std::move(a);
 i->identity=i->context->native_identity();need(native_current(i->identity),"FontShader creation requires already current SAME context");
 const auto s=i->atlas->sources_no_current();need(s.context.get()==i->context.get()&&s.window==i->identity.window&&s.cache,"Foreign native atlas/context");
 i->cache=s.cache;i->cpu=i->cache->source_cpu_owner();need(i->cpu&&i->cpu->native_window_handle()==i->identity.window,"Foreign actual font CPU client");i->native();
 i->api=&i->context->api();i->render=std::make_unique<GlRenderApi>(*i->context);i->extra=std::make_unique<Extra>(*i->context);i->native();
 const auto&r=*i->render;i->shader=program(r);r.CreateVertexArrays(1,&i->vao);r.CreateBuffers(1,&i->vertices);r.CreateBuffers(1,&i->indices);r.CreateSamplers(1,&i->sampler);
 need(i->vao&&i->vertices&&i->indices&&i->sampler,"FontShader diagnostic geometry/sampler allocation failed");
 static_assert(sizeof(BnwFontShaderVertex)==32);r.VertexArrayVertexBuffer(i->vao,0,i->vertices,0,sizeof(BnwFontShaderVertex));r.VertexArrayElementBuffer(i->vao,i->indices);
 for(unsigned k=0;k<3;++k){r.EnableVertexArrayAttrib(i->vao,k);r.VertexArrayAttribBinding(i->vao,k,0);r.VertexArrayAttribFormat(i->vao,k,k==2?4:2,GL_FLOAT,GL_FALSE,k==0?0:k==1?8:16);}
 const auto record=resolve_bnw_native_sampler_record(9);need(record.address_u==1&&record.address_v==1&&record.address_w==1&&record.min_filter==1&&record.mag_filter==1&&record.mip_filter==1&&!record.srgb,"Changed native sampler9 source contract");
 r.SamplerParameteri(i->sampler,GL_TEXTURE_MIN_FILTER,GL_NEAREST_MIPMAP_NEAREST);r.SamplerParameteri(i->sampler,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
 for(const auto field:{GL_TEXTURE_WRAP_S,GL_TEXTURE_WRAP_T,GL_TEXTURE_WRAP_R})r.SamplerParameteri(i->sampler,field,GL_REPEAT);
 if(i->context->has_extension("GL_EXT_texture_filter_anisotropic")||i->context->has_extension("GL_ARB_texture_filter_anisotropic"))r.SamplerParameterf(i->sampler,0x84FE,1.0f);
 i->native();i->context->check("Compile source-backed diagnostic FontShader; Label remains uninitialized");
 return std::shared_ptr<GlBnwFontShader>(new GlBnwFontShader(std::move(i)));
}
std::shared_ptr<GlBnwFontShader>GlBnwFontShader::create_from_owned_renderer_context(
 std::shared_ptr<const detail::BnwRendererContextPin>pin,std::shared_ptr<GlBnwSystemFontAtlas>atlas){
 need(pin&&atlas,"Owned FontShader requires actual counted Context/Atlas");
 pin->first_without_callbacks();atlas->require_same_owned_context_pin_no_current(pin);
 // Raw allocation happens before Context evaluation; new_handler may
 // retire outside authority. No proc-address load occurs until the next FIRST.
 struct DispatchStorage final{
  void* value;
  explicit DispatchStorage(std::size_t bytes):value(::operator new(bytes)){}
  ~DispatchStorage(){::operator delete(value);}
  DispatchStorage(const DispatchStorage&)=delete;
  DispatchStorage&operator=(const DispatchStorage&)=delete;
 };
 static_assert(alignof(GlRenderApi)<=alignof(std::max_align_t));
 static_assert(alignof(Extra)<=alignof(std::max_align_t));
 std::shared_ptr<GlBnwFontShader>result;
 {
  auto i=std::make_unique<Impl>();pin->first_without_callbacks();
  i->context_pin=pin;i->atlas=atlas;
  {const auto sources=atlas->sources_identity_no_current();pin->first_without_callbacks();
   need(sources.context==&i->actual_context()&&sources.cache,"Owned FontShader Atlas Context/cache missing");
   i->cache=sources.cache;i->cpu=i->cache->source_cpu_owner();
   need(bool(i->cpu),"Owned FontShader Atlas source CPU missing");
  }
  // CPU source getters requiring callbacks belong to the outer genuine
  // renderer transaction. This physical factory never manufactures FULL.
  i->identity=i->actual_context().native_identity();i->native();
  i->api=&i->actual_context().api();i->native();
  {DispatchStorage storage(sizeof(GlRenderApi));i->native();
   i->render.reset(::new(storage.value) GlRenderApi(i->actual_context()));
   storage.value=nullptr;i->native();
  }
  {DispatchStorage storage(sizeof(Extra));i->native();
   i->extra.reset(::new(storage.value) Extra(i->actual_context()));
   storage.value=nullptr;i->native();
  }
  // Capture actual loaded deletion/reset/error dispatch BEFORE any name.
  // new_handler retirement is rejected before GL effects.
  i->retirement_node=detail::BnwOwnedGlRetirementNode::shader_resources_owned(pin,*i->api,*i->render);i->native();
  auto&names=i->retirement_node->shader_resources_storage_without_callbacks();
  const auto&r=*i->render;i->create_owned_program();i->native();
  r.CreateVertexArrays(1,&names.vao);i->native();
  r.CreateBuffers(1,&names.vertices);i->native();
  r.CreateBuffers(1,&names.indices);i->native();
  r.CreateSamplers(1,&names.sampler);i->native();
  // Read aliases only. Failed creation POST leaves every name in the node.
  i->vao=names.vao;i->vertices=names.vertices;i->indices=names.indices;i->sampler=names.sampler;
 need(i->vao&&i->vertices&&i->indices&&i->sampler,"FontShader diagnostic geometry/sampler allocation failed");
 static_assert(sizeof(BnwFontShaderVertex)==32);r.VertexArrayVertexBuffer(i->vao,0,i->vertices,0,sizeof(BnwFontShaderVertex));r.VertexArrayElementBuffer(i->vao,i->indices);
 for(unsigned k=0;k<3;++k){r.EnableVertexArrayAttrib(i->vao,k);r.VertexArrayAttribBinding(i->vao,k,0);r.VertexArrayAttribFormat(i->vao,k,k==2?4:2,GL_FLOAT,GL_FALSE,k==0?0:k==1?8:16);}
 const auto record=resolve_bnw_native_sampler_record(9);need(record.address_u==1&&record.address_v==1&&record.address_w==1&&record.min_filter==1&&record.mag_filter==1&&record.mip_filter==1&&!record.srgb,"Changed native sampler9 source contract");
 r.SamplerParameteri(i->sampler,GL_TEXTURE_MIN_FILTER,GL_NEAREST_MIPMAP_NEAREST);r.SamplerParameteri(i->sampler,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
 for(const auto field:{GL_TEXTURE_WRAP_S,GL_TEXTURE_WRAP_T,GL_TEXTURE_WRAP_R})r.SamplerParameteri(i->sampler,field,GL_REPEAT);
 if(i->actual_context().has_extension("GL_EXT_texture_filter_anisotropic")||i->actual_context().has_extension("GL_ARB_texture_filter_anisotropic"))r.SamplerParameterf(i->sampler,0x84FE,1.0f);
  i->native();i->actual_context().check("Create owned FontShader storage; genuine preparedRun pending");i->native();
  result=std::shared_ptr<GlBnwFontShader>(new GlBnwFontShader(std::move(i)));
  pin->first_without_callbacks();result->impl_->native();
 }
 // All local source snapshots/allocation temporaries are gone before POST.
 pin->first_without_callbacks();result->impl_->native();return result;
}
BnwFontShaderDrawResult GlBnwFontShader::draw(const BnwFontShaderBatch&b,const BnwFontShaderTarget&t){
 need(!impl_->context_pin,"Owned FontShader draw awaits genuine sealed preparedRun/Glyph producer");
 const auto keep_alive=shared_from_this();return impl_->draw_engine(&b,t,nullptr);
}
BnwFontShaderDrawResult GlBnwFontShader::draw_packed(const BnwFontShaderPackedBatch&borrowed,const BnwFontShaderTarget&t){
 need(!impl_->context_pin,"Owned FontShader packed draw awaits genuine sealed preparedRun/Glyph producer");
 const auto keep_alive=shared_from_this();return impl_->draw_engine(nullptr,t,&borrowed);
}
BnwFontShaderDrawResult GlBnwFontShader::Impl::draw_engine(const BnwFontShaderBatch*borrowed_batch,const BnwFontShaderTarget&borrowed_target,const BnwFontShaderPackedBatch*borrowed_packed){
 auto&i=*this;
 need(!i.context_pin,"Owned FontShader engine awaits genuine sealed preparedRun/Glyph producer");
 need(std::this_thread::get_id()==i.thread,"FontShader used on foreign thread");
 if(i.busy){i.failed=true;throw std::runtime_error("Reentrant same-owner FontShader draw refused");}
 i.native();
 struct Busy{Impl&i;explicit Busy(Impl&v):i(v){i.busy=true;}~Busy(){i.busy=false;}}busy(i);
 need(bool(borrowed_batch)!=bool(borrowed_packed),"Ambiguous typed font input");
 const std::optional<BnwFontShaderPackedBatch>retained_packed=borrowed_packed?std::optional<BnwFontShaderPackedBatch>(*borrowed_packed):std::nullopt;
 const auto*packed=retained_packed?&*retained_packed:nullptr;
 const BnwFontShaderBatch b=borrowed_batch?*borrowed_batch:detail::packed_validation_view(*packed);const BnwFontShaderTarget t=borrowed_target;
 i.native();need(b.caller_declared_diagnostic_colors,"Native packed-color producer absent; diagnostic declaration required");
 const auto finite=[](float f){return std::isfinite(f);};
 for(const auto&row:b.controls.source_matrix_rows)for(const float f:row)need(finite(f),"Nonfinite FontShader source matrix");
 for(const float f:b.controls.translation)need(finite(f),"Nonfinite FontShader translation");
 for(const float f:b.controls.depth_pair)need(finite(f),"Nonfinite FontShader depth controls");
 need(finite(b.controls.depth_pair[0]+b.controls.depth_pair[1])&&b.controls.depth_pair[0]+b.controls.depth_pair[1]>0,"Nonpositive/overflow diagnostic clip depth scale");
 need(finite(b.controls.parent_alpha)&&b.controls.parent_alpha>=0&&b.controls.parent_alpha<=1,"Invalid FontShader inherited alpha");
 for(const float f:b.controls.clip_xmin_xmax_ymin_ymax)need(finite(f),"Nonfinite FontShader clip");
 need(b.controls.clip_xmin_xmax_ymin_ymax[0]<=b.controls.clip_xmin_xmax_ymin_ymax[1]&&b.controls.clip_xmin_xmax_ymin_ymax[2]<=b.controls.clip_xmin_xmax_ymin_ymax[3],"Reversed diagnostic clip bounds");
 need(t.pixel_centers==BnwFontPixelCenterControl::unshifted_gl||t.pixel_centers==BnwFontPixelCenterControl::d3d9_integer_center_diagnostic,"Unknown font coordinate control");
 unsigned previous=3;std::size_t count=0;for(const auto&l:b.layers){need(l.slot<previous,"FontShader layers must follow source order2,1,0 without duplicates");previous=l.slot;
  need(l.quads.size()<=static_cast<std::size_t>(std::numeric_limits<GLsizei>::max()/6),"FontShader layer index overflow");
  need(l.quads.size()<=(std::numeric_limits<std::size_t>::max()-count),"FontShader total count overflow");count+=l.quads.size();
  for(const auto&q:l.quads)for(const auto&v:q.vertices){need(finite(v.x)&&finite(v.y)&&finite(v.u)&&finite(v.v),"Nonfinite prepared glyph vertex");for(float f:v.diagnostic_rgba)need(finite(f)&&f>=0&&f<=1,"Invalid caller-declared normalized diagnostic color");
   const float x=v.x+b.controls.translation[0],y=v.y+b.controls.translation[1];need(finite(x)&&finite(y),"Overflow in translated diagnostic vertex");
   for(unsigned component=0;component<4;++component){const float clip=x*b.controls.source_matrix_rows[0][component]+y*b.controls.source_matrix_rows[1][component]+b.controls.source_matrix_rows[3][component];need(finite(clip)&&finite(clip*(b.controls.depth_pair[0]+b.controls.depth_pair[1])),"Overflow in diagnostic clip coordinates");}
  }}
 need(count>0,"Empty diagnostic FontShader batch");i.request_current(b,t);i.gpu_inputs(b,t);i.native();i.context->check("Before diagnostic FontShader draw");
 const auto&r=*i.render;const auto&e=*i.extra;BnwFontShaderDrawResult result;result.pixel_centers=t.pixel_centers;result.actual_target_row_origin=bnw_font_target_row_origin(t.pixel_centers);
 {
  DrawState state(*i.api,r,e,i.identity);r.UseProgram(i.shader);r.BindVertexArray(i.vao);i.vertex_declaration(packed!=nullptr);
  const auto uniform=[&](const char*n){const auto u=r.GetUniformLocation(i.shader,n);need(u>=0,"FontShader uniform missing");return u;};
  r.ProgramUniform4fv(i.shader,uniform("uRows[0]"),4,b.controls.source_matrix_rows[0].data());
  r.ProgramUniform2fv(i.shader,uniform("uTranslation"),1,b.controls.translation.data());r.ProgramUniform2fv(i.shader,uniform("uDepth"),1,b.controls.depth_pair.data());
  r.ProgramUniform1f(i.shader,uniform("uParentAlpha"),b.controls.parent_alpha);r.ProgramUniform4fv(i.shader,uniform("uClip"),1,b.controls.clip_xmin_xmax_ymin_ymax.data());r.ProgramUniform1f(i.shader,uniform("uAllowColor"),b.controls.allow_color?1:0);r.ProgramUniform1i(i.shader,uniform("uBlockTexture"),1);
  const bool integer_center=t.pixel_centers==BnwFontPixelCenterControl::d3d9_integer_center_diagnostic;r.ProgramUniform1i(i.shader,uniform("uNativeTopAtRowZero"),integer_center?1:0);r.ProgramUniform1f(i.shader,uniform("uTargetHeight"),static_cast<float>(t.height));
  const float shift=integer_center?.5f:0;const std::array<float,2>bias{-shift,-shift};r.ProgramUniform2fv(i.shader,uniform("uFragmentBias"),1,bias.data());
  r.ClipControl(integer_center?GL_UPPER_LEFT:GL_LOWER_LEFT,GL_ZERO_TO_ONE);r.ViewportIndexedf(0,shift,integer_center?shift:-shift,static_cast<float>(t.width),static_cast<float>(t.height));e.DepthRangeIndexed(0,0,1);
  for(const auto cap:state.caps)r.Disable(cap);for(unsigned k=0;k<state.clip_enabled.size();++k)r.Disable(GL_CLIP_DISTANCE0+k);
  e.Disablei(GL_SCISSOR_TEST,0);r.DepthMask(GL_FALSE);r.PolygonMode(GL_FRONT_AND_BACK,GL_FILL);
  e.Enablei(GL_BLEND,0);e.BlendFuncSeparatei(0,GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA,GL_ONE,GL_ONE);e.BlendEquationSeparatei(0,GL_FUNC_ADD,GL_FUNC_ADD);e.ColorMaski(0,GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
  std::size_t layer_index=0;for(const auto&l:b.layers){const auto current_layer=layer_index++;if(l.quads.empty())continue;
   std::vector<BnwFontShaderVertex>vertices;std::vector<std::uint32_t>indices;vertices.reserve(l.quads.size()*4);indices.reserve(l.quads.size()*6);
   for(const auto&q:l.quads){const auto n=static_cast<std::uint32_t>(vertices.size());vertices.insert(vertices.end(),q.vertices.begin(),q.vertices.end());for(const auto k:{0u,1u,2u,0u,2u,3u})indices.push_back(n+k);}
   // Both actual atlas textures remain owned by SAME atlas. This does not
   // allocate/copy a texture. Unused stage0 differs from native selected pair.
   const auto chosen=l.slot==2?b.retained_pair.shadow_texture:b.retained_pair.green_texture;
   const auto other=l.slot==2?b.retained_pair.green_texture:b.retained_pair.shadow_texture;
   i.api->BindTextureUnit(0,other);i.api->BindTextureUnit(1,chosen);r.BindSampler(0,i.sampler);r.BindSampler(1,i.sampler);
   if(packed){std::vector<ui::NativeLabelPackedVertex>stream;
    for(const auto&q:packed->layers[current_layer].quads)stream.insert(stream.end(),q.vertices.begin(),q.vertices.end());
    r.NamedBufferData(i.vertices,static_cast<GLsizeiptr>(stream.size()*sizeof(stream[0])),stream.data(),GL_STREAM_DRAW);
   }else r.NamedBufferData(i.vertices,static_cast<GLsizeiptr>(vertices.size()*sizeof(vertices[0])),vertices.data(),GL_STREAM_DRAW);
   r.NamedBufferData(i.indices,static_cast<GLsizeiptr>(indices.size()*sizeof(indices[0])),indices.data(),GL_STREAM_DRAW);
   r.DrawElements(GL_TRIANGLES,static_cast<GLsizei>(indices.size()),GL_UNSIGNED_INT,nullptr);++result.draw_calls;result.quads+=l.quads.size();
  }
  state.restore();i.native();i.context->check("Draw/restore diagnostic native FontShader controls");
 }
 // Callback/source rechecks occur after state restoration. A source mutation
 // after the actual GPU effect is a failure, never an accepted receipt.
 i.request_current(b,t);i.gpu_inputs(b,t);i.native();i.context->check("After source-backed diagnostic FontShader draw");
 result.actual_context=i.identity;result.accepted_pair=b.retained_pair;result.actual_target_framebuffer=t.framebuffer;result.actual_target_color_texture=t.color_texture;result.actual_target_width=t.width;result.actual_target_height=t.height;result.accepted_pair_revision=b.retained_pair.revision;result.source_shader_available=true;result.actual_vertex_input=packed?BnwFontVertexInputControl::source_rgba_ubyte4n_stride24:BnwFontVertexInputControl::diagnostic_float32;return result;
}
std::string_view bnw_font_shader_vertex_glsl(){return vs;}
std::string_view bnw_font_shader_fragment_glsl(){return ps;}

// Source10 immutable historical per-Get DATA. This consumer performs no GL,
// Atlas lookup, upload, late journal/current-quote test or producer cursor commit.
struct GlBnwFontShader::PositionedVertices::Storage final {
 using Data=::civ5::ui::detail::BnwLabelPaintPositionedData;
 const std::thread::id creator=std::this_thread::get_id();
 mutable std::atomic<bool> alive{true};
 const Data::Guard data_guard;
 const std::vector<Data::RecordData> records;
 const std::vector<Data::GlyphData> glyphs;
 const std::vector<Data::PassData> passes;
 const std::array<std::uint32_t,2> numeric;
 const std::array<std::vector<RawVertex>,3> vertices;
 const std::array<std::vector<std::uint32_t>,3> indices;
 const std::uint32_t captured_pass0_vertices;
 Storage(Data::Guard actual,std::vector<Data::RecordData>&& actual_records,
  std::vector<Data::GlyphData>&& actual_glyphs,std::vector<Data::PassData>&& actual_passes,
  std::array<std::uint32_t,2> actual_numeric,
  std::array<std::vector<RawVertex>,3>&& actual_vertices,
  std::array<std::vector<std::uint32_t>,3>&& actual_indices,std::uint32_t actual_count)
  :data_guard(std::move(actual)),records(std::move(actual_records)),
   glyphs(std::move(actual_glyphs)),passes(std::move(actual_passes)),numeric(actual_numeric),
   vertices(std::move(actual_vertices)),indices(std::move(actual_indices)),
   captured_pass0_vertices(actual_count){first();}
 void first()const{
  need(creator==std::this_thread::get_id()&&alive.load(),
   "Positioned vertex DATA lexical lease retired/foreign");
  data_guard.require_current_without_callbacks();
  need(alive.load(),"Positioned vertex DATA retired");
 }
 void full(const runtime::BnwWorldViewSourceOperationLoan& operation)const{
  first();data_guard.require_source_owned(operation);first();
 }
};
GlBnwFontShader::PositionedVertices::PositionedVertices(std::shared_ptr<const Storage> actual)
 :storage_(std::move(actual)){storage_->first();}
GlBnwFontShader::PositionedVertices::~PositionedVertices(){storage_->alive.store(false);}
void GlBnwFontShader::PositionedVertices::require_current_without_callbacks()const{
 const auto actual=storage_;actual->first();
}
void GlBnwFontShader::PositionedVertices::require_source_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation)const{
 const auto actual=storage_;actual->full(operation);
}
void GlBnwFontShader::PositionedVertices::require_actual_renderer_without_callbacks(
 const ::civ5::ui::UiScreenGlRenderer& renderer)const{
 const auto actual=storage_;actual->first();
 actual->data_guard.require_actual_renderer_without_callbacks(renderer);actual->first();
}
std::span<const GlBnwFontShader::PositionedVertices::RawVertex>
GlBnwFontShader::PositionedVertices::vertices_without_callbacks(unsigned pass)const{
 const auto actual=storage_;actual->first();need(pass<3,"Positioned vertex DATA pass out of range");
 return actual->vertices[pass];
}
std::span<const std::uint32_t>
GlBnwFontShader::PositionedVertices::indices_without_callbacks(unsigned pass)const{
 const auto actual=storage_;actual->first();need(pass<3,"Positioned vertex DATA pass out of range");
 return actual->indices[pass];
}
std::uint32_t GlBnwFontShader::PositionedVertices::captured_pass0_vertex_count_without_callbacks()const{
 const auto actual=storage_;actual->first();return actual->captured_pass0_vertices;
}
std::unique_ptr<GlBnwFontShader::PositionedVertices> GlBnwFontShader::positioned_vertices_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,
 const ::civ5::ui::detail::BnwLabelPaintPositionedData& actual_data){
 using Data=::civ5::ui::detail::BnwLabelPaintPositionedData;
 using RawVertex=PositionedVertices::RawVertex;
 // The ONLY facade read precedes every allocation/callback. Guard owns physical
 // immutable storage while its FIRST keeps the original DATA lexical lease.
 const auto data_guard=actual_data.capture_guard_without_callbacks();
 const auto first=[&]{data_guard.require_current_without_callbacks();};
 const auto full=[&]{first();data_guard.require_source_owned(operation);first();};
 first();
 const auto record_view=data_guard.records_without_callbacks();
 const auto glyph_view=data_guard.glyphs_without_callbacks();
 const auto pass_view=data_guard.passes_without_callbacks();
 const auto numeric=data_guard.actual_numeric_profile_without_callbacks();first();
 full();
 std::vector<Data::RecordData> records;
 std::vector<Data::GlyphData> glyphs;
 std::vector<Data::PassData> passes;
 first();records.assign(record_view.begin(),record_view.end());first();full();
 first();glyphs.assign(glyph_view.begin(),glyph_view.end());first();full();
 first();passes.assign(pass_view.begin(),pass_view.end());first();full();
 // This selected positive170 route has one genuine record with constant style
 // selectors. Other/empty branches remain STOP; no native equal-count gate is
 // asserted. Prove the captured pass0 reservation fits every admitted bucket.
 need(records.size()==1&&!glyphs.empty(),"Positioned DATA selected system pass0 is empty");
 const auto& record=records.front();
 need(record.source_discriminator==0xffffffffu,"Positioned DATA selected record unsupported");
 need((record.style_pass_selectors[0]&0x80000000u)==0,
  "Positioned DATA selected system pass0 was not emitted");
 need(glyphs.size()<=std::numeric_limits<std::uint32_t>::max()/6u,
  "Positioned DATA vertex/index count exceeds uint32");
 need(numeric[0]<=0xffffu,"Positioned DATA x87 profile is not a raw control word");
 std::size_t expected_pass=0;
 for(std::size_t ordinal=0;ordinal<glyphs.size();++ordinal){
  need(glyphs[ordinal].record_ordinal==0,"Positioned DATA glyph belongs to another record");
  for(unsigned pass=0;pass<3;++pass){
   if(record.style_pass_selectors[pass]&0x80000000u)continue;
   need(expected_pass<passes.size(),"Positioned DATA genuine emission missing");
   const auto& emitted=passes[expected_pass++];
   need(emitted.glyph_ordinal==ordinal&&emitted.pass_ordinal==pass&&
    emitted.packed_color==record.packed_colors[pass],
    "Positioned DATA emission membership/color differs from genuine record");
  }
 }
 need(expected_pass==passes.size(),"Positioned DATA contains an extra emission");first();full();
 const auto vertex_count=static_cast<std::uint32_t>(glyphs.size())*4u;
 const auto index_count=static_cast<std::uint32_t>(glyphs.size())*6u;
 static_assert(sizeof(RawVertex)==24);
 std::array<std::vector<RawVertex>,3> vertices;
 std::array<std::vector<std::uint32_t>,3> indices;
 for(unsigned pass=0;pass<3;++pass){
  if(record.style_pass_selectors[pass]&0x80000000u)continue;
  first();vertices[pass].reserve(vertex_count);first();full();
  first();indices[pass].reserve(index_count);first();full();
 }
 constexpr std::array<std::array<std::uint32_t,3>,3> xyz={{{0u,0u,0u},
  {0x3f800000u,0x3f800000u,0u},{0u,0u,0u}}};
 constexpr std::array<std::uint32_t,6> quad_indices={0u,1u,2u,0u,2u,3u};
 for(const auto& emitted:passes){
  const auto& glyph=glyphs[emitted.glyph_ordinal];const auto pass=emitted.pass_ordinal;
  full();first();
  const detail::BnwLabelPositionedNumericInputData input{
   glyph.native_bits,glyph.cursor_x,glyph.line_y,xyz[pass],emitted.packed_color,
   static_cast<std::uint16_t>(numeric[0]),numeric[1]};
  const auto generated=detail::build_label_positioned_numeric_data(input);
  first();
  // Byte copy only. Do not convert/quiet UV or color bits, and do not commit
  // generated.next_cursor_x/advance: Source10 already advanced this glyph once.
  std::array<RawVertex,4> raw;
  static_assert(sizeof(raw)==sizeof(generated.vertices));
  std::memcpy(raw.data(),generated.vertices.data(),sizeof(raw));
  const auto base=static_cast<std::uint32_t>(vertices[pass].size());
  need(vertices[pass].capacity()-vertices[pass].size()>=4&&
   indices[pass].capacity()-indices[pass].size()>=6,
   "Positioned DATA preallocated bucket capacity lost");
  for(const auto& vertex:raw)vertices[pass].push_back(vertex);
  for(const auto index:quad_indices)indices[pass].push_back(base+index);
  first();full();
 }
 for(unsigned pass=0;pass<3;++pass){
  const bool admitted=(record.style_pass_selectors[pass]&0x80000000u)==0;
  need(vertices[pass].size()==(admitted?vertex_count:0u)&&
   indices[pass].size()==(admitted?index_count:0u),
   "Positioned DATA bucket does not cover every genuine glyph");
 }
 // Both adoption allocations are guarded separately. Frozen const storage
 // keeps copied snapshots and original closed Guard, with its own retirement.
 first();
 std::unique_ptr<PositionedVertices::Storage> provisional(new PositionedVertices::Storage(
  data_guard,std::move(records),std::move(glyphs),std::move(passes),numeric,
  std::move(vertices),std::move(indices),vertex_count));
 provisional->first();full();
 first();std::shared_ptr<const PositionedVertices::Storage> frozen(std::move(provisional));
 frozen->first();full();
 frozen->first();std::unique_ptr<PositionedVertices> result(new PositionedVertices(frozen));
 frozen->first();frozen->full(operation);frozen->first();return result;
}

// Native family2B selector1 uses sharedVS and this distinct PS. No VPOS or
// clip-discard instructions exist in the pinned352-byte fragment source.
constexpr char font_original_ps1[]=R"glsl(#version 450 core
in vec2 vUv;
in vec4 vColor;
uniform sampler2D uBlockTexture;
uniform float uAllowColor;
layout(location=0) out vec4 result;
void main(){
 float coverage=1.0-texture(uBlockTexture,vUv).r;
 result=vec4(uAllowColor<=0.0?vec3(coverage):vColor.rgb,coverage*vColor.a);
})glsl";
constexpr std::string_view font_original_ps1_sha256=
 "77d997b1dca46f32833c08a92662dd17a8d04db1028841dd7760b25d9037cd7d";
// File/hash cleanup carries only actual physical handles and dispatch. It
// never invokes Source/Context or attempts to renew a retired production loan.
namespace {
struct FontOriginalFile final {
 HANDLE handle=INVALID_HANDLE_VALUE;
 bool close()noexcept{if(handle==INVALID_HANDLE_VALUE)return true;const auto h=handle;handle=INVALID_HANDLE_VALUE;return CloseHandle(h)!=FALSE;}
 ~FontOriginalFile(){close();}
};
struct FontOriginalHash final {
 BCRYPT_ALG_HANDLE algorithm=nullptr;BCRYPT_HASH_HANDLE hash=nullptr;
 bool close()noexcept{bool ok=true;if(hash){const auto h=hash;hash=nullptr;ok=BCryptDestroyHash(h)>=0&&ok;}
  if(algorithm){const auto a=algorithm;algorithm=nullptr;ok=BCryptCloseAlgorithmProvider(a,0)>=0&&ok;}return ok;}
 ~FontOriginalHash(){close();}
};
template<class First>std::array<char,64> font_original_sha256(
 std::span<const std::uint8_t> bytes,const First&first){
 FontOriginalHash handles;std::array<UCHAR,65536>object{};std::array<UCHAR,32>digest{};
 first();const auto open=BCryptOpenAlgorithmProvider(&handles.algorithm,BCRYPT_SHA256_ALGORITHM,nullptr,0);first();need(open>=0,"Original FontShader SHA256 provider unavailable");
 DWORD size=0,returned=0;first();const auto property=BCryptGetProperty(handles.algorithm,BCRYPT_OBJECT_LENGTH,reinterpret_cast<PUCHAR>(&size),sizeof(size),&returned,0);first();
 need(property>=0&&returned==sizeof(size)&&size>0&&size<=object.size(),"Unbounded Original FontShader hash object");
 first();const auto create=BCryptCreateHash(handles.algorithm,&handles.hash,object.data(),size,nullptr,0,0);first();need(create>=0,"Original FontShader hash creation failed");
 while(!bytes.empty()){const auto count=static_cast<ULONG>(std::min<std::size_t>(bytes.size(),65536));first();const auto feed=BCryptHashData(handles.hash,const_cast<PUCHAR>(bytes.data()),count,0);first();need(feed>=0,"Original FontShader hash feed failed");bytes=bytes.subspan(count);}
 first();const auto finish=BCryptFinishHash(handles.hash,digest.data(),static_cast<ULONG>(digest.size()),0);first();need(finish>=0,"Original FontShader hash finalization failed");
 const bool closed=handles.close();first();need(closed,"Original FontShader hash cleanup failed");
 std::array<char,64>result{};constexpr char hex[]="0123456789abcdef";for(std::size_t k=0;k<digest.size();++k){result[k*2]=hex[digest[k]>>4];result[k*2+1]=hex[digest[k]&15];}first();return result;
}
std::span<const std::uint8_t> font_original_embedded(
 std::span<const std::uint8_t>b,std::uint32_t va,std::uint32_t count){
 const auto word=[&](std::size_t at){need(at<=b.size()&&b.size()-at>=4,"Truncated Original FontShader PE");std::uint32_t v;std::memcpy(&v,b.data()+at,4);return v;};
 const auto half=[&](std::size_t at){need(at<=b.size()&&b.size()-at>=2,"Truncated Original FontShader PE");std::uint16_t v;std::memcpy(&v,b.data()+at,2);return v;};
 const std::size_t pe=word(0x3c);need(word(pe)==0x4550&&half(pe+24)==0x10b,"Foreign Original FontShader PE format");
 const std::uint64_t base=word(pe+24+28);const auto sections=half(pe+6),optional=half(pe+20);need(va>=base,"Original FontShader VA precedes image");
 const std::uint64_t wanted=va-base;std::size_t offset=0;unsigned matches=0;
 for(unsigned k=0;k<sections;++k){const std::size_t row=pe+24+optional+static_cast<std::size_t>(k)*40;
  const std::uint64_t rva=word(row+12),size=word(row+16),position=word(row+20);
  if(wanted>=rva&&wanted-rva<=size&&count<=size-(wanted-rva)){const auto actual=position+wanted-rva;
   need(actual<=b.size()&&count<=b.size()-actual,"Original FontShader PE range exceeds held bytes");offset=static_cast<std::size_t>(actual);++matches;}}
 need(matches==1,"Missing/ambiguous Original FontShader PE range");return b.subspan(offset,count);
}
}
struct GlBnwFontShader::OriginalProgramSource::Storage final {
 const std::thread::id creator=std::this_thread::get_id();std::atomic<bool>alive{true};
 const ui::detail::BnwLabelPaintPositionedData::Guard data;
 const std::shared_ptr<const assets::CivisResourceBankOwner>bank;
 const std::shared_ptr<const assets::CivisResourceBankCapability> capability;
 std::array<std::uint32_t,123>vertex{};std::array<std::uint32_t,127>fragment{};std::array<std::uint32_t,88>fragment1{};
 std::array<char,sizeof(vs)>translated_vertex{};std::array<char,sizeof(ps)>translated_fragment{};std::array<char,sizeof(font_original_ps1)>translated_fragment1{};
 Storage(ui::detail::BnwLabelPaintPositionedData::Guard d,std::shared_ptr<const assets::CivisResourceBankOwner>b,std::shared_ptr<const assets::CivisResourceBankCapability> c):data(std::move(d)),bank(std::move(b)),capability(std::move(c)){}
 void first()const{
  need(creator==std::this_thread::get_id()&&alive.load(),"Original FontShader program source retired/foreign");
  data.require_current_without_callbacks();capability->require_current();bank->require_current();data.require_current_without_callbacks();
 }
 void full(const runtime::BnwWorldViewSourceOperationLoan&operation)const{first();data.require_source_owned(operation);first();}
};
GlBnwFontShader::OriginalProgramSource::OriginalProgramSource(std::shared_ptr<Storage>s):storage_(std::move(s)){}
GlBnwFontShader::OriginalProgramSource::~OriginalProgramSource(){storage_->alive.store(false);}
GlBnwFontShader::OriginalProgramSource::Guard::Guard(std::shared_ptr<Storage>s):storage_(std::move(s)){}
GlBnwFontShader::OriginalProgramSource::Guard GlBnwFontShader::OriginalProgramSource::capture_guard_without_callbacks()const{
 const auto actual=storage_;actual->first();return Guard(actual);
}
void GlBnwFontShader::OriginalProgramSource::Guard::require_current_without_callbacks()const{storage_->first();}
void GlBnwFontShader::OriginalProgramSource::Guard::require_source_owned(const runtime::BnwWorldViewSourceOperationLoan&operation)const{const auto actual=storage_;actual->full(operation);}
GlBnwFontShader::OriginalProgramSource::Guard::NativeProgramPair
GlBnwFontShader::OriginalProgramSource::Guard::native_selector_without_callbacks(unsigned selector)const{
 const auto actual=storage_;actual->first();need(selector==1||selector==4,"Unsupported Original FontShader native selector");
 const auto fragment=selector==1?std::span<const std::uint32_t>(actual->fragment1):std::span<const std::uint32_t>(actual->fragment);
 const auto translated=selector==1?std::string_view(actual->translated_fragment1.data(),actual->translated_fragment1.size()-1):std::string_view(actual->translated_fragment.data(),actual->translated_fragment.size()-1);
 const NativeProgramPair result{actual->vertex,fragment,{actual->translated_vertex.data(),actual->translated_vertex.size()-1},translated,selector};actual->first();return result;
}
std::span<const std::uint32_t>GlBnwFontShader::OriginalProgramSource::Guard::original_vertex_tokens_without_callbacks()const{storage_->first();return storage_->vertex;}
std::span<const std::uint32_t>GlBnwFontShader::OriginalProgramSource::Guard::original_fragment_tokens_without_callbacks()const{storage_->first();return storage_->fragment;}
std::string_view GlBnwFontShader::OriginalProgramSource::Guard::translated_vertex_glsl_without_callbacks()const{storage_->first();return {storage_->translated_vertex.data(),storage_->translated_vertex.size()-1};}
std::string_view GlBnwFontShader::OriginalProgramSource::Guard::translated_fragment_glsl_without_callbacks()const{storage_->first();return {storage_->translated_fragment.data(),storage_->translated_fragment.size()-1};}
std::unique_ptr<GlBnwFontShader::OriginalProgramSource>GlBnwFontShader::original_program_source_owned(
 const runtime::BnwWorldViewSourceOperationLoan&operation,ui::detail::BnwLabelPaintPositionedData::Guard data,
 std::shared_ptr<const assets::CivisResourceBankOwner>borrowed_bank){
 // Parent captured DATA Guard BEFORE its callbackful selected-bank supplier.
 // No DATA facade is read here or after any allocation/Source callback.
 data.require_current_without_callbacks();
 need(bool(borrowed_bank),"Original FontShader canonical bank absent");const auto bank=borrowed_bank->retain_owner();data.require_current_without_callbacks();
 need(bank.get()==borrowed_bank.get()&&!bank.owner_before(borrowed_bank)&&!borrowed_bank.owner_before(bank),"Original FontShader foreign bank control");
 const auto initial_first=[&]{data.require_current_without_callbacks();bank->require_current();data.require_current_without_callbacks();};
 initial_first();auto raw_capability=std::unique_ptr<const assets::CivisResourceBankCapability>(
  new assets::CivisResourceBankCapability(bank->capability()));initial_first();raw_capability->require_current();initial_first();
 std::shared_ptr<const assets::CivisResourceBankCapability>capability(std::move(raw_capability));initial_first();capability->require_current();initial_first();
 const auto first=[&]{data.require_current_without_callbacks();capability->require_current();bank->require_current();data.require_current_without_callbacks();};
 const auto full=[&]{first();data.require_source_owned(operation);first();};
 // Stack copy of native path before any allocator, Source callback or native
 // call. No borrowed path/native-string reference survives this block.
 std::array<wchar_t,32768>path{};
 {first();const auto&native=bank->original_root_for_font_program_without_callbacks().native();
  constexpr wchar_t leaf[]=L"\\CivilizationV.exe";need(!native.empty(),"Original FontShader bank root absent");
  const bool extended=native.size()>=4&&native[0]==L'\\'&&native[1]==L'\\'&&native[2]==L'?'&&native[3]==L'\\';
  const bool unc=!extended&&native.size()>=2&&native[0]==L'\\'&&native[1]==L'\\';
  std::size_t at=0,start=0;if(!extended){if(unc){constexpr wchar_t prefix[]=L"\\\\?\\UNC\\";std::copy_n(prefix,8,path.data());at=8;start=2;}
   else{need(native.size()>=3&&native[1]==L':'&&(native[2]==L'\\'||native[2]==L'/'),"Original FontShader bank root is not absolute");constexpr wchar_t prefix[]=L"\\\\?\\";std::copy_n(prefix,4,path.data());at=4;}}
  need(native.size()-start<=path.size()-at-std::size(leaf),"Original FontShader path exceeds fixed native transport");
  for(std::size_t k=start;k<native.size();++k)path[at++]=native[k]==L'/'?L'\\':native[k];
  if(at&&path[at-1]==L'\\')--at;std::copy(std::begin(leaf),std::end(leaf),path.data()+at);first();}
 std::unique_ptr<OriginalProgramSource>result;
 {full();FontOriginalFile file;first();file.handle=CreateFileW(path.data(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL|FILE_FLAG_SEQUENTIAL_SCAN,nullptr);first();
  need(file.handle!=INVALID_HANDLE_VALUE,"Read-only Original FontShader executable unavailable");
  LARGE_INTEGER length{};first();const auto sized=GetFileSizeEx(file.handle,&length);first();need(sized&&length.QuadPart==10602040,"Changed Original FontShader executable size");
  constexpr std::size_t bytes_count=10602040;first();auto bytes=std::make_unique<std::uint8_t[]>(bytes_count);first();
  std::size_t offset=0;while(offset<bytes_count){DWORD count=0;const auto wanted=static_cast<DWORD>(std::min<std::size_t>(65536,bytes_count-offset));
   first();const auto read=ReadFile(file.handle,bytes.get()+offset,wanted,&count,nullptr);first();need(read&&count>0&&count<=wanted,"Truncated Original FontShader executable read");offset+=count;}
  std::uint8_t tail=0;DWORD extra=0;first();const auto ended=ReadFile(file.handle,&tail,1,&extra,nullptr);first();need(ended&&extra==0,"Original FontShader executable grew during read");
  const auto all=std::span<const std::uint8_t>(bytes.get(),bytes_count);const auto exe_hash=font_original_sha256(all,first);first();
  need(std::string_view(exe_hash.data(),exe_hash.size())==bnw_font_shader_original_exe_sha256,"Changed Original FontShader executable bytes");
  const auto vertex=font_original_embedded(all,0x5d7fd08,492),fragment=font_original_embedded(all,0x5d81868,508);first();
  const auto vertex_hash=font_original_sha256(vertex,first);first();const auto fragment_hash=font_original_sha256(fragment,first);first();
  need(std::string_view(vertex_hash.data(),vertex_hash.size())==bnw_font_shader_original_vs_sha256&&std::string_view(fragment_hash.data(),fragment_hash.size())==bnw_font_shader_original_ps_sha256,"Changed Original FontShader row4 token pair");
  const auto fragment1=font_original_embedded(all,0x5d809f0,352);first();const auto fragment1_hash=font_original_sha256(fragment1,first);first();
  need(std::string_view(fragment1_hash.data(),fragment1_hash.size())==font_original_ps1_sha256,"Changed Original FontShader selector1 token pair");
  const bool closed=file.close();first();need(closed,"Original FontShader executable cleanup failed");
  first();auto raw=std::unique_ptr<OriginalProgramSource::Storage>(new OriginalProgramSource::Storage(data,bank,capability));first();
  static_assert(std::endian::native==std::endian::little);std::memcpy(raw->vertex.data(),vertex.data(),vertex.size());std::memcpy(raw->fragment.data(),fragment.data(),fragment.size());
  std::memcpy(raw->fragment1.data(),fragment1.data(),fragment1.size());
  need(raw->vertex.front()==0xfffe0300&&raw->fragment.front()==0xffff0300&&raw->fragment1.front()==0xffff0300&&raw->vertex.back()==0xffff&&raw->fragment.back()==0xffff&&raw->fragment1.back()==0xffff,"Original FontShader version/end tokens differ");
  std::copy(std::begin(vs),std::end(vs),raw->translated_vertex.begin());std::copy(std::begin(ps),std::end(ps),raw->translated_fragment.begin());
  std::copy(std::begin(font_original_ps1),std::end(font_original_ps1),raw->translated_fragment1.begin());first();
  std::shared_ptr<OriginalProgramSource::Storage>storage(std::move(raw));first();storage->first();
  result=std::unique_ptr<OriginalProgramSource>(new OriginalProgramSource(std::move(storage)));first();
 }
 // File/hash handles, held executable, PE spans and allocation temporaries are
 // gone before final FULL/PURE. The private result retains immutable token/
 // translated bytes; it does not attest live filesystem or a material draw.
 full();result->storage_->first();return result;
}

// Physical program resources only. Matrices, GUI caller values, sampler policy,
// target, draw and actual native selector invocation remain separate suppliers.
std::shared_ptr<GlBnwFontShader>GlBnwFontShader::create_from_original_program_owned(
 std::shared_ptr<const detail::BnwRendererContextPin>pin,std::shared_ptr<GlBnwSystemFontAtlas>atlas,
 OriginalProgramSource::Guard source,unsigned selector,GlRenderApi loaded_render){
 need(pin&&atlas,"Original FontShader physical factory requires SAME Pin/Atlas");
 const auto first=[&]{source.require_current_without_callbacks();pin->first_without_callbacks();
  atlas->require_same_owned_context_pin_no_current(pin);source.require_current_without_callbacks();pin->first_without_callbacks();};
 first();const auto pair=source.native_selector_without_callbacks(selector);first();
 need(pair.translated_vertex.size()<=static_cast<std::size_t>(std::numeric_limits<GLint>::max())&&
  pair.translated_fragment.size()<=static_cast<std::size_t>(std::numeric_limits<GLint>::max()),"Unbounded Original FontShader translated source length");
 // Loaded dispatch and immutable creator identity are copied before allocation.
 // No public Context API/proc/current/check path or Context clone is used.
 first();const auto*loaded_api=detail::BnwRendererContextPin::NormalAccess::api_current_without_switch_owned(pin);first();
 const GlApi api=*loaded_api;const auto identity=pin->identity_;first();
 need(api.GetError&&api.GetGraphicsResetStatus,"Original FontShader normal error/reset dispatch absent");
 std::shared_ptr<GlBnwFontShader>result;
 {
  first();auto i=std::make_unique<Impl>();first();
  i->context_pin=pin;i->atlas=atlas;i->identity=identity;
  i->original_program_guard.emplace(source);i->original_program_selector=selector;
  i->owned_normal_api.emplace(api);i->api=&*i->owned_normal_api;first();
  first();i->render=std::unique_ptr<GlRenderApi>(new GlRenderApi(loaded_render));first();
  const auto&r=*i->render;
  need(r.CreateProgram&&r.CreateShader&&r.ShaderSource&&r.CompileShader&&r.GetShaderiv&&r.GetShaderInfoLog&&
   r.AttachShader&&r.LinkProgram&&r.GetProgramiv&&r.GetProgramInfoLog&&r.DeleteShader&&r.DeleteProgram&&
   r.CreateVertexArrays&&r.CreateBuffers&&r.CreateSamplers&&r.DeleteVertexArrays&&r.DeleteBuffers&&r.DeleteSamplers&&
   r.VertexArrayVertexBuffer&&r.VertexArrayElementBuffer&&r.EnableVertexArrayAttrib&&r.VertexArrayAttribBinding&&r.VertexArrayAttribFormat,
   "Original FontShader loaded dispatch incomplete");
  // Allocation precedes names; actual GL outputs go DIRECTLY to this node.
  first();i->retirement_node=detail::BnwOwnedGlRetirementNode::shader_resources_owned(pin,api,r);first();
  auto&names=i->retirement_node->shader_resources_storage_without_callbacks();
  const auto require_native_current=[&]{first();const auto*current=detail::BnwRendererContextPin::NormalAccess::api_current_without_switch_owned(pin);first();
   need(current&&current->GetError==api.GetError&&current->GetGraphicsResetStatus==api.GetGraphicsResetStatus,
    "Original FontShader loaded normal dispatch changed");};
  const auto effect_fence=[&]{
   require_native_current();const auto reset=api.GetGraphicsResetStatus();require_native_current();
   need(reset==GL_NO_ERROR,"Original FontShader context reset");
   require_native_current();const auto error=api.GetError();require_native_current();
   need(error==GL_NO_ERROR,"Original FontShader native GL error");
   require_native_current();};
  effect_fence();names.program=r.CreateProgram();effect_fence();need(names.program!=0,"Original FontShader program creation failed");
  const std::array<GLenum,2>types{GL_VERTEX_SHADER,GL_FRAGMENT_SHADER};
  const std::array<std::string_view,2>texts{pair.translated_vertex,pair.translated_fragment};
  for(unsigned index=0;index<types.size();++index){
   effect_fence();names.stages[index]=r.CreateShader(types[index]);effect_fence();need(names.stages[index]!=0,"Original FontShader stage creation failed");
   const auto stage=names.stages[index];const auto*text=texts[index].data();const auto length=static_cast<GLint>(texts[index].size());
   effect_fence();r.ShaderSource(stage,1,&text,&length);effect_fence();
   effect_fence();r.CompileShader(stage);effect_fence();
   GLint compiled=0;effect_fence();r.GetShaderiv(stage,GL_COMPILE_STATUS,&compiled);effect_fence();
   if(!compiled){std::array<char,8192>log{};effect_fence();r.GetShaderInfoLog(stage,static_cast<GLsizei>(log.size()),nullptr,log.data());effect_fence();log.back()='\0';throw std::runtime_error(log.data());}
   effect_fence();r.AttachShader(names.program,stage);effect_fence();
  }
  effect_fence();r.LinkProgram(names.program);effect_fence();GLint linked=0;
  effect_fence();r.GetProgramiv(names.program,GL_LINK_STATUS,&linked);effect_fence();
  if(!linked){std::array<char,8192>log{};effect_fence();r.GetProgramInfoLog(names.program,static_cast<GLsizei>(log.size()),nullptr,log.data());effect_fence();log.back()='\0';throw std::runtime_error(log.data());}
  // Attached stages remain node-owned until complete program retirement. No
  // unchecked deletion/clearing of a name occurs after a GL/source callback.
  effect_fence();r.CreateVertexArrays(1,&names.vao);effect_fence();
  effect_fence();r.CreateBuffers(1,&names.vertices);effect_fence();
  effect_fence();r.CreateBuffers(1,&names.indices);effect_fence();
  effect_fence();r.CreateSamplers(1,&names.sampler);effect_fence();
  need(names.vao&&names.vertices&&names.indices&&names.sampler,"Original FontShader geometry/resource creation failed");
  effect_fence();r.VertexArrayVertexBuffer(names.vao,0,names.vertices,0,24);effect_fence();
  effect_fence();r.VertexArrayElementBuffer(names.vao,names.indices);effect_fence();
  for(unsigned k=0;k<3;++k){
   effect_fence();r.EnableVertexArrayAttrib(names.vao,k);effect_fence();
   effect_fence();r.VertexArrayAttribBinding(names.vao,k,0);effect_fence();
   effect_fence();r.VertexArrayAttribFormat(names.vao,k,k==0?3:k==1?2:4,k==2?GL_UNSIGNED_BYTE:GL_FLOAT,k==2?GL_TRUE:GL_FALSE,k==0?0:k==1?16:12);effect_fence();
  }
  // No sampler parameters, uniforms, target bind or draw are issued. Its
  // policy/default state has no native admission until a real supplier binds it.
  i->shader=names.program;i->vao=names.vao;i->vertices=names.vertices;i->indices=names.indices;i->sampler=names.sampler;first();
  first();auto facade=std::unique_ptr<GlBnwFontShader>(new GlBnwFontShader(std::move(i)));first();
  result=std::shared_ptr<GlBnwFontShader>(std::move(facade));first();result->impl_->original_program_first();
 }
 // Factory-local allocation/callback targets are gone before final PURE.
 // Root wraps this physical-only factory with the genuine operation FULL gate.
 first();result->impl_->original_program_first();return result;
}
// Native record9 policy only. Actual GUI selection, material, texture binding,
// uniforms, target and draw remain separate closed suppliers.
void GlBnwFontShader::initialize_native_font_sampler_policy_owned(std::shared_ptr<GlBnwFontShader>shader){
 // Retain physical facade by VALUE before native observers; its unique Impl
 // remains physical custody only. Independent Source/DATA/Paint/Pin gates
 // decide authority and cannot be renewed by this strong Shader slot.
 const auto retained=std::move(shader);
 need(retained&&retained->impl_,"Original FontShader sampler initializer requires physical shader");
 auto*const actual=retained->impl_.get();actual->original_program_first();
 if(actual->busy){actual->failed=true;throw std::runtime_error("Reentrant Original FontShader sampler initializer");}
 need(!actual->native_font_sampler_policy_applied,"Original FontShader sampler policy already initialized");
 actual->busy=true;
 try{
  {
   // All operation targets and loaded dispatch are snapshotted before effects.
   const auto source=*actual->original_program_guard;
   const auto pin=actual->context_pin;const auto atlas=actual->atlas;
   auto*const node=actual->retirement_node.get();
   need(node&&actual->render,"Original FontShader sampler resource custody absent");
   const GlApi api=*actual->owned_normal_api;const GlRenderApi render=*actual->render;
   const auto sampler=node->shader_resources_storage_without_callbacks().sampler;
   need(sampler&&sampler==actual->sampler,"Original FontShader sampler custody identity changed");
   const auto first=[&]{
    source.require_current_without_callbacks();actual->original_program_first();
    need(actual->busy&&actual->context_pin.get()==pin.get()&&
     !actual->context_pin.owner_before(pin)&&!pin.owner_before(actual->context_pin)&&
     actual->atlas.get()==atlas.get()&&!actual->atlas.owner_before(atlas)&&!atlas.owner_before(actual->atlas)&&
     actual->retirement_node.get()==node&&actual->sampler==sampler&&
     node->shader_resources_storage_without_callbacks().sampler==sampler,
     "Original FontShader sampler physical custody changed");
    source.require_current_without_callbacks();pin->first_without_callbacks();
   };
   const auto current=[&]{
    first();const auto*loaded=detail::BnwRendererContextPin::NormalAccess::api_current_without_switch_owned(pin);first();
    need(loaded&&loaded->GetError==api.GetError&&loaded->GetGraphicsResetStatus==api.GetGraphicsResetStatus&&
     loaded->GetIntegerv==api.GetIntegerv&&loaded->GetStringi==api.GetStringi,
     "Original FontShader sampler normal dispatch changed");
   };
   const auto effect_fence=[&]{
    current();const auto reset=api.GetGraphicsResetStatus();current();
    need(reset==GL_NO_ERROR,"Original FontShader sampler context reset");
    current();const auto error=api.GetError();current();
    need(error==GL_NO_ERROR,"Original FontShader sampler native GL error");current();
   };
   first();need(api.GetError&&api.GetGraphicsResetStatus&&api.GetIntegerv&&api.GetStringi&&
    render.SamplerParameteri&&render.SamplerParameterf,"Original FontShader sampler loaded dispatch incomplete");
   // Index9 is established by original688E02/688E28 and8E23CE. It is not
   // inferred from vertex format9 or supplied by a public caller.
   const auto record=resolve_bnw_native_sampler_record(9);first();
   need(record==BnwWarriorSamplerRecord{1,1,1,1,1,1,1,false},"Changed native FontShader sampler9 source contract");
   // Establish the actual accepted anisotropy pname, without Context/proc APIs.
   GLint major=0,minor=0;effect_fence();api.GetIntegerv(GL_MAJOR_VERSION,&major);effect_fence();
   effect_fence();api.GetIntegerv(GL_MINOR_VERSION,&minor);effect_fence();
   bool anisotropy=major>4||(major==4&&minor>=6);
   if(!anisotropy){
    GLint count=0;effect_fence();api.GetIntegerv(GL_NUM_EXTENSIONS,&count);effect_fence();
    need(count>=0&&count<=65536,"Unbounded Original FontShader extension registry");
    for(GLint index=0;index<count&&!anisotropy;++index){
     effect_fence();const auto*extension=api.GetStringi(GL_EXTENSIONS,static_cast<GLuint>(index));effect_fence();
     need(extension,"Original FontShader extension entry absent");first();
     const auto*name=reinterpret_cast<const char*>(extension);
     anisotropy=std::strcmp(name,"GL_ARB_texture_filter_anisotropic")==0||std::strcmp(name,"GL_EXT_texture_filter_anisotropic")==0;
     first();
    }
   }
   // Each named parameter effect has normal-current/error/reset PRE and POST.
   for(const auto field:{GL_TEXTURE_WRAP_S,GL_TEXTURE_WRAP_T,GL_TEXTURE_WRAP_R}){
    effect_fence();render.SamplerParameteri(sampler,field,GL_REPEAT);effect_fence();
   }
   effect_fence();render.SamplerParameteri(sampler,GL_TEXTURE_MAG_FILTER,GL_NEAREST);effect_fence();
   effect_fence();render.SamplerParameteri(sampler,GL_TEXTURE_MIN_FILTER,GL_NEAREST_MIPMAP_NEAREST);effect_fence();
   if(anisotropy){effect_fence();render.SamplerParameterf(sampler,0x84FE,1.0f);effect_fence();}
   // No anisotropy extension/core support means ordinary isotropic sampling;
   // no unsupported pname is issued. sRGBfalse is realized by typed GL_R8
   // planes, not a generic sampler flag; that separate binding is not granted.
   effect_fence();
  }
  // Captured operation locals are gone before the final independent gate.
  actual->original_program_first();actual->native_font_sampler_policy_applied=true;
  actual->busy=false;actual->original_program_first();
 }catch(...){actual->native_font_sampler_policy_applied=false;actual->failed=true;actual->busy=false;throw;}
}


// Physical code provenance/custody only. No old DATA/Paint/Atlas is retained.
struct GlBnwFontShader::OriginalProgramLibrary::Storage final {
 // Private canonical CODE-owned dynamic descriptor, native wrapper+14.
 // This is distinct from Root shared descriptor material+10 and Queue+6F0.
 struct RendererOwnDescriptor final {
  const std::array<std::uint32_t,8> constructor_arguments={0u,144u,15u,3u,3u,0u,6u,0u};
  volatile std::uint8_t field_2c;
  std::uint64_t executed_zero_stores=0; // Later publication bookkeeping only.
  RendererOwnDescriptor()noexcept:field_2c(0u){} // Actual 8EF5D2 constructor byte store.
  RendererOwnDescriptor(const RendererOwnDescriptor&)=delete;
  RendererOwnDescriptor&operator=(const RendererOwnDescriptor&)=delete;
 };
 // Native material8E6DF0 normalized control; instantiated only after actual
 // canonical GUI shared-resource attachment. No original pool handle is minted.
 struct RendererMaterialControl final {
  using Shared=::civ5::ui::detail::BnwWindowedRendererSharedDescriptorIdentity;
  const std::array<std::uint32_t,3> counts={1u,0u,0u};
  const std::uint32_t family=0x2bu;
  const Shared shared_10;
  const RendererOwnDescriptor*const own_28;
  RendererMaterialControl(const Shared&shared,const RendererOwnDescriptor&own)noexcept
   :shared_10(shared),own_28(&own){}
  // Count-zero material lanes18/20 have no invented zero/value representation.
 };
 using Node=detail::BnwOwnedGlRetirementNode;
 const std::thread::id creator=std::this_thread::get_id();
 const std::shared_ptr<const assets::CivisResourceBankOwner> bank;
 const std::shared_ptr<const assets::CivisResourceBankCapability> capability;
 const std::shared_ptr<const detail::BnwRendererContextPin> pin;
 const GlApi api;
 const GlRenderApi render;
 const std::array<std::uint32_t,123> vertex;
 const std::array<std::uint32_t,88> fragment1;
 const std::array<std::uint32_t,127> fragment4;
 const std::array<GLuint,2> uniform_programs;
 const std::array<OriginalProgramUniformLocations,2> uniform_locations;
 RendererOwnDescriptor renderer_own_descriptor;
 std::optional<RendererMaterialControl> renderer_material_control;
 // Constructor zero is not 8EEFE0 publication, material, queue or draw readiness.
 bool submit_busy=false,submit_failed=false; // Creator-only; physical shared CODE exclusion, not authority.
 const DrawRecordState* submit_owner=nullptr; // Exact exclusive staged-record lease identity.
 const GpuStagingLease::Impl*queued_submit_owner=nullptr; // Actual selected-entry owner; never a record ID.
 std::array<Node::Owner,2> nodes;
 Storage(const OriginalProgramSource::Storage& s,std::shared_ptr<const detail::BnwRendererContextPin> token,
  GlApi a,GlRenderApi r,std::array<GLuint,2> programs,std::array<OriginalProgramUniformLocations,2> locations)
  :bank(s.bank),capability(s.capability),pin(std::move(token)),api(a),render(r),
  vertex(s.vertex),fragment1(s.fragment1),fragment4(s.fragment),uniform_programs(programs),uniform_locations(locations),
   renderer_own_descriptor(){}
 bool same_key(const std::shared_ptr<const assets::CivisResourceBankOwner>& b,
  const std::shared_ptr<const detail::BnwRendererContextPin>& p)const noexcept {
  return bank.get()==b.get()&&!bank.owner_before(b)&&!b.owner_before(bank)&&
   pin.get()==p.get()&&!pin.owner_before(p)&&!p.owner_before(pin);
 }
 void first()const {
  need(creator==std::this_thread::get_id()&&bank&&capability&&pin,"Original code library physical custody foreign/absent");
  bank->require_current();capability->require_current();pin->first_without_callbacks();
  const auto*current=detail::BnwRendererContextPin::NormalAccess::api_current_without_switch_owned(pin);
  bank->require_current();capability->require_current();pin->first_without_callbacks();
  need(current&&current->GetError==api.GetError&&current->GetGraphicsResetStatus==api.GetGraphicsResetStatus,
   "Original code library actual normal dispatch changed");
  for(const auto& node:nodes){
   need(bool(node),"Original code library resource custody not complete");
   const auto& names=node->shader_resources_storage_without_callbacks();
   need(names.program&&names.stages[0]&&names.stages[1]&&names.vao&&names.vertices&&names.indices&&names.sampler,
    "Original code library resource names incomplete");
  }
  for(unsigned index=0;index<nodes.size();++index){
   const auto& locations=uniform_locations[index];
   need(nodes[index]->shader_resources_storage_without_callbacks().program==uniform_programs[index]&&uniform_programs[index]&&
    locations.rows>=0&&locations.translation>=0&&locations.depth>=0&&locations.parent_alpha>=0&&
    locations.block_texture>=0&&locations.allow_color>=0&&locations.clip_fields.has_value()==(index==1),
    "Original code library cached uniform/program identity incomplete");
   if(locations.clip_fields){const auto& clip=*locations.clip_fields;
    need(clip.clip>=0&&clip.fragment_bias>=0&&clip.native_top_at_row_zero>=0&&clip.target_height>=0,
     "Original code library selector4 cached clip uniform absent");}
  }
  // Source3 already validated exact entire exe and all token arrays. These
  // immutable copies attest origin at creation, not current file contents.
  bank->require_current();capability->require_current();pin->first_without_callbacks();
 }
};
GlBnwFontShader::OriginalProgramLibrary::OriginalProgramLibrary(std::shared_ptr<Storage> s):storage_(std::move(s)){}
GlBnwFontShader::OriginalProgramLibrary::~OriginalProgramLibrary()=default;
bool GlBnwFontShader::OriginalProgramLibrary::same_key_without_callbacks(
 const std::shared_ptr<const assets::CivisResourceBankOwner>& b,
 const std::shared_ptr<const detail::BnwRendererContextPin>& p)const noexcept{return storage_->same_key(b,p);}
struct GlBnwFontShader::OriginalProgramUse::Storage final {
 using Data=ui::detail::BnwLabelPaintPositionedData;
 const std::thread::id creator=std::this_thread::get_id();std::atomic<bool> alive{true};
 const Data::Guard data;
 const ui::detail::BnwLabelPaintTextLoan::Guard paint;
 const ui::detail::BnwLabelPaintPreparedFlushLoan::Guard early;
 const std::shared_ptr<OriginalProgramLibrary::Storage> code;
 const std::shared_ptr<GlBnwSystemFontAtlas> atlas;
 const std::shared_ptr<const detail::BnwRendererContextPin> pin;
 Storage(Data::Guard d,std::shared_ptr<OriginalProgramLibrary::Storage> c,std::shared_ptr<GlBnwSystemFontAtlas> a,
  std::shared_ptr<const detail::BnwRendererContextPin> p):data(std::move(d)),
  paint(data.captured_paint_guard_without_callbacks()),early(data.captured_early_flush_guard_without_callbacks()),
  code(std::move(c)),atlas(std::move(a)),pin(std::move(p)){}
 void first()const {
  need(creator==std::this_thread::get_id()&&alive.load(),"Original program use lexical DATA retired/foreign");
  data.require_current_without_callbacks();paint.require_current_without_callbacks();early.require_current_without_callbacks();
  data.require_same_paint_guard_without_callbacks(paint);early.require_same_paint_guard_without_callbacks(paint);
  need(code&&atlas&&pin&&code->same_key(code->bank,pin),"Original program use code/SAME Pin relation changed");
  early.accepted_pair_without_callbacks(*atlas);atlas->require_same_owned_context_pin_no_current(pin);
  data.require_current_without_callbacks();paint.require_current_without_callbacks();early.require_current_without_callbacks();
  code->first();
  data.require_current_without_callbacks();paint.require_current_without_callbacks();early.require_current_without_callbacks();
  need(alive.load(),"Original program use retired at POST");
 }
 void full(const runtime::BnwWorldViewSourceOperationLoan& operation)const {
  first();data.require_source_owned(operation);first();paint.require_source_owned(operation);first();early.require_source_owned(operation);first();
 }
};
GlBnwFontShader::OriginalProgramUse::OriginalProgramUse(std::shared_ptr<Storage> s):storage_(std::move(s)){}
GlBnwFontShader::OriginalProgramUse::~OriginalProgramUse(){storage_->alive.store(false);}
void GlBnwFontShader::OriginalProgramUse::require_current_without_callbacks()const{const auto saved=storage_;saved->first();}
void GlBnwFontShader::OriginalProgramUse::require_source_owned(const runtime::BnwWorldViewSourceOperationLoan& operation)const{const auto saved=storage_;saved->full(operation);}
GlBnwFontShader::OriginalProgramUniformLocations
GlBnwFontShader::OriginalProgramUse::uniform_locations_without_callbacks(unsigned native_selector)const{
 const auto saved=storage_;saved->first();
 need(native_selector==1||native_selector==4,"Original program location request has foreign CODE selector");
 const auto code=saved->code;const auto result=code->uniform_locations[native_selector==1?0:1];
 saved->first();return result;
}
std::shared_ptr<GlBnwFontShader::OriginalProgramLibrary>GlBnwFontShader::create_original_program_library_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,ui::detail::BnwLabelPaintPositionedData::Guard data,
 std::shared_ptr<const assets::CivisResourceBankOwner> bank,std::shared_ptr<GlBnwSystemFontAtlas> atlas,
 std::shared_ptr<const detail::BnwRendererContextPin> pin,GlRenderApi render){
 const auto paint=data.captured_paint_guard_without_callbacks();const auto early=data.captured_early_flush_guard_without_callbacks();
 const auto first=[&]{data.require_current_without_callbacks();paint.require_current_without_callbacks();early.require_current_without_callbacks();
  data.require_same_paint_guard_without_callbacks(paint);early.require_same_paint_guard_without_callbacks(paint);
  need(bank&&atlas&&pin,"Original code library requires canonical Bank/Atlas/SAME Pin");bank->require_current();pin->first_without_callbacks();
  early.accepted_pair_without_callbacks(*atlas);atlas->require_same_owned_context_pin_no_current(pin);data.require_current_without_callbacks();};
 const auto full=[&]{first();data.require_source_owned(operation);first();};
 first();std::shared_ptr<OriginalProgramLibrary> result;
 {
  // Sole actual source reader on a cache miss; both physical variants share it.
  full();auto source=original_program_source_owned(operation,data,bank);first();
  const auto guard=source->capture_guard_without_callbacks();first();
  std::array<std::shared_ptr<GlBnwFontShader>,2> shaders;
  for(unsigned index=0;index<2;++index){
   full();shaders[index]=create_from_original_program_owned(pin,atlas,guard,index==0?1u:4u,render);first();
   full();initialize_native_font_sampler_policy_owned(shaders[index]);first();full();
  }
  // Query linked physical programs once on the cold library path. Uniform
  // values/current GUI selector/material admission are never created here.
  std::array<GLuint,2> uniform_programs{};
  std::array<OriginalProgramUniformLocations,2> uniform_locations{};
  for(unsigned index=0;index<shaders.size();++index){
   full();guard.require_current_without_callbacks();first();
   const auto shader=shaders[index]; // Actual physical facade BYVALUE.
   auto*const actual=shader->impl_.get();actual->original_program_first();first();
   if(actual->busy){actual->failed=true;throw std::runtime_error("Reentrant Original program uniform query");}
   auto*const node=actual->retirement_node.get();
   need(node&&actual->owned_normal_api&&actual->render&&actual->native_font_sampler_policy_applied,
    "Original program uniform query physical custody incomplete");
   const auto program=node->shader_resources_storage_without_callbacks().program;
   const auto api=*actual->owned_normal_api;const auto loaded_render=*actual->render;
   need(program&&actual->shader==program&&loaded_render.GetUniformLocation&&api.GetError&&api.GetGraphicsResetStatus,
    "Original program uniform query linked program/dispatch absent");
   actual->busy=true;
   struct QueryExit final {Impl*actual;bool committed=false;~QueryExit(){actual->busy=false;if(!committed)actual->failed=true;}};
   QueryExit query_exit{actual};
   const auto query_first=[&]{guard.require_current_without_callbacks();first();actual->original_program_first();
    need(actual->busy&&actual->retirement_node.get()==node&&actual->shader==program&&
     node->shader_resources_storage_without_callbacks().program==program&&
     actual->original_program_selector==(index==0?1u:4u)&&actual->native_font_sampler_policy_applied,
     "Original program uniform query SAME physical program/selector changed");
    guard.require_current_without_callbacks();first();};
   const auto current=[&]{query_first();const auto*normal=detail::BnwRendererContextPin::NormalAccess::api_current_without_switch_owned(pin);query_first();
    need(normal&&normal->GetError==api.GetError&&normal->GetGraphicsResetStatus==api.GetGraphicsResetStatus,
     "Original program uniform query loaded normal dispatch changed");};
   const auto effect_fence=[&]{current();const auto reset=api.GetGraphicsResetStatus();current();
    need(reset==GL_NO_ERROR,"Original program uniform query context reset");
    current();const auto error=api.GetError();current();
    need(error==GL_NO_ERROR,"Original program uniform query native GL error");current();};
   const auto query=[&](const char* literal){effect_fence();
    const auto location=loaded_render.GetUniformLocation(program,literal);effect_fence();
    need(location>=0,"Original linked program required uniform inactive/absent");query_first();return location;};
   auto& locations=uniform_locations[index];
   locations.rows=query("uRows[0]");locations.translation=query("uTranslation");
   locations.depth=query("uDepth");locations.parent_alpha=query("uParentAlpha");
   locations.block_texture=query("uBlockTexture");locations.allow_color=query("uAllowColor");
   if(index==1){locations.clip_fields=OriginalProgramUniformLocations::ClipFields{
    query("uClip"),query("uFragmentBias"),query("uNativeTopAtRowZero"),query("uTargetHeight")};}
   // Selector1 does not have any clip/FragCoord/height fields in its real PS.
   // Their optional is absent; no synthetic location or default query is made.
   uniform_programs[index]=program;effect_fence();query_first();query_exit.committed=true;
  } // Every shared Shader/current query scope is gone before allocator/full.
  first();full();guard.require_current_without_callbacks();first();
  const auto actual_source=guard.storage_;guard.require_current_without_callbacks();first();
  const auto*normal=detail::BnwRendererContextPin::NormalAccess::api_current_without_switch_owned(pin);first();
  const GlApi normal_api=*normal;guard.require_current_without_callbacks();first();
  first();std::unique_ptr<OriginalProgramLibrary::Storage> provisional(new OriginalProgramLibrary::Storage(*actual_source,pin,normal_api,render,uniform_programs,uniform_locations));first();
  first();std::shared_ptr<OriginalProgramLibrary::Storage> storage(std::move(provisional));first();
  first();auto facade=std::unique_ptr<OriginalProgramLibrary>(new OriginalProgramLibrary(storage));first();
  result=std::shared_ptr<OriginalProgramLibrary>(std::move(facade));first();
  // Final logical POST precedes callback-free sole-custody transfer. No
  // allocation/GL callback occurs during the two moves or shader teardown.
  full();guard.require_current_without_callbacks();first();
  for(unsigned index=0;index<2;++index){const auto& shader=shaders[index];shader->impl_->original_program_first();first();
   need(shader->impl_->original_program_selector==(index==0?1u:4u)&&shader->impl_->retirement_node&&
    shader->impl_->native_font_sampler_policy_applied,
    "Original code library physical variant custody differs");}
  for(unsigned index=0;index<2;++index){auto& impl=*shaders[index]->impl_;
   storage->nodes[index]=std::move(impl.retirement_node);impl.shader=impl.vao=impl.vertices=impl.indices=impl.sampler=0;
  }
  storage->first();first();
 } // Original source facade and both per-DATA Shader facades are destroyed.
 // The library contains no old DATA, Paint, Early, Atlas or Source Guard.
 full();result->storage_->first();first();return result;
}
std::unique_ptr<GlBnwFontShader::OriginalProgramUse>GlBnwFontShader::capture_original_program_use_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,ui::detail::BnwLabelPaintPositionedData::Guard data,
 std::shared_ptr<OriginalProgramLibrary> library,std::shared_ptr<const assets::CivisResourceBankOwner> bank,
 std::shared_ptr<GlBnwSystemFontAtlas> atlas,std::shared_ptr<const detail::BnwRendererContextPin> pin){
 const auto paint=data.captured_paint_guard_without_callbacks();const auto early=data.captured_early_flush_guard_without_callbacks();
 const auto first=[&]{data.require_current_without_callbacks();paint.require_current_without_callbacks();early.require_current_without_callbacks();
  data.require_same_paint_guard_without_callbacks(paint);early.require_same_paint_guard_without_callbacks(paint);
  need(library&&bank&&atlas&&pin&&library->same_key_without_callbacks(bank,pin),"Fresh program use canonical Bank/Pin key differs");
  early.accepted_pair_without_callbacks(*atlas);atlas->require_same_owned_context_pin_no_current(pin);
  library->storage_->first();data.require_current_without_callbacks();paint.require_current_without_callbacks();early.require_current_without_callbacks();};
 first();std::unique_ptr<OriginalProgramUse> result;
 {first();data.require_source_owned(operation);first();
  first();std::unique_ptr<OriginalProgramUse::Storage> provisional(new OriginalProgramUse::Storage(data,library->storage_,atlas,pin));first();provisional->first();
  first();std::shared_ptr<OriginalProgramUse::Storage> storage(std::move(provisional));first();storage->first();
  first();result=std::unique_ptr<OriginalProgramUse>(new OriginalProgramUse(storage));first();result->require_current_without_callbacks();
 }
 result->require_source_owned(operation);result->require_current_without_callbacks();return result;
}

// Genuine PORT records: immutable all-pass streams and an exclusive existing
// transport lease. The original plane facade is retained, never cloned.
struct GlBnwFontShader::DrawRecordState final {
 using Binding=detail::BnwNormalLabelDrawRecordBinding;
 using RawVertex=PositionedVertices::RawVertex;
 using Node=detail::BnwOwnedGlRetirementNode;
 struct Range final {std::size_t vertex_begin,vertex_count,index_begin,index_count;};
 const std::thread::id creator=std::this_thread::get_id();
 std::atomic<bool> alive{true};
 const std::shared_ptr<OriginalProgramUse::Storage> use;
 const std::shared_ptr<const PositionedVertices::Storage> vertices;
 const std::shared_ptr<OriginalProgramLibrary::Storage> code;
 const std::unique_ptr<GlBnwSystemFontAtlas::AcceptedFontPlanes> planes;
 const std::vector<RawVertex> all_vertices;
 const std::vector<std::uint32_t> all_indices;
 const std::array<Range,3> ranges;
 const std::array<GlBnwSystemFontAtlas::AcceptedFontPlanes::SelectedStages,3> stages;
 Node*const node_identity;
 const Node::ShaderResourcesStorage names;
 bool cpu_records_sealed=false; // Genuine immutable record storage, not GPU readiness.
 bool owns_lease=false,staged=false,submitting=false,submitted=false;
 DrawRecordState(std::shared_ptr<OriginalProgramUse::Storage> u,
  std::shared_ptr<const PositionedVertices::Storage> v,
  std::unique_ptr<GlBnwSystemFontAtlas::AcceptedFontPlanes> p,
  std::vector<RawVertex>&& raw,std::vector<std::uint32_t>&& indices,
  std::array<Range,3> actual_ranges,
  std::array<GlBnwSystemFontAtlas::AcceptedFontPlanes::SelectedStages,3> actual_stages,
  Node* actual_node,Node::ShaderResourcesStorage actual_names)
  :use(std::move(u)),vertices(std::move(v)),code(use->code),planes(std::move(p)),
   all_vertices(std::move(raw)),all_indices(std::move(indices)),ranges(actual_ranges),
   stages(actual_stages),node_identity(actual_node),names(actual_names){}
 std::array<std::uint64_t,3> renderer_descriptor_occurrences{};
 void renderer_descriptor_physical_first(bool require_material=true)const {
  descriptor_need(creator==std::this_thread::get_id()&&alive.load());
  descriptor_need(use&&vertices&&code&&planes&&node_identity);
  descriptor_need(use->creator==creator&&vertices->creator==creator&&code->creator==creator);
  descriptor_need(use->alive.load()&&vertices->alive.load());
  descriptor_need(use->code.get()==code.get()&&!use->code.owner_before(code)&&!code.owner_before(use->code));
  // Historical CPU descriptor/record provenance does not assert a GPU lease.
  descriptor_need(cpu_records_sealed&&!code->submit_failed);
  descriptor_need(code->nodes[0].get()==node_identity);
  descriptor_need(code->renderer_own_descriptor.constructor_arguments==
   std::array<std::uint32_t,8>{0u,144u,15u,3u,3u,0u,6u,0u});
  // This helper never calls State.first, Use/Vertices.first, Source, Pin,
  // WGL or GL. Root supplies actual FULL/current payload/scalar PRE and POST.
  if(require_material){
   descriptor_need(code->renderer_material_control.has_value());
   const auto&material=*code->renderer_material_control;
   descriptor_need(material.counts==std::array<std::uint32_t,3>{1u,0u,0u}&&material.family==0x2bu&&
    material.own_28==&code->renderer_own_descriptor);
   // Root identity owns the genuine canonical GUI shared resource. This is
   // index1A/F0/VS0..14/noPS metadata, not full-F0 values or an original pool ID.
   material.shared_10.require_current_without_callbacks();
  }
 }
 void attach_renderer_shared_descriptor_without_callbacks(
  const ::civ5::ui::detail::BnwWindowedRendererSharedDescriptorIdentity&input)const {
  const auto shared=input; // Copyable sealed physical control before checks.
  renderer_descriptor_physical_first(false);
  shared.require_current_without_callbacks();
  descriptor_need(!submitting&&!submitted);
  if(code->renderer_material_control){
   renderer_descriptor_physical_first();
   shared.require_same_owner_without_callbacks(code->renderer_material_control->shared_10);
   return; // Never silently rebind material+10 to a different GUI.
  }
  // All rejecting checks precede callback-free, allocation-free emplacement.
  code->renderer_material_control.emplace(shared,code->renderer_own_descriptor);
 }
 // Physical historical provenance only. Root separately earns current
 // Item/Source/frame/context/DATA health; no ACTIVE facade check is called.
 void physical_cpu_record_first()const {
  renderer_descriptor_physical_first();
  descriptor_need(use->atlas&&use->pin&&code->pin&&code->bank&&code->capability);
  descriptor_need(use->pin.get()==code->pin.get()&&
   !use->pin.owner_before(code->pin)&&!code->pin.owner_before(use->pin));
  descriptor_need(use->data.impl_&&vertices->data_guard.impl_&&
   use->data.impl_.get()==vertices->data_guard.impl_.get()&&
   !use->data.impl_.owner_before(vertices->data_guard.impl_)&&
   !vertices->data_guard.impl_.owner_before(use->data.impl_));
  const auto actual=node_identity->shader_resources_storage_without_callbacks();
  descriptor_need(names.program&&names.stages[0]&&names.stages[1]&&names.vertices&&
   names.indices&&names.vao&&names.sampler&&actual.program==names.program&&
   actual.stages==names.stages&&actual.vertices==names.vertices&&actual.indices==names.indices&&
   actual.vao==names.vao&&actual.sampler==names.sampler);
  std::size_t vertex_begin=0,index_begin=0;
  for(unsigned pass=0;pass<3;++pass){
   const auto&range=ranges[pass];const auto&stage=stages[pass];
   descriptor_need(range.vertex_begin==vertex_begin&&range.index_begin==index_begin&&
    range.vertex_count==vertices->vertices[pass].size()&&range.index_count==vertices->indices[pass].size());
   descriptor_need(vertex_begin<=all_vertices.size()&&range.vertex_count<=all_vertices.size()-vertex_begin&&
    index_begin<=all_indices.size()&&range.index_count<=all_indices.size()-index_begin);
   descriptor_need(stage.native_format==9u&&stage.textures[0]&&stage.textures[0]==stage.textures[1]);
   vertex_begin+=range.vertex_count;index_begin+=range.index_count;
  }
  descriptor_need(vertex_begin==all_vertices.size()&&index_begin==all_indices.size());
  // The original plane facade is uniquely owned here; its immutable captured
  // stages were sealed under ACTIVE checks at preparation. Later Pair/child
  // health is Root historical authority, not AcceptedFontPlanes::first.
  renderer_descriptor_physical_first();
 }
 void cpu_first(bool require_sealed=true)const{
  need(creator==std::this_thread::get_id()&&alive.load(),"DrawRecord original lexical facade retired/foreign");
  need(use&&vertices&&code&&planes&&use->code.get()==code.get()&&
   !use->code.owner_before(code)&&!code.owner_before(use->code),
   "CPU DrawRecord canonical original code/control absent");
  if(require_sealed)need(cpu_records_sealed,"CPU DrawRecord immutable streams not sealed");
  use->data.require_current_without_callbacks();vertices->first();
  need(use->data.impl_.get()==vertices->data_guard.impl_.get()&&
   !use->data.impl_.owner_before(vertices->data_guard.impl_)&&
   !vertices->data_guard.impl_.owner_before(use->data.impl_),"DrawRecord SAME DATA/control changed");
  use->first();planes->require_current_without_callbacks();
  planes->require_actual_atlas_without_callbacks(*use->atlas);
  need(code->nodes[0].get()==node_identity,"DrawRecord actual transport Node changed");
  const auto actual=node_identity->shader_resources_storage_without_callbacks();
  need(actual.program==names.program&&actual.vertices==names.vertices&&actual.indices==names.indices&&
   actual.vao==names.vao&&actual.sampler==names.sampler,"DrawRecord transport custody changed");
  for(unsigned pass=0;pass<3;++pass){
   const auto chosen=planes->selected_stages_without_callbacks(pass);
   need(chosen.textures==stages[pass].textures&&chosen.native_format==stages[pass].native_format,
    "DrawRecord genuine per-pass physical plane changed");
  }
  use->data.require_current_without_callbacks();vertices->first();
  need(alive.load()&&(!require_sealed||cpu_records_sealed),
   "CPU DrawRecord original facade retired at POST");
 }
 void cpu_full(const runtime::BnwWorldViewSourceOperationLoan& operation)const{
  cpu_first();use->data.require_source_owned(operation);cpu_first();
  use->paint.require_source_owned(operation);cpu_first();use->early.require_source_owned(operation);cpu_first();
  vertices->data_guard.require_source_owned(operation);cpu_first();
 }
 void first(bool require_staged=true)const{
  need(creator==std::this_thread::get_id()&&alive.load(),"DrawRecord original lexical facade retired/foreign");
  need(owns_lease&&code&&code->submit_busy&&!code->submit_failed&&code->submit_owner==this,
   "DrawRecord actual exclusive code lease lost/poisoned");
  if(require_staged)need(staged,"DrawRecord all-pass transport not staged");
  use->data.require_current_without_callbacks();vertices->first();
  need(use->data.impl_.get()==vertices->data_guard.impl_.get()&&
   !use->data.impl_.owner_before(vertices->data_guard.impl_)&&
   !vertices->data_guard.impl_.owner_before(use->data.impl_),"DrawRecord SAME DATA/control changed");
  use->first();planes->require_current_without_callbacks();
  planes->require_actual_atlas_without_callbacks(*use->atlas);
  need(code->nodes[0].get()==node_identity,"DrawRecord actual transport Node changed");
  const auto actual=node_identity->shader_resources_storage_without_callbacks();
  need(actual.program==names.program&&actual.vertices==names.vertices&&actual.indices==names.indices&&
   actual.vao==names.vao&&actual.sampler==names.sampler,"DrawRecord transport custody changed");
  for(unsigned pass=0;pass<3;++pass){
   const auto chosen=planes->selected_stages_without_callbacks(pass);
   need(chosen.textures==stages[pass].textures&&chosen.native_format==stages[pass].native_format,
    "DrawRecord genuine per-pass physical plane changed");
  }
  use->data.require_current_without_callbacks();vertices->first();
  need(alive.load()&&code->submit_busy&&!code->submit_failed&&code->submit_owner==this,
   "DrawRecord lexical/code lease retired at POST");
 }
 void full(const runtime::BnwWorldViewSourceOperationLoan& operation)const{
  first();use->data.require_source_owned(operation);first();
  use->paint.require_source_owned(operation);first();use->early.require_source_owned(operation);first();
  vertices->data_guard.require_source_owned(operation);first();
 }
 void retire()noexcept{
  if(creator!=std::this_thread::get_id())std::terminate();
  alive.store(false);
  if(owns_lease){
   if(!code->submit_busy||code->submit_owner!=this)std::terminate();
   if(!submitted)code->submit_failed=true;
   code->submit_owner=nullptr;code->submit_busy=false;owns_lease=false;
  }
 }
 ~DrawRecordState(){retire();}
};
struct detail::BnwNormalLabelDrawRecordBinding::Impl final {
 const std::shared_ptr<GlBnwFontShader::DrawRecordState> state;
 explicit Impl(std::shared_ptr<GlBnwFontShader::DrawRecordState> actual):state(std::move(actual)){}
};
detail::BnwNormalLabelDrawRecordBinding::BnwNormalLabelDrawRecordBinding(
 std::shared_ptr<Impl> actual,const IssuerKey&):impl_(std::move(actual)){}
detail::BnwNormalLabelDrawRecordBinding::~BnwNormalLabelDrawRecordBinding(){impl_->state->retire();}
detail::BnwNormalLabelDrawRecordBinding::Guard::Guard(std::shared_ptr<Impl> actual):impl_(std::move(actual)){}
detail::BnwNormalLabelDrawRecordBinding::WeakGuard::WeakGuard(std::weak_ptr<Impl> actual):impl_(std::move(actual)){}
detail::BnwNormalLabelDrawRecordBinding::OpaqueRecordIdentity::OpaqueRecordIdentity(std::shared_ptr<Impl> actual,unsigned pass)
 :impl_(std::move(actual)),pass_(pass){}
detail::BnwNormalLabelDrawRecordBinding::Guard
detail::BnwNormalLabelDrawRecordBinding::capture_guard_without_callbacks()const{
 const auto saved=impl_;saved->state->cpu_first();Guard result(saved);saved->state->cpu_first();return result;
}
void detail::BnwNormalLabelDrawRecordBinding::Guard::require_current_without_callbacks()const{
 const auto saved=impl_;saved->state->cpu_first();
}
void detail::BnwNormalLabelDrawRecordBinding::Guard::require_source_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation)const{
 const auto saved=impl_;saved->state->cpu_full(operation);
}
void detail::BnwNormalLabelDrawRecordBinding::Guard::require_same_positioned_data_without_callbacks(
 const ::civ5::ui::detail::BnwLabelPaintPositionedData& input)const{
 const auto saved=impl_;
 // Capture the borrowed DATA Guard in the actual Shader member BEFORE its
 // first normal native observation. Only retained snapshots are used afterward.
 GlBnwFontShader::require_draw_record_data_without_callbacks(saved->state,input);
 saved->state->cpu_first();
}
detail::BnwNormalLabelDrawRecordBinding::WeakGuard
detail::BnwNormalLabelDrawRecordBinding::Guard::capture_weak_without_callbacks()const{
 const auto saved=impl_;saved->state->cpu_first();WeakGuard result(saved);saved->state->cpu_first();return result;
}
detail::BnwNormalLabelDrawRecordBinding::Guard
detail::BnwNormalLabelDrawRecordBinding::WeakGuard::lock_current_without_callbacks()const{
 const auto origin=impl_;const auto saved=origin.lock();need(bool(saved),"DrawRecord weak canonical origin retired");
 saved->state->cpu_first();Guard result(saved);saved->state->cpu_first();return result;
}
void detail::BnwNormalLabelDrawRecordBinding::WeakGuard::require_current_without_callbacks()const{
 const auto origin=impl_;const auto saved=origin.lock();need(bool(saved),"DrawRecord weak canonical origin retired");
 saved->state->cpu_first();
}
detail::BnwNormalLabelDrawRecordBinding::OpaqueRecordIdentity
detail::BnwNormalLabelDrawRecordBinding::Guard::originating_record_without_callbacks(unsigned input_pass)const{
 const auto saved=impl_;const auto pass=input_pass;saved->state->cpu_first();
 need(pass<3&&saved->state->ranges[pass].index_count,"DrawRecord requested pass was not genuinely emitted");
 OpaqueRecordIdentity result(saved,pass);saved->state->cpu_first();return result;
}
void detail::BnwNormalLabelDrawRecordBinding::OpaqueRecordIdentity::require_current_without_callbacks()const{
 const auto saved=impl_;const auto pass=pass_;saved->state->cpu_first();
 need(pass<3&&saved->state->ranges[pass].index_count,"DrawRecord original pass retired/invalid");saved->state->cpu_first();
}
void detail::BnwNormalLabelDrawRecordBinding::OpaqueRecordIdentity::require_same_binding_without_callbacks(const Guard& input)const{
 const auto saved=impl_;const auto actual=input.impl_;saved->state->cpu_first();actual->state->cpu_first();
 need(saved.get()==actual.get()&&!saved.owner_before(actual)&&!actual.owner_before(saved),
  "DrawRecord identity from another canonical binding");saved->state->cpu_first();actual->state->cpu_first();
}
detail::BnwNormalLabelDrawRecordBinding::OpaqueMaterialIdentity::OpaqueMaterialIdentity(std::shared_ptr<Impl> actual)
 :impl_(std::move(actual)){}
detail::BnwNormalLabelDrawRecordBinding::RendererDescriptorReceipt::RendererDescriptorReceipt(
 std::shared_ptr<Impl> actual,unsigned pass,std::uint64_t occurrence)noexcept
 :impl_(std::move(actual)),pass_(pass),write_occurrence_(occurrence){}
void detail::BnwNormalLabelDrawRecordBinding::Guard::attach_shared_descriptor_without_callbacks(
 const ::civ5::ui::detail::BnwWindowedRendererSharedDescriptorIdentity&input)const {
 const auto saved=impl_;const auto shared=input;
 descriptor_need(saved&&saved->state);
 saved->state->attach_renderer_shared_descriptor_without_callbacks(shared);
}
detail::BnwNormalLabelDrawRecordBinding::RendererDescriptorReceipt
detail::BnwNormalLabelDrawRecordBinding::Guard::finalize_renderer_descriptor_without_callbacks(
 const OpaqueRecordIdentity&input)const {
 const auto saved=impl_;const auto record=input.impl_;const auto pass=input.pass_;
 descriptor_need(saved&&record&&saved.get()==record.get()&&
  !saved.owner_before(record)&&!record.owner_before(saved));
 const auto state=saved->state;descriptor_need(bool(state));
 state->renderer_descriptor_physical_first();
 descriptor_need(pass<3&&state->ranges[pass].index_count&&state->ranges[pass].vertex_count);
 descriptor_need(!state->submitting&&!state->submitted&&!state->renderer_descriptor_occurrences[pass]);
 auto&descriptor=state->code->renderer_own_descriptor;
 descriptor_need(descriptor.executed_zero_stores<std::numeric_limits<std::uint64_t>::max());
 const auto occurrence=descriptor.executed_zero_stores+1u;
 // Genuine 8EEFE0 repeated CPU byte store, even after constructor already zero.
 // All throwing/rejecting checks precede this effect. No dirty=true exists.
 descriptor.field_2c=0u;
 descriptor.executed_zero_stores=occurrence;
 state->renderer_descriptor_occurrences[pass]=occurrence;
 return RendererDescriptorReceipt(saved,pass,occurrence);
}
void detail::BnwNormalLabelDrawRecordBinding::RendererDescriptorReceipt::require_current_without_callbacks()const {
 const auto saved=impl_;const auto pass=pass_;const auto occurrence=write_occurrence_;
 descriptor_need(saved&&saved->state);const auto state=saved->state;
 state->renderer_descriptor_physical_first();
 descriptor_need(pass<3&&state->ranges[pass].index_count&&occurrence&&
  state->renderer_descriptor_occurrences[pass]==occurrence&&
  occurrence<=state->code->renderer_own_descriptor.executed_zero_stores&&
  state->code->renderer_own_descriptor.field_2c==0u);
}
void detail::BnwNormalLabelDrawRecordBinding::RendererDescriptorReceipt::require_same_binding_without_callbacks(
 const Guard&input)const {
 const auto saved=impl_;const auto actual=input.impl_;
 descriptor_need(saved&&actual&&saved.get()==actual.get()&&
  !saved.owner_before(actual)&&!actual.owner_before(saved));
 require_current_without_callbacks();
}
void detail::BnwNormalLabelDrawRecordBinding::RendererDescriptorReceipt::require_same_record_without_callbacks(
 const OpaqueRecordIdentity&input)const {
 const auto saved=impl_;const auto actual=input.impl_;const auto pass=input.pass_;
 descriptor_need(saved&&actual&&saved.get()==actual.get()&&
  !saved.owner_before(actual)&&!actual.owner_before(saved)&&pass_==pass);
 require_current_without_callbacks();
}
detail::BnwNormalLabelDrawRecordBinding::OpaqueMaterialIdentity
detail::BnwNormalLabelDrawRecordBinding::RendererDescriptorReceipt::material_identity_without_callbacks()const {
 const auto saved=impl_;require_current_without_callbacks();return OpaqueMaterialIdentity(saved);
}
void detail::BnwNormalLabelDrawRecordBinding::OpaqueMaterialIdentity::require_current_without_callbacks()const {
 const auto saved=impl_;descriptor_need(saved&&saved->state);
 saved->state->renderer_descriptor_physical_first();
 // SAME canonical actual material control, not Binding-as-material readiness.
}
void detail::BnwNormalLabelDrawRecordBinding::OpaqueMaterialIdentity::require_same_descriptor_without_callbacks(
 const RendererDescriptorReceipt&input)const {
 const auto saved=impl_;const auto actual=input.impl_;
 descriptor_need(saved&&actual&&saved.get()==actual.get()&&
  !saved.owner_before(actual)&&!actual.owner_before(saved));
 require_current_without_callbacks();input.require_current_without_callbacks();
}
// PRIVATE physical-only seam for the genuine Root queue permission owner.
// These checks issue no Root permission, record, receipt or GPU lease.
void detail::BnwNormalLabelDrawRecordBinding::Guard::require_physical_record_without_callbacks()const {
 const auto saved=impl_;descriptor_need(saved&&saved->state);
 saved->state->physical_cpu_record_first();
}
void detail::BnwNormalLabelDrawRecordBinding::OpaqueRecordIdentity::require_physical_record_without_callbacks()const {
 const auto saved=impl_;const auto pass=pass_;descriptor_need(saved&&saved->state);
 const auto state=saved->state;state->physical_cpu_record_first();
 descriptor_need(pass<3&&state->ranges[pass].index_count&&state->ranges[pass].vertex_count);
 state->physical_cpu_record_first();
}
void detail::BnwNormalLabelDrawRecordBinding::OpaqueRecordIdentity::require_same_binding_physical_without_callbacks(
 const Guard&input)const {
 const auto saved=impl_;const auto actual=input.impl_;const auto pass=pass_;
 descriptor_need(saved&&actual&&saved->state&&actual->state);
 saved->state->physical_cpu_record_first();actual->state->physical_cpu_record_first();
 descriptor_need(saved.get()==actual.get()&&!saved.owner_before(actual)&&!actual.owner_before(saved)&&
  pass<3&&saved->state->ranges[pass].index_count&&saved->state->ranges[pass].vertex_count);
 saved->state->physical_cpu_record_first();actual->state->physical_cpu_record_first();
}
void detail::BnwNormalLabelDrawRecordBinding::RendererDescriptorReceipt::require_physical_record_without_callbacks()const {
 const auto saved=impl_;const auto pass=pass_;const auto occurrence=write_occurrence_;
 descriptor_need(saved&&saved->state);const auto state=saved->state;
 state->physical_cpu_record_first();
 descriptor_need(pass<3&&state->ranges[pass].index_count&&state->ranges[pass].vertex_count&&occurrence&&
  state->renderer_descriptor_occurrences[pass]==occurrence&&
  occurrence<=state->code->renderer_own_descriptor.executed_zero_stores&&
  state->code->renderer_own_descriptor.field_2c==0u);
 state->physical_cpu_record_first();
}
void detail::BnwNormalLabelDrawRecordBinding::RendererDescriptorReceipt::require_same_binding_physical_without_callbacks(
 const Guard&input)const {
 const auto saved=impl_;const auto actual=input.impl_;const auto pass=pass_;const auto occurrence=write_occurrence_;
 descriptor_need(saved&&actual&&saved->state&&actual->state);
 saved->state->physical_cpu_record_first();actual->state->physical_cpu_record_first();
 descriptor_need(saved.get()==actual.get()&&!saved.owner_before(actual)&&!actual.owner_before(saved)&&
  pass<3&&saved->state->ranges[pass].index_count&&saved->state->ranges[pass].vertex_count&&occurrence&&
  saved->state->renderer_descriptor_occurrences[pass]==occurrence&&
  occurrence<=saved->state->code->renderer_own_descriptor.executed_zero_stores&&
  saved->state->code->renderer_own_descriptor.field_2c==0u);
 saved->state->physical_cpu_record_first();actual->state->physical_cpu_record_first();
}
void detail::BnwNormalLabelDrawRecordBinding::RendererDescriptorReceipt::require_same_record_physical_without_callbacks(
 const OpaqueRecordIdentity&input)const {
 const auto saved=impl_;const auto actual=input.impl_;const auto pass=pass_;const auto actual_pass=input.pass_;
 const auto occurrence=write_occurrence_;
 descriptor_need(saved&&actual&&saved->state&&actual->state);
 saved->state->physical_cpu_record_first();actual->state->physical_cpu_record_first();
 descriptor_need(saved.get()==actual.get()&&!saved.owner_before(actual)&&!actual.owner_before(saved)&&
  pass==actual_pass&&pass<3&&saved->state->ranges[pass].index_count&&saved->state->ranges[pass].vertex_count&&occurrence&&
  saved->state->renderer_descriptor_occurrences[pass]==occurrence&&
  occurrence<=saved->state->code->renderer_own_descriptor.executed_zero_stores&&
  saved->state->code->renderer_own_descriptor.field_2c==0u);
 saved->state->physical_cpu_record_first();actual->state->physical_cpu_record_first();
}
// Private real per-selected-record lease. Root guards are closed issuer values;
// their actual complete/selected-entry bodies must succeed before GL effects.
// Private original lease life. Retained result storage never renews alive.
struct GlBnwFontShader::GpuStagingLease::Life final {
 const std::thread::id creator=std::this_thread::get_id();
 const Impl*const original;
 std::atomic<bool> alive{true};
 explicit Life(const Impl* actual):original(actual){}
 Life(const Life&)=delete;Life&operator=(const Life&)=delete;
};
struct GlBnwFontShader::GpuStagingLease::Impl final {
 using Binding=detail::BnwNormalLabelDrawRecordBinding;
 const std::thread::id creator=std::this_thread::get_id();
 const std::shared_ptr<DrawRecordState> state;
 const ::civ5::ui::UiScreenGlRenderer*const renderer;
 const ::civ5::ui::detail::BnwNormalRootBatchHistory::Guard root;
 const ::civ5::ui::detail::BnwNormalItemOccurrence::Guard item;
 const ::civ5::ui::detail::BnwNormalQueueDrainPermission::Guard permission;
 const Binding::Guard binding;
 const Binding::OpaqueRecordIdentity record;
 const Binding::RendererDescriptorReceipt descriptor;
 const std::shared_ptr<Life> original_life=std::make_shared<Life>(this);
 bool reserved=false,uploaded=false,consumed_after_restore=false;
 bool submitting=false,transport_returned=false,transport_aborted=false;
 bool actual_native_failure=false;GLenum native_reset=GL_NO_ERROR,native_error=GL_NO_ERROR;
 Impl(std::shared_ptr<DrawRecordState> actual,const ::civ5::ui::UiScreenGlRenderer*actual_renderer,
  ::civ5::ui::detail::BnwNormalRootBatchHistory::Guard actual_root,
  ::civ5::ui::detail::BnwNormalItemOccurrence::Guard actual_item,
  ::civ5::ui::detail::BnwNormalQueueDrainPermission::Guard actual_permission,
  Binding::Guard actual_binding,Binding::OpaqueRecordIdentity actual_record,
  Binding::RendererDescriptorReceipt actual_descriptor)
  :state(std::move(actual)),renderer(actual_renderer),root(std::move(actual_root)),item(std::move(actual_item)),
   permission(std::move(actual_permission)),binding(std::move(actual_binding)),record(std::move(actual_record)),
   descriptor(std::move(actual_descriptor)){}
 Impl(const Impl&)=delete;
 Impl&operator=(const Impl&)=delete;
 Impl(Impl&&)=delete;
 Impl&operator=(Impl&&)=delete;
 // Genuine Root23 capsule bridge. No old ACTIVE plane/DATA/Paint/Early
 // getters and no current-Atlas-frame equality survive item retirement here.
 static void require_original_planes_without_callbacks(
  const std::shared_ptr<DrawRecordState>& input,
  const ::civ5::ui::detail::BnwNormalItemOccurrence::Guard& input_item){
  const auto saved=input;const auto actual_item=input_item;
  need(saved&&saved->planes&&saved->use&&saved->use->atlas,
   "Queued stage original planes/Atlas custody absent");
  saved->physical_cpu_record_first();actual_item.require_physical_current_without_callbacks();
  const auto planes=actual_item.historical_font_planes_without_callbacks();
  const auto atlas=actual_item.actual_font_atlas_owned_without_callbacks();
  need(atlas&&atlas.get()==saved->use->atlas.get()&&
   !atlas.owner_before(saved->use->atlas)&&!saved->use->atlas.owner_before(atlas),
   "Queued stage historical capsule has another original Atlas owner");
  planes.require_physical_current_without_callbacks();
  planes.require_actual_atlas_without_callbacks(*atlas);
  planes.require_actual_positioned_data_without_callbacks(saved->use->data);
  planes.require_actual_positioned_data_without_callbacks(saved->vertices->data_guard);
  // This bridge requires original AcceptedPlanes alive plus exact retained
  // DATA/Atlas/Pin/Pair control, Node, names and captured immutable frame.
  planes.require_same_accepted_planes_without_callbacks(*saved->planes);
  for(unsigned pass=0;pass<3;++pass){
   const auto selected=planes.selected_stages_without_callbacks(pass);
   need(selected.textures==saved->stages[pass].textures&&
    selected.native_format==saved->stages[pass].native_format,
    "Queued stage genuine historical selected planes differ from sealed CPU pass");
  }
  planes.require_same_accepted_planes_without_callbacks(*saved->planes);
  planes.require_actual_positioned_data_without_callbacks(saved->use->data);
  planes.require_actual_positioned_data_without_callbacks(saved->vertices->data_guard);
  planes.require_actual_atlas_without_callbacks(*atlas);
  planes.require_physical_current_without_callbacks();
  actual_item.require_physical_current_without_callbacks();saved->physical_cpu_record_first();
 }
 void logical_first()const{
  need(creator==std::this_thread::get_id()&&state&&renderer,"Queued stage original lease foreign/absent");
  state->physical_cpu_record_first();require_original_planes_without_callbacks(state,item);
  root.require_physical_current_without_callbacks();item.require_physical_current_without_callbacks();
  permission.require_physical_current_without_callbacks();
  root.require_actual_renderer_without_callbacks(*renderer);item.require_actual_renderer_without_callbacks(*renderer);
  permission.require_actual_renderer_without_callbacks(*renderer);
  item.require_same_root_without_callbacks(root);permission.require_same_root_without_callbacks(root);
  item.require_actual_positioned_data_without_callbacks(state->use->data);
  item.require_actual_positioned_data_without_callbacks(state->vertices->data_guard);
  binding.require_physical_record_without_callbacks();record.require_physical_record_without_callbacks();
  record.require_same_binding_physical_without_callbacks(binding);
  descriptor.require_physical_record_without_callbacks();descriptor.require_same_binding_physical_without_callbacks(binding);
  descriptor.require_same_record_physical_without_callbacks(record);
  const auto canonical=item.captured_normal_draw_records_without_callbacks();
  record.require_same_binding_physical_without_callbacks(canonical);
  descriptor.require_same_binding_physical_without_callbacks(canonical);
  // This existing closed Root method must certify CURRENT actual selected
  // inner entry, not an outer selector/raw header or a local vector ordering.
  permission.require_queued_record_without_callbacks(item,record,descriptor);
  state->physical_cpu_record_first();require_original_planes_without_callbacks(state,item);
  root.require_physical_current_without_callbacks();
  item.require_physical_current_without_callbacks();permission.require_physical_current_without_callbacks();
 }
 void first(bool require_uploaded=true)const{
  need(creator==std::this_thread::get_id()&&original_life&&original_life->alive.load()&&
   original_life->creator==creator&&original_life->original==this&&reserved&&state&&state->code,
   "Queued stage actual reserved lease retired/foreign");
  const auto code=state->code;
  need(code->submit_busy&&!code->submit_failed&&!code->submit_owner&&code->queued_submit_owner==this&&
   !consumed_after_restore&&(!require_uploaded||uploaded),"Queued stage exclusive transport lost/poisoned");
  logical_first();
  need(reserved&&code->submit_busy&&!code->submit_failed&&!code->submit_owner&&code->queued_submit_owner==this&&
   !consumed_after_restore&&(!require_uploaded||uploaded),"Queued stage transport changed at POST");
 }
 void full(const runtime::BnwWorldViewSourceOperationLoan& operation,bool require_uploaded=true)const{
  first(require_uploaded);root.require_source_owned(operation);first(require_uploaded);
  item.require_source_owned(operation);first(require_uploaded);
  permission.require_source_owned(operation);first(require_uploaded);
 }
 // Retire the ORIGINAL facade immediately, even while a callback consumer
 // retains this SAME physical control. Shared storage never renews Life.
 void retire_original_without_callbacks()noexcept{
  if(creator!=std::this_thread::get_id())std::terminate();
  if(!original_life->alive.exchange(false))return;
  if(reserved){
   const auto code=state->code;
   if(!code->submit_busy||code->submit_owner||code->queued_submit_owner!=this)std::terminate();
   // Only a future actual Shader consumer may mark this private field after
   // Authenticated command transport and Ui Rows restoration, while this SAME
   // permission is still current. Root must define legitimate skipped commands;
   // original cursor consumption alone does not certify successful Draw.
   // Stage/upload alone never earns consumption or a Root ACK.
   if(!consumed_after_restore)code->submit_failed=true;
   code->queued_submit_owner=nullptr;code->submit_busy=false;reserved=false;
  }
 }
 ~Impl()noexcept{retire_original_without_callbacks();}
};
GlBnwFontShader::GpuStagingLease::GpuStagingLease(std::unique_ptr<Impl> actual):impl_(std::move(actual)){}
GlBnwFontShader::GpuStagingLease::~GpuStagingLease(){
 if(impl_)impl_->retire_original_without_callbacks();
}

// Preallocated genuine result storage, published const only after one real
// selected Draw plus pipeline restoration. Rows remain active until Ui finish.
struct GlBnwFontShader::QueuedRecordTransportResult::Impl final {
 using Material=::civ5::ui::detail::BnwNormalLabelMaterialLoan::ShaderAccess;
 using MaterialControl=decltype(std::declval<const Material&>().impl_);
 using Target=detail::BnwLabelDefaultFontTargetLoan::Guard;
 enum class Outcome {actual_draw_issued,native_failure};
 const std::thread::id creator=std::this_thread::get_id();
 const std::shared_ptr<GpuStagingLease::Life> original_lease_life;
 const GpuStagingLease::Impl*const lease_identity;
 const std::shared_ptr<DrawRecordState> state;
 const ::civ5::ui::detail::BnwNormalRootBatchHistory::Guard root;
 const ::civ5::ui::detail::BnwNormalItemOccurrence::Guard item;
 const ::civ5::ui::detail::BnwNormalQueueDrainPermission::Guard permission;
 const detail::BnwNormalLabelDrawRecordBinding::OpaqueRecordIdentity record;
 const detail::BnwNormalLabelDrawRecordBinding::RendererDescriptorReceipt descriptor;
 const Target target;
 const Material material;
 bool sealed=false;
 Outcome outcome=Outcome::actual_draw_issued;
 Impl(const GpuStagingLease::Impl& lease,Target original_target,MaterialControl original_material,
  const ::civ5::ui::detail::BnwNormalQueueDrainPermission::Guard& original_selected)
  :original_lease_life(lease.original_life),lease_identity(&lease),state(lease.state),
   root(lease.root),item(lease.item),permission(lease.permission),record(lease.record),
   descriptor(lease.descriptor),target(std::move(original_target)),material(std::move(original_material),original_selected){}
 Impl(const Impl&)=delete;Impl&operator=(const Impl&)=delete;
 Impl(Impl&&)=delete;Impl&operator=(Impl&&)=delete;
 // No zero-count outcome can be issued: genuine opaque records require
 // nonzero saved index AND vertex ranges. native_failure remains reserved
 // until Root/Ui supply a distinct failed-command restoration protocol;
 // current healthy permission/material/Rows guards reject poisoned CODE.
};
GlBnwFontShader::QueuedRecordTransportResult::QueuedRecordTransportResult(
 std::shared_ptr<const Impl> actual)noexcept:impl_(std::move(actual)){}
GlBnwFontShader::QueuedRecordTransportResult::~QueuedRecordTransportResult()=default;

void GlBnwFontShader::require_draw_record_data_without_callbacks(
 std::shared_ptr<DrawRecordState> state,const ui::detail::BnwLabelPaintPositionedData& input){
 const auto actual=input.capture_guard_without_callbacks();state->cpu_first();actual.require_current_without_callbacks();
 need(actual.impl_.get()==state->use->data.impl_.get()&&!actual.impl_.owner_before(state->use->data.impl_)&&
  !state->use->data.impl_.owner_before(actual.impl_),"DrawRecord attachment another DATA/control");state->cpu_first();
}
std::unique_ptr<detail::BnwNormalLabelDrawRecordBinding>
GlBnwFontShader::normal_label_draw_record_binding_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,
 const OriginalProgramUse& input_use,const PositionedVertices& input_vertices,
 std::unique_ptr<GlBnwSystemFontAtlas::AcceptedFontPlanes> planes){
 using Binding=detail::BnwNormalLabelDrawRecordBinding;using Node=detail::BnwOwnedGlRetirementNode;
 // ONLY borrowed facade reads. Independent physical storage is retained before
 // every allocator/Source/native effect, without renewing original leases.
 const auto use=input_use.storage_;const auto vertices=input_vertices.storage_;
 need(use&&vertices&&planes,"DrawRecord factory requires genuine current inputs");
 const auto code=use->code;
 need(code&&code->creator==std::this_thread::get_id(),"DrawRecord factory foreign code storage");
 if(code->submit_busy||code->submit_failed){code->submit_failed=true;
  throw std::runtime_error("DrawRecord exclusive existing transport busy/poisoned");}
 code->submit_busy=true;
 struct FactoryLease final {
  OriginalProgramLibrary::Storage& code;bool transferred=false;
  ~FactoryLease(){if(!transferred){code.submit_failed=true;code.submit_owner=nullptr;code.submit_busy=false;}}
 } lease{*code};
 std::shared_ptr<DrawRecordState> state;
 const auto first=[&]{
  need(code->submit_busy&&!code->submit_failed,"DrawRecord factory lease poisoned");
  if(state){state->first(false);return;}
  use->data.require_current_without_callbacks();vertices->first();
  need(use->data.impl_.get()==vertices->data_guard.impl_.get()&&
   !use->data.impl_.owner_before(vertices->data_guard.impl_)&&!vertices->data_guard.impl_.owner_before(use->data.impl_),
   "DrawRecord factory SAME DATA/control differs");
  use->first();planes->require_current_without_callbacks();planes->require_actual_atlas_without_callbacks(*use->atlas);
  use->data.require_current_without_callbacks();vertices->first();
  need(code->submit_busy&&!code->submit_failed&&!code->submit_owner,"DrawRecord provisional code lease changed");
 };
 const auto full=[&]{first();use->data.require_source_owned(operation);first();
  use->paint.require_source_owned(operation);first();use->early.require_source_owned(operation);first();
  vertices->data_guard.require_source_owned(operation);first();};
 first();
 const GlApi api=code->api;const GlRenderApi render=code->render;
 auto*const node=code->nodes[0].get();need(node,"DrawRecord selector1 transport node absent");
 const auto names=node->shader_resources_storage_without_callbacks();first();
 need(api.GetError&&api.GetGraphicsResetStatus&&render.NamedBufferData&&render.VertexArrayVertexBuffer&&
  render.VertexArrayElementBuffer&&render.VertexArrayBindingDivisor&&render.EnableVertexArrayAttrib&&
  render.VertexArrayAttribBinding&&render.VertexArrayAttribFormat,"DrawRecord actual staging dispatch absent");
 std::array<DrawRecordState::Range,3> ranges{};
 std::array<GlBnwSystemFontAtlas::AcceptedFontPlanes::SelectedStages,3> stages{};
 std::size_t vertex_total=0,index_total=0;
 // Creation is ascending0,1,2. Submission remains descending2,1,0.
 for(unsigned pass=0;pass<3;++pass){
  const auto vc=vertices->vertices[pass].size(),ic=vertices->indices[pass].size();first();
  need(vc<=std::numeric_limits<std::size_t>::max()-vertex_total&&
   ic<=std::numeric_limits<std::size_t>::max()-index_total,"DrawRecord concat count overflow");
  ranges[pass]={vertex_total,vc,index_total,ic};vertex_total+=vc;index_total+=ic;
  stages[pass]=planes->selected_stages_without_callbacks(pass);first();
  need(stages[pass].native_format==9&&stages[pass].textures[0]&&
   stages[pass].textures[0]==stages[pass].textures[1],"DrawRecord original chosen stage/format absent");
  need(ic<=static_cast<std::size_t>(std::numeric_limits<GLsizei>::max()),"DrawRecord pass GL count overflow");
 }
 need(vertex_total<=static_cast<std::size_t>(std::numeric_limits<GLsizeiptr>::max())/sizeof(PositionedVertices::RawVertex)&&
  index_total<=static_cast<std::size_t>(std::numeric_limits<GLsizeiptr>::max())/sizeof(std::uint32_t),
  "DrawRecord concat native byte transport overflow");
 std::vector<PositionedVertices::RawVertex> all_vertices;std::vector<std::uint32_t> all_indices;
 first();all_vertices.reserve(vertex_total);first();full();
 first();all_indices.reserve(index_total);first();full();
 for(unsigned pass=0;pass<3;++pass){
  first();for(const auto& v:vertices->vertices[pass])all_vertices.push_back(v);
  for(const auto index:vertices->indices[pass])all_indices.push_back(index);first();
 }
 // Exact ranges use local per-pass DWORD indices plus the real vertex binding
 // byte offset. No native pool offset or record ID is guessed.
 first();std::unique_ptr<DrawRecordState> provisional(new DrawRecordState(use,vertices,std::move(planes),
  std::move(all_vertices),std::move(all_indices),ranges,stages,node,names));
 // Moves are callback-free AFTER allocation. The provisional original plane
 // facade belongs to this same State; commit exact ownership before observers.
 code->submit_owner=provisional.get();provisional->owns_lease=true;lease.transferred=true;
 provisional->first(false);
 // Adopt state/control blocks before final staging, so no publication allocator
 // or Source callback can mutate the staged bytes after the final upload.
 std::shared_ptr<DrawRecordState> adopted(std::move(provisional));state=std::move(adopted);first();full();
 first();std::unique_ptr<Binding::Impl> impl_provisional(new Binding::Impl(state));first();full();
 first();std::shared_ptr<Binding::Impl> impl(std::move(impl_provisional));first();full();
 first();std::unique_ptr<Binding> result(new Binding(impl,Binding::IssuerKey{}));first();full();
 const auto current=[&]{first();const auto*loaded=detail::BnwRendererContextPin::NormalAccess::api_current_without_switch_owned(use->pin);
  first();need(loaded&&loaded->GetError==api.GetError&&loaded->GetGraphicsResetStatus==api.GetGraphicsResetStatus,
   "DrawRecord actual loaded dispatch changed");};
 const auto status=[&]{current();const auto reset=api.GetGraphicsResetStatus();
  if(reset!=GL_NO_ERROR){code->submit_failed=true;throw std::runtime_error("DrawRecord staging context reset");}
  current();const auto error=api.GetError();
  if(error!=GL_NO_ERROR){code->submit_failed=true;throw std::runtime_error("DrawRecord staging GL error");}current();};
 const auto effect=[&](const auto& f){status();f();status();};
 // LAST FULL. All three actual streams are staged contiguously on SAME already
 // existing buffers before publication and before Root's Material call.
 full();status();
 effect([&]{render.NamedBufferData(names.vertices,static_cast<GLsizeiptr>(state->all_vertices.size()*sizeof(PositionedVertices::RawVertex)),state->all_vertices.data(),GL_STREAM_DRAW);});
 effect([&]{render.NamedBufferData(names.indices,static_cast<GLsizeiptr>(state->all_indices.size()*sizeof(std::uint32_t)),state->all_indices.data(),GL_STREAM_DRAW);});
 effect([&]{render.VertexArrayVertexBuffer(names.vao,0,names.vertices,0,24);});
 effect([&]{render.VertexArrayElementBuffer(names.vao,names.indices);});
 effect([&]{render.VertexArrayBindingDivisor(names.vao,0,0);});
 for(unsigned k=0;k<3;++k){
  effect([&,k]{render.EnableVertexArrayAttrib(names.vao,k);});
  effect([&,k]{render.VertexArrayAttribBinding(names.vao,k,0);});
  effect([&,k]{render.VertexArrayAttribFormat(names.vao,k,k==0?3:k==1?2:4,
   k==2?GL_UNSIGNED_BYTE:GL_FLOAT,k==2?GL_TRUE:GL_FALSE,k==0?0:k==1?16:12);});
 }
 state->cpu_records_sealed=true;state->staged=true;state->first();result->capture_guard_without_callbacks().require_current_without_callbacks();
 return result;
}

// CPU preparation is physical producer work; Root registration and FIFO
// consumer permission are mandatory external suppliers before GPU staging.
std::unique_ptr<detail::BnwNormalLabelDrawRecordBinding>
GlBnwFontShader::prepare_normal_label_draw_records_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,
 const OriginalProgramUse& input_use,const PositionedVertices& input_vertices,
 std::unique_ptr<GlBnwSystemFontAtlas::AcceptedFontPlanes> planes){
 using Binding=detail::BnwNormalLabelDrawRecordBinding;using Node=detail::BnwOwnedGlRetirementNode;
 // ONLY borrowed facade reads. Independent physical storage is retained before
 // every allocator/Source/native effect, without renewing original leases.
 const auto use=input_use.storage_;const auto vertices=input_vertices.storage_;
 need(use&&vertices&&planes,"DrawRecord factory requires genuine current inputs");
 const auto code=use->code;
 need(code&&code->creator==std::this_thread::get_id(),"DrawRecord factory foreign code storage");
 std::shared_ptr<DrawRecordState> state;
 const auto first=[&]{
  if(state){state->cpu_first(false);return;}
  use->data.require_current_without_callbacks();vertices->first();
  need(use->data.impl_.get()==vertices->data_guard.impl_.get()&&
   !use->data.impl_.owner_before(vertices->data_guard.impl_)&&!vertices->data_guard.impl_.owner_before(use->data.impl_),
   "DrawRecord factory SAME DATA/control differs");
  use->first();planes->require_current_without_callbacks();planes->require_actual_atlas_without_callbacks(*use->atlas);
  use->data.require_current_without_callbacks();vertices->first();
 };
 const auto full=[&]{first();use->data.require_source_owned(operation);first();
  use->paint.require_source_owned(operation);first();use->early.require_source_owned(operation);first();
  vertices->data_guard.require_source_owned(operation);first();};
 first();
 auto*const node=code->nodes[0].get();need(node,"DrawRecord selector1 transport node absent");
 const auto names=node->shader_resources_storage_without_callbacks();first();
 std::array<DrawRecordState::Range,3> ranges{};
 std::array<GlBnwSystemFontAtlas::AcceptedFontPlanes::SelectedStages,3> stages{};
 std::size_t vertex_total=0,index_total=0;
 // Creation is ascending0,1,2. Submission remains descending2,1,0.
 for(unsigned pass=0;pass<3;++pass){
  const auto vc=vertices->vertices[pass].size(),ic=vertices->indices[pass].size();first();
  need(vc<=std::numeric_limits<std::size_t>::max()-vertex_total&&
   ic<=std::numeric_limits<std::size_t>::max()-index_total,"DrawRecord concat count overflow");
  ranges[pass]={vertex_total,vc,index_total,ic};vertex_total+=vc;index_total+=ic;
  stages[pass]=planes->selected_stages_without_callbacks(pass);first();
  need(stages[pass].native_format==9&&stages[pass].textures[0]&&
   stages[pass].textures[0]==stages[pass].textures[1],"DrawRecord original chosen stage/format absent");
  need(ic<=static_cast<std::size_t>(std::numeric_limits<GLsizei>::max()),"DrawRecord pass GL count overflow");
 }
 need(vertex_total<=static_cast<std::size_t>(std::numeric_limits<GLsizeiptr>::max())/sizeof(PositionedVertices::RawVertex)&&
  index_total<=static_cast<std::size_t>(std::numeric_limits<GLsizeiptr>::max())/sizeof(std::uint32_t),
  "DrawRecord concat native byte transport overflow");
 std::vector<PositionedVertices::RawVertex> all_vertices;std::vector<std::uint32_t> all_indices;
 first();all_vertices.reserve(vertex_total);first();full();
 first();all_indices.reserve(index_total);first();full();
 for(unsigned pass=0;pass<3;++pass){
  first();for(const auto& v:vertices->vertices[pass])all_vertices.push_back(v);
  for(const auto index:vertices->indices[pass])all_indices.push_back(index);first();
 }
 // Exact ranges use local per-pass DWORD indices plus the real vertex binding
 // byte offset. No native pool offset or record ID is guessed.
 first();std::unique_ptr<DrawRecordState> provisional(new DrawRecordState(use,vertices,std::move(planes),
  std::move(all_vertices),std::move(all_indices),ranges,stages,node,names));
 // Only CPU custody transfers here. No CODE lease, GL upload or pool ID.
 provisional->cpu_first(false);
 // Adopt actual immutable CPU state and Binding controls under allocator
 // PRE/POST. Original independent Use/Vertices/planes remain alive.
 std::shared_ptr<DrawRecordState> adopted(std::move(provisional));state=std::move(adopted);first();full();
 first();std::unique_ptr<Binding::Impl> impl_provisional(new Binding::Impl(state));first();full();
 first();std::shared_ptr<Binding::Impl> impl(std::move(impl_provisional));first();full();
 first();std::unique_ptr<Binding> result(new Binding(impl,Binding::IssuerKey{}));first();full();
 // All allocation and Source callback POSTs have returned. Seal the exact
 // immutable streams/ranges already owned by this original per-item State.
 // This grants CPU record provenance only; no completed Root or GPU admission.
 full();first();state->cpu_records_sealed=true;state->cpu_first();
 result->capture_guard_without_callbacks().require_current_without_callbacks();
 return result;
}

// Actual private upload primitive used only by the closed selected-entry
// stage below. Cut closures bind its retained real Root/Item/permission fields.
// No Draw, queue advancement, grouping or material/payload inference occurs.
template<class PhysicalCuts,class SourceCuts>
void GlBnwFontShader::upload_prepared_record_transport_unpublished_owned(
 GpuStagingLease::Impl& entry,const PhysicalCuts& physical_cuts,const SourceCuts& source_cuts){
 const auto state=entry.state;entry.first(false);
 const auto code=state->code;const auto pin=state->use->pin;
 bool uploaded=false;
 struct FailureExit final {
  OriginalProgramLibrary::Storage& code;bool& uploaded;
  ~FailureExit(){if(!uploaded)code.submit_failed=true;}
 } failure{*code,uploaded};
 const auto first=[&]{entry.first(false);physical_cuts();entry.first(false);};
 first();
 const auto full=[&]{first();source_cuts();first();};
 const GlApi api=code->api;const GlRenderApi render=code->render;
 auto*const node=code->nodes[0].get();const auto names=state->names;
 need(node&&node==state->node_identity&&api.GetError&&api.GetGraphicsResetStatus&&
  render.NamedBufferData&&render.VertexArrayVertexBuffer&&render.VertexArrayElementBuffer&&
  render.VertexArrayBindingDivisor&&render.EnableVertexArrayAttrib&&render.VertexArrayAttribBinding&&
  render.VertexArrayAttribFormat,"Queued upload actual staging dispatch/custody absent");
 need(state->all_vertices.size()<=static_cast<std::size_t>(std::numeric_limits<GLsizeiptr>::max())/sizeof(PositionedVertices::RawVertex)&&
  state->all_indices.size()<=static_cast<std::size_t>(std::numeric_limits<GLsizeiptr>::max())/sizeof(std::uint32_t),
  "Queued upload native byte transport overflow");
 const auto current=[&]{
  first();const auto*loaded=detail::BnwRendererContextPin::NormalAccess::api_current_without_switch_owned(pin);
  first();need(loaded&&loaded->GetError==api.GetError&&loaded->GetGraphicsResetStatus==api.GetGraphicsResetStatus,
   "Queued upload actual loaded dispatch changed");
  const auto actual=node->shader_resources_storage_without_callbacks();
  need(code->nodes[0].get()==node&&actual.program==names.program&&actual.stages==names.stages&&
   actual.vertices==names.vertices&&actual.indices==names.indices&&actual.vao==names.vao&&actual.sampler==names.sampler,
   "Queued upload actual Node names changed");first();
 };
 const auto status=[&]{
  current();const auto reset=api.GetGraphicsResetStatus();
  if(reset!=GL_NO_ERROR){code->submit_failed=true;throw std::runtime_error("Queued upload context reset");}
  current();const auto error=api.GetError();
  if(error!=GL_NO_ERROR){code->submit_failed=true;throw std::runtime_error("Queued upload GL error");}current();
 };
 const auto effect=[&](const auto& f){status();f();status();};
 // The exact genuine immutable all-pass CPU streams and original stride24
 // declaration are uploaded, without assuming entry grouping/consumption.
 full();status();
 effect([&]{render.NamedBufferData(names.vertices,static_cast<GLsizeiptr>(state->all_vertices.size()*sizeof(PositionedVertices::RawVertex)),state->all_vertices.data(),GL_STREAM_DRAW);});
 effect([&]{render.NamedBufferData(names.indices,static_cast<GLsizeiptr>(state->all_indices.size()*sizeof(std::uint32_t)),state->all_indices.data(),GL_STREAM_DRAW);});
 effect([&]{render.VertexArrayVertexBuffer(names.vao,0,names.vertices,0,24);});
 effect([&]{render.VertexArrayElementBuffer(names.vao,names.indices);});
 effect([&]{render.VertexArrayBindingDivisor(names.vao,0,0);});
 for(unsigned k=0;k<3;++k){
  effect([&,k]{render.EnableVertexArrayAttrib(names.vao,k);});
  effect([&,k]{render.VertexArrayAttribBinding(names.vao,k,0);});
  effect([&,k]{render.VertexArrayAttribFormat(names.vao,k,k==0?3:k==1?2:4,
   k==2?GL_UNSIGNED_BYTE:GL_FLOAT,k==2?GL_TRUE:GL_FALSE,k==0?0:k==1?16:12);});
 }
 first();uploaded=true;
 // The actual caller publishes its own uploaded flag only after this LAST
 // real GL/Root POST. CPU history and legacy State flags remain untouched.
}

std::unique_ptr<GlBnwFontShader::GpuStagingLease>
GlBnwFontShader::stage_queued_record_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,const ::civ5::ui::UiScreenGlRenderer& renderer,
 const ::civ5::ui::detail::BnwNormalRootBatchHistory::Guard& input_root,
 const ::civ5::ui::detail::BnwNormalItemOccurrence::Guard& input_item,
 const ::civ5::ui::detail::BnwNormalQueueDrainPermission::Guard& input_permission,
 detail::BnwNormalLabelDrawRecordBinding::Guard input_binding,
 const detail::BnwNormalLabelDrawRecordBinding::OpaqueRecordIdentity& input_record,
 const detail::BnwNormalLabelDrawRecordBinding::RendererDescriptorReceipt& input_descriptor){
 // Snapshot every closed control and association BEFORE callbacks/allocators.
 const auto root=input_root;const auto item=input_item;const auto permission=input_permission;
 const auto binding=input_binding;const auto record=input_record;const auto descriptor=input_descriptor;
 const auto record_impl=binding.impl_;need(record_impl&&record_impl->state,"Queued stage original Binding absent");
 const auto state=record_impl->state;state->physical_cpu_record_first();const auto code=state->code;
 const auto first=[&]{
  state->physical_cpu_record_first();GpuStagingLease::Impl::require_original_planes_without_callbacks(state,item);
  need(!code->submit_busy&&!code->submit_failed&&!code->submit_owner&&!code->queued_submit_owner,
   "Queued stage physical CODE already busy/failed");
  root.require_physical_current_without_callbacks();item.require_physical_current_without_callbacks();
  permission.require_physical_current_without_callbacks();
  root.require_actual_renderer_without_callbacks(renderer);item.require_actual_renderer_without_callbacks(renderer);
  permission.require_actual_renderer_without_callbacks(renderer);
  item.require_same_root_without_callbacks(root);permission.require_same_root_without_callbacks(root);
  item.require_actual_positioned_data_without_callbacks(state->use->data);
  item.require_actual_positioned_data_without_callbacks(state->vertices->data_guard);
  binding.require_physical_record_without_callbacks();record.require_physical_record_without_callbacks();
  record.require_same_binding_physical_without_callbacks(binding);
  descriptor.require_physical_record_without_callbacks();descriptor.require_same_record_physical_without_callbacks(record);
  descriptor.require_same_binding_physical_without_callbacks(binding);
  const auto canonical=item.captured_normal_draw_records_without_callbacks();
  record.require_same_binding_physical_without_callbacks(canonical);
  descriptor.require_same_binding_physical_without_callbacks(canonical);
  permission.require_queued_record_without_callbacks(item,record,descriptor);
  state->physical_cpu_record_first();GpuStagingLease::Impl::require_original_planes_without_callbacks(state,item);
  need(!code->submit_busy&&!code->submit_failed&&!code->submit_owner&&!code->queued_submit_owner,
   "Queued stage physical CODE changed at allocation PRE/POST");
 };
 const auto full=[&]{first();root.require_source_owned(operation);first();
  item.require_source_owned(operation);first();permission.require_source_owned(operation);first();};
 // No reservation/poison of shared CODE for allocation or Source rejection.
 full();first();std::unique_ptr<GpuStagingLease::Impl> provisional(new GpuStagingLease::Impl(
  state,&renderer,root,item,permission,binding,record,descriptor));first();full();
 first();std::unique_ptr<GpuStagingLease> result(new GpuStagingLease(std::move(provisional)));first();full();first();
 auto&entry=*result->impl_;
 // LAST Source/allocator POST has succeeded. Exact owner reservation is PURE
 // metadata commit with no allocator/callback between its three stores.
 code->submit_busy=true;code->queued_submit_owner=&entry;entry.reserved=true;
 entry.first(false);
 const auto physical=[&]{entry.first(false);};
 const auto source=[&]{entry.full(operation,false);};
 upload_prepared_record_transport_unpublished_owned(entry,physical,source);
 entry.first(false);entry.uploaded=true;entry.first();
 // One CURRENT selected record only. Every later record needs its own new
 // permission/lease; no three-consecutive-pass optimization is assumed.
 return result;
}

GlBnwFontShader::QueuedRecordTransportResult
GlBnwFontShader::submit_queued_record_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,const ::civ5::ui::UiScreenGlRenderer& renderer,
 GpuStagingLease& original_lease,detail::BnwLabelDefaultFontTargetLoan::Guard input_target,
 const ::civ5::ui::detail::BnwNormalLabelMaterialLoan::ShaderAccess& input_material){
 using Node=detail::BnwOwnedGlRetirementNode;using Pipeline=Node::FontPipelineChannel;
 // Capture actual original controls before any Source/allocation/native effect.
 // Capture SAME physical control before any Source/allocation/native callback.
 // Callback deletion of the original facade retires Life immediately; entry,
 // Exit and failure cleanup remain safe because this original control stays held.
 const auto retained_lease=original_lease.impl_;
 auto*const saved_lease=retained_lease.get();const auto target=input_target;
 const auto material_control=input_material.impl_;
 const auto selected=input_material.selected_;
 const auto selected_record=input_material.selected_record_;
 const auto selected_descriptor=input_material.selected_descriptor_;
 need(saved_lease&&material_control&&selected&&selected_record&&selected_descriptor,
  "Queued Draw original lease/actually selected Material controls absent");
 auto&entry=*saved_lease;entry.first();
 need(entry.renderer==&renderer&&!entry.submitting&&!entry.transport_returned&&!entry.transport_aborted,
  "Queued Draw actual selected transport recursive/repeated/aborted");
 const auto state=entry.state;const auto code=state->code;const auto pin=state->use->pin;
 selected->require_physical_current_without_callbacks();
 need(selected->impl_.get()==entry.permission.impl_.get()&&
  !selected->impl_.owner_before(entry.permission.impl_)&&!entry.permission.impl_.owner_before(selected->impl_),
  "Queued Draw supplied selected Material permission differs from original lease");
 selected_record->require_same_binding_physical_without_callbacks(entry.binding);
 selected_descriptor->require_same_record_physical_without_callbacks(entry.record);
 need(selected_record->pass_==entry.record.pass_&&selected_descriptor->write_occurrence_==entry.descriptor.write_occurrence_,
  "Queued Draw supplied selected scalar record/receipt occurrence differs");
 // Rebuild only the same captured two-arg selected view; one-arg loses
 // selected authority, and the noncopyable input is never kept by reference.
 const ::civ5::ui::detail::BnwNormalLabelMaterialLoan::ShaderAccess material(material_control,*selected);
 const auto first=[&]{
  entry.first();material.require_current_without_callbacks();target.require_current_without_callbacks();
  entry.root.require_actual_renderer_without_callbacks(renderer);
  material.require_selected_queue_association_without_callbacks(entry.root,entry.item,entry.permission,entry.record,entry.descriptor);
  target.require_same_positioned_data_without_callbacks(state->use->data);
  target.require_same_positioned_data_without_callbacks(state->vertices->data_guard);
  material.require_current_without_callbacks();target.require_current_without_callbacks();entry.first();
 };
 const auto full=[&]{first();entry.full(operation);first();
  material.require_source_owned(operation);first();target.require_source_owned(operation);first();
  target.require_active_owned();first();};
 first();entry.submitting=true;bool succeeded=false;
 struct Exit final {GpuStagingLease::Impl&lease;bool&success;
  ~Exit(){if(!success){lease.state->code->submit_failed=true;lease.transport_aborted=true;}
   lease.submitting=false;}}exit{entry,succeeded};
 Pipeline::Owner pipeline;
 try{
  full();first();auto provisional=std::make_shared<QueuedRecordTransportResult::Impl>(entry,target,material_control,*selected);
  first();full();first();
  // Immutable actual Root Matrix epoch replaces the retired ACTIVE Matrix.
  const auto words=entry.root.matrix_words_without_callbacks();
  const auto translation_words=material.ordered_translation_words_without_callbacks();
  const auto depth_words=material.depth_arg4_arg5_words_without_callbacks();
  const auto alpha_word=material.composed_alpha_word_without_callbacks();
  const auto allow_word=material.allow_color_word_without_callbacks();
  const auto selector=material.native_selector_without_callbacks();
  const auto viewport_depth=material.viewport_depth_words_without_callbacks();
  const auto dimensions=material.viewport_dimensions_without_callbacks();
  const auto numeric=material.numeric_controls_without_callbacks();
  const auto matrix_numeric=entry.root.numeric_controls_without_callbacks();
  const auto matrix_return_numeric=entry.root.numeric_return_controls_without_callbacks();
  const auto matrix_dimensions=entry.root.dimensions_without_callbacks();
  const auto matrix_epoch=entry.root.matrix_epoch_without_callbacks();
  const auto batch_revision=material.normal_batch_revision_without_callbacks();
  const auto actual_target=target.captured_target_data_without_callbacks();first();
  need(selector==1&&allow_word==0x3f800000u,"Queued Font Material selector1/positive170 AllowColor absent");
  need(numeric==state->vertices->numeric&&numeric==matrix_numeric&&numeric==matrix_return_numeric,
   "Queued Font actual Material/Root/vertex numeric controls differ");
  need(dimensions==matrix_dimensions&&dimensions==actual_target.drawable_extent&&dimensions[0]>0&&dimensions[1]>0,
   "Queued Font actual Root/Material/target dimensions differ");
  need(viewport_depth==std::array<std::uint32_t,2>{0u,0x3f800000u}&&
   std::bit_cast<std::uint64_t>(actual_target.preserved_depth[0])==0ull&&
   std::bit_cast<std::uint64_t>(actual_target.preserved_depth[1])==0x3ff0000000000000ull,
   "Queued Font actual zero-batch viewport depth differs");
  const auto rows=std::bit_cast<std::array<float,16>>(words);
  const auto translation=std::bit_cast<std::array<float,2>>(translation_words);
  const auto depth=std::bit_cast<std::array<float,2>>(depth_words);
  const auto alpha=std::bit_cast<float>(alpha_word),allow=std::bit_cast<float>(allow_word);
  const auto locations=code->uniform_locations[0];
  need(!locations.clip_fields,"Queued Font selector1 inherited selector4 layout");
  auto*const resource_node=code->nodes[0].get();need(resource_node,"Queued Font real resource Node absent");
  const auto names=resource_node->shader_resources_storage_without_callbacks();
  const GlApi api=code->api;const GlRenderApi render=code->render;
  const auto current=[&]{
   first();const auto*loaded=detail::BnwRendererContextPin::NormalAccess::api_current_without_switch_owned(pin);first();
   need(loaded&&loaded->GetError==api.GetError&&loaded->GetGraphicsResetStatus==api.GetGraphicsResetStatus&&
    loaded->GetIntegerv==api.GetIntegerv&&loaded->BindTextureUnit==api.BindTextureUnit,
    "Queued Font actual loaded dispatch changed");
   const auto actual=resource_node->shader_resources_storage_without_callbacks();
   need(code->nodes[0].get()==resource_node&&actual.program==names.program&&actual.stages==names.stages&&
    actual.vao==names.vao&&actual.vertices==names.vertices&&actual.indices==names.indices&&actual.sampler==names.sampler,
    "Queued Font exact real CODE Node custody changed");first();
  };
  const auto status=[&]{
   current();const auto reset=api.GetGraphicsResetStatus();
   if(reset!=GL_NO_ERROR){entry.actual_native_failure=true;entry.native_reset=reset;
    if(pipeline)pipeline->quarantine_font_pipeline_without_callbacks(Node::Failure::preexisting_reset);
    throw std::runtime_error("Queued Font actual context reset");}
   current();const auto error=api.GetError();
   if(error!=GL_NO_ERROR){entry.actual_native_failure=true;entry.native_error=error;
    if(pipeline)pipeline->quarantine_font_pipeline_without_callbacks(Node::Failure::preexisting_error);
    throw std::runtime_error("Queued Font actual GL error");}current();
  };
  const auto pre=[&]{full();status();};const auto post=[&]{status();full();status();};
  need(api.GetError&&api.GetGraphicsResetStatus&&api.GetIntegerv&&api.BindTextureUnit&&
   render.GetIntegeri_v&&render.GetBooleanv&&render.GetBooleani_v&&render.IsEnabledi&&render.IsEnabled&&
   render.Enablei&&render.Disablei&&render.BlendFuncSeparatei&&render.BlendEquationSeparatei&&render.ColorMaski&&
   render.Enable&&render.Disable&&render.DepthMask&&render.DepthFunc&&render.PolygonMode&&
   render.UseProgram&&render.BindVertexArray&&render.BindSampler&&render.NamedBufferData&&
   render.VertexArrayVertexBuffer&&render.VertexArrayElementBuffer&&render.EnableVertexArrayAttrib&&
   render.VertexArrayAttribBinding&&render.VertexArrayAttribFormat&&render.VertexArrayBindingDivisor&&
   render.ProgramUniform4fv&&render.ProgramUniform2fv&&render.ProgramUniform1f&&render.ProgramUniform1i&&render.DrawElements,
   "Queued Font actual selected transport dispatch absent");
  full();const auto pending=Pipeline::drain();first();
  need(!pending.incomplete&&!pending.pending_creator_nodes&&!pending.cleanup_failure,
   "Queued Font actual prior pipeline cleanup pending/failed");
  pre();pipeline=Pipeline::make(pin,api,render);first();post();
  auto&saved=Pipeline::storage(*pipeline);full();status();
  // Preallocated actual storage receives query outputs directly. Each native
  // observation has independent PURE/current/error/reset PRE/POST only.
  status();render.GetIntegeri_v(GL_BLEND_SRC_RGB,0,&saved.blend_factors[0]);status();
  status();render.GetIntegeri_v(GL_BLEND_DST_RGB,0,&saved.blend_factors[1]);status();
  status();render.GetIntegeri_v(GL_BLEND_SRC_ALPHA,0,&saved.blend_factors[2]);status();
  status();render.GetIntegeri_v(GL_BLEND_DST_ALPHA,0,&saved.blend_factors[3]);status();
  saved.blend_factors_captured=true;
  status();render.GetIntegeri_v(GL_BLEND_EQUATION_RGB,0,&saved.blend_equations[0]);status();
  status();render.GetIntegeri_v(GL_BLEND_EQUATION_ALPHA,0,&saved.blend_equations[1]);status();
  saved.blend_equations_captured=true;
  status();render.GetBooleani_v(GL_COLOR_WRITEMASK,0,saved.color_mask.data());status();saved.color_mask_captured=true;
  status();saved.blend_enabled=render.IsEnabledi(GL_BLEND,0);status();saved.blend_enabled_captured=true;
  status();saved.depth_enabled=render.IsEnabled(GL_DEPTH_TEST);status();saved.depth_enabled_captured=true;
  status();saved.cull_enabled=render.IsEnabled(GL_CULL_FACE);status();saved.cull_enabled_captured=true;
  status();saved.stencil_enabled=render.IsEnabled(GL_STENCIL_TEST);status();saved.stencil_enabled_captured=true;
  status();render.GetBooleanv(GL_DEPTH_WRITEMASK,&saved.depth_mask);status();saved.depth_mask_captured=true;
  status();api.GetIntegerv(GL_DEPTH_FUNC,&saved.depth_func);status();saved.depth_func_captured=true;
  status();api.GetIntegerv(GL_CURRENT_PROGRAM,&saved.program);status();saved.program_captured=true;
  status();api.GetIntegerv(GL_VERTEX_ARRAY_BINDING,&saved.vao);status();saved.vao_captured=true;
  status();api.GetIntegerv(GL_ACTIVE_TEXTURE,&saved.active_texture);status();saved.active_texture_captured=true;
  status();api.GetIntegerv(GL_POLYGON_MODE,saved.polygon_mode.data());status();saved.polygon_mode_captured=true;
  status();render.GetIntegeri_v(GL_TEXTURE_BINDING_2D,0,&saved.textures[0]);status();saved.textures_captured[0]=true;
  status();render.GetIntegeri_v(GL_SAMPLER_BINDING,0,&saved.samplers[0]);status();saved.samplers_captured[0]=true;
  status();render.GetIntegeri_v(GL_TEXTURE_BINDING_2D,1,&saved.textures[1]);status();saved.textures_captured[1]=true;
  status();render.GetIntegeri_v(GL_SAMPLER_BINDING,1,&saved.samplers[1]);status();saved.samplers_captured[1]=true;
  // Core GL can only restore both polygon faces together. Refuse BEFORE change.
  need(saved.polygon_mode[0]==saved.polygon_mode[1],"Normal Font unsupported unequal core polygon faces");
  // This production lane does not mutate scissor/sRGB/MSAA/stencil ops/cull mode.
  // Restart/discard are unsupported backend preconditions, not claimed native
  // inherited policies and are never assigned diagnostic defaults.
  status();const auto restart=render.IsEnabled(GL_PRIMITIVE_RESTART);status();
  status();const auto fixed_restart=render.IsEnabled(GL_PRIMITIVE_RESTART_FIXED_INDEX);status();
  status();const auto discard=render.IsEnabled(GL_RASTERIZER_DISCARD);status();
  need(!restart&&!fixed_restart&&!discard,"Normal Font unsupported restart/discard physical phase");
  // Actual Material still owns selected normal zero-batch/native fixed-state
  // applicability. Source proofs do not mint that capability here.
  // Native effect boundary only. Calls are concrete internal typed setters;
  // this is not a caller callback or an authority factory.
  const auto native_effect=[&](const auto& invoke){status();invoke();status();};
  // One genuine CURRENT selected record only. Pass ordinal comes from its
  // sealed Graph identity, never a caller integer or a three-pass loop.
  const unsigned pass=entry.record.pass_;first();need(pass<3,"Queued Font selected pass outside actual3");
  bool draw_returned=false;
   const auto range=state->ranges[pass];
   const auto vertex_view=std::span<const PositionedVertices::RawVertex>(state->all_vertices).subspan(range.vertex_begin,range.vertex_count);
   const auto index_view=std::span<const std::uint32_t>(state->all_indices).subspan(range.index_begin,range.index_count);first();
   need(!index_view.empty()&&!vertex_view.empty(),"Queued Font selected opaque pass is empty");
   need(!vertex_view.empty()&&index_view.size()<=static_cast<std::size_t>(std::numeric_limits<GLsizei>::max())&&
    vertex_view.size_bytes()<=static_cast<std::size_t>(std::numeric_limits<GLsizeiptr>::max())&&
    index_view.size_bytes()<=static_cast<std::size_t>(std::numeric_limits<GLsizeiptr>::max()),
    "Normal Font genuine bucket exceeds native GL transport");
   const auto stages=state->stages[pass];first();
   need(stages.native_format==9&&stages.textures[0]&&stages.textures[0]==stages.textures[1],
    "Normal Font genuine selected plane format/stages differ");
   // LAST callbackful admission for this pass. Everything through Draw and
   // native POST below is native/PURE only. Reapply actual state/data instead
   // of assuming an earlier Source observer left them unchanged.
   full();status();
   // Original copied DWORD vertices/indices overwrite possible observer edits.
   native_effect([&]{render.NamedBufferData(names.vertices,static_cast<GLsizeiptr>(state->all_vertices.size()*sizeof(PositionedVertices::RawVertex)),state->all_vertices.data(),GL_STREAM_DRAW);});
   native_effect([&]{render.NamedBufferData(names.indices,static_cast<GLsizeiptr>(state->all_indices.size()*sizeof(std::uint32_t)),state->all_indices.data(),GL_STREAM_DRAW);});
   // Reinstall the exact real stride24/format9 declaration, including binding0
   // divisor0. Existing cached names alone cannot prove mutable VAO state.
   native_effect([&]{render.VertexArrayVertexBuffer(names.vao,0,names.vertices,static_cast<GLintptr>(range.vertex_begin*sizeof(PositionedVertices::RawVertex)),24);});
   native_effect([&]{render.VertexArrayElementBuffer(names.vao,names.indices);});
   native_effect([&]{render.VertexArrayBindingDivisor(names.vao,0,0);});
   for(unsigned k=0;k<3;++k){
    native_effect([&,k]{render.EnableVertexArrayAttrib(names.vao,k);});
    native_effect([&,k]{render.VertexArrayAttribBinding(names.vao,k,0);});
    native_effect([&,k]{render.VertexArrayAttribFormat(names.vao,k,k==0?3:k==1?2:4,
      k==2?GL_UNSIGNED_BYTE:GL_FLOAT,k==2?GL_TRUE:GL_FALSE,k==0?0:k==1?16:12);});
   }
   native_effect([&]{saved.blend_factors_changed=true;render.BlendFuncSeparatei(0,GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA,GL_ONE,GL_ONE);});
   native_effect([&]{saved.blend_equations_changed=true;render.BlendEquationSeparatei(0,GL_FUNC_ADD,GL_FUNC_ADD);});
   native_effect([&]{saved.color_mask_changed=true;render.ColorMaski(0,GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);});
   native_effect([&]{saved.blend_enabled_changed=true;render.Enablei(GL_BLEND,0);});
   native_effect([&]{saved.depth_func_changed=true;render.DepthFunc(GL_LEQUAL);});
   native_effect([&]{saved.depth_mask_changed=true;render.DepthMask(GL_FALSE);});
   native_effect([&]{saved.depth_enabled_changed=true;render.Disable(GL_DEPTH_TEST);});
   native_effect([&]{saved.cull_enabled_changed=true;render.Disable(GL_CULL_FACE);});
   native_effect([&]{saved.stencil_enabled_changed=true;render.Disable(GL_STENCIL_TEST);});
   native_effect([&]{saved.polygon_mode_changed=true;render.PolygonMode(GL_FRONT_AND_BACK,GL_FILL);});
   native_effect([&]{saved.program_changed=true;render.UseProgram(names.program);});
   native_effect([&]{saved.vao_changed=true;render.BindVertexArray(names.vao);});
   native_effect([&]{render.ProgramUniform4fv(names.program,locations.rows,4,rows.data());});
   native_effect([&]{render.ProgramUniform2fv(names.program,locations.translation,1,translation.data());});
   native_effect([&]{render.ProgramUniform2fv(names.program,locations.depth,1,depth.data());});
   native_effect([&]{render.ProgramUniform1f(names.program,locations.parent_alpha,alpha);});
   native_effect([&]{render.ProgramUniform1f(names.program,locations.allow_color,allow);});
   native_effect([&]{render.ProgramUniform1i(names.program,locations.block_texture,1);});
   for(unsigned unit=0;unit<2;++unit){
    native_effect([&,unit]{saved.textures_changed[unit]=true;api.BindTextureUnit(unit,stages.textures[unit]);});
   }
   for(unsigned unit=0;unit<2;++unit){
    native_effect([&,unit]{saved.samplers_changed[unit]=true;render.BindSampler(unit,names.sampler);});
   }
   // These unsupported states are observed freshly after LAST FULL. They are
   // never silently changed to diagnostic defaults by this producer.
   status();const auto current_restart=render.IsEnabled(GL_PRIMITIVE_RESTART);status();
   status();const auto current_fixed_restart=render.IsEnabled(GL_PRIMITIVE_RESTART_FIXED_INDEX);status();
   status();const auto current_discard=render.IsEnabled(GL_RASTERIZER_DISCARD);status();
   need(!current_restart&&!current_fixed_restart&&!current_discard,
    "Normal Font unsupported terminal restart/discard physical phase");
   // No Source callback follows replay before real Draw or its native POST.
   native_effect([&]{render.DrawElements(GL_TRIANGLES,static_cast<GLsizei>(index_view.size()),GL_UNSIGNED_INT,reinterpret_cast<const void*>(static_cast<std::uintptr_t>(range.index_begin*sizeof(std::uint32_t))));draw_returned=true;});
   full();status();
  full();first();const auto restored=Pipeline::restore(*pipeline);
  need(restored.complete&&!restored.cleanup_failure,"Queued Font actual pipeline restoration failed/incomplete");
  pipeline.reset();const auto drained=Pipeline::drain();first();
  need(!drained.incomplete&&!drained.pending_creator_nodes&&!drained.cleanup_failure,
   "Queued Font real final pipeline cleanup pending/failed");
  full();status();first();
  need(draw_returned&&entry.root.matrix_epoch_without_callbacks()==matrix_epoch&&
   material.normal_batch_revision_without_callbacks()==batch_revision,
   "Queued Font actual Draw return/Root Matrix/Material generation changed");first();
  // No allocator/Source/GL lies between the final actual POST and this private
  // immutable result seal. Ui Rows restoration/Root completion is NOT issued.
  provisional->outcome=QueuedRecordTransportResult::Impl::Outcome::actual_draw_issued;
  entry.transport_returned=true;provisional->sealed=true;succeeded=true;
  return QueuedRecordTransportResult(std::move(provisional));
 }catch(...){
  const auto original=std::current_exception();code->submit_failed=true;entry.transport_aborted=true;
  // Real native reset/error status is retained privately above. Source,
  // permission or restoration failures cannot synthesize a native outcome.
  // Healthy record/Material/Rows checks reject CODE poison, so until the owner
  // supplies a distinct failed-command protocol this path throws/no result.
  if(pipeline){const auto ignored=Pipeline::restore(*pipeline);(void)ignored;pipeline.reset();}
  const auto ignored=Pipeline::drain();(void)ignored;std::rethrow_exception(original);
 }
}

::civ5::ui::detail::BnwNormalQueueRecordCompletion
GlBnwFontShader::complete_queued_record_after_rows_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,const ::civ5::ui::UiScreenGlRenderer& renderer,
 GpuStagingLease& original_lease,const QueuedRecordTransportResult& input_result,
 const detail::BnwLabelDefaultFontTargetLoan::RowsRestorationWitness& input_witness){
 using Completion=::civ5::ui::detail::BnwNormalQueueRecordCompletion;
 // The result retains life/control identity; it never dereferences a retired
 // original lease pointer. Incoming original facade is captured independently.
 // Retain SAME physical control before FULL/Completion callbacks. Original
 // facade retirement remains observable via Life and cannot earn completion.
 const auto retained_lease=original_lease.impl_;
 auto*const actual_entry=retained_lease.get();const auto result=input_result.impl_;
 const auto witness=input_witness;
 need(actual_entry&&result,"Queued completion original lease/result absent");
 auto&entry=*actual_entry;
 need(result->creator==std::this_thread::get_id()&&result->sealed&&
  result->outcome==QueuedRecordTransportResult::Impl::Outcome::actual_draw_issued&&
  result->original_lease_life&&result->original_lease_life->alive.load()&&
  result->original_lease_life.get()==entry.original_life.get()&&
  !result->original_lease_life.owner_before(entry.original_life)&&
  !entry.original_life.owner_before(result->original_lease_life)&&
  result->lease_identity==&entry&&result->original_lease_life->original==&entry&&
  result->state.get()==entry.state.get()&&!result->state.owner_before(entry.state)&&
  !entry.state.owner_before(result->state),"Queued completion another or retired genuine original transport");
 const auto first=[&]{
  entry.first();
  need(entry.renderer==&renderer&&entry.transport_returned&&!entry.submitting&&!entry.transport_aborted&&
   !entry.actual_native_failure,"Queued completion requires actual healthy selected Draw return");
  entry.root.require_same_root_without_callbacks(result->root);
  entry.item.require_same_item_without_callbacks(result->item);
  need(entry.permission.impl_.get()==result->permission.impl_.get()&&
   !entry.permission.impl_.owner_before(result->permission.impl_)&&
   !result->permission.impl_.owner_before(entry.permission.impl_),
   "Queued completion result belongs to another selected permission");
  result->record.require_same_binding_physical_without_callbacks(entry.binding);
  result->descriptor.require_same_record_physical_without_callbacks(entry.record);
  need(result->record.pass_==entry.record.pass_&&
   result->descriptor.write_occurrence_==entry.descriptor.write_occurrence_,
   "Queued completion result has another selected pass/descriptor occurrence");
  result->material.require_selected_queue_association_without_callbacks(
   entry.root,entry.item,entry.permission,entry.record,entry.descriptor);
  witness.require_current_without_callbacks();
  // Retired Loan control association ONLY. Actual restored Ui provider does
  // not call old target Guard::current/alive or the entered Rows phase.
  witness.require_same_original_font_target_without_callbacks(result->target);
  result->material.require_current_without_callbacks();entry.first();
 };
 const auto full=[&]{first();entry.full(operation);first();
  result->material.require_source_owned(operation);first();
  witness.require_source_owned(operation);first();};
 // Wrong/retired result rejection above cannot poison another lease. Once
 // this exact original transport reaches completion, failure remains sticky.
 first();const auto code=entry.state->code;
 try{
  full();first();
  const Completion::ConstructionKey key;
  auto completed=Completion(entry.permission,entry.item,entry.record,entry.descriptor,
   Completion::Outcome::actual_draw_issued,key);
  first();completed.require_current_without_callbacks();
  completed.require_same_permission_without_callbacks(entry.permission);first();
  completed.require_source_owned(operation);first();
  // Witness FULL is the LAST callbackful cut: fresh actual restored Ui
  // baseline tuple is verified after all earlier Source observers.
  full();first();completed.require_current_without_callbacks();
  completed.require_same_permission_without_callbacks(entry.permission);first();
  // Genuine Draw result + actual Rows restoration have earned the closed
  // Root VALUE. Pure exact-owner release precedes Root callback RETURN/advance.
  entry.consumed_after_restore=true;code->queued_submit_owner=nullptr;
  code->submit_busy=false;entry.reserved=false;
  return completed;
 }catch(...){code->submit_failed=true;entry.transport_aborted=true;throw;}
}

// Direct normal Label18 / original selector1 only. Material/API definitions
// and the real Root issuer are mandatory; this entry never constructs them.
void GlBnwFontShader::submit_normal_label_owned(
 const runtime::BnwWorldViewSourceOperationLoan& operation,
 const ::civ5::ui::UiScreenGlRenderer& renderer,
 const ::civ5::ui::BnwWorldViewControlRenderPublication& publication,std::size_t item_index,
 const ::civ5::ui::BnwWorldViewControlTree& tree,std::uint64_t control_id,
 detail::BnwNormalLabelDrawRecordBinding::Guard record_binding,
 detail::BnwLabelDefaultFontTargetLoan::Guard target,
 const ::civ5::ui::detail::BnwWindowedRendererMatrixDataLoan::ShaderAccess& matrix,
 const ::civ5::ui::detail::BnwNormalLabelMaterialLoan::ShaderAccess& material){
 using Node=detail::BnwOwnedGlRetirementNode;
 using Pipeline=Node::FontPipelineChannel;
 // The ONLY Binding Guard read precedes every effect. Its State holds the
 // original planes facade and original independently alive Use/Vertices leases.
 const auto record_impl=record_binding.impl_;need(bool(record_impl),"Normal Font actual DrawRecord guard absent");
 const auto state=record_impl->state;const auto use=state->use;const auto vertices=state->vertices;
 const auto code=state->code;const auto& planes=state->planes;
 need(state->creator==std::this_thread::get_id(),"Normal Font DrawRecord foreign creator");
 if(state->submitting||state->submitted||!state->owns_lease||!code->submit_busy||code->submit_failed||code->submit_owner!=state.get()){
  code->submit_failed=true;throw std::runtime_error("Normal Font DrawRecord recursive/repeated/poisoned submit");}
 state->submitting=true;bool succeeded=false;
 struct Exit final {DrawRecordState& state;bool& succeeded;
  ~Exit(){if(!succeeded)state.code->submit_failed=true;state.submitting=false;}} exit{*state,succeeded};
 const auto data=use->data;const auto atlas=use->atlas;const auto pin=use->pin;
 Pipeline::Owner pipeline;
 try{
  state->first();
  // Raw borrowed associations are used exactly once, before first callback.
  material.require_actual_association_without_callbacks(renderer,publication,item_index,tree,control_id,data,matrix);
  data.require_actual_renderer_without_callbacks(renderer);
  need(data.impl_.get()==vertices->data_guard.impl_.get()&&
   !data.impl_.owner_before(vertices->data_guard.impl_)&&!vertices->data_guard.impl_.owner_before(data.impl_),
   "Normal Font submit vertex DATA differs from actual material/use DATA");
  target.require_same_positioned_data_without_callbacks(data);
  const auto first=[&]{
   state->first();material.require_current_without_callbacks();matrix.require_current_without_callbacks();
   data.require_current_without_callbacks();vertices->first();target.require_current_without_callbacks();
   need(code->submit_busy&&!code->submit_failed,"Normal Font submit gate poisoned");
   use->first();planes->require_current_without_callbacks();planes->require_actual_atlas_without_callbacks(*atlas);
   target.require_same_positioned_data_without_callbacks(data);
   material.require_current_without_callbacks();matrix.require_current_without_callbacks();
   data.require_current_without_callbacks();vertices->first();target.require_current_without_callbacks();
   state->first();need(code->submit_busy&&!code->submit_failed&&code->submit_owner==state.get(),"Normal Font submit gate poisoned at POST");
  };
  first();
  const auto words=matrix.matrix_words_without_callbacks();
  const auto translation_words=material.ordered_translation_words_without_callbacks();
  const auto depth_words=material.depth_arg4_arg5_words_without_callbacks();
  const auto alpha_word=material.composed_alpha_word_without_callbacks();
  const auto allow_word=material.allow_color_word_without_callbacks();
  const auto selector=material.native_selector_without_callbacks();
  const auto viewport_depth=material.viewport_depth_words_without_callbacks();
  const auto dimensions=material.viewport_dimensions_without_callbacks();
  const auto numeric=material.numeric_controls_without_callbacks();
  const auto matrix_numeric=matrix.numeric_controls_without_callbacks();
  const auto matrix_return_numeric=matrix.numeric_return_controls_without_callbacks();
  const auto matrix_dimensions=matrix.dimensions_without_callbacks();
  const auto matrix_revision=matrix.manager_revision_without_callbacks();
  const auto batch_revision=material.normal_batch_revision_without_callbacks();
  const auto actual_target=target.captured_target_data_without_callbacks();first();
  need(selector==1&&allow_word==0x3f800000u,"Normal Font material did not earn selector1/positive170 AllowColor");
  need(numeric==vertices->numeric&&numeric==matrix_numeric&&numeric==matrix_return_numeric,
   "Normal Font material/matrix/vertex numeric controls differ");
  need(dimensions==matrix_dimensions&&dimensions==actual_target.drawable_extent&&dimensions[0]>0&&dimensions[1]>0,
   "Normal Font actual matrix/material/target dimensions differ");
  need(viewport_depth==std::array<std::uint32_t,2>{0u,0x3f800000u}&&
   std::bit_cast<std::uint64_t>(actual_target.preserved_depth[0])==0ull&&
   std::bit_cast<std::uint64_t>(actual_target.preserved_depth[1])==0x3ff0000000000000ull,
   "Normal Font target depth does not match genuine zero-batch viewport");
  const auto rows=std::bit_cast<std::array<float,16>>(words);
  const auto translation=std::bit_cast<std::array<float,2>>(translation_words);
  const auto depth=std::bit_cast<std::array<float,2>>(depth_words);
  const auto alpha=std::bit_cast<float>(alpha_word),allow=std::bit_cast<float>(allow_word);
  const auto locations=code->uniform_locations[0];
  need(!locations.clip_fields,"Normal Font selector1 inherited selector4 clip layout");
  auto*const resource_node=code->nodes[0].get();
  need(resource_node,"Normal Font actual selector1 resource node absent");
  const auto names=resource_node->shader_resources_storage_without_callbacks();
  const GlApi api=code->api;const GlRenderApi render=code->render;
  const auto full=[&]{
   first();use->data.require_source_owned(operation);first();
   use->paint.require_source_owned(operation);first();use->early.require_source_owned(operation);first();
   vertices->data_guard.require_source_owned(operation);first();
   material.require_source_owned(operation);first();matrix.require_source_owned(operation);first();
   target.require_source_owned(operation);first();target.require_active_owned();first();
  };
  const auto current=[&]{
   first();const auto*loaded=detail::BnwRendererContextPin::NormalAccess::api_current_without_switch_owned(pin);first();
   need(loaded&&loaded->GetError==api.GetError&&loaded->GetGraphicsResetStatus==api.GetGraphicsResetStatus&&
    loaded->GetIntegerv==api.GetIntegerv&&loaded->BindTextureUnit==api.BindTextureUnit,
    "Normal Font submit loaded normal dispatch changed");
   const auto actual=resource_node->shader_resources_storage_without_callbacks();
   need(code->nodes[0].get()==resource_node&&actual.program==names.program&&actual.vao==names.vao&&
    actual.vertices==names.vertices&&actual.indices==names.indices&&actual.sampler==names.sampler,
    "Normal Font selector1 physical resource custody changed");first();
  };
  const auto status=[&]{
   current();const auto reset=api.GetGraphicsResetStatus();
   // Record consumed failure BEFORE any possibly throwing logical POST.
   if(reset!=GL_NO_ERROR){if(pipeline)pipeline->quarantine_font_pipeline_without_callbacks(Node::Failure::preexisting_reset);
    throw std::runtime_error("Normal Font submit context reset");}
   current();const auto error=api.GetError();
   if(error!=GL_NO_ERROR){if(pipeline)pipeline->quarantine_font_pipeline_without_callbacks(Node::Failure::preexisting_error);
    throw std::runtime_error("Normal Font submit GL error");}current();
  };
  const auto pre=[&]{full();status();};
  const auto post=[&]{status();full();status();};
  need(api.GetError&&api.GetGraphicsResetStatus&&api.GetIntegerv&&api.BindTextureUnit&&
   render.GetIntegeri_v&&render.GetBooleanv&&render.GetBooleani_v&&render.IsEnabledi&&render.IsEnabled&&
   render.Enablei&&render.Disablei&&render.BlendFuncSeparatei&&render.BlendEquationSeparatei&&render.ColorMaski&&
   render.Enable&&render.Disable&&render.DepthMask&&render.DepthFunc&&render.PolygonMode&&
   render.UseProgram&&render.BindVertexArray&&render.BindSampler&&render.NamedBufferData&&
   render.VertexArrayVertexBuffer&&render.VertexArrayElementBuffer&&render.EnableVertexArrayAttrib&&
   render.VertexArrayAttribBinding&&render.VertexArrayAttribFormat&&render.VertexArrayBindingDivisor&&
   render.ProgramUniform4fv&&render.ProgramUniform2fv&&render.ProgramUniform1f&&render.ProgramUniform1i&&render.DrawElements,
   "Normal Font loaded submission dispatch unavailable");
  full();const auto pending=Pipeline::drain();first();
  need(!pending.incomplete&&!pending.pending_creator_nodes&&!pending.cleanup_failure,
   "Normal Font previous physical cleanup pending/failed");
  pre();pipeline=Pipeline::make(pin,api,render);first();post();
  auto& saved=Pipeline::storage(*pipeline);
  // ONE admission, then a contiguous native/PURE capture. No Source callback
  // separates saved fields, so the restoration baseline is one physical epoch.
  full();status();
  // Preallocated actual storage receives query outputs directly. Each native
  // observation has independent PURE/current/error/reset PRE/POST only.
  status();render.GetIntegeri_v(GL_BLEND_SRC_RGB,0,&saved.blend_factors[0]);status();
  status();render.GetIntegeri_v(GL_BLEND_DST_RGB,0,&saved.blend_factors[1]);status();
  status();render.GetIntegeri_v(GL_BLEND_SRC_ALPHA,0,&saved.blend_factors[2]);status();
  status();render.GetIntegeri_v(GL_BLEND_DST_ALPHA,0,&saved.blend_factors[3]);status();
  saved.blend_factors_captured=true;
  status();render.GetIntegeri_v(GL_BLEND_EQUATION_RGB,0,&saved.blend_equations[0]);status();
  status();render.GetIntegeri_v(GL_BLEND_EQUATION_ALPHA,0,&saved.blend_equations[1]);status();
  saved.blend_equations_captured=true;
  status();render.GetBooleani_v(GL_COLOR_WRITEMASK,0,saved.color_mask.data());status();saved.color_mask_captured=true;
  status();saved.blend_enabled=render.IsEnabledi(GL_BLEND,0);status();saved.blend_enabled_captured=true;
  status();saved.depth_enabled=render.IsEnabled(GL_DEPTH_TEST);status();saved.depth_enabled_captured=true;
  status();saved.cull_enabled=render.IsEnabled(GL_CULL_FACE);status();saved.cull_enabled_captured=true;
  status();saved.stencil_enabled=render.IsEnabled(GL_STENCIL_TEST);status();saved.stencil_enabled_captured=true;
  status();render.GetBooleanv(GL_DEPTH_WRITEMASK,&saved.depth_mask);status();saved.depth_mask_captured=true;
  status();api.GetIntegerv(GL_DEPTH_FUNC,&saved.depth_func);status();saved.depth_func_captured=true;
  status();api.GetIntegerv(GL_CURRENT_PROGRAM,&saved.program);status();saved.program_captured=true;
  status();api.GetIntegerv(GL_VERTEX_ARRAY_BINDING,&saved.vao);status();saved.vao_captured=true;
  status();api.GetIntegerv(GL_ACTIVE_TEXTURE,&saved.active_texture);status();saved.active_texture_captured=true;
  status();api.GetIntegerv(GL_POLYGON_MODE,saved.polygon_mode.data());status();saved.polygon_mode_captured=true;
  status();render.GetIntegeri_v(GL_TEXTURE_BINDING_2D,0,&saved.textures[0]);status();saved.textures_captured[0]=true;
  status();render.GetIntegeri_v(GL_SAMPLER_BINDING,0,&saved.samplers[0]);status();saved.samplers_captured[0]=true;
  status();render.GetIntegeri_v(GL_TEXTURE_BINDING_2D,1,&saved.textures[1]);status();saved.textures_captured[1]=true;
  status();render.GetIntegeri_v(GL_SAMPLER_BINDING,1,&saved.samplers[1]);status();saved.samplers_captured[1]=true;
  // Core GL can only restore both polygon faces together. Refuse BEFORE change.
  need(saved.polygon_mode[0]==saved.polygon_mode[1],"Normal Font unsupported unequal core polygon faces");
  // This production lane does not mutate scissor/sRGB/MSAA/stencil ops/cull mode.
  // Restart/discard are unsupported backend preconditions, not claimed native
  // inherited policies and are never assigned diagnostic defaults.
  status();const auto restart=render.IsEnabled(GL_PRIMITIVE_RESTART);status();
  status();const auto fixed_restart=render.IsEnabled(GL_PRIMITIVE_RESTART_FIXED_INDEX);status();
  status();const auto discard=render.IsEnabled(GL_RASTERIZER_DISCARD);status();
  need(!restart&&!fixed_restart&&!discard,"Normal Font unsupported restart/discard physical phase");
  // Actual Material still owns selected normal zero-batch/native fixed-state
  // applicability. Source proofs do not mint that capability here.
  // Native effect boundary only. Calls are concrete internal typed setters;
  // this is not a caller callback or an authority factory.
  const auto native_effect=[&](const auto& invoke){status();invoke();status();};
  for(unsigned remaining=3;remaining!=0;){
   const unsigned pass=--remaining;
   const auto range=state->ranges[pass];
   const auto vertex_view=std::span<const PositionedVertices::RawVertex>(state->all_vertices).subspan(range.vertex_begin,range.vertex_count);
   const auto index_view=std::span<const std::uint32_t>(state->all_indices).subspan(range.index_begin,range.index_count);first();
   if(index_view.empty()){need(vertex_view.empty(),"Normal Font empty index bucket has vertices");continue;}
   need(!vertex_view.empty()&&index_view.size()<=static_cast<std::size_t>(std::numeric_limits<GLsizei>::max())&&
    vertex_view.size_bytes()<=static_cast<std::size_t>(std::numeric_limits<GLsizeiptr>::max())&&
    index_view.size_bytes()<=static_cast<std::size_t>(std::numeric_limits<GLsizeiptr>::max()),
    "Normal Font genuine bucket exceeds native GL transport");
   const auto stages=state->stages[pass];first();
   need(stages.native_format==9&&stages.textures[0]&&stages.textures[0]==stages.textures[1],
    "Normal Font genuine selected plane format/stages differ");
   // LAST callbackful admission for this pass. Everything through Draw and
   // native POST below is native/PURE only. Reapply actual state/data instead
   // of assuming an earlier Source observer left them unchanged.
   full();status();
   // Original copied DWORD vertices/indices overwrite possible observer edits.
   native_effect([&]{render.NamedBufferData(names.vertices,static_cast<GLsizeiptr>(state->all_vertices.size()*sizeof(PositionedVertices::RawVertex)),state->all_vertices.data(),GL_STREAM_DRAW);});
   native_effect([&]{render.NamedBufferData(names.indices,static_cast<GLsizeiptr>(state->all_indices.size()*sizeof(std::uint32_t)),state->all_indices.data(),GL_STREAM_DRAW);});
   // Reinstall the exact real stride24/format9 declaration, including binding0
   // divisor0. Existing cached names alone cannot prove mutable VAO state.
   native_effect([&]{render.VertexArrayVertexBuffer(names.vao,0,names.vertices,static_cast<GLintptr>(range.vertex_begin*sizeof(PositionedVertices::RawVertex)),24);});
   native_effect([&]{render.VertexArrayElementBuffer(names.vao,names.indices);});
   native_effect([&]{render.VertexArrayBindingDivisor(names.vao,0,0);});
   for(unsigned k=0;k<3;++k){
    native_effect([&,k]{render.EnableVertexArrayAttrib(names.vao,k);});
    native_effect([&,k]{render.VertexArrayAttribBinding(names.vao,k,0);});
    native_effect([&,k]{render.VertexArrayAttribFormat(names.vao,k,k==0?3:k==1?2:4,
      k==2?GL_UNSIGNED_BYTE:GL_FLOAT,k==2?GL_TRUE:GL_FALSE,k==0?0:k==1?16:12);});
   }
   native_effect([&]{saved.blend_factors_changed=true;render.BlendFuncSeparatei(0,GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA,GL_ONE,GL_ONE);});
   native_effect([&]{saved.blend_equations_changed=true;render.BlendEquationSeparatei(0,GL_FUNC_ADD,GL_FUNC_ADD);});
   native_effect([&]{saved.color_mask_changed=true;render.ColorMaski(0,GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);});
   native_effect([&]{saved.blend_enabled_changed=true;render.Enablei(GL_BLEND,0);});
   native_effect([&]{saved.depth_func_changed=true;render.DepthFunc(GL_LEQUAL);});
   native_effect([&]{saved.depth_mask_changed=true;render.DepthMask(GL_FALSE);});
   native_effect([&]{saved.depth_enabled_changed=true;render.Disable(GL_DEPTH_TEST);});
   native_effect([&]{saved.cull_enabled_changed=true;render.Disable(GL_CULL_FACE);});
   native_effect([&]{saved.stencil_enabled_changed=true;render.Disable(GL_STENCIL_TEST);});
   native_effect([&]{saved.polygon_mode_changed=true;render.PolygonMode(GL_FRONT_AND_BACK,GL_FILL);});
   native_effect([&]{saved.program_changed=true;render.UseProgram(names.program);});
   native_effect([&]{saved.vao_changed=true;render.BindVertexArray(names.vao);});
   native_effect([&]{render.ProgramUniform4fv(names.program,locations.rows,4,rows.data());});
   native_effect([&]{render.ProgramUniform2fv(names.program,locations.translation,1,translation.data());});
   native_effect([&]{render.ProgramUniform2fv(names.program,locations.depth,1,depth.data());});
   native_effect([&]{render.ProgramUniform1f(names.program,locations.parent_alpha,alpha);});
   native_effect([&]{render.ProgramUniform1f(names.program,locations.allow_color,allow);});
   native_effect([&]{render.ProgramUniform1i(names.program,locations.block_texture,1);});
   for(unsigned unit=0;unit<2;++unit){
    native_effect([&,unit]{saved.textures_changed[unit]=true;api.BindTextureUnit(unit,stages.textures[unit]);});
   }
   for(unsigned unit=0;unit<2;++unit){
    native_effect([&,unit]{saved.samplers_changed[unit]=true;render.BindSampler(unit,names.sampler);});
   }
   // These unsupported states are observed freshly after LAST FULL. They are
   // never silently changed to diagnostic defaults by this producer.
   status();const auto current_restart=render.IsEnabled(GL_PRIMITIVE_RESTART);status();
   status();const auto current_fixed_restart=render.IsEnabled(GL_PRIMITIVE_RESTART_FIXED_INDEX);status();
   status();const auto current_discard=render.IsEnabled(GL_RASTERIZER_DISCARD);status();
   need(!current_restart&&!current_fixed_restart&&!current_discard,
    "Normal Font unsupported terminal restart/discard physical phase");
   // No Source callback follows replay before real Draw or its native POST.
   native_effect([&]{render.DrawElements(GL_TRIANGLES,static_cast<GLsizei>(index_view.size()),GL_UNSIGNED_INT,reinterpret_cast<const void*>(static_cast<std::uintptr_t>(range.index_begin*sizeof(std::uint32_t))));});
   full();status();
  }
  full();first();const auto restored=Pipeline::restore(*pipeline);
  need(restored.complete&&!restored.cleanup_failure,"Normal Font physical pipeline restore incomplete/failed");
  pipeline.reset();const auto drained=Pipeline::drain();first();
  need(!drained.incomplete&&!drained.pending_creator_nodes&&!drained.cleanup_failure,
   "Normal Font final physical cleanup pending/failed");
  full();status();first();
  need(matrix.manager_revision_without_callbacks()==matrix_revision&&
   material.normal_batch_revision_without_callbacks()==batch_revision,
   "Normal Font genuine matrix/batch generation changed");first();
  state->submitted=true;succeeded=true;
 }catch(...){
  // Preserve the ORIGINAL failure. Physical cleanup needs no Source/Paint
  // admission and cannot grant acceptance, switch current or throw here.
  const auto original=std::current_exception();code->submit_failed=true;
  if(pipeline){const auto ignored=Pipeline::restore(*pipeline);(void)ignored;pipeline.reset();}
  const auto ignored=Pipeline::drain();(void)ignored;
  std::rethrow_exception(original);
 }
}
}
