#pragma once
#include "civ5/graphics/gl_terrain_decal.hpp"
#include "civ5/terrain/terrain_decal_geometry.hpp"
#include "material_blend_native.hpp"
#include <map>
namespace terrain_decal_validation {
using namespace civ5::graphics;
using namespace civ5::materials;
using namespace civ5::terrain;
namespace assets=civ5::assets;
using blend_validation::Com;
using blend_validation::hr;
using blend_validation::require;
constexpr unsigned W=256,H=192;
using Vertex=std::array<std::uint16_t,4>;
inline assets::DdsData control_texture(unsigned variant) {
    assets::Bytes b(128);const auto set=[&](unsigned at,std::uint32_t v){std::memcpy(b.data()+at,&v,4);};
    set(0,0x20534444);set(4,124);set(8,0x2100f);set(12,32);set(16,32);set(20,128);set(28,6);
    set(76,32);set(80,0x41);set(88,32);set(92,0xff0000);set(96,0xff00);set(100,0xff);set(104,0xff000000);set(108,0x401008);
    for(unsigned mip=0,size=32;mip<6;++mip,size=std::max(1u,size/2))
        for(unsigned y=0;y<size;++y)for(unsigned x=0;x<size;++x) {
            b.push_back(static_cast<std::uint8_t>(37+(x*17+y*5+variant*19+mip*13)%190));
            b.push_back(static_cast<std::uint8_t>(19+(y*21+x*3+variant*37+mip*23)%220));
            b.push_back(static_cast<std::uint8_t>(11+(x*7+y*11+variant*71+mip*43)%230));
            b.push_back(static_cast<std::uint8_t>(100+(x+y*2+variant*41)%155));
        }
    return assets::DdsData::parse(std::move(b));
}
struct Native final : TerrainDecalBufferBackend {
    blend_validation::OwnWindow window;
    Com<IDirect3D9> api{Direct3DCreate9(D3D_SDK_VERSION)};
    Com<IDirect3DDevice9> device;
    Com<IDirect3DVertexShader9> vs;
    Com<IDirect3DPixelShader9> ps;
    Com<IDirect3DVertexDeclaration9> declaration;
    Com<IDirect3DIndexBuffer9> indices;
    std::vector<Com<IDirect3DVertexBuffer9>> buffers;
    std::vector<Com<IDirect3DTexture9>> textures;
    Com<IDirect3DTexture9> zero_texture;
    std::array<Com<IDirect3DSurface9>,2> targets,readbacks;
    Native(const std::filesystem::path &shaders,const std::vector<assets::DdsData> &data, IDirect3DDevice9 *shared=nullptr,
           const assets::DdsData *zero=nullptr) {
        require(bool(api.value),"D3D9 API");D3DPRESENT_PARAMETERS pp{};
        pp.BackBufferWidth=W;pp.BackBufferHeight=H;pp.BackBufferFormat=D3DFMT_X8R8G8B8;pp.BackBufferCount=1;
        pp.SwapEffect=D3DSWAPEFFECT_DISCARD;pp.hDeviceWindow=window.value;pp.Windowed=TRUE;pp.PresentationInterval=D3DPRESENT_INTERVAL_IMMEDIATE;
        if (shared) { device.value=shared;shared->AddRef(); }
        else hr(api->CreateDevice(0,D3DDEVTYPE_HAL,window.value,D3DCREATE_HARDWARE_VERTEXPROCESSING,&pp,&device.value),"Native decal device");
        const auto vertex=blend_validation::read(shaders/"original-vs.bin"),pixel=blend_validation::read(shaders/"original-ps.bin");
        require(vertex.size()==604&&pixel.size()==408,"Original decal shader sizes");
        hr(device->CreateVertexShader(reinterpret_cast<const DWORD *>(vertex.data()),&vs.value),"Original decal vertex shader");
        hr(device->CreatePixelShader(reinterpret_cast<const DWORD *>(pixel.data()),&ps.value),"Original decal pixel shader");
        const D3DVERTEXELEMENT9 elements[]{{0,0,D3DDECLTYPE_FLOAT16_2,0,D3DDECLUSAGE_POSITION,0},
                                         {0,4,D3DDECLTYPE_FLOAT16_2,0,D3DDECLUSAGE_TEXCOORD,0},D3DDECL_END()};
        hr(device->CreateVertexDeclaration(elements,&declaration.value),"Original half XYUV declaration");
        hr(device->CreateIndexBuffer(65535*2,0,D3DFMT_INDEX16,D3DPOOL_MANAGED,&indices.value,nullptr),"Identity indices");
        void *p=nullptr;hr(indices->Lock(0,0,&p,0),"Identity index lock");
        for(unsigned i=0;i<65535;++i)static_cast<std::uint16_t *>(p)[i]=static_cast<std::uint16_t>(i);
        hr(indices->Unlock(),"Identity index unlock");
        if (!shared) {
        for(unsigned i=0;i<2;++i) {
            hr(device->CreateRenderTarget(W,H,D3DFMT_A32B32G32R32F,D3DMULTISAMPLE_NONE,0,FALSE,&targets[i].value,nullptr),"Native MRT");
            hr(device->CreateOffscreenPlainSurface(W,H,D3DFMT_A32B32G32R32F,D3DPOOL_SYSTEMMEM,&readbacks[i].value,nullptr),"Native readback");
            hr(device->SetRenderTarget(i,targets[i].value),"Native MRT bind");
        }
        hr(device->SetDepthStencilSurface(nullptr),"Native no depth");
        D3DVIEWPORT9 viewport{0,0,W,H,0,1};hr(device->SetViewport(&viewport),"Native viewport");
        }
        for(auto [state,value]:std::initializer_list<std::pair<D3DRENDERSTATETYPE,DWORD>>{
            {D3DRS_ZENABLE,FALSE},{D3DRS_ZWRITEENABLE,FALSE},{D3DRS_ALPHABLENDENABLE,FALSE},
            {D3DRS_ALPHATESTENABLE,FALSE},{D3DRS_CULLMODE,D3DCULL_NONE},{D3DRS_SCISSORTESTENABLE,FALSE},
            {D3DRS_STENCILENABLE,FALSE},{D3DRS_DITHERENABLE,FALSE},{D3DRS_SRGBWRITEENABLE,FALSE},
            {D3DRS_COLORWRITEENABLE,15},{D3DRS_COLORWRITEENABLE1,15}})hr(device->SetRenderState(state,value),"Controlled native raster");
        for(unsigned i=0;i<2;++i)for(auto [state,value]:std::initializer_list<std::pair<D3DSAMPLERSTATETYPE,DWORD>>{
            {D3DSAMP_ADDRESSU,D3DTADDRESS_WRAP},{D3DSAMP_ADDRESSV,D3DTADDRESS_WRAP},{D3DSAMP_ADDRESSW,D3DTADDRESS_WRAP},
            {D3DSAMP_MAGFILTER,D3DTEXF_LINEAR},{D3DSAMP_MINFILTER,D3DTEXF_LINEAR},{D3DSAMP_MIPFILTER,D3DTEXF_LINEAR},
            {D3DSAMP_MAXANISOTROPY,1},{D3DSAMP_SRGBTEXTURE,i==0},{D3DSAMP_MAXMIPLEVEL,0},{D3DSAMP_MIPMAPLODBIAS,0}})
                hr(device->SetSamplerState(i,state,value),"Original decal sampler 2/0");
        const auto load_texture=[&](const assets::DdsData &d, Com<IDirect3DTexture9> &t) {
            D3DFORMAT format{};
            if(d.format()==assets::DdsFormat::dxt1)format=D3DFMT_DXT1;
            else if(d.format()==assets::DdsFormat::dxt3)format=D3DFMT_DXT3;
            else if(d.format()==assets::DdsFormat::dxt5)format=D3DFMT_DXT5;
            else {require(d.bits_per_pixel()==32&&d.channel_masks()==std::array<std::uint32_t,4>{0xff0000,0xff00,0xff,0xff000000},"Native BGRA8 fixture");format=D3DFMT_A8R8G8B8;}
            hr(device->CreateTexture(d.width(),d.height(),d.mip_count(),0,format,D3DPOOL_MANAGED,&t.value,nullptr),"Native authored texture");
            for(const auto &level:d.levels()) {
                D3DLOCKED_RECT lock{};hr(t->LockRect(level.mip,&lock,nullptr,0),"Native texture lock");
                const auto bytes=d.payload(0,level.mip);const auto rows=bytes.size()/level.row_pitch;
                require(lock.Pitch>=static_cast<int>(level.row_pitch),"Native texture pitch");
                for(std::size_t y=0;y<rows;++y)std::memcpy(static_cast<std::uint8_t *>(lock.pBits)+y*lock.Pitch,bytes.data()+y*level.row_pitch,level.row_pitch);
                hr(t->UnlockRect(level.mip),"Native texture unlock");
            }
        };
        for(const auto &d:data) { textures.emplace_back();load_texture(d,textures.back()); }
        if(zero)load_texture(*zero,zero_texture);
    }
    std::uint32_t create() override {
        buffers.emplace_back();hr(device->CreateVertexBuffer(65536*8,0,0,D3DPOOL_MANAGED,&buffers.back().value,nullptr),"Native vertex buffer");
        return static_cast<std::uint32_t>(buffers.size());
    }
    void upload(std::uint32_t id,std::span<const Vertex> vertices) override {
        if(vertices.empty())return;void *p=nullptr;
        hr(buffers.at(id-1)->Lock(0,static_cast<UINT>(vertices.size_bytes()),&p,0),"Native vertex upload lock");
        std::memcpy(p,vertices.data(),vertices.size_bytes());hr(buffers.at(id-1)->Unlock(),"Native vertex upload unlock");
    }
    void render(const TerrainDecalDrawList &list,DWORD clear=0,unsigned blend=0,bool clear_target=true) {
        if (clear_target) hr(device->Clear(0,nullptr,D3DCLEAR_TARGET,clear,1,0),"Native fixture background");
        for(auto [state,value]:std::initializer_list<std::pair<D3DRENDERSTATETYPE,DWORD>>{
            {D3DRS_ALPHABLENDENABLE,blend!=0},{D3DRS_SEPARATEALPHABLENDENABLE,TRUE},
            {D3DRS_SRCBLEND,blend==8?D3DBLEND_SRCALPHA:D3DBLEND_ONE},
            {D3DRS_DESTBLEND,blend==8?D3DBLEND_INVSRCALPHA:blend==4?D3DBLEND_ONE:D3DBLEND_ZERO},
            {D3DRS_BLENDOP,D3DBLENDOP_ADD},{D3DRS_SRCBLENDALPHA,D3DBLEND_ONE},
            {D3DRS_DESTBLENDALPHA,blend?D3DBLEND_ONE:D3DBLEND_ZERO},{D3DRS_BLENDOPALPHA,D3DBLENDOP_ADD}})
            hr(device->SetRenderState(state,value),"Original decal blend/depth9");
        for(unsigned i=0;i<2;++i)for(auto [state,value]:std::initializer_list<std::pair<D3DSAMPLERSTATETYPE,DWORD>>{
            {D3DSAMP_ADDRESSU,D3DTADDRESS_WRAP},{D3DSAMP_ADDRESSV,D3DTADDRESS_WRAP},{D3DSAMP_ADDRESSW,D3DTADDRESS_WRAP},
            {D3DSAMP_MAGFILTER,D3DTEXF_LINEAR},{D3DSAMP_MINFILTER,D3DTEXF_LINEAR},{D3DSAMP_MIPFILTER,D3DTEXF_LINEAR},
            {D3DSAMP_MAXANISOTROPY,1},{D3DSAMP_SRGBTEXTURE,i==0},{D3DSAMP_MAXMIPLEVEL,0},{D3DSAMP_MIPMAPLODBIAS,0}})
                hr(device->SetSamplerState(i,state,value),"Original decal sampler 2/0");
        hr(device->SetVertexShader(vs.value),"Native VS bind");hr(device->SetPixelShader(ps.value),"Native PS bind");
        hr(device->SetVertexDeclaration(declaration.value),"Native declaration bind");hr(device->SetIndices(indices.value),"Native indices bind");
        hr(device->BeginScene(),"Native begin");
        for(const auto &d:list.draws) {
            std::array<float,28> c{};std::copy(d.constants.begin(),d.constants.end(),c.begin());
            hr(device->SetVertexShaderConstantF(0,c.data(),7),"Native seven constant vectors");
            require(d.albedo!=0 || zero_texture.value,"Missing original zero albedo fixture");
            require(d.heightmap!=0 || zero_texture.value,"Missing original zero height fixture");
            hr(device->SetTexture(0,d.albedo?textures.at(d.albedo-1).value:zero_texture.value),"Native albedo bind");
            hr(device->SetTexture(1,d.heightmap?textures.at(d.heightmap-1).value:zero_texture.value),"Native height bind");
            hr(device->SetStreamSource(0,buffers.at(d.vertex_buffer-1).value,0,8),"Native half stream");
            hr(device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,65536,d.first_vertex,d.triangles),"Original indexed draw arguments");
        }
        hr(device->EndScene(),"Native end");
    }
    assets::Bytes pixels(unsigned count,unsigned pixel_bytes) {
        assets::Bytes result(count*W*H*pixel_bytes);
        for(unsigned i=0;i<count;++i) {
            hr(device->GetRenderTargetData(targets[i].value,readbacks[i].value),"Native MRT readback");D3DLOCKED_RECT lock{};
            hr(readbacks[i]->LockRect(&lock,nullptr,D3DLOCK_READONLY),"Native readback lock");
            require(lock.Pitch>=int(W*pixel_bytes),"Native readback pitch");
            for(unsigned y=0;y<H;++y)std::memcpy(result.data()+(i*W*H+y*W)*pixel_bytes,static_cast<const std::uint8_t *>(lock.pBits)+y*lock.Pitch,W*pixel_bytes);
            hr(readbacks[i]->UnlockRect(),"Native readback unlock");
        }
        return result;
    }
    void cache_targets(bool shadow) {
        for(unsigned i=1;i<4;++i)hr(device->SetRenderTarget(i,nullptr),"Detach prior cache MRT");
        for(unsigned i=0;i<2;++i) {
            if(targets[i].value)targets[i].value->Release();targets[i].value=nullptr;
            if(readbacks[i].value)readbacks[i].value->Release();readbacks[i].value=nullptr;
            if(shadow&&i==1)continue;
            const auto format=shadow?D3DFMT_A8R8G8B8:D3DFMT_A16B16G16R16F;
            hr(device->CreateRenderTarget(W,H,format,D3DMULTISAMPLE_NONE,0,FALSE,&targets[i].value,nullptr),"Native physical cache target");
            hr(device->CreateOffscreenPlainSurface(W,H,format,D3DPOOL_SYSTEMMEM,&readbacks[i].value,nullptr),"Native physical cache readback");
            hr(device->SetRenderTarget(i,targets[i].value),"Native physical cache target bind");
        }
        D3DVIEWPORT9 viewport{0,0,W,H,0,1};hr(device->SetViewport(&viewport),"Native cache viewport");
    }
    std::vector<float> draw(const TerrainDecalDrawList &list) {
        render(list);const auto raw=pixels(2,16);std::vector<float> result(raw.size()/4);
        std::memcpy(result.data(),raw.data(),raw.size());return result;
    }
};
struct Both : TerrainDecalBufferBackend {
    GlTerrainDecalRenderer &gl;Native &native;
    Both(GlTerrainDecalRenderer &g,Native &n):gl(g),native(n){}
    std::uint32_t create() override {auto id=gl.create();require(id==native.create(),"Backend identity mismatch");return id;}
    void upload(std::uint32_t id,std::span<const Vertex> v) override {
        gl.upload(id,v);native.upload(id,v);require(gl.read_vertices(id)==std::vector<Vertex>(v.begin(),v.end()),"GPU packed bytes differ");
    }
};
struct Inputs {
    std::shared_ptr<GlContext> context;
    const std::vector<assets::DdsData> &data;
    std::map<unsigned,std::unique_ptr<GlTexture>> textures;
    const GlTexture &get(std::uint16_t id,DdsSamplingColor color) {
        const auto key=unsigned(id)*2+(color==DdsSamplingColor::srgb);auto &t=textures[key];
        if(!t)t=std::make_unique<GlTexture>(*context,data.at(id-1),color);return *t;
    }
};
inline TerrainDecalGeometry quad(float x,float y,float w,float h,float uv=1) {
    return {{{x,y,-.25f,0},{x+w,y,uv,0},{x,y+h,-.25f,uv},{x+w,y+h,uv,uv}},{2,1,0,2,3,1}};
}
inline TerrainDecalAssets authored(const char *root) {
    assets::AssetEnvironment env(root);
    const auto open=[&](const char *name){auto r=env.open_fpk_archive(name);require(std::holds_alternative<std::shared_ptr<assets::FpkSource>>(r),"Missing authored FPK");return std::get<std::shared_ptr<assets::FpkSource>>(std::move(r));};
    auto definitions=open("Resource/Common/MiscData.fpk");auto found=definitions->find(assets::AssetKey("decal_filenames.xml"));
    auto *definition=std::get_if<assets::AssetHandle>(&found);require(bool(definition),"Missing decal XML");
    struct Source : assets::AssetSource {
        std::shared_ptr<assets::FpkSource> decals,upgrade;
        assets::AssetLookup find(const assets::AssetKey &key) const override {
            const auto name=key.normalized_name();if(name=="blank_texture_spec_decal.dds")return upgrade->find(key);
            require(name.starts_with("assets/decals/")&&key.safe_relative_path(),"Authored decal alias");
            return decals->find(assets::AssetKey(name.substr(name.find_last_of('/')+1)));
        }
        assets::Provenance provenance() const override {return {assets::SourceKind::fpk,"controlled-decal-aliases","",{}};}
    } source;
    source.decals=open("Resource/DX9/DecalTextures.fpk");source.upgrade=open("Resource/DX9/Upgrade1Textures.fpk");
    return TerrainDecalAssets::load(*definition,source);
}
} // namespace terrain_decal_validation
