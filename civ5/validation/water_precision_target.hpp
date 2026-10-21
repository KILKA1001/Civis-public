#pragma once
// Validation-only second MRT. The complete original color stays live while an
// additional output observes an intermediate register from the same program.
struct WaterPrecisionTarget {
    GlContext &context;
    GlRenderApi &gl;
    GLuint framebuffer, texture = 0;
    IDirect3DDevice9 *device;
    Com<IDirect3DSurface9> native_color, native_pixels;
    DWORD write_mask = 15;
    unsigned width, height;
    WaterPrecisionTarget(GlContext &c, GlRenderApi &api, GLuint fbo, IDirect3DDevice9 *d, unsigned w = 64,
                         unsigned h = 64)
        : context(c), gl(api), framebuffer(fbo), device(d), width(w), height(h) {
        try {
            checked(device->GetRenderState(D3DRS_COLORWRITEENABLE1, &write_mask), "Water observation mask");
            checked(device->CreateRenderTarget(width, height, D3DFMT_A32B32G32R32F, D3DMULTISAMPLE_NONE, 0,
                                               FALSE, &native_color.value, nullptr),
                    "Water observation target");
            checked(device->CreateOffscreenPlainSurface(width, height, D3DFMT_A32B32G32R32F,
                                                        D3DPOOL_SYSTEMMEM, &native_pixels.value, nullptr),
                    "Water observation readback");
            checked(device->SetRenderTarget(1, native_color.value), "Water observation attachment");
            checked(device->SetRenderState(D3DRS_COLORWRITEENABLE1, 15), "Water observation write mask");
            context.api().CreateTextures(GL_TEXTURE_2D, 1, &texture);
            context.api().TextureStorage2D(texture, 1, GL_RGBA32F, width, height);
            gl.NamedFramebufferTexture(framebuffer, GL_COLOR_ATTACHMENT1, texture, 0);
            const GLenum buffers[]{GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
            gl.NamedFramebufferDrawBuffers(framebuffer, 2, buffers);
            require(gl.CheckNamedFramebufferStatus(framebuffer, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
                    "Complete water observation framebuffer");
            context.check("Water second MRT initialization");
        } catch (...) {
            release();
            throw;
        }
    }
    void release() noexcept {
        device->SetRenderTarget(1, nullptr);
        device->SetRenderState(D3DRS_COLORWRITEENABLE1, write_mask);
        gl.NamedFramebufferTexture(framebuffer, GL_COLOR_ATTACHMENT1, 0, 0);
        const GLenum buffer = GL_COLOR_ATTACHMENT0;
        gl.NamedFramebufferDrawBuffers(framebuffer, 1, &buffer);
        context.api().DeleteTextures(1, &texture);
    }
    ~WaterPrecisionTarget() { release(); }
    void clear() {
        const float sentinel[]{187 / 255.f, 51 / 255.f, 119 / 255.f, 1};
        gl.ClearNamedFramebufferfv(framebuffer, GL_COLOR, 1, sentinel);
    }
    std::vector<float> gl_output() {
        std::vector<float> result(width * height * 4);
        context.api().GetTextureImage(texture, 0, GL_RGBA, GL_FLOAT, result.size() * 4, result.data());
        context.check("Water intermediate observation");
        return result;
    }
    std::vector<float> native_output() {
        checked(device->GetRenderTargetData(native_color.value, native_pixels.value), "Water observation readback");
        D3DLOCKED_RECT lock{};
        checked(native_pixels->LockRect(&lock, nullptr, D3DLOCK_READONLY), "Water observation lock");
        std::vector<float> result(width * height * 4);
        const bool valid = lock.pBits && lock.Pitch >= int(width * 16);
        if (valid)
            for (unsigned y = 0; y < height; ++y)
                std::memcpy(result.data() + y * width * 4,
                            static_cast<char *>(lock.pBits) + std::size_t(y) * lock.Pitch, width * 16);
        checked(native_pixels->UnlockRect(), "Water observation unlock");
        require(valid, "Water observation pitch");
        return result;
    }
};
