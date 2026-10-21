#include "font_shader_dx9_oracle.hpp"
#include "civ5/graphics/detail/bnw_font_shader_packed_validation.hpp"
#include <optional>
#include "civ5/runtime/file_identity.hpp"
#include <d3d9.h>
#include <windows.h>
#include <algorithm>
#include <bit>
#include <cstring>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <thread>
namespace civ5::validation {
namespace {
void need(bool v,const char*s){if(!v)throw std::runtime_error(s);}
void hr(HRESULT v,const char*s){if(FAILED(v)){std::ostringstream out;out<<s<<" HRESULT=0x"<<std::hex<<static_cast<std::uint32_t>(v);throw std::runtime_error(out.str());}}
template<class T>struct Com {
 T*p=nullptr;Com()=default;Com(const Com&)=delete;Com&operator=(const Com&)=delete;
 ~Com(){if(p)p->Release();}T**out(){need(!p,"Oracle COM output already populated");return &p;}
 T*operator->()const{return p;}explicit operator bool()const{return p!=nullptr;}
};
std::uint32_t u32(const assets::Bytes&b,std::size_t at){need(at<=b.size()&&b.size()-at>=4,"Truncated original PE");std::uint32_t value;std::memcpy(&value,b.data()+at,4);return value;}
std::uint16_t u16(const assets::Bytes&b,std::size_t at){need(at<=b.size()&&b.size()-at>=2,"Truncated original PE");std::uint16_t value;std::memcpy(&value,b.data()+at,2);return value;}
assets::Bytes embedded(const assets::Bytes&b,std::uint32_t va,std::uint32_t count){
 const auto pe=u32(b,0x3c);need(u32(b,pe)==0x4550&&u16(b,pe+24)==0x10b,"Foreign original PE format");
 const auto base=u32(b,pe+24+28);const auto sections=u16(b,pe+6),opt=u16(b,pe+20);need(va>=base,"Original shader VA precedes image");
 std::size_t offset=0;unsigned matches=0;
 for(unsigned k=0;k<sections;++k){const auto row=static_cast<std::size_t>(pe)+24+opt+k*40;
  const std::uint64_t rva=u32(b,row+12),size=u32(b,row+16),position=u32(b,row+20),wanted=va-base;
  if(wanted>=rva&&wanted-rva<=size&&count<=size-(wanted-rva)){
   const std::uint64_t actual=position+wanted-rva;need(actual<=b.size()&&count<=b.size()-actual,"Original shader PE range exceeds retained bytes");offset=static_cast<std::size_t>(actual);++matches;
  }
 }
 need(matches==1,"Missing/ambiguous original shader PE range");return assets::Bytes(b.begin()+offset,b.begin()+offset+count);
}
bool native_current(const graphics::GlContextNativeIdentity&i){return i.owner_thread&&i.window&&i.dc&&i.context&&
 reinterpret_cast<std::uintptr_t>(wglGetCurrentContext())==i.context&&reinterpret_cast<std::uintptr_t>(wglGetCurrentDC())==i.dc&&
 reinterpret_cast<std::uintptr_t>(WindowFromDC(reinterpret_cast<HDC>(i.dc)))==i.window;}
struct DxVertex {float x,y,z,u,v,r,g,b,a;};
static_assert(sizeof(DxVertex)==36);
}
OriginalFontShaderPrograms read_original_font_shader_programs(const std::filesystem::path&path){
 std::ifstream in(path,std::ios::binary|std::ios::ate);need(in.is_open(),"Read-only original executable unavailable");const auto length=in.tellg();need(length>0&&length<512*1024*1024,"Unbounded original executable size");
 assets::Bytes bytes(static_cast<std::size_t>(length));in.seekg(0);need(static_cast<bool>(in.read(reinterpret_cast<char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()))),"Failed original executable read");
 OriginalFontShaderPrograms result;result.exe_sha256=runtime::identify_bytes(bytes).sha256;need(result.exe_sha256==graphics::bnw_font_shader_original_exe_sha256,"Changed original FontShader executable");
 const auto vs=embedded(bytes,0x5d7fd08,492),ps=embedded(bytes,0x5d81868,508);
 result.vs_sha256=runtime::identify_bytes(vs).sha256;result.ps_sha256=runtime::identify_bytes(ps).sha256;
 need(result.vs_sha256==graphics::bnw_font_shader_original_vs_sha256&&result.ps_sha256==graphics::bnw_font_shader_original_ps_sha256,"Changed original row4 shader bytes");
 result.vs.resize(vs.size()/4);result.ps.resize(ps.size()/4);std::memcpy(result.vs.data(),vs.data(),vs.size());std::memcpy(result.ps.data(),ps.data(),ps.size());
 need(result.vs.front()==0xfffe0300&&result.ps.front()==0xffff0300&&result.vs.back()==0xffff&&result.ps.back()==0xffff,"Unexpected original shader version/end tokens");return result;
}
void validate_original_font_shader_programs(const OriginalFontShaderPrograms&p){
 need(p.exe_sha256==graphics::bnw_font_shader_original_exe_sha256&&p.vs_sha256==graphics::bnw_font_shader_original_vs_sha256&&p.ps_sha256==graphics::bnw_font_shader_original_ps_sha256,"Missing pinned original shader sources");
 need(p.vs.size()==123&&p.ps.size()==127,"Changed original shader byte counts");
 need(runtime::identify_bytes(std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(p.vs.data()),p.vs.size()*4)).sha256==p.vs_sha256,"Original VS bytes substituted");
 need(runtime::identify_bytes(std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(p.ps.data()),p.ps.size()*4)).sha256==p.ps_sha256,"Original PS bytes substituted");
 need(p.vs.front()==0xfffe0300&&p.ps.front()==0xffff0300&&p.vs.back()==0xffff&&p.ps.back()==0xffff,"Changed original shader version/end tokens");
}
struct Dx9FontShaderOracle::Impl {
 graphics::GlContextNativeIdentity identity;int width=0,height=0;
 std::thread::id thread=std::this_thread::get_id();bool busy=false,failed=false;
 std::function<void()>source;FontShaderOracleDeviceReceipt device_receipt;
 Com<IDirect3D9>api;Com<IDirect3DDevice9>device;
 Com<IDirect3DVertexShader9>vs;Com<IDirect3DPixelShader9>ps;Com<IDirect3DVertexDeclaration9>declaration,packed_declaration;
 FontShaderOracleImage draw_engine(const graphics::BnwFontShaderBatch*,FontShaderOracleStorage,bool,const graphics::BnwFontShaderPackedBatch*);
 std::array<Com<IDirect3DTexture9>,2>planes;
 void native()const{
  need(std::this_thread::get_id()==thread,"Original shader oracle used on foreign thread");need(!failed,"Original shader oracle quarantined after nested draw");need(native_current(identity),"Original shader oracle rejects foreign WGL without switching");
  RECT rect{};DWORD process=0;GetWindowThreadProcessId(reinterpret_cast<HWND>(identity.window),&process);
  need(process==GetCurrentProcessId()&&GetClientRect(reinterpret_cast<HWND>(identity.window),&rect)&&rect.right-rect.left==width&&rect.bottom-rect.top==height,"Original shader oracle client replaced/resized");
 }
 void current()const{native();need(static_cast<bool>(source),"Missing SAME source validation");source();native();if(device)hr(device->TestCooperativeLevel(),"Actual original shader HAL device cooperative state");}
 void state(bool blend){
  const std::array<std::pair<D3DRENDERSTATETYPE,DWORD>,20>states{{
   {D3DRS_ZENABLE,D3DZB_FALSE},{D3DRS_ZWRITEENABLE,FALSE},{D3DRS_ZFUNC,D3DCMP_LESSEQUAL},
   {D3DRS_CULLMODE,D3DCULL_NONE},{D3DRS_STENCILENABLE,FALSE},{D3DRS_ALPHATESTENABLE,FALSE},
   {D3DRS_ALPHABLENDENABLE,blend?TRUE:FALSE},{D3DRS_SEPARATEALPHABLENDENABLE,blend?TRUE:FALSE},
   {D3DRS_SRCBLEND,D3DBLEND_SRCALPHA},{D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA},{D3DRS_BLENDOP,D3DBLENDOP_ADD},
   {D3DRS_SRCBLENDALPHA,D3DBLEND_ONE},{D3DRS_DESTBLENDALPHA,D3DBLEND_ONE},{D3DRS_BLENDOPALPHA,D3DBLENDOP_ADD},
   {D3DRS_COLORWRITEENABLE,15},{D3DRS_SRGBWRITEENABLE,FALSE},{D3DRS_SCISSORTESTENABLE,FALSE},
   {D3DRS_DITHERENABLE,FALSE},{D3DRS_FILLMODE,D3DFILL_SOLID},{D3DRS_CLIPPLANEENABLE,0}}};
  for(const auto&[key,value]:states){hr(device->SetRenderState(key,value),"Issue original shader fixture state");DWORD actual=~value;hr(device->GetRenderState(key,&actual),"Verify original shader fixture state");need(actual==value,"Original shader state not actually applied");}
  for(DWORD stage=0;stage<2;++stage){const std::array<std::pair<D3DSAMPLERSTATETYPE,DWORD>,8>s{{
   {D3DSAMP_ADDRESSU,D3DTADDRESS_WRAP},{D3DSAMP_ADDRESSV,D3DTADDRESS_WRAP},{D3DSAMP_ADDRESSW,D3DTADDRESS_WRAP},
   {D3DSAMP_MAGFILTER,D3DTEXF_POINT},{D3DSAMP_MINFILTER,D3DTEXF_POINT},{D3DSAMP_MIPFILTER,D3DTEXF_POINT},
   {D3DSAMP_MAXANISOTROPY,1},{D3DSAMP_SRGBTEXTURE,FALSE}}};
   for(const auto&[key,value]:s){hr(device->SetSamplerState(stage,key,value),"Issue native sampler9");DWORD actual=~value;hr(device->GetSamplerState(stage,key,&actual),"Verify native sampler9");need(actual==value,"Native sampler9 not actually applied");}}
 }
};
Dx9FontShaderOracle::Dx9FontShaderOracle(std::unique_ptr<Impl>i):impl_(std::move(i)){}
Dx9FontShaderOracle::~Dx9FontShaderOracle()=default;
const FontShaderOracleDeviceReceipt&Dx9FontShaderOracle::receipt()const noexcept{return impl_->device_receipt;}
std::shared_ptr<Dx9FontShaderOracle>Dx9FontShaderOracle::create(graphics::GlContextNativeIdentity identity,int width,int height,
 const OriginalFontShaderPrograms&borrowed_programs,const GeneratedFontShaderPlanes&borrowed_inputs,std::function<void()>source){
 const auto p=borrowed_programs;const auto inputs=borrowed_inputs;validate_original_font_shader_programs(p);
 auto i=std::make_unique<Impl>();i->identity=identity;i->width=width;i->height=height;i->source=std::move(source);i->current();
 need(width>0&&height>0&&p.exe_sha256==graphics::bnw_font_shader_original_exe_sha256&&p.vs_sha256==graphics::bnw_font_shader_original_vs_sha256&&p.ps_sha256==graphics::bnw_font_shader_original_ps_sha256,"Missing pinned original programs");
 need(p.vs.size()==123&&p.ps.size()==127&&runtime::identify_bytes(std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(p.vs.data()),p.vs.size()*4)).sha256==p.vs_sha256,"Original VS byte vector substituted");
 need(runtime::identify_bytes(std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(p.ps.data()),p.ps.size()*4)).sha256==p.ps_sha256,"Original PS byte vector substituted");
 const auto&pair=inputs.pair;need(pair.pair_storage_available&&pair.width>0&&pair.height>0&&pair.green_texture&&pair.shadow_texture&&pair.green_texture!=pair.shadow_texture,"Missing generated SAME atlas pair");
 const auto pixels=static_cast<std::uint64_t>(pair.width)*pair.height;need(pixels==inputs.green.size()&&pixels==inputs.shadow.size()&&pixels<=128*1024*1024,"Invalid actual generated plane range");
 need(runtime::identify_bytes(inputs.green).sha256==inputs.green_sha256&&runtime::identify_bytes(inputs.shadow).sha256==inputs.shadow_sha256,"Generated plane bytes substituted");
 i->api.p=Direct3DCreate9(D3D_SDK_VERSION);need(static_cast<bool>(i->api),"Actual DX9 HAL API unavailable");
 D3DADAPTER_IDENTIFIER9 adapter{};hr(i->api->GetAdapterIdentifier(0,0,&adapter),"Actual DX9 adapter identity");
 D3DDISPLAYMODE mode{};hr(i->api->GetAdapterDisplayMode(0,&mode),"Actual DX9 display format");D3DCAPS9 caps{};hr(i->api->GetDeviceCaps(0,D3DDEVTYPE_HAL,&caps),"Actual DX9 HAL shader caps");
 auto&receipt=i->device_receipt;receipt.adapter_description=adapter.Description;receipt.vendor=adapter.VendorId;receipt.device=adapter.DeviceId;receipt.vs_version=caps.VertexShaderVersion;receipt.ps_version=caps.PixelShaderVersion;receipt.hal=true;
 need(caps.VertexShaderVersion>=D3DVS_VERSION(3,0)&&caps.PixelShaderVersion>=D3DPS_VERSION(3,0),"Actual HAL cannot execute original row4 programs");
 need((caps.TextureAddressCaps&D3DPTADDRESSCAPS_WRAP)&&((caps.TextureFilterCaps&(D3DPTFILTERCAPS_MINFPOINT|D3DPTFILTERCAPS_MAGFPOINT|D3DPTFILTERCAPS_MIPFPOINT))==(D3DPTFILTERCAPS_MINFPOINT|D3DPTFILTERCAPS_MAGFPOINT|D3DPTFILTERCAPS_MIPFPOINT)),"Actual L8 POINT/WRAP sampler unavailable");
 hr(i->api->CheckDeviceFormat(0,D3DDEVTYPE_HAL,mode.Format,0,D3DRTYPE_TEXTURE,D3DFMT_L8),"Actual HAL L8 input format");receipt.l8_point_wrap=true;
 need((caps.PrimitiveMiscCaps&D3DPMISCCAPS_SEPARATEALPHABLEND)&&((caps.SrcBlendCaps&(D3DPBLENDCAPS_SRCALPHA|D3DPBLENDCAPS_ONE))==(D3DPBLENDCAPS_SRCALPHA|D3DPBLENDCAPS_ONE))&&((caps.DestBlendCaps&(D3DPBLENDCAPS_INVSRCALPHA|D3DPBLENDCAPS_ONE))==(D3DPBLENDCAPS_INVSRCALPHA|D3DPBLENDCAPS_ONE)),"Actual sourceBlend8 caps unavailable");
 hr(i->api->CheckDeviceFormat(0,D3DDEVTYPE_HAL,mode.Format,D3DUSAGE_RENDERTARGET|D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING,D3DRTYPE_SURFACE,D3DFMT_A8R8G8B8),"Actual RGBA8 Blend8 target support");receipt.rgba8_blend8=true;
 hr(i->api->CheckDeviceFormat(0,D3DDEVTYPE_HAL,mode.Format,D3DUSAGE_RENDERTARGET,D3DRTYPE_SURFACE,D3DFMT_A32B32G32R32F),"Actual original shader float RT support");receipt.float_rt=true;
 receipt.float_post_shader_blend=SUCCEEDED(i->api->CheckDeviceFormat(0,D3DDEVTYPE_HAL,mode.Format,D3DUSAGE_RENDERTARGET|D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING,D3DRTYPE_SURFACE,D3DFMT_A32B32G32R32F));
 D3DPRESENT_PARAMETERS pp{};pp.BackBufferWidth=width;pp.BackBufferHeight=height;pp.BackBufferFormat=D3DFMT_X8R8G8B8;pp.BackBufferCount=1;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;pp.hDeviceWindow=reinterpret_cast<HWND>(identity.window);pp.Windowed=TRUE;pp.PresentationInterval=D3DPRESENT_INTERVAL_IMMEDIATE;
 hr(i->api->CreateDevice(0,D3DDEVTYPE_HAL,pp.hDeviceWindow,D3DCREATE_HARDWARE_VERTEXPROCESSING|D3DCREATE_FPU_PRESERVE,&pp,i->device.out()),"Create actual owned original shader HAL device");i->current();
 hr(i->device->CreateVertexShader(reinterpret_cast<const DWORD*>(p.vs.data()),i->vs.out()),"Create untouched original row4 VS");hr(i->device->CreatePixelShader(reinterpret_cast<const DWORD*>(p.ps.data()),i->ps.out()),"Create untouched original row4 PS");
 const D3DVERTEXELEMENT9 decl[]={{0,0,D3DDECLTYPE_FLOAT3,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_POSITION,0},{0,12,D3DDECLTYPE_FLOAT2,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_TEXCOORD,0},{0,20,D3DDECLTYPE_FLOAT4,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_COLOR,0},D3DDECL_END()};hr(i->device->CreateVertexDeclaration(decl,i->declaration.out()),"Create declared diagnostic float-color input");
 need(static_cast<UINT>(pair.width)<=caps.MaxTextureWidth&&static_cast<UINT>(pair.height)<=caps.MaxTextureHeight,"Source GL atlas exceeds actual original HAL texture caps");
 for(unsigned k=0;k<2;++k){hr(i->device->CreateTexture(pair.width,pair.height,1,0,D3DFMT_L8,D3DPOOL_MANAGED,i->planes[k].out(),nullptr),"Create transient original shader L8 oracle input");D3DLOCKED_RECT lock{};hr(i->planes[k]->LockRect(0,&lock,nullptr,0),"Lock actual L8 oracle input");
  const auto&bytes=k?inputs.shadow:inputs.green;try{need(lock.Pitch>=pair.width&&lock.pBits,"Invalid actual L8 lock pitch");for(int y=0;y<pair.height;++y)std::memcpy(static_cast<std::uint8_t*>(lock.pBits)+static_cast<std::size_t>(y)*lock.Pitch,bytes.data()+static_cast<std::size_t>(y)*pair.width,pair.width);}catch(...){i->planes[k]->UnlockRect(0);throw;}hr(i->planes[k]->UnlockRect(0),"Unlock actual L8 oracle input");}
 i->current();return std::shared_ptr<Dx9FontShaderOracle>(new Dx9FontShaderOracle(std::move(i)));
}
FontShaderOracleImage Dx9FontShaderOracle::draw(const graphics::BnwFontShaderBatch&b,FontShaderOracleStorage storage,bool blend){
 const auto keep_alive=shared_from_this();return impl_->draw_engine(&b,storage,blend,nullptr);
}
FontShaderOracleImage Dx9FontShaderOracle::draw_packed(const graphics::BnwFontShaderPackedBatch&borrowed,FontShaderOracleStorage storage,bool blend){
 const auto keep_alive=shared_from_this();return impl_->draw_engine(nullptr,storage,blend,&borrowed);
}
FontShaderOracleImage Dx9FontShaderOracle::Impl::draw_engine(const graphics::BnwFontShaderBatch*borrowed,FontShaderOracleStorage storage,bool blend,const graphics::BnwFontShaderPackedBatch*borrowed_packed){
 auto&i=*this;need(std::this_thread::get_id()==i.thread,"Original shader oracle foreign-thread draw");if(i.busy){i.failed=true;throw std::runtime_error("Nested original shader oracle draw rejected");}
 i.native();struct Busy{Impl&i;explicit Busy(Impl&x):i(x){i.busy=true;}~Busy(){i.busy=false;}}busy(i);need(bool(borrowed)!=bool(borrowed_packed),"Ambiguous typed oracle input");
 const std::optional<graphics::BnwFontShaderPackedBatch>retained_packed=borrowed_packed?std::optional<graphics::BnwFontShaderPackedBatch>(*borrowed_packed):std::nullopt;
 const auto*packed=retained_packed?&*retained_packed:nullptr;
 const auto batch=borrowed?*borrowed:graphics::detail::packed_validation_view(*packed);
 need(batch.caller_declared_diagnostic_colors&&batch.current,"Oracle needs explicitly current diagnostic colors/quad inputs");need(batch.current(),"Stale original shader oracle batch");i.current();
 need(storage==FontShaderOracleStorage::rgba8||storage==FontShaderOracleStorage::rgba32f,"Unknown original shader readback storage");need(storage!=FontShaderOracleStorage::rgba32f||!blend||i.device_receipt.float_post_shader_blend,"Unverified original float blend capability");
 if(packed&&!i.packed_declaration){
 D3DCAPS9 caps{};hr(i.api->GetDeviceCaps(0,D3DDEVTYPE_HAL,&caps),"Actual packed declaration HAL caps");
 need((caps.DeclTypes&D3DDTCAPS_UBYTE4N)!=0,"Actual HAL lacks source UBYTE4N declaration");
 const D3DVERTEXELEMENT9 packed_decl[]={{0,0,D3DDECLTYPE_FLOAT3,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_POSITION,0},{0,12,D3DDECLTYPE_UBYTE4N,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_COLOR,0},{0,16,D3DDECLTYPE_FLOAT2,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_TEXCOORD,0},D3DDECL_END()};
 hr(i.device->CreateVertexDeclaration(packed_decl,i.packed_declaration.out()),"Create actual source format9 stride24 declaration");
 D3DVERTEXELEMENT9 observed[MAXD3DDECLLENGTH+1]{};UINT element_count=MAXD3DDECLLENGTH+1;
 hr(i.packed_declaration->GetDeclaration(observed,&element_count),"Observe actual UBYTE4N declaration");
 need(element_count==4&&std::memcmp(observed,packed_decl,sizeof(packed_decl))==0,"Actual source format9 declaration substituted");
 }
 const auto format=storage==FontShaderOracleStorage::rgba32f?D3DFMT_A32B32G32R32F:D3DFMT_A8R8G8B8;Com<IDirect3DSurface9>target,readback;
 hr(i.device->CreateRenderTarget(i.width,i.height,format,D3DMULTISAMPLE_NONE,0,FALSE,target.out(),nullptr),"Create actual original shader target");hr(i.device->CreateOffscreenPlainSurface(i.width,i.height,format,D3DPOOL_SYSTEMMEM,readback.out(),nullptr),"Create independent original shader readback");
 hr(i.device->SetRenderTarget(0,target.p),"Bind original shader oracle target");hr(i.device->SetDepthStencilSurface(nullptr),"Original shader control has no depth surface");D3DVIEWPORT9 viewport{0,0,static_cast<DWORD>(i.width),static_cast<DWORD>(i.height),0,1};hr(i.device->SetViewport(&viewport),"Bind actual source client viewport");
 // Same exactly representable background for float and RGBA8. Separate alpha
 // addition must differ from conventional source-over on the overlap fixture.
 hr(i.device->Clear(0,nullptr,D3DCLEAR_TARGET,D3DCOLOR_ARGB(51,26,51,77),1,0),"Clear independent original oracle target");i.state(blend);
 hr(i.device->SetVertexDeclaration(packed?i.packed_declaration.p:i.declaration.p),"Bind declared oracle vertex stream");hr(i.device->SetVertexShader(i.vs.p),"Bind untouched original VS");hr(i.device->SetPixelShader(i.ps.p),"Bind untouched original PS");
 const auto&c=batch.controls;hr(i.device->SetVertexShaderConstantF(0,c.source_matrix_rows[0].data(),4),"Original source matrix c0..3");
 const std::array<float,4>pos{0,0,c.translation[0],c.translation[1]},alpha{0,0,0,c.parent_alpha},depth{c.depth_pair[0],c.depth_pair[1],0,0},allow{c.allow_color?1.f:0.f,0,0,0};
 hr(i.device->SetVertexShaderConstantF(15,pos.data(),1),"Original translation c15");hr(i.device->SetVertexShaderConstantF(16,alpha.data(),1),"Original parent alpha c16");hr(i.device->SetVertexShaderConstantF(17,depth.data(),1),"Original depth scale c17");hr(i.device->SetPixelShaderConstantF(0,c.clip_xmin_xmax_ymin_ymax.data(),1),"Original inclusive clip c0");hr(i.device->SetPixelShaderConstantF(1,allow.data(),1),"Original AllowColor c1");
 bool scene=false;try{hr(i.device->BeginScene(),"Begin original shader oracle scene");scene=true;unsigned previous=3;
  std::size_t layer_index=0;for(const auto&layer:batch.layers){const auto current_layer=layer_index++;need(layer.slot<previous,"Original oracle source layer ordering changed");previous=layer.slot;std::size_t quad_index=0;for(const auto&q:layer.quads){const auto current_quad=quad_index++;std::array<DxVertex,4>v{};for(unsigned k=0;k<4;++k){const auto&s=q.vertices[k];v[k]={s.x,s.y,0,s.u,s.v,s.diagnostic_rgba[0],s.diagnostic_rgba[1],s.diagnostic_rgba[2],s.diagnostic_rgba[3]};}
   const WORD indices[]{0,1,2,0,2,3};const auto plane=layer.slot==2?1u:0u;hr(i.device->SetTexture(0,i.planes[plane].p),"Native selected brightness stage0");hr(i.device->SetTexture(1,i.planes[plane].p),"Native selected brightness stage1");hr(i.device->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST,0,4,2,indices,D3DFMT_INDEX16,packed?static_cast<const void*>(packed->layers[current_layer].quads[current_quad].vertices.data()):static_cast<const void*>(v.data()),packed?sizeof(ui::NativeLabelPackedVertex):sizeof(DxVertex)),"Actual untouched original row4 draw");}
  }hr(i.device->EndScene(),"End original shader oracle scene");scene=false;
 }catch(...){if(scene)i.device->EndScene();throw;}
 hr(i.device->GetRenderTargetData(target.p,readback.p),"Observe independent actual original shader output");D3DLOCKED_RECT lock{};hr(readback->LockRect(&lock,nullptr,D3DLOCK_READONLY),"Read untouched original shader output");
 FontShaderOracleImage image;image.storage=storage;image.width=i.width;image.height=i.height;image.native_format=format;image.native_row_pitch=lock.Pitch;
 try{const auto channels=static_cast<std::size_t>(i.width)*i.height*4;need(lock.pBits&&lock.Pitch>=i.width*(storage==FontShaderOracleStorage::rgba32f?16:4),"Invalid actual original shader output pitch");if(storage==FontShaderOracleStorage::rgba32f){image.rgba32f.resize(channels);for(int y=0;y<i.height;++y)std::memcpy(image.rgba32f.data()+static_cast<std::size_t>(y)*i.width*4,static_cast<const std::uint8_t*>(lock.pBits)+static_cast<std::size_t>(y)*lock.Pitch,static_cast<std::size_t>(i.width)*16);}else{image.rgba8.resize(channels);for(int y=0;y<i.height;++y)for(int x=0;x<i.width;++x){const auto*s=static_cast<const std::uint8_t*>(lock.pBits)+static_cast<std::size_t>(y)*lock.Pitch+x*4;auto*d=image.rgba8.data()+(static_cast<std::size_t>(y)*i.width+x)*4;d[0]=s[2];d[1]=s[1];d[2]=s[0];d[3]=s[3];}}}catch(...){readback->UnlockRect();throw;}hr(readback->UnlockRect(),"Unlock original shader output");
 i.current();need(batch.current(),"Original source changed after shader GPU effect");i.current();image.original_shader_fixture_executed=true;image.actual_vertex_stride=packed?24:36;image.actual_color_ubyte4n=packed!=nullptr;return image;
}
}
