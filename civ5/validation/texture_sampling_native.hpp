#pragma once
#include "civ5/graphics/gl_texture.hpp"
#include <algorithm>
#include <bit>
#include <cstring>
#include <d3d9.h>
#include <d3dcompiler.h>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <utility>


namespace texture_sampling_validation {
using namespace civ5::assets;
using namespace civ5::graphics;
inline void require(bool ok, const char *reason) {
    if (!ok)
        throw std::runtime_error(reason);
}
inline void checked(HRESULT code, const char *operation) {
    if (FAILED(code))
        throw std::runtime_error(std::string(operation) + ": HRESULT " +
                                 std::to_string(static_cast<std::uint32_t>(code)));
}
template <class T> struct Com {
    T *value = nullptr;
    Com() = default;
    explicit Com(T *p) : value(p) {}
    ~Com() {
        if (value)
            value->Release();
    }
    Com(const Com &) = delete;
    Com &operator=(const Com &) = delete;
    Com(Com &&other) noexcept : value(std::exchange(other.value, nullptr)) {}
    T *operator->() const { return value; }
};
struct OwnWindow {
    HWND value = CreateWindowExW(0, L"STATIC", L"Civ5 own x64 D3D9 texture control", WS_POPUP, 0, 0, 32, 32,
                                 nullptr, nullptr, nullptr, nullptr);
    OwnWindow() { require(value != nullptr, "Own hidden D3D window failed"); }
    ~OwnWindow() { DestroyWindow(value); }
};
struct NativeSampler {
    OwnWindow window;
    Com<IDirect3D9> api{Direct3DCreate9(D3D_SDK_VERSION)};
    Com<IDirect3DDevice9> device;
    Com<IDirect3DSurface9> target, readback;
    Com<IDirect3DPixelShader9> shader2d, shader_cube;
    explicit NativeSampler(IDirect3DDevice9 *shared_device = nullptr) {
        require(api.value != nullptr, "Own D3D9 API creation failed");
        D3DPRESENT_PARAMETERS pp{};
        pp.BackBufferWidth = 1;
        pp.BackBufferHeight = 1;
        pp.BackBufferFormat = D3DFMT_X8R8G8B8;
        pp.BackBufferCount = 1;
        pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
        pp.hDeviceWindow = window.value;
        pp.Windowed = TRUE;
        pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
        if (shared_device) {
            shared_device->AddRef();
            device.value = shared_device;
        } else
        checked(api->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, window.value,
                                  D3DCREATE_HARDWARE_VERTEXPROCESSING, &pp, &device.value),
                "Own HAL/HWVP device");
        checked(device->CreateRenderTarget(1, 1, D3DFMT_A32B32G32R32F, D3DMULTISAMPLE_NONE, 0, FALSE,
                                           &target.value, nullptr),
                "Float target");
        checked(device->CreateOffscreenPlainSurface(1, 1, D3DFMT_A32B32G32R32F, D3DPOOL_SYSTEMMEM,
                                                    &readback.value, nullptr),
                "Float readback");
        checked(device->SetRenderTarget(0, target.value), "Set target");
        checked(device->SetDepthStencilSurface(nullptr), "Disable depth surface");
        D3DVIEWPORT9 viewport{0, 0, 1, 1, 0, 1};
        checked(device->SetViewport(&viewport), "Viewport");
        for (auto [state, value] :
             std::initializer_list<std::pair<D3DRENDERSTATETYPE, DWORD>>{{D3DRS_ZENABLE, FALSE},
                                                                         {D3DRS_CULLMODE, D3DCULL_NONE},
                                                                         {D3DRS_ALPHABLENDENABLE, FALSE},
                                                                         {D3DRS_ALPHATESTENABLE, FALSE},
                                                                         {D3DRS_LIGHTING, FALSE},
                                                                         {D3DRS_COLORWRITEENABLE, 15},
                                                                         {D3DRS_SRGBWRITEENABLE, FALSE}})
            checked(device->SetRenderState(state, value), "Explicit render state");
        for (auto [state, value] : std::initializer_list<std::pair<D3DSAMPLERSTATETYPE, DWORD>>{
                 {D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP},
                 {D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP},
                 {D3DSAMP_ADDRESSW, D3DTADDRESS_CLAMP},
                 {D3DSAMP_MINFILTER, D3DTEXF_POINT},
                 {D3DSAMP_MAGFILTER, D3DTEXF_POINT},
                 {D3DSAMP_MIPFILTER, D3DTEXF_POINT},
                 {D3DSAMP_SRGBTEXTURE, FALSE}})
            checked(device->SetSamplerState(0, state, value), "Explicit point sampler");
        checked(device->SetFVF(D3DFVF_XYZRHW), "Position-only pretransformed control");
        for (bool cube : {false, true}) {
            const auto source = std::string("float4 probe:register(c0);sampler") + (cube ? "CUBE" : "2D") +
                                " s:register(s0);float4 main():COLOR0{return " +
                                (cube ? "texCUBElod" : "tex2Dlod") + "(s,probe);}";
            Com<ID3DBlob> code, error;
            const auto hr = D3DCompile(source.data(), source.size(), "own_civ5_x64_dds_control", nullptr,
                                       nullptr, "main", "ps_3_0", 0, 0, &code.value, &error.value);
            if (FAILED(hr) && error.value)
                throw std::runtime_error(static_cast<const char *>(error->GetBufferPointer()));
            checked(hr, "Compile own HLSL");
            checked(device->CreatePixelShader(static_cast<const DWORD *>(code->GetBufferPointer()),
                                              cube ? &shader_cube.value : &shader2d.value),
                    "Create own PS");
        }
    }
    Com<IDirect3DBaseTexture9> upload(const DdsData &data, D3DFORMAT format) {
        Com<IDirect3DBaseTexture9> result;
        if (data.face_count() == 6) {
            IDirect3DCubeTexture9 *value = nullptr;
            checked(device->CreateCubeTexture(data.width(), data.mip_count(), 0, format, D3DPOOL_MANAGED,
                                              &value, nullptr),
                    "Create native cube");
            result.value = value;
        } else {
            IDirect3DTexture9 *value = nullptr;
            checked(device->CreateTexture(data.width(), data.height(), data.mip_count(), 0, format,
                                          D3DPOOL_MANAGED, &value, nullptr),
                    "Create native 2D texture");
            result.value = value;
        }
        for (const auto &level : data.levels()) {
            D3DLOCKED_RECT lock{};
            const auto cube = data.face_count() == 6;
            if (cube)
                checked(
                    static_cast<IDirect3DCubeTexture9 *>(result.value)
                        ->LockRect(static_cast<D3DCUBEMAP_FACES>(level.face), level.mip, &lock, nullptr, 0),
                    "Lock cube");
            else
                checked(
                    static_cast<IDirect3DTexture9 *>(result.value)->LockRect(level.mip, &lock, nullptr, 0),
                    "Lock 2D");
            const auto payload = data.payload(level.face, level.mip);
            const auto rows = level.length / level.row_pitch;
            require(lock.Pitch >= static_cast<INT>(level.row_pitch) && lock.pBits != nullptr,
                    "Native texture row extent");
            for (std::size_t row = 0; row < rows; ++row)
                std::memcpy(static_cast<std::uint8_t *>(lock.pBits) + row * lock.Pitch,
                            payload.data() + row * level.row_pitch,
                            static_cast<std::size_t>(level.row_pitch));
            if (cube)
                checked(static_cast<IDirect3DCubeTexture9 *>(result.value)
                            ->UnlockRect(static_cast<D3DCUBEMAP_FACES>(level.face), level.mip),
                        "Unlock cube");
            else
                checked(static_cast<IDirect3DTexture9 *>(result.value)->UnlockRect(level.mip), "Unlock 2D");
        }
        return result;
    }
    std::array<float, 4> sample(IDirect3DBaseTexture9 *texture, bool cube,
                                const std::array<float, 4> &coordinate) {
        checked(device->SetTexture(0, texture), "Set native texture");
        checked(device->SetPixelShader(cube ? shader_cube.value : shader2d.value), "Set native PS");
        checked(device->SetPixelShaderConstantF(0, coordinate.data(), 1), "Native sample coordinate");
        const float quad[] = {-.5f, -.5f, 0, 1, .5f, -.5f, 0, 1, -.5f, .5f, 0, 1, .5f, .5f, 0, 1};
        checked(device->BeginScene(), "Begin own draw");
        const auto draw = device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, quad, 16);
        checked(device->EndScene(), "End own draw");
        checked(draw, "Own point-sample draw");
        checked(device->GetRenderTargetData(target.value, readback.value), "Native float readback");
        D3DLOCKED_RECT lock{};
        checked(readback->LockRect(&lock, nullptr, D3DLOCK_READONLY), "Read native sample");
        std::array<float, 4> result{};
        std::memcpy(result.data(), lock.pBits, 16);
        checked(readback->UnlockRect(), "Unlock native sample");
        return result;
    }
};

#define PROBE_GL_FUNCTIONS(X)                                                                                \
    X(PFNGLCREATESHADERPROC, CreateShader)                                                                   \
    X(PFNGLSHADERSOURCEPROC, ShaderSource) X(PFNGLCOMPILESHADERPROC, CompileShader)                          \
        X(PFNGLGETSHADERIVPROC, GetShaderiv) X(PFNGLGETSHADERINFOLOGPROC, GetShaderInfoLog)                  \
            X(PFNGLDELETESHADERPROC, DeleteShader) X(PFNGLCREATEPROGRAMPROC, CreateProgram)                  \
                X(PFNGLATTACHSHADERPROC, AttachShader) X(PFNGLLINKPROGRAMPROC, LinkProgram)                  \
                    X(PFNGLGETPROGRAMIVPROC, GetProgramiv) X(PFNGLGETPROGRAMINFOLOGPROC, GetProgramInfoLog)  \
                        X(PFNGLDELETEPROGRAMPROC, DeleteProgram) X(PFNGLUSEPROGRAMPROC, UseProgram)          \
                            X(PFNGLPROGRAMUNIFORM4FVPROC, ProgramUniform4fv)                                 \
                                X(PFNGLDISPATCHCOMPUTEPROC, DispatchCompute) X(PFNGLMEMORYBARRIERPROC,       \
                                                                               MemoryBarrier)                \
                                    X(PFNGLCREATEBUFFERSPROC, CreateBuffers) X(PFNGLNAMEDBUFFERSTORAGEPROC,  \
                                                                               NamedBufferStorage)           \
                                        X(PFNGLBINDBUFFERBASEPROC, BindBufferBase)                           \
                                            X(PFNGLGETNAMEDBUFFERSUBDATAPROC, GetNamedBufferSubData)         \
                                                X(PFNGLDELETEBUFFERSPROC, DeleteBuffers)                     \
                                                    X(PFNGLCREATESAMPLERSPROC, CreateSamplers)               \
                                                        X(PFNGLSAMPLERPARAMETERIPROC, SamplerParameteri)     \
                                                            X(PFNGLBINDSAMPLERPROC, BindSampler)             \
                                                                X(PFNGLDELETESAMPLERSPROC, DeleteSamplers)
struct GlSampler {
    GlContext &context;
#define DECL(type, name) type name = nullptr;
    PROBE_GL_FUNCTIONS(DECL)
#undef DECL
    GLuint programs[2]{}, buffer = 0, sampler = 0;
    explicit GlSampler(GlContext &owner) : context(owner) {
#define LOAD(type, name) name = reinterpret_cast<type>(context.proc_address("gl" #name));
        PROBE_GL_FUNCTIONS(LOAD)
#undef LOAD
        CreateBuffers(1, &buffer);
        NamedBufferStorage(buffer, 16, nullptr, 0);
        BindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, buffer);
        CreateSamplers(1, &sampler);
        SamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        SamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        for (auto key : {GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T, GL_TEXTURE_WRAP_R})
            SamplerParameteri(sampler, key, GL_CLAMP_TO_EDGE);
        BindSampler(0, sampler);
        for (unsigned cube = 0; cube < 2; ++cube) {
            const auto source =
                std::string(
                    "#version 460 core\nlayout(local_size_x=1) in;layout(binding=0) uniform sampler") +
                (cube ? "Cube" : "2D") +
                " s;layout(location=0) uniform vec4 probe;layout(std430,binding=0) buffer Output{vec4 "
                "result;};void main(){result=textureLod(s,probe." +
                (cube ? "xyz" : "xy") + ",probe.w);}";
            const auto shader = CreateShader(GL_COMPUTE_SHADER);
            const char *pointer = source.c_str();
            ShaderSource(shader, 1, &pointer, nullptr);
            CompileShader(shader);
            GLint ok = 0;
            GetShaderiv(shader, GL_COMPILE_STATUS, &ok);
            if (!ok) {
                char message[4096]{};
                GetShaderInfoLog(shader, 4096, nullptr, message);
                DeleteShader(shader);
                throw std::runtime_error(message);
            }
            programs[cube] = CreateProgram();
            AttachShader(programs[cube], shader);
            LinkProgram(programs[cube]);
            DeleteShader(shader);
            GetProgramiv(programs[cube], GL_LINK_STATUS, &ok);
            if (!ok) {
                char message[4096]{};
                GetProgramInfoLog(programs[cube], 4096, nullptr, message);
                throw std::runtime_error(message);
            }
        }
        context.check("Create own GLSL sampler control");
    }
    ~GlSampler() {
        context.api();
        UseProgram(0);
        BindSampler(0, 0);
        DeleteSamplers(1, &sampler);
        DeleteBuffers(1, &buffer);
        for (auto program : programs)
            if (program)
                DeleteProgram(program);
    }
    std::array<float, 4> sample(const GlTexture &texture, bool cube, const std::array<float, 4> &coordinate) {
        texture.bind(0);
        const auto program = programs[cube];
        UseProgram(program);
        ProgramUniform4fv(program, 0, 1, coordinate.data());
        DispatchCompute(1, 1, 1);
        MemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT | GL_SHADER_STORAGE_BARRIER_BIT);
        std::array<float, 4> result{};
        GetNamedBufferSubData(buffer, 0, 16, result.data());
        context.check("Own GLSL point sample");
        return result;
    }
};
#undef PROBE_GL_FUNCTIONS

} // namespace texture_sampling_validation
