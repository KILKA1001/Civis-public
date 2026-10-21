#include "bnw_warrior_skin_diagnostic.hpp"
#include "civ5/graphics/gl_render_api.hpp"
#include <array>
#include <fstream>
#include <iostream>
#include <memory>

namespace {
void require(bool value,const char *message) {
    if(!value) throw std::runtime_error(message);
}

GLuint program(civ5::graphics::GlRenderApi &gl) {
    const char *vertex=
        "#version 460 core\n"
        "layout(location=0) in vec3 sourcePosition;"
        "layout(location=1) in vec3 sourceNormal;"
        "layout(location=2) in vec2 sourceUV;"
        "uniform float panel; out vec2 uv;"
        "void main(){vec2 p=vec2((sourcePosition.x+sourcePosition.y*0.45+21.)/160.,"
        "(sourcePosition.z-59.)/125.);"
        "gl_Position=vec4(p*0.55+vec2(panel,0.),0.,1.); uv=sourceUV;}";
    const char *fragment=
        "#version 460 core\n"
        "in vec2 uv; uniform sampler2D authoredDiffuse; out vec4 pixel;"
        "void main(){pixel=texture(authoredDiffuse,uv);}";
    const auto result=gl.CreateProgram();
    for(const auto &[stage,source]:{
            std::pair{GL_VERTEX_SHADER,vertex},
            std::pair{GL_FRAGMENT_SHADER,fragment}}) {
        const auto shader=gl.CreateShader(stage);
        gl.ShaderSource(shader,1,&source,nullptr);
        gl.CompileShader(shader);
        GLint ok=0;
        gl.GetShaderiv(shader,GL_COMPILE_STATUS,&ok);
        if(!ok) {
            char log[4096]{};
            gl.GetShaderInfoLog(shader,sizeof(log),nullptr,log);
            gl.DeleteShader(shader);
            throw std::runtime_error(std::string("Warrior diagnostic shader: ")+log);
        }
        gl.AttachShader(result,shader);
        gl.DeleteShader(shader);
    }
    gl.LinkProgram(result);
    GLint ok=0;
    gl.GetProgramiv(result,GL_LINK_STATUS,&ok);
    if(!ok) {
        char log[4096]{};
        gl.GetProgramInfoLog(result,sizeof(log),nullptr,log);
        throw std::runtime_error(std::string("Warrior diagnostic program: ")+log);
    }
    return result;
}
}

int main(int argc,char **argv) {
    try {
        if(argc!=5) throw std::runtime_error(
            "Usage: gl_bnw_warrior_skin_probe CIV5_ASSET_ROOT LOCAL_SDK_CACHE OUTPUT_PPM SDK_SAMPLE_INDEX");
        const std::filesystem::path root=argv[1],cache=argv[2],output=argv[3];
        const int sample=std::stoi(argv[4]); double bind_error=0;
        constexpr std::array<std::size_t,3> vertices{829,863,796};
        constexpr std::array<std::size_t,3> triangles{654,674,638};
        constexpr std::array<float,3> panels{-0.66f,0.f,0.66f};
        auto context=std::make_shared<civ5::graphics::GlContext>(512,512,false);
        civ5::graphics::GlRenderApi gl(*context);
        require(civ5::graphics::probe_bnw_gandhi_warrior_art(root,*context)==48,
                "Warrior FXS/DDS source chain differs");
        const civ5::assets::AssetEnvironment environment(root);
        const auto opened=environment.open_fpk_archive("Resource/DX9/UnitsTextures.fpk");
        const auto *textures=std::get_if<std::shared_ptr<civ5::assets::FpkSource>>(&opened);
        require(textures!=nullptr,"Warrior authored texture archive unavailable");
        auto color=civ5::graphics::GlTexture::render_target(
            *context,512,512,1,civ5::graphics::RenderTextureFormat::rgba8);
        GLuint fbo=0;
        gl.CreateFramebuffers(1,&fbo);
        gl.NamedFramebufferTexture(fbo,GL_COLOR_ATTACHMENT0,color.id(),0);
        const GLenum attachment=GL_COLOR_ATTACHMENT0;
        gl.NamedFramebufferDrawBuffers(fbo,1,&attachment);
        require(gl.CheckNamedFramebufferStatus(fbo,GL_FRAMEBUFFER)==
                GL_FRAMEBUFFER_COMPLETE,"Warrior diagnostic target incomplete");
        gl.BindFramebuffer(GL_FRAMEBUFFER,fbo);
        gl.Viewport(0,0,512,512);
        gl.Disable(GL_DEPTH_TEST);
        gl.Disable(GL_CULL_FACE);
        gl.Disable(GL_BLEND);
        gl.ClearColor(0.03f,0.04f,0.06f,1.f);
        gl.Clear(GL_COLOR_BUFFER_BIT);
        const auto shader=program(gl);
        gl.UseProgram(shader);
        const auto panel_uniform=gl.GetUniformLocation(shader,"panel");
        const auto diffuse_uniform=gl.GetUniformLocation(shader,"authoredDiffuse");
        require(panel_uniform>=0 && diffuse_uniform>=0,"Warrior diagnostic uniforms missing");
        gl.ProgramUniform1i(shader,diffuse_uniform,0);
        std::uint64_t vertex_total=0,triangle_total=0;
        for(std::size_t i=0;i<civ5::graphics::bnw_gandhi_warrior_art.size();++i) {
            const auto &spec=civ5::graphics::bnw_gandhi_warrior_art[i];
            auto mesh=civ5::graphics::load_bnw_warrior_source_mesh(
                cache,spec,vertices[i],triangles[i]);
            const auto skin=civ5::graphics::load_bnw_warrior_skin_diagnostic(cache,spec,vertices[i]);
            bind_error=std::max(bind_error,skin.bind_matrix_error());
            mesh.vertex_pnuv=skin.pose(mesh,sample);
            const auto texture_bytes=civ5::graphics::warrior_archive_asset(*textures,
                                                                          spec.diffuse_name);
            civ5::graphics::require_bnw_warrior_asset_sha(
                civ5::runtime::identify_bytes(texture_bytes).sha256,
                spec.diffuse_sha256);
            const auto dds=civ5::assets::DdsData::parse(texture_bytes);
            require(dds.format()==civ5::assets::DdsFormat::dxt5,
                    "Warrior authored diffuse format differs");
            civ5::graphics::GlTexture diffuse(*context,dds,
                                               civ5::graphics::DdsSamplingColor::linear);
            GLuint vao=0,vbo=0,ibo=0;
            gl.CreateVertexArrays(1,&vao);
            gl.CreateBuffers(1,&vbo);
            gl.CreateBuffers(1,&ibo);
            gl.NamedBufferData(vbo,static_cast<GLsizeiptr>(mesh.vertex_pnuv.size()*sizeof(float)),
                               mesh.vertex_pnuv.data(),GL_STATIC_DRAW);
            gl.NamedBufferData(ibo,static_cast<GLsizeiptr>(mesh.indices.size()*sizeof(std::uint32_t)),
                               mesh.indices.data(),GL_STATIC_DRAW);
            gl.VertexArrayVertexBuffer(vao,0,vbo,0,8*sizeof(float));
            gl.VertexArrayElementBuffer(vao,ibo);
            for(GLuint attribute=0;attribute<3;++attribute) {
                gl.EnableVertexArrayAttrib(vao,attribute);
                gl.VertexArrayAttribBinding(vao,attribute,0);
            }
            gl.VertexArrayAttribFormat(vao,0,3,GL_FLOAT,GL_FALSE,0);
            gl.VertexArrayAttribFormat(vao,1,3,GL_FLOAT,GL_FALSE,3*sizeof(float));
            gl.VertexArrayAttribFormat(vao,2,2,GL_FLOAT,GL_FALSE,6*sizeof(float));
            diffuse.bind(0);
            gl.ProgramUniform1f(shader,panel_uniform,panels[i]);
            gl.BindVertexArray(vao);
            gl.DrawElements(GL_TRIANGLES,static_cast<GLsizei>(mesh.indices.size()),
                            GL_UNSIGNED_INT,nullptr);
            vertex_total+=mesh.vertex_pnuv.size()/8;
            triangle_total+=mesh.indices.size()/3;
            gl.BindVertexArray(0);
            gl.DeleteBuffers(1,&ibo);
            gl.DeleteBuffers(1,&vbo);
            gl.DeleteVertexArrays(1,&vao);
        }
        context->check("Draw local SDK Warrior source geometry");
        auto pixels=color.read_level_raw(0,0);
        require(pixels.size()==512u*512u*4u,"Warrior diagnostic readback extent differs");
        std::size_t changed=0;
        for(std::size_t i=0;i<pixels.size();i+=4)
            if(pixels[i]>16 || pixels[i+1]>18 || pixels[i+2]>24) ++changed;
        require(changed>1000,"Warrior SDK source geometry produced no visible pixels");
        std::ofstream image(output,std::ios::binary);
        require(static_cast<bool>(image),"Warrior diagnostic image output unavailable");
        image << "P6\n512 512\n255\n";
        for(int y=511;y>=0;--y)
            for(int x=0;x<512;++x) {
                const auto at=(std::size_t(y)*512+x)*4;
                image.write(reinterpret_cast<const char *>(pixels.data()+at),3);
            }
        require(static_cast<bool>(image),"Warrior diagnostic image write failed");
        gl.UseProgram(0);
        gl.BindFramebuffer(GL_FRAMEBUFFER,0);
        gl.DeleteProgram(shader);
        gl.DeleteFramebuffers(1,&fbo);
        std::cout << "{\"status\":\"PASS\",\"sdkSourceVertices\":" << vertex_total << ","
                     "\"sdkSourceTriangles\":" << triangle_total << ","
                     "\"visiblePixels\":" << changed << ","
                     "\"diagnosticSampleIndex\":" << sample << ",\"bindMatrixError\":" << bind_error << ","
                     "\"diagnosticSkinPose\":true,\"modelDrawable\":false,"
                     "\"fullGraphicsParity\":false}\n";
        return 0;
    } catch(const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
