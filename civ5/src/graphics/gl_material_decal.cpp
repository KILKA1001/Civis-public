#include "civ5/graphics/gl_material_decal.hpp"
#include "civ5/graphics/gl_terrain_decal.hpp"
#include "civ5/graphics/gl_render_api.hpp"
#include "civ5/materials/material_assets.hpp"
#include "gl_native_clip.hpp"
#include "gl_sampled_input.hpp"
#include <algorithm>
#include <cmath>
#include <exception>
#include <stdexcept>

namespace civ5::graphics {
GlTerrainDecalTextures::GlTerrainDecalTextures(std::shared_ptr<GlContext> context,
    std::shared_ptr<const terrain::TerrainDecalAssets> assets)
    : context_(std::move(context)), assets_(std::move(assets)) {
    if (!context_ || !assets_ || assets_->textures().size() >= 65535)
        throw std::invalid_argument("Decal textures need a context and bounded authored source");
    textures_.resize(assets_->textures().size() + 1);
}
const GlTexture &GlTerrainDecalTextures::resource(std::uint16_t id, DdsSamplingColor color) {
    if (id >= textures_.size() || (color != DdsSamplingColor::linear && color != DdsSamplingColor::srgb))
        throw std::invalid_argument("Unknown decal resource or sampler decode");
    auto &texture = textures_[id][color == DdsSamplingColor::srgb];
    if (!texture) {
        if (id == 0)
            texture = std::make_unique<GlTexture>(GlTexture::rgba8(*context_, materials::material_resource_zero(), color));
        else
            texture = std::make_unique<GlTexture>(*context_, assets_->textures()[id - 1].data, color);
    }
    return *texture;
}
namespace {
void require(bool condition, const char *why) {
    if (!condition)
        throw std::invalid_argument(why);
}

std::string decal_vertex_shader(bool native_clip = false) {
    // 05D98A10: lrp r0.xy,v0,c5.zwzw,c5 means v0*max+(1-v0)*min.
    // Add 1, then c0/c1/c2/c3 row-major accumulation. v1 receives the
    // retained float mesh's UV stream at byte +20 through this adapter.
    return std::string("#version 460 core\n") + (native_clip ? "#define DECAL_NATIVE_CLIP\n" : "") + R"(
layout(location=0) in vec3 aPosition;
layout(location=1) in vec3 aTexcoord;
uniform vec4 uTransform[4];
    uniform vec4 uTextureScales;
    uniform vec4 uBounds;
    uniform float uZDistance;
uniform vec2 uPixelCenter;
out vec2 vUv;
void main() {
    precise vec2 q=aPosition.xy*uBounds.zw+uBounds.xy*(vec2(1.0)-aPosition.xy);
    q+=vec2(1.0);
    precise vec4 p=q.x*uTransform[0]+q.y*uTransform[1]+uZDistance*uTransform[2]+uTransform[3];
#ifdef DECAL_NATIVE_CLIP
    gl_Position=vec4(p.x,-p.y,p.z,p.w);
#else
    gl_Position=vec4(vec2(p.x,-p.y)+uPixelCenter,p.z*2.0-p.w,p.w);
#endif
    vUv=aTexcoord.xy*uTextureScales.xy+uTextureScales.zw;
})";
}

std::string decal_fragment_shader(bool write_specular) {
    return std::string(R"(#version 460 core
layout(binding=0) uniform sampler2D uAlbedo;
layout(binding=1) uniform sampler2D uHeight;
)" ) + (write_specular ? "layout(binding=2) uniform sampler2D uSpecular;\n" : "") +
           R"(in vec2 vUv;
layout(location=0) out vec4 outAlbedo;
layout(location=1) out vec4 outHeight;
)" + (write_specular ? "layout(location=2) out vec4 outSpecular;\n" : "") +
           R"(
void main() {
    vec4 heightSample=texture(uHeight,vUv);
    precise float selector=heightSample.y*2.0-0.999899983;
    precise vec4 positive=heightSample.xxxx;
    positive.w=heightSample.y*2.0-1.0;
    precise vec4 negative=vec4(1.0-heightSample.x,1.0-heightSample.x,
                               1.0-heightSample.x,1.0-heightSample.y*2.0);
    outAlbedo=texture(uAlbedo,vUv);
    outHeight=selector>=0.0?positive:negative;
)" + (write_specular ? "    outSpecular=texture(uSpecular,vUv);\n" : "") + R"(}
)";
}
GLuint create_decal_program(const GlRenderApi &gl, bool write_specular, bool native_clip) {
    GLuint program = 0;
    try {
            program = gl.CreateProgram();
            const auto vertex = decal_vertex_shader(native_clip);
            const auto fragment = decal_fragment_shader(write_specular);
            for (auto [kind, source] : {std::pair{GL_VERTEX_SHADER, std::string_view(vertex)},
                                        std::pair{GL_FRAGMENT_SHADER, std::string_view(fragment)}}) {
                const GLuint shader = gl.CreateShader(kind);
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
            GLint linked = 0;
            gl.GetProgramiv(program, GL_LINK_STATUS, &linked);
            if (!linked) {
                char log[8192]{};
                gl.GetProgramInfoLog(program, sizeof(log), nullptr, log);
                throw std::runtime_error(log);
            }
        return program;
    } catch (...) { if (program) gl.DeleteProgram(program); throw; }
}
} // namespace

struct GlMaterialDecalRenderer::Impl {
    std::shared_ptr<GlContext> context;
    materials::MaterialPatchMesh mesh;
    GlRenderApi gl;
    GLuint program = 0, vao = 0, vb = 0, ib = 0, sampler = 0;
    GLint transform = -1, texture_scales = -1, bounds = -1, z_distance = -1, pixel_center = -1;
    GLsizei index_count = 0;
    bool write_specular = false;

    Impl(std::shared_ptr<GlContext> c, const materials::MaterialPatchMesh &mesh, bool specular)
        : context(std::move(c)), mesh(mesh), gl(*context), index_count(static_cast<GLsizei>(mesh.indices().size())),
          write_specular(specular) {
        try {
            require(context != nullptr, "Decal renderer requires owned context");
            context->check("Before decal renderer initialization");
            program = create_decal_program(gl,write_specular,false);
            transform = gl.GetUniformLocation(program, "uTransform[0]");
            texture_scales = gl.GetUniformLocation(program, "uTextureScales");
            bounds = gl.GetUniformLocation(program, "uBounds");
            z_distance = gl.GetUniformLocation(program, "uZDistance");
            pixel_center = gl.GetUniformLocation(program, "uPixelCenter");
            require(transform >= 0 && texture_scales >= 0 && bounds >= 0 && z_distance >= 0 &&
                        pixel_center >= 0,
                    "Missing decal constants");

            gl.CreateVertexArrays(1, &vao);
            gl.CreateBuffers(1, &vb);
            gl.CreateBuffers(1, &ib);
            gl.NamedBufferData(vb, mesh.vertices().size_bytes(), mesh.vertices().data(), GL_STATIC_DRAW);
            gl.NamedBufferData(ib, mesh.indices().size_bytes(), mesh.indices().data(), GL_STATIC_DRAW);
            gl.VertexArrayVertexBuffer(vao, 0, vb, 0, 48);
            gl.VertexArrayElementBuffer(vao, ib);
            for (GLuint location = 0; location < 2; ++location) {
                gl.EnableVertexArrayAttrib(vao, location);
                gl.VertexArrayAttribBinding(vao, location, 0);
                gl.VertexArrayAttribFormat(vao, location, 3, GL_FLOAT, GL_FALSE,
                                           location == 0 ? 0 : 20);
            }
            gl.CreateSamplers(1, &sampler);
            gl.SamplerParameteri(sampler, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            gl.SamplerParameteri(sampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            gl.SamplerParameteri(sampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            gl.SamplerParameteri(sampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            gl.SamplerParameterf(sampler, GL_TEXTURE_LOD_BIAS, 0.f);
            gl.SamplerParameterf(sampler, GL_TEXTURE_MIN_LOD, -1000.f);
            gl.SamplerParameterf(sampler, GL_TEXTURE_MAX_LOD, 1000.f);
            context->check("Decal program/geometry/sampler initialized");
        } catch (...) {
            release();
            throw;
        }
    }

    void release() noexcept {
        try {
            const auto &api = context->api();
            GLint current = 0;
            api.GetIntegerv(GL_CURRENT_PROGRAM, &current);
            if (static_cast<GLuint>(current) == program)
                gl.UseProgram(0);
            gl.DeleteSamplers(1, &sampler);
            gl.DeleteVertexArrays(1, &vao);
            gl.DeleteBuffers(1, &vb);
            gl.DeleteBuffers(1, &ib);
            if (program)
                gl.DeleteProgram(program);
            sampler = vao = vb = ib = program = 0;
        } catch (...) {
            std::terminate();
        }
    }
    ~Impl() { release(); }

    void draw(const GlMaterialDecalFrame &frame, GLuint albedo, GLuint height, GLuint specular,
              std::array<std::int32_t, 2> target_size) {
        require(std::all_of(frame.transform.begin(), frame.transform.end(), [](float x) {
                    return std::isfinite(x);
                }) &&
                    std::all_of(frame.texture_scales.begin(), frame.texture_scales.end(),
                                [](float x) { return std::isfinite(x); }) &&
                    std::all_of(frame.bounds.begin(), frame.bounds.end(),
                                [](float x) { return std::isfinite(x); }) &&
                    std::isfinite(frame.z_distance),
                "Nonfinite decal constants");
        const auto &api = context->api();
        require(albedo && height && api.IsTexture(albedo) && api.IsTexture(height),
                "Decal draw requires albedo and height textures");
        require(!write_specular || (specular && api.IsTexture(specular)),
                "Specular decal draw requires a third texture");
        require(target_size[0] > 0 && target_size[1] > 0, "Decal draw requires target dimensions");
        gl.ProgramUniform4fv(program, transform, 4, frame.transform.data());
        gl.ProgramUniform4fv(program, texture_scales, 1, frame.texture_scales.data());
        gl.ProgramUniform4fv(program, bounds, 1, frame.bounds.data());
        gl.ProgramUniform1f(program, z_distance, frame.z_distance);
        const std::array<float, 2> center{1.f / target_size[0], 1.f / target_size[1]};
        gl.ProgramUniform2fv(program, pixel_center, 1, center.data());
        api.BindTextureUnit(0, albedo);
        api.BindTextureUnit(1, height);
        gl.BindSampler(0, sampler);
        gl.BindSampler(1, sampler);
        if (write_specular) {
            api.BindTextureUnit(2, specular);
            gl.BindSampler(2, sampler);
        }
        gl.UseProgram(program);
        gl.BindVertexArray(vao);
        gl.DrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_SHORT, nullptr);
        context->check("Decal cache draw");
    }
};

GlMaterialDecalRenderer::GlMaterialDecalRenderer(std::shared_ptr<GlContext> context,
                                                 const materials::MaterialPatchMesh &mesh,
                                                 bool write_specular)
{
    require(context != nullptr, "Decal renderer requires owned context");
    impl_ = std::make_unique<Impl>(std::move(context), mesh, write_specular);
}
GlMaterialDecalRenderer::~GlMaterialDecalRenderer() = default;
void GlMaterialDecalRenderer::rebuild_context(std::shared_ptr<GlContext> context) {
    require(context != nullptr, "Decal context recovery needs a live replacement context");
    require(context != impl_->context, "Decal context recovery requires a different OpenGL context");
    auto replacement = std::make_unique<Impl>(context, impl_->mesh, impl_->write_specular);
    impl_.swap(replacement);
}
void GlMaterialDecalRenderer::draw(const GlMaterialDecalFrame &frame, GLuint albedo, GLuint height,
                                   GLuint specular, std::array<std::int32_t, 2> target_size) {
    impl_->draw(frame, albedo, height, specular, target_size);
}

struct GlTerrainDecalRenderer::Impl {
    using Vertex = std::array<std::uint16_t,4>;
    struct Buffer { GLuint name = 0; std::vector<Vertex> vertices; };
    struct Textures { std::array<GLuint,2> ids; };
    std::shared_ptr<GlContext> context;
    GlRenderApi gl;
    GLuint program = 0, vao = 0, indices = 0, sampler = 0;
    std::array<GLint,4> uniforms{};
    std::vector<Buffer> buffers;
    explicit Impl(std::shared_ptr<GlContext> c) : context(std::move(c)), gl(*context) {
        try {
            context->check("Before packed decal initialization");
            program = create_decal_program(gl,false,true);
            uniforms = {gl.GetUniformLocation(program,"uTransform[0]"),
                        gl.GetUniformLocation(program,"uTextureScales"),
                        gl.GetUniformLocation(program,"uBounds"),
                        gl.GetUniformLocation(program,"uZDistance")};
            for (auto u : uniforms) require(u >= 0,"Missing packed decal constant");
            std::vector<std::uint16_t> identity(65535);
            for (std::size_t i=0;i<identity.size();++i) identity[i]=static_cast<std::uint16_t>(i);
            gl.CreateBuffers(1,&indices);
            gl.NamedBufferData(indices,identity.size()*2,identity.data(),GL_STATIC_DRAW);
            gl.CreateVertexArrays(1,&vao);
            gl.VertexArrayElementBuffer(vao,indices);
            for (GLuint i=0;i<2;++i) {
                gl.EnableVertexArrayAttrib(vao,i); gl.VertexArrayAttribBinding(vao,i,0);
                gl.VertexArrayAttribFormat(vao,i,2,GL_HALF_FLOAT,GL_FALSE,i*4);
            }
            gl.CreateSamplers(1,&sampler);
            for (auto wrap : {GL_TEXTURE_WRAP_S,GL_TEXTURE_WRAP_T,GL_TEXTURE_WRAP_R})
                gl.SamplerParameteri(sampler,wrap,GL_REPEAT);
            gl.SamplerParameteri(sampler,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
            gl.SamplerParameteri(sampler,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
            gl.SamplerParameterf(sampler,GL_TEXTURE_MAX_ANISOTROPY,1);
            gl.SamplerParameterf(sampler,GL_TEXTURE_LOD_BIAS,0);
            gl.SamplerParameterf(sampler,GL_TEXTURE_MIN_LOD,-1000);
            gl.SamplerParameterf(sampler,GL_TEXTURE_MAX_LOD,1000);
            context->check("Packed decal declaration/identity indices/samplers");
        } catch (...) { release(); throw; }
    }
    void release() noexcept {
        try {
            const auto &api=context->api(); GLint current=0;api.GetIntegerv(GL_CURRENT_PROGRAM,&current);
            if (static_cast<GLuint>(current)==program) gl.UseProgram(0);
            for (auto &b:buffers) gl.DeleteBuffers(1,&b.name);
            gl.DeleteBuffers(1,&indices);gl.DeleteVertexArrays(1,&vao);gl.DeleteSamplers(1,&sampler);
            if (program) gl.DeleteProgram(program);
            buffers.clear();program=indices=vao=sampler=0;
        } catch (...) { std::terminate(); }
    }
    ~Impl() { release(); }
    const Buffer &buffer(std::uint32_t id) const {
        require(id && id<=buffers.size(),"Unknown packed decal buffer handle");return buffers[id-1];
    }
    std::uint32_t create() {
        require(buffers.size()<materials::no_terrain_decal_slot,"Packed decal buffer handle capacity");
        buffers.emplace_back();
        try {
            context->api();gl.CreateBuffers(1,&buffers.back().name);
            gl.NamedBufferData(buffers.back().name,65535*8,nullptr,GL_DYNAMIC_DRAW);
            context->check("Create packed decal buffer");
        } catch (...) {gl.DeleteBuffers(1,&buffers.back().name);buffers.pop_back();throw;}
        return static_cast<std::uint32_t>(buffers.size());
    }
    void upload(std::uint32_t id,std::span<const Vertex> vertices) {
        buffer(id);require(vertices.size()<=65535,"Packed decal upload capacity");
        std::vector<Vertex> retained(vertices.begin(),vertices.end());
        context->api();
        if (!vertices.empty()) gl.NamedBufferSubData(buffers[id-1].name,0,vertices.size_bytes(),vertices.data());
        context->check("Upload packed decal prefix");buffers[id-1].vertices.swap(retained);
    }
    std::vector<Textures> prepare(const materials::TerrainDecalDrawList &,
        const TerrainDecalTextureResolver &, bool current_framebuffer, std::span<const GLuint> targets) const;
};
GlTerrainDecalRenderer::GlTerrainDecalRenderer(std::shared_ptr<GlContext> context) {
    require(bool(context),"Packed decal renderer requires a GL context");
    impl_=std::make_unique<Impl>(std::move(context));
}
GlTerrainDecalRenderer::~GlTerrainDecalRenderer()=default;
std::uint32_t GlTerrainDecalRenderer::create() {return impl_->create();}
void GlTerrainDecalRenderer::upload(std::uint32_t id,std::span<const std::array<std::uint16_t,4>> vertices) {
    impl_->upload(id,vertices);
}
void GlTerrainDecalRenderer::rebuild_context(std::shared_ptr<GlContext> context) {
    require(bool(context) && context!=impl_->context,"Packed decal recovery needs a different live context");
    auto replacement=std::make_unique<Impl>(context);
    for (const auto &b:impl_->buffers) {const auto id=replacement->create();replacement->upload(id,b.vertices);}
    impl_.swap(replacement);
}
std::vector<std::array<std::uint16_t,4>> GlTerrainDecalRenderer::read_vertices(std::uint32_t id) const {
    auto &s=*impl_;const auto &b=s.buffer(id);s.context->api();
    std::vector<Impl::Vertex> result(b.vertices.size());
    if (!result.empty()) s.gl.GetNamedBufferSubData(b.name,0,result.size()*8,result.data());
    s.context->check("Read uploaded decal prefix");return result;
}
std::vector<GlTerrainDecalRenderer::Impl::Textures> GlTerrainDecalRenderer::Impl::prepare(
    const materials::TerrainDecalDrawList &list, const TerrainDecalTextureResolver &resolve,
    bool current_framebuffer, std::span<const GLuint> targets) const {
    const auto &s=*this;
    require(list.capacity_exhausted ? list.draws.empty() : list.draws.size()==list.selected,
            "Malformed packed decal draw list");
    if (list.draws.empty()) return {};
    require(bool(resolve),"Packed decal draw needs a texture resolver");
    std::vector<Textures> textures; textures.reserve(list.draws.size());
    for (const auto &d:list.draws) {
        require(d.index_buffer==1,"Foreign packed decal index buffer");
        const auto &b=s.buffer(d.vertex_buffer);
        require(d.triangles && std::uint64_t(d.first_vertex)+std::uint64_t(d.triangles)*3<=b.vertices.size(),
                "Packed decal draw outside uploaded prefix");
        for (float f:d.constants) require(std::isfinite(f),"Nonfinite packed decal constants");
        Textures selected{};
        for (unsigned i=0;i<2;++i) {
            const auto &t=resolve(i ? d.heightmap : d.albedo,i ? DdsSamplingColor::linear : DdsSamplingColor::srgb);
            require(t.belongs_to(*s.context),"Foreign-context decal texture");
            detail::validate_sampled_2d(*s.context,s.gl,t.id(),current_framebuffer);
            for (auto target:targets)
                require(target!=t.id(),"Packed decal input/output feedback");
            const auto f=t.plan().internal_format;
            const bool srgb=f==GL_SRGB8_ALPHA8 || f==0x8c4d || f==0x8c4e || f==0x8c4f;
            require(srgb==(i==0),"Decal sampler color decode mismatch");
            selected.ids[i]=t.id();
        }
        textures.push_back(selected);
    }
    return textures;
}
void GlTerrainDecalRenderer::validate(const materials::TerrainDecalDrawList &list,
    const TerrainDecalTextureResolver &resolve, const GlContext &context,
    std::span<const GLuint> targets) const {
    require(impl_->context.get()==&context,"Foreign-context decal backend");
    impl_->prepare(list,resolve,false,targets);
}
void GlTerrainDecalRenderer::draw(const materials::TerrainDecalDrawList &list,
                                 const TerrainDecalTextureResolver &resolve) {
    auto &s=*impl_;
    const auto textures=s.prepare(list,resolve,true,{});
    if (list.draws.empty()) return;
    s.context->api();s.gl.UseProgram(s.program);s.gl.BindVertexArray(s.vao);
    s.gl.BindSampler(0,s.sampler);s.gl.BindSampler(1,s.sampler);
    const detail::NativeClipScope clip(*s.context,s.gl);
    for (std::size_t i=0;i<list.draws.size();++i) {
        const auto &d=list.draws[i];const auto &b=s.buffer(d.vertex_buffer);
        s.gl.VertexArrayVertexBuffer(s.vao,0,b.name,0,8);
        s.gl.ProgramUniform4fv(s.program,s.uniforms[0],4,d.constants.data());
        s.gl.ProgramUniform4fv(s.program,s.uniforms[1],1,d.constants.data()+16);
        s.gl.ProgramUniform4fv(s.program,s.uniforms[2],1,d.constants.data()+20);
        s.gl.ProgramUniform1f(s.program,s.uniforms[3],d.constants[24]);
        s.context->api().BindTextureUnit(0,textures[i].ids[0]);s.context->api().BindTextureUnit(1,textures[i].ids[1]);
        // Original 8E2350 uses descriptor+10 as StartIndex, with BaseVertex=0.
        s.gl.DrawElements(GL_TRIANGLES,static_cast<GLsizei>(d.triangles*3),GL_UNSIGNED_SHORT,
                          reinterpret_cast<const void *>(std::uintptr_t(d.first_vertex)*2));
    }
    s.context->check("Packed terrain decal draw list");
}
} // namespace civ5::graphics
