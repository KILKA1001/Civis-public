#pragma once
namespace {
struct Control {
    std::shared_ptr<GlContext> context;
    GlRenderApi gl;
    GLuint fbo = 0;
    std::vector<GLuint> owned;
    std::array<GLuint, 7> targets{};
    explicit Control(std::shared_ptr<GlContext> c,bool diagnostic_targets=true) : context(std::move(c)), gl(*context) {
        if(!diagnostic_targets)return; // Actual Party supplies held authored owners, never fixture textures.
        try {
            gl.CreateFramebuffers(1, &fbo);
            for (auto &v : targets)
                v = image({64, false, std::vector<float>(16384)});
            gl.Viewport(0, 0, 64, 64);
            for (auto flag : {GL_DEPTH_TEST, GL_BLEND, GL_CULL_FACE, GL_SCISSOR_TEST, GL_DITHER,
                              GL_FRAMEBUFFER_SRGB, GL_RASTERIZER_DISCARD})
                gl.Disable(flag);
        } catch (...) {
            release();
            throw;
        }
    }
    void release() noexcept {
        context->api();
        gl.BindFramebuffer(GL_FRAMEBUFFER, 0);
        gl.DeleteFramebuffers(1, &fbo);
        context->api().DeleteTextures(static_cast<GLsizei>(owned.size()), owned.data());
        owned.clear();
    }
    ~Control() { release(); }
    GLuint image(const FloatImage &source) {
        const auto &api = context->api();
        GLuint id = 0;
        api.CreateTextures(source.volume ? GL_TEXTURE_3D : GL_TEXTURE_2D, 1, &id);
        owned.push_back(id);
        const auto size = source.size;
        if (source.volume) {
            const auto storage =
                reinterpret_cast<PFNGLTEXTURESTORAGE3DPROC>(context->proc_address("glTextureStorage3D"));
            storage(id, 1, GL_RGBA32F, size, size, size);
            api.TextureSubImage3D(id, 0, 0, 0, 0, size, size, size, GL_RGBA, GL_FLOAT, source.values.data());
        } else {
            api.TextureStorage2D(id, 1, GL_RGBA32F, size, size);
            api.TextureSubImage2D(id, 0, 0, 0, size, size, GL_RGBA, GL_FLOAT, source.values.data());
        }
        std::vector<float> actual(source.values.size());
        api.GetTextureImage(id, 0, GL_RGBA, GL_FLOAT, static_cast<GLsizei>(actual.size() * 4), actual.data());
        context->check("LAND GL source upload/readback");
        require(std::memcmp(actual.data(), source.values.data(), actual.size() * 4) == 0,
                "LAND GL source upload identity");
        return id;
    }
    void attach(std::initializer_list<GLuint> textures, bool poison = false) {
        unsigned i = 0;
        for (auto texture : textures)
            gl.NamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT0 + i++, texture, 0);
        if (i == 1)
            gl.NamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT1, 0, 0);
        const GLenum buffers[]{GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
        gl.NamedFramebufferDrawBuffers(fbo, i, buffers);
        require(gl.CheckNamedFramebufferStatus(fbo, GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE,
                "LAND FBO incomplete");
        gl.BindFramebuffer(GL_FRAMEBUFFER, fbo);
        const float value = poison ? std::numeric_limits<float>::quiet_NaN() : 0.f;
        const float clear[]{value, value, value, value};
        for (unsigned j = 0; j < i; ++j)
            gl.ClearNamedFramebufferfv(fbo, GL_COLOR, j, clear);
    }
    std::vector<float> pixels(GLuint texture) {
        std::vector<float> result(16384);
        context->api().GetTextureImage(texture, 0, GL_RGBA, GL_FLOAT, 65536, result.data());
        context->check("LAND readback");
        require(std::all_of(result.begin(), result.end(), [](float v) { return std::isfinite(v); }),
                "LAND nonfinite/unwritten output");
        return result;
    }
    std::vector<float> draw(GlLandMaterialRenderer &renderer, const LandMaterialConstants &parameters,
                            const GlLandMaterialInputs &inputs) {
        attach({targets[6]}, true);
        renderer.draw(parameters, inputs, 64, 64);
        return pixels(targets[6]);
    }
};
LandMatrix identity() { return {{{1, 0, 0, 0}, {0, 1, 0, 0}, {0, 0, 1, 0}, {0, 0, 0, 1}}}; }
LandMaterialConstants constants(unsigned columns, unsigned rows, unsigned variant) {
    LandMaterialConstants c;
    c.world = identity();
    c.world[3] = {2, -3, 0, 1};
    const float sx = 2.f / (columns - 1), sy = -2.f / ((rows - 1) * .866f);
    c.view_projection = {{{sx, 0, 0, 0}, {0, sy, 0, 0}, {0, 0, .01f, 0}, {-1 - 2 * sx, 1 + 3 * sy, 0, 1}}};
    c.composite_projection = {
        {{.011f, .003f, 0, 0}, {.002f, .017f, 0, 0}, {.004f, -.003f, 0, 0}, {-.43f, .61f, 0, 1}}};
    c.shadow_projection = {{{.02f, .003f, 0, .001f},
                            {-.002f, .021f, 0, -.0005f},
                            {.001f, -.003f, 0, .0002f},
                            {.25f, -.3f, 0, 1}}};
    c.fow_scale = {.021f, -.017f, 3.25f, -2.5f};
    c.world_scales = {1, .866f, .2f, 0};
    c.height_texture_scale = {1.f / 1024, -1.f / 1024, 0, 0};
    c.offset = {0, 0, 64, 64};
    c.camera = {4, -2, 110, 1};
    c.fog_color = {.7f, .4f, .2f, 1};
    c.fog = {.007f, .00008f, 19, .37f};
    c.shadow_fade = .65f;
    c.apply_diffuse = 1;
    c.apply_specular = 1;
    if (variant == 0) {
        c.shadow_fade = 0;
        c.fog[1] = 0;
    }
    if (variant == 2)
        c.apply_diffuse = 0;
    if (variant == 3)
        c.apply_specular = 0;
    if (variant == 4) {
        c.apply_diffuse = -1;
        c.apply_specular = -1;
    }
    if (variant == 5) {
        c.shadow_fade = 1.2f;
        c.camera = {-30, 21, 80, 1};
        c.fog[0] = .03f;
        c.fog[1] = .0004f;
    }
    return c;
}
std::array<FloatImage, 10> fixtures() {
    std::array<FloatImage, 10> images;
    for (unsigned slot = 0; slot < 10; ++slot) {
        auto &f = images[slot];
        f.size = slot == 6 || slot == 7 ? 16 : 64;
        f.volume = slot == 6 || slot == 7;
        f.values.resize(std::size_t(f.size) * f.size * (f.volume ? f.size : 1) * 4);
        for (std::size_t i = 0; i < f.values.size(); ++i) {
            const unsigned lane = i % 4, x = i / 4 % f.size, y = i / 4 / f.size % f.size,
                           z = i / 4 / f.size / f.size;
            float v = 0;
            if (f.volume)
                v = lane == 3 ? 1.f : .05f + (lane == 0 ? x + z : lane == 1 ? y + x : z + y) / 40.f;
            else
                switch (slot) {
                case 0:
                    v = lane == 3 ? 1.f : .15f + (x + lane * 13) / 150.f;
                    break;
                case 1:
                    v = lane == 3 ? (x % 9) / 12.f : (lane + 1) * .013f + (y % 7) * .001f;
                    break;
                case 2:
                    v = lane == 0 ? .2f + y / 100.f : .731f;
                    break;
                case 3:
                    v = lane == 0 ? -.2f + (x % 11) / 7.f : .831f;
                    break;
                case 4:
                    v = lane == 0 ? x / 63.f : lane == 2 ? y / 63.f : .231f;
                    break;
                case 5:
                    v = lane == 3 ? 1.f : .1f + (y + lane * 3) / 220.f;
                    break;
                case 8:
                    v = lane == 3 ? .25f : .01f + (x % 17 + lane * 5) / 150.f;
                    break;
                case 9:
                    v = .1f + (y % 13 + lane * 4) / 35.f;
                    break;
                }
            f.values[i] = v;
        }
    }
    return images;
}
LightingCubeAssets cube_assets(const fs::path &root) {
    AssetEnvironment environment(root);
    const auto archive = [&](const char *name) {
        const auto v = environment.open_fpk_archive(name);
        require(std::holds_alternative<std::shared_ptr<FpkSource>>(v), "Exact cube archive");
        return std::get<std::shared_ptr<FpkSource>>(v);
    };
    auto base = archive("Resource/DX9/TerrainTextures.fpk"),
         upgrade = archive("Resource/DX9/Upgrade1Textures.fpk");
    return LightingCubeAssets::load([&](const LightingSource &s) {
        const auto name = s.native_path.substr(s.native_path.find_last_of('\\') + 1);
        const auto found = (name.ends_with("_2.dds") ? upgrade : base)->find(AssetKey(name));
        require(std::holds_alternative<AssetHandle>(found), "Exact lighting cube");
        return std::get<AssetHandle>(found);
    });
}
} // namespace
