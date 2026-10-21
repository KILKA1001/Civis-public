// Own DX9/GL devices compare the complete pinned native PS against the port.
// Constant sample fixtures are shader controls, never live scene lighting.
#include "../validation/bnw_warrior_material_sources.hpp"
#include "civ5/graphics/bnw_warrior_code_material.hpp"
#include "../validation/material_blend_native.hpp"
#include "../validation/bnw_warrior_material_pose.hpp"
#include "civ5/graphics/gl_render_api.hpp"
#include <d3dcompiler.h>
#include <iostream>
#include <limits>
using namespace civ5::graphics;
using blend_validation::Com;using blend_validation::OwnWindow;using blend_validation::hr;
static void check(bool b,const char *m){if(!b)throw std::runtime_error(m);}
struct Native {
 OwnWindow window;Com<IDirect3D9> api{Direct3DCreate9(D3D_SDK_VERSION)};Com<IDirect3DDevice9> device;
 bool control_software_vp=false;Com<IDirect3DSurface9> target,readback;Com<IDirect3DVertexShader9> vs;Com<IDirect3DPixelShader9> ps;Com<IDirect3DVertexDeclaration9> decl;
 Native(const std::filesystem::path &shaders){
  check(api.value,"Own D3D9 API");D3DPRESENT_PARAMETERS pp{};pp.BackBufferWidth=pp.BackBufferHeight=64;pp.BackBufferFormat=D3DFMT_UNKNOWN;pp.BackBufferCount=1;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;pp.hDeviceWindow=window.value;pp.Windowed=TRUE;pp.PresentationInterval=D3DPRESENT_INTERVAL_IMMEDIATE;
    DWORD window_pid=0;const auto window_thread=GetWindowThreadProcessId(window.value,&window_pid);RECT client{};
  check(IsWindow(window.value)&&window_thread==GetCurrentThreadId()&&window_pid==GetCurrentProcessId()&&GetClientRect(window.value,&client),"Own native window/thread identity");
  D3DDISPLAYMODE mode{};D3DCAPS9 caps{};hr(api->GetAdapterDisplayMode(0,&mode),"Actual adapter display mode");const auto caps_result=api->GetDeviceCaps(0,D3DDEVTYPE_HAL,&caps); D3DADAPTER_IDENTIFIER9 identifier{};hr(api->GetAdapterIdentifier(0,0,&identifier),"Actual adapter identity");
  const auto type_check=api->CheckDeviceType(0,D3DDEVTYPE_HAL,mode.Format,mode.Format,TRUE);
  std::cerr<<"Native HAL creation inputs: window/thread/process valid, client="<<(client.right-client.left)<<"x"<<(client.bottom-client.top)<<", adapters="<<api->GetAdapterCount()<<", adapter="<<identifier.Description<<", capsResult="<<caps_result<<", displayFormat="<<mode.Format<<", HWTransform="<<((caps.DevCaps&D3DDEVCAPS_HWTRANSFORMANDLIGHT)?1:0)<<", VS="<<caps.VertexShaderVersion<<", PS="<<caps.PixelShaderVersion<<", CheckDeviceType="<<type_check<<'\n';
  hr(caps_result,"Actual HAL device caps");hr(type_check,"Actual HAL/windowed desktop format support");
  hr(api->CreateDevice(0,D3DDEVTYPE_HAL,window.value,D3DCREATE_HARDWARE_VERTEXPROCESSING,&pp,&device.value),"Own native material HAL/HWVP device");
  hr(device->CreateRenderTarget(16,16,D3DFMT_A32B32G32R32F,D3DMULTISAMPLE_NONE,0,FALSE,&target.value,nullptr),"Native float target");
  hr(device->CreateOffscreenPlainSurface(16,16,D3DFMT_A32B32G32R32F,D3DPOOL_SYSTEMMEM,&readback.value,nullptr),"Native float readback");
  hr(device->SetRenderTarget(0,target.value),"Native target");hr(device->SetDepthStencilSurface(nullptr),"Native no depth");
  const auto blob=blend_validation::read(shaders/"code-skinned-ps.bin");check(blob.size()==1732 && civ5::runtime::identify_bytes(blob).sha256==bnw_warrior_code_ps_sha,"Native PS missing/foreign");
  std::vector<DWORD> words(blob.size()/4);std::memcpy(words.data(),blob.data(),blob.size());hr(device->CreatePixelShader(words.data(),&ps.value),"Complete original Warrior PS");
  const char *v="struct O{float4 p:POSITION;float4 n:TEXCOORD0;float4 v:TEXCOORD1;float3 w:TEXCOORD2;float3 c:TEXCOORD3;};O main(float3 p:POSITION){O o;o.p=float4(p,1);o.n=float4(0,0,1,.5);o.v=float4(0,0,1,.5);o.w=float3(0,0,1);o.c=float3(0,0,1);return o;}";
  Com<ID3DBlob> code,error;hr(D3DCompile(v,std::strlen(v),"material control VS",nullptr,nullptr,"main","vs_3_0",D3DCOMPILE_ENABLE_STRICTNESS,0,&code.value,&error.value),"Native control VS compile");hr(device->CreateVertexShader(static_cast<const DWORD*>(code->GetBufferPointer()),&vs.value),"Native control VS");
  const D3DVERTEXELEMENT9 e[]={{0,0,D3DDECLTYPE_FLOAT3,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_POSITION,0},D3DDECL_END()};hr(device->CreateVertexDeclaration(e,&decl.value),"Native control declaration");
  for(auto [s,v]:std::initializer_list<std::pair<D3DRENDERSTATETYPE,DWORD>>{{D3DRS_ZENABLE,0},{D3DRS_CULLMODE,D3DCULL_NONE},{D3DRS_ALPHABLENDENABLE,0},{D3DRS_ALPHATESTENABLE,0},{D3DRS_SRGBWRITEENABLE,0}})hr(device->SetRenderState(s,v),"Native control raster state");
 }
 WarriorColor draw(const BnwWarriorMaterialSamples &s,float fog_density,const BnwWarriorCodeAlphaInputs &alpha){
  std::array<Com<IDirect3DBaseTexture9>,6> textures;
  const std::array<WarriorColor,6> samples{s.diffuse,s.sref,s.irradiance,s.dull_environment,s.sharp_environment,WarriorColor{1,1,1,1}};
  for(unsigned i=0;i<6;++i){
   if(i>=2&&i<=4){Com<IDirect3DCubeTexture9> t;hr(device->CreateCubeTexture(1,1,0,D3DFMT_A32B32G32R32F,D3DPOOL_MANAGED,&t.value,nullptr),"Native constant cube");for(unsigned f=0;f<6;f++){D3DLOCKED_RECT l{};hr(t->LockRect(static_cast<D3DCUBEMAP_FACES>(f),0,&l,nullptr,0),"Cube lock");std::memcpy(l.pBits,samples[i].data(),16);hr(t->UnlockRect(static_cast<D3DCUBEMAP_FACES>(f),0),"Cube unlock");}textures[i].value=std::exchange(t.value,nullptr);}
   else {Com<IDirect3DTexture9> t;hr(device->CreateTexture(1,1,1,0,D3DFMT_A32B32G32R32F,D3DPOOL_MANAGED,&t.value,nullptr),"Native constant image");D3DLOCKED_RECT l{};hr(t->LockRect(0,&l,nullptr,0),"Image lock");std::memcpy(l.pBits,samples[i].data(),16);hr(t->UnlockRect(0),"Image unlock");textures[i].value=std::exchange(t.value,nullptr);}
   hr(device->SetTexture(i,textures[i].value),"Native shader resource");for(auto [state,v]:std::initializer_list<std::pair<D3DSAMPLERSTATETYPE,DWORD>>{{D3DSAMP_MAGFILTER,D3DTEXF_POINT},{D3DSAMP_MINFILTER,D3DTEXF_POINT},{D3DSAMP_MIPFILTER,D3DTEXF_NONE},{D3DSAMP_SRGBTEXTURE,0}})hr(device->SetSamplerState(i,state,v),"Control sampler");
  }
  const float fog[]={.1f,fog_density,0,0},color[]={s.fog_color[0],s.fog_color[1],s.fog_color[2],0},camera[]={0,0,0,0};
  hr(device->SetPixelShaderConstantF(1,fog,1),"Native fog parameters");hr(device->SetPixelShaderConstantF(2,color,1),"Native fog color");hr(device->SetPixelShaderConstantF(3,camera,1),"Native camera");hr(device->SetPixelShaderConstantF(4,s.team_color.data(),1),"Native team color control");
  const float water[]={alpha.water_height,0,0,0},values[]={alpha.alpha_values_s[0],alpha.alpha_values_s[1],alpha.alpha_values_s[2],0};hr(device->SetPixelShaderConstantF(0,water,1),"Required water input");hr(device->SetPixelShaderConstantF(5,values,1),"Required member alpha input");
  hr(device->SetVertexShader(vs.value),"Native VS bind");hr(device->SetPixelShader(ps.value),"Native original PS bind");hr(device->SetVertexDeclaration(decl.value),"Native declaration");
  const float vertices[]={-1,-1,.5f,3,-1,.5f,-1,3,.5f};hr(device->BeginScene(),"Begin native shader control");hr(device->DrawPrimitiveUP(D3DPT_TRIANGLELIST,1,vertices,12),"Native shader control draw");hr(device->EndScene(),"End native shader control");
  hr(device->GetRenderTargetData(target.value,readback.value),"Native control readback");D3DLOCKED_RECT l{};hr(readback->LockRect(&l,nullptr,D3DLOCK_READONLY),"Native result lock");WarriorColor out;std::memcpy(out.data(),static_cast<char*>(l.pBits)+8*l.Pitch+8*16,16);hr(readback->UnlockRect(),"Native result unlock");return out;
 }
};
struct Port {
 std::shared_ptr<GlContext> context=std::make_shared<GlContext>(16,16,false);GlRenderApi gl{*context};GLuint program=0,vao=0,fbo=0;
 GlTexture target=GlTexture::render_target(*context,16,16,1,RenderTextureFormat::rgba32_float);
 Port(){
  program=gl.CreateProgram();const char *vs="#version 460 core\nout vec4 warriorNormalU;out vec4 warriorViewV;out vec3 warriorWorld;out vec3 warriorClip;void main(){vec2 p=vec2(gl_VertexID==1?3:-1,gl_VertexID==2?3:-1);gl_Position=vec4(p,.5,1);warriorNormalU=vec4(0,0,1,.5);warriorViewV=vec4(0,0,1,.5);warriorWorld=vec3(0,0,1);warriorClip=vec3(0,0,1);}";const char *fs=bnw_warrior_code_material_fragment.data();
  for(auto [stage,source]:{std::pair{GL_VERTEX_SHADER,vs},std::pair{GL_FRAGMENT_SHADER,fs}}){GLuint s=gl.CreateShader(stage);gl.ShaderSource(s,1,&source,nullptr);gl.CompileShader(s);GLint ok=0;gl.GetShaderiv(s,GL_COMPILE_STATUS,&ok);if(!ok){char log[2048]{};gl.GetShaderInfoLog(s,2048,nullptr,log);throw std::runtime_error(log);}gl.AttachShader(program,s);gl.DeleteShader(s);}
  gl.LinkProgram(program);GLint ok=0;gl.GetProgramiv(program,GL_LINK_STATUS,&ok);check(ok,"Port material link");gl.CreateVertexArrays(1,&vao);gl.CreateFramebuffers(1,&fbo);gl.NamedFramebufferTexture(fbo,GL_COLOR_ATTACHMENT0,target.id(),0);const GLenum a=GL_COLOR_ATTACHMENT0;gl.NamedFramebufferDrawBuffers(fbo,1,&a);check(gl.CheckNamedFramebufferStatus(fbo,GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE,"Port target");
 }
 ~Port(){gl.DeleteFramebuffers(1,&fbo);gl.DeleteVertexArrays(1,&vao);gl.DeleteProgram(program);}
 WarriorColor draw(const BnwWarriorMaterialSamples &s,float fog_density,const BnwWarriorCodeAlphaInputs &alpha){
  auto create=reinterpret_cast<PFNGLCREATETEXTURESPROC>(context->proc_address("glCreateTextures"));auto storage=reinterpret_cast<PFNGLTEXTURESTORAGE2DPROC>(context->proc_address("glTextureStorage2D"));auto sub2=reinterpret_cast<PFNGLTEXTURESUBIMAGE2DPROC>(context->proc_address("glTextureSubImage2D"));auto sub3=reinterpret_cast<PFNGLTEXTURESUBIMAGE3DPROC>(context->proc_address("glTextureSubImage3D"));auto bind=reinterpret_cast<PFNGLBINDTEXTUREUNITPROC>(context->proc_address("glBindTextureUnit"));auto param=reinterpret_cast<PFNGLTEXTUREPARAMETERIPROC>(context->proc_address("glTextureParameteri"));
  const std::array<WarriorColor,6> samples{s.diffuse,s.sref,s.irradiance,s.dull_environment,s.sharp_environment,WarriorColor{1,1,1,1}};std::array<GLuint,6> textures{};
  for(unsigned i=0;i<6;i++){const bool cube=i>=2&&i<=4;create(cube?GL_TEXTURE_CUBE_MAP:GL_TEXTURE_2D,1,&textures[i]);storage(textures[i],1,GL_RGBA32F,1,1);if(cube)for(unsigned f=0;f<6;f++)sub3(textures[i],0,0,0,f,1,1,1,GL_RGBA,GL_FLOAT,samples[i].data());else sub2(textures[i],0,0,0,1,1,GL_RGBA,GL_FLOAT,samples[i].data());param(textures[i],GL_TEXTURE_MIN_FILTER,GL_NEAREST);param(textures[i],GL_TEXTURE_MAG_FILTER,GL_NEAREST);bind(i,textures[i]);gl.BindSampler(i,0);}
  const float fog[]={.1f,fog_density,0,0};gl.ProgramUniform4fv(program,gl.GetUniformLocation(program,"g_FogConsts"),1,fog);gl.ProgramUniform4fv(program,gl.GetUniformLocation(program,"TeamColorS"),1,s.team_color.data());gl.ProgramUniform3f(program,gl.GetUniformLocation(program,"f3FogColor"),s.fog_color[0],s.fog_color[1],s.fog_color[2]);gl.ProgramUniform3f(program,gl.GetUniformLocation(program,"f3CameraWorld"),0,0,0);
  gl.ProgramUniform1f(program,gl.GetUniformLocation(program,"fWaterHeight"),alpha.water_height);gl.ProgramUniform3f(program,gl.GetUniformLocation(program,"AlphaValuesS"),alpha.alpha_values_s[0],alpha.alpha_values_s[1],alpha.alpha_values_s[2]);
  gl.BindFramebuffer(GL_FRAMEBUFFER,fbo);gl.Viewport(0,0,16,16);gl.Disable(GL_DEPTH_TEST);gl.Disable(GL_CULL_FACE);gl.Disable(GL_BLEND);gl.Disable(GL_FRAMEBUFFER_SRGB);gl.UseProgram(program);gl.BindVertexArray(vao);gl.DrawArrays(GL_TRIANGLES,0,3);context->check("Source-bound Warrior material control");const auto bytes=target.read_level_raw(0,0);WarriorColor out;std::memcpy(out.data(),bytes.data()+(8*16+8)*16,16);for(unsigned i=0;i<6;i++)bind(i,0);context->api().DeleteTextures(6,textures.data());return out;
 }
};
int main(int argc,char **argv){try{
 check(argc==7,"Usage: ASSET_ROOT SDK_CACHE METADATA CODE_SHADER_EXPORT OUTPUT_DIR MODE(native|gl)");
 check(std::string(argv[6])=="native"||std::string(argv[6])=="gl","Explicit oracle mode required");
 std::filesystem::create_directories(argv[5]);
 const auto source=load_bnw_warrior_material_sources(argv[1],argv[2],argv[3]);check(source.resources.size()==9,"Actual9 source DDS");
 std::unique_ptr<Native> native;if(std::string(argv[6])=="native")native=std::make_unique<Native>(argv[4]);
 {GlContext c(16,16,false);unsigned levels=0;for(const auto&r:source.resources){GlTexture tex(c,r.data,DdsSamplingColor::linear);for(unsigned f=0;f<r.data.face_count();++f)for(unsigned m=0;m<r.data.mip_count();++m){const auto b=tex.read_level_raw(f,m);const auto e=r.data.payload(f,m);check(b.size()==e.size()&&std::equal(b.begin(),b.end(),e.begin()),"Source mip upload drift");++levels;}}check(levels==156,"156 actual DDS mips");}
 Port port;double worst=0;unsigned cases=0;
 for(float water:{0.f,1.f,2.f})for(float a:{0.f,.5f,1.f})for(float b:{0.f,.5f,1.f})for(float t:{0.f,.5f,1.f})for(float density:{0.f,.75f}){
  BnwWarriorMaterialSamples s{{.65f,.4f,.25f,.5f},{.3f,.55f,.8f,.5f},{.7f,.6f,.5f,1},{.2f,.3f,.4f,1},{.8f,.7f,.6f,1},{.2f,.75f,.4f,1},{.12f,.15f,.19f},0};
  s.fog_factor=1.f-std::clamp(std::exp2((1.f-std::exp2(.1f*1.44269502f))*density*1.44269502f),0.f,1.f);
  BnwWarriorCodeAlphaInputs alpha{1,water,{a,b,t}};const auto cpu=evaluate_bnw_warrior_code_material_samples({s,alpha}),gl=port.draw(s,density,alpha);const auto dx=native?native->draw(s,density,alpha):cpu;
  for(unsigned i=0;i<4;++i){check(std::isfinite(gl[i])&&std::isfinite(dx[i]),"Nonfinite CODE oracle");const auto e=std::max(std::abs(double(gl[i])-cpu[i]),std::abs(double(gl[i])-dx[i]));worst=std::max(worst,e);check(e<.00002,"CODE PS port differs");}++cases;
 }
 std::ofstream receipt(std::filesystem::path(argv[5])/"code-material-shader-receipt.json");receipt<<"{\"schema\":\"civis-warrior-code-material-gpu-v1\",\"cases\":"<<cases<<",\"nativeOracleExecuted\":"<<(native?"true":"false")<<",\"maxFloatError\":"<<worst<<",\"actualDdsMipUploads\":156,\"controlSamplerPolicy\":true,\"nativeSamplerPolicyProven\":false,\"waterHeightProducerProven\":false,\"modelDrawable\":false,\"fullGraphicsParity\":false}\n";
 std::cout<<"PASS CODE GL/CPU "<<cases<<" controls; nativeOracle="<<(native?"yes":"no")<<"; max="<<worst<<"; DDS156\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
