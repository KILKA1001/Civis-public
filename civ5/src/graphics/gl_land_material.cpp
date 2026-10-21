#include "civ5/graphics/gl_land_material.hpp"
#include "civ5/graphics/gl_material_blend.hpp"
#include "gl_native_clip.hpp"
#include "gl_sampled_input.hpp"
#include <algorithm>
#include <exception>
#include <stdexcept>
namespace civ5::graphics {
namespace {
void require(bool ok, const char *why) {
    if (!ok)
        throw std::invalid_argument(why);
}
constexpr std::string_view rgb565_sampling = R"GLSL(
dvec4 nativeLandRgb565Level(sampler2D source,vec2 uv,int level){
    ivec2 size=textureSize(source,level);
    precise vec2 pixel=uv*vec2(size);pixel=pixel-vec2(.5);
    ivec2 base=ivec2(floor(pixel));
    precise vec2 fraction=pixel-vec2(base);
    uvec2 w=uvec2(floor(fraction*256.0+vec2(.5)));
    uvec4 a=uvec4(round(texelFetch(source,clamp(base,ivec2(0),size-1),level)*255.0));
    uvec4 b=uvec4(round(texelFetch(source,clamp(base+ivec2(1,0),ivec2(0),size-1),level)*255.0));
    uvec4 c=uvec4(round(texelFetch(source,clamp(base+ivec2(0,1),ivec2(0),size-1),level)*255.0));
    uvec4 d=uvec4(round(texelFetch(source,clamp(base+ivec2(1),ivec2(0),size-1),level)*255.0));
    return dvec4((a*(256u-w.x)+b*w.x)*(256u-w.y)+(c*(256u-w.x)+d*w.x)*w.y);
}
vec4 nativeLandRgb565(sampler2D source,vec2 uv){
    float last=float(textureQueryLevels(source)-1);
    float level=clamp(textureQueryLod(source,uv).y,0.0,last);
    int low=int(floor(level));
    double weight=double(level-float(low));
    // Integer-level bilinear sampling has its own UNORM rounding. Full
    // original mip0/mip1 observations match hardware, including magnification.
    if(weight==0.0)return textureLod(source,uv,float(low));
    dvec4 first=nativeLandRgb565Level(source,uv,low);
    dvec4 second=nativeLandRgb565Level(source,uv,min(low+1,int(last)));
    precise dvec4 result=first*(1.0-weight)+second*weight;
    // Complete original HAL shader observations, with unchanged primary color,
    // use the fixed UNORM8 coefficient 0x01010101 / 2^32. Dividing by 255
    // instead changes LUT coordinates at rounding boundaries. The denominator
    // also includes the two 8-bit spatial weights retained above.
    return vec4(result*(double(16843009u)/double(281474976710656.0)));
}
)GLSL";
} // namespace
std::string land_material_fragment_source(materials::LandPixelProgram pixel, LandShadowArithmetic arithmetic,
                                          LandMipSampling mip_sampling) {
    require(arithmetic == LandShadowArithmetic::instruction_order ||
                arithmetic == LandShadowArithmetic::measured_fma_t_x_z_y,
            "Invalid LAND shadow arithmetic profile");
    require(mip_sampling == LandMipSampling::ordinary || mip_sampling == LandMipSampling::d3d9_trilinear ||
                mip_sampling == LandMipSampling::d3d9_rgb565,
            "Invalid LAND mip sampling profile");
    std::string source(land_material_fragment_shader(pixel));
    if (arithmetic == LandShadowArithmetic::measured_fma_t_x_z_y)
        source.insert(source.find('\n') + 1, "#define CIV5_MEASURED_SHADOW_AFFINE 1\n");
    if (mip_sampling != LandMipSampling::ordinary) {
        // Reuse the tested material sampler: integer-level hardware bilinear
        // samples plus explicit float interpolation at textureQueryLod.y.
        // The two 3D LUT slots and vertex displacement retain their original paths.
        source.insert(source.find('\n') + 1, material_sampling_glsl());
        if (mip_sampling == LandMipSampling::d3d9_rgb565)
            source.insert(source.find('\n') + 1, rgb565_sampling);
        for (unsigned slot : {0u, 1u, 2u, 3u, 4u, 5u, 8u, 9u}) {
            if (slot == 5 && mip_sampling == LandMipSampling::d3d9_rgb565)
                continue;
            const std::string token = "texture(s" + std::to_string(slot) + ",";
            const std::string replacement =
                (slot == 0 && mip_sampling == LandMipSampling::d3d9_rgb565 ? "nativeLandRgb565(s"
                                                                           : "nativeMaterialImplicit(s") +
                std::to_string(slot) + ",";
            for (auto at = source.find(token); at != std::string::npos;
                 at = source.find(token, at + replacement.size()))
                source.replace(at, token.size(), replacement);
        }
    }
    return source;
}
struct GlLandMaterialRenderer::Impl {
    std::shared_ptr<GlContext> context;
    GlRenderApi gl;
    materials::LandTechnique technique;
    LandMipSampling mip_sampling;
    GLuint program = 0, vao = 0, vb = 0, ib = 0;
    std::array<GLuint, 3> samplers{};
    GLsizei vertices = 0, indices = 0;
    std::shared_ptr<GlLandGeometryPool> geometry_pool;
    GlLandGeometryLease geometry_lease;
    GLint vs = -1, ps = -1;
    Impl(std::shared_ptr<GlContext> c, const materials::LandMaterialMesh *mesh,
         std::shared_ptr<GlLandGeometryPool> pool, const GlLandGeometryLease &lease, unsigned selector,
         LandShadowArithmetic arithmetic, LandMipSampling mip_sampling)
        : context(std::move(c)), gl(*context), technique(materials::land_technique(selector)),
          mip_sampling(mip_sampling), vertices(static_cast<GLsizei>(mesh ? mesh->vertices().size() : lease.vertex_count())),
          indices(static_cast<GLsizei>(mesh ? mesh->indices().size() : lease.index_count())),
          geometry_pool(std::move(pool)), geometry_lease(lease) {
        try {
            context->check("Before LAND material initialization");
            program = gl.CreateProgram();
            const auto fragment = land_material_fragment_source(technique.pixel, arithmetic, mip_sampling);
            for (auto [stage, text] :
                 {std::pair{GL_VERTEX_SHADER, land_material_vertex_shader(technique.input)},
                  {GL_FRAGMENT_SHADER, std::string_view(fragment)}}) {
                const auto shader = gl.CreateShader(stage);
                const char *source = text.data();
                const auto length = static_cast<GLint>(text.size());
                gl.ShaderSource(shader, 1, &source, &length);
                gl.CompileShader(shader);
                GLint ok = 0;
                gl.GetShaderiv(shader, GL_COMPILE_STATUS, &ok);
                if (!ok) {
                    char log[8192]{};
                    gl.GetShaderInfoLog(shader, sizeof(log), nullptr, log);
                    gl.DeleteShader(shader);
                    throw std::runtime_error(log);
                }
                gl.AttachShader(program, shader);
                gl.DeleteShader(shader);
            }
            const char *varyings[]{"tfClip", "tfWorld", "landT0", "landT1", "landWorld", "landNdc"};
            gl.TransformFeedbackVaryings(program, 6, varyings, GL_INTERLEAVED_ATTRIBS);
            gl.LinkProgram(program);
            GLint ok = 0;
            gl.GetProgramiv(program, GL_LINK_STATUS, &ok);
            if (!ok) {
                char log[8192]{};
                gl.GetProgramInfoLog(program, sizeof(log), nullptr, log);
                throw std::runtime_error(log);
            }
            vs = gl.GetUniformLocation(program, "uV[0]");
            ps = gl.GetUniformLocation(program, "uP[0]");
            require(vs >= 0 && (technique.pixel == materials::LandPixelProgram::overlay || ps >= 0),
                    "Missing LAND constants");
            gl.CreateVertexArrays(1, &vao);
            if (geometry_pool) {
                const auto names = geometry_pool->buffers(geometry_lease, *context);
                vb = names[0];
                ib = names[1];
            } else {
                gl.CreateBuffers(1, &vb);
                gl.CreateBuffers(1, &ib);
                gl.NamedBufferData(vb, mesh->vertices().size_bytes(), mesh->vertices().data(), GL_STATIC_DRAW);
                gl.NamedBufferData(ib, mesh->indices().size_bytes(), mesh->indices().data(), GL_STATIC_DRAW);
            }
            gl.VertexArrayVertexBuffer(vao, 0, vb, 0, 24);
            gl.VertexArrayElementBuffer(vao, ib);
            for (GLuint i = 0; i < 3; ++i) {
                gl.EnableVertexArrayAttrib(vao, i);
                gl.VertexArrayAttribBinding(vao, i, 0);
                gl.VertexArrayAttribFormat(vao, i,
                                           i == 0   ? 3
                                           : i == 1 ? 4
                                                    : 2,
                                           i == 0 ? GL_FLOAT : GL_HALF_FLOAT, GL_FALSE,
                                           i == 0   ? 0
                                           : i == 1 ? 12
                                                    : 20);
            }
            gl.CreateSamplers(3, samplers.data());
            for (unsigned i = 0; i < 3; ++i) {
                for (auto wrap : {GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T, GL_TEXTURE_WRAP_R})
                    gl.SamplerParameteri(samplers[i], wrap, i == 0 ? GL_CLAMP_TO_EDGE : GL_REPEAT);
                gl.SamplerParameteri(samplers[i], GL_TEXTURE_MIN_FILTER,
                                     i == 2 ? GL_NEAREST_MIPMAP_NEAREST : GL_LINEAR_MIPMAP_LINEAR);
                gl.SamplerParameteri(samplers[i], GL_TEXTURE_MAG_FILTER, i == 2 ? GL_NEAREST : GL_LINEAR);
                gl.SamplerParameterf(samplers[i], GL_TEXTURE_MAX_ANISOTROPY, 1);
                gl.SamplerParameterf(samplers[i], GL_TEXTURE_LOD_BIAS, 0);
                gl.SamplerParameterf(samplers[i], GL_TEXTURE_MIN_LOD, -1000);
                gl.SamplerParameterf(samplers[i], GL_TEXTURE_MAX_LOD, 1000);
            }
            context->check("LAND material geometry/program/samplers");
        } catch (...) {
            release();
            throw;
        }
    }
    void release() noexcept {
        try {
            const auto &api = context->api();
            if (!geometry_pool) {
                gl.DeleteBuffers(1, &vb);
                gl.DeleteBuffers(1, &ib);
            }
            gl.DeleteVertexArrays(1, &vao);
            gl.DeleteSamplers(3, samplers.data());
            if (program) {
                GLint current = 0;
                api.GetIntegerv(GL_CURRENT_PROGRAM, &current);
                if (static_cast<GLuint>(current) == program)
                    gl.UseProgram(0);
                gl.DeleteProgram(program);
            }
            vb = ib = vao = program = 0;
            samplers = {};
        } catch (...) {
            std::terminate();
        }
    }
    ~Impl() { release(); }
    void constants(const materials::LandMaterialConstants &c, int width, int height) {
        c.validate();
        require(width > 0 && height > 0, "Positive LAND target dimensions required");
        std::array<materials::LandVector, 40> v{};
        std::array<materials::LandVector, 23> p{};
        std::copy(c.view_projection.begin(), c.view_projection.end(), v.begin());
        std::copy(c.composite_projection.begin(), c.composite_projection.end(), v.begin() + 4);
        v[9] = c.fow_scale;
        v[16] = c.world_scales;
        v[22] = c.height_texture_scale;
        std::copy(c.world.begin(), c.world.end(), v.begin() + 35);
        v[39] = c.offset;
        p[3] = c.camera;
        p[9][0] = c.shadow_fade;
        p[11][0] = c.apply_specular;
        p[12][0] = c.apply_diffuse;
        std::copy(c.shadow_projection.begin(), c.shadow_projection.end(), p.begin() + 15);
        p[21] = c.fog_color;
        p[22] = c.fog;
        gl.ProgramUniform4fv(program, vs, 40, v[0].data());
        if (ps >= 0)
            gl.ProgramUniform4fv(program, ps, 23, p[0].data());
    }
    void bind_height(GLuint texture) {
        if (technique.input == materials::LandVertexInput::local_grid) {
            detail::validate_sampled_2d(*context, gl, texture);
            context->api().BindTextureUnit(10, texture);
            gl.BindSampler(10, samplers[2]);
        }
    }
};
GlLandMaterialRenderer::GlLandMaterialRenderer(std::shared_ptr<GlContext> context,
                                               const materials::LandMaterialMesh &mesh, unsigned selector,
                                               LandShadowArithmetic arithmetic,
                                               LandMipSampling mip_sampling) {
    require(bool(context), "LAND renderer requires owned context");
    const auto technique = materials::land_technique(selector);
    require(mesh.input() == technique.input, "LAND input convention would double displacement/placement");
    require(!mesh.vertices().empty() && !mesh.indices().empty(), "Moved-from LAND material mesh");
    impl_ = std::make_unique<Impl>(std::move(context), &mesh, nullptr, GlLandGeometryLease{}, selector, arithmetic, mip_sampling);
}
GlLandMaterialRenderer::GlLandMaterialRenderer(std::shared_ptr<GlContext> context,
                                               std::shared_ptr<GlLandGeometryPool> pool,
                                               const GlLandGeometryLease &lease, unsigned selector,
                                               LandShadowArithmetic arithmetic, LandMipSampling mip_sampling) {
    require(bool(context) && bool(pool), "Pooled LAND renderer requires owned context and geometry");
    require(lease.input() == materials::land_technique(selector).input, "Pooled LAND input convention mismatch");
    pool->buffers(lease, *context);
    impl_ = std::make_unique<Impl>(std::move(context), nullptr, std::move(pool), lease, selector, arithmetic, mip_sampling);
}
GlLandMaterialRenderer::~GlLandMaterialRenderer() = default;
void GlLandMaterialRenderer::set_geometry(const GlLandGeometryLease &lease) {
    auto &s = *impl_;
    require(bool(s.geometry_pool) && lease.input() == s.technique.input, "LAND geometry pool/input mismatch");
    const auto names = s.geometry_pool->buffers(lease, *s.context);
    s.gl.VertexArrayVertexBuffer(s.vao, 0, names[0], 0, 24);
    s.gl.VertexArrayElementBuffer(s.vao, names[1]);
    s.context->check("LAND geometry lease binding");
    s.geometry_lease = lease;
    s.vb = names[0];
    s.ib = names[1];
    s.vertices = static_cast<GLsizei>(lease.vertex_count());
    s.indices = static_cast<GLsizei>(lease.index_count());
}
void GlLandMaterialRenderer::draw(const materials::LandMaterialConstants &constants,
                                  const GlLandMaterialInputs &inputs, int width, int height) {
    constants.validate();
    require(width > 0 && height > 0, "Positive LAND target dimensions required");
    auto &s = *impl_;
    if (s.geometry_pool) s.geometry_pool->buffers(s.geometry_lease, *s.context);
    const auto &api = s.context->api();
    const auto overlay = s.technique.pixel == materials::LandPixelProgram::overlay;
    const unsigned count = s.technique.pixel == materials::LandPixelProgram::cached_specular ? 10 : 8;
    for (unsigned i = 0; i < count; ++i)
        if (!overlay || i == 1)
            detail::validate_sampled_texture(*s.context, s.gl, inputs.pixel[i],
                                             i == 6 || i == 7 ? GL_TEXTURE_3D : GL_TEXTURE_2D);
    if (!overlay && s.mip_sampling == LandMipSampling::d3d9_rgb565) {
        GLint format = 0;
        api.GetTextureLevelParameteriv(inputs.pixel[0], 0, GL_TEXTURE_INTERNAL_FORMAT, &format);
        require(format == GL_RGB565, "Native LAND RGB565 sampling requires an RGB565 cache");
    }
    s.bind_height(inputs.height);
    s.constants(constants, width, height);
    for (unsigned i = 0; i < count; ++i)
        if (!overlay || i == 1) {
            api.BindTextureUnit(i, inputs.pixel[i]);
            s.gl.BindSampler(i, i >= 3 && i <= 5 ? s.samplers[1] : s.samplers[0]);
        }
    s.gl.UseProgram(s.program);
    s.gl.BindVertexArray(s.vao);
    const detail::NativeClipScope clip(*s.context, s.gl);
    s.gl.DrawElements(GL_TRIANGLES, s.indices, GL_UNSIGNED_SHORT, nullptr);
    s.context->check("Full key49 LAND material draw");
}
void GlLandMaterialRenderer::draw_packet(const materials::MaterialCachePacket &packet,
                                          const materials::LandMaterialConstants &constants,
                                          const std::function<GLuint(std::uint16_t)> &resolve,
                                          int width, int height) {
    const auto &s = *impl_;
    require(s.geometry_pool && resolve, "LAND packet draw requires a geometry owner and resource resolver");
    const auto &lease = s.geometry_lease;
    s.geometry_pool->buffers(lease, *s.context);
    require(packet.key == 0x49 && packet.pixel_count == 10 &&
                packet.geometry == std::array<std::uint32_t, 5>{lease.index_buffer(), lease.vertex_buffer(),
                                                                lease.index_count() / 3, 0, 0},
            "LAND packet does not reference the current complete geometry");
    for (unsigned i = 0; i < 10; ++i)
        require(packet.pixel_samplers[i] == (i >= 3 && i <= 5 ? 0 : 5), "Unsupported LAND packet pixel sampler");
    const bool displaced = lease.input() == materials::LandVertexInput::local_grid;
    require(packet.vertex_count == (displaced ? 1 : 0) && (!displaced || packet.vertex_samplers[0] == 9),
            "LAND packet displacement convention differs from its geometry");
    GlLandMaterialInputs inputs;
    for (unsigned i = 0; i < 10; ++i) inputs.pixel[i] = resolve(packet.pixel_textures[i]);
    if (displaced) inputs.height = resolve(packet.vertex_textures[0]);
    draw(constants, inputs, width, height);
}
std::vector<float> GlLandMaterialRenderer::capture(const materials::LandMaterialConstants &constants,
                                                   GLuint height, bool indexed) {
    constants.validate();
    auto &s = *impl_;
    if (s.geometry_pool) s.geometry_pool->buffers(s.geometry_lease, *s.context);
    const auto &api = s.context->api();
    GLint active = 0, previous = 0;
    api.GetIntegerv(GL_TRANSFORM_FEEDBACK_ACTIVE, &active);
    require(!active, "Nested LAND transform feedback rejected");
    api.GetIntegerv(GL_TRANSFORM_FEEDBACK_BINDING, &previous);
    const auto discard = s.gl.IsEnabled(GL_RASTERIZER_DISCARD);
    s.bind_height(height);
    s.constants(constants, 64, 64);
    GLuint buffer = 0, feedback = 0;
    const auto count = indexed ? s.indices : s.vertices;
    std::vector<float> output(static_cast<std::size_t>(count) * 21);
    const auto cleanup = [&] {
        s.gl.BindTransformFeedback(GL_TRANSFORM_FEEDBACK, previous);
        if (!discard)
            s.gl.Disable(GL_RASTERIZER_DISCARD);
        s.gl.DeleteTransformFeedbacks(1, &feedback);
        s.gl.DeleteBuffers(1, &buffer);
    };
    bool started = false;
    try {
        s.gl.CreateBuffers(1, &buffer);
        s.gl.NamedBufferData(buffer, output.size() * 4, nullptr, GL_STREAM_READ);
        s.gl.CreateTransformFeedbacks(1, &feedback);
        s.gl.TransformFeedbackBufferBase(feedback, 0, buffer);
        s.gl.BindTransformFeedback(GL_TRANSFORM_FEEDBACK, feedback);
        s.gl.Enable(GL_RASTERIZER_DISCARD);
        s.gl.UseProgram(s.program);
        s.gl.BindVertexArray(s.vao);
        s.gl.BeginTransformFeedback(indexed ? GL_TRIANGLES : GL_POINTS);
        started = true;
        if (indexed)
            s.gl.DrawElements(GL_TRIANGLES, count, GL_UNSIGNED_SHORT, nullptr);
        else
            s.gl.DrawArrays(GL_POINTS, 0, count);
        s.gl.EndTransformFeedback();
        started = false;
        s.gl.GetNamedBufferSubData(buffer, 0, output.size() * 4, output.data());
        s.context->check("LAND original varyings capture");
        cleanup();
    } catch (...) {
        if (started)
            s.gl.EndTransformFeedback();
        cleanup();
        throw;
    }
    return output;
}
} // namespace civ5::graphics
