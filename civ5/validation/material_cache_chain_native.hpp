#pragma once
#include "material_cache_raster_native.hpp"
#include "material_shadow_native.hpp"
#include "material_lighting_native.hpp"
#include "civ5/graphics/gl_material_cache_renderer.hpp"
#include <map>

namespace cache_chain_validation {
using namespace cache_raster_validation;
using namespace civ5::graphics;
struct NativeCacheChain : NativeCacheRaster {
    shadow_validation::NativeShadows shadows;
    lighting_validation::NativeLighting lights;
    Com<IDirect3DTexture9> world_height;
    std::array<Com<IDirect3DTexture9>, 3> extra;
    std::map<std::uint32_t, IDirect3DBaseTexture9 *> resources;
    NativeCacheChain(const std::array<std::filesystem::path, 4> &shaders, const MaterialAssets &assets,
                     const LightingCubeAssets &cubes, std::span<const float> lut, unsigned size,
                     std::span<const std::uint8_t> geometry, std::span<const std::uint8_t> height,
                     const MaterialCachePassSource &bindings, std::uint32_t height_handle,
                     std::uint32_t final_handle, std::uint32_t higher_handle)
        : NativeCacheRaster(shaders[0], shaders[1], assets, size, geometry),
          shadows(shaders[2], device.value), lights(shaders[3], cubes, lut, device.value),
          world_height(shadows.texture(1024, height, true)) {
        require(shadows.device.value == device.value && lights.device.value == device.value,
                "All cache shader controls must share one D3D9 device");
        for (unsigned i = 0; i < extra.size(); ++i)
            hr(device->CreateTexture(size, size, 0, D3DUSAGE_RENDERTARGET | D3DUSAGE_AUTOGENMIPMAP,
                                     i == 1 ? D3DFMT_R5G6B5 : D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT,
                                     &extra[i].value, nullptr),
               "Original shadow/final/higher cache texture");
        for (unsigned i = 0; i < authored_textures.size(); ++i)
            resources.emplace(i, authored_textures[i].value);
        resources.emplace(bindings.textures[0], cache[0].value);
        resources.emplace(bindings.textures[1], cache[1].value);
        resources.emplace(bindings.textures[4], cache[2].value);
        resources.emplace(bindings.textures[3], extra[0].value);
        resources.emplace(final_handle, extra[1].value);
        resources.emplace(higher_handle, extra[2].value);
        resources.emplace(height_handle, world_height.value);
        for (unsigned i = 0; i < 5; ++i) {
            resources.emplace(bindings.lighting.sunlight[i], lights.cubes[i * 2].value);
            resources.emplace(bindings.lighting.shadowlight[i], lights.cubes[i * 2 + 1].value);
            resources.emplace(bindings.lighting.color_correct[i], lights.lut.value);
        }
    }
    IDirect3DTexture9 *texture(std::uint32_t handle) {
        auto *r = resources.at(handle);
        require(r->GetType() == D3DRTYPE_TEXTURE, "Native cache target/input must be 2D");
        return static_cast<IDirect3DTexture9 *>(r);
    }
    void begin(const MaterialCachePass &pass, bool conditional) {
        for (unsigned i = 0; i < 16; ++i)
            hr(device->SetTexture(i, nullptr), "Native plan unbind inputs");
        for (unsigned i = 1; i < 4; ++i)
            hr(device->SetRenderTarget(i, nullptr), "Native plan unbind previous MRT");
        for (unsigned i = 0; i < 4; ++i)
            if (pass.targets[i]) {
                Com<IDirect3DSurface9> surface;
                hr(texture(pass.targets[i])->GetSurfaceLevel(pass.mip, &surface.value),
                   "Plan target surface");
                hr(device->SetRenderTarget(i, surface.value), "Original plan target binding");
            }
        D3DSURFACE_DESC descriptor{};
        hr(texture(pass.targets[0])->GetLevelDesc(0, &descriptor), "Plan viewport descriptor");
        const D3DVIEWPORT9 viewport{0, 0, descriptor.Width, descriptor.Height, 0, 1};
        hr(device->SetViewport(&viewport), "Original plan viewport");
        hr(device->SetDepthStencilSurface(nullptr), "Original depth9 null surface");
        for (auto [state, value] : std::initializer_list<std::pair<D3DRENDERSTATETYPE, DWORD>>{
                 {D3DRS_ZENABLE, FALSE},
                 {D3DRS_ZWRITEENABLE, FALSE},
                 {D3DRS_STENCILENABLE, FALSE},
                 {D3DRS_CULLMODE, D3DCULL_NONE},
                 {D3DRS_SCISSORTESTENABLE, FALSE},
                 {D3DRS_DITHERENABLE, FALSE},
                 {D3DRS_SRGBWRITEENABLE, FALSE},
                 {D3DRS_FILLMODE, D3DFILL_SOLID},
                 {D3DRS_ALPHATESTENABLE, FALSE},
                 {D3DRS_ALPHABLENDENABLE, pass.blend_state != 0},
                 {D3DRS_SEPARATEALPHABLENDENABLE, pass.blend_state != 0},
                 {D3DRS_SRCBLEND, pass.blend_state == 8 ? D3DBLEND_SRCALPHA : D3DBLEND_ONE},
                 {D3DRS_DESTBLEND, pass.blend_state == 8 ? D3DBLEND_INVSRCALPHA : pass.blend_state == 4 ? D3DBLEND_ONE : D3DBLEND_ZERO},
                 {D3DRS_SRCBLENDALPHA, D3DBLEND_ONE},
                 {D3DRS_DESTBLENDALPHA, pass.blend_state ? D3DBLEND_ONE : D3DBLEND_ZERO},
                 {D3DRS_BLENDOP, D3DBLENDOP_ADD},
                 {D3DRS_BLENDOPALPHA, D3DBLENDOP_ADD},
                 {D3DRS_COLORWRITEENABLE, 15},
                 {D3DRS_COLORWRITEENABLE1, 15}})
            hr(device->SetRenderState(state, value), "Original plan raster state");
        if ((pass.flags & 4) || ((pass.flags & 128) && conditional)) {
            // This corpus uses only the original producer's in-range 0/1 colors.
            const auto &c = pass.clear_color;
            const auto color = D3DCOLOR_ARGB(unsigned(c[3] * 255.f), unsigned(c[0] * 255.f),
                                             unsigned(c[1] * 255.f), unsigned(c[2] * 255.f));
            hr(device->Clear(0, nullptr, D3DCLEAR_TARGET, color, 1.f, 0), "Original plan Clear");
        }
    }
    void execute(const MaterialCachePassPlan &plan, const MaterialPatchMesh &material,
                 const LightingPatchMesh &lighting, const GlMaterialCacheFrame &frame, bool conditional,
                 IDirect3DSurface9 *single_float_control = nullptr,
                 std::span<const float> original_frame = {},
                 const std::function<void(const MaterialCachePass &, const MaterialCachePassDraw &)> &decal = {}) {
        require(!single_float_control || plan.passes.size() == 1, "One native float control pass required");
        require(original_frame.empty() || original_frame.size() == 148, "Original terrain frame extent");
        for (const auto &pass : plan.passes) {
            begin(pass, conditional);
            if (single_float_control) {
                hr(device->SetRenderTarget(0, single_float_control),
                   "Native independent float control target");
                hr(device->Clear(0, nullptr, D3DCLEAR_TARGET, 0, 1.f, 0), "Native float control clear");
                hr(device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE),
                   "Native float shader value before blending");
            }
            for (const auto &draw : pass.draws) {
                if (pass.kind == MaterialCachePassKind::decal || pass.kind == MaterialCachePassKind::decal_shadow) {
                    require(bool(decal), "Native chain requires a decal shader backend");
                    decal(pass, draw);
                    continue;
                }
                if (!original_frame.empty()) {
                    hr(device->SetVertexShaderConstantF(0, original_frame.data(), 35),
                       "Original wrapper12 full vertex upload");
                    hr(device->SetPixelShaderConstantF(0, original_frame.data() + 64, 21),
                       "Original wrapper12 full pixel upload");
                }
                const auto &packet = plan.packets.at(draw.packet);
                std::array<float, 304> constants{};
                const bool inherited = pass.kind == MaterialCachePassKind::normal || pass.kind == MaterialCachePassKind::lighting;
                require((draw.constant_bytes == 0x4c0 || (inherited && draw.constant_bytes == 112)) &&
                            draw.constant_offset <= plan.constants.size() &&
                            plan.constants.size() - draw.constant_offset >= draw.constant_bytes,
                        "Native plan constants extent");
                std::memcpy(constants.data(), plan.constants.data() + draw.constant_offset, draw.constant_bytes);
                const auto ps = [&](unsigned start, std::span<const float> values) {
                    std::array<float, 4> lanes{};
                    std::copy(values.begin(), values.end(), lanes.begin());
                    hr(device->SetPixelShaderConstantF(start, lanes.data(), 1), "Native plan PS constants");
                };
                const auto set = [&](unsigned slot, bool cube = false) {
                    hr(device->SetTexture(slot, resources.at(packet.pixel_textures[slot])),
                       "Native plan sampler texture");
                    sampler(slot, packet.pixel_samplers[slot] == 5 ? D3DTADDRESS_CLAMP : D3DTADDRESS_WRAP);
                    if (cube)
                        hr(device->SetSamplerState(slot, D3DSAMP_SRGBTEXTURE, TRUE),
                           "Original cube sRGB read");
                };
                const void *vertices = material.vertices().data(), *indices = material.indices().data();
                unsigned stride = 48, vertex_count = material.columns() * material.rows();
                unsigned primitives = static_cast<unsigned>(material.indices().size() / 3);
                D3DFORMAT index_format = D3DFMT_INDEX16;
                if (pass.kind == MaterialCachePassKind::height) {
                    require(draw.selector == 0, "Native complete cache ordinary material selector");
                    hr(device->SetVertexDeclaration(declaration.value), "Original material declaration");
                    hr(device->SetVertexShader(vertex_shader.value), "Original material VS");
                    hr(device->SetPixelShader(pixel_shader.value), "Original material PS");
                    hr(device->SetVertexShaderConstantF(35, constants.data(), 1), "Original cache offsets");
                    hr(device->SetVertexShaderConstantF(109, frame.material_coefficients.data(), 96),
                       "Original global coefficients");
                    for (unsigned slot = 0; slot < 16; ++slot)
                        set(slot);
                } else if (pass.kind == MaterialCachePassKind::normal) {
                    hr(device->SetVertexDeclaration(normal_declaration.value), "Original normal declaration");
                    hr(device->SetVertexShader(normal_vs.value), "Original normal VS");
                    hr(device->SetPixelShader(normal_ps.value), "Original normal PS");
                    hr(device->SetPixelShaderConstantF(0, plan.normal_constants.data(), 2),
                       "Original normal constants");
                    set(0);
                    set(1);
                    vertices = quad.data();
                    indices = quad_indices.data();
                    stride = 32;
                    vertex_count = 4;
                    primitives = 2;
                    index_format = D3DFMT_INDEX32;
                } else if (pass.kind == MaterialCachePassKind::shadow) {
                    hr(device->SetVertexDeclaration(shadows.declaration.value),
                       "Original shadow declaration");
                    hr(device->SetVertexShader(shadows.vs.value), "Original shadow VS");
                    hr(device->SetPixelShader(shadows.ps.value), "Original shadow PS");
                    hr(device->SetVertexShaderConstantF(35, constants.data(), 1), "Original shadow offsets");
                    if (original_frame.empty()) {
                        ps(0, frame.world_scales);
                        ps(1, frame.sun_direction);
                        ps(6, frame.height_texture_scale);
                    }
                    ps(21, std::span(constants).subspan(0x490 / 4, 4));
                    ps(23, std::span(constants).subspan(0x4b0 / 4, 4));
                    set(0);
                } else {
                    require(pass.kind == MaterialCachePassKind::lighting && draw.selector == 0,
                            "Native complete cache diffuse lighting selector");
                    hr(device->SetVertexDeclaration(lights.declaration.value),
                       "Original lighting declaration");
                    hr(device->SetVertexShader(lights.vs.value), "Original lighting VS");
                    hr(device->SetPixelShader(lights.ps[0].value), "Original diffuse PS");
                    if (original_frame.empty())
                        ps(5, frame.half_vector);
                    for (unsigned slot : {0u, 1u, 2u, 8u})
                        set(slot);
                    set(4, true);
                    set(5, true);
                    vertices = lighting.vertices().data();
                    indices = lighting.indices().data();
                    stride = 24;
                }
                hr(device->BeginScene(), "Native cache chain BeginScene");
                const auto result = device->DrawIndexedPrimitiveUP(
                    D3DPT_TRIANGLELIST, 0, vertex_count, primitives, indices, index_format, vertices, stride);
                const auto ended = device->EndScene();
                hr(result, "Complete original cache chain shader draw");
                hr(ended, "Native cache chain EndScene");
            }
            for (auto handle : pass.targets)
                if (handle)
                    texture(handle)->GenerateMipSubLevels();
        }
    }
    Bytes pixels(std::uint32_t handle, unsigned mip) {
        auto *tex = texture(handle);
        Com<IDirect3DSurface9> surface;
        hr(tex->GetSurfaceLevel(mip, &surface.value), "Native chain readback surface");
        return surface_pixels(surface.value);
    }
    Bytes surface_pixels(IDirect3DSurface9 *surface) {
        D3DSURFACE_DESC descriptor{};
        hr(surface->GetDesc(&descriptor), "Native chain readback descriptor");
        Com<IDirect3DSurface9> staging;
        hr(device->CreateOffscreenPlainSurface(descriptor.Width, descriptor.Height, descriptor.Format,
                                               D3DPOOL_SYSTEMMEM, &staging.value, nullptr),
           "Native chain readback allocation");
        hr(device->GetRenderTargetData(surface, staging.value), "Native chain readback");
        const unsigned pitch = descriptor.Width * (descriptor.Format == D3DFMT_A32B32G32R32F   ? 16
                                                   : descriptor.Format == D3DFMT_A16B16G16R16F ? 8
                                                   : descriptor.Format == D3DFMT_A8R8G8B8      ? 4
                                                                                               : 2);
        Bytes result(pitch * descriptor.Height);
        D3DLOCKED_RECT lock{};
        hr(staging->LockRect(&lock, nullptr, D3DLOCK_READONLY), "Native chain pixels lock");
        const bool valid = lock.Pitch >= static_cast<int>(pitch);
        if (valid)
            for (unsigned y = 0; y < descriptor.Height; ++y)
                std::memcpy(result.data() + y * pitch, static_cast<const char *>(lock.pBits) + y * lock.Pitch,
                            pitch);
        hr(staging->UnlockRect(), "Native chain pixels unlock");
        require(valid, "Native chain readback pitch");
        return result;
    }
    Bytes float_control(const MaterialCachePassPlan &plan, const MaterialPatchMesh &material,
                        const LightingPatchMesh &lighting, const GlMaterialCacheFrame &frame) {
        Com<IDirect3DSurface9> target;
        hr(device->CreateRenderTarget(size, size, D3DFMT_A32B32G32R32F, D3DMULTISAMPLE_NONE, 0, FALSE,
                                      &target.value, nullptr),
           "Independent native float shader target");
        execute(plan, material, lighting, frame, false, target.value);
        return surface_pixels(target.value);
    }
};
} // namespace cache_chain_validation
