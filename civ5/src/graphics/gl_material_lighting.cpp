#include "civ5/graphics/gl_material_lighting.hpp"
#include "gl_sampled_input.hpp"
#include <algorithm>
#include <cmath>
#include <exception>
#include <optional>
#include <stdexcept>

namespace civ5::graphics {
GlTexture make_lighting_coordinate_probe(const GlContext &context) {
    std::vector<assets::Rgba8CubeLevel> levels;
    for (unsigned mip = 0; mip < 8; ++mip) {
        const auto size = 128u >> mip;
        assets::Rgba8CubeLevel level{size, {}};
        for (unsigned face = 0; face < 6; ++face) {
            auto &bytes = level.faces[face];
            bytes.resize(size * size * 4);
            for (unsigned i = 0; i < bytes.size(); ++i) {
                const auto x = i / 4 % size;
                bytes[i] = (i % 4 == 3 || (face == 4 ? x % 2 == 1 : face == 1 || (face == 0 && size == 1)))
                               ? 255
                               : 0;
            }
        }
        levels.push_back(std::move(level));
    }
    return GlTexture::rgba8_cube(context, levels);
}
namespace {
void require(bool ok, const char *why) {
    if (!ok)
        throw std::invalid_argument(why);
}
} // namespace
struct GlMaterialLightingRenderer::Impl {
    std::shared_ptr<GlContext> context;
    GlRenderApi gl;
    bool specular;
    LightingCubeSampling sampling;
    std::optional<GlTexture> probe;
    GLuint program = 0, vao = 0, vb = 0, ib = 0, clamp_sampler = 0, cube_sampler = 0;
    GLsizei count;
    GLint center = -1, half = -1, tap_filter = -1, tap_coordinates = -1;
    Impl(std::shared_ptr<GlContext> c, const materials::LightingPatchMesh &mesh, bool spec,
         LightingCubeSampling mode)
        : context(std::move(c)), gl(*context), specular(spec), sampling(mode),
          count(static_cast<GLsizei>(mesh.indices().size())) {
        try {
            context->check("Before material lighting initialization");
            program = gl.CreateProgram();
            const auto fragment = material_lighting_fragment_shader(specular);
            for (auto [kind, source] : {std::pair{GL_VERTEX_SHADER, material_lighting_vertex_shader()},
                                        {GL_FRAGMENT_SHADER, std::string_view(fragment)}}) {
                const auto shader = gl.CreateShader(kind);
                const char *text = source.data();
                const auto length = static_cast<GLint>(source.size());
                gl.ShaderSource(shader, 1, &text, &length);
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
            gl.LinkProgram(program);
            GLint ok = 0;
            gl.GetProgramiv(program, GL_LINK_STATUS, &ok);
            if (!ok) {
                char log[8192]{};
                gl.GetProgramInfoLog(program, sizeof(log), nullptr, log);
                throw std::runtime_error(log);
            }
            center = gl.GetUniformLocation(program, "uPixelCenter");
            half = gl.GetUniformLocation(program, "uHalf");
            tap_filter = gl.GetUniformLocation(program, "uCubeTapFilter");
            tap_coordinates = gl.GetUniformLocation(program, "uCubeTapCoordinates");
            require(center >= 0 && tap_filter >= 0 && tap_coordinates >= 0 && (!specular || half >= 0),
                    "Missing lighting uniforms");
            gl.CreateVertexArrays(1, &vao);
            gl.CreateBuffers(1, &vb);
            gl.CreateBuffers(1, &ib);
            gl.NamedBufferData(vb, mesh.vertices().size_bytes(), mesh.vertices().data(), GL_STATIC_DRAW);
            gl.NamedBufferData(ib, mesh.indices().size_bytes(), mesh.indices().data(), GL_STATIC_DRAW);
            gl.VertexArrayVertexBuffer(vao, 0, vb, 0, 24);
            gl.VertexArrayElementBuffer(vao, ib);
            for (GLuint i = 0; i < 2; ++i) {
                gl.EnableVertexArrayAttrib(vao, i);
                gl.VertexArrayAttribBinding(vao, i, 0);
                gl.VertexArrayAttribFormat(vao, i, i == 0 ? 4 : 2, GL_HALF_FLOAT, GL_FALSE, i == 0 ? 12 : 20);
            }
            gl.CreateSamplers(1, &clamp_sampler);
            gl.CreateSamplers(1, &cube_sampler);
            for (auto sampler : {clamp_sampler, cube_sampler}) {
                for (auto wrap : {GL_TEXTURE_WRAP_S, GL_TEXTURE_WRAP_T, GL_TEXTURE_WRAP_R})
                    gl.SamplerParameteri(sampler, wrap, GL_CLAMP_TO_EDGE);
                gl.SamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                gl.SamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                gl.SamplerParameterf(sampler, GL_TEXTURE_MAX_ANISOTROPY, 1);
                gl.SamplerParameterf(sampler, GL_TEXTURE_LOD_BIAS, 0);
                gl.SamplerParameterf(sampler, GL_TEXTURE_MIN_LOD, -1000);
                gl.SamplerParameterf(sampler, GL_TEXTURE_MAX_LOD, 1000);
            }
            if (sampling == LightingCubeSampling::measured_coordinates)
                probe.emplace(make_lighting_coordinate_probe(*context));
            context->check("Material lighting program/normal24/samplers");
        } catch (...) {
            release();
            throw;
        }
    }
    void release() noexcept {
        try {
            const auto &api = context->api();
            probe.reset();
            gl.DeleteBuffers(1, &vb);
            gl.DeleteBuffers(1, &ib);
            gl.DeleteVertexArrays(1, &vao);
            gl.DeleteSamplers(1, &clamp_sampler);
            gl.DeleteSamplers(1, &cube_sampler);
            if (program) {
                GLint current = 0;
                api.GetIntegerv(GL_CURRENT_PROGRAM, &current);
                if (static_cast<GLuint>(current) == program)
                    gl.UseProgram(0);
                gl.DeleteProgram(program);
            }
            program = vao = vb = ib = clamp_sampler = cube_sampler = 0;
        } catch (...) {
            std::terminate();
        }
    }
    ~Impl() { release(); }
};
GlMaterialLightingRenderer::GlMaterialLightingRenderer(std::shared_ptr<GlContext> context,
                                                       const materials::LightingPatchMesh &mesh,
                                                       bool specular, LightingCubeSampling sampling) {
    require(bool(context), "Lighting renderer requires owned context");
    require(sampling == LightingCubeSampling::ordinary ||
                sampling == LightingCubeSampling::integer_no_coordinates ||
                sampling == LightingCubeSampling::measured_coordinates,
            "Invalid lighting cube sampling mode");
    require(mesh.vertices().size() == static_cast<std::size_t>(mesh.columns() * mesh.rows() * 24) &&
                mesh.indices().size() ==
                    static_cast<std::size_t>((mesh.columns() - 1) * (mesh.rows() - 1) * 6),
            "Moved-from lighting mesh");
    impl_ = std::make_unique<Impl>(std::move(context), mesh, specular, sampling);
}
GlMaterialLightingRenderer::~GlMaterialLightingRenderer() = default;
void GlMaterialLightingRenderer::draw(GLuint normal, GLuint albedo, GLuint shadow, GLuint spec,
                                      const GlTexture &sun, const GlTexture &shade, GLuint color_correct,
                                      int width, int height, const std::array<float, 3> &half) {
    require(width > 0 && height > 0 &&
                std::all_of(half.begin(), half.end(), [](float v) { return std::isfinite(v); }),
            "Invalid lighting dimensions or half vector");
    auto &s = *impl_;
    const auto &api = s.context->api();
    require(sun.belongs_to(*s.context) && shade.belongs_to(*s.context),
            "Lighting cubes belong to another context or were moved");
    for (auto texture : {normal, albedo, shadow})
        detail::validate_sampled_2d(*s.context, s.gl, texture);
    if (s.specular)
        detail::validate_sampled_2d(*s.context, s.gl, spec);
    detail::validate_sampled_texture(*s.context, s.gl, color_correct, GL_TEXTURE_3D);
    for (auto texture : {sun.id(), shade.id()}) {
        detail::validate_sampled_texture(*s.context, s.gl, texture, GL_TEXTURE_CUBE_MAP);
        GLint format = 0;
        api.GetTextureLevelParameteriv(texture, 0, GL_TEXTURE_INTERNAL_FORMAT, &format);
        require(format == 0x8c4d || format == 0x8c4e || format == 0x8c4f,
                "Lighting sampler2 requires authored sRGB BC cube");
        if (s.sampling != LightingCubeSampling::ordinary) {
            GLint size = 0, levels = 0, base = 0, maximum = 0;
            api.GetTextureLevelParameteriv(texture, 0, GL_TEXTURE_WIDTH, &size);
            api.GetTextureParameteriv(texture, GL_TEXTURE_IMMUTABLE_LEVELS, &levels);
            api.GetTextureParameteriv(texture, GL_TEXTURE_BASE_LEVEL, &base);
            api.GetTextureParameteriv(texture, GL_TEXTURE_MAX_LEVEL, &maximum);
            require(size == 128 && levels == 8 && base == 0 && maximum == 7 && format == 0x8c4d,
                    "Measured lighting filter requires original 128/8 BC1 profile");
        }
    }
    s.context->check("Before lighting bindings");
    const std::array<float, 2> center{1.f / width, 1.f / height};
    s.gl.ProgramUniform2fv(s.program, s.center, 1, center.data());
    s.gl.ProgramUniform1i(s.program, s.tap_filter, s.sampling != LightingCubeSampling::ordinary);
    s.gl.ProgramUniform1i(s.program, s.tap_coordinates,
                          s.sampling == LightingCubeSampling::measured_coordinates);
    if (s.specular)
        s.gl.ProgramUniform3f(s.program, s.half, half[0], half[1], half[2]);
    for (auto [slot, texture] : std::array<std::pair<GLuint, GLuint>, 6>{
             {{0, normal}, {1, albedo}, {2, shadow}, {4, sun.id()}, {5, shade.id()}, {8, color_correct}}}) {
        api.BindTextureUnit(slot, texture);
        s.gl.BindSampler(slot, slot == 4 || slot == 5 ? s.cube_sampler : s.clamp_sampler);
    }
    if (s.specular) {
        api.BindTextureUnit(3, spec);
        s.gl.BindSampler(3, s.clamp_sampler);
    }
    if (s.probe) {
        s.probe->bind(6);
        s.gl.BindSampler(6, s.cube_sampler);
    }
    s.gl.UseProgram(s.program);
    s.gl.BindVertexArray(s.vao);
    const auto seamless = s.gl.IsEnabled(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    s.gl.Enable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    try {
        s.gl.DrawElements(GL_TRIANGLES, s.count, GL_UNSIGNED_SHORT, nullptr);
        s.context->check("Original key4B lighting draw");
    } catch (...) {
        if (!seamless)
            s.gl.Disable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
        throw;
    }
    if (!seamless)
        s.gl.Disable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    s.context->check("Restore lighting seamless state");
}
} // namespace civ5::graphics
